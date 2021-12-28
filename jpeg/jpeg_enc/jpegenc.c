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
static int64_t total_enc_time=0;
static int total_enc_frame=0;
struct jpegenc_request_s {
	u32 src;
	u32 encoder_width;
	u32 encoder_height;
	u32 framesize;
	u32 jpeg_quality;
	u32 QuantTable_id;
	u32 flush_flag;
	u32 block_mode;
	enum jpegenc_mem_type_e type;
	enum jpegenc_frame_fmt_e input_fmt;
	enum jpegenc_frame_fmt_e output_fmt;

	u32 y_off;
	u32 u_off;
	u32 v_off;

	u32 y_stride;
	u32 u_stride;
	u32 v_stride;

	u32 h_stride;
};

//#include <media/stagefright/foundation/ALooper.h>
//using namespace android;
#define ENCODER_PATH       "/dev/jpegenc"

#define JPEGENC_IOC_MAGIC  'J'

#define JPEGENC_IOC_GET_DEVINFO         _IOW(JPEGENC_IOC_MAGIC, 0xf0, uint32_t)
#define JPEGENC_IOC_GET_BUFFINFO        _IOW(JPEGENC_IOC_MAGIC, 0x00, uint32_t)
#define JPEGENC_IOC_CONFIG_INIT         _IOW(JPEGENC_IOC_MAGIC, 0x01, uint32_t)
#define JPEGENC_IOC_NEW_CMD             _IOW(JPEGENC_IOC_MAGIC, 0x02, uint32_t)
#define JPEGENC_IOC_GET_STAGE           _IOW(JPEGENC_IOC_MAGIC, 0x03, uint32_t)
#define JPEGENC_IOC_GET_OUTPUT_SIZE     _IOW(JPEGENC_IOC_MAGIC, 0x04, uint32_t)
#define JPEGENC_IOC_SET_EXT_QUANT_TABLE _IOW(JPEGENC_IOC_MAGIC, 0x05, uint32_t)
#define JPEGENC_IOC_QUERY_DMA_SUPPORT   _IOW(JPEGENC_IOC_MAGIC, 0x06, uint32_t)
#define JPEGENC_IOC_CONFIG_DMA_INPUT    _IOW(JPEGENC_IOC_MAGIC, 0x07, int32_t)
#define JPEGENC_IOC_RELEASE_DMA_INPUT   _IOW(JPEGENC_IOC_MAGIC, 0x08, uint32_t)
#define JPEGENC_IOC_NEW_CMD2            _IOW(JPEGENC_IOC_MAGIC, 0x09, struct jpegenc_request_s)

#define JPEGENC_ENCODER_IDLE        0
#define JPEGENC_ENCODER_START       1
/* #define JPEGENC_ENCODER_SOS_HEADER          2 */
#define JPEGENC_ENCODER_MCU         3
#define JPEGENC_ENCODER_DONE        4

//#define simulation
#define ALIGNE_64(a) (((a + 63) >> 6) << 6)
static int64_t file_read_time_total = 0;
typedef unsigned char uint8_t;
typedef unsigned int uint32_t;

#define ENCODE_DONE_TIMEOUT 5000

//#define DEBUG_TIME
#ifdef DEBUG_TIME
static struct timeval start_test, end_test;
static unsigned long long g_total_time;
#define PER_FRAME 100
static unsigned int g_frame_cnt, g_avg_time;
#endif

#define JPEGENC_FLUSH_FLAG_INPUT			0x1
#define JPEGENC_FLUSH_FLAG_OUTPUT		0x2

void RGB_to_YUV(int w, int h, unsigned char* yuv, unsigned char* rgb) {
	unsigned char val_r = rgb[0], val_g = rgb[1], val_b = rgb[2];
	int y, u = 0, v = 0;
	y = 0.30078 * val_r + 0.5859 * val_g + 0.11328 * val_b;
	if (y > 255) {
		y = 255;
	} else if (y < 0) {
		y = 0;
	}
	yuv[0] = y;
	if (0 == h % 2 && 0 == w % 2) {
		u = -0.11328 * val_r - 0.33984 * val_g + 0.51179 * val_b + 128;
		if (u > 255) {
			u = 255;
		} else if (u < 0) {
			u = 0;
		}
		yuv[1] = u;
		v = 0.51179 * val_r - 0.429688 * val_g - 0.08203 * val_b + 128;
		if (v > 255) {
			v = 255;
		} else if (v < 0) {
			v = 0;
		}
		yuv[2] = v;
	}
}

