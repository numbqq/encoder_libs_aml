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
--                 The entire notice above must be reproduced                                                  --
--                  on all copies and should not be removed.                                                     --
--                                                                                                                                --
--------------------------------------------------------------------------------
--
--  Abstract : VC Encoder Cache Interface Implementation
--
------------------------------------------------------------------------------*/
#ifdef SUPPORT_CACHE

#include "vsi_string.h"
#include "hevcenccache.h"
#include "instance.h"
#include "sw_slice.h"
#include "ewl.h"

#include "cacheapi.h"

i32 VCEncEnableCache(struct vcenc_instance *vcenc_instance, asicData_s *asic,
                     struct sw_picture *pic, u32 tileId)
{
    i32 ret = CACHE_OK;
    char buf[18];
    CWLChannelConf_t channel_cfg;
    u32 rem;
    u32 alignment = vcenc_instance->input_alignment;
    u32 byte_per_compt = 0;
    vcenc_instance->channel_idx = 0;
#define NUM_REF_BUF 5
    static i32 refLum[NUM_REF_BUF] = {0};
    static i32 ref4nLum[NUM_REF_BUF] = {0};
    static i32 refChr[NUM_REF_BUF] = {0};
    i32 base[5], i, j;
    i32 traceRefLu[4] = {BaseRefLum0, BaseRefLum1, BaseRefLum2, BaseRefLum3};
    i32 traceRef4N[4] = {BaseRef4nLum0, BaseRef4nLum1, BaseRef4nLum2, BaseRef4nLum3};
    i32 traceRefChr[4] = {BaseRefChr0, BaseRefChr1, BaseRefChr2, BaseRefChr3};

    if (vcenc_instance->preProcess.inputFormat != VCENC_YUV420_SEMIPLANAR &&
        vcenc_instance->preProcess.inputFormat != VCENC_YUV420_SEMIPLANAR_VU &&
        vcenc_instance->preProcess.inputFormat != VCENC_YUV422_INTERLEAVED_UYVY &&
        vcenc_instance->preProcess.inputFormat != VCENC_YUV422_INTERLEAVED_YUYV &&
        vcenc_instance->preProcess.inputFormat != VCENC_YUV420_PLANAR_10BIT_P010 &&
        vcenc_instance->preProcess.inputFormat != VCENC_YUV420_SEMIPLANAR_8BIT_TILE_4_4 &&
        vcenc_instance->preProcess.inputFormat != VCENC_YUV420_SEMIPLANAR_VU_8BIT_TILE_4_4 &&
        vcenc_instance->preProcess.inputFormat != VCENC_YUV420_PLANAR_10BIT_P010_TILE_4_4)
        return CACHE_OK;

#if 0
  if ((vcenc_instance->preProcess.inputFormat == VCENC_YUV420_PLANAR_10BIT_P010 ||
       vcenc_instance->preProcess.inputFormat == VCENC_YUV420_PLANAR_10BIT_P010_TILE_4_4)&&
       vcenc_instance->asic.regs.P010RefEnable == 0)
    return CACHE_OK;
#endif

    //luma ref, chroma ref, luma table, chroma table, luma 4n
    base[0] = (i32)asic->regs.pRefPic_recon_l0[0][0];
    base[1] = (i32)asic->regs.pRefPic_recon_l1[0][0];
    base[2] = (i32)asic->regs.reconLumBase;
    base[3] = (i32)asic->regs.pRefPic_recon_l0[0][1];
    base[4] = (i32)asic->regs.pRefPic_recon_l1[0][1];
    for (j = 0; j < 5; j++) {
        for (i = 0; i < NUM_REF_BUF; i++) {
            if (refLum[i] == base[j])
                break;
            if (!refLum[i]) {
                refLum[i] = base[j];
                break;
            }
        }
    }

    base[0] = (i32)asic->regs.pRefPic_recon_l0_4n[0];
    base[1] = (i32)asic->regs.pRefPic_recon_l1_4n[0];
    base[2] = (i32)asic->regs.reconL4nBase;
    base[3] = (i32)asic->regs.pRefPic_recon_l0_4n[1];
    base[4] = (i32)asic->regs.pRefPic_recon_l1_4n[1];
    for (j = 0; j < 5; j++) {
        for (i = 0; i < NUM_REF_BUF; i++) {
            if (ref4nLum[i] == base[j])
                break;
            if (!ref4nLum[i]) {
                ref4nLum[i] = base[j];
                break;
            }
        }
    }

