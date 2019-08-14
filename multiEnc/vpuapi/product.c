/*
* Copyright (c) 2019 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in below
* which is part of this source code package.
*
* Description:
*/

// Copyright (C) 2019 Amlogic, Inc. All rights reserved.
//
// All information contained herein is Amlogic confidential.
//
// This software is provided to you pursuant to Software License
// Agreement (SLA) with Amlogic Inc ("Amlogic"). This software may be
// used only in accordance with the terms of this agreement.
//
// Redistribution and use in source and binary forms, with or without
// modification is strictly prohibited without prior written permission
// from Amlogic.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#include "product.h"
#include "enc_driver.h"
#include "vdi_osal.h"

VpuAttr g_VpuCoreAttributes[MAX_NUM_VPU_CORE];

static Int32 s_ProductIds[MAX_NUM_VPU_CORE] = {
    PRODUCT_ID_NONE,
};

typedef struct FrameBufInfoStruct {
    Uint32 unitSizeHorLuma;
    Uint32 sizeLuma;
    Uint32 sizeChroma;
    BOOL   fieldMap;
} FrameBufInfo;


Uint32 ProductVpuScan(Uint32 coreIdx)
{
    Uint32  i, productId;
    Uint32 foundProducts = 0;

    /* Already scanned */
    if (s_ProductIds[coreIdx] != PRODUCT_ID_NONE) 
        return 1;

    for (i=0; i<MAX_NUM_VPU_CORE; i++) {
            productId = VpVpuGetProductId(i);
        if (productId != PRODUCT_ID_NONE) {
            s_ProductIds[i] = productId;
            foundProducts++;
        }
    }

    return (foundProducts >= 1);
}


Int32 ProductVpuGetId(Uint32 coreIdx)
{
    return s_ProductIds[coreIdx];
}

RetCode ProductVpuGetVersion(
    Uint32  coreIdx, 
    Uint32* versionInfo, 
    Uint32* revision 
    )
{
    Int32   productId = s_ProductIds[coreIdx];
    RetCode ret = RETCODE_SUCCESS;

    switch (productId) {
    case PRODUCT_ID_512:
    case PRODUCT_ID_520:
    case PRODUCT_ID_515:
    case PRODUCT_ID_525:
    case PRODUCT_ID_521:
    case PRODUCT_ID_511:
        ret = Vp5VpuGetVersion(coreIdx, versionInfo, revision);
        break;
    default:
        ret = RETCODE_NOT_FOUND_VPU_DEVICE;
    }

    return ret;
}

RetCode ProductVpuGetProductInfo(
    Uint32  coreIdx, 
    ProductInfo* productInfo 
    )
{
    Int32   productId = s_ProductIds[coreIdx];
    RetCode ret = RETCODE_SUCCESS;

    switch (productId) {
    case PRODUCT_ID_512:
    case PRODUCT_ID_515:
    case PRODUCT_ID_520:
    case PRODUCT_ID_525:
    case PRODUCT_ID_521:
    case PRODUCT_ID_511:
        ret = Vp5VpuGetProductInfo(coreIdx, productInfo);
        break;
    default:
        ret = RETCODE_NOT_FOUND_VPU_DEVICE;
    }

    return ret;
}

RetCode ProductVpuInit(Uint32 coreIdx, void* firmware, Uint32 size)
{
    RetCode ret = RETCODE_SUCCESS; 
    int     productId;

    productId  = s_ProductIds[coreIdx];

    switch (productId) {
    case PRODUCT_ID_512:
    case PRODUCT_ID_515:
    case PRODUCT_ID_520:
    case PRODUCT_ID_525:
    case PRODUCT_ID_521:
    case PRODUCT_ID_511:
        ret = Vp5VpuInit(coreIdx, firmware, size);
        break;
    default:
        ret = RETCODE_NOT_FOUND_VPU_DEVICE;
    }

    return ret;
}

RetCode ProductVpuReInit(Uint32 coreIdx, void* firmware, Uint32 size)
{
    RetCode ret = RETCODE_SUCCESS; 
    int     productId;

    productId  = s_ProductIds[coreIdx];

    switch (productId) {
    case PRODUCT_ID_512:
    case PRODUCT_ID_515:
    case PRODUCT_ID_520:
    case PRODUCT_ID_525:
    case PRODUCT_ID_521:
    case PRODUCT_ID_511:
        ret = Vp5VpuReInit(coreIdx, firmware, size);
        break;
    default:
        ret = RETCODE_NOT_FOUND_VPU_DEVICE;
    }

    return ret;
}

Uint32 ProductVpuIsInit(Uint32 coreIdx)
{
    Uint32  pc = 0;
    int     productId;

    productId  = s_ProductIds[coreIdx];

    if (productId == PRODUCT_ID_NONE) {
        ProductVpuScan(coreIdx);
        productId  = s_ProductIds[coreIdx];
    }

    switch (productId) {
    case PRODUCT_ID_512:
    case PRODUCT_ID_515:
    case PRODUCT_ID_520:
    case PRODUCT_ID_525:
    case PRODUCT_ID_521:
    case PRODUCT_ID_511:
        pc = Vp5VpuIsInit(coreIdx);
        break;
    }

    return pc;
}