long hw_encode_init(int timeout) {
	int ret = -1;
	uint32_t buff_info[7];

	hw_jpegenc_t* hw_info = (hw_jpegenc_t*) malloc(sizeof(hw_jpegenc_t));

	if (!hw_info)
		return (long)NULL;

	hw_info->jpeg_size = 0;

	hw_info->dev_fd = open(ENCODER_PATH, O_RDWR);

	if (hw_info->dev_fd < 0) {
		printf("hw_encode open device fail, %d:%s\n", errno, strerror(errno));
		goto EXIT_INIT;
	}

	memset(buff_info, 0, sizeof(buff_info));

	ret = ioctl(hw_info->dev_fd, JPEGENC_IOC_GET_BUFFINFO, &buff_info[0]);

	if ((ret) || (buff_info[0] == 0)) {
		printf("hw_encode -- no buffer information\n");
		goto EXIT_INIT;
	}

	hw_info->mmap_buff.addr = (unsigned char*) mmap(0, buff_info[0], PROT_READ | PROT_WRITE, MAP_SHARED, hw_info->dev_fd, 0);

	if (hw_info->mmap_buff.addr == MAP_FAILED) {
		printf("hw_encode Error: failed to map framebuffer device to memory.\n");
		goto EXIT_INIT;
	} else
		printf("mapped address is %p\n", hw_info->mmap_buff.addr);

	ret = ioctl(hw_info->dev_fd, JPEGENC_IOC_CONFIG_INIT, NULL);

	hw_info->mmap_buff.size = buff_info[0];
	hw_info->input_buf.addr = hw_info->mmap_buff.addr + buff_info[1];
	hw_info->input_buf.size = buff_info[2];
	hw_info->assit_buf.addr = hw_info->mmap_buff.addr + buff_info[3];
	hw_info->assit_buf.size = buff_info[4];
	hw_info->output_buf.addr = hw_info->mmap_buff.addr + buff_info[5];
	hw_info->output_buf.size = buff_info[6];
	hw_info->timeout = timeout;

	if (hw_info->timeout == 0)
		hw_info->timeout = -1;

	printf("hw_info->mmap_buff.size, 0x%x, hw_info->input_buf.addr:0x%p\n", hw_info->mmap_buff.size, hw_info->input_buf.addr);
	printf("hw_info->assit_buf.addr, 0x%p, hw_info->output_buf.addr:0x%p\n", hw_info->assit_buf.addr, hw_info->output_buf.addr);

	return (long)hw_info;

EXIT_INIT:
	if (hw_info->mmap_buff.addr) {
		munmap(hw_info->mmap_buff.addr, hw_info->mmap_buff.size);
		hw_info->mmap_buff.addr = NULL;
	}

	close(hw_info->dev_fd);

	hw_info->dev_fd = -1;

	return (long)NULL;
}

int hw_encode_uninit(jpegenc_handle_t handle) {
	int ret = 0;
	uint32_t buff_info[5];

	hw_jpegenc_t* hw_info = (hw_jpegenc_t*) handle;

	if (!hw_info)
		return -1;

	if (hw_info->mmap_buff.addr) {
		munmap(hw_info->mmap_buff.addr, hw_info->mmap_buff.size);
		hw_info->mmap_buff.addr = NULL;
	}

    if (ioctl(hw_info->dev_fd, JPEGENC_IOC_RELEASE_DMA_INPUT, NULL) < 0)
        printf("JPEGENC_IOC_RELEASE_DMA_INPUT fail %d %s\n", errno, strerror(errno));

    if (hw_info->dev_fd >= 0)
        close(hw_info->dev_fd);

    hw_info->dev_fd = -1;

	return ret;
}

static void rgb32to24(const uint8_t *src, uint8_t *dst, int src_size) {
	int i;
	int num_pixels = src_size >> 2;
	for (i = 0; i < num_pixels; i++) {
		*dst++ = *src++;
		*dst++ = *src++;
		*dst++ = *src++;
		src++;
	}
}

