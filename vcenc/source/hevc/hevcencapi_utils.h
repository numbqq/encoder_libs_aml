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
-- Abstract : Hevc API utils Messages.
--
------------------------------------------------------------------------------*/
#ifndef API_UTILS_H
#define API_UTILS_H

#include "instance.h"

#define VCENC_SW_BUILD                                                                             \
    ((VCENC_BUILD_MAJOR * 1000000) + (VCENC_BUILD_MINOR * 1000) + VCENC_BUILD_REVISION)

#define PRM_SET_BUFF_SIZE 1024            /* Parameter sets max buffer size */
#define RPS_SET_BUFF_SIZE 140             /* rps sets max buffer size */
#define VCENC_MAX_BITRATE (800000 * 1000) /* Level 6.2 high tier limit */

/* HW ID check. VCEncInit() will fail if HW doesn't match. */

#define VCENC_MIN_ENC_WIDTH 64 //132     /* 144 - 12 pixels overfill */
#define VCENC_MAX_ENC_WIDTH 8192
#define VCENC_MIN_ENC_HEIGHT 64 //96
#define VCENC_MAX_ENC_HEIGHT 8192
#define VCENC_MAX_ENC_HEIGHT_EXT 8640
#define VCENC_HEVC_MAX_LEVEL VCENC_HEVC_LEVEL_6_2
#define VCENC_H264_MAX_LEVEL VCENC_H264_LEVEL_6_2

#define VCENC_DEFAULT_QP 26

#define VCENC_MAX_PP_INPUT_WIDTH 8192

#define VCENC_MAX_USER_DATA_SIZE 2048

#define VCENC_AV1_MAX_ENC_WIDTH 4096
#define VCENC_AV1_MAX_ENC_AREA (4096 * 2304)

#define VCENC_VP9_MAX_ENC_WIDTH 4096
#define VCENC_VP9_MAX_ENC_HEIGHT 8384
#define VCENC_VP9_MAX_ENC_AREA (4096 * 2176)

#define VCENC_BUS_ADDRESS_VALID(bus_address)                                                       \
    (((bus_address) != 0) /*&& \
                                              ((bus_address & 0x0F) == 0)*/)

#define VCENC_BUS_CH_ADDRESS_VALID(bus_address)                                                    \
    (((bus_address) != 0) /*&& \
                                              ((bus_address & 0x07) == 0)*/)

#define FIX_POINT_LAMBDA

#define LARGE_QPFACTOR_FOR_BIGQP

