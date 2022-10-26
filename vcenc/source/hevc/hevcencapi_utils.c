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
--  Abstract : VC Encoder API utils
--
------------------------------------------------------------------------------*/
#include <math.h>

#include "hevcencapi.h"
#include "hevcencapi_utils.h"
#include "encasiccontroller.h"
#include "sw_put_bits.h"
#include "pool.h"
#include "sw_slice.h"
#include "tools.h"
#include "axife.h"
#include "ewl.h"
#include "apbfilter.h"
#include "encdec400.h"
#include "hevcenccache.h"

#ifdef INTERNAL_TEST
#include "sw_test_id.h"
#endif

#ifdef TEST_DATA
#include "enctrace.h"
#endif

#include "enctrace.h"

#ifdef VIDEOSTAB_ENABLED
#include "vidstabcommon.h"
#endif

#ifdef SUPPORT_VP9
#include "vp9encapi.h"
#endif

#ifdef SUPPORT_AV1
#include "av1encapi.h"
#endif

#define MAX_SLICE_SIZE_V0 127
/*------------------------------------------------------------------------------

    VCEncShutdown

    Function frees the encoder instance.

    Input   inst    Pointer to the encoder instance to be freed.
                            After this the pointer is no longer valid.

------------------------------------------------------------------------------*/
void VCEncShutdown(VCEncInst inst)
{
    struct vcenc_instance *pEncInst = (struct vcenc_instance *)inst;
    const void *ewl;

    ASSERT(inst);

    ewl = pEncInst->asic.ewl;

    if (pEncInst->asic.regs.vcmdBuf)
        EWLfree(pEncInst->asic.regs.vcmdBuf);

    if (pEncInst->cb_try_new_params)
        if (pEncInst->regs_bak)
            EWLfree(pEncInst->regs_bak);

    if (pEncInst->dec400_data_bak)
        EWLfree(pEncInst->dec400_data_bak);

    if (pEncInst->dec400_osd_bak)
        EWLfree(pEncInst->dec400_osd_bak);

    EncAsicMemFree_V2(&pEncInst->asic);

    EWLfree(pEncInst);

    (void)EWLRelease(ewl);
}

VCEncRet VCEncClear(VCEncInst inst)
{
    struct vcenc_instance *vcenc_instance = (struct vcenc_instance *)inst;
    asicData_s *asic = &vcenc_instance->asic;
    i32 ret = VCENC_OK;
    u32 status = ASIC_STATUS_ERROR;
    EWLCoreWaitJob_t *out = NULL;
    u32 waitCoreJobid;
    u32 next_core_index;

    /* wait for all running job and put to pool in multi-core cases*/
    while (vcenc_instance->reservedCore > 0 && !asic->regs.bVCMDEnable) {
        next_core_index = (vcenc_instance->jobCnt + 1) % vcenc_instance->parallelCoreNum;
        waitCoreJobid = vcenc_instance->waitJobid[next_core_index];

        if ((out = (EWLCoreWaitJob_t *)EWLDequeueCoreOutJob(asic->ewl, waitCoreJobid)) == NULL)
            break;

        EWLPutJobtoPool(asic->ewl, (struct node *)out);
        vcenc_instance->reservedCore--;
        vcenc_instance->jobCnt++;
    }

    /* clear job from job queue only in 1pass */
    if (0 == vcenc_instance->pass) {
        VCEncJob *job = NULL, *prevJob = NULL;
        job = prevJob = (VCEncJob *)queue_tail(&vcenc_instance->jobQueue);

        /* remove the job in queue to buffer pool if it's not empty*/
        while (job != NULL) {
            prevJob = job;
            queue_remove(&vcenc_instance->jobQueue, (struct node *)job);
            PutBufferToPool(vcenc_instance->jobBufferPool, (void **)&job);
            job = (VCEncJob *)prevJob->next;
        }
    }

    return ret;
}

/*------------------------------------------------------------------------------

    Function name : VCEncStop
    Description   : stop encoding process and clear jobs in queue

    Return type   : VCEncRet
    Argument      : inst - the instance to be stopped
------------------------------------------------------------------------------*/
VCEncRet VCEncStop(VCEncInst inst)
{
    struct vcenc_instance *pEncInst = (struct vcenc_instance *)inst;
    struct container *c;
    VCEncRet ret = VCENC_OK;

    APITRACE("VCEncRelease#\n");

    /* Check for illegal inputs */
    if (pEncInst == NULL) {
        APITRACEERR("VCEncRelease: ERROR Null argument\n");
        return VCENC_NULL_ARGUMENT;
    }

    /* Check for existing instance */
    if (pEncInst->inst != inst) {
        APITRACEERR("VCEncRelease: ERROR Invalid instance\n");
        return VCENC_INSTANCE_ERROR;
    }

    /*for 2pass, need to stop lookahead and cutree thread*/
    if (pEncInst->pass == 2 && pEncInst->lookahead.priv_inst) {
        struct vcenc_instance *pEncInst_priv =
            (struct vcenc_instance *)pEncInst->lookahead.priv_inst;

        /* Stop lookahead thread first, put correct status */
        StopLookaheadThread(&pEncInst->lookahead, pEncInst->encStatus == VCENCSTAT_ERROR);

        StopCuTreeThread(&pEncInst_priv->cuTreeCtl, pEncInst->encStatus == VCENCSTAT_ERROR);
    }

    /* just wait for encoding job and clear all of them*/
    if (pEncInst->pass != 1)
        VCEncClear(inst);

    return ret;
}

/*------------------------------------------------------------------------------
  next_picture calculates next input picture depending input and output
  frame rates.
------------------------------------------------------------------------------*/
u64 CalNextPic(VCEncGopConfig *cfg, int picture_cnt)
{
    u64 numer, denom;

    numer = (u64)cfg->inputRateNumer * (u64)cfg->outputRateDenom;
    denom = (u64)cfg->inputRateDenom * (u64)cfg->outputRateNumer;

    return numer * (picture_cnt / (1 << cfg->interlacedFrame)) / denom;
}

/*------------------------------------------------------------------------------
Function name : GenNextPicConfig
Description   : generate the pic reference configure befor one picture encoded
Return type   : void
Argument      : VCEncIn *pEncIn
Argument      : u8 *gopCfgOffset
Argument      : i32 codecH264
Argument      : i32 i32LastPicPoc
------------------------------------------------------------------------------*/
void GenNextPicConfig(VCEncIn *pEncIn, const u8 *gopCfgOffset, i32 i32LastPicPoc,
                      struct vcenc_instance *vcenc_instance)
{
    i32 i, j, k, i32Poc, i32LTRIdx, numRefPics, numRefPics_org = 0;
    u8 u8CfgStart, u8IsLTR_ref, u8IsUpdated;
    i32 i32MaxpicOrderCntLsb = 1 << 16;

    ASSERT(pEncIn != NULL);
    ASSERT(gopCfgOffset != NULL);

    u8CfgStart = gopCfgOffset[pEncIn->gopSize];
    memcpy(&pEncIn->gopCurrPicConfig,
           &(pEncIn->gopConfig.pGopPicCfg[u8CfgStart + pEncIn->gopPicIdx]),
           sizeof(VCEncGopPicConfig));

    pEncIn->i8SpecialRpsIdx = -1;
    pEncIn->i8SpecialRpsIdx_next = -1;
    memset(pEncIn->bLTR_used_by_cur, 0, sizeof(u32) * VCENC_MAX_LT_REF_FRAMES);
    if (0 == pEncIn->gopConfig.special_size)
        return;

    /* update ltr */
    i32 i32RefIdx;
    for (i32RefIdx = 0; i32RefIdx < pEncIn->gopConfig.ltrcnt; i32RefIdx++) {
        if (HANTRO_TRUE == pEncIn->bLTR_need_update[i32RefIdx])
            pEncIn->long_term_ref_pic[i32RefIdx] = i32LastPicPoc;
    }

    memset(pEncIn->bLTR_need_update, 0, sizeof(u32) * pEncIn->gopConfig.ltrcnt);
    if (0 != pEncIn->bIsIDR) {
        i32Poc = 0;
        pEncIn->gopCurrPicConfig.temporalId = 0;
        for (i = 0; i < pEncIn->gopConfig.ltrcnt; i++) {
            for (j = 0; j < pEncIn->gopConfig.special_size; j++) {
                if ((pEncIn->gopConfig.pGopPicSpecialCfg[j].i32Interval <= 0) ||
                    (pEncIn->gopConfig.pGopPicSpecialCfg[j].i32Ltr == 0) ||
                    (((pEncIn->gopConfig.pGopPicSpecialCfg[j].i32Ltr > 0) &&
                      (pEncIn->gopConfig.pGopPicSpecialCfg[j].i32Ltr - 1) != i)))
                    continue;

                pEncIn->long_term_ref_pic[i] = INVALITED_POC;
            }
        }
    } else
        i32Poc = pEncIn->poc;
    /* check the current picture encoded as LTR*/
    pEncIn->u8IdxEncodedAsLTR = 0;
    for (j = 0; j < pEncIn->gopConfig.special_size; j++) {
        if (pEncIn->bIsPeriodUsingLTR == HANTRO_FALSE)
            break;

        true_e bLTRUpdatePeriod = pEncIn->gopConfig.pGopPicSpecialCfg[j].i32Interval > 0;
        true_e bLTRUpdateOneTimes =
            (pEncIn->gopConfig.pGopPicSpecialCfg[j].i32Ltr > 0) &&
            (pEncIn->gopConfig.pGopPicSpecialCfg[j].i32Interval == 0) &&
            (pEncIn->long_term_ref_pic[pEncIn->gopConfig.pGopPicSpecialCfg[j].i32Ltr - 1] ==
             INVALITED_POC);
        if (!(bLTRUpdatePeriod || bLTRUpdateOneTimes) ||
            (pEncIn->gopConfig.pGopPicSpecialCfg[j].i32Ltr == 0))
            continue;

        i32Poc = i32Poc - pEncIn->gopConfig.pGopPicSpecialCfg[j].i32Offset;

        if (i32Poc < 0) {
            i32Poc += i32MaxpicOrderCntLsb;
            if (i32Poc > (i32MaxpicOrderCntLsb >> 1))
                i32Poc = -1;
        }

        i32 interval = pEncIn->gopConfig.pGopPicSpecialCfg[j].i32Interval
                           ? pEncIn->gopConfig.pGopPicSpecialCfg[j].i32Interval
                           : i32MaxpicOrderCntLsb;
        if ((i32Poc >= 0) && (i32Poc % interval == 0)) {
            /* more than one LTR at the same frame position */
            if (0 != pEncIn->u8IdxEncodedAsLTR) {
                // reuse the same POC LTR
                pEncIn->bLTR_need_update[pEncIn->gopConfig.pGopPicSpecialCfg[j].i32Ltr - 1] =
                    HANTRO_TRUE;
                continue;
            }

            pEncIn->gopCurrPicConfig.codingType =
                ((i32)pEncIn->gopConfig.pGopPicSpecialCfg[j].codingType == FRAME_TYPE_RESERVED)
                    ? pEncIn->gopCurrPicConfig.codingType
                    : pEncIn->gopConfig.pGopPicSpecialCfg[j].codingType;
            pEncIn->gopCurrPicConfig.nonReference =
                ((i32)pEncIn->gopConfig.pGopPicSpecialCfg[j].nonReference == FRAME_TYPE_RESERVED)
                    ? pEncIn->gopCurrPicConfig.nonReference
                    : pEncIn->gopConfig.pGopPicSpecialCfg[j].nonReference;
            pEncIn->gopCurrPicConfig.numRefPics =
                ((i32)pEncIn->gopConfig.pGopPicSpecialCfg[j].numRefPics == NUMREFPICS_RESERVED)
                    ? pEncIn->gopCurrPicConfig.numRefPics
                    : pEncIn->gopConfig.pGopPicSpecialCfg[j].numRefPics;
            pEncIn->gopCurrPicConfig.QpFactor =
                (pEncIn->gopConfig.pGopPicSpecialCfg[j].QpFactor == QPFACTOR_RESERVED)
                    ? pEncIn->gopCurrPicConfig.QpFactor
                    : pEncIn->gopConfig.pGopPicSpecialCfg[j].QpFactor;
            pEncIn->gopCurrPicConfig.QpOffset =
                (pEncIn->gopConfig.pGopPicSpecialCfg[j].QpOffset == QPOFFSET_RESERVED)
                    ? pEncIn->gopCurrPicConfig.QpOffset
                    : pEncIn->gopConfig.pGopPicSpecialCfg[j].QpOffset;
            pEncIn->gopCurrPicConfig.temporalId =
                (pEncIn->gopConfig.pGopPicSpecialCfg[j].temporalId == TEMPORALID_RESERVED)
                    ? pEncIn->gopCurrPicConfig.temporalId
                    : pEncIn->gopConfig.pGopPicSpecialCfg[j].temporalId;

            if (((i32)pEncIn->gopConfig.pGopPicSpecialCfg[j].numRefPics != NUMREFPICS_RESERVED)) {
                for (k = 0; k < (i32)pEncIn->gopCurrPicConfig.numRefPics; k++) {
                    pEncIn->gopCurrPicConfig.refPics[k].ref_pic =
                        pEncIn->gopConfig.pGopPicSpecialCfg[j].refPics[k].ref_pic;
                    pEncIn->gopCurrPicConfig.refPics[k].used_by_cur =
                        pEncIn->gopConfig.pGopPicSpecialCfg[j].refPics[k].used_by_cur;
                }
            }

            pEncIn->u8IdxEncodedAsLTR = pEncIn->gopConfig.pGopPicSpecialCfg[j].i32Ltr;
            pEncIn->bLTR_need_update[pEncIn->u8IdxEncodedAsLTR - 1] = HANTRO_TRUE;
        }
    }

    if (0 != pEncIn->bIsIDR)
        return;

    u8IsUpdated = 0;
    for (j = 0; j < pEncIn->gopConfig.special_size; j++) {
        if (pEncIn->gopConfig.pGopPicSpecialCfg[j].i32Interval <= 0)
            continue;

        /* no changed */
        if (((i32)pEncIn->gopConfig.pGopPicSpecialCfg[j].codingType == FRAME_TYPE_RESERVED) &&
            ((i32)pEncIn->gopConfig.pGopPicSpecialCfg[j].numRefPics == NUMREFPICS_RESERVED) &&
            (pEncIn->gopConfig.pGopPicSpecialCfg[j].QpFactor == QPFACTOR_RESERVED) &&
            (pEncIn->gopConfig.pGopPicSpecialCfg[j].QpOffset == QPOFFSET_RESERVED) &&
            (pEncIn->gopConfig.pGopPicSpecialCfg[j].temporalId == TEMPORALID_RESERVED))
            continue;

        /* only consider LTR ref config */
        if (((i32)pEncIn->gopConfig.pGopPicSpecialCfg[j].numRefPics == NUMREFPICS_RESERVED)) {
            /* reserved for later */
            pEncIn->i8SpecialRpsIdx = -1;
            u8IsUpdated = 1;
            continue;
        }

        if ((pEncIn->gopConfig.pGopPicSpecialCfg[j].codingType == VCENC_INTRA_FRAME) &&
            (pEncIn->gopConfig.pGopPicSpecialCfg[j].numRefPics == 0))
            numRefPics = 1;
        else
            numRefPics = pEncIn->gopConfig.pGopPicSpecialCfg[j].numRefPics;

        numRefPics_org = pEncIn->gopConfig.pGopPicSpecialCfg[j].numRefPics;

        // check if all LTR are OK for current cfg
        true_e bUsedCurrCfg = HANTRO_FALSE;
        for (k = 0; k < numRefPics; k++) {
            u8IsLTR_ref = IS_LONG_TERM_REF_DELTAPOC(
                pEncIn->gopConfig.pGopPicSpecialCfg[j].refPics[k].ref_pic);
            if ((u8IsLTR_ref == HANTRO_FALSE) &&
                (0 == pEncIn->gopConfig.pGopPicSpecialCfg[j].i32short_change))
                continue;
            i32LTRIdx = (u8IsLTR_ref == HANTRO_TRUE)
                            ? LONG_TERM_REF_DELTAPOC2ID(
                                  pEncIn->gopConfig.pGopPicSpecialCfg[j].refPics[k].ref_pic)
                            : 0;

            if ((pEncIn->long_term_ref_pic[i32LTRIdx] == INVALITED_POC) &&
                (u8IsLTR_ref != HANTRO_FALSE) &&
                (0 == pEncIn->gopConfig.pGopPicSpecialCfg[j].i32short_change))
                continue;

            i32Poc = pEncIn->poc - pEncIn->gopConfig.pGopPicSpecialCfg[j].i32Offset;

            if (i32Poc < 0) {
                i32Poc += i32MaxpicOrderCntLsb;
                if (i32Poc > (i32MaxpicOrderCntLsb >> 1))
                    i32Poc = -1;
            }

            if ((i32Poc >= 0) &&
                (i32Poc % pEncIn->gopConfig.pGopPicSpecialCfg[j].i32Interval == 0)) {
                bUsedCurrCfg = HANTRO_TRUE;
            } else {
                bUsedCurrCfg = HANTRO_FALSE;
                break;
            }
        }

        if (bUsedCurrCfg == HANTRO_TRUE) {
            pEncIn->gopCurrPicConfig.codingType =
                ((i32)pEncIn->gopConfig.pGopPicSpecialCfg[j].codingType == FRAME_TYPE_RESERVED)
                    ? pEncIn->gopCurrPicConfig.codingType
                    : pEncIn->gopConfig.pGopPicSpecialCfg[j].codingType;
            pEncIn->gopCurrPicConfig.nonReference =
                ((i32)pEncIn->gopConfig.pGopPicSpecialCfg[j].nonReference == FRAME_TYPE_RESERVED)
                    ? pEncIn->gopCurrPicConfig.nonReference
                    : pEncIn->gopConfig.pGopPicSpecialCfg[j].nonReference;
            pEncIn->gopCurrPicConfig.numRefPics =
                ((i32)pEncIn->gopConfig.pGopPicSpecialCfg[j].numRefPics == NUMREFPICS_RESERVED)
                    ? pEncIn->gopCurrPicConfig.numRefPics
                    : pEncIn->gopConfig.pGopPicSpecialCfg[j].numRefPics;
            pEncIn->gopCurrPicConfig.QpFactor =
                (pEncIn->gopConfig.pGopPicSpecialCfg[j].QpFactor == QPFACTOR_RESERVED)
                    ? pEncIn->gopCurrPicConfig.QpFactor
                    : pEncIn->gopConfig.pGopPicSpecialCfg[j].QpFactor;
            pEncIn->gopCurrPicConfig.QpOffset =
                (pEncIn->gopConfig.pGopPicSpecialCfg[j].QpOffset == QPOFFSET_RESERVED)
                    ? pEncIn->gopCurrPicConfig.QpOffset
                    : pEncIn->gopConfig.pGopPicSpecialCfg[j].QpOffset;
            pEncIn->gopCurrPicConfig.temporalId =
                (pEncIn->gopConfig.pGopPicSpecialCfg[j].temporalId == TEMPORALID_RESERVED)
                    ? pEncIn->gopCurrPicConfig.temporalId
                    : pEncIn->gopConfig.pGopPicSpecialCfg[j].temporalId;

            if ((i32)pEncIn->gopConfig.pGopPicSpecialCfg[j].numRefPics != NUMREFPICS_RESERVED) {
                for (i = 0; i < (i32)pEncIn->gopConfig.pGopPicSpecialCfg[j].numRefPics; i++) {
                    pEncIn->gopCurrPicConfig.refPics[i].ref_pic =
                        pEncIn->gopConfig.pGopPicSpecialCfg[j].refPics[i].ref_pic;
                    pEncIn->gopCurrPicConfig.refPics[i].used_by_cur =
                        pEncIn->gopConfig.pGopPicSpecialCfg[j].refPics[i].used_by_cur;
                }
            }
            pEncIn->i8SpecialRpsIdx = j;
            u8IsUpdated = 1;
        }

        // if curr cfg is OK, not check next special cfg
        if (0 != u8IsUpdated)
            break;
    }

    if (IS_H264(vcenc_instance->codecFormat)) {
        u8IsUpdated = 0;
        for (j = 0; j < pEncIn->gopConfig.special_size; j++) {
            if (pEncIn->gopConfig.pGopPicSpecialCfg[j].i32Interval <= 0)
                continue;

            /* no changed */
            if (((i32)pEncIn->gopConfig.pGopPicSpecialCfg[j].codingType == FRAME_TYPE_RESERVED) &&
                ((i32)pEncIn->gopConfig.pGopPicSpecialCfg[j].numRefPics == NUMREFPICS_RESERVED) &&
                (pEncIn->gopConfig.pGopPicSpecialCfg[j].QpFactor == QPFACTOR_RESERVED) &&
                (pEncIn->gopConfig.pGopPicSpecialCfg[j].QpOffset == QPOFFSET_RESERVED) &&
                (pEncIn->gopConfig.pGopPicSpecialCfg[j].temporalId == TEMPORALID_RESERVED))
                continue;

            /* only consider LTR ref config */
            if (((i32)pEncIn->gopConfig.pGopPicSpecialCfg[j].numRefPics == NUMREFPICS_RESERVED)) {
                /* reserved for later */
                pEncIn->i8SpecialRpsIdx_next = -1;
                u8IsUpdated = 1;
                continue;
            }

            if ((pEncIn->gopConfig.pGopPicSpecialCfg[j].codingType == VCENC_INTRA_FRAME) &&
                (pEncIn->gopConfig.pGopPicSpecialCfg[j].numRefPics == 0))
                numRefPics = 1;
            else
                numRefPics = pEncIn->gopConfig.pGopPicSpecialCfg[j].numRefPics;
            true_e bUsedCurrCfg = HANTRO_FALSE;
            for (k = 0; k < numRefPics; k++) {
                u8IsLTR_ref = IS_LONG_TERM_REF_DELTAPOC(
                    pEncIn->gopConfig.pGopPicSpecialCfg[j].refPics[k].ref_pic);
                if ((u8IsLTR_ref == HANTRO_FALSE) &&
                    (0 == pEncIn->gopConfig.pGopPicSpecialCfg[j].i32short_change))
                    continue;
                i32LTRIdx = (u8IsLTR_ref == HANTRO_TRUE)
                                ? LONG_TERM_REF_DELTAPOC2ID(
                                      pEncIn->gopConfig.pGopPicSpecialCfg[j].refPics[k].ref_pic)
                                : 0;

                if ((pEncIn->long_term_ref_pic[i32LTRIdx] == INVALITED_POC) &&
                    (u8IsLTR_ref != HANTRO_FALSE) &&
                    (0 == pEncIn->gopConfig.pGopPicSpecialCfg[j].i32short_change))
                    continue;

                i32Poc = pEncIn->poc + pEncIn->gopConfig.delta_poc_to_next -
                         pEncIn->gopConfig.pGopPicSpecialCfg[j].i32Offset;

                if (i32Poc < 0) {
                    i32Poc += i32MaxpicOrderCntLsb;
                    if (i32Poc > (i32MaxpicOrderCntLsb >> 1))
                        i32Poc = -1;
                }

                if ((i32Poc >= 0) &&
                    (i32Poc % pEncIn->gopConfig.pGopPicSpecialCfg[j].i32Interval == 0)) {
                    bUsedCurrCfg = HANTRO_TRUE;
                } else {
                    bUsedCurrCfg = HANTRO_FALSE;
                    break;
                }
            }

            if (bUsedCurrCfg == HANTRO_TRUE) {
                pEncIn->i8SpecialRpsIdx_next = j;
                u8IsUpdated = 1;
            }

            if (0 != u8IsUpdated)
                break;
        }
    }

    if (pEncIn->gopCurrPicConfig.codingType == VCENC_INTRA_FRAME)
        pEncIn->gopCurrPicConfig.numRefPics = numRefPics_org;

    j = 0;
    for (k = 0; k < (i32)pEncIn->gopCurrPicConfig.numRefPics; k++) {
        if (0 != pEncIn->gopCurrPicConfig.refPics[k].used_by_cur)
            j++;

        u8IsLTR_ref = IS_LONG_TERM_REF_DELTAPOC(pEncIn->gopCurrPicConfig.refPics[k].ref_pic);
        if (u8IsLTR_ref == HANTRO_FALSE)
            continue;
        i32LTRIdx = LONG_TERM_REF_DELTAPOC2ID(pEncIn->gopCurrPicConfig.refPics[k].ref_pic);

        pEncIn->bLTR_used_by_cur[i32LTRIdx] =
            pEncIn->gopCurrPicConfig.refPics[k].used_by_cur ? HANTRO_TRUE : HANTRO_FALSE;
    }

    // check LTRs is same picture or NOT
    for (i = 0; i < pEncIn->gopConfig.ltrcnt; i++) {
        for (i32 jj = 0; jj < i; jj++) {
            if (pEncIn->bLTR_used_by_cur[i] && pEncIn->bLTR_used_by_cur[jj] &&
                (pEncIn->long_term_ref_pic[i] == pEncIn->long_term_ref_pic[jj])) {
                pEncIn->bLTR_used_by_cur[i] = HANTRO_FALSE;
                j--;
            }
        }
    }

    u32 client_type = VCEncGetClientType(vcenc_instance->codecFormat);
    EWLHwConfig_t cfg = EncAsicGetAsicConfig(client_type, vcenc_instance->ctx);
    //if 2-ref-p support, not need to mark mulitiref frame as a B frame.
    if (j > 1 && cfg.maxRefNumList0Minus1 == 0)
        pEncIn->gopCurrPicConfig.codingType = VCENC_BIDIR_PREDICTED_FRAME;
}

/*------------------------------------------------------------------------------

    Function name : StrmEncodeTraceEncInPara
    Description   : Trace EncIn parameters
    Return type   : void
    Argument      : pEncIn - input parameters provided by user
                    vcenc_instance - encoder instance
------------------------------------------------------------------------------*/
void StrmEncodeTraceEncInPara(VCEncIn *pEncIn, struct vcenc_instance *vcenc_instance)
{
    u32 tileId;
    ptr_t inputBus;
    u32 *pOutBuf;
    ptr_t busOutBuf;
    u32 outBufSize;
    APITRACE("VCEncStrmEncode#\n");
    if (pEncIn != NULL) {
        APITRACEPARAM(" %s : %d\n", "timeIncrement", pEncIn->timeIncrement);

        for (tileId = 0; tileId < vcenc_instance->num_tile_columns; tileId++) {
            inputBus = (tileId == 0) ? pEncIn->busLuma : pEncIn->tileExtra[tileId - 1].busLuma;
            APITRACEPARAM_X(" %s : %p\n", "busLuma", inputBus);
            inputBus =
                (tileId == 0) ? pEncIn->busChromaU : pEncIn->tileExtra[tileId - 1].busChromaU;
            APITRACEPARAM_X(" %s : %p\n", "busChromaU", inputBus);
            inputBus =
                (tileId == 0) ? pEncIn->busChromaV : pEncIn->tileExtra[tileId - 1].busChromaV;
            APITRACEPARAM_X(" %s : %p\n", "busChromaV", inputBus);

            pOutBuf = (tileId == 0) ? pEncIn->pOutBuf[0] : pEncIn->tileExtra[tileId - 1].pOutBuf[0];
            APITRACEPARAM_X(" %s : %p\n", "pOutBuf%d", tileId, pOutBuf);
            busOutBuf =
                (tileId == 0) ? pEncIn->busOutBuf[0] : pEncIn->tileExtra[tileId - 1].busOutBuf[0];
            APITRACEPARAM_X(" %s : %p\n", "busOutBuf%d", tileId, busOutBuf);
            outBufSize =
                (tileId == 0) ? pEncIn->outBufSize[0] : pEncIn->tileExtra[tileId - 1].outBufSize[0];
            APITRACEPARAM(" %s : %d\n", "outBufSize%d", tileId, outBufSize);
            if (vcenc_instance->asic.regs.asicCfg.streamBufferChain) {
                pOutBuf =
                    (tileId == 0) ? pEncIn->pOutBuf[1] : pEncIn->tileExtra[tileId - 1].pOutBuf[1];
                APITRACEPARAM_X(" %s : %p\n", "pOutBuf1", pOutBuf);
                busOutBuf = (tileId == 0) ? pEncIn->busOutBuf[1]
                                          : pEncIn->tileExtra[tileId - 1].busOutBuf[1];
                APITRACEPARAM_X(" %s : %p\n", "busOutBuf1", busOutBuf);
                outBufSize = (tileId == 0) ? pEncIn->outBufSize[1]
                                           : pEncIn->tileExtra[tileId - 1].outBufSize[1];
                APITRACEPARAM(" %s : %d\n", "outBufSize1", outBufSize);
            }
        }

        APITRACEPARAM(" %s : %d\n", "codingType", pEncIn->codingType);
        APITRACEPARAM(" %s : %d\n", "poc", pEncIn->poc);
        APITRACEPARAM(" %s : %d\n", "gopSize", pEncIn->gopSize);
        APITRACEPARAM(" %s : %d\n", "gopPicIdx", pEncIn->gopPicIdx);
        APITRACEPARAM_X(" %s : %p\n", "roiMapDeltaQpAddr", pEncIn->roiMapDeltaQpAddr);
    }
}