static int RGBA32_To_RGB24Canvas(hw_jpegenc_t* hw_info) {
	unsigned char* src = NULL;
	unsigned char* dst = NULL;
	int bytes_per_line = hw_info->width * 4;
	unsigned int canvas_w = (((hw_info->width * 3) + 31) >> 5) << 5;
	int mb_h = hw_info->height;
	int i;

	src = (unsigned char*) hw_info->src;
	dst = hw_info->input_buf.addr;
	if (canvas_w != (hw_info->width * 3)) {
		for (i = 0; i < mb_h; i++) {
			rgb32to24(src, dst, bytes_per_line);
			dst += canvas_w;
			src += bytes_per_line;
		}
	} else {
		rgb32to24(src, dst, bytes_per_line * mb_h);
	}
	return canvas_w * mb_h;
}

static int RGB24_To_RGB24Canvas(hw_jpegenc_t* hw_info) {
	unsigned char* src = NULL;
	unsigned char* dst = NULL;
	int bytes_per_line = hw_info->width * 3;
	int canvas_w = (((hw_info->width * 3) + 31) >> 5) << 5;
	int mb_h = hw_info->height;
	unsigned int i;

	src = (unsigned char*) hw_info->src;
	dst = hw_info->input_buf.addr;
	if (bytes_per_line != canvas_w) {
		for (i = 0; i < hw_info->height; i++) {
			memcpy(dst, src, bytes_per_line);
			dst += canvas_w;
			src += bytes_per_line;
		}
	} else {
		memcpy(dst, src, bytes_per_line * mb_h);
	}
	return canvas_w * mb_h;
}

static int YUV422_To_Canvas(hw_jpegenc_t* hw_info) {
	unsigned char* src = NULL;
	unsigned char* dst = NULL;
	int bytes_per_line = hw_info->width * 2;
	int canvas_w = (((hw_info->width * 2) + 31) >> 5) << 5;
	int mb_h = hw_info->height;
	unsigned int i;

	src = (unsigned char*) hw_info->src;
	dst = hw_info->input_buf.addr;
	if (bytes_per_line != canvas_w) {
		for (i = 0; i < hw_info->height; i++) {
			memcpy(dst, src, bytes_per_line);
			dst += canvas_w;
			src += bytes_per_line;
		}
	} else {
		memcpy(dst, src, bytes_per_line * mb_h);
	}
	return canvas_w * mb_h;
}

static int encode_poll(int fd, int timeout) {
	struct pollfd poll_fd[1];
	poll_fd[0].fd = fd;
	poll_fd[0].events = POLLIN | POLLERR;
	return poll(poll_fd, 1, timeout);
}

