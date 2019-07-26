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
int main(int argc, const char* argv[])
{
    int width, height, gop, framerate, bitrate, num;
    int ret = 0;
    int outfd = -1;
    FILE* fp = NULL;
    int datalen = 0;
    vl_img_format_t fmt = IMG_FMT_NONE;

    unsigned char* vaddr = NULL;
    vl_codec_handle_t handle_enc = 0;

    vl_param_runtime_t param_runtime;
    //amvenc_frame_stat_t* frame_info = NULL;
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

    if (argc < 12) {
        printf("Amlogic Encoder API \n");
        printf(
            " usage: output "
            "[srcfile][outfile][width][height][gop][framerate][bitrate][num][fmt]["
            "buf type][num_planes][codec_id]\n");
        printf("  options  :\n");
        printf("  srcfile  : yuv data url in your root fs\n");
        printf("  outfile  : stream url in your root fs\n");
        printf("  width    : width\n");
        printf("  height   : height\n");
        printf("  gop      : I frame refresh interval\n");
        printf("  framerate: framerate \n ");
        printf("  bitrate  : bit rate \n ");
        printf("  num      : encode frame count \n ");
        printf("  fmt      : encode input fmt 1 : nv12, 2 : nv21, 3 : yuv420p(yv12/yu12)\n ");
        printf("  buf_type : 0:vmalloc, 3:dma buffer\n ");
        printf(
            "  num_planes : used for dma buffer case. 1: nv12/nv21/yv12. "
            "2:nv12/nv21. 3:yv12\n ");
        printf("  codec_id : 4 : h.264, 5 : h.265\n ");

        return -1;
    } else {
        printf("%s\n", argv[1]);
        printf("%s\n", argv[2]);
    }

    width = atoi(argv[3]);
    if ((width < 1) || (width > WIDTH)) {
        printf("invalid width \n");
        return -1;
    }

    height = atoi(argv[4]);
    if ((height < 1) || (height > HEIGHT)) {
        printf("invalid height \n");
        return -1;
    }

    gop = atoi(argv[5]);
    framerate = atoi(argv[6]);
    bitrate = atoi(argv[7]);
    num = atoi(argv[8]);
    fmt = (vl_img_format_t)atoi(argv[9]);
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

    if ((framerate < 0) || (framerate > FRAMERATE)) {
        printf("invalid framerate %d \n", framerate);
        return -1;
    }
    if (bitrate <= 0) {
        printf("invalid bitrate \n");
        return -1;
    }
    if (num < 0) {
        printf("invalid num \n");
        return -1;
    }
    if (buf_type == 3 && (num_planes < 1 || num_planes > 3)) {
        printf("invalid num_planes \n");
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
    printf("fmt is: %d ;\n", fmt);
    printf("buf_type is: %d ;\n", buf_type);
    printf("num_planes is: %d ;\n", num_planes);

    unsigned int framesize = width * height * 3 / 2;
    unsigned ysize = width * height;
    unsigned usize = width * height / 4;
    unsigned vsize = width * height / 4;
    unsigned uvsize = width * height / 2;

    unsigned int outbuffer_len = 1024 * 1024 * sizeof(char);
    unsigned char* outbuffer = (unsigned char*)malloc(outbuffer_len);
    unsigned char* inputBuffer = NULL;
    unsigned char* input[3] = {NULL};
    vl_buffer_info_t inbuf_info;
    vl_dma_info_t* dma_info = NULL;

    memset(&inbuf_info, 0, sizeof(vl_buffer_info_t));
    inbuf_info.buf_type = (vl_buffer_type_t)buf_type;

#if 0
    fd = open("/dev/amvenc_multi", O_RDWR);
    if (fd < 0) {
        printf("unable to open the encoder path /dev/amvenc_multi\n");
        goto exit;
    }
#endif

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

    //  encode_info.dev_fd = fd;
    //  encode_info.qp_mode = qp_mode;

  if (inbuf_info.buf_type == DMA_TYPE)
  {
    dma_info = &(inbuf_info.buf_info.dma_info);
    dma_info->num_planes = num_planes;
    dma_info->shared_fd[0] = -1;  // dma buf fd
    dma_info->shared_fd[1] = -1;
    dma_info->shared_fd[2] = -1;
    ret = create_ctx(&ctx);
    if (ret < 0) {
      printf("gdc create fail ret=%d\n", ret);
      goto exit;
    }

    if (dma_info->num_planes == 3)
    {
        if (fmt != IMG_FMT_YUV420P) {
            printf("error fmt %d\n", fmt);
            goto exit;
        }
        ret = alloc_dma_buffer(&ctx, INPUT_BUFF_TYPE, ysize);
        if (ret < 0) {
            printf("alloc fail ret=%d, len=%u\n", ret, ysize);
            goto exit;
        }
        vaddr = (unsigned char*)ctx.i_buff[0];
        if (!vaddr) {
            printf("mmap failed,Not enough memory\n");
            goto exit;
        }
        dma_info->shared_fd[0] = ret;
        input[0] = vaddr;

        ret = alloc_dma_buffer(&ctx, INPUT_BUFF_TYPE, usize);
        if (ret < 0) {
            printf("alloc fail ret=%d, len=%u\n", ret, usize);
            goto exit;
        }
        vaddr = (unsigned char*)ctx.i_buff[1];
        if (!vaddr) {
            printf("mmap failed,Not enough memory\n");
            goto exit;
        }
        dma_info->shared_fd[1] = ret;
        input[1] = vaddr;

        ret = alloc_dma_buffer(&ctx, INPUT_BUFF_TYPE, vsize);
        if (ret < 0) {
            printf("alloc fail ret=%d, len=%u\n", ret, vsize);
            goto exit;
        }
        vaddr = (unsigned char*)ctx.i_buff[2];
        if (!vaddr) {
            printf("mmap failed,Not enough memory\n");
            goto exit;
        }
        dma_info->shared_fd[2] = ret;
        input[2] = vaddr;
    }
    else if (dma_info->num_planes == 2)
    {
        ret = alloc_dma_buffer(&ctx, INPUT_BUFF_TYPE, ysize);
        if (ret < 0) {
            printf("alloc fail ret=%d, len=%u\n", ret, ysize);
            goto exit;
        }
        vaddr = (unsigned char*)ctx.i_buff[0];
        if (!vaddr) {
            printf("mmap failed,Not enough memory\n");
            goto exit;
        }
        dma_info->shared_fd[0] = ret;
        input[0] = vaddr;
        if (fmt != IMG_FMT_NV12 && fmt != IMG_FMT_NV21) {
            printf("error fmt %d\n", fmt);
            goto exit;
        }
        ret = alloc_dma_buffer(&ctx, INPUT_BUFF_TYPE, uvsize);
        if (ret < 0) {
            printf("alloc fail ret=%d, len=%u\n", ret, uvsize);
            goto exit;
        }
        vaddr = (unsigned char*)ctx.i_buff[1];
        if (!vaddr) {
            printf("mmap failed,Not enough memory\n");
            goto exit;
        }
        dma_info->shared_fd[1] = ret;
        input[1] = vaddr;
    }
    else if (dma_info->num_planes == 1)
    {
        ret = alloc_dma_buffer(&ctx, INPUT_BUFF_TYPE, framesize);
        if (ret < 0) {
            printf("alloc fail ret=%d, len=%u\n", ret, framesize);
            goto exit;
        }
        vaddr = (unsigned char*)ctx.i_buff[0];
        if (!vaddr) {
            printf("mmap failed,Not enough memory\n");
            goto exit;
        }
        dma_info->shared_fd[0] = ret;
        input[0] = vaddr;
    }

    printf("in[0] %d, in[1] %d, in[2] %d\n", dma_info->shared_fd[0],
       dma_info->shared_fd[1], dma_info->shared_fd[2]);
    printf("input[0] %p, input[1] %p,input[2] %p\n", input[0], input[1],
       input[2]);
    } else {
        inputBuffer = (unsigned char*)malloc(framesize);
        inbuf_info.buf_info.in_ptr[0] = (ulong)(inputBuffer);
        inbuf_info.buf_info.in_ptr[1] = (ulong)(inputBuffer + width * height);
        inbuf_info.buf_info.in_ptr[2] = (ulong)(inputBuffer + width * height * 5 / 4);
    }

    memset(&param_runtime, 0, sizeof(param_runtime));
    param_runtime.idr = &tmp_idr;
    param_runtime.bitrate = bitrate;
    param_runtime.frame_rate = framerate;

    fp = fopen((argv[1]), "rb");
    if (fp == NULL) {
        printf("open src file error!\n");
        goto exit;
    }

    outfd = open((argv[2]),
               O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (outfd < 0) {
    printf("open dest file error!\n");
    goto exit;
    }

    handle_enc = vl_multi_encoder_init(codec_id, encode_info, &qp_tbl);

    while (num > 0)
    {
        if (inbuf_info.buf_type == DMA_TYPE)// read data to dma buf vaddr
        {
          if (dma_info->num_planes == 1) {
            if (fread(input[0], 1, framesize, fp) != framesize) {
              printf("read input file error!\n");
              goto exit;
            }
          } else if (dma_info->num_planes == 2) {
            if (fread(input[0], 1, ysize, fp) != ysize) {
              printf("read input file error!\n");
              goto exit;
            }
            if (fread(input[1], 1, uvsize, fp) != uvsize) {
              printf("read input file error!\n");
              goto exit;
            }
          } else if (dma_info->num_planes == 3) {
            if (fread(input[0], 1, ysize, fp) != ysize) {
              printf("read input file error!\n");
              goto exit;
            }
            if (fread(input[1], 1, usize, fp) != usize) {
              printf("read input file error!\n");
              goto exit;
            }
            if (fread(input[2], 1, vsize, fp) != vsize) {
              printf("read input file error!\n");
              goto exit;
            }
          }
        }
        else
        {  // read data to vmalloc buf vaddr
          if (fread(inputBuffer, 1, framesize, fp) != framesize) {
            printf("read input file error!\n");
            goto exit;
          }
        }

        memset(outbuffer, 0, outbuffer_len);
        if (inbuf_info.buf_type == DMA_TYPE) {
          sync_cpu(&ctx);
        }

        encoding_metadata = vl_multi_encoder_encode(handle_enc, FRAME_TYPE_AUTO, outbuffer,\
                                          &inbuf_info, &ret_buf);

        if (encoding_metadata.is_valid) {
          write(outfd, (unsigned char*)outbuffer, encoding_metadata.encoded_data_length_in_bytes);
        } else {
          printf("encode error %d!\n", encoding_metadata.encoded_data_length_in_bytes);
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

#if 1
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
#endif

    {
        if (inputBuffer)
          free(inputBuffer);
    }

    if (fd >= 0)
    close(fd);

    printf("end test!\n");

  return 0;

}
