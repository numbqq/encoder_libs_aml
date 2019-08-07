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

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include "./test_dma_api.h"
#include "../amvenc_lib/include/vp_multi_codec_1_0.h"

#define WIDTH 3840
#define HEIGHT 2160
#define FRAMERATE 60

#define INIT_RETRY 100

int main(int argc, const char *argv[])
{
	int width, height, gop, framerate, bitrate, num;
	int ret = 0;
	int outfd = -1;
	FILE *fp = NULL;
	int datalen = 0;
	int retry_cnt = 0;
	vl_img_format_t fmt = IMG_FMT_NONE;

	unsigned char *vaddr = NULL;
	vl_codec_handle_t handle_enc = 0;
	vl_encode_info_t encode_info;
	vl_buffer_info_t ret_buf;
	vl_codec_id_t codec_id;
	encoding_metadata_t encoding_metadata;

	int tmp_idr;
	int fd = -1;
	qp_param_t qp_tbl;
	int qp_mode = 0;
	int buf_type = 0;
	int num_planes = 1;
	struct usr_ctx_s ctx;

	if (argc < 12)
	{
		printf("Amlogic Encoder API \n");
		printf(" usage: aml_enc_test "
		       "[srcfile][outfile][width][height][gop][framerate][bitrate][num][fmt]["
		       "buf type][num_planes][codec_id]\n");
		printf("  options  \t:\n");
		printf("  srcfile  \t: yuv data url in your root fs\n");
		printf("  outfile  \t: stream url in your root fs\n");
		printf("  width    \t: width\n");
		printf("  height   \t: height\n");
		printf("  gop      \t: I frame refresh interval\n");
		printf("  framerate\t: framerate \n");
		printf("  bitrate  \t: bit rate \n");
		printf("  num      \t: encode frame count \n");
		printf("  fmt      \t: encode input fmt 1 : nv12, 2 : nv21, 3 : yuv420p(yv12/yu12)\n");
		printf("  buf_type \t: 0:vmalloc, 3:dma buffer\n");
		printf("  num_planes \t: used for dma buffer case. 2 : nv12/nv21, 3 : yuv420p(yv12/yu12)\n");
		printf("  codec_id \t: 4 : h.264, 5 : h.265\n");
		return -1;
	} else
	{
		printf("%s\n", argv[1]);
		printf("%s\n", argv[2]);
	}

	width = atoi(argv[3]);
	if ((width < 1) || (width > WIDTH))
	{
		printf("invalid width \n");
		return -1;
	}

	height = atoi(argv[4]);
	if ((height < 1) || (height > HEIGHT))
	{
		printf("invalid height \n");
		return -1;
	}

	gop = atoi(argv[5]);
	framerate = atoi(argv[6]);
	bitrate = atoi(argv[7]);
	num = atoi(argv[8]);
	fmt = (vl_img_format_t) atoi(argv[9]);
	buf_type = atoi(argv[10]);
	num_planes = atoi(argv[11]);

	switch (atoi(argv[12]))
	{
	case 0:
		codec_id = CODEC_ID_NONE;
		break;
	case 1:
		codec_id = CODEC_ID_H261;
		break;
	case 2:
		codec_id = CODEC_ID_H261;
		break;
	case 3:
		codec_id = CODEC_ID_H263;
		break;
	case 4:
		codec_id = CODEC_ID_H264;
		break;
	case 5:
		codec_id = CODEC_ID_H265;
		break;
	default:
		break;
	}

	if ((framerate < 0) || (framerate > FRAMERATE))
	{
		printf("invalid framerate %d \n", framerate);
		return -1;
	}
	if (bitrate <= 0)
	{
		printf("invalid bitrate \n");
		return -1;
	}
	if (num < 0)
	{
		printf("invalid num \n");
		return -1;
	}
	if (buf_type == 3 && (num_planes < 1 || num_planes > 3))
	{
		printf("invalid num_planes \n");
		return -1;
	}
	printf("src_url is\t: %s ;\n", argv[1]);
	printf("out_url is\t: %s ;\n", argv[2]);
	printf("width   is\t: %d ;\n", width);
	printf("height  is\t: %d ;\n", height);
	printf("gop     is\t: %d ;\n", gop);
	printf("frmrate is\t: %d ;\n", framerate);
	printf("bitrate is\t: %d ;\n", bitrate);
	printf("frm_num is\t: %d ;\n", num);
	printf("fmt     is\t: %d ;\n", fmt);
	printf("buf_type is\t: %d ;\n", buf_type);
	printf("num_planes is\t: %d ;\n", num_planes);

	unsigned int framesize = width * height * 3 / 2;
	unsigned ysize = width * height;
	unsigned usize = width * height / 4;
	unsigned vsize = width * height / 4;
	unsigned uvsize = width * height / 2;

	unsigned int outbuffer_len = 1024 * 1024 * sizeof(char);
	unsigned char *outbuffer = (unsigned char *) malloc(outbuffer_len);
	unsigned char *inputBuffer = NULL;
	unsigned char *input[3] = { NULL };
	vl_buffer_info_t inbuf_info;
	vl_dma_info_t *dma_info = NULL;

	memset(&inbuf_info, 0, sizeof(vl_buffer_info_t));
	inbuf_info.buf_type = (vl_buffer_type_t) buf_type;

	qp_tbl.qp_min = 0;
	qp_tbl.qp_max = 51;
	qp_tbl.qp_I_base = 30;
	qp_tbl.qp_I_min = 0;
	qp_tbl.qp_I_max = 51;
	qp_tbl.qp_P_base = 30;
	qp_tbl.qp_P_min = 0;
	qp_tbl.qp_P_max = 51;

	encode_info.width = width;
	encode_info.height = height;
	encode_info.bit_rate = bitrate;
	encode_info.frame_rate = framerate;
	encode_info.gop = gop;
	encode_info.img_format = fmt;

	if (inbuf_info.buf_type == DMA_TYPE)
	{
		dma_info = &(inbuf_info.buf_info.dma_info);
		dma_info->num_planes = num_planes;
		dma_info->shared_fd[0] = -1;	// dma buf fd
		dma_info->shared_fd[1] = -1;
		dma_info->shared_fd[2] = -1;
		ret = create_ctx(&ctx);
		if (ret < 0)
		{
			printf("gdc create fail ret=%d\n", ret);
			goto exit;
		}

		if (dma_info->num_planes == 3)
		{
			if (fmt != IMG_FMT_YUV420P)
			{
				printf("error fmt %d\n", fmt);
				goto exit;
			}
			ret = alloc_dma_buffer(&ctx, INPUT_BUFF_TYPE, ysize);
			if (ret < 0)
			{
				printf("alloc fail ret=%d, len=%u\n", ret,
				       ysize);
				goto exit;
			}
			vaddr = (unsigned char *) ctx.i_buff[0];
			if (!vaddr)
			{
				printf("mmap failed,Not enough memory\n");
				goto exit;
			}
			dma_info->shared_fd[0] = ret;
			input[0] = vaddr;

			ret = alloc_dma_buffer(&ctx, INPUT_BUFF_TYPE, usize);
			if (ret < 0)
			{
				printf("alloc fail ret=%d, len=%u\n", ret,
				       usize);
				goto exit;
			}
			vaddr = (unsigned char *) ctx.i_buff[1];
			if (!vaddr)
			{
				printf("mmap failed,Not enough memory\n");
				goto exit;
			}
			dma_info->shared_fd[1] = ret;
			input[1] = vaddr;

			ret = alloc_dma_buffer(&ctx, INPUT_BUFF_TYPE, vsize);
			if (ret < 0)
			{
				printf("alloc fail ret=%d, len=%u\n", ret,
				       vsize);
				goto exit;
			}
			vaddr = (unsigned char *) ctx.i_buff[2];
			if (!vaddr)
			{
				printf("mmap failed,Not enough memory\n");
				goto exit;
			}
			dma_info->shared_fd[2] = ret;
			input[2] = vaddr;
		} else if (dma_info->num_planes == 2)
		{
			ret = alloc_dma_buffer(&ctx, INPUT_BUFF_TYPE, ysize);
			if (ret < 0)
			{
				printf("alloc fail ret=%d, len=%u\n", ret,
				       ysize);
				goto exit;
			}
			vaddr = (unsigned char *) ctx.i_buff[0];
			if (!vaddr)
			{
				printf("mmap failed,Not enough memory\n");
				goto exit;
			}
			dma_info->shared_fd[0] = ret;
			input[0] = vaddr;
			if (fmt != IMG_FMT_NV12 && fmt != IMG_FMT_NV21)
			{
				printf("error fmt %d\n", fmt);
				goto exit;
			}
			ret = alloc_dma_buffer(&ctx, INPUT_BUFF_TYPE, uvsize);
			if (ret < 0)
			{
				printf("alloc fail ret=%d, len=%u\n", ret,
				       uvsize);
				goto exit;
			}
			vaddr = (unsigned char *) ctx.i_buff[1];
			if (!vaddr)
			{
				printf("mmap failed,Not enough memory\n");
				goto exit;
			}
			dma_info->shared_fd[1] = ret;
			input[1] = vaddr;
		} else if (dma_info->num_planes == 1)
		{
			ret =
			    alloc_dma_buffer(&ctx, INPUT_BUFF_TYPE, framesize);
			if (ret < 0)
			{
				printf("alloc fail ret=%d, len=%u\n", ret,
				       framesize);
				goto exit;
			}
			vaddr = (unsigned char *) ctx.i_buff[0];
			if (!vaddr)
			{
				printf("mmap failed,Not enough memory\n");
				goto exit;
			}
			dma_info->shared_fd[0] = ret;
			input[0] = vaddr;
		}

		printf("in[0] %d, in[1] %d, in[2] %d\n", dma_info->shared_fd[0],
		       dma_info->shared_fd[1], dma_info->shared_fd[2]);
		printf("input[0] %p, input[1] %p,input[2] %p\n", input[0],
		       input[1], input[2]);
	} else
	{
		inputBuffer = (unsigned char *) malloc(framesize);
		inbuf_info.buf_info.in_ptr[0] = (ulong) (inputBuffer);
		inbuf_info.buf_info.in_ptr[1] =
		    (ulong) (inputBuffer + width * height);
		inbuf_info.buf_info.in_ptr[2] =
		    (ulong) (inputBuffer + width * height * 5 / 4);
	}

	fp = fopen((argv[1]), "rb");
	if (fp == NULL)
	{
		printf("open src file error!\n");
		goto exit;
	}

	outfd = open((argv[2]), O_CREAT | O_RDWR | O_TRUNC, 0644);
	if (outfd < 0)
	{
		printf("open dest file error!\n");
		goto exit;
	}

retry:
	handle_enc = vl_multi_encoder_init(codec_id, encode_info, &qp_tbl);
	if (handle_enc == 0) {
		if ( retry_cnt >= INIT_RETRY) {
			printf("Init encoder retry timeout\n");
			goto exit;
		} else
		{
			printf("Init encoder fail retrying \n");
			retry_cnt ++;
			usleep(100*1000);
			goto retry;
		}
	}

	while (num > 0)
	{
		if (inbuf_info.buf_type == DMA_TYPE)	// read data to dma buf vaddr
		{
			if (dma_info->num_planes == 1)
			{
				if (fread(input[0], 1, framesize, fp) !=
				    framesize)
				{
					printf("read input file error!\n");
					goto exit;
				}
			} else if (dma_info->num_planes == 2)
			{
				if (fread(input[0], 1, ysize, fp) != ysize)
				{
					printf("read input file error!\n");
					goto exit;
				}
				if (fread(input[1], 1, uvsize, fp) != uvsize)
				{
					printf("read input file error!\n");
					goto exit;
				}
			} else if (dma_info->num_planes == 3)
			{
				if (fread(input[0], 1, ysize, fp) != ysize)
				{
					printf("read input file error!\n");
					goto exit;
				}
				if (fread(input[1], 1, usize, fp) != usize)
				{
					printf("read input file error!\n");
					goto exit;
				}
				if (fread(input[2], 1, vsize, fp) != vsize)
				{
					printf("read input file error!\n");
					goto exit;
				}
			}
		} else
		{		// read data to vmalloc buf vaddr
			if (fread(inputBuffer, 1, framesize, fp) != framesize)
			{
				printf("read input file error!\n");
				goto exit;
			}
		}

		memset(outbuffer, 0, outbuffer_len);
		if (inbuf_info.buf_type == DMA_TYPE)
		{
			sync_cpu(&ctx);
		}

		encoding_metadata =
		    vl_multi_encoder_encode(handle_enc, FRAME_TYPE_AUTO,
					    outbuffer, &inbuf_info, &ret_buf);

		if (encoding_metadata.is_valid)
		{
			write(outfd, (unsigned char *) outbuffer,
			      encoding_metadata.encoded_data_length_in_bytes);
		} else
		{
			printf("encode error %d!\n",
			       encoding_metadata.encoded_data_length_in_bytes);
		}
		num--;
	}

exit:

	if (handle_enc)
		vl_multi_encoder_destroy(handle_enc);

	if (outfd >= 0)
		close(outfd);

	if (fp)
		fclose(fp);

	if (outbuffer != NULL)
		free(outbuffer);

	if (inbuf_info.buf_type == DMA_TYPE)
	{
		if (dma_info->shared_fd[0] >= 0)
			close(dma_info->shared_fd[0]);

		if (dma_info->shared_fd[1] >= 0)
			close(dma_info->shared_fd[1]);

		if (dma_info->shared_fd[2] >= 0)
			close(dma_info->shared_fd[2]);

		destroy_ctx(&ctx);
	} else
	{
		if (inputBuffer)
			free(inputBuffer);
	}
	printf("Encode End!\n");

	return 0;
}
