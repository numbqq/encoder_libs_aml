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
#ifndef __TAHOE_H__
#define __TAHOE_H__


#define CHIP_ID          0x0
#define MAX_CHANNEL 1

#define MAX_VPU_BUFFER_POOL (1000)
#define MAX_INSTANCE  4

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long ulong;

typedef signed int s32;

typedef struct venc_drv_buffer_t {
    u32 size;
    u32 cached;
    ulong phys_addr;
    ulong base; /* kernel logical address in use kernel */
    ulong virt_addr; /* virtual user space address */
}venc_buffer_t;

typedef struct {
    u32 chnid;
    u32 size;
    u32 cached;
    ulong phys_addr;
    ulong base; /* kernel logical address in use kernel */
    ulong virt_addr; /* virtual user space address */
}venc_common_buffer_t;

struct venc_bit_firmware_info_t {
    u32 size;                       /* size of this structure*/
    ulong reg_base_offset;
    u16 bit_code[512];
};

struct venc_inst_info_t {
    u32 inst_idx;
    s32 inst_open_count;    /* for output only*/
};

struct venc_drv_intr_info_t {
    u32 timeout;
    s32 intr_reason;
};

struct venc_drv_context_t {
    struct fasync_struct *async_queue;
    ulong interrupt_reason;
    u32 interrupt_flag;
    u32 open_count;                  /*!<< device reference count. Not instance count */
};

typedef enum osd_format_e {
    OSD_FMT_ARGB8888  = 0,
    OSD_FMT_ARGB4444  = 1,
    OSD_FMT_ARGB1555 = 2,
    OSD_FMT_NV21 = 3,
    OSD_FMT_NV12 = 4,
    OSD_FMT_BITMAP = 5,
} osd_format_e;

/*PRP_OSD_COMMON_x*/
//Bit     0      osd_enable           // Unsigned   , RW  , default = 0x0      OSD enable
//Bit  3: 1      osd_format           // Unsigned   , RW  , default = 0x0      OSD Format: 0:ARGB8888 1:ARGB4444 2:ARGB1555 3:NV21 4:NV12 5:BITMAP
//Bit     4      osd_alpha_mode       // Unsigned   , RW  , default = 0x0      OSD alpha mode: 0:alpha add 1 started from 1; 1:add 1 started from 128
//Bit  6: 5      osd_r2y_mode         // Unsigned   , RW  , default = 0x0      OSD RGB2YUV mode 0:bt601_full 1:bt601_limit 2:bt709_full 3:bt709_limit
//Bit 10: 7      osd_endian           // Unsigned   , RW  , default = 0x0      OSD 4bits endian control
//Bit 12:11      osd_y_baseaddr_h2bit // Unsigned   , RW  , default = 0x0      OSD Higher 2 bits of base address of Luma or RGB
//Bit 14:13      osd_c_baseaddr_h2bit // Unsigned   , RW  , default = 0x0      OSD Higher 2 bits of base address of Chroma
//Bit 31:15      osd_cmn_rsv          // Unsigned   , RW  , default = 0x0      OSD common reserved bits

/*PRP_OSD_REGION_LT_x*/
//Bit 15: 0      osd_region_x         // Unsigned   , RW  , default = 0x0      OSD x position of region
//Bit 31:16      osd_region_y         // Unsigned   , RW  , default = 0x0      OSD y position of region

/*PRP_OSD_REGION_WH_x*/
//Bit 15: 0      osd_region_width     // Unsigned   , RW  , default = 0x0      OSD width of region
//Bit 31:16      osd_region_height    // Unsigned   , RW  , default = 0x0      OSD height of region

/*PRP_OSD_BITMAPA_x*/
//Bit  7: 0      osd_alpha            // Unsigned   , RW  , default = 0x0      OSD preset alpha for NV21 or BITMAP or Global Alpha for ARGB
//Bit 15: 8      osd_bitmap_y         // Unsigned   , RW  , default = 0x0      OSD preset Y for bitmap mode
//Bit 23:16      osd_bitmap_u         // Unsigned   , RW  , default = 0x0      OSD Preset U for bitmap mode
//Bit 31:24      osd_bitmap_v         // Unsigned   , RW  , default = 0x0      OSD preset V for bitmap mode

