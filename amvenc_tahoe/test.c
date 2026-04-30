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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/poll.h>
#include <pthread.h>
#include <assert.h>

#include "vdi.h"
#include "tahoe.h"
#include "tahoe_register.h"
#include "libvptahoe_codec.h"
#include <stdint.h>
#include "enc_driver.h"


//#define TAHOE_DEVICE_NAME "/dev/tahoe"

#define FORCE_PICTURE_TYPE	0x1
#define CHANGE_TARGET_RATE	0x2
#define CHANGE_MIN_MAX_QP	0x4
#define CHANGE_GOP_PERIOD	0x8
#define LONGTERM_REF_SET	0x10
#define CUST_SIZE_SMOOTH	0x20
#define CUST_APP_H264_PLUS	0x40
#define CHANGE_MULTI_SLICE	0x80
#define FORCE_PICTURE_SKIP	0x100
#define CHANGE_STRICT_RC	0x200
#define ENCODER_STRING_LEN_MAX	256


#define WIDTH 3840
#define HEIGHT 3840    // consider rotated frame
#define FRAMERATE 60

#define ENC_STREAM_BUF_SIZE  0x100000
#define CFG_ENCODE_NUM_MAX 6

typedef struct {
    int osdNum;
    //int enable;
    int format;      /*0:ARGB8888 1:ARGB4444 2:ARGB1555 3:NV21 4:NV12 5:BITMAP*/
    int alpha_mode;
    int r2y_mode;
    //int endian;
    //int y_baseaddr_h2bit;
    //int c_baseaddr_h2bit;
    //int cmn_rsv;
    int x;
    int y;
    int w;
    int h;
    int alpha;
    int bitmap_y;
    int bitmap_u;
    int bitmap_v;
    //int y_baseaddr;
    //int c_baseaddr;
    //int stride;
    char osdfile[ENCODER_STRING_LEN_MAX];
}osdCfgParam;

typedef struct {
    int ds_only;
    int ds_osd;
    int ds_w;
    int ds_h;
    char dsfile[ENCODER_STRING_LEN_MAX];
}dsCfgParam;

typedef struct {
    int x;
    int y;
    int w;
    int h;
}cropCfgParam;

typedef struct {
    int FrameNum; //change frame number
    int enable_option;
    //FORCE_PICTURE_TYPE
    int picType; //vl_frame_type_t
    //CHANGE_TARGET_RATE
    int bitRate;
    //CHANGE_MIN_MAX_QP
    int minQpI;
    int maxQpI;
    int maxDeltaQp;
    int minQpP;
    int maxQpP;
    int minQpB;
    int maxQpB;
    //CHANGE_GOP_PERIOD
    int intraQP;
    int GOPPeriod;
    //LONGTERM_REF_SET
    int LTRFlags; //bit 0: UseCurSrcAsLongtermPic; bit 1: Use LTR frame
    // CUST_SIZE_SMOOTH
    int smooth_level; //0 normal, 50(middle), 100 (high)
    // CUST_APP_H264_PLUS
    int newIDRInterval; // shall be multiply of gop length
    int QPdelta_IDR;     // QP delta apply to IDRs
    int QPdelta_LTR;     // QP delta apply to LTR P frames
    // Change multi-slice
    int Slice_Mode;  //multi slice mode
    int Slice_Para;  //para
    // Change strict bitrate control
    int bitrate_window;  //bitrate cal window
    int pskip_threshold;  //threshold for pskip
} CfgChangeParam;

typedef struct {
    int   seqNum;
    int   chnid;
    struct timeval timeFrameStart;
    struct timeval timeFrameEnd;
    int   encodeEnd;
    char  srcfile[ENCODER_STRING_LEN_MAX];    // yuv data url in your root fs
    char  outfile[ENCODER_STRING_LEN_MAX];    // stream url in your root fs
    int   width;           // width
    int   height;          // height
    int   gop;             // I frame refresh interval
    int   framerate;       // framerate
    int   bitrate;         // bit rate
    int   num;             // encode frame count
    int   fmt;             // encode input fmt 1 : nv12, 2 : nv21, 3 : yuv420p(yv12/yu12)
    int   buf_type;        // 0:vmalloc, 3:dma buffer
    int   num_planes;      // used for dma buffer case. 2 : nv12/nv21, 3 : yuv420p(yv12/yu12)
    int   codec_id;        // 4 : h.264, 5 : h.265
    int   cfg_opt;         /* optional, flags to control the encode feature settings
                            bit 0:roi control. 0: disabled (default) 1: enabled
                            bit 1: update control. 0: disabled (default) 1: enabled
                            bit 2 ~ bit 6 encode GOP pattern options:
                                0 default(IP) 1:I only    2:IP_only    3: IBBBP
                                4:IP_SVC1     5:IP_SVC2   6: IP_SVC3   7: IP_SVC4  8: CUSTP
                            bit 7:long term references. 0: disabled (default) 1: enabled
                            bit 8:cust source buffer stride . 0: disabled (default) 1: enabled
                            bit 9: convert normal source file stride to cust source stride. 0: disabled (default) 1: enabled
                            bit 10:enable intraRefresh. 0: disabled (default) 1: enabled
                            bit 11 ~ bit 13: profile 0: auto (default) others as following
                                    1: BASELINE(AVC) MAIN (HEVC);    2 MAIN (AVC) main10 (HEVC)
                                    3: HIGH (AVC) STILLPICTURE (HEVC); 4 HIGH10 (AVC)
                            bit 14 ~ bit 15: frame rotation(counter-clockwise, in degree): 0:none, 1:90, 2:180, 3:270
                            bit 16 ~ bit 17: frame mirroring: 0:none, 1:vertical, 2:horizontal, 3:both V and H
                            bit 18: enable customer bitstream buffer size
                            bit 19 ~ bit 20: enable multi slice mode: 0, one slice(no para), 1 by MB(16x16)/CTU(64x64), 2 by CTU + size(byte)
                            bit 21 cust_qp_delta_en
                            bit 22 strict_rate_ctrl_en (skipP allowed to limit bitrate) default 0
                            bit 23 forcePicQpEnable default 0
                            */
    char  roifile[ENCODER_STRING_LEN_MAX];      // optional, roi info url in root fs, no present if roi is disabled
    int   cfg_upd_enabled;
    int   has_cfg_update;
    char  cfg_file[ENCODER_STRING_LEN_MAX];     // optional, cfg update info url. no present if update is disabled
    int   src_buf_stride;    // optional, source buffer stride
    int   intraRefresh ;     // optional, lower 4 bits for intra-refresh mode; others for refresh parameter, controlled by intraRefresh bit
    int   bitStreamBuf ;     // optional, encoded bitstream buffer size in MB
    int   MultiSlice_parameter;         // optional, multi slice parameter in MB/CTU or CTU and size (HIGH_16bits size + LOW_16bit CTU) combined
    int   cust_qp_delta;                // optional, cust_qp_delta apply to P frames value >0 lower; <0 increase the quality
    int   strict_rate_control_parameter; // optional, High 16 bit bitrate window (frames max 120). low 16 bit bitrate threshold (percent)
    int   forcePicQpIPB;
    int   profile;
    int   level;
    int   tier;
    int   osd_enabled;
    char  osdfile[ENCODER_STRING_LEN_MAX];
    int   ds_enabled;
    char  dsfile[ENCODER_STRING_LEN_MAX];
    int   crop_enabled;
    char  cropfile[ENCODER_STRING_LEN_MAX];
    int   forcePicQpI;
    int   forcePicQpPB;
    int   rc_mode;
    unsigned int frame_rotation;
    unsigned int frame_mirroring;
} encodecCfgParam;


//int venc_init_flag = 0;

static int ParseOsdCfgFile(FILE *fp, osdCfgParam *cfg_osd);
static int ParseDsCfgFile(FILE *fp, dsCfgParam *cfg_ds);
static int ParseCropCfgFile(FILE *fp, cropCfgParam *cfg_crop);
static int ParseEncodeCfgFile(FILE *fp, encodecCfgParam *cfg_encode);
static int ParseCfgUpdateFile(FILE *fp, CfgChangeParam *cfg_update);


static char*mystrncpy(char* dest, const char* src, int len)
{
    assert(dest != NULL && src != NULL);
    char* temp = dest;
    int i=0;
    //while (i++ < len  && (*temp++ = *src++) != '\0') {}
    while (i++ < len  && (*temp = *src) != '\n' && (*temp++ = *src++) != '\0') {}
    if (*(temp) != '\0')
        *temp = '\0';
    return dest;
}