    base[0] = (i32)asic->regs.pRefPic_recon_l0[1][0];
    base[1] = (i32)asic->regs.pRefPic_recon_l1[1][0];
    base[2] = (i32)asic->regs.reconCbBase;
    base[3] = (i32)asic->regs.pRefPic_recon_l0[1][1];
    base[4] = (i32)asic->regs.pRefPic_recon_l1[1][1];
    for (j = 0; j < 5; j++) {
        for (i = 0; i < NUM_REF_BUF; i++) {
            if (refChr[i] == base[j])
                break;
            if (!refChr[i]) {
                refChr[i] = base[j];
                break;
            }
        }
    }

    /*cache read channel config*/
    cache_dir dir = CACHE_RD;

    /*If cache is used, call cache API to config and enable it. Input picture & reference frame luma/chroma will be cached*/
#ifdef SUPPORT_INPUT_CACHE
    /*1. input picture luma data*/
    if (vcenc_instance->inputLineBuf.inputLineBufLoopBackEn == 0) {
        switch (vcenc_instance->preProcess.inputFormat) {
        case VCENC_YUV420_SEMIPLANAR:
        case VCENC_YUV420_SEMIPLANAR_VU:
            memset(&channel_cfg, 0, sizeof(CWLChannelConf_t));

            channel_cfg.trace_start_addr = BaseInLum;
            channel_cfg.trace_end_addr = asic->regs.input_luma_stride * vcenc_instance->height;

            channel_cfg.start_addr = pic->input[tileId].lum >> 4;
            channel_cfg.end_addr =
                (pic->input[tileId].lum + (asic->regs.input_luma_stride *
                                           vcenc_instance->preProcess.lumHeightSrc[tileId])) >>
                4; ///

            ret = EnableCacheChannel(&vcenc_instance->cache, &vcenc_instance->channel_idx,
                                     &channel_cfg, ENCODER_VC8000E, dir);
            break;
        case VCENC_YUV422_INTERLEAVED_UYVY:
        case VCENC_YUV422_INTERLEAVED_YUYV:
        case VCENC_YUV420_PLANAR_10BIT_P010:
            memset(&channel_cfg, 0, sizeof(CWLChannelConf_t));

            channel_cfg.trace_start_addr = BaseInLum;
            channel_cfg.trace_end_addr =
                asic->regs.input_luma_stride * vcenc_instance->preProcess.lumHeightSrc[tileId];

            channel_cfg.start_addr = pic->input[tileId].lum >> 4;
            channel_cfg.end_addr =
                (pic->input[tileId].lum + (asic->regs.input_luma_stride *
                                           vcenc_instance->preProcess.lumHeightSrc[tileId])) >>
                4;

            ret = EnableCacheChannel(&vcenc_instance->cache, &vcenc_instance->channel_idx,
                                     &channel_cfg, ENCODER_VC8000E, dir);
            break;
        case VCENC_YUV420_SEMIPLANAR_8BIT_TILE_4_4:
        case VCENC_YUV420_SEMIPLANAR_VU_8BIT_TILE_4_4:
        case VCENC_YUV420_PLANAR_10BIT_P010_TILE_4_4:
            memset(&channel_cfg, 0, sizeof(CWLChannelConf_t));

            if (vcenc_instance->preProcess.inputFormat == VCENC_YUV420_SEMIPLANAR_8BIT_TILE_4_4 ||
                vcenc_instance->preProcess.inputFormat == VCENC_YUV420_SEMIPLANAR_VU_8BIT_TILE_4_4)
                byte_per_compt = 1;
            else if (vcenc_instance->preProcess.inputFormat ==
                     VCENC_YUV420_PLANAR_10BIT_P010_TILE_4_4)
                byte_per_compt = 2;

            channel_cfg.trace_start_addr = BaseInLum;
            channel_cfg.trace_end_addr = (asic->regs.input_luma_stride *
                                          vcenc_instance->preProcess.lumHeightSrc[tileId] / 4);

            channel_cfg.start_addr = pic->input[tileId].lum >> 4;
            channel_cfg.end_addr =
                (pic->input[tileId].lum + (asic->regs.input_luma_stride *
                                           vcenc_instance->preProcess.lumHeightSrc[tileId] / 4)) >>
                4;

            ret = EnableCacheChannel(&vcenc_instance->cache, &vcenc_instance->channel_idx,
                                     &channel_cfg, ENCODER_VC8000E, dir);
            break;
        }

        if (ret != CACHE_OK)
            return ret;
    }

