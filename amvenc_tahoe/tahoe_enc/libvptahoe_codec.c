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
#include <pthread.h>
#include "tahoe.h"
#include "vp_codec_tahoe.h"
#include "vdi.h"
#include "enc_driver.h"
#include "tahoe_register.h"
#include <string.h>
#include <sys/poll.h>

#define MAX_FRAME_WINDOW       (120)
venc_info_t s_venc_info;

int get_header(handle_t *handle, unsigned char *pHeader, unsigned int *pLength)
{
    venc_info_t *venc = &s_venc_info;
    unsigned long vpsPhyAddr, spsPhyAddr, ppsPhyAddr;
    int headerLength, vpsLength, spsLength, ppsLength;
    unsigned int headeroffset = 0;
    unsigned long headerVirAddr = 0;
    unsigned int des_offset = 0;
    char header_tmp[128];


    venc_buffer_t vdb;

    if (!pHeader)
        return -1;
    memset(&vdb, 0x00, sizeof(venc_buffer_t));

    vpsPhyAddr = handle->enc_header.vpsPhyAddr;
    spsPhyAddr = handle->enc_header.spsPhyAddr;
    ppsPhyAddr = handle->enc_header.ppsPhyAddr;
    headerLength = handle->enc_header.headerLength;
    VLOG(INFO,"[%s][%d]vpsPhyAddr:0x%lx\n", __func__, __LINE__,vpsPhyAddr);
    VLOG(INFO,"[%s][%d]spsPhyAddr:0x%lx\n", __func__, __LINE__,spsPhyAddr);
    VLOG(INFO,"[%s][%d]ppsPhyAddr:0x%lx\n", __func__, __LINE__,ppsPhyAddr);
    VLOG(INFO,"[%s][%d]headerLength:0x%lx\n", __func__, __LINE__,headerLength);

//00 vps sps pps，各占一个字节，最高8位没有
    vpsLength = (headerLength & 0xff0000) >> 16;
    spsLength = (headerLength & 0xff00) >> 8;
    ppsLength = (headerLength & 0xff);

    VLOG(INFO,"header origin addr:%lx %lx %lx\n",handle->header_buf_addr[0],handle->header_buf_addr[1],handle->header_buf_addr[2]);

    if (handle->enc_fmt == CODEC_ID_H265) {
        headeroffset = vpsPhyAddr - handle->header_buf_addr[0];
        vdi_read_memory(/*vpsPhyAddr*/handle->header_buf_addr[0], /*pHeader*/header_tmp, headeroffset + vpsLength);
        memcpy(pHeader,(header_tmp + headeroffset),vpsLength);
        des_offset = vpsLength;
    }
    headeroffset = spsPhyAddr - handle->header_buf_addr[1];
    vdi_read_memory(/*spsPhyAddr*/handle->header_buf_addr[1], /*pHeader+vpsLength*/header_tmp, headeroffset + spsLength);
    memcpy((pHeader + des_offset),(header_tmp  + headeroffset),spsLength);
    des_offset += spsLength;

    headeroffset = ppsPhyAddr - handle->header_buf_addr[2];
    vdi_read_memory(/*ppsPhyAddr*/handle->header_buf_addr[2], /*pHeader+vpsLength+spsLength*/header_tmp, headeroffset + ppsLength);
    memcpy((pHeader + des_offset),(header_tmp + headeroffset),ppsLength);
    *pLength = vpsLength + spsLength + ppsLength;

#if 0
    if (handle->enc_fmt == CODEC_ID_H265) {
        headeroffset = vpsPhyAddr - handle->header_buf_addr[0];
        vdi_read_memory(/*vpsPhyAddr*/handle->header_buf_addr[0], /*pHeader*/header_tmp, headeroffset + vpsLength);
        memcpy(pHeader,header_tmp + headeroffset,vpsLength);
        headeroffset = vpsPhyAddr - handle->header_buf_addr[1];
        vdi_read_memory(/*spsPhyAddr*/handle->header_buf_addr[1], /*pHeader+vpsLength*/header_tmp, headeroffset + spsLength);
        memcpy(pHeader,header_tmp + vpsLength + headeroffset,spsLength);
        headeroffset = vpsPhyAddr - handle->header_buf_addr[2];
        vdi_read_memory(/*ppsPhyAddr*/handle->header_buf_addr[0], /*pHeader+vpsLength+spsLength*/header_tmp, headeroffset + ppsLength);
        memcpy(pHeader,header_tmp + vpsLength + spsLength + headeroffset,ppsLength);
        *pLength = vpsLength + spsLength + ppsLength;
    } else if (handle->enc_fmt == CODEC_ID_H264) {
        vdi_read_memory(spsPhyAddr, pHeader, spsLength);
        vdi_read_memory(ppsPhyAddr, pHeader+spsLength, ppsLength);
        *pLength = spsLength + ppsLength;
    }
#endif
    VLOG(INFO,"[%s][%d]headerLength:%d\n", __func__, __LINE__,*pLength);
    return 0;
}

