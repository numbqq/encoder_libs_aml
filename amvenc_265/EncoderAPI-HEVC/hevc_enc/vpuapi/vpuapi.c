//--=========================================================================--
//  This file is a part of VPU Reference API project
//-----------------------------------------------------------------------------
//
//       This confidential and proprietary software may be used only
//     as authorized by a licensing agreement from Chips&Media Inc.
//     In the event of publication, the following notice is applicable:
//
//            (C) COPYRIGHT 2006 - 2013  CHIPS&MEDIA INC.
//                      ALL RIGHTS RESERVED
//
//       The entire notice above must be reproduced on all authorized
//       copies.
//
//--=========================================================================--

#include "vpuapifunc.h"
#include "product.h"
#include "common_regdefine.h"
#include "monet.h"


#define INVALID_CORE_INDEX_RETURN_ERROR(_coreIdx)  \
    if (_coreIdx >= MAX_NUM_VPU_CORE) \
    return -1;

static const Uint16* s_pusBitCode[MAX_NUM_VPU_CORE] = {NULL,};
static int s_bitCodeSize[MAX_NUM_VPU_CORE] = {0,};

Uint32 __VPU_BUSY_TIMEOUT = VPU_BUSY_CHECK_TIMEOUT;

static RetCode CheckInstanceValidity(CodecInst * pCodecInst)
{
    int i;
    vpu_instance_pool_t *vip;

    vip = (vpu_instance_pool_t *)vdi_get_instance_pool(pCodecInst->coreIdx);
    if (!vip)
        return RETCODE_INSUFFICIENT_RESOURCE;

    for (i = 0; i < MAX_NUM_INSTANCE; i++) {
        if ((CodecInst *)vip->codecInstPool[i] == pCodecInst)
            return RETCODE_SUCCESS;
    }

    return RETCODE_INVALID_HANDLE;
}

Int32 VPU_IsBusy(Uint32 coreIdx)
{
    Uint32 ret = 0;

    INVALID_CORE_INDEX_RETURN_ERROR(coreIdx);

    SetClockGate(coreIdx, 1);
    ret = ProductVpuIsBusy(coreIdx);
    SetClockGate(coreIdx, 0);

    return ret != 0;
}

Int32 VPU_IsInit(Uint32 coreIdx)
{
    Int32 pc;

    INVALID_CORE_INDEX_RETURN_ERROR(coreIdx);

    SetClockGate(coreIdx, 1);
    pc = ProductVpuIsInit(coreIdx);
    SetClockGate(coreIdx, 0);

    return pc;
}

Int32 VPU_WaitInterrupt(Uint32 coreIdx, int timeout)
{
    Int32 ret;
    CodecInst* instance;

    INVALID_CORE_INDEX_RETURN_ERROR(coreIdx);

    if ((instance=GetPendingInst(coreIdx)) != NULL) {
        ret = ProductVpuWaitInterrupt(instance, timeout);
    }
    else {
        ret = -1;
    }

    return ret;
}

Int32 VPU_WaitInterruptEx(VpuHandle handle, int timeout)
{
    Int32 ret;
    CodecInst *pCodecInst;

    pCodecInst  = handle;

    INVALID_CORE_INDEX_RETURN_ERROR(pCodecInst->coreIdx);

    ret = ProductVpuWaitInterrupt(pCodecInst, timeout);

    return ret;
}

void VPU_ClearInterrupt(Uint32 coreIdx)
{
    /* clear all interrupt flags */
    ProductVpuClearInterrupt(coreIdx, 0xffff);
}

void VPU_ClearInterruptEx(VpuHandle handle, Int32 intrFlag)
{
    CodecInst *pCodecInst;

    pCodecInst  = handle;

    ProductVpuClearInterrupt(pCodecInst->coreIdx, intrFlag);
}

int VPU_GetMvColBufSize(CodStd codStd, int width, int height, int num)
{
    int size_mvcolbuf = ProductCalculateAuxBufferSize(AUX_BUF_TYPE_MVCOL, codStd, width, height);

    if (codStd == STD_AVC || codStd == STD_HEVC || codStd == STD_VP9 )
        size_mvcolbuf *= num;

    return size_mvcolbuf;
}

RetCode VPU_GetFBCOffsetTableSize(CodStd codStd, int width, int height, int* ysize, int* csize)
{
    if (ysize == NULL || csize == NULL)
        return RETCODE_INVALID_PARAM;

    *ysize = ProductCalculateAuxBufferSize(AUX_BUF_TYPE_FBC_Y_OFFSET, codStd, width, height);
    *csize = ProductCalculateAuxBufferSize(AUX_BUF_TYPE_FBC_C_OFFSET, codStd, width, height);

    return RETCODE_SUCCESS;
}

int VPU_GetFrameBufSize(int coreIdx, int stride, int height, int mapType, int format, int interleave, DRAMConfig *pDramCfg)
{
    int productId;
    UNREFERENCED_PARAMETER(interleave);             /*!<< for backward compatibility */

    if (coreIdx < 0 || coreIdx >= MAX_NUM_VPU_CORE)
        return -1;

    productId = ProductVpuGetId(coreIdx);

    return ProductCalculateFrameBufSize(productId, stride, height, (TiledMapType)mapType, (FrameBufferFormat)format, (BOOL)interleave, pDramCfg);
}


int VPU_GetProductId(int coreIdx)
{
    Int32 productId;

    INVALID_CORE_INDEX_RETURN_ERROR(coreIdx);

    if ((productId=ProductVpuGetId(coreIdx)) != PRODUCT_ID_NONE) {
        return productId;
    }

    if (vdi_init(coreIdx) < 0)
        return -1;

    EnterLock(coreIdx);
    if (ProductVpuScan(coreIdx) == FALSE)
        productId = -1;
    else
        productId = ProductVpuGetId(coreIdx);
    LeaveLock((coreIdx));

    vdi_release(coreIdx);

    return productId;
}

int VPU_GetOpenInstanceNum(Uint32 coreIdx)
{
    INVALID_CORE_INDEX_RETURN_ERROR(coreIdx);

    return vdi_get_instance_num(coreIdx);
}

