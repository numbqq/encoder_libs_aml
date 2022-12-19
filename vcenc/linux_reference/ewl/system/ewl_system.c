/*------------------------------------------------------------------------------
--                                                                                                                               --
--       This software is confidential and proprietary and may be used                                   --
--        only as expressly authorized by a licensing agreement from                                     --
--                                                                                                                               --
--                            Verisilicon.                                                                                    --
--                                                                                                                               --
--                   (C) COPYRIGHT 2014 VERISILICON                                                            --
--                            ALL RIGHTS RESERVED                                                                    --
--                                                                                                                               --
--                 The entire notice above must be reproduced                                                 --
--                  on all copies and should not be removed.                                                    --
--                                                                                                                               --
--------------------------------------------------------------------------------
--
--  Abstract : Encoder Wrapper Layer system model adapter
--
------------------------------------------------------------------------------*/

#include "tools.h"

//#define V4L2
#ifdef V4L2
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#endif

/* Common EWL interface */
#include "ewl.h"
#include "ewl_common.h"
#include "encasiccontroller.h"

/* HW register definitions */
#include "enc_core.h"
#include "osal.h"
#include "ewl_sys_local.h"
#include "ewl_memsync.h"
//#define CORE_NUM 4  /*move to enc_core.h*/
//#define MAX_CORE_NUM 4  /*move to enc_core.h*/

static EWLCoreWait_t coreWait;
static pthread_mutex_t ewl_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t ewl_refer_counter_mutex = PTHREAD_MUTEX_INITIALIZER;
static EWLSubSysOut_t subSysOut[MAX_CORE_NUM];
static u32 reserved_job_id[MAX_CORE_NUM];
static u32 instCounter;
static pthread_mutex_t instCounter_mutex = PTHREAD_MUTEX_INITIALIZER;

static u32 EWLGetCoreNumSys(void *ctx);
static i32 EWLHwSysDone(const void *instance);
static void EWLReleaseHwSys(const void *inst);
static i32 EWLWaitHwRdySys(const void *instance, u32 *slicesReady, void *waitOut,
                           u32 *status_register);
static void EWLGetCoreOutRelSys(const void *inst, i32 ewl_ret, EWLCoreWaitJob_t *job);
static void *EWLCoreWaitThreadSys(void *pCoreWait);
static void EwlCreateCoreWaitSys(void);
i32 EWLSyncMemDataSys(EWLLinearMem_t *mem, u32 offset, u32 length, enum EWLMemSyncDirection dir);
i32 EWLMemSyncAllocHostBufferSys(const void *instance, u32 size, u32 alignment,
                                 EWLLinearMem_t *buff);
i32 EWLMemSyncFreeHostBufferSys(const void *instance, EWLLinearMem_t *buff);
#ifndef SUPPORT_MEM_SYNC
#define EWLSyncMemDataSys(mem, offset, length, dir) (EWL_OK)
#define EWLMemSyncAllocHostBufferSys(inst, size, alignment, buff) (EWL_NOT_SUPPORT)
#define EWLMemSyncFreeHostBufferSys(inst, buff) (EWL_NOT_SUPPORT)
#endif

#ifdef VIRTUAL_PLATFORM_TEST
u32 *VirtualPlatformBuf[ASIC_SWREG_AMOUNT] = {NULL};
u32 *CmodelUseOrigBuf[ASIC_SWREG_AMOUNT] = {NULL};
static void EWLVirtualPlatformTestIn(SysCore *core);
static void EWLVirtualPlatformTestOut(SysCore *core);
#endif

/*------------------------------------------------------------------------------

------------------------------------------------------------------------------*/
static u32 EWLReadAsicIDSys(u32 core_id, void *ctx)
{
    SysCore *core;
    u32 slice_idx = NODE(core_id);
    u32 core_idx = CORE(core_id);

    if (core_idx >= MAX_HWCORE_NUM)
        return 0;

    core = CoreEncSetup(slice_idx, core_idx);

    return CoreEncGetRegister(core, 0);
}

/*******************************************************************************
 Function name   : EWLReadAsicConfig
 Description     : Reads ASIC capability register, static implementation
 Return type     : EWLHwConfig_t
 Argument        : void
*******************************************************************************/
static EWLHwConfig_t EWLReadAsicConfigSys(u32 core_id, void *ctx)
{
    u32 slice_idx = NODE(core_id);
    u32 core_idx = CORE(core_id);
    SysCore *core = CoreEncSetup(slice_idx, core_idx);
    EWLCoreSignature_t signature;
    EWLHwConfig_t cfg_info;

    signature.hw_asic_id = EWLReadAsicIDSys(core_id, ctx);

    signature.hw_build_id = CoreEncGetRegister(core, HWIF_REG_BUILD_ID * 4);
    signature.fuse[0] = CoreEncGetRegister(core, HWIF_REG_BUILD_REV * 4);

    signature.fuse[1] = CoreEncGetRegister(core, HWIF_REG_CFG1 * 4);
    signature.fuse[2] = CoreEncGetRegister(core, HWIF_REG_CFG2 * 4);
    signature.fuse[3] = CoreEncGetRegister(core, HWIF_REG_CFG3 * 4);
    signature.fuse[4] = CoreEncGetRegister(core, HWIF_REG_CFG4 * 4);
    signature.fuse[5] = CoreEncGetRegister(core, HWIF_REG_CFG5 * 4);
    signature.fuse[6] = CoreEncGetRegister(core, HWIF_REG_CFGAXI * 4);

    signature.hw_asic_id = EWLReadAsicIDSys(core_id, ctx);
    signature.fuse[1] = CoreEncGetRegister(core, HWIF_REG_CFG1 * 4);

    if (EWL_OK != EWLGetCoreConfig(&signature, &cfg_info))
        memset(&cfg_info, 0, sizeof(cfg_info));

    return cfg_info;
}

/*------------------------------------------------------------------------------

    System model adapter for EWL.

------------------------------------------------------------------------------*/
static u32 EWLReadRegSys(const void *inst, u32 offset)
{
    ewlSysInstance *instance = (ewlSysInstance *)inst;
    assert(inst != NULL);
    assert(offset < ASIC_SWREG_AMOUNT * 4);
    u32 core_id = FIRST_CORE(instance);
    SysCore *core = instance->core[CORE(core_id)];
    (void)inst;

    return CoreEncGetRegister(core, offset);
}

static void EWLWriteCoreRegSys(const void *inst, u32 offset, u32 val, u32 core_id)
{
    ewlSysInstance *instance = (ewlSysInstance *)inst;
    assert(inst != NULL);
    assert(offset < ASIC_SWREG_AMOUNT * 4);
    SysCore *core = instance->core[CORE(core_id)];
    (void)inst;

    // PTRACE_R((void*)inst, "EWLWriteReg 0x%02x with value %08x\n", offset, val);

    CoreEncSetRegister(core, offset, val);
}
static void EWLWriteRegSys(const void *inst, u32 offset, u32 val)
{
    ewlSysInstance *instance = (ewlSysInstance *)inst;
    u32 core_id = LAST_CORE(instance);
    EWLWriteCoreRegSys(inst, offset, val, core_id);
}
static void EWLWriteBackRegSys(const void *inst, u32 offset, u32 val)
{
    ewlSysInstance *instance = (ewlSysInstance *)inst;
    u32 core_id = FIRST_CORE(instance);
    EWLWriteCoreRegSys(inst, offset, val, core_id);
}

u32 EWLGetClientTypeSys(const void *inst)
{
    ewlSysInstance *enc = (ewlSysInstance *)inst;
    u32 client_type = enc->clientType;

    return client_type;
}

static u32 EWLGetCoreTypeByClientTypeSys(u32 client_type)
{
    return 0;
}

static u32 EWLChangeClientTypeSys(const void *inst, u32 client_type)
{
    return 0;
}

static i32 EWLCheckCutreeValidSys(const void *inst)
{
    return EWL_OK;
}

/*------------------------------------------------------------------------------
    Function name   : EWLEnableHW
    Description     :
    Return type     : void
    Argument        : const void *inst
    Argument        : u32 offset
    Argument        : u32 val
------------------------------------------------------------------------------*/
static void EWLEnableHWSys(const void *instance, u32 offset, u32 val)
{
    ewlSysInstance *inst = (ewlSysInstance *)instance;
    assert(inst != NULL);
    (void)inst;
    (void)val;
    u32 core_id = LAST_CORE(inst);
    SysCore *core = inst->core[CORE(core_id)];
    u32 width = 0, height = 0;

#ifdef VIRTUAL_PLATFORM_TEST
    EWLVirtualPlatformTestIn(core);
#endif

    PTRACE_I((void *)instance, "EWLEnableHW core_id(%x) offset:0x%02x with value %08x\n", core_id,
             offset, val);

    /* setup callback if need */
    if (pollInputLineBufTestFunc)
        core->cfg.cb_get_input_rows = pollInputLineBufTestFunc;

    CoreEncSetRegister(core, offset, val);

    if (offset == (ASIC_REG_INDEX_STATUS * 4)) {
        if (CoreEncGetRegisterValue(core, HWIF_ENC_STRM_SEGMENT_EN) &&
            CoreEncGetRegisterValue(core, HWIF_ENC_MODE) != ASIC_CUTREE) {
            if (CoreEncGetRegisterValue(core, HWIF_ENC_MODE) == ASIC_HEVC ||
                CoreEncGetRegisterValue(core, HWIF_ENC_MODE) == ASIC_H264 ||
                CoreEncGetRegisterValue(core, HWIF_ENC_MODE) == ASIC_AV1 ||
                CoreEncGetRegisterValue(core, HWIF_ENC_MODE) == ASIC_VP9) {
                width = (CoreEncGetRegisterValue(core, HWIF_ENC_PIC_WIDTH) |
                         (CoreEncGetRegisterValue(core, HWIF_ENC_PIC_WIDTH_MSB) << 10) |
                         (CoreEncGetRegisterValue(core, HWIF_ENC_PIC_WIDTH_MSB2) << 12)) *
                        8;
                height = CoreEncGetRegisterValue(core, HWIF_ENC_PIC_HEIGHT) * 8;
            } else {
                width = (CoreEncGetRegisterValue(core, HWIF_ENC_JPEG_PIC_WIDTH) |
                         (CoreEncGetRegisterValue(core, HWIF_ENC_JPEG_PIC_WIDTH_MSB) << 12)) *
                        8;
                height = (CoreEncGetRegisterValue(core, HWIF_ENC_JPEG_PIC_HEIGHT) |
                          (CoreEncGetRegisterValue(core, HWIF_ENC_JPEG_PIC_HEIGHT_MSB) << 12)) *
                         8;
            }

            inst->streamTempBuffer = calloc(width * height * 2 * 4, sizeof(u8));

            inst->streamLength = 0;
            inst->frameRdy = -1;
            inst->segmentAmount = CoreEncGetRegisterValue(core, HWIF_ENC_OUTPUT_STRM_BUFFER_LIMIT) /
                                  CoreEncGetRegisterValue(core, HWIF_ENC_STRM_SEGMENT_SIZE);
            inst->streamBase = (u8 *)CoreEncGetAddrRegisterValue(core, HWIF_ENC_OUTPUT_STRM_BASE);
            CoreEncSetAddrRegisterValue(core, HWIF_ENC_OUTPUT_STRM_BASE,
                                        (ptr_t)inst->streamTempBuffer);
            CoreEncSetRegisterValue(core, HWIF_ENC_STRM_SEGMENT_WR_PTR, 0);
        }
    }

    CoreEncRun(inst->core[CORE(core_id)], offset, val, &inst->winst);

    EWLHwSysDone(inst);
}

