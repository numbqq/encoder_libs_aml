//#define LOG_NDEBUG 0
#define LOG_TAG "AMLVenc"
#include <utils/Log.h>

#include<stdio.h>
#include <media/hardware/VideoAPI.h>
#include <Codec2Mapper.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MediaErrors.h>
#include <media/stagefright/MetaData.h>
#include <media/stagefright/foundation/AUtils.h>
#include <ui/GraphicBuffer.h>
#include <C2AllocatorGralloc.h>
#include "AmlVencInstIntf.h"
#include "AmlVencInst.h"


namespace android {

static unsigned int gloglevel = 1;

#define CODEC2_LOG_ERR 0
#define CODEC2_LOG_INFO 1
#define CODEC2_LOG_TAG_BUFFER 2
#define CODEC2_LOG_DEBUG_LEVEL1 4
#define CODEC2_LOG_DEBUG_LEVEL2 8
#define CODEC2_LOG_TRACE 16

#define CODEC2_VENC_LOG_ERR    CODEC2_LOG_ERR
#define CODEC2_VENC_LOG_INFO   CODEC2_LOG_INFO
#define CODEC2_VENC_LOG_DEBUG   CODEC2_LOG_DEBUG_LEVEL1
#define CODEC2_VENC_LOG_TAG_BUFFER  CODEC2_LOG_TAG_BUFFER
#define CODEC2_VENC_LOG_TRACE  CODEC2_LOG_TRACE

#define CODEC2_LOG(level, f, s...) \
do { \
    if (level & gloglevel) { \
        if (level == CODEC2_LOG_INFO) \
            ALOGI(f, ##s); \
        else if (level == CODEC2_LOG_DEBUG_LEVEL1 || level == CODEC2_LOG_DEBUG_LEVEL2) \
            ALOGD(f, ##s); \
        else if (level == CODEC2_LOG_TAG_BUFFER) \
            ALOGD(f, ##s); \
    } else { \
        if (level == CODEC2_LOG_ERR) \
            ALOGE(f, ##s); \
    } \
} while(0)


#define AMLVenc_LOG(level, fmt, str...) CODEC2_LOG(level, fmt, ##str)


#define USE_CONTINUES_PHYBUFF(h) (am_gralloc_get_usage(h) & GRALLOC_USAGE_HW_VIDEO_ENCODER)
#define align_32(x)  ((((x)+31)>>5)<<5)


#define kMetadataBufferTypeCanvasSource 3
#define kMetadataBufferTypeANWBuffer 2

#define CANVAS_MODE_ENABLE  1


AmlVencInst::AmlVencInst()
            :mVencParamInst(NULL),
             mHelper(NULL),
             mVencHandle(0),
             mRequestSync(false),
             mIsChangeResolutionInternal(false) {
    ALOGD("AmlVencInst constructor!");
    gloglevel = 255;
    mHelper = new AmlVencInstHelper;
}

AmlVencInst::~AmlVencInst() {
    ALOGD("~AmlVencInst destructor!");
    Destroy();
    if (mHelper) {
        delete mHelper;
        mHelper = NULL;
    }
}


amvenc_img_format_t AmlVencInst::PixelFormatConvert(int FormatIn) {
    amvenc_img_format_t FormatOut;
    ALOGE("FORMAT input:%d",FormatIn);
    switch (FormatIn) {
        case HAL_PIXEL_FORMAT_YCbCr_420_888:
        case HAL_PIXEL_FORMAT_YCrCb_420_SP: {
            FormatOut = AML_IMG_FMT_NV21;
            ALOGE("AML_IMG_FMT_NV21");
            break;
        }
        case HAL_PIXEL_FORMAT_RGBA_8888:
        case HAL_PIXEL_FORMAT_RGB_888:
            FormatOut = AML_IMG_FMT_RGBA8888;
            ALOGE("AML_IMG_FMT_RGBA8888");
            break;
        case HAL_PIXEL_FORMAT_YV12:
            FormatOut = AML_IMG_FMT_YUV420P;
            ALOGE("AML_IMG_FMT_YUV420P");
            break;
        default: {
            ALOGE("cannot support colorformat:%x,use default nv21",FormatIn);
            FormatOut = AML_IMG_FMT_NV21;
            break;
        }
    }
    return FormatOut;
}


c2_status_t AmlVencInst::genVuiParam(stVencParam VencParam,int32_t *primaries,int32_t *transfer,int32_t *matrixCoeffs,bool *range)
{
    ColorAspects sfAspects;
    if (!C2Mapper::map(VencParam.ColorAspect.primaries, &sfAspects.mPrimaries)) {
        sfAspects.mPrimaries = android::ColorAspects::PrimariesUnspecified;
    }
    if (!C2Mapper::map(VencParam.ColorAspect.range, &sfAspects.mRange)) {
        sfAspects.mRange = android::ColorAspects::RangeUnspecified;
    }
    if (!C2Mapper::map(VencParam.ColorAspect.matrix, &sfAspects.mMatrixCoeffs)) {
        sfAspects.mMatrixCoeffs = android::ColorAspects::MatrixUnspecified;
    }
    if (!C2Mapper::map(VencParam.ColorAspect.transfer, &sfAspects.mTransfer)) {
        sfAspects.mTransfer = android::ColorAspects::TransferUnspecified;
    }
    ColorUtils::convertCodecColorAspectsToIsoAspects(sfAspects,primaries,transfer,matrixCoeffs,range);
    ALOGI("vui info:primaries:%d,transfer:%d,matrixCoeffs:%d,range:%d",*primaries,*transfer,*matrixCoeffs,(int)(*range));
    return C2_OK;
}


bool AmlVencInst::InitEncoder() {
    stVencParam VencParam;
    amvenc_codec_id_t CodecId = AML_CODEC_ID_NONE;
    amvenc_info_t VencInfo;
    amvenc_qp_param_t QpParam;

    memset(&VencParam,0,sizeof(VencParam));
    memset(&QpParam,0,sizeof(QpParam));

    mVencParamInst->GetVencParam(VencParam);
    CodecId = (mVencParamInst->GetCodecType() == H265) ? AML_CODEC_ID_H265 : AML_CODEC_ID_H264;

    VencInfo.width = VencParam.Width;
    VencInfo.height = VencParam.Height;
    VencInfo.frame_rate = VencParam.FrameRate;
    VencInfo.bit_rate = VencParam.Bitrate;
    VencInfo.img_format = PixelFormatConvert(VencParam.PixFormat);

    mBitRate = VencParam.Bitrate;

    if (C2_OK == genVuiParam(VencParam,(int32_t *)&VencInfo.colour_primaries,(int32_t *)&VencInfo.transfer_characteristics,(int32_t *)&VencInfo.matrix_coefficients,(bool *)&VencInfo.video_full_range_flag)) {
        VencInfo.vui_parameters_present_flag = 1;
        VencInfo.video_signal_type_present_flag = 1;
        VencInfo.colour_description_present_flag = 1;
        ALOGD("enable vui info");
    }

    ExtraInit(VencInfo,QpParam); //extra init action for each ip core

    ALOGE("CodecId:%d",CodecId);
    AMLVenc_LOG(CODEC2_VENC_LOG_INFO,"init param width:%d,height:%d,frame_rate:%d,bit_rate:%d,img_format:%d,qp_mode:%d",
                                     VencInfo.width,VencInfo.height,VencInfo.frame_rate,VencInfo.bit_rate,VencInfo.img_format,VencInfo.qp_mode);
    AMLVenc_LOG(CODEC2_VENC_LOG_INFO,"qp info min_qp:%d,max_qp:%d,imin:%d,imax:%d,pmin:%d,pmax:%d",
                                     QpParam.qp_min,QpParam.qp_max,QpParam.qp_I_min,QpParam.qp_I_max,QpParam.qp_P_min,QpParam.qp_P_max);
    mVencHandle = amvenc_init(CodecId,VencInfo,&QpParam);
    if (!mVencHandle) {
        ALOGE("init amlvenc failed!!!");
        return false;
    }
    ALOGE("init amvenc success!!");
    mHelper->SetCodecType(mVencParamInst->GetCodecType());
    mHelper->SetCodecHandle(mVencHandle);
    mHelper->OpenFile(DUMP_RAW);
    mHelper->OpenFile(DUMP_ES);
    return true;
}


bool AmlVencInst::init() {
    return InitEncoder();
}


void AmlVencInst::ExtraInit(amvenc_info_t &VencInfo,amvenc_qp_param_t &QpParam) {
    return;
}



bool AmlVencInst::IsYUV420(const C2GraphicView &view) {
    const C2PlanarLayout &layout = view.layout();
    return (layout.numPlanes == 3
            && layout.type == C2PlanarLayout::TYPE_YUV
            && layout.planes[layout.PLANE_Y].channel == C2PlaneInfo::CHANNEL_Y
            && layout.planes[layout.PLANE_Y].allocatedDepth == 8
            && layout.planes[layout.PLANE_Y].bitDepth == 8
            && layout.planes[layout.PLANE_Y].rightShift == 0
            && layout.planes[layout.PLANE_Y].colSampling == 1
            && layout.planes[layout.PLANE_Y].rowSampling == 1
            && layout.planes[layout.PLANE_U].channel == C2PlaneInfo::CHANNEL_CB
            && layout.planes[layout.PLANE_U].allocatedDepth == 8
            && layout.planes[layout.PLANE_U].bitDepth == 8
            && layout.planes[layout.PLANE_U].rightShift == 0
            && layout.planes[layout.PLANE_U].colSampling == 2
            && layout.planes[layout.PLANE_U].rowSampling == 2
            && layout.planes[layout.PLANE_V].channel == C2PlaneInfo::CHANNEL_CR
            && layout.planes[layout.PLANE_V].allocatedDepth == 8
            && layout.planes[layout.PLANE_V].bitDepth == 8
            && layout.planes[layout.PLANE_V].rightShift == 0
            && layout.planes[layout.PLANE_V].colSampling == 2
            && layout.planes[layout.PLANE_V].rowSampling == 2);
}

bool AmlVencInst::IsNV12(const C2GraphicView &view) {
    if (!IsYUV420(view)) {
        return false;
    }
    const C2PlanarLayout &layout = view.layout();
    return (layout.rootPlanes == 2
            && layout.planes[layout.PLANE_U].colInc == 2
            && layout.planes[layout.PLANE_U].rootIx == layout.PLANE_U
            && layout.planes[layout.PLANE_U].offset == 0
            && layout.planes[layout.PLANE_V].colInc == 2
            && layout.planes[layout.PLANE_V].rootIx == layout.PLANE_U
            && layout.planes[layout.PLANE_V].offset == 1);
}

bool AmlVencInst::IsNV21(const C2GraphicView &view) {
    if (!IsYUV420(view)) {
        return false;
    }
    const C2PlanarLayout &layout = view.layout();
    return (layout.rootPlanes == 2
            && layout.planes[layout.PLANE_U].colInc == 2
            && layout.planes[layout.PLANE_U].rootIx == layout.PLANE_V
            && layout.planes[layout.PLANE_U].offset == 1
            && layout.planes[layout.PLANE_V].colInc == 2
            && layout.planes[layout.PLANE_V].rootIx == layout.PLANE_V
            && layout.planes[layout.PLANE_V].offset == 0);
}

bool AmlVencInst::IsI420(const C2GraphicView &view) {
    if (!IsYUV420(view)) {
        return false;
    }
    const C2PlanarLayout &layout = view.layout();
    return (layout.rootPlanes == 3
            && layout.planes[layout.PLANE_U].colInc == 1
            && layout.planes[layout.PLANE_U].rootIx == layout.PLANE_U
            && layout.planes[layout.PLANE_U].offset == 0
            && layout.planes[layout.PLANE_V].colInc == 1
            && layout.planes[layout.PLANE_V].rootIx == layout.PLANE_V
            && layout.planes[layout.PLANE_V].offset == 0);
}



c2_status_t AmlVencInst::LinearDataProc(std::shared_ptr<const C2ReadView> view,stInputFrameInfo *pFrameInfo) {
    //private_handle_t *priv_handle = NULL;
//    uint32_t dumpFileSize = 0;
    stVencParam VencParam;

    if ( nullptr == pFrameInfo) {
        return C2_BAD_VALUE;
    }

    ALOGE("datalen:%d",view->capacity());
    DataModeInfo_t *pDataMode = (DataModeInfo_t *)(view->data());
    AMLVenc_LOG(CODEC2_VENC_LOG_INFO,"type:%lx",pDataMode->type);

    if (kMetadataBufferTypeCanvasSource == pDataMode->type && sizeof(DataModeInfo_t) == view->capacity()) {
        return CanvasDataProc(pDataMode,pFrameInfo);
    }
    else if(kMetadataBufferTypeANWBuffer == pDataMode->type && sizeof(DataModeInfo_t) == view->capacity()) {
        #if 0
        ALOGE("111111,pDataMode:%p",pDataMode);
        ANativeWindowBuffer *pNativeWindow = (ANativeWindowBuffer *)pDataMode->pAddr;
        int fence_id = pDataMode->canvas;
        ALOGE("111111,pNativeWindow:%p,fence_id:%d",pNativeWindow,fence_id);
        priv_handle = (private_handle_t *)pNativeWindow->handle;
        C2Venc_LOG(CODEC2_VENC_LOG_INFO,"native window source,handle:%p",priv_handle);
        //int share_fd = priv_handle->share_fd;
        //C2Venc_LOG(CODEC2_VENC_LOG_INFO,"share_fd:%d",priv_handle->share_fd);
        if (priv_handle && priv_handle->share_fd && USE_CONTINUES_PHYBUFF(priv_handle) && isSupportDMA()) {
            //const C2PlanarLayout &layout = view->layout();

            c2_status_t err = intf()->query_vb({&PicSize,&InputFmt},{},C2_DONT_BLOCK,nullptr);
            if (err == C2_BAD_INDEX) {
                if (!PicSize || !InputFmt) {
                    C2Venc_LOG(CODEC2_VENC_LOG_ERR,"get PicSize or InputFmt failed!!");
                    return C2_BAD_VALUE;
                }
            } else if (err != C2_OK) {
                C2Venc_LOG(CODEC2_VENC_LOG_ERR,"get subclass param failed!!");
                return C2_BAD_VALUE;
            }
            pFrameInfo->bufType = DMA;
            pFrameInfo->shareFd[0] = priv_handle->share_fd;
            pFrameInfo->shareFd[1] = -1;
            pFrameInfo->shareFd[2] = -1;
            pFrameInfo->planeNum = 1;
            pFrameInfo->yStride = priv_handle->plane_info[0].byte_stride;
            pFrameInfo->uStride = priv_handle->plane_info[1].byte_stride;
            pFrameInfo->vStride = priv_handle->plane_info[2].byte_stride;
            int format = priv_handle->format;
            switch (format) {
                case HAL_PIXEL_FORMAT_RGBA_8888:
                case HAL_PIXEL_FORMAT_RGB_888:
                    pFrameInfo->colorFmt = C2_ENC_FMT_RGBA8888;
                    dumpFileSize = pFrameInfo->yStride * PicSize.height;
                    break;
                case HAL_PIXEL_FORMAT_YCbCr_420_888:
                case HAL_PIXEL_FORMAT_YCrCb_420_SP:
                    pFrameInfo->colorFmt = C2_ENC_FMT_NV21;
                    dumpFileSize = pFrameInfo->yStride * PicSize.height * 3 / 2;
                    break;
                case HAL_PIXEL_FORMAT_YV12:
                    pFrameInfo->colorFmt = C2_ENC_FMT_YV12;
                    dumpFileSize = pFrameInfo->yStride * PicSize.height * 3 / 2;
                    break;
                default:
                    pFrameInfo->colorFmt = C2_ENC_FMT_NV21;
                    dumpFileSize = pFrameInfo->yStride * PicSize.height * 3 / 2;
                    C2Venc_LOG(CODEC2_VENC_LOG_ERR,"cannot find support fmt,default:%d",pFrameInfo->colorFmt);
                    break;
            }
            C2Venc_LOG(CODEC2_VENC_LOG_DEBUG,"native window source:yStride:%d,uStride:%d,vStride:%d,view->width():%d,view->height():%d,plane num:%d",
                pFrameInfo->yStride,pFrameInfo->uStride,pFrameInfo->vStride,PicSize.width,PicSize.height,pFrameInfo->planeNum);
        }
        #endif
    }
    else {  //linear picture buffer
        memset(&VencParam,0,sizeof(VencParam));
        mVencParamInst->GetVencParam(VencParam);

        AMLVenc_LOG(CODEC2_VENC_LOG_INFO,"get width:%d,height:%d,colorfmt:%d,ptr:%p",VencParam.Width,VencParam.Height,VencParam.PixFormat,const_cast<uint8_t *>(view->data()));
        pFrameInfo->bufType = VMALLOC;
        pFrameInfo->colorFmt = PixelFormatConvert(VencParam.PixFormat);
        pFrameInfo->yPlane = const_cast<uint8_t *>(view->data());
        pFrameInfo->uPlane = pFrameInfo->yPlane + VencParam.Width * VencParam.Height;
        pFrameInfo->vPlane = pFrameInfo->uPlane;
        pFrameInfo->yStride = VencParam.Width;
        pFrameInfo->uStride = VencParam.Width;
        pFrameInfo->vStride = VencParam.Width;
    }
    return C2_OK;
}


c2_status_t AmlVencInst::DMAProc(const native_handle_t *priv_handle,stInputFrameInfo *pFrameInfo,uint32_t *dumpFileSize) {
    stVencParam VencParam;

    if (NULL == priv_handle || NULL == pFrameInfo || NULL == dumpFileSize) {
        return C2_BAD_VALUE;
    }
    memset(&VencParam,0,sizeof(VencParam));
    mVencParamInst->GetVencParam(VencParam);

    pFrameInfo->bufType = DMA;
    pFrameInfo->shareFd[0] = am_gralloc_get_buffer_fd(priv_handle);
    pFrameInfo->shareFd[1] = -1;
    pFrameInfo->shareFd[2] = -1;
    pFrameInfo->planeNum = 1;
    pFrameInfo->yStride = am_gralloc_get_stride_in_byte(priv_handle);
    pFrameInfo->uStride = am_gralloc_get_stride_in_byte(priv_handle);
    pFrameInfo->vStride = am_gralloc_get_stride_in_byte(priv_handle);
    pFrameInfo->HStride = am_gralloc_get_aligned_height(priv_handle);
    AMLVenc_LOG(CODEC2_VENC_LOG_DEBUG,"actual height:%d",pFrameInfo->HStride);
    int format = am_gralloc_get_format(priv_handle);
    pFrameInfo->colorFmt = PixelFormatConvert(format);
    if (AML_IMG_FMT_RGBA8888 == pFrameInfo->colorFmt) {
        pFrameInfo->yStride = am_gralloc_get_stride_in_byte(priv_handle) / 4;
        (*dumpFileSize) = pFrameInfo->yStride * pFrameInfo->HStride * 4;
    }
    else {
        (*dumpFileSize) = pFrameInfo->yStride * pFrameInfo->HStride * 3 / 2;
    }
    AMLVenc_LOG(CODEC2_VENC_LOG_DEBUG,"yStride:%d,uStride:%d,vStride:%d,view->width():%d,view->height():%d,plane num:%d",
    pFrameInfo->yStride,pFrameInfo->uStride,pFrameInfo->vStride,VencParam.Width,pFrameInfo->HStride,pFrameInfo->planeNum);

    return C2_OK;
}


c2_status_t AmlVencInst::GraphicDataProc(std::shared_ptr<C2Buffer> inputBuffer,stInputFrameInfo *pFrameInfo) {
    std::shared_ptr<const C2GraphicView> view;
    uint32_t dumpFileSize = 0;
    c2_status_t res = C2_OK;
    stVencParam VencParam;

    if (nullptr == inputBuffer.get() || nullptr == pFrameInfo ) {
        AMLVenc_LOG(CODEC2_VENC_LOG_ERR,"GraphicDataProc bad parameter!!");
        return C2_BAD_VALUE;
    }

    const native_handle_t* c2Handle = inputBuffer->data().graphicBlocks().front().handle();
    native_handle_t *handle = UnwrapNativeCodec2GrallocHandle(c2Handle);

    if (handle && am_gralloc_get_buffer_fd(handle) && USE_CONTINUES_PHYBUFF(handle) && mVencParamInst->GetIsSupportDMAMode()/*isSupportDMA()*/) {
        res = DMAProc(handle,pFrameInfo,&dumpFileSize);
        if (C2_OK != res) {
            goto fail_process;
        }
    }
    else {
        view = std::make_shared<const C2GraphicView>(inputBuffer->data().graphicBlocks().front().map().get());
        if (C2_OK != view->error() || nullptr == view.get()) {
            AMLVenc_LOG(CODEC2_VENC_LOG_ERR,"graphic view map err = %d or view is null", view->error());
            res = C2_BAD_VALUE;
            goto fail_process;
        }

        memset(&VencParam,0,sizeof(VencParam));
        mVencParamInst->GetVencParam(VencParam);
        if (VencParam.canvas_mode == CANVAS_MODE_ENABLE) {
            DataModeInfo_t *pDataMode = (DataModeInfo_t *)(view->data()[C2PlanarLayout::PLANE_Y]);
            if (kMetadataBufferTypeCanvasSource == pDataMode->type) {
                AMLVenc_LOG(CODEC2_VENC_LOG_INFO,"enter canvas mode!!");
                res = CanvasDataProc(pDataMode,pFrameInfo);
                if (C2_OK != res) {
                    AMLVenc_LOG(CODEC2_VENC_LOG_INFO,"CanvasDataProc failed!!!");
                    goto fail_process;
                }
            }
            else {
                AMLVenc_LOG(CODEC2_VENC_LOG_ERR,"canvas mode,but the data is not right!!!,type:%lx,canvas:%lx",pDataMode->type,pDataMode->canvas);
                res = C2_BAD_VALUE;
                goto fail_process;
            }
        }
        else {
            res = ViewDataProc(view,pFrameInfo,&dumpFileSize);
            if (C2_OK != res) {
                AMLVenc_LOG(CODEC2_VENC_LOG_ERR,"ViewDataProc failed!!!");
                goto fail_process;
            }
        }
    }

    if (pFrameInfo->bufType == DMA) {
        uint8_t *pVirAddr = NULL;
        pVirAddr = (uint8_t *)mmap( NULL, dumpFileSize,PROT_READ | PROT_WRITE, MAP_SHARED, pFrameInfo->shareFd[0], 0);
        if (pVirAddr) {
            AMLVenc_LOG(CODEC2_VENC_LOG_DEBUG,"dma mode,viraddr: %p", pVirAddr);
            mHelper->dumpDataToFile(DUMP_RAW,pVirAddr,dumpFileSize);
            munmap(pVirAddr, dumpFileSize);
        }
    }
    else {
        mHelper->dumpDataToFile(DUMP_RAW,pFrameInfo->yPlane,dumpFileSize);
    }
    return C2_OK;

fail_process:
    if (handle) {
        native_handle_delete(handle);
    }
    return res;
}


c2_status_t AmlVencInst::CheckPicSize(std::shared_ptr<const C2GraphicView> view,uint64_t frameIndex) {
    stVencParam VencParam;

    memset(&VencParam,0,sizeof(stVencParam));
    mVencParamInst->GetVencParam(VencParam);

    if (VencParam.Width != view->width() || VencParam.Height != view->height()) {
        if (0 == frameIndex) {  //first frame is normally
            AMLVenc_LOG(CODEC2_VENC_LOG_ERR,"pic size :%d x %d,this buffer is:%d x %d,change pic size...",VencParam.Width,VencParam.Height,view->width(),view->height());
            mVencParamInst->UpdatePicSize(view->width(), view->height());
        }
        else {
            AMLVenc_LOG(CODEC2_VENC_LOG_ERR,"buffer is not valid,pic size :%d x %d,but this buffer is:%d x %d",VencParam.Width,VencParam.Height,view->width(),view->height());
            return C2_BAD_VALUE;
        }
    }

    return C2_OK;
}


c2_status_t AmlVencInst::ViewDataProc(std::shared_ptr<const C2GraphicView> view,stInputFrameInfo *pFrameInfo,uint32_t *dumpFileSize) {
    if (NULL == view || NULL == pFrameInfo || NULL == dumpFileSize) {
        return C2_BAD_VALUE;
    }
    const C2PlanarLayout &layout = view->layout();
    pFrameInfo->bufType = VMALLOC;
    pFrameInfo->yPlane = const_cast<uint8_t *>(view->data()[C2PlanarLayout::PLANE_Y]);
    pFrameInfo->uPlane = const_cast<uint8_t *>(view->data()[C2PlanarLayout::PLANE_U]);
    pFrameInfo->vPlane = const_cast<uint8_t *>(view->data()[C2PlanarLayout::PLANE_V]);
    pFrameInfo->yStride = layout.planes[C2PlanarLayout::PLANE_Y].rowInc;
    pFrameInfo->uStride = layout.planes[C2PlanarLayout::PLANE_U].rowInc;
    pFrameInfo->vStride = layout.planes[C2PlanarLayout::PLANE_V].rowInc;
    (*dumpFileSize) = view->width() * view->height() * 3 / 2;

    if (C2_OK != CheckPicSize(view,pFrameInfo->frameIndex)) {
        return C2_BAD_VALUE;
    }

    AMLVenc_LOG(CODEC2_VENC_LOG_DEBUG,"yStride:%d,uStride:%d,vStride:%d,view->width():%d,view->height():%d,root plane num:%d,component plan num:%d",
        pFrameInfo->yStride,pFrameInfo->uStride,pFrameInfo->vStride,view->width(),view->height(),layout.rootPlanes,layout.numPlanes);
    switch (layout.type) {
        case C2PlanarLayout::TYPE_RGB:
        case C2PlanarLayout::TYPE_RGBA: {
            AMLVenc_LOG(CODEC2_VENC_LOG_DEBUG,"TYPE_RGBA or TYPE_RGB");
            pFrameInfo->yPlane = const_cast<uint8_t *>(view->data()[C2PlanarLayout::PLANE_R]);
            pFrameInfo->uPlane = const_cast<uint8_t *>(view->data()[C2PlanarLayout::PLANE_G]);
            pFrameInfo->vPlane = const_cast<uint8_t *>(view->data()[C2PlanarLayout::PLANE_B]);
            pFrameInfo->yStride = layout.planes[C2PlanarLayout::PLANE_R].rowInc / 4;
            pFrameInfo->uStride = layout.planes[C2PlanarLayout::PLANE_G].rowInc;
            pFrameInfo->vStride = layout.planes[C2PlanarLayout::PLANE_B].rowInc;
            (*dumpFileSize) = view->width() * view->height() * 4;
            pFrameInfo->colorFmt = AML_IMG_FMT_RGBA8888;
            break;
        }
        case C2PlanarLayout::TYPE_YUV: {
            AMLVenc_LOG(CODEC2_VENC_LOG_DEBUG,"TYPE_YUV");
            if (IsNV12(*view.get())) {
                pFrameInfo->colorFmt = AML_IMG_FMT_NV12;
                AMLVenc_LOG(CODEC2_VENC_LOG_DEBUG,"InputFrameInfo colorfmt :AML_IMG_FMT_NV12");
            }
            else if (IsNV21(*view.get())) {
                pFrameInfo->colorFmt = AML_IMG_FMT_NV21;
                AMLVenc_LOG(CODEC2_VENC_LOG_DEBUG,"InputFrameInfo colorfmt :AML_IMG_FMT_NV21");
            }
            else if (IsI420(*view.get())) {
                pFrameInfo->colorFmt = AML_IMG_FMT_YUV420P;
                AMLVenc_LOG(CODEC2_VENC_LOG_DEBUG,"InputFrameInfo colorfmt :AML_IMG_FMT_YUV420P");
            }
            else {
                AMLVenc_LOG(CODEC2_VENC_LOG_ERR,"type yuv,but not support fmt!!!");
            }
            (*dumpFileSize) = pFrameInfo->yStride * view->height() * 3 / 2;
            break;
        }
        case C2PlanarLayout::TYPE_YUVA: {
            AMLVenc_LOG(CODEC2_VENC_LOG_ERR,"TYPE_YUVA not support!!");
            break;
        }
        default:
            AMLVenc_LOG(CODEC2_VENC_LOG_ERR,"layout.type:%d",layout.type);
            break;
    }

    //pFrameInfo->colorFmt = AML_IMG_FMT_YUV420P;
    return C2_OK;
}



c2_status_t AmlVencInst::CanvasDataProc(DataModeInfo_t *pDataMode,stInputFrameInfo *pFrameInfo) {
    if (pFrameInfo == nullptr) {
        return C2_BAD_VALUE;
    }

    pFrameInfo->bufType = CANVAS;
    pFrameInfo->canvas = pDataMode->canvas;
    pFrameInfo->yPlane = (uint8_t *)pDataMode->pAddr;
    pFrameInfo->uPlane = 0;
    pFrameInfo->vPlane = 0;
    AMLVenc_LOG(CODEC2_VENC_LOG_ERR,"canvas mode,canvas id:%" PRId64"",pFrameInfo->canvas);
    return C2_OK;
}


bool AmlVencInst::GenerateHeader(char *pHeader,unsigned int &Length) {
    amvenc_metadata_t ret;

    memset(&ret,0,sizeof(ret));
    if (NULL == pHeader) {
        return false;
    }
    ret = amvenc_generate_header(mVencHandle,(unsigned char *)pHeader,&Length);
    if (!ret.is_valid) {
        return false;
    }
    mHelper->dumpDataToFile(DUMP_ES,(uint8_t *)pHeader,Length);
    AMLVenc_LOG(CODEC2_VENC_LOG_ERR,"header len:%d",Length);
    return true;
}



eResult AmlVencInst::PreProcess(std::shared_ptr<C2Buffer> inputBuffer,stInputFrameInfo &InputFrameInfo) {
    std::shared_ptr<const C2ReadView> Linearview;

    int type = inputBuffer->data().type();

    AMLVenc_LOG(CODEC2_VENC_LOG_DEBUG,"inputbuffer type:%d",type);

    if (C2BufferData::GRAPHIC == type) {
        if (C2_OK != GraphicDataProc(inputBuffer,&InputFrameInfo)) {
            AMLVenc_LOG(CODEC2_VENC_LOG_ERR,"graphic buffer proc failed!!");
            return WORK_IS_VALID;
        }
    }
    else if(C2BufferData::LINEAR == type) {
        Linearview = std::make_shared<const C2ReadView>(inputBuffer->data().linearBlocks().front().map().get());
        if (C2_OK != Linearview->error() || nullptr == Linearview.get()) {
            AMLVenc_LOG(CODEC2_VENC_LOG_ERR,"linear view map err = %d or view is null", Linearview->error());
            return FAIL;
        }
        if (C2_OK != LinearDataProc(Linearview,&InputFrameInfo)) {
            AMLVenc_LOG(CODEC2_VENC_LOG_ERR,"linear buffer proc failed!!");
            return WORK_IS_VALID;
        }
    }
    else {
        AMLVenc_LOG(CODEC2_VENC_LOG_ERR,"invalid data type:%d!!",type);
        return FAIL;
    }
    ExtraPreProcess(InputFrameInfo);
    return SUCCESS;
}



void AmlVencInst::ExtraPreProcess(stInputFrameInfo &InputFrameInfo) {
    return;
}


void AmlVencInst::ExtraProcess() {
    return;
}



c2_status_t AmlVencInst::ProcessOneFrame(stInputFrameInfo InputFrameInfo,stOutputFrame &OutFrame) {
    AMLVenc_LOG(CODEC2_VENC_LOG_DEBUG,"ProcessOneFrame! yPlane:%p,uPlane:%p,vPlane:%p",InputFrameInfo.yPlane,InputFrameInfo.uPlane,InputFrameInfo.vPlane);

    amvenc_metadata_t ret;
    amvenc_frame_info_t InputInfo;
    amvenc_buffer_info_t BufferInfo;
    amvenc_buffer_info_t retbuf;
    stVencParam VencParam;
    amvenc_frame_type_t FrameType = AML_FRAME_TYPE_AUTO;
//    int32_t avg_qp = 0;
    if (!OutFrame.Data) {
        AMLVenc_LOG(CODEC2_VENC_LOG_ERR,"ProcessOneFrame parameter bad value,pls check!");
        return C2_BAD_VALUE;
    }

    memset(&ret,0,sizeof(ret));
    memset(&InputInfo,0,sizeof(InputInfo));
    memset(&VencParam,0,sizeof(VencParam));
    memset(&BufferInfo,0,sizeof(BufferInfo));
    memset(&retbuf,0,sizeof(retbuf));

    mVencParamInst->GetVencParam(VencParam);

    FrameType = AML_FRAME_TYPE_P;
    if (VencParam.SyncFrameRequest != mRequestSync) {
        // we can handle IDR immediately
        if (VencParam.SyncFrameRequest) {
            // unset request
            FrameType = AML_FRAME_TYPE_IDR;
            mVencParamInst->UpdateSyncFrameReq(false);
            AMLVenc_LOG(CODEC2_VENC_LOG_ERR,"Got dynamic IDR request");
        }
        mRequestSync = false;
    }

    // codec2TypeTrans(InputFrameInfo.colorFmt,&inputInfo.buf_fmt);
    BufferInfo.buf_fmt = (amvenc_img_format_t)InputFrameInfo.colorFmt;
    //BufferInfo.buf_fmt = AML_IMG_FMT_YUV420P;
    if (DMA == InputFrameInfo.bufType) {
        BufferInfo.buf_type = AML_DMA_TYPE;
        BufferInfo.buf_stride = InputFrameInfo.yStride;
        BufferInfo.buf_info.dma_info.shared_fd[0] = InputFrameInfo.shareFd[0];
        BufferInfo.buf_info.dma_info.shared_fd[1] = 0;//InputFrameInfo.shareFd[1];
        BufferInfo.buf_info.dma_info.shared_fd[2] = 0;//InputFrameInfo.shareFd[2];
        BufferInfo.buf_info.dma_info.num_planes = InputFrameInfo.planeNum;
        AMLVenc_LOG(CODEC2_VENC_LOG_DEBUG,"dma mode,plan num:%d,fd[%d %d %d]",InputFrameInfo.planeNum,InputFrameInfo.shareFd[0],
                                                  InputFrameInfo.shareFd[1],
                                                  InputFrameInfo.shareFd[2]);
    }
    else if (CANVAS == InputFrameInfo.bufType) {
        BufferInfo.buf_type = AML_CANVAS_TYPE;
        BufferInfo.buf_info.canvas = InputFrameInfo.canvas;
    }
    else {
        BufferInfo.buf_type = AML_VMALLOC_TYPE;
        if (AML_IMG_FMT_RGBA8888 == BufferInfo.buf_fmt) {
            BufferInfo.buf_info.in_ptr[0] = (unsigned long)InputFrameInfo.yPlane;
            BufferInfo.buf_info.in_ptr[1] = (unsigned long)InputFrameInfo.uPlane;
            BufferInfo.buf_info.in_ptr[2] = (unsigned long)InputFrameInfo.vPlane;
        }
        else {
            BufferInfo.buf_info.in_ptr[0] = (unsigned long)InputFrameInfo.yPlane;
            BufferInfo.buf_info.in_ptr[1] = (unsigned long)InputFrameInfo.vPlane;//inputInfo.YCbCr[0] + mSize->width * mSize->height;//(unsigned long)uPlane;
            BufferInfo.buf_info.in_ptr[2] = (unsigned long)InputFrameInfo.vPlane;//(unsigned long)vPlane;
        }
        BufferInfo.buf_stride = InputFrameInfo.yStride;
    }

    ExtraProcess();  //extra process action for each ip core

    //C2MULTI_LOG(CODEC2_VENC_LOG_DEBUG,"mPixelFormat->value:0x%x,InputFrameInfo.colorFmt:%x",mPixelFormat->value,InputFrameInfo.colorFmt);

    if (mBitRate != VencParam.Bitrate) {
        AMLVenc_LOG(CODEC2_VENC_LOG_ERR,"bitrate change to %d",VencParam.Bitrate);
        amvenc_change_bitrate(mVencHandle,VencParam.Bitrate);
        mBitRate = VencParam.Bitrate;
    }

    AMLVenc_LOG(CODEC2_VENC_LOG_DEBUG,"Debug input info:yAddr:0x%lx,uAddr:0x%lx,vAddr:0x%lx,frame_type:%d,fmt:%d,pitch:%d,bitrate:%d",BufferInfo.buf_info.in_ptr[0],
         BufferInfo.buf_info.in_ptr[1],BufferInfo.buf_info.in_ptr[2],BufferInfo.buf_type,BufferInfo.buf_fmt,BufferInfo.buf_stride,VencParam.Bitrate);

    ret = amvenc_encode(mVencHandle, InputInfo, FrameType, (unsigned char*)OutFrame.Data, &BufferInfo, &retbuf);
    OutFrame.Length = ret.encoded_data_length_in_bytes;
    AMLVenc_LOG(CODEC2_VENC_LOG_DEBUG,"encode finish,output len:%d",ret.encoded_data_length_in_bytes);
    if (!ret.is_valid) {
        AMLVenc_LOG(CODEC2_VENC_LOG_ERR,"multi encode frame failed,errcode:%d",ret.err_cod);
        return C2_CORRUPTED;
    }
#if 0 //need add this function
    int re = 0;
    re = mEncFrameQpFunc(mCodecHandle,&avg_qp);
    if (re != 1) {
        C2MULTI_LOG(CODEC2_VENC_LOG_ERR,"get avg_qp failed,re:%d",re);
    }
    C2MULTI_LOG(CODEC2_VENC_LOG_DEBUG,"per frame avg_qp=%d",avg_qp);
#endif
    mHelper->dumpDataToFile(DUMP_ES,OutFrame.Data,OutFrame.Length);
    OutFrame.FrameType = P_FRAME;
    if (ret.is_key_frame) {
        OutFrame.FrameType = IDR_FRAME;
        mVencParamInst->UpdateFrameType(IDR_FRAME/*C2Config::SYNC_FRAME*/);
    }
    else {
        mVencParamInst->UpdateFrameType(P_FRAME/*C2Config::P_FRAME*/);
    }
#if 0 //need add this function
    mIntfImpl->setAverageQp(avg_qp);
#endif
    return C2_OK;
}



bool AmlVencInst::Destroy() {
    AMLVenc_LOG(CODEC2_VENC_LOG_ERR,"destroy venc instance,mVencHandle:%lx",mVencHandle);
    if (mVencHandle) {
        amvenc_destroy(mVencHandle);
        mVencHandle = 0;
    }
    else {
        AMLVenc_LOG(CODEC2_VENC_LOG_ERR,"venc instance have been destroyed already!!");
    }
    AMLVenc_LOG(CODEC2_VENC_LOG_ERR,"mVencHandle:%lx",mVencHandle);
    mHelper->CloseFile(DUMP_RAW);
    mHelper->CloseFile(DUMP_ES);
    return true;
}


void AmlVencInst::SetVencParamInst(IAmlVencParam *pVencParamInst) {
    if (NULL != pVencParamInst)
        mVencParamInst = pVencParamInst;
}


}