static RetCode InitializeVPU(Uint32 coreIdx, const Uint16* code, Uint32 size)
{
    RetCode     ret;

    if (vdi_init(coreIdx) < 0)
        return RETCODE_FAILURE;

    EnterLock(coreIdx);

    if (vdi_get_instance_num(coreIdx) > 0) {
        if (ProductVpuScan(coreIdx) == 0) {
            LeaveLock(coreIdx);
            return RETCODE_NOT_FOUND_VPU_DEVICE;
        }
    }

    if (VPU_IsInit(coreIdx) != 0) {
        SetClockGate(coreIdx, 1);
        ProductVpuReInit(coreIdx, (void *)code, size);
        LeaveLock(coreIdx);
        return RETCODE_CALLED_BEFORE;
    }

    InitCodecInstancePool(coreIdx);

    SetClockGate(coreIdx, 1);
    ret = ProductVpuReset(coreIdx, SW_RESET_ON_BOOT);
    if (ret != RETCODE_SUCCESS) {
        LeaveLock(coreIdx);
        return ret;
    }

    ret = ProductVpuInit(coreIdx, (void*)code, size);
    if (ret != RETCODE_SUCCESS) {
        LeaveLock(coreIdx);
        return ret;
    }

    LeaveLock(coreIdx);
    return RETCODE_SUCCESS;
}

RetCode VPU_Init(Uint32 coreIdx)
{
    if (coreIdx >= MAX_NUM_VPU_CORE)
        return RETCODE_INVALID_PARAM;
#if 0
    if (s_bitCodeSize[coreIdx] == 0)
        return RETCODE_NOT_FOUND_BITCODE_PATH;
#else
    if (s_bitCodeSize[coreIdx] == 0) {
        s_bitCodeSize[coreIdx] = (sizeof(bit_code) + 1)/2;
        s_pusBitCode[coreIdx] = bit_code;
    }
#endif

    return InitializeVPU(coreIdx, s_pusBitCode[coreIdx], s_bitCodeSize[coreIdx]);
}

RetCode VPU_InitWithBitcode(Uint32 coreIdx, const Uint16* code, Uint32 size)
{
    if (coreIdx >= MAX_NUM_VPU_CORE)
        return RETCODE_INVALID_PARAM;
    if (code == NULL || size == 0)
        return RETCODE_INVALID_PARAM;

    s_pusBitCode[coreIdx] = NULL;
    s_pusBitCode[coreIdx] = (Uint16 *)osal_malloc(size*sizeof(Uint16));
    if (!s_pusBitCode[coreIdx])
        return RETCODE_INSUFFICIENT_RESOURCE;
    osal_memcpy((void *)s_pusBitCode[coreIdx], (const void *)code, size*sizeof(Uint16));
    s_bitCodeSize[coreIdx] = size;

    return InitializeVPU(coreIdx, code, size);
}

RetCode VPU_DeInit(Uint32 coreIdx)
{
    int ret;

    if (coreIdx >= MAX_NUM_VPU_CORE)
        return RETCODE_INVALID_PARAM;

    if (s_pusBitCode[coreIdx] && s_pusBitCode[coreIdx] != bit_code) {
        osal_free((void *)s_pusBitCode[coreIdx]);
    }

    s_pusBitCode[coreIdx] = NULL;
    s_bitCodeSize[coreIdx] = 0;

    ret = vdi_release(coreIdx);
    if (ret != 0)
        return RETCODE_FAILURE;

    return RETCODE_SUCCESS;
}

RetCode VPU_GetVersionInfo(Uint32 coreIdx, Uint32 *versionInfo, Uint32 *revision, Uint32 *productId)
{
    RetCode  ret;

    if (coreIdx >= MAX_NUM_VPU_CORE)
        return RETCODE_INVALID_PARAM;

    EnterLock(coreIdx);

    if (ProductVpuIsInit(coreIdx) == 0) {
        LeaveLock(coreIdx);
        return RETCODE_NOT_INITIALIZED;
    }

    if (GetPendingInst(coreIdx)) {
        LeaveLock(coreIdx);
        return RETCODE_FRAME_NOT_COMPLETE;
    }

    if (productId != NULL) {
        *productId = ProductVpuGetId(coreIdx);
    }
    ret = ProductVpuGetVersion(coreIdx, versionInfo, revision);

    LeaveLock(coreIdx);

    return ret;
}

RetCode VPU_HWReset(Uint32 coreIdx)
{
    if (vdi_hw_reset(coreIdx) < 0 )
        return RETCODE_FAILURE;

    if (GetPendingInst(coreIdx))
    {
        SetPendingInst(coreIdx, 0);
        LeaveLock(coreIdx);    //if vpu is in a lock state. release the state;
    }
    return RETCODE_SUCCESS;
}

/**
* VPU_SWReset
* IN
*    forcedReset : 1 if there is no need to waiting for BUS transaction,
*                  0 for otherwise
* OUT
*    RetCode : RETCODE_FAILURE if failed to reset,
*              RETCODE_SUCCESS for otherwise
*/
RetCode VPU_SWReset(Uint32 coreIdx, SWResetMode resetMode, void *pendingInst)
{
    CodecInst *pCodecInst = (CodecInst *)pendingInst;
    RetCode    ret = RETCODE_SUCCESS;

    SetClockGate(coreIdx, 1);

    if (pCodecInst) {
        SetPendingInst(pCodecInst->coreIdx, 0);
        LeaveLock(coreIdx);
        SetClockGate(coreIdx, 1);
        if (pCodecInst->loggingEnable) {
            vdi_log(pCodecInst->coreIdx, (pCodecInst->productId == PRODUCT_ID_960 || pCodecInst->productId == PRODUCT_ID_980)?0x10:0x10000, 1);
        }
    }

    ret = ProductVpuReset(coreIdx, resetMode);

    if (pCodecInst) {
        if (pCodecInst->loggingEnable) {
            vdi_log(pCodecInst->coreIdx, (pCodecInst->productId == PRODUCT_ID_960 || pCodecInst->productId == PRODUCT_ID_980)?0x10:0x10000, 0);
        }
    }

    SetClockGate(coreIdx, 0);

    return ret;
}