Int32 ProductVpuIsBusy(Uint32 coreIdx)
{
    Int32  busy;
    int    productId;

    productId = s_ProductIds[coreIdx];

    switch (productId) {
    case PRODUCT_ID_512:
    case PRODUCT_ID_515:
    case PRODUCT_ID_520:
    case PRODUCT_ID_525:
    case PRODUCT_ID_521:
    case PRODUCT_ID_511:
        busy = Vp5VpuIsBusy(coreIdx);
        break;
    default:
        busy = 0;
        break;
    }

    return busy;
}

Int32 ProductVpuWaitInterrupt(CodecInst *instance, Int32 timeout)
{
    int     productId;
    int     flag = -1;

    productId = s_ProductIds[instance->coreIdx];

    switch (productId) {
    case PRODUCT_ID_512:
    case PRODUCT_ID_515:
    case PRODUCT_ID_520:
    case PRODUCT_ID_525:
    case PRODUCT_ID_521:
    case PRODUCT_ID_511:
        flag = Vp5VpuWaitInterrupt(instance, timeout, FALSE);
        break;
    default:
        flag = -1;
        break;
    }

    return flag;
}

RetCode ProductVpuReset(Uint32 coreIdx, SWResetMode resetMode)
{
    int     productId;
    RetCode ret = RETCODE_SUCCESS;

    productId = s_ProductIds[coreIdx];

    switch (productId) {
    case PRODUCT_ID_512:
    case PRODUCT_ID_515:
    case PRODUCT_ID_520:
    case PRODUCT_ID_525:
    case PRODUCT_ID_521:
    case PRODUCT_ID_511:
        ret = Vp5VpuReset(coreIdx, resetMode);
        break;
    default:
        ret = RETCODE_NOT_FOUND_VPU_DEVICE;
        break;
    }

    return ret;
}

RetCode ProductVpuSleepWake(Uint32 coreIdx, int iSleepWake, const Uint16* code, Uint32 size)
{
    int     productId;
    RetCode ret = RETCODE_NOT_FOUND_VPU_DEVICE;

    productId = s_ProductIds[coreIdx];

    switch (productId) {
    case PRODUCT_ID_512:
    case PRODUCT_ID_515:
    case PRODUCT_ID_520:
    case PRODUCT_ID_525:
    case PRODUCT_ID_521:
    case PRODUCT_ID_511:
        ret = Vp5VpuSleepWake(coreIdx, iSleepWake, (void*)code, size, FALSE);
        break;
    }

    return ret;
}
RetCode ProductVpuClearInterrupt(Uint32 coreIdx, Uint32 flags)
{
    int     productId;
    RetCode ret = RETCODE_NOT_FOUND_VPU_DEVICE;

    productId = s_ProductIds[coreIdx];

    switch (productId) {
    case PRODUCT_ID_512:
    case PRODUCT_ID_515:
    case PRODUCT_ID_520:
    case PRODUCT_ID_525:
    case PRODUCT_ID_521:
    case PRODUCT_ID_511:
        ret = Vp5VpuClearInterrupt(coreIdx, flags);
        break;
    }

    return ret;
}

RetCode ProductVpuEncUpdateBitstreamBuffer(CodecInst* instance)
{
    Int32   productId;
    Uint32  coreIdx;
    RetCode ret = RETCODE_SUCCESS;

    coreIdx   = instance->coreIdx;
    productId = s_ProductIds[coreIdx];

    switch (productId) {
    case PRODUCT_ID_520:
    case PRODUCT_ID_525:
    case PRODUCT_ID_521:
        ret = Vp5VpuEncUpdateBS(instance);
        break;
    default:
        ret = RETCODE_NOT_FOUND_VPU_DEVICE;
    }

    return ret;
}
RetCode ProductVpuEncGetRdWrPtr(CodecInst* instance, PhysicalAddress* rdPtr, PhysicalAddress* wrPtr)
{
    Int32   productId;
    Uint32  coreIdx;
    EncInfo*    pEncInfo;
    RetCode ret = RETCODE_SUCCESS;
    pEncInfo = VPU_HANDLE_TO_ENCINFO(instance);

    coreIdx   = instance->coreIdx;
    productId = s_ProductIds[coreIdx];

    switch (productId) {
    case PRODUCT_ID_520:
    case PRODUCT_ID_525:
    case PRODUCT_ID_521:
        ret = Vp5VpuEncGetRdWrPtr(instance, rdPtr, wrPtr);
        if (ret != RETCODE_SUCCESS) {
            *rdPtr = pEncInfo->streamRdPtr;
            *wrPtr = pEncInfo->streamWrPtr;
        }
        else {
            pEncInfo->streamRdPtr = *rdPtr;
            pEncInfo->streamWrPtr = *wrPtr;
        }
        break;
    default:
        *wrPtr = pEncInfo->streamWrPtr;
        *rdPtr = pEncInfo->streamRdPtr;
        break;
    }

    return ret;

}

RetCode ProductVpuEncBuildUpOpenParam(CodecInst* pCodec, EncOpenParam* param)
{
    Int32   productId;
    Uint32  coreIdx;
    RetCode ret = RETCODE_NOT_SUPPORTED_FEATURE;

    coreIdx   = pCodec->coreIdx;
    productId = s_ProductIds[coreIdx];

    switch (productId) {
    case PRODUCT_ID_520:
    case PRODUCT_ID_525:
    case PRODUCT_ID_521:
        ret = Vp5VpuBuildUpEncParam(pCodec, param);
        break;
    default:
        ret = RETCODE_NOT_SUPPORTED_FEATURE;
    }

    return ret;
}

