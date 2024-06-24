/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <asm/types.h>
#include <string.h>
#include <errno.h>
#include "test_dma_api.h"
#include "kernel-headers/linux/dma-direction.h"
#include "kernel-headers/linux/dma-heap.h"

#define PAGE_ALIGN(x)		 (((x) + 4095) & ~4095)

int dmabuf_heap_open(char *name)
{
	int ret, fd;
	char buf[256];

	ret = snprintf(buf, 256, "%s/%s", DEVPATH, name);
	if (ret < 0) {
		printf("snprintf failed!\n");
		return ret;
	}

	fd = open(buf, O_RDWR);
	/* if errno == 2, which means there is no heap_codecmm, using ion replace
	 */
	if (fd < 0) {
		if (errno != 2)
			printf("open %s failed, %s!\n", buf, strerror(errno));
	}
	return fd;
}

static int dmabuf_heap_alloc_fdflags(int fd, size_t len, unsigned int fd_flags,
					 unsigned int heap_flags, int *dmabuf_fd)
{
	struct dma_heap_allocation_data data = {
		.len = len,
		.fd = 0,
		.fd_flags = fd_flags,
		.heap_flags = heap_flags,
	};
	int ret;

	if (!dmabuf_fd)
		return -EINVAL;

	ret = ioctl(fd, DMA_HEAP_IOCTL_ALLOC, &data);
	if (ret < 0)
		return ret;
	*dmabuf_fd = (int)data.fd;
	return ret;
}

int dmabuf_heap_alloc(int fd, size_t len, unsigned int flags,
				 int *dmabuf_fd)
{
	return dmabuf_heap_alloc_fdflags(fd, len, O_RDWR | O_CLOEXEC, flags,
					 dmabuf_fd);
}

void dmabuf_sync(int fd, int start_stop)
{
	struct dma_buf_sync sync = {
		.flags = start_stop | DMA_BUF_SYNC_RW,
	};
	int ret;

	ret = ioctl(fd, DMA_BUF_IOCTL_SYNC, &sync);
	if (ret)
		printf("sync failed %d\n", errno);
}

void dmabuf_heap_close(int heap_fd)
{
	if (heap_fd >= 0)
		close(heap_fd);
}
