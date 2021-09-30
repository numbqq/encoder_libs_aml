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
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include "jpegenc_api.h"
#include "test_dma_api.h"
#include <sys/time.h>
//#include <media/stagefright/foundation/ALooper.h>
//using namespace android;
//#include <malloc.h>
#define LOG_LINE() printf("[%s:%d]\n", __FUNCTION__, __LINE__)
static int64_t GetNowUs() {
	struct timeval tv;
	gettimeofday(&tv, NULL);

	return (int64_t)(tv.tv_sec) * 1000000 + (int64_t)(tv.tv_usec);
}

static unsigned copy_to_dmabuf(unsigned char *vaddr, unsigned char *yuv_buf, int width, int height, int w_stride, int h_stride, int iformat) {
    unsigned offset = 0;
    int luma_stride = 0, chroma_stride = 0;
    int i = 0;
    unsigned total_size = 0;
    unsigned char* src = NULL;
    unsigned char* dst = NULL;
    int plane_num = 2;

    if ((iformat == FMT_YUV420) || (iformat == FMT_YUV444_PLANE) || (iformat == FMT_RGB888_PLANE))
        plane_num = 3;
    else
        plane_num = 2;

    luma_stride = w_stride;

    src = yuv_buf;
    dst = vaddr;

    if (luma_stride != width) {
        for (i = 0; i < height; i++) {
            memcpy(dst, src, width);
            dst += luma_stride;
            src += width;
        }
    } else {
        memcpy(dst, src, height * width);
    }

    if (plane_num == 2) {
        offset = h_stride * luma_stride;

        chroma_stride = luma_stride;
        printf("offset=%d\n", offset);

        src = (unsigned char*) (yuv_buf + width * height);
        dst = (unsigned char*) (vaddr + offset);

        if (chroma_stride != width) {
            for (i = 0; i < height / 2; i++) {
                memcpy(dst, src, width);
                dst += chroma_stride;
                src += width;
            }
        } else {
            memcpy(dst, src, width * height / 2);
        }
    } else if (plane_num == 3) {
        if (iformat == FMT_YUV420) {
            chroma_stride = luma_stride / 2;
            offset = h_stride * luma_stride;

            int i=0;
            src = (unsigned char*) (yuv_buf + width * height);
            dst = (unsigned char*) (vaddr + offset);

            for (i = 0; i < height / 2; i++) {
                memcpy(dst, src, width / 2);
                src += width / 2;
                dst += chroma_stride;
            }

            offset = h_stride * luma_stride + (chroma_stride * h_stride / 2);
            printf("voff=%d\n", offset);

            src = (unsigned char*) (yuv_buf + width * height * 5 / 4);
            dst = (unsigned char*) (vaddr + offset);

            for (i = 0; i < height / 2; i++) {
                memcpy(dst, src, width / 2);
                src += width / 2;
                dst += chroma_stride;
            }
        } else {

        }
    }

    printf("luma_stride=%d, h_stride=%d, hw_info->bpp=%d\n", luma_stride, h_stride, 12);

    total_size = luma_stride * h_stride * 12 / 8;
    return total_size;
}