//---- VPU_SLEEP/WAKE
RetCode VPU_SleepWake(Uint32 coreIdx, int iSleepWake)
{
    RetCode ret;

    SetClockGate(coreIdx, TRUE);
    ret = ProductVpuSleepWake(coreIdx, iSleepWake, s_pusBitCode[coreIdx], s_bitCodeSize[coreIdx]);
    SetClockGate(coreIdx, FALSE);

    return ret;
}

RetCode VPU_EncSetWrPtr(EncHandle handle, PhysicalAddress addr, int updateRdPtr)
{
    CodecInst* pCodecInst;
    CodecInst* pPendingInst;
    EncInfo * pEncInfo;
    RetCode ret;

    ret = CheckEncInstanceValidity(handle);
    if (ret != RETCODE_SUCCESS)
        return ret;
    pCodecInst = (CodecInst*)handle;

    if (pCodecInst->productId == PRODUCT_ID_960 || pCodecInst->productId == PRODUCT_ID_980) {
        return RETCODE_NOT_SUPPORTED_FEATURE;
    }

    pEncInfo = &handle->CodecInfo->encInfo;
    pPendingInst = GetPendingInst(pCodecInst->coreIdx);
    if (pCodecInst == pPendingInst) {
        VpuWriteReg(pCodecInst->coreIdx, pEncInfo->streamWrPtrRegAddr, addr);
    }
    else {
        EnterLock(pCodecInst->coreIdx);
        VpuWriteReg(pCodecInst->coreIdx, pEncInfo->streamWrPtrRegAddr, addr);
        LeaveLock(pCodecInst->coreIdx);
    }
    pEncInfo->streamWrPtr = addr;
    if (updateRdPtr)
        pEncInfo->streamRdPtr = addr;

    return RETCODE_SUCCESS;
}

RetCode VPU_EncOpen(EncHandle* pHandle, EncOpenParam * pop)
{
    CodecInst*  pCodecInst;
    EncInfo*    pEncInfo;
    RetCode     ret;

    if ((ret=ProductCheckEncOpenParam(pop)) != RETCODE_SUCCESS)
        return ret;

    EnterLock(pop->coreIdx);

    if (VPU_IsInit(pop->coreIdx) == 0) {
        LeaveLock(pop->coreIdx);
        return RETCODE_NOT_INITIALIZED;
    }

    ret = GetCodecInstance(pop->coreIdx, &pCodecInst);
    if (ret == RETCODE_FAILURE) {
        *pHandle = 0;
        LeaveLock(pop->coreIdx);
        return RETCODE_FAILURE;
    }

    pCodecInst->isDecoder = FALSE;
    *pHandle = pCodecInst;
    pEncInfo = &pCodecInst->CodecInfo->encInfo;

    osal_memset(pEncInfo, 0x00, sizeof(EncInfo));
    pEncInfo->openParam = *pop;

    SetClockGate(pop->coreIdx, TRUE);
    if ((ret=ProductVpuEncBuildUpOpenParam(pCodecInst, pop)) != RETCODE_SUCCESS) {
        *pHandle = 0;
    }
    SetClockGate(pop->coreIdx, FALSE);

    LeaveLock(pCodecInst->coreIdx);

    return ret;
}

RetCode VPU_EncClose(EncHandle handle)
{
    CodecInst*  pCodecInst;
    EncInfo*    pEncInfo;
    RetCode     ret;

    ret = CheckEncInstanceValidity(handle);
    if (ret != RETCODE_SUCCESS)
        return ret;

    pCodecInst = handle;
    pEncInfo = &pCodecInst->CodecInfo->encInfo;

    EnterLock(pCodecInst->coreIdx);

    if (pEncInfo->initialInfoObtained) {
        VpuWriteReg(pCodecInst->coreIdx, pEncInfo->streamWrPtrRegAddr, pEncInfo->streamWrPtr);
        VpuWriteReg(pCodecInst->coreIdx, pEncInfo->streamRdPtrRegAddr, pEncInfo->streamRdPtr);

        if ((ret=ProductVpuEncFiniSeq(pCodecInst)) != RETCODE_SUCCESS) {
            if (pCodecInst->loggingEnable)
                vdi_log(pCodecInst->coreIdx, ENC_SEQ_END, 0);
            LeaveLock(pCodecInst->coreIdx);
            ret = RETCODE_VPU_RESPONSE_TIMEOUT;
        }
        if (pCodecInst->loggingEnable)
            vdi_log(pCodecInst->coreIdx, ENC_SEQ_END, 0);
        pEncInfo->streamWrPtr = VpuReadReg(pCodecInst->coreIdx, pEncInfo->streamWrPtrRegAddr);
    }

    if (pEncInfo->vbScratch.size) {
        vdi_free_dma_memory(pCodecInst->coreIdx, &pEncInfo->vbScratch);
    }

    if (pEncInfo->vbWork.size) {
        vdi_free_dma_memory(pCodecInst->coreIdx, &pEncInfo->vbWork);
    }

    if (pEncInfo->vbFrame.size) {
        if (pEncInfo->frameAllocExt == 0)
            vdi_free_dma_memory(pCodecInst->coreIdx, &pEncInfo->vbFrame);
    }

    if (pCodecInst->codecMode == HEVC_ENC) {
        if (pEncInfo->vbSubSamBuf.size)
            vdi_free_dma_memory(pCodecInst->coreIdx, &pEncInfo->vbSubSamBuf);

        if (pEncInfo->vbMV.size)
            vdi_free_dma_memory(pCodecInst->coreIdx, &pEncInfo->vbMV);

        if (pEncInfo->vbFbcYTbl.size)
            vdi_free_dma_memory(pCodecInst->coreIdx, &pEncInfo->vbFbcYTbl);

        if (pEncInfo->vbFbcCTbl.size)
            vdi_free_dma_memory(pCodecInst->coreIdx, &pEncInfo->vbFbcCTbl);

        if (pEncInfo->vbTemp.size) {
            //vdi_dettach_dma_memory(pCodecInst->coreIdx, &pEncInfo->vbTemp);
            vdi_free_dma_memory(pCodecInst->coreIdx, &pEncInfo->vbTemp);
        }
    }

    if (pCodecInst->codecMode == C7_AVC_ENC) {
        if (pEncInfo->vbFbcYTbl.size)
            vdi_free_dma_memory(pCodecInst->coreIdx, &pEncInfo->vbFbcYTbl);

        if (pEncInfo->vbFbcCTbl.size)
            vdi_free_dma_memory(pCodecInst->coreIdx, &pEncInfo->vbFbcCTbl);
    }

    if (pEncInfo->vbPPU.size) {
        if (pEncInfo->ppuAllocExt == 0)
            vdi_free_dma_memory(pCodecInst->coreIdx, &pEncInfo->vbPPU);
    }
    if (pEncInfo->vbSubSampFrame.size)
        vdi_free_dma_memory(pCodecInst->coreIdx, &pEncInfo->vbSubSampFrame);
    if (pEncInfo->vbMvcSubSampFrame.size)
        vdi_free_dma_memory(pCodecInst->coreIdx, &pEncInfo->vbMvcSubSampFrame);

    LeaveLock(pCodecInst->coreIdx);

    FreeCodecInstance(pCodecInst);

    return ret;
}

