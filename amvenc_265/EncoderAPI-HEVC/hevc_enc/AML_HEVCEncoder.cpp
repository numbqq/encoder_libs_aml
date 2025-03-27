//#define LOG_NDEBUG 0
#define LOG_TAG "AMLVHEVCENC"
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
#include <sys/time.h>
//#include <malloc.h>
//#include <stdlib.h>
//#include <fcntl.h>
#include "./vpuapi/common_regdefine.h"
#include "./vpuapi/wave4_regdefine.h"
#include "./vpuapi/include/vputypes.h"
#include "./vpuapi/common.h"
#include "./include/AML_HEVCEncoder.h"

#define SUPPORT_SCALE 1

#if SUPPORT_SCALE
#include <ge2d_port.h>
#include <aml_ge2d.h>
#endif

#define wave420l_align4(a)      ((((a)+3)>>2)<<2)
#define wave420l_align8(a)      ((((a)+7)>>3)<<3)
#define wave420l_align16(a)     ((((a)+15)>>4)<<4)
#define wave420l_align32(a)     ((((a)+31)>>5)<<5)
#define wave420l_align64(a)     ((((a)+63)>>6)<<6)
#define wave420l_align128(a)    ((((a)+127)>>7)<<7)
#define wave420l_align256(a)    ((((a)+255)>>8)<<8)

#define VUI_HRD_RBSP_BUF_SIZE           0x4000
#define MULTI_ENC_MAGIC ('A' << 24| 'M' <<16 | 'L' << 8 |'G')
/* timeout in ms */
#define VPU_WAIT_TIME_OUT_CQ    100
/* extra source frame buffer required.*/
#define EXTRA_SRC_BUFFER_NUM    0

typedef enum {
    ENC_INT_STATUS_NONE,        // Interrupt not asserted yet
    ENC_INT_STATUS_FULL,        // Need more buffer
    ENC_INT_STATUS_DONE,        // Interrupt asserted
    ENC_INT_STATUS_LOW_LATENCY,
    ENC_INT_STATUS_TIMEOUT,     // Interrupt not asserted during given time.
} ENC_INT_STATUS;

typedef struct AMVHEVCContext_s {
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
  uint32 averageQP;

  uint32 src_num;   /*total src_frame buffer needed */
  uint32 fb_num;    /* total reconstuction  frame buffer encoder needed */

  uint32 enc_counter;
  uint frame_delay;

  AMVEncFrameFmt fmt;

  bool mPrependSPSPPSToIDRFrames;
  uint32 initQP; /* initial QP */
  //AVCNalUnitType nal_unit_type;
  bool rcEnable; /* rate control enable, on: RC on, off: constant QP */
  AMVHEVCEncParams mInitParams; //copy of initial set up from upper layer
/* VPU API interfaces  */
  EncHandle enchandle; /*VPU encoder handler */
  EncOpenParam encOpenParam; /* api open param */
  EncChangeParam changeParam;
  Uint32 param_change_flag;
  Uint32                      frameIdx;
  vpu_buffer_t                vbCustomLambda;
  vpu_buffer_t                vbScalingList;
  //Uint32                      customLambda[NUM_CUSTOM_LAMBDA];
  //UserScalingList             scalingList;
  vpu_buffer_t                vbCustomMap[MAX_REG_FRAME];
  //EncoderState                state;
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
  //AMVMultiEncFrameIO          FrameIO[ENC_SRC_BUF_NUM];



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

  uint8 *CustomRoiMapBuf; // store the latest value
  uint8 *CustomLambdaMapBuf;
  uint8 *CustomModeMapBuf;
  uint32 CustomMapSize; // this is the max value
  uint32 CustomUpdateID; //store the update count
  uint32 CustomMapUpdatedId[MAX_REG_FRAME]; // updated ID cnt
  //WeigtedPredInfo wp_para;

#if SUPPORT_SCALE
  uint32 ge2d_initial_done;
  aml_ge2d_t amlge2d;
  bool INIT_GE2D;
#endif
} AMVHEVCCtx;

#if SUPPORT_SCALE
static int SRC1_PIXFORMAT = PIXEL_FORMAT_YCrCb_420_SP;
static int SRC2_PIXFORMAT = PIXEL_FORMAT_YCrCb_420_SP;
static int DST_PIXFORMAT = PIXEL_FORMAT_YCrCb_420_SP;
static bool INIT_GE2D = false;

static GE2DOP OP = AML_GE2D_STRETCHBLIT;
static int do_strechblit(aml_ge2d_info_t *pge2dinfo, AMVHEVCEncFrameIO *input)
{
    int ret = -1;
    VLOG(DEBUG, "do_strechblit test case:\n");
    pge2dinfo->src_info[0].memtype = GE2D_CANVAS_ALLOC;
    pge2dinfo->dst_info.memtype = GE2D_CANVAS_ALLOC;
    pge2dinfo->src_info[0].canvas_w = input->pitch; //SX_SRC1;
    pge2dinfo->src_info[0].canvas_h = input->height; //SY_SRC1;
    pge2dinfo->src_info[0].format = SRC1_PIXFORMAT;
    if ((input->scale_width != 0) && (input->scale_height !=0)) {
        pge2dinfo->dst_info.canvas_w = input->scale_width; //SX_DST;
        pge2dinfo->dst_info.canvas_h = input->scale_height; //SY_DST;
    } else {
        pge2dinfo->dst_info.canvas_w = input->pitch - input->crop_left - input->crop_right;
        pge2dinfo->dst_info.canvas_h = input->height - input->crop_top - input->crop_bottom;
    }
    pge2dinfo->dst_info.format = DST_PIXFORMAT;

    pge2dinfo->src_info[0].rect.x = input->crop_left;
    pge2dinfo->src_info[0].rect.y = input->crop_top;
    pge2dinfo->src_info[0].rect.w = input->pitch - input->crop_left - input->crop_right;
    pge2dinfo->src_info[0].rect.h = input->height - input->crop_top - input->crop_bottom;
    pge2dinfo->dst_info.rect.x = 0;
    pge2dinfo->dst_info.rect.y = 0;
    if ((input->scale_width != 0) && (input->scale_height !=0)) {
        pge2dinfo->dst_info.rect.w = input->scale_width; //SX_DST;
        pge2dinfo->dst_info.rect.h = input->scale_height; //SY_DST;
    } else {
        pge2dinfo->dst_info.rect.w = input->pitch - input->crop_left - input->crop_right;
        pge2dinfo->dst_info.rect.h = input->height - input->crop_top - input->crop_bottom;
    }
    pge2dinfo->dst_info.rotation = GE2D_ROTATION_0;
    ret = aml_ge2d_process(pge2dinfo);
    return ret;
}

static void set_ge2dinfo(aml_ge2d_info_t *pge2dinfo, AMVHEVCEncParams *encParam)
{
    pge2dinfo->src_info[0].memtype = GE2D_CANVAS_ALLOC;
    pge2dinfo->src_info[0].canvas_w = encParam->src_width; //SX_SRC1;
    pge2dinfo->src_info[0].canvas_h = encParam->src_height; //SY_SRC1;
    pge2dinfo->src_info[0].format = SRC1_PIXFORMAT;
    //pge2dinfo->src_info[0].plane_number = 0;
    pge2dinfo->src_info[1].memtype = GE2D_CANVAS_TYPE_INVALID;
    pge2dinfo->src_info[1].canvas_w = 0;
    pge2dinfo->src_info[1].canvas_h = 0;
    pge2dinfo->src_info[1].format = SRC2_PIXFORMAT;
    //pge2dinfo->src_info[1].plane_number = 0;

    pge2dinfo->dst_info.memtype = GE2D_CANVAS_ALLOC;
    pge2dinfo->dst_info.canvas_w = encParam->width; //SX_DST;
    pge2dinfo->dst_info.canvas_h = encParam->height; //SY_DST;
    pge2dinfo->dst_info.format = DST_PIXFORMAT;
    pge2dinfo->dst_info.rotation = GE2D_ROTATION_0;
    //pge2dinfo->dst_info.plane_number = 0;
    pge2dinfo->offset = 0;
    pge2dinfo->ge2d_op = OP;
    pge2dinfo->blend_mode = BLEND_MODE_PREMULTIPLIED;
}
#endif

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

void static yuv_plane_memcpy(int coreIdx, int dst, char *src, uint32 width,
                                uint32 height, uint32 stride, uint32 src_stride,
                                bool aligned, int endian) {
    unsigned i;
    if (dst == 0 || src == NULL) {
        VLOG(ERR, "yuv_plane_memcpy error ptr\n");
        return;
    }
    if (!aligned) {
        for (i = 0; i < height; i++) {
            vdi_write_memory(coreIdx, dst, (Uint8 *)src, width, endian);
            dst += stride;
            src += src_stride;
        }
    } else {
        vdi_write_memory(coreIdx, dst, (Uint8 *)src, stride * height, endian);
    }
}