int get_bsbframe(handle_t *handle, unsigned char *buffer, unsigned int *buf_nal_size, signed char *frame_type)
{
    venc_info_t *venc = &s_venc_info;
    unsigned int esaddr_offset = 0;
    unsigned long esPhyAddr;
    int eslength;
    unsigned long espVirAddr = 0;
    unsigned long slice_header_length = 0;
    int length_cp = 0;

    slice_header_length = handle->enc_es.slice_header_length;
    esPhyAddr = handle->enc_es.esPhyAddr;
    eslength = handle->enc_es.eslength;
    esaddr_offset = esPhyAddr - venc->common_buffer.phys_addr;
    espVirAddr = venc->common_buffer.virt_addr + esaddr_offset;
    VLOG(INFO,"[%s][%d]offset:0x%x\n", __func__, __LINE__, slice_header_length);
    VLOG(INFO,"[%s][%d]esPhyAddr:0x%lx\n", __func__, __LINE__, esPhyAddr);
    VLOG(INFO,"[%s][%d]esaddr_offset:0x%lx\n", __func__, __LINE__, esaddr_offset);
    VLOG(INFO,"[%s][%d]espVirAddr:0x%lx\n", __func__, __LINE__, espVirAddr);
    VLOG(INFO,"[%s][%d]eslength:%d\n", __func__, __LINE__, eslength);

    length_cp = ((eslength + 15) >> 4) << 4;
    memcpy(buffer, (const void *)((ulong)espVirAddr), length_cp);
    *buf_nal_size = eslength;
    (*frame_type) = handle->enc_es.frame_type;
    VLOG(INFO,"[%s][%d]buf_nal_size:%d,frame_type:%d\n", __func__, __LINE__,*buf_nal_size,(*frame_type));
    return 0;
}

int get_dsframe(handle_t *handle, unsigned char *buffer, unsigned int *buf_nal_size)
{
    venc_info_t *venc = &s_venc_info;
    unsigned long dsPhyAddr;
    int dslength;
    unsigned long dsVirAddr = 0;
    if (!handle)
        return -1;

    dsPhyAddr = venc->common_buffer.phys_addr;
    dsVirAddr = venc->common_buffer.virt_addr;
    if ((handle->mfdin_input_fmt == MFDI_INPUT_FMT_NV12) ||
        (handle->mfdin_input_fmt == MFDI_INPUT_FMT_NV21) ||
        (handle->mfdin_input_fmt == MFDI_INPUT_FMT_YUV420_PLANAR)) {
        dslength = handle->ds_w * handle->ds_h * 3 / 2;
    } else if (handle->mfdin_input_fmt == MFDI_INPUT_FMT_RGB888) {
        dslength = handle->ds_w * handle->ds_h * 3;
    } else if (handle->mfdin_input_fmt == MFDI_INPUT_FMT_RGBA8888) {
        dslength = handle->ds_w * handle->ds_h * 4;
    }
    VLOG(INFO,"[%s][%d]dsPhyAddr:0x%lx\n", __func__, __LINE__, dsPhyAddr);
    VLOG(INFO,"[%s][%d]dsVirAddr:0x%lx\n", __func__, __LINE__, dsVirAddr);
    VLOG(INFO,"[%s][%d]dslength:%d\n", __func__, __LINE__, dslength);

    memcpy(buffer, (const void *)((ulong)dsVirAddr), dslength);
    *buf_nal_size = dslength;
    VLOG(INFO,"[%s][%d]buf_nal_size:%d\n", __func__, __LINE__,*buf_nal_size);
    return 0;
}

int check_initparam(handle_t handle)
{
    if (handle.enc_width < MIN_WIDTH || handle.enc_height < MIN_HEIGHT) {
        VLOG(ERR,"[%s][%d]width %d x height %d must be greater than %d x %d.\n",__func__,__LINE__, handle.enc_width, handle.enc_height, MIN_WIDTH, MIN_HEIGHT);
        return -1;
    }

    if (handle.crop_enable && (handle.crop_w < MIN_WIDTH || handle.crop_h < MIN_HEIGHT)) {
        VLOG(ERR,"[%s][%d]crop_w %d x crop_h %d must be greater than %d x %d.\n",__func__,__LINE__, handle.crop_w, handle.crop_h, MIN_WIDTH, MIN_HEIGHT);
        return -1;
    }
    return 0;
}

void encoder_open(char *fwfile) {

  venc_init();
  vrisc_reset();
  firmware_load(fwfile);
  firmware_boot();
}

