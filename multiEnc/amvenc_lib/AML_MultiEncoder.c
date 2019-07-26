// #define LOG_NDEBUG 0
#define LOG_TAG "AMLVENC"
#ifdef MAKEANDROID
#include <utils/Log.h>
#endif
#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include "AML_MultiEncoder.h"
#include "vpuapi.h"
#include "vpuconfig.h"
#include "vdi_osal.h"
#include "debug.h"
#include "vpuapifunc.h"

#ifdef MAKEANDROID
#define LOGAPI ALOGE
#else
#define LOGAPI  // printf
#endif
#define SUPPORT_SCALE 0

static bool INIT_GE2D = false;
#define MULTI_ENC_MAGIC ('A' << 24| 'M' <<16 | 'L' << 8 |'G')
/* timeout in ms */
#define VPU_WAIT_TIME_OUT_CQ    2
/* extra source frame buffer required.*/
#define EXTRA_SRC_BUFFER_NUM    0

// user scaling list
#define SL_NUM_MATRIX (6)

#define wave420l_align4(a)      ((((a)+3)>>2)<<2)
#define wave420l_align8(a)      ((((a)+7)>>3)<<3)
#define wave420l_align16(a)     ((((a)+15)>>4)<<4)
#define wave420l_align32(a)     ((((a)+31)>>5)<<5)
#define wave420l_align64(a)     ((((a)+63)>>6)<<6)
#define wave420l_align128(a)    ((((a)+127)>>7)<<7)
#define wave420l_align256(a)    ((((a)+255)>>8)<<8)
typedef struct
{
    Uint8 s4[SL_NUM_MATRIX][16]; // [INTRA_Y/U/V,INTER_Y/U/V][NUM_COEFF]
    Uint8 s8[SL_NUM_MATRIX][64];
    Uint8 s16[SL_NUM_MATRIX][64];
    Uint8 s32[SL_NUM_MATRIX][64];
    Uint8 s16dc[SL_NUM_MATRIX];
    Uint8 s32dc[2];
}UserScalingList;

typedef enum {
    ENC_INT_STATUS_NONE,        // Interrupt not asserted yet
    ENC_INT_STATUS_FULL,        // Need more buffer
    ENC_INT_STATUS_DONE,        // Interrupt asserted
    ENC_INT_STATUS_LOW_LATENCY,
    ENC_INT_STATUS_TIMEOUT,     // Interrupt not asserted during given time.
} ENC_INT_STATUS;

typedef enum {
    ENCODER_STATE_OPEN,
    ENCODER_STATE_INIT_SEQ,
    ENCODER_STATE_REGISTER_FB,
    ENCODER_STATE_ENCODE_HEADER,
    ENCODER_STATE_ENCODING,
} EncoderState;

typedef struct AMVEncContext_s {
  uint32 magic_num;
  uint32 instance_id;
  uint32 src_idx;
  uint32 src_count;
  uint32 enc_width;
  uint32 enc_height;
  uint32 bitrate;    /* target encoding bit rate in bits/second */
  uint32 frame_rate; /* frame rate */
  int32 idrPeriod;   /* IDR period in number of frames */
  uint32 op_flag;

  uint32 src_num;   /*total src_frame buffer needed */
  uint32 fb_num;    /* total reconstuction  frame buffer encoder needed */

  uint32 enc_counter;
  uint frame_delay;

  AMVEncFrameFmt fmt;

  bool mPrependSPSPPSToIDRFrames;
  uint32 initQP; /* initial QP */
  //AVCNalUnitType nal_unit_type;
  bool rcEnable; /* rate control enable, on: RC on, off: constant QP */
  AMVEncInitParams mInitParams; //copy of initial set up from upper layer
/* VPU API interfaces  */
  EncHandle enchandle; /*VPU encoder handler */
  EncOpenParam encOpenParam; /* api open param */
  EncChangeParam changeParam;
  Uint32 param_change_flag;
  Uint32                      frameIdx;
  vpu_buffer_t                vbCustomLambda;
  vpu_buffer_t                vbScalingList;
  Uint32                      customLambda[NUM_CUSTOM_LAMBDA];
  UserScalingList             scalingList;
  vpu_buffer_t                vbCustomMap[MAX_REG_FRAME];
  EncoderState                state;
  EncInitialInfo              initialInfo;
  EncParam                    encParam;
  Int32                       encodedSrcFrmIdxArr[ENC_SRC_BUF_NUM];
  Int32                       encMEMSrcFrmIdxArr[ENC_SRC_BUF_NUM];
  vpu_buffer_t                bsBuffer[ENC_SRC_BUF_NUM];
  BOOL                        fullInterrupt;
  Uint32                      changedCount;
  Uint64                      startTimeout;
  Uint32                      cyclePerTick;



  Uint32                      reconFbStride;
  Uint32                      reconFbHeight;
  Uint32                      srcFbStride;
  Uint32                      srcFbSize;
  FrameBufferAllocInfo        srcFbAllocInfo;
  BOOL                        fbAllocated;
  FrameBuffer                 pFbRecon[MAX_REG_FRAME];
  vpu_buffer_t                pFbReconMem[MAX_REG_FRAME];
  FrameBuffer                 pFbSrc[ENC_SRC_BUF_NUM];
  vpu_buffer_t                pFbSrcMem[ENC_SRC_BUF_NUM];
  AMVMultiEncFrameIO          FrameIO[ENC_SRC_BUF_NUM];



  vpu_buffer_t work_vb;
  vpu_buffer_t temp_vb;
  vpu_buffer_t fbc_ltable_vb;
  vpu_buffer_t fbc_ctable_vb;
  vpu_buffer_t fbc_mv_vb;
  vpu_buffer_t subsample_vb;

  uint32 debugEnable;
  BOOL force_terminate;
  unsigned long long mNumInputFrames;
  AMVEncBufferType bufType;
  uint32 mNumPlanes;
#if SUPPORT_SCALE
  uint32 ge2d_initial_done;
#endif
} AMVMultiCtx;


#if SUPPORT_SCALE
#include <aml_ge2d.h>
#include <ge2d_port.h>

extern aml_ge2d_t amlge2d;
aml_ge2d_info_t ge2dinfo;

static int SRC1_PIXFORMAT = PIXEL_FORMAT_YCrCb_420_SP;
static int SRC2_PIXFORMAT = PIXEL_FORMAT_YCrCb_420_SP;
static int DST_PIXFORMAT = PIXEL_FORMAT_YCrCb_420_SP;

static GE2DOP OP = AML_GE2D_STRETCHBLIT;
static int do_strechblit(aml_ge2d_info_t* pge2dinfo, AMVMultiEncFrameIO* input) {
  int ret = -1;
  char code = 0;
  VLOG(INFO, "do_strechblit test case:\n");
  pge2dinfo->src_info[0].memtype = GE2D_CANVAS_ALLOC;
  pge2dinfo->dst_info.memtype = GE2D_CANVAS_ALLOC;
  pge2dinfo->src_info[0].canvas_w = input->pitch;   // SX_SRC1;
  pge2dinfo->src_info[0].canvas_h = input->height;  // SY_SRC1;
  pge2dinfo->src_info[0].format = SRC1_PIXFORMAT;
  if ((input->scale_width != 0) && (input->scale_height != 0)) {
    pge2dinfo->dst_info.canvas_w = input->scale_width;   // SX_DST;
    pge2dinfo->dst_info.canvas_h = input->scale_height;  // SY_DST;
  } else {
    pge2dinfo->dst_info.canvas_w =
        input->pitch - input->crop_left - input->crop_right;
    pge2dinfo->dst_info.canvas_h =
        input->height - input->crop_top - input->crop_bottom;
  }
  pge2dinfo->dst_info.format = DST_PIXFORMAT;

  pge2dinfo->src_info[0].rect.x = input->crop_left;
  pge2dinfo->src_info[0].rect.y = input->crop_top;
  pge2dinfo->src_info[0].rect.w =
      input->pitch - input->crop_left - input->crop_right;
  pge2dinfo->src_info[0].rect.h =
      input->height - input->crop_top - input->crop_bottom;
  pge2dinfo->dst_info.rect.x = 0;
  pge2dinfo->dst_info.rect.y = 0;
  if ((input->scale_width != 0) && (input->scale_height != 0)) {
    pge2dinfo->dst_info.rect.w = input->scale_width;   // SX_DST;
    pge2dinfo->dst_info.rect.h = input->scale_height;  // SY_DST;
  } else {
    pge2dinfo->dst_info.rect.w =
        input->pitch - input->crop_left - input->crop_right;
    pge2dinfo->dst_info.rect.h =
        input->height - input->crop_top - input->crop_bottom;
  }
  pge2dinfo->dst_info.rotation = GE2D_ROTATION_0;
  ret = aml_ge2d_process(pge2dinfo);
  return ret;
}

static void set_ge2dinfo(aml_ge2d_info_t* pge2dinfo,
                         AMVEncInitParams* encParam) {
  pge2dinfo->src_info[0].memtype = GE2D_CANVAS_ALLOC;
  pge2dinfo->src_info[0].canvas_w = encParam->src_width;   // SX_SRC1;
  pge2dinfo->src_info[0].canvas_h = encParam->src_height;  // SY_SRC1;
  pge2dinfo->src_info[0].format = SRC1_PIXFORMAT;
  pge2dinfo->src_info[1].memtype = GE2D_CANVAS_TYPE_INVALID;
  pge2dinfo->src_info[1].canvas_w = 0;
  pge2dinfo->src_info[1].canvas_h = 0;
  pge2dinfo->src_info[1].format = SRC2_PIXFORMAT;

  pge2dinfo->dst_info.memtype = GE2D_CANVAS_ALLOC;
  pge2dinfo->dst_info.canvas_w = encParam->width;   // SX_DST;
  pge2dinfo->dst_info.canvas_h = encParam->height;  // SY_DST;
  pge2dinfo->dst_info.format = DST_PIXFORMAT;
  pge2dinfo->dst_info.rotation = GE2D_ROTATION_0;
  pge2dinfo->offset = 0;
  pge2dinfo->ge2d_op = OP;
  pge2dinfo->blend_mode = BLEND_MODE_PREMULTIPLIED;
}
#endif

void static yuv_plane_memcpy(int coreIdx, int dst, char *src, uint32 width, uint32 height, uint32 stride,
                        bool aligned, EndianMode endian) {
    unsigned i;
  if (dst == NULL || src == NULL) {
    VLOG(ERR, "yuv_plane_memcpy error ptr\n");
    return;
  }
  if (!aligned) {
    for (i = 0; i < height; i++) {
      vdi_write_memory(coreIdx, dst, (Uint8 *)src, width, endian);
//      memcpy((void *)dst, (void *)src, width);
      dst += stride;
      src += width;
    }
  } else {
//    memcpy(dst, (void *)src, stride * height);
     vdi_write_memory(coreIdx, dst, (Uint8 *)src, stride * height, endian);
  }
}

