
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
#ifndef __VPUAPI_PRODUCT_ABSTRACT_H__
#define __VPUAPI_PRODUCT_ABSTRACT_H__

#include "vpuapi.h"
#include "vpuapifunc.h"

typedef struct _tag_VpuAttrStruct {
    Uint32  productId;
    Uint32  productNumber;
    char    productName[8];
    Uint32  supportDecoders;            /* bitmask: See CodStd in vpuapi.h */
    Uint32  supportEncoders;            /* bitmask: See CodStd in vpuapi.h */
    Uint32  numberOfMemProtectRgns;     /* always 10 */
    Uint32  numberOfVCores;
    BOOL    supportWTL;
    BOOL    supportTiled2Linear;
    BOOL    supportTiledMap;
    BOOL    supportMapTypes;            /* Framebuffer map type */
    BOOL    supportLinear2Tiled;        /* Encoder */
    BOOL    support128bitBus;
    BOOL    supportThumbnailMode;
    BOOL    supportBitstreamMode;
    BOOL    supportFBCBWOptimization;   /* VPUxxx decoder feature */
    BOOL    supportGDIHW;
    BOOL    supportCommandQueue;
    BOOL    supportBackbone;            /* Enhanced version of GDI */
    BOOL    supportNewTimer;            /* 0 : timeeScale=32768, 1 : timerScale=256 (tick = cycle/timerScale) */
    BOOL    support2AlignScaler;
    Uint32  supportEndianMask;
    Uint32  framebufferCacheType;       /* 0: for None, 1 - Maverick-I, 2 - Maverick-II */
    Uint32  bitstreamBufferMargin;
    Uint32  interruptEnableFlag;
    Uint32  hwConfigDef0;
    Uint32  hwConfigDef1;
    Uint32  hwConfigFeature;            /**<< supported codec standards */
    Uint32  hwConfigDate;               /**<< Configuation Date Decimal, ex)20151130 */
    Uint32  hwConfigRev;                /**<< SVN revision, ex) 83521 */
    Uint32  hwConfigType;               /**<< Not defined yet */
} VpuAttr;

extern VpuAttr g_VpuCoreAttributes[MAX_NUM_VPU_CORE];

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
 * Returns 0 - not found product id
 *         1 - found product id
 */
extern Uint32 ProductVpuScan(
    Uint32 coreIdx
    );


extern Int32 ProductVpuGetId(
    Uint32 coreIdx
    );

extern RetCode ProductVpuGetVersion(
    Uint32  coreIdx, 
    Uint32* versionInfo, 
    Uint32* revision 
    );

extern RetCode ProductVpuGetProductInfo(
    Uint32  coreIdx, 
    ProductInfo* productInfo 
    );

extern RetCode ProductVpuInit(
    Uint32 coreIdx,
    void*  firmware,
    Uint32 size
    );

extern RetCode ProductVpuReInit(
    Uint32 coreIdx,
    void*  firmware,
    Uint32 size
    );

extern Uint32 ProductVpuIsInit(
    Uint32 coreIdx
    );

extern Int32 ProductVpuIsBusy(
    Uint32 coreIdx
    );

extern Int32 ProductVpuWaitInterrupt(
    CodecInst*  instance,
    Int32       timeout
    );

extern RetCode ProductVpuReset(
    Uint32      coreIdx,
    SWResetMode resetMode
    );

extern RetCode ProductVpuSleepWake(
    Uint32 coreIdx,
    int iSleepWake,
     const Uint16* code, 
     Uint32 size
    );

extern RetCode ProductVpuClearInterrupt(
    Uint32      coreIdx,
    Uint32      flags
    );

/**
 *  \brief      Allocate framebuffers with given parameters 
 */
extern RetCode ProductVpuAllocateFramebuffer(
    CodecInst*          instance, 
    FrameBuffer*        fbArr, 
    TiledMapType        mapType, 
    Int32               num, 
    Int32               stride, 
    Int32               height,
    FrameBufferFormat   format,
    BOOL                cbcrInterleave,
    BOOL                nv21,
    Int32               endian,
    vpu_buffer_t*       vb,
    Int32               gdiIndex,
    FramebufferAllocType fbType
    );

/**
 *  \brief      Register framebuffers to VPU
 */
extern RetCode ProductVpuRegisterFramebuffer(
    CodecInst*      instance
    );

extern Int32 ProductCalculateFrameBufSize(
    Int32               productId, 
    Int32               stride, 
    Int32               height, 
    TiledMapType        mapType, 
    FrameBufferFormat   format,
    BOOL                interleave,
    DRAMConfig*         pDramCfg
    );
extern Int32 ProductCalculateAuxBufferSize(
    AUX_BUF_TYPE    type, 
    CodStd          codStd, 
    Int32           width, 
    Int32           height
    );

extern RetCode ProductVpuGetBandwidth(
    CodecInst* instance, 
    VPUBWData* data
    );


/************************************************************************/
/* ENCODER                                                              */
/************************************************************************/
extern RetCode ProductVpuEncUpdateBitstreamBuffer(
    CodecInst* instance
    );

extern RetCode ProductVpuEncGetRdWrPtr(
    CodecInst* instance, 
    PhysicalAddress* rdPtr, 
    PhysicalAddress* wrPtr
    );

extern RetCode ProductVpuEncBuildUpOpenParam(
    CodecInst*      pCodec, 
    EncOpenParam*   param
    );

extern RetCode ProductVpuEncFiniSeq(
    CodecInst*      instance
    );

extern RetCode ProductCheckEncOpenParam(
    EncOpenParam*   param
    );

extern RetCode ProductVpuEncSetup(
    CodecInst*      instance
    );

extern RetCode ProductVpuEncode(
    CodecInst*      instance, 
    EncParam*       param
    );

extern RetCode ProductVpuEncGetResult(
    CodecInst*      instance, 
    EncOutputInfo*  result
    );

extern RetCode ProductVpuEncGiveCommand(
    CodecInst* instance, 
    CodecCommand cmd, 
    void* param);

extern RetCode ProductVpuEncInitSeq(
    CodecInst*  instance
    );
extern RetCode ProductVpuEncGetSeqInfo(
    CodecInst* instance, 
    EncInitialInfo* info
    );


#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /* __VPUAPI_PRODUCT_ABSTRACT_H__ */
 