int main(int argc, const char *argv[])
{
    int width, height ,quality;
    enum jpegenc_frame_fmt_e iformat;
    enum jpegenc_frame_fmt_e oformat;
    struct usr_ctx_s ctx;
    int shared_fd = -1;
	int64_t encoding_time = 0;
	int enc_frame=0;
	int count=0;
    if (argc < 6) {
        printf("Amlogic AVC Encode API \n");
        printf("usage: output [srcfile] [outfile] [width] [height] [quality] [iformat] [oformat] [w_stride] [h_stride] [memtype]\n");
        printf("options :\n");
        printf("         srcfile  : yuv data url in your root fs \n");
        printf("         outfile  : stream url in your root fs \n");
        printf("         width    : width \n");
        printf("         height   : height \n");
        printf("         qulaity  : jpeg quality scale \n");
        printf("         iformat  : 0:422, 1:444, 2:NV21, 3:NV12, 4:420M, 5:444M \n");
        printf("         oformat  : 0:420, 1:422, 2:444 \n");
        printf("         width alignment  : width alignment \n");
        printf("         height alignment : height alignment \n");
        printf("         memory type   : memory type for input data, 0:vmalloc, 3:dma\n");

        return -1;
    } else {
        printf("%s\n", argv[1]);
        printf("%s\n", argv[2]);
    }
    width =  atoi(argv[3]);
    if (width < 1) {
        printf("invalid width \n");
        return -1;
    }
    height = atoi(argv[4]);
    if (height < 1) {
        printf("invalid height \n");
        return -1;
    }
    quality	= atoi(argv[5]);
    iformat	= (enum jpegenc_frame_fmt_e)atoi(argv[6]);
    oformat	= (enum jpegenc_frame_fmt_e)atoi(argv[7]);
    int w_alignment, h_alignment;
    w_alignment	= atoi(argv[8]);
    h_alignment	= atoi(argv[9]);

    enum jpegenc_mem_type_e mem_type = (enum jpegenc_mem_type_e)atoi(argv[10]);
    //int dma_fd = atoi(argv[11]);

    printf("src url: %s \n", argv[1]);
    printf("out url: %s \n", argv[2]);
    printf("width  : %d \n", width);
    printf("height : %d \n", height);
    printf("quality: %d \n", quality);
    printf("iformat: %d \n", iformat);
    printf("oformat: %d \n", oformat);
    printf("width alignment: %d \n", w_alignment);
    printf("height alignment: %d \n", h_alignment);
    printf("memory type: %s\n", mem_type == JPEGENC_LOCAL_BUFF ? "VMALLOC" : mem_type == JPEGENC_DMA_BUFF ? "DMA" : "unsupported mem type");
    int w_stride = (width % w_alignment) == 0 ? width : (((width / w_alignment)+1)*w_alignment);
    int h_stride = (height % h_alignment) == 0 ? height : (((height / h_alignment)+1)*h_alignment);
    printf("align: %d->%d\n", width, w_stride);
    printf("align: %d->%d\n", height, h_stride);

	jpegenc_handle_t handle = jpegenc_init();

    if (handle == (long)NULL) {
        printf("jpegenc_init failed\n");
        return -1;
    }

    FILE *fin = fopen(argv[1], "rb");
    if (!fin) {
        printf("open input file %s failed: %s\n", argv[1], strerror(errno));
        return -1;
    }

    int frame_size = width*height*3/2;
    if (iformat == 1 || iformat == 5) {
    	frame_size = width*height*3;
    } else if (iformat == 0) {
    	frame_size = width*height*2;
    }

    printf("frame_size=%d\n", frame_size);

    uint8_t *yuv_buf = (uint8_t *)malloc(frame_size);
    if (!yuv_buf) {
        printf("alloc buffer for yuv failed\n");
        return -1;
    }

    uint8_t *out_buf = (uint8_t *)malloc(1048576*10);
    if (!out_buf) {
        printf("alloc buffer for output jpeg failed\n");
        return -1;
    }

    if (mem_type == JPEGENC_DMA_BUFF) {
        int ret = create_ctx(&ctx);

        if (ret < 0) {
            printf("gdc create fail ret=%d\n", ret);
            return -1;
        }

        int frame_size = w_stride * h_stride * 3 / 2;
        shared_fd = alloc_dma_buffer(&ctx, INPUT_BUFF_TYPE, frame_size);

        if (shared_fd < 0) {
            printf("alloc fail ret=%d, len=%u\n", ret, frame_size);
            return -1;
        }

        uint8_t *vaddr = (uint8_t *) ctx.i_buff[0];
        if (!vaddr) {
            printf("mmap failed,Not enough memory\n");
            return -1;
        }
    }

    int rd_size;

    rd_size = fread(yuv_buf, 1, frame_size, fin);

    printf("rd_size=%d, frame_size=%d\n", rd_size, frame_size);
    if (rd_size < frame_size) {
        printf("short read on yuv file\n");
        return -1;
    }

    if (mem_type != JPEGENC_LOCAL_BUFF && mem_type != JPEGENC_DMA_BUFF) {
        printf("unsupported mem type\n");
        return -1;
    }

    if (mem_type == JPEGENC_DMA_BUFF) {
        uint8_t *vaddr = (uint8_t *) ctx.i_buff[0];
        if (!vaddr) {
            printf("mmap failed,Not enough memory\n");
            return -1;
        }

        copy_to_dmabuf(vaddr, yuv_buf, width, height, w_stride, h_stride, iformat);
        sync_cpu(&ctx);
    }

    int64_t t_1=GetNowUs();

#if 0
    int datalen = 0;
    while (1)
    {

        datalen = jpegenc_encode(handle, width, height, w_stride, h_stride, quality, iformat, oformat, mem_type, shared_fd, yuv_buf, out_buf);
    }
#else
    int datalen = jpegenc_encode(handle, width, height, w_stride, h_stride, quality, iformat, oformat, mem_type, shared_fd, yuv_buf, out_buf);
#endif
    int64_t t_2=GetNowUs();
    //fprintf(stderr, "jpegenc_encode time: %lld\n", t_2-t_1);

    if (datalen == 0) {
        printf("jpegenc_encode failed\n");
        return -1;
    }
    encoding_time+=t_2-t_1;
    enc_frame++;

    FILE *fout = fopen(argv[2], "wb");
    if (!fout) {
        printf("open file for writting jpeg file failed\n");
        return -1;
    }
    fwrite(out_buf, 1, datalen, fout);
    fclose(fout);

    //fprintf(stderr, "encode time:%lld, fps:%3.1f\n", encoding_time, enc_frame*1.0/(encoding_time/1000000.0));

    jpegenc_destroy(handle);
    if (mem_type == JPEGENC_DMA_BUFF) {
        if (shared_fd >= 0)
            close(shared_fd);
        destroy_ctx(&ctx);
    }

    free(yuv_buf);
    yuv_buf = NULL;

    fclose(fin);



	return 0;
}
