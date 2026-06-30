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
#include "enc_driver.h"
#include "tahoe_register.h"
#include "vp_codec_tahoe.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "vdi.h"


extern venc_info_t s_venc_info;

void venc_write_register(unsigned int addr, unsigned int data)
{
    venc_info_t *venc;
    unsigned long *reg_addr;

    venc = &s_venc_info;

    if (!venc || venc->venc_fd == -1 || venc->venc_fd == 0x00)
        return;

    reg_addr = (unsigned long *)(addr + (unsigned long)venc->venc_register.virt_addr);

    *(volatile unsigned int *)reg_addr = data;
}

unsigned int venc_read_register(unsigned int addr)
{
    venc_info_t *venc;
    unsigned long *reg_addr;

    venc = &s_venc_info;

    if (!venc || venc->venc_fd == -1)
        return (unsigned int)-1;

    reg_addr = (unsigned long *)(addr + (unsigned long)venc->venc_register.virt_addr);

    return *(volatile unsigned int *)reg_addr;
}


void disable_host_scrw_irq(void)
{
    venc_write_register(TAHOE_VRISC_IRQ_MASK,
      0 | //frme
      1 << 1 | //frms
      1 << 2 | //scrw
      1 << 3 | //rdma
      1 << 4); //prel
}

void enable_vrisc_scrw_irq(void)
{
    venc_write_register(TAHOE_VRISC_EIRQ_MASK,
      0 | //frme
      0 << 1 | //frms
      0 << 2 | //scrw
      0 << 3 | //rdma
      0 << 4); //prel
}

void firmware_load(char *filename)
{

    venc_info_t *venc = &s_venc_info;

    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        VLOG(ERR,"Error opening file");
    }

    fseek(file, 0, SEEK_END);
    int file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    void *ptr = malloc(file_size);

    int ret = fread(ptr, 1, file_size, file);
    memcpy((void*)venc->fw_buffer.virt_addr, ptr, file_size);
//    vdi_flush_memory(&venc->fw_buffer);

    free(ptr);

    if (file)
        fclose(file);

    return;
}

void firmware_boot()
{
    int data = 0;
    venc_info_t *venc = &s_venc_info;
    venc_write_register(TAHOE_VRISC_MBASE, venc->fw_buffer.phys_addr);
    venc_write_register(TAHOE_VRISC_MISC, ((venc->fw_buffer.phys_addr >> 32 & 0x03) << 16));
    VLOG(INFO,"TAHOE MBASE:%x,MISC:%x\n",venc_read_register(TAHOE_VRISC_MBASE),venc_read_register(TAHOE_VRISC_MISC));

    venc_write_register(TAHOE_VRISC_MPRELOAD,
        (PRELOAD_SIZE << 0) |    // mpreload_size_m1
        (0  << 12) |             // mpreload_axi_id
        (1  << 31));             // mpreload_enable);
    data = venc_read_register(TAHOE_VRISC_MPRELOAD) >> 31;
    while (0 != data) {
        data = venc_read_register(TAHOE_VRISC_MPRELOAD) >> 31;
        usleep(1000);
    }
    venc_write_register(TAHOE_VRISC_CORECFG, ((1 << 0) |    // stop_on_reset release
                             (0 << 1) |    // clkgate_bypass
                             (0 << 2) |    // reset_bypass
                             (0 << 3) ));  // reserved

    data = venc_read_register(CMD_OPT_CODE);
    while (CMD_DONE != data) {
        data = venc_read_register(CMD_OPT_CODE);
        usleep(100);
    }
    VLOG(INFO,"The vrisc cpu has started !!!\n");
}

void vrisc_reset(void)
{
    venc_write_register(TAHOE_VRISC_CORECFG, 0);
}

void create_channel(handle_t handle)
{
    int data = 0;
    VLOG(INFO,"[%s][%d]create_channel start\n", __func__, __LINE__);
    venc_info_t *venc = &s_venc_info;
    ulong fw_addr = 0;
    disable_host_scrw_irq();

    venc_write_register(CHN_ID, handle.chn_id);
    venc_write_register(ENC_FMT, handle.enc_fmt);  // 0: h264; 1: h265;
    venc_write_register(MFDIN_INPUT_FMT, handle.mfdin_input_fmt); //input_fmt
    venc_write_register(ENC_PIC_W, handle.enc_width);
    venc_write_register(ENC_PIC_H, handle.enc_height);

    fw_addr = venc->common_buffer[handle.chn_id].phys_addr;
    venc_write_register(MEM_POOL_ADDR_MSB, fw_addr >> 32 & 0x03);
    venc_write_register(MEM_POOL_ADDR_LSB, fw_addr); //fw memory pool
    VLOG(ERR,"msb:%x,lsb:%x\n",venc_read_register(MEM_POOL_ADDR_MSB),venc_read_register(MEM_POOL_ADDR_LSB));

    venc_write_register(ENC_FPS, handle.enc_fps);
    venc_write_register(ENC_GOP, handle.enc_gop);
    venc_write_register(ENC_PROFILE, handle.profile);
    venc_write_register(ENC_BITRATE, handle.enc_bitrate);
    venc_write_register(ENC_LEVEL, (handle.level & 0xff) << 0 | (handle.tier & 0xff ) << 16);
    venc_write_register(ENC_RC_MODE, (handle.enc_rc_mode & 0xff) |
                                     (handle.init_qp & 0xff) << 8 |
                                     (handle.fix_qp_I & 0xff) << 16 |
                                     (handle.fix_qp_P & 0xff) << 24);

    venc_write_register(MFDI_ROTATION_MODE, (handle.rotation_mode & 0x3) << 1 |
                                            (handle.mirror_mode & 0x3) << 3);

    venc_write_register(MFDI_CROP_START, (handle.crop_x & 0x3fff) << 0 |
                                        (handle.crop_y & 0x3fff) << 16);
    venc_write_register(ENC_PARA_CROP_WH, (handle.crop_w & 0x3fff) << 0 |
                                            (handle.crop_h & 0x3fff) << 16);
    venc_write_register(GOLDEN_MATCH_FLAG, handle.golden_match_flag);

    if (handle.svc_enable) {
        venc_write_register(CRE_SVC_LTR,0x03);
        VLOG(ERR,"svc_enable! 3 layers\n");
    }

    enable_vrisc_scrw_irq();

    venc_write_register(CMD_OPT_CODE, CMD_CREATE_CHN);
    data = venc_read_register(CMD_OPT_CODE);
    while (CMD_DONE != data) {
        data = venc_read_register(CMD_OPT_CODE);
    }
    VLOG(INFO,"[%s][%d]create_channel end, handle->enc_bitrate %d, golden_match_flag %d, fix_qp_I %d \n", __func__, __LINE__, handle.enc_bitrate, handle.golden_match_flag, handle.fix_qp_I);
}