#ifdef simulation
static unsigned copy_to_local(hw_jpegenc_t* hw_info)
{
	int size = 0;
	unsigned char* src = NULL;
	unsigned char* dst = NULL;

	size = hw_info->input_size;
	src = (unsigned char*)hw_info->src;
	dst = hw_info->input_buf.addr;

	memcpy(dst, src, size);

	return size;
}
#else
static unsigned copy_to_local(hw_jpegenc_t* hw_info) {
	unsigned offset = 0;
	int luma_stride = 0, chroma_stride = 0;
	unsigned i = 0;
	unsigned total_size = 0;
	unsigned char* src = NULL;
	unsigned char* dst = NULL;
	int plane_num = 2;

	//unsigned int aligned_height = ((hw_info->height + 15) >> 4) << 4;

	if ((hw_info->in_format == FMT_YUV420) || (hw_info->in_format == FMT_YUV444_PLANE) || (hw_info->in_format == FMT_RGB888_PLANE))
		plane_num = 3;
	else
		plane_num = 2;

	luma_stride = hw_info->y_stride;

//	if (hw_info->in_format == FMT_YUV420) {
//		luma_stride = ((hw_info->width + 63) >> 6) << 6;
//		chroma_stride = luma_stride / 2; //TODO
//	} else {
//		luma_stride = ((hw_info->width + 31) >> 5) << 5;
//		chroma_stride = luma_stride;
//	}

	src = (unsigned char*) hw_info->src;
	dst = hw_info->input_buf.addr;

	if (luma_stride != hw_info->width) {
		printf("copy y line by line\n");

		for (i = 0; i < hw_info->height; i++) {
			memcpy(dst, src, hw_info->width);
			dst += luma_stride;
			src += hw_info->width;
		}
	} else {
		memcpy(dst, src, hw_info->height * hw_info->width);
	}

	if (plane_num == 2) {
		offset = hw_info->h_stride * luma_stride;

		chroma_stride = luma_stride;
		printf("offset=%d\n", offset);

		src = (unsigned char*) (hw_info->src + hw_info->width * hw_info->height);
		dst = (unsigned char*) (hw_info->input_buf.addr + offset);

		if (chroma_stride != hw_info->width) {
			for (i = 0; i < hw_info->height / 2; i++) {
				memcpy(dst, src, hw_info->width);
				dst += chroma_stride;
				src += hw_info->width;
			}
		} else {
			memcpy(dst, src, hw_info->width * hw_info->height / 2);
		}
	} else if (plane_num == 3) {
		if (hw_info->in_format == FMT_YUV420) {
			chroma_stride = luma_stride / 2;
			offset = hw_info->h_stride * luma_stride;

			hw_info->u_stride = hw_info->y_stride / 2;
			hw_info->v_stride = hw_info->y_stride / 2;

			printf("uoff=%d\n", offset);
			int i=0;
			src = (unsigned char*) (hw_info->src + hw_info->width * hw_info->height);
			dst = (unsigned char*) (hw_info->input_buf.addr + offset);

			for (i = 0; i < hw_info->height / 2; i++) {
				memcpy(	dst, src, hw_info->width / 2);
				src += hw_info->width / 2;
				dst += chroma_stride;
			}

			offset = hw_info->h_stride * luma_stride + (chroma_stride * hw_info->h_stride / 2);
			printf("voff=%d\n", offset);

			src = (unsigned char*) (hw_info->src + hw_info->width * hw_info->height * 5 / 4);
			dst = (unsigned char*) (hw_info->input_buf.addr + offset);

			for (i = 0; i < hw_info->height / 2; i++) {
				memcpy(	dst, src, hw_info->width / 2);
				src += hw_info->width / 2;
				dst += chroma_stride;
			}
		} else if (hw_info->in_format == FMT_YUV444_PLANE) {
			chroma_stride = luma_stride;
			offset = hw_info->h_stride * luma_stride;

			hw_info->u_stride = hw_info->y_stride;
			hw_info->v_stride = hw_info->y_stride;

			printf("uoff=%d\n", offset);
			int i=0;
			src = (unsigned char*) (hw_info->src + hw_info->width * hw_info->height);
			dst = (unsigned char*) (hw_info->input_buf.addr + offset);

			for (i = 0; i < hw_info->height; i++) {
				memcpy(	dst, src, hw_info->width);
				src += hw_info->width;
				dst += chroma_stride;
			}

			offset = hw_info->h_stride * luma_stride * 2;
			printf("voff=%d\n", offset);

			src = (unsigned char*) (hw_info->src + hw_info->width * hw_info->height * 2);
			dst = (unsigned char*) (hw_info->input_buf.addr + offset);

			for (i = 0; i < hw_info->height; i++) {
				memcpy(	dst, src, hw_info->width);
				src += hw_info->width;
				dst += chroma_stride;
			}
		}
	}
	printf("luma_stride=%d, h_stride=%d, hw_info->bpp=%d\n",
			luma_stride, hw_info->h_stride, hw_info->bpp);
	total_size = luma_stride * hw_info->h_stride * hw_info->bpp / 8;
	return total_size;
}
#endif
static int64_t GetNowUs() {
	struct timeval tv;
	gettimeofday(&tv, NULL);

	return (int64_t)(tv.tv_sec) * 1000000 + (int64_t)(tv.tv_usec);
}
static size_t start_encoder(hw_jpegenc_t* hw_info) {
	int i;
	int bpp;
	uint32_t size = 0;
	uint32_t cmd[34], status;
	uint32_t in_format = hw_info->in_format;

#ifdef simulation
	hw_info->src_size = copy_to_local(hw_info);
	cmd[5] = hw_info->src_size;
#else
	//printf("hw_info->type=%d\n", hw_info->type);
	if (hw_info->type == JPEGENC_LOCAL_BUFF) {
		if ((hw_info->in_format == FMT_RGB888) || (hw_info->in_format == FMT_YUV444_SINGLE)) {
			hw_info->src_size = RGB24_To_RGB24Canvas(hw_info);
		} else if (hw_info->in_format == FMT_RGBA8888) {
			hw_info->src_size = RGBA32_To_RGB24Canvas(hw_info);
			hw_info->in_format = FMT_RGB888;
		} else if (hw_info->in_format == FMT_YUV422_SINGLE) {
			//printf("[%s:%d]\n", __FUNCTION__, __LINE__);
			hw_info->src_size = YUV422_To_Canvas(hw_info);
		} else {
			//printf("[%s:%d]\n", __FUNCTION__, __LINE__);
			hw_info->src_size = copy_to_local(hw_info);
		}
		cmd[5] = hw_info->src_size;
		//printf("cmd[5]=%d\n", cmd[5]);

	} else if (hw_info->type == JPEGENC_DMA_BUFF) {
		cmd[5] = hw_info->width * hw_info->height * hw_info->bpp / 8;
		int dma_io_status = ioctl(hw_info->dev_fd, JPEGENC_IOC_CONFIG_DMA_INPUT, &(hw_info->dma_fd));
		if (dma_io_status < 0) {
			//printf("JPEGENC_IOC_CONFIG_DMA_INPUT failed\n");
			return -1;
		}
	}
#endif
	//printf("[%s:%d]\n", __FUNCTION__, __LINE__);
	cmd[0] = hw_info->type;     //input buffer type
	cmd[1] = hw_info->in_format;
	cmd[2] = hw_info->out_format;
	cmd[3] = hw_info->width;
	cmd[4] = hw_info->height;
	cmd[6] = (hw_info->type == JPEGENC_LOCAL_BUFF) ? 0 : hw_info->canvas;
	cmd[7] = hw_info->quality;
	cmd[8] = hw_info->qtbl_id;
	cmd[9] = (hw_info->type == JPEGENC_LOCAL_BUFF) ? (JPEGENC_FLUSH_FLAG_INPUT | JPEGENC_FLUSH_FLAG_OUTPUT) : JPEGENC_FLUSH_FLAG_OUTPUT;
	cmd[10] = hw_info->block_mode;

	cmd[11] = hw_info->y_stride;
	cmd[12] = hw_info->y_stride;
	cmd[13] = hw_info->y_stride;
	cmd[14] = hw_info->h_stride;

	struct jpegenc_request_s request;
	request.type = hw_info->type;
	request.input_fmt = hw_info->in_format;
	request.output_fmt = hw_info->out_format;
	request.encoder_width = hw_info->width;
	request.encoder_height = hw_info->height;
	request.src = (hw_info->type == JPEGENC_LOCAL_BUFF) ? 0 : hw_info->canvas;

	request.jpeg_quality = hw_info->quality;
	request.QuantTable_id = hw_info->qtbl_id;
	request.flush_flag = (hw_info->type == JPEGENC_LOCAL_BUFF) ? (JPEGENC_FLUSH_FLAG_INPUT | JPEGENC_FLUSH_FLAG_OUTPUT) : JPEGENC_FLUSH_FLAG_OUTPUT;
	request.block_mode = hw_info->block_mode;

	request.y_off = hw_info->y_off;
	request.u_off = hw_info->u_off;
	request.v_off = hw_info->v_off;
	request.y_stride = hw_info->y_stride;
	request.u_stride = hw_info->u_stride;
	request.v_stride = hw_info->v_stride;
	request.h_stride = hw_info->h_stride;
	request.framesize = cmd[5];

	ioctl(hw_info->dev_fd, JPEGENC_IOC_NEW_CMD, cmd);
	//ioctl(hw_info->dev_fd, JPEGENC_IOC_NEW_CMD2, (unsigned long)(&request));

	if (encode_poll(hw_info->dev_fd, hw_info->timeout) <= 0) {
		printf("hw_encode: poll fail\n");
		return 0;
	}

	ioctl(hw_info->dev_fd, JPEGENC_IOC_GET_STAGE, &status);

	if (status == JPEGENC_ENCODER_DONE) {
		cmd[0] = 0;
		cmd[1] = 0;

		ioctl(hw_info->dev_fd, JPEGENC_IOC_GET_OUTPUT_SIZE, &cmd);

		if ((cmd[0] > 0) && (cmd[1] > 0)) {
		    //printf("hw_encode: done head size:%d, size: %d\n", cmd[0], cmd[1]);
			memcpy(hw_info->dst, hw_info->assit_buf.addr, cmd[0]);
			memcpy(hw_info->dst + cmd[0], hw_info->output_buf.addr, cmd[1]);

			size = cmd[0] + cmd[1];
		} else {
			printf("hw_encode: output buffer size error: bitstream buffer size: %d, jpeg size: %d, output buffer size: %d\n",
					hw_info->output_buf.size, cmd[1], hw_info->dst_size);
			size = 0;
		}
	}
	//printf("[%s:%d]\n", __FUNCTION__, __LINE__);
	return size;
}