typedef struct {
    s32 enable;
    osd_format_e format;
    s32 alpha_mode;
    s32 r2y_mode;
    s32 endian;
    s32 y_baseaddr_h2bit;
    s32 c_baseaddr_h2bit;
    s32 cmn_rsv;
    s32 region_x;
    s32 region_y;
    s32 region_width;
    s32 region_height;
    s32 alpha;
    s32 bitmap_y;
    s32 bitmap_u;
    s32 bitmap_v;
    ulong y_baseaddr;
    ulong c_baseaddr;
    s32 stride;
} osd_config_t;

typedef struct {
    s32 prp_osd_common;
    s32 prp_osd_region_lt;
    s32 prp_osd_region_wh;
    s32 prp_osd_bitmapa;
    s32 prp_osd_y_baseaddr;
    s32 prp_osd_c_baseaddr;
    s32 prp_osd_stride;
} venc_osd_element_t;

typedef struct {
    ulong inpt_buf_addr[3];
    s32 input_stride[3];
    s32 inpt_buff_pts;
    s32 osd_enable;
    s32 osd_init;
    venc_osd_element_t osd[8];
    osd_config_t osd_config[8];
} venc_pic_t;

typedef struct {
    unsigned long vpsPhyAddr;
    unsigned long spsPhyAddr;
    unsigned long ppsPhyAddr;
    int headerLength;
} header_outparam_t;

typedef struct {
    unsigned long esPhyAddr;
    int eslength;
    unsigned long slice_header_length;
    unsigned char frame_type;
} es_outparam_t;

typedef struct {
    s32 chn_id;
    s32 prp_resolution;
    s32 prp_stride;
    s32 vce_resolution;
    s32 prp_crop_start;
    s32 enc_fmt;
    s32 enc_frame_type;
    s32 mfdin_input_fmt;
    s32 ds_parent_chn;
    s32 enc_rc_mode; //1:CBR,2:VBR
    s32 enc_bitrate;
    s32 enc_fps;
    s32 enc_gop;
    s32 enc_slice_num;
    s32 enc_ref_num;
    s32 enc_b_frames;
    s32 cu_depth_sel;
    s32 enc_frm_end_clr;
    s32 mono_flag;
    s32 work_mode;
    s32 rfmu_en;
    s32 rfmu_loop_en;
    s32 ec_loop_en;

    s32 enc_min_qp_i;
    s32 enc_max_qp_i;
    s32 enc_min_qp_p;
    s32 enc_max_qp_p;
    s32 enc_qp_range_delta;
    s32 enc_intra_qp_offset;
    s32 enc_aq_en;
    s32 poc;
    s32 poc_init;
    s32 enc_width;
    s32 enc_height;
    s32 buf_stride; //buf stride for Y, if 0,use width as stride (default)
    venc_pic_t tahoe_enc_pic;
    header_outparam_t enc_header;
    es_outparam_t enc_es;
    s32 mfdin_input_stride;
    s32 mfdin_input_stride1;
    s32 mfdin_input_stride2;
    ulong header_buf_addr[3]; //vps/sps/pps

    u32 ds_w;
    u32 ds_h;
    u8 ds_osd;
    u8 ds_only;

    u8 rotation_mode; //rotation mode: 00:0 R 01:90R 10:180R 11:270R
    u8 mirror_mode;  //mirror mode: bit1 : mirror;  bit2: flip;

    u8  crop_enable;
    u32 crop_x;
    u32 crop_y;
    u32 crop_w;
    u32 crop_h;
    u8 init_qp;
    u8 fix_qp_I;
    u8 fix_qp_P;
    u32 profile;
    u32 level;
    u32 tier;
    u8 cfg_update;
    u8 golden_match_flag;
    venc_buffer_t input_buf[3];
} handle_t;

#define TAHOE_REG_BASE_ADDR 0xFE370000
#define TAHOE_REG_SIZE 0x10000
#define MIN_WIDTH 128
#define MIN_HEIGHT 128

typedef enum {
    AVC_BASELINE = 66,
    AVC_MAIN     = 77,
    AVC_HIGH    = 100,
} AvcProfile;

typedef enum {
    HEVC_NONE = 0,
    HEVC_MAIN = 1,
} HevcProfile;