void destroy_channel(handle_t handle)
{
    int data = 0;
    VLOG(INFO,"[%s][%d]destroy_channel start\n", __func__, __LINE__);
    disable_host_scrw_irq();

    venc_write_register(CHN_ID, handle.chn_id);

    enable_vrisc_scrw_irq();

    venc_write_register(CMD_OPT_CODE, CMD_DESTROY_CHN);
    data = venc_read_register(CMD_OPT_CODE);
    while (CMD_DONE != data) {
        data = venc_read_register(CMD_OPT_CODE);
    }
    VLOG(INFO,"[%s][%d]destroy_channel end\n", __func__, __LINE__);
}

void encode_header(handle_t *handle)
{
    int data = 0;

    disable_host_scrw_irq();
    venc_write_register(CHN_ID, handle->chn_id);

    VLOG(INFO,"header_buf_addr %lx %lx %lx\n",handle->header_buf_addr[0],handle->header_buf_addr[1],handle->header_buf_addr[2]);
//for vps/sps/pps
    venc_write_register(HEADER_VPS_ADDR, handle->header_buf_addr[0]);
    venc_write_register(HEADER_SPS_ADDR, handle->header_buf_addr[1]);
    venc_write_register(HEADER_PPS_ADDR, handle->header_buf_addr[2]);
    venc_write_register(HEADER_ADDR_HIGHER, (((handle->header_buf_addr[1] >> 32) & 0x3) | \
                                            (((handle->header_buf_addr[2] >> 32) & 0x03) << 2) | \
                                            (((handle->header_buf_addr[0] >> 32) & 0x03) << 4)));

    VLOG(INFO,"msb: %x %x %x\n",((handle->header_buf_addr[1] >> 32) & 0x3),(((handle->header_buf_addr[2] >> 32) & 0x03) << 2),(((handle->header_buf_addr[0] >> 32) & 0x03) << 4));
    VLOG(INFO,"[%s][%d]handle->header_buff 0:0x%lx, 1:0x%lx, 2:0x%lx msb:%x\n", \
        __func__,__LINE__,
        handle->header_buf_addr[0], \
        handle->header_buf_addr[1], \
        handle->header_buf_addr[2], \
        venc_read_register(HEADER_ADDR_HIGHER));

    enable_vrisc_scrw_irq();
    venc_write_register(CMD_OPT_CODE, CMD_GET_ENC_PARAMETERS);
    data = venc_read_register(CMD_OPT_CODE);

    while (CMD_DONE != data) {
        data = venc_read_register(CMD_OPT_CODE);
    }
    VLOG(INFO,"[%s][%d]encode_header end\n", __func__, __LINE__);
}

