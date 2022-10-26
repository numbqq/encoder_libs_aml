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
#ifndef __EWL_LOCAL_H__
#define __EWL_LOCAL_H__

#include "vsi_queue.h"
#include "osal.h"

/* Macro for debug printing */
#undef PTRACE_E
#undef PTRACE_I
#include <stdio.h>

#ifdef __ANDROID__
#define LOG_NDEBUG 0
#define LOG_TAG "MultiDriver"
#include <utils/Log.h>

#define PTRACE_E(fmt, ...) //        ALOGV(fmt,##__VA_ARGS__);
#define PTRACE_I(fmt, ...) //        ALOGV(fmt,##__VA_ARGS__);
#define MEM_LOG_I(fmt, ...) //       ALOGE(fmt,##__VA_ARGS__);
#else
// #define PTRACE_E( fmt, ... )
//     VCEncTraceMsg(NULL, VCENC_LOG_ERROR, VCENC_LOG_TRACE_EWL, "[%s:%d]" fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define PTRACE_E(fmt, ...)                                                                         \
    VCEncTraceMsg(NULL, VCENC_LOG_ERROR, VCENC_LOG_TRACE_EWL, fmt, ##__VA_ARGS__)
#define PTRACE_I(fmt, ...)                                                                         \
    VCEncTraceMsg(NULL, VCENC_LOG_INFO, VCENC_LOG_TRACE_EWL, fmt, ##__VA_ARGS__)
#define MEM_LOG_I(fmt, ...)                                                                        \
    VCEncTraceMsg(NULL, VCENC_LOG_INFO, VCENC_LOG_TRACE_MEM, fmt, ##__VA_ARGS__)
#endif
/* the encoder device driver nod */
#ifndef MEMALLOC_MODULE_PATH
#define MEMALLOC_MODULE_PATH "/tmp/dev/memalloc"
#endif

#ifndef ENC_MODULE_PATH
#define ENC_MODULE_PATH "/tmp/dev/vc8000"
#endif

#ifndef MEM_MODULE_PATH
#define MEM_MODULE_PATH "/dev/memalloc"
#endif

#ifndef SDRAM_LM_BASE
#define SDRAM_LM_BASE 0x00000000
#endif

enum {
    VCMD_MODE_DISABLED = 0, /* vcmd not present or disabled */
    VCMD_MODE_ENABLED = 1,  /* vcmd present and enabled */
};

typedef struct {
    i32 core_id; //physical core id
    u32 regSize; /* IO mem size */
    u32 regBase;
    volatile u32 *pRegBase; /* IO mem base */
} regMapping;

typedef struct {
    u32 subsys_id;
    u32 *pRegBase;
    u32 regSize;
    regMapping core[CORE_MAX];
} subsysReg;

typedef struct {
    struct node *next;
    int core_id;
    int cmdbuf_id;
} EWLWorker;

#define FIRST_CORE(inst) (((EWLWorker *)(inst->workers.tail))->core_id)
#define LAST_CORE(inst) (((EWLWorker *)(inst->workers.head))->core_id)
#define FIRST_CMDBUF_ID(inst) (((EWLWorker *)(inst->workers.tail))->cmdbuf_id)
#define LAST_CMDBUF_ID(inst) (((EWLWorker *)(inst->workers.head))->cmdbuf_id)

typedef struct {
    u32 clientType;
    int fd_mem;      /* /dev/mem */
    int fd_enc;      /* /dev/vc8000_vcmd_driver */
    int fd_memalloc; /* /dev/memalloc */
    regMapping reg;  //register for reserved cores
    subsysReg *reg_all_cores;
    u32 coreAmout;
    u32 performance;
    struct queue freelist;
    struct queue workers;

    /* loopback line buffer in on-chip SRAM*/
    u32 lineBufSramBase;        /* bus addr */
    volatile u32 *pLineBufSram; /* virtual addr */
    u32 lineBufSramSize;
    u32 mmuEnable;
    u32 dec400Enable;
    u32 unCheckPid;
    /*vcmd*/
    struct config_parameter vcmd_enc_core_info;
    struct cmdbuf_mem_parameter vcmd_cmdbuf_info;

    struct exchange_parameter reserve_cmdbuf_info;
    u16 wait_core_id_polling;
    u16 wait_polling_break;
    u32 *main_module_init_status_addr;

    u32 vcmd_mode; /* vcmd's working mode: disable or enable */
} vc8000_cwl_t;

#endif /*__EWL_LOCAL_H__*/
