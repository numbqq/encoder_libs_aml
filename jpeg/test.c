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
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "jpegenc_api.h"
#include "test_dma_api.h"

//#include <media/stagefright/foundation/ALooper.h>
//using namespace android;
//#include <malloc.h>
#define LOG_LINE() printf("[%s:%d]\n", __FUNCTION__, __LINE__)
static int64_t GetNowUs() {
    struct timeval tv;
    gettimeofday(&tv, NULL);

    return (int64_t)(tv.tv_sec) * 1000000 + (int64_t)(tv.tv_usec);
}

int main(int argc, const char *argv[])
{
    int outfd = -1;
    FILE *fp = NULL;
    int ret = -1;

    int width, height ,quality;
    jpegenc_frame_fmt_e iformat;
    jpegenc_frame_fmt_e oformat;
    struct usr_ctx_s ctx;
    int shared_fd[3] = {0};
    int64_t encoding_time = 0;
    int enc_frame = 0;
    int framesize  = 0;
    unsigned char *vaddr = NULL;
    unsigned char *input[3] = { NULL };
    jpegenc_frame_info_t frame_info;
    int datalen = 0;
    int num_planes = 1;
    int w_stride = 0;
    int h_stride = 0;
    int w_alignment = 0;
    int h_alignment = 0;
    int num = 1;

    unsigned int output_size  = 0;
    unsigned char *output_buffer = NULL;
    unsigned char *inputBuffer = NULL;
    unsigned int ysize;
    unsigned int usize;
    unsigned int vsize;
    unsigned int uvsize;
    jpegenc_result_e result = ENC_FAILED;
    jpegenc_handle_t handle = 0;
    int End_status = 0;

    if (argc < 11) {
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
        printf("         num      \t: encode frame count \n");
        printf("         num_planes \t: used for dma buffer case. 2 : nv12/nv21, 3 : yuv420p(yv12/yu12)\n");

        return -1;
    } else {
        printf("%s\n", argv[1]);
        printf("%s\n", argv[2]);
    }
    width =  atoi(argv[3]);
    if (width < 1) {
        printf("Encode Failed! invalid width \n");
        return -1;
    }
    height = atoi(argv[4]);
    if (height < 1) {
        printf("Encode Failed! invalid height \n");
        return -1;
    }
    quality	= atoi(argv[5]);
    iformat	= (jpegenc_frame_fmt_e)atoi(argv[6]);
    oformat	= (jpegenc_frame_fmt_e)atoi(argv[7]);
    w_alignment	= atoi(argv[8]);
    h_alignment	= atoi(argv[9]);

    jpegenc_mem_type_e mem_type = (jpegenc_mem_type_e)atoi(argv[10]);
    if (argc > 11)
        num = atoi(argv[11]);
    if (argc > 12)
        num_planes = atoi(argv[12]);

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
    printf("num: %d \n", num);
    if (w_alignment > 0)
        w_stride = (width % w_alignment) == 0 ? width : (((width / w_alignment)+1)*w_alignment);
    else
        w_stride = width;

    if (h_alignment > 0)
        h_stride = (height % h_alignment) == 0 ? height : (((height / h_alignment)+1)*h_alignment);
    else
        h_stride = height;

    printf("align: %d->%d\n", width, w_stride);
    printf("align: %d->%d\n", height, h_stride);

    if (mem_type != JPEGENC_LOCAL_BUFF && mem_type != JPEGENC_DMA_BUFF) {
        printf("Encode Failed! unsupported mem type\n");
        return -1;
    }
    memset(&frame_info, 0, sizeof(frame_info));

    framesize = w_stride * h_stride * 3 / 2;
    ysize = w_stride * h_stride;
    usize = w_stride * h_stride / 4;
    vsize = w_stride * h_stride / 4;
    uvsize = w_stride * h_stride / 2;
    if (iformat == 1 || iformat == 5 || iformat == 6) {
        framesize = w_stride * h_stride * 3;
    } else if (iformat == 0) {
        framesize = w_stride * h_stride * 2;
    }

    printf("framesize=%d\n", framesize);
    if (mem_type == JPEGENC_DMA_BUFF) {
        ret = create_ctx(&ctx);
        if (ret < 0) {
            printf("gdc create fail ret=%d\n", ret);
            End_status = -1;
            goto exit;
        }
        if (num_planes == 3)
        {
            if (iformat != FMT_YUV420)
            {
                printf("error fmt %d\n", iformat);
                End_status = -1;
                goto exit;
            }
            ret = alloc_dma_buffer(&ctx, INPUT_BUFF_TYPE, ysize);
            if (ret < 0)
            {
                printf("alloc fail ret=%d, len=%u\n", ret,
                       ysize);
                End_status = -1;
                goto exit;
            }
            vaddr = (unsigned char *) ctx.i_buff[0];
            if (!vaddr)
            {
                printf("mmap failed,Not enough memory\n");
                End_status = -1;
                goto exit;
            }

            shared_fd[0] = ret;
            input[0] = vaddr;

            ret = alloc_dma_buffer(&ctx, INPUT_BUFF_TYPE, usize);
            if (ret < 0)
            {
                printf("alloc fail ret=%d, len=%u\n", ret,
                       usize);
                End_status = -1;
                goto exit;
            }
            vaddr = (unsigned char *) ctx.i_buff[1];
            if (!vaddr)
            {
                printf("mmap failed,Not enough memory\n");
                End_status = -1;
                goto exit;
            }
            shared_fd[1] = ret;
            input[1] = vaddr;

            ret = alloc_dma_buffer(&ctx, INPUT_BUFF_TYPE, vsize);
            if (ret < 0)
            {
                printf("alloc fail ret=%d, len=%u\n", ret,
                       vsize);
                End_status = -1;
                goto exit;
            }
            vaddr = (unsigned char *) ctx.i_buff[2];
            if (!vaddr)
            {
                printf("mmap failed,Not enough memory\n");
                End_status = -1;
                goto exit;
            }

            shared_fd[2] = ret;
            input[2] = vaddr;
        } else if (num_planes == 2)
        {
            ret = alloc_dma_buffer(&ctx, INPUT_BUFF_TYPE, ysize);
            if (ret < 0)
            {
                printf("alloc fail ret=%d, len=%u\n", ret,
                       ysize);
                End_status = -1;
                goto exit;
            }
            vaddr = (unsigned char *) ctx.i_buff[0];
            if (!vaddr)
            {
                printf("mmap failed,Not enough memory\n");
                End_status = -1;
                goto exit;
            }
            shared_fd[0] = ret;
            input[0] = vaddr;
            if (iformat != FMT_NV12 && iformat != FMT_NV21)
            {
                printf("error fmt %d\n", iformat);
                End_status = -1;
                goto exit;
            }
            ret = alloc_dma_buffer(&ctx, INPUT_BUFF_TYPE, uvsize);
            if (ret < 0)
            {
                printf("alloc fail ret=%d, len=%u\n", ret,
                       uvsize);
                End_status = -1;
                goto exit;
            }
            vaddr = (unsigned char *) ctx.i_buff[1];
            if (!vaddr)
            {
                printf("mmap failed,Not enough memory\n");
                End_status = -1;
                goto exit;
            }
            shared_fd[1] = ret;
            input[1] = vaddr;
        } else if (num_planes == 1)
        {
            ret =
                alloc_dma_buffer(&ctx, INPUT_BUFF_TYPE, framesize);
            if (ret < 0)
            {
                printf("alloc fail ret=%d, len=%u\n", ret,
                       framesize);
                End_status = -1;
                goto exit;
            }
            vaddr = (unsigned char *) ctx.i_buff[0];
            if (!vaddr)
            {
                printf("mmap failed,Not enough memory\n");
                End_status = -1;
                goto exit;
            }
            shared_fd[0] = ret;
            input[0] = vaddr;
        }

        frame_info.plane_num = num_planes;
        frame_info.YCbCr[0] = shared_fd[0];
        frame_info.YCbCr[1] = shared_fd[1];
        frame_info.YCbCr[2] = shared_fd[2];
    } else {
        inputBuffer = (unsigned char *) malloc(framesize);
        if (inputBuffer == NULL) {
            printf("Can not allocat inputBuffer\n");
            End_status = -1;
            goto exit;
        }
        frame_info.YCbCr[0] = (ulong) (inputBuffer);
        frame_info.YCbCr[1] =
            (ulong) (inputBuffer + ysize);
        frame_info.YCbCr[2] =
            (ulong) (inputBuffer + ysize + usize);
    }

    output_size = 1024 * 1024 * 10;
    output_buffer = (unsigned char *)malloc(output_size);
    if (!output_buffer) {
        printf("alloc buffer for output jpeg failed\n");
        End_status = -1;
        goto exit;
    }

    fp = fopen(argv[1], "rb");
    if (!fp) {
        printf("open input file %s failed: %s\n", argv[1], strerror(errno));
        End_status = -1;
        goto exit;
    }
    outfd = open(argv[2], O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (outfd < 0)
    {
        printf("open dist file error!\n");
        End_status = -1;
        goto exit;
    }

    frame_info.width = width;
    frame_info.height = height;
    frame_info.w_stride = w_stride;
    frame_info.h_stride = h_stride;
    frame_info.quality = quality;
    frame_info.iformat = iformat;
    frame_info.oformat = oformat;
    frame_info.mem_type = mem_type;

    handle = jpegenc_init();

    if (handle == (long)NULL) {
        printf("jpegenc_init failed\n");
        End_status = -1;
        goto exit;
    }

    if (mem_type == JPEGENC_DMA_BUFF) {	// read data to dma buf vaddr
        if (num_planes == 1)
        {
            if (fread(input[0], 1, framesize, fp) !=
                framesize)
            {
                printf("read input file error!\n");
                End_status = -1;
                goto exit;
            }
        } else if (num_planes == 2)
        {
            if (fread(input[0], 1, ysize, fp) != ysize)
            {
                printf("read input file error!\n");
                End_status = -1;
                goto exit;
            }
            if (fread(input[1], 1, uvsize, fp) != uvsize)
            {
                printf("read input file error!\n");
                End_status = -1;
                goto exit;
            }
        } else if (num_planes == 3)
        {
            if (fread(input[0], 1, ysize, fp) != ysize)
            {
                printf("read input file error!\n");
                End_status = -1;
                goto exit;
            }
            if (fread(input[1], 1, usize, fp) != usize)
            {
                printf("read input file error!\n");
                End_status = -1;
                goto exit;
            }
            if (fread(input[2], 1, vsize, fp) != vsize)
            {
                printf("read input file error!\n");
                End_status = -1;
                goto exit;
            }
        }
    } else {
        if (fread(inputBuffer, 1, framesize, fp) != framesize)
        {
            printf("read input file error!\n");
            End_status = -1;
            goto exit;
        }
    }
    memset(output_buffer, 0, output_size);

    for (int i = 0; i < num; i++) {
        int64_t t_1=GetNowUs();
        result = jpegenc_encode(handle, frame_info, output_buffer, &datalen);

        int64_t t_2=GetNowUs();
        fprintf(stderr, "jpegenc_encode time: %lld\n", t_2-t_1);
        if (result != ENC_FAILED) {
            write(outfd, (unsigned char *)output_buffer, datalen);
            enc_frame++;
            encoding_time+=t_2-t_1;
        } else {
            End_status = ENC_FAILED;
            goto exit;
        }
    }

    fprintf(stderr, "encode time:%lld, fps:%3.1f\n", encoding_time, enc_frame*1.0/(encoding_time/1000000.0));

exit:
    if (handle)
        jpegenc_destroy(handle);

    if (outfd >= 0)
        close(outfd);

    if (fp)
        fclose(fp);

    if (output_buffer != NULL)
        free(output_buffer);

    if (mem_type == JPEGENC_DMA_BUFF)
    {
        if (shared_fd[0] >= 0)
            close(shared_fd[0]);

        if (shared_fd[1] >= 0)
            close(shared_fd[1]);

        if (shared_fd[2] >= 0)
            close(shared_fd[2]);

        destroy_ctx(&ctx);
    } else {
        if (inputBuffer)
            free(inputBuffer);
    }

    if (End_status == 0)
        printf("Encode End Succeeded!\n");
    else if (End_status == -1)
        printf("Encode Failed!\n");
    else
        printf("Encode End with unknown status:%d\n", End_status);
    return 0;
}