void osd_config(handle_t *handle)
{
    int data = 0;
    int format = 0;
    int i = 0;

    disable_host_scrw_irq();
    venc_write_register(CHN_ID, handle->chn_id);
    venc_write_register(PRP_OSD_COMMON_0, 0);
    venc_write_register(PRP_OSD_COMMON_1, 0);
    venc_write_register(PRP_OSD_COMMON_2, 0);
    venc_write_register(PRP_OSD_COMMON_3, 0);
    venc_write_register(PRP_OSD_COMMON_4, 0);
    venc_write_register(PRP_OSD_COMMON_5, 0);
    venc_write_register(PRP_OSD_COMMON_6, 0);
    venc_write_register(PRP_OSD_COMMON_7, 0);

    if (handle->tahoe_enc_pic.osd_enable) {
        for (i = 0; i < MAX_OSD_NUM; i++) {
            if (handle->tahoe_enc_pic.osd_config[i].enable == 1) {
                handle->tahoe_enc_pic.osd_config[i].y_baseaddr_h2bit = ((handle->tahoe_enc_pic.osd_config[i].y_baseaddr >> 32) & 0x03);
                handle->tahoe_enc_pic.osd_config[i].c_baseaddr_h2bit = handle->tahoe_enc_pic.osd_config[i].y_baseaddr_h2bit;
                VLOG(INFO,"osd [%d] y_addr:%lx,h2bit:%x\n",i,handle->tahoe_enc_pic.osd_config[i].y_baseaddr,handle->tahoe_enc_pic.osd_config[i].y_baseaddr_h2bit);
                handle->tahoe_enc_pic.osd[i].prp_osd_common = (handle->tahoe_enc_pic.osd_config[i].enable << 0) |\
                    (handle->tahoe_enc_pic.osd_config[i].format << 1) |\
                    (handle->tahoe_enc_pic.osd_config[i].alpha_mode << 4) |\
                    (handle->tahoe_enc_pic.osd_config[i].r2y_mode << 5) | \
                    (handle->tahoe_enc_pic.osd_config[i].endian << 7) |\
                    (handle->tahoe_enc_pic.osd_config[i].y_baseaddr_h2bit << 11) |\
                    (handle->tahoe_enc_pic.osd_config[i].c_baseaddr_h2bit << 13) |\
                    (handle->tahoe_enc_pic.osd_config[i].cmn_rsv << 15);
                handle->tahoe_enc_pic.osd[i].prp_osd_region_lt = (handle->tahoe_enc_pic.osd_config[i].region_x << 0) |\
                    (handle->tahoe_enc_pic.osd_config[i].region_y << 16);
                handle->tahoe_enc_pic.osd[i].prp_osd_region_wh = (handle->tahoe_enc_pic.osd_config[i].region_width << 0) |
                    (handle->tahoe_enc_pic.osd_config[i].region_height << 16);
                handle->tahoe_enc_pic.osd[i].prp_osd_bitmapa = (handle->tahoe_enc_pic.osd_config[i].alpha << 0) |\
                    (handle->tahoe_enc_pic.osd_config[i].bitmap_y << 8) |\
                    (handle->tahoe_enc_pic.osd_config[i].bitmap_u << 16) |\
                    (handle->tahoe_enc_pic.osd_config[i].bitmap_v << 24);
                handle->tahoe_enc_pic.osd[i].prp_osd_y_baseaddr = handle->tahoe_enc_pic.osd_config[i].y_baseaddr;
                handle->tahoe_enc_pic.osd[i].prp_osd_c_baseaddr = handle->tahoe_enc_pic.osd_config[i].c_baseaddr;
                handle->tahoe_enc_pic.osd[i].prp_osd_stride = handle->tahoe_enc_pic.osd_config[i].stride;
                if (i == 0) {
                    venc_write_register(PRP_OSD_COMMON_0, handle->tahoe_enc_pic.osd[i].prp_osd_common);
                    venc_write_register(PRP_OSD_REGION_LT_0, handle->tahoe_enc_pic.osd[i].prp_osd_region_lt);
                    venc_write_register(PRP_OSD_REGION_WH_0, handle->tahoe_enc_pic.osd[i].prp_osd_region_wh);
                    venc_write_register(PRP_OSD_BITMAPA_0, handle->tahoe_enc_pic.osd[i].prp_osd_bitmapa);
                    venc_write_register(PRP_OSD_Y_BASEADDR_0, (handle->tahoe_enc_pic.osd[i].prp_osd_y_baseaddr << 0));
                    venc_write_register(PRP_OSD_STRIDE_0_1, (handle->tahoe_enc_pic.osd[i].prp_osd_stride << 16));
                    VLOG(INFO,"[%s][%d]handle->tahoe_enc_pic.osd[%d].prp_osd_common = 0x%x\n", __func__, __LINE__, i, handle->tahoe_enc_pic.osd[i].prp_osd_common);
                    VLOG(INFO,"[%s][%d]handle->tahoe_enc_pic.osd[%d].prp_osd_region_lt = 0x%x\n", __func__, __LINE__, i, handle->tahoe_enc_pic.osd[i].prp_osd_region_lt);
                    VLOG(INFO,"[%s][%d]handle->tahoe_enc_pic.osd[%d].prp_osd_region_wh = 0x%x\n", __func__, __LINE__, i, handle->tahoe_enc_pic.osd[i].prp_osd_region_wh);
                    VLOG(INFO,"[%s][%d]handle->tahoe_enc_pic.osd[%d].prp_osd_bitmapa = 0x%x\n", __func__, __LINE__, i, handle->tahoe_enc_pic.osd[i].prp_osd_bitmapa);
                    VLOG(INFO,"[%s][%d]handle->tahoe_enc_pic.osd[%d].prp_osd_y_baseaddr = 0x%x\n", __func__, __LINE__, i, handle->tahoe_enc_pic.osd[i].prp_osd_y_baseaddr);
                    VLOG(INFO,"[%s][%d]handle->tahoe_enc_pic.osd[%d].prp_osd_stride = 0x%x\n", __func__, __LINE__, i, handle->tahoe_enc_pic.osd[i].prp_osd_stride);
                }
                if (i == 1) {
                    venc_write_register(PRP_OSD_COMMON_1, handle->tahoe_enc_pic.osd[i].prp_osd_common);
                    venc_write_register(PRP_OSD_REGION_LT_1,handle->tahoe_enc_pic.osd[i].prp_osd_region_lt);
                    venc_write_register(PRP_OSD_REGION_WH_1, handle->tahoe_enc_pic.osd[i].prp_osd_region_wh);
                    venc_write_register(PRP_OSD_BITMAPA_1, handle->tahoe_enc_pic.osd[i].prp_osd_bitmapa);
                    venc_write_register(PRP_OSD_Y_BASEADDR_1, (handle->tahoe_enc_pic.osd[i].prp_osd_y_baseaddr << 0));
                    venc_write_register(PRP_OSD_STRIDE_0_1, (handle->tahoe_enc_pic.osd[i].prp_osd_stride |\
                            venc_read_register(PRP_OSD_STRIDE_0_1)));
                    VLOG(INFO,"[%s][%d]handle->tahoe_enc_pic.osd[%d].prp_osd_common = 0x%x\n", __func__, __LINE__, i, handle->tahoe_enc_pic.osd[i].prp_osd_common);
                    VLOG(INFO,"[%s][%d]handle->tahoe_enc_pic.osd[%d].prp_osd_region_lt = 0x%x\n", __func__, __LINE__, i, handle->tahoe_enc_pic.osd[i].prp_osd_region_lt);
                    VLOG(INFO,"[%s][%d]handle->tahoe_enc_pic.osd[%d].prp_osd_region_wh = 0x%x\n", __func__, __LINE__, i, handle->tahoe_enc_pic.osd[i].prp_osd_region_wh);
                    VLOG(INFO,"[%s][%d]handle->tahoe_enc_pic.osd[%d].prp_osd_bitmapa = 0x%x\n", __func__, __LINE__, i, handle->tahoe_enc_pic.osd[i].prp_osd_bitmapa);
                    VLOG(INFO,"[%s][%d]handle->tahoe_enc_pic.osd[%d].prp_osd_y_baseaddr = 0x%x\n", __func__, __LINE__, i, handle->tahoe_enc_pic.osd[i].prp_osd_y_baseaddr);
                    VLOG(INFO,"[%s][%d]handle->tahoe_enc_pic.osd[%d].prp_osd_stride = 0x%x\n", __func__, __LINE__, i, handle->tahoe_enc_pic.osd[i].prp_osd_stride);
                }
                if (i == 2) {
                    venc_write_register(PRP_OSD_COMMON_2, handle->tahoe_enc_pic.osd[i].prp_osd_common);
                    venc_write_register(PRP_OSD_REGION_LT_2, handle->tahoe_enc_pic.osd[i].prp_osd_region_lt);
                    venc_write_register(PRP_OSD_REGION_WH_2, handle->tahoe_enc_pic.osd[i].prp_osd_region_wh);
                    venc_write_register(PRP_OSD_BITMAPA_2, handle->tahoe_enc_pic.osd[i].prp_osd_bitmapa);
                    venc_write_register(PRP_OSD_Y_BASEADDR_2, (handle->tahoe_enc_pic.osd[i].prp_osd_y_baseaddr << 0));
                    venc_write_register(PRP_OSD_STRIDE_2_3, (handle->tahoe_enc_pic.osd[i].prp_osd_stride << 16));
                    VLOG(INFO,"[%s][%d]handle->tahoe_enc_pic.osd[%d].prp_osd_common = 0x%x\n", __func__, __LINE__, i, handle->tahoe_enc_pic.osd[i].prp_osd_common);
                    VLOG(INFO,"[%s][%d]handle->tahoe_enc_pic.osd[%d].prp_osd_region_lt = 0x%x\n", __func__, __LINE__, i, handle->tahoe_enc_pic.osd[i].prp_osd_region_lt);
                    VLOG(INFO,"[%s][%d]handle->tahoe_enc_pic.osd[%d].prp_osd_region_wh = 0x%x\n", __func__, __LINE__, i, handle->tahoe_enc_pic.osd[i].prp_osd_region_wh);
                    VLOG(INFO,"[%s][%d]handle->tahoe_enc_pic.osd[%d].prp_osd_bitmapa = 0x%x\n", __func__, __LINE__, i, handle->tahoe_enc_pic.osd[i].prp_osd_bitmapa);
                    VLOG(INFO,"[%s][%d]handle->tahoe_enc_pic.osd[%d].prp_osd_y_baseaddr = 0x%x\n", __func__, __LINE__, i, handle->tahoe_enc_pic.osd[i].prp_osd_y_baseaddr);
                    VLOG(INFO,"[%s][%d]handle->tahoe_enc_pic.osd[%d].prp_osd_stride = 0x%x\n", __func__, __LINE__, i, handle->tahoe_enc_pic.osd[i].prp_osd_stride);
                }
                if (i == 3) {
                    venc_write_register(PRP_OSD_COMMON_3, handle->tahoe_enc_pic.osd[i].prp_osd_common);
                    venc_write_register(PRP_OSD_REGION_LT_3, handle->tahoe_enc_pic.osd[i].prp_osd_region_lt);
                    venc_write_register(PRP_OSD_REGION_WH_3, handle->tahoe_enc_pic.osd[i].prp_osd_region_wh);
                    venc_write_register(PRP_OSD_BITMAPA_3, handle->tahoe_enc_pic.osd[i].prp_osd_bitmapa);
                    venc_write_register(PRP_OSD_Y_BASEADDR_3, (handle->tahoe_enc_pic.osd[i].prp_osd_y_baseaddr << 0));
                    venc_write_register(PRP_OSD_STRIDE_2_3, (handle->tahoe_enc_pic.osd[i].prp_osd_stride |\
                            venc_read_register(PRP_OSD_STRIDE_2_3)));
                    VLOG(INFO,"[%s][%d]handle->tahoe_enc_pic.osd[%d].prp_osd_common = 0x%x\n", __func__, __LINE__, i, handle->tahoe_enc_pic.osd[i].prp_osd_common);
                    VLOG(INFO,"[%s][%d]handle->tahoe_enc_pic.osd[%d].prp_osd_region_lt = 0x%x\n", __func__, __LINE__, i, handle->tahoe_enc_pic.osd[i].prp_osd_region_lt);
                    VLOG(INFO,"[%s][%d]handle->tahoe_enc_pic.osd[%d].prp_osd_region_wh = 0x%x\n", __func__, __LINE__, i, handle->tahoe_enc_pic.osd[i].prp_osd_region_wh);
                    VLOG(INFO,"[%s][%d]handle->tahoe_enc_pic.osd[%d].prp_osd_bitmapa = 0x%x\n", __func__, __LINE__, i, handle->tahoe_enc_pic.osd[i].prp_osd_bitmapa);
                    VLOG(INFO,"[%s][%d]handle->tahoe_enc_pic.osd[%d].prp_osd_y_baseaddr = 0x%x\n", __func__, __LINE__, i, handle->tahoe_enc_pic.osd[i].prp_osd_y_baseaddr);
                    VLOG(INFO,"[%s][%d]handle->tahoe_enc_pic.osd[%d].prp_osd_stride = 0x%x\n", __func__, __LINE__, i, handle->tahoe_enc_pic.osd[i].prp_osd_stride);
                }
                if (i == 4) {
                    venc_write_register(PRP_OSD_COMMON_4, handle->tahoe_enc_pic.osd[i].prp_osd_common);
                    venc_write_register(PRP_OSD_REGION_LT_4, handle->tahoe_enc_pic.osd[i].prp_osd_region_lt);
                    venc_write_register(PRP_OSD_REGION_WH_4, handle->tahoe_enc_pic.osd[i].prp_osd_region_wh);
                    venc_write_register(PRP_OSD_BITMAPA_4, handle->tahoe_enc_pic.osd[i].prp_osd_bitmapa);
                    venc_write_register(PRP_OSD_Y_BASEADDR_4, (handle->tahoe_enc_pic.osd[i].prp_osd_y_baseaddr << 0));
                    venc_write_register(PRP_OSD_STRIDE_4_5, (handle->tahoe_enc_pic.osd[i].prp_osd_stride << 16));
                    VLOG(INFO,"[%s][%d]handle->tahoe_enc_pic.osd[%d].prp_osd_common = 0x%x\n", __func__, __LINE__, i, handle->tahoe_enc_pic.osd[i].prp_osd_common);
                    VLOG(INFO,"[%s][%d]handle->tahoe_enc_pic.osd[%d].prp_osd_region_lt = 0x%x\n", __func__, __LINE__, i, handle->tahoe_enc_pic.osd[i].prp_osd_region_lt);
                    VLOG(INFO,"[%s][%d]handle->tahoe_enc_pic.osd[%d].prp_osd_region_wh = 0x%x\n", __func__, __LINE__, i, handle->tahoe_enc_pic.osd[i].prp_osd_region_wh);
                    VLOG(INFO,"[%s][%d]handle->tahoe_enc_pic.osd[%d].prp_osd_bitmapa = 0x%x\n", __func__, __LINE__, i, handle->tahoe_enc_pic.osd[i].prp_osd_bitmapa);
                    VLOG(INFO,"[%s][%d]handle->tahoe_enc_pic.osd[%d].prp_osd_y_baseaddr = 0x%x\n", __func__, __LINE__, i, handle->tahoe_enc_pic.osd[i].prp_osd_y_baseaddr);
                    VLOG(INFO,"[%s][%d]handle->tahoe_enc_pic.osd[%d].prp_osd_stride = 0x%x\n", __func__, __LINE__, i, handle->tahoe_enc_pic.osd[i].prp_osd_stride);
                }
                if (i == 5) {
                    venc_write_register(PRP_OSD_COMMON_5, handle->tahoe_enc_pic.osd[i].prp_osd_common);
                    venc_write_register(PRP_OSD_REGION_LT_5, handle->tahoe_enc_pic.osd[i].prp_osd_region_lt);
                    venc_write_register(PRP_OSD_REGION_WH_5, handle->tahoe_enc_pic.osd[i].prp_osd_region_wh);
                    venc_write_register(PRP_OSD_BITMAPA_5, handle->tahoe_enc_pic.osd[i].prp_osd_bitmapa);
                    venc_write_register(PRP_OSD_Y_BASEADDR_5, (handle->tahoe_enc_pic.osd[i].prp_osd_y_baseaddr << 0));
                    venc_write_register(PRP_OSD_STRIDE_4_5, (handle->tahoe_enc_pic.osd[i].prp_osd_stride |\
                            venc_read_register(PRP_OSD_STRIDE_4_5)));
                    VLOG(INFO,"[%s][%d]handle->tahoe_enc_pic.osd[%d].prp_osd_common = 0x%x\n", __func__, __LINE__, i, handle->tahoe_enc_pic.osd[i].prp_osd_common);
                    VLOG(INFO,"[%s][%d]handle->tahoe_enc_pic.osd[%d].prp_osd_region_lt = 0x%x\n", __func__, __LINE__, i, handle->tahoe_enc_pic.osd[i].prp_osd_region_lt);
                    VLOG(INFO,"[%s][%d]handle->tahoe_enc_pic.osd[%d].prp_osd_region_wh = 0x%x\n", __func__, __LINE__, i, handle->tahoe_enc_pic.osd[i].prp_osd_region_wh);
                    VLOG(INFO,"[%s][%d]handle->tahoe_enc_pic.osd[%d].prp_osd_bitmapa = 0x%x\n", __func__, __LINE__, i, handle->tahoe_enc_pic.osd[i].prp_osd_bitmapa);
                    VLOG(INFO,"[%s][%d]handle->tahoe_enc_pic.osd[%d].prp_osd_y_baseaddr = 0x%x\n", __func__, __LINE__, i, handle->tahoe_enc_pic.osd[i].prp_osd_y_baseaddr);
                    VLOG(INFO,"[%s][%d]handle->tahoe_enc_pic.osd[%d].prp_osd_stride = 0x%x\n", __func__, __LINE__, i, handle->tahoe_enc_pic.osd[i].prp_osd_stride);
                }
                if (i == 6) {
                    venc_write_register(PRP_OSD_COMMON_6, handle->tahoe_enc_pic.osd[i].prp_osd_common);
                    venc_write_register(PRP_OSD_REGION_LT_6, handle->tahoe_enc_pic.osd[i].prp_osd_region_lt);
                    venc_write_register(PRP_OSD_REGION_WH_6, handle->tahoe_enc_pic.osd[i].prp_osd_region_wh);
                    venc_write_register(PRP_OSD_BITMAPA_6, handle->tahoe_enc_pic.osd[i].prp_osd_bitmapa);
                    venc_write_register(PRP_OSD_Y_BASEADDR_6, (handle->tahoe_enc_pic.osd[i].prp_osd_y_baseaddr << 0));
                    venc_write_register(PRP_OSD_STRIDE_6_7, (handle->tahoe_enc_pic.osd[i].prp_osd_stride << 16));
                    VLOG(INFO,"[%s][%d]handle->tahoe_enc_pic.osd[%d].prp_osd_common = 0x%x\n", __func__, __LINE__, i, handle->tahoe_enc_pic.osd[i].prp_osd_common);
                    VLOG(INFO,"[%s][%d]handle->tahoe_enc_pic.osd[%d].prp_osd_region_lt = 0x%x\n", __func__, __LINE__, i, handle->tahoe_enc_pic.osd[i].prp_osd_region_lt);
                    VLOG(INFO,"[%s][%d]handle->tahoe_enc_pic.osd[%d].prp_osd_region_wh = 0x%x\n", __func__, __LINE__, i, handle->tahoe_enc_pic.osd[i].prp_osd_region_wh);
                    VLOG(INFO,"[%s][%d]handle->tahoe_enc_pic.osd[%d].prp_osd_bitmapa = 0x%x\n", __func__, __LINE__, i, handle->tahoe_enc_pic.osd[i].prp_osd_bitmapa);
                    VLOG(INFO,"[%s][%d]handle->tahoe_enc_pic.osd[%d].prp_osd_y_baseaddr = 0x%x\n", __func__, __LINE__, i, handle->tahoe_enc_pic.osd[i].prp_osd_y_baseaddr);
                    VLOG(INFO,"[%s][%d]handle->tahoe_enc_pic.osd[%d].prp_osd_stride = 0x%x\n", __func__, __LINE__, i, handle->tahoe_enc_pic.osd[i].prp_osd_stride);
                }
                if (i == 7) {
                    venc_write_register(PRP_OSD_COMMON_7, handle->tahoe_enc_pic.osd[i].prp_osd_common);
                    venc_write_register(PRP_OSD_REGION_LT_7, handle->tahoe_enc_pic.osd[i].prp_osd_region_lt);
                    venc_write_register(PRP_OSD_REGION_WH_7, handle->tahoe_enc_pic.osd[i].prp_osd_region_wh);
                    venc_write_register(PRP_OSD_BITMAPA_7, handle->tahoe_enc_pic.osd[i].prp_osd_bitmapa);
                    venc_write_register(PRP_OSD_Y_BASEADDR_7, (handle->tahoe_enc_pic.osd[i].prp_osd_y_baseaddr << 0));
                    venc_write_register(PRP_OSD_STRIDE_6_7, (handle->tahoe_enc_pic.osd[i].prp_osd_stride |\
                            venc_read_register(PRP_OSD_STRIDE_6_7)));
                    VLOG(INFO,"[%s][%d]handle->tahoe_enc_pic.osd[%d].prp_osd_common = 0x%x\n", __func__, __LINE__, i, handle->tahoe_enc_pic.osd[i].prp_osd_common);
                    VLOG(INFO,"[%s][%d]handle->tahoe_enc_pic.osd[%d].prp_osd_region_lt = 0x%x\n", __func__, __LINE__, i, handle->tahoe_enc_pic.osd[i].prp_osd_region_lt);
                    VLOG(INFO,"[%s][%d]handle->tahoe_enc_pic.osd[%d].prp_osd_region_wh = 0x%x\n", __func__, __LINE__, i, handle->tahoe_enc_pic.osd[i].prp_osd_region_wh);
                    VLOG(INFO,"[%s][%d]handle->tahoe_enc_pic.osd[%d].prp_osd_bitmapa = 0x%x\n", __func__, __LINE__, i, handle->tahoe_enc_pic.osd[i].prp_osd_bitmapa);
                    VLOG(INFO,"[%s][%d]handle->tahoe_enc_pic.osd[%d].prp_osd_y_baseaddr = 0x%x\n", __func__, __LINE__, i, handle->tahoe_enc_pic.osd[i].prp_osd_y_baseaddr);
                    VLOG(INFO,"[%s][%d]handle->tahoe_enc_pic.osd[%d].prp_osd_stride = 0x%x\n", __func__, __LINE__, i, handle->tahoe_enc_pic.osd[i].prp_osd_stride);
                }
            }
        }
    }

    enable_vrisc_scrw_irq();
    venc_write_register(CMD_OPT_CODE, CMD_SET_OSD_INFO);

    data = venc_read_register(CMD_OPT_CODE);

    while (CMD_DONE != data) {
        data = venc_read_register(CMD_OPT_CODE);
    }

    data = venc_read_register(TAHOE_DEBUG_6);
    VLOG(INFO,"==FW cmd Osdqq OUT, TAHOE_DEBUG_6 qq = 0x%x\n", data);
}

