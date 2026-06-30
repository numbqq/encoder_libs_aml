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
#ifndef __TAHOE_REGISTER_H__
#define __TAHOE_REGISTER_H__

typedef union TAHOE_VRISC_MISC_FIELD {
    unsigned int data;
    struct {
        unsigned int soft_reset : 16;
        unsigned int baseaddr_ilm_higher : 2;
        unsigned int baseaddr_rdma_higher : 2;
        unsigned int scatchw_irq_mask : 1;
        unsigned int vrisc_misc_rsv : 11;
    } b;
} TAHOE_VRISC_MISC_FIELD_T;

#define FORCE_IDR_TYPE                     2

#define TAHOE_VRISC_BASE        0x0000

#define TAHOE_VRISC_HAO         (TAHOE_VRISC_BASE + 0x00)
#define TAHOE_VRISC_MISC        (TAHOE_VRISC_BASE + 0x04)
#define TAHOE_VRISC_MPRELOAD    (TAHOE_VRISC_BASE + 0x08)
#define TAHOE_VRISC_MBASE       (TAHOE_VRISC_BASE + 0x0c)   //fw addr
#define TAHOE_VRISC_RSTVEC      (TAHOE_VRISC_BASE + 0x10)
#define TAHOE_VRISC_CORECFG     (TAHOE_VRISC_BASE + 0x14)
#define TAHOE_VRISC_RDMABASE    (TAHOE_VRISC_BASE + 0x18)
#define TAHOE_VRISC_RDMACMD     (TAHOE_VRISC_BASE + 0x1c)
#define TAHOE_VRISC_SCRATCH_0   (TAHOE_VRISC_BASE + 0x20)
#define TAHOE_VRISC_SCRATCH_1   (TAHOE_VRISC_BASE + 0x24)
#define TAHOE_VRISC_SCRATCH_2   (TAHOE_VRISC_BASE + 0x28)
#define TAHOE_VRISC_SCRATCH_3   (TAHOE_VRISC_BASE + 0x2c)
#define TAHOE_VRISC_SCRATCH_4   (TAHOE_VRISC_BASE + 0x30)
#define TAHOE_VRISC_SCRATCH_5   (TAHOE_VRISC_BASE + 0x34)
#define TAHOE_VRISC_SCRATCH_6   (TAHOE_VRISC_BASE + 0x38)
#define TAHOE_VRISC_SCRATCH_7   (TAHOE_VRISC_BASE + 0x3c)
#define TAHOE_VRISC_SCRATCH_8   (TAHOE_VRISC_BASE + 0x40)
#define TAHOE_VRISC_SCRATCH_9   (TAHOE_VRISC_BASE + 0x44)
#define TAHOE_VRISC_SCRATCH_10  (TAHOE_VRISC_BASE + 0x48)
#define TAHOE_VRISC_SCRATCH_11  (TAHOE_VRISC_BASE + 0x4c)
#define TAHOE_VRISC_SCRATCH_12  (TAHOE_VRISC_BASE + 0x50)
#define TAHOE_VRISC_SCRATCH_13  (TAHOE_VRISC_BASE + 0x54)
#define TAHOE_VRISC_SCRATCH_14  (TAHOE_VRISC_BASE + 0x58)
#define TAHOE_VRISC_SCRATCH_15  (TAHOE_VRISC_BASE + 0x5c)
#define TAHOE_VRISC_SCRATCH_16  (TAHOE_VRISC_BASE + 0x60)
#define TAHOE_VRISC_SCRATCH_17  (TAHOE_VRISC_BASE + 0x64)
#define TAHOE_VRISC_SCRATCH_18  (TAHOE_VRISC_BASE + 0x68)
#define TAHOE_VRISC_SCRATCH_19  (TAHOE_VRISC_BASE + 0x6c)
#define TAHOE_VRISC_SCRATCH_20  (TAHOE_VRISC_BASE + 0x70)
#define TAHOE_VRISC_SCRATCH_21  (TAHOE_VRISC_BASE + 0x74)
#define TAHOE_VRISC_SCRATCH_22  (TAHOE_VRISC_BASE + 0x78)
#define TAHOE_VRISC_SCRATCH_23  (TAHOE_VRISC_BASE + 0x7c)
#define TAHOE_VRISC_SCRATCH_24  (TAHOE_VRISC_BASE + 0x80)
#define TAHOE_VRISC_SCRATCH_25  (TAHOE_VRISC_BASE + 0x84)
#define TAHOE_VRISC_SCRATCH_26  (TAHOE_VRISC_BASE + 0x88)
#define TAHOE_VRISC_SCRATCH_27  (TAHOE_VRISC_BASE + 0x8c)
#define TAHOE_VRISC_SCRATCH_28  (TAHOE_VRISC_BASE + 0x90)
#define TAHOE_VRISC_SCRATCH_29  (TAHOE_VRISC_BASE + 0x94)
#define TAHOE_VRISC_SCRATCH_30  (TAHOE_VRISC_BASE + 0x98)
#define TAHOE_VRISC_SCRATCH_31  (TAHOE_VRISC_BASE + 0x9c)
#define TAHOE_VRISC_ASSIST_IRQ  (TAHOE_VRISC_BASE + 0xa0)
#define TAHOE_VRISC_IRQ_MASK    (TAHOE_VRISC_BASE + 0xa4)
#define TAHOE_VRISC_IRQ_STAT    (TAHOE_VRISC_BASE + 0xa8)
#define TAHOE_VRISC_EIRQ_MASK   (TAHOE_VRISC_BASE + 0xac)
#define TAHOE_VRISC_EIRQ_STAT   (TAHOE_VRISC_BASE + 0xb0)
#define TAHOE_DEBUG_0           (TAHOE_VRISC_BASE + 0xc0)
#define TAHOE_DEBUG_1           (TAHOE_VRISC_BASE + 0xc4)
#define TAHOE_DEBUG_2           (TAHOE_VRISC_BASE + 0xc8)
#define TAHOE_DEBUG_3           (TAHOE_VRISC_BASE + 0xcc)
#define TAHOE_DEBUG_4           (TAHOE_VRISC_BASE + 0xd0)
#define TAHOE_DEBUG_5           (TAHOE_VRISC_BASE + 0xd4)
#define TAHOE_DEBUG_6           (TAHOE_VRISC_BASE + 0xd8)
#define TAHOE_DEBUG_7           (TAHOE_VRISC_BASE + 0xdc)