/*------------------------------------------------------------------------------
    Function name   : EWLDisableHW
    Description     :
    Return type     : void
    Argument        : const void *inst
    Argument        : u32 offset
    Argument        : u32 val
------------------------------------------------------------------------------*/
static void EWLDisableHWSys(const void *inst, u32 offset, u32 val)
{
    assert(inst != NULL);
    (void)inst;
    (void)val;
    if (offset != (4 * 4)) {
        assert(0);
    }

    PTRACE_I((void *)inst, "EWLDisableHW 0x%02x with value %08x\n", offset * 4, val);
}

/*------------------------------------------------------------------------------
    Function name   : EWLGetPerformance
    Description     :
    Return type     : void
    Argument        : const void *inst
    Argument        : u32 offset
    Argument        : u32 val
------------------------------------------------------------------------------*/
static u32 EWLGetPerformanceSys(const void *instance)
{
    ewlSysInstance *inst = (ewlSysInstance *)instance;
    return inst->performance;
}

static i32 EWLGetDec400CoreidSys(const void *inst)
{
    return -1;
}

static u32 EWLGetDec400CustomerIDSys(const void *inst, u32 core_id)
{
    return 0;
}

static void EWLGetDec400AttributeSys(const void *inst, u32 *tile_size, u32 *bits_tile_in_table,
                                     u32 *planar420_cbcr_arrangement_style)
{
    *tile_size = 256;
    *bits_tile_in_table = 4;
    *planar420_cbcr_arrangement_style = 0;
}

static EWLCoreWaitJob_t *EWLDequeueCoreOutJobSys(const void *inst, u32 waitCoreJobid)
{
    EWLCoreWait_t *pCoreWait = (EWLCoreWait_t *)&coreWait;
    EWLCoreWaitJob_t *job, *out;

    while (!pCoreWait->bFlush) {
        out = NULL;
        pthread_mutex_lock(&pCoreWait->out_mutex);

        job = (EWLCoreWaitJob_t *)queue_tail(&pCoreWait->out);
        while (job == NULL && !pCoreWait->bFlush) {
            pthread_cond_wait(&pCoreWait->out_cond, &pCoreWait->out_mutex);
            job = (EWLCoreWaitJob_t *)queue_tail(&pCoreWait->out);
        }

        while (job != NULL) {
            if (job->id == waitCoreJobid) {
                out = job;
                queue_remove(&pCoreWait->out, (struct node *)job);
                break;
            }

            job = (EWLCoreWaitJob_t *)((struct node *)job->next);
        }

        pthread_mutex_unlock(&pCoreWait->out_mutex);
        if (out != NULL)
            return out;
    }

    return NULL;
}

/* Get one job from queue */
static EWLCoreWaitJob_t *DequeueCoreWaitJobSys(EWLCoreWait_t *coreWait)
{
    pthread_mutex_lock(&coreWait->job_mutex);
    EWLCoreWaitJob_t *job = (EWLCoreWaitJob_t *)queue_tail(&coreWait->jobs);
    while (job == NULL && !coreWait->bFlush) {
        pthread_cond_wait(&coreWait->job_cond, &coreWait->job_mutex);
        job = (EWLCoreWaitJob_t *)queue_tail(&coreWait->jobs);
    }
    pthread_mutex_unlock(&coreWait->job_mutex);
    return job;
}

/* Get one job from job pool and if no available allocate a new one */
static EWLCoreWaitJob_t *EWLGetJobfromPoolSys(EWLCoreWait_t *coreWait)
{
    EWLCoreWaitJob_t *job = (EWLCoreWaitJob_t *)queue_get(&coreWait->job_pool);
    if (job == NULL)
        job = (EWLCoreWaitJob_t *)malloc(sizeof(EWLCoreWaitJob_t));

    if (job != NULL)
        memset(job, 0, sizeof(EWLCoreWaitJob_t));

    return job;
}

/* Get one job from job pool and if no available allocate a new one */
void EWLPutJobtoPoolSys(const void *inst, struct node *job)
{
    pthread_mutex_lock(&coreWait.job_mutex);
    queue_put(&coreWait.job_pool, job);
    pthread_mutex_unlock(&coreWait.job_mutex);
}

void EWLGetRegsAfterFrameDoneSys(const void *inst, u32 *reg, u32 irq_status)
{
    ewlSysInstance *enc = (ewlSysInstance *)inst;
    int i;
    u32 core_id = FIRST_CORE(enc);
    SysCore *core = enc->core[CORE(core_id)];

    if (irq_status == ASIC_STATUS_FRAME_READY) {
        for (i = 1; i < ASIC_SWREG_AMOUNT; i++) {
            reg[i] = CoreEncGetRegister(core, HSWREG(i));
        }
    }

    EWLReleaseHwSys(inst);
}

void EWLGetCoreOutRelSys(const void *inst, i32 ewl_ret, EWLCoreWaitJob_t *job)
{
    ewlSysInstance *enc = (ewlSysInstance *)inst;
    u32 status = job->out_status;

    if (ewl_ret != EWL_OK) {
        job->out_status = ASIC_STATUS_ERROR;

        EWLDisableHWSys(inst, HSWREG(ASIC_REG_INDEX_STATUS), 0);

        /* Release Core so that it can be used by other codecs */
        EWLGetRegsAfterFrameDoneSys(inst, job->VC8000E_reg, job->out_status);
    } else {
        /* Check ASIC status bits and release HW */
        status &= ASIC_STATUS_ALL;

        if (status & ASIC_STATUS_ERROR) {
            /* Get registers for debugging */
            status = ASIC_STATUS_ERROR;
            EWLGetRegsAfterFrameDoneSys(inst, job->VC8000E_reg, status);
        } else if (status & ASIC_STATUS_FUSE_ERROR) {
            /* Get registers for debugging */
            status = ASIC_STATUS_ERROR;
            EWLGetRegsAfterFrameDoneSys(inst, job->VC8000E_reg, status);
        } else if (status & ASIC_STATUS_HW_TIMEOUT) {
            /* Get registers for debugging */
            status = ASIC_STATUS_HW_TIMEOUT;
            EWLGetRegsAfterFrameDoneSys(inst, job->VC8000E_reg, status);
        } else if (status & ASIC_STATUS_BUFF_FULL) {
            /* ASIC doesn't support recovery from buffer full situation,
             * at the same time with buff full ASIC also resets itself. */
            status = ASIC_STATUS_BUFF_FULL;
            EWLGetRegsAfterFrameDoneSys(inst, job->VC8000E_reg, status);
        } else if (status & ASIC_STATUS_HW_RESET) {
            status = ASIC_STATUS_HW_RESET;
            EWLGetRegsAfterFrameDoneSys(inst, job->VC8000E_reg, status);
        } else if (status & ASIC_STATUS_FRAME_READY) {
            /* read out all register */
            status = ASIC_STATUS_FRAME_READY;
            EWLGetRegsAfterFrameDoneSys(inst, job->VC8000E_reg, status);
        } else if (status & ASIC_STATUS_SLICE_READY) {
            status = ASIC_STATUS_SLICE_READY;

            u32 core_id = FIRST_CORE(enc);
            SysCore *core = enc->core[CORE(core_id)];
            job->slices_rdy = ((CoreEncGetRegister(core, HSWREG(7)) >> 17) & 0xFF);
        } else if (status & ASIC_STATUS_LINE_BUFFER_DONE) {
            status = ASIC_STATUS_LINE_BUFFER_DONE;

            u32 core_id = FIRST_CORE(enc);
            SysCore *core = enc->core[CORE(core_id)];
            job->VC8000E_reg[196] = CoreEncGetRegister(core, HSWREG(196));

            //save sw handshake rd pointer if sw handshake is enabled
            if ((!(job->VC8000E_reg[196] >> 31)) &&
                (job->low_latency_rd < ((job->VC8000E_reg[196] & 0x000ffc00) >> 10)))
                job->low_latency_rd = ((job->VC8000E_reg[196] & 0x000ffc00) >> 10);
            else
                status = 0;
        } else if (status & ASIC_STATUS_SEGMENT_READY) {
            status = ASIC_STATUS_SEGMENT_READY;
        }

        job->out_status = status;
    }
}