/*------------------------------------------------------------------------------

    Function name : StrmEncodeCheckPara
    Description   : check illegal condition and return a status
    Return type   : VCEncRet
    Argument      : vcenc_instance - encoder instance
                    pEncIn - input parameters provided by user
                    pEncOut - place where output info is returned
                    asic - asic data
------------------------------------------------------------------------------*/
VCEncRet StrmEncodeCheckPara(struct vcenc_instance *vcenc_instance, VCEncIn *pEncIn,
                             VCEncOut *pEncOut, asicData_s *asic, u32 client_type)
{
    u32 asicId;
    u32 tileId;
    /* Check for illegal inputs. */
    if ((vcenc_instance == NULL) || (pEncIn == NULL) || (pEncOut == NULL)) {
        APITRACEERR("VCEncStrmEncode: ERROR Null argument\n");
        return VCENC_NULL_ARGUMENT;
    }

    /* Check for existing instance. */
    if (vcenc_instance->inst != vcenc_instance) {
        APITRACEERR("VCEncStrmEncode: ERROR Invalid instance\n");
        return VCENC_INSTANCE_ERROR;
    }
    /* Check status, INIT and ERROR not allowed. */
    if ((vcenc_instance->encStatus != VCENCSTAT_START_STREAM) &&
        (vcenc_instance->encStatus != VCENCSTAT_START_FRAME)) {
        APITRACEERR("VCEncStrmEncode: ERROR Invalid status\n");
        return VCENC_INVALID_STATUS;
    }

    /* Check for features not compiled.*/
#ifndef SUPPORT_AXIFE
    if (vcenc_instance->axiFEEnable != 0) {
        APITRACEERR("VCEncStrmEncode: AXIFE not supported at compile time.\n");
        return VCENC_ERROR;
    }
#endif
#ifndef CHECKSUM_CRC_BUILD_SUPPORT
    if (vcenc_instance->hashctx.hash_type != 0) {
        APITRACEERR("VCEncStrmEncode: hash(crc&checksum) not supported at compile time.\n");
        return VCENC_ERROR;
    }
#endif
#ifndef CUTREE_BUILD_SUPPORT
    if (vcenc_instance->cuTreeCtl.lookaheadDepth != 0) {
        APITRACEERR("VCEncStrmEncode: cutree not supported at compile time.\n");
        return VCENC_ERROR;
    }
#endif
#ifndef VCMD_BUILD_SUPPORT
    if (vcenc_instance->asic.regs.bVCMDAvailable != 0) {
        APITRACEERR("VCEncStrmEncode: VCMD not supported at compile time.\n");
        return VCENC_ERROR;
    }
#endif
#ifndef SUPPORT_DEC400
    if (pEncIn->dec400Enable != 1) /**< \brief 1: bypass 2: enable */
    {
        APITRACEERR("VCEncStrmEncode: DEC400 not supported at compile time.\n");
        return VCENC_ERROR;
    }
#endif
#ifndef LOW_LATENCY_BUILD_SUPPORT
    if (vcenc_instance->inputLineBuf.inputLineBufEn != 0) {
        APITRACEERR("VCEncStrmEncode: inputLineBuffer not supported at compile time.\n");
        return VCENC_ERROR;
    }
#endif
#ifndef SEI_BUILD_SUPPORT
    if (vcenc_instance->rateControl.sei.enabled != 0 || pEncIn->externalSEICount > 0) {
        APITRACEERR("VCEncStrmEncode: sei not supported at compile time.\n");
        return VCENC_ERROR;
    }
#endif
#ifndef RATE_CONTROL_BUILD_SUPPORT
    if (vcenc_instance->rateControl.picRc == 1 && vcenc_instance->rateControl.rcMode != 5) {
        APITRACEERR("VCEncStrmEncode: rateControl not supported at compile time.\n");
        return VCENC_ERROR;
    }
#endif
#ifndef INTERNAL_TEST
    if (vcenc_instance->testId != 0) {
        APITRACEERR("VCEncStrmEncode: test id not supported at compile time.\n");
        return VCENC_ERROR;
    }
#endif
#ifndef CUINFO_BUILD_SUPPORT
    if (vcenc_instance->outputCuInfo != 0) {
        APITRACEERR("VCEncStrmEncode: cuinfo not supported at compile time.\n");
        return VCENC_ERROR;
    }
#endif
#ifndef ROI_BUILD_SUPPORT
    if (vcenc_instance->roiMapEnable != 0) {
        APITRACEERR("VCEncStrmEncode: roi not supported at compile time.\n");
        return VCENC_ERROR;
    }
#endif
#ifndef STREAM_CTRL_BUILD_SUPPORT
    if (vcenc_instance->bInputfileList != 0) {
        APITRACEERR("VCEncStrmEncode: stream ctrl not supported at compile time.\n");
        return VCENC_ERROR;
    }
#endif

    asicId = EncAsicGetAsicHWid(client_type, vcenc_instance->ctx);
    /* Check for invalid input values. */
    if ((pEncIn->gopSize > 1) && ((HW_ID_MAJOR_NUMBER(asicId) <= 1) && HW_PRODUCT_H2(asicId))) {
        APITRACEERR("VCEncStrmEncode: ERROR Invalid gopSize\n");
        return VCENC_INVALID_ARGUMENT;
    }

    if (pEncIn->codingType > VCENC_NOTCODED_FRAME) {
        APITRACEERR("VCEncStrmEncode: ERROR Invalid coding type\n");
        return VCENC_INVALID_ARGUMENT;
    }

    /* Check output stream buffers. */
    u32 *pOutBuf;
    ptr_t busOutBuf;
    u32 outBufSize;
    for (tileId = 0; tileId < vcenc_instance->num_tile_columns; tileId++) {
        pOutBuf = (tileId == 0) ? pEncIn->pOutBuf[0] : pEncIn->tileExtra[tileId - 1].pOutBuf[0];
        busOutBuf =
            (tileId == 0) ? pEncIn->busOutBuf[0] : pEncIn->tileExtra[tileId - 1].busOutBuf[0];
        outBufSize =
            (tileId == 0) ? pEncIn->outBufSize[0] : pEncIn->tileExtra[tileId - 1].outBufSize[0];
        if ((busOutBuf == 0) || (pOutBuf == NULL)) {
            APITRACEERR("VCEncStrmEncode: ERROR Invalid output stream buffer\n");
            return VCENC_INVALID_ARGUMENT;
        }

        if ((vcenc_instance->streamMultiSegment.streamMultiSegmentMode == 0) &&
            (outBufSize < VCENC_STREAM_MIN_BUF0_SIZE)) {
            APITRACEERR("VCEncStrmEncode: ERROR Too small output stream buffer\n");
            return VCENC_INVALID_ARGUMENT;
        }
    }

    if (pEncIn->busOutBuf[1] || pEncIn->pOutBuf[1] || pEncIn->outBufSize[1]) {
        if (!asic->regs.asicCfg.streamBufferChain) {
            APITRACEERR("VCEncStrmEncode: ERROR Two stream buffer not supported\n");
            return VCENC_INVALID_ARGUMENT;
        } else if ((pEncIn->busOutBuf[1] == 0) || (pEncIn->pOutBuf[1] == NULL)) {
            APITRACEERR("VCEncStrmEncode: ERROR Invalid output stream buffer1\n");
            return VCENC_INVALID_ARGUMENT;
        } else if (vcenc_instance->streamMultiSegment.streamMultiSegmentMode != 0) {
            APITRACEERR("VCEncStrmEncode:two output buffer not support multi-segment\n");
            return VCENC_INVALID_ARGUMENT;
        } else if (IS_VP9(vcenc_instance->codecFormat) || IS_AV1(vcenc_instance->codecFormat)) {
            APITRACEERR("VCEncStrmEncode: ERROR Two stream buffer not supported by VP9 and AV1\n");
            return VCENC_INVALID_ARGUMENT;
        }
    }

    if (vcenc_instance->streamMultiSegment.streamMultiSegmentMode != 0 &&
        vcenc_instance->parallelCoreNum > 1) {
        APITRACEERR("VCEncStrmEncode: multi-segment not support multi-core\n");
        return VCENC_INVALID_ARGUMENT;
    }

    /* Check GDR. */
    if ((vcenc_instance->gdrEnabled) && (pEncIn->codingType == VCENC_BIDIR_PREDICTED_FRAME)) {
        APITRACEERR("VCEncSetCodingCtrl: ERROR gdr not support B frame\n");
        return VCENC_INVALID_ARGUMENT;
    }
    /* Check limitation for H.264 baseline profile. */
    if (IS_H264(vcenc_instance->codecFormat) && vcenc_instance->profile == 66 &&
        pEncIn->codingType == VCENC_BIDIR_PREDICTED_FRAME) {
        APITRACEERR("VCEncSetCodingCtrl: ERROR Invalid frame type for baseline profile\n");
        return VCENC_INVALID_ARGUMENT;
    }

    switch (vcenc_instance->preProcess.inputFormat) {
    case VCENC_YUV420_PLANAR:
    case VCENC_YVU420_PLANAR:
    case VCENC_YUV420_PLANAR_10BIT_I010:
    case VCENC_YUV420_PLANAR_10BIT_PACKED_PLANAR:
    case VCENC_YUV420_PLANAR_8BIT_TILE_32_32:
        if (!VCENC_BUS_CH_ADDRESS_VALID(pEncIn->busChromaV)) {
            APITRACEERR("VCEncStrmEncode: ERROR Invalid input busChromaV\n");
            return VCENC_INVALID_ARGUMENT;
        }
    /* Fall through. */
    case VCENC_YUV420_SEMIPLANAR:
    case VCENC_YUV420_SEMIPLANAR_VU:
    case VCENC_YUV420_PLANAR_10BIT_P010:
    case VCENC_YUV420_SEMIPLANAR_8BIT_TILE_4_4:
    case VCENC_YUV420_SEMIPLANAR_VU_8BIT_TILE_4_4:
    case VCENC_YUV420_PLANAR_10BIT_P010_TILE_4_4:
    case VCENC_YUV420_SEMIPLANAR_101010:
    case VCENC_YUV420_8BIT_TILE_64_4:
    case VCENC_YUV420_UV_8BIT_TILE_64_4:
    case VCENC_YUV420_10BIT_TILE_32_4:
    case VCENC_YUV420_10BIT_TILE_48_4:
    case VCENC_YUV420_VU_10BIT_TILE_48_4:
    case VCENC_YUV420_8BIT_TILE_128_2:
    case VCENC_YUV420_UV_8BIT_TILE_128_2:
    case VCENC_YUV420_10BIT_TILE_96_2:
    case VCENC_YUV420_VU_10BIT_TILE_96_2:
    case VCENC_YUV420_8BIT_TILE_8_8:
    case VCENC_YUV420_10BIT_TILE_8_8:
    case VCENC_YUV420_UV_8BIT_TILE_64_2:
    case VCENC_YUV420_UV_10BIT_TILE_128_2:
        if (!VCENC_BUS_ADDRESS_VALID(pEncIn->busChromaU)) {
            APITRACEERR("VCEncStrmEncode: ERROR Invalid input busChromaU\n");
            return VCENC_INVALID_ARGUMENT;
        }
    /* Fall through. */
    case VCENC_YUV422_INTERLEAVED_YUYV:
    case VCENC_YUV422_INTERLEAVED_UYVY:
    case VCENC_RGB565:
    case VCENC_BGR565:
    case VCENC_RGB555:
    case VCENC_BGR555:
    case VCENC_RGB444:
    case VCENC_BGR444:
    case VCENC_RGB888:
    case VCENC_BGR888:
    case VCENC_RGB888_24BIT:
    case VCENC_BGR888_24BIT:
    case VCENC_RBG888_24BIT:
    case VCENC_GBR888_24BIT:
    case VCENC_BRG888_24BIT:
    case VCENC_GRB888_24BIT:
    case VCENC_RGB101010:
    case VCENC_BGR101010:
    case VCENC_YUV420_10BIT_PACKED_Y0L2:
    case VCENC_YUV420_PLANAR_8BIT_TILE_16_16_PACKED_4:
        if (!VCENC_BUS_ADDRESS_VALID(pEncIn->busLuma)) {
            APITRACEERR("VCEncStrmEncode: ERROR Invalid input busLuma\n");
            return VCENC_INVALID_ARGUMENT;
        }
        break;
    default:
        APITRACEERR("VCEncStrmEncode: ERROR Invalid input format\n");
        return VCENC_INVALID_ARGUMENT;
    }

    if (vcenc_instance->preProcess.videoStab) {
        if (!VCENC_BUS_ADDRESS_VALID(pEncIn->busLumaStab)) {
            APITRACE("VCEncStrmEncodeExt: ERROR Invalid input busLumaStab\n");
            return VCENC_INVALID_ARGUMENT;
        }
    }

    /* Stride feature only support YUV420SP and YUV422. */
    if ((vcenc_instance->input_alignment > 1) &&
        ((vcenc_instance->preProcess.inputFormat == VCENC_YUV420_PLANAR_10BIT_PACKED_PLANAR) ||
         (vcenc_instance->preProcess.inputFormat == VCENC_YUV420_10BIT_PACKED_Y0L2) ||
         (vcenc_instance->preProcess.inputFormat == VCENC_YUV420_PLANAR_8BIT_TILE_32_32) ||
         (vcenc_instance->preProcess.inputFormat ==
          VCENC_YUV420_PLANAR_8BIT_TILE_16_16_PACKED_4))) {
        APITRACEERR("VCEncStrmEncode: WARNING alignment doesn't support input format\n");
    }
    return VCENC_OK;
}

/*------------------------------------------------------------------------------

    Function name : StrmEncodeGlobalmvConfig
    Description   : configure global MV parameter
    Return type   : void
    Argument      : asic - asic data
                    pic - picture
                    pEncIn - input parameters provided by user
                    vcenc_instance - encoder instance
------------------------------------------------------------------------------*/
void StrmEncodeGlobalmvConfig(asicData_s *asic, struct sw_picture *pic, VCEncIn *pEncIn,
                              struct vcenc_instance *vcenc_instance)
{
#ifdef GLOBAL_MV_ON_SEARCH_RANGE
    int inLoopDSRatio = (vcenc_instance->pass == 2) ? 2 : 1;
    //int inLoopDSRatio = (vcenc_instance->extDSRatio)? (vcenc_instance->extDSRatio + 1) : (vcenc_instance->inLoopDSRatio + 1);
    asic->regs.gmv[0][0] = asic->regs.gmv[0][1] = asic->regs.gmv[1][0] = asic->regs.gmv[1][1] = 0;
    if (pic->sliceInst->type != I_SLICE) {
        asic->regs.gmv[0][0] = (i16)(pEncIn->gmv[0][0] * inLoopDSRatio);
        asic->regs.gmv[0][1] = (i16)(pEncIn->gmv[0][1] * inLoopDSRatio);
    }
    if (pic->sliceInst->type == B_SLICE) {
        asic->regs.gmv[1][0] = (i16)(pEncIn->gmv[1][0] * inLoopDSRatio);
        asic->regs.gmv[1][1] = (i16)(pEncIn->gmv[1][1] * inLoopDSRatio);
    }
    if (vcenc_instance->pass == 2) {
        printf("    gmv[0]=(%d,%d), gmv[1]=(%d,%d)\n", asic->regs.gmv[0][0], asic->regs.gmv[0][1],
               asic->regs.gmv[1][0], asic->regs.gmv[1][1]);
    }
#else
    asic->regs.gmv[0][0] = asic->regs.gmv[0][1] = asic->regs.gmv[1][0] = asic->regs.gmv[1][1] = 0;
    if (pic->sliceInst->type != I_SLICE) {
        asic->regs.gmv[0][0] = pEncIn->gmv[0][0];
        asic->regs.gmv[0][1] = pEncIn->gmv[0][1];
    }
    if (pic->sliceInst->type == B_SLICE) {
        asic->regs.gmv[1][0] = pEncIn->gmv[1][0];
        asic->regs.gmv[1][1] = pEncIn->gmv[1][1];
    }
#endif

    /* Check GMV */
    if (asic->regs.asicCfg.gmvSupport) {
        i16 maxX, maxY;
        getGMVRange(&maxX, &maxY, 0, IS_H264(vcenc_instance->codecFormat),
                    pic->sliceInst->type == B_SLICE);

        if ((asic->regs.gmv[0][0] > maxX) || (asic->regs.gmv[0][0] < -maxX) ||
            (asic->regs.gmv[0][1] > maxY) || (asic->regs.gmv[0][1] < -maxY) ||
            (asic->regs.gmv[1][0] > maxX) || (asic->regs.gmv[1][0] < -maxX) ||
            (asic->regs.gmv[1][1] > maxY) || (asic->regs.gmv[1][1] < -maxY)) {
            asic->regs.gmv[0][0] = CLIP3(-maxX, maxX, asic->regs.gmv[0][0]);
            asic->regs.gmv[0][1] = CLIP3(-maxY, maxY, asic->regs.gmv[0][1]);
            asic->regs.gmv[1][0] = CLIP3(-maxX, maxX, asic->regs.gmv[1][0]);
            asic->regs.gmv[1][1] = CLIP3(-maxY, maxY, asic->regs.gmv[1][1]);
            APITRACEERR("VCEncStrmEncode: Global MV out of valid range\n");
            APIPRINT(1,
                     "VCEncStrmEncode: Clip Global MV to valid range: (%d, %d) for "
                     "list0 and (%d, %d) for list1.\n",
                     asic->regs.gmv[0][0], asic->regs.gmv[0][1], asic->regs.gmv[1][0],
                     asic->regs.gmv[1][1]);
        }

        if (asic->regs.gmv[0][0] || asic->regs.gmv[0][1] || asic->regs.gmv[1][0] ||
            asic->regs.gmv[1][1]) {
            if ((pic->sps->width < 320) || ((pic->sps->width * pic->sps->height) < (320 * 256))) {
                asic->regs.gmv[0][0] = asic->regs.gmv[0][1] = asic->regs.gmv[1][0] =
                    asic->regs.gmv[1][1] = 0;
                APITRACEERR("VCEncStrmEncode: Video size is too small to support Global MV, "
                            "reset Global MV zero\n");
            }
        }
    }
}

/*------------------------------------------------------------------------------

    Function name : StrmEncodeOverlayConfig
    Description   : configure OSD and mosaic parameter
    Return type   : void
    Argument      : asic - asic data
                    pEncIn - input parameters provided by user
                    vcenc_instance - encoder instance
------------------------------------------------------------------------------*/
void StrmEncodeOverlayConfig(asicData_s *asic, VCEncIn *pEncIn,
                             struct vcenc_instance *vcenc_instance)
{
    int i;
    /* overlay regs */
    for (i = 0; i < MAX_OVERLAY_NUM; i++) {
        //Offsets and cropping are handled in software prp, so we don't need to adjust them here
        asic->regs.overlayYAddr[i] = pEncIn->overlayInputYAddr[i];
        asic->regs.overlayUAddr[i] = pEncIn->overlayInputUAddr[i];
        asic->regs.overlayVAddr[i] = pEncIn->overlayInputVAddr[i];
        asic->regs.overlayEnable[i] = (vcenc_instance->pass == 1) ? 0 : pEncIn->overlayEnable[i];
        asic->regs.overlayFormat[i] = vcenc_instance->preProcess.overlayFormat[i];
        asic->regs.overlayAlpha[i] = vcenc_instance->preProcess.overlayAlpha[i];
        asic->regs.overlayXoffset[i] = vcenc_instance->preProcess.overlayXoffset[i];
        asic->regs.overlayYoffset[i] = vcenc_instance->preProcess.overlayYoffset[i];
        asic->regs.overlayWidth[i] = vcenc_instance->preProcess.overlayWidth[i];
        asic->regs.overlayHeight[i] = vcenc_instance->preProcess.overlayHeight[i];
        asic->regs.overlayYStride[i] = vcenc_instance->preProcess.overlayYStride[i];
        asic->regs.overlayUVStride[i] = vcenc_instance->preProcess.overlayUVStride[i];
        asic->regs.overlayBitmapY[i] = vcenc_instance->preProcess.overlayBitmapY[i];
        asic->regs.overlayBitmapU[i] = vcenc_instance->preProcess.overlayBitmapU[i];
        asic->regs.overlayBitmapV[i] = vcenc_instance->preProcess.overlayBitmapV[i];
    }
    if (vcenc_instance->preProcess.overlaySuperTile[0]) {
        //To use 20 bit reg support 4k, times 64 in Cmodel
        asic->regs.overlayYStride[0] = vcenc_instance->preProcess.overlayYStride[0] / 64;
        asic->regs.overlayUVStride[0] = vcenc_instance->preProcess.overlayUVStride[0] / 64;
    }
    asic->regs.overlaySuperTile = vcenc_instance->preProcess.overlaySuperTile[0];
    asic->regs.overlayScaleWidth = vcenc_instance->preProcess.overlayScaleWidth[0];
    asic->regs.overlayScaleHeight = vcenc_instance->preProcess.overlayScaleHeight[0];
    asic->regs.overlayScaleStepW =
        (u16)((double)(vcenc_instance->preProcess.overlayCropWidth[0] << 16) /
              vcenc_instance->preProcess.overlayScaleWidth[0]);
    asic->regs.overlayScaleStepH =
        (u16)((double)(vcenc_instance->preProcess.overlayCropHeight[0] << 16) /
              vcenc_instance->preProcess.overlayScaleHeight[0]);

    /* Mosaic region(reuse OSD registers) */
    for (i = 0; i < MAX_OVERLAY_NUM; i++) {
        /* Reuse OSD registers */
        if (vcenc_instance->preProcess.mosEnable[i]) {
            asic->regs.overlayEnable[i] = (vcenc_instance->pass != 1);
            asic->regs.overlayFormat[i] = 3;
            asic->regs.overlayXoffset[i] = vcenc_instance->preProcess.mosXoffset[i];
            asic->regs.overlayYoffset[i] = vcenc_instance->preProcess.mosYoffset[i];
            asic->regs.overlayWidth[i] = vcenc_instance->preProcess.mosWidth[i];
            asic->regs.overlayHeight[i] = vcenc_instance->preProcess.mosHeight[i];
        }
    }
}

/*------------------------------------------------------------------------------

    Function name : StrmEncodePrefixSei
    Description   : PREFIX sei message
    Return type   : void
    Argument      : vcenc_instance - encoder instance
                    s - sps
                    pEncOut - place where output info is returned
                    pic - picture
                    pEncIn - input parameters provided by user
------------------------------------------------------------------------------*/
void StrmEncodePrefixSei(struct vcenc_instance *vcenc_instance, struct sps *s, VCEncOut *pEncOut,
                         struct sw_picture *pic, VCEncIn *pEncIn)
{
    i32 tmp;

    if (pEncIn->bIsIDR &&
        (IS_HEVC(vcenc_instance->codecFormat) || IS_H264(vcenc_instance->codecFormat)) &&
        (vcenc_instance->Hdr10Display.hdr10_display_enable == (u8)HDR10_CFGED ||
         vcenc_instance->Hdr10LightLevel.hdr10_lightlevel_enable == (u8)HDR10_CFGED))
        VCEncEncodeSeiHdr10(vcenc_instance, pEncOut);

    if (IS_HEVC(vcenc_instance->codecFormat)) {
        sei_s *sei = &vcenc_instance->rateControl.sei;

        if (sei->enabled == ENCHW_YES || sei->userDataEnabled == ENCHW_YES ||
            sei->insertRecoveryPointMessage == ENCHW_YES || pEncIn->externalSEICount > 0) {
            if (sei->activated_sps == 0) {
                tmp = vcenc_instance->stream.byteCnt;
                HevcNalUnitHdr(&vcenc_instance->stream, PREFIX_SEI_NUT, sei->byteStream);
                HevcActiveParameterSetsSei(&vcenc_instance->stream, sei, &s->vui);
                rbsp_trailing_bits(&vcenc_instance->stream);

                sei->nalUnitSize = vcenc_instance->stream.byteCnt;
                printf(" activated_sps sei size=%d\n", vcenc_instance->stream.byteCnt);
                //pEncOut->streamSize+=vcenc_instance->stream.byteCnt;
                VCEncAddNaluSize(pEncOut, vcenc_instance->stream.byteCnt - tmp, 0);
                sei->activated_sps = 1;
                pEncOut->PreDataSize += (vcenc_instance->stream.byteCnt - tmp);
                pEncOut->PreNaluNum++;
            }
            if (sei->enabled == ENCHW_YES) {
                if ((pic->sliceInst->type == I_SLICE) && (sei->hrd == ENCHW_YES)) {
                    tmp = vcenc_instance->stream.byteCnt;
                    HevcNalUnitHdr(&vcenc_instance->stream, PREFIX_SEI_NUT, sei->byteStream);
                    HevcBufferingSei(&vcenc_instance->stream, sei, &s->vui);
                    rbsp_trailing_bits(&vcenc_instance->stream);

                    sei->nalUnitSize = vcenc_instance->stream.byteCnt;
                    printf("BufferingSei sei size=%d\n", vcenc_instance->stream.byteCnt);
                    //pEncOut->streamSize+=vcenc_instance->stream.byteCnt;
                    VCEncAddNaluSize(pEncOut, vcenc_instance->stream.byteCnt - tmp, 0);
                    pEncOut->PreDataSize += (vcenc_instance->stream.byteCnt - tmp);
                    pEncOut->PreNaluNum++;
                }
                tmp = vcenc_instance->stream.byteCnt;
                HevcNalUnitHdr(&vcenc_instance->stream, PREFIX_SEI_NUT, sei->byteStream);
                HevcPicTimingSei(&vcenc_instance->stream, sei, &s->vui);
                rbsp_trailing_bits(&vcenc_instance->stream);

                sei->nalUnitSize = vcenc_instance->stream.byteCnt;
                printf("PicTiming sei size=%d\n", vcenc_instance->stream.byteCnt);
                //pEncOut->streamSize+=vcenc_instance->stream.byteCnt;
                VCEncAddNaluSize(pEncOut, vcenc_instance->stream.byteCnt - tmp, 0);
                pEncOut->PreDataSize += (vcenc_instance->stream.byteCnt - tmp);
                pEncOut->PreNaluNum++;
            }
            if (sei->userDataEnabled == ENCHW_YES) {
                tmp = vcenc_instance->stream.byteCnt;
                HevcNalUnitHdr(&vcenc_instance->stream, PREFIX_SEI_NUT, sei->byteStream);
                HevcUserDataUnregSei(&vcenc_instance->stream, sei);
                rbsp_trailing_bits(&vcenc_instance->stream);

                sei->nalUnitSize = vcenc_instance->stream.byteCnt;
                printf("UserDataUnreg sei size=%d\n", vcenc_instance->stream.byteCnt);
                //pEncOut->streamSize+=vcenc_instance->stream.byteCnt;
                VCEncAddNaluSize(pEncOut, vcenc_instance->stream.byteCnt - tmp, 0);
                pEncOut->PreDataSize += (vcenc_instance->stream.byteCnt - tmp);
                pEncOut->PreNaluNum++;
            }
            if (sei->insertRecoveryPointMessage == ENCHW_YES) {
                tmp = vcenc_instance->stream.byteCnt;
                HevcNalUnitHdr(&vcenc_instance->stream, PREFIX_SEI_NUT, sei->byteStream);
                HevcRecoveryPointSei(&vcenc_instance->stream, sei);
                rbsp_trailing_bits(&vcenc_instance->stream);

                sei->nalUnitSize = vcenc_instance->stream.byteCnt;
                printf("RecoveryPoint sei size=%d\n", vcenc_instance->stream.byteCnt);
                //pEncOut->streamSize+=vcenc_instance->stream.byteCnt;
                VCEncAddNaluSize(pEncOut, vcenc_instance->stream.byteCnt - tmp, 0);
                pEncOut->PreDataSize += (vcenc_instance->stream.byteCnt - tmp);
                pEncOut->PreNaluNum++;
            }
            if (pEncIn->externalSEICount > 0 && pEncIn->pExternalSEI != NULL) {
                for (int k = 0; k < pEncIn->externalSEICount; k++) {
                    /* Skip only explicit SUFFIX_SEI_NUT */
                    if (pEncIn->pExternalSEI[k].nalType == SUFFIX_SEI_NUT)
                        continue;
                    tmp = vcenc_instance->stream.byteCnt;
                    HevcNalUnitHdr(&vcenc_instance->stream, PREFIX_SEI_NUT, ENCHW_YES);

                    u8 type = pEncIn->pExternalSEI[k].payloadType;
                    u8 *content = pEncIn->pExternalSEI[k].pPayloadData;
                    u32 size = pEncIn->pExternalSEI[k].payloadDataSize;
                    HevcExternalSei(&vcenc_instance->stream, type, content, size);

                    rbsp_trailing_bits(&vcenc_instance->stream);
                    sei->nalUnitSize = vcenc_instance->stream.byteCnt;
                    printf("External sei %d, size=%d\n", k, vcenc_instance->stream.byteCnt - tmp);
                    VCEncAddNaluSize(pEncOut, vcenc_instance->stream.byteCnt - tmp, 0);
                    pEncOut->PreDataSize += (vcenc_instance->stream.byteCnt - tmp);
                    pEncOut->PreNaluNum++;
                }
            }
        }
    } else if (IS_H264(vcenc_instance->codecFormat)) {
        sei_s *sei = &vcenc_instance->rateControl.sei;
        if (sei->enabled == ENCHW_YES || sei->userDataEnabled == ENCHW_YES ||
            sei->insertRecoveryPointMessage == ENCHW_YES || pEncIn->externalSEICount > 0) {
            tmp = vcenc_instance->stream.byteCnt;
            H264NalUnitHdr(&vcenc_instance->stream, 0, H264_SEI, sei->byteStream);
            if (sei->enabled == ENCHW_YES) {
                if ((pic->sliceInst->type == I_SLICE) && (sei->hrd == ENCHW_YES)) {
                    H264BufferingSei(&vcenc_instance->stream, sei);
                    printf("H264BufferingSei, ");
                }
                H264PicTimingSei(&vcenc_instance->stream, sei);
                printf("PicTiming, ");
            }
            if (sei->userDataEnabled == ENCHW_YES) {
                H264UserDataUnregSei(&vcenc_instance->stream, sei);
                printf("UserDataUnreg, ");
            }
            if (sei->insertRecoveryPointMessage == ENCHW_YES) {
                H264RecoveryPointSei(&vcenc_instance->stream, sei);
                printf("RecoveryPoint, ");
            }
            if (pEncIn->externalSEICount > 0 && pEncIn->pExternalSEI != NULL) {
                for (int k = 0; k < pEncIn->externalSEICount; k++) {
                    u8 type = pEncIn->pExternalSEI[k].payloadType;
                    u8 *content = pEncIn->pExternalSEI[k].pPayloadData;
                    u32 size = pEncIn->pExternalSEI[k].payloadDataSize;
                    H264ExternalSei(&vcenc_instance->stream, type, content, size);
                    printf("External %d, ", k);
                }
            }
            rbsp_trailing_bits(&vcenc_instance->stream);
            sei->nalUnitSize = vcenc_instance->stream.byteCnt;
            printf("h264 sei total size=%d \n", vcenc_instance->stream.byteCnt);
            VCEncAddNaluSize(pEncOut, vcenc_instance->stream.byteCnt - tmp, 0);
            pEncOut->PreDataSize += (vcenc_instance->stream.byteCnt - tmp);
            pEncOut->PreNaluNum++;
        }
    }
}

/*------------------------------------------------------------------------------

    Function name : StrmEncodeSuffixSei
    Description   : SUFFIX SEI message
    Return type   : void
    Argument      : vcenc_instance - encoder instance
                    pEncIn - input parameters provided by user
                    pEncOut - place where output info is returned
------------------------------------------------------------------------------*/
void StrmEncodeSuffixSei(struct vcenc_instance *vcenc_instance, VCEncIn *pEncIn, VCEncOut *pEncOut)
{
    if (IS_HEVC(vcenc_instance->codecFormat)) {
        u32 tmp;
        sei_s *sei = &vcenc_instance->rateControl.sei;
        if (pEncIn->externalSEICount > 0 && pEncIn->pExternalSEI != NULL) {
            for (int k = 0; k < pEncIn->externalSEICount; k++) {
                if (pEncIn->pExternalSEI[k].nalType != SUFFIX_SEI_NUT)
                    continue;
                u8 type = pEncIn->pExternalSEI[k].payloadType;
                u8 *content = pEncIn->pExternalSEI[k].pPayloadData;
                u32 size = pEncIn->pExternalSEI[k].payloadDataSize;
                if (!(type == 3 || type == 4 || type == 5 || type == 17 || type == 22 ||
                      type == 132 || type == 146)) {
                    printf("Payload type %d not allowed at SUFFIX_SEI_NUT\n", type);
                    ASSERT(0);
                }
                tmp = vcenc_instance->stream.byteCnt;
                HevcNalUnitHdr(&vcenc_instance->stream, SUFFIX_SEI_NUT, ENCHW_YES);
                HevcExternalSei(&vcenc_instance->stream, type, content, size);
                rbsp_trailing_bits(&vcenc_instance->stream);
                sei->nalUnitSize = vcenc_instance->stream.byteCnt;
                printf("External sei %d, size=%d\n", k, vcenc_instance->stream.byteCnt - tmp);
                VCEncAddNaluSize(pEncOut, vcenc_instance->stream.byteCnt - tmp, 0);
                pEncOut->PostDataSize += (vcenc_instance->stream.byteCnt - tmp);
            }
        }
    }
}

