#ifndef __AML_VENC_INST_H_
#define __AML_VENC_INST_H_

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include "AmlVencInstIntf.h"
#include "amvenc.h"
#include "AmlVencInstHelper.h"
#include <am_gralloc_ext.h>


namespace android {


typedef struct DataModeInfo {
    unsigned long type;
    unsigned long pAddr;
    unsigned long canvas;
}DataModeInfo_t;


typedef enum _AVCProfile{
    MEDIA_AVC_BASELINE = 66,
    MEDIA_AVC_MAIN = 77,
    MEDIA_AVC_EXTENDED = 88,
    MEDIA_AVC_HIGH = 100,
    MEDIA_AVC_HIGH10 = 110,
    MEDIA_AVC_HIGH422 = 122,
    MEDIA_AVC_HIGH444 = 144
} AVCProfile;

typedef enum _AVCLevel{
    MEDIA_AVC_LEVEL_AUTO = 0,
    MEDIA_AVC_LEVEL1_B = 9,
    MEDIA_AVC_LEVEL1 = 10,
    MEDIA_AVC_LEVEL1_1 = 11,
    MEDIA_AVC_LEVEL1_2 = 12,
    MEDIA_AVC_LEVEL1_3 = 13,
    MEDIA_AVC_LEVEL2 = 20,
    MEDIA_AVC_LEVEL2_1 = 21,
    MEDIA_AVC_LEVEL2_2 = 22,
    MEDIA_AVC_LEVEL3 = 30,
    MEDIA_AVC_LEVEL3_1 = 31,
    MEDIA_AVC_LEVEL3_2 = 32,
    MEDIA_AVC_LEVEL4 = 40,
    MEDIA_AVC_LEVEL4_1 = 41,
    MEDIA_AVC_LEVEL4_2 = 42,
    MEDIA_AVC_LEVEL5 = 50,
    MEDIA_AVC_LEVEL5_1 = 51
} AVCLevel;


typedef enum _HEVCProfile
{
    MEDIA_HEVC_NONE = 0,
    MEDIA_HEVC_MAIN = 1,
    MEDIA_HEVC_MAIN10 = 2,
    MEDIA_HEVC_MAINSTILLPICTURE = 3,
    MEDIA_HEVC_MAINREXT = 4,
    MEDIA_HEVC_HIGHTHROUGHPUTREXT = 5
} HEVCProfile;

typedef enum {
    MEDIA_HEVC_TIER_MAIN = 0,
    MEDIA_HEVC_TIER_HIGH = 1
} HEVCTier;

typedef enum _HEVCLevel{
    MEDIA_HEVC_LEVEL_NONE = 0,
    MEDIA_HEVC_LEVEL1 = 30,
    MEDIA_HEVC_LEVEL2 = 60,
    MEDIA_HEVC_LEVEL2_1 = 63,
    MEDIA_HEVC_LEVEL3 = 90,
    MEDIA_HEVC_LEVEL3_1 = 93,
    MEDIA_HEVC_LEVEL4 = 120,
    MEDIA_HEVC_LEVEL4_1 = 123,
    MEDIA_HEVC_LEVEL5 = 150,
    MEDIA_HEVC_LEVEL5_1 = 153,
    MEDIA_HEVC_LEVEL5_2 = 156,
    MEDIA_HEVC_LEVEL6 = 180,
    MEDIA_HEVC_LEVEL6_1 = 183,
    MEDIA_HEVC_LEVEL6_2 = 186,
    MEDIA_HEVC_LEVEL8_5 = 255
} HEVCLevel;


class AmlVencInst : public IAmlVencInst {

public:
    AmlVencInst();
    virtual ~AmlVencInst();

public:
    virtual bool init();
    virtual bool GenerateHeader(char *pHeader,unsigned int &Length);
    virtual eResult PreProcess(std::shared_ptr<C2Buffer> inputBuffer,stInputFrameInfo &InputFrameInfo);
    virtual c2_status_t ProcessOneFrame(stInputFrameInfo InputFrameInfo,stOutputFrame &OutFrame);
    virtual bool Destroy();
    virtual void SetVencParamInst(IAmlVencParam *pVencParamInst);

public:
    virtual void ExtraInit(amvenc_info_t &VencInfo,amvenc_qp_param_t &QpParam);
    virtual void ExtraProcess();
    virtual void ExtraPreProcess(stInputFrameInfo &InputFrameInfo);
    virtual c2_status_t CheckPicSize(std::shared_ptr<const C2GraphicView> view,uint64_t frameIndex);
private:
    bool IsYUV420(const C2GraphicView &view);
    bool IsNV12(const C2GraphicView &view);
    bool IsNV21(const C2GraphicView &view);
    bool IsI420(const C2GraphicView &view);
    c2_status_t GraphicDataProc(std::shared_ptr<C2Buffer> inputBuffer,stInputFrameInfo *pFrameInfo);
    c2_status_t LinearDataProc(std::shared_ptr<const C2ReadView> view,stInputFrameInfo *pFrameInfo);
    c2_status_t DMAProc(const native_handle_t *priv_handle,stInputFrameInfo *pFrameInfo,uint32_t *dumpFileSize);
    c2_status_t ViewDataProc(std::shared_ptr<const C2GraphicView> view,stInputFrameInfo *pFrameInfo,uint32_t *dumpFileSize);
    c2_status_t CanvasDataProc(DataModeInfo_t *pDataMode,stInputFrameInfo *pFrameInfo);
    c2_status_t genVuiParam(stVencParam VencParam,int32_t *primaries,int32_t *transfer,int32_t *matrixCoeffs,bool *range);
protected:
    bool InitEncoder();
    amvenc_img_format_t PixelFormatConvert(int FormatIn);

protected:
    IAmlVencParam *mVencParamInst;
    AmlVencInstHelper *mHelper;
    amvenc_handle_t mVencHandle;
    bool mRequestSync;
    //int mFrameRate;
    int mBitRate;
    bool mIsChangeResolutionInternal;
};



}







#endif