static BOOL SetupEncoderOpenParam(EncOpenParam *pEncOP, AMVHEVCEncParams* InitParam)
{
    Int32   i = 0;

    EncHevcParam *param = &pEncOP->EncStdParam.hevcParam;

    pEncOP->bitstreamFormat = STD_HEVC;
    pEncOP->picWidth        = InitParam->width;
    pEncOP->picHeight       = InitParam->height;
    pEncOP->frameRateInfo   = InitParam->frame_rate;
    pEncOP->bitRate         = InitParam->bitrate;
    pEncOP->rcEnable        = InitParam->rate_control; //pCfg->RcEnable;

    param->level            = 0;
    param->tier             = 0;
    pEncOP->srcBitDepth     = 8; //pCfg->SrcBitDepth;
    param->internalBitDepth = 8; //pCfg->hevcCfg.internalBitDepth;

    //if (pCfg->hevcCfg.internalBitDepth == 0)
        //param->internalBitDepth = pCfg->SrcBitDepth;
    //else
        //param->internalBitDepth = pCfg->hevcCfg.internalBitDepth;

    if (param->internalBitDepth > 8)
        param->profile   = HEVC_PROFILE_MAIN10;
    else
        param->profile   = HEVC_PROFILE_MAIN;

    param->chromaFormatIdc  = 0;
    param->losslessEnable   = 0; //pCfg->hevcCfg.losslessEnable;
    param->constIntraPredFlag = 0; //pCfg->hevcCfg.constIntraPredFlag;



    //if (pCfg->hevcCfg.useAsLongtermPeriod > 0 || pCfg->hevcCfg.refLongtermPeriod > 0)
        //param->useLongTerm = 1;
    //else
        param->useLongTerm = 0;

    /* for CMD_ENC_SEQ_GOP_PARAM */
    //param->gopPresetIdx     = PRESET_IDX_IPP; //pCfg->hevcCfg.gopPresetIdx;//
    if (InitParam->idr_period == 1)
        param->gopPresetIdx = PRESET_IDX_ALL_I; //all I frame
    else
        param->gopPresetIdx = PRESET_IDX_IPP     ;//PRESET_IDX_IPP PRESET_IDX_IPP_SINGLE
    /* for CMD_ENC_SEQ_INTRA_PARAM */
    param->decodingRefreshType = 1; //pCfg->hevcCfg.decodingRefreshType;
    param->intraPeriod      = InitParam->idr_period; //pCfg->hevcCfg.intraPeriod;
    param->intraQP          = InitParam->initQP; //pCfg->hevcCfg.intraQP;
    param->forcedIdrHeaderEnable    = 0; //pCfg->hevcCfg.forcedIdrHeaderEnable;

    /* for CMD_ENC_SEQ_CONF_WIN_TOP_BOT/LEFT_RIGHT */
    param->confWinTop    = 0; //pCfg->hevcCfg.confWinTop;
    param->confWinBot    = 0; //pCfg->hevcCfg.confWinBot;
    param->confWinLeft   = 0; //pCfg->hevcCfg.confWinLeft;
    param->confWinRight  = 0; //pCfg->hevcCfg.confWinRight;

    if ((pEncOP->picHeight % 8) && param->confWinBot == 0) {
        param->confWinBot = 8 - (pEncOP->picHeight % 8);
    }

    /* for CMD_ENC_SEQ_INDEPENDENT_SLICE */
    param->independSliceMode     = 0; //pCfg->hevcCfg.independSliceMode;
    param->independSliceModeArg  = 0; //pCfg->hevcCfg.independSliceModeArg;

    /* for CMD_ENC_SEQ_DEPENDENT_SLICE */
    param->dependSliceMode     = 0; //pCfg->hevcCfg.dependSliceMode;
    param->dependSliceModeArg  = 0; //pCfg->hevcCfg.dependSliceModeArg;

    /* for CMD_ENC_SEQ_INTRA_REFRESH_PARAM */
    param->intraRefreshMode     = 0; //pCfg->hevcCfg.intraRefreshMode;
    param->intraRefreshArg      = 0; //pCfg->hevcCfg.intraRefreshArg;
    param->useRecommendEncParam = 1; //pCfg->hevcCfg.useRecommendEncParam;


    /* for CMD_ENC_PARAM */
    //param->scalingListEnable        = pCfg->hevcCfg.scalingListEnable;
    //param->cuSizeMode               = pCfg->hevcCfg.cuSizeMode;
    //param->tmvpEnable               = pCfg->hevcCfg.tmvpEnable;
    //param->wppEnable                = pCfg->hevcCfg.wppenable;
    //param->maxNumMerge              = pCfg->hevcCfg.maxNumMerge;
    //param->dynamicMerge8x8Enable    = pCfg->hevcCfg.dynamicMerge8x8Enable;
    //param->dynamicMerge16x16Enable  = pCfg->hevcCfg.dynamicMerge16x16Enable;
    //param->dynamicMerge32x32Enable  = pCfg->hevcCfg.dynamicMerge32x32Enable;
    //param->disableDeblk             = pCfg->hevcCfg.disableDeblk;
    //param->lfCrossSliceBoundaryEnable   = pCfg->hevcCfg.lfCrossSliceBoundaryEnable;
    //param->betaOffsetDiv2           = pCfg->hevcCfg.betaOffsetDiv2;
    //param->tcOffsetDiv2             = pCfg->hevcCfg.tcOffsetDiv2;
    //param->skipIntraTrans           = pCfg->hevcCfg.skipIntraTrans;
    //param->saoEnable                = pCfg->hevcCfg.saoEnable;
    //param->intraInInterSliceEnable  = pCfg->hevcCfg.intraInInterSliceEnable;
    //param->intraNxNEnable           = pCfg->hevcCfg.intraNxNEnable;*/

    //default
    /* for CMD_ENC_PARAM */
    if (param->useRecommendEncParam != 1) {     // 0 : Custom,  2 : Boost mode (normal encoding speed, normal picture quality),  3 : Fast mode (high encoding speed, low picture quality)
        param->scalingListEnable        = 0;
        param->cuSizeMode               = 0x7;
        param->tmvpEnable               = 1;
        param->wppEnable                = 0;
        param->maxNumMerge              = 2;
        param->dynamicMerge8x8Enable    = 1;
        param->dynamicMerge16x16Enable  = 1;
        param->dynamicMerge32x32Enable  = 1;
        param->disableDeblk             = 0;
        param->lfCrossSliceBoundaryEnable   = 1;
        param->betaOffsetDiv2           = 0;
        param->tcOffsetDiv2             = 0;
        param->skipIntraTrans           = 1;
        param->saoEnable                = 1;
        param->intraInInterSliceEnable  = 1;
        param->intraNxNEnable           = 1;
    }

    /* for CMD_ENC_RC_PARAM */
    pEncOP->initialDelay         = 3000; //pCfg->RcInitDelay;
    param->cuLevelRCEnable       = 0; //pCfg->hevcCfg.cuLevelRCEnable;
    param->hvsQPEnable           = 1; //pCfg->hevcCfg.hvsQPEnable;
    param->hvsQpScale            = 2; //pCfg->hevcCfg.hvsQpScale;
    param->hvsQpScaleEnable      = (param->hvsQpScale > 0) ? 1: 0; //pCfg->hevcCfg.hvsQpScaleEnable;

    param->ctuOptParam.roiDeltaQp= 3; //pCfg->hevcCfg.ctuOptParam.roiDeltaQp;
    param->intraQpOffset         = 0; //pCfg->hevcCfg.intraQpOffset;
    param->initBufLevelx8        = 1; //pCfg->hevcCfg.initBufLevelx8;
    param->bitAllocMode          = 0; //pCfg->hevcCfg.bitAllocMode;
    for (i = 0; i < MAX_GOP_NUM; i++) {
        param->fixedBitRatio[i] = 1; //pCfg->hevcCfg.fixedBitRatio[i];
    }

    /* for CMD_ENC_RC_MIN_MAX_QP */
    param->minQp             = 8; //pCfg->hevcCfg.minQp;
    param->maxQp             = 51; //pCfg->hevcCfg.maxQp;
    param->maxDeltaQp        = 10; //pCfg->hevcCfg.maxDeltaQp;
    param->transRate         = 0; //pCfg->hevcCfg.transRate;

    if (InitParam->qp_mode == 1) {
      param->minQp = InitParam->minQP;
      param->maxQp = InitParam->maxQP;
      param->maxDeltaQp = InitParam->maxDeltaQP;
    }

    /* for CMD_ENC_CUSTOM_GOP_PARAM */
    param->gopParam.customGopSize     = 0; //pCfg->hevcCfg.gopParam.customGopSize;
    param->gopParam.useDeriveLambdaWeight = 0; //pCfg->hevcCfg.gopParam.useDeriveLambdaWeight;

    for (i= 0; i<param->gopParam.customGopSize; i++) {
        param->gopParam.picParam[i].picType      = PIC_TYPE_I; //pCfg->hevcCfg.gopParam.picParam[i].picType;
        param->gopParam.picParam[i].pocOffset    = 1; //pCfg->hevcCfg.gopParam.picParam[i].pocOffset;
        param->gopParam.picParam[i].picQp        = 30; //pCfg->hevcCfg.gopParam.picParam[i].picQp;
        param->gopParam.picParam[i].refPocL0     = 0; //pCfg->hevcCfg.gopParam.picParam[i].refPocL0;
        param->gopParam.picParam[i].refPocL1     = 0; //pCfg->hevcCfg.gopParam.picParam[i].refPocL1;
        param->gopParam.picParam[i].temporalId   = 0; //pCfg->hevcCfg.gopParam.picParam[i].temporalId;
        param->gopParam.gopPicLambda[i]           = 0; //pCfg->hevcCfg.gopParam.gopPicLambda[i];
    }

    param->ctuOptParam.roiEnable = 0; //pCfg->hevcCfg.ctuOptParam.roiEnable;
    param->ctuOptParam.ctuQpEnable = 0; //pCfg->hevcCfg.ctuOptParam.ctuQpEnable;
    // VPS & VUI

    param->numUnitsInTick       = 1000; //pCfg->hevcCfg.numUnitsInTick;
    param->timeScale            = pEncOP->frameRateInfo * 1000; //pCfg->hevcCfg.timeScale;
    param->numTicksPocDiffOne   = 0; //pCfg->hevcCfg.numTicksPocDiffOne;

	if (InitParam->vui_info_present) {
        param->vuiParam.vuiParamFlags       = 0xFFFFFFFF & ((InitParam->video_signal_type<<5) | (InitParam->color_description<<6)); // when vuiParamFlags == 0, VPU doesn't encode VUI//pCfg->hevcCfg.vuiParam.vuiParamFlags;
        //param->vuiParam.vuiAspectRatioIdc   = pCfg->hevcCfg.vuiParam.vuiAspectRatioIdc;
        //param->vuiParam.vuiSarSize          = pCfg->hevcCfg.vuiParam.vuiSarSize;
        //param->vuiParam.vuiOverScanAppropriate  = pCfg->hevcCfg.vuiParam.vuiOverScanAppropriate;
        param->vuiParam.videoSignal         = (
                                                    (0)|  // VIDEO_FORMAT
                                                    ((InitParam->range)<<3)|  // VIDEO_FULL_RANGE_FLAG
                                                    ((InitParam->primaries)<<4)|  // COLOUR_PRIMARIES
                                                    ((InitParam->transfer)<<12)|  // TRANSFER_CHARACTERISTICS
                                                    ((InitParam->matrix)<<20)  // MATRIX_COEFFS
                                                );
        //param->vuiParam.vuiChromaSampleLoc  = pCfg->hevcCfg.vuiParam.vuiChromaSampleLoc;
        //param->vuiParam.vuiDispWinLeftRight = pCfg->hevcCfg.vuiParam.vuiDispWinLeftRight;
        //param->vuiParam.vuiDispWinTopBottom = pCfg->hevcCfg.vuiParam.vuiDispWinTopBottom;
   }
    pEncOP->encodeVuiRbsp        = 0; //pCfg->hevcCfg.vuiDataEnable;
    pEncOP->vuiRbspDataSize      = VUI_HRD_RBSP_BUF_SIZE; //pCfg->hevcCfg.vuiDataSize;
    pEncOP->encodeHrdRbspInVPS   = 0; //pCfg->hevcCfg.hrdInVPS;
    pEncOP->hrdRbspDataSize      = VUI_HRD_RBSP_BUF_SIZE; //pCfg->hevcCfg.hrdDataSize;
    pEncOP->encodeHrdRbspInVUI   = 0; //pCfg->hevcCfg.hrdInVUI;

    param->chromaCbQpOffset = 0; //pCfg->hevcCfg.chromaCbQpOffset;
    param->chromaCrQpOffset = 0; //pCfg->hevcCfg.chromaCrQpOffset;
    param->initialRcQp      = 63; //pCfg->hevcCfg.initialRcQp;

    param->nrYEnable        = 0; //pCfg->hevcCfg.nrYEnable;
    param->nrCbEnable       = 0; //pCfg->hevcCfg.nrCbEnable;
    param->nrCrEnable       = 0; //pCfg->hevcCfg.nrCrEnable;
    param->nrNoiseEstEnable = 0; //pCfg->hevcCfg.nrNoiseEstEnable;
    //param->nrNoiseSigmaY    = pCfg->hevcCfg.nrNoiseSigmaY;
    //param->nrNoiseSigmaCb   = pCfg->hevcCfg.nrNoiseSigmaCb;
    //param->nrNoiseSigmaCr   = pCfg->hevcCfg.nrNoiseSigmaCr;
    //param->nrIntraWeightY   = pCfg->hevcCfg.nrIntraWeightY;
    //param->nrIntraWeightCb  = pCfg->hevcCfg.nrIntraWeightCb;
    //param->nrIntraWeightCr  = pCfg->hevcCfg.nrIntraWeightCr;
    //param->nrInterWeightY   = pCfg->hevcCfg.nrInterWeightY;
    //param->nrInterWeightCb  = pCfg->hevcCfg.nrInterWeightCb;
    //param->nrInterWeightCr  = pCfg->hevcCfg.nrInterWeightCr;

    param->intraMinQp       = 8; //pCfg->hevcCfg.intraMinQp;
    param->intraMaxQp       = 51; //pCfg->hevcCfg.intraMaxQp;

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
    pEncOP->lineBufIntEn   = 0;
    pEncOP->coreIdx        = 0;
    pEncOP->cbcrOrder      = CBCR_ORDER_NORMAL;

    return 1;
}

