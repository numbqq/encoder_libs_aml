/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in below
 * which is part of this source code package.
 *
 * Description:
 */

// Copyright (C) 2019 Amlogic, Inc. All rights reserved.
//
// All information contained herein is Amlogic confidential.
//
// This software is provided to you pursuant to Software License
// Agreement (SLA) with Amlogic Inc ("Amlogic"). This software may be
// used only in accordance with the terms of this agreement.
//
// Redistribution and use in source and binary forms, with or without
// modification is strictly prohibited without prior written permission
// from Amlogic.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#define LOG_TAG "jenc_test"
#define LOG_LINE() printf("[%s:%d]\n", __FUNCTION__, __LINE__)
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/poll.h>
#include <stdint.h>

#include "jpegenc_api.h"
#include "jpegenc.h"

jpegenc_handle_t jpegenc_init() {
	jpegenc_handle_t handle = hw_encode_init(0);

	return handle;

exit:
	if (handle != (long)NULL)
		free((void *)handle);

	return (jpegenc_handle_t) NULL;
}

int jpegenc_encode(jpegenc_handle_t handle, int width, int height, int w_stride, int h_stride, int quality,
		enum jpegenc_frame_fmt_e iformat, enum jpegenc_frame_fmt_e oformat, enum jpegenc_mem_type_e mem_type, int dma_fd, uint8_t *in_buf,
		uint8_t *out_buf) {
	int format;
	int i;
	uint32_t datalen = 0;
	uint8_t* temp_ptr;
	int k, g, bytes;

	unsigned char c_table_r[] = { 0xff, 0, 0 };
	unsigned char c_table_g[] = { 0, 0xff, 0 };
	unsigned char c_table_b[] = { 0, 0, 0xff };
	unsigned char c_table_x[] = { 0, 0, 0 };

	//hw_jpegenc_t* info = (hw_jpegenc_t*) handle;

	if (iformat == 0)
		format = FMT_YUV422_SINGLE;
	else if (iformat == 1)
		format = FMT_YUV444_SINGLE;
	else if (iformat == 2)
		format = FMT_NV21;
	else if (iformat == 3)
		format = FMT_NV12;
	else if (iformat == 4)
		format = FMT_YUV420;
	else if (iformat == 5)
		format = FMT_YUV444_PLANE;

	if (oformat == 0)
		oformat = FMT_YUV420;     //4
	else if (oformat == 1)
		oformat = FMT_YUV422_SINGLE;     //0
	else if (oformat == 2)
		oformat = FMT_YUV444_SINGLE;     //1

	//info->block_mode = 0;     //block_mode;

	do {
		if ((format == FMT_RGB888) || (format == FMT_RGBA8888)) {
			temp_ptr = in_buf;

			bytes = (format == FMT_RGB888) ? 3 : 4;

			for (k = 0; k < height; k++) {
				for (g = 0; g < width; g++) {
					if (g < width / 3) {
						*temp_ptr++ = c_table_r[0];
						*temp_ptr++ = c_table_r[1];
						*temp_ptr++ = c_table_r[2];
					} else if (g < width * 2 / 3) {
						*temp_ptr++ = c_table_g[0];
						*temp_ptr++ = c_table_g[1];
						*temp_ptr++ = c_table_g[2];
					} else {
						*temp_ptr++ = c_table_b[0];
						*temp_ptr++ = c_table_b[1];
						*temp_ptr++ = c_table_b[2];
					}

					if (format == FMT_RGBA8888)
						*temp_ptr++ = 0xff;
				}
			}
		}

		hw_encode(handle, in_buf, out_buf, mem_type, dma_fd, width, height, w_stride, h_stride, quality, format, oformat, &datalen);

		if (datalen == 0) {
			printf("hw_encode error!\n");
			return 0;
		}

		return datalen;
	} while (0);
}

int jpegenc_destroy(jpegenc_handle_t handle) {
	hw_encode_uninit(handle);

	free((void *)handle);
	return 0;
}