RetCode VPU_EncGetInitialInfo(EncHandle handle, EncInitialInfo * info)
{
    CodecInst*  pCodecInst;
    EncInfo*    pEncInfo;
    RetCode     ret;

    ret = CheckEncInstanceValidity(handle);
    if (ret != RETCODE_SUCCESS)
        return ret;

    if (info == 0) {
        return RETCODE_INVALID_PARAM;
    }

    pCodecInst = handle;
    pEncInfo = &pCodecInst->CodecInfo->encInfo;

    EnterLock(pCodecInst->coreIdx);

    if (GetPendingInst(pCodecInst->coreIdx)) {
        LeaveLock(pCodecInst->coreIdx);
        return RETCODE_FRAME_NOT_COMPLETE;
    }

    if ((ret=ProductVpuEncSetup(pCodecInst)) != RETCODE_SUCCESS) {
        LeaveLock(pCodecInst->coreIdx);
        return ret;
    }

    if (pCodecInst->codecMode == HEVC_ENC) {
        info->minFrameBufferCount   = pEncInfo->initialInfo.minFrameBufferCount;
        info->minSrcFrameCount      = pEncInfo->initialInfo.minSrcFrameCount;
    }
    else
        info->minFrameBufferCount = 2; // reconstructed frame + reference frame

    pEncInfo->initialInfo         = *info;
    pEncInfo->initialInfoObtained = TRUE;

    LeaveLock(pCodecInst->coreIdx);

    return RETCODE_SUCCESS;
}

RetCode VPU_EncRegisterFrameBuffer(EncHandle handle, FrameBuffer* bufArray, int num, int stride, int height, int mapType)
{
    CodecInst*      pCodecInst;
    EncInfo*        pEncInfo;
    Int32           i;
    RetCode         ret;
    EncOpenParam*   openParam;
    FrameBuffer*    fb;

    ret = CheckEncInstanceValidity(handle);
    // FIXME temp
    if (ret != RETCODE_SUCCESS)
        return ret;

    pCodecInst = handle;
    pEncInfo = &pCodecInst->CodecInfo->encInfo;
    openParam = &pEncInfo->openParam;

    if (pEncInfo->stride)
        return RETCODE_CALLED_BEFORE;

    if (!pEncInfo->initialInfoObtained)
        return RETCODE_WRONG_CALL_SEQUENCE;

    if (num < pEncInfo->initialInfo.minFrameBufferCount)
        return RETCODE_INSUFFICIENT_FRAME_BUFFERS;

    if (stride == 0 || (stride % 8 != 0) || stride < 0)
        return RETCODE_INVALID_STRIDE;

    if (height == 0 || height < 0)
        return RETCODE_INVALID_PARAM;

    if (openParam->bitstreamFormat == STD_HEVC) {
        if (stride % 32 != 0)
            return RETCODE_INVALID_STRIDE;
    }

    EnterLock(pCodecInst->coreIdx);

    if (GetPendingInst(pCodecInst->coreIdx)) {
        LeaveLock(pCodecInst->coreIdx);
        return RETCODE_FRAME_NOT_COMPLETE;
    }

    pEncInfo->numFrameBuffers   = num;
    pEncInfo->stride            = stride;
    pEncInfo->frameBufferHeight = height;
    pEncInfo->mapType           = mapType;
    pEncInfo->mapCfg.productId  = pCodecInst->productId;

    if (bufArray) {
        for (i=0; i<num; i++)
            pEncInfo->frameBufPool[i] = bufArray[i];
    }

    if (pEncInfo->frameAllocExt == FALSE) {
        fb = pEncInfo->frameBufPool;
        if (bufArray) {
            if (bufArray[0].bufCb == (Uint32)-1 && bufArray[0].bufCr == (Uint32)-1) {
                Uint32 size;
                pEncInfo->frameAllocExt = TRUE;
                size = ProductCalculateFrameBufSize(pCodecInst->productId, stride, height,
                                                    (TiledMapType)mapType, (FrameBufferFormat)openParam->srcFormat,
                                                    (BOOL)openParam->cbcrInterleave, NULL);
                if (mapType == LINEAR_FRAME_MAP)
                {
                    pEncInfo->vbFrame.phys_addr = bufArray[0].bufY;
                    pEncInfo->vbFrame.size      = size * num;
                }
            }
        }
        ret = ProductVpuAllocateFramebuffer(
            pCodecInst, fb, (TiledMapType)mapType, num, stride, height, (FrameBufferFormat)openParam->srcFormat,
            openParam->cbcrInterleave, FALSE, openParam->frameEndian, &pEncInfo->vbFrame, 0, FB_TYPE_CODEC);
        if (ret != RETCODE_SUCCESS) {
            SetPendingInst(pCodecInst->coreIdx, 0);
            LeaveLock(pCodecInst->coreIdx);
            return ret;
        }
    }
    ret = ProductVpuRegisterFramebuffer(pCodecInst);

    SetPendingInst(pCodecInst->coreIdx, 0);

    LeaveLock(pCodecInst->coreIdx);

    return ret;
}