/* get register setting from api, reserve a core and set registers*/
static void *EWLCoreWaitThreadSys(void *pCoreWait)
{
    EWLCoreWait_t *coreWait = (EWLCoreWait_t *)pCoreWait;
    struct node *job;
    u32 i;
    EWLCoreWaitOut_t waitOut;
    i32 ewl_ret;
    u32 has_output = 0;

    while ((job = (struct node *)DequeueCoreWaitJobSys(coreWait)) != NULL) {
        memset(&waitOut, 0, sizeof(EWLCoreWaitOut_t));

        /* wait for VC8000E Core done */
        ewl_ret = EWLWaitHwRdySys(((EWLCoreWaitJob_t *)job)->inst, NULL, &waitOut, NULL);

        pthread_mutex_lock(&coreWait->job_mutex);
        job = queue_tail(&coreWait->jobs);

        while (job != NULL) {
            struct node *next = job->next;
            for (i = 0; i < waitOut.irq_num; i++) {
                if (waitOut.job_id[i] == ((EWLCoreWaitJob_t *)job)->id) {
                    ((EWLCoreWaitJob_t *)job)->out_status = waitOut.irq_status[i];
                    EWLGetCoreOutRelSys(((EWLCoreWaitJob_t *)job)->inst, ewl_ret,
                                        (EWLCoreWaitJob_t *)job);
                    if (((EWLCoreWaitJob_t *)job)->out_status &
                        (ASIC_STATUS_FUSE_ERROR | ASIC_STATUS_HW_TIMEOUT | ASIC_STATUS_BUFF_FULL |
                         ASIC_STATUS_HW_RESET | ASIC_STATUS_ERROR | ASIC_STATUS_FRAME_READY)) {
                        queue_remove(&coreWait->jobs, (struct node *)job);
                        pthread_mutex_lock(&coreWait->out_mutex);
                        queue_put(&coreWait->out, job);
                        pthread_mutex_unlock(&coreWait->out_mutex);
                        has_output = 1;
                    } else if (((EWLCoreWaitJob_t *)job)->out_status) {
                        struct node *tmp_out = (struct node *)EWLGetJobfromPoolSys(coreWait);
                        memcpy(tmp_out, job, sizeof(EWLCoreWaitJob_t));
                        pthread_mutex_lock(&coreWait->out_mutex);
                        queue_put(&coreWait->out, tmp_out);
                        pthread_mutex_unlock(&coreWait->out_mutex);
                        has_output = 1;
                    }

                    break;
                }
            }

            job = next;
        }

        pthread_mutex_unlock(&coreWait->job_mutex);

        if (has_output == 1) {
            pthread_cond_broadcast(&coreWait->out_cond);
            has_output = 0;
        }
    }
    return NULL;
}

void EWLEnqueueWaitjobSys(const void *inst, u32 waitCoreJobid)
{
    //enqueue the job for core wait thread
    pthread_mutex_lock(&coreWait.job_mutex);

    EWLCoreWaitJob_t *job = EWLGetJobfromPoolSys(&coreWait);
    job->id = waitCoreJobid;
    job->inst = inst;

    queue_put(&coreWait.jobs, (struct node *)job);
    pthread_cond_signal(&coreWait.job_cond);
    pthread_mutex_unlock(&coreWait.job_mutex);
}

/* create ewl core wait thread and now just support VC8000E*/
static void EwlCreateCoreWaitSys(void)
{
    EWLCoreWait_t *pCoreWait = &coreWait;

    pthread_attr_t attr;
    pthread_t *tid_CoreWait = (pthread_t *)malloc(sizeof(pthread_t));
    pthread_mutexattr_t mutexattr;
    pthread_condattr_t condattr;

    queue_init(&coreWait.jobs);
    queue_init(&coreWait.out);

    pthread_mutexattr_init(&mutexattr);
    pthread_mutex_init(&coreWait.job_mutex, &mutexattr);
    pthread_mutex_init(&coreWait.out_mutex, &mutexattr);
    pthread_mutexattr_destroy(&mutexattr);
    pthread_condattr_init(&condattr);
    pthread_cond_init(&coreWait.job_cond, &condattr);
    pthread_cond_init(&coreWait.out_cond, &condattr);
    pthread_condattr_destroy(&condattr);

    pthread_attr_init(&attr);
    pthread_create(tid_CoreWait, &attr, &EWLCoreWaitThreadSys, pCoreWait);
    pthread_attr_destroy(&attr);

    coreWait.tid_CoreWait = tid_CoreWait;

    return;
}

void EwlReleaseCoreWaitSys(void *inst)
{
    /* Wait for Core Wait Thread finish */
    pthread_mutex_lock(&ewl_refer_counter_mutex);
    if (coreWait.tid_CoreWait && coreWait.refer_counter == 0) {
        pthread_join(*coreWait.tid_CoreWait, NULL);

        pthread_mutex_destroy(&coreWait.job_mutex);
        pthread_mutex_destroy(&coreWait.out_mutex);
        pthread_cond_destroy(&coreWait.job_cond);
        pthread_cond_destroy(&coreWait.out_cond);

        free(coreWait.tid_CoreWait);
        coreWait.tid_CoreWait = NULL;
        EWLCoreWaitJob_t *job = NULL;
        while ((job = (EWLCoreWaitJob_t *)queue_get(&coreWait.jobs)) != NULL) {
            free(job);
            job = NULL;
        }

        while ((job = (EWLCoreWaitJob_t *)queue_get(&coreWait.out)) != NULL) {
            free(job);
            job = NULL;
        }

        while ((job = (EWLCoreWaitJob_t *)queue_get(&coreWait.job_pool)) != NULL) {
            free(job);
            job = NULL;
        }
    }
    pthread_mutex_unlock(&ewl_refer_counter_mutex);
}

/*------------------------------------------------------------------------------
    Function name   : EWLInit
    Description     :
    Return type     : void
    Argument        : const void *inst
------------------------------------------------------------------------------*/
static const void *EWLInitSys(EWLInitParam_t *param)
{
    int i;
    ewlSysInstance *inst;
    u32 core_num, slice_num, slice;

    if (param == NULL || param->clientType >= EWL_CLIENT_TYPE_MAX)
        return NULL;

    inst = (ewlSysInstance *)malloc(sizeof(ewlSysInstance));
    if (inst == NULL)
        return NULL;
    memset(inst, 0, sizeof(ewlSysInstance));

    slice_num = CoreEncGetSliceNum();
    (void)slice_num;
    slice = param->slice_idx;

    ASSERT(slice < slice_num);

    core_num = CoreEncGetCoreNum(slice);

    for (i = 0; i < core_num; i++) {
        inst->core[i] = CoreEncSetup(slice, i);
    }

    inst->clientType = param->clientType;
    inst->linMemChunks = 0;
    inst->refFrmChunks = 0;
    inst->totalChunks = 0;
    inst->streamTempBuffer = NULL;

#ifdef V4L2
    inst->fd_v4l2 = open(V4L2_DRV_PATH, O_RDWR);
    if (inst->fd_v4l2 == -1) {
        PTRACE_I(NULL, "EWLInit: failed to open: %s\n", V4L2_DRV_PATH);
        return NULL;
    }
#endif

    inst->prof.frame_number = 0;
    inst->prof.total_bits = 0;
    inst->prof.total_Y_PSNR = 0.0;
    inst->prof.total_U_PSNR = 0.0;
    inst->prof.total_V_PSNR = 0.0;
    inst->prof.Y_PSNR_Max = -1;
    inst->prof.Y_PSNR_Min = 1000.0;
    inst->prof.U_PSNR_Max = -1;
    inst->prof.U_PSNR_Min = 1000.0;
    inst->prof.V_PSNR_Max = -1;
    inst->prof.V_PSNR_Min = 1000.0;
    inst->prof.total_ssim = 0.0;
    inst->prof.total_ssim_y = 0.0;
    inst->prof.total_ssim_u = 0.0;
    inst->prof.total_ssim_v = 0.0;
    inst->prof.QP_Max = 0;
    inst->prof.QP_Min = 52;

    queue_init(&inst->freelist);
    queue_init(&inst->workers);
    for (i = 0; i < (param->clientType == EWL_CLIENT_TYPE_JPEG_ENC
                         ? 1
                         : (int)EWLGetCoreNumSys(param->context));
         i++) {
        EWLWorker *worker = EWLmalloc(sizeof(EWLWorker));
        worker->core_id = COREID(slice, i);
        worker->next = NULL;
        queue_put(&inst->freelist, (struct node *)worker);
    }

    if (inst->clientType == EWL_CLIENT_TYPE_HEVC_ENC ||
        inst->clientType == EWL_CLIENT_TYPE_H264_ENC ||
        inst->clientType == EWL_CLIENT_TYPE_AV1_ENC ||
        inst->clientType == EWL_CLIENT_TYPE_VP9_ENC) {
        pthread_mutex_lock(&ewl_refer_counter_mutex);
        if (coreWait.refer_counter == 0)
            EwlCreateCoreWaitSys();

        coreWait.refer_counter++;
        pthread_mutex_unlock(&ewl_refer_counter_mutex);
    }
    pthread_mutex_lock(&instCounter_mutex);
    instCounter++;
    pthread_mutex_unlock(&instCounter_mutex);
    return (void *)inst;
}