#define TAHOE_TCTRL_TOP         (TAHOE_VRISC_BASE + 0x400)
#define TAHOE_TCTRL_WORK_MOD    (TAHOE_VRISC_BASE + 0x404)
#define TAHOE_VCE_SCRATCH_0     (TAHOE_VRISC_BASE + 0x410)
#define TAHOE_VCE_SCRATCH_1     (TAHOE_VRISC_BASE + 0x414)
#define TAHOE_VCE_SCRATCH_2     (TAHOE_VRISC_BASE + 0x418)
#define TAHOE_VCE_SCRATCH_3     (TAHOE_VRISC_BASE + 0x41c)
#define TAHOE_VCE_SCRATCH_4     (TAHOE_VRISC_BASE + 0x420)
#define TAHOE_VCE_SCRATCH_5     (TAHOE_VRISC_BASE + 0x424)
#define TAHOE_VCE_SCRATCH_6     (TAHOE_VRISC_BASE + 0x428)
#define TAHOE_VCE_SCRATCH_7     (TAHOE_VRISC_BASE + 0x42c)
#define TAHOE_VCE_SCRATCH_8     (TAHOE_VRISC_BASE + 0x430)
#define TAHOE_VCE_SCRATCH_9     (TAHOE_VRISC_BASE + 0x434)
#define TAHOE_VCE_SCRATCH_10    (TAHOE_VRISC_BASE + 0x438)
#define TAHOE_VCE_SCRATCH_11    (TAHOE_VRISC_BASE + 0x43c)
#define TAHOE_VCE_SCRATCH_12    (TAHOE_VRISC_BASE + 0x440)
#define TAHOE_VCE_SCRATCH_13    (TAHOE_VRISC_BASE + 0x444)
#define TAHOE_VCE_SCRATCH_14    (TAHOE_VRISC_BASE + 0x448)
#define TAHOE_VCE_SCRATCH_15    (TAHOE_VRISC_BASE + 0x44c)

