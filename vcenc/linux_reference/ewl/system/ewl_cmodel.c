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

#include "ewl.h"

/* HW register definitions */
#include "enc_core.h"
#include "osal.h"

EWLFun *EWLFunCmodelP = NULL;

/* Global variables */
#ifdef EWL_PERFORMANCE
static u32 vopNum;
static u32 memAlloc[MEM_CHUNKS];
static u32 memAllocTotal;
static u32 memChunks;
#endif

/* Function to test input line buffer with hardware handshake, should be set by app. (invalid for system) */
u32 (*pollInputLineBufTestFunc)(void) = NULL;

void EWLAttachVCDM(EWLFun *EWLFunP);
void EWLAttachSys(EWLFun *EWLFunP);

void *EWLmalloc(u32 n)
{
#ifdef EWL_PERFORMANCE
    assert(memChunks < MEM_CHUNKS);
    memAlloc[memChunks++] = n;
    memAllocTotal += n;
#endif
    return malloc((size_t)n);
}

void EWLfree(void *p)
{
    free(p);
}

void *EWLcalloc(u32 n, u32 s)
{
#ifdef EWL_PERFORMANCE
    assert(memChunks < MEM_CHUNKS);
    memAlloc[memChunks++] = n * s;
    memAllocTotal += n * s;
#endif
    return calloc((size_t)n, (size_t)s);
}

mem_ret EWLmemcpy(void *d, const void *s, u32 n)
{
    return memcpy(d, s, (size_t)n);
}

mem_ret EWLmemset(void *d, i32 c, u32 n)
{
    return memset(d, (int)c, (size_t)n);
}

int EWLmemcmp(const void *s1, const void *s2, u32 n)
{
#ifdef SAFESTRING
    int ind = 0;
    int rc = 0;
    if ((rc = memcmp_s(s1, (size_t)n, s2, (size_t)n, &ind)) != EOK) {
        printf("%s %u  Ind=%d  Error rc=%u \n", __FUNCTION__, __LINE__, ind, rc);
        ASSERT(0);
    }

    return ind;
#else
    return memcmp(s1, s2, (size_t)n);
#endif
}

i32 EWLGetLineBufSram(const void *instance, EWLLinearMem_t *info)
{
    return EWLFunCmodelP->EWLGetLineBufSramP(instance, info);
}
i32 EWLMallocLoopbackLineBuf(const void *instance, u32 size, EWLLinearMem_t *info)
{
    return EWLFunCmodelP->EWLMallocLoopbackLineBufP(instance, size, info);
}

u32 EWLGetClientType(const void *inst)
{
    return EWLFunCmodelP->EWLGetClientTypeP(inst);
}

u32 EWLGetCoreTypeByClientType(u32 client_type)
{
    return EWLFunCmodelP->EWLGetCoreTypeByClientTypeP(client_type);
}

i32 EWLCheckCutreeValid(const void *inst)
{
    return EWLFunCmodelP->EWLCheckCutreeValidP(inst);
}

u32 EWLReadAsicID(u32 client_type, void *ctx)
{
    return EWLFunCmodelP->EWLReadAsicIDP(client_type, ctx);
}

EWLHwConfig_t EWLReadAsicConfig(u32 client_type, void *ctx)
{
    return EWLFunCmodelP->EWLReadAsicConfigP(client_type, ctx);
}

const void *EWLInit(EWLInitParam_t *param)
{
    return EWLFunCmodelP->EWLInitP(param);
}

i32 EWLRelease(const void *instance)
{
    return EWLFunCmodelP->EWLReleaseP(instance);
}

void EWLWriteRegbyClientType(const void *inst, u32 offset, u32 val, u32 client_type)
{
    (void)inst;
    (void)offset;
    (void)client_type;
}

void EwlReleaseCoreWait(void *inst)
{
    EWLFunCmodelP->EwlReleaseCoreWaitP(inst);
}

EWLCoreWaitJob_t *EWLDequeueCoreOutJob(const void *inst, u32 waitCoreJobid)
{
    return EWLFunCmodelP->EWLDequeueCoreOutJobP(inst, waitCoreJobid);
}