RetCode VPU_EncGetFrameBuffer(
    EncHandle     handle,
    int           frameIdx,
    FrameBuffer * frameBuf)
{
    CodecInst * pCodecInst;
    EncInfo * pEncInfo;
    RetCode ret;

    ret = CheckEncInstanceValidity(handle);
    if (ret != RETCODE_SUCCESS)
        return ret;

    pCodecInst = handle;
    pEncInfo = &pCodecInst->CodecInfo->encInfo;

    if (frameIdx<0 || frameIdx>pEncInfo->numFrameBuffers)
        return RETCODE_INVALID_PARAM;

    if (frameBuf == 0)
        return RETCODE_INVALID_PARAM;

    *frameBuf = pEncInfo->frameBufPool[frameIdx];

    return RETCODE_SUCCESS;

}

RetCode VPU_EncGetBitstreamBuffer( EncHandle handle,
    PhysicalAddress * prdPrt,
    PhysicalAddress * pwrPtr,
    int * size)
{
    CodecInst * pCodecInst;
    EncInfo * pEncInfo;
    PhysicalAddress rdPtr;
    PhysicalAddress wrPtr;
    Uint32 room;
    RetCode ret;

    ret = CheckEncInstanceValidity(handle);
    if (ret != RETCODE_SUCCESS)
        return ret;

    if ( prdPrt == 0 || pwrPtr == 0 || size == 0) {
        return RETCODE_INVALID_PARAM;
    }

    pCodecInst = handle;
    pEncInfo = &pCodecInst->CodecInfo->encInfo;

    rdPtr = pEncInfo->streamRdPtr;

    SetClockGate(pCodecInst->coreIdx, 1);

    if (GetPendingInst(pCodecInst->coreIdx) == pCodecInst)
        wrPtr = VpuReadReg(pCodecInst->coreIdx, pEncInfo->streamWrPtrRegAddr);
    else
        wrPtr = pEncInfo->streamWrPtr;

    SetClockGate(pCodecInst->coreIdx, 0);
    if (pEncInfo->ringBufferEnable == 1 || pEncInfo->lineBufIntEn == 1) {
        if (wrPtr >= rdPtr) {
            room = wrPtr - rdPtr;
        }
        else {
            room = (pEncInfo->streamBufEndAddr - rdPtr) + (wrPtr - pEncInfo->streamBufStartAddr);
        }
    }
    else {
        if (wrPtr >= rdPtr)
            room = wrPtr - rdPtr;
        else
            return RETCODE_INVALID_PARAM;
    }

    *prdPrt = rdPtr;
    *pwrPtr = wrPtr;
    *size = room;

    return RETCODE_SUCCESS;
}

RetCode VPU_EncUpdateBitstreamBuffer(
    EncHandle handle,
    int size)
{
    CodecInst * pCodecInst;
    EncInfo * pEncInfo;
    PhysicalAddress wrPtr;
    PhysicalAddress rdPtr;
    RetCode ret;
    int        room = 0;
    ret = CheckEncInstanceValidity(handle);
    if (ret != RETCODE_SUCCESS)
        return ret;

    pCodecInst = handle;
    pEncInfo = &pCodecInst->CodecInfo->encInfo;

    rdPtr = pEncInfo->streamRdPtr;

    SetClockGate(pCodecInst->coreIdx, 1);

    if (GetPendingInst(pCodecInst->coreIdx) == pCodecInst)
        wrPtr = VpuReadReg(pCodecInst->coreIdx, pEncInfo->streamWrPtrRegAddr);
    else
        wrPtr = pEncInfo->streamWrPtr;

    if ( rdPtr < wrPtr ) {
        if ( rdPtr + size  > wrPtr ) {
            SetClockGate(pCodecInst->coreIdx, 0);
            return RETCODE_INVALID_PARAM;
        }
    }

    if (pEncInfo->ringBufferEnable == TRUE || pEncInfo->lineBufIntEn == TRUE) {
        rdPtr += size;
        if (rdPtr > pEncInfo->streamBufEndAddr) {
            if (pEncInfo->lineBufIntEn == TRUE) {
                return RETCODE_INVALID_PARAM;
            }
            room = rdPtr - pEncInfo->streamBufEndAddr;
            rdPtr = pEncInfo->streamBufStartAddr;
            rdPtr += room;
        }

        if (rdPtr == pEncInfo->streamBufEndAddr) {
            rdPtr = pEncInfo->streamBufStartAddr;
        }
    }
    else {
        rdPtr = pEncInfo->streamBufStartAddr;
    }

    pEncInfo->streamRdPtr = rdPtr;
    pEncInfo->streamWrPtr = wrPtr;
    if (GetPendingInst(pCodecInst->coreIdx) == pCodecInst)
        VpuWriteReg(pCodecInst->coreIdx, pEncInfo->streamRdPtrRegAddr, rdPtr);

    if (pEncInfo->lineBufIntEn == TRUE) {
        pEncInfo->streamRdPtr = pEncInfo->streamBufStartAddr;
    }

    SetClockGate(pCodecInst->coreIdx, 0);
    return RETCODE_SUCCESS;
}