static ENC_INT_STATUS HandlingInterruptFlag(AMVHEVCCtx *ctx,bool useLock = false)
{
    EncHandle handle = ctx->enchandle;
    Int32 interruptFlag = 0;
    Uint32 interruptWaitTime = VPU_WAIT_TIME_OUT_CQ;
    Uint32 interruptTimeout = VPU_ENC_TIMEOUT;
    ENC_INT_STATUS status = ENC_INT_STATUS_NONE;
    Uint32 coreIdx = ctx->encOpenParam.coreIdx;

    if (ctx->startTimeout == 0ULL) {
        ctx->startTimeout = osal_gettime();
    }
    if (useLock)
        EnterLock(coreIdx);

    do {
        interruptFlag = VPU_WaitInterruptEx(handle, interruptWaitTime);
        if (INTERRUPT_TIMEOUT_VALUE == interruptFlag) {
            Uint64 currentTimeout = osal_gettime();
            if ((currentTimeout - ctx->startTimeout) > interruptTimeout) {
                VLOG(ERR, "startTimeout(%ld) currentTime(%ld) diff(%d)\n",
                ctx->startTimeout, currentTimeout, (Uint32)(currentTimeout - ctx->startTimeout));
                status = ENC_INT_STATUS_TIMEOUT;
                break;
            }
            interruptFlag = 0;
        }
        if (interruptFlag < 0)
            VLOG(ERR, "interruptFlag is negative value! %08x\n", interruptFlag);

        if (interruptFlag & (1<<INT_BIT_BIT_BUF_FULL)) {
            VLOG(WARN,"INT_BIT_BIT_BUF_FULL \n");
            status = ENC_INT_STATUS_FULL;
        }

        if (interruptFlag > 0) {
            VPU_ClearInterruptEx(handle, interruptFlag);
            ctx->startTimeout = 0ULL;

            if (interruptFlag & (1<<INT_WAVE_ENC_PIC)) {
                status = ENC_INT_STATUS_DONE;
                break;
            }
        }
    } while (FALSE);

    if (useLock)
        LeaveLock(coreIdx);
    return status;
}

static void    DisplayEncodedInformation(
    EncHandle      handle,
    CodStd         codec,
    Uint32         frameNo,
    EncOutputInfo* encodedInfo,
    Int32           srcEndFlag,
    Int32           srcFrameIdx,
    Int32           performance
    )
{
        if (encodedInfo == NULL) {
            VLOG(INFO, "------------------------------------------------------------------------------\n");
            VLOG(INFO, "I     NO     T   RECON   RD_PTR    WR_PTR     BYTES  SRCIDX  USEDSRCIDX Cycle \n");
            VLOG(INFO, "------------------------------------------------------------------------------\n");
        } else {
            VLOG(INFO, "%02d %5d %5d %5d    %08x  %08x %8x     %2d        %2d    %8d\n",
                handle->instIndex, encodedInfo->encPicCnt, encodedInfo->picType, encodedInfo->reconFrameIndex, encodedInfo->rdPtr, encodedInfo->wrPtr,
                encodedInfo->bitstreamSize, (srcEndFlag == 1 ? -1 : srcFrameIdx), encodedInfo->encSrcIdx, encodedInfo->frameCycle);
        }
}

static BOOL AllocateReconFrameBuffer(AMVHEVCCtx* ctx)
{
    Uint32 i;
    Uint32 reconFbSize;
    Uint32 reconFbStride;
    Uint32 reconFbWidth;
    Uint32 reconFbHeight;
    EncOpenParam encOpenParam = ctx->encOpenParam;


    reconFbWidth  = VPU_ALIGN32(encOpenParam.picWidth);
    reconFbHeight = VPU_ALIGN8(encOpenParam.picHeight);

    /* Allocate framebuffers for recon. */
    reconFbStride = CalcStride(reconFbWidth, reconFbHeight,
                    (FrameBufferFormat)encOpenParam.srcFormat,
                    encOpenParam.cbcrInterleave,
                    COMPRESSED_FRAME_MAP, FALSE);
    reconFbSize = VPU_GetFrameBufSize(encOpenParam.coreIdx,
                    reconFbStride, reconFbHeight, COMPRESSED_FRAME_MAP,
                    (FrameBufferFormat)encOpenParam.srcFormat,
                    encOpenParam.cbcrInterleave, NULL);

    for (i = 0; i < ctx->fb_num; i++) {
        ctx->pFbReconMem[i].size = ((reconFbSize + 4095) & ~4095) + 4096;
        if (vdi_allocate_dma_memory(encOpenParam.coreIdx, &ctx->pFbReconMem[i]) < 0) {
            ctx->pFbReconMem[i].size = 0;
            VLOG(ERR, "fail to allocate recon buffer\n");
            return FALSE;
        }
        ctx->pFbRecon[i].bufY  = ctx->pFbReconMem[i].phys_addr;
        ctx->pFbRecon[i].bufCb = (PhysicalAddress) - 1;
        ctx->pFbRecon[i].bufCr = (PhysicalAddress) - 1;
        ctx->pFbRecon[i].size  = reconFbSize;
        ctx->pFbRecon[i].updateFbInfo = TRUE;
    }

    ctx->reconFbStride = reconFbStride;
    ctx->reconFbHeight = reconFbHeight;
    return TRUE;
}