/************************************************************************/
/* ENC_ENCODE (CHN PARAM)                                               */
/************************************************************************/
#define CMD_OPT_CODE                    TAHOE_VRISC_SCRATCH_0
#define CHN_ID                          TAHOE_VRISC_SCRATCH_1
#define ENC_FMT                         TAHOE_VRISC_SCRATCH_2   // 0: h264; 1: h265;
#define ENC_PROFILE                     TAHOE_VRISC_SCRATCH_3   //for es buffer
#define ENC_LEVEL                       TAHOE_VRISC_SCRATCH_4   //for poc
#define MFDIN_INPUT_FMT                 TAHOE_VRISC_SCRATCH_5
#define MFDI_LOW_DELAY_MODE             TAHOE_VRISC_SCRATCH_6
#define MFDI_ROTATION_MODE              TAHOE_VRISC_SCRATCH_7  // default = 0x0      rotation mode: 00:0 R 01:90R 10:180R 11:270R
#define MFDI_MIRROR_MODE                TAHOE_VRISC_SCRATCH_8  // default = 0x0      mirror mode: bit1 : mirror;  bit2: flip;
//#define MFDI_PADDING_MODE               TAHOE_VRISC_SCRATCH_9  // default = 0x0      mfdi padding mode, 0: copy 1: special value
#define ENC_PARA_CROP_WH                TAHOE_VRISC_SCRATCH_9
#define MFDI_CROP_START                 TAHOE_VRISC_SCRATCH_10 // default = 0x0      [15:0] x, [31:16] y
#define MFDIN_INPUT_W                   TAHOE_VRISC_SCRATCH_11
#define MFDIN_INPUT_STRIDE              TAHOE_VRISC_SCRATCH_12
#define MFDIN_INPUT_H                   TAHOE_VRISC_SCRATCH_13
#define DS_PARENT_CHN                   TAHOE_VRISC_SCRATCH_14
#define ENC_PIC_W                       TAHOE_VRISC_SCRATCH_15   // encode pic width
#define ENC_PIC_H                       TAHOE_VRISC_SCRATCH_16   // encode pic height
#define ENC_RC_MODE                     TAHOE_VRISC_SCRATCH_17
#define ENC_BITRATE                     TAHOE_VRISC_SCRATCH_18
#define ENC_FPS                         TAHOE_VRISC_SCRATCH_19  // fps
#define ENC_GOP                         TAHOE_VRISC_SCRATCH_20  // gop
#define ENC_SLICE_NUM                   TAHOE_VRISC_SCRATCH_21  // max slices
#define ENC_REF_NUM                     TAHOE_VRISC_SCRATCH_22  // ref num
#define ENC_B_FRAMES                    TAHOE_VRISC_SCRATCH_23  // b frames
#define CU_DEPTH_SEL                    TAHOE_VRISC_SCRATCH_24  // hevc depth num, 0-two depth, 1-three depth
#define MONO_FLAG                       TAHOE_VRISC_SCRATCH_25  // mono flag
#define WORK_MODE                       TAHOE_VRISC_SCRATCH_26  // 0: frame mode without epb wait1: frame mode with epb wait 2: epb mode3: epb segment mode
#define RFMU_EN                         TAHOE_VRISC_SCRATCH_27
#define RFMU_LOOP_EN                    TAHOE_VRISC_SCRATCH_28
#define EC_LOOP_EN                      TAHOE_VRISC_SCRATCH_29
//#define MEM_POOL_ADDR_LSB               TAHOE_VRISC_SCRATCH_30
//#define MEM_POOL_ADDR_MSB               TAHOE_VRISC_SCRATCH_31
#define MEM_POOL_ADDR_MSB               TAHOE_VCE_SCRATCH_12
#define MEM_POOL_ADDR_LSB               TAHOE_VCE_SCRATCH_13

#define ENC_MIN_QP_I                    TAHOE_VCE_SCRATCH_0
#define ENC_MAX_QP_I                    TAHOE_VCE_SCRATCH_1
#define ENC_MIN_QP_P                    TAHOE_VCE_SCRATCH_2
#define ENC_MAX_QP_P                    TAHOE_VCE_SCRATCH_3
#define ENC_QP_RANGE_DELTA              TAHOE_VCE_SCRATCH_4
#define ENC_INTRA_QP_OFFSET             TAHOE_VCE_SCRATCH_5
#define ENC_AQ_EN                       TAHOE_VCE_SCRATCH_6

