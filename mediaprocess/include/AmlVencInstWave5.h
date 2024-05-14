#ifndef __AML_VENC_INST_WAVE5_H_
#define __AML_VENC_INST_WAVE5_H_

#include "AmlVencInst.h"


namespace android {


class AmlVencInstWave5 : public AmlVencInst {

public:
    AmlVencInstWave5();
    virtual ~AmlVencInstWave5();

public:
    virtual bool init();
    virtual void ExtraInit(amvenc_info_t &VencInfo,amvenc_qp_param_t &QpParam);
    virtual void ExtraProcess();
    virtual void ExtraPreProcess(stInputFrameInfo &InputFrameInfo);
    virtual c2_status_t CheckPicSize(std::shared_ptr<const C2GraphicView> view,uint64_t frameIndex);
private:
    bool GetIsSupportSVC();
    int AVCProfileConvert(C2Config::profile_t profile);
    int AVCLevelConvert(C2Config::level_t Level);
    int HEVCLevelConvert(C2Config::level_t Level);
    void GenQpTable(stVencParam VencParam,amvenc_qp_param_t &QpParam);
private:
    bool mIsInit;
    int mActualFormat;
    int mWidthBackup;
    int mHeightBackup;
};



}


#endif


