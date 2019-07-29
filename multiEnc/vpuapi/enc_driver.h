//--=========================================================================--
//  This file is a part of VPU Reference API project
//-----------------------------------------------------------------------------
//
//       This confidential and proprietary software may be used only
//     as authorized by a licensing agreement from Chips&Media Inc.
//     In the event of publication, the following notice is applicable:
//
//            (C) COPYRIGHT 2006 - 2011  CHIPS&MEDIA INC.
//                      ALL RIGHTS RESERVED
//
//       The entire notice above must be reproduced on all authorized
//       copies.
//
//--=========================================================================--
#ifndef __VP5_FUNCTION_H__
#define __VP5_FUNCTION_H__

#include "vpuapi.h"
#include "product.h"

#define VP5_TEMPBUF_OFFSET                (1024*1024)
#define VP5_TEMPBUF_SIZE                  (1024*1024)
#define VP5_TASK_BUF_OFFSET               (2*1024*1024)   // common mem = | codebuf(1M) | tempBuf(1M) | taskbuf0x0 ~ 0xF |



#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define VP5_SUBSAMPLED_ONE_SIZE(_w, _h)       ((((_w/4)+15)&~15)*(((_h/4)+7)&~7))
#define VP5_AVC_SUBSAMPLED_ONE_SIZE(_w, _h)   ((((_w/4)+31)&~31)*(((_h/4)+3)&~3))

#define BSOPTION_ENABLE_EXPLICIT_END        (1<<0)

#define WTL_RIGHT_JUSTIFIED          0
#define WTL_LEFT_JUSTIFIED           1
#define WTL_PIXEL_8BIT               0
#define WTL_PIXEL_16BIT              1
#define WTL_PIXEL_32BIT              2

extern Uint32 Vp5VpuIsInit(
    Uint32      coreIdx
    );

extern Int32 Vp5VpuIsBusy(
    Uint32 coreIdx
    );

extern Int32 WaveVpuGetProductId(
    Uint32      coreIdx
    );

extern RetCode Vp5VpuEncGiveCommand(
    CodecInst *pCodecInst,
    CodecCommand cmd,
    void *param
    );

extern void Vp5BitIssueCommand(
    CodecInst* instance,
    Uint32 cmd
    );

extern RetCode Vp5VpuGetVersion(
    Uint32  coreIdx,
    Uint32* versionInfo,
    Uint32* revision
    );

extern RetCode Vp5VpuInit(
    Uint32      coreIdx,
    void*       firmware,
    Uint32      size
    );

extern RetCode Vp5VpuSleepWake(
    Uint32 coreIdx, 
    int iSleepWake,
    const Uint16* code, 
    Uint32 size,
    BOOL reset
    );

extern RetCode Vp5VpuReset(
    Uint32 coreIdx, 
    SWResetMode resetMode
    );

extern RetCode Vp5VpuReInit(
    Uint32 coreIdx, 
    void* firmware, 
    Uint32 size
    );

extern Int32 Vp5VpuWaitInterrupt(
    CodecInst*  instance, 
    Int32       timeout,
    BOOL        pending
    );

extern RetCode Vp5VpuClearInterrupt(
    Uint32 coreIdx, 
    Uint32 flags
    );

extern RetCode Vp5VpuGetProductInfo(
    Uint32          coreIdx, 
    ProductInfo*    productInfo
    );

extern RetCode Vp5VpuGetBwReport(
    CodecInst*  instance,
    VPUBWData*  bwMon 
    );

/***< VP5 Encoder >******/
RetCode Vp5VpuEncUpdateBS(
    CodecInst* instance
    );

RetCode Vp5VpuEncGetRdWrPtr(CodecInst* instance, 
    PhysicalAddress *rdPtr, 
    PhysicalAddress *wrPtr
    );

extern RetCode Vp5VpuBuildUpEncParam(
    CodecInst* instance, 
    EncOpenParam* param
    );

extern RetCode Vp5VpuEncInitSeq(
    CodecInst*instance
    );

extern RetCode Vp5VpuEncGetSeqInfo(
    CodecInst* instance, 
    EncInitialInfo* info
    );

extern RetCode Vp5VpuEncRegisterFramebuffer(
    CodecInst* inst, 
    FrameBuffer* fbArr, 
    TiledMapType mapType, 
    Uint32 count
    );

extern RetCode Vp5EncWriteProtect(
    CodecInst* instance
    );

extern RetCode Vp5VpuEncode(
    CodecInst* instance,
    EncParam* option
    );

extern RetCode Vp5VpuEncGetResult(
    CodecInst* instance,
    EncOutputInfo* result
    );

extern RetCode Vp5VpuEncGetHeader(
    EncHandle instance, 
    EncHeaderParam * encHeaderParam
    );

extern RetCode Vp5VpuEncFiniSeq(
    CodecInst*  instance 
    );

extern RetCode Vp5VpuEncParaChange(
    EncHandle instance, 
    EncChangeParam* param
    );

extern RetCode CheckEncCommonParamValid(
    EncOpenParam* pop
    );

extern RetCode CheckEncRcParamValid(
    EncOpenParam* pop
    );

extern RetCode CheckEncCustomGopParamValid(
    EncOpenParam* pop
    );

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __VP5_FUNCTION_H__ */