RetCode VPU_EncStartOneFrame(
    EncHandle handle,
    EncParam * param
    )
{
    CodecInst*           pCodecInst;
    EncInfo*             pEncInfo;
    RetCode              ret;
    vpu_instance_pool_t* vip;
    FrameBuffer*        pSrcFrame;

    ret = CheckEncInstanceValidity(handle);
    if (ret != RETCODE_SUCCESS)
        return ret;

    pCodecInst = handle;
    pEncInfo   = &pCodecInst->CodecInfo->encInfo;
    vip        = (vpu_instance_pool_t *)vdi_get_instance_pool(pCodecInst->coreIdx);
    if (!vip) {
        return RETCODE_INVALID_HANDLE;
    }

    if (pEncInfo->stride == 0) { // This means frame buffers have not been registered.
        return RETCODE_WRONG_CALL_SEQUENCE;
    }

    if (pEncInfo->srcBufUseIndex[param->srcIdx] == 1
        && param->srcEndFlag == 0) {
        VLOG(ERR,"source frame was already in encoding index %d \n",
                param->srcIdx);
        return RETCODE_INVALID_PARAM;
    }

    pSrcFrame = param->sourceFrame;
    if (pSrcFrame && pSrcFrame->dma_buf_planes && (param->srcEndFlag == 0)) { // use dma_buf
        vpu_multi_dma_buf_info_t dma_info;
        int i;
        if (pSrcFrame->dma_buf_planes > 3)
            return RETCODE_INVALID_FRAME_BUFFER;
        osal_memset(&dma_info, 0x00, sizeof(vpu_multi_dma_buf_info_t));
        dma_info.num_planes = pSrcFrame->dma_buf_planes;
        for (i = 0; i < pSrcFrame->dma_buf_planes; i ++)
                dma_info.fd[i] = pSrcFrame->dma_shared_fd[i];
        VLOG(INFO,"DMA source fd[%d, %d, %d] planes %d\n",
                dma_info.fd[0], dma_info.fd[1], dma_info.fd[2],
                pSrcFrame->dma_buf_planes);
        if (vdi_config_dma(pCodecInst->coreIdx, &dma_info) !=0)
                return RETCODE_INVALID_FRAME_BUFFER;
        pSrcFrame->bufY = dma_info.phys_addr[0];
        if (dma_info.num_planes == 1 && pEncInfo->openParam.packedFormat == 0)
        { // only one buffer plane cb/cr from the same buffer  but no packed
              pSrcFrame->bufCb = (PhysicalAddress) (pSrcFrame->bufY +
                        pSrcFrame->height * pSrcFrame->stride);
              if (pSrcFrame->cbcrInterleave == 0) { //
                  pSrcFrame->bufCr = (PhysicalAddress) (pSrcFrame->bufCb +
                            pSrcFrame->height * pSrcFrame->stride/4);
              }
        }
        if (dma_info.num_planes > 1)
                pSrcFrame->bufCb = dma_info.phys_addr[1];
        if (dma_info.num_planes>2)
               pSrcFrame->bufCr = dma_info.phys_addr[2];
        VLOG(INFO,"DMA frame physical bufY 0x%x Cb 0x%x Cr 0x%x planes %d \n",
                pSrcFrame->bufY, pSrcFrame->bufCb, pSrcFrame->bufCr,
                dma_info.num_planes);
    }

    ret = CheckEncParam(handle, param);
    if (ret != RETCODE_SUCCESS) {
        return ret;
    }

    EnterLock(pCodecInst->coreIdx);

    pEncInfo->ptsMap[param->srcIdx] = (pEncInfo->openParam.enablePTS == TRUE) ? GetTimestamp(handle) : param->pts;

    if (GetPendingInst(pCodecInst->coreIdx)) {
        LeaveLock(pCodecInst->coreIdx);
        return RETCODE_FRAME_NOT_COMPLETE;
    }

    ret = ProductVpuEncode(pCodecInst, param);

    if (ret == RETCODE_SUCCESS) {
        /* record the used source frame */
        pEncInfo->srcBufUseIndex[param->srcIdx] = 1;
        pEncInfo->srcBufMap[param->srcIdx] = *pSrcFrame;
    }

    SetPendingInst(pCodecInst->coreIdx, pCodecInst);

    return ret;
}

RetCode VPU_EncGetOutputInfo(
    EncHandle       handle,
    EncOutputInfo*  info
    )
{
    CodecInst*  pCodecInst;
    EncInfo*    pEncInfo;
    RetCode     ret;
    FrameBuffer*  pSrcFrame = NULL;

    ret = CheckEncInstanceValidity(handle);
    if (ret != RETCODE_SUCCESS) {
        return ret;
    }

    if (info == 0) {
        return RETCODE_INVALID_PARAM;
    }

    pCodecInst = handle;
    pEncInfo   = &pCodecInst->CodecInfo->encInfo;

    if (pCodecInst != GetPendingInst(pCodecInst->coreIdx)) {
        SetPendingInst(pCodecInst->coreIdx, 0);
        LeaveLock(pCodecInst->coreIdx);
        return RETCODE_WRONG_CALL_SEQUENCE;
    }

    ret = ProductVpuEncGetResult(pCodecInst, info);

    if (ret == RETCODE_SUCCESS) {
        if (info->encSrcIdx >= 0 && info->reconFrameIndex >= 0 )
        {

            info->pts = pEncInfo->ptsMap[info->encSrcIdx];
             /* return the used soure frame */
            if (pEncInfo->srcBufUseIndex[info->encSrcIdx] == 1) {
                pSrcFrame = &(pEncInfo->srcBufMap[info->encSrcIdx]);
                info->encSrcFrame = *pSrcFrame;
                pEncInfo->srcBufUseIndex[info->encSrcIdx] = 0;
             } else
               VLOG(ERR, "Soure Frame already retired index= %d use %d\n",
                        info->encSrcIdx,
                        pEncInfo->srcBufUseIndex[info->encSrcIdx]);
        }
    }
    else {
        info->pts = 0LL;
    }

    SetPendingInst(pCodecInst->coreIdx, 0);
    LeaveLock(pCodecInst->coreIdx);
    /*unmap the dma buf if it has retired*/
    if (pSrcFrame && pSrcFrame->dma_buf_planes) { // use dma_buf
        vpu_multi_dma_buf_info_t dma_info;
        int i;
        osal_memset(&dma_info, 0x00, sizeof(vpu_multi_dma_buf_info_t));
        dma_info.num_planes = pSrcFrame->dma_buf_planes;
        for (i = 0; i < pSrcFrame->dma_buf_planes; i ++)
                dma_info.fd[i] = pSrcFrame->dma_shared_fd[i];
        dma_info.phys_addr[0] = pSrcFrame->bufY;
        if (dma_info.num_planes > 1)
            dma_info.phys_addr[1]= pSrcFrame->bufCb;
        if (dma_info.num_planes > 2)
            dma_info.phys_addr[2] = pSrcFrame->bufCr;
        if (vdi_unmap_dma(pCodecInst->coreIdx, &dma_info) !=0) {
            VLOG(ERR,"Failed to de-reference DMA buffer \n");
            ret = RETCODE_FAILURE;
        }
    }

    return ret;
}