/*------------------------------------------------------------------------------

    Function name : StrmEncodeGradualDecoderRefresh
    Description   : configure GDR parameters
    Return type   : void
    Argument      : vcenc_instance - encoder instance
                    asic - asic data
                    pEncIn - input parameters provided by user
                    codingType - picture coding type
                    cfg - ewl HW config
------------------------------------------------------------------------------*/
void StrmEncodeGradualDecoderRefresh(struct vcenc_instance *vcenc_instance, asicData_s *asic,
                                     VCEncIn *pEncIn, VCEncPictureCodingType *codingType,
                                     EWLHwConfig_t cfg)
{
    i32 top_pos, bottom_pos;

    /* to workaround HW GDR qp in None-GDR frame including I/P frames*/
    if (vcenc_instance->gdrEnabled == 1) {
        asic->regs.rcRoiEnable = 0x01;
        asic->regs.roiUpdate = 1;
    }

    if ((vcenc_instance->gdrEnabled == 1) && (vcenc_instance->encStatus == VCENCSTAT_START_FRAME) &&
        (vcenc_instance->gdrFirstIntraFrame == 0)) {
        asic->regs.intraAreaTop = asic->regs.intraAreaLeft = asic->regs.intraAreaBottom =
            asic->regs.intraAreaRight = INVALID_POS;
        asic->regs.roi1Top = asic->regs.roi1Left = asic->regs.roi1Bottom = asic->regs.roi1Right =
            INVALID_POS;
        //asic->regs.roi1DeltaQp = 0;
        asic->regs.roi1Qp = -1;
        if (pEncIn->codingType == VCENC_INTRA_FRAME) {
            //vcenc_instance->gdrStart++ ;
            *codingType = VCENC_PREDICTED_FRAME;
        }
        if (vcenc_instance->gdrStart) {
            if (vcenc_instance->gdrCount == 0)
                vcenc_instance->rateControl.sei.insertRecoveryPointMessage = ENCHW_YES;
            else
                vcenc_instance->rateControl.sei.insertRecoveryPointMessage = ENCHW_NO;
            top_pos = (vcenc_instance->gdrCount / (1 + vcenc_instance->interlaced)) *
                      vcenc_instance->gdrAverageMBRows;
            bottom_pos = 0;
            if (vcenc_instance->gdrMBLeft) {
                /* We need overlay rows between two GDR frame to make sure inter prediction vertical search range is
           within gdr intra area */
                u8 overlap_rows = 1;
                if ((vcenc_instance->gdrCount / (1 + (i32)vcenc_instance->interlaced)) <
                    vcenc_instance->gdrMBLeft) {
                    top_pos += (vcenc_instance->gdrCount / (1 + (i32)vcenc_instance->interlaced));
                    /* Vertical search range is from 8-64. So for HEVC, 1 more ctb always fit.
             But for h264, we need search_range/16 more mbs */
                    if (IS_H264(vcenc_instance->codecFormat)) {
                        overlap_rows = MAX((cfg.meVertSearchRangeH264 + 15) / 16, 1);
                    }
                    bottom_pos += overlap_rows;
                } else {
                    top_pos += vcenc_instance->gdrMBLeft;
                }
            }
            bottom_pos += top_pos + vcenc_instance->gdrAverageMBRows;
            if (bottom_pos > ((i32)vcenc_instance->ctbPerCol - 1)) {
                bottom_pos = vcenc_instance->ctbPerCol - 1;
            }

            asic->regs.intraAreaTop = top_pos;
            asic->regs.intraAreaLeft = 0;
            asic->regs.intraAreaBottom = bottom_pos;
            asic->regs.intraAreaRight = vcenc_instance->ctbPerRow - 1;

            //to make video quality in intra area is close to inter area.
            asic->regs.roi1Top = top_pos;
            asic->regs.roi1Left = 0;
            asic->regs.roi1Bottom = bottom_pos;
            asic->regs.roi1Right = vcenc_instance->ctbPerRow - 1;

            /*
          roi1DeltaQp from user setting, or if user not provide roi1DeltaQp, use default roi1DeltaQp=3
      */
            if (!asic->regs.roi1DeltaQp)
                asic->regs.roi1DeltaQp = 3;

            asic->regs.rcRoiEnable = 0x01;
        }
        asic->regs.roiUpdate = 1; /* ROI has changed from previous frame. */
    }
}

/*------------------------------------------------------------------------------

    Function name : StrmEncodeRegionOfInterest
    Description   : configure ROI parameters
    Return type   : void
    Argument      : vcenc_instance - encoder instance
                    asic - asic data
------------------------------------------------------------------------------*/
void StrmEncodeRegionOfInterest(struct vcenc_instance *vcenc_instance, asicData_s *asic)
{
    asic->regs.offsetSliceQp = 0;
    if (asic->regs.qp >= 35) {
        asic->regs.offsetSliceQp = 35 - asic->regs.qp;
    }
    if (asic->regs.qp <= 15) {
        asic->regs.offsetSliceQp = 15 - asic->regs.qp;
    }

    if ((vcenc_instance->asic.regs.rcRoiEnable & 0x0c) == 0) {
        if (vcenc_instance->asic.regs.rcRoiEnable & 0x3) {
            if (asic->regs.asicCfg.roiAbsQpSupport) {
                i32 minDelta = (i32)asic->regs.qp - 51;

                asic->regs.roi1DeltaQp =
                    CLIP3(minDelta, (i32)asic->regs.qp, asic->regs.roi1DeltaQp);
                asic->regs.roi2DeltaQp =
                    CLIP3(minDelta, (i32)asic->regs.qp, asic->regs.roi2DeltaQp);

                if (asic->regs.roi1Qp >= 0)
                    asic->regs.roi1Qp =
                        CLIP3((i32)asic->regs.qpMin, (i32)asic->regs.qpMax, asic->regs.roi1Qp);

                if (asic->regs.roi2Qp >= 0)
                    asic->regs.roi2Qp =
                        CLIP3((i32)asic->regs.qpMin, (i32)asic->regs.qpMax, asic->regs.roi2Qp);

                if (asic->regs.asicCfg.ROI8Support) {
                    asic->regs.roi3DeltaQp =
                        CLIP3(minDelta, (i32)asic->regs.qp, asic->regs.roi3DeltaQp);
                    asic->regs.roi4DeltaQp =
                        CLIP3(minDelta, (i32)asic->regs.qp, asic->regs.roi4DeltaQp);
                    asic->regs.roi5DeltaQp =
                        CLIP3(minDelta, (i32)asic->regs.qp, asic->regs.roi5DeltaQp);
                    asic->regs.roi6DeltaQp =
                        CLIP3(minDelta, (i32)asic->regs.qp, asic->regs.roi6DeltaQp);
                    asic->regs.roi7DeltaQp =
                        CLIP3(minDelta, (i32)asic->regs.qp, asic->regs.roi7DeltaQp);
                    asic->regs.roi8DeltaQp =
                        CLIP3(minDelta, (i32)asic->regs.qp, asic->regs.roi8DeltaQp);

                    if (asic->regs.roi3Qp >= 0)
                        asic->regs.roi3Qp =
                            CLIP3((i32)asic->regs.qpMin, (i32)asic->regs.qpMax, asic->regs.roi3Qp);

                    if (asic->regs.roi4Qp >= 0)
                        asic->regs.roi4Qp =
                            CLIP3((i32)asic->regs.qpMin, (i32)asic->regs.qpMax, asic->regs.roi4Qp);

                    if (asic->regs.roi5Qp >= 0)
                        asic->regs.roi5Qp =
                            CLIP3((i32)asic->regs.qpMin, (i32)asic->regs.qpMax, asic->regs.roi5Qp);

                    if (asic->regs.roi6Qp >= 0)
                        asic->regs.roi6Qp =
                            CLIP3((i32)asic->regs.qpMin, (i32)asic->regs.qpMax, asic->regs.roi6Qp);

                    if (asic->regs.roi7Qp >= 0)
                        asic->regs.roi7Qp =
                            CLIP3((i32)asic->regs.qpMin, (i32)asic->regs.qpMax, asic->regs.roi7Qp);

                    if (asic->regs.roi8Qp >= 0)
                        asic->regs.roi8Qp =
                            CLIP3((i32)asic->regs.qpMin, (i32)asic->regs.qpMax, asic->regs.roi8Qp);
                }
            } else {
                asic->regs.roi1DeltaQp =
                    CLIP3(0, 15 - asic->regs.offsetSliceQp, asic->regs.roi1DeltaQp);
                asic->regs.roi2DeltaQp =
                    CLIP3(0, 15 - asic->regs.offsetSliceQp, asic->regs.roi2DeltaQp);
            }

            if (((i32)asic->regs.qp - (i32)asic->regs.qpMin) < asic->regs.roi1DeltaQp) {
                asic->regs.roi1DeltaQp = (i32)asic->regs.qp - (i32)asic->regs.qpMin;
            }

            if (((i32)asic->regs.qp - (i32)asic->regs.qpMin) < asic->regs.roi2DeltaQp) {
                asic->regs.roi2DeltaQp = (i32)asic->regs.qp - (i32)asic->regs.qpMin;
            }

            if (asic->regs.asicCfg.ROI8Support) {
                if (((i32)asic->regs.qp - (i32)asic->regs.qpMin) < asic->regs.roi3DeltaQp) {
                    asic->regs.roi3DeltaQp = (i32)asic->regs.qp - (i32)asic->regs.qpMin;
                }

                if (((i32)asic->regs.qp - (i32)asic->regs.qpMin) < asic->regs.roi4DeltaQp) {
                    asic->regs.roi4DeltaQp = (i32)asic->regs.qp - (i32)asic->regs.qpMin;
                }

                if (((i32)asic->regs.qp - (i32)asic->regs.qpMin) < asic->regs.roi5DeltaQp) {
                    asic->regs.roi5DeltaQp = (i32)asic->regs.qp - (i32)asic->regs.qpMin;
                }

                if (((i32)asic->regs.qp - (i32)asic->regs.qpMin) < asic->regs.roi6DeltaQp) {
                    asic->regs.roi6DeltaQp = (i32)asic->regs.qp - (i32)asic->regs.qpMin;
                }

                if (((i32)asic->regs.qp - (i32)asic->regs.qpMin) < asic->regs.roi7DeltaQp) {
                    asic->regs.roi7DeltaQp = (i32)asic->regs.qp - (i32)asic->regs.qpMin;
                }

                if (((i32)asic->regs.qp - (i32)asic->regs.qpMin) < asic->regs.roi8DeltaQp) {
                    asic->regs.roi8DeltaQp = (i32)asic->regs.qp - (i32)asic->regs.qpMin;
                }
            }
        }
    }
}

/*------------------------------------------------------------------------------

    Function name : EncGetSSIM
    Description   : Calculate SSIM
    Return type   : VCEncRet
    Argument      : vcenc_instance - encoder instance
                    pEncOut - place where output info is returned
------------------------------------------------------------------------------*/
VCEncRet EncGetSSIM(struct vcenc_instance *inst, VCEncOut *pEncOut)
{
    if ((inst == NULL) || (pEncOut == NULL))
        return VCENC_ERROR;

    pEncOut->ssim[0] = pEncOut->ssim[1] = pEncOut->ssim[2] = 0.0;

    asicData_s *asic = &inst->asic;
    if ((!asic->regs.asicCfg.ssimSupport) || (!asic->regs.ssim))
        return VCENC_ERROR;

    i64 ssim_numerator_y = (i32)EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror,
                                                        HWIF_ENC_SSIM_Y_NUMERATOR_MSB);
    i64 ssim_numerator_u = (i32)EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror,
                                                        HWIF_ENC_SSIM_U_NUMERATOR_MSB);
    i64 ssim_numerator_v = (i32)EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror,
                                                        HWIF_ENC_SSIM_V_NUMERATOR_MSB);
    u32 ssim_denominator_y =
        EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror, HWIF_ENC_SSIM_Y_DENOMINATOR);
    u32 ssim_denominator_uv =
        EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror, HWIF_ENC_SSIM_UV_DENOMINATOR);

    ssim_numerator_y = ssim_numerator_y << 32;
    ssim_numerator_u = ssim_numerator_u << 32;
    ssim_numerator_v = ssim_numerator_v << 32;
    ssim_numerator_y |=
        EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror, HWIF_ENC_SSIM_Y_NUMERATOR_LSB);
    ssim_numerator_u |=
        EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror, HWIF_ENC_SSIM_U_NUMERATOR_LSB);
    ssim_numerator_v |=
        EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror, HWIF_ENC_SSIM_V_NUMERATOR_LSB);

    CalculateSSIM(inst, pEncOut, ssim_numerator_y, ssim_numerator_u, ssim_numerator_v,
                  ssim_denominator_y, ssim_denominator_uv);

    return VCENC_OK;
}

void CalculateSSIM(struct vcenc_instance *inst, VCEncOut *pEncOut, i64 ssim_numerator_y,
                   i64 ssim_numerator_u, i64 ssim_numerator_v, u32 ssim_denominator_y,
                   u32 ssim_denominator_uv)
{
    const i32 shift_y = (inst->sps->bit_depth_luma_minus8 == 0) ? SSIM_FIX_POINT_FOR_8BIT
                                                                : SSIM_FIX_POINT_FOR_10BIT;
    const i32 shift_uv = (inst->sps->bit_depth_chroma_minus8 == 0) ? SSIM_FIX_POINT_FOR_8BIT
                                                                   : SSIM_FIX_POINT_FOR_10BIT;
    if (ssim_denominator_y)
        pEncOut->ssim[0] = ssim_numerator_y * 1.0 / (1 << shift_y) / ssim_denominator_y;

    if (ssim_denominator_uv) {
        pEncOut->ssim[1] = ssim_numerator_u * 1.0 / (1 << shift_uv) / ssim_denominator_uv;
        pEncOut->ssim[2] = ssim_numerator_v * 1.0 / (1 << shift_uv) / ssim_denominator_uv;
    }

#if 0
  double ssim = pEncOut->ssim[0] * 0.8 + 0.1 * (pEncOut->ssim[1] + pEncOut->ssim[2]);
  printf("    SSIM %.4f SSIM Y %.4f U %.4f V %.4f\n", ssim, pEncOut->ssim[0], pEncOut->ssim[1], pEncOut->ssim[2]);
#endif
}

/*------------------------------------------------------------------------------

    Function name : EncGetPSNR
    Description   : operations about PSNR under several conditions
    Return type   : VCEncRet
    Argument      : vcenc_instance - encoder instance
                    pEncOut - place where output info is returned
------------------------------------------------------------------------------*/
VCEncRet EncGetPSNR(struct vcenc_instance *inst, VCEncOut *pEncOut)
{
    if ((inst == NULL) || (pEncOut == NULL))
        return VCENC_ERROR;

    asicData_s *asic = &inst->asic;

    /* calculate PSNR_Y,
  HW calculate SSE, SW calculate log10f
  In HW, 8 bottom lines in every 64 height unit is skipped to save line buffer
  SW log10f operation will waste too much CPU cycles,
  we suggest not use it, comment it out by default*/
    asic->regs.SSEDivide256 =
        EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror, HWIF_ENC_SSE_DIV_256);
    {
        //pEncOut->psnr_y = (asic->regs.SSEDivide256 == 0)? 999999.0 :
        //                  10.0 * log10f((float)((256 << asic->regs.outputBitWidthLuma) - 1) * ((256 << asic->regs.outputBitWidthLuma) - 1) * asic->regs.picWidth * (asic->regs.picHeight - ((asic->regs.picHeight>>6)<<3))/ (float)(asic->regs.SSEDivide256 * ((256 << asic->regs.outputBitWidthLuma) << asic->regs.outputBitWidthLuma)));
    }

    /*PSNR for DJI*/
    asic->regs.lumSSEDivide256 =
        EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror, HWIF_ENC_LUM_SSE_DIV_256);
    asic->regs.cbSSEDivide64 =
        EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror, HWIF_ENC_CB_SSE_DIV_64);
    asic->regs.crSSEDivide64 =
        EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror, HWIF_ENC_CR_SSE_DIV_64);

    CalculatePSNR(inst, pEncOut, inst->width);

    return VCENC_OK;
}

void CalculatePSNR(struct vcenc_instance *inst, VCEncOut *pEncOut, u32 width)
{
    asicData_s *asic = &inst->asic;

#ifdef DJI
    pEncOut->psnr_y_predeb =
        (asic->regs.lumSSEDivide256 == 0)
            ? 999999.0
            : 10.0 * log10f((float)((256 << asic->regs.outputBitWidthLuma) - 1) *
                            ((256 << asic->regs.outputBitWidthLuma) - 1) * asic->regs.picWidth *
                            asic->regs.picHeight /
                            (float)(asic->regs.lumSSEDivide256 *
                                    ((256 << asic->regs.outputBitWidthLuma)
                                     << asic->regs.outputBitWidthLuma)));
    pEncOut->psnr_cb_predeb =
        (asic->regs.cbSSEDivide64 == 0)
            ? 999999.0
            : 10.0 *
                  log10f((float)((256 << asic->regs.outputBitWidthChroma) - 1) *
                         ((256 << asic->regs.outputBitWidthChroma) - 1) *
                         (asic->regs.picWidth / 2) * (asic->regs.picHeight / 2) /
                         (float)(asic->regs.cbSSEDivide64 * ((64 << asic->regs.outputBitWidthChroma)
                                                             << asic->regs.outputBitWidthChroma)));
    pEncOut->psnr_cr_predeb =
        (asic->regs.crSSEDivide64 == 0)
            ? 999999.0
            : 10.0 *
                  log10f((float)((256 << asic->regs.outputBitWidthChroma) - 1) *
                         ((256 << asic->regs.outputBitWidthChroma) - 1) *
                         (asic->regs.picWidth / 2) * (asic->regs.picHeight / 2) /
                         (float)(asic->regs.crSSEDivide64 * ((64 << asic->regs.outputBitWidthChroma)
                                                             << asic->regs.outputBitWidthChroma)));
#endif

    inst->rateControl.hierarchical_sse[inst->rateControl.hierarchical_bit_allocation_GOP_size - 1]
                                     [inst->rateControl.gopPoc] = asic->regs.SSEDivide256;

    /*calculate accurate PSNR*/
    if (asic->regs.asicCfg.psnrSupport && asic->regs.psnr) {
        pEncOut->psnr[0] = pEncOut->psnr[1] = pEncOut->psnr[2] = 0.0;

        long long lum_sse, cb_sse, cr_sse;
        lum_sse = ((u32)asic->regs.lumSSEDivide256)
                  << 8 << inst->sps->bit_depth_luma_minus8 << inst->sps->bit_depth_luma_minus8;
        cb_sse = ((u32)asic->regs.cbSSEDivide64)
                 << 6 << inst->sps->bit_depth_chroma_minus8 << inst->sps->bit_depth_chroma_minus8;
        cr_sse = ((u32)asic->regs.crSSEDivide64)
                 << 6 << inst->sps->bit_depth_chroma_minus8 << inst->sps->bit_depth_chroma_minus8;

        float lum_mse, cb_mse, cr_mse;
        lum_mse = (float)lum_sse / (float)(width * inst->height);
        cb_mse = ((float)cb_sse / (float)(width * inst->height)) * 4;
        cr_mse = ((float)cr_sse / (float)(width * inst->height)) * 4;

        int lum_max_value, cbcr_max_value;
        lum_max_value = (1 << inst->sps->bit_depth_luma_minus8 << 8) - 1;
        cbcr_max_value = (1 << inst->sps->bit_depth_chroma_minus8 << 8) - 1;

        if (lum_mse == 0.0)
            pEncOut->psnr[0] = 999999.0;
        else
            pEncOut->psnr[0] = 10.0 * log10f(lum_max_value * lum_max_value / lum_mse);

        if (cb_mse == 0.0)
            pEncOut->psnr[1] = 999999.0;
        else
            pEncOut->psnr[1] = 10.0 * log10f(cbcr_max_value * cbcr_max_value / cb_mse);

        if (cr_mse == 0.0)
            pEncOut->psnr[2] = 999999.0;
        else
            pEncOut->psnr[2] = 10.0 * log10f(cbcr_max_value * cbcr_max_value / cr_mse);
    }
}

VCEncRet GenralRefPicMarking(struct vcenc_instance *vcenc_instance, struct container *c,
                             struct rps *r, VCEncPictureCodingType codingType)
{
    VCEncRet ret = VCENC_OK;

    /* For VP9 TMVP, we may need to store a pic which is not for referencing
     Since VP9 TMVP use last coded frame but not reference frame */
    i32 savePrePoc = -1;
#ifdef SUPPORT_VP9
    if (IS_VP9(vcenc_instance->codecFormat)) {
        if (vcenc_instance->enableTMVP && (codingType != VCENC_INTRA_FRAME)) {
            savePrePoc = vcenc_instance->vp9_inst.previousPoc;
        }
    }
#endif

    if (ref_pic_marking(c, r, savePrePoc))
        ret = VCENC_ERROR;

    return ret;
}

VCEncRet TemporalMvpGenConfig(struct vcenc_instance *vcenc_instance, VCEncIn *pEncIn,
                              struct container *c, struct sw_picture *pic,
                              VCEncPictureCodingType codingType)
{
    i32 i;
    asicData_s *asic = &vcenc_instance->asic;
    asic->regs.spsTmvpEnable = vcenc_instance->enableTMVP;

    if (!vcenc_instance->enableTMVP || IS_H264(vcenc_instance->codecFormat)) {
        asic->regs.tmvpMvInfoBase = 0;
        asic->regs.sliceTmvpEnable = 0;
        asic->regs.tmvpRefMvInfoBaseL0 = 0;
        asic->regs.tmvpRefMvInfoBaseL1 = 0;
        asic->regs.writeTMVinfoDDR = 0;
        return VCENC_OK;
    }

    asic->regs.tmvpMvInfoBase = (ptr_t)pic->mvInfoBase;
    asic->regs.sliceTmvpEnable = codingType != VCENC_INTRA_FRAME;
    asic->regs.writeTMVinfoDDR = codingType != VCENC_INTRA_FRAME;

    if (pic->sliceInst->type != I_SLICE) {
        pic->deltaPocL0[0] = pic->rpl[0][0]->poc - pic->poc;
        if (pic->sliceInst->active_l0_cnt > 1)
            pic->deltaPocL0[1] = pic->rpl[0][1]->poc - pic->poc;
        else
            pic->deltaPocL0[1] = 0;
        asic->regs.tmvpRefMvInfoBaseL0 = pic->rpl[0][0]->mvInfoBase;
#ifdef SUPPORT_VP9
        if (IS_VP9(vcenc_instance->codecFormat)) {
            struct sw_picture *previousPic = get_picture(c, vcenc_instance->vp9_inst.previousPoc);
            asic->regs.tmvpRefMvInfoBaseL0 = previousPic->mvInfoBase;
        }
#endif
    }

    if (pic->sliceInst->type == B_SLICE) {
        pic->deltaPocL1[0] = pic->rpl[1][0]->poc - pic->poc;
        if (pic->sliceInst->active_l1_cnt > 1)
            pic->deltaPocL1[1] = pic->rpl[1][1]->poc - pic->poc;
        else
            pic->deltaPocL1[1] = 0;
        asic->regs.tmvpRefMvInfoBaseL1 = pic->rpl[1][0]->mvInfoBase;
    }

    if (pic->sliceInst->type != I_SLICE) {
        asic->regs.rplL0DeltaPocL0[0] = pic->rpl[0][0]->deltaPocL0[0];
        asic->regs.rplL0DeltaPocL0[1] = pic->rpl[0][0]->deltaPocL0[1];
        asic->regs.rplL0DeltaPocL1[0] = pic->rpl[0][0]->deltaPocL1[0];
        asic->regs.rplL0DeltaPocL1[1] = pic->rpl[0][0]->deltaPocL1[1];
    }

    if (pic->sliceInst->type == B_SLICE) {
        asic->regs.rplL1DeltaPocL0[0] = pic->rpl[1][0]->deltaPocL0[0];
        asic->regs.rplL1DeltaPocL0[1] = pic->rpl[1][0]->deltaPocL0[1];
        asic->regs.rplL1DeltaPocL1[0] = pic->rpl[1][0]->deltaPocL1[0];
        asic->regs.rplL1DeltaPocL1[1] = pic->rpl[1][0]->deltaPocL1[1];
    }

    if (!IS_AV1(vcenc_instance->codecFormat) && !IS_VP9(vcenc_instance->codecFormat)) {
        asic->regs.colFrameFromL0 = 1;
        if (codingType == VCENC_BIDIR_PREDICTED_FRAME) {
            /* Use Closest frame as collacated frame */
            asic->regs.colFrameFromL0 =
                (ABS((int)pic->deltaPocL0[0]) < ABS((int)pic->deltaPocL1[0])) ? 1 : 0;

            //Do not use Intra frame
            if (pic->rpl[!asic->regs.colFrameFromL0][0]->sliceInst->type == I_SLICE)
                asic->regs.colFrameFromL0 = !asic->regs.colFrameFromL0;
        }
        /* Use closest ref */
        asic->regs.colFrameRefIdx = 0;

        //If collocated frame is intra, disable tmvp
        if (pic->sliceInst->type != I_SLICE &&
            pic->rpl[!asic->regs.colFrameFromL0][0]->sliceInst->type == I_SLICE)
            asic->regs.sliceTmvpEnable = 0;

        //not-used-as-reference frame does not need to write TMVP info
        if (!pic->reference)
            asic->regs.writeTMVinfoDDR = 0;
    }

#ifdef SUPPORT_AV1
    /* We need to store order hint for av1 */
    if (IS_AV1(vcenc_instance->codecFormat)) {
        for (i = 0; i < SW_REF_FRAMES; i++) {
            if (pic->sliceInst->type == I_SLICE)
                pic->av1OrderHint[i] = 0;
            else {
                pic->av1OrderHint[i] = pic->rpl[0][0]->poc;
                if (pic->sliceInst->type == B_SLICE && i > GOLDEN_FRAME)
                    pic->av1OrderHint[i] = pic->rpl[1][0]->poc;
            }
        }

        if (pic->sliceInst->type != I_SLICE)
            asic->regs.av1LastAltRefOrderHint = pic->rpl[0][0]->av1OrderHint[ALTREF_FRAME];

        /* For AV1, enable controled by allow_ref_frame_mvs */
        asic->regs.sliceTmvpEnable = vcenc_instance->av1_inst.allow_ref_frame_mvs;

        //not-used-as-reference frame does not need to write TMVP info
        if (!pic->reference)
            asic->regs.writeTMVinfoDDR = 0;
        else {
            /* AV1 may need to access TMV info from Intra frame, although it won't use intra CU.
         To avoid clear tmv buffer in software, ask HW to always write to DDR
      */
            asic->regs.writeTMVinfoDDR = 1;
        }
    }
#endif

#ifdef SUPPORT_VP9
    /* For VP9, tmvp is only enabled when last encoded frame is showable
       Or if last frame is intra */
    if (IS_VP9(vcenc_instance->codecFormat)) {
        if (!vcenc_instance->vp9_inst.lastFrameShowed ||
            vcenc_instance->vp9_inst.last_frame_type == VP9_KEY_FRAME)
            asic->regs.sliceTmvpEnable = 0;

        /* For vp9, no need to write out tmv if not show frame */
        asic->regs.writeTMVinfoDDR &= vcenc_instance->vp9_inst.show_frame;
    }
#endif
    return VCENC_OK;
}

/*------------------------------------------------------------------------------
    Function name : VCEncAddNaluSize
    Description   : Adds the size of a NAL unit into NAL size output buffer.

    Return type   : void
    Argument      : pEncOut - encoder output structure
    Argument      : naluSizeBytes - size of the NALU in bytes
------------------------------------------------------------------------------*/
void VCEncAddNaluSize(VCEncOut *pEncOut, u32 naluSizeBytes, u32 tileId)
{
    if (tileId == 0) {
        if (pEncOut->pNaluSizeBuf != NULL) {
            pEncOut->pNaluSizeBuf[pEncOut->numNalus++] = naluSizeBytes;
            pEncOut->pNaluSizeBuf[pEncOut->numNalus] = 0;
        }
    } else {
        if (pEncOut->tileExtra[tileId - 1].pNaluSizeBuf != NULL) {
            pEncOut->tileExtra[tileId - 1].pNaluSizeBuf[pEncOut->tileExtra[tileId - 1].numNalus++] =
                naluSizeBytes;
            pEncOut->tileExtra[tileId - 1].pNaluSizeBuf[pEncOut->tileExtra[tileId - 1].numNalus] =
                0;
        }
    }
}

/*------------------------------------------------------------------------------
    Function name : VCEncCodecPrepareEncode
    Description   : prepare encode for av1/vp9.
    Return type   : VCEncRet
------------------------------------------------------------------------------*/
VCEncRet VCEncCodecPrepareEncode(struct vcenc_instance *vcenc_instance, const VCEncIn *pEncIn,
                                 VCEncOut *pEncOut, VCEncPictureCodingType codingType,
                                 struct sw_picture *pic, struct container *c, u32 *segcounts)
{
#ifdef SUPPORT_VP9
    if (IS_VP9(vcenc_instance->codecFormat))
        return VCEncCodecPrepareEncodeVP9(vcenc_instance, pEncIn, pEncOut, codingType, pic, c,
                                          segcounts);
#endif

#ifdef SUPPORT_AV1
    if (IS_AV1(vcenc_instance->codecFormat))
        return VCEncCodecPrepareEncodeAV1(vcenc_instance, pEncIn, pEncOut, codingType, pic, c);
#endif

    return VCENC_OK;
}