static BOOL AllocateYuvBufferHeader(AMVHEVCCtx* ctx)
{
    Uint32 i;
    FrameBufferAllocInfo srcFbAllocInfo;
    Uint32 srcFbSize;
    Uint32 srcFbStride;
    EncOpenParam encOpenParam = ctx->encOpenParam;

    Uint32 srcFbWidth = VPU_ALIGN32(encOpenParam.picWidth);
    Uint32 srcFbHeight  = VPU_ALIGN16(encOpenParam.picHeight);

    memset(&srcFbAllocInfo, 0x00, sizeof(FrameBufferAllocInfo));

    /* calculate sourec buffer stride and size */
    srcFbStride = CalcStride(srcFbWidth, srcFbHeight,
        (FrameBufferFormat)encOpenParam.srcFormat,
        encOpenParam.cbcrInterleave, LINEAR_FRAME_MAP, FALSE);
    srcFbSize = VPU_GetFrameBufSize(encOpenParam.coreIdx, srcFbStride,
        srcFbHeight, LINEAR_FRAME_MAP,
        (FrameBufferFormat)encOpenParam.srcFormat,
        encOpenParam.cbcrInterleave, NULL);

    srcFbAllocInfo.format  = (FrameBufferFormat)encOpenParam.srcFormat;
    srcFbAllocInfo.cbcrInterleave = encOpenParam.cbcrInterleave;
    srcFbAllocInfo.stride  = srcFbStride;
    srcFbAllocInfo.height  = srcFbHeight;
    srcFbAllocInfo.endian  = encOpenParam.sourceEndian;
    srcFbAllocInfo.type    = FB_TYPE_PPU;
    srcFbAllocInfo.num     = ctx->src_num;
    srcFbAllocInfo.nv21    = encOpenParam.nv21;

#if 0  //allocat later when use input buffer, in case DMA buffer no need copy.
    for (i = 0; i < ctx->src_num; i++) {
        ctx->pFbSrcMem[i].size = ((srcFbSize + 4095) & ~4095) + 4096;
        if (vdi_allocate_dma_memory(encOpenParam.coreIdx, &ctx->pFbSrcMem[i]) < 0) {
            VLOG(ERR, "fail to allocate src buffer\n");
            ctx->pFbSrcMem[i].size = 0;
            return FALSE;
        }
        ctx->pFbSrc[i].bufY  = ctx->pFbSrcMem[i].phys_addr;
        ctx->pFbSrc[i].bufCb = (PhysicalAddress) - 1;
        ctx->pFbSrc[i].bufCr = (PhysicalAddress) - 1;
        ctx->pFbSrc[i].updateFbInfo = TRUE;
    }
#else
    for (i = 0; i < ctx->src_num; i++) {
        ctx->pFbSrc[i].bufY  = (PhysicalAddress) 0;
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

static BOOL RegisterFrameBuffers(AMVHEVCCtx *ctx)
{
    FrameBuffer *pReconFb;
    FrameBuffer *pSrcFb;
    FrameBufferAllocInfo srcFbAllocInfo;
    Uint32 reconFbStride;
    Uint32 reconFbHeight;
    RetCode result;
    Uint32 idx;

    pReconFb = ctx->pFbRecon;
    reconFbStride = ctx->reconFbStride;
    reconFbHeight = ctx->reconFbHeight;
    result = VPU_EncRegisterFrameBuffer(ctx->enchandle, pReconFb,
        ctx->fb_num, reconFbStride, reconFbHeight, COMPRESSED_FRAME_MAP);
    if (result != RETCODE_SUCCESS) {
    VLOG(ERR, "Failed to VPU_EncRegisterFrameBuffer(%d)\n", result);
    return FALSE;
    }
    VLOG(INFO, " finish register frame buffer \n");

    pSrcFb         = ctx->pFbSrc;
    srcFbAllocInfo = ctx->srcFbAllocInfo;
    if (VPU_EncAllocateFrameBuffer(ctx->enchandle, srcFbAllocInfo, pSrcFb) != RETCODE_SUCCESS) {
        VLOG(ERR, "VPU_EncAllocateFrameBuffer fail to allocate source frame buffer\n");
        return FALSE;
    }
    #if 0  //ctu_qp_enable=0
    for (idx = 0; idx < ctx->src_num; idx++) {
      ctx->vbCustomMap[idx].size = MAX_CTU_NUM;
      if (vdi_allocate_dma_memory(ctx->encOpenParam.coreIdx, &ctx->vbCustomMap[idx]) < 0) {
        ctx->vbCustomMap[idx].size = 0;
        VLOG(ERR, "fail to allocate ROI buffer\n");
        return FALSE;
      }
    }
    #endif
    return TRUE;
}

amv_enc_handle_hevc_t AML_HEVCInitialize(AMVHEVCEncParams *encParam)
{
    Uint32 coreIdx, idx;
    RetCode retCode;
    AMVHEVCCtx * ctx = (AMVHEVCCtx* )malloc(sizeof(AMVHEVCCtx));

    if (ctx == NULL) {
      VLOG(ERR, "ctx == NULL");
      return (amv_enc_handle_hevc_t) ctx;
    }

    memset(ctx, 0, sizeof(AMVHEVCCtx));
    ctx->magic_num = MULTI_ENC_MAGIC;

    ctx->instance_id = 0;
    ctx->src_idx = 0;
    ctx->enc_counter = 0;
    ctx->enc_width = encParam->width;
    ctx->enc_height = encParam->height;

    memcpy((char *)&ctx->mInitParams, encParam, sizeof(AMVHEVCEncParams));
    if (encParam->idr_period == -1) {
      ctx->idrPeriod = 0;
    } else {
      ctx->idrPeriod = encParam->idr_period;
    }
    VLOG(INFO, "ctx->idrPeriod: %d\n", ctx->idrPeriod);

    ctx->bitrate = encParam->bitrate;
    ctx->frame_rate = encParam->frame_rate;
    ctx->mPrependSPSPPSToIDRFrames = encParam->prepend_spspps_to_idr_frames;;
    ctx->mNumPlanes = 0;

    if (SetupEncoderOpenParam(&(ctx->encOpenParam), encParam) == FALSE)
          goto fail_exit;


    coreIdx = ctx->encOpenParam.coreIdx;
    retCode = VPU_Init(coreIdx);
    if (retCode != RETCODE_SUCCESS && retCode != RETCODE_CALLED_BEFORE) {
          VLOG(INFO, "Failed to VPU_Init, ret(%08x)\n", retCode);
          goto fail_exit;
    }

    /* prepare stream buffers */
    if (encParam->es_buf_sz)
         ctx->bsBuffer[0].size = encParam->es_buf_sz; //ENC_STREAM_BUF_SIZE;
    else
         ctx->bsBuffer[0].size = ENC_STREAM_BUF_SIZE; // default value

    if (vdi_allocate_dma_memory(coreIdx, &ctx->bsBuffer[0]) < 0) {
        VLOG(ERR, "fail to allocate bitstream buffer\n" );
        ctx->bsBuffer[0].size = 0;
        goto fail_exit;
    }

    ctx->encOpenParam.bitstreamBuffer = ctx->bsBuffer[0].phys_addr;
    ctx->encOpenParam.bitstreamBufferSize = ctx->bsBuffer[0].size;

    VLOG(DEBUG, "ctx->encOpenParam.bitstreamBuffer %0x, ctx->encOpenParam.bitstreamBufferSiz %0x",
        ctx->encOpenParam.bitstreamBuffer, ctx->encOpenParam.bitstreamBufferSize);
    #if 0
    if (ctx->encOpenParam.encodeVuiRbsp == TRUE) {
        ctx->vbVuiRbsp.size = VUI_HRD_RBSP_BUF_SIZE;

        if (vdi_allocate_dma_memory(coreIdx, &ctx->vbVuiRbsp) < 0) {
            VLOG(ERR, "fail to allocate VUI rbsp buffer\n" );
            Handle->vbVuiRbsp.size = 0;
            goto fail_exit;
        }
        ctx->encOpenParam.vuiRbspDataAddr = ctx->vbVuiRbsp.phys_addr;
    }

    if (ctx->encOpenParam.encodeHrdRbspInVPS || ctx->encOpenParam.encodeHrdRbspInVUI)
    {
        ctx->vbHrdRbsp.size = VUI_HRD_RBSP_BUF_SIZE;
        if (vdi_allocate_dma_memory(coreIdx, &ctx->vbHrdRbsp) < 0) {
            VLOG(ERR, "fail to allocate HRD rbsp buffer\n" );
            ctx->vbHrdRbsp.size = 0;
            goto fail_exit;
        }
        ctx->encOpenParam.hrdRbspDataAddr = ctx->vbHrdRbsp.phys_addr;
    }
    #endif
    retCode = VPU_EncOpen(&ctx->enchandle, &ctx->encOpenParam);
    if (retCode != RETCODE_SUCCESS) {
        VLOG(ERR, "VPU_EncOpen failed Error code is 0x%x \n", retCode);
        ctx->enchandle = NULL;
        goto fail_exit;
    }
    retCode = VPU_EncGetInitialInfo(ctx->enchandle, &ctx->initialInfo);

    if (retCode != RETCODE_SUCCESS) {
        VLOG(ERR, "VPU_EncGetInitialInfo failed Error code is 0x%x \n", retCode);
        goto fail_exit;
    }
    VLOG(INFO, "* Enc InitialInfo =>\n instance #%d, \n minframeBuffercount: %u\n minSrcBufferCount: %d\n", 1, ctx->initialInfo.minFrameBufferCount, ctx->initialInfo.minSrcFrameCount);
    VLOG(INFO, " picWidth: %u\n picHeight: %u\n ",ctx->encOpenParam.picWidth, ctx->encOpenParam.picHeight);

    ctx->fb_num = ctx->initialInfo.minFrameBufferCount;
    ctx->src_num = ctx->initialInfo.minSrcFrameCount + EXTRA_SRC_BUFFER_NUM;

    if (AllocateReconFrameBuffer(ctx) == FALSE)
      goto fail_exit;
    if (AllocateYuvBufferHeader(ctx) == FALSE)
      goto fail_exit;
    if (RegisterFrameBuffers(ctx) == FALSE)
      goto fail_exit;

#if SUPPORT_SCALE
    if ((encParam->width != encParam->src_width) || (encParam->height != encParam->src_height) || true) {
        memset(&ctx->amlge2d,0x0,sizeof(aml_ge2d_t));
        aml_ge2d_info_t *pge2dinfo = &ctx->amlge2d.ge2dinfo;
        memset(pge2dinfo, 0, sizeof(aml_ge2d_info_t));
        memset(&(pge2dinfo->src_info[0]), 0, sizeof(buffer_info_t));
        memset(&(pge2dinfo->src_info[1]), 0, sizeof(buffer_info_t));
        memset(&(pge2dinfo->dst_info), 0, sizeof(buffer_info_t));

        set_ge2dinfo(pge2dinfo, encParam);

        int ret = aml_ge2d_init(&ctx->amlge2d);
        if (ret < 0) {
            VLOG(ERR, "encode open ge2d failed, ret=0x%x", ret);
            return AMVENC_FAIL;
        }
        ctx->ge2d_initial_done = 1;
        ctx->INIT_GE2D = true;
    }
#endif

    VLOG(INFO, "AML_HEVCInitialize succeed\n");
    return (amv_enc_handle_hevc_t) ctx;
fail_exit:
    if (ctx->enchandle)
    VPU_EncClose(ctx->enchandle);
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
    #if 0
    if (ctx->vbHrdRbsp.size)
        vdi_free_dma_memory(coreIdx, &ctx->vbHrdRbsp);

    if (ctx->vbVuiRbsp.size)
        vdi_free_dma_memory(coreIdx, &ctx->vbVuiRbsp);
    #endif
#if SUPPORT_SCALE
    if (ctx->ge2d_initial_done) {
        aml_ge2d_exit(&ctx->amlge2d);
    }
#endif

    free(ctx);
    return (amv_enc_handle_hevc_t) NULL;
}

AMVEnc_Status AML_HEVCSetInput(amv_enc_handle_hevc_t ctx_handle, AMVHEVCEncFrameIO *input)
{
    Uint32 src_stride;
    Uint32 luma_stride, chroma_stride;
    Uint32 size_src_luma, size_src_chroma;
    char *y_dst = NULL;
    char *src = NULL;
    int idx, idx_1, endian;
    bool width32alinged = true; //width is multiple of 32 or not
    Uint32 is_DMA_buffer = 0;
    EncParam *param;

    AMVHEVCCtx *ctx = (AMVHEVCCtx* )ctx_handle;

    VLOG(NONE, "AML_MultiEncSetInput, canvas=%u, input fmt=%d, num_planes=%d, type=%d",
            input->canvas, input->fmt, input->num_planes, input->type);

    if (ctx == NULL) return AMVENC_FAIL;
    if (ctx->magic_num != MULTI_ENC_MAGIC) return AMVENC_FAIL;

    if (ctx->enc_width % 32) {
      width32alinged = false;
    }
    if ((input->num_planes > 0) && (input->type == DMA_BUFF)) { /* DMA buffer, no need allocate */
      is_DMA_buffer = 1;
    }
    param = &(ctx->encParam);

    ctx->op_flag = input->op_flag;
    ctx->fmt = input->fmt;
    if (ctx->fmt != AMVENC_NV12 && ctx->fmt != AMVENC_NV21 && ctx->fmt != AMVENC_YUV420) {
    #if SUPPORT_SCALE
        if (ctx->INIT_GE2D) {
            if (ge2d_colorFormat(ctx->fmt) == AMVENC_SUCCESS) {
                VLOG(DEBUG, "The %d of color format that HEVC need ge2d to change!", ctx->fmt);
            } else {
                VLOG(INFO, "Encoder only support NV12/NV21, not support %d", ctx->fmt);
                return AMVENC_NOT_SUPPORTED;
            }
        }
#else
        VLOG(INFO, "Encoder only support NV12/NV21, not support %d", ctx->fmt);
        return AMVENC_NOT_SUPPORTED;
#endif
    }

    VLOG(INFO, "fmt %d , is dma %d \n", ctx->fmt, is_DMA_buffer);

    if (is_DMA_buffer) {
        if (!input->pitch) {
            src_stride = wave420l_align32(ctx->enc_width);
        } else {
            src_stride = input->pitch;
            if (ctx->fmt == AMVENC_NV12 || ctx->fmt == AMVENC_NV21 || ctx->fmt == AMVENC_YUV420) {
                if (src_stride % 16) {
                    VLOG(ERR, "DMA buffer stride %d  to 16 byte align\n", src_stride);
                    return AMVENC_FAIL;
                }
            } else {
                src_stride = wave420l_align32(input->pitch);
            }
        }
    } else {
        src_stride = wave420l_align32(ctx->enc_width);
    }
    if (src_stride != input->pitch) {
        width32alinged = false;
    }
    luma_stride = src_stride;
    chroma_stride = src_stride;
     //  Pick a no used src frame buffer descriptor
    idx_1 = -1;
    for (idx = 0; idx < ENC_SRC_BUF_NUM; idx++) {
        if (ctx->encodedSrcFrmIdxArr[idx] == 0
            && ctx->pFbSrcMem[idx].size) break; // free and located mem
        if (ctx->encodedSrcFrmIdxArr[idx] == 0
            && idx_1 == -1) idx_1 = idx;
    }
    if (idx == ENC_SRC_BUF_NUM) { //no found, just get a free header
        if (idx_1 >= 0 ) idx = idx_1;
        else {
            VLOG(ERR, "can not find a free src buffer\n");
            return AMVENC_FAIL;
        }
    }
    if (idx >= ctx->src_num) {
        VLOG(ERR, "Too many src buffer request\n");
        return AMVENC_FAIL;
    }
    if (is_DMA_buffer == 0 && input->type != CANVAS_BUFFER) {
        // need allocate buffer and copy
        ctx->pFbSrc[idx].dma_buf_planes = 0; //clean the DMA buffer
        if (ctx->pFbSrcMem[idx].size == 0) { // allocate buffer
            ctx->pFbSrcMem[idx].size = ctx->srcFbSize;
            if (vdi_allocate_dma_memory(ctx->encOpenParam.coreIdx, &ctx->pFbSrcMem[idx]) < 0) {
                VLOG(ERR, "fail to allocate src buffer, short of memory\n");
                ctx->pFbSrcMem[idx].size = 0;
                return AMVENC_FAIL;
            }
            ctx->pFbSrc[idx].bufY = ctx->pFbSrcMem[idx].phys_addr;
            if (ctx->pFbSrc[idx].bufCb != -1)
                ctx->pFbSrc[idx].bufCb += ctx->pFbSrc[idx].bufY;
            if (ctx->pFbSrc[idx].bufCr != -1)
                ctx->pFbSrc[idx].bufCr += ctx->pFbSrc[idx].bufY;
            VLOG(INFO, "New allocate frame idx %d address y:0x%x cb:0x%x cr:0x%x\n",
                        idx, ctx->pFbSrc[idx].bufY, ctx->pFbSrc[idx].bufCb,ctx->pFbSrc[idx].bufCr);
        }
#if SUPPORT_SCALE
        if ((input->scale_width !=0 && input->scale_height !=0) || input->crop_left != 0 ||
            input->crop_right != 0 || input->crop_top != 0 || input->crop_bottom != 0 ||
            (ctx->fmt != AMVENC_NV12 && ctx->fmt != AMVENC_NV21 && ctx->fmt != AMVENC_YUV420)) {
            if (ctx->INIT_GE2D) {
                ctx->INIT_GE2D = false;

                ctx->amlge2d.ge2dinfo.src_info[0].format = SRC1_PIXFORMAT;
                ctx->amlge2d.ge2dinfo.src_info[1].format = SRC2_PIXFORMAT;
                ctx->amlge2d.ge2dinfo.src_info[0].plane_number = 1;
                ctx->amlge2d.ge2dinfo.dst_info.plane_number = 1;
                int ret = aml_ge2d_mem_alloc(&ctx->amlge2d);
                if (ret < 0) {
                    VLOG(ERR, "encode ge2di mem alloc failed, ret=0x%x", ret);
                    return AMVENC_FAIL;
                }
                VLOG(DEBUG, "ge2d init successful!");
            }
            VLOG(DEBUG, "HEVC TEST sclale, enc_width:%d enc_height:%d  pitch:%d height:%d",
                        ctx->enc_width, ctx->enc_height, input->pitch, input->height);
            if (ctx->fmt == AMVENC_NV12 || ctx->fmt == AMVENC_NV21 || ctx->fmt == AMVENC_YUV420) {
                if (input->pitch % 32) {
                    VLOG(ERR, "HEVC crop and scale must be 32bit aligned");
                    return AMVENC_FAIL;
                }
            }
            else {
                width32alinged = true;
            }

            if (ctx->fmt == AMVENC_RGBA8888) {
                memcpy((void *)ctx->amlge2d.ge2dinfo.src_info[0].vaddr[0], (void *)input->YCbCr[0], 4 * input->pitch * input->height);
            } else if (ctx->fmt == AMVENC_NV12 || ctx->fmt == AMVENC_NV21) {
                //memcpy((void *)ctx->amlge2d.ge2dinfo.src_info[0].vaddr[0], (void *)input->YCbCr[0], input->pitch * input->height);
                //memcpy((void *) ((char *)ctx->amlge2d.ge2dinfo.src_info[0].vaddr[0] + input->pitch * input->height), (void *)input->YCbCr[1], input->pitch * input->height / 2);
            } else if (ctx->fmt == AMVENC_YUV420) {
                //memcpy((void *)ctx->amlge2d.ge2dinfo.src_info[0].vaddr[0], (void *)input->YCbCr[0], input->pitch * input->height);
                //memcpy((void *) ((char *)ctx->amlge2d.ge2dinfo.src_info[0].vaddr[0] + input->pitch * input->height), (void *)input->YCbCr[1], input->pitch * input->height / 4);
                //memcpy((void *) ((char *)ctx->amlge2d.ge2dinfo.src_info[0].vaddr[0] + (input->pitch * input->height * 5) /4), (void *)input->YCbCr[2], input->pitch * input->height / 4);
            } else if (ctx->fmt == AMVENC_RGB888) {
                memcpy((void *)ctx->amlge2d.ge2dinfo.src_info[0].vaddr[0], (void *)input->YCbCr[0], input->pitch * input->height * 3);
            }

            do_strechblit(&ctx->amlge2d.ge2dinfo, input);
            aml_ge2d_invalid_cache(&ctx->amlge2d.ge2dinfo);
            size_src_luma = luma_stride * wave420l_align32(ctx->enc_height);
            //size_src_chroma = luma_stride * (wave420l_align16(ctx->enc_height) / 2);
            endian = ctx->pFbSrc[idx].endian;
            y_dst = (char *) ctx->pFbSrcMem[idx].virt_addr;
            src = (char *) ctx->amlge2d.ge2dinfo.dst_info.vaddr[0];
            yuv_plane_memcpy(ctx->encOpenParam.coreIdx, ctx->pFbSrc[idx].bufY, src,
                            ctx->enc_width, ctx->enc_height, luma_stride,
                            input->pitch, width32alinged, endian);
            src = (char *) ctx->amlge2d.ge2dinfo.dst_info.vaddr[0] + luma_stride * ctx->enc_height;
            yuv_plane_memcpy(ctx->encOpenParam.coreIdx, ctx->pFbSrc[idx].bufCb,
                            src, ctx->enc_width, ctx->enc_height / 2,
                            chroma_stride, input->pitch, width32alinged,endian);
        } else
#endif
        {
            // set frame buffers
            // calculate required buffer size
            size_src_luma = luma_stride * wave420l_align32(ctx->enc_height);
            //      size_src_chroma = luma_stride * (vp_align16(ctx->enc_height) / 2);
            endian = ctx->pFbSrc[idx].endian;

            y_dst = (char *) ctx->pFbSrcMem[idx].virt_addr;

            //for (idx_1 = 0; idx_1 < size_src_luma; idx_1++)  *(y_dst + idx_1) = 0;
            //for (idx_1 = 0; idx_1 < size_src_chroma; idx_1++)  *(y_dst + size_src_luma + idx_1) = 0x80;
            //memset(y_dst, 0x0, size_src_luma);
            //memset(y_dst + size_src_luma, 0x80, size_src_chroma);

            src = (char *) input->YCbCr[0];
            VLOG(INFO, "idx %d luma %d src %p dst %p  width %d, height %d \n",
                idx, size_src_luma, src, y_dst, ctx->enc_width, ctx->enc_height);

            yuv_plane_memcpy(ctx->encOpenParam.coreIdx, ctx->pFbSrc[idx].bufY, src,
                            ctx->enc_width, ctx->enc_height, luma_stride,
                            input->pitch, width32alinged, endian);

            if (ctx->fmt == AMVENC_NV12 || ctx->fmt == AMVENC_NV21) {
                src = (char *)input->YCbCr[1];
                yuv_plane_memcpy(ctx->encOpenParam.coreIdx, ctx->pFbSrc[idx].bufCb,
                                src, ctx->enc_width, ctx->enc_height / 2,
                                chroma_stride, input->pitch, width32alinged,endian);
            } else if (ctx->fmt == AMVENC_YUV420) {
                src = (char *)input->YCbCr[1];
                yuv_plane_memcpy(ctx->encOpenParam.coreIdx, ctx->pFbSrc[idx].bufCb,
                                src, ctx->enc_width/2, ctx->enc_height/ 2,
                                chroma_stride / 2, input->pitch / 2, width32alinged,
                                endian);
                //        v_dst = (char *)(ctx->pFbSrcMem[idx].virt_addr + size_src_luma + size_src_luma / 4);
                src = (char *)input->YCbCr[2];
                yuv_plane_memcpy(ctx->encOpenParam.coreIdx, ctx->pFbSrc[idx].bufCr,
                                src, ctx->enc_width/2, ctx->enc_height/ 2,
                                chroma_stride / 2, input->pitch / 2, width32alinged,
                                endian);
            }
        }
        vdi_flush_memory(ctx->instance_id, &ctx->pFbSrcMem[idx]);
        VLOG(INFO,"frame %d stride %d address Y 0x%x cb 0x%x cr 0x%x \n",
                    idx, size_src_luma, ctx->pFbSrc[idx].bufY,
                    ctx->pFbSrc[idx].bufCb,ctx->pFbSrc[idx].bufCr);
    } else { //DMA buffer
#if SUPPORT_SCALE
        if (ctx->fmt != AMVENC_NV12 && ctx->fmt != AMVENC_NV21 && ctx->fmt != AMVENC_YUV420) {
            if (ctx->INIT_GE2D) {
                ctx->INIT_GE2D = false;
                ctx->amlge2d.ge2dinfo.src_info[0].format = SRC1_PIXFORMAT;
                ctx->amlge2d.ge2dinfo.src_info[1].format = SRC2_PIXFORMAT;
                ctx->amlge2d.ge2dinfo.dst_info.plane_number = 1;
                int ret = aml_ge2d_mem_alloc(&ctx->amlge2d);
                if (ret < 0) {
                    VLOG(ERR, "encode ge2di mem alloc failed, ret=0x%x", ret);
                    return AMVENC_FAIL;
                }
                VLOG(DEBUG, "ge2d init successful!");
            }
            ctx->amlge2d.ge2dinfo.src_info[0].shared_fd[0] = input->shared_fd[0];

            do_strechblit(&ctx->amlge2d.ge2dinfo, input);
            aml_ge2d_invalid_cache(&ctx->amlge2d.ge2dinfo);

            ctx->pFbSrc[idx].dma_shared_fd[0] = ctx->amlge2d.ge2dinfo.dst_info.shared_fd[0];
            ctx->pFbSrc[idx].dma_buf_planes = 1;
            ctx->amlge2d.ge2dinfo.src_info[0].shared_fd[0] = -1;
            VLOG(INFO,"Set DMA buffer index %d planes %d fd[%d]\n", idx, ctx->pFbSrc[idx].dma_buf_planes,
                ctx->pFbSrc[idx].dma_shared_fd[0]);
           } else
#endif
        {
            ctx->pFbSrc[idx].dma_buf_planes = input->num_planes;
            ctx->pFbSrc[idx].dma_shared_fd[0] = input->shared_fd[0];
            ctx->pFbSrc[idx].dma_shared_fd[1] = input->shared_fd[1];
            ctx->pFbSrc[idx].dma_shared_fd[2] = input->shared_fd[2];
            VLOG(INFO,"Set DMA buffer index %d planes %d fd[%d, %d, %d]\n", idx, input->num_planes,
                input->shared_fd[0], input->shared_fd[1], input->shared_fd[2]);
        }
      }
    //ctx->FrameIO[idx] = *input;
    ctx->encodedSrcFrmIdxArr[idx] = 1; // occupied the frames.
    ctx->encMEMSrcFrmIdxArr[param->srcIdx]  = idx; //indirect  link due to reordering. srcIdx increase linear.
    ctx->pFbSrc[idx].stride = src_stride; /**< A horizontal stride for given frame buffer */
    ctx->pFbSrc[idx].height = input->height; /**< A height for given frame buffer */

    VLOG(INFO, "Assign src buffer,input idx %d poolidx %d stride %d \n",param->srcIdx, idx, src_stride);
    if (ctx->bsBuffer[idx].size == 0) { // allocate buffer
      ctx->bsBuffer[idx].size = ctx->bsBuffer[0].size; //ENC_STREAM_BUF_SIZE;
      if (vdi_allocate_dma_memory(ctx->encOpenParam.coreIdx, &ctx->bsBuffer[idx]) < 0) {
          VLOG(ERR, "fail to allocate bitstream buffer\n");
          ctx->bsBuffer[idx].size = 0;
          ctx->encodedSrcFrmIdxArr[idx] = 0;
          return AMVENC_FAIL;
      }
    }
    param->sourceFrame = &ctx->pFbSrc[idx];
    param->picStreamBufferAddr = ctx->bsBuffer[idx].phys_addr;
    param->picStreamBufferSize = ctx->bsBuffer[idx].size;
    VLOG(INFO, "Assign stream buffer, idx %d address  0x%x size %d\n",
          idx, param->picStreamBufferAddr, param->picStreamBufferSize);
    //  param->srcIdx                             = param->srcIdx;
    param->srcEndFlag                         = 0; //in->last;
    //    encParam->sourceFrame                        = &in->fb;
    param->sourceFrame->sourceLBurstEn        = 0;

    if (ctx->op_flag & AMVEncFrameIO_FORCE_SKIP_FLAG) {
        param->skipPicture                        = 1;
    } else {
        param->skipPicture                        = 0;
    }

    //param->forceAllCtuCoefDropEnable           = 0;

    //param->forcePicQpEnable                    = ctx->mInitParams.forcePicQpEnable;
    //param->forcePicQpI                         = ctx->mInitParams.forcePicQpI;
    //param->forcePicQpP                         = ctx->mInitParams.forcePicQpP;
    //param->forcePicQpB                         = ctx->mInitParams.forcePicQpB;

    if (ctx->op_flag & AMVEncFrameIO_FORCE_IDR_FLAG) {
        param->forcePicTypeEnable = 1;
        param->forcePicType = 3; //  0 for I frame. 3 for IDR
    } else {
        param->forcePicTypeEnable = 0;
        param->forcePicType = 0;
    }

    // FW will encode header data implicitly when changing the header syntaxes
    param->codeOption.implicitHeaderEncode    = 1;
    param->codeOption.encodeAUD               = 0;
    param->codeOption.encodeEOS               = 0;
    param->codeOption.encodeEOB               = 0;

    return AMVENC_SUCCESS;
}

AMVEnc_Status AML_HEVCEncChangeBitRate(amv_enc_handle_hevc_t ctx_handle,
                                int BitRate)
{
  AMVHEVCCtx * ctx = (AMVHEVCCtx* ) ctx_handle;
  EncChangeParam *ChgParam;

  if (ctx == NULL) return AMVENC_FAIL;
  if (ctx->magic_num != MULTI_ENC_MAGIC)
    return AMVENC_FAIL;
  if (ctx->mInitParams.param_change_enable == 0)
    return AMVENC_FAIL;
  VLOG(INFO, "Change bit rate to %d Count %d\n", BitRate, ctx->changedCount);

  ChgParam = &ctx->changeParam;

  ChgParam->bitRate = BitRate;
  ChgParam->enable_option |= ENC_RC_TARGET_RATE_CHANGE;
  ctx->param_change_flag ++;

  return AMVENC_SUCCESS;
}

AMVEnc_Status AML_HEVCEncChangeFrameRate(amv_enc_handle_hevc_t ctx_handle,
                                int FrameRate,int BitRate)
{
  AMVHEVCCtx * ctx = (AMVHEVCCtx* ) ctx_handle;
  EncChangeParam *ChgParam;

  if (ctx == NULL) return AMVENC_FAIL;
  if (ctx->magic_num != MULTI_ENC_MAGIC)
    return AMVENC_FAIL;
  if (ctx->mInitParams.param_change_enable == 0)
    return AMVENC_FAIL;
  VLOG(INFO, "Change frame rate to %d Count %d\n", FrameRate, ctx->changedCount);

  ChgParam = &ctx->changeParam;

  ChgParam->frameRate = FrameRate;
  ChgParam->bitRate = BitRate;
  ChgParam->enable_option |= ENC_FRAME_RATE_CHANGE | ENC_RC_TARGET_RATE_CHANGE;
  ctx->param_change_flag ++;

  return AMVENC_SUCCESS;
}


AMVEnc_Status AML_HEVCEncHeader(amv_enc_handle_hevc_t ctx_handle,
                                unsigned char* buffer,
                                unsigned int* buf_nal_size)
{
    EncHeaderParam encHeaderParam;
    EncOutputInfo  encOutputInfo;
    //QueueStatusInfo queueStatus;
    ENC_INT_STATUS status;
    PhysicalAddress paRdPtr = 0;
    PhysicalAddress paWrPtr = 0;
    int header_size = 0;
    int retry = 0;
    RetCode result= RETCODE_SUCCESS;
    AMVHEVCCtx * ctx = (AMVHEVCCtx* ) ctx_handle;

    if (ctx == NULL) return AMVENC_FAIL;
    if (ctx->magic_num != MULTI_ENC_MAGIC) return AMVENC_FAIL;

    memset(&encHeaderParam, 0x00, sizeof(EncHeaderParam));
    encHeaderParam.buf = ctx->bsBuffer[0].phys_addr;
    encHeaderParam.size = ctx->bsBuffer[0].size;

    encHeaderParam.headerType = CODEOPT_ENC_VPS | CODEOPT_ENC_SPS | CODEOPT_ENC_PPS;
    result = VPU_EncGiveCommand(ctx->enchandle, ENC_PUT_VIDEO_HEADER, &encHeaderParam);
    if (result != RETCODE_SUCCESS) {
        VLOG(ERR, "VPU_EncGiveCommand ( ENC_PUT_VIDEO_HEADER ) for VPS/SPS/PPS failed Error Reason code : 0x%x \n", result);
        return AMVENC_FAIL;
    }
    if (encHeaderParam.size == 0) {
        VLOG(ERR, "encHeaderParam.size=0\n");
        return AMVENC_FAIL;
    }
    VLOG(DEBUG, "Enc HEADER size %d\n",encHeaderParam.size);
    *buf_nal_size = encHeaderParam.size;

    vdi_read_memory(ctx->encOpenParam.coreIdx,
          ctx->bsBuffer[0].phys_addr, buffer, encHeaderParam.size,
          ctx->encOpenParam.streamEndian);
    return AMVENC_SUCCESS;
}
#define VPU_WAIT_TIME_OUT               10  //should be less than normal decoding time to give a chance to fill stream. if this value happens some problem. we should fix VPU_WaitInterrupt function

AMVEnc_Status AML_HEVCEncGetAvgQp(amv_enc_handle_hevc_t ctx_handle, int *avgqp) {
    AMVEnc_Status ret = AMVENC_FAIL;
    AMVHEVCCtx *ctx = (AMVHEVCCtx* )ctx_handle;

    if (ctx == NULL) return AMVENC_FAIL;
    if (ctx->averageQP) {
        *avgqp = ctx->averageQP;
        ret = AMVENC_SUCCESS;
    }
    return ret;
}

AMVEnc_Status AML_HEVCEncNAL(amv_enc_handle_hevc_t ctx_handle,
                             unsigned char* buffer,
                             unsigned int* buf_nal_size,
                             int *nal_type)
{
    RetCode                 result;
    EncParam*               encParam;
    EncOutputInfo           encOutputInfo;
    ENC_INT_STATUS          intStatus;
    int retry_cnt = 0;
    int idx;
    int                 ret = RETCODE_SUCCESS;
    int                 coreIdx;

    AMVHEVCCtx *ctx = (AMVHEVCCtx* )ctx_handle;
    if (ctx == NULL) return AMVENC_FAIL;
    if (ctx->magic_num != MULTI_ENC_MAGIC) return AMVENC_FAIL;

    if (ctx ->param_change_flag) {
        result = VPU_EncGiveCommand(ctx->enchandle, ENC_SET_PARA_CHANGE, &ctx->changeParam);
        if (result == RETCODE_SUCCESS) {
            VLOG(INFO, "ENC_SET_PARA_CHANGE queue success option 0x%x\n",
                        ctx->changeParam.enable_option);

            //if (VPU_EncInstParamSync(ctx->enchandle, 0, 0, &ctx->changeParam) != RETCODE_SUCCESS) {
            //    VLOG(ERR, "VPU instance param sync with change param failed!\n");
            //}

            osal_memset(&ctx->changeParam, 0x00, sizeof(EncChangeParam));
            ctx->param_change_flag = 0;
            ctx->changedCount ++;
        }
        else { // Error
            VLOG(ERR, "ENC_SET_PARA_CHANGE failed Error code is 0x%x \n", result);
            return AMVENC_FAIL;
        }
    }

    encParam        = &ctx->encParam;
    coreIdx = ctx->encOpenParam.coreIdx;

    result = VPU_EncStartOneFrame(ctx->enchandle, encParam);
    if (result == RETCODE_SUCCESS) {
        ctx->frameIdx++;
        encParam->srcIdx++;
        if (encParam ->srcIdx >= ctx->src_num) encParam->srcIdx = 0;
        VLOG(INFO, "VPU_EncStartOneFrame sucesss frameIdx %d srcIdx %d  \n", ctx->frameIdx, encParam->srcIdx);
    }
    else { // Error
        VLOG(ERR, "VPU_EncStartOneFrame failed Error code is 0x%x \n", result);
        idx = ctx->encMEMSrcFrmIdxArr[encParam->srcIdx];
        ctx->encodedSrcFrmIdxArr[idx] = 0; //free slot as failed to start
        return AMVENC_FAIL;
    }
retry_pointB:
    while (retry_cnt++ < 100) {
        if ((intStatus=HandlingInterruptFlag(ctx)) == ENC_INT_STATUS_TIMEOUT) {
            VPU_SWReset(ctx->encOpenParam.coreIdx, SW_RESET_SAFETY, ctx->enchandle);
            VLOG(ERR, "Timeout of encoder interrupt reset \n");
            return AMVENC_FAIL;
        }
        else if (intStatus == ENC_INT_STATUS_DONE) break; //success get  done intr
    }
    osal_memset(&encOutputInfo, 0x00, sizeof(EncOutputInfo));
    ret = VPU_EncGetOutputInfo(ctx->enchandle, &encOutputInfo);
    if (ret != RETCODE_SUCCESS) {
        VLOG(ERR, "VPU_EncGetOutputInfo failed Error code is 0x%x \n", ret);
        if (ret == RETCODE_STREAM_BUF_FULL) {
            VLOG(ERR, "RETCODE_STREAM_BUF_FULL\n");
            //goto retry_pointB; //return TRUE; /* Try again */
            return AMVENC_BITSTREAM_BUFFER_FULL;
        }
        else {
            EnterLock(coreIdx);
            VPU_SWReset(coreIdx, SW_RESET_SAFETY, ctx->enchandle);
            LeaveLock(coreIdx);
        }
        return AMVENC_FAIL;
    }
    ctx->averageQP = encOutputInfo.avgCtuQp;

    DisplayEncodedInformation(ctx->enchandle,
                    ctx->encOpenParam.bitstreamFormat,
                    ctx->frameIdx, &encOutputInfo,
                    encParam->srcEndFlag,
                    encParam->srcIdx,
                    1);

    if (encOutputInfo.bitstreamWrapAround == 1) {
         VLOG(WARN, "Warning!! BitStream buffer wrap arounded. prepare more large buffer \n", ret );
     }
     if (encOutputInfo.bitstreamSize == 0 && encOutputInfo.reconFrameIndex >= 0) {
         VLOG(ERR, "ERROR!!! bitstreamsize = 0 \n");
     }

     if (ctx->encOpenParam.lineBufIntEn == 0) {
         if (encOutputInfo.wrPtr < encOutputInfo.rdPtr)
         {
             VLOG(ERR, "wrptr < rdptr\n");
             return AMVENC_FAIL;
         }
     }

    if (encOutputInfo.reconFrameIndex == -1) { // end of encoding
        VLOG(INFO, "end of encoding!\n");
        retry_cnt = 0;
        return AMVENC_FAIL;
    }

    VLOG(INFO, "encOutputInfo.encSrcIdx %d  reconFrameIndex %d \n",
    encOutputInfo.encSrcIdx, encOutputInfo.reconFrameIndex);

#if 0  //release flag
    for (i = 0; i < ctx->fbCount.srcFbNum; i++) {
        if ( (encOutputInfo.releaseSrcFlag >> i) & 0x01) {
            ctx->encodedSrcFrmIdxArr[i] = 1;
        }
    }
#endif

    if ( encOutputInfo.encSrcIdx >= 0) {
        idx = ctx->encMEMSrcFrmIdxArr[encOutputInfo.encSrcIdx];
        ctx->encodedSrcFrmIdxArr[idx] = 0;
        ctx->fullInterrupt  = FALSE;
        // copy frames

        VLOG(INFO, "Enc bitstream size %d pic_type %d avgQP %d\n",
                encOutputInfo.bitstreamSize, encOutputInfo.picType,
                encOutputInfo.avgCtuQp);
        *buf_nal_size = encOutputInfo.bitstreamSize;
        vdi_read_memory(ctx->encOpenParam.coreIdx,
            encOutputInfo.bitstreamBuffer,
            buffer, encOutputInfo.bitstreamSize,
            ctx->encOpenParam.streamEndian);
        #if 0
        if ((encOutputInfo.picType & 0xff) == I_SLICE) {
            if (ctx->mNumInputFrames == 1)
                *nal_type = HEVC_IDR;
            else
                *nal_type = ctx->mInitParams.refresh_type;
        } else if ((encOutputInfo.picType & 0xff) == P_SLICE || (encOutputInfo.picType & 0xff) == B_SLICE)
           *nal_type = NON_IRAP;
        #else
        *nal_type = encOutputInfo.picType;
        #endif
        /**Retframe= ctx->FrameIO[idx];
        Retframe->encoded_frame_type = encOutputInfo.picType;
        Retframe->enc_average_qp = encOutputInfo.avgCtuQp;
        Retframe->enc_intra_blocks = encOutputInfo.numOfIntra;
        Retframe->enc_merged_blocks = encOutputInfo.numOfMerge;
        Retframe->enc_skipped_blocks = encOutputInfo.numOfSkipBlock;*/

    }

    VLOG(INFO, "Done one picture !! \n");
    return AMVENC_PICTURE_READY;
}

AMVEnc_Status AML_HEVCRelease(amv_enc_handle_hevc_t ctx_handle)
{
    AMVEnc_Status ret = AMVENC_SUCCESS;
    RetCode       result;
    EncParam*    encParam;
    AMVHEVCCtx * ctx = (AMVHEVCCtx* )ctx_handle;
    ENC_INT_STATUS intStatus;
    Uint32 coreIdx = ctx->encOpenParam.coreIdx;
    int idx;
    bool useLock = false;

    if (ctx == NULL)
        return AMVENC_FAIL;
    if (ctx->magic_num != MULTI_ENC_MAGIC)
        return AMVENC_FAIL;

    if (ctx->frameIdx) {
        encParam        = &ctx->encParam;
        encParam->srcEndFlag = 1;  // flush out frame
        result = VPU_EncStartOneFrame(ctx->enchandle, encParam);
        if (result == RETCODE_SUCCESS) {
            ctx->frameIdx++;
        }
        else { // Error
            VLOG(ERR, "VPU_EncStartOneFrame failed Error code is 0x%x \n", result);
            //return AMVENC_FAIL;
        }
    }
    else {
        useLock = true;
    }

    if ((intStatus=HandlingInterruptFlag(ctx,useLock)) == ENC_INT_STATUS_TIMEOUT) {
        VPU_SWReset(ctx->encOpenParam.coreIdx, SW_RESET_SAFETY, ctx->enchandle);
        VLOG(ERR, "Timeout of encoder interrupt reset \n");
        return AMVENC_FAIL;
    }else if (intStatus == ENC_INT_STATUS_DONE) {
        EncOutputInfo   outputInfo;
        VPU_EncGetOutputInfo(ctx->enchandle, &outputInfo);
    }
    VPU_EncClose(ctx->enchandle);

    //if (ctx->vbCustomLambda.size)
        //vdi_free_dma_memory(coreIdx, &ctx->vbCustomLambda);
    //if (ctx->vbScalingList.size)
        //vdi_free_dma_memory(coreIdx, &ctx->vbScalingList);

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
    //if (ctx->CustomRoiMapBuf)
    //{
    //    free(ctx->CustomRoiMapBuf);
    //    ctx->CustomRoiMapBuf = NULL;
    //}
    //if (ctx->CustomLambdaMapBuf)
    //{
    //    free(ctx->CustomLambdaMapBuf);
    //    ctx->CustomLambdaMapBuf = NULL;
    //}
    //if (ctx->CustomModeMapBuf)
    //{
    //    free(ctx->CustomModeMapBuf);
    //    ctx->CustomModeMapBuf = NULL;
    //}

#if SUPPORT_SCALE
      if (ctx->ge2d_initial_done) {
        aml_ge2d_mem_free(&ctx->amlge2d);
        aml_ge2d_exit(&ctx->amlge2d);
        VLOG(DEBUG, "ge2d exit!!!\n");
      }
#endif
    VPU_DeInit(ctx->encOpenParam.coreIdx);

    free(ctx);
    VLOG(INFO, "AML_HEVCRelease succeed\n");
    return ret;
}