    /*2. input picture chroma data*/
    if (vcenc_instance->inputLineBuf.inputLineBufLoopBackEn == 0) {
        switch (vcenc_instance->preProcess.inputFormat) {
        case VCENC_YUV420_SEMIPLANAR:
        case VCENC_YUV420_SEMIPLANAR_VU:
            memset(&channel_cfg, 0, sizeof(CWLChannelConf_t));

            channel_cfg.trace_start_addr = BaseInCb;
            channel_cfg.trace_end_addr =
                asic->regs.input_chroma_stride * vcenc_instance->height / 2;

            channel_cfg.start_addr = asic->regs.inputCbBase >> 4;
            channel_cfg.end_addr =
                (asic->regs.inputCbBase + (asic->regs.input_chroma_stride *
                                           vcenc_instance->preProcess.lumHeightSrc[tileId] / 2)) >>
                4;

            ret = EnableCacheChannel(&vcenc_instance->cache, &vcenc_instance->channel_idx,
                                     &channel_cfg, ENCODER_VC8000E, dir);

            if (ret != CACHE_OK)
                return ret;
            break;
        case VCENC_YUV420_PLANAR_10BIT_P010:
            memset(&channel_cfg, 0, sizeof(CWLChannelConf_t));

            channel_cfg.trace_start_addr = BaseInCb;
            channel_cfg.trace_end_addr =
                asic->regs.input_chroma_stride * vcenc_instance->height / 2;

            channel_cfg.start_addr = asic->regs.inputCbBase >> 4;
            channel_cfg.end_addr =
                (asic->regs.inputCbBase + (asic->regs.input_chroma_stride *
                                           vcenc_instance->preProcess.lumHeightSrc[tileId] / 2)) >>
                4;

            ret = EnableCacheChannel(&vcenc_instance->cache, &vcenc_instance->channel_idx,
                                     &channel_cfg, ENCODER_VC8000E, dir);

            if (ret != CACHE_OK)
                return ret;
            break;
        case VCENC_YUV420_SEMIPLANAR_8BIT_TILE_4_4:
        case VCENC_YUV420_SEMIPLANAR_VU_8BIT_TILE_4_4:
        case VCENC_YUV420_PLANAR_10BIT_P010_TILE_4_4:
            memset(&channel_cfg, 0, sizeof(CWLChannelConf_t));

            if (vcenc_instance->preProcess.inputFormat == VCENC_YUV420_SEMIPLANAR_8BIT_TILE_4_4 ||
                vcenc_instance->preProcess.inputFormat == VCENC_YUV420_SEMIPLANAR_VU_8BIT_TILE_4_4)
                byte_per_compt = 1;
            else if (vcenc_instance->preProcess.inputFormat ==
                     VCENC_YUV420_PLANAR_10BIT_P010_TILE_4_4)
                byte_per_compt = 2;

            channel_cfg.trace_start_addr = BaseInCb;
            channel_cfg.trace_end_addr =
                asic->regs.input_chroma_stride *
                ((vcenc_instance->preProcess.lumHeightSrc[tileId] / 2) + 3) / 4;

            channel_cfg.start_addr = asic->regs.inputCbBase >> 4;
            channel_cfg.end_addr =
                (asic->regs.inputCbBase +
                 (asic->regs.input_chroma_stride *
                  ((vcenc_instance->preProcess.lumHeightSrc[tileId] / 2) + 3) / 4)) >>
                4;

            ret = EnableCacheChannel(&vcenc_instance->cache, &vcenc_instance->channel_idx,
                                     &channel_cfg, ENCODER_VC8000E, dir);

            if (ret != CACHE_OK)
                return ret;
            break;
        }
    }

#endif

#ifdef SUPPORT_REF_CACHE
    byte_per_compt = vcenc_instance->asic.regs.P010RefEnable + 1;

#ifdef RECON_REF_1KB_BURST_RW
    u32 reference_luma_size = asic->regs.ref_frame_stride *
                              (((vcenc_instance->height + 63) & (~63)) / (8 / byte_per_compt));
#else
    u32 reference_luma_size =
        asic->regs.ref_frame_stride * ((vcenc_instance->height + 63) & (~63)) / 4;
#endif