BOOL SetupEncoderOpenParam(EncOpenParam *pEncOP, AMVEncInitParams* InitParam)
{
    int i;
  EncWaveParam *param = &pEncOP->EncStdParam.waveParam;
  /*basic settings */
  if (InitParam->stream_type == AMV_AVC)
    pEncOP->bitstreamFormat = STD_AVC;
  else if(InitParam->stream_type == AMV_HEVC)
    pEncOP->bitstreamFormat = STD_HEVC;
  else {
    VLOG(ERR, "[ERROR] Not supported format (%d)\n", InitParam->stream_type);
    return FALSE;
  }
  pEncOP->picWidth        = InitParam->width;
  pEncOP->picHeight       = InitParam->height;
  pEncOP->frameRateInfo  =  InitParam->frame_rate;
  pEncOP->bitRate = InitParam->bitrate;
  pEncOP->rcEnable = InitParam->rate_control;
  /* default */
  pEncOP->srcBitDepth     = 8; /*pCfg->SrcBitDepth; */
  param->level = 0;
  param->tier  = 0;
  param->internalBitDepth = 8; /*pCfg->waveCfg.internalBitDepth;*/

  if ( param->internalBitDepth == 10 ) {
    pEncOP->outputFormat = FORMAT_420_P10_16BIT_MSB;
    param->profile = HEVC_PROFILE_MAIN10;
  } else if ( param->internalBitDepth == 8 ) {
    pEncOP->outputFormat = FORMAT_420;
    param->profile = HEVC_PROFILE_MAIN;
  }
  param->losslessEnable = 0;
  param->constIntraPredFlag = 0;
  param->lfCrossSliceBoundaryEnable = 1;
  param->useLongTerm = 0;

  /* for CMD_ENC_SEQ_GOP_PARAM */
   //param->gopPresetIdx     = pCfg->waveCfg.gopPresetIdx;
  if (InitParam->GopPreset == GOP_OPT_NONE)
  {
     if (InitParam->idr_period == 1)
        param->gopPresetIdx = PRESET_IDX_ALL_I; //all I frame
     else
        param->gopPresetIdx = PRESET_IDX_IPP;
  } else if (InitParam->GopPreset == GOP_ALL_I) {
    param->gopPresetIdx = PRESET_IDX_ALL_I;
  }
  else if (InitParam->GopPreset == GOP_IP) {
    param->gopPresetIdx = PRESET_IDX_IPP;
  }
  else if (InitParam->GopPreset == GOP_IBBBP) {
    param->gopPresetIdx = PRESET_IDX_IBBBP;
  }
  else {
    VLOG(ERR, "[ERROR] Not supported GOP format (%d)\n", InitParam->GopPreset);
    return FALSE;
  }

  /* for CMD_ENC_SEQ_INTRA_PARAM */
  param->decodingRefreshType = 1; //pCfg->waveCfg.decodingRefreshType;
  param->intraPeriod = InitParam->idr_period;//pCfg->waveCfg.intraPeriod;
  if(InitParam->idr_period == 0) { // only one I then all P
    param->gopPresetIdx = PRESET_IDX_IPP;
    param->intraPeriod = 0xffff;
  } else if (InitParam ->idr_period == 1) {
    param->gopPresetIdx = PRESET_IDX_ALL_I;
    param->intraPeriod = InitParam ->idr_period;
  } else
    param->intraPeriod = InitParam ->idr_period;

  VLOG(ERR, "GopPreset GOP format (%d) period %d \n",
        param->gopPresetIdx, param->intraPeriod);

  param->intraQP = InitParam ->initQP; //pCfg->waveCfg.intraQP;

  /* for CMD_ENC_SEQ_CONF_WIN_TOP_BOT/LEFT_RIGHT */
  param->confWinTop = 0; //pCfg->waveCfg.confWinTop;
  param->confWinBot = 0; //pCfg->waveCfg.confWinBot;
  param->confWinLeft = 0; //pCfg->waveCfg.confWinLeft;
  param->confWinRight = 0; //pCfg->waveCfg.confWinRight;

  /* for CMD_ENC_SEQ_INDEPENDENT_SLICE */
  param->independSliceMode = 0; //pCfg->waveCfg.independSliceMode;
  param->independSliceModeArg = 0; //pCfg->waveCfg.independSliceModeArg;

  /* for CMD_ENC_SEQ_DEPENDENT_SLICE */
  param->dependSliceMode = 0; //pCfg->waveCfg.dependSliceMode;
  param->dependSliceModeArg = 0; //pCfg->waveCfg.dependSliceModeArg;

  /* for CMD_ENC_SEQ_INTRA_REFRESH_PARAM */
  param->intraRefreshMode = 0; //pCfg->waveCfg.intraRefreshMode;
  param->intraRefreshArg = 0; //pCfg->waveCfg.intraRefreshArg;
  param->useRecommendEncParam = 1; //pCfg->waveCfg.useRecommendEncParam;

  /* for CMD_ENC_PARAM */
  param->scalingListEnable = 0; //pCfg->waveCfg.scalingListEnable;
  param->cuSizeMode = 0x7; // always set cu8x8/16x16/32x32 enable to 1.
  param->tmvpEnable = 1; //pCfg->waveCfg.tmvpEnable;
  param->wppEnable = 0; //pCfg->waveCfg.wppenable;
  param->maxNumMerge = 2; //pCfg->waveCfg.maxNumMerge;

  param->disableDeblk = 0; //pCfg->waveCfg.disableDeblk;

  param->lfCrossSliceBoundaryEnable = 1; //pCfg->waveCfg.lfCrossSliceBoundaryEnable;
  param->betaOffsetDiv2 = 0;//pCfg->waveCfg.betaOffsetDiv2;
  param->tcOffsetDiv2 = 0; //pCfg->waveCfg.tcOffsetDiv2;
  param->skipIntraTrans = 1;//pCfg->waveCfg.skipIntraTrans;
  param->saoEnable = 1;//pCfg->waveCfg.saoEnable;
  param->intraNxNEnable = 1; //pCfg->waveCfg.intraNxNEnable;

  /* for CMD_ENC_RC_PARAM */
  //pEncOP->rcEnable             = InitParam -> rate_control;
  pEncOP->vbvBufferSize = 3000;//pCfg->VbvBufferSize;
  param->cuLevelRCEnable = 1; //pCfg->waveCfg.cuLevelRCEnable;
  param->hvsQPEnable = 1;//pCfg->waveCfg.hvsQPEnable;
  param->hvsQpScale = 2;//pCfg->waveCfg.hvsQpScale;

  param->bitAllocMode = 0; //pCfg->waveCfg.bitAllocMode;
  for (i = 0; i < MAX_GOP_NUM; i++) {
    param->fixedBitRatio[i] = 1;//pCfg->waveCfg.fixedBitRatio[i];
  }

  if (InitParam->qp_mode == 1) {
    param->minQpI = InitParam->minQP;
    param->minQpP = InitParam->minQP_P;
    param->minQpB = InitParam->minQP_B;
    param->maxQpI = InitParam->maxQP;
    param->maxQpP = InitParam->maxQP_P;
    param->maxQpB = InitParam->maxQP_B;
    param->maxDeltaQp = InitParam->maxDeltaQP;
  } else {
    param->minQpI = 8;
    param->minQpP = 8;
    param->minQpB = 8;
    param->maxQpI = 51;
    param->maxQpP = 51;
    param->maxQpB = 51;
    param->maxDeltaQp = 10;
  }

#if 0
    /* for CMD_ENC_CUSTOM_GOP_PARAM */
    param->gopParam.customGopSize     = pCfg->waveCfg.gopParam.customGopSize;

    for (i= 0; i<param->gopParam.customGopSize; i++) {
        param->gopParam.picParam[i].picType      = pCfg->waveCfg.gopParam.picParam[i].picType;
        param->gopParam.picParam[i].pocOffset    = pCfg->waveCfg.gopParam.picParam[i].pocOffset;
        param->gopParam.picParam[i].picQp        = pCfg->waveCfg.gopParam.picParam[i].picQp;
        param->gopParam.picParam[i].refPocL0     = pCfg->waveCfg.gopParam.picParam[i].refPocL0;
        param->gopParam.picParam[i].refPocL1     = pCfg->waveCfg.gopParam.picParam[i].refPocL1;
        param->gopParam.picParam[i].temporalId   = pCfg->waveCfg.gopParam.picParam[i].temporalId;
        param->gopParam.picParam[i].numRefPicL0  = pCfg->waveCfg.gopParam.picParam[i].numRefPicL0;
    }
#endif
  param->roiEnable = 0; //pCfg->waveCfg.roiEnable;
  // VPS & VUI
  param->numUnitsInTick = 0; //1000;//pCfg->waveCfg.numUnitsInTick;
  param->timeScale = 0; //pEncOP->frameRateInfo * 1000; //pCfg->waveCfg.timeScale;
  param->numTicksPocDiffOne = 0;//pCfg->waveCfg.numTicksPocDiffOne;

  param->chromaCbQpOffset = 0; //pCfg->waveCfg.chromaCbQpOffset;
  param->chromaCrQpOffset = 0; //pCfg->waveCfg.chromaCrQpOffset;
  param->initialRcQp      = -1; //63//pCfg->waveCfg.initialRcQp;

  param->nrYEnable = 1; //pCfg->waveCfg.nrYEnable;
  param->nrCbEnable = 1; //pCfg->waveCfg.nrCbEnable;
  param->nrCrEnable = 1; //pCfg->waveCfg.nrCrEnable;
  param->nrNoiseEstEnable = 1;// pCfg->waveCfg.nrNoiseEstEnable;
  param->nrNoiseSigmaY = 0; //pCfg->waveCfg.nrNoiseSigmaY;
  param->nrNoiseSigmaCb = 0; //pCfg->waveCfg.nrNoiseSigmaCb;
  param->nrNoiseSigmaCr = 0; //pCfg->waveCfg.nrNoiseSigmaCr;
  param->nrIntraWeightY = 7; //pCfg->waveCfg.nrIntraWeightY;
  param->nrIntraWeightCb = 7; //pCfg->waveCfg.nrIntraWeightCb;
  param->nrIntraWeightCr = 7; //pCfg->waveCfg.nrIntraWeightCr;
  param->nrInterWeightY = 4; //pCfg->waveCfg.nrInterWeightY;
  param->nrInterWeightCb = 4; //pCfg->waveCfg.nrInterWeightCb;
  param->nrInterWeightCr = 4; //pCfg->waveCfg.nrInterWeightCr;

  param->monochromeEnable = 0; //pCfg->waveCfg.monochromeEnable;
  param->strongIntraSmoothEnable = 1; //pCfg->waveCfg.strongIntraSmoothEnable;
  param->weightPredEnable = 0; //pCfg->waveCfg.weightPredEnable;
  param->bgDetectEnable = 0; //pCfg->waveCfg.bgDetectEnable;
  param->bgThrDiff = 8; //pCfg->waveCfg.bgThrDiff;
  param->bgThrMeanDiff = 1;//pCfg->waveCfg.bgThrMeanDiff;
  param->bgLambdaQp = 32 ; //pCfg->waveCfg.bgLambdaQp;
  param->bgDeltaQp = 3; //pCfg->waveCfg.bgDeltaQp;
  param->customLambdaEnable = 0; //pCfg->waveCfg.customLambdaEnable;
  param->customMDEnable = 0; //pCfg->waveCfg.customMDEnable;
  param->pu04DeltaRate = 0; //pCfg->waveCfg.pu04DeltaRate;
  param->pu08DeltaRate = 0; //pCfg->waveCfg.pu08DeltaRate;
  param->pu16DeltaRate = 0; //pCfg->waveCfg.pu16DeltaRate;
  param->pu32DeltaRate = 0; //pCfg->waveCfg.pu32DeltaRate;
  param->pu04IntraPlanarDeltaRate = 0; //pCfg->waveCfg.pu04IntraPlanarDeltaRate;
  param->pu04IntraDcDeltaRate = 0; //pCfg->waveCfg.pu04IntraDcDeltaRate;
  param->pu04IntraAngleDeltaRate = 0; //pCfg->waveCfg.pu04IntraAngleDeltaRate;
  param->pu08IntraPlanarDeltaRate = 0; //pCfg->waveCfg.pu08IntraPlanarDeltaRate;
  param->pu08IntraDcDeltaRate = 0; //pCfg->waveCfg.pu08IntraDcDeltaRate;
  param->pu08IntraAngleDeltaRate = 0; //pCfg->waveCfg.pu08IntraAngleDeltaRate;
  param->pu16IntraPlanarDeltaRate = 0; //pCfg->waveCfg.pu16IntraPlanarDeltaRate;
  param->pu16IntraDcDeltaRate = 0; //pCfg->waveCfg.pu16IntraDcDeltaRate;
  param->pu16IntraAngleDeltaRate = 0; //pCfg->waveCfg.pu16IntraAngleDeltaRate;
  param->pu32IntraPlanarDeltaRate = 0; // pCfg->waveCfg.pu32IntraPlanarDeltaRate;
  param->pu32IntraDcDeltaRate = 0; //pCfg->waveCfg.pu32IntraDcDeltaRate;
  param->pu32IntraAngleDeltaRate = 0; //pCfg->waveCfg.pu32IntraAngleDeltaRate;
  param->cu08IntraDeltaRate = 0; //pCfg->waveCfg.cu08IntraDeltaRate;
  param->cu08InterDeltaRate = 0; //pCfg->waveCfg.cu08InterDeltaRate;
  param->cu08MergeDeltaRate = 0; //pCfg->waveCfg.cu08MergeDeltaRate;
  param->cu16IntraDeltaRate = 0; //pCfg->waveCfg.cu16IntraDeltaRate;
  param->cu16InterDeltaRate = 0; //pCfg->waveCfg.cu16InterDeltaRate;
  param->cu16MergeDeltaRate = 0; //pCfg->waveCfg.cu16MergeDeltaRate;
  param->cu32IntraDeltaRate = 0; // pCfg->waveCfg.cu32IntraDeltaRate;
  param->cu32InterDeltaRate = 0; //pCfg->waveCfg.cu32InterDeltaRate;
  param->cu32MergeDeltaRate = 0; //pCfg->waveCfg.cu32MergeDeltaRate;
  param->coefClearDisable = 0; //pCfg->waveCfg.coefClearDisable;

  param->s2fmeDisable                = 0; //pCfg->waveCfg.s2fmeDisable;
  // for H.264 on WAVE
  param->rdoSkip = 1; //pCfg->waveCfg.rdoSkip;
  param->lambdaScalingEnable = 1; //pCfg->waveCfg.lambdaScalingEnable;
  param->transform8x8Enable = 1; //pCfg->waveCfg.transform8x8;
  param->avcSliceMode = 0; //pCfg->waveCfg.avcSliceMode;
  param->avcSliceArg = 0; //pCfg->waveCfg.avcSliceArg;
  param->intraMbRefreshMode = 0; //pCfg->waveCfg.intraMbRefreshMode;
  param->intraMbRefreshArg = 1; //pCfg->waveCfg.intraMbRefreshArg;
  param->mbLevelRcEnable = 0; //pCfg->waveCfg.mbLevelRc;
  param->entropyCodingMode = 1; //pCfg->waveCfg.entropyCodingMode;;


  pEncOP->streamBufCount = ENC_STREAM_BUF_COUNT;
  pEncOP->streamBufSize  = ENC_STREAM_BUF_SIZE;

  if (pEncOP->streamBufCount < COMMAND_QUEUE_DEPTH )
    pEncOP->streamBufCount = COMMAND_QUEUE_DEPTH; //for encoder->numSinkPortQueue
  pEncOP->srcFormat = FORMAT_420;
  if (InitParam->fmt == AMVENC_NV21)
    pEncOP->nv21 = 1;
  else
    pEncOP->nv21 = 0;
  pEncOP->packedFormat   = 0;

  if (InitParam->fmt == AMVENC_NV21 || InitParam->fmt == AMVENC_NV12)
     pEncOP->cbcrInterleave = 1;
  else
     pEncOP->cbcrInterleave = 0;
  pEncOP->frameEndian    = VPU_FRAME_ENDIAN;
  pEncOP->streamEndian   = VPU_STREAM_ENDIAN;
  pEncOP->sourceEndian   = VPU_SOURCE_ENDIAN;
  pEncOP->lineBufIntEn   = 1;
  pEncOP->coreIdx        = 0;
  pEncOP->cbcrOrder      = CBCR_ORDER_NORMAL;
  pEncOP->lowLatencyMode = 0; // 2bits lowlatency mode setting. bit[1]: low latency interrupt enable, bit[0]: fast bitstream-packing enable.
  pEncOP->EncStdParam.waveParam.useLongTerm = 0;
  return TRUE;
}