void EWLEnqueueOutToWait(const void *inst, EWLCoreWaitJob_t *job)
{
    EWLFunCmodelP->EWLEnqueueOutToWaitP(inst, job);
}

void EWLEnqueueWaitjob(const void *inst, EWLWaitJobCfg_t *cfg)
{
    EWLFunCmodelP->EWLEnqueueWaitjobP(inst, cfg->waitCoreJobid);
}

void EWLPutJobtoPool(const void *inst, struct node *job)
{
    EWLFunCmodelP->EWLPutJobtoPoolP(inst, job);
}

void EWLWriteCoreReg(const void *instance, u32 offset, u32 val, u32 core_id)
{
    EWLFunCmodelP->EWLWriteCoreRegP(instance, offset, val, core_id);
}

void EWLWriteReg(const void *instance, u32 offset, u32 val)
{
    EWLFunCmodelP->EWLWriteRegP(instance, offset, val);
}

void EWLWriteBackRegbyClientType(const void *inst, u32 offset, u32 val, u32 client_type)
{
    (void)inst;
    (void)offset;
    (void)val;
    (void)client_type;
}

void EWLSetReserveBaseData(const void *inst, u32 width, u32 height, u32 rdoLevel, u32 bRDOQEnable,
                           u32 client_type, u16 priority)
{
    EWLFunCmodelP->EWLSetReserveBaseDataP(inst, width, height, rdoLevel, bRDOQEnable, client_type,
                                          priority);
}

void EWLCollectWriteRegData(const void *inst, u32 *src, u32 *dst, u16 reg_start, u32 reg_length,
                            u32 *total_length)
{
    EWLFunCmodelP->EWLCollectWriteRegDataP(inst, src, dst, reg_start, reg_length, total_length);
}

void EWLCollectNopData(const void *inst, u32 *dst, u32 *total_length)
{
    EWLFunCmodelP->EWLCollectNopDataP(inst, dst, total_length);
}
void EWLCollectStallDataEncVideo(const void *inst, u32 *dst, u32 *total_length)
{
    EWLFunCmodelP->EWLCollectStallDataEncVideoP(inst, dst, total_length);
}

void EWLCollectStallDataCuTree(const void *inst, u32 *dst, u32 *total_length)
{
    EWLFunCmodelP->EWLCollectStallDataCuTreeP(inst, dst, total_length);
}

void EWLCollectReadRegData(const void *inst, u32 *dst, u16 reg_start, u32 reg_length,
                           u32 *total_length, u16 cmdbuf_id)
{
    EWLFunCmodelP->EWLCollectReadRegDataP(inst, dst, reg_start, reg_length, total_length,
                                          cmdbuf_id);
}

void EWLCollectIntData(const void *inst, u32 *dst, u32 *total_length, u16 cmdbuf_id)
{
    EWLFunCmodelP->EWLCollectIntDataP(inst, dst, total_length, cmdbuf_id);
}

void EWLCollectJmpData(const void *inst, u32 *dst, u32 *total_length, u16 cmdbuf_id)
{
    EWLFunCmodelP->EWLCollectJmpDataP(inst, dst, total_length, cmdbuf_id);
}

void EWLCollectClrIntData(const void *inst, u32 *dst, u32 *total_length)
{
    EWLFunCmodelP->EWLCollectClrIntDataP(inst, dst, total_length);
}

void EWLCollectClrIntReadClearDec400Data(const void *inst, u32 *dst, u32 *total_length,
                                         u16 addr_offset)
{
    EWLFunCmodelP->EWLCollectClrIntReadClearDec400DataP(inst, dst, total_length, addr_offset);
}

void EWLCollectStopHwData(const void *inst, u32 *dst, u32 *total_length)
{
    EWLFunCmodelP->EWLCollectStopHwDataP(inst, dst, total_length);
}

void EWLCollectReadVcmdRegData(const void *inst, u32 *dst, u16 reg_start, u32 reg_length,
                               u32 *total_length, u16 cmdbuf_id)
{
    EWLFunCmodelP->EWLCollectReadVcmdRegDataP(inst, dst, reg_start, reg_length, total_length,
                                              cmdbuf_id);
}