    u32 reference_4n_size =
        asic->regs.ref_ds_luma_stride * ((vcenc_instance->height + 63) & (~63)) / 4 / 4;
    u32 reference_chroma_size = reference_luma_size / 2;

    /*3. reference L0 luma data*/
    if (pic->sliceInst->type != I_SLICE) {
        memset(&channel_cfg, 0, sizeof(CWLChannelConf_t));

        j = 0;
        while ((i32)asic->regs.pRefPic_recon_l0[0][0] != refLum[j++])
            ASSERT(j < NUM_REF_BUF);

        channel_cfg.trace_start_addr = traceRefLu[j];
        channel_cfg.trace_end_addr = ((vcenc_instance->width + alignment) & (~alignment)) *
                                     ((vcenc_instance->height + 63) & (~63));

        channel_cfg.start_addr = asic->regs.pRefPic_recon_l0[0][0] >> 4;
        channel_cfg.end_addr = (asic->regs.pRefPic_recon_l0[0][0] + reference_luma_size) >> 4;

        ret = EnableCacheChannel(&vcenc_instance->cache, &vcenc_instance->channel_idx, &channel_cfg,
                                 ENCODER_VC8000E, dir);

        if (ret != CACHE_OK)
            return ret;
    }

    /*4. reference L0 chroma data*/
    if (pic->sliceInst->type != I_SLICE) {
        memset(&channel_cfg, 0, sizeof(CWLChannelConf_t));

        j = 0;
        while ((i32)asic->regs.pRefPic_recon_l0[1][0] != refChr[j++])
            ASSERT(j < NUM_REF_BUF);

        channel_cfg.trace_start_addr = traceRefChr[j];
        channel_cfg.trace_end_addr = ((vcenc_instance->width + alignment) & (~alignment)) *
                                     ((vcenc_instance->height + 63) & (~63)) / 2;

        channel_cfg.start_addr = asic->regs.pRefPic_recon_l0[1][0] >> 4;
        channel_cfg.end_addr = (asic->regs.pRefPic_recon_l0[1][0] + reference_chroma_size) >> 4;

        ret = EnableCacheChannel(&vcenc_instance->cache, &vcenc_instance->channel_idx, &channel_cfg,
                                 ENCODER_VC8000E, dir);

        if (ret != CACHE_OK)
            return ret;
    }

    /*5. reference L1 luma data*/
    if (pic->sliceInst->type == B_SLICE) {
        memset(&channel_cfg, 0, sizeof(CWLChannelConf_t));

        j = 0;
        while ((i32)asic->regs.pRefPic_recon_l1[0][0] != refLum[j++])
            ASSERT(j < NUM_REF_BUF);

        channel_cfg.trace_start_addr = traceRefLu[j];
        channel_cfg.trace_end_addr = ((vcenc_instance->width + alignment) & (~alignment)) *
                                     ((vcenc_instance->height + 63) & (~63));

        channel_cfg.start_addr = asic->regs.pRefPic_recon_l1[0][0] >> 4;
        channel_cfg.end_addr = (asic->regs.pRefPic_recon_l1[0][0] + reference_luma_size) >> 4;

        ret = EnableCacheChannel(&vcenc_instance->cache, &vcenc_instance->channel_idx, &channel_cfg,
                                 ENCODER_VC8000E, dir);

        if (ret != CACHE_OK)
            return ret;
    }

    /*6. reference L1 chroma data*/
    if (pic->sliceInst->type == B_SLICE) {
        memset(&channel_cfg, 0, sizeof(CWLChannelConf_t));

        j = 0;
        while ((i32)asic->regs.pRefPic_recon_l1[1][0] != refChr[j++])
            ASSERT(j < NUM_REF_BUF);

        channel_cfg.trace_start_addr = traceRefChr[j];
        channel_cfg.trace_end_addr = ((vcenc_instance->width + alignment) & (~alignment)) *
                                     ((vcenc_instance->height + 63) & (~63)) / 2;

        channel_cfg.start_addr = asic->regs.pRefPic_recon_l1[1][0] >> 4;
        channel_cfg.end_addr = (asic->regs.pRefPic_recon_l1[1][0] + reference_chroma_size) >> 4;

        ret = EnableCacheChannel(&vcenc_instance->cache, &vcenc_instance->channel_idx, &channel_cfg,
                                 ENCODER_VC8000E, dir);

        if (ret != CACHE_OK)
            return ret;
    }

