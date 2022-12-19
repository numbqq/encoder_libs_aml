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
--  Abstract : VC Encoder API
--
------------------------------------------------------------------------------*/

/**
 * \addtogroup api_video
 *
 * @{
 */
#ifdef CUINFO_BUILD_SUPPORT

/* Includes */

#include <math.h> /* for pow */
#include "vsi_string.h"
#include "osal.h"

#include "vc8000e_version.h"

#include "hevcencapi.h"

#include "base_type.h"
#include "error.h"
#include "instance.h"
#include "sw_parameter_set.h"
#include "rate_control_picture.h"
#include "sw_slice.h"
#include "tools.h"
#include "enccommon.h"
#include "hevcApiVersion.h"
#include "enccfg.h"
#include "hevcenccache.h"
#include "encdec400.h"
#include "enc_log.h"

#include "sw_put_bits.h"

#ifdef SUPPORT_AV1
#include "av1_prob_init.h"
#endif

#ifdef SUPPORT_VP9
#include "vp9encapi.h"
#endif

#ifdef INTERNAL_TEST
#include "sw_test_id.h"
#endif

#ifdef TEST_DATA
#include "enctrace.h"
#endif

/* External compiler flags */

/* Module defines */
/* Tracing macro */

#define APITRACEERR(fmt, ...)                                                                      \
    VCEncTraceMsg(NULL, VCENC_LOG_ERROR, VCENC_LOG_TRACE_API, "[%s:%d]" fmt, __FUNCTION__,         \
                  __LINE__, ##__VA_ARGS__)
#define APITRACE(fmt, ...)                                                                         \
    VCEncTraceMsg(NULL, VCENC_LOG_INFO, VCENC_LOG_TRACE_API, fmt, ##__VA_ARGS__)
#define APITRACEPARAM_X(fmt, ...)                                                                  \
    VCEncTraceMsg(NULL, VCENC_LOG_INFO, VCENC_LOG_TRACE_API, fmt, ##__VA_ARGS__)
#define APITRACEPARAM(fmt, ...)                                                                    \
    VCEncTraceMsg(NULL, VCENC_LOG_INFO, VCENC_LOG_TRACE_API, fmt, ##__VA_ARGS__)
#define APITRACE_I(inst, fmt, ...)                                                                 \
    VCEncTraceMsg(inst, VCENC_LOG_INFO, VCENC_LOG_TRACE_API, fmt, ##__VA_ARGS__)
/* Local function prototypes */

/*
 * Parse Cu Information Format 0 in Version 0
 */
static VCEncRet VCEncGetCuInfoV0(bits_buffer_s *b, VCEncCuInfo *pEncCuInfo,
                                 VCEncVideoCodecFormat codecFormat)
{
    i32 i;
    if (!IS_H264(codecFormat)) {
        pEncCuInfo->cuLocationY = get_value(b, CU_INFO_LOC_Y_BITS, HANTRO_FALSE) * 8;
        pEncCuInfo->cuLocationX = get_value(b, CU_INFO_LOC_X_BITS, HANTRO_FALSE) * 8;
        pEncCuInfo->cuSize = (1 << (get_value(b, CU_INFO_SIZE_BITS, HANTRO_FALSE) + 3));
    } else {
        pEncCuInfo->cuSize = 16;
    }

    pEncCuInfo->cuMode = get_value(b, CU_INFO_MODE_BITS, HANTRO_FALSE);

    pEncCuInfo->cost = get_value(b, CU_INFO_COST_BITS, HANTRO_FALSE);

    if (pEncCuInfo->cuMode == 0) {
        /* inter: 2+2*(2+14+14)=62 bits */
        pEncCuInfo->interPredIdc = get_value(b, CU_INFO_INTER_PRED_IDC_BITS, HANTRO_FALSE);
        for (i = 0; i < 2; i++) {
            pEncCuInfo->mv[i].refIdx = get_value(b, CU_INFO_MV_REF_IDX_BITS, HANTRO_FALSE);
            pEncCuInfo->mv[i].mvX = get_value(b, CU_INFO_MV_X_BITS, HANTRO_TRUE);
            pEncCuInfo->mv[i].mvY = get_value(b, CU_INFO_MV_Y_BITS, HANTRO_TRUE);
        }
    } else {
        /* for h264: 2+16*4 = 68 bits
     * for hevc: 1+ 4*6 = 25 bits
     */
        pEncCuInfo->intraPartMode = get_value(
            b, IS_H264(codecFormat) ? CU_INFO_INTRA_PART_BITS_H264 : CU_INFO_INTRA_PART_BITS,
            HANTRO_FALSE);
        for (i = 0; i < (IS_H264(codecFormat) ? 16 : 4); i++)
            pEncCuInfo->intraPredMode[i] =
                get_value(b,
                          IS_H264(codecFormat) ? CU_INFO_INTRA_PRED_MODE_BITS_H264
                                               : CU_INFO_INTRA_PRED_MODE_BITS,
                          HANTRO_FALSE);
        if (pEncCuInfo->intraPredMode[0] == (IS_H264(codecFormat) ? 15 : 63))
            pEncCuInfo->cuMode = 2;
    }

    return VCENC_OK;
}

/*
 * Parse Cu Information Format 1 in Version 1
 */
static VCEncRet VCEncGetCuInfoV1(bits_buffer_s *b, VCEncCuInfo *pEncCuInfo,
                                 VCEncVideoCodecFormat codecFormat)
{
    VCEncGetCuInfoV0(b, pEncCuInfo, codecFormat);

    get_align(b, CU_INFO_OUTPUT_SIZE);

    pEncCuInfo->mean = get_value(b, CU_INFO_MEAN_BITS, HANTRO_FALSE);
    pEncCuInfo->variance = get_value(b, CU_INFO_VAR_BITS, HANTRO_FALSE);
    pEncCuInfo->qp = get_value(b, CU_INFO_QP_BITS, HANTRO_FALSE);
    pEncCuInfo->costOfOtherMode = get_value(b, CU_INFO_COST_BITS, HANTRO_FALSE);
    pEncCuInfo->costIntraSatd = get_value(b, CU_INFO_COST_BITS, HANTRO_FALSE);
    pEncCuInfo->costInterSatd = get_value(b, CU_INFO_COST_BITS, HANTRO_FALSE);
    return VCENC_OK;
}

/*
 * Parse Cu Information Format 2 in Version 1
 */
static VCEncRet VCEncGetCuInfoV2(bits_buffer_s *b, VCEncCuInfo *pEncCuInfo,
                                 VCEncVideoCodecFormat codecFormat)
{
    i32 i;

    pEncCuInfo->cuLocationY = get_value(b, CU_INFO_LOC_Y_BITS, HANTRO_FALSE) * 8;
    pEncCuInfo->cuLocationX = get_value(b, CU_INFO_LOC_X_BITS, HANTRO_FALSE) * 8;
    pEncCuInfo->cuSize = (1 << (get_value(b, CU_INFO_SIZE_BITS, HANTRO_FALSE) + 3));
    if (IS_H264(codecFormat)) {
        pEncCuInfo->cuSize = 16;
    }

    pEncCuInfo->cuMode = get_value(b, CU_INFO_MODE_BITS, HANTRO_FALSE);

    if (pEncCuInfo->cuMode == 0) {
        /* inter: 2+2*(2+14+14)=62 bits */
        pEncCuInfo->interPredIdc = get_value(b, CU_INFO_INTER_PRED_IDC_BITS, HANTRO_FALSE);
        for (i = 0; i < 2; i++) {
            pEncCuInfo->mv[i].refIdx = get_value(b, CU_INFO_MV_REF_IDX_BITS, HANTRO_FALSE);
            pEncCuInfo->mv[i].mvX = get_value(b, CU_INFO_MV_X_BITS, HANTRO_TRUE);
            pEncCuInfo->mv[i].mvY = get_value(b, CU_INFO_MV_Y_BITS, HANTRO_TRUE);
        }
    } else {
        get_value(b, 32, HANTRO_FALSE);
        get_value(b, 30, HANTRO_FALSE);
    }

    pEncCuInfo->costIntraSatd = get_value(b, CU_INFO_COST_BITS, HANTRO_FALSE);
    pEncCuInfo->costInterSatd = get_value(b, CU_INFO_COST_BITS, HANTRO_FALSE);

    return VCENC_OK;
}

/*
 * Parse Cu Information Format 3 in Version 1
 */
static VCEncRet VCEncGetCuInfoV3(bits_buffer_s *b, VCEncCuInfo *pEncCuInfo,
                                 VCEncVideoCodecFormat codecFormat)
{
    VCEncGetCuInfoV2(b, pEncCuInfo, codecFormat);
    pEncCuInfo->variance = get_value(b, CU_INFO_VAR_BITS_V3, HANTRO_FALSE);

    return VCENC_OK;
}

/*
 * Get num CUs, CuX & CuY based on ctuNum & cuNum for cuInforVersion=2
 */
static int getCuNumLocationV2(struct vcenc_instance *pEncInst, u32 ctuNum, u32 cuNum, u32 *cuX,
                              u32 *cuY)
{
    u32 nCuInCtuX, nCuInCtuY;
    nCuInCtuX = nCuInCtuY = (pEncInst->max_cu_size / 16);
    /* cuInfo version 2 is 16x16 aligned */
    if (ctuNum % pEncInst->ctbPerRow == pEncInst->ctbPerRow - 1)
        nCuInCtuX = ((pEncInst->width + 15) / 16 - 1) % (pEncInst->max_cu_size / 16) + 1;
    if (ctuNum / pEncInst->ctbPerRow == pEncInst->ctbPerCol - 1)
        nCuInCtuY = ((pEncInst->height + 15) / 16 - 1) % (pEncInst->max_cu_size / 16) + 1;
    if (cuX)
        *cuX = ctuNum % pEncInst->ctbPerRow * (pEncInst->max_cu_size / 16) + cuNum % nCuInCtuX;
    if (cuY)
        *cuY = ctuNum / pEncInst->ctbPerRow * (pEncInst->max_cu_size / 16) + cuNum / nCuInCtuX;
    return nCuInCtuX * nCuInCtuY;
}

/**
 * \brief Get the encoding information of a CU in a CTU
 *
 * \param [in] inst - encoder instance
 * \param [in]  pEncCuOutData - structure containing ctu table and cu information stream output by HW
 * \param [in]  ctuNum - ctu number within picture
 * \param [in]  cuNum - cu number within ctu
 * \param [out]  pEncCuInfo - structure returns the parsed cu information
 * \return VCENC_INVALID_ARGUMENT invalid argument. e.g. CU number is beyond the boundary of given CTU.
 * \return VCENC_ERROR error version number for cu information
 * \return VCENC_OK success
 */
VCEncRet VCEncGetCuInfo(VCEncInst inst, VCEncCuOutData *pEncCuOutData, u32 ctuNum, u32 cuNum,
                        VCEncCuInfo *pEncCuInfo)
{
    struct vcenc_instance *pEncInst = (struct vcenc_instance *)inst;
    u32 nCuInCtu;
    u32 *ctuTable;
    u8 *cuStream;
    bits_buffer_s b;
    i32 version, infoSize;

    APITRACE("VCEncGetCuInfo#\n");

    /* Check for illegal inputs */
    if (!pEncInst || !pEncCuOutData || !pEncCuInfo) {
        APITRACEERR("VCEncGetCuInfo: ERROR Null argument\n");
        return VCENC_INVALID_ARGUMENT;
    }

    if ((i32)ctuNum >= pEncInst->ctbPerFrame) {
        APITRACEERR("VCEncGetCuInfo: ERROR Invalid ctu number\n");
        return VCENC_INVALID_ARGUMENT;
    }

    version = pEncInst->asic.regs.cuInfoVersion;

    switch (version) {
    case 0:
        infoSize = CU_INFO_OUTPUT_SIZE;
        break;
    case 1:
        infoSize = CU_INFO_OUTPUT_SIZE_V1;
        break;
    case 2:
        infoSize = CU_INFO_OUTPUT_SIZE_V2;
        break;
    case 3:
        infoSize = CU_INFO_OUTPUT_SIZE_V3;
        break;
    default:
        APITRACEERR("VCEncGetCuInfo: ERROR format version.\n");
        return VCENC_INVALID_ARGUMENT;
    }

    ctuTable = pEncCuOutData->ctuOffset;
    cuStream = pEncCuOutData->cuData;
    if (!ctuTable || !cuStream) {
        APITRACEERR("VCEncGetCuInfo: ERROR Null argument\n");
        return VCENC_INVALID_ARGUMENT;
    }

    nCuInCtu = ctuTable[ctuNum];
    if (ctuNum) {
        nCuInCtu -= ctuTable[ctuNum - 1];
        cuStream += ctuTable[ctuNum - 1] * infoSize;
    }
    if (IS_H264(pEncInst->codecFormat)) {
        nCuInCtu = 1;
        cuStream = pEncCuOutData->cuData + ctuNum * infoSize;
    }
    if (version == 2) {
        u32 cuX, cuY;
        nCuInCtu = getCuNumLocationV2(pEncInst, ctuNum, cuNum, &cuX, &cuY);
        cuStream = pEncCuOutData->cuData + (cuX + cuY * ((pEncInst->width + 15) / 16)) * infoSize;
    }
    if (cuNum >= nCuInCtu) {
        APITRACEERR("VCEncGetCuInfo: ERROR CU number is beyond the boundary of given "
                    "CTU\n");
        return VCENC_INVALID_ARGUMENT;
    }

    /* clear infor */
    memset(pEncCuInfo, 0, sizeof(VCEncCuInfo));

    if (version != 2)
        cuStream += cuNum * infoSize;
    b.cache = 0;
    b.bit_cnt = 0;
    b.stream = cuStream;
    b.accu_bits = 0;

    switch (version) {
    case 0:
        VCEncGetCuInfoV0(&b, pEncCuInfo, pEncInst->codecFormat);
        break;
    case 1:
        VCEncGetCuInfoV1(&b, pEncCuInfo, pEncInst->codecFormat);
        break;
    case 2:
        VCEncGetCuInfoV2(&b, pEncCuInfo, pEncInst->codecFormat);
        break;
    case 3:
        VCEncGetCuInfoV3(&b, pEncCuInfo, pEncInst->codecFormat);
        break;
    default:
        printf("Unknown CU Information Format %d\n", version);
        ASSERT(0);
        return VCENC_ERROR;
    }

    return VCENC_OK;
}

/** @} */
#endif