RetCode VPU_EncGiveCommand(
    EncHandle       handle,
    CodecCommand    cmd,
    void*           param
    )
{
    CodecInst*  pCodecInst;
    EncInfo*    pEncInfo;
    RetCode     ret;

    ret = CheckEncInstanceValidity(handle);
    if (ret != RETCODE_SUCCESS) {
        return ret;
    }

    pCodecInst = handle;
    pEncInfo = &pCodecInst->CodecInfo->encInfo;
    switch (cmd)
    {
    case ENABLE_ROTATION :
        {
            pEncInfo->rotationEnable = 1;
        }
        break;
    case DISABLE_ROTATION :
        {
            pEncInfo->rotationEnable = 0;
        }
        break;
    case ENABLE_MIRRORING :
        {
            pEncInfo->mirrorEnable = 1;
        }
        break;
    case DISABLE_MIRRORING :
        {
            pEncInfo->mirrorEnable = 0;
        }
        break;
    case SET_MIRROR_DIRECTION :
        {
            MirrorDirection mirDir;

            if (param == 0) {
                return RETCODE_INVALID_PARAM;
            }
            mirDir = *(MirrorDirection *)param;
            if ( !(mirDir == MIRDIR_NONE) && !(mirDir == MIRDIR_HOR) && !(mirDir == MIRDIR_VER) && !(mirDir == MIRDIR_HOR_VER)) {
                return RETCODE_INVALID_PARAM;
            }
            pEncInfo->mirrorDirection = mirDir;
        }
        break;
    case SET_ROTATION_ANGLE :
        {
            int angle;

            if (param == 0) {
                return RETCODE_INVALID_PARAM;
            }
            angle = *(int *)param;
            if (angle != 0 && angle != 90 &&
                angle != 180 && angle != 270) {
                    return RETCODE_INVALID_PARAM;
            }
            if (pEncInfo->initialInfoObtained && (angle == 90 || angle ==270)) {
                return RETCODE_INVALID_PARAM;
            }
            pEncInfo->rotationAngle = angle;
        }
        break;
    case SET_CACHE_CONFIG:
        {
            MaverickCacheConfig *mcCacheConfig;
            if (param == 0) {
                return RETCODE_INVALID_PARAM;
            }
            mcCacheConfig = (MaverickCacheConfig *)param;
            pEncInfo->cacheConfig = *mcCacheConfig;
        }
        break;
    case ENC_PUT_VIDEO_HEADER:
        {
            EncHeaderParam *encHeaderParam;

            if (param == 0) {
                return RETCODE_INVALID_PARAM;
            }
            encHeaderParam = (EncHeaderParam *)param;
            if (pCodecInst->codecMode == MP4_ENC || pCodecInst->codecMode == C7_MP4_ENC ) {
                if (!( VOL_HEADER <= encHeaderParam->headerType && encHeaderParam->headerType <= VIS_HEADER)) {
                    return RETCODE_INVALID_PARAM;
                }
            }
            else if  (pCodecInst->codecMode == AVC_ENC || pCodecInst->codecMode == C7_AVC_ENC) {
                if (!( SPS_RBSP <= encHeaderParam->headerType && encHeaderParam->headerType <= PPS_RBSP_MVC)) {
                    return RETCODE_INVALID_PARAM;
                }
            }
            else if (pCodecInst->codecMode == HEVC_ENC) {
                if (!( CODEOPT_ENC_VPS <= encHeaderParam->headerType && encHeaderParam->headerType <= (CODEOPT_ENC_VPS|CODEOPT_ENC_SPS|CODEOPT_ENC_PPS))) {
                    return RETCODE_INVALID_PARAM;
                }
                if (pEncInfo->ringBufferEnable == 0 ) {
                    if (encHeaderParam->buf % 16 || encHeaderParam->size == 0)
                        return RETCODE_INVALID_PARAM;
                }
                if (encHeaderParam->headerType & CODEOPT_ENC_VCL)   // ENC_PUT_VIDEO_HEADER encode only non-vcl header.
                    return RETCODE_INVALID_PARAM;

            }
            else
                return RETCODE_INVALID_PARAM;

            if (pEncInfo->ringBufferEnable == 0 ) {
                if (encHeaderParam->buf % 8 || encHeaderParam->size == 0) {
                    return RETCODE_INVALID_PARAM;
                }
            }
            if (pCodecInst->codecMode == HEVC_ENC) {
                return Wave4VpuEncGetHeader(handle, encHeaderParam);
            }
        }
    case SET_SEC_AXI:
        {
            SecAxiUse secAxiUse;

            if (param == 0) {
                return RETCODE_INVALID_PARAM;
            }
            secAxiUse = *(SecAxiUse *)param;
            if (handle->productId == PRODUCT_ID_410 || handle->productId == PRODUCT_ID_4102 || handle->productId == PRODUCT_ID_420 ||
                handle->productId == PRODUCT_ID_412  || handle->productId == PRODUCT_ID_420L || handle->productId == PRODUCT_ID_510 ||
                handle->productId == PRODUCT_ID_512 || handle->productId == PRODUCT_ID_515 || handle->productId == PRODUCT_ID_520) {
                if (handle->codecMode == HEVC_DEC) {
                    pEncInfo->secAxiInfo.u.wave4.useIpEnable    = secAxiUse.u.wave4.useIpEnable;
                    pEncInfo->secAxiInfo.u.wave4.useLfRowEnable = secAxiUse.u.wave4.useLfRowEnable;
                    pEncInfo->secAxiInfo.u.wave4.useBitEnable   = secAxiUse.u.wave4.useBitEnable;
                }
                else {
                    pEncInfo->secAxiInfo.u.wave4.useEncImdEnable = secAxiUse.u.wave4.useEncImdEnable;
                    pEncInfo->secAxiInfo.u.wave4.useEncRdoEnable = secAxiUse.u.wave4.useEncRdoEnable;
                    pEncInfo->secAxiInfo.u.wave4.useEncLfEnable  = secAxiUse.u.wave4.useEncLfEnable;
                }
            }
            else { // coda9 or coda7q or ...
                pEncInfo->secAxiInfo.u.coda9.useBitEnable  = secAxiUse.u.coda9.useBitEnable;
                pEncInfo->secAxiInfo.u.coda9.useIpEnable   = secAxiUse.u.coda9.useIpEnable;
                pEncInfo->secAxiInfo.u.coda9.useDbkYEnable = secAxiUse.u.coda9.useDbkYEnable;
                pEncInfo->secAxiInfo.u.coda9.useDbkCEnable = secAxiUse.u.coda9.useDbkCEnable;
                pEncInfo->secAxiInfo.u.coda9.useOvlEnable  = secAxiUse.u.coda9.useOvlEnable;
                pEncInfo->secAxiInfo.u.coda9.useBtpEnable  = secAxiUse.u.coda9.useBtpEnable;
            }
        }
        break;
    case GET_TILEDMAP_CONFIG:
        {
            TiledMapConfig *pMapCfg = (TiledMapConfig *)param;
            if (!pMapCfg) {
                return RETCODE_INVALID_PARAM;
            }
            *pMapCfg = pEncInfo->mapCfg;
            break;
        }
    case SET_DRAM_CONFIG:
        {
            DRAMConfig *cfg = (DRAMConfig *)param;

            if (!cfg) {
                return RETCODE_INVALID_PARAM;
            }

            pEncInfo->dramCfg = *cfg;
            break;
        }
    case GET_DRAM_CONFIG:
        {
            DRAMConfig *cfg = (DRAMConfig *)param;

            if (!cfg) {
                return RETCODE_INVALID_PARAM;
            }

            *cfg = pEncInfo->dramCfg;

            break;
        }
    case ENABLE_LOGGING:
        {
            pCodecInst->loggingEnable = 1;
        }
        break;
    case DISABLE_LOGGING:
        {
            pCodecInst->loggingEnable = 0;
        }
        break;
    case ENC_SET_PARA_CHANGE:
        {
            EncChangeParam* option = (EncChangeParam*)param;
            return Wave4VpuEncParaChange(handle, option);
        }
        break;
    case ENC_SET_SEI_NAL_DATA:
        {
            HevcSEIDataEnc* option = (HevcSEIDataEnc*)param;
            pEncInfo->prefixSeiNalEnable    = option->prefixSeiNalEnable;
            pEncInfo->prefixSeiDataSize     = option->prefixSeiDataSize;
            pEncInfo->prefixSeiDataEncOrder = option->prefixSeiDataEncOrder;
            pEncInfo->prefixSeiNalAddr      = option->prefixSeiNalAddr;

            pEncInfo->suffixSeiNalEnable    = option->suffixSeiNalEnable;
            pEncInfo->suffixSeiDataSize     = option->suffixSeiDataSize;
            pEncInfo->suffixSeiDataEncOrder = option->suffixSeiDataEncOrder;
            pEncInfo->suffixSeiNalAddr      = option->suffixSeiNalAddr;
        }
        break;
    default:
        return RETCODE_INVALID_COMMAND;
    }
    return RETCODE_SUCCESS;
}