/**
 * \param   stride          stride of framebuffer in pixel.
 */
RetCode ProductVpuAllocateFramebuffer(
    CodecInst* inst, FrameBuffer* fbArr, TiledMapType mapType, Int32 num, 
    Int32 stride, Int32 height, FrameBufferFormat format, 
    BOOL cbcrInterleave, BOOL nv21, Int32 endian, 
    vpu_buffer_t* vb, Int32 gdiIndex,
    FramebufferAllocType fbType)
{
    Int32           i;
    vpu_buffer_t    vbFrame;
    FrameBufInfo    fbInfo;
    DecInfo*        pDecInfo = &inst->CodecInfo->decInfo;
    EncInfo*        pEncInfo = &inst->CodecInfo->encInfo;
    // Variables for TILED_FRAME/FILED_MB_RASTER
    Uint32          sizeLuma;
    Uint32          sizeChroma;
    ProductId       productId     = (ProductId)inst->productId;
    RetCode         ret           = RETCODE_SUCCESS;
    
    osal_memset((void*)&vbFrame, 0x00, sizeof(vpu_buffer_t));
    osal_memset((void*)&fbInfo,  0x00, sizeof(FrameBufInfo));

    DRAMConfig* bufferConfig = NULL;
    sizeLuma   = CalcLumaSize(inst->productId, stride, height, format, cbcrInterleave, mapType, bufferConfig);
    sizeChroma = CalcChromaSize(inst->productId, stride, height, format, cbcrInterleave, mapType, bufferConfig);

    // Framebuffer common informations
    for (i=0; i<num; i++) {
        if (fbArr[i].updateFbInfo == TRUE ) {
            fbArr[i].updateFbInfo = FALSE;
            fbArr[i].myIndex        = i+gdiIndex;
            fbArr[i].stride         = stride;
            fbArr[i].height         = height;
            fbArr[i].mapType        = mapType;
            fbArr[i].format         = format;
            fbArr[i].cbcrInterleave = (mapType == COMPRESSED_FRAME_MAP ? TRUE : cbcrInterleave);
            fbArr[i].nv21           = nv21;
            fbArr[i].endian         = (mapType == COMPRESSED_FRAME_MAP ? VDI_128BIT_LITTLE_ENDIAN : endian);
            fbArr[i].lumaBitDepth   = pDecInfo->initialInfo.lumaBitdepth;
            fbArr[i].chromaBitDepth = pDecInfo->initialInfo.chromaBitdepth;
            fbArr[i].sourceLBurstEn = FALSE;
            if(inst->codecMode == W_HEVC_ENC || inst->codecMode == W_SVAC_ENC || inst->codecMode == W_AVC_ENC) {
                fbArr[i].endian         = (mapType == COMPRESSED_FRAME_MAP ? VDI_128BIT_LITTLE_ENDIAN : endian);
                fbArr[i].lumaBitDepth   = pEncInfo->openParam.EncStdParam.vpParam.internalBitDepth;
                fbArr[i].chromaBitDepth = pEncInfo->openParam.EncStdParam.vpParam.internalBitDepth;
            }
        }
    }

    switch (mapType) {
    case LINEAR_FRAME_MAP:
    case LINEAR_FIELD_MAP:
    case COMPRESSED_FRAME_MAP:
        ret = UpdateFrameBufferAddr(mapType, fbArr, num, sizeLuma, sizeChroma);
        if (ret != RETCODE_SUCCESS)
            break;
        break;

    default:
        /* Tiled map */
        VLOG(ERR, "shall not reach Tile map vb %p fbType %d \n",vb, fbType);
        break;
    }
    for (i=0; i<num; i++) {
        if(inst->codecMode == W_HEVC_ENC || inst->codecMode == W_SVAC_ENC || inst->codecMode == W_AVC_ENC) {
            if (gdiIndex != 0) {        // FB_TYPE_PPU
                APIDPRINT("SOURCE FB[%02d] Y(0x%08x), Cb(0x%08x), Cr(0x%08x)\n", i, fbArr[i].bufY, fbArr[i].bufCb, fbArr[i].bufCr);
            }
        }
    }
    return ret;
}

