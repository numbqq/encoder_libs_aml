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
    int fix_qp = -1;
    vl_codec_handle_t handle_enc;
    int64_t total_encode_time=0, t1,t2;
    int num_actually_encoded=0;
    if (argc < 9)
    {
        printf("Amlogic AVC Encode API \n");
        printf(" usage: output [srcfile][outfile][width][height][gop][framerate][bitrate][num][fmt][const_qp][i_qp_in][i_qp_max][p_qp_min][p_qp_max]\n");
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

    if (argc == 11)
    {
        fix_qp = atoi(argv[10]);
        printf("fix_qp is: %d ;\n", fix_qp);
    }

    if (argc == 14)
    {
        i_qp_min = atoi(argv[10]);
        i_qp_max = atoi(argv[11]);
        p_qp_min = atoi(argv[12]);
        p_qp_max = atoi(argv[13]);

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

    unsigned framesize  = width * height * 3 / 2;
    if (fmt == 3 || fmt == 4) { /* modify for yuv size calc */
        framesize = width * height * 3;
    }
    unsigned output_size  = 1024 * 1024 * sizeof(char);
    unsigned char *output_buffer = (unsigned char *)malloc(output_size);
    unsigned char *input_buffer = (unsigned char *)malloc(framesize);

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
        if (fread(input_buffer, 1, framesize, fp) != framesize) {
            printf("read input file error!\n");
            goto exit;
        }
        memset(output_buffer, 0, output_size);
#ifdef __ANDROID__
		t1=ALooper::GetNowUs();
#endif

        datalen = vl_video_encoder_encode(handle_enc, FRAME_TYPE_AUTO, input_buffer, in_size, output_buffer, fmt);

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
    vl_video_encoder_destory(handle_enc);
    close(outfd);
    fclose(fp);
    free(output_buffer);
    free(input_buffer);

#ifdef __ANDROID__
	printf("total_encode_time: %lld, num_actually_encoded: %d, fps=%3.3f\n", total_encode_time, num_actually_encoded, num_actually_encoded*1.0*1000000/total_encode_time);
#endif

    return 0;
exit:
    if (input_buffer)
        free(input_buffer);
    if (output_buffer)
        free(output_buffer);
    if (outfd >= 0)
        close(outfd);
    if (fp)
        fclose(fp);
    return -1;
}