/************************************************************************/
/* ENC_SET_PARAM (PRP-OSD)                                              */
/************************************************************************/
#define CMD_OPT_CODE                    TAHOE_VRISC_SCRATCH_0
#define CHN_ID                          TAHOE_VRISC_SCRATCH_1
#define PRP_OSD_COMMON_0                TAHOE_VRISC_SCRATCH_2
#define PRP_OSD_COMMON_1                TAHOE_VRISC_SCRATCH_3
#define PRP_OSD_COMMON_2                TAHOE_VRISC_SCRATCH_4
#define PRP_OSD_COMMON_3                TAHOE_VRISC_SCRATCH_5
#define PRP_OSD_COMMON_4                TAHOE_VRISC_SCRATCH_6
#define PRP_OSD_COMMON_5                TAHOE_VCE_SCRATCH_0
#define PRP_OSD_COMMON_6                TAHOE_VCE_SCRATCH_1
#define PRP_OSD_COMMON_7                TAHOE_VCE_SCRATCH_2
#define PRP_OSD_REGION_LT_0             TAHOE_VRISC_SCRATCH_7
#define PRP_OSD_REGION_LT_1             TAHOE_VRISC_SCRATCH_8
#define PRP_OSD_REGION_LT_2             TAHOE_VRISC_SCRATCH_9
#define PRP_OSD_REGION_LT_3             TAHOE_VRISC_SCRATCH_10
#define PRP_OSD_REGION_LT_4             TAHOE_VRISC_SCRATCH_11
#define PRP_OSD_REGION_LT_5             TAHOE_VCE_SCRATCH_3
#define PRP_OSD_REGION_LT_6             TAHOE_VCE_SCRATCH_4
#define PRP_OSD_REGION_LT_7             TAHOE_VCE_SCRATCH_5
#define PRP_OSD_REGION_WH_0             TAHOE_VRISC_SCRATCH_12
#define PRP_OSD_REGION_WH_1             TAHOE_VRISC_SCRATCH_13
#define PRP_OSD_REGION_WH_2             TAHOE_VRISC_SCRATCH_14
#define PRP_OSD_REGION_WH_3             TAHOE_VRISC_SCRATCH_15
#define PRP_OSD_REGION_WH_4             TAHOE_VRISC_SCRATCH_16
#define PRP_OSD_REGION_WH_5             TAHOE_VCE_SCRATCH_6
#define PRP_OSD_REGION_WH_6             TAHOE_VCE_SCRATCH_7
#define PRP_OSD_REGION_WH_7             TAHOE_VCE_SCRATCH_8
#define PRP_OSD_BITMAPA_0               TAHOE_VRISC_SCRATCH_17
#define PRP_OSD_BITMAPA_1               TAHOE_VRISC_SCRATCH_18
#define PRP_OSD_BITMAPA_2               TAHOE_VRISC_SCRATCH_19
#define PRP_OSD_BITMAPA_3               TAHOE_VRISC_SCRATCH_20
#define PRP_OSD_BITMAPA_4               TAHOE_VRISC_SCRATCH_21
#define PRP_OSD_BITMAPA_5               TAHOE_VCE_SCRATCH_9
#define PRP_OSD_BITMAPA_6               TAHOE_VCE_SCRATCH_10
#define PRP_OSD_BITMAPA_7               TAHOE_VCE_SCRATCH_11
#define PRP_OSD_Y_BASEADDR_0            TAHOE_VRISC_SCRATCH_22
#define PRP_OSD_Y_BASEADDR_1            TAHOE_VRISC_SCRATCH_23
#define PRP_OSD_Y_BASEADDR_2            TAHOE_VRISC_SCRATCH_24
#define PRP_OSD_Y_BASEADDR_3            TAHOE_VRISC_SCRATCH_25
#define PRP_OSD_Y_BASEADDR_4            TAHOE_VRISC_SCRATCH_26
#define PRP_OSD_Y_BASEADDR_5            TAHOE_VCE_SCRATCH_12
#define PRP_OSD_Y_BASEADDR_6            TAHOE_VCE_SCRATCH_13
#define PRP_OSD_Y_BASEADDR_7            TAHOE_VCE_SCRATCH_14
#define PRP_OSD_STRIDE_0_1              TAHOE_VRISC_SCRATCH_27
#define PRP_OSD_STRIDE_2_3              TAHOE_VRISC_SCRATCH_28
#define PRP_OSD_STRIDE_4_5              TAHOE_VRISC_SCRATCH_29
#define PRP_OSD_STRIDE_6_7              TAHOE_VRISC_SCRATCH_30

