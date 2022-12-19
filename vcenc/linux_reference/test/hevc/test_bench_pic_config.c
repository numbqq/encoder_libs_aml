
#include <math.h>

#include "vsi_string.h"
#include "base_type.h"
#include "hevcencapi.h"
#include "HevcTestBench.h"
#include "test_bench_utils.h"
//#include "instance.h"

#include "test_bench_pic_config.h"

/*------------------------------------------------------------------------------
Function name : InitPicConfig
Description   : initial pic reference configure
Return type   : void
Argument      : VCEncIn *pEncIn
------------------------------------------------------------------------------*/
void InitPicConfig(VCEncIn *pEncIn, struct test_bench *tb, commandLine_s *cml)
{
    i32 i, j, k, i32Poc;
    i32 i32MaxpicOrderCntLsb = 1 << 16;

    ASSERT(pEncIn != NULL);

    pEncIn->gopCurrPicConfig.codingType = FRAME_TYPE_RESERVED;
    pEncIn->gopCurrPicConfig.nonReference = FRAME_TYPE_RESERVED;
    pEncIn->gopCurrPicConfig.numRefPics = NUMREFPICS_RESERVED;
    pEncIn->gopCurrPicConfig.poc = -1;
    pEncIn->gopCurrPicConfig.QpFactor = QPFACTOR_RESERVED;
    pEncIn->gopCurrPicConfig.QpOffset = QPOFFSET_RESERVED;
    pEncIn->gopCurrPicConfig.temporalId = 0;
    pEncIn->i8SpecialRpsIdx = -1;
    for (k = 0; k < VCENC_MAX_REF_FRAMES; k++) {
        pEncIn->gopCurrPicConfig.refPics[k].ref_pic = INVALITED_POC;
        pEncIn->gopCurrPicConfig.refPics[k].used_by_cur = 0;
    }

    for (k = 0; k < VCENC_MAX_LT_REF_FRAMES; k++)
        pEncIn->long_term_ref_pic[k] = INVALITED_POC;

    pEncIn->bIsPeriodUsingLTR = HANTRO_FALSE;
    pEncIn->bIsPeriodUpdateLTR = HANTRO_FALSE;

    for (i = 0; i < pEncIn->gopConfig.special_size; i++) {
        if (pEncIn->gopConfig.pGopPicSpecialCfg[i].i32Interval <= 0)
            continue;

        if (pEncIn->gopConfig.pGopPicSpecialCfg[i].i32Ltr == 0)
            pEncIn->bIsPeriodUsingLTR = HANTRO_TRUE;
        else {
            pEncIn->bIsPeriodUpdateLTR = HANTRO_TRUE;

            for (k = 0; k < (i32)pEncIn->gopConfig.pGopPicSpecialCfg[i].numRefPics; k++) {
                i32 i32LTRIdx = pEncIn->gopConfig.pGopPicSpecialCfg[i].refPics[k].ref_pic;
                if ((IS_LONG_TERM_REF_DELTAPOC(i32LTRIdx)) &&
                    ((pEncIn->gopConfig.pGopPicSpecialCfg[i].i32Ltr - 1) ==
                     LONG_TERM_REF_DELTAPOC2ID(i32LTRIdx))) {
                    pEncIn->bIsPeriodUsingLTR = HANTRO_TRUE;
                }
            }
        }
    }

    memset(pEncIn->bLTR_need_update, 0, sizeof(u32) * VCENC_MAX_LT_REF_FRAMES);
    pEncIn->bIsIDR = HANTRO_TRUE;

    i32Poc = 0;
    /* check current picture encoded as LTR*/
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

    pEncIn->timeIncrement = 0;
    pEncIn->vui_timing_info_enable = cml->vui_timing_info_enable;
    if (cml->hrdConformance)
        pEncIn->vui_timing_info_enable = 1;
    pEncIn->hashType = cml->hashtype;
    pEncIn->poc = 0;
    pEncIn->last_idr_picture_cnt = pEncIn->picture_cnt = 0;
}