int hw_encode(jpegenc_handle_t handle, uint8_t *src, uint8_t *dst, enum jpegenc_mem_type_e mem_type, int dma_fd, uint32_t width, uint32_t height, int w_stride, int h_stride, int quality,
		uint8_t format, uint8_t oformat, uint32_t* datalen) {
	int ret;

#ifdef DEBUG_TIME
	uint32_t total_time = 0;
	gettimeofday(&start_test, NULL);
#endif

	if (handle == (long)NULL) {
		printf("[%s:%d] handle is NULL!\n", __FUNCTION__, __LINE__);
		return -1;
	}

	if ((src == NULL) || (dst == NULL)) {
		if (src == NULL)
			printf("[%s:%d]param err!, src is NULL!\n", __FUNCTION__, __LINE__);
		if (dst == NULL)
			printf("[%s:%d]param err!, dst is NULL!\n", __FUNCTION__, __LINE__);
		return -1;
	}

	hw_jpegenc_t* hw_info = (hw_jpegenc_t*)handle;

	hw_info->qtbl_id = 0;
	hw_info->quality = quality;
	hw_info->in_format = (enum jpegenc_frame_fmt_e) format;
	hw_info->width = width;
	hw_info->height = height;
	hw_info->y_stride = w_stride;
	hw_info->u_stride = w_stride;
	hw_info->v_stride = w_stride;
	hw_info->h_stride = h_stride;
	hw_info->canvas = 0;
	hw_info->src = src;
	hw_info->dst = dst;
	//printf("hw_info->dst=%p\n", dst);
	hw_info->dst_size = *datalen;

	switch (hw_info->in_format) {
	case FMT_NV21:
		hw_info->bpp = 12;
		break;
	case FMT_RGB888:
		hw_info->bpp = 24;
		break;
	case FMT_YUV422_SINGLE:
		hw_info->bpp = 16;
		break;
	case FMT_YUV444_PLANE:
	case FMT_RGB888_PLANE:
	case FMT_YUV444_SINGLE:
		hw_info->bpp = 24;
		break;
	default:
		hw_info->bpp = 12;
	}

	hw_info->type = mem_type;
	hw_info->dma_fd = dma_fd;
	//printf("hw_info->dma_fd=%d\n", dma_fd);
	//hw_info->out_format = FMT_YUV420; //FMT_YUV422_SINGLE
	hw_info->out_format = (enum jpegenc_frame_fmt_e) oformat;

	hw_info->jpeg_size = start_encoder(hw_info);

	*datalen = hw_info->jpeg_size;

#ifdef DEBUG_TIME
	gettimeofday(&end_test, NULL);
	total_time = (end_test.tv_sec - start_test.tv_sec) * 1000000 + end_test.tv_usec - start_test.tv_usec;
	g_total_time += total_time;
	g_frame_cnt++;
	if (g_frame_cnt >= PER_FRAME)
	{
		g_avg_time = g_total_time / PER_FRAME;
		g_frame_cnt = 0;
		g_total_time = 0;
		printf("jpeg hw_encode: average per frame need %d us\n", g_avg_time);
	}
	//printf("hw_encode: need time: %d us, jpeg size:%zu\n", total_time, hw_info->jpeg_size);
#endif

	return 0;
}