void EWLCollectWriteDec400RegData(const void *inst, u32 *src, u32 *dst, u16 reg_start,
                                  u32 reg_length, u32 *total_length)
{
    EWLFunCmodelP->EWLCollectWriteDec400RegDataP(inst, src, dst, reg_start, reg_length,
                                                 total_length);
}

void EWLCollectStallDec400(const void *inst, u32 *dst, u32 *total_length)
{
    EWLFunCmodelP->EWLCollectStallDec400P(inst, dst, total_length);
}

void EWLCollectWriteMMURegData(const void *inst, u32 *dst, u32 *total_length)
{
    (void)inst;
    (void)dst;
    (void)total_length;
}

void EWLWriteBackReg(const void *inst, u32 offset, u32 val)
{
    EWLFunCmodelP->EWLWriteBackRegP(inst, offset, val);
}

void EWLEnableHW(const void *inst, u32 offset, u32 val)
{
    EWLFunCmodelP->EWLEnableHWP(inst, offset, val);
}

void EWLDisableHW(const void *inst, u32 offset, u32 val)
{
    EWLFunCmodelP->EWLDisableHWP(inst, offset, val);
}

u32 EWLGetPerformance(const void *inst)
{
    return EWLFunCmodelP->EWLGetPerformanceP(inst);
}

u32 EWLReadRegbyClientType(const void *inst, u32 offset, u32 client_type)
{
    (void)inst;
    (void)offset;
    (void)client_type;

    return 0;
}

u32 EWLReadReg(const void *inst, u32 offset)
{
    return EWLFunCmodelP->EWLReadRegP(inst, offset);
}

u32 EWLReadRegInit(const void *inst, u32 offset)
{
    return EWLFunCmodelP->EWLReadRegInitP(inst, offset);
}

i32 EWLMallocRefFrm(const void *instance, u32 size, u32 alignment, EWLLinearMem_t *info)
{
    return EWLFunCmodelP->EWLMallocRefFrmP(instance, size, alignment, info);
}

void EWLFreeRefFrm(const void *instance, EWLLinearMem_t *info)
{
    EWLFunCmodelP->EWLFreeRefFrmP(instance, info);
}

i32 EWLMallocLinear(const void *instance, u32 size, u32 alignment, EWLLinearMem_t *info)
{
    return EWLFunCmodelP->EWLMallocLinearP(instance, size, alignment, info);
}

void EWLFreeLinear(const void *instance, EWLLinearMem_t *info)
{
    EWLFunCmodelP->EWLFreeLinearP(instance, info);
}

#ifdef SUPPORT_MEM_SYNC
i32 EWLSyncMemData(EWLLinearMem_t *mem, u32 offset, u32 length, enum EWLMemSyncDirection dir)
{
    return EWLFunCmodelP->EWLSyncMemDataP(mem, offset, length, dir);
}

i32 EWLMemSyncAllocHostBuffer(const void *inst, u32 size, u32 alignment, EWLLinearMem_t *buff)
{
    return EWLFunCmodelP->EWLMemSyncAllocHostBufferP(inst, size, alignment, buff);
}

i32 EWLMemSyncFreeHostBuffer(const void *inst, EWLLinearMem_t *buff)
{
    return EWLFunCmodelP->EWLMemSyncFreeHostBufferP(inst, buff);
}
#endif

i32 EWLGetDec400Coreid(const void *inst)
{
    return EWLFunCmodelP->EWLGetDec400CoreidP(inst);
}

u32 EWLGetDec400CustomerID(const void *inst, u32 core_id)
{
    return EWLFunCmodelP->EWLGetDec400CustomerIDP(inst, core_id);
}

void EWLGetDec400Attribute(const void *inst, u32 *tile_size, u32 *bits_tile_in_table,
                           u32 *planar420_cbcr_arrangement_style)
{
    EWLFunCmodelP->EWLGetDec400AttributeP(inst, tile_size, bits_tile_in_table,
                                          planar420_cbcr_arrangement_style);
}