/*------------------------------------------------------------------------------
    Function name   : EWLRelease
    Description     :
    Return type     : void
    Argument        : const void *inst
------------------------------------------------------------------------------*/
static i32 EWLReleaseSys(const void *instance)
{
    u32 i, tothw, totswhw;
    ewlSysInstance *inst = (ewlSysInstance *)instance;
    u32 core_id = LAST_WAIT_CORE(inst);
    SysCore *core = inst->core[CORE(core_id)];

    assert(inst != NULL);

    CoreEncRelease(core, &inst->winst, inst->clientType);

    if (inst->clientType == EWL_CLIENT_TYPE_HEVC_ENC ||
        inst->clientType == EWL_CLIENT_TYPE_H264_ENC ||
        inst->clientType == EWL_CLIENT_TYPE_AV1_ENC ||
        inst->clientType == EWL_CLIENT_TYPE_VP9_ENC) {
        pthread_mutex_lock(&ewl_refer_counter_mutex);
        if (coreWait.refer_counter > 0)
            coreWait.refer_counter--;

        if (coreWait.refer_counter == 0) {
            pthread_mutex_lock(&coreWait.job_mutex);
            coreWait.bFlush = HANTRO_TRUE;
            pthread_cond_signal(&coreWait.job_cond);
            pthread_mutex_unlock(&coreWait.job_mutex);
        }

        pthread_mutex_unlock(&ewl_refer_counter_mutex);
    }

    if (inst->streamTempBuffer)
        free(inst->streamTempBuffer);
#ifndef V4L2
    for (i = 0, tothw = 0; i < inst->refFrmChunks; i++) {
        tothw += inst->refFrmAlloc[i];
        MEM_LOG_I((void *)inst, "\n      HW Memory:   %u\n", inst->refFrmAlloc[i]);
    }
    printf("\nTotal HW Memory:   %u", tothw);
    for (i = 0, totswhw = 0; i < inst->linMemChunks; i++) {
        totswhw += inst->linMemAlloc[i];
        MEM_LOG_I((void *)inst, "\n      SWHW Memory: %u\n", inst->linMemAlloc[i]);
    }
    printf("\nTotal SWHW Memory: %u\n", totswhw);

#else
    if (inst->fd_v4l2 != -1)
        close(inst->fd_v4l2);
#endif //end of #ifndef V4l2
    free_nodes(inst->workers.tail);
    free_nodes(inst->freelist.tail);
    free(inst);
    pthread_mutex_lock(&instCounter_mutex);
    if (instCounter > 0)
        instCounter--;
    pthread_mutex_unlock(&instCounter_mutex);
    if (instCounter == 0)
        CoreEncShutdown();
    return EWL_OK;
}

/*------------------------------------------------------------------------------
    Function name   : EWLMallocLinear
    Description     : Allocate a contiguous, linear RAM  memory buffer

    Return type     : i32 - 0 for success or a negative error code

    Argument        : const void * instance - EWL instance
    Argument        : u32 size - size in bytes of the requested memory
    Argument        : EWLLinearMem_t *info - place where the allocated
                        memory buffer parameters are returned
------------------------------------------------------------------------------*/
static i32 EWLMallocLinearSys(const void *instance, u32 size, u32 alignment, EWLLinearMem_t *info)
{
#ifndef V4L2
    ewlSysInstance *inst = (ewlSysInstance *)instance;
    assert(instance != NULL);
    assert(inst->linMemChunks < MEM_CHUNKS);
    assert(inst->totalChunks < MEM_CHUNKS);
    alignment = MAX(alignment, LINMEM_ALIGN);
    i32 ret;

    if (((info->mem_type & VPU_RD) || (info->mem_type & VPU_WR)) &&
        ((info->mem_type & CPU_RD) == 0 && (info->mem_type & CPU_WR) == 0))
        inst->refFrmAlloc[inst->refFrmChunks++] = size;
    else
        inst->linMemAlloc[inst->linMemChunks++] = size;

    PTRACE_I((void *)instance, "EWLMallocLinear: %8d bytes (aligned %8ld)\n", size,
             NEXT_ALIGNED_SYS(size, alignment));

    size = NEXT_ALIGNED_SYS(size, alignment);

    inst->chunks[inst->totalChunks] = (u32 *)calloc(1, size + alignment);
    if (inst->chunks[inst->totalChunks] == NULL)
        return EWL_ERROR;

    inst->alignedChunks[inst->totalChunks] =
        (u32 *)NEXT_ALIGNED_SYS(inst->chunks[inst->totalChunks], alignment);
    ;
    PTRACE_I((void *)instance, "EWLMallocLinear: %p, aligned %p\n",
             (void *)inst->chunks[inst->totalChunks],
             (void *)inst->alignedChunks[inst->totalChunks]);

    info->busAddress = (ptr_t)inst->alignedChunks[inst->totalChunks++];

    info->virtualAddress = NULL;
    ret = EWLMemSyncAllocHostBufferSys(inst, size, alignment, info);
    if (ret == EWL_NOT_SUPPORT)
        info->virtualAddress = (u32 *)info->busAddress;
    else if (ret == EWL_ERROR)
        return ret;
    info->size = info->total_size = size;
    return EWL_OK;

#else
    ewlSysInstance *enc_ewl = (ewlSysInstance *)instance;
    EWLLinearMem_t *buff = (EWLLinearMem_t *)info;
    assert(enc_ewl != NULL);
    assert(buff != NULL);
    PTRACE_I((void *)instance, "EWLMallocLinear\t%8d bytes\n", size);
    if (alignment == 0)
        alignment = 1;

    u32 pgsize = getpagesize();
    int err;
    vsi_v4l2_mem_info params;
    memset(&params, 0, sizeof(params));

    buff->size = buff->total_size =
        (((size + (alignment - 1)) & (~(alignment - 1))) + (pgsize - 1)) & (~(pgsize - 1));
    params.size = (size + (alignment - 1)) & (~(alignment - 1));

    buff->virtualAddress = 0;
    buff->busAddress = 0;
    buff->allocVirtualAddr = 0;
    buff->allocBusAddr = 0;

    err = ioctl(enc_ewl->fd_v4l2, VSI_IOCTL_CMD_ALLOC, &params);
    if (err < 0) {
        PTRACE_R((void *)instance, "EWLMallocLiner: failed to alloc mem.\n");
        return EWL_ERROR;
    }
    buff->allocVirtualAddr =
        (u32 *)mmap((void *)params.busaddr, params.size, PROT_READ | PROT_WRITE, MAP_SHARED,
                    enc_ewl->fd_v4l2, (unsigned long long)params.id);
    if (buff->allocVirtualAddr == MAP_FAILED) {
        PTRACE_I((void *)instance, "EWLInit: Failed to mmap busAddress: %p\n",
                 (void *)params.busaddr);
        return EWL_ERROR;
    }

    /* ASIC might be in different address space */
    buff->allocBusAddr =
        params.busaddr; // BUS_CPU_TO_ASIC(params.busAddress, params.translation_offset);
    buff->busAddress =
        (buff->allocBusAddr + (alignment - 1)) & (~(((u64)alignment) - 1)); //left allign

    buff->virtualAddress = buff->allocVirtualAddr + ((alignment - 1) & (~(((u64)alignment) - 1)));
    buff->busAddress = (ptr_t)buff->virtualAddress; //For c-model, busAddress is virtualAddress
    buff->id = params.id;

    return EWL_OK;
#endif
}

/*------------------------------------------------------------------------------
    Function name   : EWLFreeLinear
    Description     : Release a linera memory buffer, previously allocated with
                        EWLMallocLinear.

    Return type     : void

    Argument        : const void * instance - EWL instance
    Argument        : EWLLinearMem_t *info - linear buffer memory information
------------------------------------------------------------------------------*/
static void EWLFreeLinearSys(const void *instance, EWLLinearMem_t *info)
{
#ifndef V4L2
    ewlSysInstance *inst = (ewlSysInstance *)instance;
    u32 i;
    assert(instance != NULL);

    if (info->busAddress != 0) {
        for (i = 0; i < inst->totalChunks; i++) {
            if (inst->alignedChunks[i] == (u32 *)info->busAddress) {
                PTRACE_I((void *)instance, "EWLFreeLinear busAddress \t%p\n",
                         (void *)info->busAddress);

                free(inst->chunks[i]);
                inst->chunks[i] = NULL;
                inst->alignedChunks[i] = NULL;
                info->busAddress = 0;
                info->total_size = 0;
                info->size = 0;
                break;
            }
        }
    }

    EWLMemSyncFreeHostBufferSys(inst, info);
#else
    ewlSysInstance *enc_ewl = (ewlSysInstance *)instance;
    EWLLinearMem_t *buff = (EWLLinearMem_t *)info;
    vsi_v4l2_mem_info meminfo;
    meminfo.id = buff->id;

    assert(enc_ewl != NULL);
    assert(buff != NULL);

    if (buff->allocBusAddr != 0)
        ioctl(enc_ewl->fd_v4l2, VSI_IOCTL_CMD_FREE, &meminfo);

    if (buff->allocVirtualAddr != MAP_FAILED)
        munmap(buff->allocVirtualAddr, buff->total_size);

    PTRACE_I((void *)instance, "EWLFreeLinear\t%p\n", buff->allocVirtualAddr);

    //reset buffer information
    buff->allocBusAddr = 0;
    buff->allocVirtualAddr = NULL;
    buff->busAddress = 0;
    buff->virtualAddress = NULL;
    buff->total_size = 0;
    buff->size = 0;

#endif
}