    /*7. reference L0 4N*/
    if (pic->sliceInst->type != I_SLICE) {
        memset(&channel_cfg, 0, sizeof(CWLChannelConf_t));

        j = 0;
        while ((i32)asic->regs.pRefPic_recon_l0_4n[0] != ref4nLum[j++])
            ASSERT(j < NUM_REF_BUF);

        channel_cfg.trace_start_addr = traceRef4N[j];
        channel_cfg.trace_end_addr =
            ((vcenc_instance->height / 4 + alignment) & (~alignment)) * vcenc_instance->height / 4;

        channel_cfg.start_addr = asic->regs.pRefPic_recon_l0_4n[0] >> 4;
        channel_cfg.end_addr = (asic->regs.pRefPic_recon_l0_4n[0] + reference_4n_size) >> 4;

        ret = EnableCacheChannel(&vcenc_instance->cache, &vcenc_instance->channel_idx, &channel_cfg,
                                 ENCODER_VC8000E, dir);

        if (ret != CACHE_OK)
            return ret;
    }

    /*8. reference L1 4N*/
    if (pic->sliceInst->type == B_SLICE) {
        memset(&channel_cfg, 0, sizeof(CWLChannelConf_t));

        j = 0;
        while ((i32)asic->regs.pRefPic_recon_l1_4n[0] != ref4nLum[j++])
            ASSERT(j < NUM_REF_BUF);

        channel_cfg.trace_start_addr = traceRef4N[j];
        channel_cfg.trace_end_addr =
            ((vcenc_instance->height / 4 + alignment) & (~alignment)) * vcenc_instance->height / 4;

        channel_cfg.start_addr = asic->regs.pRefPic_recon_l1_4n[0] >> 4;
        channel_cfg.end_addr = (asic->regs.pRefPic_recon_l1_4n[0] + reference_4n_size) >> 4;

        ret = EnableCacheChannel(&vcenc_instance->cache, &vcenc_instance->channel_idx, &channel_cfg,
                                 ENCODER_VC8000E, dir);

        if (ret != CACHE_OK)
            return ret;
    }
#endif

#if 0
  /*cache write channel config*/
  dir = CACHE_WR;
  u32 alignment_size = IS_H264(vcenc_instance->codecFormat)?16:8;
  u32 encoding_hight = (vcenc_instance->height+alignment_size-1)&(~(alignment_size-1));
  u32 cb_size = IS_H264(vcenc_instance->codecFormat)?16:64;
  byte_per_compt = vcenc_instance->asic.regs.P010RefEnable+1;

  /*1. Luma recon write*/
  j = 0;
  while ((i32)asic->regs.reconLumBase != refLum[j++]) ASSERT(j < NUM_REF_BUF);

  channel_cfg.trace_start_addr = traceRefLu[j];

  channel_cfg.start_addr = asic->regs.reconLumBase>>4;

  channel_cfg.line_size = asic->regs.picWidth*byte_per_compt*4;
  channel_cfg.line_stride = STRIDE(channel_cfg.line_size,alignment)>>4;
  channel_cfg.line_cnt = encoding_hight/4;
  channel_cfg.stripe_e = 0;
  channel_cfg.max_h = (cb_size+8)/4;
  channel_cfg.pad_e = 1;
  channel_cfg.rfc_e = 0;
  ret = EnableCacheChannel(&vcenc_instance->cache,&vcenc_instance->channel_idx,&channel_cfg,ENCODER_VC8000E,dir);

  if (ret != CACHE_OK)
   return ret;

  /*2. chroma recon write*/
  j = 0;
  while ((i32)asic->regs.reconCbBase != refChr[j++]) ASSERT(j < NUM_REF_BUF);

  channel_cfg.trace_start_addr = traceRefChr[j];

  channel_cfg.start_addr = asic->regs.reconCbBase>>4;

  channel_cfg.line_size = asic->regs.picWidth*byte_per_compt*4;
  channel_cfg.line_stride = STRIDE(channel_cfg.line_size,alignment)>>4;
  channel_cfg.line_cnt = encoding_hight/4/2;
  channel_cfg.stripe_e = 0;
  channel_cfg.max_h =  (cb_size/2+8)/4;
  channel_cfg.pad_e = 1;
  channel_cfg.rfc_e = 0;
  ret = EnableCacheChannel(&vcenc_instance->cache,&vcenc_instance->channel_idx,&channel_cfg,ENCODER_VC8000E,dir);