void encoder_close() {
    venc_destroy();
}

int vl_tahoe_encoder_init(int codec_id, handle_t encode_info) {
//  int ret;
//  VPMultiEncHandle* mHandle = (VPMultiEncHandle *) malloc(sizeof(VPMultiEncHandle));
//
//  if (mHandle == NULL)
//    goto exit;
//  memset(mHandle, 0, sizeof(VPMultiEncHandle));
//  if (!check_qp_tbl(qp_tbl)) {
//    goto exit;
//  }
//
//  vp5_set_log_level(ERR);
//
//  ret = initEncParams(mHandle, codec_id, encode_info, qp_tbl);
//  if (ret < 0)
//    goto exit;
//
//  mHandle->am_enc_handle = AML_MultiEncInitialize(&(mHandle->mEncParams));
//  if (mHandle->am_enc_handle == 0)
//    goto exit;
//  mHandle->mPrependSPSPPSToIDRFrames =
//                encode_info.prepend_spspps_to_idr_frames;
//  mHandle->mSpsPpsHeaderReceived = false;
//  mHandle->mNumInputFrames = -1;  // 1st two buffers contain SPS and PPS
//  if (encode_info.strict_rc_window > MAX_FRAME_WINDOW)
//     encode_info.strict_rc_window = MAX_FRAME_WINDOW;
//  mHandle->hist_win_len = encode_info.strict_rc_window;
//  mHandle->hist_skip_thresh = encode_info.strict_rc_skip_thresh;
  if (check_initparam(encode_info)) {
      printf("check_initparam failed\n");
      return -1;
  }

  venc_lock();
  create_channel(encode_info);
  venc_unlock();
  return 0;

//  return (vl_codec_handle_t)mHandle;

//exit:
//  if (mHandle != NULL)
//    free(mHandle);
//  return (vl_codec_handle_t)NULL;
}

int vl_video_encoder_change_bitrate(handle_t *encode_info,
                            int bitRate)
{
    int ret;
//    VPMultiEncHandle* handle = (VPMultiEncHandle *)codec_handle;

//    if (handle->am_enc_handle == 0) //not init the encoder yet
//        return -1;
//    if (handle->mEncParams.param_change_enable == 0) //no change enabled
//        return -2;
//    ret = AML_MultiEncChangeBitRate(handle->am_enc_handle, bitRate);
//    if (ret != AMVENC_SUCCESS)
//        return -3;
//    hist_reset(handle); //bitrate target change reset the hist
//    handle->mEncParams.bitrate = bitRate;
    encode_info->enc_bitrate = bitRate;
    encode_info->cfg_update = 1;

    return 0;
}

int vl_video_encoder_change_gop(handle_t *encode_info,
                                int IntraQP, int IntraPeriod)
{
    int ret;
//    VPMultiEncHandle* handle = (VPMultiEncHandle *)codec_handle;

//    if (handle->am_enc_handle == 0) //not init the encoder yet
//        return -1;
//    if (handle->mEncParams.param_change_enable == 0) //no change enabled
//        return -2;
    if (IntraQP < 0 || IntraQP > 51 ) {
           VLOG(ERR,"QP value out of range [0, 51]\n");
        return -3;
    }
    if (IntraPeriod <= 1 ) {
           VLOG(ERR,"Invalid Intra Period %d\n", IntraPeriod);
        return -4;
    }
//    ret = AML_MultiEncChangeIntraPeriod(handle->am_enc_handle,
//              IntraQP, IntraPeriod);
//    if (ret != AMVENC_SUCCESS)
//        return -5;
    encode_info->enc_gop = IntraPeriod;
    encode_info->cfg_update = 1;
    return 0;
}

int vl_video_encoder_change_qp(handle_t *encode_info,
                                int minQpI, int maxQpI, int maxDeltaQp,
                                int minQpP, int maxQpP,
                                int minQpB, int maxQpB)
{
    int ret;
//    VPMultiEncHandle* handle = (VPMultiEncHandle *)codec_handle;

//    if (handle->am_enc_handle == 0) //not init the encoder yet
//        return -1;
//    if (handle->mEncParams.param_change_enable == 0) //no change enabled
//        return -2;

    if (minQpI < 0 || minQpI > 51 || maxQpI < 0 || maxQpI > 51 ||
        maxDeltaQp < 0 || maxDeltaQp > 51 || minQpP < 0 || minQpP > 51 ||
        maxQpP < 0 || maxQpP > 51 || minQpB < 0 || minQpB > 51 ||
        maxQpB < 0 || maxQpB > 51) {
        VLOG(ERR,"qp min or qp max out of range [0, 51]\n");
        return -3;
    }
    if (minQpI >= maxQpI || minQpP >= maxQpP || minQpB >= maxQpB) {
        VLOG(ERR,"qp min  or qp_max out of range [min, max]\n");
        return -4;
    }

//    ret = AML_MultiEncChangeQPMinMax(handle->am_enc_handle,
//             minQpI, maxQpI, maxDeltaQp, minQpP, maxQpP, minQpB, maxQpB);
//    if (ret != AMVENC_SUCCESS)
//        return -5;
    return 0;
}