static ENC_INT_STATUS HandlingInterruptFlag(AMVMultiCtx* ctx)
{
  EncHandle handle = ctx->enchandle;
  Int32 interruptFlag = 0;
  Uint32 interruptWaitTime = VPU_WAIT_TIME_OUT_CQ;
  Uint32 interruptTimeout = VPU_ENC_TIMEOUT;
  ENC_INT_STATUS status = ENC_INT_STATUS_NONE;

  if (ctx->startTimeout == 0ULL) {
    ctx->startTimeout = osal_gettime();
  }
  do {
    interruptFlag = VPU_WaitInterruptEx(handle, interruptWaitTime);
    if (interruptFlag == -1) {
      Uint64 currentTimeout = osal_gettime();
      if ((currentTimeout - ctx->startTimeout) > interruptTimeout) {
        VLOG(ERR, "<%s:%d> startTimeout(%lld) currentTime(%lld) diff(%d)\n",
                    __FUNCTION__, __LINE__, ctx->startTimeout, currentTimeout, (Uint32)(currentTimeout - ctx->startTimeout));
        status = ENC_INT_STATUS_TIMEOUT;
        break;
      }
      interruptFlag = 0;
    }
    if (interruptFlag < 0)
      VLOG(ERR, "<%s:%d> interruptFlag is negative value! %08x\n", __FUNCTION__, __LINE__, interruptFlag);

    if (interruptFlag > 0) {
      VPU_ClearInterruptEx(handle, interruptFlag);
      ctx->startTimeout = 0ULL;
    }

    if (interruptFlag & (1<<INT_WAVE5_ENC_SET_PARAM)) {
      status = ENC_INT_STATUS_DONE;
      break;
    }

    if (interruptFlag & (1<<INT_WAVE5_ENC_PIC)) {
      status = ENC_INT_STATUS_DONE;
      break;
    }

    if (interruptFlag & (1<<INT_WAVE5_BSBUF_FULL)) {
      status = ENC_INT_STATUS_FULL;
      break;
    }

    if (interruptFlag & (1<<INT_WAVE5_ENC_LOW_LATENCY)) {
       status = ENC_INT_STATUS_LOW_LATENCY;
       break;
    }
  } while (FALSE);

  return status;
}

static void DisplayEncodedInformation(
    EncHandle       handle,
    CodStd          codec,
    Uint32          frameNo,
    EncOutputInfo*  encodedInfo,
    Int32           srcEndFlag,
    Int32           srcFrameIdx,
    Int32           performance
    )
{
    QueueStatusInfo queueStatus;

    VPU_EncGiveCommand(handle, ENC_GET_QUEUE_STATUS, &queueStatus);

    if (encodedInfo == NULL) {
        if (performance == TRUE ) {
            VLOG(INFO, "------------------------------------------------------------------------------------------------------------------------------------------------------------------------\n");
            VLOG(INFO, "                                                                        | FRAME  | HOST | PREP_S PREP_E    PREP  | PROCE_S PROCE_E  PROCE | ENC_S  ENC_E   ENC   |\n");
            VLOG(INFO, "I     NO     T   RECON   RD_PTR    WR_PTR     BYTES  SRCIDX  USEDSRCIDX | CYCLE  | TICK |  TICK   TICK     CYCLE |  TICK    TICK    CYCLE |  TICK   TICK   CYCLE | TQ IQ\n");
            VLOG(INFO, "------------------------------------------------------------------------------------------------------------------------------------------------------------------------\n");
        }
        else {
            VLOG(INFO, "---------------------------------------------------------------------------------------------------------------------\n");
            VLOG(INFO, "                                                                        |                CYCLE\n");
            VLOG(INFO, "I     NO     T   RECON   RD_PTR    WR_PTR     BYTES  SRCIDX  USEDSRCIDX | FRAME PREPARING PROCESSING ENCODING | TQ IQ\n");
            VLOG(INFO, "---------------------------------------------------------------------------------------------------------------------\n");
        }
    } else {
        if (performance == TRUE) {
            VLOG(INFO, "%02d %5d %5d %5d    %08x  %08x %8x     %2d        %2d    %8d %6d (%6d,%6d,%8d) (%6d,%6d,%8d) (%6d,%6d,%8d) %d %d\n",
                handle->instIndex, encodedInfo->encPicCnt, encodedInfo->picType, encodedInfo->reconFrameIndex, encodedInfo->rdPtr, encodedInfo->wrPtr,
                encodedInfo->bitstreamSize, (srcEndFlag == 1 ? -1 : srcFrameIdx), encodedInfo->encSrcIdx,
                encodedInfo->frameCycle, encodedInfo->encHostCmdTick,
                encodedInfo->encPrepareStartTick, encodedInfo->encPrepareEndTick, encodedInfo->prepareCycle,
                encodedInfo->encProcessingStartTick, encodedInfo->encProcessingEndTick, encodedInfo->processing,
                encodedInfo->encEncodeStartTick, encodedInfo->encEncodeEndTick, encodedInfo->EncodedCycle,
                queueStatus.totalQueueCount, queueStatus.instanceQueueCount);
        }
        else {
            VLOG(INFO, "%02d %5d %5d %5d    %08x  %08x %8x     %2d        %2d    %8d %8d %8d %8d      %d %d\n",
                handle->instIndex, encodedInfo->encPicCnt, encodedInfo->picType, encodedInfo->reconFrameIndex, encodedInfo->rdPtr, encodedInfo->wrPtr,
                encodedInfo->bitstreamSize, (srcEndFlag == 1 ? -1 : srcFrameIdx), encodedInfo->encSrcIdx,
                encodedInfo->frameCycle, encodedInfo->prepareCycle, encodedInfo->processing, encodedInfo->EncodedCycle,
                queueStatus.totalQueueCount, queueStatus.instanceQueueCount);
        }
    }
}