int update_cfg(FILE *fp_cfg, int frame_num, handle_t *encode_info, encodecCfgParam *cfg_encode, CfgChangeParam cfgChange) {

    if (cfgChange.FrameNum == frame_num) { // apply updates
        if (cfgChange.enable_option
            & FORCE_PICTURE_TYPE)
        {
            if (cfgChange.picType == FRAME_TYPE_IDR
                || cfgChange.picType
                == FRAME_TYPE_I)
            {
                encode_info->enc_frame_type = FRAME_TYPE_I;
                printf("force I frame on %d \n",
                    frame_num);
            }
        }
//        if (cfgChange.enable_option
//            & FORCE_PICTURE_SKIP)
//        {
//            vl_video_encoder_skip_frame(handle_enc);
//            printf("force skip frame on %d \n",
//                    frame_num);
//        }
        if (cfgChange.enable_option
            & CHANGE_TARGET_RATE)
        {
            vl_video_encoder_change_bitrate(
                encode_info,
                cfgChange.bitRate);
            printf("Change bitrate to %d on %d \n",
                cfgChange.bitRate, frame_num);
        }
//        if (cfgChange.enable_option
//            & CHANGE_MIN_MAX_QP)
//        {
//            vl_video_encoder_change_qp(
//                handle_enc,
//                cfgChange.minQpI,cfgChange.maxQpI,
//                cfgChange.maxDeltaQp,
//                cfgChange.minQpP,cfgChange.maxQpP,
//                cfgChange.minQpB,cfgChange.maxQpB);
//            printf("Change QP table on %d QPs: ",
//                frame_num);
//            printf("%d %d %d %d %d %d %d\n",
//                cfgChange.minQpI,cfgChange.maxQpI,
//                cfgChange.maxDeltaQp,
//                cfgChange.minQpP,cfgChange.maxQpP,
//                cfgChange.minQpB,cfgChange.maxQpB);
//            playStat.minQpI_base = cfgChange.minQpI;
//            playStat.maxQpI_base = cfgChange.maxQpI;
//            playStat.maxDeltaQp_base =
//                     cfgChange.maxDeltaQp;
//            playStat.minQpP_base = cfgChange.minQpP;
//            playStat.maxQpP_base = cfgChange.maxQpP;
//            playStat.minQpB_base = cfgChange.minQpB;
//            playStat.maxQpB_base = cfgChange.maxQpB;
//        }
        if (cfgChange.enable_option
            & CHANGE_GOP_PERIOD)
        {
            vl_video_encoder_change_gop(
                encode_info,
                cfgChange.intraQP,
                cfgChange.GOPPeriod);
            printf("Change gop on %d QP %d %d\n",
                frame_num, cfgChange.intraQP,
                cfgChange.GOPPeriod);
//            playStat.CurGOPSize =  cfgChange.GOPPeriod;
//            if (playStat.CurGOPCounter >= playStat.CurGOPSize)
//                playStat.CurGOPCounter = 0;
        }
//        if ((cfgChange.enable_option
//            & LONGTERM_REF_SET) && ltf_enabled)
//        {
//            vl_video_encoder_longterm_ref(
//                handle_enc,
//                cfgChange.LTRFlags);
//            printf("Longterm Ref on %d flag 0x%x\n",
//                frame_num, cfgChange.LTRFlags);
//        }
//        if (cfgChange.enable_option
//            & CUST_SIZE_SMOOTH)
//        {
//            if (cfgChange.smooth_level) {
//                playStat.cust_size_smooth_enable = 1;
//                if (cfgChange.smooth_level< 100)
//                playStat.smooth_qpdelta = 4;
//                else
//                playStat.smooth_qpdelta = 6;
//            } else {
//                playStat.cust_size_smooth_enable = 0;
//                playStat.smooth_qpdelta = 0;
//                playStat.qp_ltr = -1;
//                playStat.qp_idr = -1;
//            }
//            printf("Set CustSizeSmooth on %d level %d \n",
//                   frame_num, cfgChange.smooth_level);
//        }
//        if ((cfgChange.enable_option
//            & CUST_APP_H264_PLUS) && ltf_enabled)
//        {
//            if (cfgChange.newIDRInterval) { //enable
//            if (playStat.cust_ltr_enable == 0) {
//                playStat.cust_ltr_enable = 1;
//                playStat.GopLTRInterval = playStat.CurGOPSize;
//                playStat.GopLTRCounter = playStat.CurGOPCounter;
//            }
//            if (playStat.CurGOPSize != cfgChange.newIDRInterval)
//            {
//                vl_video_encoder_change_gop(
//                handle_enc,
//                playStat.PrevFrameQP,
//                cfgChange.newIDRInterval
//                );
//                printf("Gop change on %d  to %d qp %d\n",
//                frame_num, cfgChange.newIDRInterval,
//                playStat.PrevFrameQP);
//            }
//            playStat.CurGOPSize = cfgChange.newIDRInterval;
//            playStat.QPdelta_IDR = cfgChange.QPdelta_IDR;
//            playStat.QPdelta_LTR = cfgChange.QPdelta_LTR;
//            if ( playStat.CurGOPCounter >= playStat.CurGOPSize)
//                playStat.CurGOPCounter = 0;
//            if (playStat.GopLTRCounter >= playStat.GopLTRInterval)
//                playStat.GopLTRCounter = 0;
//            } else {
//                if (playStat.cust_ltr_enable == 1) {
//                playStat.cust_ltr_enable = 0;
//                playStat.CurGOPSize = playStat.GopLTRInterval;
//                vl_video_encoder_change_gop(
//                    handle_enc,
//                    playStat.PrevFrameQP,
//                    playStat.CurGOPSize);
//                printf("Gop change on %d  to %d qp %d\n",
//                    frame_num,
//                    playStat.CurGOPSize,
//                    playStat.PrevFrameQP);
//                if (playStat.CurGOPCounter >= playStat.CurGOPSize)
//                    playStat.CurGOPCounter = 0;
//                playStat.GopLTRInterval = 0;
//                playStat.GopLTRCounter = 0;
//                playStat.QPdelta_IDR = 0;
//                playStat.QPdelta_LTR = 0;
//                playStat.qp_ltr = -1;
//                playStat.qp_idr = -1;
//                }
//            }
//            printf("Set Cust APP H264+ on %d interval %d qp delta idr %d, ltr %d \n",
//                frame_num, cfgChange.newIDRInterval,
//                cfgChange.QPdelta_IDR,
//                cfgChange.QPdelta_LTR);
//        }
//        if (cfgChange.enable_option
//            & CHANGE_MULTI_SLICE)
//        {
//            vl_video_encoder_change_multi_slice(
//                handle_enc,
//                cfgChange.Slice_Mode,
//                cfgChange.Slice_Para);
//            printf("Change slice on %d mode %d %d\n",
//                   frame_num,
//                   cfgChange.Slice_Mode,
//                   cfgChange.Slice_Para);
//        }
        if (cfgChange.enable_option
            & CHANGE_STRICT_RC)
        {
            vl_video_encoder_change_strict_rc(
                encode_info,
                cfgChange.bitrate_window,
                cfgChange.pskip_threshold);
            printf("Change strict rate control on %d window %d threshold %d\n",
                   frame_num,
                   cfgChange.bitrate_window,
                   cfgChange.pskip_threshold);
        }
    }
    if (cfgChange.FrameNum <= frame_num) {
        cfg_encode->has_cfg_update = ParseCfgUpdateFile(fp_cfg, &cfgChange);
    }
    return 0;
}