  if (ret != CACHE_OK)
    return ret;

  /*3. 4N Luma recon write*/
  j = 0;
  while ((i32)asic->regs.reconL4nBase != ref4nLum[j++]) ASSERT(j < NUM_REF_BUF);

  channel_cfg.trace_start_addr = traceRef4N[j];

  channel_cfg.start_addr = asic->regs.reconL4nBase>>4;

  channel_cfg.line_size = ((asic->regs.picWidth + 15) / 16) * 16 * byte_per_compt;
  channel_cfg.line_stride = STRIDE(channel_cfg.line_size,alignment)>>4;
  channel_cfg.line_cnt = ((vcenc_instance->height + 15) / 16);
  channel_cfg.stripe_e = 0;
  channel_cfg.max_h = (16+4)/4;
  channel_cfg.pad_e = 1;
  channel_cfg.rfc_e = 0;
  ret = EnableCacheChannel(&vcenc_instance->cache,&vcenc_instance->channel_idx,&channel_cfg,ENCODER_VC8000E,dir);

  if (ret != CACHE_OK)
    return ret;

#if 0
  if (pic->sliceSize == 0)
  {
    channel_cfg.trace_start_addr = BaseStream;

    channel_cfg.start_addr = asic->regs.outputStrmBase>>4;

    channel_cfg.line_size = asic->regs.outputStrmSize;
    channel_cfg.line_stride = 0xFFFF;
    channel_cfg.line_cnt = 1;
    channel_cfg.stripe_e = 0;
    channel_cfg.max_h = 1;
    ret = EnableCacheChannel(&vcenc_instance->cache,&vcenc_instance->channel_idx,&channel_cfg,ENCODER_VC8000E,dir);

    if (ret != CACHE_OK)
    return ret;
  }
#endif
#endif
    return CACHE_OK;
}

VCEncRet EnableCache(struct vcenc_instance *vcenc_instance, asicData_s *asic,
                     struct sw_picture *pic, u32 tileId)
{
#ifdef SUPPORT_CACHE

    u32 i, val;
    CWLRegOut cacheRegInfo;

    if (CACHE_OK != VCEncEnableCache(vcenc_instance, asic, pic, tileId)) {
        printf("Encoder enable cache failed!!\n");
        return VCENC_ERROR;
    }

    memset(&cacheRegInfo, 0, sizeof(cacheRegInfo));

    GenCacheChannelModeReg(&vcenc_instance->cache, CACHE_RD, &cacheRegInfo, CACHE_ENABLE);

    for (i = 0; i < CWL_CACHE_REG_NUM_MAX; i++) {
        if (0 != cacheRegInfo.cacheRegs[i].flag && i != 1) {
            EWLWriteRegbyClientType(asic->ewl, cacheRegInfo.cacheRegs[i].offset,
                                    cacheRegInfo.cacheRegs[i].val, EWL_CLIENT_TYPE_L2CACHE);
        }
    }

    EWLWriteRegbyClientType(asic->ewl, cacheRegInfo.cacheRegs[1].offset,
                            cacheRegInfo.cacheRegs[1].val,
                            EWL_CLIENT_TYPE_L2CACHE); //enable
    return VCENC_OK;

#else //No Cache

    return VCENC_OK;

#endif
}

void DisableCache(const void *ewl, void *cache_data)
{
#ifdef SUPPORT_CACHE
    u32 i;
    CacheData_t *cacheData = (CacheData_t *)cache_data;
    CWLRegOut cacheRegInfo;
    memset(&cacheRegInfo, 0, sizeof(cacheRegInfo));

    GenCacheChannelModeReg(cacheData->cache, CACHE_RD, &cacheRegInfo, CACHE_DISABLE);

    for (i = 0; i < cacheRegInfo.regNums; i++) {
        if (0 != cacheRegInfo.cacheRegs[i].flag) {
            EWLWriteRegbyClientType(ewl, cacheRegInfo.cacheRegs[i].offset,
                                    cacheRegInfo.cacheRegs[i].val, EWL_CLIENT_TYPE_L2CACHE);
        }
    }

#endif
}

#endif
