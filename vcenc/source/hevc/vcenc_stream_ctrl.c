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
#ifdef STREAM_CTRL_BUILD_SUPPORT
/*------------------------------------------------------------------------------
       Version Information
------------------------------------------------------------------------------*/
#include "hevcencapi.h"
#include "cutree_stream_ctrl.h"
#include "instance.h"
#include "encasiccontroller.h"
#include "hevcencapi_utils.h"
#include "sw_picture.h"
#include "ewl.h"
#include "tools.h"

#ifdef SUPPORT_AV1
#include "av1encapi.h"
#endif

#ifdef SUPPORT_VP9
#include "vp9encapi.h"
#endif

/**
 * @brief client type parser, it translate the codec format defined in api to client type defined in EWL
          which is used to select available HW engine.
 * @param[in] codecFormat codec format defined in api.
 * @return CLIENT_TYPE client type defined in EWL.
 */
static CLIENT_TYPE get_client_type(VCEncVideoCodecFormat codecFormat)
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

/**
 * @brief VCEncChangeResolutionCheck(), check the new resolution.
 * @param VCEncInst inst: encoder instance.
 * @param void *ctx: encoder context.
 * @param int w: new width.
 * @param int h: new height.
 * @param int exteralReconAlloc: if the memory is allocated externally.
 * @return VCEncRet: VCENC_OK successful, other unsuccessful.
 */
