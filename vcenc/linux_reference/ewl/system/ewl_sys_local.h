/*------------------------------------------------------------------------------
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                                                                            --
--                            Verisilicon.                                    --
--                                                                            --
--                   (C) COPYRIGHT 2019 VERISILICON                           --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
--------------------------------------------------------------------------------

--
--  Abstract : VC8000 CODEC Wrapper Layer for OS services
--
------------------------------------------------------------------------------*/
#ifndef __EWL_SYS_LOCAL_H__
#define __EWL_SYS_LOCAL_H__

/* Macro for debug printing
  */
#include "base_type.h"
#include "enc_core.h"
#include "ewl.h"
#include "encasiccontroller.h"

#include <stdio.h>
#define PTRACE_I(inst, ...) VCEncTraceMsg(inst, VCENC_LOG_INFO, VCENC_LOG_TRACE_EWL, ##__VA_ARGS__)
#define PTRACE_R(inst, ...)                                                                        \
    VCEncTraceMsg(inst, VCENC_LOG_INFO, VCENC_LOG_TRACE_REGS, "[%s:%d]", __FUNCTION__, __LINE__,   \
                  ##__VA_ARGS__)
#define PTRACE_E(inst, ...)                                                                        \
    VCEncTraceMsg(inst, VCENC_LOG_ERROR, VCENC_LOG_TRACE_EWL, "[%s:%d]", __FUNCTION__, __LINE__,   \
                  ##__VA_ARGS__)
#define MEM_LOG_I(inst, ...) VCEncTraceMsg(inst, VCENC_LOG_INFO, VCENC_LOG_TRACE_MEM, ##__VA_ARGS__)

#define ASIC_STATUS_IRQ_INTERVAL 0x100

#ifdef V4L2
#define V4L2_DRV_PATH "/dev/v4l2"
typedef struct vsi_v4l2_mem_info {
    unsigned long id;
    unsigned long size;
    unsigned long long busaddr;
} vsi_v4l2_mem_info;

/* daemon ioctl id definitions */
#define VSIV4L2_IOCTL_BASE 'd'
#define VSI_IOCTL_CMD_BASE _IO(VSIV4L2_IOCTL_BASE, 0x44)

#define VSI_IOCTL_CMD_ALLOC _IOWR(VSIV4L2_IOCTL_BASE, 47, struct vsi_v4l2_mem_info)
#define VSI_IOCTL_CMD_FREE _IOWR(VSIV4L2_IOCTL_BASE, 48, struct vsi_v4l2_mem_info)

/* macro to convert CPU bus address to ASIC bus address */
#ifdef PC_PCI_FPGA_DEMO
//#define BUS_CPU_TO_ASIC(address)    (((address) & (~0xff000000)) | SDRAM_LM_BASE)
#define BUS_CPU_TO_ASIC(address, offset) ((address) - (offset))
#else
#define BUS_CPU_TO_ASIC(address, offset) ((address) | SDRAM_LM_BASE)
#endif
#endif

/* Mask fields */
#define mask_1b (u32)0x00000001
#define mask_2b (u32)0x00000003
#define mask_3b (u32)0x00000007
#define mask_4b (u32)0x0000000F
#define mask_5b (u32)0x0000001F
#define mask_6b (u32)0x0000003F
#define mask_8b (u32)0x000000FF
#define mask_9b (u32)0x000001FF
#define mask_10b (u32)0x000003FF
#define mask_18b (u32)0x0003FFFF

#define HSWREG(n) ((n)*4)

typedef struct {
    struct node *next;
    u32 core_id; /* bit 0~7 is core index, bit 16~23 is slice node index */
} EWLWorker;

/** Core wait job output structure*/
typedef struct EWLCoreWaitOut {
    u32 job_id[MAX_SUPPORT_CORE_NUM];
    u32 irq_status[MAX_SUPPORT_CORE_NUM];
    u32 irq_num;
} EWLCoreWaitOut_t;

#define FIRST_CORE(inst) (((EWLWorker *)(inst->workers.tail))->core_id)
#define LAST_CORE(inst) (((EWLWorker *)(inst->workers.head))->core_id)
#define LAST_WAIT_CORE(inst) (((EWLWorker *)(inst->freelist.head))->core_id)

#define BUS_WIDTH_128_BITS 128
#define BUS_WRITE_U32_CNT_MAX (BUS_WIDTH_128_BITS / (8 * sizeof(u32)))
#define BUS_WRITE_U8_CNT_MAX (BUS_WIDTH_128_BITS / (8 * sizeof(u8)))

/* Maximum number of chunks allowed. */

struct qualityProfile {
    i32 frame_number;
    float total_bits;
    float total_Y_PSNR;
    float total_U_PSNR;
    float total_V_PSNR;
    float Y_PSNR_Max;
    float Y_PSNR_Min;
    float U_PSNR_Max;
    float U_PSNR_Min;
    float V_PSNR_Max;
    float V_PSNR_Min;
    double total_ssim;
    double total_ssim_y;
    double total_ssim_u;
    double total_ssim_v;
    i32 QP_Max;
    i32 QP_Min;
};

typedef struct {
#ifdef V4L2
    int fd_v4l2;
#endif
    u32 refFrmAlloc[MEM_CHUNKS];
    u32 linMemAlloc[MEM_CHUNKS];
    u32 refFrmChunks;
    u32 linMemChunks;
    u32 totalChunks;                /* Total malloc's */
    u32 *chunks[MEM_CHUNKS];        /* Returned by malloc */
    u32 *alignedChunks[MEM_CHUNKS]; /* Aligned */

    struct queue freelist;
    struct queue workers;
    u8 *streamBase;
    u8 *streamTempBuffer;
    u32 streamLength;
    u8 segmentAmount;
    i8 frameRdy;
    u32 clientType;
    u32 performance;

    struct qualityProfile prof;

    WorkInst winst;

    SysCore *core[MAX_CORE_NUM];

} ewlSysInstance;

typedef struct EWLSubSysOut_t {
    u32 job_id;
    u32 irq_status;
    u32 slice_rdy_num;
    u32 total_slice_num;
    u32 has_irq;
} EWLSubSysOut_t;

#endif /*__EWL_SYS_LOCAL_H__*/
