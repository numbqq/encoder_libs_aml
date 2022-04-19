#include "bjunion_enc/vpcodec_1_0.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <pthread.h>
#ifdef __ANDROID__
#include <media/stagefright/foundation/ALooper.h>
using namespace android;
#endif
extern "C"{
    #include "test_dma_api.h"
}
#include "enc_define.h"


#define COMBINE_STR " = "
#define MAX_ENCODER_INSTANCE  3

typedef struct {
    int instance_index;
    bool exit_flag;
    char src_file[256];
    char dst_file[256];
    int width;
    int height;
    int gop;
    int framerate;
    int bitrate;
    int num;
    int fmt;
    int buf_type;
    int num_planes;
    int fix_qp;
    int i_qp_min;
    int i_qp_max;
    int p_qp_min;
    int p_qp_max;
}config_param_t;

bool get_valid_int_param(char *linestr,char *var_str,int *param) {
    char *valid_str = NULL;
    char sub_str[256] = {0};
    if (NULL == var_str || NULL == linestr || NULL == param) {
        //printf("%s return",var_str);
        return false;
    }
    sprintf(sub_str,"%s%s",var_str,COMBINE_STR);
    valid_str = strstr(linestr,sub_str);
    if (NULL == valid_str) {
        //printf("sub_str:%s return\n",sub_str);
        return false;
    }
    valid_str += strlen(sub_str);

    //printf("sub_str:%s,valid_str:%s",sub_str,valid_str);
    if (valid_str)
    {
        *param = atoi(valid_str);
        //printf("%s%d\n",sub_str,*param);
        return true;
    }
    return false;
}

int get_end_str_len(char *linestr) {
    if (strstr(linestr,"\r\n"))
        return 2;
    else if (strstr(linestr,"\r"))
        return 1;
    else if (strstr(linestr,"\n"))
        return 1;
    return 0;
}


bool get_valid_str_param(char *linestr,char *var_str,char *param) {
    char *valid_str = NULL;
    char sub_str[256] = {0};
    char end_str_len = 0;
    if (NULL == var_str || NULL == linestr || NULL == param) {
        //printf("var_str return",var_str);
        return false;
    }
    end_str_len = get_end_str_len(linestr);
    sprintf(sub_str,"%s%s",var_str,COMBINE_STR);
    valid_str = strstr(linestr,sub_str);
    if (NULL == valid_str) {
        //printf("sub_str:%s return\n",sub_str);
        return false;
    }
    valid_str += strlen(sub_str);
    //printf("sub_str:%s,valid_str:%s",sub_str,valid_str);
    if (valid_str)
    {
        strncpy(param,valid_str,strlen(valid_str) - end_str_len); //except '\r\n'
        //printf("%s%s",sub_str,param);
        return true;
    }
    return false;
}


bool parse_config_file(const char *cfg_file,config_param_t *param) {
    FILE *fp = NULL;
    char lineStr[256] = {0};
    char *valid_str = NULL;
    printf("parse config file:%s\n",cfg_file);
    fp = fopen(cfg_file, "rb");
    if (fp == NULL)
    {
        printf("open cfg file error!\n");
        return false;
    }
    while (!feof(fp)) {
        if (fgets(lineStr, sizeof(lineStr), fp) == NULL )
        {
            continue;
        }
        //printf("linestr:%s",lineStr);
        if (lineStr[0] == '#')
            continue;
        if (get_valid_str_param(lineStr,"src_file",param->src_file))
            continue;
        if (get_valid_str_param(lineStr,"dst_file",param->dst_file))
            continue;
        if (get_valid_int_param(lineStr,"width",&param->width))
            continue;
        if (get_valid_int_param(lineStr,"height",&param->height))
            continue;
        if (get_valid_int_param(lineStr,"gop",&param->gop))
            continue;
        if (get_valid_int_param(lineStr,"framerate",&param->framerate))
            continue;
        if (get_valid_int_param(lineStr,"bitrate",&param->bitrate))
            continue;
        if (get_valid_int_param(lineStr,"num",&param->num))
            continue;
        if (get_valid_int_param(lineStr,"fmt",&param->fmt))
            continue;
        if (get_valid_int_param(lineStr,"buf_type",&param->buf_type))
            continue;
        if (get_valid_int_param(lineStr,"num_planes",&param->num_planes))
            continue;
        if (get_valid_int_param(lineStr,"fix_qp",&param->fix_qp))
            continue;
        if (get_valid_int_param(lineStr,"i_qp_min",&param->i_qp_min))
            continue;
        if (get_valid_int_param(lineStr,"i_qp_max",&param->i_qp_max))
            continue;
        if (get_valid_int_param(lineStr,"p_qp_min",&param->p_qp_min))
            continue;
        if (get_valid_int_param(lineStr,"p_qp_max",&param->p_qp_max))
            continue;
    }
    if (fp)
    {
        fclose(fp);
        fp = NULL;
    }
    return true;
}