/*------------------------------------------------------------------------------
    Function name   : EWLMallocRefFrm
    Description     : Allocate a frame buffer (contiguous linear RAM memory)

    Return type     : i32 - 0 for success or a negative error code

    Argument        : const void * instance - EWL instance
    Argument        : u32 size - size in bytes of the requested memory
    Argument        : EWLLinearMem_t *info - place where the allocated memory
                        buffer parameters are returned
------------------------------------------------------------------------------*/
static i32 EWLMallocRefFrmSys(const void *instance, u32 size, u32 alignment, EWLLinearMem_t *info)
{
    ewlSysInstance *inst = (ewlSysInstance *)instance;
    assert(instance != NULL);
    assert(inst->refFrmChunks < MEM_CHUNKS);
    assert(inst->totalChunks < MEM_CHUNKS);
    i32 ret;

    ret = EWLMallocLinearSys(instance, size, alignment, info);

    PTRACE_I((void *)instance, "EWLMallocRefFrm\t%8d bytes\n", size);
    return ret;
}

/*------------------------------------------------------------------------------
    Function name   : EWLFreeRefFrm
    Description     : Release a frame buffer previously allocated with
                        EWLMallocRefFrm.

    Return type     : void

    Argument        : const void * instance - EWL instance
    Argument        : EWLLinearMem_t *info - frame buffer memory information
------------------------------------------------------------------------------*/
static void EWLFreeRefFrmSys(const void *instance, EWLLinearMem_t *info)
{
    ewlSysInstance *inst = (ewlSysInstance *)instance;
    u32 i;
    assert(instance != NULL);

    if (info->busAddress != 0) {
        for (i = 0; i < inst->totalChunks; i++) {
            if (inst->alignedChunks[i] == (u32 *)info->busAddress) {
                PTRACE_I((void *)instance, "EWLFreeRefFrm busAddress \t%p\n",
                         (void *)info->busAddress);

                free(inst->chunks[i]);
                inst->chunks[i] = NULL;
                inst->alignedChunks[i] = NULL;
                info->busAddress = 0;
                info->total_size = 0;
                info->size = 0;
                break;
            }
        }
    }
    EWLMemSyncFreeHostBufferSys(inst, info);
}

static void EWLDCacheRangeFlushSys(const void *instance, EWLLinearMem_t *info)
{
    assert(instance != NULL);
    (void)instance;
    assert(info != NULL);
    (void)info;
}

static void EWLDCacheRangeRefreshSys(const void *instance, EWLLinearMem_t *info)
{
    assert(instance != NULL);
    (void)instance;
    assert(info != NULL);
    (void)info;
}

static i32 EWLWaitHwRdySys(const void *instance, u32 *slicesReady, void *waitOut,
                           u32 *status_register)
{
    ewlSysInstance *inst = (ewlSysInstance *)instance;
    assert(inst != NULL);
    u32 core_id = FIRST_CORE(inst);
    SysCore *core = inst->core[CORE(core_id)];
    u32 idx;

    if (!(CoreEncGetRegisterValue(core, HWIF_ENC_MODE) == ASIC_HEVC ||
          CoreEncGetRegisterValue(core, HWIF_ENC_MODE) == ASIC_H264 ||
          CoreEncGetRegisterValue(core, HWIF_ENC_MODE) == ASIC_AV1 ||
          CoreEncGetRegisterValue(core, HWIF_ENC_MODE) == ASIC_VP9)) {
        CoreEncWaitHwRdy(inst->core[CORE(core_id)]);
        *status_register = EWLReadRegSys(inst, HSWREG(1));
#ifdef VIRTUAL_PLATFORM_TEST
        if (CoreEncGetRegisterValue(core, HWIF_ENC_MODE) == ASIC_JPEG) {
            goto vdk_test;
        }
#endif
        return EWL_OK;
    }

    pthread_mutex_lock(&ewl_mutex);

    for (int i = 0; i < MAX_CORE_NUM; i++) {
        if (!subSysOut[i].has_irq)
            continue;

        core = inst->core[i];
        if (subSysOut[i].total_slice_num) {
            /* The model has finished encoding the whole frame but here
             * we can emulate the slice ready interrupt by outputting
             * different values of slicesReady. */
            subSysOut[i].slice_rdy_num = subSysOut[i].slice_rdy_num + 1;
            /* Set the frame ready status so that SW thinks a slice
             * is ready but frame encoding continues. This function
             * will be called again for every slice. */
            if (subSysOut[i].slice_rdy_num < subSysOut[i].total_slice_num) {
                CoreEncSetRegisterValue(core, HWIF_ENC_FRAME_RDY_STATUS, 0);
                CoreEncSetRegisterValue(core, HWIF_ENC_SLICE_RDY_STATUS, 1);
            } else {
                CoreEncSetRegisterValue(core, HWIF_ENC_FRAME_RDY_STATUS, 1);
            }
        }
        /*simulate stream segment interrupt when frame_rdy_irq is triggered*/
        if (CoreEncGetRegisterValue(core, HWIF_ENC_STRM_SEGMENT_EN)) {
            if (inst->frameRdy == -1)
                inst->frameRdy = (CoreEncGetRegister(core, HSWREG(1)) & 0x04) == 0x04;

            if (inst->frameRdy == 0)
                goto end;

            if (inst->streamLength == 0)
                inst->streamLength =
                    CoreEncGetRegisterValue(core, HWIF_ENC_OUTPUT_STRM_BUFFER_LIMIT);
            u32 segmentSize = CoreEncGetRegisterValue(core, HWIF_ENC_STRM_SEGMENT_SIZE);

            u32 rd = CoreEncGetRegisterValue(core, HWIF_ENC_STRM_SEGMENT_RD_PTR);
            u32 wr = CoreEncGetRegisterValue(core, HWIF_ENC_STRM_SEGMENT_WR_PTR);
            u8 *streamBase = inst->streamBase + segmentSize * (wr % inst->segmentAmount);
            u8 *streamTmpBase = inst->streamTempBuffer + segmentSize * wr;

            if ((CoreEncGetRegisterValue(core, HWIF_ENC_STRM_SEGMENT_SW_SYNC_EN) == 0) ||
                ((CoreEncGetRegisterValue(core, HWIF_ENC_STRM_SEGMENT_SW_SYNC_EN) == 1) &&
                 (wr >= rd && wr - rd < inst->segmentAmount))) {
                memcpy(streamBase, streamTmpBase, segmentSize);
                CoreEncSetRegisterValue(core, HWIF_ENC_STRM_SEGMENT_WR_PTR, ++wr);

                if (inst->streamLength > segmentSize) {
                    printf("---->trigger segment IRQ\n");
                    CoreEncSetRegisterValue(core, HWIF_ENC_FRAME_RDY_STATUS, 0);
                    CoreEncSetRegisterValue(core, HWIF_ENC_STRM_SEGMENT_RDY_INT, 1);
                } else {
                    printf("---->trigger frame ready IRQ\n");
                    CoreEncSetRegisterValue(core, HWIF_ENC_FRAME_RDY_STATUS, 1);
                    CoreEncSetRegisterValue(core, HWIF_ENC_STRM_SEGMENT_RDY_INT, 0);
                }

                inst->streamLength -= segmentSize;
            }
        }
        CoreEncWaitHwRdy(inst->core[CORE(core_id)]);
    end:
        idx = ((EWLCoreWaitOut_t *)waitOut)->irq_num;
        ((EWLCoreWaitOut_t *)waitOut)->irq_status[idx] = CoreEncGetRegister(core, HSWREG(1));
        ((EWLCoreWaitOut_t *)waitOut)->job_id[idx] = subSysOut[i].job_id;
        ((EWLCoreWaitOut_t *)waitOut)->irq_num++;
    }
    pthread_mutex_unlock(&ewl_mutex);

#ifdef VIRTUAL_PLATFORM_TEST
vdk_test:
    EWLVirtualPlatformTestOut(core);
#endif

    return EWL_OK;
}

static i32 EWLHwSysDone(const void *instance)
{
    ewlSysInstance *inst = (ewlSysInstance *)instance;
    assert(inst != NULL);
    u32 core_id = FIRST_CORE(inst);
    SysCore *core = inst->core[CORE(core_id)];

    if (!(CoreEncGetRegisterValue(core, HWIF_ENC_MODE) == ASIC_HEVC ||
          CoreEncGetRegisterValue(core, HWIF_ENC_MODE) == ASIC_H264 ||
          CoreEncGetRegisterValue(core, HWIF_ENC_MODE) == ASIC_AV1 ||
          CoreEncGetRegisterValue(core, HWIF_ENC_MODE) == ASIC_VP9))
        return EWL_OK;

    pthread_mutex_lock(&ewl_mutex);

    subSysOut[CORE(core_id)].has_irq = 1;

    subSysOut[CORE(core_id)].job_id = reserved_job_id[CORE(core_id)];
    subSysOut[CORE(core_id)].total_slice_num =
        CoreEncGetRegisterValue(core, HWIF_ENC_NUM_SLICES_READY);

    /*simulate stream segment interrupt when frame_rdy_irq is triggered*/
    if (CoreEncGetRegisterValue(core, HWIF_ENC_STRM_SEGMENT_EN)) {
        if (inst->frameRdy == -1)
            inst->frameRdy = (CoreEncGetRegister(core, HSWREG(1)) & 0x04) == 0x04;

        if (inst->frameRdy == 0)
            goto end;

        if (inst->streamLength == 0)
            inst->streamLength = CoreEncGetRegisterValue(core, HWIF_ENC_OUTPUT_STRM_BUFFER_LIMIT);
        u32 segmentSize = CoreEncGetRegisterValue(core, HWIF_ENC_STRM_SEGMENT_SIZE);

        u32 rd = CoreEncGetRegisterValue(core, HWIF_ENC_STRM_SEGMENT_RD_PTR);
        u32 wr = CoreEncGetRegisterValue(core, HWIF_ENC_STRM_SEGMENT_WR_PTR);
        u8 *streamBase = inst->streamBase + segmentSize * (wr % inst->segmentAmount);
        u8 *streamTmpBase = inst->streamTempBuffer + segmentSize * wr;

        if ((CoreEncGetRegisterValue(core, HWIF_ENC_STRM_SEGMENT_SW_SYNC_EN) == 0) ||
            ((CoreEncGetRegisterValue(core, HWIF_ENC_STRM_SEGMENT_SW_SYNC_EN) == 1) &&
             (wr >= rd && wr - rd < inst->segmentAmount))) {
            memcpy(streamBase, streamTmpBase, segmentSize);
            CoreEncSetRegisterValue(core, HWIF_ENC_STRM_SEGMENT_WR_PTR, ++wr);

            if (inst->streamLength > segmentSize) {
                printf("---->trigger segment IRQ\n");
                CoreEncSetRegisterValue(core, HWIF_ENC_FRAME_RDY_STATUS, 0);
                CoreEncSetRegisterValue(core, HWIF_ENC_STRM_SEGMENT_RDY_INT, 1);
            } else {
                printf("---->trigger frame ready IRQ\n");
                CoreEncSetRegisterValue(core, HWIF_ENC_FRAME_RDY_STATUS, 1);
                CoreEncSetRegisterValue(core, HWIF_ENC_STRM_SEGMENT_RDY_INT, 0);
            }

            inst->streamLength -= segmentSize;
        }
    }

end:

    pthread_mutex_unlock(&ewl_mutex);

    return EWL_OK;
}