/*------------------------------------------------------------------------------
    Function name : VCEncCodecPostEncodeUpdate
    Description   : post encode update for av1/vp9.
    Return type   : VCEncRet
------------------------------------------------------------------------------*/
VCEncRet VCEncCodecPostEncodeUpdate(struct vcenc_instance *vcenc_instance, const VCEncIn *pEncIn,
                                    VCEncOut *pEncOut, VCEncPictureCodingType codingType,
                                    struct sw_picture *pic)
{
    VCEncRet ret = VCENC_OK;
#ifdef SUPPORT_VP9
    if (IS_VP9(vcenc_instance->codecFormat)) {
        ret = VCEncCodecPostEncodeUpdateVP9(vcenc_instance, pEncIn, pEncOut, codingType, pic);
    }
#endif

#ifdef SUPPORT_AV1
    if (IS_AV1(vcenc_instance->codecFormat)) {
        EWLLinearMem_t FrameCtxAV1 =
            vcenc_instance->asic.internalFrameContext[pic->picture_memory_id];
        EWLSyncMemData(&FrameCtxAV1, 0, FRAME_CONTEXT_LENGTH, DEVICE_TO_HOST);
        ret = VCEncCodecPostEncodeUpdateAV1(vcenc_instance, pEncIn, pEncOut, codingType);
    }
#endif

    return ret;
}

/* Write end-of-stream code */
void EndOfSequence(struct vcenc_instance *vcenc_instance, const VCEncIn *pEncIn, VCEncOut *pEncOut)
{
    if (IS_H264(vcenc_instance->codecFormat))
        H264EndOfSequence(&vcenc_instance->stream, vcenc_instance->asic.regs.streamMode);
    else if (IS_HEVC(vcenc_instance->codecFormat))
        HEVCEndOfSequence(&vcenc_instance->stream, vcenc_instance->asic.regs.streamMode);
#ifdef SUPPORT_AV1
    else if (IS_AV1(vcenc_instance->codecFormat))
        AV1EndOfSequence(vcenc_instance, pEncIn, pEncOut, &vcenc_instance->stream.byteCnt);
#endif
#ifdef SUPPORT_VP9
    else if (IS_VP9(vcenc_instance->codecFormat))
        VP9EndOfSequence(vcenc_instance, pEncIn, pEncOut, &vcenc_instance->stream.byteCnt);
#endif
}

i32 EncInitLookAheadBufCnt(const VCEncConfig *config, i32 lookaheadDepth)
{
    /* calculate lookahead frame buffer count / delay
   * maxLaFrames = lookAheadDepth + maxGopSize/2
   * +-------------------+----------+----------------+-----------------------------------------+
   * | input down-sample | rdoLevel | lookAheadDepth |      delay or buffer number             |
   * +-------------------+----------+----------------+-----------------------------------------+
   * |       0           |    1     |     > 20       | maxLaFrames + maxGopSize + maxGopSize   |
   * |       0           |    1     |    <= 20       | maxLaFrames + maxGopSize + maxGopSize/2 |
   * |       0           |   > 1    |     > 20       | maxLaFrames + maxGopSize + maxGopSize/2 |
   * |       0           |   > 1    |    <= 20       | maxLaFrames + maxGopSize + maxGopSize/4 |
   * |       1           |    1     |     > 20       | maxLaFrames + maxGopSize + maxGopSize/4 |
   * |       1           |    1     |    <= 20       | maxLaFrames + maxGopSize                |
   * |       1           |   > 1    |     > 20       | maxLaFrames + maxGopSize                |
   * |       1           |   > 1    |    <= 20       | maxLaFrames + maxGopSize                |
   * +-------------------+----------+----------------+-----------------------------------------+ */
    i32 lookAheadBufCnt = CUTREE_MAX_LOOKAHEAD_FRAMES(lookaheadDepth, config->gopSize) +
                          MAX_GOP_SIZE_INUSE(config->gopSize);
    i32 extraDelay = MAX_GOP_SIZE_INUSE(config->gopSize);
    /* check by 0 (not 1 in above table) since config->rdoLevel (0~2) = rdoLevel_by_user(1~3) - 1 */
    if (config->rdoLevel > 0)
        extraDelay /= 2;
    if (lookaheadDepth <= 20)
        extraDelay /= 2;
    if (config->inLoopDSRatio)
        extraDelay -= MAX_GOP_SIZE_INUSE(config->gopSize) * 3 / 4;
    extraDelay = MAX(0, extraDelay);
    lookAheadBufCnt += extraDelay;

    return lookAheadBufCnt;
}

/*------------------------------------------------------------------------------
    Function name   : FindNextForceIdr
    Description     : find next idr frame in queue
    Return type     : i32 next picture idr
    Argument        : struct queue *jobQueue                 [in/out]   jobQueue
------------------------------------------------------------------------------*/
i32 FindNextForceIdr(struct queue *jobQueue)
{
    VCEncJob *job = (VCEncJob *)queue_tail(jobQueue);
    i32 idrPicCnt = -1;
    /*get job from job queue*/
    while (NULL != job) {
        if (HANTRO_TRUE == job->encIn.bIsIDR) {
            idrPicCnt = job->encIn.picture_cnt;
            break;
        }
        job = (VCEncJob *)job->next;
    }
    return idrPicCnt;
}

/*------------------------------------------------------------------------------
    Function name   : AGopDecision
    Description     : single pass: adaptive gop
    Return type     : i32   next gop size
    Argument        : struct vcenc_instance *vcenc_instance  [in]           instance
    Argument        : VCEncIn *pEncIn                        [in]
    Argument        : const VCEncOut *pEncOut                [in]
    Argument        : i32 *pNextGopSize                      [out]          next gop size
    Argument        : VCENCAdapGopCtr * agop                 [in/out]       agop information
------------------------------------------------------------------------------*/
i32 AGopDecision(const struct vcenc_instance *vcenc_instance, VCEncIn *pEncIn,
                 const VCEncOut *pEncOut, i32 *pNextGopSize, VCENCAdapGopCtr *agop)
{
    i32 nextGopSize = -1;

    u32 uiIntraCu8Num = pEncOut->cuStatis.intraCu8Num;
    u32 uiSkipCu8Num = pEncOut->cuStatis.skipCu8Num;
    u32 uiPBFrameCost = pEncOut->cuStatis.PBFrame4NRdCost;
    i32 width = vcenc_instance->width;
    i32 height = vcenc_instance->height;
    double dIntraVsInterskip = (double)uiIntraCu8Num / (double)((width / 8) * (height / 8));
    double dSkipVsInterskip = (double)uiSkipCu8Num / (double)((width / 8) * (height / 8));

    agop->gop_frm_num++;
    agop->sum_intra_vs_interskip += dIntraVsInterskip;
    agop->sum_skip_vs_interskip += dSkipVsInterskip;
    agop->sum_costP += (pEncIn->codingType == VCENC_PREDICTED_FRAME) ? uiPBFrameCost : 0;
    agop->sum_costB += (pEncIn->codingType == VCENC_BIDIR_PREDICTED_FRAME) ? uiPBFrameCost : 0;
    agop->sum_intra_vs_interskipP +=
        (pEncIn->codingType == VCENC_PREDICTED_FRAME) ? dIntraVsInterskip : 0;
    agop->sum_intra_vs_interskipB +=
        (pEncIn->codingType == VCENC_BIDIR_PREDICTED_FRAME) ? dIntraVsInterskip : 0;

    if (pEncIn->gopPicIdx ==
        pEncIn->gopSize - 1) //last frame of the current gop. decide the gopsize of next gop.
    {
        dIntraVsInterskip = agop->sum_intra_vs_interskip / agop->gop_frm_num;
        dSkipVsInterskip = agop->sum_skip_vs_interskip / agop->gop_frm_num;
        agop->sum_costB =
            (agop->gop_frm_num > 1) ? (agop->sum_costB / (agop->gop_frm_num - 1)) : 0xFFFFFFF;
        agop->sum_intra_vs_interskipB =
            (agop->gop_frm_num > 1) ? (agop->sum_intra_vs_interskipB / (agop->gop_frm_num - 1))
                                    : 0xFFFFFFF;
        //Enabled adaptive GOP size for large resolution
        if (((width * height) >= (1280 * 720)) ||
            ((MAX_ADAPTIVE_GOP_SIZE > 3) && ((width * height) >= (416 * 240)))) {
            if ((((double)agop->sum_costP / (double)agop->sum_costB) < 1.1) &&
                (dSkipVsInterskip >= 0.95)) {
                agop->last_gopsize = nextGopSize = 1;
            } else if (((double)agop->sum_costP / (double)agop->sum_costB) > 5) {
                nextGopSize = agop->last_gopsize;
            } else {
                if (((agop->sum_intra_vs_interskipP > 0.40) &&
                     (agop->sum_intra_vs_interskipP < 0.70) &&
                     (agop->sum_intra_vs_interskipB < 0.10))) {
                    agop->last_gopsize++;
                    if (agop->last_gopsize == 5 || agop->last_gopsize == 7) {
                        agop->last_gopsize++;
                    }
                    agop->last_gopsize = MIN(agop->last_gopsize, MAX_ADAPTIVE_GOP_SIZE);
                    nextGopSize = agop->last_gopsize;
                } else if (dIntraVsInterskip >= 0.30) {
                    agop->last_gopsize = nextGopSize = 1; //No B
                } else if (dIntraVsInterskip >= 0.20) {
                    agop->last_gopsize = nextGopSize = 2; //One B
                } else if (dIntraVsInterskip >= 0.10) {
                    agop->last_gopsize--;
                    if (agop->last_gopsize == 5 || agop->last_gopsize == 7) {
                        agop->last_gopsize--;
                    }
                    agop->last_gopsize = MAX(agop->last_gopsize, 3);
                    nextGopSize = agop->last_gopsize;
                } else {
                    agop->last_gopsize++;
                    if (agop->last_gopsize == 5 || agop->last_gopsize == 7) {
                        agop->last_gopsize++;
                    }
                    agop->last_gopsize = MIN(agop->last_gopsize, MAX_ADAPTIVE_GOP_SIZE);
                    nextGopSize = agop->last_gopsize;
                }
            }
        } else {
            nextGopSize = 3;
        }
        agop->gop_frm_num = 0;
        agop->sum_intra_vs_interskip = 0;
        agop->sum_skip_vs_interskip = 0;
        agop->sum_costP = 0;
        agop->sum_costB = 0;
        agop->sum_intra_vs_interskipP = 0;
        agop->sum_intra_vs_interskipB = 0;

        nextGopSize = MIN(nextGopSize, MAX_ADAPTIVE_GOP_SIZE);
    }

    if (nextGopSize != -1)
        *pNextGopSize = nextGopSize;

    return nextGopSize;
}

/*------------------------------------------------------------------------------
    Function name   : SavePicCfg
    Description     : single pass: save encIn' picture config
    Return type     : void
    Argument        : const VCEncIn * pEncIn                 [in]
    Argument        : VCEncPicConfig* pPicCfg                [out]        picture config
------------------------------------------------------------------------------*/
void SavePicCfg(const VCEncIn *pEncIn, VCEncPicConfig *pPicCfg)
{
    if (NULL == pEncIn || NULL == pPicCfg)
        return;

    pPicCfg->codingType = pEncIn->codingType;
    pPicCfg->poc = pEncIn->poc;
    pPicCfg->bIsIDR = pEncIn->bIsIDR;
    memcpy(&pPicCfg->gopConfig, &pEncIn->gopConfig, sizeof(pPicCfg->gopConfig));
    pPicCfg->gopSize = pEncIn->gopSize;
    pPicCfg->gopPicIdx = pEncIn->gopPicIdx;
    pPicCfg->picture_cnt = pEncIn->picture_cnt;
    pPicCfg->last_idr_picture_cnt = pEncIn->last_idr_picture_cnt;

    pPicCfg->bIsPeriodUsingLTR = pEncIn->bIsPeriodUsingLTR;
    pPicCfg->bIsPeriodUpdateLTR = pEncIn->bIsPeriodUpdateLTR;
    memcpy(&pPicCfg->gopCurrPicConfig, &pEncIn->gopCurrPicConfig,
           sizeof(pPicCfg->gopCurrPicConfig));
    memcpy(pPicCfg->long_term_ref_pic, pEncIn->long_term_ref_pic,
           sizeof(pPicCfg->long_term_ref_pic));
    memcpy(pPicCfg->bLTR_used_by_cur, pEncIn->bLTR_used_by_cur, sizeof(pPicCfg->bLTR_used_by_cur));
    memcpy(pPicCfg->bLTR_need_update, pEncIn->bLTR_need_update, sizeof(pPicCfg->bLTR_need_update));

    pPicCfg->i8SpecialRpsIdx = pEncIn->i8SpecialRpsIdx;
    pPicCfg->i8SpecialRpsIdx_next = pEncIn->i8SpecialRpsIdx_next;
    pPicCfg->u8IdxEncodedAsLTR = pEncIn->u8IdxEncodedAsLTR;

    return;
}

/*------------------------------------------------------------------------------
    Function name   : SinglePassEnqueueJob
    Description     : single pass: add job to job queue
    Return type     : VCEncRet
    Argument        : struct vcenc_instance *vcenc_instance  [in/out]   instance
    Argument        : const VCEncIn *pEncIn                  [in]       encIn for the job
------------------------------------------------------------------------------*/
VCEncRet SinglePassEnqueueJob(struct vcenc_instance *vcenc_instance, const VCEncIn *pEncIn)
{
    VCEncRet ret = VCENC_ERROR;
    VCEncJob *job = NULL;
    ptr_t *tmpBusLuma, *tmpBusU, *tmpBusV;
    u32 iBuf, tileId;
    ret = GetBufferFromPool(vcenc_instance->jobBufferPool, (void **)&job);
    if (VCENC_OK != ret || !job)
        return ret;

    memset(job, 0, sizeof(VCEncJob));
    memcpy(&job->encIn, pEncIn, sizeof(VCEncIn));

    if (vcenc_instance->num_tile_columns > 1)
        job->encIn.tileExtra = (VCEncInTileExtra *)((u8 *)job + sizeof(VCEncJob));

    for (tileId = 0; tileId < vcenc_instance->num_tile_columns; tileId++) {
        for (iBuf = 0; iBuf < MAX_STRM_BUF_NUM; iBuf++) {
            if (tileId == 0) {
                job->encIn.pOutBuf[iBuf] = pEncIn->pOutBuf[iBuf];
                job->encIn.busOutBuf[iBuf] = pEncIn->busOutBuf[iBuf];
                job->encIn.outBufSize[iBuf] = pEncIn->outBufSize[iBuf];
                job->encIn.cur_out_buffer[iBuf] = pEncIn->cur_out_buffer[iBuf];

                if (iBuf == 0) {
                    job->encIn.busLuma = pEncIn->busLuma;
                    job->encIn.busChromaU = pEncIn->busChromaU;
                    job->encIn.busChromaV = pEncIn->busChromaV;
                }
            } else {
                job->encIn.tileExtra[tileId - 1].pOutBuf[iBuf] =
                    pEncIn->tileExtra[tileId - 1].pOutBuf[iBuf];
                job->encIn.tileExtra[tileId - 1].busOutBuf[iBuf] =
                    pEncIn->tileExtra[tileId - 1].busOutBuf[iBuf];
                job->encIn.tileExtra[tileId - 1].outBufSize[iBuf] =
                    pEncIn->tileExtra[tileId - 1].outBufSize[iBuf];
                job->encIn.tileExtra[tileId - 1].cur_out_buffer[iBuf] =
                    pEncIn->tileExtra[tileId - 1].cur_out_buffer[iBuf];
                if (iBuf == 0) {
                    job->encIn.tileExtra[tileId - 1].busLuma =
                        pEncIn->tileExtra[tileId - 1].busLuma;
                    job->encIn.tileExtra[tileId - 1].busChromaU =
                        pEncIn->tileExtra[tileId - 1].busChromaU;
                    job->encIn.tileExtra[tileId - 1].busChromaV =
                        pEncIn->tileExtra[tileId - 1].busChromaV;
                }
            }
        }
    }

    if (pEncIn->bIsIDR &&
        (vcenc_instance->nextIdrCnt > pEncIn->picture_cnt || vcenc_instance->nextIdrCnt < 0)) {
        // current next idr > picCnt or there is no next idr, reset next idr.
        vcenc_instance->nextIdrCnt = pEncIn->picture_cnt;
    }
    //match frame with parameter
    EncCodingCtrlParam *pEncCodingCtrlParam =
        (EncCodingCtrlParam *)queue_head(&vcenc_instance->codingCtrl.codingCtrlQueue);
    job->pCodingCtrlParam = pEncCodingCtrlParam;
    if (pEncCodingCtrlParam) {
        if (pEncCodingCtrlParam->startPicCnt <
            0) //whether latest parameter is set from current frame
            pEncCodingCtrlParam->startPicCnt = pEncIn->picture_cnt;
        pEncCodingCtrlParam->refCnt++;
    }
    queue_put(&vcenc_instance->jobQueue, (struct node *)job);
    vcenc_instance->enqueueJobNum++;
    return VCENC_OK;
}

/*------------------------------------------------------------------------------
    Function name   : InitAgop
    Description     : single pass: init agop for AGopDecision
    Return type     : void
    Argument        : VCENCAdapGopCtr * agop                 [out]       agop information
------------------------------------------------------------------------------*/
void InitAgop(VCENCAdapGopCtr *agop)
{
    //Adaptive Gop variables
    agop->last_gopsize = MAX_ADAPTIVE_GOP_SIZE;
    agop->gop_frm_num = 0;
    agop->sum_intra_vs_interskip = 0;
    agop->sum_skip_vs_interskip = 0;
    agop->sum_intra_vs_interskipP = 0;
    agop->sum_intra_vs_interskipB = 0;
    agop->sum_costP = 0;
    agop->sum_costB = 0;
}

/*------------------------------------------------------------------------------
    Function name   : SinglePassGetNextJob
    Description     : single pass: get job to be handled according to given picture cnt
    Return type     : VCEncJob *  job to be handled
    Argument        : struct vcenc_instance *vcenc_instance  [in/out]   instance
    Argument        : const i32 picCnt                       [in]       picture cnt
------------------------------------------------------------------------------*/
VCEncJob *SinglePassGetNextJob(struct vcenc_instance *vcenc_instance, const i32 picCnt)
{
    VCEncJob *job = NULL, *prevJob = NULL;
    job = prevJob = (VCEncJob *)queue_tail(&vcenc_instance->jobQueue);
    /*get job from job queue*/
    while (NULL != job) {
        if (picCnt == job->encIn.picture_cnt) {
            queue_remove(&vcenc_instance->jobQueue, (struct node *)job);
            break;
        }
        job = (VCEncJob *)job->next;
    }
    return job;
}

/*------------------------------------------------------------------------------
    Function name   : SetPicCfgToEncIn
    Description     : single pass: set picture config to encIn
    Return type     : void
    Argument        : const VCEncPicConfig* pPicCfg          [in]           picture config
    Argument        : VCEncIn * pEncIn                       [out]
------------------------------------------------------------------------------*/
void SetPicCfgToEncIn(const VCEncPicConfig *pPicCfg, VCEncIn *pEncIn)
{
    if (NULL == pPicCfg || NULL == pEncIn)
        return;

    pEncIn->codingType = pPicCfg->codingType;
    pEncIn->poc = pPicCfg->poc;
    pEncIn->bIsIDR = pPicCfg->bIsIDR;

    //don't reset lastPic to make sure encode can end early if necessary.
    i32 lastPic = pEncIn->gopConfig.lastPic;
    memcpy(&pEncIn->gopConfig, &pPicCfg->gopConfig, sizeof(pEncIn->gopConfig));
    pEncIn->gopConfig.lastPic = lastPic;

    pEncIn->gopSize = pPicCfg->gopSize;
    pEncIn->gopPicIdx = pPicCfg->gopPicIdx;
    pEncIn->picture_cnt = pPicCfg->picture_cnt;
    pEncIn->last_idr_picture_cnt = pPicCfg->last_idr_picture_cnt;

    pEncIn->bIsPeriodUsingLTR = pPicCfg->bIsPeriodUsingLTR;
    pEncIn->bIsPeriodUpdateLTR = pPicCfg->bIsPeriodUpdateLTR;
    memcpy(&pEncIn->gopCurrPicConfig, &pPicCfg->gopCurrPicConfig, sizeof(pEncIn->gopCurrPicConfig));
    memcpy(pEncIn->long_term_ref_pic, pPicCfg->long_term_ref_pic,
           sizeof(pEncIn->long_term_ref_pic));
    memcpy(pEncIn->bLTR_used_by_cur, pPicCfg->bLTR_used_by_cur, sizeof(pEncIn->bLTR_used_by_cur));
    memcpy(pEncIn->bLTR_need_update, pPicCfg->bLTR_need_update, sizeof(pEncIn->bLTR_need_update));

    pEncIn->i8SpecialRpsIdx = pPicCfg->i8SpecialRpsIdx;
    pEncIn->i8SpecialRpsIdx_next = pPicCfg->i8SpecialRpsIdx_next;
    pEncIn->u8IdxEncodedAsLTR = pPicCfg->u8IdxEncodedAsLTR;

    return;
}

/*------------------------------------------------------------------------------
    Function name : FindNextPic
    Description   : get gopSize/rps for next frame
    Return type   : void
    Argument      : inst - encoder instance
------------------------------------------------------------------------------*/
VCEncPictureCodingType FindNextPic(VCEncInst inst, VCEncIn *encIn, i32 nextGopSize,
                                   const u8 *gopCfgOffset, i32 nextIdrCnt)
{
    struct vcenc_instance *vcenc_instance = (struct vcenc_instance *)inst;
    VCEncPictureCodingType nextCodingType;
    int idx, offset, cur_poc, delta_poc_to_next;
    bool bIsCodingTypeChanged;
    int next_gop_size = nextGopSize;
    VCEncGopConfig *gopCfg = (VCEncGopConfig *)(&(encIn->gopConfig));
    i32 *p_picture_cnt = &encIn->picture_cnt;
    i32 last_idr_picture_cnt = encIn->last_idr_picture_cnt;
    int picture_cnt_tmp = *p_picture_cnt;
    i32 i32LastPicPoc;
    i32 idr_interval = 0;

    //update idr interval
    if (nextIdrCnt >= 0)
        idr_interval = nextIdrCnt - last_idr_picture_cnt;
    else
        idr_interval = 0;

    //get current poc within GOP
    if (encIn->codingType == VCENC_INTRA_FRAME && (encIn->poc == 0)) {
        // last is an IDR
        cur_poc = 0;
        encIn->gopPicIdx = 0;
    } else {
        //Update current idx and poc within a GOP
        idx = encIn->gopPicIdx + gopCfgOffset[encIn->gopSize];
        cur_poc = gopCfg->pGopPicCfg[idx].poc;
        encIn->gopPicIdx = (encIn->gopPicIdx + 1) % encIn->gopSize;
        if (encIn->gopPicIdx == 0)
            cur_poc -= encIn->gopSize;
    }

    //a GOP end, to start next GOP
    if (encIn->gopPicIdx == 0)
        offset = gopCfgOffset[next_gop_size];
    else
        offset = gopCfgOffset[encIn->gopSize];

    //get next poc within GOP, and delta_poc
    idx = encIn->gopPicIdx + offset;
    delta_poc_to_next = gopCfg->pGopPicCfg[idx].poc - cur_poc;
    //next picture cnt
    *p_picture_cnt = picture_cnt_tmp + delta_poc_to_next;

    //Handle Tail (seqence end or cut by an I frame)
    {
        //just finished a GOP and will jump to a P frame
        if (encIn->gopPicIdx == 0 && delta_poc_to_next > 1) {
            int gop_end_pic = *p_picture_cnt;
            int gop_shorten = 0, gop_shorten_idr = 0, gop_shorten_tail = 0;

            //cut  by an IDR
            if ((idr_interval) && ((gop_end_pic - last_idr_picture_cnt) >= idr_interval) &&
                !vcenc_instance->gdrDuration)
                gop_shorten_idr = 1 + ((gop_end_pic - last_idr_picture_cnt) - idr_interval);

            //handle sequence tail
            while (((CalNextPic(gopCfg, gop_end_pic--) + gopCfg->firstPic) > gopCfg->lastPic) &&
                   (gop_shorten_tail < next_gop_size - 1))
                gop_shorten_tail++;

            gop_shorten = gop_shorten_idr > gop_shorten_tail ? gop_shorten_idr : gop_shorten_tail;

            if (gop_shorten >= next_gop_size) {
                //for gopsize = 1
                *p_picture_cnt = picture_cnt_tmp + 1 - cur_poc;
            } else if (gop_shorten > 0) {
                //reduce gop size
                const int max_reduced_gop_size = gopCfg->gopLowdelay ? 1 : 4;
                next_gop_size -= gop_shorten;
                if (next_gop_size > max_reduced_gop_size)
                    next_gop_size = max_reduced_gop_size;

                idx = gopCfgOffset[next_gop_size];
                delta_poc_to_next = gopCfg->pGopPicCfg[idx].poc - cur_poc;
                *p_picture_cnt = picture_cnt_tmp + delta_poc_to_next;
            }
        }

        if (encIn->gopPicIdx == 0)
            encIn->gopSize = next_gop_size;

        i32LastPicPoc = encIn->poc;
        encIn->poc += *p_picture_cnt - picture_cnt_tmp;
        bIsCodingTypeChanged = HANTRO_FALSE;
        //next coding type
        bool forceIntra =
            (idr_interval && ((*p_picture_cnt - last_idr_picture_cnt) >= idr_interval));
        if (forceIntra) {
            nextCodingType = VCENC_INTRA_FRAME;
            encIn->bIsIDR = HANTRO_TRUE;
            bIsCodingTypeChanged = HANTRO_TRUE;
        } else {
            encIn->bIsIDR = HANTRO_FALSE;
            idx = encIn->gopPicIdx + gopCfgOffset[encIn->gopSize];
            nextCodingType = gopCfg->pGopPicCfg[idx].codingType;
        }
    }
    gopCfg->id = encIn->gopPicIdx + gopCfgOffset[encIn->gopSize];
    {
        // guess next rps needed for H.264 DPB management (MMO), assume gopSize unchanged.
        // gopSize change only occurs on adaptive GOP or tail GOP (lowdelay = 0).
        // then the next RPS is 1st of default RPS of some gopSize, which only includes the P frame of last GOP
        i32 next_poc = gopCfg->pGopPicCfg[gopCfg->id].poc;
        i32 gopPicIdx = (encIn->gopPicIdx + 1) % encIn->gopSize;
        i32 gopSize = encIn->gopSize;
        if (gopPicIdx == 0) {
            next_poc -= gopSize;
        }
        gopCfg->id_next = gopPicIdx + gopCfgOffset[gopSize];
        gopCfg->delta_poc_to_next = gopCfg->pGopPicCfg[gopCfg->id_next].poc - next_poc;

        if ((gopPicIdx == 0) && (gopCfg->delta_poc_to_next > 1) &&
            (idr_interval && ((encIn->poc + gopCfg->delta_poc_to_next) >= idr_interval))) {
            i32 i32gopsize;
            i32gopsize = idr_interval - encIn->poc - 2;

            if (i32gopsize > 0) {
                int max_reduced_gop_size = gopCfg->gopLowdelay ? 1 : 4;
                if (i32gopsize > max_reduced_gop_size)
                    i32gopsize = max_reduced_gop_size;

                idx = gopCfgOffset[i32gopsize];
                delta_poc_to_next = gopCfg->pGopPicCfg[idx].poc - cur_poc;

                gopCfg->id_next = gopPicIdx + gopCfgOffset[i32gopsize];
                gopCfg->delta_poc_to_next = gopCfg->pGopPicCfg[gopCfg->id_next].poc - next_poc;
            }
        }

        if (vcenc_instance->gdrDuration == 0 && idr_interval &&
            (encIn->poc + gopCfg->delta_poc_to_next) % idr_interval == 0)
            gopCfg->id_next = -1;
    }

    //only can work for fix gop
    if (0 /*vcenc_instance->lookaheadDepth == 0*/) {
        //Handle the first few frames for lowdelay
        if ((nextCodingType != VCENC_INTRA_FRAME)) {
            int i;
            VCEncGopPicConfig *cfg = &(gopCfg->pGopPicCfg[gopCfg->id]);
            for (i = 0; i < cfg->numRefPics; i++) {
                if ((encIn->poc + cfg->refPics[i].ref_pic) < 0) {
                    int curCfgEnd = gopCfgOffset[encIn->gopSize] + encIn->gopSize;
                    int cfgOffset = encIn->poc - 1;
                    if ((curCfgEnd + cfgOffset) > gopCfg->size)
                        cfgOffset = 0;

                    gopCfg->id = curCfgEnd + cfgOffset;
                    nextCodingType = gopCfg->pGopPicCfg[gopCfg->id].codingType;
                    bIsCodingTypeChanged = HANTRO_TRUE;
                    break;
                }
            }
        }
    }

    GenNextPicConfig(encIn, gopCfg->gopCfgOffset, i32LastPicPoc, vcenc_instance);
    if (bIsCodingTypeChanged == HANTRO_FALSE)
        nextCodingType = encIn->gopCurrPicConfig.codingType;
    if (nextCodingType == VCENC_INTRA_FRAME && ((encIn->poc == 0) || (encIn->bIsIDR))) {
        // refresh IDR POC for !GDR
        if (!vcenc_instance->gdrDuration)
            encIn->poc = 0;
        encIn->last_idr_picture_cnt = encIn->picture_cnt;
    }

    encIn->codingType = (encIn->poc == 0) ? VCENC_INTRA_FRAME : nextCodingType;

    return nextCodingType;
}

static VCEncRet vcencCheckRoiCtrl(struct vcenc_instance *pEncInst,
                                  const VCEncCodingCtrl *pCodeParams)
{
    i32 has_roiarea_change;
    // get last coding ctrl parameter set
    EncCodingCtrlParam *pLastCodingCtrlParam =
        (EncCodingCtrlParam *)queue_head(&pEncInst->codingCtrl.codingCtrlQueue);
    VCEncCodingCtrl *pLastCodingCtrl = &pLastCodingCtrlParam->encCodingCtrl;
    if (pLastCodingCtrlParam) {
        has_roiarea_change = (pLastCodingCtrl->roi1Area.enable != pCodeParams->roi1Area.enable) ||
                             (pLastCodingCtrl->roi2Area.enable != pCodeParams->roi2Area.enable) ||
                             (pLastCodingCtrl->roi3Area.enable != pCodeParams->roi3Area.enable) ||
                             (pLastCodingCtrl->roi4Area.enable != pCodeParams->roi4Area.enable) ||
                             (pLastCodingCtrl->roi5Area.enable != pCodeParams->roi5Area.enable) ||
                             (pLastCodingCtrl->roi6Area.enable != pCodeParams->roi6Area.enable) ||
                             (pLastCodingCtrl->roi7Area.enable != pCodeParams->roi7Area.enable) ||
                             (pLastCodingCtrl->roi8Area.enable != pCodeParams->roi8Area.enable);
    } else {
        has_roiarea_change = (pEncInst->roi1Enable != pCodeParams->roi1Area.enable) ||
                             (pEncInst->roi2Enable != pCodeParams->roi2Area.enable) ||
                             (pEncInst->roi3Enable != pCodeParams->roi3Area.enable) ||
                             (pEncInst->roi4Enable != pCodeParams->roi4Area.enable) ||
                             (pEncInst->roi5Enable != pCodeParams->roi5Area.enable) ||
                             (pEncInst->roi6Enable != pCodeParams->roi6Area.enable) ||
                             (pEncInst->roi7Enable != pCodeParams->roi7Area.enable) ||
                             (pEncInst->roi8Enable != pCodeParams->roi8Area.enable);
    }

    if ((pEncInst->encStatus >= VCENCSTAT_START_STREAM) && (has_roiarea_change)) {
        struct container *c = get_container(pEncInst);
        struct pps *p = (struct pps *)get_parameter_set(c, PPS_NUT, pEncInst->pps_id);

        if (p->cu_qp_delta_enabled_flag == 0) {
            return VCENC_INVALID_ARGUMENT;
        }
    }

    return VCENC_OK;
}