static BOOL SetSequenceInfo (AMVMultiCtx* ctx)
{
  EncHandle handle = ctx->enchandle;
  RetCode ret = RETCODE_SUCCESS;
  EncInitialInfo* initialInfo = &ctx->initialInfo;
  ENC_INT_STATUS status;
  int retry_cnt = 0;
  do {
    ret = VPU_EncIssueSeqInit(handle);
    retry_cnt++;
  } while (ret == RETCODE_QUEUEING_FAILURE && retry_cnt < 10);
  if (ret != RETCODE_SUCCESS) {
    VLOG(ERR, "%s:%d Failed to VPU_EncIssueSeqInit() ret(%d)\n", __FUNCTION__, __LINE__, ret);
    return FALSE;
  }
  VLOG(INFO, "%s:%d Encoder issue  VPU_EncIssueSeq \n", __FUNCTION__, __LINE__);
  retry_cnt = 0;
  while (retry_cnt++ < 100) {
    if ((status = HandlingInterruptFlag(ctx)) == ENC_INT_STATUS_DONE) {
      break;
    }
    else if (status == ENC_INT_STATUS_NONE) {
      usleep(1000);
    }
    else if (status == ENC_INT_STATUS_TIMEOUT) {
       VLOG(INFO, "%s:%d INSTANCE #%d INTERRUPT TIMEOUT\n", __FUNCTION__, __LINE__, handle->instIndex);
       HandleEncoderError(ctx->enchandle, ctx->frameIdx, NULL);
       return FALSE;
    }
    else {
      VLOG(INFO, "%s:%d Unknown interrupt status: %d\n", __FUNCTION__, __LINE__, status);
      return FALSE;
    }
    usleep(1000);
  }

  if ((ret=VPU_EncCompleteSeqInit(handle, initialInfo)) != RETCODE_SUCCESS) {
    VLOG(ERR, "%s:%d FAILED TO ENC_PIC_HDR: ret(%d), SEQERR(%08x)\n",
      __FUNCTION__, __LINE__, ret, initialInfo->seqInitErrReason);
    return FALSE;
  }
  ctx->fb_num = initialInfo->minFrameBufferCount;
  ctx->src_num = initialInfo->minSrcFrameCount + COMMAND_QUEUE_DEPTH
                + EXTRA_SRC_BUFFER_NUM;

  if (ctx->encOpenParam.sourceBufCount > ctx->src_num)
    ctx->src_num = ctx->encOpenParam.sourceBufCount;

  VLOG(INFO, "Required buffer fb_num=%d, src_num=%d, actual src=%d %dx%d\n",
    ctx->fb_num, initialInfo->minSrcFrameCount,
    ctx->src_num, ctx->encOpenParam.picWidth, ctx->encOpenParam.picHeight);

  VLOG(INFO, "Done sequence init can setup BW and performance Monitor here\n");
  return TRUE;
}

static BOOL AllocateReconFrameBuffer(AMVMultiCtx* ctx)
{
  Uint32 i;
  Uint32 reconFbSize;
  Uint32 reconFbStride;
  Uint32 reconFbWidth;
  Uint32 reconFbHeight;
  EncOpenParam encOpenParam = ctx->encOpenParam;


  if (ctx->encOpenParam.bitstreamFormat == STD_AVC) {
    reconFbWidth  = VPU_ALIGN16(encOpenParam.picWidth);
    reconFbHeight = VPU_ALIGN16(encOpenParam.picHeight);
  } else {
    reconFbWidth  = VPU_ALIGN8(encOpenParam.picWidth);
    reconFbHeight = VPU_ALIGN8(encOpenParam.picHeight);
  }

  if ((ctx->mInitParams.rotate_angle != 0 || ctx->mInitParams.mirror != 0)
    && !(ctx->mInitParams.rotate_angle == 180
         && ctx->mInitParams.mirror == MIRDIR_HOR_VER)) {
    reconFbWidth  = VPU_ALIGN32(encOpenParam.picWidth);
    reconFbHeight = VPU_ALIGN32(encOpenParam.picHeight);
  }

  if (ctx->mInitParams.rotate_angle == 90
    || ctx->mInitParams.rotate_angle== 270) {
    reconFbWidth  = VPU_ALIGN32(encOpenParam.picHeight);
    reconFbHeight = VPU_ALIGN32(encOpenParam.picWidth);
  }

  /* Allocate framebuffers for recon. */
  reconFbStride = CalcStride(reconFbWidth, reconFbHeight,
        (FrameBufferFormat)encOpenParam.outputFormat,
        encOpenParam.cbcrInterleave,
        COMPRESSED_FRAME_MAP, FALSE);
  reconFbSize = VPU_GetFrameBufSize(encOpenParam.coreIdx,
        reconFbStride, reconFbHeight, COMPRESSED_FRAME_MAP,
        (FrameBufferFormat)encOpenParam.outputFormat,
        encOpenParam.cbcrInterleave, NULL);

  vdi_lock(encOpenParam.coreIdx);
  for (i = 0; i < ctx->fb_num; i++) {
    ctx->pFbReconMem[i].size = reconFbSize;
    if (vdi_allocate_dma_memory(encOpenParam.coreIdx, &ctx->pFbReconMem[i]) < 0) {
      vdi_unlock(encOpenParam.coreIdx);
      ctx->pFbReconMem[i].size = 0;
      VLOG(ERR, "fail to allocate recon buffer\n");
      return FALSE;
    }
    ctx->pFbRecon[i].bufY  = ctx->pFbReconMem[i].phys_addr;
    ctx->pFbRecon[i].bufCb = (PhysicalAddress) - 1;
    ctx->pFbRecon[i].bufCr = (PhysicalAddress) - 1;
    ctx->pFbRecon[i].size  = reconFbSize;
    ctx->pFbRecon[i].updateFbInfo = TRUE;
    ctx->pFbRecon[i].dma_buf_planes = 0;
  }
  vdi_unlock(encOpenParam.coreIdx);

  ctx->reconFbStride = reconFbStride;
  ctx->reconFbHeight = reconFbHeight;
  return TRUE;
}

static BOOL AllocateYuvBufferHeader(AMVMultiCtx* ctx)
{
  Uint32 i;
  FrameBufferAllocInfo srcFbAllocInfo;
  Uint32 srcFbSize;
  Uint32 srcFbStride;
  EncOpenParam encOpenParam = ctx->encOpenParam;
  Uint32 srcFbWidth = VPU_ALIGN8(encOpenParam.picWidth);
  Uint32 srcFbHeight  = VPU_ALIGN8(encOpenParam.picHeight);

  memset(&srcFbAllocInfo, 0x00, sizeof(FrameBufferAllocInfo));

  /* calculate sourec buffer stride and size */
  srcFbStride = CalcStride(srcFbWidth, srcFbHeight,
        (FrameBufferFormat)encOpenParam.srcFormat,
        encOpenParam.cbcrInterleave, LINEAR_FRAME_MAP, FALSE);
  srcFbSize = VPU_GetFrameBufSize(encOpenParam.coreIdx, srcFbStride,
        srcFbHeight, LINEAR_FRAME_MAP,
        (FrameBufferFormat)encOpenParam.srcFormat,
        encOpenParam.cbcrInterleave, NULL);

  srcFbAllocInfo.mapType = LINEAR_FRAME_MAP;
  srcFbAllocInfo.format  = (FrameBufferFormat)encOpenParam.srcFormat;
  srcFbAllocInfo.cbcrInterleave = encOpenParam.cbcrInterleave;
  srcFbAllocInfo.stride  = srcFbStride;
  srcFbAllocInfo.height  = srcFbHeight;
  srcFbAllocInfo.endian  = encOpenParam.sourceEndian;
  srcFbAllocInfo.type    = FB_TYPE_PPU;
  srcFbAllocInfo.num     = ctx->src_num;
  srcFbAllocInfo.nv21    = encOpenParam.nv21;

#if 1  //allocat later when use input buffer, in case DMA buffer no need copy.
    vdi_lock(encOpenParam.coreIdx);
    for (i = 0; i < ctx->src_num; i++) {
        ctx->pFbSrcMem[i].size = srcFbSize;
        if (vdi_allocate_dma_memory(encOpenParam.coreIdx, &ctx->pFbSrcMem[i]) < 0) {
            vdi_unlock(encOpenParam.coreIdx);
            VLOG(ERR, "fail to allocate src buffer\n");
            ctx->pFbSrcMem[i].size = 0;
            return FALSE;
        }
        ctx->pFbSrc[i].bufY  = ctx->pFbSrcMem[i].phys_addr;
        ctx->pFbSrc[i].bufCb = (PhysicalAddress) - 1;
        ctx->pFbSrc[i].bufCr = (PhysicalAddress) - 1;
        ctx->pFbSrc[i].updateFbInfo = TRUE;
    }
    vdi_unlock(encOpenParam.coreIdx);
#else
   for (i = 0; i < ctx->src_num; i++) {
        ctx->pFbSrc[i].bufY  = (PhysicalAddress) - 1;
        ctx->pFbSrc[i].bufCb = (PhysicalAddress) - 1;
        ctx->pFbSrc[i].bufCr = (PhysicalAddress) - 1;
        ctx->pFbSrc[i].updateFbInfo = TRUE;
   }
#endif
    ctx->srcFbStride = srcFbStride;
    ctx->srcFbSize = srcFbSize;
    ctx->srcFbAllocInfo = srcFbAllocInfo;
    return TRUE;
}

static BOOL RegisterFrameBuffers(AMVMultiCtx *ctx)
{
  FrameBuffer *pReconFb;
  FrameBuffer *pSrcFb;
  FrameBufferAllocInfo srcFbAllocInfo;
  Uint32 reconFbStride;
  Uint32 reconFbHeight;
  RetCode result;
  BOOL success;
  Uint32 idx;

  pReconFb = ctx->pFbRecon;
  reconFbStride = ctx->reconFbStride;
  reconFbHeight = ctx->reconFbHeight;
  result = VPU_EncRegisterFrameBuffer(ctx->enchandle, pReconFb,
        ctx->fb_num, reconFbStride, reconFbHeight, COMPRESSED_FRAME_MAP);
  if (result != RETCODE_SUCCESS) {
    VLOG(ERR, "%s:%d Failed to VPU_EncRegisterFrameBuffer(%d)\n", __FUNCTION__, __LINE__, result);
    return FALSE;
  }
  VLOG(INFO, " finish register frame buffer \n");

  pSrcFb         = ctx->pFbSrc;
  srcFbAllocInfo = ctx->srcFbAllocInfo;
  if (VPU_EncAllocateFrameBuffer(ctx->enchandle, srcFbAllocInfo, pSrcFb) != RETCODE_SUCCESS) {
    VLOG(ERR, "VPU_EncAllocateFrameBuffer fail to allocate source frame buffer\n");
    return FALSE;
  }

  if (ctx->mInitParams.roi_enable
        || ctx->mInitParams.lambda_map_enable
        || ctx->mInitParams.mode_map_enable) {
    vdi_lock(ctx->encOpenParam.coreIdx);
    for (idx = 0; idx < ctx->src_num; idx++) {
      ctx->vbCustomMap[idx].size = (ctx->encOpenParam.bitstreamFormat == STD_AVC) ? MAX_MB_NUM : MAX_CTU_NUM * 8;
      if (vdi_allocate_dma_memory(ctx->encOpenParam.coreIdx, &ctx->vbCustomMap[idx]) < 0) {
        vdi_unlock(ctx->encOpenParam.coreIdx);
        ctx->vbCustomMap[idx].size = 0;
        VLOG(ERR, "fail to allocate ROI buffer\n");
        return FALSE;
      }
    }
    vdi_unlock(ctx->encOpenParam.coreIdx);
  }
  return TRUE;
}

