#include "bjunion_enc/vpcodec_1_0.h"

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
#ifdef __ANDROID__
#include <media/stagefright/foundation/ALooper.h>
using namespace android;
#endif
extern "C"{
    #include "test_dma_api.h"
}
#include "enc_define.h"

int main(int argc, const char *argv[]) {
    int width, height, gop, framerate, bitrate, num, in_size = 0;
    int i_qp_min = -1;
    int i_qp_max = -1;
    int p_qp_min = -1;
    int p_qp_max = -1;

    int outfd = -1;
    FILE *fp = NULL;
    int datalen = 0;
    int fmt = 0;
    int buf_type = 0;
    int num_planes = 0;
    int fix_qp = -1;
    vl_codec_handle_t handle_enc;
    int64_t total_encode_time=0, t1,t2;
    int num_actually_encoded=0;

    unsigned int framesize;
    unsigned ysize;
    unsigned usize;
    unsigned vsize;
    unsigned uvsize;
    unsigned char *vaddr = NULL;

    vl_dma_info_t dma_info;
    struct usr_ctx_s ctx;
    int ret = -1;
    if (argc < 12)
    {
        printf("Amlogic AVC Encode API \n");
        printf(" usage: output [srcfile][outfile][width][height][gop][framerate][bitrate][num][fmt][buf_type][num_planes][const_qp][i_qp_in][i_qp_max][p_qp_min][p_qp_max]\n");
        printf("  options  :\n");
        printf("  srcfile  : yuv data url in your root fs\n");
        printf("  outfile  : stream url in your root fs\n");
        printf("  width    : width\n");
        printf("  height   : height\n");
        printf("  gop      : I frame refresh interval\n");
        printf("  framerate: framerate \n ");
        printf("  bitrate  : bit rate \n ");
        printf("  num      : encode frame count \n ");
        printf("  fmt      : encode input fmt 0:nv12 1:nv21 2:yv12 3:rgb888 4:bgr888\n ");
        printf("  buf_type : 0:vmalloc, 3:dma buffer\n");
        printf("  num_planes : used for dma buffer case. 2 : nv12/nv21, 3 : yuv420p(yv12/yu12)\n");
        printf("  const_qp : optional, [0-51]\n ");
        printf("  i_qp_min : i frame qp min\n ");
        printf("  i_qp_max : i frame qp max\n ");
        printf("  p_qp_min : p frame qp min\n ");
        printf("  p_qp_max : p frame qp max\n ");
        return -1;
    }
    else
    {
        printf("%s\n", argv[1]);
        printf("%s\n", argv[2]);
    }
    width =  atoi(argv[3]);
    if ((width < 1) || (width > 1920))
    {
        printf("invalid width \n");
        return -1;
    }
    height = atoi(argv[4]);
    if ((height < 1) || (height > 1080))
    {
        printf("invalid height \n");
        return -1;
    }
    gop = atoi(argv[5]);
    framerate = atoi(argv[6]);
    bitrate = atoi(argv[7]);
    num = atoi(argv[8]);
    fmt = atoi(argv[9]);
    buf_type = atoi(argv[10]);
    num_planes = atoi(argv[11]);

    if (argc == 13)
    {
        fix_qp = atoi(argv[12]);
        printf("fix_qp is: %d ;\n", fix_qp);
    }

    if (argc == 16)
    {
        i_qp_min = atoi(argv[12]);
        i_qp_max = atoi(argv[13]);
        p_qp_min = atoi(argv[14]);
        p_qp_max = atoi(argv[15]);

        printf("i frame qp min:%d\n ",i_qp_min);
        printf("i frame qp max:%d\n ",i_qp_max);
        printf("p frame qp min:%d\n ",p_qp_min);
        printf("p frame qp max:%d\n ",p_qp_max);
    }

    if ((framerate < 0) || (framerate > 30))
    {
        printf("invalid framerate %d \n",framerate);
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
    printf("src_url is: %s ;\n", argv[1]);
    printf("out_url is: %s ;\n", argv[2]);
    printf("width   is: %d ;\n", width);
    printf("height  is: %d ;\n", height);
    printf("gop     is: %d ;\n", gop);
    printf("frmrate is: %d ;\n", framerate);
    printf("bitrate is: %d ;\n", bitrate);
    printf("frm_num is: %d ;\n", num);
    printf("buf_type is: %d ;\n", buf_type);
    printf("num_planes is: %d ;\n", num_planes);

    framesize  = width * height * 3 / 2;
    ysize = width * height;
    usize = width * height / 4;
    vsize = width * height / 4;
    uvsize = width * height / 2;
    if (fmt == IMG_FMT_RGB888 || fmt == IMG_FMT_BGR888) { /* modify for yuv size calc */
        framesize = width * height * 3;
    }
    unsigned output_size  = 1024 * 1024 * sizeof(char);
    unsigned char *output_buffer = (unsigned char *)malloc(output_size);
    unsigned char *input_buffer = NULL;
    unsigned char *input[3] = { NULL };

    if (buf_type == DMA_BUFF)
    {
        memset(&dma_info, 0, sizeof(dma_info));
        dma_info.num_planes = num_planes;
        dma_info.shared_fd[0] = -1; // dma buf fd
        dma_info.shared_fd[1] = -1;
        dma_info.shared_fd[2] = -1;
        ret = create_ctx(&ctx);
        if (ret < 0)
        {
            printf("gdc create fail ret=%d\n", ret);
            goto exit;
        }

        if (dma_info.num_planes == 3)
        {
            if (fmt != IMG_FMT_YV12)
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
            dma_info.shared_fd[0] = ret;
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
            dma_info.shared_fd[1] = ret;
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
            dma_info.shared_fd[2] = ret;
            input[2] = vaddr;
        } else if (dma_info.num_planes == 2)
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
            dma_info.shared_fd[0] = ret;
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
            dma_info.shared_fd[1] = ret;
            input[1] = vaddr;
        } else if (dma_info.num_planes == 1)
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
            dma_info.shared_fd[0] = ret;
            input[0] = vaddr;
        }

        printf("in[0] %d, in[1] %d, in[2] %d\n", dma_info.shared_fd[0],
               dma_info.shared_fd[1], dma_info.shared_fd[2]);
        printf("input[0] %p, input[1] %p,input[2] %p\n", input[0],
               input[1], input[2]);
    } else {
        input_buffer = (unsigned char *) malloc(framesize);
        if (input_buffer == NULL) {
            printf("Can not allocat inputBuffer\n");
            goto exit;
        }
    }

    fp = fopen((char *)argv[1], "rb");
    if (fp == NULL)
    {
        printf("open src file error!\n");
        goto exit;
    }
    outfd = open((char *)argv[2], O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (outfd < 0)
    {
        printf("open dist file error!\n");
        goto exit;
    }

    if (fix_qp >= 0)
    {
        handle_enc = vl_video_encoder_init_fix_qp(CODEC_ID_H264, width, height, framerate, bitrate, gop, IMG_FMT_YV12, fix_qp);
    }
    else
    {
        handle_enc = vl_video_encoder_init(CODEC_ID_H264, width, height, framerate, bitrate, gop, IMG_FMT_YV12, i_qp_min, i_qp_max, p_qp_min, p_qp_max);
    }

    while (num > 0) {
        if (buf_type == DMA_BUFF) { // read data to dma buf vaddr
            if (dma_info.num_planes == 1)
            {
                if (fread(input[0], 1, framesize, fp) !=
                    framesize)
                {
                    printf("read input file error!\n");
                    goto exit;
                }
            } else if (dma_info.num_planes == 2)
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
            } else if (dma_info.num_planes == 3)
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
        {       // read data to vmalloc buf vaddr
            if (fread(input_buffer, 1, framesize, fp) != framesize)
            {
                printf("read input file error!\n");
                goto exit;
            }
        }
        memset(output_buffer, 0, output_size);
#ifdef __ANDROID__
        t1=ALooper::GetNowUs();
#endif

        datalen = vl_video_encoder_encode(handle_enc, FRAME_TYPE_AUTO, input_buffer, in_size, output_buffer, fmt, buf_type, &dma_info);

#ifdef __ANDROID__
        t2=ALooper::GetNowUs();
        total_encode_time+=t2-t1;
#endif

        if (datalen >= 0) {
            num_actually_encoded++;
            write(outfd, (unsigned char *)output_buffer, datalen);
        }
        num--;
    }

#ifdef __ANDROID__
    printf("total_encode_time: %lld, num_actually_encoded: %d, fps=%3.3f\n", total_encode_time, num_actually_encoded, num_actually_encoded*1.0*1000000/total_encode_time);
#endif

exit:
    if (handle_enc)
        vl_video_encoder_destory(handle_enc);

    if (outfd >= 0)
        close(outfd);
    if (fp)
        fclose(fp);
    if (output_buffer != NULL)
        free(output_buffer);

    if (buf_type == DMA_BUFF)
    {
        if (dma_info.shared_fd[0] >= 0)
            close(dma_info.shared_fd[0]);

        if (dma_info.shared_fd[1] >= 0)
            close(dma_info.shared_fd[1]);

        if (dma_info.shared_fd[2] >= 0)
            close(dma_info.shared_fd[2]);
        destroy_ctx(&ctx);
    } else
    {
        if (input_buffer)
            free(input_buffer);
    }

    return 0;
}