static void VCEncHEVCDnfSetParameters(struct vcenc_instance *inst,
                                      const VCEncCodingCtrl *pCodeParams)
{
    regValues_s *regs = &inst->asic.regs;

    //wiener denoise parameter set
    regs->noiseReductionEnable = inst->uiNoiseReductionEnable =
        pCodeParams->noiseReductionEnable;  //0: disable noise reduction; 1: enable noise reduction
    regs->noiseLow = pCodeParams->noiseLow; //0: use default value; valid value range: [1, 30]

#if USE_TOP_CTRL_DENOISE
    inst->iNoiseL = pCodeParams->noiseLow << FIX_POINT_BIT_WIDTH;
    regs->nrSigmaCur = inst->iSigmaCur = inst->iFirstFrameSigma = pCodeParams->firstFrameSigma
                                                                  << FIX_POINT_BIT_WIDTH;
    //printf("init seq noiseL = %d, SigCur = %d, first = %d\n\n\n\n\n",pEncInst->iNoiseL, regs->nrSigmaCur,  pCodeParams->firstFrameSigma);
#endif
}

void TileInfoConfig(struct vcenc_instance *vcenc_instance, struct sw_picture *pic, u32 tileId,
                    VCEncPictureCodingType codingType, VCEncOut *pEncOut)
{
    u32 leftTileWidth;
    u32 luma_off, chroma_off;
    asicData_s *asic = &vcenc_instance->asic;

    /* basic tile info */
    vcenc_instance->asic.regs.tileLeftStart = vcenc_instance->tileCtrl[tileId].tileLeft;
    vcenc_instance->asic.regs.tileWidthIn8 =
        MIN((vcenc_instance->tileCtrl[tileId].tileRight + 1 -
             vcenc_instance->tileCtrl[tileId].tileLeft) *
                (vcenc_instance->max_cu_size / 8),
            vcenc_instance->asic.regs.picWidth / 8 -
                vcenc_instance->tileCtrl[tileId].tileLeft * (vcenc_instance->max_cu_size / 8));
    vcenc_instance->asic.regs.startTileIdx = vcenc_instance->tileCtrl[tileId].startTileIdx;

    /* output buffer. tile0 stream address is already configured considering some header size before */
    if (vcenc_instance->pass != 1 && tileId > 0) {
        vcenc_instance->asic.regs.outputStrmBase[0] = vcenc_instance->tileCtrl[tileId].busOutBuf;
        vcenc_instance->asic.regs.outputStrmSize[0] = vcenc_instance->tileCtrl[tileId].outBufSize;
    }

    /* input YUV buffer */
    vcenc_instance->asic.regs.input_luma_stride =
        vcenc_instance->tileCtrl[tileId].input_luma_stride;
    vcenc_instance->asic.regs.input_chroma_stride =
        vcenc_instance->tileCtrl[tileId].input_chroma_stride;
    vcenc_instance->asic.regs.inputLumBase = vcenc_instance->tileCtrl[tileId].inputLumBase;
    vcenc_instance->asic.regs.inputCbBase = vcenc_instance->tileCtrl[tileId].inputCbBase;
    vcenc_instance->asic.regs.inputCrBase = vcenc_instance->tileCtrl[tileId].inputCrBase;
    vcenc_instance->asic.regs.inputChromaBaseOffset =
        vcenc_instance->tileCtrl[tileId].inputChromaBaseOffset;
    vcenc_instance->asic.regs.inputLumaBaseOffset =
        vcenc_instance->tileCtrl[tileId].inputLumaBaseOffset;
    vcenc_instance->asic.regs.stabNextLumaBase = vcenc_instance->tileCtrl[tileId].stabNextLumaBase;

    /* NAL size table */
    vcenc_instance->asic.regs.sizeTblBase =
        (vcenc_instance->asic.regs.tiles_enabled_flag)
            ? (vcenc_instance->asic.sizeTbl[tileId].busAddress)
            : (asic->sizeTbl[vcenc_instance->jobCnt % vcenc_instance->parallelCoreNum].busAddress);
    if (tileId == 0)
        vcenc_instance->asic.regs.sizeTblBase += pEncOut->numNalus * sizeof(u32);

    /* deblock+SAO+SAOPP left buffer data for multi-tile */
    if (vcenc_instance->tiles_enabled_flag && tileId > 0) {
        vcenc_instance->asic.regs.tileSyncReadBase = vcenc_instance->asic.regs.tileSyncWriteBase;
        vcenc_instance->asic.regs.mc_sync_l0_addr = vcenc_instance->asic.regs.mc_sync_rec_addr;
        if (tileId != vcenc_instance->num_tile_columns - 1) {
            vcenc_instance->asic.regs.tileSyncWriteBase +=
                (DEB_TILE_SYNC_SIZE + SAODEC_TILE_SYNC_SIZE + SAOPP_TILE_SYNC_SIZE) *
                ((vcenc_instance->height + 63) / 64);
            vcenc_instance->asic.regs.mc_sync_rec_addr += 4;
        }
    }

    /* Ctb bits output */
    if (asic->regs.enableOutputCtbBits) {
        asic->regs.ctbBitsDataBase = pic->ctbBitsDataBase;
        /* for multi-tile case, add the address offset with alignment */
        if (vcenc_instance->num_tile_columns > 1) {
            i32 offset = vcenc_instance->ctbPerCol * vcenc_instance->asic.regs.tileLeftStart * 2;
            offset = (offset + vcenc_instance->ref_alignment - 1) / vcenc_instance->ref_alignment *
                     vcenc_instance->ref_alignment;
            asic->regs.ctbBitsDataBase += offset;
        }
    }

    /* ctbRc config */
    asic->regs.targetPicSize = vcenc_instance->rateControl.targetPicSize;
    if (asic->regs.asicCfg.ctbRcVersion &&
        IS_CTBRC_FOR_BITRATE(vcenc_instance->rateControl.ctbRc)) {
        // New ctb rc testing
        vcencRateControl_s *rc = &(vcenc_instance->rateControl);

        enum slice_type idxType = rc->predId < 3 ? rc->predId : B_SLICE;
        if (rc->ctbRateCtrl.models[idxType].started == 0) {
            if ((idxType == B_SLICE) && rc->ctbRateCtrl.models[P_SLICE].started)
                idxType = P_SLICE;
            else if (rc->ctbRateCtrl.models[I_SLICE].started)
                idxType = I_SLICE;
        }

        asic->regs.ctbRcMemAddrCur = rc->ctbRateCtrl.ctbMemCurAddr;
        asic->regs.ctbRcMemAddrPre = rc->ctbRateCtrl.models[idxType].ctbMemPreAddr;
        /* for multi-tile case, add the address offset with alignment */
        if (vcenc_instance->num_tile_columns > 1) {
            i32 offset = vcenc_instance->ctbPerCol * vcenc_instance->asic.regs.tileLeftStart;
            offset = (offset + vcenc_instance->ref_alignment - 1) / vcenc_instance->ref_alignment *
                     vcenc_instance->ref_alignment;
            asic->regs.ctbRcMemAddrCur += offset;
            asic->regs.ctbRcMemAddrPre += offset;
        }

        float f_tolCtbRc =
            (codingType == VCENC_INTRA_FRAME) ? rc->tolCtbRcIntra : rc->tolCtbRcInter;
        const i32 tolScale = 1 << TOL_CTB_RC_FIX_POINT;
        i32 tolCtbRc = (i32)(f_tolCtbRc * tolScale);

        if (tolCtbRc >= 0) {
            /* tolCtbRc >= 0: ctbRc mode 2 is set by user */
            i32 baseSize = rc->virtualBuffer.bitPerPic;
            if ((codingType != VCENC_INTRA_FRAME) && (rc->targetPicSize < baseSize))
                baseSize = rc->targetPicSize;

            i32 minDeltaSize = rcCalculate(baseSize, tolCtbRc, (tolScale + tolCtbRc));
            i32 maxDeltaSize = rcCalculate(baseSize, tolCtbRc, tolScale);

            asic->regs.minPicSize = MAX(0, (rc->targetPicSize - minDeltaSize));
            asic->regs.maxPicSize = rc->targetPicSize + maxDeltaSize;

            asic->regs.minPicSize = MAX(asic->regs.minPicSize, rc->minPicSize);
            if (rc->maxPicSize > 0)
                asic->regs.maxPicSize = MIN(asic->regs.maxPicSize, rc->maxPicSize);
        } else {
            /* ctbRc mode 2 is enabled by RC forcibly for cpb control */
            asic->regs.minPicSize = asic->regs.maxPicSize = 0;

            if (rc->minPicSize > 0)
                asic->regs.minPicSize = rc->minPicSize;

            if (rc->maxPicSize > 0)
                asic->regs.maxPicSize = rc->maxPicSize;
        }

        asic->regs.targetPicSize = MIN(rc->targetPicSize, asic->regs.maxPicSize);
        asic->regs.ctbRcRowFactor = rc->ctbRateCtrl.rowFactor;
        asic->regs.ctbRcQpStep = rc->ctbRateCtrl.qpStep;
        asic->regs.ctbRcMemAddrCur = rc->ctbRateCtrl.ctbMemCurAddr;
        asic->regs.ctbRcModelParamMin = rc->ctbRateCtrl.models[idxType].xMin;
        if (rc->ctbRateCtrl.models[idxType].started == 0) {
            if ((idxType == B_SLICE) && rc->ctbRateCtrl.models[P_SLICE].started)
                idxType = P_SLICE;
            else if (rc->ctbRateCtrl.models[I_SLICE].started)
                idxType = I_SLICE;
        }
        asic->regs.ctbRcModelParam0 = rc->ctbRateCtrl.models[idxType].x0;
        asic->regs.ctbRcModelParam1 = rc->ctbRateCtrl.models[idxType].x1;
        asic->regs.prevPicLumMad = rc->ctbRateCtrl.models[idxType].preFrameMad;
        asic->regs.ctbRcMemAddrPre = rc->ctbRateCtrl.models[idxType].ctbMemPreAddr;
        asic->regs.ctbRcPrevMadValid = rc->ctbRateCtrl.models[idxType].started ? 1 : 0;
        asic->regs.ctbRcDelay = IS_H264(vcenc_instance->codecFormat) ? 5 : 2;
        if (pic->sliceInst->type != I_SLICE) {
            i32 bpBlk = rc->targetPicSize / (rc->picArea >> 6);
            i32 bpTh = 16;
            if (bpBlk >= bpTh)
                asic->regs.ctbRcDelay = IS_H264(vcenc_instance->codecFormat) ? 7 : 3;
        }
        if (vcenc_instance->num_tile_columns > 1) {
            i32 tileWidthInCtb = vcenc_instance->tileCtrl[tileId].tileRight + 1 -
                                 vcenc_instance->tileCtrl[tileId].tileLeft;

            asic->regs.ctbRcRowFactor =
                asic->regs.ctbRcRowFactor * vcenc_instance->ctbPerRow / tileWidthInCtb;
            asic->regs.ctbRcRowFactor = MIN(0xffff, asic->regs.ctbRcRowFactor);
            asic->regs.targetPicSize =
                asic->regs.targetPicSize * tileWidthInCtb / vcenc_instance->ctbPerRow;
            asic->regs.minPicSize =
                asic->regs.minPicSize * tileWidthInCtb / vcenc_instance->ctbPerRow;
            asic->regs.maxPicSize =
                asic->regs.maxPicSize * tileWidthInCtb / vcenc_instance->ctbPerRow;
            asic->regs.prevPicLumMad =
                asic->regs.prevPicLumMad * tileWidthInCtb / vcenc_instance->ctbPerRow;
        }
    }
    /* cuInfo config. only cuinfo_v1 is supported when multi-tile. */
    vcenc_instance->asic.regs.cuInfoTableBase = vcenc_instance->tileCtrl[tileId].cuinfoTableBase;
    vcenc_instance->asic.regs.cuInfoDataBase = vcenc_instance->tileCtrl[tileId].cuinfoDataBase;
}

u32 TileTotalStreamSize(struct vcenc_instance *vcenc_instance)
{
    u32 totalStreamSize = 0, tileId;
    for (tileId = 0; tileId < vcenc_instance->asic.regs.num_tile_columns; tileId++) {
        totalStreamSize = totalStreamSize + vcenc_instance->tileCtrl[tileId].streamSize;
    }
    return totalStreamSize;
}

void TileInfoCollect(struct vcenc_instance *vcenc_instance, u32 tileId, u32 numNalu)
{
    asicData_s *asic = &vcenc_instance->asic;

    vcenc_instance->tileCtrl[tileId].streamSize =
        EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror, HWIF_ENC_OUTPUT_STRM_BUFFER_LIMIT);
    vcenc_instance->tileCtrl[tileId].numNalu = numNalu;

#ifdef CTBRC_STRENGTH
    vcenc_instance->tileCtrl[tileId].sumOfQP =
        EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror, HWIF_ENC_QP_SUM) |
        (EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror, HWIF_ENC_QP_SUM_MSB) << 26);
    vcenc_instance->tileCtrl[tileId].sumOfQPNumber =
        EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror, HWIF_ENC_QP_NUM) |
        (EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror, HWIF_ENC_QP_NUM_MSB) << 20);
    vcenc_instance->tileCtrl[tileId].picComplexity =
        EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror, HWIF_ENC_PIC_COMPLEXITY) |
        (EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror, HWIF_ENC_PIC_COMPLEXITY_MSB)
         << 23);
#endif

    vcenc_instance->tileCtrl[tileId].intraCu8Num =
        EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror, HWIF_ENC_INTRACU8NUM) |
        (EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror, HWIF_ENC_INTRACU8NUM_MSB) << 20);
    vcenc_instance->tileCtrl[tileId].skipCu8Num =
        EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror, HWIF_ENC_SKIPCU8NUM) |
        (EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror, HWIF_ENC_SKIPCU8NUM_MSB) << 20);
    vcenc_instance->tileCtrl[tileId].PBFrame4NRdCost =
        EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror, HWIF_ENC_PBFRAME4NRDCOST);

    vcenc_instance->tileCtrl[tileId].SSEDivide256 =
        EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror, HWIF_ENC_SSE_DIV_256);
    vcenc_instance->tileCtrl[tileId].lumSSEDivide256 =
        EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror, HWIF_ENC_LUM_SSE_DIV_256);
    vcenc_instance->tileCtrl[tileId].cbSSEDivide64 =
        EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror, HWIF_ENC_CB_SSE_DIV_64);
    vcenc_instance->tileCtrl[tileId].crSSEDivide64 =
        EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror, HWIF_ENC_CR_SSE_DIV_64);

    vcenc_instance->tileCtrl[tileId].ssim_numerator_y = (i32)EncAsicGetRegisterValue(
        asic->ewl, asic->regs.regMirror, HWIF_ENC_SSIM_Y_NUMERATOR_MSB);
    vcenc_instance->tileCtrl[tileId].ssim_numerator_u = (i32)EncAsicGetRegisterValue(
        asic->ewl, asic->regs.regMirror, HWIF_ENC_SSIM_U_NUMERATOR_MSB);
    vcenc_instance->tileCtrl[tileId].ssim_numerator_v = (i32)EncAsicGetRegisterValue(
        asic->ewl, asic->regs.regMirror, HWIF_ENC_SSIM_V_NUMERATOR_MSB);
    vcenc_instance->tileCtrl[tileId].ssim_denominator_y =
        EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror, HWIF_ENC_SSIM_Y_DENOMINATOR);
    vcenc_instance->tileCtrl[tileId].ssim_denominator_uv =
        EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror, HWIF_ENC_SSIM_UV_DENOMINATOR);

    vcenc_instance->tileCtrl[tileId].ssim_numerator_y =
        vcenc_instance->tileCtrl[tileId].ssim_numerator_y << 32;
    vcenc_instance->tileCtrl[tileId].ssim_numerator_u =
        vcenc_instance->tileCtrl[tileId].ssim_numerator_u << 32;
    vcenc_instance->tileCtrl[tileId].ssim_numerator_v =
        vcenc_instance->tileCtrl[tileId].ssim_numerator_v << 32;

    vcenc_instance->tileCtrl[tileId].ssim_numerator_y |=
        EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror, HWIF_ENC_SSIM_Y_NUMERATOR_LSB);
    vcenc_instance->tileCtrl[tileId].ssim_numerator_u |=
        EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror, HWIF_ENC_SSIM_U_NUMERATOR_LSB);
    vcenc_instance->tileCtrl[tileId].ssim_numerator_v |=
        EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror, HWIF_ENC_SSIM_V_NUMERATOR_LSB);

    if ((asic->regs.asicCfg.ctbRcVersion) && (asic->regs.rcRoiEnable & 0x08)) {
        vcenc_instance->tileCtrl[tileId].ctbRcX0 =
            EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror, HWIF_ENC_CTB_RC_MODEL_PARAM0);
        vcenc_instance->tileCtrl[tileId].ctbRcX1 =
            EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror, HWIF_ENC_CTB_RC_MODEL_PARAM1);
        vcenc_instance->tileCtrl[tileId].ctbRcFrameMad =
            EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror, HWIF_ENC_PREV_PIC_LUM_MAD);
        vcenc_instance->tileCtrl[tileId].ctbRcQpSum =
            EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror, HWIF_ENC_CTB_QP_SUM_FOR_RC) |
            (EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror,
                                     HWIF_ENC_CTB_QP_SUM_FOR_RC_MSB)
             << 24);
    }
}

u32 FindIndexBywaitCoreJobid(struct vcenc_instance *vcenc_instance, u32 waitCoreJobid)
{
    u32 i;
    for (i = 0; i < vcenc_instance->num_tile_columns; i++) {
        if (waitCoreJobid == vcenc_instance->tileCtrl[i].job_id)
            return i;
    }
    return 0;
}

void FillVCEncout(struct vcenc_instance *vcenc_instance, VCEncOut *pEncOut)
{
    u32 i;
    u32 mulitTileEnable = (vcenc_instance->num_tile_columns > 1);
    /* output the statistics data of cus. */
    pEncOut->cuStatis.intraCu8Num = mulitTileEnable ? 0 : vcenc_instance->asic.regs.intraCu8Num;
    pEncOut->cuStatis.skipCu8Num = mulitTileEnable ? 0 : vcenc_instance->asic.regs.skipCu8Num;
    pEncOut->cuStatis.PBFrame4NRdCost =
        mulitTileEnable ? 0 : vcenc_instance->asic.regs.PBFrame4NRdCost;

    if (mulitTileEnable) {
        vcenc_instance->asic.regs.SSEDivide256 = 0;
        vcenc_instance->asic.regs.lumSSEDivide256 = 0;
        vcenc_instance->asic.regs.cbSSEDivide64 = 0;
        vcenc_instance->asic.regs.crSSEDivide64 = 0;

        i64 ssim_numerator_y = 0;
        i64 ssim_numerator_u = 0;
        i64 ssim_numerator_v = 0;
        u32 ssim_denominator_y = 0;
        u32 ssim_denominator_uv = 0;

        for (i = 0; i < vcenc_instance->num_tile_columns; i++) {
            /* accumulate statistics data of cus when multi tile */
            pEncOut->cuStatis.intraCu8Num += vcenc_instance->tileCtrl[i].intraCu8Num;
            pEncOut->cuStatis.skipCu8Num += vcenc_instance->tileCtrl[i].skipCu8Num;
            pEncOut->cuStatis.PBFrame4NRdCost += vcenc_instance->tileCtrl[i].PBFrame4NRdCost;

            /* accumulate sse when multi-tile*/
            vcenc_instance->asic.regs.SSEDivide256 += vcenc_instance->tileCtrl[i].SSEDivide256;
            vcenc_instance->asic.regs.lumSSEDivide256 +=
                vcenc_instance->tileCtrl[i].lumSSEDivide256;
            vcenc_instance->asic.regs.cbSSEDivide64 += vcenc_instance->tileCtrl[i].cbSSEDivide64;
            vcenc_instance->asic.regs.crSSEDivide64 += vcenc_instance->tileCtrl[i].crSSEDivide64;

            /* accumulate data about ssim when multi-tile*/
            if (vcenc_instance->asic.regs.asicCfg.ssimSupport && vcenc_instance->asic.regs.ssim) {
                ssim_numerator_y += vcenc_instance->tileCtrl[i].ssim_numerator_y;
                ssim_numerator_u += vcenc_instance->tileCtrl[i].ssim_numerator_u;
                ssim_numerator_v += vcenc_instance->tileCtrl[i].ssim_numerator_v;
                ssim_denominator_y += vcenc_instance->tileCtrl[i].ssim_denominator_y;
                ssim_denominator_uv += vcenc_instance->tileCtrl[i].ssim_denominator_uv;
            }
        }
        /* calculate psnr in frame level when multi-tile*/
        CalculatePSNR(vcenc_instance, pEncOut,
                      vcenc_instance->width - (vcenc_instance->num_tile_columns - 1) * 8);

        /* calculate ssim in frame level when multi-tile*/
        if (vcenc_instance->asic.regs.asicCfg.ssimSupport && vcenc_instance->asic.regs.ssim) {
            CalculateSSIM(vcenc_instance, pEncOut, ssim_numerator_y, ssim_numerator_u,
                          ssim_numerator_v, ssim_denominator_y, ssim_denominator_uv);
        }
    }
}

/*------------------------------------------------------------------------------
    Function name : EncMaxSliceSize
    Description   : get the limited slice size when it is greater than the limits.
    Return type   : u32 - limited sliceSize
    Argument      : pEncInst - encoder instance
    Argument      : sliceSize - slice size
------------------------------------------------------------------------------*/
static u32 EncMaxSliceSize(struct vcenc_instance *pEncInst, u32 sliceSize)
{
    u32 sliceSize_temp = sliceSize;
    u32 client_type = VCEncGetClientType(pEncInst->codecFormat);
    EWLHwConfig_t cfg = EncAsicGetAsicConfig(client_type, pEncInst->ctx);

    /* limit slice size*/
    if (sliceSize_temp > ((1 << cfg.encSliceSizeBits) - 1))
        sliceSize_temp = (1 << cfg.encSliceSizeBits) - 1;

    return sliceSize_temp;
}