#if SUPPORT_SCALE
AMVEnc_Status ge2d_colorFormat(AMVEncFrameFmt format) {
    switch (format) {
        case AMVENC_RGB888:
            SRC1_PIXFORMAT = PIXEL_FORMAT_RGB_888;
            SRC2_PIXFORMAT = PIXEL_FORMAT_RGB_888;
            return AMVENC_SUCCESS;
        case AMVENC_RGBA8888:
            SRC1_PIXFORMAT = PIXEL_FORMAT_RGBA_8888;
            SRC2_PIXFORMAT = PIXEL_FORMAT_RGBA_8888;
            return AMVENC_SUCCESS;
        default:
            VLOG(ERR, "not support color format!");
            return AMVENC_FAIL;
    }
}
#endif

amv_enc_handle_t AML_MultiEncInitialize(AMVEncInitParams* encParam)
{

  RetCode retCode;
  ProductInfo productInfo;
  SecAxiUse secAxiUse;
  Uint32 coreIdx, idx;
  RetCode result;
  AMVMultiCtx * ctx = (AMVMultiCtx* )malloc(sizeof(AMVMultiCtx));

  if (ctx == NULL) {
    VLOG(ERR, "ctx == NULL");
    return (amv_enc_handle_t) ctx;
  }

  memset(ctx, 0, sizeof(AMVMultiCtx));
  ctx->magic_num = MULTI_ENC_MAGIC;

  ctx->instance_id = 0;
  ctx->src_idx = 0;
  ctx->enc_counter = 0;
  ctx->enc_width = encParam->width;
  ctx->enc_height = encParam->height;
  // copy a backup on initial parameter
  memcpy((char *)&ctx->mInitParams, encParam, sizeof(AMVEncInitParams));
  if (encParam->idr_period == -1) {
    ctx->idrPeriod = 0;
  } else {
    ctx->idrPeriod = encParam->idr_period;
  }

  VLOG(INFO, "ctx->idrPeriod: %d\n", ctx->idrPeriod);

  ctx->bitrate = encParam->bitrate;
  ctx->frame_rate = encParam->frame_rate;
  ctx->mPrependSPSPPSToIDRFrames = encParam->prepend_spspps_to_idr_frames;
  ctx->mNumPlanes = 0;
  SetupEncoderOpenParam(&(ctx->encOpenParam), encParam);
  coreIdx = ctx->encOpenParam.coreIdx;
  retCode = VPU_Init(coreIdx);
  if (retCode != RETCODE_SUCCESS && retCode != RETCODE_CALLED_BEFORE) {
        VLOG(INFO, "%s:%d Failed to VPU_Init, ret(%08x)\n", __FUNCTION__, __LINE__, retCode);
        goto fail_exit;
  }
  retCode = PrintVpuProductInfo(coreIdx, &productInfo);
  if (retCode == RETCODE_VPU_RESPONSE_TIMEOUT ) {
     VLOG(INFO, "<%s:%d> Failed to PrintVpuProductInfo()\n", __FUNCTION__, __LINE__);
     HandleEncoderError(ctx->enchandle, 0, NULL);
     goto fail_exit;
  }
  ctx->cyclePerTick = 32768;
  if (((productInfo.stdDef1>>27)&1) == 1 )
    ctx->cyclePerTick = 256;
  /* prepare stream buffers */
  ctx->bsBuffer[0].size = ENC_STREAM_BUF_SIZE;

  vdi_lock(coreIdx);

  if (vdi_allocate_dma_memory(coreIdx, &ctx->bsBuffer[0]) < 0) {
    VLOG(ERR, "%s:%d fail to allocate bitstream buffer\n", __FUNCTION__, __LINE__);
    ctx->bsBuffer[0].size = 0;
    vdi_unlock(coreIdx);
    goto fail_exit;
  }

  vdi_unlock(coreIdx);
  ctx->encOpenParam.bitstreamBuffer = ctx->bsBuffer[0].phys_addr;
  ctx->encOpenParam.bitstreamBufferSize = ctx->bsBuffer[0].size;

    VLOG(DEBUG, "ctx->encOpenParam.bitstreamBuffer %0x, ctx->encOpenParam.bitstreamBufferSiz %0x",
        ctx->encOpenParam.bitstreamBuffer, ctx->encOpenParam.bitstreamBufferSize);

  // customer buffers allocat and settins
  /* Allocate Buffer and Set Data as needed*/
  if (ctx->encOpenParam.EncStdParam.waveParam.scalingListEnable) {
    ctx->vbScalingList.size = 0x1000;
    vdi_lock(coreIdx);
    if (vdi_allocate_dma_memory(coreIdx, &ctx->vbScalingList) < 0) {
      vdi_unlock(coreIdx);
      VLOG(ERR, "fail to allocate scaling list buffer\n");
      ctx->vbScalingList.size = 0;
      goto fail_exit;
    }
    vdi_unlock(coreIdx);
    ctx->encOpenParam.EncStdParam.waveParam.userScalingListAddr = ctx->vbScalingList.phys_addr;
    // fill the scaling list data here, configure APIs to be added
    //parse_user_scaling_list(&ctx->scalingList, testEncConfig.scaling_list_file, testEncConfig.stdMode);
    vdi_write_memory(coreIdx, ctx->vbScalingList.phys_addr,
                (unsigned char*)&ctx->scalingList,
                ctx->vbScalingList.size,
                VDI_LITTLE_ENDIAN);
  }

  if (ctx->encOpenParam.EncStdParam.waveParam.customLambdaEnable) {
    ctx->vbCustomLambda.size = 0x200;
    vdi_lock(coreIdx);
    if (vdi_allocate_dma_memory(coreIdx, &ctx->vbCustomLambda) < 0) {
      vdi_unlock(coreIdx);
      VLOG(ERR, "fail to allocate Lambda map buffer\n");
      ctx->vbCustomLambda.size = 0;
      goto fail_exit;
    }
    vdi_unlock(coreIdx);
    ctx->encOpenParam.EncStdParam.waveParam.customLambdaAddr = ctx->vbCustomLambda.phys_addr;
    // fill the customLambda buffer,API to be add as needed.
    //parse_custom_lambda(ctx->customLambda, testEncConfig.custom_lambda_file);
    vdi_write_memory(coreIdx, ctx->vbCustomLambda.phys_addr, (unsigned char*)&ctx->customLambda[0], ctx->vbCustomLambda.size, VDI_LITTLE_ENDIAN);
  }
  // real open the encoder with OpenParam
  if ((result = VPU_EncOpen(&ctx->enchandle, &ctx->encOpenParam)) != RETCODE_SUCCESS) {
    VLOG(ERR, "VPU_EncOpen failed Error code is 0x%x \n", result);
    ctx->enchandle = NULL;
    goto fail_exit;
  }
  // set rotate and mirror
  //VPU_EncGiveCommand(ctx->enchandle, ENABLE_LOGGING, 0);
  if (encParam->rotate_angle != 0 || encParam->mirror != 0) {
        VPU_EncGiveCommand(ctx->enchandle, ENABLE_ROTATION, 0);
        VPU_EncGiveCommand(ctx->enchandle, ENABLE_MIRRORING, 0);
        VPU_EncGiveCommand(ctx->enchandle, SET_ROTATION_ANGLE, &ctx->mInitParams.rotate_angle);
        VPU_EncGiveCommand(ctx->enchandle, SET_MIRROR_DIRECTION, &ctx->mInitParams.mirror);
    }
  memset(&secAxiUse, 0x00, sizeof(SecAxiUse));
  secAxiUse.u.wave.useEncRdoEnable =0; //USE_RDO_INTERNAL_BUF
  secAxiUse.u.wave.useEncLfEnable = 0; //USE_LF_INTERNAL_BUF
  VPU_EncGiveCommand(ctx->enchandle, SET_SEC_AXI, &secAxiUse);
  VPU_EncGiveCommand(ctx->enchandle, SET_CYCLE_PER_TICK,   (void*)&ctx->cyclePerTick);
  // get the seqence buffer requirement info
  if (SetSequenceInfo(ctx) == FALSE)
    goto fail_exit;
  // Allocate and register encoder frame buffer
  if(AllocateReconFrameBuffer(ctx) == FALSE)
    goto fail_exit;
  if(AllocateYuvBufferHeader(ctx) == FALSE)
    goto fail_exit;
  if(RegisterFrameBuffers(ctx) == FALSE)
    goto fail_exit;

#if SUPPORT_SCALE
    if ((encParam->width != encParam->src_width) || (encParam->height != encParam->src_height) || true) {
        memset(&amlge2d,0x0,sizeof(aml_ge2d_t));
        aml_ge2d_info_t *pge2dinfo = &amlge2d.ge2dinfo;
        memset(pge2dinfo, 0, sizeof(aml_ge2d_info_t));
        memset(&(pge2dinfo->src_info[0]), 0, sizeof(buffer_info_t));
        memset(&(pge2dinfo->src_info[1]), 0, sizeof(buffer_info_t));
        memset(&(pge2dinfo->dst_info), 0, sizeof(buffer_info_t));

        set_ge2dinfo(pge2dinfo, encParam);

        int ret = aml_ge2d_init(&amlge2d);
        if (ret < 0) {
            VLOG(ERR, "encode open ge2d failed, ret=0x%x", ret);
            return AMVENC_FAIL;
        }
        ctx-> ge2d_initial_done = 1;
        INIT_GE2D = true;
    }
#endif
  return (amv_enc_handle_t) ctx;
fail_exit:
  if (ctx->enchandle)
    VPU_EncClose(ctx->enchandle);
  if (ctx->vbCustomLambda.size)
    vdi_free_dma_memory(coreIdx, &ctx->vbCustomLambda);
  if (ctx->vbScalingList.size)
    vdi_free_dma_memory(coreIdx, &ctx->vbScalingList);
  if (ctx->bsBuffer[0].size)
    vdi_free_dma_memory(coreIdx, &ctx->bsBuffer[0]);
  for (idx = 0; idx < MAX_REG_FRAME; idx++) {
    if (ctx->pFbReconMem[idx].size)
      vdi_free_dma_memory(coreIdx, &ctx->pFbReconMem[idx]);
    if (ctx->vbCustomMap[idx].size)
      vdi_free_dma_memory(coreIdx, &ctx->vbCustomMap[idx]);
  }
  for (idx = 0; idx < ENC_SRC_BUF_NUM; idx++) {
    if (ctx->pFbSrcMem[idx].size)
      vdi_free_dma_memory(coreIdx, &ctx->pFbSrcMem[idx]);
  }
#if SUPPORT_SCALE
  if (ctx->ge2d_initial_done) {
    aml_ge2d_exit();
    ge2dinfo.src_info[0].vaddr = NULL;
  }
#endif
  free(ctx);
  return (amv_enc_handle_t) NULL;
}