RetCode ProductVpuRegisterFramebuffer(CodecInst* instance)
{
    RetCode         ret = RETCODE_FAILURE;
    FrameBuffer*    fb;
    Int32           gdiIndex = 0;
    EncInfo*        pEncInfo = &instance->CodecInfo->encInfo;

    switch (instance->productId) {
    default:
        {
            // ENCODER
            if (pEncInfo->mapType != COMPRESSED_FRAME_MAP)
                return RETCODE_NOT_SUPPORTED_FEATURE;

            fb = pEncInfo->frameBufPool;

            if (instance->codecMode == W_SVAC_ENC && pEncInfo->openParam.EncStdParam.vpParam.svcEnable == TRUE) {  // for BL
                gdiIndex = pEncInfo->numFrameBuffers;
                ret = Vp5VpuEncRegisterFramebuffer(instance, &fb[gdiIndex], COMPRESSED_FRAME_MAP_SVAC_SVC_BL, pEncInfo->numFrameBuffers);
            }
            gdiIndex = 0;   // for EL
            ret = Vp5VpuEncRegisterFramebuffer(instance, &fb[gdiIndex], COMPRESSED_FRAME_MAP, pEncInfo->numFrameBuffers);

            if (ret != RETCODE_SUCCESS)
                return ret;
        }
        break;
    }
    return ret;
}
Int32 ProductCalculateFrameBufSize(Int32 productId, Int32 stride, Int32 height, TiledMapType mapType, FrameBufferFormat format, BOOL interleave, DRAMConfig* pDramCfg)
{
    Int32 size_dpb_lum, size_dpb_chr, size_dpb_all;

    size_dpb_lum = CalcLumaSize(productId, stride, height, format, interleave, mapType, pDramCfg);
    size_dpb_chr = CalcChromaSize(productId, stride, height, format, interleave, mapType, pDramCfg);
    size_dpb_all = size_dpb_lum + size_dpb_chr*2;

    return size_dpb_all;
}


Int32 ProductCalculateAuxBufferSize(AUX_BUF_TYPE type, CodStd codStd, Int32 width, Int32 height)
{
    Int32 size = 0;

    switch (type) {
    case AUX_BUF_TYPE_MVCOL:
        if (codStd == STD_AVC || codStd == STD_VC1 || codStd == STD_MPEG4 || codStd == STD_H263 || codStd == STD_RV || codStd == STD_AVS ) {
            size =  VPU_ALIGN32(width)*VPU_ALIGN32(height);
            size = (size*3)/2;
            size = (size+4)/5;
            size = ((size+7)/8)*8;
        } 
        else if (codStd == STD_HEVC) {
            size = VP5_DEC_HEVC_MVCOL_BUF_SIZE(width, height);
        }
        else if (codStd == STD_VP9) {
            size = VP5_DEC_VP9_MVCOL_BUF_SIZE(width, height);
        }
        else if (codStd == STD_AVS2) {
            size = VP5_DEC_AVS2_MVCOL_BUF_SIZE(width, height);
        }
        else {
            size = 0;
        }
        break;
    case AUX_BUF_TYPE_FBC_Y_OFFSET:
        size  = VP5_FBC_LUMA_TABLE_SIZE(width, height);
        break;
    case AUX_BUF_TYPE_FBC_C_OFFSET:
        size  = VP5_FBC_CHROMA_TABLE_SIZE(width, height);
        break;
    }

    return size;
}

RetCode ProductVpuGetBandwidth(CodecInst* instance, VPUBWData* data)
{
    if (data == 0) {
        return RETCODE_INVALID_PARAM;
    }                

    if (instance->productId < PRODUCT_ID_520)
        return RETCODE_INVALID_COMMAND;

    return Vp5VpuGetBwReport(instance, data);
}