static void *encode_thread(void *param)
{
    int chn_id;
    int outfd = -1;
    int ret = 0;
    int i = 0;
    int k = 0;
    FILE *fp = NULL;
    FILE *fp_osdcfg = NULL;
    FILE *fp_osd[MAX_OSD_NUM];
    FILE *fp_ds = NULL;
    FILE *fp_crop = NULL;
    FILE *fp_cfg = NULL;
    int dsfd = -1;
    unsigned char *outbuffer = NULL;
    unsigned int outbuffer_len = 0;
    unsigned int dataLength = 0;
    unsigned char *inputBuffer = NULL;
    unsigned char *osd_buffer[MAX_OSD_NUM];
    venc_buffer_t pFbSrcMem;
    venc_buffer_t posdMem[MAX_OSD_NUM];
    venc_buffer_t bsBuffer[3];
    unsigned int framesize;
    unsigned int osdsize;
    unsigned ysize;
    unsigned usize;
    unsigned vsize;
    unsigned uvsize;
    int intraRefresh;
    osdCfgParam cfg_osd[MAX_OSD_NUM];
    dsCfgParam cfg_ds;
    cropCfgParam cfg_crop;
    encodecCfgParam *cfg_encode = NULL;
    CfgChangeParam cfgChange;
    //vl_codec_handle_t handle_enc = 0;
    handle_t encode_info;
    signed char frame_type = -1;

    if (NULL != param)
        cfg_encode = (encodecCfgParam *)param;
    else {
        goto end;
    }

    switch (cfg_encode->codec_id)
    {
        case 4:
            cfg_encode->codec_id = CODEC_ID_H264;
            break;
        case 5:
            cfg_encode->codec_id = CODEC_ID_H265;
            break;
        default:
            break;
    }

    if ((cfg_encode->framerate < 0) || (cfg_encode->framerate > FRAMERATE))
    {
        printf("invalid framerate %d \n", cfg_encode->framerate);
        goto end;
    }

    if (cfg_encode->bitrate <= 0)
    {
        printf("invalid bitrate \n");
        goto end;
    }

    if (cfg_encode->num < 0)
    {
        printf("invalid num \n");
        goto end;
    }

    cfg_encode->num_planes = 1;
    if (cfg_encode->buf_type == 3 && (cfg_encode->num_planes < 1 || cfg_encode->num_planes > 3))
    {
        printf("invalid num_planes \n");
        goto end;
    }

    if (cfg_encode->src_buf_stride && cfg_encode->src_buf_stride < cfg_encode->width) {
        printf("incorrect cust source stride %d width %d",
            cfg_encode->src_buf_stride, cfg_encode->width);
        goto end;
    }

    printf("src_url is\t: %s ;\n", cfg_encode->srcfile);
    printf("out_url is\t: %s ;\n", cfg_encode->outfile);
    printf("width   is\t: %d ;\n", cfg_encode->width);
    printf("height  is\t: %d ;\n", cfg_encode->height);
    printf("gop     is\t: %d ;\n", cfg_encode->gop);
    printf("frmrate is\t: %d ;\n", cfg_encode->framerate);
    printf("bitrate is\t: %d ;\n", cfg_encode->bitrate);
    printf("frm_num is\t: %d ;\n", cfg_encode->num);
    printf("fmt     is\t: %d ;\n", cfg_encode->fmt);
    printf("buf_type is\t: %d ;\n", cfg_encode->buf_type);
    printf("num_planes is\t: %d ;\n", cfg_encode->num_planes);
    printf("codec is\t: %d ;\n", cfg_encode->codec_id);

    if (cfg_encode->codec_id == CODEC_ID_H265)
        printf("codec is H265\n");
    else
        printf("codec is H264\n");

    outbuffer_len = 8 * 1024 * 1024 * sizeof(char);
    outbuffer = (unsigned char *) malloc(outbuffer_len);
    if (outbuffer == NULL) {
        printf("Can not malloc outbuffer\n");
        goto end;
    }
    if (cfg_encode->src_buf_stride) {
        if (cfg_encode->fmt == IMG_FMT_RGB888)
        {
            framesize = cfg_encode->src_buf_stride * cfg_encode->height * 3;
        }
        else if (cfg_encode->fmt == IMG_FMT_RGBA8888)
        {
            framesize = cfg_encode->src_buf_stride * cfg_encode->height * 4;
        }
        else
        {
            framesize = cfg_encode->src_buf_stride * cfg_encode->height * 3 / 2;
        }
        ysize = cfg_encode->src_buf_stride * cfg_encode->height;
        usize = cfg_encode->src_buf_stride * cfg_encode->height / 4;
        vsize = cfg_encode->src_buf_stride * cfg_encode->height / 4;
        uvsize = cfg_encode->src_buf_stride * cfg_encode->height / 2;
    } else {
        if (cfg_encode->fmt == IMG_FMT_RGB888)
        {
            framesize = cfg_encode->width * cfg_encode->height * 3;
        }
        else if (cfg_encode->fmt == IMG_FMT_RGBA8888)
        {
            framesize = cfg_encode->width * cfg_encode->height * 4;
        }
        else
        {
            framesize = cfg_encode->width * cfg_encode->height * 3 / 2;
        }
        ysize = cfg_encode->width * cfg_encode->height;
        usize = cfg_encode->width * cfg_encode->height / 4;
        vsize = cfg_encode->width * cfg_encode->height / 4;
        uvsize = cfg_encode->width * cfg_encode->height / 2;
    }

    inputBuffer = (unsigned char *) malloc(framesize);
    if (inputBuffer == NULL) {
        printf("Can not allocat inputBuffer\n");
        goto exit;
    }
    fp = fopen((const char*)cfg_encode->srcfile, "rb");
    if (fp == NULL)
    {
        printf("open src file error!\n");
        goto exit;
    }

    if (cfg_encode->cfg_upd_enabled) {
        fp_cfg = fopen((const char*)cfg_encode->cfg_file, "r");
        if (fp_cfg == NULL)
        {
            printf("open cfg file error!\n");
            goto exit;
        }
        cfg_encode->has_cfg_update = ParseCfgUpdateFile(fp_cfg, &cfgChange);
        printf("==%s, %d, cfgChange.bitRate:%d\n", __func__, __LINE__, cfgChange.bitRate);
        printf("==%s, %d, cfgChange.picType:%d\n", __func__, __LINE__, cfgChange.picType);
        printf("==%s, %d, minQpI:%d, maxQpI:%d, minQpP:%d, maxQpP:%d\n", __func__, __LINE__, cfgChange.minQpI, cfgChange.maxQpI, cfgChange.minQpP, cfgChange.maxQpP);
        printf("==%s, %d, cfgChange.GOPPeriod:%d\n", __func__, __LINE__, cfgChange.GOPPeriod);
        printf("==%s, %d, cfgChange.bitrate_window:%d\n", __func__, __LINE__, cfgChange.bitrate_window);
    }
    if (cfg_encode->osd_enabled) {
        fp_osdcfg = fopen((const char*)cfg_encode->osdfile, "r");
        if (fp_osdcfg == NULL)
        {
            printf("open osd config file error!\n");
            goto exit;
        }
        memset(cfg_osd, 0, MAX_OSD_NUM * sizeof(osdCfgParam));
        ParseOsdCfgFile(fp_osdcfg, cfg_osd);
        fclose(fp_osdcfg);
    }
    if (cfg_encode->ds_enabled) {
        fp_ds = fopen((const char*)cfg_encode->dsfile, "r");
        if (fp_ds == NULL)
        {
             printf("open ds config file error!\n");
             goto exit;
        }
        memset(&cfg_ds, 0, sizeof(dsCfgParam));
        ParseDsCfgFile(fp_ds, &cfg_ds);
        fclose(fp_ds);
    }

    if (cfg_encode->crop_enabled) {
        fp_crop = fopen((const char*)cfg_encode->cropfile, "r");
        if (fp_crop == NULL)
        {
             printf("open crop config file error!\n");
             goto exit;
        }
        memset(&cfg_crop, 0, sizeof(cropCfgParam));
        ParseCropCfgFile(fp_crop, &cfg_crop);
        fclose(fp_crop);
    }
    outfd = open((const char*)cfg_encode->outfile, O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (outfd < 0)
    {
        printf("open dest file error!\n");
        goto exit;
    }

    memset(&encode_info, 0, sizeof(handle_t));
    encode_info.init_qp = 30;
    encode_info.enc_rc_mode = 2;
    encode_info.chn_id = cfg_encode->chnid;
    encode_info.enc_fmt = cfg_encode->codec_id;  // 0: h264 1: h265
    encode_info.enc_width = cfg_encode->width;
    encode_info.enc_height = cfg_encode->height;
    encode_info.poc = 0;
    encode_info.enc_fps = cfg_encode->framerate;
    encode_info.enc_gop = cfg_encode->gop;
    encode_info.buf_stride = cfg_encode->src_buf_stride;
    encode_info.level = cfg_encode->level;
    encode_info.tier = cfg_encode->tier;
    encode_info.enc_bitrate = cfg_encode->bitrate;
    if (cfg_encode->rc_mode)
        encode_info.enc_rc_mode = cfg_encode->rc_mode;
    if (cfg_encode->buf_type == 0)
        encode_info.golden_match_flag = 1;

    if (cfg_encode->codec_id == CODEC_ID_H265) {
        if (cfg_encode->profile == 1)
            encode_info.profile = HEVC_MAIN;
        else
            encode_info.profile = 0;
    } else {
        if (cfg_encode->profile == 1)
            encode_info.profile = AVC_BASELINE;
        else if (cfg_encode->profile == 2)
            encode_info.profile = AVC_MAIN;
        else if (cfg_encode->profile == 3)
            encode_info.profile = AVC_HIGH;
        else
            encode_info.profile = 0;
    }

    if (cfg_encode->forcePicQpI && cfg_encode->forcePicQpPB) {
        encode_info.fix_qp_I = cfg_encode->forcePicQpI;
        encode_info.fix_qp_P = cfg_encode->forcePicQpPB;
    }

    if (cfg_encode->frame_rotation == 0)
        encode_info.rotation_mode = 0;
    else if (cfg_encode->frame_rotation == 1)
        encode_info.rotation_mode = 1;
    else if (cfg_encode->frame_rotation == 2)
        encode_info.rotation_mode = 2;
    else if (cfg_encode->frame_rotation == 3)
        encode_info.rotation_mode = 3;
    else {
        printf("frame rotation angle is wrong\n");
        encode_info.rotation_mode = 0;
    }
    if (cfg_encode->frame_mirroring <= 3)
        encode_info.mirror_mode = cfg_encode->frame_mirroring;
    else
        encode_info.mirror_mode = 0;
    if (cfg_encode->osd_enabled) {
        encode_info.tahoe_enc_pic.osd_enable = 1;
        printf("[%s][%d]cfg_osd[0].osdNum:%d\n",__func__,__LINE__, cfg_osd[0].osdNum);
        for (i = 0; i < cfg_osd[0].osdNum; i ++)    {
            if (i >= MAX_OSD_NUM)
                break;
            switch (cfg_osd[i].format) /*0:ARGB8888 1:ARGB4444 2:ARGB1555 3:NV21 4:NV12 5:BITMAP*/
            {
                case 0:
                    osdsize = cfg_osd[i].w * cfg_osd[i].h * 4;
                    encode_info.tahoe_enc_pic.osd_config[i].format = OSD_FMT_ARGB8888;
                    encode_info.tahoe_enc_pic.osd_config[i].stride = cfg_osd[i].w * 4;
                    break;
                case 1:
                    osdsize = cfg_osd[i].w * cfg_osd[i].h * 2;
                    encode_info.tahoe_enc_pic.osd_config[i].format = OSD_FMT_ARGB4444;
                    encode_info.tahoe_enc_pic.osd_config[i].stride = cfg_osd[i].w * 2;
                    break;
                case 2:
                    osdsize = cfg_osd[i].w * cfg_osd[i].h * 2;
                    encode_info.tahoe_enc_pic.osd_config[i].format = OSD_FMT_ARGB1555;
                    encode_info.tahoe_enc_pic.osd_config[i].stride = cfg_osd[i].w * 2;
                    break;
                case 3:
                    osdsize = cfg_osd[i].w * cfg_osd[i].h * 3 / 2;
                    encode_info.tahoe_enc_pic.osd_config[i].format = OSD_FMT_NV21;
                    encode_info.tahoe_enc_pic.osd_config[i].stride = cfg_osd[i].w;
                    break;
                case 4:
                    osdsize = cfg_osd[i].w * cfg_osd[i].h * 3 / 2;
                    encode_info.tahoe_enc_pic.osd_config[i].format = OSD_FMT_NV12;
                    encode_info.tahoe_enc_pic.osd_config[i].stride = cfg_osd[i].w;
                    break;
                case 5:
                    osdsize = cfg_osd[i].w * cfg_osd[i].h / 8;
                    encode_info.tahoe_enc_pic.osd_config[i].format = OSD_FMT_BITMAP;
                    encode_info.tahoe_enc_pic.osd_config[i].stride = cfg_osd[i].w / 8;
                    break;
                default:
                    break;
            }
            encode_info.tahoe_enc_pic.osd_config[i].enable = 1;
            //encode_info.tahoe_enc_pic.osd_config[i].format = cfg_osd[i].format;
            encode_info.tahoe_enc_pic.osd_config[i].alpha_mode = cfg_osd[i].alpha_mode;
            encode_info.tahoe_enc_pic.osd_config[i].r2y_mode = cfg_osd[i].r2y_mode;
            encode_info.tahoe_enc_pic.osd_config[i].endian = 0;
            encode_info.tahoe_enc_pic.osd_config[i].y_baseaddr_h2bit = 0;
            encode_info.tahoe_enc_pic.osd_config[i].c_baseaddr_h2bit = 0;
            encode_info.tahoe_enc_pic.osd_config[i].cmn_rsv = 0;
            encode_info.tahoe_enc_pic.osd_config[i].region_x = cfg_osd[i].x;
            encode_info.tahoe_enc_pic.osd_config[i].region_y = cfg_osd[i].y;
            encode_info.tahoe_enc_pic.osd_config[i].region_width = cfg_osd[i].w;
            encode_info.tahoe_enc_pic.osd_config[i].region_height = cfg_osd[i].h;
            encode_info.tahoe_enc_pic.osd_config[i].alpha = cfg_osd[i].alpha;
            encode_info.tahoe_enc_pic.osd_config[i].bitmap_y = cfg_osd[i].bitmap_y;
            encode_info.tahoe_enc_pic.osd_config[i].bitmap_u = cfg_osd[i].bitmap_u;
            encode_info.tahoe_enc_pic.osd_config[i].bitmap_v = cfg_osd[i].bitmap_v;
            //encode_info.tahoe_enc_pic.osd_config[i].y_baseaddr = 0;
            encode_info.tahoe_enc_pic.osd_config[i].c_baseaddr = 0;
            //encode_info.tahoe_enc_pic.osd_config[i].stride = 1;
            memset(&posdMem[i], 0, sizeof(venc_buffer_t));
            posdMem[i].size = osdsize;
            posdMem[i].cached = 1;
            if (vdi_allocate_dma_memory(&posdMem[i]) < 0) {
                printf("Can not allocat dma Buffer\n");
                goto end;
            }
            fp_osd[i] = fopen((const char*)cfg_osd[i].osdfile, "rb");
            if (fp_osd[i] == NULL)
            {
                printf("open osd file %d error!\n", i);
                goto exit;
            }
            osd_buffer[i] = (unsigned char *) malloc(osdsize);
            if (osd_buffer[i] == NULL) {
                printf("Can not allocat roi_buffer\n");
                goto end;
            }
        }
    }

    if (cfg_encode->ds_enabled) {
        encode_info.ds_only = cfg_ds.ds_only;
        encode_info.ds_osd = cfg_ds.ds_osd;
        encode_info.ds_w = cfg_ds.ds_w;
        encode_info.ds_h = cfg_ds.ds_h;
        dsfd = open((const char*)cfg_ds.dsfile, O_CREAT | O_RDWR | O_TRUNC, 0644);
        if (dsfd < 0)
        {
            printf("open ds file error!\n");
            goto end;
        }
    }
    if (cfg_encode->crop_enabled) {
        encode_info.crop_enable = cfg_encode->crop_enabled;
        encode_info.crop_x = cfg_crop.x;
        encode_info.crop_y = cfg_crop.y;
        encode_info.crop_w = cfg_crop.w;
        encode_info.crop_h = cfg_crop.h;
    }
    if (cfg_encode->fmt == IMG_FMT_NV12) {
        printf("img_format is IMG_FMT_NV12 \n");
        encode_info.mfdin_input_fmt = MFDI_INPUT_FMT_NV12;
    } else if (cfg_encode->fmt == IMG_FMT_NV21) {
        printf("img_format is IMG_FMT_NV21 \n");
        encode_info.mfdin_input_fmt = MFDI_INPUT_FMT_NV21;
    } else if (cfg_encode->fmt == IMG_FMT_YUV420P) {
        printf("img_format is IMG_FMT_YUV420P \n");
        encode_info.mfdin_input_fmt = MFDI_INPUT_FMT_YUV420_PLANAR;
    } else if (cfg_encode->fmt == IMG_FMT_RGB888) {
        printf("img_format is IMG_FMT_RGB888 \n");
        encode_info.mfdin_input_fmt = MFDI_INPUT_FMT_RGB888;
    } else if (cfg_encode->fmt == IMG_FMT_RGBA8888) {
        printf("img_format is IMG_FMT_RGBA8888 \n");
        encode_info.mfdin_input_fmt = MFDI_INPUT_FMT_RGBA8888;
    }

    memset(&pFbSrcMem, 0, sizeof(pFbSrcMem));
    pFbSrcMem.size = framesize;
    pFbSrcMem.cached = 1;

    if (vdi_allocate_dma_memory(&pFbSrcMem) < 0) {
        printf("Can not allocat dma Buffer chnid %d\n", encode_info.chn_id);
        goto end;
    }
    //for vps/sps/pps
    for (i = 0; i < 3; i++) {
        memset(&bsBuffer[i], 0, sizeof(bsBuffer[i]));
        bsBuffer[i].size = ENC_STREAM_BUF_SIZE;
        if (vdi_allocate_dma_memory(&bsBuffer[i]) < 0) {
            printf("Can not allocat dma Buffer chnid %d\n", encode_info.chn_id);
            goto end;
        }
        printf("[%s][%d]bsBuffer[%d].phys_addr:0x%x\n", \
            __func__,__LINE__, i, bsBuffer[i].phys_addr);
    }

    if (!encode_info.buf_stride)
    {
        encode_info.buf_stride = encode_info.enc_width;
    }
    printf("handle[chn_id].buf_stride is: %d \n", encode_info.buf_stride);

    encode_info.tahoe_enc_pic.inpt_buf_addr_lsb[0] = pFbSrcMem.phys_addr;
    if ((cfg_encode->fmt == IMG_FMT_NV12) || (cfg_encode->fmt == IMG_FMT_NV21)) {
        encode_info.tahoe_enc_pic.inpt_buf_addr_lsb[1] = pFbSrcMem.phys_addr + encode_info.buf_stride * encode_info.enc_height;
        encode_info.tahoe_enc_pic.inpt_buf_addr_lsb[2] = encode_info.tahoe_enc_pic.inpt_buf_addr_lsb[1];
    } else if (cfg_encode->fmt == IMG_FMT_YUV420P) {
        encode_info.tahoe_enc_pic.inpt_buf_addr_lsb[1] = pFbSrcMem.phys_addr + encode_info.buf_stride * encode_info.enc_height;
        encode_info.tahoe_enc_pic.inpt_buf_addr_lsb[2] = pFbSrcMem.phys_addr + (encode_info.buf_stride * encode_info.enc_height * 5 / 4);
    }
    encode_info.header_buf_addr_lsb[0] = bsBuffer[0].phys_addr;
    encode_info.header_buf_addr_lsb[1] = bsBuffer[1].phys_addr;
    encode_info.header_buf_addr_lsb[2] = bsBuffer[2].phys_addr;

    printf("New allocate frame address y:0x%x cb:0x%x cr:0x%x\n",
               encode_info.tahoe_enc_pic.inpt_buf_addr_lsb[0], encode_info.tahoe_enc_pic.inpt_buf_addr_lsb[1], encode_info.tahoe_enc_pic.inpt_buf_addr_lsb[2]);

    if (vl_tahoe_encoder_init(encode_info.enc_fmt, encode_info) < 0) {
        printf("vl_tahoe_encoder_init failed chnid %d\n", encode_info.chn_id);
        goto exit;
    }

    for (i = 0; i < cfg_encode->num; i++) {
        //for (int chn_id = 0; chn_id < MAX_CHANNEL; chn_id++) {
        if (fread(inputBuffer, 1, framesize, fp) != framesize)
        {
            rewind(fp);
            clearerr(fp);
            if (fread(inputBuffer, 1, framesize, fp) != framesize) {
                printf("read input file error!\n");
                goto exit;
            }
        }
        memcpy((void *)((unsigned long)pFbSrcMem.virt_addr), inputBuffer, framesize);
        /*venc_buffer_t vdb;
        memset(&vdb, 0x00, sizeof(venc_buffer_t));
        vdb.phys_addr = pFbSrcMem.phys_addr;
        vdb.size = framesize;
        printf("[%s][%d]vdb.phys_addr:0x%lx\n", __func__, __LINE__, vdb.phys_addr);
        printf("[%s][%d]vdb.size:%d\n", __func__, __LINE__, vdb.size);*/
        vdi_flush_memory(&pFbSrcMem);

        if (cfg_encode->osd_enabled) {
            for (k = 0; k < cfg_osd[0].osdNum; k++) {
                size_t n = fread(osd_buffer[k], 1, posdMem[k].size, fp_osd[k]);
                if (n != posdMem[k].size) {
                    fseek(fp_osd[k], 0, SEEK_SET);
                    clearerr(fp_osd[k]);
                    fread(osd_buffer[k] + n, 1, posdMem[k].size - n, fp_osd[k]);
                }
                memcpy((void *)((unsigned long)posdMem[k].virt_addr), osd_buffer[k], posdMem[k].size);
                vdi_flush_memory(&posdMem[k]);
                encode_info.tahoe_enc_pic.osd_config[k].y_baseaddr = posdMem[k].phys_addr;
            }
        }

        encode_info.enc_frame_type = FRAME_TYPE_AUTO;
        if (cfg_encode->cfg_upd_enabled && cfg_encode->has_cfg_update) {
            update_cfg(fp_cfg, i, &encode_info, cfg_encode, cfgChange);
        }
        if (encode_info.poc == 0 || encode_info.poc % encode_info.enc_gop == 0)
            encode_info.enc_frame_type = FRAME_TYPE_IDR;
        if (encode_info.enc_frame_type == FRAME_TYPE_IDR || encode_info.enc_frame_type == FRAME_TYPE_I)
            encode_info.poc = 0;
        if (encode_info.enc_frame_type == FRAME_TYPE_IDR || encode_info.enc_frame_type == FRAME_TYPE_I)
        {
            vl_tahoe_encoder_generate_header(&encode_info);
            ret = get_header(&encode_info, outbuffer, (unsigned int *)&dataLength);
            if (ret == 0) {
                write(outfd, (unsigned char *) outbuffer, dataLength);
            }
        }

        vl_tahoe_encoder_encode(&encode_info);
        ret = get_bsbframe(&encode_info, outbuffer, (unsigned int *)&dataLength, &frame_type);
        if (ret == 0) {
            write(outfd, (unsigned char *) outbuffer,
                  dataLength);
        }
        if (cfg_encode->ds_enabled) {
            ret = get_dsframe(&encode_info, outbuffer, (unsigned int *)&dataLength);
            if (ret == 0) {
                write(dsfd, (unsigned char *) outbuffer,
                      dataLength);
            }
        }
        encode_info.poc++;
        printf("==%s, %d, chn_id:%d num %d\n", __func__, __LINE__, encode_info.chn_id, i);
    }

exit:

    vl_tahoe_encoder_destroy(encode_info);

    if (inputBuffer)
        free(inputBuffer);
    if (outbuffer)
        free(outbuffer);
    if (outfd >= 0)
        close(outfd);
    if (fp)
        fclose(fp);

    for (i = 0; i < MAX_OSD_NUM; i++) {
        if (osd_buffer[i]) {
            free(osd_buffer[i]);
            osd_buffer[i] = NULL;
        }

        if (fp_osd[i]) {
            fclose(fp_osd[i]);
            fp_osd[i] = NULL;
        }
    }

    if (dsfd >= 0)
        close(dsfd);
    printf("Encode End! width:%d\n", cfg_encode->width);

end:
    cfg_encode->encodeEnd  = 1;
    pthread_exit(0);
    return NULL;
}

int main(int argc, char **argv)
{
    int i = 0;
    int arg_count = 0;
    int len = 0;
    FILE *fp_cfg_encode = NULL;
    encodecCfgParam cfg_encode[CFG_ENCODE_NUM_MAX];

    if ((argc != 2) && (argc < 13)) {
        printf("Amlogic Encoder API \n");
        printf(" usage: tahoe_test "
               "[srcfile][outfile][width][height][gop][framerate][bitrate][num][fmt]["
               "buf type][num_planes][codec_id] \n");
        printf("  options  \t:\n");
        printf("  srcfile  \t: yuv data url in your root fs\n");
        printf("  outfile  \t: stream url in your root fs\n");
        printf("  width    \t: width\n");
        printf("  height   \t: height\n");
        printf("  gop      \t: I frame refresh interval\n");
        printf("  framerate\t: framerate \n");
        printf("  bitrate  \t: bit rate \n");
        printf("  num      \t: encode frame count \n");
        printf("  fmt      \t: encode input fmt 1 : nv12, 2 : nv21, 3 : yuv420p(yv12/yu12), 5 : RGB888, 6 : RGBA8888\n");
        printf("  buf_type (golden_match: 0 bitmatch, 1 mismatch) \t: 0:vmalloc, 3:dma buffer\n");
        printf("  num_planes \t: used for dma buffer case. 2 : nv12/nv21, 3 : yuv420p(yv12/yu12)\n");
        printf("  codec_id \t: 4 : h.264, 5 : h.265\n");
        printf("  cfg_opt  \t: optional, flags to control the encode feature settings\n");
        printf("            \t\t  bit 0:roi control. 0: disabled (default) 1: enabled\n");
        printf("            \t\t  bit 1: update control. 0: disabled (default) 1: enabled\n");
        printf("            \t\t  bit 2 ~ bit 6 encode GOP pattern options:\n");
        printf("            \t\t\t 0 default(IP) 1:I only    2:IP_only    3: IBBBP\n");
        printf("            \t\t\t 4:IP_SVC1     5:IP_SVC2   6: IP_SVC3   7: IP_SVC4  8: CUSTP  10: IP_SVC5\n");
        printf("            \t\t  bit 7:long term references. 0: disabled (default) 1: enabled\n");
        printf("            \t\t  bit 8:cust source buffer stride . 0: disabled (default) 1: enabled\n");
        printf("            \t\t  bit 9: convert normal source file stride to cust source stride. 0: disabled (default) 1: enabled\n");
        printf("            \t\t  bit 10: enable rc mode. 1: CBR 2: VBR (default)\n");
        printf("            \t\t  bit 11: profile 0: auto (default), h.264 (1: BASELINE, 2: MAIN, 3: HIGH), h.265 (1: MAIN)\n");
        printf("            \t\t  bit 12: level 0: auto (default)\n");
        printf("            \t\t  bit 13: tier 0: auto (default)\n");
        printf("            \t\t  bit 14 ~ bit 15: frame rotation(counter-clockwise, in degree): 0:none, 1:90, 2:180, 3:270\n");
        printf("            \t\t  bit 16 ~ bit 17: frame mirror mode: bit1 : mirror;  bit2: flip;\n");
        printf("            \t\t  bit 18: enable customer bitstream buffer size\n");
        printf("            \t\t  bit 19 ~ bit 20: enable multi slice mode: 0, one slice(no para), 1 by MB(16x16)/CTU(64x64), 2 by CTU + size(byte)\n");
        printf("            \t\t  bit 21 cust_qp_delta_en \n");
        printf("            \t\t  bit 22 strict_rate_ctrl_en (skipP allowed to limit bitrate) default 0 \n");
        printf("            \t\t  bit 23 forcePicQpEnable default 0 \n");
        printf("            \t\t  bit 24: osd control. 0: disabled (default) 1: enabled\n");
        printf("            \t\t  bit 25: ds control. 0: disabled (default) 1: enabled\n");
        printf("            \t\t  bit 26: crop control. 0: disabled (default) 1: enabled\n");
        printf("  roifile  \t: optional, roi info url in root fs, no present if roi is disabled\n");
        printf("  cfg_file \t: optional, cfg update info url. no present if update is disabled\n");
        printf("  src_buf_stride \t: optional, source buffer stride\n");
        printf("  intraRefresh \t: optional, lower 4 bits for intra-refresh mode; others for refresh parameter, controlled by intraRefresh bit\n");
        printf("  bitStreamBuf \t: optional, encoded bitstream buffer size in MB\n");
        printf("  MultiSlice parameter \t: optional, multi slice parameter in MB/CTU or CTU and size (HIGH_16bits size + LOW_16bit CTU) combined\n");
        printf("  cust_qp_delta  \t: optional, cust_qp_delta apply to P frames value >0 lower; <0 increase the quality \n");
        printf("  strict rate control parameter  \t: optional, High 16 bit bitrate window (frames max 120). low 16 bit bitrate threshold (percent)\n");
        printf("  forcePicQpIPB: force qp value\n");
        printf("  osdfile \t: optional, osd info url in root fs, no present if osd is disabled\n");
        printf("  dsfile \t: optional, ds info url in root fs, no present if ds is disabled\n");
        printf("  cropfile \t: optional, crop info url in root fs, no present if crop is disabled\n");
        return -1;
    } else if (argc == 2) {
        printf("argv[1]:%s\n", argv[1]);
        fp_cfg_encode = fopen((argv[1]), "r");
        if (fp_cfg_encode == NULL) {
            printf("open encode cfg file error!\n");
            return 0;
        }
        memset(cfg_encode, 0, CFG_ENCODE_NUM_MAX * sizeof(encodecCfgParam));
        ParseEncodeCfgFile(fp_cfg_encode, cfg_encode);
        fclose(fp_cfg_encode);
        if (cfg_encode[0].seqNum == 0)
            return 0;
    } else {
        memset(&cfg_encode[0], 0, sizeof(encodecCfgParam));
        len = strlen(argv[1]) > ENCODER_STRING_LEN_MAX ? ENCODER_STRING_LEN_MAX : strlen(argv[1]);
        strncpy(cfg_encode[0].srcfile, argv[1], len);
        len = strlen(argv[2]) > ENCODER_STRING_LEN_MAX ? ENCODER_STRING_LEN_MAX : strlen(argv[2]);
        strncpy(cfg_encode[0].outfile, argv[2], len);

        cfg_encode[0].width = atoi(argv[3]);
        cfg_encode[0].height = atoi(argv[4]);

        cfg_encode[0].gop = atoi(argv[5]);
        cfg_encode[0].framerate = atoi(argv[6]);
        cfg_encode[0].bitrate = atoi(argv[7]);
        cfg_encode[0].num = atoi(argv[8]);
        cfg_encode[0].fmt = atoi(argv[9]);
        cfg_encode[0].buf_type = atoi(argv[10]);
        cfg_encode[0].num_planes = atoi(argv[11]);
        cfg_encode[0].codec_id = atoi(argv[12]);
        if (argc > 13) {
            cfg_encode[0].cfg_opt = (int)strtol(argv[13], NULL, 16);//atoi(argv[13]);
        }
        if (argc > 14) {
            arg_count = 14;
            /*if ((cfg_opt & 0x1) == 0x1 && argc > arg_count) {
                if (strlen(argv[arg_count]) >= ENCODER_STRING_LEN_MAX) {
                    printf("Warning: roi filename too long (max length: %d), will be truncated\n",
                            ENCODER_STRING_LEN_MAX - 1);
                }
                snprintf(roifile, ENCODER_STRING_LEN_MAX, "%s", argv[arg_count]);
                printf("roi_url is\t: %s ;\n", roifile);
                arg_count ++;
            }*/
            if ((cfg_encode[0].cfg_opt & 0x2) == 0x2 && argc > arg_count) {
                if (strlen(argv[arg_count]) >= ENCODER_STRING_LEN_MAX) {
                    printf("Warning: config filename too long (max length: %d), will be truncated\n",
                            ENCODER_STRING_LEN_MAX - 1);
                }
                cfg_encode[0].cfg_upd_enabled = 1;
                snprintf(cfg_encode[0].cfg_file, ENCODER_STRING_LEN_MAX, "%s", argv[arg_count]);
                printf("cfg_upd_url is\t: %s ;\n", cfg_encode[0].cfg_file);
                arg_count ++;
            }
            if ((cfg_encode[0].cfg_opt & 0x100) == 0x100 && argc > arg_count) {
                cfg_encode[0].src_buf_stride = atoi(argv[arg_count]);
                arg_count ++;
            }
            if ((cfg_encode[0].cfg_opt >> 10) & 0x1 && argc > arg_count) {
                cfg_encode[0].rc_mode = atoi(argv[arg_count]);
                printf("rc_mode %d\n", cfg_encode[0].rc_mode);
                arg_count ++;
            }
            if ((cfg_encode[0].cfg_opt >> 11) & 0x1 && argc > arg_count) {
                cfg_encode[0].profile = atoi(argv[arg_count]);
                printf("profile %d\n", cfg_encode[0].profile);
                arg_count ++;
            }
            if ((cfg_encode[0].cfg_opt >> 12) & 0x1 && argc > arg_count) {
                cfg_encode[0].level = atoi(argv[arg_count]);
                printf("level %d\n", cfg_encode[0].level);
                arg_count ++;
            }
            if ((cfg_encode[0].cfg_opt >> 13) & 0x1 && argc > arg_count) {
                cfg_encode[0].tier = atoi(argv[arg_count]);
                printf("tier %d\n",cfg_encode[0].tier);
                arg_count ++;
            }
            /*if ((cfg_opt & 0x400) == 0x400 && argc > arg_count) {
                intraRefresh = atoi(argv[arg_count]);
                printf("IntraRefresh mode %d, arg %d;\n",
                    (intraRefresh & 0xf),
                    (intraRefresh >> 4));
                arg_count ++;
            }
            if ((cfg_opt & 0x40000) == 0x40000 && argc > arg_count) {
                bitStreamBuf = atoi(argv[arg_count]);
                printf("bitstream_buf_size: %d \n",
                    bitStreamBuf);
                arg_count ++;
            }
            if ((cfg_opt >> 19) & 0x3 && argc > arg_count)    {
                MultiSlice_parameter = atoi(argv[arg_count]);
                printf("Multi-slices para %d \n",
                    MultiSlice_parameter);
                arg_count ++;
            }
            if ((cfg_opt >> 21) & 0x1 && argc > arg_count) {
                cust_qp_delta = atoi(argv[arg_count]);
                printf("cust_qp_delta %d \n",
                    cust_qp_delta);
                arg_count ++;
            }
            if ((cfg_opt >> 22) & 0x1 && argc > arg_count) {
                strict_rate_control_parameter  = atoi(argv[arg_count]);
                printf("strict_rate_control_parameter %d\n",
                       strict_rate_control_parameter);
                printf("bit-rate window %d frames, skip_threshold %d\n",
                       strict_rate_control_parameter>>16,
                       strict_rate_control_parameter& 0xffff);
                arg_count ++;
            }*/
            if ((cfg_encode[0].cfg_opt >> 23) & 0x1 && argc > arg_count) {
                cfg_encode[0].forcePicQpI = atoi(argv[arg_count]); arg_count++;
                cfg_encode[0].forcePicQpPB = atoi(argv[arg_count]);
                printf("forcePicQpI %d, forcePicQpPB %d\n", cfg_encode[0].forcePicQpI, cfg_encode[0].forcePicQpPB);
                arg_count ++;
            }
            if ((cfg_encode[0].cfg_opt >> 24) & 0x1 && argc > arg_count) {
                if (strlen(argv[arg_count]) >= ENCODER_STRING_LEN_MAX) {
                    printf("Warning: roi filename too long (max length: %d), will be truncated\n",
                            ENCODER_STRING_LEN_MAX - 1);
                }
                cfg_encode[0].osd_enabled = 1;
                snprintf(cfg_encode[0].osdfile, ENCODER_STRING_LEN_MAX, "%s", argv[arg_count]);
                printf("osdfile is\t: %s ;\n", cfg_encode[0].osdfile);
                arg_count ++;
            }
            if ((cfg_encode[0].cfg_opt >> 25) & 0x1 && argc > arg_count) {
                if (strlen(argv[arg_count]) >= ENCODER_STRING_LEN_MAX) {
                    printf("Warning: ds filename too long (max length: %d), will be truncated\n",
                            ENCODER_STRING_LEN_MAX - 1);
                }
                cfg_encode[0].ds_enabled = 1;
                snprintf(cfg_encode[0].dsfile, ENCODER_STRING_LEN_MAX, "%s", argv[arg_count]);
                printf("dsfile is\t: %s ;\n", cfg_encode[0].dsfile);
                arg_count ++;
            }
            if ((cfg_encode[0].cfg_opt >> 26) & 0x1 && argc > arg_count) {
                if (strlen(argv[arg_count]) >= ENCODER_STRING_LEN_MAX) {
                    printf("Warning: ds filename too long (max length: %d), will be truncated\n",
                            ENCODER_STRING_LEN_MAX - 1);
                }
                cfg_encode[0].crop_enabled = 1;
                snprintf(cfg_encode[0].cropfile, ENCODER_STRING_LEN_MAX, "%s", argv[arg_count]);
                printf("cropfile is\t: %s ;\n", cfg_encode[0].cropfile);
                arg_count ++;
            }
            if (arg_count != argc) {
                printf("config no match conf %d argc %d\n",
                    cfg_encode[0].cfg_opt, argc);
                return -1;
            }
        }
        if (cfg_encode[0].cfg_opt > 0) {
            cfg_encode[0].frame_rotation = (cfg_encode[0].cfg_opt >> 14) & 0x03;
            cfg_encode[0].frame_mirroring = (cfg_encode[0].cfg_opt >> 16) & 0x03;
            printf("frame_rotation=%d, frame_mirroring=%d\n",
                   cfg_encode[0].frame_rotation, cfg_encode[0].frame_mirroring);
        }
        cfg_encode[0].seqNum = 1;
    }

    encoder_open("fw.bin");
    printf("===========%s, %d, cfg_encode[0].seqNum:%d\n", __func__, __LINE__, cfg_encode[0].seqNum);

    pthread_t tid[CFG_ENCODE_NUM_MAX];
    for (i = 0; i < cfg_encode[0].seqNum; i ++) {
        if (i >= CFG_ENCODE_NUM_MAX)
            break;
        cfg_encode[i].chnid = i;
        pthread_create(&tid[i], NULL, encode_thread, &cfg_encode[i]);
    }
    for (i = 0; i < cfg_encode[0].seqNum; i ++) {
        if (i >= CFG_ENCODE_NUM_MAX)
            break;
        pthread_join(tid[i], NULL);
    }
    encoder_close();
    return 0;
}

//#osdfile format x y w h alpha alpha_mode r2ymode bitmap_y bitmap_u bitmap_v
//#format 0:ARGB8888 1:ARGB4444 2:ARGB1555 3:NV21 4:NV12 5:BITMAP
static int ParseOsdCfgFile(FILE *fp, osdCfgParam *cfg_osd)
{
    char* token = NULL;
    const char *delim = " ";
    static char lineStr[ENCODER_STRING_LEN_MAX*4] = {0};
    char valueStr[ENCODER_STRING_LEN_MAX] = {0};
    int i = 0;
    while (1) {
        if (i >= MAX_OSD_NUM)
            return 0;
        if (lineStr[0] == 0x0) { //need a new line;
            if (fgets(lineStr, ENCODER_STRING_LEN_MAX*4, fp) == NULL) {
                if (feof(fp))
                    return 0;
                continue;
            }
        }
        if (strlen(lineStr) == 1) { // skip null
            lineStr[0] = 0x0;
            continue;
        }

        if ((lineStr[0] == '#')
        ||(lineStr[0] == ';')||(lineStr[0] == ':')) { // skip comments
            lineStr[0] = 0x0;
            continue;
        }

        printf("[%s][%d]:%s\n",__func__,__LINE__,lineStr);
        // keep the original
        memcpy(valueStr, lineStr, strlen(lineStr));
        //param is separated by ' '
        token = strtok(valueStr, delim);
        if (token == NULL) {
            lineStr[0] = 0x0;
            continue;
        }
        snprintf(cfg_osd[i].osdfile, ENCODER_STRING_LEN_MAX, "%s", token);

        token = strtok(NULL, delim);
        if (token == NULL) {
            lineStr[0] = 0x0;
            continue;
        }
        cfg_osd[i].format = atoi(token);

        token = strtok(NULL, delim);
        if (token == NULL) {
            lineStr[0] = 0x0;
            continue;
        }
        cfg_osd[i].x = atoi(token);

        token = strtok(NULL, delim);
        if (token == NULL) {
            lineStr[0] = 0x0;
            continue;
        }
        cfg_osd[i].y = atoi(token);

        token = strtok(NULL, delim);
        if (token == NULL) {
            lineStr[0] = 0x0;
            continue;
        }
        cfg_osd[i].w = atoi(token);

        token = strtok(NULL, delim);
        if (token == NULL) {
            lineStr[0] = 0x0;
            continue;
        }
        cfg_osd[i].h = atoi(token);

        token = strtok(NULL, delim);
        if (token == NULL) {
            lineStr[0] = 0x0;
            continue;
        }
        cfg_osd[i].alpha = atoi(token);

        token = strtok(NULL, delim);
        if (token == NULL) {
            lineStr[0] = 0x0;
            continue;
        }
        cfg_osd[i].alpha_mode = atoi(token);

        token = strtok(NULL, delim);
        if (token == NULL) {
            lineStr[0] = 0x0;
            continue;
        }
        cfg_osd[i].r2y_mode = atoi(token);

        token = strtok(NULL, delim);
        if (token == NULL) {
            lineStr[0] = 0x0;
            continue;
        }
        cfg_osd[i].bitmap_y = atoi(token);

        token = strtok(NULL, delim);
        if (token == NULL) {
            lineStr[0] = 0x0;
            continue;
        }
        cfg_osd[i].bitmap_u = atoi(token);

        token = strtok(NULL, delim);
        if (token == NULL) {
            lineStr[0] = 0x0;
            continue;
        }
        cfg_osd[i].bitmap_v = atoi(token);

        i++;
        cfg_osd[0].osdNum++;
        lineStr[0] = 0x0;
        continue;
    }
    return 0;
}
//#dsfile ds_only ds_osd w h
static int ParseDsCfgFile(FILE *fp, dsCfgParam *cfg_ds)
{
    char* token = NULL;
    const char *delim = " ";
    static char lineStr[ENCODER_STRING_LEN_MAX*4] = {0};
    char valueStr[ENCODER_STRING_LEN_MAX] = {0};
    while (1) {
        if (lineStr[0] == 0x0) { //need a new line;
            if (fgets(lineStr, ENCODER_STRING_LEN_MAX*4, fp) == NULL) {
                if (feof(fp))
                    return 0;
                continue;
            }
        }
        if (strlen(lineStr) == 1) { // skip null
            lineStr[0] = 0x0;
            continue;
        }

        if ((lineStr[0] == '#')
        ||(lineStr[0] == ';')||(lineStr[0] == ':')) { // skip comments
            lineStr[0] = 0x0;
            continue;
        }

        printf("[%s][%d]:%s\n",__func__,__LINE__,lineStr);
        // keep the original
        memcpy(valueStr, lineStr, strlen(lineStr));
        //param is separated by ' '
        token = strtok(valueStr, delim);
        if (token == NULL) {
            lineStr[0] = 0x0;
            continue;
        }
        snprintf(cfg_ds->dsfile, ENCODER_STRING_LEN_MAX, "%s", token);

        token = strtok(NULL, delim);
        if (token == NULL) {
            lineStr[0] = 0x0;
            continue;
        }
        cfg_ds->ds_only = atoi(token);

        token = strtok(NULL, delim);
        if (token == NULL) {
            lineStr[0] = 0x0;
            continue;
        }
        cfg_ds->ds_osd = atoi(token);

        token = strtok(NULL, delim);
        if (token == NULL) {
            lineStr[0] = 0x0;
            continue;
        }
        cfg_ds->ds_w = atoi(token);

        token = strtok(NULL, delim);
        if (token == NULL) {
            lineStr[0] = 0x0;
            continue;
        }
        cfg_ds->ds_h = atoi(token);

        lineStr[0] = 0x0;
        continue;
    }
    return 0;
}

//#cropfile x y w h
static int ParseCropCfgFile(FILE *fp, cropCfgParam *cfg_crop)
{
    char* token = NULL;
    const char *delim = " ";
    static char lineStr[ENCODER_STRING_LEN_MAX*4] = {0};
    char valueStr[ENCODER_STRING_LEN_MAX] = {0};
    while (1) {
        if (lineStr[0] == 0x0) { //need a new line;
            if (fgets(lineStr, ENCODER_STRING_LEN_MAX*4, fp) == NULL) {
                if (feof(fp))
                    return 0;
                continue;
            }
        }
        if (strlen(lineStr) == 1) { // skip null
            lineStr[0] = 0x0;
            continue;
        }

        if ((lineStr[0] == '#')
        ||(lineStr[0] == ';')||(lineStr[0] == ':')) { // skip comments
            lineStr[0] = 0x0;
            continue;
        }

        printf("[%s][%d]:%s\n",__func__,__LINE__,lineStr);
        // keep the original
        memcpy(valueStr, lineStr, strlen(lineStr));
        //param is separated by ' '
        token = strtok(valueStr, delim);
        if (token == NULL) {
            lineStr[0] = 0x0;
            continue;
        }
        cfg_crop->x = atoi(token);

        token = strtok(NULL, delim);
        if (token == NULL) {
            lineStr[0] = 0x0;
            continue;
        }
        cfg_crop->y = atoi(token);

        token = strtok(NULL, delim);
        if (token == NULL) {
            lineStr[0] = 0x0;
            continue;
        }
        cfg_crop->w = atoi(token);

        token = strtok(NULL, delim);
        if (token == NULL) {
            lineStr[0] = 0x0;
            continue;
        }
        cfg_crop->h = atoi(token);

        lineStr[0] = 0x0;
        continue;
    }
    return 0;
}

static int ParseEncodeCfgFile(FILE *fp, encodecCfgParam *cfg_encode)
{
    char* token = NULL;
    const char *delim = " \t\r\n";
    static char lineStr[ENCODER_STRING_LEN_MAX*4] = {0};
    char valueStr[ENCODER_STRING_LEN_MAX] = {0};
    int i = 0;
    int len = 0;
    while (1) {
        if (i >= CFG_ENCODE_NUM_MAX)
            return 0;
        if (lineStr[0] == 0x0) { //need a new line;
            if (fgets(lineStr, ENCODER_STRING_LEN_MAX*4, fp) == NULL) {
                if (feof(fp))
                    return 0;
                continue;
            }
        }
        if (strlen(lineStr) == 1) { // skip null
            lineStr[0] = 0x0;
            continue;
        }

        if ((lineStr[0] == '#')
        ||(lineStr[0] == ';')||(lineStr[0] == ':')) { // skip comments
            lineStr[0] = 0x0;
            continue;
        }

        printf("[%s][%d]:%s\n",__func__,__LINE__,lineStr);
        // keep the original
        memcpy(valueStr, lineStr, strlen(lineStr));
        //param is separated by ' '
        token = strtok(valueStr, delim);
        if (token == NULL) {
            lineStr[0] = 0x0;
            continue;
        }
        token = strtok(NULL, delim);
        if (token == NULL) {
            lineStr[0] = 0x0;
            continue;
        }
        len = strlen(token) > ENCODER_STRING_LEN_MAX ? ENCODER_STRING_LEN_MAX : strlen(token);
        mystrncpy(cfg_encode[i].srcfile, token, len);

        token = strtok(NULL, delim);
        if (token == NULL) {
            lineStr[0] = 0x0;
            continue;
        }
        len = strlen(token) > ENCODER_STRING_LEN_MAX ? ENCODER_STRING_LEN_MAX : strlen(token);
        mystrncpy(cfg_encode[i].outfile, token, len);

        token = strtok(NULL, delim);
        if (token == NULL) {
            lineStr[0] = 0x0;
            continue;
        }
        cfg_encode[i].width = atoi(token);

        token = strtok(NULL, delim);
        if (token == NULL) {
            lineStr[0] = 0x0;
            continue;
        }
        cfg_encode[i].height = atoi(token);

        token = strtok(NULL, delim);
        if (token == NULL) {
            lineStr[0] = 0x0;
            continue;
        }
        cfg_encode[i].gop = atoi(token);

        token = strtok(NULL, delim);
        if (token == NULL) {
            lineStr[0] = 0x0;
            continue;
        }
        cfg_encode[i].framerate = atoi(token);

        token = strtok(NULL, delim);
        if (token == NULL) {
            lineStr[0] = 0x0;
            continue;
        }
        cfg_encode[i].bitrate = atoi(token);

        token = strtok(NULL, delim);
        if (token == NULL) {
            lineStr[0] = 0x0;
            continue;
        }
        cfg_encode[i].num = atoi(token);

        token = strtok(NULL, delim);
        if (token == NULL) {
            lineStr[0] = 0x0;
            continue;
        }
        cfg_encode[i].fmt = atoi(token);

        token = strtok(NULL, delim);
        if (token == NULL) {
            lineStr[0] = 0x0;
            continue;
        }
        cfg_encode[i].buf_type = atoi(token);

        token = strtok(NULL, delim);
        if (token == NULL) {
            lineStr[0] = 0x0;
            continue;
        }
        cfg_encode[i].num_planes = atoi(token);

        token = strtok(NULL, delim);
        if (token == NULL) {
            lineStr[0] = 0x0;
            continue;
        }
        cfg_encode[i].codec_id = atoi(token);

        token = strtok(NULL, delim);//cfg_opt optional
        if (token == NULL) {
            i++;
            cfg_encode[0].seqNum++;
            lineStr[0] = 0x0;
            continue;
        }
        cfg_encode[i].cfg_opt = (int)strtol(token, NULL, 0);
        token = strtok(NULL, delim);
        if (token == NULL) {
            i++;
            cfg_encode[0].seqNum++;
            lineStr[0] = 0x0;
            continue;
        }

        if (cfg_encode[i].cfg_opt > 0) {
            cfg_encode[i].frame_rotation = (cfg_encode[i].cfg_opt >> 14) & 0x03;
            cfg_encode[i].frame_mirroring = (cfg_encode[i].cfg_opt >> 16) & 0x03;
            printf("frame_rotation=%d, frame_mirroring=%d\n",
                   cfg_encode[i].frame_rotation, cfg_encode[i].frame_mirroring);
        }

        if ((cfg_encode[i].cfg_opt & 0x2) == 0x2) {
            len = strlen(token) > ENCODER_STRING_LEN_MAX ? ENCODER_STRING_LEN_MAX : strlen(token);
            mystrncpy(cfg_encode[i].cfg_file, token, len);
            cfg_encode[i].cfg_upd_enabled = 1;
            printf("cfg_upd_url[%d] is\t: %s ;\n", i, cfg_encode[i].cfg_file);
            token = strtok(NULL, delim);
            if (token == NULL) {
                i++;
                cfg_encode[0].seqNum++;
                lineStr[0] = 0x0;
                continue;
            }
        }

        if ((cfg_encode[i].cfg_opt & 0x100) == 0x100) {
            cfg_encode[i].src_buf_stride = atoi(token);
            token = strtok(NULL, delim);
            if (token == NULL) {
                i++;
                cfg_encode[0].seqNum++;
                lineStr[0] = 0x0;
                continue;
            }
        }

        if ((cfg_encode[i].cfg_opt >> 11) & 0x1) {
            cfg_encode[i].profile = atoi(token);
            printf("profile %d\n", cfg_encode[i].profile);
            token = strtok(NULL, delim);
            if (token == NULL) {
                i++;
                cfg_encode[0].seqNum++;
                lineStr[0] = 0x0;
                continue;
            }
        }

        if ((cfg_encode[i].cfg_opt >> 12) & 0x1) {
            cfg_encode[i].level = atoi(token);
            printf("level %d\n", cfg_encode[i].level);
            token = strtok(NULL, delim);
            if (token == NULL) {
                i++;
                cfg_encode[0].seqNum++;
                lineStr[0] = 0x0;
                continue;
            }
        }
        if ((cfg_encode[i].cfg_opt >> 13) & 0x1) {
            cfg_encode[i].tier = atoi(token);
            printf("tier %d\n",cfg_encode[i].tier);
            token = strtok(NULL, delim);
            if (token == NULL) {
                i++;
                cfg_encode[0].seqNum++;
                lineStr[0] = 0x0;
                continue;
            }
        }

        if ((cfg_encode[i].cfg_opt >> 23) & 0x1) {
            cfg_encode[i].forcePicQpI = atoi(token);
            token = strtok(NULL, delim);
            if (token == NULL) {
                printf("missing forcePicQpPB\n");
                continue;
            }
            cfg_encode[i].forcePicQpPB = atoi(token);
            printf("forcePicQpI %d, forcePicQpPB %d\n", cfg_encode[i].forcePicQpI, cfg_encode[i].forcePicQpPB);
            token = strtok(NULL, delim);
            if (token == NULL) {
                i++;
                cfg_encode[0].seqNum++;
                lineStr[0] = 0x0;
                continue;
            }
        }

        if ((cfg_encode[i].cfg_opt >> 24) & 0x1) {
            len = strlen(token) > ENCODER_STRING_LEN_MAX ? ENCODER_STRING_LEN_MAX : strlen(token);
            mystrncpy(cfg_encode[i].osdfile, token, len);
            cfg_encode[i].osd_enabled = 1;
            printf("osdfile is\t: %s ;\n", cfg_encode[i].osdfile);
            token = strtok(NULL, delim);
            if (token == NULL) {
                i++;
                cfg_encode[0].seqNum++;
                lineStr[0] = 0x0;
                continue;
            }
        }

        if ((cfg_encode[i].cfg_opt >> 25) & 0x1) {
            len = strlen(token) > ENCODER_STRING_LEN_MAX ? ENCODER_STRING_LEN_MAX : strlen(token);
            printf("len %d\n", len);
            mystrncpy(cfg_encode[i].dsfile, token, len);
            cfg_encode[i].ds_enabled = 1;
            printf("dsfile is\t: %s ;\n", cfg_encode[i].dsfile);
            token = strtok(NULL, delim);
            if (token == NULL) {
                i++;
                cfg_encode[0].seqNum++;
                lineStr[0] = 0x0;
                continue;
            }
        }

        if ((cfg_encode[i].cfg_opt >> 26) & 0x1) {
            len = strlen(token) > ENCODER_STRING_LEN_MAX ? ENCODER_STRING_LEN_MAX : strlen(token);
            printf("len %d\n", len);
            memset(cfg_encode[i].cropfile, 0, ENCODER_STRING_LEN_MAX);
            snprintf(cfg_encode[i].cropfile, sizeof(cfg_encode[i].cropfile), "%s", token);
            cfg_encode[i].crop_enabled = 1;
            printf("cropfile is\t: %s ;\n", cfg_encode[i].cropfile);
            token = strtok(NULL, delim);
            if (token == NULL) {
                i++;
                cfg_encode[0].seqNum++;
                lineStr[0] = 0x0;
                continue;
            }
        }

        token = strtok(NULL, delim);
        if (token == NULL) {
            i++;
            cfg_encode[0].seqNum++;
            lineStr[0] = 0x0;
            continue;
        } else { //config no match
            printf("config no match conf %d command: %s\n",
                cfg_encode[i].cfg_opt, lineStr);
            lineStr[0] = 0x0;
            continue;
        }
    }
    return 0;
}

static int ParseCfgUpdateFile(FILE *fp, CfgChangeParam *cfg_update)
{
    int parsed_num = 0, frameIdx = 0;
    char* token = NULL;
    static char lineStr[256] = {0, };
    char valueStr[256] = {0, };
    int has_update = 0;
    while (1) {
        if (lineStr[0] == 0x0) { //need a new line;
            if (fgets(lineStr, 256, fp) == NULL ) {
                if (cfg_update->enable_option && has_update)
                    return 1;
                else {
                    if (feof(fp)) return 0;
                    continue;
                }
            }
        }
        if ((lineStr[0] == '#')
            ||(lineStr[0] == ';')||(lineStr[0] == ':'))
        { // skip comments
            lineStr[0] = 0x0;
            continue;
        }
        // keep the original
        memcpy(valueStr, lineStr, strlen(lineStr));
        //frame number is separated by ' ' or ':'
        token = strtok(valueStr, ": ");
        if (token == NULL) {
            lineStr[0] = 0x0;
            continue;
        }
        frameIdx = atoi(token);
        if (frameIdx <0 || frameIdx < cfg_update->FrameNum) {
            // invid number
            lineStr[0] = 0x0;
            continue;
        }
        else if (frameIdx > cfg_update->FrameNum && has_update)
        {// done the current frame;
            if (cfg_update->enable_option)
                return 1;
            has_update = 0;
            continue;

        } else if (has_update ==0) {// clear the options
            cfg_update->FrameNum = frameIdx;
            cfg_update->enable_option = 0;
            has_update = 1;
        }

        token = strtok(NULL, " :"); // check the operation code
        while (token && strlen(token) == 1
            && strncmp(token, " ", 1) == 0)
            token = strtok(NULL, " :"); // skip spaces
        if (token == NULL) { // no operate code
            lineStr[0] = 0x0;
            continue;
        }
        if (strcasecmp("ForceType",token) == 0) {
            // force picture type
            token = strtok(NULL, ":\r\n");
            printf("===========%s, %d, frameIdx %d\n", __func__, __LINE__, frameIdx);
            while (token && strlen(token) == 1
                && strncmp(token, " ", 1) == 0)
            token = strtok(NULL, ":\r\n"); //check space
            if (token) {
                while ( *token == ' ' ) token++;//skip spaces;
                cfg_update->picType = atoi(token);
                cfg_update->enable_option |=
                    FORCE_PICTURE_TYPE;
            }
        } else if (strcasecmp("ForceSkip",token) == 0) {
            // force picture type
            if (token) {
                cfg_update->enable_option |=
                    FORCE_PICTURE_SKIP;
            }
        } else if (strcasecmp("ChangeBitRate",token) == 0) {
            token = strtok(NULL, ":\r\n");
            while ( token && strlen(token) == 1
                && strncmp(token, " ", 1) == 0)
                token = strtok(NULL, ":\r\n"); //check space
            if (token) {
                while ( *token == ' ' ) token++;//skip spaces;
                cfg_update->bitRate = atoi(token);
                cfg_update->enable_option |=
                    CHANGE_TARGET_RATE;
            }
        } else if (strcasecmp("ChangeMinMaxQP",token) == 0) {
            token = strtok(NULL, ":\r\n");
            while (token && strlen(token) == 1
                && strncmp(token, " ", 1) == 0)
                token = strtok(NULL, ":\r\n"); //check space
            if (token) {
                while ( *token == ' ' ) token++;//skip spaces;
                parsed_num = sscanf(token,
                    "%d %d %d %d %d %d %d",
                    &cfg_update->minQpI, &cfg_update->maxQpI,
                    &cfg_update->maxDeltaQp,
                    &cfg_update->minQpP, &cfg_update->maxQpP,
                    &cfg_update->minQpB, &cfg_update->maxQpB);
                if (parsed_num == 7)
                    cfg_update->enable_option |=
                        CHANGE_MIN_MAX_QP;
            }
        } else if (strcasecmp("ChangePeriodGOP",token) == 0) {
            token = strtok(NULL, ":\r\n");
            while ( token && strlen(token) == 1
                && strncmp(token, " ", 1) == 0)
                token = strtok(NULL, ":\r\n"); //check space
            if (token) {
                while ( *token == ' ' ) token++;//skip spaces;
                parsed_num = sscanf(token, "%d %d",
                    &cfg_update->intraQP,
                    &cfg_update->GOPPeriod);
                if (parsed_num == 2)
                    cfg_update->enable_option |=
                        CHANGE_GOP_PERIOD;
            }
        } else if (strcasecmp("SetLongTermRef",token) == 0) {
            token = strtok(NULL, ":\r\n");
            while ( token && strlen(token) == 1
                && strncmp(token, " ", 1) == 0)
                token = strtok(NULL, ":\r\n"); //check space
            if (token) {
                while ( *token == ' ' ) token++;//skip spaces;
                cfg_update->LTRFlags = atoi(token);
                cfg_update->enable_option |=
                    LONGTERM_REF_SET;
            }
        } else if (strcasecmp("SetCustSmooth",token) == 0) {
            token = strtok(NULL, ":\r\n");
            while ( token && strlen(token) == 1
                && strncmp(token, " ", 1) == 0)
                token = strtok(NULL, ":\r\n"); //check space
            if (token) {
                while ( *token == ' ' ) token++;//skip spaces;
                cfg_update->smooth_level = atoi(token);
                cfg_update->enable_option |=
                    CUST_SIZE_SMOOTH;
            }
        } else if (strcasecmp("SetCustH264P",token) == 0) {
            token = strtok(NULL, ":\r\n");
            while ( token && strlen(token) == 1
                && strncmp(token, " ", 1) == 0)
                token = strtok(NULL, ":\r\n"); //check space
            if (token) {
                while ( *token == ' ' ) token++;//skip spaces;
                parsed_num = sscanf(token,
                    "%d %d %d",
                    &cfg_update->newIDRInterval,
                    &cfg_update->QPdelta_IDR,
                    &cfg_update->QPdelta_LTR);
                if (parsed_num == 3)
                    cfg_update->enable_option |=
                        CUST_APP_H264_PLUS;
            }
        } else if (strcasecmp("ChangeMultiSlice",token) == 0) {
            token = strtok(NULL, ":\r\n");
            while ( token && strlen(token) == 1
                && strncmp(token, " ", 1) == 0)
                token = strtok(NULL, ":\r\n"); //check space
            if (token) {
                while ( *token == ' ' ) token++;//skip spaces;
                parsed_num = sscanf(token, "%d %d",
                    &cfg_update->Slice_Mode,
                    &cfg_update->Slice_Para);
                if (parsed_num == 2)
                    cfg_update->enable_option |=
                        CHANGE_MULTI_SLICE;
            }
        }
        else if (strcasecmp("ChangeStrictRC",token) == 0) {
            token = strtok(NULL, ":\r\n");
            while ( token && strlen(token) == 1
                && strncmp(token, " ", 1) == 0)
                token = strtok(NULL, ":\r\n"); //check space
            if (token) {
                while ( *token == ' ' ) token++;//skip spaces;
                parsed_num = sscanf(token, "%d %d",
                    &cfg_update->bitrate_window,
                    &cfg_update->pskip_threshold);
                if (parsed_num == 2)
                    cfg_update->enable_option |=
                        CHANGE_STRICT_RC;
            }
        }
        lineStr[0] = 0x0;
        continue;
    }
}