AMVEnc_Status AML_MultiEncSetInput(amv_enc_handle_t ctx_handle,
                               AMVMultiEncFrameIO* input) {
  Uint32 src_stride;
  Uint32 luma_stride, chroma_stride;
  Uint32 size_src_luma, size_src_chroma;
  char *y_dst = NULL;
  char *u_dst = NULL;
  char *v_dst = NULL;
  char *src = NULL;
  int idx, idx_1, endian;
  bool width32alinged = true; //width is multiple of 32 or not
  Uint32 is_DMA_buffer = 0;
  EncParam * param;

  AMVMultiCtx * ctx = (AMVMultiCtx* )ctx_handle;
  if (ctx == NULL) return AMVENC_FAIL;
  if (ctx->magic_num != MULTI_ENC_MAGIC) return AMVENC_FAIL;

  if (ctx->enc_width % 32) {
    width32alinged = false;
  }
  if ((input->num_planes >0) && (input->type == DMA_BUFF)) { /* DMA buffer, no need allocate */
    is_DMA_buffer = 1;
  }
  param = &(ctx->encParam);

  ctx->op_flag = input->op_flag;
  ctx->fmt = input->fmt;
  if (ctx->fmt != AMVENC_NV12 && ctx->fmt != AMVENC_NV21 && ctx->fmt != AMVENC_YUV420P) {
        if (INIT_GE2D) {
#if SUPPORT_SCALE
            if (ge2d_colorFormat(ctx->fmt) == AMVENC_SUCCESS) {
                VLOG(DEBUG, "The %d of color format that HEVC need ge2d to change!", ctx->fmt);
            } else
#endif
            {
                VLOG(INFO, "Encoder only support NV12/NV21, not support %d", ctx->fmt);
                return AMVENC_NOT_SUPPORTED;
            }
        }
  }
  if (ctx->fmt == AMVENC_NV12) {
    //uv_swap = 0;
  }
  VLOG(INFO, "fmt %d , is dma %d \n", ctx->fmt, is_DMA_buffer);
#if 0
  if (ctx->bitrate != input->bitrate) { // change bitrate
    ctx->bitrate = input->bitrate;
    VLOG(INFO, "Change set bitrate to %d", Handle->bitrate);
    Wave4VpuEncSeqInit(Handle, 0);
  }
#endif
  if (is_DMA_buffer) {
    src_stride = wave420l_align16(ctx->enc_width);
  } else {
    src_stride = wave420l_align32(ctx->enc_width);
  }
  luma_stride = src_stride;
  chroma_stride = src_stride;
  //  Pick a no used src frame buffer descriptor
 idx_1 = -1;
  for (idx = 0; idx< ENC_SRC_BUF_NUM; idx++) {
     if(ctx->encodedSrcFrmIdxArr[idx] == 0
        && ctx->pFbSrcMem[idx].size) break; // free and located mem
     if(ctx->encodedSrcFrmIdxArr[idx] == 0
        && idx_1 == -1) idx_1 = idx;
  }
  if (idx == ENC_SRC_BUF_NUM) { //no found, just get a free header
     if(idx_1 >= 0 ) idx = idx_1;
     else {
       VLOG(ERR, "can not find a free src buffer\n");
       return AMVENC_FAIL;
     }
  }

  if (is_DMA_buffer == 0) {
    // need allocate buffer and copy
    ctx->pFbSrc[idx].dma_buf_planes = 0; //clean the DMA buffer
    if(ctx->pFbSrcMem[idx].size == 0) { // allocate buffer
      vdi_lock(ctx->encOpenParam.coreIdx);
      ctx->pFbSrcMem[idx].size = ctx->srcFbSize;
      if (vdi_allocate_dma_memory(ctx->encOpenParam.coreIdx, &ctx->pFbSrcMem[idx]) < 0) {
        vdi_unlock(ctx->encOpenParam.coreIdx);
        VLOG(ERR, "fail to allocate src buffer, short of memory\n");
        ctx->pFbSrcMem[idx].size = 0;
        return AMVENC_FAIL;
       }
       vdi_unlock(ctx->encOpenParam.coreIdx);
       ctx->pFbSrc[idx].bufY = ctx->pFbSrcMem[idx].phys_addr;
    }
#if SUPPORT_SCALE
    if ((input->scale_width !=0 && input->scale_height !=0) || input->crop_left != 0 ||
        input->crop_right != 0 || input->crop_top != 0 || input->crop_bottom != 0 ||
        (ctx->fmt != AMVENC_NV12 && ctx->fmt != AMVENC_NV21 && ctx->fmt != AMVENC_YUV420P)) {
        if (INIT_GE2D) {
            INIT_GE2D = false;
            amlge2d.ge2dinfo.src_info[0].format = SRC1_PIXFORMAT;
            amlge2d.ge2dinfo.src_info[1].format = SRC2_PIXFORMAT;
            int ret = aml_ge2d_mem_alloc(&amlge2d);
            if (ret < 0) {
                VLOG(ERR, "encode ge2di mem alloc failed, ret=0x%x", ret);
                return AMVENC_FAIL;
            }
            VLOG(DEBUG, "ge2d init successful!");
        }
        VLOG(DEBUG, "HEVC TEST sclale, enc_width:%d enc_height:%d  pitch:%d height:%d, line:%d",
                 ctx->enc_width, ctx->enc_height, input->pitch, input->height, __LINE__);
        if (input->pitch % 32) {
            VLOG(ERR, "HEVC crop and scale must be 32bit aligned");
            return AMVENC_FAIL;
        }
        if (ctx->fmt == AMVENC_RGBA8888)
            memcpy((void *)amlge2d.ge2dinfo.src_info[0].vaddr, (void *)input->YCbCr[0], 4 * input->pitch * input->height);
        else if (ctx->fmt == AMVENC_NV12 || ctx->fmt == AMVENC_NV21) {
            memcpy((void *)amlge2d.ge2dinfo.src_info[0].vaddr, (void *)input->YCbCr[0], input->pitch * input->height);
            memcpy((void *) ((char *)amlge2d.ge2dinfo.src_info[0].vaddr + input->pitch * input->height), (void *)input->YCbCr[1], input->pitch * input->height / 2);
        } else if (ctx->fmt == AMVENC_YUV420P) {
            memcpy((void *)amlge2d.ge2dinfo.src_info[0].vaddr, (void *)input->YCbCr[0], input->pitch * input->height);
            memcpy((void *) ((char *)amlge2d.ge2dinfo.src_info[0].vaddr + input->pitch * input->height), (void *)input->YCbCr[1], input->pitch * input->height / 4);
            memcpy((void *) ((char *)amlge2d.ge2dinfo.src_info[0].vaddr + (input->pitch * input->height * 5) /4), (void *)input->YCbCr[2], input->pitch * input->height / 4);
        } else if (ctx->fmt == AMVENC_RGB888)
            memcpy((void *)amlge2d.ge2dinfo.src_info[0].vaddr, (void *)input->YCbCr[0], input->pitch * input->height * 3);


        do_strechblit(&amlge2d.ge2dinfo, input);
        aml_ge2d_invalid_cache(&amlge2d.ge2dinfo);
        size_src_luma = luma_stride * wave420l_align32(ctx->enc_height);
        size_src_chroma = luma_stride * (wave420l_align16(ctx->enc_height) / 2);
        y = (char *) ctx->pFbSrcMem[idx].virt_addr;
        if (ctx->enc_width % 32) {
            for (int i = 0; i < ctx->enc_height; i++) {
                memcpy(y + i * luma_stride, (void *) ((char *) amlge2d.ge2dinfo.dst_info.vaddr + i * Handle->enc_width), ctx->enc_width);
            }
        } else {
            memcpy((void *)ctx->pFbSrcMem[idx].virt_addr, amlge2d.ge2dinfo.dst_info.vaddr, ctx->enc_width * ctx->enc_height);
        }
        y = (char *) (ctx->pFbSrcMem[idx].virt_addr + size_src_luma);
        memcpy(y, (void *) ((char *)amlge2d.ge2dinfo.dst_info.vaddr + ctx->enc_width * ctx->enc_height), chroma_stride * ctx->enc_height / 2);
   } else
#endif
    {
      // set frame buffers
      // calculate required buffer size
      size_src_luma = luma_stride * wave420l_align32(ctx->enc_height);
      size_src_chroma = luma_stride * (wave420l_align16(ctx->enc_height) / 2);
      endian = ctx->pFbSrc[idx].endian;

      y_dst = (char *) ctx->pFbSrcMem[idx].virt_addr;

  //for (idx_1 = 0; idx_1 < size_src_luma; idx_1++)  *(y_dst + idx_1) = 0;
  //for (idx_1 = 0; idx_1 < size_src_chroma; idx_1++)  *(y_dst + size_src_luma + idx_1) = 0x80;
//memset(y_dst, 0x0, size_src_luma);
//memset(y_dst + size_src_luma, 0x80, size_src_chroma);

      src = (char *) input->YCbCr[0];
      VLOG(INFO, "idx %d luma %d src %p dst %p  width %d, height %d \n", idx, size_src_luma, src, y_dst, ctx->enc_width, ctx->enc_height);

//  vdi_write_memory(ctx->encOpenParam.coreIdx, ctx->pFbSrcMem[idx].phys_addr, src, ctx->enc_width*ctx->enc_height, 0);
// VLOG(ERR, "aeeeeeeaaaa, idx %d u_dst %d src %p src %p \n", idx, size_src_luma, input->YCbCr[1], input->YCbCr[2]);
      yuv_plane_memcpy(ctx->encOpenParam.coreIdx, ctx->pFbSrc[idx].bufY, src,
                              ctx->enc_width, ctx->enc_height, luma_stride, width32alinged, endian);

      if (ctx->fmt == AMVENC_NV12 || ctx->fmt == AMVENC_NV21) {
        src = (char *)input->YCbCr[1];
        yuv_plane_memcpy(ctx->encOpenParam.coreIdx, ctx->pFbSrc[idx].bufCb,
                              src, ctx->enc_width, ctx->enc_height / 2, chroma_stride, width32alinged, endian);
      } else if (ctx->fmt == AMVENC_YUV420P) {
        src = (char *)input->YCbCr[1];
        yuv_plane_memcpy(ctx->encOpenParam.coreIdx, ctx->pFbSrc[idx].bufCb,
                        src, ctx->enc_width/2, ctx->enc_height/ 2, chroma_stride / 2, width32alinged, endian);
//        v_dst = (char *)(ctx->pFbSrcMem[idx].virt_addr + size_src_luma + size_src_luma / 4);
        src = (char *)input->YCbCr[2];
        yuv_plane_memcpy(ctx->encOpenParam.coreIdx, ctx->pFbSrc[idx].bufCr,
                        src, ctx->enc_width/2, ctx->enc_height/ 2, chroma_stride / 2, width32alinged, endian);
      }
    }
    vdi_flush_memory(ctx->encOpenParam.coreIdx, &ctx->pFbSrcMem[idx]);
    VLOG(INFO,"frame %d stride %d address Y 0x%x cb 0x%x cr 0x%x \n",
                idx, size_src_luma, ctx->pFbSrc[idx].bufY, ctx->pFbSrc[idx].bufCb,ctx->pFbSrc[idx].bufCr);
#if 0
    if (ctx->fmt == AMVENC_NV12 || ctx->fmt == AMVENC_NV21) {
      ctx->pFbSrc[idx].bufCb = (PhysicalAddress) (ctx->pFbSrc[idx].bufY + size_src_luma);
      ctx->pFbSrc[idx].bufCr = (PhysicalAddress) - 1;
	  ctx->pFbSrc[idx].cbcrInterleave  = 1;
    } else if (ctx->fmt == AMVENC_YUV420P) {
       ctx->pFbSrc[idx].bufCb = (PhysicalAddress) (ctx->pFbSrc[idx].bufY + size_src_luma);
       ctx->pFbSrc[idx].bufCr = (PhysicalAddress) (ctx->pFbSrc[idx].bufY + size_src_luma + size_src_luma / 4);
    }
#endif
    VLOG(INFO,"frame %d stride %d address Y 0x%x cb 0x%x cr 0x%x \n",
                idx, size_src_luma, ctx->pFbSrc[idx].bufY,
                ctx->pFbSrc[idx].bufCb,ctx->pFbSrc[idx].bufCr);
  }
  else { //DMA buffer
    ctx->pFbSrc[idx].dma_buf_planes = input->num_planes;
    ctx->pFbSrc[idx].dma_shared_fd[0] = input->shared_fd[0];
    ctx->pFbSrc[idx].dma_shared_fd[1] = input->shared_fd[1];
    ctx->pFbSrc[idx].dma_shared_fd[2] = input->shared_fd[2];
    VLOG(INFO,"%s:%d Set DMA buffer index %d planes %d fd[%d, %d, %d]\n",
         __FUNCTION__, __LINE__, idx, input->num_planes,
        input->shared_fd[0], input->shared_fd[1], input->shared_fd[2]);
  }
  ctx->FrameIO[idx] = *input;
  ctx->encodedSrcFrmIdxArr[idx] = 1; // occupic the frames.
  ctx->encMEMSrcFrmIdxArr[param->srcIdx]  = idx; //indirect  link due to reodering. srcIdx increase linear.
  ctx->pFbSrc[idx].stride = src_stride; /**< A horizontal stride for given frame buffer */

  VLOG(INFO, "Assign src buffer,input idx %d poolidx %d stride %d \n",
                param->srcIdx, idx, src_stride);
  if (ctx->bsBuffer[idx].size == 0) { // allocate buffer
    ctx->bsBuffer[idx].size = ENC_STREAM_BUF_SIZE;
	vdi_lock(ctx->encOpenParam.coreIdx);
	if (vdi_allocate_dma_memory(ctx->encOpenParam.coreIdx, &ctx->bsBuffer[idx]) < 0) {
		VLOG(ERR, "%s:%d fail to allocate bitstream buffer\n", __FUNCTION__, __LINE__);
		ctx->bsBuffer[idx].size = 0;
		vdi_unlock(ctx->encOpenParam.coreIdx);
		return AMVENC_FAIL;
	}
        vdi_unlock(ctx->encOpenParam.coreIdx);
  }
  param->sourceFrame = &ctx->pFbSrc[idx];
  param->picStreamBufferAddr = ctx->bsBuffer[idx].phys_addr;
  param->picStreamBufferSize = ctx->bsBuffer[idx].size;


  param->srcIdx                             = param->srcIdx;
  param->srcEndFlag                         = 0; //in->last;
//    encParam->sourceFrame                        = &in->fb;
  param->sourceFrame->sourceLBurstEn        = 0;
#if 0
    if (testEncConfig.useAsLongtermPeriod > 0 && testEncConfig.refLongtermPeriod > 0) {
        encParam->useCurSrcAsLongtermPic         = (frameIdx % testEncConfig.useAsLongtermPeriod) == 0 ? 1 : 0;
        encParam->useLongtermRef                 = (frameIdx % testEncConfig.refLongtermPeriod)   == 0 ? 1 : 0;
    }
#else
  param->useCurSrcAsLongtermPic         = 0;
  param->useLongtermRef                 = 0;
#endif
  param->skipPicture                        = 0;
  param->forceAllCtuCoefDropEnable	         = 0;

  param->forcePicQpEnable		             = 0;
  param->forcePicQpI			             = 0;
  param->forcePicQpP			             = 0;
  param->forcePicQpB			             = 0;
  param->forcePicTypeEnable	             = 0;
  param->forcePicType			             = 0;

    // FW will encode header data implicitly when changing the header syntaxes
  param->codeOption.implicitHeaderEncode    = 1;
  param->codeOption.encodeAUD               = 0;
  param->codeOption.encodeEOS               = 0;
  param->codeOption.encodeEOB               = 0;

  param->customMapOpt.customLambdaMapEnable = 0; //testEncConfig.lambda_map_enable;
  param->customMapOpt.customModeMapEnable = 0; //testEncConfig.mode_map_flag & 0x1;
  param->customMapOpt.customCoefDropEnable  = 0; //(testEncConfig.mode_map_flag & 0x2) >> 1;
  param->customMapOpt.customRoiMapEnable    = 0; //testEncConfig.roi_enable;

  return AMVENC_SUCCESS;
}