/************************************************************************/
/* ENCODER                                                              */
/************************************************************************/
RetCode ProductCheckEncOpenParam(EncOpenParam* pop)
{
    Int32       coreIdx;
    Int32       picWidth;
    Int32       picHeight;
    Int32       productId;
    VpuAttr*    pAttr;

    if (pop == 0) 
        return RETCODE_INVALID_PARAM;

    if (pop->coreIdx > MAX_NUM_VPU_CORE) 
        return RETCODE_INVALID_PARAM;

    coreIdx   = pop->coreIdx;
    picWidth  = pop->picWidth;
    picHeight = pop->picHeight;    
    productId = s_ProductIds[coreIdx];
    pAttr     = &g_VpuCoreAttributes[coreIdx];

    if ((pAttr->supportEncoders&(1<<pop->bitstreamFormat)) == 0) 
        return RETCODE_NOT_SUPPORTED_FEATURE;

    if (pop->ringBufferEnable == TRUE) {
        if (pop->bitstreamBuffer % 8) {
            return RETCODE_INVALID_PARAM;
        }

        if (productId == PRODUCT_ID_520 || productId == PRODUCT_ID_525 || productId == PRODUCT_ID_521) {
            if (pop->bitstreamBuffer % 16)
                return RETCODE_INVALID_PARAM;
        }

        if (productId == PRODUCT_ID_520 || productId == PRODUCT_ID_525 || productId == PRODUCT_ID_521) {
            if (pop->bitstreamBufferSize < (1024*64)) {
                return RETCODE_INVALID_PARAM;
            }
        }

        if (pop->bitstreamBufferSize % 1024 || pop->bitstreamBufferSize < 1024) 
            return RETCODE_INVALID_PARAM;
    }

    if (pop->frameRateInfo == 0) 
        return RETCODE_INVALID_PARAM;

    if (pop->bitstreamFormat == STD_AVC) {
        if (productId == PRODUCT_ID_980) {
            if (pop->bitRate > 524288 || pop->bitRate < 0) 
                return RETCODE_INVALID_PARAM;
        }
    }
    else if (pop->bitstreamFormat == STD_HEVC || pop->bitstreamFormat == STD_SVAC) {
        if (productId == PRODUCT_ID_520 || productId == PRODUCT_ID_525 || productId == PRODUCT_ID_521) {
            if (pop->bitRate > 700000000 || pop->bitRate < 0) 
                return RETCODE_INVALID_PARAM;
        }
    }
    else {
        if (pop->bitRate > 32767 || pop->bitRate < 0) 
            return RETCODE_INVALID_PARAM;
    }

    if (pop->frameSkipDisable != 0 && pop->frameSkipDisable != 1)
        return RETCODE_INVALID_PARAM;

    if (pop->sliceMode.sliceMode != 0 && pop->sliceMode.sliceMode != 1) 
        return RETCODE_INVALID_PARAM;

    if (pop->sliceMode.sliceMode == 1) {
        if (pop->sliceMode.sliceSizeMode != 0 && pop->sliceMode.sliceSizeMode != 1) {
            return RETCODE_INVALID_PARAM;
        }
        if (pop->sliceMode.sliceSizeMode == 1 && pop->sliceMode.sliceSize == 0 ) {
            return RETCODE_INVALID_PARAM;
        }
    }

    if (pop->intraRefreshNum < 0) 
        return RETCODE_INVALID_PARAM;

    if (pop->MEUseZeroPmv != 0 && pop->MEUseZeroPmv != 1) 
        return RETCODE_INVALID_PARAM;

    if (pop->intraCostWeight < 0 || pop->intraCostWeight >= 65535) 
        return RETCODE_INVALID_PARAM;

    if (productId == PRODUCT_ID_980) {
        if (pop->MESearchRangeX < 0 || pop->MESearchRangeX > 4) {
            return RETCODE_INVALID_PARAM;
        }
        if (pop->MESearchRangeY < 0 || pop->MESearchRangeY > 3) {
            return RETCODE_INVALID_PARAM;
        }        
    }
    else {
        if (pop->MESearchRange < 0 || pop->MESearchRange >= 4) 
            return RETCODE_INVALID_PARAM;
    }

    if (pop->bitstreamFormat == STD_MPEG4) {
        EncMp4Param * param = &pop->EncStdParam.mp4Param;
        if (param->mp4DataPartitionEnable != 0 && param->mp4DataPartitionEnable != 1) {
            return RETCODE_INVALID_PARAM;
        }
        if (param->mp4DataPartitionEnable == 1) {
            if (param->mp4ReversibleVlcEnable != 0 && param->mp4ReversibleVlcEnable != 1) {
                return RETCODE_INVALID_PARAM;
            }
        }
        if (param->mp4IntraDcVlcThr < 0 || 7 < param->mp4IntraDcVlcThr) {
            return RETCODE_INVALID_PARAM;
        }

        if (picWidth < MIN_ENC_PIC_WIDTH || picWidth > MAX_ENC_PIC_WIDTH ) {
            return RETCODE_INVALID_PARAM;
        }

        if (picHeight < MIN_ENC_PIC_HEIGHT) {
            return RETCODE_INVALID_PARAM;
        }
    }
    else if (pop->bitstreamFormat == STD_H263) {
        EncH263Param * param = &pop->EncStdParam.h263Param;
        Uint32 frameRateInc, frameRateRes;

        if (param->h263AnnexJEnable != 0 && param->h263AnnexJEnable != 1) {
            return RETCODE_INVALID_PARAM;
        }
        if (param->h263AnnexKEnable != 0 && param->h263AnnexKEnable != 1) {
            return RETCODE_INVALID_PARAM;
        }
        if (param->h263AnnexTEnable != 0 && param->h263AnnexTEnable != 1) {
            return RETCODE_INVALID_PARAM;
        }

        if (picWidth < MIN_ENC_PIC_WIDTH || picWidth > MAX_ENC_PIC_WIDTH ) {
            return RETCODE_INVALID_PARAM;
        }
        if (picHeight < MIN_ENC_PIC_HEIGHT) {
            return RETCODE_INVALID_PARAM;
        }

        frameRateInc = ((pop->frameRateInfo>>16) &0xFFFF) + 1;
        frameRateRes = pop->frameRateInfo & 0xFFFF;

        if ((frameRateRes/frameRateInc) <15) {
            return RETCODE_INVALID_PARAM;
        }
    }
    else if (pop->bitstreamFormat == STD_AVC && productId != PRODUCT_ID_521) {

        EncAvcParam* param     = &pop->EncStdParam.avcParam;

        if (param->constrainedIntraPredFlag != 0 && param->constrainedIntraPredFlag != 1) 
            return RETCODE_INVALID_PARAM;
        if (param->disableDeblk != 0 && param->disableDeblk != 1 && param->disableDeblk != 2) 
            return RETCODE_INVALID_PARAM;
        if (param->deblkFilterOffsetAlpha < -6 || 6 < param->deblkFilterOffsetAlpha) 
            return RETCODE_INVALID_PARAM;
        if (param->deblkFilterOffsetBeta < -6 || 6 < param->deblkFilterOffsetBeta) 
            return RETCODE_INVALID_PARAM;
        if (param->chromaQpOffset < -12 || 12 < param->chromaQpOffset) 
            return RETCODE_INVALID_PARAM;
        if (param->audEnable != 0 && param->audEnable != 1) 
            return RETCODE_INVALID_PARAM;
        if (param->frameCroppingFlag != 0 &&param->frameCroppingFlag != 1) 
            return RETCODE_INVALID_PARAM;
        if (param->frameCropLeft & 0x01 || param->frameCropRight & 0x01 ||
            param->frameCropTop & 0x01  || param->frameCropBottom & 0x01) {
                return RETCODE_INVALID_PARAM;
        }

        if (picWidth < MIN_ENC_PIC_WIDTH || picWidth > MAX_ENC_PIC_WIDTH ) 
            return RETCODE_INVALID_PARAM;
        if (picHeight < MIN_ENC_PIC_HEIGHT) 
            return RETCODE_INVALID_PARAM;
    }
    else if (pop->bitstreamFormat == STD_HEVC || pop->bitstreamFormat == STD_SVAC || (pop->bitstreamFormat == STD_AVC && productId == PRODUCT_ID_521)) {
        EncVpParam* param     = &pop->EncStdParam.vpParam;

        if (param->svcEnable == TRUE && pop->bitstreamFormat != STD_SVAC)
            return RETCODE_INVALID_PARAM;

        if (picWidth < VP_MIN_ENC_PIC_WIDTH || picWidth > VP_MAX_ENC_PIC_WIDTH)
            return RETCODE_INVALID_PARAM;

        if (picHeight < VP_MIN_ENC_PIC_HEIGHT || picHeight > VP_MAX_ENC_PIC_HEIGHT)
            return RETCODE_INVALID_PARAM;

        if (param->profile != HEVC_PROFILE_MAIN && param->profile != HEVC_PROFILE_MAIN10 && param->profile != HEVC_PROFILE_STILLPICTURE)
            return RETCODE_INVALID_PARAM;

        if (param->internalBitDepth != 8 && param->internalBitDepth != 10)
            return RETCODE_INVALID_PARAM;

        if (param->internalBitDepth > 8 && param->profile == HEVC_PROFILE_MAIN)
            return RETCODE_INVALID_PARAM;

        if (pop->bitstreamFormat == STD_SVAC && param->svcEnable == TRUE && param->svcMode == 1) {
            // only low delay encoding supported when svc enable.
            if (param->gopPresetIdx == PRESET_IDX_IBPBP || param->gopPresetIdx == PRESET_IDX_IBBBP || param->gopPresetIdx == PRESET_IDX_RA_IB)
                return RETCODE_INVALID_PARAM;
        }

        if (param->decodingRefreshType < 0 || param->decodingRefreshType > 2)
            return RETCODE_INVALID_PARAM;

        if (param->gopPresetIdx == PRESET_IDX_CUSTOM_GOP) {
            if ( param->gopParam.customGopSize < 1 || param->gopParam.customGopSize > MAX_GOP_NUM)
                return RETCODE_INVALID_PARAM;
        }


        if (pop->bitstreamFormat == STD_AVC) {
            if (param->customLambdaEnable == 1) 
                return RETCODE_INVALID_PARAM;
        }
        if (param->constIntraPredFlag != 1 && param->constIntraPredFlag != 0)
            return RETCODE_INVALID_PARAM;

        if (param->intraRefreshMode < 0 || param->intraRefreshMode > 4)
            return RETCODE_INVALID_PARAM;
        

        if (pop->bitstreamFormat == STD_HEVC) {
            if (param->independSliceMode < 0 || param->independSliceMode > 1)
                return RETCODE_INVALID_PARAM;

            if (param->independSliceMode != 0) {
                if (param->dependSliceMode < 0 || param->dependSliceMode > 2)
                    return RETCODE_INVALID_PARAM;
            }
        }
        

        if (param->useRecommendEncParam < 0 && param->useRecommendEncParam > 3) 
            return RETCODE_INVALID_PARAM;

        if (param->useRecommendEncParam == 0 || param->useRecommendEncParam == 2 || param->useRecommendEncParam == 3) {

            if (param->intraNxNEnable != 1 && param->intraNxNEnable != 0)
                return RETCODE_INVALID_PARAM;
            
            if (param->skipIntraTrans != 1 && param->skipIntraTrans != 0)
                return RETCODE_INVALID_PARAM;

            if (param->scalingListEnable != 1 && param->scalingListEnable != 0)
                return RETCODE_INVALID_PARAM;

            if (param->tmvpEnable != 1 && param->tmvpEnable != 0)
                return RETCODE_INVALID_PARAM;

            if (param->wppEnable != 1 && param->wppEnable != 0)
                return RETCODE_INVALID_PARAM;

            if (param->useRecommendEncParam != 3) {     // in FAST mode (recommendEncParam==3), maxNumMerge value will be decided in FW
                if (param->maxNumMerge < 0 || param->maxNumMerge > 3) 
                    return RETCODE_INVALID_PARAM;
            }

            if (param->disableDeblk != 1 && param->disableDeblk != 0)
                return RETCODE_INVALID_PARAM;

            if (param->disableDeblk == 0 || param->saoEnable != 0) {
                if (param->lfCrossSliceBoundaryEnable != 1 && param->lfCrossSliceBoundaryEnable != 0)
                    return RETCODE_INVALID_PARAM;
            }

            if (param->disableDeblk == 0) {
                if (param->betaOffsetDiv2 < -6 || param->betaOffsetDiv2 > 6)
                    return RETCODE_INVALID_PARAM;

                if (param->tcOffsetDiv2 < -6 || param->tcOffsetDiv2 > 6)
                    return RETCODE_INVALID_PARAM;
            }
        }

        if (param->losslessEnable != 1 && param->losslessEnable != 0) 
            return RETCODE_INVALID_PARAM;

        if (param->intraQP < 0 || param->intraQP > 63) 
            return RETCODE_INVALID_PARAM;

        if (pop->bitstreamFormat == STD_SVAC && param->internalBitDepth != 8) {
            if (param->intraQP < 1 || param->intraQP > 63) 
                return RETCODE_INVALID_PARAM;
        }

        if (pop->rcEnable != 1 && pop->rcEnable != 0) 
            return RETCODE_INVALID_PARAM;

        if (pop->rcEnable == 1) {

            if (param->minQpI < 0 || param->minQpI > 63) 
                return RETCODE_INVALID_PARAM;
            if (param->maxQpI < 0 || param->maxQpI > 63) 
                return RETCODE_INVALID_PARAM;

            if (param->minQpP < 0 || param->minQpP > 63) 
                return RETCODE_INVALID_PARAM;
            if (param->maxQpP < 0 || param->maxQpP > 63) 
                return RETCODE_INVALID_PARAM;

            if (param->minQpB < 0 || param->minQpB > 63) 
                return RETCODE_INVALID_PARAM;
            if (param->maxQpB < 0 || param->maxQpB > 63) 
                return RETCODE_INVALID_PARAM;

            if (pop->bitstreamFormat == STD_SVAC && param->internalBitDepth != 8) {
                if (param->minQpI < 1 || param->minQpI > 63) 
                    return RETCODE_INVALID_PARAM;
                if (param->maxQpI < 1 || param->maxQpI > 63) 
                    return RETCODE_INVALID_PARAM;

                if (param->minQpP < 1 || param->minQpP > 63) 
                    return RETCODE_INVALID_PARAM;
                if (param->maxQpP < 1 || param->maxQpP > 63) 
                    return RETCODE_INVALID_PARAM;

                if (param->minQpB < 1 || param->minQpB > 63) 
                    return RETCODE_INVALID_PARAM;
                if (param->maxQpB < 1 || param->maxQpB > 63) 
                    return RETCODE_INVALID_PARAM;
            }
         
            if (param->cuLevelRCEnable != 1 && param->cuLevelRCEnable != 0) 
                return RETCODE_INVALID_PARAM;
            
            if (param->hvsQPEnable != 1 && param->hvsQPEnable != 0) 
                return RETCODE_INVALID_PARAM;

            if (param->hvsQPEnable) {
                if (param->maxDeltaQp < 0 || param->maxDeltaQp > 51) 
                    return RETCODE_INVALID_PARAM;
            }            
            
            if (param->bitAllocMode < 0 && param->bitAllocMode > 2) 
                return RETCODE_INVALID_PARAM;

            if (pop->vbvBufferSize < 10 || pop->vbvBufferSize > 3000 )
                return RETCODE_INVALID_PARAM;
        }

        // packed format & cbcrInterleave & nv12 can't be set at the same time. 
        if (pop->packedFormat == 1 && pop->cbcrInterleave == 1)
            return RETCODE_INVALID_PARAM;

        if (pop->packedFormat == 1 && pop->nv21 == 1)
            return RETCODE_INVALID_PARAM;

        // check valid for common param
        if (CheckEncCommonParamValid(pop) == RETCODE_FAILURE)
            return RETCODE_INVALID_PARAM;

        // check valid for RC param
        if (CheckEncRcParamValid(pop) == RETCODE_FAILURE)
            return RETCODE_INVALID_PARAM;

        if (param->gopPresetIdx == PRESET_IDX_CUSTOM_GOP) {
            if (CheckEncCustomGopParamValid(pop) == RETCODE_FAILURE)
                return RETCODE_INVALID_COMMAND;
        }

        if (param->wppEnable && pop->ringBufferEnable)      // WPP can be processed on only linebuffer mode.
            return RETCODE_INVALID_PARAM;

        if (param->chromaCbQpOffset < -12 || param->chromaCbQpOffset > 12)
            return RETCODE_INVALID_PARAM;

        if (param->chromaCrQpOffset < -12 || param->chromaCrQpOffset > 12)
            return RETCODE_INVALID_PARAM;
        
        if (param->intraRefreshMode == 3 && param-> intraRefreshArg == 0)
            return RETCODE_INVALID_PARAM;

        if (pop->bitstreamFormat == STD_HEVC) {
            if (param->nrYEnable != 1 && param->nrYEnable != 0) 
                return RETCODE_INVALID_PARAM;

            if (param->nrCbEnable != 1 && param->nrCbEnable != 0) 
                return RETCODE_INVALID_PARAM;

            if (param->nrCrEnable != 1 && param->nrCrEnable != 0) 
                return RETCODE_INVALID_PARAM;

            if (param->nrNoiseEstEnable != 1 && param->nrNoiseEstEnable != 0) 
                return RETCODE_INVALID_PARAM;

            if (param->nrNoiseSigmaY > 255)
                return RETCODE_INVALID_PARAM;

            if (param->nrNoiseSigmaCb > 255)
                return RETCODE_INVALID_PARAM;

            if (param->nrNoiseSigmaCr > 255)
                return RETCODE_INVALID_PARAM;

            if (param->nrIntraWeightY > 31)
                return RETCODE_INVALID_PARAM;

            if (param->nrIntraWeightCb > 31)
                return RETCODE_INVALID_PARAM;

            if (param->nrIntraWeightCr > 31)
                return RETCODE_INVALID_PARAM;

            if (param->nrInterWeightY > 31)
                return RETCODE_INVALID_PARAM;

            if (param->nrInterWeightCb > 31)
                return RETCODE_INVALID_PARAM;

            if (param->nrInterWeightCr > 31)
                return RETCODE_INVALID_PARAM;

            if((param->nrYEnable == 1 || param->nrCbEnable == 1 || param->nrCrEnable == 1) && (param->losslessEnable == 1))
                return RETCODE_INVALID_PARAM;
        }
        
        
        if (pop->bitstreamFormat == STD_SVAC) {
            // set parameters that not used for SVAC encoder to default value (=0)
            param->decodingRefreshType  = 0;
            param->customMDEnable       = 0;
            param->customLambdaEnable   = 0;
            param->maxNumMerge          = 0;

            if (param->chromaAcQpOffset < -3 || param->chromaAcQpOffset> 3)
                return RETCODE_INVALID_PARAM;

            if (param->chromaDcQpOffset < -3 || param->chromaDcQpOffset> 3)
                return RETCODE_INVALID_PARAM;

            if (param->lumaDcQpOffset < -3 || param->lumaDcQpOffset> 3)
                return RETCODE_INVALID_PARAM;
        }
       
    }
    if (pop->linear2TiledEnable == TRUE) {
        if (pop->linear2TiledMode != FF_FRAME && pop->linear2TiledMode != FF_FIELD ) 
            return RETCODE_INVALID_PARAM;
    }
    
    return RETCODE_SUCCESS;
}