#define ROI_CTRL TAHOE_VRISC_SCRATCH_16
#define ROI_MAP_ADDRL TAHOE_VRISC_SCRATCH_17
#define ROI_REGION_START_0 TAHOE_VRISC_SCRATCH_18
#define ROI_REGION_SIZE_0 TAHOE_VRISC_SCRATCH_19
#define ROI_REGION_PARAM_0 TAHOE_VRISC_SCRATCH_20
#define ROI_REGION_START_1 TAHOE_VRISC_SCRATCH_21
#define ROI_REGION_SIZE_1 TAHOE_VRISC_SCRATCH_22
#define ROI_REGION_PARAM_1 TAHOE_VRISC_SCRATCH_23
#define ROI_REGION_START_2 TAHOE_VRISC_SCRATCH_24
#define ROI_REGION_SIZE_2 TAHOE_VRISC_SCRATCH_25
#define ROI_REGION_PARAM_2 TAHOE_VRISC_SCRATCH_26
#define ROI_REGION_START_3 TAHOE_VRISC_SCRATCH_27
#define ROI_REGION_SIZE_3 TAHOE_VRISC_SCRATCH_28
#define ROI_REGION_PARAM_3 TAHOE_VRISC_SCRATCH_29

/************************************************************************/
/* ENC_PARAM_SET (DYNAMIC SETTING)                                      */
/************************************************************************/
#define CMD_OPT_CODE                       TAHOE_VRISC_SCRATCH_0
#define CHN_ID                             TAHOE_VRISC_SCRATCH_1
#define ENC_PARAM_BITRATE                  TAHOE_VRISC_SCRATCH_2
#define ENC_PARAM_GOP                      TAHOE_VRISC_SCRATCH_3
#define ENC_PARAM_FPS                      TAHOE_VRISC_SCRATCH_4
#define ENC_PARAM_RC_MODE                  TAHOE_VRISC_SCRATCH_5
#define GOLDEN_MATCH_FLAG                  TAHOE_VRISC_SCRATCH_28
#define CRE_SVC_LTR                        TAHOE_VRISC_SCRATCH_22

#define CMD_CREATE_CHN              1
#define CMD_DESTROY_CHN             2
#define CMD_ENC_ONE_FRAME           3
#define CMD_ENC_PARAMETER_SETS      4
#define CMD_SET_ENC_PARAMETERS      5
#define CMD_GET_ENC_PARAMETERS      6
#define CMD_SET_ENC_PRP             7
#define CMD_SET_ENC_IMD             8
#define CMD_SET_ENC_ME              9
#define CMD_SET_ENC_RDO             10
#define CMD_SET_ENC_FBCD            11
#define CMD_SET_ENC_LPF             12
#define CMD_SET_ENC_EC              13
#define CMD_RDMA_TEST               14
#define CMD_DO_ENC_ONE_FRAME        15
#define CMD_DO_ENC_ONE_FRAME_EXT    16
#define CMD_TOPCASE_ENC_ONE_FRAME   17

#define CMD_SET_OSD_INFO            27

// host to vrisc
#define CMD_DONE                (1 << 16)
#define CMD_ERROR               (2 << 16)
#define CMD_PUSHED              (3 << 16)
#define CMD_BUSY                (4 << 16)

#define TAHOE_CFG                       TAHOE_VCE_SCRATCH_15
#define TAHOE_EC_DMA0                   (TAHOE_VRISC_BASE + 0x1c08)
#define TAHOE_EC_DMA6                   (TAHOE_VRISC_BASE + 0x1c20)
#define TAHOE_EC_LENGTH                 (TAHOE_VRISC_BASE + 0x1c78)
#define TAHOE_RDO_PREMD_PARA            (TAHOE_VRISC_BASE + 0x1424)
#define TAHOE_DQME_SEARCH_CFG           (TAHOE_VRISC_BASE + 0x1018)
#define PRP_MFDI_STRIDE_0               TAHOE_VRISC_SCRATCH_25
#define PRP_MFDI_STRIDE_1               TAHOE_VRISC_SCRATCH_26
#define PRP_MFDI_STRIDE_2               TAHOE_VRISC_SCRATCH_27
#define INPUT_BUF_ADDR_MSB              TAHOE_VRISC_SCRATCH_3 //bit 0~1:y 2~3:cb 4~5:cr

#define HEADER_VPS_ADDR               TAHOE_VCE_SCRATCH_9
#define HEADER_SPS_ADDR               TAHOE_VCE_SCRATCH_10
#define HEADER_PPS_ADDR               TAHOE_VCE_SCRATCH_11
#define HEADER_SIZE                   TAHOE_VCE_SCRATCH_12
#define HEADER_ADDR_HIGHER            TAHOE_VCE_SCRATCH_13 //bit 0~1:sps 2~3:pps 4~5:vps
#define ENC_PARA_FRAME_TYPE           TAHOE_VRISC_SCRATCH_15 //force frame type I/IDR


#define OUTPUT_FRAME_TYPE                  TAHOE_VRISC_SCRATCH_2

#endif