void ds_config(handle_t *handle)
{
    if (handle) {
        VLOG(INFO,"[%s][%d]handle->ds_h:%d\n",__func__,__LINE__, handle->ds_h);
        VLOG(INFO,"[%s][%d]handle->ds_w:%d\n",__func__,__LINE__, handle->ds_w);
        VLOG(INFO,"[%s][%d]handle->ds_osd:%d\n",__func__,__LINE__, handle->ds_osd);
        VLOG(INFO,"[%s][%d]handle->ds_only:%d\n",__func__,__LINE__, handle->ds_only);
        //if (handle->ds_w > 0 && handle->ds_h > 0)
        venc_write_register(DS_PARENT_CHN, ((handle->ds_h & 0xfff) << 14) |  ((handle->ds_w & 0xfff) << 2) | ((handle->ds_osd & 0x1) << 1) | (handle->ds_only & 0x1));
    }
}

void roi_config(handle_t *handle)
{
    int data = 0;
    int i = 0;

    disable_host_scrw_irq();
    venc_write_register(CHN_ID, handle->chn_id);
    venc_write_register(ROI_CTRL, 0);

    if (handle->tahoe_enc_pic.prp_roi_map_enable) {
        venc_write_register(ROI_CTRL, (handle->tahoe_enc_pic.prp_roi_map_enable << 0));
        venc_write_register(ROI_MAP_ADDRL, (handle->tahoe_enc_pic.prp_roi_map_addr << 0));
        venc_write_register(ROI_CTRL, (venc_read_register(ROI_CTRL)|(((handle->tahoe_enc_pic.prp_roi_map_addr >> 32) & 0x03) << 6)));
        VLOG(INFO,"%s, %d, handle->tahoe_enc_pic.prp_roi_map_enable:%d, handle->chn_id %d\n", __func__, __LINE__, handle->tahoe_enc_pic.prp_roi_map_enable, handle->chn_id);
    }

    if (handle->tahoe_enc_pic.roi_enable) {
        for (i = 0; i < MAX_ROI_NUM; i++) {
            handle->tahoe_enc_pic.roi[i].prp_roi_region_start = (handle->tahoe_enc_pic.roi_config[i].region_x << 0) |\
                (handle->tahoe_enc_pic.roi_config[i].region_y << 16);
            handle->tahoe_enc_pic.roi[i].prp_roi_region_size = (handle->tahoe_enc_pic.roi_config[i].region_w << 0) |\
                (handle->tahoe_enc_pic.roi_config[i].region_h << 16);
            handle->tahoe_enc_pic.roi[i].prp_roi_region_param = (handle->tahoe_enc_pic.roi_config[i].region_inv << 0) |\
                (handle->tahoe_enc_pic.roi_config[i].QpMode << 1) |\
                (handle->tahoe_enc_pic.roi_config[i].roiQp << 2) |\
                (handle->tahoe_enc_pic.roi_config[i].lossless << 8) |\
                (handle->tahoe_enc_pic.roi_config[i].EncBiasMode << 9) |\
                (handle->tahoe_enc_pic.roi_config[i].minQp << 14) |\
                (handle->tahoe_enc_pic.roi_config[i].maxQp << 20);

            if (handle->tahoe_enc_pic.roi_config[i].enable && i == 0) {
                venc_write_register(ROI_CTRL, (venc_read_register(ROI_CTRL)|(1<<2)));
                venc_write_register(ROI_REGION_START_0, handle->tahoe_enc_pic.roi[i].prp_roi_region_start);
                venc_write_register(ROI_REGION_SIZE_0, handle->tahoe_enc_pic.roi[i].prp_roi_region_size);
                venc_write_register(ROI_REGION_PARAM_0, handle->tahoe_enc_pic.roi[i].prp_roi_region_param);
                VLOG(INFO,"i %d, ROI_REGION_PARAM_0 == 0x%x\n", i, venc_read_register(ROI_REGION_PARAM_0));
            }

            if (handle->tahoe_enc_pic.roi_config[i].enable && i == 1) {
                venc_write_register(ROI_CTRL,(venc_read_register(ROI_CTRL)|(1<<3)));
                venc_write_register(ROI_REGION_START_1, handle->tahoe_enc_pic.roi[i].prp_roi_region_start);
                venc_write_register(ROI_REGION_SIZE_1, handle->tahoe_enc_pic.roi[i].prp_roi_region_size);
                venc_write_register(ROI_REGION_PARAM_1, handle->tahoe_enc_pic.roi[i].prp_roi_region_param);
                VLOG(INFO,"i %d, ROI_REGION_PARAM_1 == 0x%x\n", i, venc_read_register(ROI_REGION_PARAM_1));
            }

            if (handle->tahoe_enc_pic.roi_config[i].enable && i == 2) {
                venc_write_register(ROI_CTRL,(venc_read_register(ROI_CTRL)|(1<<4)));
                venc_write_register(ROI_REGION_START_2, handle->tahoe_enc_pic.roi[i].prp_roi_region_start);
                venc_write_register(ROI_REGION_SIZE_2, handle->tahoe_enc_pic.roi[i].prp_roi_region_size);
                venc_write_register(ROI_REGION_PARAM_2, handle->tahoe_enc_pic.roi[i].prp_roi_region_param);
                VLOG(INFO,"i %d, ROI_REGION_PARAM_2 == 0x%x\n", i, venc_read_register(ROI_REGION_PARAM_2));
            }

            if (handle->tahoe_enc_pic.roi_config[i].enable && i == 3) {
                venc_write_register(ROI_CTRL,(venc_read_register(ROI_CTRL)|(1<<5)));
                venc_write_register(ROI_REGION_START_3, handle->tahoe_enc_pic.roi[i].prp_roi_region_start);
                venc_write_register(ROI_REGION_SIZE_3, handle->tahoe_enc_pic.roi[i].prp_roi_region_size);
                venc_write_register(ROI_REGION_PARAM_3, handle->tahoe_enc_pic.roi[i].prp_roi_region_param);
                VLOG(INFO,"i %d, ROI_REGION_PARAM_3 == 0x%x\n", i, venc_read_register(ROI_REGION_PARAM_3));
            }
        }
    }
    enable_vrisc_scrw_irq();
    venc_write_register(CMD_OPT_CODE, CMD_SET_ENC_PRP);

    while (venc_read_register(CMD_OPT_CODE) != CMD_DONE)
    ;
    data = venc_read_register(TAHOE_DEBUG_3);
    VLOG(INFO,"==FW cmd ROS qq OUT, TAHOE_DEBUG_3 qq = 0x%x\n", data);
}