static VCEncRet VCEncChangeResolutionCheck(VCEncInst inst, void *ctx, int w, int h,
                                           int exteralReconAlloc)
{
    struct vcenc_instance *vcenc_instance = (struct vcenc_instance *)inst;
    asicData_s *asic = &vcenc_instance->asic;
    u32 width = w, height = h;
    u32 client_type = get_client_type(vcenc_instance->codecFormat);
    EWLHwConfig_t cfg = EncAsicGetAsicConfig(client_type, ctx);
    VCEncRet ret = VCENC_OK;

    /* Check status, this also makes sure that the instance is valid */
    if (vcenc_instance->encStatus != VCENCSTAT_INIT) {
        APITRACEERR("VCEncChangeResolution: ERROR Invalid status");
        return VCENC_INVALID_STATUS;
    }

    APITRACEPARAM("width", width);
    APITRACEPARAM("height", height);

    if (vcenc_instance->parallelCoreNum > 1 && width * height < 256 * 256) {
        APITRACE("VCEncChangeResolution: Use single core for small resolution");
        vcenc_instance->parallelCoreNum = 1;
    }

    if (IS_AV1(vcenc_instance->codecFormat) || IS_VP9(vcenc_instance->codecFormat)) {
        if (width > 4096) {
            APITRACEERR("VCEncChangeResolution: Invalid width, need 4096 or smaller when AV1 "
                        "or VP9");
            return VCENC_INVALID_ARGUMENT;
        }
        if (width * height > 4096 * 2304) {
            APITRACEERR("VCEncChangeResolution: Invalid area, need 4096*2304 or below when "
                        "AV1 or VP9");
            return VCENC_INVALID_ARGUMENT;
        }
    }

    /* Encoded image height limits, multiple of 2 */
    if (width < VCENC_MIN_ENC_WIDTH || width > VCENC_MAX_ENC_WIDTH || (width & 0x1) != 0) {
        APITRACEERR("VCEncChangeResolution: Invalid width");
        return VCENC_INVALID_ARGUMENT;
    }

    /* Encoded image width limits, multiple of 2 */
    if (height < VCENC_MIN_ENC_HEIGHT || height > VCENC_MAX_ENC_HEIGHT_EXT || (height & 0x1) != 0) {
        APITRACEERR("VCEncChangeResolution: Invalid height");
        return VCENC_INVALID_ARGUMENT;
    }

    /* max width supported */
    if ((IS_H264(vcenc_instance->codecFormat) ? cfg.maxEncodedWidthH264 : cfg.maxEncodedWidthHEVC) <
        width) {
        APITRACEERR("VCEncChangeResolution: Invalid width, not supported by HW coding "
                    "core");
        return VCENC_INVALID_ARGUMENT;
    }

    /*extend video coding height */
    if ((height > VCENC_MAX_ENC_HEIGHT) && (cfg.videoHeightExt == EWL_HW_CONFIG_NOT_SUPPORTED)) {
        APITRACEERR("VCEncChangeResolution: Invalid height, height extension not supported "
                    "by HW coding core");
        return VCENC_INVALID_ARGUMENT;
    }

    if (width > vcenc_instance->ori_width && height > vcenc_instance->ori_height) {
        APITRACEERR("VCEncChangeResolution: width and height exceeded max(ori) value.");
        return VCENC_INVALID_ARGUMENT;
    }

    if (asic->regs.codingType == ASIC_JPEG || asic->regs.codingType == ASIC_CUTREE)
        return VCENC_OK;

    if (vcenc_instance->ori_width < w && vcenc_instance->ori_height < h)
        return VCENC_INVALID_ARGUMENT;

    asicMemAlloc_s allocCfg;
    asicData_s asic_tmp;
    asic_tmp.regs.asicHwId = asic->regs.asicHwId;
    u32 internalImageLumaSize_ori, lumaSize_ori, lumaSize4N_ori, chromaSize_ori,
        u32FrameContextSize_ori;
    u32 internalImageLumaSize, lumaSize, lumaSize4N, chromaSize, u32FrameContextSize;

    memset(&allocCfg, 0, sizeof(asicMemAlloc_s));
    allocCfg.width = vcenc_instance->ori_width;
    allocCfg.height = vcenc_instance->ori_height;
    allocCfg.encodingType =
        (IS_H264(vcenc_instance->codecFormat)
             ? ASIC_H264
             : (IS_HEVC(vcenc_instance->codecFormat)
                    ? ASIC_HEVC
                    : IS_VP9(vcenc_instance->codecFormat) ? ASIC_VP9 : ASIC_AV1));
    allocCfg.ref_alignment = vcenc_instance->ref_alignment;
    allocCfg.ref_ch_alignment = vcenc_instance->ref_ch_alignment;
    allocCfg.bitDepthLuma = vcenc_instance->sps->bit_depth_luma_minus8 + 8;
    allocCfg.bitDepthChroma = vcenc_instance->sps->bit_depth_chroma_minus8 + 8;
    allocCfg.compressor = vcenc_instance->compressor;
    EncAsicGetSizeAndSetRegs(&asic_tmp, &allocCfg, &internalImageLumaSize_ori, &lumaSize_ori,
                             &lumaSize4N_ori, &chromaSize_ori, &u32FrameContextSize_ori);

    allocCfg.width = w;
    allocCfg.height = h;
    EncAsicGetSizeAndSetRegs(&asic_tmp, &allocCfg, &internalImageLumaSize, &lumaSize, &lumaSize4N,
                             &chromaSize, &u32FrameContextSize);

    if (internalImageLumaSize > internalImageLumaSize_ori || lumaSize > lumaSize_ori ||
        lumaSize4N > lumaSize4N_ori || chromaSize > chromaSize_ori) {
        APITRACEERR("VCEncChangeResolution: Memory exceeded max(ori) value.");
        return VCENC_INVALID_ARGUMENT;
    }

    return VCENC_OK;
}

/**
 * @brief VCEncChangeResolution(), change to the new resolution.
 * @param VCEncInst inst: encoder instance.
 * @param void *ctx: encoder context.
 * @param int w: new width.
 * @param int h: new height.
 * @param int exteralReconAlloc: if the memory is allocated externally.
 * @return VCEncRet: VCENC_OK successful, other unsuccessful.
 */