int vl_video_encoder_change_strict_rc(handle_t *encode_info,
                                  int bitrate_window, int skip_threshold)
{
    int ret;
//    VPMultiEncHandle* handle = (VPMultiEncHandle *)codec_handle;

//    if (handle->am_enc_handle == 0) //not init the encoder yet
//        return -1;
    if (bitrate_window > MAX_FRAME_WINDOW)
        bitrate_window = MAX_FRAME_WINDOW;
    else if (bitrate_window < 0)
        bitrate_window = 0;

//    handle->hist_win_len = bitrate_window;
//    handle->hist_skip_thresh = skip_threshold;

//    if (bitrate_window == 0)
//    {// disable
//        hist_reset(handle);
//    }
    encode_info->enc_fps = bitrate_window;
    encode_info->cfg_update = 1;
    return 0;
}

int vl_tahoe_encoder_generate_header(handle_t *encode_info)
{
    int ret = 0;

    venc_lock();
    encode_header(encode_info);
    encode_info->enc_header.vpsPhyAddr = ((((ulong)(venc_read_register(HEADER_ADDR_HIGHER) >> 4) & 0x03)) << 32) | venc_read_register(HEADER_VPS_ADDR);
    encode_info->enc_header.spsPhyAddr = (((ulong)(venc_read_register(HEADER_ADDR_HIGHER) & 0x03)) << 32) | venc_read_register(HEADER_SPS_ADDR);
    encode_info->enc_header.ppsPhyAddr = ((((ulong)(venc_read_register(HEADER_ADDR_HIGHER) >> 2) & 0x03)) << 32) | venc_read_register(HEADER_PPS_ADDR);
    encode_info->enc_header.headerLength = venc_read_register(HEADER_SIZE);

    VLOG(INFO,"header addr %lx %lx %lx\n",encode_info->enc_header.vpsPhyAddr,encode_info->enc_header.spsPhyAddr,encode_info->enc_header.ppsPhyAddr);
    venc_unlock();

    return ret;
}

static int encode_poll(int fd, int timeout) {
    struct pollfd poll_fd[1];
    poll_fd[0].fd = fd;
    poll_fd[0].events = POLLIN | POLLERR;
    return poll(poll_fd, 1, timeout);
}

int vl_tahoe_encoder_encode(handle_t *encode_info)
{
    int ret = 0;

    //vdi_debug(encode_info->chn_id);

    venc_lock();
    osd_config(encode_info);
    if (encode_info->cfg_update == 1) {
        dynamic_parameter_set(encode_info);
        encode_info->cfg_update = 0;
    }
    encode_one_frame(encode_info);
    if (encode_poll(s_venc_info.venc_fd, -1) <= 0) {
        VLOG(ERR,"[%s][%d]poll fail\n", __func__, __LINE__);
        return -1;
    }
    encode_info->enc_es.slice_header_length = venc_read_register(TAHOE_EC_DMA0) & 0x3F;
    encode_info->enc_es.esPhyAddr = venc_read_register(TAHOE_EC_DMA0) - encode_info->enc_es.slice_header_length;
    encode_info->enc_es.esPhyAddr |= (((ulong)(venc_read_register(TAHOE_EC_DMA6) & 0x03)) << 32);
    encode_info->enc_es.eslength = venc_read_register(TAHOE_EC_LENGTH);
    encode_info->enc_es.frame_type = venc_read_register(OUTPUT_FRAME_TYPE);

    VLOG(INFO,"es buffer phyaddr:%lx,lsb:%x",encode_info->enc_es.esPhyAddr,(venc_read_register(TAHOE_EC_DMA0) - encode_info->enc_es.slice_header_length));

    venc_unlock();
    return ret;
}

int vl_tahoe_encoder_destroy(handle_t encode_info) {
//    VPMultiEncHandle *handle = (VPMultiEncHandle *)codec_handle;
//    AML_MultiEncRelease(handle->am_enc_handle);
//#if ENCODE_TIME_OUTER
//    printf("%p#Total encode time : %llu, Total encode frames : %llu\n",
//        handle->am_enc_handle, total_encode_time, total_encode_frames);
//#endif
//    if (handle->mSPSPPSData)
//        free(handle->mSPSPPSData);
//    if (handle)
//        free(handle);
    venc_lock();
    destroy_channel(encode_info);
    venc_unlock();

    return 1;
}

