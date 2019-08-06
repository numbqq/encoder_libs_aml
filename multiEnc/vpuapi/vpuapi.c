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
#include "enc_regdefine.h"
#include "enc_driver.h"
#include "debug.h"
#include "vdi_osal.h"
#include "vpuapi.h"
#include "chagall.h"

#define INVALID_CORE_INDEX_RETURN_ERROR(_coreIdx)  \
    if (_coreIdx >= MAX_NUM_VPU_CORE) \
    return -1;

static const Uint16* s_pusBitCode[MAX_NUM_VPU_CORE] = {NULL,};
static int s_bitCodeSize[MAX_NUM_VPU_CORE] = {0,};

Uint32 __VPU_BUSY_TIMEOUT = VPU_BUSY_CHECK_TIMEOUT;

Int32 VPU_IsInit(Uint32 coreIdx)
{
    Int32 pc;

    INVALID_CORE_INDEX_RETURN_ERROR(coreIdx);

    pc = ProductVpuIsInit(coreIdx);

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



int VPU_GetFrameBufSize(int coreIdx, int stride, int height, int mapType, int format, int interleave, DRAMConfig *pDramCfg)
{
    int productId;
    UNREFERENCED_PARAMETER(interleave);             /*!<< for backward compatiblity */

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
        ProductVpuReInit(coreIdx, (void *)code, size);
        LeaveLock(coreIdx);
        return RETCODE_CALLED_BEFORE;
    }

    InitCodecInstancePool(coreIdx);

    ret = ProductVpuReset(coreIdx, SW_RESET_ON_BOOT);
    if (ret != RETCODE_SUCCESS) {
        LeaveLock(coreIdx);
        return ret;
    }

#if defined(SUPPORT_SW_UART) || defined(SUPPORT_SW_UART_V2)
	create_sw_uart_thread(coreIdx);
	usleep(500*1000);
#endif
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

    EnterLock(coreIdx);
    if (s_pusBitCode[coreIdx] && s_pusBitCode[coreIdx] != bit_code) {
        osal_free((void *)s_pusBitCode[coreIdx]);
    }

    s_pusBitCode[coreIdx] = NULL;
    s_bitCodeSize[coreIdx] = 0;
    LeaveLock(coreIdx);

#if defined(SUPPORT_SW_UART) || defined(SUPPORT_SW_UART_V2)
	destory_sw_uart_thread();
#endif
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

RetCode VPU_GetProductInfo(Uint32 coreIdx, ProductInfo *productInfo)
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
    productInfo->productId = ProductVpuGetId(coreIdx);
    ret = ProductVpuGetProductInfo(coreIdx, productInfo);

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
    VpuAttr*   attr = &g_VpuCoreAttributes[coreIdx];

    if (attr->supportCommandQueue == TRUE) {
        if (pCodecInst && pCodecInst->loggingEnable) {
            vdi_log(pCodecInst->coreIdx, 0x10000, 1);
        }

        EnterLock(coreIdx);
        ret = ProductVpuReset(coreIdx, resetMode);
        LeaveLock(coreIdx);

        if (pCodecInst && pCodecInst->loggingEnable) {
            vdi_log(pCodecInst->coreIdx, 0x10000, 0);
        }
    }
    else {
        if (pCodecInst) {
            if (pCodecInst->loggingEnable) {
                vdi_log(pCodecInst->coreIdx, (pCodecInst->productId == PRODUCT_ID_960 || pCodecInst->productId == PRODUCT_ID_980)?0x10:0x10000, 1);			
            }
        }
        else {
            EnterLock(coreIdx);
        }

        ret = ProductVpuReset(coreIdx, resetMode);

        if (pCodecInst) {
            if (pCodecInst->loggingEnable) {
                vdi_log(pCodecInst->coreIdx, (pCodecInst->productId == PRODUCT_ID_960 || pCodecInst->productId == PRODUCT_ID_980)?0x10:0x10000, 0);			
            }
            SetPendingInst(pCodecInst->coreIdx, 0);
            LeaveLock(coreIdx);
        }
        else {
            LeaveLock(coreIdx);
        }
    }

    return ret;
}

//---- VPU_SLEEP/WAKE
RetCode VPU_SleepWake(Uint32 coreIdx, int iSleepWake)
{
    RetCode ret;

    EnterLock(coreIdx);
    ret = ProductVpuSleepWake(coreIdx, iSleepWake, s_pusBitCode[coreIdx], s_bitCodeSize[coreIdx]);
    LeaveLock(coreIdx);

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

    if ((ret=ProductVpuEncBuildUpOpenParam(pCodecInst, pop)) != RETCODE_SUCCESS) {
        *pHandle = 0;
    }

    LeaveLock(pCodecInst->coreIdx);

    return ret;
}