static VCEncRet VCEncChangeResolution(VCEncInst inst, void *ctx, int w, int h,
                                      int exteralReconAlloc)
{
    struct vcenc_instance *vcenc_instance = (struct vcenc_instance *)inst;
    asicData_s *asic = &vcenc_instance->asic;
    struct container *c;
    VCEncRet ret = VCENC_OK;
    u32 ctb_size, width, height;

    /* always set the delay after EOS */
    vcenc_instance->idelay = 0;

    if ((vcenc_instance->width == w) && (vcenc_instance->height == h))
        return VCENC_OK;

    ret = VCEncChangeResolutionCheck(inst, ctx, w, h, exteralReconAlloc);
    if (ret != VCENC_OK)
        return ret;

    if (asic->regs.codingType == ASIC_JPEG || asic->regs.codingType == ASIC_CUTREE)
        return VCENC_OK;

    asicMemAlloc_s allocCfg;
    memset(&allocCfg, 0, sizeof(asicMemAlloc_s));
    allocCfg.width = w;
    allocCfg.height = h / (vcenc_instance->interlaced + 1);
    allocCfg.encodingType =
        (IS_H264(vcenc_instance->codecFormat)
             ? ASIC_H264
             : (IS_HEVC(vcenc_instance->codecFormat)
                    ? ASIC_HEVC
                    : IS_VP9(vcenc_instance->codecFormat) ? ASIC_VP9 : ASIC_AV1));
    allocCfg.ref_alignment = vcenc_instance->ref_alignment;
    allocCfg.ref_ch_alignment = vcenc_instance->ref_ch_alignment;
    allocCfg.bitDepthLuma = vcenc_instance->sps->bit_depth_luma_minus8 + 8;
    allocCfg.bitDepthChroma = vcenc_instance->sps->bit_depth_chroma_minus8 + 8;
    allocCfg.compressor = vcenc_instance->compressor;
    allocCfg.outputCuInfo = vcenc_instance->outputCuInfo;
    allocCfg.outputCtbBits = vcenc_instance->outputCtbBits;
    allocCfg.exteralReconAlloc = exteralReconAlloc;
    allocCfg.ctbRcMode = vcenc_instance->ctbRcMode;
    allocCfg.aqInfoAlignment = vcenc_instance->aqInfoAlignment;
    allocCfg.maxTemporalLayers = vcenc_instance->maxTLayers;
    allocCfg.numRefBuffsLum = allocCfg.numRefBuffsChr =
        vcenc_instance->sps->max_dec_pic_buffering[0] - 1 + vcenc_instance->parallelCoreNum - 1;
    allocCfg.numCuInfoBuf = vcenc_instance->numCuInfoBuf;
    allocCfg.numCtbBitsBuf = vcenc_instance->parallelCoreNum;
    allocCfg.pass = vcenc_instance->pass;
    if (vcenc_instance->asic.regs.P010RefEnable) {
        if (allocCfg.bitDepthLuma > 8 && (allocCfg.compressor & 1) == 0)
            allocCfg.bitDepthLuma = 16;
        if (allocCfg.bitDepthChroma > 8 && (allocCfg.compressor & 2) == 0)
            allocCfg.bitDepthChroma = 16;
    }
    allocCfg.codedChromaIdc =
        (vcenc_instance->asic.regs.asicCfg.MonoChromeSupport == EWL_HW_CONFIG_NOT_SUPPORTED)
            ? VCENC_CHROMA_IDC_420
            : vcenc_instance->asic.regs.codedChromaIdc;
    allocCfg.tmvpEnable = vcenc_instance->enableTMVP;
    allocCfg.is_malloc = 0;
    EncAsicMemAlloc_V2(asic, &allocCfg);

    vcenc_instance->asic.regs.picWidth = vcenc_instance->preProcess.lumWidth =
        vcenc_instance->width = w; //crop disabled, todo
    vcenc_instance->asic.regs.picHeight = vcenc_instance->preProcess.lumHeight =
        vcenc_instance->height = h;

    if (vcenc_instance->lookaheadDepth == 0) {
        vcenc_instance->preProcess.lumWidthSrc[0] = w;
        vcenc_instance->preProcess.lumHeightSrc[0] = h;
    } else if (vcenc_instance->lookaheadDepth > 0 && vcenc_instance->pass == 2) {
        vcenc_instance->preProcess.lumWidthSrc[0] = w;
        vcenc_instance->preProcess.lumHeightSrc[0] = h;
        if (vcenc_instance->lookahead.priv_inst) {
            struct vcenc_instance *vcenc_instance_pass1 =
                (struct vcenc_instance *)vcenc_instance->lookahead.priv_inst;
            vcenc_instance_pass1->preProcess.lumWidthSrc[0] = w;
            vcenc_instance_pass1->preProcess.lumHeightSrc[0] = h;
        }
    }

    ctb_size = (IS_H264(vcenc_instance->codecFormat) ? 16 : 64);
    width = ctb_size * ((vcenc_instance->width + ctb_size - 1) / ctb_size);
    height = ctb_size * ((vcenc_instance->height + ctb_size - 1) / ctb_size);
    vcenc_instance->ctbPerRow = width / ctb_size;
    vcenc_instance->ctbPerCol = height / ctb_size;
    vcenc_instance->ctbPerFrame = vcenc_instance->ctbPerRow * vcenc_instance->ctbPerCol;

#ifdef SUPPORT_AV1
    if (IS_AV1(vcenc_instance->codecFormat)) {
        VCEncAV1SetResolution(inst);
    }
#endif

#ifdef SUPPORT_VP9
    if (IS_VP9(vcenc_instance->codecFormat)) {
        VCEncVP9SetResolution(inst);
    }
#endif

    /* adjust the acllocated sw_picture parameters */
    if ((c = get_container(vcenc_instance))) {
        /* Adjust Input and reconstructed pictures size for recon dump */
        struct node *n;
        struct sw_picture *p;

        for (n = c->picture.tail; n; n = n->next) {
            p = (struct sw_picture *)n;
            p->picture_memory_init = 0;
            if (sw_init_image(p, &p->input[0], w, h, SW_IMAGE_U8, HANTRO_FALSE, 0, 0))
                goto out;
            if (sw_init_image(p, &p->recon, WIDTH_64_ALIGN(width), height, SW_IMAGE_U8,
                              HANTRO_FALSE, 0, 0))
                goto out;
        }
    out:
        if (n != NULL) {
            APITRACEERR("fail to init the picture structure\n");
        }
    }

    asic->regs.l0_delta_poc[0] = 0;
    asic->regs.l0_delta_poc[1] = 0;
    asic->regs.l0_used_by_curr_pic[0] = 0;
    asic->regs.l0_used_by_curr_pic[1] = 0;

    asic->regs.l1_delta_poc[0] = 0;
    asic->regs.l1_delta_poc[1] = 0;
    asic->regs.l1_used_by_curr_pic[0] = 0;
    asic->regs.l1_used_by_curr_pic[1] = 0;

    asic->regs.intraCu8Num = 0;
    asic->regs.skipCu8Num = 0;
    asic->regs.PBFrame4NRdCost = 0;
    asic->regs.lumSSEDivide256 = 0;
    asic->regs.SSEDivide256 = 0;
    asic->regs.nrMbNumInvert = 0;

    asic->regs.cbSSEDivide64 = 0;
    asic->regs.crSSEDivide64 = 0;
    asic->regs.motionScore[0][0] = 0;
    asic->regs.motionScore[0][1] = 0;
    asic->regs.motionScore[1][0] = 0;
    asic->regs.motionScore[1][1] = 0;

    if (vcenc_instance->lookaheadDepth > 0 && vcenc_instance->pass == 2) {
        asic->regs.rcRoiEnable = 1;
    }
    memset(asic->regs.regMirror, 0, sizeof(u32) * ASIC_SWREG_AMOUNT);

    if (vcenc_instance->lookaheadDepth > 0 && vcenc_instance->lookahead.priv_inst) {
        int w_pass1 = ((w / 2) >> 1) << 1;
        int h_pass1 = ((h / 2) >> 1) << 1;
        ret = VCEncChangeResolution(vcenc_instance->lookahead.priv_inst, ctx, w_pass1, h_pass1, 0);
        if (ret != VCENC_OK)
            return ret;
    }

    return VCENC_OK;
}