static i32 EWLReserveHwSys(const void *inst, u32 *core_info, u32 *job_id)
{
    ewlSysInstance *instance = (ewlSysInstance *)inst;
    i32 core_num = EWLGetCoreNumSys(NULL);
    u32 core_bits = 0;
    i32 ret, core_idx;
    u32 client_type = instance->clientType;
    EWLHwConfig_t hw_cfg;
    static u32 jobId = 0;

    assert(inst != NULL);
    (void)inst;
    PTRACE_I((void *)instance, "EWLReserveHw\n");
    EWLWorker *worker;
    while (1) {
        pthread_mutex_lock(&ewl_mutex);
        worker = (EWLWorker *)queue_get(&instance->freelist);
        pthread_mutex_unlock(&ewl_mutex);
        if (worker == NULL)
            usleep(10);
        else
            break;
    }

    for (core_idx = 0; core_idx < core_num; core_idx++) {
        hw_cfg = EWLReadAsicConfigSys(core_idx, NULL);
        if (hw_cfg.hevcEnabled == 1 && client_type == EWL_CLIENT_TYPE_HEVC_ENC) {
            core_bits |= 1 << core_idx;
            continue;
        } else if (hw_cfg.h264Enabled == 1 && client_type == EWL_CLIENT_TYPE_H264_ENC) {
            core_bits |= 1 << core_idx;
            continue;
        } else if (hw_cfg.av1Enabled == 1 && client_type == EWL_CLIENT_TYPE_AV1_ENC) {
            core_bits |= 1 << core_idx;
            continue;
        } else if (hw_cfg.jpegEnabled == 1 && client_type == EWL_CLIENT_TYPE_JPEG_ENC) {
            core_bits |= 1 << core_idx;
            continue;
        } else if (hw_cfg.cuTreeSupport == 1 && client_type == EWL_CLIENT_TYPE_CUTREE) {
            core_bits |= 1 << core_idx;
            continue;
        } else if (hw_cfg.vsSupport == 1 && client_type == EWL_CLIENT_TYPE_VIDEOSTAB) {
            core_bits |= 1 << core_idx;
            continue;
        } else if (hw_cfg.vp9Enabled == 1 && client_type == EWL_CLIENT_TYPE_VP9_ENC) {
            core_bits |= 1 << core_idx;
            continue;
        }
    }

    core_idx = 0;
    while (1) {
        if ((core_bits >> core_idx) & 0x1) {
            ret = CoreEncTryReserveHw(instance->core[core_idx]);
            if (ret == 0) {
                PTRACE_I((void *)instance, "Reserve: inst %p %p %p, core=%d\n",
                         instance->winst.hevc, instance->winst.jpeg, instance->winst.cutree,
                         core_idx);
                break;
            } else {
                usleep(10);
            }
        }
        core_idx = (core_idx + 1) % core_num;
    }
    worker->core_id = COREID(NODE(worker->core_id), core_idx);

    pthread_mutex_lock(&ewl_mutex);
    //queue_remove(&instance->freelist, (struct node *)worker);
    queue_put(&instance->workers, (struct node *)worker);
    pthread_mutex_unlock(&ewl_mutex);

    if (instance->clientType == EWL_CLIENT_TYPE_HEVC_ENC ||
        instance->clientType == EWL_CLIENT_TYPE_H264_ENC ||
        instance->clientType == EWL_CLIENT_TYPE_AV1_ENC ||
        instance->clientType == EWL_CLIENT_TYPE_VP9_ENC) {
        pthread_mutex_lock(&ewl_mutex);
        reserved_job_id[core_idx] = jobId;
        *job_id = jobId++;
        pthread_mutex_unlock(&ewl_mutex);
    }

    return EWL_OK;
}

static void EWLReleaseHwSys(const void *inst)
{
    ewlSysInstance *instance = (ewlSysInstance *)inst;
    assert(inst != NULL);
    (void)inst;
    PTRACE_I((void *)inst, "EWLReleaseHw\n");
    instance->performance = EWLReadRegSys(inst, 82 * 4); //save the performance before release hw
    pthread_mutex_lock(&ewl_mutex);
    EWLWorker *worker = (EWLWorker *)queue_get(&instance->workers);
    pthread_mutex_unlock(&ewl_mutex);
    if (worker == NULL)
        return;

    if (instance->clientType == EWL_CLIENT_TYPE_HEVC_ENC ||
        instance->clientType == EWL_CLIENT_TYPE_H264_ENC ||
        instance->clientType == EWL_CLIENT_TYPE_AV1_ENC ||
        instance->clientType == EWL_CLIENT_TYPE_VP9_ENC) {
        pthread_mutex_lock(&ewl_mutex);
        subSysOut[CORE(worker->core_id)].has_irq = 0;
        subSysOut[CORE(worker->core_id)].slice_rdy_num = 0;
        subSysOut[CORE(worker->core_id)].total_slice_num = 0;
        subSysOut[CORE(worker->core_id)].irq_status = 0;
        pthread_mutex_unlock(&ewl_mutex);
    }

    //queue_remove(&instance->workers, (struct node *)worker);
    CoreEncReleaseHw(instance->core[CORE(worker->core_id)]);
    pthread_mutex_lock(&ewl_mutex);
    queue_put(&instance->freelist, (struct node *)worker);
    pthread_mutex_unlock(&ewl_mutex);
    PTRACE_I((void *)inst, "Release: inst %p %p %p, core=%x\n", instance->winst.hevc,
             instance->winst.jpeg, instance->winst.cutree, worker->core_id);
}

/* use slice 0 because slice node is symitrical */
static u32 EWLGetCoreNumSys(void *ctx)
{
    return CoreEncGetCoreNum(0);
}

#if 0
/*------------------------------------------------------------------------------

    Swap endianess of buffer

    wordCnt is the amount of 32-bit words that are swapped

------------------------------------------------------------------------------*/
void EWL_TEST_SwapEndian(u32 *buf, u32 wordCnt)
{
    u32 i = 0;

    while (wordCnt != 0) {
        u32 val = buf[i];
        u32 tmp = 0;

        tmp |= (val & 0xFF) << 24;
        tmp |= (val & 0xFF00) << 8;
        tmp |= (val & 0xFF0000) >> 8;
        tmp |= (val & 0xFF000000) >> 24;
        buf[i] = tmp;
        i++;
        wordCnt--;
    }
}
#endif

static void EWLClearTraceProfileSys(const void *instance)
{
    ewlSysInstance *inst = (ewlSysInstance *)instance;

    inst->prof.frame_number = 0;
    inst->prof.total_bits = 0;
    inst->prof.total_Y_PSNR = 0.0;
    inst->prof.total_U_PSNR = 0.0;
    inst->prof.total_V_PSNR = 0.0;
    inst->prof.Y_PSNR_Max = -1;
    inst->prof.Y_PSNR_Min = 1000.0;
    inst->prof.U_PSNR_Max = -1;
    inst->prof.U_PSNR_Min = 1000.0;
    inst->prof.V_PSNR_Max = -1;
    inst->prof.V_PSNR_Min = 1000.0;
    inst->prof.total_ssim = 0.0;
    inst->prof.total_ssim_y = 0.0;
    inst->prof.total_ssim_u = 0.0;
    inst->prof.total_ssim_v = 0.0;
    inst->prof.QP_Max = 0;
    inst->prof.QP_Min = 52;
}