RetCode VPU_EncAllocateFrameBuffer(EncHandle handle, FrameBufferAllocInfo info, FrameBuffer *frameBuffer)
{
    CodecInst*  pCodecInst;
    EncInfo*    pEncInfo;
    RetCode     ret;
    int         gdiIndex;

    ret = CheckEncInstanceValidity(handle);
    if (ret != RETCODE_SUCCESS)
        return ret;

    pCodecInst = handle;
    pEncInfo   = &pCodecInst->CodecInfo->encInfo;

    if (!frameBuffer) {
        return RETCODE_INVALID_PARAM;
    }
    if (info.num == 0 || info.num < 0)  {
        return RETCODE_INVALID_PARAM;
    }
    if (info.stride == 0 || info.stride < 0) {
        return RETCODE_INVALID_PARAM;
    }
    if (info.height == 0 || info.height < 0) {
        return RETCODE_INVALID_PARAM;
    }

    if (info.type == FB_TYPE_PPU) {
        if (pEncInfo->numFrameBuffers == 0) {
            return RETCODE_WRONG_CALL_SEQUENCE;
        }
        pEncInfo->ppuAllocExt = frameBuffer[0].updateFbInfo;
        gdiIndex = pEncInfo->numFrameBuffers;
        ret = ProductVpuAllocateFramebuffer(pCodecInst, frameBuffer, (TiledMapType)info.mapType, (Int32)info.num,
                                            info.stride, info.height, info.format, info.cbcrInterleave, info.nv21,
                                            info.endian, &pEncInfo->vbPPU, gdiIndex, (FramebufferAllocType)info.type);
    }
    else if (info.type == FB_TYPE_CODEC) {
        gdiIndex = 0;
        pEncInfo->frameAllocExt = frameBuffer[0].updateFbInfo;
        ret = ProductVpuAllocateFramebuffer(
            pCodecInst, frameBuffer, (TiledMapType)info.mapType, (Int32)info.num,
            info.stride, info.height, info.format, info.cbcrInterleave, FALSE, info.endian, &pEncInfo->vbFrame, gdiIndex, (FramebufferAllocType)info.type);
    }
    else {
        ret = RETCODE_INVALID_PARAM;
    }

    return ret;
}