void dynamic_parameter_set(handle_t *handle)
{
    int data = 0;

    disable_host_scrw_irq();

    venc_write_register(ENC_PARAM_FPS, handle->enc_fps);
    venc_write_register(ENC_PARAM_GOP, handle->enc_gop);
    venc_write_register(ENC_PARAM_BITRATE, handle->enc_bitrate);
    venc_write_register(ENC_PARAM_RC_MODE, (handle->enc_rc_mode & 0xff));
    VLOG(INFO,"[%s][%d]handle->enc_fps:%d, handle->enc_gop:%d, handle->enc_bitrate:%d, handle->enc_rc_mode:%d\n",__func__,__LINE__,
        handle->enc_fps, handle->enc_gop, handle->enc_bitrate, handle->enc_rc_mode);
    enable_vrisc_scrw_irq();
    venc_write_register(CMD_OPT_CODE, CMD_ENC_PARAMETER_SETS);
    data = venc_read_register(CMD_OPT_CODE);
    while (CMD_DONE != data) {
        data = venc_read_register(CMD_OPT_CODE);
    }
}

void encode_one_frame(handle_t *handle)
{
    int data = 0;
    int slice_qp = 0;

    venc_write_register(TAHOE_CFG,
            1 << 0 |        //prp_enable
            1 << 1 |        //imd_enable
            0 << 2 |        //me_enable
            0 << 3 |        //rdo_enable
            0 << 4 |        //lpf_enable
            1 << 5 |        //tctrl_enable
            1 << 6 |        //fbcd_enable
            0 << 7 );       //ec_enable
    disable_host_scrw_irq();
    venc_write_register(CHN_ID, handle->chn_id);

    ds_config(handle);

    //printf("[%s][%d]handle->mfdin_input_fmt %d\n", __func__,__LINE__, handle->mfdin_input_fmt);
    if ((handle->mfdin_input_fmt == MFDI_INPUT_FMT_NV12) ||
        (handle->mfdin_input_fmt == MFDI_INPUT_FMT_NV21)) {
        venc_write_register(PRP_MFDI_STRIDE_0, handle->buf_stride);
        venc_write_register(PRP_MFDI_STRIDE_1, handle->buf_stride);
    } else if (handle->mfdin_input_fmt == MFDI_INPUT_FMT_YUV420_PLANAR) {
        venc_write_register(PRP_MFDI_STRIDE_0, handle->buf_stride);
        venc_write_register(PRP_MFDI_STRIDE_1, handle->buf_stride / 2);
        venc_write_register(PRP_MFDI_STRIDE_2, handle->buf_stride / 2);
    } else if (handle->mfdin_input_fmt == MFDI_INPUT_FMT_RGB888) {
        venc_write_register(PRP_MFDI_STRIDE_0, handle->buf_stride * 3);
    } else if (handle->mfdin_input_fmt == MFDI_INPUT_FMT_RGBA8888) {
        venc_write_register(PRP_MFDI_STRIDE_0, handle->buf_stride * 4);
    }

    if (handle->enc_frame_type == FRAME_TYPE_I) {
        venc_write_register(ENC_PARA_FRAME_TYPE, FORCE_IDR_TYPE);
        VLOG(INFO,"[%s][%d]encode_one_frame force idr %d \n", __func__, __LINE__, handle->poc);
    }

    if (handle->poc_init <= 1) {
        venc_write_register(TAHOE_DQME_SEARCH_CFG, 0x840);
        handle->poc_init++;
    } else {
        if (handle->enc_fmt == CODEC_ID_H264)
            venc_write_register(TAHOE_DQME_SEARCH_CFG, 0x9a840);
        else
            venc_write_register(TAHOE_DQME_SEARCH_CFG, 0x66840);
    }
    VLOG(INFO,"buf_stride:%d,fmt:%d",handle->buf_stride,handle->mfdin_input_fmt);
    //printf("buf_stride:%d,fmt:%d",handle->buf_stride,handle->mfdin_input_fmt);
    venc_write_register(TAHOE_VRISC_SCRATCH_29, handle->tahoe_enc_pic.inpt_buf_addr[0]);//传yuv的地址；
    venc_write_register(TAHOE_VRISC_SCRATCH_30, handle->tahoe_enc_pic.inpt_buf_addr[1]);
    venc_write_register(TAHOE_VRISC_SCRATCH_31, handle->tahoe_enc_pic.inpt_buf_addr[2]);
    venc_write_register(INPUT_BUF_ADDR_MSB, ((handle->tahoe_enc_pic.inpt_buf_addr[0] >> 32) & 0x03) | \
                                            (((handle->tahoe_enc_pic.inpt_buf_addr[0] >> 32) & 0x03) << 2) | \
                                            (((handle->tahoe_enc_pic.inpt_buf_addr[0] >> 32) & 0x03) << 4));
    //printf("[%s][%d]y:0x%x, u:0x%x, v:0x%x\n", \
    //    __func__,__LINE__,
    //    handle->tahoe_enc_pic.inpt_buf_addr_lsb[0], \
    //    handle->tahoe_enc_pic.inpt_buf_addr_lsb[1], \
    //    handle->tahoe_enc_pic.inpt_buf_addr_lsb[2]);
    VLOG(INFO,"[%s][%d]y:0x%lx, u:0x%lx, v:0x%lx,msb:%x\n", \
        __func__,__LINE__,
        handle->tahoe_enc_pic.inpt_buf_addr[0], \
        handle->tahoe_enc_pic.inpt_buf_addr[1], \
        handle->tahoe_enc_pic.inpt_buf_addr[2], \
        venc_read_register(INPUT_BUF_ADDR_MSB));

    //venc_write_register(MFDI_ROTATION_MODE, (handle->rotation_mode & 0x3) << 1 |
    //                                        (handle->mirror_mode & 0x3) << 3);

    //venc_write_register(MFDI_CROP_START, (handle->crop_x & 0x3fff) << 0 |
    //                                    (handle->crop_y & 0x3fff) << 16);
    //venc_write_register(ENC_PARA_CROP_WH, (handle->crop_w & 0x3fff) << 0 |
    //                                        (handle->crop_h & 0x3fff) << 16);

    enable_vrisc_scrw_irq();
    venc_write_register(CMD_OPT_CODE, CMD_ENC_ONE_FRAME);
    data = venc_read_register(CMD_OPT_CODE);

    while (CMD_DONE != data) {
        data = venc_read_register(CMD_OPT_CODE);
    }
    //printf("[%s][%d]encode_one_frame end\n", __func__, __LINE__);
    VLOG(INFO,"[%s][%d]encode_one_frame end\n", __func__, __LINE__);
}