bool parse_config_param(const char *argv[],int argc,config_param_t *param) {
    if (NULL == argv || NULL == param || argc < 9)
        return false;
    memcpy(param->src_file,argv[1],strlen(argv[1]));
    memcpy(param->dst_file,argv[2],strlen(argv[2]));
    param->width =  atoi(argv[3]);
    param->height = atoi(argv[4]);
    param->gop = atoi(argv[5]);
    param->framerate = atoi(argv[6]);
    param->bitrate = atoi(argv[7]);
    param->num = atoi(argv[8]);
    param->fmt = atoi(argv[9]);
    param->buf_type = atoi(argv[10]);
    param->num_planes = atoi(argv[11]);

    if (argc == 13)
    {
        param->fix_qp = atoi(argv[12]);
    }

    if (argc == 17)
    {
        param->i_qp_min = atoi(argv[13]);
        param->i_qp_max = atoi(argv[14]);
        param->p_qp_min = atoi(argv[15]);
        param->p_qp_max = atoi(argv[16]);
    }
    return true;
}

bool check_params(config_param_t *pconfig_param) {
    if (NULL == pconfig_param)
        return false;
    if ((pconfig_param->width < 1) || (pconfig_param->width > 1920))
    {
        printf("instance:%d,invalid width \n",pconfig_param->instance_index);
        return false;//-1;
    }

    if ((pconfig_param->height < 1) || (pconfig_param->height > 1080))
    {
        printf("instance:%d,invalid height \n",pconfig_param->instance_index);
        return false;// -1;
    }

    if ((pconfig_param->framerate < 0) || (pconfig_param->framerate > 30))
    {
        printf("instance:%d,invalid framerate %d \n",pconfig_param->instance_index,pconfig_param->framerate);
        return false;// -1;
    }
    if (pconfig_param->bitrate <= 0)
    {
        printf("instance:%d,invalid bitrate \n",pconfig_param->instance_index);
        return false;// -1;
    }
    if (pconfig_param->num < 0)
    {
        printf("instance:%d,invalid num \n",pconfig_param->instance_index);
        return false;// -1;
    }
    printf("*******instance %d param list*****\n",pconfig_param->instance_index);
    printf("src_url is: %s ;\n", pconfig_param->src_file);
    printf("out_url is: %s ;\n", pconfig_param->dst_file);
    printf("width   is: %d ;\n", pconfig_param->width);
    printf("height  is: %d ;\n", pconfig_param->height);
    printf("gop     is: %d ;\n", pconfig_param->gop);
    printf("frmrate is: %d ;\n", pconfig_param->framerate);
    printf("bitrate is: %d ;\n", pconfig_param->bitrate);
    printf("frm_num is: %d ;\n", pconfig_param->num);
    printf("buf_type is: %d ;\n", pconfig_param->buf_type);
    printf("num_planes is: %d ;\n", pconfig_param->num_planes);
    printf("fix_qp is: %d ;\n", pconfig_param->fix_qp);
    printf("i frame qp min:%d\n",pconfig_param->i_qp_min);
    printf("i frame qp max:%d\n",pconfig_param->i_qp_max);
    printf("p frame qp min:%d\n",pconfig_param->p_qp_min);
    printf("p frame qp max:%d\n",pconfig_param->p_qp_max);
    return true;
}