/*------------------------------------------------------------------------------
    Function name   : EWLTraceProfile
    Description     : print the PSNR and SSIM data, only valid for c-model.
    Return type     : void
    Argument        : none
------------------------------------------------------------------------------*/
static void EWLTraceProfileSys(const void *instance, void *prof_data, i32 qp, i32 poc)
{
    ewlSysInstance *inst = (ewlSysInstance *)instance;
    float bit_rate_avg;
    float Y_PSNR_avg;
    float U_PSNR_avg;
    float V_PSNR_avg;
    //u32 core_id = LAST_CORE(inst);
    //SysCore *core = inst->core[CORE(core_id)];

    //i32 qp;

    // MUST be the same as the struct in "pictur".
    struct {
        i32 bitnum;
        float psnr_y, psnr_u, psnr_v;
        double ssim;
        double ssim_y, ssim_u, ssim_v;
    } prof;

    //void *prof_data;
    //i32 poc;

    //prof_data = (void*)CoreEncGetAddrRegisterValue(core, HWIF_ENC_COMPRESSEDCOEFF_BASE);
    //qp = CoreEncGetRegisterValue(core, HWIF_ENC_PIC_QP);
    //poc = CoreEncGetRegisterValue(core, HWIF_ENC_POC);

    memcpy(&prof, prof_data, sizeof(prof));

    if (inst->prof.QP_Max < qp)
        inst->prof.QP_Max = qp;
    if (inst->prof.QP_Min > qp)
        inst->prof.QP_Min = qp;
    if (inst->prof.Y_PSNR_Max < prof.psnr_y)
        inst->prof.Y_PSNR_Max = prof.psnr_y;
    if (inst->prof.Y_PSNR_Min > prof.psnr_y)
        inst->prof.Y_PSNR_Min = prof.psnr_y;
    if (inst->prof.U_PSNR_Max < prof.psnr_y)
        inst->prof.U_PSNR_Max = prof.psnr_y;
    if (inst->prof.U_PSNR_Min > prof.psnr_y)
        inst->prof.U_PSNR_Min = prof.psnr_y;
    if (inst->prof.V_PSNR_Max < prof.psnr_y)
        inst->prof.V_PSNR_Max = prof.psnr_y;
    if (inst->prof.V_PSNR_Min > prof.psnr_y)
        inst->prof.V_PSNR_Min = prof.psnr_y;
    inst->prof.frame_number++;
    inst->prof.total_bits += prof.bitnum;
    bit_rate_avg = (inst->prof.total_bits / inst->prof.frame_number) * 30;
    inst->prof.total_Y_PSNR += prof.psnr_y;
    Y_PSNR_avg = inst->prof.total_Y_PSNR / inst->prof.frame_number;
    inst->prof.total_U_PSNR += prof.psnr_u;
    U_PSNR_avg = inst->prof.total_U_PSNR / inst->prof.frame_number;
    inst->prof.total_V_PSNR += prof.psnr_v;
    V_PSNR_avg = inst->prof.total_V_PSNR / inst->prof.frame_number;

    inst->prof.total_ssim += prof.ssim;
    inst->prof.total_ssim_y += prof.ssim_y;
    inst->prof.total_ssim_u += prof.ssim_u;
    inst->prof.total_ssim_v += prof.ssim_v;

    printf("    CModel::POC %3d QP %3d %9d bits [Y %.4f dB  U %.4f dB  V %.4f dB] "
           "[SSIM %.4f average_SSIM %.4f] [SSIM Y %.4f U %.4f V %.4f average_SSIM Y "
           "%.4f U %.4f V %.4f]\n",
           poc, qp, prof.bitnum, prof.psnr_y, prof.psnr_u, prof.psnr_v, prof.ssim,
           inst->prof.total_ssim / inst->prof.frame_number, prof.ssim_y, prof.ssim_u, prof.ssim_v,
           inst->prof.total_ssim_y / inst->prof.frame_number,
           inst->prof.total_ssim_u / inst->prof.frame_number,
           inst->prof.total_ssim_v / inst->prof.frame_number);
    printf("    CModel::POC %3d QPMin/QPMax %d/%d Y_PSNR_Min/Max %.4f/%.4f dB "
           "U_PSNR_Min/Max %.4f/%.4f dB V_PSNR_Min/Max %.4f/%.4f \n",
           poc, inst->prof.QP_Min, inst->prof.QP_Max, inst->prof.Y_PSNR_Min, inst->prof.Y_PSNR_Max,
           inst->prof.U_PSNR_Min, inst->prof.U_PSNR_Max, inst->prof.V_PSNR_Min,
           inst->prof.V_PSNR_Max);
    printf("    CModel::POC %3d frame %d Y_PSNR_avg %.4f dB  U_PSNR_avg %.4f dB "
           "V_PSNR_avg %.4f dB \n",
           poc, inst->prof.frame_number - 1, Y_PSNR_avg, U_PSNR_avg, V_PSNR_avg);
}

/*------------------------------------------------------------------------------
    Function name   : EWLGetLineBufSram
    Description        : Get the base address of on-chip sram used for input MB line buffer.

    Return type     : i32 - 0 for success or a negative error code

    Argument        : const void * instance - EWL instance
    Argument        : EWLLinearMem_t *info - place where the sram parameters are returned
------------------------------------------------------------------------------*/
static i32 EWLGetLineBufSramSys(const void *instance, EWLLinearMem_t *info)
{
    ewlSysInstance *inst = (ewlSysInstance *)instance;
    assert(inst != NULL);
    assert(info != NULL);

    info->virtualAddress = NULL;
    info->busAddress = 0;
    info->size = 0;

    return EWL_OK;
}

/*------------------------------------------------------------------------------
    Function name   : EWLMallocLoopbackLineBuf
    Description        : allocate loopback line buffer in memory, mainly used when there is no on-chip sram

    Return type     : i32 - 0 for success or a negative error code

    Argument        : const void * instance - EWL instance
    Argument        : EWLLinearMem_t *info - place where the mem parameters are returned
------------------------------------------------------------------------------*/
static i32 EWLMallocLoopbackLineBufSys(const void *instance, u32 size, EWLLinearMem_t *info)
{
    ewlSysInstance *inst = (ewlSysInstance *)instance;
    assert(inst != NULL);
    assert(info != NULL);

    info->virtualAddress = NULL;
    info->busAddress = 0;
    info->size = 0;

    return EWL_OK;
}

#ifdef VIRTUAL_PLATFORM_TEST
static void EWLVirtualPlatformTestIn(SysCore *core)
{
    SysBufferInfo BufInfo;
    int ret = 0;
    int i, k, VPBufNum;
    u32 BufLength = 0;
    u32 *tmp_addr = NULL;
    u32 *src_addr = NULL;
    ptr_t addr_lsb, addr_msb;
    for (i = 0, VPBufNum = 0; i < ASIC_SWREG_AMOUNT; i++) {
        ret = CoreEncGetMemoryDescription(core, i, 0, &BufInfo);
        if (ret == 0) {
            addr_lsb = CoreEncGetRegister(core, i * 4);
            addr_msb = CoreEncGetRegister(core, BufInfo.reg_msb_idx * 4);
            if (sizeof(ptr_t) == 8) {
                src_addr = (u32 *)((addr_msb << 32) | addr_lsb);

            } else {
                src_addr = (u32 *)addr_lsb;
            }
            CmodelUseOrigBuf[VPBufNum] = src_addr;
            if (src_addr == NULL) {
                //printf("Note: With current case in buffer, address register index %d is not be used\n", i);
                VPBufNum++;
                continue;
            }
            BufLength = BufInfo.stride * BufInfo.height;
            if (i != 60 || i != 186 || i != 449) {
                BufLength = NEXT_ALIGNED(BufLength);
                BufLength += LINMEM_ALIGN;
            }
            VirtualPlatformBuf[VPBufNum] = (u32 *)calloc(1, BufLength);
            if (sizeof(ptr_t) == 8) {
                core->asicRegs[i] = (ptr_t)VirtualPlatformBuf[VPBufNum] & 0xffffffff;
                core->asicRegs[BufInfo.reg_msb_idx] =
                    ((ptr_t)VirtualPlatformBuf[VPBufNum] >> 32) & 0xffffffff;
            } else {
                core->asicRegs[i] = (ptr_t)VirtualPlatformBuf[VPBufNum] & 0xffffffff;
            }
            /* copy input buffer data */
            tmp_addr = VirtualPlatformBuf[VPBufNum];
            if (BufInfo.flag & 1) { //IN
                for (k = 0; k < BufInfo.height; k++) {
                    EWLmemcpy(tmp_addr, src_addr, BufInfo.width);
                    tmp_addr = (u32 *)((u8 *)tmp_addr + BufInfo.stride);
                    src_addr = (u32 *)((u8 *)src_addr + BufInfo.stride);
                }
            }
            VPBufNum++;
        }
    }
}

static void EWLVirtualPlatformTestOut(SysCore *core)
{
    SysBufferInfo BufInfo;
    int ret = 0;
    int i, k, VPBufNum;
    u32 BufLength = 0;
    u32 *tmp_addr = NULL;
    u32 *src_addr = NULL;
    ptr_t addr_lsb, addr_msb;
    for (i = 0, VPBufNum = 0; i < ASIC_SWREG_AMOUNT; i++) {
        ret = CoreEncGetMemoryDescription(core, i, 1, &BufInfo);
        if (ret == 0) {
            addr_lsb = CoreEncGetRegister(core, i * 4);
            addr_msb = CoreEncGetRegister(core, BufInfo.reg_msb_idx * 4);
            if (sizeof(ptr_t) == 8) {
                tmp_addr = (u32 *)((addr_msb << 32) | addr_lsb);

            } else {
                tmp_addr = (u32 *)addr_lsb;
            }
            src_addr = CmodelUseOrigBuf[VPBufNum];
            if (src_addr == NULL) {
                VPBufNum++;
                //printf("Note: With current case out buffer, address register index %d is not be used\n", i);
                continue;
            }
            //copy input buffer data
            if (BufInfo.flag & 2) { //OUT
                for (k = 0; k < BufInfo.height; k++) {
                    EWLmemcpy(src_addr, tmp_addr, BufInfo.width);
                    tmp_addr = (u32 *)((u8 *)tmp_addr + BufInfo.stride);
                    src_addr = (u32 *)((u8 *)src_addr + BufInfo.stride);
                }
            }
            if (sizeof(ptr_t) == 8) {
                core->asicRegs[i] = (ptr_t)CmodelUseOrigBuf[VPBufNum] & 0xffffffff;
                core->asicRegs[BufInfo.reg_msb_idx] =
                    ((ptr_t)CmodelUseOrigBuf[VPBufNum] >> 32) & 0xffffffff;
            } else {
                core->asicRegs[i] = (ptr_t)CmodelUseOrigBuf[VPBufNum] & 0xffffffff;
            }
            if (VirtualPlatformBuf[VPBufNum]) {
                free(VirtualPlatformBuf[VPBufNum]);
                VirtualPlatformBuf[VPBufNum] = NULL;
            }
            VPBufNum++;
        }
    }
}
#endif