RetCode VPU_EncClose(EncHandle handle)
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

    EnterLock(pCodecInst->coreIdx);

    if (pEncInfo->initialInfoObtained) {
        VpuWriteReg(pCodecInst->coreIdx, pEncInfo->streamWrPtrRegAddr, pEncInfo->streamWrPtr);
        VpuWriteReg(pCodecInst->coreIdx, pEncInfo->streamRdPtrRegAddr, pEncInfo->streamRdPtr);

        if ((ret=ProductVpuEncFiniSeq(pCodecInst)) != RETCODE_SUCCESS) {
            if (pCodecInst->loggingEnable)
                vdi_log(pCodecInst->coreIdx, ENC_SEQ_END, 2);

            if (ret == RETCODE_VPU_STILL_RUNNING) {
                LeaveLock(pCodecInst->coreIdx);
                return ret;
            }
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

    if (pCodecInst->codecMode == W_HEVC_ENC || pCodecInst->codecMode == W_SVAC_ENC || pCodecInst->codecMode == W_AVC_ENC) {
        if (pEncInfo->vbSubSamBuf.size)
            vdi_free_dma_memory(pCodecInst->coreIdx, &pEncInfo->vbSubSamBuf);   

        if (pEncInfo->vbMV.size) 
            vdi_free_dma_memory(pCodecInst->coreIdx, &pEncInfo->vbMV);

        if (pEncInfo->vbFbcYTbl.size)
            vdi_free_dma_memory(pCodecInst->coreIdx, &pEncInfo->vbFbcYTbl);

        if (pEncInfo->vbFbcCTbl.size)
            vdi_free_dma_memory(pCodecInst->coreIdx, &pEncInfo->vbFbcCTbl);

        if (pEncInfo->vbTemp.size) 
            vdi_dettach_dma_memory(pCodecInst->coreIdx, &pEncInfo->vbTemp);
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

    if (pCodecInst->codecMode == AVC_ENC && pCodecInst->codecModeAux == AVC_AUX_MVC)
        info->minFrameBufferCount = 3; // reconstructed frame + 2 reference frame
    else if(pCodecInst->codecMode == W_HEVC_ENC || pCodecInst->codecMode == W_SVAC_ENC || pCodecInst->codecMode == W_AVC_ENC) {
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

RetCode VPU_EncRegisterFrameBuffer(EncHandle handle, FrameBuffer* bufArray, int num, int stride, int height, TiledMapType mapType)
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
        for(i=0; i<num; i++) {
            pEncInfo->frameBufPool[i] = bufArray[i];
        }

        if (openParam->EncStdParam.vpParam.svcEnable == TRUE) {
            for(i=num; i<num*2; i++) {
                pEncInfo->frameBufPool[i] = bufArray[i];
            }
        }
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
                if (mapType == LINEAR_FRAME_MAP) {
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

    if (GetPendingInst(pCodecInst->coreIdx) == pCodecInst)
        wrPtr = VpuReadReg(pCodecInst->coreIdx, pEncInfo->streamWrPtrRegAddr);
    else {
        if (handle->productId == PRODUCT_ID_520 || handle->productId == PRODUCT_ID_525 || handle->productId == PRODUCT_ID_521) {
            EnterLock(pCodecInst->coreIdx);
            ProductVpuEncGetRdWrPtr(pCodecInst, &rdPtr, &wrPtr);
            LeaveLock(pCodecInst->coreIdx);
        }
        else
            wrPtr = pEncInfo->streamWrPtr;
    }

    if(pEncInfo->ringBufferEnable == 1 || pEncInfo->lineBufIntEn == 1) {
        if (wrPtr >= rdPtr) {
            room = wrPtr - rdPtr;
        }
        else {
            room = (pEncInfo->streamBufEndAddr - rdPtr) + (wrPtr - pEncInfo->streamBufStartAddr);
        }
    }
    else {
        if(wrPtr >= rdPtr)
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

    if (GetPendingInst(pCodecInst->coreIdx) == pCodecInst)
        wrPtr = VpuReadReg(pCodecInst->coreIdx, pEncInfo->streamWrPtrRegAddr);
    else
        wrPtr = pEncInfo->streamWrPtr;

    if ( rdPtr < wrPtr ) {
        if ( rdPtr + size  > wrPtr ) {
            return RETCODE_INVALID_PARAM;
        }
    }

    if (pEncInfo->ringBufferEnable == TRUE || pEncInfo->lineBufIntEn == TRUE) {
        if (handle->productId == PRODUCT_ID_520 || handle->productId == PRODUCT_ID_525 || handle->productId == PRODUCT_ID_521) {
            if (VPU_ALIGN16(wrPtr) == pEncInfo->streamRdPtr + pEncInfo->streamBufSize) {  // linebuffer full detected.
                EnterLock(pCodecInst->coreIdx);
                ProductVpuEncUpdateBitstreamBuffer(pCodecInst);
                LeaveLock(pCodecInst->coreIdx);
            }
        }
        else {
            rdPtr += size;
            if (rdPtr > pEncInfo->streamBufEndAddr) {
                if (pEncInfo->lineBufIntEn == TRUE) {
                    return RETCODE_INVALID_PARAM;
                }
                room = rdPtr - pEncInfo->streamBufEndAddr;
                rdPtr = pEncInfo->streamBufStartAddr;
                rdPtr += room;
            }
            if (rdPtr == pEncInfo->streamBufEndAddr)
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

    return RETCODE_SUCCESS;
}

RetCode VPU_EncStartOneFrame(
    EncHandle handle,
    EncParam * param 
    )
{
    CodecInst*          pCodecInst;
    EncInfo*            pEncInfo;
    RetCode             ret;
    VpuAttr*            pAttr   = NULL;
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

    if (param->srcIdx >= MAX_SRC_FRAME) {
        VLOG(ERR,"source frame index over range index %d \n", param->srcIdx);
        return RETCODE_INVALID_PARAM;
    }
    if (pEncInfo->srcBufUseIndex[param->srcIdx] == 1
        && param->srcEndFlag == 0) {
        VLOG(ERR,"source frame was already in encoding index %d \n",
                param->srcIdx);
        return RETCODE_INVALID_PARAM;
    }
    pSrcFrame = param -> sourceFrame;
    if (pSrcFrame && pSrcFrame->dma_buf_planes) { // use dma_buf
        vpu_dma_buf_info_t dma_info;
        int i;
        if (pSrcFrame->dma_buf_planes > 2)
            return RETCODE_INVALID_FRAME_BUFFER;
        osal_memset(&dma_info, 0x00, sizeof(vpu_dma_buf_info_t));
        dma_info.num_planes = pSrcFrame->dma_buf_planes;
        for (i = 0; i < pSrcFrame->dma_buf_planes; i ++)
                dma_info.fd[i] = pSrcFrame->dma_shared_fd[i];
        VLOG(INFO,"DMA source fd[%d, %d, %d] planes %d\n",
                dma_info.fd[0],dma_info.fd[1],dma_info.fd[2],
                pSrcFrame->dma_buf_planes);
        if (vdi_config_dma(pCodecInst->coreIdx, &dma_info) !=0)
                return RETCODE_INVALID_FRAME_BUFFER;
        pSrcFrame -> bufY = dma_info.phys_addr[0];
        if (dma_info.num_planes >1 )
                pSrcFrame -> bufCb = dma_info.phys_addr[1];
        if (dma_info.num_planes > 2)
               pSrcFrame->bufCr = dma_info.phys_addr[2];
        VLOG(INFO,"DMA frame physical bufY 0x%x Cb 0x%x Cr 0x%x planes %d \n",
                pSrcFrame->bufY, pSrcFrame->bufCb, pSrcFrame->bufCr,
                dma_info.num_planes);
    }

    ret = CheckEncParam(handle, param);
    if (ret != RETCODE_SUCCESS) {
        return ret;
    }

    pAttr = &g_VpuCoreAttributes[pCodecInst->coreIdx];

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

    if (pAttr->supportCommandQueue == TRUE) {
        SetPendingInst(pCodecInst->coreIdx, NULL);
        LeaveLock(pCodecInst->coreIdx);
    }
    else {
        SetPendingInst(pCodecInst->coreIdx, pCodecInst);
    }

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
    VpuAttr*    pAttr;
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
    pAttr      = &g_VpuCoreAttributes[pCodecInst->coreIdx];

    if (pAttr->supportCommandQueue == TRUE) {
        EnterLock(pCodecInst->coreIdx);
    }
    else {
        if (pCodecInst != GetPendingInst(pCodecInst->coreIdx)) {
            SetPendingInst(pCodecInst->coreIdx, 0);
            LeaveLock(pCodecInst->coreIdx);
            return RETCODE_WRONG_CALL_SEQUENCE;
        }
    }

    ret = ProductVpuEncGetResult(pCodecInst, info);

    if (ret == RETCODE_SUCCESS) {
        if (info->encSrcIdx >= 0 && info->reconFrameIndex >= 0 )
        {

            info->pts = pEncInfo->ptsMap[info->encSrcIdx];
             /* return the used soure frame */
            if (pEncInfo->srcBufUseIndex[info->encSrcIdx] == 1) {
                pSrcFrame = &(pEncInfo->srcBufMap[info->encSrcIdx]);
                info -> encSrcFrame = *pSrcFrame;
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
        vpu_dma_buf_info_t dma_info;
        int i;
        osal_memset(&dma_info, 0x00, sizeof(vpu_dma_buf_info_t));
        dma_info.num_planes = pSrcFrame->dma_buf_planes;
        for (i = 0; i < pSrcFrame->dma_buf_planes; i ++)
                dma_info.fd[i] = pSrcFrame->dma_shared_fd[i];
        dma_info.phys_addr[0] = pSrcFrame -> bufY;
        if (dma_info.num_planes > 1)
            dma_info.phys_addr[1]= pSrcFrame -> bufCb;
        if (dma_info.num_planes > 2)
            dma_info.phys_addr[2] = pSrcFrame -> bufCr;
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
            if ( !(mirDir == MIRDIR_NONE) && !(mirDir==MIRDIR_HOR) && !(mirDir==MIRDIR_VER) && !(mirDir==MIRDIR_HOR_VER)) {
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
            if (pCodecInst->codecMode == MP4_ENC ) {
                if (!( VOL_HEADER<=encHeaderParam->headerType && encHeaderParam->headerType <= VIS_HEADER)) {
                    return RETCODE_INVALID_PARAM;
                }
            } 
            else if  (pCodecInst->codecMode == AVC_ENC) {
                if (!( SPS_RBSP<=encHeaderParam->headerType && encHeaderParam->headerType <= PPS_RBSP_MVC)) {
                    return RETCODE_INVALID_PARAM;
                }
            }
            else if (pCodecInst->codecMode == W_HEVC_ENC || pCodecInst->codecMode == W_SVAC_ENC || pCodecInst->codecMode == W_AVC_ENC) {
                if (!( CODEOPT_ENC_VPS<=encHeaderParam->headerType && encHeaderParam->headerType <= (CODEOPT_ENC_VPS|CODEOPT_ENC_SPS|CODEOPT_ENC_PPS))) {
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
            if (pCodecInst->codecMode == W_HEVC_ENC || pCodecInst->codecMode == W_SVAC_ENC || pCodecInst->codecMode == W_AVC_ENC) {
                if (handle->productId == PRODUCT_ID_520 || handle->productId == PRODUCT_ID_525 || handle->productId == PRODUCT_ID_521)
                    return Vp5VpuEncGetHeader(handle, encHeaderParam);
                else
                    return RETCODE_INVALID_PARAM;
            }
        }        
    case ENC_SET_PARAM:
        {
            if (param == 0) {
                return RETCODE_INVALID_PARAM;
            }
            return RETCODE_INVALID_COMMAND;
        }
    case ENC_SET_GOP_NUMBER:
        {
            return RETCODE_INVALID_COMMAND;
        }
        break;
    case ENC_SET_INTRA_QP:
        {
            return RETCODE_INVALID_COMMAND;
        }
        break;
    case ENC_SET_BITRATE:
        {
            return RETCODE_INVALID_COMMAND;
        }
        break;
    case ENC_SET_FRAME_RATE:
        {
            return RETCODE_INVALID_COMMAND;
        }
        break;
    case ENC_SET_INTRA_MB_REFRESH_NUMBER:
        {
            return RETCODE_INVALID_COMMAND;
        }
        break;

    case ENC_SET_SLICE_INFO:
        {
            return RETCODE_INVALID_COMMAND;
        }
        break;
    case ENC_ENABLE_HEC:
        {
            return RETCODE_INVALID_COMMAND;
        }
        break;
    case ENC_DISABLE_HEC:
        {
            return RETCODE_INVALID_COMMAND;
        }
        break;
    case SET_SEC_AXI:
        {
            SecAxiUse secAxiUse;
            if (param == 0) {
                return RETCODE_INVALID_PARAM;
            }
            secAxiUse = *(SecAxiUse *)param;
            if (handle->productId == PRODUCT_ID_520 || handle->productId == PRODUCT_ID_525 || handle->productId == PRODUCT_ID_521) {
                pEncInfo->secAxiInfo.u.vp.useEncRdoEnable = secAxiUse.u.vp.useEncRdoEnable;
                pEncInfo->secAxiInfo.u.vp.useEncLfEnable  = secAxiUse.u.vp.useEncLfEnable;
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
            if (handle->productId == PRODUCT_ID_520 || handle->productId == PRODUCT_ID_525 || handle->productId == PRODUCT_ID_521)
                return Vp5VpuEncParaChange(handle, option);
            else
                return RETCODE_INVALID_PARAM;
        }
    case GET_BANDWIDTH_REPORT :
        return ProductVpuGetBandwidth(pCodecInst, (VPUBWData*)param);
    case ENC_GET_QUEUE_STATUS:
        {
            QueueStatusInfo* queueInfo = (QueueStatusInfo*)param;
            queueInfo->instanceQueueCount = pEncInfo->instanceQueueCount;
            queueInfo->totalQueueCount    = pEncInfo->totalQueueCount;
            break;
        }
    case ENC_WRPTR_SEL:
        {
            pEncInfo->encWrPtrSel = *(int *)param;
        }
        break;
    case SET_CYCLE_PER_TICK:
        {
            pEncInfo->cyclePerTick = *(Uint32 *)param;
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
        ret = ProductVpuAllocateFramebuffer(pCodecInst, frameBuffer, (TiledMapType)info.mapType, (Int32)info.num,
                                            info.stride, info.height, info.format, info.cbcrInterleave, FALSE,
                                            info.endian, &pEncInfo->vbFrame, gdiIndex, (FramebufferAllocType)info.type);
    }
    else {
        ret = RETCODE_INVALID_PARAM;
    }

    return ret; 
}

RetCode VPU_EncIssueSeqInit(EncHandle handle)
{
    CodecInst*  pCodecInst;
    RetCode     ret;
    VpuAttr*    pAttr;

    ret = CheckEncInstanceValidity(handle);
    if (ret != RETCODE_SUCCESS)
        return ret;

    pCodecInst = handle;

    EnterLock(pCodecInst->coreIdx);

    pAttr = &g_VpuCoreAttributes[pCodecInst->coreIdx];

    if (GetPendingInst(pCodecInst->coreIdx)) {
        LeaveLock(pCodecInst->coreIdx);
        return RETCODE_FRAME_NOT_COMPLETE;
    }

    ret = ProductVpuEncInitSeq(handle);
    if (ret == RETCODE_SUCCESS) {
        SetPendingInst(pCodecInst->coreIdx, pCodecInst);
    }

    if (pAttr->supportCommandQueue == TRUE) {
        SetPendingInst(pCodecInst->coreIdx, NULL);
        LeaveLock(pCodecInst->coreIdx);
    }

    return ret;
}

RetCode VPU_EncCompleteSeqInit(EncHandle handle, EncInitialInfo * info)
{
    CodecInst*  pCodecInst;
    EncInfo*    pEncInfo;
    RetCode     ret;
    VpuAttr*    pAttr;

    ret = CheckEncInstanceValidity(handle);
    if (ret != RETCODE_SUCCESS) {
        return ret;
    }

    if (info == 0) {
        return RETCODE_INVALID_PARAM;
    }

    pCodecInst = handle;
    pEncInfo = &pCodecInst->CodecInfo->encInfo;

    pAttr = &g_VpuCoreAttributes[pCodecInst->coreIdx];

    if (pAttr->supportCommandQueue == TRUE) {
        EnterLock(pCodecInst->coreIdx);
    }
    else {
        if (pCodecInst != GetPendingInst(pCodecInst->coreIdx)) {
            SetPendingInst(pCodecInst->coreIdx, 0);
            LeaveLock(pCodecInst->coreIdx);    
            return RETCODE_WRONG_CALL_SEQUENCE;
        }
    }

    ret = ProductVpuEncGetSeqInfo(handle, info);
    if (ret == RETCODE_SUCCESS) {
        pEncInfo->initialInfoObtained = 1;
    }

    //info->rdPtr = VpuReadReg(pCodecInst->coreIdx, pEncInfo->streamRdPtrRegAddr);
    //info->wrPtr = VpuReadReg(pCodecInst->coreIdx, pEncInfo->streamWrPtrRegAddr);

    //pEncInfo->prevFrameEndPos = info->rdPtr;

    pEncInfo->initialInfo = *info;

    SetPendingInst(pCodecInst->coreIdx, NULL);

    LeaveLock(pCodecInst->coreIdx);

    return ret;
}

 