static void *encode_thread(void *param) {
    int outfd = -1;
    FILE *fp = NULL;
    int in_size = 0;
    int datalen = 0;
    unsigned ysize;
    unsigned usize;
    unsigned vsize;
    unsigned uvsize;
    unsigned char *vaddr = NULL;
    vl_dma_info_t dma_info;
    struct usr_ctx_s ctx;
    vl_codec_handle_t handle_enc = 0;
    config_param_t *pconfig_param = (config_param_t *)param;
    int64_t total_encode_time=0, t1,t2;
    int num_actually_encoded=0;
    int ret = -1;

    unsigned framesize  = pconfig_param->width * pconfig_param->height * 3 / 2;
    ysize = pconfig_param->width * pconfig_param->height;
    usize = pconfig_param->width * pconfig_param->height / 4;
    vsize = pconfig_param->width * pconfig_param->height / 4;
    uvsize = pconfig_param->width * pconfig_param->height / 2;
    if (pconfig_param->fmt == IMG_FMT_RGB888 || pconfig_param->fmt == IMG_FMT_BGR888) { /* modify for yuv size calc */
        framesize = pconfig_param->width * pconfig_param->height * 3;
    }
    unsigned output_size  = 1024 * 1024 * sizeof(char);
    unsigned char *output_buffer = (unsigned char *)malloc(output_size);
    unsigned char *input_buffer = NULL;
    unsigned char *input[3] = { NULL };

    if (pconfig_param->buf_type == DMA_BUFF)
    {
        memset(&dma_info, 0, sizeof(dma_info));
        dma_info.num_planes = pconfig_param->num_planes;
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
            if (pconfig_param->fmt != IMG_FMT_YV12)
            {
                printf("error fmt %d\n", pconfig_param->fmt);
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
            if (pconfig_param->fmt != IMG_FMT_NV12 && pconfig_param->fmt != IMG_FMT_NV21)
            {
                printf("error fmt %d\n", pconfig_param->fmt);
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

    fp = fopen(pconfig_param->src_file, "rb");
    if (fp == NULL)
    {
        printf("open src file error!\n");
        goto exit;
    }
    outfd = open(pconfig_param->dst_file, O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (outfd < 0)
    {
        printf("open dist file error!\n");
        goto exit;
    }

    if (pconfig_param->fix_qp >= 0)
    {
        handle_enc = vl_video_encoder_init_fix_qp(CODEC_ID_H264, pconfig_param->width, pconfig_param->height, pconfig_param->framerate, pconfig_param->bitrate, pconfig_param->gop, IMG_FMT_YV12, pconfig_param->fix_qp);
    }
    else
    {
        handle_enc = vl_video_encoder_init(CODEC_ID_H264, pconfig_param->width, pconfig_param->height, pconfig_param->framerate, pconfig_param->bitrate, pconfig_param->gop, IMG_FMT_YV12, pconfig_param->i_qp_min, pconfig_param->i_qp_max, pconfig_param->p_qp_min, pconfig_param->p_qp_max);
    }

    while (pconfig_param->num > 0) {
        if (pconfig_param->buf_type == DMA_BUFF) { // read data to dma buf vaddr
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

        datalen = vl_video_encoder_encode(handle_enc, FRAME_TYPE_AUTO, input_buffer, in_size, output_buffer, pconfig_param->fmt, pconfig_param->buf_type, &dma_info);

#ifdef __ANDROID__
        t2=ALooper::GetNowUs();
        total_encode_time+=t2-t1;
#endif

        if (datalen >= 0) {
            num_actually_encoded++;
            write(outfd, (unsigned char *)output_buffer, datalen);
        }
        pconfig_param->num--;
    }

    vl_video_encoder_destory(handle_enc);
#ifdef __ANDROID__
    printf("instance:%d,total_encode_time: %lld, num_actually_encoded: %d, fps=%3.3f\n", pconfig_param->instance_index,total_encode_time, num_actually_encoded, num_actually_encoded*1.0*1000000/total_encode_time);
#endif
exit:
    if (pconfig_param->buf_type == DMA_BUFF)
    {
        if (dma_info.shared_fd[0] >= 0)
            close(dma_info.shared_fd[0]);

        if (dma_info.shared_fd[1] >= 0)
            close(dma_info.shared_fd[1]);

        if (dma_info.shared_fd[2] >= 0)
            close(dma_info.shared_fd[2]);
        destroy_ctx(&ctx);
    } else {
        if (input_buffer)
            free(input_buffer);
    }
    if (output_buffer)
        free(output_buffer);
    if (outfd >= 0)
        close(outfd);
    if (fp)
        fclose(fp);
    pconfig_param->exit_flag = true;
    return NULL;
}

void print_help() {
    printf("Amlogic AVC Encode API \n");
    printf("single instance can run like this below:\n");
    printf(" usage: output [instancenum][usecfgfile][srcfile][outfile][width][height][gop][framerate][bitrate][num][fmt][buf_type][num_planes][const_qp][i_qp_in][i_qp_max][p_qp_min][p_qp_max]\n");
    printf("multi instance can run like this below \n");
    printf(" usage: output [instancenum][usecfgfile][cfgfile1][cfgfile2][cfgfile3]...[cfgfile_instancenum]\n");
    printf("  options  :\n");
    printf("  instance num  : the count of encoder instances,max is 3,multi instance must use cfg file option\n");
    printf("  usecfgfile  : 0:not use cfgfile 1:use cfgfile\n");
    printf("  ******************plese set filesrc when cfgfile = 1********************\n");
    printf("  srcfile1  : cfg file url1 in your root fs when multi instance\n");
    printf("  srcfile2  : cfg file url2 in your root fs when multi instance\n");
    printf("  srcfile3  : cfg file url3 in your root fs when multi instance\n");
    printf("  ...\n");
    printf("  ******************plese set params when cfgfile = 0********************\n");
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
}


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
    config_param_t config_param[MAX_ENCODER_INSTANCE] = {0};
    char *cfg_file = NULL;
    bool use_cfg_file = false;
    unsigned char inst_num = 0;
    pthread_t tid;
    int iloop = 0;
    if (argc < 2)
    {
        print_help();
        return -1;
    }
    inst_num = atoi(argv[1]);
    use_cfg_file = atoi(argv[2]);
    if (inst_num <= 0 || inst_num > MAX_ENCODER_INSTANCE)
    {
        printf("invalid instance num!\n");
        return -1;
    }
    if (use_cfg_file == 0 && argc < 9) //1 instance,use param
    {
        print_help();
        return -1;
    }
    else
    {
        printf("instance num:%s\n", argv[1]);
        printf("use_cfg_file:%s\n", argv[2]);
    }
    if (inst_num > 1 && (!use_cfg_file))
    {
        printf("please use config file when test multi param\n");
        return -1;
    }
    if (use_cfg_file && argc < (inst_num + 2))
    {
        printf("please enter right number of param!\n");
        return -1;
    }
    if (1 == inst_num && 0 == use_cfg_file)
    {
        parse_config_param(&argv[2],argc-2,&config_param[0]);
        check_params(&config_param[0]);
    }
    else {
        for (iloop = 0;iloop < inst_num;iloop++)
        {
            config_param[iloop].instance_index = iloop;
            parse_config_file(argv[iloop + 2 + 1], &config_param[iloop]);
            check_params(&config_param[iloop]);
        }
    }
    for (iloop = 0;iloop < inst_num;iloop++)
    {
        pthread_create(&tid, NULL, encode_thread, &config_param[iloop]);
    }

    //wait for all instances exit
    for (iloop = 0; iloop < inst_num; ) {
        if (iloop >= MAX_ENCODER_INSTANCE)
            break;
        while (1) {
            if (config_param[iloop].exit_flag) {
                iloop++;
                break;
            }
            usleep(40*1000);
        }
    }

    return -1;
}