RetCode ProductVpuEncFiniSeq(CodecInst* instance)
{
    RetCode     ret = RETCODE_NOT_FOUND_VPU_DEVICE;

    switch (instance->productId) {

    case PRODUCT_ID_512:
    case PRODUCT_ID_515:
    case PRODUCT_ID_511:
        ret = RETCODE_NOT_SUPPORTED_FEATURE;
        break;
    case PRODUCT_ID_520:
    case PRODUCT_ID_525:
    case PRODUCT_ID_521:
        ret = Vp5VpuEncFiniSeq(instance);
        break;
    }
    return ret;
}

RetCode ProductVpuEncSetup(CodecInst* instance)
{
    RetCode     ret = RETCODE_NOT_FOUND_VPU_DEVICE;

    switch (instance->productId) {
    case PRODUCT_ID_512:
    case PRODUCT_ID_515:
    case PRODUCT_ID_520:
    case PRODUCT_ID_525:
    case PRODUCT_ID_521:
    case PRODUCT_ID_511:
        ret = RETCODE_NOT_SUPPORTED_FEATURE;
        break;
    }

    return ret;
}

RetCode ProductVpuEncode(CodecInst* instance, EncParam* param)
{
    RetCode     ret = RETCODE_NOT_FOUND_VPU_DEVICE;

    switch (instance->productId) {

   case PRODUCT_ID_512:
    case PRODUCT_ID_515:
    case PRODUCT_ID_511:
        ret = RETCODE_NOT_SUPPORTED_FEATURE;
        break;
    case PRODUCT_ID_520:
    case PRODUCT_ID_525:
    case PRODUCT_ID_521:
        ret = Vp5VpuEncode(instance, param);
        break;
    default:
        break;
    }

    return ret;
}