enum {
    IOC_NR_GET_REGISTER_INFO = 0,
    IOC_NR_GET_COMMON_BUF_INFO,
    IOC_NR_WAIT_INTERRUPT,
    IOC_NR_GET_FW_BUF_INFO,
    IOC_NR_ALLOCATE_PHYSICAL_MEMORY,
    IOC_NR_FLUSH_BUFFER,
    IOC_NR_CACHE_INV_BUFFER,
    IOC_NR_FOR_DEBUG,
    IOC_NR_DMA_MAP,
    IOC_NR_DMA_UNMAP
};

typedef enum {
    MFDI_INPUT_FMT_NV21 = 0,
    MFDI_INPUT_FMT_NV12 = 1,
    MFDI_INPUT_FMT_YUV420_PLANAR = 2,
    MFDI_INPUT_FMT_YUV444_PACKET = 3,
    MFDI_INPUT_FMT_RGB565 = 4,
    MFDI_INPUT_FMT_BGR565 = 5,
    MFDI_INPUT_FMT_ARGB1555 = 6,
    MFDI_INPUT_FMT_BRGA5551 = 7,
    MFDI_INPUT_FMT_ARGB8888 = 8,
    MFDI_INPUT_FMT_BGRA8888 = 9,
    MFDI_INPUT_FMT_RGBA8888 = 10,
    MFDI_INPUT_FMT_RGB888 = 11,
    MFDI_INPUT_FMT_BGR888 = 12,
    MFDI_INPUT_FMT_COM_4U4V = 13,
    MFDI_INPUT_FMT_COM_8U8V = 14,
} TahoeFrameFmt;

typedef struct vpudrv_buffer_pool_t
{
    venc_buffer_t vdb;
    int inuse;
} vpudrv_buffer_pool_t;

typedef struct  {
    int venc_fd;
    venc_buffer_t venc_register;
    venc_buffer_t fw_buffer;
    venc_common_buffer_t common_buffer[MAX_INSTANCE];
    int vpu_buffer_pool_count;
    vpudrv_buffer_pool_t vpu_buffer_pool[MAX_VPU_BUFFER_POOL];
    pthread_mutex_t vmem_mutex;
} venc_info_t;

typedef struct tahoe_dma_buf_info_t {
    u32 num_planes;
    int fd[3];
    ulong phys_addr[3]; /* phys address for DMA buffer */
} tahoe_dma_buf_info_t;

#define MAX_OSD_NUM 8

#define VENC_DRV_BUF_LEN venc_buffer_t

#define VENC_DMA_MAP_LEN tahoe_dma_buf_info_t

#define TAHOE_MAGIC 'x'
#define IOC_GET_REGISTER_INFO      _IO(TAHOE_MAGIC, IOC_NR_GET_REGISTER_INFO)
#define IOC_GET_COMMON_BUF_INFO    _IOR(TAHOE_MAGIC, IOC_NR_GET_COMMON_BUF_INFO, VENC_DRV_BUF_LEN)
#define IOC_WAIT_INTERRUPT         _IO(TAHOE_MAGIC, IOC_NR_WAIT_INTERRUPT)
#define IOC_GET_FW_BUF_INFO        _IOR(TAHOE_MAGIC, IOC_NR_GET_FW_BUF_INFO, VENC_DRV_BUF_LEN)
#define IOC_ALLOCATE_PHYSICAL_MEMORY        _IOWR(TAHOE_MAGIC, IOC_NR_ALLOCATE_PHYSICAL_MEMORY, VENC_DRV_BUF_LEN)
#define IOC_FLUSH_BUFFER            _IOW(TAHOE_MAGIC, IOC_NR_FLUSH_BUFFER, VENC_DRV_BUF_LEN)
#define IOC_CACHE_INV_BUFFER        _IOW(TAHOE_MAGIC, IOC_NR_CACHE_INV_BUFFER, VENC_DRV_BUF_LEN)
#define IOC_NR_FOR_DEBUG            _IOR(TAHOE_MAGIC, IOC_NR_FOR_DEBUG, u32)
#define IOC_CONFIG_DMA              _IOR(TAHOE_MAGIC, IOC_NR_DMA_MAP, VENC_DMA_MAP_LEN)
#define IOC_UNMAP_DMA               _IOR(TAHOE_MAGIC, IOC_NR_DMA_UNMAP, VENC_DMA_MAP_LEN)

void vl_multi_encoder_init(int codec_id, handle_t encode_info);

#endif