/* Tracing macro */
#define APITRACEERR(fmt, ...)                                                                      \
    VCEncTraceMsg(NULL, VCENC_LOG_ERROR, VCENC_LOG_TRACE_API, "[%s:%d]" fmt, __FUNCTION__,         \
                  __LINE__, ##__VA_ARGS__)
#define APITRACEWRN(fmt, ...)                                                                      \
    VCEncTraceMsg(NULL, VCENC_LOG_WARN, VCENC_LOG_TRACE_API, "[%s:%d]Warning: " fmt, __FUNCTION__, \
                  __LINE__, ##__VA_ARGS__)
#define APITRACE(fmt, ...)                                                                         \
    VCEncTraceMsg(NULL, VCENC_LOG_INFO, VCENC_LOG_TRACE_API, fmt, ##__VA_ARGS__)
#define APITRACEPARAM_X(fmt, ...)                                                                  \
    VCEncTraceMsg(NULL, VCENC_LOG_INFO, VCENC_LOG_TRACE_API, fmt, ##__VA_ARGS__)
#define APITRACEPARAM(fmt, ...)                                                                    \
    VCEncTraceMsg(NULL, VCENC_LOG_INFO, VCENC_LOG_TRACE_API, fmt, ##__VA_ARGS__)

#define APIPRINT(v, ...)                                                                           \
    {                                                                                              \
        if (v)                                                                                     \
            printf(__VA_ARGS__);                                                                   \
    }

#define HEVC_LEVEL_NUM 13
#define H264_LEVEL_NUM 20
#define AV1_LEVEL_NUM 24
#define VP9_LEVEL_NUM 14
#define AV1_VALID_MAX_LEVEL 15
#define VP9_VALID_MAX_LEVEL 10

#define HDR10_NOCFGED 0
#define HDR10_CFGED 1
#define HDR10_CODED 2

/*------------------------------------------------------------------------------
       Encoder API utils function prototypes
  ------------------------------------------------------------------------------*/
void VCEncShutdown(VCEncInst inst);
VCEncRet VCEncClear(VCEncInst inst);
VCEncRet VCEncStop(VCEncInst inst);
void GenNextPicConfig(VCEncIn *pEncIn, const u8 *gopCfgOffset, i32 i32LastPicPoc,
                      struct vcenc_instance *vcenc_instance);
u64 CalNextPic(VCEncGopConfig *cfg, int picture_cnt);
void StrmEncodeTraceEncInPara(VCEncIn *pEncIn, struct vcenc_instance *vcenc_instance);
VCEncRet StrmEncodeCheckPara(struct vcenc_instance *vcenc_instance, VCEncIn *pEncIn,
                             VCEncOut *pEncOut, asicData_s *asic, u32 client_type);
void StrmEncodeGlobalmvConfig(asicData_s *asic, struct sw_picture *pic, VCEncIn *pEncIn,
                              struct vcenc_instance *vcenc_instance);
void StrmEncodeOverlayConfig(asicData_s *asic, VCEncIn *pEncIn,
                             struct vcenc_instance *vcenc_instance);
void StrmEncodePrefixSei(struct vcenc_instance *vcenc_instance, struct sps *s, VCEncOut *pEncOut,
                         struct sw_picture *pic, VCEncIn *pEncIn);
void StrmEncodeSuffixSei(struct vcenc_instance *vcenc_instance, VCEncIn *pEncIn, VCEncOut *pEncOut);
void StrmEncodeGradualDecoderRefresh(struct vcenc_instance *vcenc_instance, asicData_s *asic,
                                     VCEncIn *pEncIn, VCEncPictureCodingType *codingType,
                                     EWLHwConfig_t cfg);
void StrmEncodeRegionOfInterest(struct vcenc_instance *vcenc_instance, asicData_s *asic);
VCEncRet EncGetSSIM(struct vcenc_instance *inst, VCEncOut *pEncOut);
void CalculateSSIM(struct vcenc_instance *inst, VCEncOut *pEncOut, i64 ssim_numerator_y,
                   i64 ssim_numerator_u, i64 ssim_numerator_v, u32 ssim_denominator_y,
                   u32 ssim_denominator_uv);
VCEncRet EncGetPSNR(struct vcenc_instance *inst, VCEncOut *pEncOut);
void CalculatePSNR(struct vcenc_instance *inst, VCEncOut *pEncOut, u32 width);
VCEncRet TemporalMvpGenConfig(struct vcenc_instance *vcenc_instance, VCEncIn *pEncIn,
                              struct container *c, struct sw_picture *pic,
                              VCEncPictureCodingType codingType);
VCEncRet GenralRefPicMarking(struct vcenc_instance *vcenc_instance, struct container *c,
                             struct rps *r, VCEncPictureCodingType codingType);
i32 EncInitLookAheadBufCnt(const VCEncConfig *config, i32 lookaheadDepth);

i32 FindNextForceIdr(struct queue *jobQueue);

i32 AGopDecision(const struct vcenc_instance *vcenc_instance, VCEncIn *pEncIn,
                 const VCEncOut *pEncOut, i32 *pNextGopSize, VCENCAdapGopCtr *agop);

void VCEncAddNaluSize(VCEncOut *pEncOut, u32 naluSizeBytes, u32 tileId);

VCEncRet VCEncCodecPrepareEncode(struct vcenc_instance *vcenc_instance, const VCEncIn *pEncIn,
                                 VCEncOut *pEncOut, VCEncPictureCodingType codingType,
                                 struct sw_picture *pic, struct container *c, u32 *segcounts);

VCEncRet VCEncCodecPostEncodeUpdate(struct vcenc_instance *vcenc_instance, const VCEncIn *pEncIn,
                                    VCEncOut *pEncOut, VCEncPictureCodingType codingType,
                                    struct sw_picture *pic);

void EndOfSequence(struct vcenc_instance *vcenc_instance, const VCEncIn *pEncIn, VCEncOut *pEncOut);
void SavePicCfg(const VCEncIn *pEncIn, VCEncPicConfig *pPicCfg);

VCEncRet SinglePassEnqueueJob(struct vcenc_instance *vcenc_instance, const VCEncIn *pEncIn);

VCEncJob *SinglePassGetNextJob(struct vcenc_instance *vcenc_instance, const i32 picCnt);

void InitAgop(VCENCAdapGopCtr *agop);

void SetPicCfgToEncIn(const VCEncPicConfig *pPicCfg, VCEncIn *pEncIn);

VCEncPictureCodingType FindNextPic(VCEncInst inst, VCEncIn *encIn, i32 nextGopSize,
                                   const u8 *gopCfgOffset, i32 nextIdrCnt);

/* multi-tile */
void TileInfoConfig(struct vcenc_instance *vcenc_instance, struct sw_picture *pic, u32 tileId,
                    VCEncPictureCodingType codingType, VCEncOut *pEncOut);
u32 TileTotalStreamSize(struct vcenc_instance *vcenc_instance);
void TileInfoCollect(struct vcenc_instance *vcenc_instance, u32 tileId, u32 numNalu);
u32 FindIndexBywaitCoreJobid(struct vcenc_instance *vcenc_instance, u32 waitCoreJobid);
void FillVCEncout(struct vcenc_instance *vcenc_instance, VCEncOut *pEncOut);

//[pass2/single pass]update coding ctrl parameters matched with current frame in instance
void EncUpdateCodingCtrlParam(struct vcenc_instance *pEncInst, EncCodingCtrlParam *pCodingCtrlParam,
                              const i32 picCnt);
//[pass1]update coding ctrl parameters matched with current frame in instance
void EncUpdateCodingCtrlForPass1(VCEncInst pEncInst, EncCodingCtrlParam *pCodingCtrlParam);
/* check inputed coding ctrl parameters*/
VCEncRet EncCheckCodingCtrlParam(struct vcenc_instance *pEncInst, VCEncCodingCtrl *pCodeParams);
CLIENT_TYPE VCEncGetClientType(VCEncVideoCodecFormat codecFormat);
void VCEncEncodeSeiHdr10(struct vcenc_instance *vcenc_instance, VCEncOut *pEncOut);
void VideoTuneConfig(const VCEncConfig *config, struct vcenc_instance *vcenc_inst);
/* set picture configs to job */
void SetPictureCfgToJob(const VCEncIn *pEncIn, VCEncIn *pJobEncIn, u8 gdrDuration);

#ifndef RATE_CONTROL_BUILD_SUPPORT
/* set qpHdr and fixedQp. */
bool_e VCEncInitQp(vcencRateControl_s *rc, u32 newStream);
/* calculate qpHdr for current picture. */
void VCEncBeforeCQP(vcencRateControl_s *rc, u32 timeInc, u32 sliceType, bool use_ltr_cur,
                    struct sw_picture *pic);
#endif

/*init coding ctrl parameter queue*/
void EncInitCodingCtrlQueue(VCEncInst inst);

void VCEncAxiFeEnable(void *inst, void *ewl, u32 tileId);

void VCEncAxiFeDisable(const void *ewl, void *data);

void VCEncSetApbFilter(void *inst);

void VCEncSetwaitJobCfg(VCEncIn *pEncIn, struct vcenc_instance *vcenc_instance, asicData_s *asic,
                        u32 waitCoreJobid);

VCEncRet VCEncSetDec400(VCEncIn *pEncIn, VCDec400data *dec400_data, VCDec400data *dec400_osd,
                        asicData_s *asic);

void VCEncCfgDec400(VCEncIn *pEncIn, VCDec400data *dec400_data, VCDec400data *dec400_osd,
                    struct vcenc_instance *vcenc_instance, asicData_s *asic, u32 tileId);

VCEncRet VCEncSetSubsystem(struct vcenc_instance *vcenc_instance, VCEncIn *pEncIn, asicData_s *asic,
                           VCDec400data *dec400_data, VCDec400data *dec400_osd,
                           struct sw_picture *pic, u32 tileId);

void VCEncResetCallback(VCEncSliceReady *slice_callback, struct vcenc_instance *vcenc_instance,
                        VCEncIn *pEncIn, VCEncOut *pEncOut, u32 next_core_index,
                        u32 multicore_flag);
void VCEncSetRingBuffer(struct vcenc_instance *vcenc_instance, asicData_s *asic,
                        struct sw_picture *pic);
#endif