/*------------------------------------------------------------------------------
    Function name : EncUpdateCodingCtrlParam
    Description   : [pass2/single pass]update coding ctrl parameters matched with current frame into instance
    Return type   : void
    Argument      : inst - encoder instance
    Argument      : pCodingCtrlParam - coding ctrl
    Argumnet      : picCnt - current picture cnt
------------------------------------------------------------------------------*/
void EncUpdateCodingCtrlParam(struct vcenc_instance *pEncInst, EncCodingCtrlParam *pCodingCtrlParam,
                              const i32 picCnt)
{
    // if pCodingCtrlParam==NULL, there's no coding ctrl parameters need to be updated into instance.
    if (!pEncInst || !pCodingCtrlParam)
        return;
    u32 i;
    VCEncCodingCtrl *pCodeParams = &pCodingCtrlParam->encCodingCtrl;
    regValues_s *regs = &pEncInst->asic.regs;
    bool bAdvanceHW = (pEncInst->asic.regs.asicCfg.roiMapVersion == 2) &&
                      ((HW_ID_MAJOR_NUMBER(pEncInst->asic.regs.asicCfg.hw_asic_id) >= 0x82) ||
                       (HW_PRODUCT_VC9000(pEncInst->asic.regs.asicCfg.hw_asic_id)) ||
                       HW_PRODUCT_SYSTEM6010(pEncInst->asic.regs.asicCfg.hw_asic_id) ||
                       (HW_PRODUCT_VC9000LE(pEncInst->asic.regs.asicCfg.hw_asic_id)));

    /*for parameters need to be enforced in encode order,
    be enforce if current frame is the frame when this set of parameters setted,
    otherwise, keep the value in instance*/
    u32 gdrDuration = pEncInst->gdrDuration;
    //whether need to update parameters enforced in encode order
    u32 bUpdateEncOrderEnforceParam =
        (picCnt == pCodingCtrlParam->startPicCnt || VCENCSTAT_INIT == pEncInst->encStatus)
            ? HANTRO_TRUE
            : HANTRO_FALSE;
    //whether need to update parameters enforced in input order
    u32 bUpdateInputOrderEnforceParam =
        (pEncInst->codingCtrl.pCodingCtrlParam != pCodingCtrlParam ||
         VCENCSTAT_INIT == pEncInst->encStatus)
            ? HANTRO_TRUE
            : HANTRO_FALSE;

    if (!bUpdateInputOrderEnforceParam &&
        !bUpdateEncOrderEnforceParam) //don't need to update parameters
        goto end;

    /*update parameters in instance*/
    /*update parameters enforced in input order*/
    if (bUpdateInputOrderEnforceParam) {
        pEncInst->codingCtrl.pCodingCtrlParam = pCodingCtrlParam; //update codingctrl pointer

        pEncInst->featureToSupport.deNoiseEnabled =
            (pCodeParams->noiseReductionEnable == 1) ? 1 : 0;

        regs->meAssignedVertRange = pCodeParams->meVertSearchRange >> 3;

        pEncInst->itu_t_t35 = pCodeParams->itu_t_t35;

        pEncInst->write_once_HDR10 = pCodeParams->write_once_HDR10;
        if (pEncInst->Hdr10Display.hdr10_display_enable == (u8)HDR10_NOCFGED) {
            pEncInst->Hdr10Display = pCodeParams->Hdr10Display;
            pEncInst->Hdr10Display.hdr10_display_enable =
                pCodeParams->Hdr10Display.hdr10_display_enable ? ((u8)HDR10_CFGED)
                                                               : ((u8)HDR10_NOCFGED);
        }

        pEncInst->vuiColorDescription = pCodeParams->vuiColorDescription;

        if (pEncInst->Hdr10LightLevel.hdr10_lightlevel_enable == (u8)HDR10_NOCFGED) {
            pEncInst->Hdr10LightLevel = pCodeParams->Hdr10LightLevel;
            pEncInst->Hdr10LightLevel.hdr10_lightlevel_enable =
                pCodeParams->Hdr10LightLevel.hdr10_lightlevel_enable ? ((u8)HDR10_CFGED)
                                                                     : ((u8)HDR10_NOCFGED);
        }

        pEncInst->vuiVideoSignalTypePresentFlag = pCodeParams->vuiVideoSignalTypePresentFlag;
        pEncInst->vuiVideoFormat = pCodeParams->vuiVideoFormat;

        pEncInst->sarWidth = pCodeParams->sampleAspectRatioWidth;
        pEncInst->sarHeight = pCodeParams->sampleAspectRatioHeight;
        pEncInst->vuiVideoFullRange = pCodeParams->vuiVideoFullRange;

        pEncInst->RpsInSliceHeader =
            IS_H264(pEncInst->codecFormat) ? 0 : pCodeParams->RpsInSliceHeader;

        if (pCodeParams->sliceSize) {
            /* Multi-slice mode is not supported by VCMD, so need to disable (bypass) VCMD */
            EWLSetVCMDMode(pEncInst->asic.ewl, 0);
            regs->bVCMDEnable = EWLGetVCMDMode(pEncInst->asic.ewl) ? true : false;
        }

        regs->sliceSize = (pCodeParams->sliceSize <= MAX_SLICE_SIZE_V0)
                              ? pCodeParams->sliceSize
                              : EncMaxSliceSize(pEncInst, pCodeParams->sliceSize);

        regs->sliceNum = (regs->sliceSize == 0)
                             ? 1
                             : ((pEncInst->ctbPerCol + (regs->sliceSize - 1)) / regs->sliceSize);

        if ((IS_AV1(pEncInst->codecFormat) || IS_VP9(pEncInst->codecFormat)) &&
            regs->sliceNum > 1) {
            APITRACEERR("EncUpdateCodingCtrlParam: WARNING No multi slice support in AV1 or "
                        "VP9\n");
            regs->sliceSize = 0;
            regs->sliceNum = 1;
        }

        /* limit slice num to make it smaller than MAX_SLICE_NUM. If slice size is limited by
     * EncMaxSliceSize() previously, it will not go to following do-while loop due to the
     * corresponding slice num will not be greater than 9, which less than MAX_SLICE_NUM.
     */
        if (regs->sliceNum > MAX_SLICE_NUM) {
            do {
                regs->sliceSize++;
                regs->sliceNum = (pEncInst->ctbPerCol + (regs->sliceSize - 1)) / regs->sliceSize;
            } while (regs->sliceNum > MAX_SLICE_NUM);
        }

        pEncInst->enableScalingList = pCodeParams->enableScalingList;

        if (pEncInst->pass == 0 &&
            (pCodeParams->roiMapDeltaQpEnable || pCodeParams->RoimapCuCtrl_enable)) {
            i32 log2_ctu_size = IS_H264(pEncInst->codecFormat) ? 4 : 6;
            pEncInst->log2_qp_size =
                CLIP3(3, log2_ctu_size, (6 - pCodeParams->roiMapDeltaQpBlockUnit));
        }

        /* Set CIR, intra forcing and ROI parameters */
        regs->cirStart = pCodeParams->cirStart;
        regs->cirInterval = pCodeParams->cirInterval;

        pEncInst->pcm_enabled_flag =
            pCodeParams->pcm_enabled_flag || (pCodeParams->RoimapCuCtrl_ver > 3);
        regs->ipcmFilterDisable = pEncInst->pcm_loop_filter_disabled_flag =
            pCodeParams->pcm_loop_filter_disabled_flag;
        regs->ipcmMapEnable = pCodeParams->ipcmMapEnable;

        if (pCodeParams->roi2Area.enable) {
            regs->roi2Top = pCodeParams->roi2Area.top;
            regs->roi2Left = pCodeParams->roi2Area.left;
            regs->roi2Bottom = pCodeParams->roi2Area.bottom;
            regs->roi2Right = pCodeParams->roi2Area.right;
            pEncInst->roi2Enable = 1;
        } else {
            regs->roi2Top = regs->roi2Left = regs->roi2Bottom = regs->roi2Right = INVALID_POS;
            pEncInst->roi2Enable = 0;
        }
        if (pCodeParams->roi3Area.enable) {
            regs->roi3Top = pCodeParams->roi3Area.top;
            regs->roi3Left = pCodeParams->roi3Area.left;
            regs->roi3Bottom = pCodeParams->roi3Area.bottom;
            regs->roi3Right = pCodeParams->roi3Area.right;
            pEncInst->roi3Enable = 1;
        } else {
            regs->roi3Top = regs->roi3Left = regs->roi3Bottom = regs->roi3Right = INVALID_POS;
            pEncInst->roi3Enable = 0;
        }
        if (pCodeParams->roi4Area.enable) {
            regs->roi4Top = pCodeParams->roi4Area.top;
            regs->roi4Left = pCodeParams->roi4Area.left;
            regs->roi4Bottom = pCodeParams->roi4Area.bottom;
            regs->roi4Right = pCodeParams->roi4Area.right;
            pEncInst->roi4Enable = 1;
        } else {
            regs->roi4Top = regs->roi4Left = regs->roi4Bottom = regs->roi4Right = INVALID_POS;
            pEncInst->roi4Enable = 0;
        }
        if (pCodeParams->roi5Area.enable) {
            regs->roi5Top = pCodeParams->roi5Area.top;
            regs->roi5Left = pCodeParams->roi5Area.left;
            regs->roi5Bottom = pCodeParams->roi5Area.bottom;
            regs->roi5Right = pCodeParams->roi5Area.right;
            pEncInst->roi5Enable = 1;
        } else {
            regs->roi5Top = regs->roi5Left = regs->roi5Bottom = regs->roi5Right = INVALID_POS;
            pEncInst->roi5Enable = 0;
        }
        if (pCodeParams->roi6Area.enable) {
            regs->roi6Top = pCodeParams->roi6Area.top;
            regs->roi6Left = pCodeParams->roi6Area.left;
            regs->roi6Bottom = pCodeParams->roi6Area.bottom;
            regs->roi6Right = pCodeParams->roi6Area.right;
            pEncInst->roi6Enable = 1;
        } else {
            regs->roi6Top = regs->roi6Left = regs->roi6Bottom = regs->roi6Right = INVALID_POS;
            pEncInst->roi6Enable = 0;
        }
        if (pCodeParams->roi7Area.enable) {
            regs->roi7Top = pCodeParams->roi7Area.top;
            regs->roi7Left = pCodeParams->roi7Area.left;
            regs->roi7Bottom = pCodeParams->roi7Area.bottom;
            regs->roi7Right = pCodeParams->roi7Area.right;
            pEncInst->roi7Enable = 1;
        } else {
            regs->roi7Top = regs->roi7Left = regs->roi7Bottom = regs->roi7Right = INVALID_POS;
            pEncInst->roi7Enable = 0;
        }
        if (pCodeParams->roi8Area.enable) {
            regs->roi8Top = pCodeParams->roi8Area.top;
            regs->roi8Left = pCodeParams->roi8Area.left;
            regs->roi8Bottom = pCodeParams->roi8Area.bottom;
            regs->roi8Right = pCodeParams->roi8Area.right;
            pEncInst->roi8Enable = 1;
        } else {
            regs->roi8Top = regs->roi8Left = regs->roi8Bottom = regs->roi8Right = INVALID_POS;
            pEncInst->roi8Enable = 0;
        }

        regs->roi1DeltaQp = -pCodeParams->roi1DeltaQp;
        regs->roi2DeltaQp = -pCodeParams->roi2DeltaQp;
        regs->roi1Qp = pCodeParams->roi1Qp;
        regs->roi2Qp = pCodeParams->roi2Qp;
        regs->roi3DeltaQp = -pCodeParams->roi3DeltaQp;
        regs->roi4DeltaQp = -pCodeParams->roi4DeltaQp;
        regs->roi5DeltaQp = -pCodeParams->roi5DeltaQp;
        regs->roi6DeltaQp = -pCodeParams->roi6DeltaQp;
        regs->roi7DeltaQp = -pCodeParams->roi7DeltaQp;
        regs->roi8DeltaQp = -pCodeParams->roi8DeltaQp;
        regs->roi3Qp = pCodeParams->roi3Qp;
        regs->roi4Qp = pCodeParams->roi4Qp;
        regs->roi5Qp = pCodeParams->roi5Qp;
        regs->roi6Qp = pCodeParams->roi6Qp;
        regs->roi7Qp = pCodeParams->roi7Qp;
        regs->roi8Qp = pCodeParams->roi8Qp;

        regs->roiUpdate = 1; /* ROI has changed from previous frame. */

        pEncInst->roiMapEnable = 0;
        if (pEncInst->asic.regs.asicCfg.roiMapVersion >= 3) {
            pEncInst->RoimapCuCtrl_index_enable = pCodeParams->RoimapCuCtrl_index_enable;
            pEncInst->RoimapCuCtrl_enable = pCodeParams->RoimapCuCtrl_enable;

            pEncInst->RoimapCuCtrl_ver = pCodeParams->RoimapCuCtrl_ver;
            pEncInst->RoiQpDelta_ver = pCodeParams->RoiQpDelta_ver;
        }
        if (bAdvanceHW)
            pEncInst->RoiQpDelta_ver = pCodeParams->RoiQpDelta_ver;

        if (pEncInst->pass == 2 &&
            !((pEncInst->asic.regs.asicCfg.roiMapVersion == 4) && (pEncInst->RoiQpDelta_ver == 4)))
            pEncInst->RoiQpDelta_ver = 1;

#ifndef CTBRC_STRENGTH
        if (pCodeParams->roiMapDeltaQpEnable == 1) {
            pEncInst->roiMapEnable = 1;
            regs->rcRoiEnable = RCROIMODE_ROIMAP_ENABLE;
        } else if (pCodeParams->roi1Area.enable || pCodeParams->roi2Area.enable)
            regs->rcRoiEnable = RCROIMODE_ROIAREA_ENABLE;
        else
            regs->rcRoiEnable = RCROIMODE_RC_ROIMAP_ROIAREA_DIABLE;
#else
        /* rdoqSkipOnlyMapEnableFlag just for 2pass when only rdoq or skip , no Qp */
        true_e rdoqSkipOnlyMapEnableFlag =
            (pEncInst->pass != 0) && (pCodeParams->skipMapEnable || pCodeParams->rdoqMapEnable) &&
            (pEncInst->asic.regs.asicCfg.roiMapVersion == 4) && (pEncInst->RoiQpDelta_ver == 4);
        /*rcRoiEnable : bit2: rc control bit, bit1: roi map control bit, bit 0: roi area control bit*/
        if (((pCodeParams->roiMapDeltaQpEnable == 1) || rdoqSkipOnlyMapEnableFlag ||
             pCodeParams->RoimapCuCtrl_enable) &&
            (pCodeParams->roi1Area.enable == 0 && pCodeParams->roi2Area.enable == 0 &&
             pCodeParams->roi3Area.enable == 0 && pCodeParams->roi4Area.enable == 0 &&
             pCodeParams->roi5Area.enable == 0 && pCodeParams->roi6Area.enable == 0 &&
             pCodeParams->roi7Area.enable == 0 && pCodeParams->roi8Area.enable == 0)) {
            if (pCodeParams->roiMapDeltaQpEnable == 1)
                pEncInst->roiMapEnable = 1;
            regs->rcRoiEnable = 0x02;
        } else if ((pCodeParams->roiMapDeltaQpEnable == 0) &&
                   (pCodeParams->RoimapCuCtrl_enable == 0) &&
                   (pCodeParams->roi1Area.enable != 0 || pCodeParams->roi2Area.enable != 0 ||
                    pCodeParams->roi3Area.enable != 0 || pCodeParams->roi4Area.enable != 0 ||
                    pCodeParams->roi5Area.enable != 0 || pCodeParams->roi6Area.enable != 0 ||
                    pCodeParams->roi7Area.enable != 0 || pCodeParams->roi8Area.enable != 0)) {
            regs->rcRoiEnable = 0x01;
        } else if (((pCodeParams->roiMapDeltaQpEnable != 0) || rdoqSkipOnlyMapEnableFlag ||
                    (pCodeParams->RoimapCuCtrl_enable != 0)) &&
                   (pCodeParams->roi1Area.enable != 0 || pCodeParams->roi2Area.enable != 0 ||
                    pCodeParams->roi3Area.enable != 0 || pCodeParams->roi4Area.enable != 0 ||
                    pCodeParams->roi5Area.enable != 0 || pCodeParams->roi6Area.enable != 0 ||
                    pCodeParams->roi7Area.enable != 0 || pCodeParams->roi8Area.enable != 0)) {
            if (pCodeParams->roiMapDeltaQpEnable != 0)
                pEncInst->roiMapEnable = 1;
            regs->rcRoiEnable = 0x03;
        } else
            regs->rcRoiEnable = 0x00;
#endif

        regs->skipMapEnable = pCodeParams->skipMapEnable ? 1 : 0;

        /* low latency */
        pEncInst->inputLineBuf.inputLineBufEn = pCodeParams->inputLineBufEn;
        pEncInst->inputLineBuf.inputLineBufLoopBackEn = pCodeParams->inputLineBufLoopBackEn;
        pEncInst->inputLineBuf.inputLineBufDepth = pCodeParams->inputLineBufDepth;
        pEncInst->inputLineBuf.amountPerLoopBack = pCodeParams->amountPerLoopBack;
        pEncInst->inputLineBuf.inputLineBufHwModeEn = pCodeParams->inputLineBufHwModeEn;
        pEncInst->inputLineBuf.cbFunc = pCodeParams->inputLineBufCbFunc;
        pEncInst->inputLineBuf.cbData = pCodeParams->inputLineBufCbData;
        pEncInst->inputLineBuf.sbi_id_0 = pCodeParams->sbi_id_0;
        pEncInst->inputLineBuf.sbi_id_1 = pCodeParams->sbi_id_1;
        pEncInst->inputLineBuf.sbi_id_2 = pCodeParams->sbi_id_2;

        /*stream multi-segment*/
        pEncInst->streamMultiSegment.streamMultiSegmentMode = pCodeParams->streamMultiSegmentMode;
        pEncInst->streamMultiSegment.streamMultiSegmentAmount =
            pCodeParams->streamMultiSegmentAmount;
        pEncInst->streamMultiSegment.cbFunc = pCodeParams->streamMultiSegCbFunc;
        pEncInst->streamMultiSegment.cbData = pCodeParams->streamMultiSegCbData;

        /* smart */
        pEncInst->smartModeEnable = pCodeParams->smartModeEnable;
        pEncInst->smartH264LumDcTh = pCodeParams->smartH264LumDcTh;
        pEncInst->smartH264CbDcTh = pCodeParams->smartH264CbDcTh;
        pEncInst->smartH264CrDcTh = pCodeParams->smartH264CrDcTh;
        for (i = 0; i < 3; i++) {
            pEncInst->smartHevcLumDcTh[i] = pCodeParams->smartHevcLumDcTh[i];
            pEncInst->smartHevcChrDcTh[i] = pCodeParams->smartHevcChrDcTh[i];
            pEncInst->smartHevcLumAcNumTh[i] = pCodeParams->smartHevcLumAcNumTh[i];
            pEncInst->smartHevcChrAcNumTh[i] = pCodeParams->smartHevcChrAcNumTh[i];
        }
        pEncInst->smartH264Qp = pCodeParams->smartH264Qp;
        pEncInst->smartHevcLumQp = pCodeParams->smartHevcLumQp;
        pEncInst->smartHevcChrQp = pCodeParams->smartHevcChrQp;
        for (i = 0; i < 4; i++)
            pEncInst->smartMeanTh[i] = pCodeParams->smartMeanTh[i];
        pEncInst->smartPixNumCntTh = pCodeParams->smartPixNumCntTh;

        /* psy factor */
        pEncInst->psyFactor = pCodeParams->psyFactor;
        regs->psyFactor = (u32)(pEncInst->psyFactor * (1 << PSY_FACTOR_SCALE_BITS) + 0.5);

        /* multipass */
        pEncInst->cuTreeCtl.inQpDeltaBlkSize = 64 >> pCodeParams->roiMapDeltaQpBlockUnit;
        pEncInst->cuTreeCtl.aq_mode = pCodeParams->aq_mode;
        pEncInst->cuTreeCtl.aqStrength = pCodeParams->aq_strength;
    }

    /*update parameters enforced in encode order*/
    if (bUpdateEncOrderEnforceParam) {
        gdrDuration = pCodeParams->gdrDuration;
        pEncInst->rateControl.sei.insertRecoveryPointMessage = ENCHW_NO;
        pEncInst->rateControl.sei.recoveryFrameCnt = gdrDuration;
        if (pEncInst->encStatus < VCENCSTAT_START_FRAME) {
            pEncInst->gdrFirstIntraFrame = (1 + pEncInst->interlaced);
        }

        pEncInst->gdrEnabled = (gdrDuration > 0);
        bool gdrInit = pEncInst->gdrEnabled && (pEncInst->gdrDuration != (i32)gdrDuration);
        pEncInst->gdrDuration = gdrDuration;

        if (gdrInit) {
            pEncInst->gdrStart = 0;
            pEncInst->gdrCount = 0;

            pEncInst->gdrAverageMBRows = (pEncInst->ctbPerCol - 1) / pEncInst->gdrDuration;
            pEncInst->gdrMBLeft =
                pEncInst->ctbPerCol - 1 - pEncInst->gdrAverageMBRows * pEncInst->gdrDuration;

            if (pEncInst->gdrAverageMBRows == 0) {
                pEncInst->rateControl.sei.recoveryFrameCnt = pEncInst->gdrMBLeft;
                pEncInst->gdrDuration = pEncInst->gdrMBLeft;
            }
        }

        if (!pEncInst->gdrEnabled) {
            /* Allow user to turn off gdr */
            pEncInst->gdrStart = 0;
            pEncInst->gdrCount = 0;
            pEncInst->gdrAverageMBRows = 0;
            pEncInst->gdrMBLeft = 0;
            pEncInst->rateControl.sei.recoveryFrameCnt = 0;
            pEncInst->gdrDuration = 0;
        }
    }

    /*whether parameters enforced in input order or parameters enforced in encode order are updated,
    need to update follow parameters*/
    if (pCodeParams->intraArea.enable && gdrDuration == 0) {
        regs->intraAreaTop = pCodeParams->intraArea.top;
        regs->intraAreaLeft = pCodeParams->intraArea.left;
        regs->intraAreaBottom = pCodeParams->intraArea.bottom;
        regs->intraAreaRight = pCodeParams->intraArea.right;
    } else {
        regs->intraAreaTop = regs->intraAreaLeft = regs->intraAreaBottom = regs->intraAreaRight =
            INVALID_POS;
    }
    if (pCodeParams->ipcm1Area.enable && gdrDuration == 0) {
        regs->ipcm1AreaTop = pCodeParams->ipcm1Area.top;
        regs->ipcm1AreaLeft = pCodeParams->ipcm1Area.left;
        regs->ipcm1AreaBottom = pCodeParams->ipcm1Area.bottom;
        regs->ipcm1AreaRight = pCodeParams->ipcm1Area.right;
    } else {
        regs->ipcm1AreaTop = regs->ipcm1AreaLeft = regs->ipcm1AreaBottom = regs->ipcm1AreaRight =
            INVALID_POS;
    }

    if (pCodeParams->ipcm2Area.enable && gdrDuration == 0) {
        regs->ipcm2AreaTop = pCodeParams->ipcm2Area.top;
        regs->ipcm2AreaLeft = pCodeParams->ipcm2Area.left;
        regs->ipcm2AreaBottom = pCodeParams->ipcm2Area.bottom;
        regs->ipcm2AreaRight = pCodeParams->ipcm2Area.right;
    } else {
        regs->ipcm2AreaTop = regs->ipcm2AreaLeft = regs->ipcm2AreaBottom = regs->ipcm2AreaRight =
            INVALID_POS;
    }

    if (pCodeParams->ipcm3Area.enable && gdrDuration == 0) {
        regs->ipcm3AreaTop = pCodeParams->ipcm3Area.top;
        regs->ipcm3AreaLeft = pCodeParams->ipcm3Area.left;
        regs->ipcm3AreaBottom = pCodeParams->ipcm3Area.bottom;
        regs->ipcm3AreaRight = pCodeParams->ipcm3Area.right;
    } else {
        regs->ipcm3AreaTop = regs->ipcm3AreaLeft = regs->ipcm3AreaBottom = regs->ipcm3AreaRight =
            INVALID_POS;
    }

    if (pCodeParams->ipcm4Area.enable && gdrDuration == 0) {
        regs->ipcm4AreaTop = pCodeParams->ipcm4Area.top;
        regs->ipcm4AreaLeft = pCodeParams->ipcm4Area.left;
        regs->ipcm4AreaBottom = pCodeParams->ipcm4Area.bottom;
        regs->ipcm4AreaRight = pCodeParams->ipcm4Area.right;
    } else {
        regs->ipcm4AreaTop = regs->ipcm4AreaLeft = regs->ipcm4AreaBottom = regs->ipcm4AreaRight =
            INVALID_POS;
    }

    if (pCodeParams->ipcm5Area.enable && gdrDuration == 0) {
        regs->ipcm5AreaTop = pCodeParams->ipcm5Area.top;
        regs->ipcm5AreaLeft = pCodeParams->ipcm5Area.left;
        regs->ipcm5AreaBottom = pCodeParams->ipcm5Area.bottom;
        regs->ipcm5AreaRight = pCodeParams->ipcm5Area.right;
    } else {
        regs->ipcm5AreaTop = regs->ipcm5AreaLeft = regs->ipcm5AreaBottom = regs->ipcm5AreaRight =
            INVALID_POS;
    }

    if (pCodeParams->ipcm6Area.enable && gdrDuration == 0) {
        regs->ipcm6AreaTop = pCodeParams->ipcm6Area.top;
        regs->ipcm6AreaLeft = pCodeParams->ipcm6Area.left;
        regs->ipcm6AreaBottom = pCodeParams->ipcm6Area.bottom;
        regs->ipcm6AreaRight = pCodeParams->ipcm6Area.right;
    } else {
        regs->ipcm6AreaTop = regs->ipcm6AreaLeft = regs->ipcm6AreaBottom = regs->ipcm6AreaRight =
            INVALID_POS;
    }

    if (pCodeParams->ipcm7Area.enable && gdrDuration == 0) {
        regs->ipcm7AreaTop = pCodeParams->ipcm7Area.top;
        regs->ipcm7AreaLeft = pCodeParams->ipcm7Area.left;
        regs->ipcm7AreaBottom = pCodeParams->ipcm7Area.bottom;
        regs->ipcm7AreaRight = pCodeParams->ipcm7Area.right;
    } else {
        regs->ipcm7AreaTop = regs->ipcm7AreaLeft = regs->ipcm7AreaBottom = regs->ipcm7AreaRight =
            INVALID_POS;
    }

    if (pCodeParams->ipcm8Area.enable && gdrDuration == 0) {
        regs->ipcm8AreaTop = pCodeParams->ipcm8Area.top;
        regs->ipcm8AreaLeft = pCodeParams->ipcm8Area.left;
        regs->ipcm8AreaBottom = pCodeParams->ipcm8Area.bottom;
        regs->ipcm8AreaRight = pCodeParams->ipcm8Area.right;
    } else {
        regs->ipcm8AreaTop = regs->ipcm8AreaLeft = regs->ipcm8AreaBottom = regs->ipcm8AreaRight =
            INVALID_POS;
    }

    //keep pcm_enabled_flag the same in sw and cmodel
    if ((pCodeParams->ipcm1Area.enable || pCodeParams->ipcm2Area.enable ||
         pCodeParams->ipcm3Area.enable || pCodeParams->ipcm4Area.enable ||
         pCodeParams->ipcm5Area.enable || pCodeParams->ipcm6Area.enable ||
         pCodeParams->ipcm7Area.enable || pCodeParams->ipcm8Area.enable ||
         pCodeParams->ipcmMapEnable) == 0 &&
        pEncInst->sps->pcm_enabled_flag == 1) {
        regs->ipcm1AreaTop = 1;
        regs->ipcm1AreaBottom = 0;
        regs->ipcm1AreaLeft = 1;
        regs->ipcm1AreaRight = 0;
    }

    if (pCodeParams->roi1Area.enable && gdrDuration == 0) {
        regs->roi1Top = pCodeParams->roi1Area.top;
        regs->roi1Left = pCodeParams->roi1Area.left;
        regs->roi1Bottom = pCodeParams->roi1Area.bottom;
        regs->roi1Right = pCodeParams->roi1Area.right;
        pEncInst->roi1Enable = 1;
    } else {
        regs->roi1Top = regs->roi1Left = regs->roi1Bottom = regs->roi1Right = INVALID_POS;
        pEncInst->roi1Enable = 0;
    }

    pEncInst->asic.regs.bCodingCtrlUpdate = 1;

    /* these parameters only update when init*/
    if (pEncInst->encStatus == VCENCSTAT_INIT) {
        if (IS_H264(pEncInst->codecFormat)) {
            pEncInst->layerInRefIdc = pCodeParams->layerInRefIdcEnable;
        }

        regs->bRDOQEnable = pCodeParams->enableRdoQuant;
        regs->rdoqMapEnable = pCodeParams->rdoqMapEnable ? 1 : 0;

        regs->dynamicRdoEnable = pCodeParams->enableDynamicRdo;

        if (regs->dynamicRdoEnable) {
            regs->dynamicRdoCu16Bias = pCodeParams->dynamicRdoCu16Bias;
            regs->dynamicRdoCu16Factor = pCodeParams->dynamicRdoCu16Factor;
            regs->dynamicRdoCu32Bias = pCodeParams->dynamicRdoCu32Bias;
            regs->dynamicRdoCu32Factor = pCodeParams->dynamicRdoCu32Factor;
        }
        pEncInst->chromaQpOffset = pCodeParams->chroma_qp_offset;
        pEncInst->fieldOrder = pCodeParams->fieldOrder ? 1 : 0;
        pEncInst->disableDeblocking = pCodeParams->disableDeblockingFilter;
        pEncInst->enableDeblockOverride = pCodeParams->enableDeblockOverride;
        if (pEncInst->enableDeblockOverride) {
            regs->slice_deblocking_filter_override_flag = pCodeParams->deblockOverride;
        } else {
            regs->slice_deblocking_filter_override_flag = 0;
        }
        if (IS_H264(pEncInst->codecFormat)) {
            /* always enable deblocking override for H.264 */
            pEncInst->enableDeblockOverride = 1;
            regs->slice_deblocking_filter_override_flag = 1;
        }
        pEncInst->tc_Offset = pCodeParams->tc_Offset;
        pEncInst->beta_Offset = pCodeParams->beta_Offset;
        pEncInst->enableSao = pCodeParams->enableSao;

        VCEncHEVCDnfSetParameters(pEncInst, pCodeParams);

        regs->cabac_init_flag = pCodeParams->cabacInitFlag;
        regs->entropy_coding_mode_flag = (pCodeParams->enableCabac ? 1 : 0);

        /* SEI messages are written in the beginning of each frame */
        if (pCodeParams->seiMessages)
            pEncInst->rateControl.sei.enabled = ENCHW_YES;
        else
            pEncInst->rateControl.sei.enabled = ENCHW_NO;

        /* SRAM power down mode */
        regs->sramPowerdownDisable = pCodeParams->sramPowerdownDisable;

        //when init don't need to adjust refCnt
        return;
    }

end:
    pCodingCtrlParam->refCnt--;
    //remove useless parameters from queue
    if (0 == pCodingCtrlParam->refCnt) {
        queue_remove(&pEncInst->codingCtrl.codingCtrlQueue, (struct node *)pCodingCtrlParam);
        DynamicPutBufferToPool(&pEncInst->codingCtrl.codingCtrlBufPool, (void *)pCodingCtrlParam);
    }

    return;
}

/*------------------------------------------------------------------------------
    Function name : EncUpdateCodingCtrlForPass1
    Description   : [pass1]update coding ctrl parameters matched with current frame into instance
    Return type   : void
    Argument      : inst - encoder instance
    Argument      : pCodingCtrl - coding ctrl
------------------------------------------------------------------------------*/
void EncUpdateCodingCtrlForPass1(VCEncInst instAddr, EncCodingCtrlParam *pCodingCtrlParam)
{
    // if pCodingCtrlParam==NULL, there's no coding ctrl parameters need to be updated into instance.
    if (!instAddr || !pCodingCtrlParam)
        return;
    struct vcenc_instance *pEncInst = (struct vcenc_instance *)instAddr;
    //if this frame's codingCtrl is same with codingCtrl in instance, don't need to reset.
    if (pEncInst->codingCtrl.pCodingCtrlParam == pCodingCtrlParam)
        return;

    VCEncCodingCtrl *pCodingCtrl = &pCodingCtrlParam->encCodingCtrl;
    regValues_s *regs = &pEncInst->asic.regs;

    pEncInst->codingCtrl.pCodingCtrlParam = pCodingCtrlParam;
    pEncInst->roiMapEnable = (pCodingCtrl->roiMapDeltaQpEnable == 1) ? 1 : 0;
    regs->rcRoiEnable = 0x00;
    pEncInst->cuTreeCtl.inQpDeltaBlkSize = 64 >> pCodingCtrl->roiMapDeltaQpBlockUnit;
    pEncInst->cuTreeCtl.aq_mode = pCodingCtrl->aq_mode;
    pEncInst->cuTreeCtl.aqStrength = pCodingCtrl->aq_strength;
    /* psy factor (for quality)*/
    pEncInst->psyFactor = pCodingCtrl->psyFactor;
    regs->psyFactor = (u32)(pEncInst->psyFactor * (1 << PSY_FACTOR_SCALE_BITS) + 0.5);
    return;
}