AMVEnc_Status AML_MultiEncHeader(amv_enc_handle_t ctx_handle,
                                unsigned char* buffer,
                                unsigned int* buf_nal_size)
{
  int ret;
  EncHeaderParam encHeaderParam;
  EncOutputInfo  encOutputInfo;
  ENC_INT_STATUS status;
  int retry_cnt = 0, header_size = 0;
  PhysicalAddress prdPrt, pwrPtr;
  RetCode result= RETCODE_SUCCESS;
  AMVMultiCtx * ctx = (AMVMultiCtx* ) ctx_handle;
  ENC_QUERY_WRPTR_SEL encWrPtrSel = GET_ENC_PIC_DONE_WRPTR;

  if (ctx == NULL) return AMVENC_FAIL;
  if (ctx->magic_num != MULTI_ENC_MAGIC) return AMVENC_FAIL;
  //EncHandle handle = ctx->enchandle;
  memset(&encHeaderParam, 0x00, sizeof(EncHeaderParam));

  /*Just reuse bitstream buffer or allocat a new one*/
  encHeaderParam.buf  = ctx->bsBuffer[0].phys_addr;
  encHeaderParam.size = ctx->bsBuffer[0].size;


  if (ctx->encOpenParam.bitstreamFormat == STD_HEVC) {
    encHeaderParam.headerType = CODEOPT_ENC_VPS | CODEOPT_ENC_SPS | CODEOPT_ENC_PPS;
  }
  else {
    // H.264
    encHeaderParam.headerType = CODEOPT_ENC_SPS | CODEOPT_ENC_PPS;
  }
  retry_cnt = 0;
  PhysicalAddress paRdPtr = 0;
  PhysicalAddress paWrPtr = 0;
  while(1)
  {
        result = VPU_EncGiveCommand(ctx->enchandle, ENC_PUT_VIDEO_HEADER, &encHeaderParam);
        if ( result == RETCODE_QUEUEING_FAILURE ) continue;
#if defined(HAPS_SIM) || defined(CNM_SIM_DPI_INTERFACE)
        usleep(1000); //osal_msleep(1);
#endif

  //  wait interrupt */
    if ((status = HandlingInterruptFlag(ctx)) == ENC_INT_STATUS_DONE) {
      break;
    }
    else if (status == ENC_INT_STATUS_NONE) {
      usleep(1000); //osal_msleep(1);
    }
    else if (status == ENC_INT_STATUS_TIMEOUT) {
       VLOG(INFO, "%s:%d INSTANCE #%d INTERRUPT TIMEOUT\n", __FUNCTION__, __LINE__, ctx->enchandle->instIndex);
       HandleEncoderError(ctx->enchandle, ctx->frameIdx, NULL);
       return AMVENC_FAIL;
    } else {
      VLOG(INFO, "%s:%d Unknown interrupt status: %d\n", __FUNCTION__, __LINE__, status);
      return AMVENC_FAIL;
    }
    usleep(1000); //osal_msleep(1);
  }

  DisplayEncodedInformation(ctx->enchandle, ctx->encOpenParam.bitstreamFormat, 0, NULL, 0, 0, 1);

  VPU_EncGiveCommand(ctx->enchandle, ENC_WRPTR_SEL, &encWrPtrSel);
  encOutputInfo.result = VPU_EncGetOutputInfo(ctx->enchandle, &encOutputInfo);

    if (encOutputInfo.result == RETCODE_REPORT_NOT_READY) {
        return TRUE; /* Not encoded yet */
    }
    else if (encOutputInfo.result == RETCODE_VLC_BUF_FULL) {
        VLOG(ERR, "VLC BUFFER FULL!!! ALLOCATE MORE TASK BUFFER(%d)!!!\n", ONE_TASKBUF_SIZE_FOR_CQ);
    }
    else if (encOutputInfo.result != RETCODE_SUCCESS) {
        /* ERROR */
        VLOG(ERR, "Failed to encode error = %d\n", encOutputInfo.result);
        HandleEncoderError(ctx->enchandle, encOutputInfo.encPicCnt, &encOutputInfo);
        VPU_SWReset(ctx->encOpenParam.coreIdx, SW_RESET_SAFETY, ctx->enchandle);
        return FALSE;
    }
    else {
        ;/* SUCCESS */
    }

    paRdPtr = encOutputInfo.rdPtr;
    paWrPtr = encOutputInfo.wrPtr;
    header_size = encOutputInfo.bitstreamSize;
    VLOG(DEBUG, "header_size %d paWrPtr-paRdPtr %d", header_size, paWrPtr-paRdPtr);
  if (paRdPtr != ctx->bsBuffer[0].phys_addr) {
    VLOG(ERR, "BS buffer no match!! expect %x acutual %x size %d\n",
        ctx->bsBuffer[0].phys_addr, paRdPtr, header_size);
  }

  VLOG(INFO, "%s:%d Enc HEADER size %d\n", __FUNCTION__, __LINE__, header_size);
  *buf_nal_size = header_size;
  // cache invalid to read out by CPU
  //vdi_invalitate_memory(ctx->encOpenParam.coreIdx, &ctx->bsBuffer[0]);
  vdi_read_memory(ctx->encOpenParam.coreIdx,
        ctx->bsBuffer[0].phys_addr, buffer, header_size,
        ctx->encOpenParam.streamEndian);

  return AMVENC_SUCCESS;
}