/**
 * @brief VCEncReleaseCoreInst(), Release core instance in C-model.
 * @param VCEncInst inst: encoder instance.
 * @return void.
 */
static void VCEncReleaseCoreInst(VCEncInst inst)
{
    if (inst == NULL) {
        APITRACEERR("VCEncReleaseCoreInst: ERROR Null argument");
        return;
    }

    struct vcenc_instance *pEncInst = (struct vcenc_instance *)inst;
    struct vcenc_instance *pEncInst_pass1 = (struct vcenc_instance *)pEncInst->lookahead.priv_inst;
    const void *ewl;

    ewl = pEncInst->asic.ewl;
    EWLReleaseEwlWorkerInst(ewl);

    if (pEncInst->lookaheadDepth && pEncInst_pass1) {
        const void *ewl_pass1 = pEncInst_pass1->asic.ewl;
        EWLReleaseEwlWorkerInst(ewl_pass1);
        EWLReleaseEwlWorkerInst(pEncInst_pass1->cuTreeCtl.asic.ewl);
    }
}

/**
 * @brief VCEncSetStrmCtrl(), Set stream control.In previous VC8000E software, there's no way to
 *        change sequence level parameters such as width height etc dynamically.
 *        To support related requirement, this new API named VCEncSetStrmCtrl is designed.
 * @param VCEncInst inst: encoder instance.
 * @param VCEncStrmCtrl*params: stream control params.
 * @return VCEncRet: VCENC_OK successful, other unsuccessful.
 */