/*------------------------------------------------------------------------------
    Function name : EncCheckCodingCtrlParam
    Description   : check inputed coding ctrl parameters
    Return type   : VCEncRet
    Argument      : inst - encoder instance
    Argument      : pCodeParams - coding ctrl
------------------------------------------------------------------------------*/
VCEncRet EncCheckCodingCtrlParam(struct vcenc_instance *pEncInst, VCEncCodingCtrl *pCodeParams)
{
    regValues_s *regs = &pEncInst->asic.regs;
    VCEncRet ret = VCENC_INVALID_ARGUMENT;

    do {
        /* Check for illegal inputs */
        if ((pEncInst == NULL) || (pCodeParams == NULL)) {
            APITRACEERR("VCEncSetCodingCtrl: ERROR Null argument\n");
            break;
        }

        u32 client_type = VCEncGetClientType(pEncInst->codecFormat);

        i32 core_id = -1;

        EWLHwConfig_t cfg = EncAsicGetAsicConfig(client_type, pEncInst->ctx);
        if (client_type == EWL_CLIENT_TYPE_H264_ENC &&
            cfg.h264Enabled == EWL_HW_CONFIG_NOT_SUPPORTED) {
            APITRACEERR("VCEncSetCodingCtrl: ERROR codecFormat=h264 unsupported by HW\n");
            break;
        }

        if (client_type == EWL_CLIENT_TYPE_HEVC_ENC &&
            cfg.hevcEnabled == EWL_HW_CONFIG_NOT_SUPPORTED) {
            APITRACEERR("VCEncSetCodingCtrl: ERROR codecFormat=hevc unsupported by HW\n");
            break;
        }

        if (client_type == EWL_CLIENT_TYPE_AV1_ENC &&
            cfg.av1Enabled == EWL_HW_CONFIG_NOT_SUPPORTED) {
            APITRACEERR("VCEncSetCodingCtrl: ERROR codecFormat=av1 unsupported by HW\n");
            break;
        }

        if (client_type == EWL_CLIENT_TYPE_VP9_ENC &&
            cfg.vp9Enabled == EWL_HW_CONFIG_NOT_SUPPORTED) {
            APITRACEERR("VCEncSetCodingCtrl: ERROR codecFormat=vp9 unsupported by HW\n");
            break;
        }

        if (cfg.deNoiseEnabled == EWL_HW_CONFIG_NOT_SUPPORTED &&
            pCodeParams->noiseReductionEnable == 1) {
            APITRACEERR("VCEncSetCodingCtrl: ERROR Denoise unsupported by HW\n");
            break;
        }

        if (pCodeParams->streamMultiSegmentMode != 0 &&
            cfg.streamMultiSegment == EWL_HW_CONFIG_NOT_SUPPORTED) {
            APITRACEERR("VCEncSetCodingCtrl: ERROR stream multiSegment mode unsupported by "
                        "HW\n");
            break;
        }

        if (((HW_ID_MAJOR_NUMBER(regs->asicHwId) < 2 /* H2V1 */) &&
             HW_PRODUCT_H2(regs->asicHwId)) &&
            (pCodeParams->roiMapDeltaQpEnable == 1)) {
            APITRACEERR("VCEncSetCodingCtrl: ERROR ROI MAP not supported\n");
            break;
        }

        if ((pEncInst->asic.regs.asicCfg.roiMapVersion < 3) &&
            pCodeParams->RoimapCuCtrl_index_enable) {
            APITRACEERR("VCEncSetCodingCtrl: ERROR ROI MAP cu ctrl index not supported\n");
            break;
        }
        if ((pEncInst->asic.regs.asicCfg.roiMapVersion < 3) && pCodeParams->RoimapCuCtrl_enable) {
            APITRACEERR("VCEncSetCodingCtrl: ERROR ROI MAP cu ctrl not supported\n");
            break;
        }

        bool bAdvanceHW = (pEncInst->asic.regs.asicCfg.roiMapVersion == 2) &&
                          ((HW_ID_MAJOR_NUMBER(pEncInst->asic.regs.asicCfg.hw_asic_id) >= 0x82) ||
                           (HW_PRODUCT_VC9000(pEncInst->asic.regs.asicCfg.hw_asic_id)) ||
                           HW_PRODUCT_SYSTEM6010(pEncInst->asic.regs.asicCfg.hw_asic_id) ||
                           (HW_PRODUCT_VC9000LE(pEncInst->asic.regs.asicCfg.hw_asic_id)));

        if (pCodeParams->RoimapCuCtrl_enable &&
            ((pCodeParams->RoimapCuCtrl_ver < 3) || (pCodeParams->RoimapCuCtrl_ver > 7) ||
             IS_AV1(pEncInst->codecFormat))) {
            APITRACEERR("VCEncSetCodingCtrl: ERROR Invalid RoiCuCtrlVer\n");
            break;
        }
        if ((pCodeParams->RoimapCuCtrl_enable == 0) && pCodeParams->RoimapCuCtrl_ver != 0) {
            APITRACEERR("VCEncSetCodingCtrl: ERROR Invalid RoimapCuCtrl_ver\n");
            break;
        }

        true_e qpMapInvalid =
            pCodeParams->roiMapDeltaQpEnable && (pCodeParams->RoiQpDelta_ver == 0);
        true_e ipcmMapInvalid =
            pCodeParams->ipcmMapEnable &&
            (((pCodeParams->RoiQpDelta_ver != 1) && (pCodeParams->RoimapCuCtrl_enable == 0)) ||
             (pCodeParams->RoimapCuCtrl_enable && (4 > pCodeParams->RoimapCuCtrl_ver)));
        true_e skipMapInvalid =
            pCodeParams->skipMapEnable &&
            (((pCodeParams->RoiQpDelta_ver < 2) && (pCodeParams->RoimapCuCtrl_enable == 0)) ||
             (pCodeParams->RoimapCuCtrl_enable && (4 > pCodeParams->RoimapCuCtrl_ver)));
        true_e rdoqMapInvalid = pCodeParams->rdoqMapEnable && (pCodeParams->RoiQpDelta_ver != 4);
        true_e roiMapVersion3QpDeltaNotValid =
            (pEncInst->asic.regs.asicCfg.roiMapVersion == 3) &&
            (qpMapInvalid || ipcmMapInvalid || skipMapInvalid || (pCodeParams->RoiQpDelta_ver > 3));
        true_e roiMapVersion4QpDeltaNotValid =
            (pEncInst->asic.regs.asicCfg.roiMapVersion == 4) &&
            (qpMapInvalid || ipcmMapInvalid || skipMapInvalid || rdoqMapInvalid);
        if (roiMapVersion3QpDeltaNotValid || roiMapVersion4QpDeltaNotValid ||
            ((pEncInst->asic.regs.asicCfg.roiMapVersion < 3) &&
             (pCodeParams->RoiQpDelta_ver > 2)) ||
            (pCodeParams->RoiQpDelta_ver > 4)) {
            APITRACEERR("VCEncSetCodingCtrl: ERROR Invalid RoiQpDelta_ver\n");
            break;
        }
        if (bAdvanceHW && (pCodeParams->ipcmMapEnable || pCodeParams->skipMapEnable) &&
            (pCodeParams->RoiQpDelta_ver == 0)) {
            APITRACEERR("VCEncSetCodingCtrl: ERROR Invalid RoiQpDelta_ver when advance HW\n");
            break;
        }

        /* roiMap can be enabled only if cuQpDelta is enabled in pps */
        i32 roiMapEnableAllowed =
            pEncInst->encStatus < VCENCSTAT_START_STREAM || pEncInst->asic.regs.cuQpDeltaEnabled;
        if (!roiMapEnableAllowed && pCodeParams->roiMapDeltaQpEnable) {
            APITRACEERR("VCEncSetCodingCtrl: ERROR Invalid encoding status to config roi Map "
                        "Enable\n");
            break;
        }

        if ((pEncInst->encStatus >= VCENCSTAT_START_STREAM) &&
            (pEncInst->RoimapCuCtrl_enable != pCodeParams->RoimapCuCtrl_enable)) {
            APITRACEERR("VCEncSetCodingCtrl: ERROR Invalid encoding status to config roi Map "
                        "cu ctrl Enable\n");
            break;
        }

        /* Check for invalid values */
        if (pCodeParams->sliceSize > (u32)pEncInst->ctbPerCol) {
            APITRACEERR("VCEncSetCodingCtrl: ERROR Invalid sliceSize\n");
            break;
        }
        if ((IS_AV1(pEncInst->codecFormat) || IS_VP9(pEncInst->codecFormat)) &&
            (pCodeParams->sliceSize != 0)) {
            APITRACEERR("VCEncSetCodingCtrl: ERROR Invalid sliceSize for AV1 or VP9, it "
                        "should be 0\n");
            break;
        }

        if (pEncInst->tiles_enabled_flag && (pCodeParams->sliceSize != 0)) {
            APITRACEERR("VCEncSetCodingCtrl: sliceSize NOT supported when multi tiles\n");
            break;
        }

        if (pCodeParams->cirStart > (u32)pEncInst->ctbPerFrame ||
            pCodeParams->cirInterval > (u32)pEncInst->ctbPerFrame) {
            APITRACEERR("VCEncSetCodingCtrl: ERROR Invalid CIR value\n");
            break;
        }
        if (pCodeParams->intraArea.enable) {
            if (!(pCodeParams->intraArea.top <= pCodeParams->intraArea.bottom &&
                  pCodeParams->intraArea.bottom < (u32)pEncInst->ctbPerCol &&
                  pCodeParams->intraArea.left <= pCodeParams->intraArea.right &&
                  pCodeParams->intraArea.right < (u32)pEncInst->ctbPerRow)) {
                APITRACEERR("VCEncSetCodingCtrl: ERROR Invalid intraArea\n");
                break;
            }
            if ((pCodeParams->gdrDuration > 0)) {
                APITRACEERR("VCEncSetCodingCtrl: WARNING GDR will overwrite intraArea");
            }
        }

        true_e ipcmAreaSupport = pCodeParams->ipcm1Area.enable || pCodeParams->ipcm2Area.enable ||
                                 pCodeParams->ipcm3Area.enable || pCodeParams->ipcm4Area.enable ||
                                 pCodeParams->ipcm5Area.enable || pCodeParams->ipcm6Area.enable ||
                                 pCodeParams->ipcm7Area.enable || pCodeParams->ipcm8Area.enable;
        if ((IS_AV1(pEncInst->codecFormat) || IS_VP9(pEncInst->codecFormat)) &&
            (ipcmAreaSupport || pCodeParams->ipcmMapEnable)) {
            APITRACEERR("VCEncSetCodingCtrl: ERROR IPCM not supported for AV1 or VP9\n");
            break;
        }

        if (pEncInst->encStatus != VCENCSTAT_INIT && pEncInst->pcm_enabled_flag == 0 &&
            (ipcmAreaSupport || pCodeParams->ipcmMapEnable)) {
            APITRACEERR("VCEncSetCodingCtrl: ERROR Invalid ipcmArea, ipcm is disabled \n");
            break;
        }

        if (pCodeParams->ipcm1Area.enable) {
            if (!(pCodeParams->ipcm1Area.top <= pCodeParams->ipcm1Area.bottom &&
                  pCodeParams->ipcm1Area.bottom < (u32)pEncInst->ctbPerCol &&
                  pCodeParams->ipcm1Area.left <= pCodeParams->ipcm1Area.right &&
                  pCodeParams->ipcm1Area.right < (u32)pEncInst->ctbPerRow)) {
                APITRACEERR("VCEncSetCodingCtrl: ERROR Invalid ipcm1Area\n");
                break;
            }
            if ((pCodeParams->gdrDuration > 0)) {
                APITRACEERR("VCEncSetCodingCtrl: WARNING GDR will overwrite ipcm1Area");
            }
        }
        if (pCodeParams->ipcm2Area.enable) {
            if (!(pCodeParams->ipcm2Area.top <= pCodeParams->ipcm2Area.bottom &&
                  pCodeParams->ipcm2Area.bottom < (u32)pEncInst->ctbPerCol &&
                  pCodeParams->ipcm2Area.left <= pCodeParams->ipcm2Area.right &&
                  pCodeParams->ipcm2Area.right < (u32)pEncInst->ctbPerRow)) {
                APITRACEERR("VCEncSetCodingCtrl: ERROR Invalid ipcm2Area\n");
                break;
            }
            if ((pCodeParams->gdrDuration > 0)) {
                APITRACEERR("VCEncSetCodingCtrl: WARNING GDR will overwrite ipcm2Area");
            }
        }

        if ((regs->asicCfg.IPCM8Support == 0) &&
            (pCodeParams->ipcm3Area.enable || pCodeParams->ipcm4Area.enable ||
             pCodeParams->ipcm5Area.enable || pCodeParams->ipcm6Area.enable ||
             pCodeParams->ipcm7Area.enable || pCodeParams->ipcm8Area.enable)) {
            APITRACEERR("VCEncSetCodingCtrl: ERROR Invalid ipcmArea\n");
            break;
        }

        if (pCodeParams->ipcm3Area.enable) {
            if (!(pCodeParams->ipcm3Area.top <= pCodeParams->ipcm3Area.bottom &&
                  pCodeParams->ipcm3Area.bottom < (u32)pEncInst->ctbPerCol &&
                  pCodeParams->ipcm3Area.left <= pCodeParams->ipcm3Area.right &&
                  pCodeParams->ipcm3Area.right < (u32)pEncInst->ctbPerRow)) {
                APITRACEERR("VCEncSetCodingCtrl: ERROR Invalid ipcm3Area\n");
                break;
            }
            if ((pCodeParams->gdrDuration > 0)) {
                APITRACEERR("VCEncSetCodingCtrl: WARNING GDR will overwrite ipcm3Area");
            }
        }

        if (pCodeParams->ipcm4Area.enable) {
            if (!(pCodeParams->ipcm4Area.top <= pCodeParams->ipcm4Area.bottom &&
                  pCodeParams->ipcm4Area.bottom < (u32)pEncInst->ctbPerCol &&
                  pCodeParams->ipcm4Area.left <= pCodeParams->ipcm4Area.right &&
                  pCodeParams->ipcm4Area.right < (u32)pEncInst->ctbPerRow)) {
                APITRACEERR("VCEncSetCodingCtrl: ERROR Invalid ipcm4Area\n");
                break;
            }
            if ((pCodeParams->gdrDuration > 0)) {
                APITRACEERR("VCEncSetCodingCtrl: WARNING GDR will overwrite ipcm4Area");
            }
        }

        if (pCodeParams->ipcm5Area.enable) {
            if (!(pCodeParams->ipcm5Area.top <= pCodeParams->ipcm5Area.bottom &&
                  pCodeParams->ipcm5Area.bottom < (u32)pEncInst->ctbPerCol &&
                  pCodeParams->ipcm5Area.left <= pCodeParams->ipcm5Area.right &&
                  pCodeParams->ipcm5Area.right < (u32)pEncInst->ctbPerRow)) {
                APITRACEERR("VCEncSetCodingCtrl: ERROR Invalid ipcm5Area\n");
                break;
            }
            if ((pCodeParams->gdrDuration > 0)) {
                APITRACEERR("VCEncSetCodingCtrl: WARNING GDR will overwrite ipcm5Area");
            }
        }

        if (pCodeParams->ipcm6Area.enable) {
            if (!(pCodeParams->ipcm6Area.top <= pCodeParams->ipcm6Area.bottom &&
                  pCodeParams->ipcm6Area.bottom < (u32)pEncInst->ctbPerCol &&
                  pCodeParams->ipcm6Area.left <= pCodeParams->ipcm6Area.right &&
                  pCodeParams->ipcm6Area.right < (u32)pEncInst->ctbPerRow)) {
                APITRACEERR("VCEncSetCodingCtrl: ERROR Invalid ipcm6Area\n");
                break;
            }
            if ((pCodeParams->gdrDuration > 0)) {
                APITRACEERR("VCEncSetCodingCtrl: WARNING GDR will overwrite ipcm6Area");
            }
        }

        if (pCodeParams->ipcm7Area.enable) {
            if (!(pCodeParams->ipcm7Area.top <= pCodeParams->ipcm7Area.bottom &&
                  pCodeParams->ipcm7Area.bottom < (u32)pEncInst->ctbPerCol &&
                  pCodeParams->ipcm7Area.left <= pCodeParams->ipcm7Area.right &&
                  pCodeParams->ipcm7Area.right < (u32)pEncInst->ctbPerRow)) {
                APITRACEERR("VCEncSetCodingCtrl: ERROR Invalid ipcm7Area\n");
                break;
            }
            if ((pCodeParams->gdrDuration > 0)) {
                APITRACEERR("VCEncSetCodingCtrl: WARNING GDR will overwrite ipcm7Area");
            }
        }

        if (pCodeParams->ipcm8Area.enable) {
            if (!(pCodeParams->ipcm8Area.top <= pCodeParams->ipcm8Area.bottom &&
                  pCodeParams->ipcm8Area.bottom < (u32)pEncInst->ctbPerCol &&
                  pCodeParams->ipcm8Area.left <= pCodeParams->ipcm8Area.right &&
                  pCodeParams->ipcm8Area.right < (u32)pEncInst->ctbPerRow)) {
                APITRACEERR("VCEncSetCodingCtrl: ERROR Invalid ipcm8Area\n");
                break;
            }
            if ((pCodeParams->gdrDuration > 0)) {
                APITRACEERR("VCEncSetCodingCtrl: WARNING GDR will overwrite ipcm8Area");
            }
        }

        if (VCENC_OK != vcencCheckRoiCtrl(pEncInst, pCodeParams)) {
            APITRACEERR("VCEncSetCodingCtrl: ERROR Invalid encoding status to config "
                        "roi1Area\n");
            break;
        }

        if (pCodeParams->roi1Area.enable) {
            if (!(pCodeParams->roi1Area.top <= pCodeParams->roi1Area.bottom &&
                  pCodeParams->roi1Area.bottom < (u32)pEncInst->ctbPerCol &&
                  pCodeParams->roi1Area.left <= pCodeParams->roi1Area.right &&
                  pCodeParams->roi1Area.right < (u32)pEncInst->ctbPerRow)) {
                APITRACEERR("VCEncSetCodingCtrl: ERROR Invalid roi1Area\n");
                break;
            }
            if ((pCodeParams->gdrDuration > 0)) {
                APITRACEERR("VCEncSetCodingCtrl: WARNING GDR will overwrite roi1Area");
            }
        }

        if (pCodeParams->roi2Area.enable) {
            if (!(pCodeParams->roi2Area.top <= pCodeParams->roi2Area.bottom &&
                  pCodeParams->roi2Area.bottom < (u32)pEncInst->ctbPerCol &&
                  pCodeParams->roi2Area.left <= pCodeParams->roi2Area.right &&
                  pCodeParams->roi2Area.right < (u32)pEncInst->ctbPerRow)) {
                APITRACEERR("VCEncSetCodingCtrl: ERROR Invalid roi2Area\n");
                break;
            }
        }

        if ((regs->asicCfg.ROI8Support == 0) &&
            (pCodeParams->roi3Area.enable || pCodeParams->roi4Area.enable ||
             pCodeParams->roi5Area.enable || pCodeParams->roi6Area.enable ||
             pCodeParams->roi7Area.enable || pCodeParams->roi8Area.enable)) {
            APITRACEERR("VCEncSetCodingCtrl: ERROR Invalid roiArea\n");
            break;
        }

        if (pCodeParams->roi3Area.enable) {
            if (!(pCodeParams->roi3Area.top <= pCodeParams->roi3Area.bottom &&
                  pCodeParams->roi3Area.bottom < (u32)pEncInst->ctbPerCol &&
                  pCodeParams->roi3Area.left <= pCodeParams->roi3Area.right &&
                  pCodeParams->roi3Area.right < (u32)pEncInst->ctbPerRow)) {
                APITRACEERR("VCEncSetCodingCtrl: ERROR Invalid roi3Area\n");
                break;
            }
        }

        if (pCodeParams->roi4Area.enable) {
            if (!(pCodeParams->roi4Area.top <= pCodeParams->roi4Area.bottom &&
                  pCodeParams->roi4Area.bottom < (u32)pEncInst->ctbPerCol &&
                  pCodeParams->roi4Area.left <= pCodeParams->roi4Area.right &&
                  pCodeParams->roi4Area.right < (u32)pEncInst->ctbPerRow)) {
                APITRACEERR("VCEncSetCodingCtrl: ERROR Invalid roi4Area\n");
                break;
            }
        }

        if (pCodeParams->roi5Area.enable) {
            if (!(pCodeParams->roi5Area.top <= pCodeParams->roi5Area.bottom &&
                  pCodeParams->roi5Area.bottom < (u32)pEncInst->ctbPerCol &&
                  pCodeParams->roi5Area.left <= pCodeParams->roi5Area.right &&
                  pCodeParams->roi5Area.right < (u32)pEncInst->ctbPerRow)) {
                APITRACEERR("VCEncSetCodingCtrl: ERROR Invalid roi5Area\n");
                break;
            }
        }

        if (pCodeParams->roi6Area.enable) {
            if (!(pCodeParams->roi6Area.top <= pCodeParams->roi6Area.bottom &&
                  pCodeParams->roi6Area.bottom < (u32)pEncInst->ctbPerCol &&
                  pCodeParams->roi6Area.left <= pCodeParams->roi6Area.right &&
                  pCodeParams->roi6Area.right < (u32)pEncInst->ctbPerRow)) {
                APITRACEERR("VCEncSetCodingCtrl: ERROR Invalid roi6Area\n");
                break;
            }
        }

        if (pCodeParams->roi7Area.enable) {
            if (!(pCodeParams->roi7Area.top <= pCodeParams->roi7Area.bottom &&
                  pCodeParams->roi7Area.bottom < (u32)pEncInst->ctbPerCol &&
                  pCodeParams->roi7Area.left <= pCodeParams->roi7Area.right &&
                  pCodeParams->roi7Area.right < (u32)pEncInst->ctbPerRow)) {
                APITRACEERR("VCEncSetCodingCtrl: ERROR Invalid roi7Area\n");
                break;
            }
        }

        if (pCodeParams->roi8Area.enable) {
            if (!(pCodeParams->roi8Area.top <= pCodeParams->roi8Area.bottom &&
                  pCodeParams->roi8Area.bottom < (u32)pEncInst->ctbPerCol &&
                  pCodeParams->roi8Area.left <= pCodeParams->roi8Area.right &&
                  pCodeParams->roi8Area.right < (u32)pEncInst->ctbPerRow)) {
                APITRACEERR("VCEncSetCodingCtrl: ERROR Invalid roi8Area\n");
                break;
            }
        }

        if ((((!bAdvanceHW) &&
              (regs->asicCfg.roiMapVersion != 1 && regs->asicCfg.roiMapVersion != 3)) ||
             (bAdvanceHW && pCodeParams->RoiQpDelta_ver != 1)) &&
            pCodeParams->ipcmMapEnable) {
            APITRACEERR("VCEncSetCodingCtrl: ERROR IPCM MAP not supported\n");
            break;
        }

        if (pCodeParams->skipMapEnable) {
            if (IS_AV1(pEncInst->codecFormat) || IS_VP9(pEncInst->codecFormat)) {
                APITRACEERR("VCEncSetCodingCtrl: ERROR SKIP MAP not supported for AV1/VP9\n");
                break;
            } else if (((!bAdvanceHW) &&
                        (regs->asicCfg.roiMapVersion != 2 && regs->asicCfg.roiMapVersion != 3) &&
                        regs->asicCfg.roiMapVersion != 4) ||
                       (bAdvanceHW && pCodeParams->RoiQpDelta_ver != 2)) {
                APITRACEERR("VCEncSetCodingCtrl: ERROR SKIP MAP not supported by HW\n");
                break;
            }
        }

        if (pCodeParams->rdoqMapEnable) {
            if ((regs->asicCfg.roiMapVersion != 4) && (pCodeParams->RoiQpDelta_ver != 4)) {
                APITRACEERR("VCEncSetCodingCtrl: ERROR RDOQ MAP not supported by HW\n");
                break;
            }
        }

        if (pCodeParams->aq_mode > 3) {
            APITRACEERR("VCEncSetCodingCtrl ERROR: INVALID aq_mode, should be within [0, "
                        "3]\n");
            break;
        }

        if ((pCodeParams->aq_strength > 3.0) || (pCodeParams->aq_strength < 0.0)) {
            APITRACEERR("VCEncSetCodingCtrl ERROR: INVALID aq_strength, should be within "
                        "[0.0, 3.0]\n");
            break;
        }

        if ((pCodeParams->psyFactor > 4.0) || (pCodeParams->psyFactor < 0.0)) {
            APITRACEERR("VCEncSetCodingCtrl ERROR: INVALID psyFactor, should be within [0.0, "
                        "4.0]\n");
            break;
        } else if ((pCodeParams->psyFactor > 0.0) && !regs->asicCfg.encPsyTuneSupport) {
            APITRACEERR("VCEncSetCodingCtrl ERROR: psy is not supported by HW\n");
            break;
        }

        if (regs->asicCfg.roiAbsQpSupport) {
            if (pCodeParams->roi1DeltaQp < -51 || pCodeParams->roi1DeltaQp > 51 ||
                pCodeParams->roi2DeltaQp < -51 || pCodeParams->roi2DeltaQp > 51 ||
                pCodeParams->roi3DeltaQp < -51 || pCodeParams->roi3DeltaQp > 51 ||
                pCodeParams->roi4DeltaQp < -51 || pCodeParams->roi4DeltaQp > 51 ||
                pCodeParams->roi5DeltaQp < -51 || pCodeParams->roi5DeltaQp > 51 ||
                pCodeParams->roi6DeltaQp < -51 || pCodeParams->roi6DeltaQp > 51 ||
                pCodeParams->roi7DeltaQp < -51 || pCodeParams->roi7DeltaQp > 51 ||
                pCodeParams->roi8DeltaQp < -51 || pCodeParams->roi8DeltaQp > 51 ||
                pCodeParams->roi1Qp > 51 || pCodeParams->roi2Qp > 51 || pCodeParams->roi3Qp > 51 ||
                pCodeParams->roi4Qp > 51 || pCodeParams->roi5Qp > 51 || pCodeParams->roi6Qp > 51 ||
                pCodeParams->roi7Qp > 51 || pCodeParams->roi8Qp > 51) {
                APITRACEERR("VCEncSetCodingCtrl: ERROR Invalid ROI delta QP\n");
                break;
            }
        } else {
            if (pCodeParams->roi1DeltaQp < -30 ||
          pCodeParams->roi1DeltaQp > 0 ||
          pCodeParams->roi2DeltaQp < -30 ||
          pCodeParams->roi2DeltaQp > 0 /*||
         pCodeParams->adaptiveRoi < -51 ||
         pCodeParams->adaptiveRoi > 0*/)
      {
                APITRACEERR("VCEncSetCodingCtrl: ERROR Invalid ROI delta QP\n");
                break;
            }
        }

        /* low latency : Input Line buffer check */
        if (pCodeParams->inputLineBufEn) {
            /* check zero depth */
            /* 6.0 does not check for amoutPerLoopBack */
            bool loopBackFalse = (pCodeParams->amountPerLoopBack == 0) &&
                                 (HW_ID_MAJOR_NUMBER(regs->asicHwId) == 0x62);
            if ((pCodeParams->inputLineBufDepth == 0 || loopBackFalse) &&
                (pCodeParams->inputLineBufLoopBackEn || pCodeParams->inputLineBufHwModeEn)) {
                APITRACEERR("VCEncSetCodingCtrl: ERROR Invalid input buffer depth\n");
                break;
            }

            /*prp sbi just support HW handshake loop back mode*/
            if (regs->asicCfg.prpSbiSupport &&
                !(pCodeParams->inputLineBufLoopBackEn == 1 && pCodeParams->inputLineBufHwModeEn)) {
                APITRACEERR("VCEncSetCodingCtrl: ERROR flexa only support HW handshake loop "
                            "back mode\n");
                break;
            }
        }

        if (pCodeParams->streamMultiSegmentMode > 2 ||
            ((pCodeParams->streamMultiSegmentMode > 0) &&
             (pCodeParams->streamMultiSegmentAmount <= 1 ||
              pCodeParams->streamMultiSegmentAmount > 16))) {
            APITRACEERR("VCEncSetCodingCtrl: ERROR Invalid stream multi-segment config\n");
            break;
        }

        if (pCodeParams->streamMultiSegmentMode != 0 && pCodeParams->sliceSize != 0) {
            APITRACEERR("VCEncSetCodingCtrl: ERROR multi-segment not support slice "
                        "encoding\n");
            break;
        }

        if ((pCodeParams->Hdr10Display.hdr10_display_enable == ENCHW_YES) ||
            (pCodeParams->Hdr10LightLevel.hdr10_lightlevel_enable == ENCHW_YES)) {
            // parameters check
            if (!IS_HEVC(pEncInst->codecFormat) && !IS_AV1(pEncInst->codecFormat) &&
                !IS_H264(pEncInst->codecFormat)) {
                APITRACEERR("VCEncSetCodingCtrl: ERROR Invalid Encoder Type, It should be HEVC "
                            "or AV1 or H264!\n\n");
                break;
            }

            if (IS_HEVC(pEncInst->codecFormat) && pEncInst->profile != VCENC_HEVC_MAIN_10_PROFILE) {
                APITRACEERR("VCEncSetCodingCtrl: ERROR Invalid profile with HEVC, It should be "
                            "HEVC_MAIN_10_PROFILE!\n\n");
                break;
            }

            if (IS_H264(pEncInst->codecFormat) && pEncInst->profile != VCENC_PROFILE_H264_HIGH_10) {
                APITRACEERR("VCEncSetCodingCtrl: ERROR Invalid profile with H264, It should be "
                            "VCENC_H264_HIGH_10_PROFILE!\n\n");
                break;
            }

            /*
      if (pCodeParams->Hdr10Color.hdr10_color_enable == ENCHW_YES)
      {
        if ((pCodeParams->Hdr10Color.hdr10_matrix != 9) || (pCodeParams->Hdr10Color.hdr10_primary != 9))
        {
          APITRACEERR("VCEncSetCodingCtrl: ERROR Invalid HDR10 colour transfer VUI parameter\n\n");
          return VCENC_INVALID_ARGUMENT;
        }
      }
      */
        }

        /* Programable ME vertical search range */
        if (pCodeParams->meVertSearchRange) {
            u8 permitVertRange[3] = {
                0,
            };
            u8 bPermit = 0;
            if (!regs->asicCfg.meVertRangeProgramable) {
                APITRACEERR("VCEncSetCodingCtrl: Programable ME vertical search range not "
                            "supported\n");
                break;
            }

            if (IS_H264(pEncInst->codecFormat)) {
                permitVertRange[0] = 24;
                permitVertRange[1] = 48;
                permitVertRange[2] = regs->asicCfg.meVertSearchRangeH264 * 8;
            } else {
                permitVertRange[0] = 40;
                permitVertRange[1] = 64;
                permitVertRange[2] = regs->asicCfg.meVertSearchRangeHEVC * 8;
            }

            /* check if the range is permissible */
            for (i32 i = 0; i < 3; i++) {
                if (permitVertRange[i] && pCodeParams->meVertSearchRange == permitVertRange[i]) {
                    bPermit = 1;
                    break;
                }
            }

            if (!bPermit) {
                APITRACEERR("VCEncSetCodingCtrl: Invalid ME vertical search range. Should be "
                            "24|48|64 for H264; 40|64 for others.\n");
                break;
            }
        }

        if (pCodeParams->smartModeEnable && !pEncInst->asic.regs.asicCfg.backgroundDetSupport) {
            APITRACEERR("VCEncSetCodingCtrl: Smart background Mode not supported\n");
            break;
        }

        if (pCodeParams->enableSao > 1 || pCodeParams->roiMapDeltaQpEnable > 1 ||
            pCodeParams->disableDeblockingFilter > 1 || pCodeParams->RoimapCuCtrl_enable > 1 ||
            pCodeParams->seiMessages > 1 || pCodeParams->vuiVideoFullRange > 1) {
            APITRACEERR("VCEncSetCodingCtrl: ERROR Invalid enable/disable\n");
            break;
        }

        if (pCodeParams->enableSao &&
            (!cfg.saoSupport && cfg.hw_asic_id >= MIN_ASIC_ID_WITH_BUILD_ID)) {
            APITRACEERR("VCEncSetCodingCtrl: SAO not support\n");
            break;
        }

        if (pCodeParams->sampleAspectRatioWidth > 65535 ||
            pCodeParams->sampleAspectRatioHeight > 65535) {
            APITRACEERR("VCEncSetCodingCtrl: ERROR Invalid sampleAspectRatio\n");
            break;
        }

        //follow parameters only enforce when init
        if (pEncInst->encStatus == VCENCSTAT_INIT) {
            if (pCodeParams->cabacInitFlag > 1) {
                APITRACEERR("VCEncSetCodingCtrl: ERROR Invalid cabacInitIdc\n");
                break;
            }
            if (pCodeParams->enableCabac > 2) {
                APITRACEERR("VCEncSetCodingCtrl: ERROR Invalid enableCabac\n");
                break;
            }

            /* check limitation for H.264 baseline profile */
            if (IS_H264(pEncInst->codecFormat) && pEncInst->profile == 66 &&
                pCodeParams->enableCabac > 0) {
                APITRACEERR("VCEncSetCodingCtrl: ERROR Invalid entropy coding mode for "
                            "baseline profile\n");
                break;
            }

            if ((IS_H264(pEncInst->codecFormat) && pCodeParams->roiMapDeltaQpBlockUnit >= 3) ||
                (IS_AV1(pEncInst->codecFormat) &&
                 pCodeParams->roiMapDeltaQpBlockUnit >= 1)) //AV1 qpDelta in CTU level
            {
                APITRACEERR("VCEncSetCodingCtrl: ERROR Invalid roiMapDeltaQpBlockUnit\n");
                break;
            }

            if (pCodeParams->enableRdoQuant || pCodeParams->rdoqMapEnable) {
                i32 rdoqSupport =
                    (IS_H264(pEncInst->codecFormat) && regs->asicCfg.RDOQSupportH264) ||
                    (IS_AV1(pEncInst->codecFormat) && regs->asicCfg.RDOQSupportAV1) ||
                    (IS_HEVC(pEncInst->codecFormat) && regs->asicCfg.RDOQSupportHEVC) ||
                    (IS_VP9(pEncInst->codecFormat) && regs->asicCfg.RDOQSupportVP9);
                if ((pCodeParams->enableRdoQuant || pCodeParams->rdoqMapEnable) &&
                    (rdoqSupport == 0)) {
                    APITRACEERR("VCEncSetCodingCtrl: RDO Quant is not supported by this HW "
                                "version\n");
                    break;
                }
                /* RDO Quant is not supported when cavlc is enabled for H264 encoder */
                if (IS_H264(pEncInst->codecFormat) && (pCodeParams->enableCabac == 0)) {
                    if (pCodeParams->rdoqMapEnable) {
                        APITRACEERR("VCEncSetCodingCtrl: WARNING. RDO Quant map is not supported "
                                    "by H264 CAVLC and forced to be disabled\n");
                        pCodeParams->rdoqMapEnable = 0;
                    }
                    if (pCodeParams->enableRdoQuant) {
                        APITRACEERR("VCEncSetCodingCtrl: WARNING. RDO Quant is not supported by "
                                    "H264 CAVLC and forced to be disabled\n");
                        pCodeParams->enableRdoQuant = 0;
                    }
                }
            }

            if (pCodeParams->enableDynamicRdo) {
                if (regs->asicCfg.dynamicRdoSupport == 0) {
                    pCodeParams->enableDynamicRdo = 0;
                    APITRACEERR("VCEncSetCodingCtrl: Dynamic RDO is not supported by HW and "
                                "forced to be disabled\n");
                }
            }
        }
        ret = VCENC_OK;
    } while (0);

    return ret;
}

/* get client type according to coding type */
CLIENT_TYPE VCEncGetClientType(VCEncVideoCodecFormat codecFormat)
{
    u32 clientType = EWL_CLIENT_TYPE_JPEG_ENC;
    if (IS_H264(codecFormat))
        clientType = EWL_CLIENT_TYPE_H264_ENC;
    else if (IS_AV1(codecFormat))
        clientType = EWL_CLIENT_TYPE_AV1_ENC;
    else if (IS_VP9(codecFormat))
        clientType = EWL_CLIENT_TYPE_VP9_ENC;
    else if (IS_HEVC(codecFormat))
        clientType = EWL_CLIENT_TYPE_HEVC_ENC;
    else {
        ASSERT(0 && "Unsupported codecFormat");
    }
    return clientType;
}