AMVEnc_Status AML_MultiEncNAL(amv_enc_handle_t ctx_handle,
                             unsigned char* buffer,
                             unsigned int* buf_nal_size,
                             AMVMultiEncFrameIO *Retframe) {
  AMVEnc_Status ret = AMVENC_FAIL;
  RetCode                 result;
  EncParam*               encParam;
  EncOutputInfo           encOutputInfo;
  ENC_INT_STATUS          intStatus;
  ENC_QUERY_WRPTR_SEL     encWrPtrSel     = GET_ENC_PIC_DONE_WRPTR;
  int retry_cnt = 0;
  int end = 0, idx;
  AMVMultiCtx * ctx = (AMVMultiCtx* ) ctx_handle;
  if (ctx == NULL) return AMVENC_FAIL;
  if (ctx->magic_num != MULTI_ENC_MAGIC) return AMVENC_FAIL;

retry_point:
    if(ctx ->param_change_flag) {
        VPU_EncGiveCommand(ctx->enchandle, ENC_SET_PARA_CHANGE, &ctx->changeParam);
        ctx ->param_change_flag = 0;
    }
        encParam        = &ctx->encParam;

        result = VPU_EncStartOneFrame(ctx->enchandle, encParam);
        if (result == RETCODE_SUCCESS) {
            ctx->frameIdx++;
            encParam -> srcIdx++;
            if(encParam -> srcIdx >= ctx->src_num) encParam -> srcIdx = 0;
            VLOG(INFO, "VPU_EncStartOneFrame sucesss frameIdx %d srcIdx %d  \n", ctx->frameIdx, encParam -> srcIdx);
        }
        else if (result == RETCODE_QUEUEING_FAILURE) { // Just retry
            QueueStatusInfo qStatus;
            VLOG(ERR, "VPU_EncStartOneFrame retry code is 0x%x \n", result);
           // Just retry
            VPU_EncGiveCommand(ctx->enchandle, ENC_GET_QUEUE_STATUS, (void*)&qStatus);
            if (qStatus.instanceQueueCount == 0) {
                goto retry_point;//return TRUE;
            }
        }
        else { // Error
            VLOG(ERR, "VPU_EncStartOneFrame failed Error code is 0x%x \n", result);
            return AMVENC_FAIL;
        }


  while (retry_cnt++ < 100) {
    if ((intStatus=HandlingInterruptFlag(ctx)) == ENC_INT_STATUS_TIMEOUT) {
        VPU_SWReset(ctx->encOpenParam.coreIdx, SW_RESET_SAFETY, ctx->enchandle);
        VLOG(ERR, "<%s:%d> Timeout of encoder interrupt reset \n", __FUNCTION__, __LINE__);
       return AMVENC_FAIL;
    }
    else if (intStatus == ENC_INT_STATUS_FULL || intStatus == ENC_INT_STATUS_LOW_LATENCY) {
        PhysicalAddress         paRdPtr;
        PhysicalAddress         paWrPtr;
        int                     size;
        encWrPtrSel = (intStatus==ENC_INT_STATUS_FULL) ? GET_ENC_BSBUF_FULL_WRPTR : GET_ENC_LOW_LATENCY_WRPTR;
        VPU_EncGiveCommand(ctx->enchandle, ENC_WRPTR_SEL, &encWrPtrSel);
        VPU_EncGetBitstreamBuffer(ctx->enchandle, &paRdPtr, &paWrPtr, &size);
        VLOG(TRACE, "<%s:%d> INT_BSBUF_FULL %p, %p\n", __FUNCTION__, __LINE__, paRdPtr, paWrPtr);
        ctx->fullInterrupt = TRUE;
        return AMVENC_FAIL; //return TRUE;
    }
    else if (intStatus == ENC_INT_STATUS_NONE) usleep(1000); //osal_msleep(1);
    else if (intStatus == ENC_INT_STATUS_DONE) break; //success get  done intr
   }
    // get the interrupt
    // get result andc output */
    VPU_EncGiveCommand(ctx->enchandle, ENC_WRPTR_SEL, &encWrPtrSel);
    osal_memset(&encOutputInfo, 0x00, sizeof(EncOutputInfo));
    encOutputInfo.result = VPU_EncGetOutputInfo(ctx->enchandle, &encOutputInfo);
    if (encOutputInfo.result == RETCODE_REPORT_NOT_READY) {
         VLOG(ERR, "no encode yet !!!\n");
        return AMVENC_FAIL; //return TRUE; /* Not encoded yet */
    }
    else if (encOutputInfo.result == RETCODE_VLC_BUF_FULL) {
        VLOG(ERR, "VLC BUFFER FULL!!! ALLOCATE MORE TASK BUFFER(%d)!!!\n", ONE_TASKBUF_SIZE_FOR_CQ);
    }
    else if (encOutputInfo.result != RETCODE_SUCCESS) {
        /* ERROR */
        VLOG(ERR, "Failed to encode error = %d\n", encOutputInfo.result);
        HandleEncoderError(ctx->enchandle, encOutputInfo.encPicCnt, &encOutputInfo);
        VPU_SWReset(ctx->encOpenParam.coreIdx, SW_RESET_SAFETY, ctx->enchandle);
        return AMVENC_FAIL;
    }
    else {
        ;/* SUCCESS */
       VLOG(INFO, "Success one = %d\n", encOutputInfo.result);
   }

    if (encOutputInfo.reconFrameIndex == RECON_IDX_FLAG_CHANGE_PARAM) {
        VLOG(TRACE, "CHANGE PARAMETER!\n");
        goto retry_point; //return TRUE; /* Try again */
    }
    else {
        // out put frame info
        DisplayEncodedInformation(ctx->enchandle,
                        ctx->encOpenParam.bitstreamFormat,
                        ctx->frameIdx, &encOutputInfo,
                        encParam->srcEndFlag,
                        encParam->srcIdx,
                        1);
    }

    if ( encOutputInfo.result != RETCODE_SUCCESS ) {
        VLOG(ERR, "output info result error \n");
        return AMVENC_FAIL;
    }

    VLOG(INFO, "encOutputInfo.encSrcIdx %d  reconFrameIndex %d \n",
                encOutputInfo.encSrcIdx, encOutputInfo.reconFrameIndex);
    if (encOutputInfo.reconFrameIndex == RECON_IDX_FLAG_CHANGE_PARAM) {
        VLOG(TRACE, "CHANGE PARAMETER!\n");
 //       return TRUE; /* Try again */
    }
    if ( encOutputInfo.encSrcIdx >= 0) {
        idx = ctx->encMEMSrcFrmIdxArr[encOutputInfo.encSrcIdx];
        ctx->encodedSrcFrmIdxArr[idx] = 0;
        ctx->fullInterrupt  = FALSE;
    // copy frames
    if (encOutputInfo.bitstreamBuffer !=
        ctx->bsBuffer[idx].phys_addr) {
        VLOG(ERR, "BS buffer no match!! expect %x acutual %x size %d\n",
                ctx->bsBuffer[idx].phys_addr,
                encOutputInfo.bitstreamBuffer, encOutputInfo.bitstreamSize);
     }

  VLOG(INFO, "%s:%d Enc bitstream size %d pic_type %d \n", __FUNCTION__, __LINE__,
                encOutputInfo.bitstreamSize, encOutputInfo.picType);
     *buf_nal_size = encOutputInfo.bitstreamSize;
  // cache invalid to read out by CPU
  //vdi_invalitate_memory(ctx->encOpenParam.coreIdx,
  //              &ctx->bsBuffer[encOutputInfo.encSrcIdx]);
  vdi_read_memory(ctx->encOpenParam.coreIdx,
        encOutputInfo.bitstreamBuffer, buffer, encOutputInfo.bitstreamSize,
        ctx->encOpenParam.streamEndian);

    *Retframe= ctx->FrameIO[idx];
    Retframe -> encoded_frame_type = encOutputInfo.picType;
    } else if( encOutputInfo.encSrcIdx == 0xfffffffe)
        {
        VLOG(INFO, "%s:%d delay frame delay %d \n", __FUNCTION__, __LINE__, ctx->frame_delay);
        ctx->frame_delay ++;
        *buf_nal_size = 0;
        Retframe ->YCbCr[0] = 0;
    } else if(encOutputInfo.encSrcIdx == 0xfffffffb)  {
        VLOG(INFO, "%s:%d d[WAVE] non-reference picture !! \n", __FUNCTION__, __LINE__);
        *buf_nal_size = 0;
        Retframe ->YCbCr[0] = 0;
    }

 VLOG(INFO, "%s:%d d[WAVE] done one picture !! \n", __FUNCTION__, __LINE__);
    return AMVENC_PICTURE_READY;
}

AMVEnc_Status AML_MultiEncRelease(amv_enc_handle_t ctx_handle) {
  int end = 1;
  AMVEnc_Status ret = AMVENC_FAIL;
  RetCode       result;
  EncParam*    encParam;
  AMVMultiCtx * ctx = (AMVMultiCtx* ) ctx_handle;
  ENC_INT_STATUS intStatus;
  Uint32 coreIdx = ctx->encOpenParam.coreIdx;
  int idx;

  if (ctx == NULL) return AMVENC_FAIL;
  if (ctx->magic_num != MULTI_ENC_MAGIC) return AMVENC_FAIL;


flush_retry_point:
        encParam        = &ctx->encParam;
        encParam->srcEndFlag = 1;  // flush out frame
        result = VPU_EncStartOneFrame(ctx->enchandle, encParam);
        if (result == RETCODE_SUCCESS) {
//           Queue_Dequeue(ctx->encOutQ);
            ctx->frameIdx++;
        }
        else if (result == RETCODE_QUEUEING_FAILURE) { // Just retry
            QueueStatusInfo qStatus;
            // Just retry
            VPU_EncGiveCommand(ctx->enchandle, ENC_GET_QUEUE_STATUS, (void*)&qStatus);
            if (qStatus.instanceQueueCount == 0) {
                goto flush_retry_point;//return TRUE;
            }
        }
        else { // Error
            VLOG(ERR, "VPU_EncStartOneFrame failed Error code is 0x%x \n", result);
            return AMVENC_FAIL;
        }

    while (VPU_EncClose(ctx->enchandle) == RETCODE_VPU_STILL_RUNNING) {
        if ((intStatus = HandlingInterruptFlag(ctx)) == ENC_INT_STATUS_TIMEOUT) {
            HandleEncoderError(ctx->enchandle, ctx->frameIdx, NULL);
            VLOG(ERR, "NO RESPONSE FROM VPU_EncClose2()\n");
            ret = AMVENC_TIMEOUT;
            break;
        }
        else if (intStatus == ENC_INT_STATUS_DONE) {
            EncOutputInfo   outputInfo;
            VLOG(INFO, "VPU_EncClose() : CLEAR REMAIN INTERRUPT\n");
            VPU_EncGetOutputInfo(ctx->enchandle, &outputInfo);
            continue;
        }

        osal_msleep(10);
    }

  if (ctx->vbCustomLambda.size)
    vdi_free_dma_memory(coreIdx, &ctx->vbCustomLambda);
  if (ctx->vbScalingList.size)
    vdi_free_dma_memory(coreIdx, &ctx->vbScalingList);

  for (idx = 0; idx < MAX_REG_FRAME; idx++) {
    if (ctx->pFbReconMem[idx].size)
      vdi_free_dma_memory(coreIdx, &ctx->pFbReconMem[idx]);
    if (ctx->vbCustomMap[idx].size)
      vdi_free_dma_memory(coreIdx, &ctx->vbCustomMap[idx]);
  }
  for (idx = 0; idx < ENC_SRC_BUF_NUM; idx++) {
    if (ctx->pFbSrcMem[idx].size)
      vdi_free_dma_memory(coreIdx, &ctx->pFbSrcMem[idx]);
    if (ctx->bsBuffer[idx].size)
      vdi_free_dma_memory(coreIdx, &ctx->bsBuffer[idx]);
  }


#if SUPPORT_SCALE
  if (ctx-> ge2d_initial_done) {
    aml_ge2d_mem_free(&amlge2d);
    aml_ge2d_exit(&amlge2d);
    amlge2d.ge2dinfo.src_info[0].vaddr = NULL;
    VLOG(DEBUG, "ge2d exit!!!\n");
  }
#endif
  VPU_DeInit(ctx->encOpenParam.coreIdx);

  free(ctx);
  VLOG(INFO, "AML_HEVCRelease succeed\n");
  return AMVENC_SUCCESS;
}