static void EWLSetReserveBaseDataSys(const void *inst, u32 width, u32 height, u32 rdoLevel,
                                     u32 bRDOQEnable, u32 client_type, u16 priority)
{
}

static void EWLCollectWriteRegDataSys(const void *inst, u32 *src, u32 *dst, u16 reg_start,
                                      u32 reg_length, u32 *total_length)
{
}

static void EWLCollectNopDataSys(const void *inst, u32 *dst, u32 *total_length)
{
}

static void EWLCollectStallDataEncVideoSys(const void *inst, u32 *dst, u32 *total_length)
{
}

static void EWLCollectStallDataCuTreeSys(const void *inst, u32 *dst, u32 *total_length)
{
}

static void EWLCollectReadRegDataSys(const void *inst, u32 *dst, u16 reg_start, u32 reg_length,
                                     u32 *total_length, u16 cmdbuf_id)
{
}

static void EWLCollectIntDataSys(const void *inst, u32 *dst, u32 *total_length, u16 cmdbuf_id)
{
}

static void EWLCollectJmpDataSys(const void *inst, u32 *dst, u32 *total_length, u16 cmdbuf_id)
{
}

static void EWLCollectClrIntDataSys(const void *inst, u32 *dst, u32 *total_length)
{
}

static void EWLCollectClrIntReadClearDec400DataSys(const void *inst, u32 *dst, u32 *total_length,
                                                   u16 addr_offset)
{
}

static void EWLCollectStopHwDataSys(const void *inst, u32 *dst, u32 *total_length)
{
}

static void EWLCollectReadVcmdRegDataSys(const void *inst, u32 *dst, u16 reg_start, u32 reg_length,
                                         u32 *total_length, u16 cmdbuf_id)
{
}

static void EWLCollectWriteDec400RegDataSys(const void *inst, u32 *src, u32 *dst, u16 reg_start,
                                            u32 reg_length, u32 *total_length)
{
}

static void EWLCollectStallDec400Sys(const void *inst, u32 *dst, u32 *total_length)
{
}

static u32 EWLReadRegInitSys(const void *inst, u32 offset)
{
    return EWL_OK;
}

static i32 EWLReserveCmdbufSys(const void *inst, u16 size, u16 *cmdbufid)
{
    return EWL_OK;
}

static void EWLCopyDataToCmdbufSys(const void *inst, u32 *src, u32 size, u16 cmdbuf_id)
{
}

static i32 EWLLinkRunCmdbufSys(const void *inst, u16 cmdbufid, u16 cmdbuf_size)
{
    return EWL_OK;
}

static i32 EWLWaitCmdbufSys(const void *inst, u16 cmdbufid, u32 *status)
{
    return EWL_OK;
}

static void EWLGetRegsByCmdbufSys(const void *inst, u16 cmdbufid, u32 *regMirror)
{
    (void)inst;
    (void)cmdbufid;
    (void)regMirror;
}

static i32 EWLReleaseCmdbufSys(const void *inst, u16 cmdbufid)
{
    return EWL_OK;
}

void EWLSetVCMDModeSys(const void *inst, u32 mode)
{
    (void)inst;
    (void)mode;
}

u32 EWLGetVCMDModeSys(const void *inst)
{
    (void)inst;
    return 0;
}

u32 EWLGetVCMDSupportSys()
{
    return 0;
}

u32 EWLReleaseEwlWorkerInstSys(const void *inst)
{
    ewlSysInstance *instance = (ewlSysInstance *)inst;
    if (instance->winst.hevc != NULL) {
        free(instance->winst.hevc);
        instance->winst.hevc = NULL;
    }
    if (instance->winst.jpeg != NULL) {
        free(instance->winst.jpeg);
        instance->winst.jpeg = NULL;
    }
    if (instance->winst.cutree != NULL) {
        free(instance->winst.cutree);
        instance->winst.cutree = NULL;
    }
    return 0;
}

void EWLAttachSys(EWLFun *EWLFunP)
{
    EWLFunP->EWLReadAsicIDP = EWLReadAsicIDSys;
    EWLFunP->EWLReadAsicConfigP = EWLReadAsicConfigSys;
    EWLFunP->EWLReadRegP = EWLReadRegSys;
    EWLFunP->EWLWriteCoreRegP = EWLWriteCoreRegSys;
    EWLFunP->EWLWriteRegP = EWLWriteRegSys;
    EWLFunP->EWLWriteBackRegP = EWLWriteBackRegSys;
    EWLFunP->EWLGetClientTypeP = EWLGetClientTypeSys;
    EWLFunP->EWLGetCoreTypeByClientTypeP = EWLGetCoreTypeByClientTypeSys;
    EWLFunP->EWLCheckCutreeValidP = EWLCheckCutreeValidSys;
    EWLFunP->EWLEnableHWP = EWLEnableHWSys;
    EWLFunP->EWLDisableHWP = EWLDisableHWSys;
    EWLFunP->EWLGetPerformanceP = EWLGetPerformanceSys;
    EWLFunP->EWLGetDec400CoreidP = EWLGetDec400CoreidSys;
    EWLFunP->EWLGetDec400CustomerIDP = EWLGetDec400CustomerIDSys;
    EWLFunP->EWLGetDec400AttributeP = EWLGetDec400AttributeSys;
    EWLFunP->EWLInitP = EWLInitSys;
    EWLFunP->EWLReleaseP = EWLReleaseSys;
    EWLFunP->EwlReleaseCoreWaitP = EwlReleaseCoreWaitSys;
    EWLFunP->EWLDequeueCoreOutJobP = EWLDequeueCoreOutJobSys;
    EWLFunP->EWLEnqueueWaitjobP = EWLEnqueueWaitjobSys;
    EWLFunP->EWLPutJobtoPoolP = EWLPutJobtoPoolSys;
    EWLFunP->EWLMallocRefFrmP = EWLMallocRefFrmSys;
    EWLFunP->EWLFreeRefFrmP = EWLFreeRefFrmSys;
    EWLFunP->EWLMallocLinearP = EWLMallocLinearSys;
    EWLFunP->EWLFreeLinearP = EWLFreeLinearSys;
    EWLFunP->EWLSyncMemDataP = EWLSyncMemDataSys;
    EWLFunP->EWLMemSyncAllocHostBufferP = EWLMemSyncAllocHostBufferSys;
    EWLFunP->EWLMemSyncFreeHostBufferP = EWLMemSyncFreeHostBufferSys;
    EWLFunP->EWLDCacheRangeFlushP = EWLDCacheRangeFlushSys;
    EWLFunP->EWLDCacheRangeRefreshP = EWLDCacheRangeRefreshSys;
    EWLFunP->EWLWaitHwRdyP = EWLWaitHwRdySys;
    EWLFunP->EWLReserveHwP = EWLReserveHwSys;
    EWLFunP->EWLReleaseHwP = EWLReleaseHwSys;
    EWLFunP->EWLGetCoreNumP = EWLGetCoreNumSys;
    EWLFunP->EWLTraceProfileP = EWLTraceProfileSys;
    EWLFunP->EWLGetLineBufSramP = EWLGetLineBufSramSys;
    EWLFunP->EWLMallocLoopbackLineBufP = EWLMallocLoopbackLineBufSys;

    //empty function
    EWLFunP->EWLSetReserveBaseDataP = EWLSetReserveBaseDataSys;
    EWLFunP->EWLCollectWriteRegDataP = EWLCollectWriteRegDataSys;
    EWLFunP->EWLCollectNopDataP = EWLCollectNopDataSys;
    EWLFunP->EWLCollectStallDataEncVideoP = EWLCollectStallDataEncVideoSys;
    EWLFunP->EWLCollectStallDataCuTreeP = EWLCollectStallDataCuTreeSys;
    EWLFunP->EWLCollectReadRegDataP = EWLCollectReadRegDataSys;
    EWLFunP->EWLCollectIntDataP = EWLCollectIntDataSys;
    EWLFunP->EWLCollectJmpDataP = EWLCollectJmpDataSys;
    EWLFunP->EWLCollectClrIntDataP = EWLCollectClrIntDataSys;
    EWLFunP->EWLCollectClrIntReadClearDec400DataP = EWLCollectClrIntReadClearDec400DataSys;
    EWLFunP->EWLCollectStopHwDataP = EWLCollectStopHwDataSys;
    EWLFunP->EWLCollectReadVcmdRegDataP = EWLCollectReadVcmdRegDataSys;
    EWLFunP->EWLCollectWriteDec400RegDataP = EWLCollectWriteDec400RegDataSys;
    EWLFunP->EWLCollectStallDec400P = EWLCollectStallDec400Sys;
    EWLFunP->EWLReadRegInitP = EWLReadRegInitSys;
    EWLFunP->EWLReserveCmdbufP = EWLReserveCmdbufSys;
    EWLFunP->EWLCopyDataToCmdbufP = EWLCopyDataToCmdbufSys;
    EWLFunP->EWLLinkRunCmdbufP = EWLLinkRunCmdbufSys;
    EWLFunP->EWLWaitCmdbufP = EWLWaitCmdbufSys;
    EWLFunP->EWLGetRegsByCmdbufP = EWLGetRegsByCmdbufSys;
    EWLFunP->EWLReleaseCmdbufP = EWLReleaseCmdbufSys;
    EWLFunP->EWLGetVCMDSupportP = EWLGetVCMDSupportSys;
    EWLFunP->EWLSetVCMDModeP = EWLSetVCMDModeSys;
    EWLFunP->EWLGetVCMDModeP = EWLGetVCMDModeSys;
    EWLFunP->EWLReleaseEwlWorkerInstP = EWLReleaseEwlWorkerInstSys;
    EWLFunP->EWLClearTraceProfileP = EWLClearTraceProfileSys;
}