/** hevc and h264 HDR10 mdc and cll */
void VCEncEncodeSeiHdr10(struct vcenc_instance *vcenc_instance, VCEncOut *pEncOut)
{
    u32 tmp = 0;
    u32 byte_stream = (vcenc_instance->sps->streamMode == ASIC_VCENC_BYTE_STREAM);
    u32 enc_status = (vcenc_instance->encStatus == VCENCSTAT_INIT);

    if ((vcenc_instance->Hdr10Display.hdr10_display_enable == (u8)HDR10_CFGED)) {
        u32 u32SeiCnt = 0, nal_size = 0;
        if (enc_status)
            vcenc_instance->stream.cnt = &u32SeiCnt; //just used to initial the cnt when StrmStart()
        else
            tmp = vcenc_instance->stream.byteCnt;

        if (IS_HEVC(vcenc_instance->codecFormat))
            HevcNalUnitHdr(&vcenc_instance->stream, PREFIX_SEI_NUT, byte_stream);
        else if (IS_H264(vcenc_instance->codecFormat))
            H264NalUnitHdr(&vcenc_instance->stream, 0, H264_SEI, byte_stream);

        HevcMasteringDisplayColourSei(&vcenc_instance->stream, &vcenc_instance->Hdr10Display);
        if (enc_status)
            nal_size = u32SeiCnt;
        else
            nal_size = vcenc_instance->stream.byteCnt - tmp;

        pEncOut->streamSize += nal_size;
        VCEncAddNaluSize(pEncOut, nal_size, 0);
        pEncOut->PreDataSize += nal_size;
        pEncOut->PreNaluNum++;
        hash(&vcenc_instance->hashctx, vcenc_instance->stream.stream, nal_size);
    }

    if ((vcenc_instance->Hdr10LightLevel.hdr10_lightlevel_enable == (u8)HDR10_CFGED)) {
        u32 u32SeiCnt = 0, nal_size = 0;
        if (enc_status)
            vcenc_instance->stream.cnt = &u32SeiCnt; //just used to initial the cnt when StrmStart()
        else
            tmp = vcenc_instance->stream.byteCnt;

        if (IS_HEVC(vcenc_instance->codecFormat))
            HevcNalUnitHdr(&vcenc_instance->stream, PREFIX_SEI_NUT, byte_stream);
        else if (IS_H264(vcenc_instance->codecFormat))
            H264NalUnitHdr(&vcenc_instance->stream, 0, H264_SEI, byte_stream);

        HevcContentLightLevelSei(&vcenc_instance->stream, &vcenc_instance->Hdr10LightLevel);
        if (enc_status)
            nal_size = u32SeiCnt;
        else
            nal_size = vcenc_instance->stream.byteCnt - tmp;

        pEncOut->streamSize += nal_size;
        VCEncAddNaluSize(pEncOut, nal_size, 0);
        pEncOut->PreDataSize += nal_size;
        pEncOut->PreNaluNum++;
        hash(&vcenc_instance->hashctx, vcenc_instance->stream.stream, nal_size);
    }
}

/* Some params for different quality config and different HW version */
void VideoTuneConfig(const VCEncConfig *config, struct vcenc_instance *vcenc_inst)
{
    if (!config || !vcenc_inst)
        return;
    regValues_s *regs = &(vcenc_inst->asic.regs);
    vcencRateControl_s *rc = &(vcenc_inst->rateControl);

    vcenc_inst->cuTreeCtl.aqStrength = 1.0;
    vcenc_inst->cuTreeCtl.aq_mode = 0;
    vcenc_inst->psyFactor = 0.0;
    rc->IFrameQpVisualTuning = rc->pass1EstCostAll = rc->visualLmdTuning = rc->initialQpTuning = 0;
    if (regs->asicCfg.tuneToolsSet2Support) {
#ifdef RDO_CHECK_ZERO_TU
        regs->bRdoCheckChromaZeroTu = 1;
#endif
#ifdef REFINE_MEXN_BINCOST
        regs->lambdaCostScaleMExN[0] = 2;
        regs->lambdaCostScaleMExN[1] = 1;
        regs->lambdaCostScaleMExN[2] = 0;
#endif
    }

    switch (config->tune) {
    case VCENC_TUNE_PSNR:
#ifdef HEVC_QUALITY_LA_TUNE
        if (regs->asicCfg.tuneToolsSet2Support) {
            /* currently these tunings are only applied to 2-pass encoding including 1st and 2nd pass. */
            if (config->pass) {
                rc->pass1EstCostAll = rc->visualLmdTuning = 1;
                rc->initialQpTuning = FIRST_I_TUNE;

                if (config->pass == 1) {
                    regs->RefFrameUsingInputFrameEnable = 1;
                    regs->InLoopDSBilinearEnable = 1;
                    regs->H264Intramode4x4Disable = 1;
                    regs->H264Intramode8x8Disable = 1;
                    regs->HevcSimpleRdoAssign = 2;
                    regs->PredModeBySatdEnable = 1;
                }
            }
        }
#endif
        break;
    case VCENC_TUNE_SSIM:
        vcenc_inst->cuTreeCtl.aq_mode = 2;
        break;
    case VCENC_TUNE_VISUAL:
    case VCENC_TUNE_SHARP_VISUAL:
        vcenc_inst->cuTreeCtl.aq_mode = 2;
        rc->IFrameQpVisualTuning = rc->pass1EstCostAll = rc->visualLmdTuning = 1;

        /* PSY factor */
        if (regs->asicCfg.encPsyTuneSupport)
            vcenc_inst->psyFactor = 0.75;

        /* encoder mode decision tools tuning */
        if (regs->asicCfg.encVisualTuneSupport || regs->asicCfg.tuneToolsSet2Support) {
#ifdef H264_LA_INTRA_MODE_NON_4x4
            regs->H264Intramode4x4Disable = (config->pass == 1);
#endif
#ifdef H264_LA_INTRA_MODE_NON_8x8
            regs->H264Intramode8x8Disable = (config->pass == 1);
#endif
#ifdef LA_REFERENCE_USE_INPUT
            regs->RefFrameUsingInputFrameEnable = (config->pass == 1);
#endif
        }

        if (regs->asicCfg.tuneToolsSet2Support) {
#ifdef RDOQ_LAMBDA_ADJ
            vcenc_inst->bRdoqLambdaAdjust = 1;
#endif
#ifdef BILINEAR_DOWNSAMPLE
            regs->InLoopDSBilinearEnable = (config->pass == 1);
#endif
#ifdef LA_INTRA_BY_SATD
            regs->PredModeBySatdEnable = (config->pass == 1);
#endif
#ifdef LA_HEVC_SIMPLE_RDO
            if ((config->pass == 1) && !IS_H264(config->codecFormat))
                regs->HevcSimpleRdoAssign = 2;
#endif
        }
        break;
    default:
        break;
    }

    regs->psyFactor = (u32)(vcenc_inst->psyFactor * (1 << PSY_FACTOR_SCALE_BITS) + 0.5);
}

/*------------------------------------------------------------------------------
    Function name   : SetPictureCfgToJob
    Description     : set picture config to job' encIn
    Return type     : void
    Argument        : const VCEncIn* pEncIn              [in]
    Argument        : VCEncIn* pJobEncIn                 [out]
------------------------------------------------------------------------------*/
void SetPictureCfgToJob(const VCEncIn *pEncIn, VCEncIn *pJobEncIn, u8 gdrDuration)
{
    if (NULL == pEncIn || NULL == pJobEncIn)
        return;

    if (HANTRO_TRUE == pJobEncIn->bIsIDR && !gdrDuration) {
        pJobEncIn->codingType = VCENC_INTRA_FRAME;
        pJobEncIn->poc = 0;
    } else {
        pJobEncIn->codingType = pEncIn->codingType;
        pJobEncIn->poc = pEncIn->poc;
        pJobEncIn->bIsIDR = pEncIn->bIsIDR;
    }
    memcpy(&pJobEncIn->gopConfig, &pEncIn->gopConfig, sizeof(pJobEncIn->gopConfig));
    pJobEncIn->gopSize = pEncIn->gopSize;
    pJobEncIn->gopPicIdx = pEncIn->gopPicIdx;
    pJobEncIn->picture_cnt = pEncIn->picture_cnt;
    pJobEncIn->last_idr_picture_cnt = pEncIn->last_idr_picture_cnt;

    pJobEncIn->bIsPeriodUsingLTR = pEncIn->bIsPeriodUsingLTR;
    pJobEncIn->bIsPeriodUpdateLTR = pEncIn->bIsPeriodUpdateLTR;
    memcpy(&pJobEncIn->gopCurrPicConfig, &pEncIn->gopCurrPicConfig,
           sizeof(pJobEncIn->gopCurrPicConfig));
    memcpy(pJobEncIn->long_term_ref_pic, pEncIn->long_term_ref_pic,
           sizeof(pJobEncIn->long_term_ref_pic));
    memcpy(pJobEncIn->bLTR_used_by_cur, pEncIn->bLTR_used_by_cur,
           sizeof(pJobEncIn->bLTR_used_by_cur));
    memcpy(pJobEncIn->bLTR_need_update, pEncIn->bLTR_need_update,
           sizeof(pJobEncIn->bLTR_need_update));

    pJobEncIn->i8SpecialRpsIdx = pEncIn->i8SpecialRpsIdx;
    pJobEncIn->i8SpecialRpsIdx_next = pEncIn->i8SpecialRpsIdx_next;
    pJobEncIn->u8IdxEncodedAsLTR = pEncIn->u8IdxEncodedAsLTR;
}

#ifndef RATE_CONTROL_BUILD_SUPPORT
/* set qpHdr and fixedQp. */
bool_e VCEncInitQp(vcencRateControl_s *rc, u32 newStream)
{
    if (rc->qpHdr == (-1 << QP_FRACTIONAL_BITS))
        rc->qpHdr = QP_DEFAULT << QP_FRACTIONAL_BITS;
    rc->fixedQp = rc->qpHdr;
    return 0;
}

/* calculate qpHdr for current picture. */
void VCEncBeforeCQP(vcencRateControl_s *rc, u32 timeInc, u32 sliceType, bool use_ltr_cur,
                    struct sw_picture *pic)
{
    rc->sliceTypeCur = sliceType;

    if (rc->pass == 1) {
        // pass 1 uses constant qp10 to encode
        rc->qpHdr = CU_TREE_QP << QP_FRACTIONAL_BITS;
        return;
    }

    if (rc->rcMode == VCE_RC_CQP) {
        /* calculate qp */
        rc->qpHdr = rc->fixedQp;
        if (rc->sliceTypeCur == I_SLICE) {
            if (rc->fixedIntraQp)
                rc->qpHdr = rc->fixedIntraQp;
            else if (rc->sliceTypePrev != I_SLICE) {
                rc->qpHdr += rc->intraQpDelta;
            }
        } else {
            //B frm not referenced
            if (rc->picRc != ENCHW_YES)
                rc->qpHdr += rc->frameQpDelta;
            if (use_ltr_cur)
                rc->qpHdr += rc->longTermQpDelta;
            if (rc->qpHdr > (51 << QP_FRACTIONAL_BITS))
                rc->qpHdr = (51 << QP_FRACTIONAL_BITS);
        }

        /* quantization parameter user defined limitations */
        rc->qpHdr = MIN(rc->qpMax, MAX(rc->qpMin, rc->qpHdr));
        rc->sliceTypePrev = rc->sliceTypeCur;
    }
}
#endif

/*init coding ctrl parameter queue*/
void EncInitCodingCtrlQueue(VCEncInst inst)
{
    struct vcenc_instance *pEncInst = (struct vcenc_instance *)inst;
    if (NULL == pEncInst->codingCtrl.codingCtrlQueue.head) {
        //get buffer
        EncCodingCtrlParam *pEncCodingCtrlParam = (EncCodingCtrlParam *)DynamicGetBufferFromPool(
            &pEncInst->codingCtrl.codingCtrlBufPool, sizeof(EncCodingCtrlParam));
        if (!pEncCodingCtrlParam) {
            APITRACEWRN("EncInitCodingCtrlQueue: ERROR Get coding ctrl buffer failed\n");
            return;
        }
        VCEncCodingCtrl *pCodeParams = &pEncCodingCtrlParam->encCodingCtrl;

        //get coding ctrl parameters.
        VCEncGetCodingCtrl(inst, pCodeParams);

        /*save default parameters in codingctrl queue*/
        /*The newest parameter's refCnt need be realRefCnt+1,
      so that next frame can also use these parameters if no new parameters are set*/
        pEncCodingCtrlParam->refCnt = 1;
        pEncCodingCtrlParam->startPicCnt = -1;
        queue_put(&pEncInst->codingCtrl.codingCtrlQueue, (struct node *)pEncCodingCtrlParam);
    }

    return;
}

/* Disable AxiFe */
void VCEncAxiFeDisable(const void *ewl, void *data)
{
#ifdef SUPPORT_AXIFE
    AXIFE_REG_OUT regs;
    REG *reg = &(regs.registers[0]);

    memset(&regs, 0, sizeof(AXIFE_REG_OUT));
    DisableAxiFe(reg);
    EWLWriteBackRegbyClientType(ewl, regs.registers[10].offset, regs.registers[10].value,
                                EWL_CLIENT_TYPE_AXIFE);
#endif
}

/* Enable AxiFe */
void VCEncAxiFeEnable(void *inst, void *ewl, u32 tileId)
{
#ifdef SUPPORT_AXIFE
    u32 axiFeFuse;
    AXIFE_REG_OUT regs;
    struct AxiFeChns channelCfg;
    struct AxiFeCommonCfg commonCfg;
    u32 i;
    struct vcenc_instance *vcenc_instance = (struct vcenc_instance *)inst;
    asicData_s *asic = &vcenc_instance->asic;
    REG *reg = &(regs.registers[0]);
    struct ChnDesc *rdChannel = NULL;
    struct ChnDesc *wrChannel = NULL;

    if (vcenc_instance->axiFEEnable == 0)
        return;

    //cutree not support security mode
    if (ewl != NULL && vcenc_instance->axiFEEnable == 3)
        return;

    if (ewl == NULL)
        ewl = (void *)asic->ewl;

    memset(&regs, 0, sizeof(AXIFE_REG_OUT));
    if (vcenc_instance->axiFEEnable == 3) {
        memset(&channelCfg, 0, sizeof(struct AxiFeChns));
        memset(&commonCfg, 0, sizeof(struct AxiFeCommonCfg));
        axiFeFuse = EWLReadRegbyClientType(ewl, 0, EWL_CLIENT_TYPE_AXIFE);

        //example to config AXIFE
        channelCfg.nbr_rd_chns = 1;
        channelCfg.nbr_wr_chns = 1;
        rdChannel = EWLcalloc(1, sizeof(struct ChnDesc));
        wrChannel = EWLcalloc(1, sizeof(struct ChnDesc));

        rdChannel->sw_axi_base_addr_id = asic->regs.inputLumBase >> 32;
        rdChannel->sw_axi_start_addr = asic->regs.inputLumBase & 0xFFFFFFFF;
        rdChannel->sw_axi_end_addr =
            (asic->regs.inputLumBase +
             asic->regs.input_luma_stride * vcenc_instance->preProcess.lumHeightSrc[tileId]) &
            0xFFFFFFFF;
        rdChannel->sw_axi_user = 0;
        rdChannel->sw_axi_ns = 0;

        wrChannel->sw_axi_base_addr_id = asic->regs.inputLumBase >> 32;
        wrChannel->sw_axi_start_addr = asic->regs.inputLumBase & 0xFFFFFFFF;
        wrChannel->sw_axi_end_addr =
            (asic->regs.inputLumBase +
             asic->regs.input_luma_stride * vcenc_instance->preProcess.lumHeightSrc[tileId]) &
            0xFFFFFFFF;
        wrChannel->sw_axi_user = 0;
        wrChannel->sw_axi_ns = 0;

        channelCfg.rd_channels = rdChannel;
        channelCfg.wr_channels = wrChannel;

        ConfigAxiFeChns(reg, axiFeFuse, &channelCfg);

        commonCfg.sw_secure_mode = 1;
        commonCfg.sw_axi_user_mode = 3;
        commonCfg.sw_axi_prot_mode = 1;
        ConfigAxiFe(reg, &commonCfg);
    }

    EnableAxiFe(reg, vcenc_instance->axiFEEnable == 2);
    //write registers to AXIFE
    for (i = 0; i < AXIFE_REG_NUM; i++) {
        if (regs.registers[i].flag && i != 10)
            EWLWriteRegbyClientType(ewl, regs.registers[i].offset, regs.registers[i].value,
                                    EWL_CLIENT_TYPE_AXIFE);
    }

    //finally set enable register
    EWLWriteRegbyClientType(ewl, regs.registers[10].offset, regs.registers[10].value,
                            EWL_CLIENT_TYPE_AXIFE);

    for (i = 0; i < AXIFE_REG_NUM; i++) {
        if (regs.registers[i].flag && i != 10)
            EWLWriteRegbyClientType(ewl, regs.registers[i].offset, regs.registers[i].value,
                                    EWL_CLIENT_TYPE_AXIFE_1);
    }

    //finally set enable register
    EWLWriteRegbyClientType(ewl, regs.registers[10].offset, regs.registers[10].value,
                            EWL_CLIENT_TYPE_AXIFE_1);

    if (rdChannel)
        VCENC_FREE(rdChannel);

    if (wrChannel)
        VCENC_FREE(wrChannel);

#endif
}

/* Set ApbFilter */
void VCEncSetApbFilter(void *inst)
{
#ifdef SUPPORT_APBFT
    ApbFilterREG *regs;
    struct ApbFilterCfg cfg;
    struct ApbFilterMask *mask;
    struct vcenc_instance *vcenc_instance = (struct vcenc_instance *)inst;
    asicData_s *asic = &vcenc_instance->asic;
    u32 i;
    u32 protect_reg_size = 400; //for VC8000E, only 400 regs will be protected

    //example to config ApbFilter
    regs = EWLcalloc(protect_reg_size, sizeof(ApbFilterREG));
    mask = EWLcalloc(protect_reg_size, sizeof(struct ApbFilterMask));

    for (i = 0; i < protect_reg_size; i++) {
        mask[i].page_idx = 0;
        mask[i].reg_idx = i;
        mask[i].value = 0;
    }
    cfg.reg_mask = mask;
    cfg.num_regs = protect_reg_size;

    ConfigApbFilter(regs, &cfg);

    //write registers to AXIFE
    for (i = 0; i < protect_reg_size; i++) {
        if (regs[i].flag)
            EWLWriteRegbyClientType(asic->ewl, regs[i].offset, regs[i].value,
                                    EWL_CLIENT_TYPE_APBFT);
    }

    //set page select
    SelApbFilterPage(&regs[0], 0, protect_reg_size * 4);
    EWLWriteRegbyClientType(asic->ewl, regs[0].offset, regs[0].value, EWL_CLIENT_TYPE_APBFT);

    EWLfree(regs);
    EWLfree(mask);
#endif
}

/* Set waitJobCfg */
void VCEncSetwaitJobCfg(VCEncIn *pEncIn, struct vcenc_instance *vcenc_instance, asicData_s *asic,
                        u32 waitCoreJobid)
{
    EWLWaitJobCfg_t waitJobCfg;
    CacheData_t cache_data;
    memset(&waitJobCfg, 0, sizeof(EWLWaitJobCfg_t));
    waitJobCfg.waitCoreJobid = waitCoreJobid;
    waitJobCfg.dec400_enable = pEncIn->dec400Enable;
    waitJobCfg.dec400_data = NULL;
    waitJobCfg.axife_enable = pEncIn->axiFEEnable;
    waitJobCfg.axife_callback = VCEncAxiFeDisable;
#ifdef SUPPORT_CACHE
    waitJobCfg.l2cache_enable = 1;
#endif
    cache_data.cache = &vcenc_instance->cache;
    waitJobCfg.l2cache_data = &cache_data;
    waitJobCfg.l2cache_callback = DisableCache;

    EWLEnqueueWaitjob(asic->ewl, &waitJobCfg);
}

/* SetDec400 */
VCEncRet VCEncSetDec400(VCEncIn *pEncIn, VCDec400data *dec400_data, VCDec400data *dec400_osd,
                        asicData_s *asic)
{
    //DEC400
    if (pEncIn->dec400Enable == 2 && VCEncEnableDec400(dec400_data) == VCENC_INVALID_ARGUMENT) {
        APITRACEERR("VCEncStrmEncode: DEC400 doesn't exist or format not supported\n");
        EWLReleaseHw(asic->ewl);
        return VCENC_INVALID_ARGUMENT;
    } else if (pEncIn->dec400Enable == 1) {
        APITRACE("VCEncStrmEncode: DEC400 set stream bypass\n");
        asic->regs.AXI_ENC_WR_ID_E = 1;
        asic->regs.AXI_ENC_RD_ID_E = 1;
        VCEncSetDec400StreamBypass(dec400_data);
    }

    //OSD dec400
    if (pEncIn->osdDec400Enable && VCEncEnableDec400(dec400_osd) == VCENC_INVALID_ARGUMENT) {
        APITRACEERR("VCEncStrmEncode: osd DEC400 error\n\n");
        EWLReleaseHw(asic->ewl);
        return VCENC_INVALID_ARGUMENT;
    }
    return VCENC_OK;
}

/* enable DEC400 to get input data */ //TODO, rename and optimize, cfgDec400
void VCEncCfgDec400(VCEncIn *pEncIn, VCDec400data *dec400_data, VCDec400data *dec400_osd,
                    struct vcenc_instance *vcenc_instance, asicData_s *asic, u32 tileId)
{
    memset(dec400_data, 0, sizeof(VCDec400data));
    memset(dec400_osd, 0, sizeof(VCDec400data));

    dec400_data->format = vcenc_instance->preProcess.inputFormat;
    dec400_data->lumWidthSrc = vcenc_instance->preProcess.lumWidthSrc[tileId];
    dec400_data->lumHeightSrc = vcenc_instance->preProcess.lumHeightSrc[tileId];
    dec400_data->input_alignment = vcenc_instance->preProcess.input_alignment;
    dec400_data->dec400LumTableBase = pEncIn->dec400LumTableBase;
    dec400_data->dec400CbTableBase = pEncIn->dec400CbTableBase;
    dec400_data->dec400CrTableBase = pEncIn->dec400CrTableBase;
    dec400_data->dec400Enable = pEncIn->dec400Enable;

    dec400_data->wlInstance = asic->ewl;
    dec400_data->regVcmdBuf = asic->regs.vcmdBuf;
    dec400_data->regVcmdBufSize = &asic->regs.vcmdBufSize;
    dec400_data->dec400CrDataBase = asic->regs.inputCrBase;
    dec400_data->dec400CbDataBase = asic->regs.inputCbBase;
    dec400_data->dec400LumDataBase = asic->regs.inputLumBase;

    /* enable DEC400 to get decompressed input data */
    if (pEncIn->osdDec400Enable) {
        dec400_osd->format = VCENC_RGB888;
        dec400_osd->lumWidthSrc = vcenc_instance->preProcess.overlayYStride[0];
        dec400_osd->lumHeightSrc = vcenc_instance->preProcess.overlayHeight[0];
        dec400_osd->input_alignment = vcenc_instance->preProcess.input_alignment;
        dec400_osd->dec400LumTableBase = pEncIn->osdDec400TableBase;
        dec400_osd->dec400CbTableBase = 0;
        dec400_osd->dec400CrTableBase = 0;
        dec400_osd->dec400Enable = 0;

        dec400_osd->wlInstance = asic->ewl;
        dec400_osd->regVcmdBuf = asic->regs.vcmdBuf;
        dec400_osd->regVcmdBufSize = &asic->regs.vcmdBufSize;
        dec400_osd->dec400CrDataBase = asic->regs.overlayVAddr[0];
        dec400_osd->dec400CbDataBase = asic->regs.overlayUAddr[0];
        dec400_osd->dec400LumDataBase = asic->regs.overlayYAddr[0];

        dec400_osd->osdInputSuperTile = vcenc_instance->preProcess.overlaySuperTile[0];
    }
}

/* Enable Subsystem L2Cache/dec400/AXIFE/APBFilter */
VCEncRet VCEncSetSubsystem(struct vcenc_instance *vcenc_instance, VCEncIn *pEncIn, asicData_s *asic,
                           VCDec400data *dec400_data, VCDec400data *dec400_osd,
                           struct sw_picture *pic, u32 tileId)
{
    //L2CACHE
    if (EnableCache(vcenc_instance, asic, pic, tileId) != VCENC_OK) {
        APITRACEERR("Encoder enable cache failed!!\n");
        return VCENC_ERROR;
    }

    //dec400
    if (VCEncSetDec400(pEncIn, dec400_data, dec400_osd, asic) != VCENC_OK)
        return VCENC_INVALID_ARGUMENT;

    //AXIFE
    if (pEncIn->axiFEEnable) {
        vcenc_instance->axiFEEnable = pEncIn->axiFEEnable;
        VCEncAxiFeEnable((void *)vcenc_instance, NULL, tileId);
    }

    //APBfilter
    if (pEncIn->apbFTEnable) {
        VCEncSetApbFilter((void *)vcenc_instance);
    }
    return VCENC_OK;
}

/* Reset callback struct */
void VCEncResetCallback(VCEncSliceReady *slice_callback, struct vcenc_instance *vcenc_instance,
                        VCEncIn *pEncIn, VCEncOut *pEncOut, u32 next_core_index, u32 multicore_flag)
{
    slice_callback->slicesReadyPrev = 0;
    slice_callback->slicesReady = 0;
    slice_callback->sliceSizes =
        (u32 *)vcenc_instance->asic.sizeTbl[next_core_index].virtualAddress;
    //slice_callback.sliceSizes += vcenc_instance->numNalus;
    slice_callback->nalUnitInfoNum = vcenc_instance->numNalus[next_core_index];
    slice_callback->nalUnitInfoNumPrev = 0;
    slice_callback->streamBufs = vcenc_instance->streamBufs[next_core_index];
    slice_callback->pAppData = vcenc_instance->pAppData;
    for (i32 i = 0; i < MAX_STRM_BUF_NUM; i++) {
        slice_callback->outbufMem[i] = (EWLLinearMem_t *)pEncIn->cur_out_buffer[i];
    }
    if (!multicore_flag) //MultiCorFlush do not set parameters
    {
        slice_callback->PreNaluNum = pEncOut->PreNaluNum;
        slice_callback->PreDataSize = pEncOut->PreDataSize;
    }
}

/*set RingBuffer registers */
void VCEncSetRingBuffer(struct vcenc_instance *vcenc_instance, asicData_s *asic,
                        struct sw_picture *pic)
{
    if (vcenc_instance->refRingBufEnable) {
        asic->regs.refRingBufEnable = 1;
        asic->regs.pRefRingBuf_base = asic->RefRingBuf.busAddress;

        //each block buffer size
        u32 lumaBufSize = asic->regs.refRingBuf_luma_size;
        u32 chromaBufSize = asic->regs.refRingBuf_chroma_size;
        u32 lumaBufSize4N = asic->regs.refRingBuf_4n_size;
        u32 chromaHalfSize = asic->regs.recon_chroma_half_size;

        //each block base address offset from RingBuffer base address.
        u32 lumaBaseAddress = asic->RefRingBuf.busAddress;
        u32 chromaBaseAddress = asic->RefRingBuf.busAddress + lumaBufSize;
        u32 luma4NBaseAddress = asic->RefRingBuf.busAddress + lumaBufSize + chromaBufSize;

        if (pic->rpl[0][0] == NULL) //I slice  CHECK //pic->sliceInst->type == I_SLICE
        {
            pic->recon.lum = asic->RefRingBuf.busAddress;
            pic->recon.cb = asic->RefRingBuf.busAddress + lumaBufSize;
            pic->recon.cr = pic->recon.cb + chromaHalfSize;
            pic->recon_4n_base = pic->recon.cb + chromaBufSize;
        } else {
            //reference data reading offset from each block base address.
            u32 lumaReadOffset = pic->rpl[0][0]->recon.lum - lumaBaseAddress;
            u32 cbReadOffset = pic->rpl[0][0]->recon.cb - chromaBaseAddress;
            u32 crReadOffset = pic->rpl[0][0]->recon.cr - chromaBaseAddress;
            u32 luma4NReadOffset = pic->rpl[0][0]->recon_4n_base - luma4NBaseAddress;

            asic->regs.refRingBuf_luma_rd_offset = lumaReadOffset;
            asic->regs.refRingBuf_chroma_rd_offset = cbReadOffset;
            asic->regs.refRingBuf_4n_rd_offset = luma4NReadOffset;

            u32 width, width_4n, height;
            width = ((vcenc_instance->width + 63) >> 6) << 6;
            width_4n = ((vcenc_instance->width + 15) / 16) * 4;

            //actual luma/chroma/4N size
            u32 lumaSize = lumaBufSize -
                           vcenc_instance->RefRingBufExtendHeight * asic->regs.ref_frame_stride / 4;
            u32 chromaSize = chromaBufSize - vcenc_instance->RefRingBufExtendHeight / 2 *
                                                 asic->regs.ref_frame_stride_ch / 4;
            u32 lumaSize4N = lumaBufSize4N - vcenc_instance->RefRingBufExtendHeight / 4 *
                                                 asic->regs.ref_ds_luma_stride / 4;

            //update recon address
            if (lumaReadOffset + lumaSize < lumaBufSize)
                pic->recon.lum = pic->rpl[0][0]->recon.lum + lumaSize;
            else
                pic->recon.lum = pic->rpl[0][0]->recon.lum + lumaSize - lumaBufSize;

            if (cbReadOffset + chromaSize < chromaBufSize)
                pic->recon.cb = pic->rpl[0][0]->recon.cb + chromaSize;
            else
                pic->recon.cb = pic->rpl[0][0]->recon.cb + chromaSize - chromaBufSize;

            if (crReadOffset + chromaSize < chromaBufSize)
                pic->recon.cr = pic->rpl[0][0]->recon.cr + chromaSize;
            else
                pic->recon.cr = pic->rpl[0][0]->recon.cr + chromaSize - chromaBufSize;

            if (luma4NReadOffset + lumaSize4N < lumaBufSize4N)
                pic->recon_4n_base = pic->rpl[0][0]->recon_4n_base + lumaSize4N;
            else
                pic->recon_4n_base = pic->rpl[0][0]->recon_4n_base + lumaSize4N - lumaBufSize4N;
        }
        //recon data writing offset from each block base address.
        asic->regs.refRingBuf_luma_wr_offset = pic->recon.lum - lumaBaseAddress;
        asic->regs.refRingBuf_chroma_wr_offset = pic->recon.cb - chromaBaseAddress;
        asic->regs.refRingBuf_4n_wr_offset = pic->recon_4n_base - luma4NBaseAddress;
    }
}