i32 EWLReserveCmdbuf(const void *inst, u16 size, u16 *cmdbufid)
{
    return EWLFunCmodelP->EWLReserveCmdbufP(inst, size, cmdbufid);
}

void EWLCopyDataToCmdbuf(const void *inst, u32 *src, u32 size, u16 cmdbuf_id)
{
    EWLFunCmodelP->EWLCopyDataToCmdbufP(inst, src, size, cmdbuf_id);
}

i32 EWLLinkRunCmdbuf(const void *inst, u16 cmdbufid, u16 cmdbuf_size)
{
    return EWLFunCmodelP->EWLLinkRunCmdbufP(inst, cmdbufid, cmdbuf_size);
}

i32 EWLWaitCmdbuf(const void *inst, u16 cmdbufid, u32 *status)
{
    return EWLFunCmodelP->EWLWaitCmdbufP(inst, cmdbufid, status);
}

void EWLGetRegsByCmdbuf(const void *inst, u16 cmdbufid, u32 *regMirror)
{
    EWLFunCmodelP->EWLGetRegsByCmdbufP(inst, cmdbufid, regMirror);
}

i32 EWLReleaseCmdbuf(const void *inst, u16 cmdbufid)
{
    return EWLFunCmodelP->EWLReleaseCmdbufP(inst, cmdbufid);
}

void EWLTraceProfile(const void *instance, void *prof_data, i32 qp, i32 poc)
{
    EWLFunCmodelP->EWLTraceProfileP(instance, prof_data, qp, poc);
}

void EWLDCacheRangeFlush(const void *instance, EWLLinearMem_t *info)
{
    EWLFunCmodelP->EWLDCacheRangeFlushP(instance, info);
}

void EWLDCacheRangeRefresh(const void *instance, EWLLinearMem_t *info)
{
    EWLFunCmodelP->EWLDCacheRangeRefreshP(instance, info);
}

i32 EWLWaitHwRdy(const void *instance, u32 *slicesReady, void *waitOut, u32 *status_register)
{
    return EWLFunCmodelP->EWLWaitHwRdyP(instance, slicesReady, waitOut, status_register);
}

i32 EWLReserveHw(const void *inst, u32 *core_info, u32 *job_id)
{
    return EWLFunCmodelP->EWLReserveHwP(inst, core_info, job_id);
}

void EWLReleaseHw(const void *inst)
{
    EWLFunCmodelP->EWLReleaseHwP(inst);
}

u32 EWLGetCoreNum(void *ctx)
{
    return EWLFunCmodelP->EWLGetCoreNumP(ctx);
}

void EWLSetVCMDMode(const void *inst, u32 mode)
{
    EWLFunCmodelP->EWLSetVCMDModeP(inst, mode);
}

u32 EWLGetVCMDMode(const void *inst)
{
    return EWLFunCmodelP->EWLGetVCMDModeP(inst);
}

u32 EWLGetVCMDSupport()
{
    return EWLFunCmodelP->EWLGetVCMDSupportP();
}

u32 EWLReleaseEwlWorkerInst(const void *inst)
{
    return EWLFunCmodelP->EWLReleaseEwlWorkerInstP(inst);
}

void EWLClearTraceProfile(const void *inst)
{
    EWLFunCmodelP->EWLClearTraceProfileP(inst);
}

void EWLAttach(void *ctx, int slice_idx, i32 vcmd_support)
{
    SysCoreInfo CoreInfo;
    if (!EWLFunCmodelP)
        EWLFunCmodelP = (EWLFun *)EWLcalloc(1, sizeof(EWLFun));

    CoreInfo = CoreEncGetHwInfo(0, 0);
    if (vcmd_support == 0) {
        EWLAttachSys(EWLFunCmodelP);
    } else {
        if (CoreInfo.Cfg[0].has_vcmd == HasExtHw)
            EWLAttachVCDM(EWLFunCmodelP);
        else
            EWLAttachSys(EWLFunCmodelP);
    }
    return;
}

void EWLDetach()
{
    if (EWLFunCmodelP) {
        EWLfree(EWLFunCmodelP);
        EWLFunCmodelP = NULL;
    }

    return;
}