RetCode ProductVpuEncGetResult(CodecInst* instance, EncOutputInfo* result)
{
    RetCode     ret = RETCODE_NOT_FOUND_VPU_DEVICE;

    switch (instance->productId) {
    case PRODUCT_ID_512:
    case PRODUCT_ID_515:
    case PRODUCT_ID_511:
        ret = RETCODE_NOT_SUPPORTED_FEATURE;
        break;
    case PRODUCT_ID_520:
    case PRODUCT_ID_525:
    case PRODUCT_ID_521:
        ret = Vp5VpuEncGetResult(instance, result);
        break;
    }

    return ret;
}

RetCode ProductVpuEncGiveCommand(CodecInst* instance, CodecCommand cmd, void* param)
{
    RetCode     ret = RETCODE_NOT_SUPPORTED_FEATURE;

    switch (instance->productId) {
    default:
        ret = Vp5VpuEncGiveCommand(instance, cmd, param);
        break;
    }
    
    return ret;
}

RetCode ProductVpuEncInitSeq(CodecInst* instance)
{
    int         productId;
    RetCode     ret = RETCODE_NOT_FOUND_VPU_DEVICE;

    productId   = instance->productId;

    switch (productId) {
    case PRODUCT_ID_520:
    case PRODUCT_ID_525:
    case PRODUCT_ID_521:
        ret = Vp5VpuEncInitSeq(instance);
        break;
    default:
        break;
    }

    return ret;
}

RetCode ProductVpuEncGetSeqInfo(CodecInst* instance, EncInitialInfo* info)
{
    int         productId;
    RetCode     ret = RETCODE_NOT_FOUND_VPU_DEVICE;

    productId   = instance->productId;

    switch (productId) {
    case PRODUCT_ID_520:
    case PRODUCT_ID_525:
    case PRODUCT_ID_521:
        ret = Vp5VpuEncGetSeqInfo(instance, info);
        break;
    default:
        break;
    }

    return ret;
}
 