VCEncRet VCEncSetStrmCtrl(VCEncInst inst, VCEncStrmCtrl *params)
{
    if (inst == NULL) {
        APITRACEERR("VCEncSetStrmCtrl: ERROR Null argument");
        return VCENC_NULL_ARGUMENT;
    }

    VCEncRet ret;
    struct vcenc_instance *vcenc_instance = (struct vcenc_instance *)inst;
    struct vcenc_instance *vcenc_instance_pass1 =
        (struct vcenc_instance *)vcenc_instance->lookahead.priv_inst;

    if (vcenc_instance->encStatus != VCENCSTAT_INIT) {
        APITRACEERR("VCEncSetStrmCtrl: ERROR Invalid status");
        return VCENC_INVALID_STATUS;
    }

    /*Make sure the lookahead and cutree thread is terminated.*/
    if (vcenc_instance->lookaheadDepth && vcenc_instance_pass1) {
        TerminateLookaheadThread(&vcenc_instance->lookahead,
                                 vcenc_instance->encStatus == VCENCSTAT_ERROR);
        TerminateCuTreeThread(&vcenc_instance_pass1->cuTreeCtl,
                              vcenc_instance->encStatus == VCENCSTAT_ERROR);
    }

    /*resolution change*/
    if ((ret = VCEncChangeResolution(inst, vcenc_instance->ctx, params->width, params->height,
                                     0)) != VCENC_OK) {
        return ret;
    }

    /*reset rate control*/
    i32 ctb_size = (IS_H264(vcenc_instance->codecFormat) ? 16 : 64);
    vcenc_instance->rateControl.picArea =
        ((vcenc_instance->width + 7) & (~7)) * ((vcenc_instance->height + 7) & (~7));
    vcenc_instance->rateControl.ctbPerPic = vcenc_instance->ctbPerFrame;
    vcenc_instance->rateControl.reciprocalOfNumBlk8 =
        1.0 / (vcenc_instance->rateControl.ctbPerPic * (ctb_size / 8) * (ctb_size / 8));
    vcenc_instance->rateControl.ctbRows = vcenc_instance->ctbPerCol;
    vcenc_instance->rateControl.ctbCols = vcenc_instance->ctbPerRow;

    vcenc_instance->rateControl.ctbRateCtrl.models[0]
        .x0 = vcenc_instance->rateControl.ctbRateCtrl.models[0].x1 =
        vcenc_instance->rateControl.ctbRateCtrl.models[0].started =
            vcenc_instance->rateControl.ctbRateCtrl.models[0].preFrameMad =
                vcenc_instance->rateControl.ctbRateCtrl.models[1].x0 =
                    vcenc_instance->rateControl.ctbRateCtrl.models[1].x1 =
                        vcenc_instance->rateControl.ctbRateCtrl.models[1].started =
                            vcenc_instance->rateControl.ctbRateCtrl.models[1].preFrameMad =
                                vcenc_instance->rateControl.ctbRateCtrl.models[2].x0 =
                                    vcenc_instance->rateControl.ctbRateCtrl.models[2].x1 =
                                        vcenc_instance->rateControl.ctbRateCtrl.models[2].started =
                                            vcenc_instance->rateControl.ctbRateCtrl.models[2]
                                                .preFrameMad = 0;

    if (vcenc_instance->lookaheadDepth && vcenc_instance_pass1) {
        vcenc_instance_pass1->rateControl.picArea = ((vcenc_instance_pass1->width + 7) & (~7)) *
                                                    ((vcenc_instance_pass1->height + 7) & (~7));
        vcenc_instance_pass1->rateControl.ctbPerPic = vcenc_instance_pass1->ctbPerFrame;
        vcenc_instance_pass1->rateControl.reciprocalOfNumBlk8 =
            1.0 / (vcenc_instance_pass1->rateControl.ctbPerPic * (ctb_size / 8) * (ctb_size / 8));
        vcenc_instance_pass1->rateControl.ctbRows = vcenc_instance_pass1->ctbPerCol;
        vcenc_instance_pass1->rateControl.ctbCols = vcenc_instance_pass1->ctbPerRow;

        vcenc_instance_pass1->rateControl.ctbRateCtrl.models[0]
            .x0 = vcenc_instance_pass1->rateControl.ctbRateCtrl.models[0]
                      .x1 = vcenc_instance_pass1->rateControl.ctbRateCtrl.models[0].started =
            vcenc_instance_pass1->rateControl.ctbRateCtrl.models[0].preFrameMad =
                vcenc_instance_pass1->rateControl.ctbRateCtrl.models[1].x0 =
                    vcenc_instance_pass1->rateControl.ctbRateCtrl.models[1].x1 =
                        vcenc_instance_pass1->rateControl.ctbRateCtrl.models[1].started =
                            vcenc_instance_pass1->rateControl.ctbRateCtrl.models[1].preFrameMad =
                                vcenc_instance_pass1->rateControl.ctbRateCtrl.models[2].x0 =
                                    vcenc_instance_pass1->rateControl.ctbRateCtrl.models[2].x1 =
                                        vcenc_instance_pass1->rateControl.ctbRateCtrl.models[2]
                                            .started =
                                            vcenc_instance_pass1->rateControl.ctbRateCtrl.models[2]
                                                .preFrameMad = 0;
    }
    VCEncSetRateCtrl(inst, &vcenc_instance->rateControl_ori);

    /*cutree related params*/
    if (vcenc_instance->lookaheadDepth && vcenc_instance_pass1) {
        if ((ret = cuTreeChangeResolution(vcenc_instance_pass1,
                                          &vcenc_instance_pass1->cuTreeCtl)) != VCENC_OK) {
            return ret;
        }
    }

    /*ewl clear trace*/
    EWLClearTraceProfile(vcenc_instance->asic.ewl);
    if (vcenc_instance->lookaheadDepth && vcenc_instance_pass1) {
        EWLClearTraceProfile(vcenc_instance_pass1->asic.ewl);
    }

    /*Release core instance in C-model*/
    VCEncReleaseCoreInst(inst);

    /*other parameters, todo*/

    return VCENC_OK;
}

#endif
