#include "vp_hevc_codec_1_0.h"
#include <stdio.h>
#include <stdlib.h>

#ifdef MAKEANDROID
#include <utils/Log.h>
#define LOGCAT
#endif

#include "include/AML_HEVCEncoder.h"
#include "include/enc_define.h"

#ifdef LOGCAT
#define VLOG(level, x...) \
    do { \
        if (level == INFO) \
            ALOGI(x); \
        else if (level == DEBUG) \
            ALOGD(x); \
        else if (level == WARN) \
            ALOGW(x); \
        else if (level >= ERR) \
            ALOGE(x); \
    }while(0);
#else
#define VLOG(level, x...) \
    do { \
        if (level >= 0) { \
            printf(x); \
            printf("\n"); \
        } \
    }while(0);
#endif

const char version[] = "Amlogic libvp_hevc_codec version 1.0";

const char *vl_get_version() {
    return version;
}

int initEncParams(AMVHEVCEncHandle *handle, int width, int height, int frame_rate, int bit_rate, int gop) {
    memset(&(handle->mEncParams), 0, sizeof(AMVHEVCEncParams));
    VLOG(INFO, "bit_rate:%d", bit_rate);
    if ((width % 16 != 0 || height % 2 != 0)) {
        VLOG(INFO, "Video frame size %dx%d must be a multiple of 16", width, height);
        //return -1;
    } else if (height % 16 != 0) {
        VLOG(INFO, "Video frame height is not standard:%d", height);
    } else {
        VLOG(INFO, "Video frame size is %d x %d", width, height);
    }
    handle->mEncParams.rate_control = AVC_ON;
    handle->mEncParams.initQP = 0;
    handle->mEncParams.init_CBP_removal_delay = 1600;
    handle->mEncParams.auto_scd = AVC_ON;
    handle->mEncParams.out_of_band_param_set = AVC_ON;
    handle->mEncParams.num_ref_frame = 1;
    handle->mEncParams.num_slice_group = 1;
    //handle->mEncParams.nSliceHeaderSpacing = 0;
    handle->mEncParams.fullsearch = AVC_OFF;
    handle->mEncParams.search_range = 16;
    //handle->mEncParams.sub_pel = AVC_OFF;
    //handle->mEncParams.submb_pred = AVC_OFF;
    handle->mEncParams.width = width;
    handle->mEncParams.height = height;
	handle->mEncParams.src_width = width;
    handle->mEncParams.src_height = height;
    handle->mEncParams.bitrate = bit_rate;
    handle->mEncParams.frame_rate = frame_rate;
    handle->mEncParams.CPB_size = (uint32)(bit_rate >> 1);
    handle->mEncParams.FreeRun = AVC_OFF;
    handle->mEncParams.MBsIntraRefresh = 0;
    handle->mEncParams.MBsIntraOverlap = 0;
    handle->mEncParams.encode_once = 1;
    // Set IDR frame refresh interval
    if ( gop == -1) {
        handle->mEncParams.idr_period = -1; // All I frames
    } else if (gop == 0) {
        handle->mEncParams.idr_period = 0;   //Only one I frame
    } else {
        handle->mEncParams.idr_period = gop; //period of I frame
    }

	VLOG(INFO, "mEncParams.idrPeriod: %d,gop %d\n", handle->mEncParams.idr_period,gop);
    // Set profile and level
    handle->mEncParams.profile = AVC_BASELINE;
    handle->mEncParams.level = AVC_LEVEL4;
    handle->mEncParams.initQP = 30;
    handle->mEncParams.BitrateScale = AVC_OFF;
    return 0;
}


vl_codec_handle_t vl_video_encoder_init(vl_codec_id_t codec_id, int width, int height, int frame_rate, int bit_rate, int gop, vl_img_format_t img_format) {
    int ret;
    AMVHEVCEncHandle *mHandle = new AMVHEVCEncHandle;
    bool has_mix = false;
	(void)codec_id;
	(void)img_format;

    if (mHandle == NULL)
        goto exit;
    memset(mHandle, 0, sizeof(AMVHEVCEncHandle));
    ret = initEncParams(mHandle, width, height, frame_rate, bit_rate, gop);
    if (ret < 0)
        goto exit;
    ret = AML_HEVCInitialize(mHandle, &(mHandle->mEncParams), &has_mix, 2);
    if (ret < 0)
        goto exit;
    mHandle->mSpsPpsHeaderReceived = false;
    mHandle->mNumInputFrames = -1;  // 1st two buffers contain SPS and PPS
    return (vl_codec_handle_t) mHandle;
exit:
    if (mHandle != NULL)
        delete mHandle;
    return (vl_codec_handle_t) NULL;
}



int vl_video_encoder_encode(vl_codec_handle_t codec_handle, vl_frame_type_t frame_type, unsigned char *in, int in_size, unsigned char *out, int format) {
    int ret;
    uint32_t dataLength = 0;
    int type;
    AMVHEVCEncHandle *handle = (AMVHEVCEncHandle *)codec_handle;

	(void)frame_type;

    if (!handle->mSpsPpsHeaderReceived) {
        ret = AML_HEVCEncHeader(handle, (unsigned char *)out, (unsigned int *)&in_size/*should be out size*/, &type);
        if (ret == AMVENC_SUCCESS) {
            handle->mSPSPPSDataSize = 0;
            handle->mSPSPPSData = (uint8_t *)malloc(in_size);
            if (handle->mSPSPPSData) {
                handle->mSPSPPSDataSize = in_size;
                memcpy(handle->mSPSPPSData, (unsigned char *)out, handle->mSPSPPSDataSize);
                VLOG(INFO, "get mSPSPPSData size= %d at line %d \n", handle->mSPSPPSDataSize, __LINE__);
            }
            handle->mNumInputFrames = 0;
            handle->mSpsPpsHeaderReceived = true;
        } else {
            VLOG(INFO, "Encode SPS and PPS error, encoderStatus = %d. handle: %p\n", ret, (void *)handle);
            return -1;
        }
    }
    if (handle->mNumInputFrames >= 0) {
        AMVHEVCEncFrameIO videoInput;
        memset(&videoInput, 0, sizeof(videoInput));
        videoInput.height = handle->mEncParams.height;
        videoInput.pitch = handle->mEncParams.width;//((handle->mEncParams.width + 15) >> 4) << 4;
        /* TODO*/
        videoInput.bitrate = handle->mEncParams.bitrate;
        videoInput.frame_rate = handle->mEncParams.frame_rate / 1000;
        videoInput.coding_timestamp = handle->mNumInputFrames * 1000 / videoInput.frame_rate;  // in ms
        videoInput.YCbCr[0] = (unsigned long)&in[0];
        videoInput.YCbCr[1] = (unsigned long)(videoInput.YCbCr[0] + videoInput.height * videoInput.pitch);
        if (format == 0) {
            videoInput.fmt = AMVENC_NV21;
            videoInput.YCbCr[2] = 0;
        } else if (format == 1) {
            videoInput.fmt = AMVENC_NV12;
            videoInput.YCbCr[2] = 0;
        } else {
            videoInput.fmt = AMVENC_YUV420;
            videoInput.YCbCr[2] = (unsigned long)(videoInput.YCbCr[1] + videoInput.height * videoInput.pitch / 4);
        }
        videoInput.canvas = 0xffffffff;
        videoInput.type = VMALLOC_BUFFER;
        videoInput.disp_order = handle->mNumInputFrames;
        videoInput.op_flag = 0;
		//if(handle->mNumInputFrames==0)
			//handle->mKeyFrameRequested = true;
        if (handle->mKeyFrameRequested == true) {
            videoInput.op_flag = AMVEncFrameIO_FORCE_IDR_FLAG;
            handle->mKeyFrameRequested = false;
			VLOG(INFO, "Force encode a IDR frame at %d frame", handle->mNumInputFrames);
        }
		//if (handle->idrPeriod == 0) {
            //videoInput.op_flag |= AMVEncFrameIO_FORCE_IDR_FLAG;
        //}
        ret = AML_HEVCSetInput(handle, &videoInput);

		VLOG(INFO, "AML_HEVCSetInput ret %d\n", ret);
        if (ret == AMVENC_SUCCESS) {
            ++(handle->mNumInputFrames);
            dataLength  = /*should be out size */in_size;
        } else if (ret < AMVENC_SUCCESS) {
            VLOG(INFO, "encoderStatus = %d at line %d, handle: %p", ret, __LINE__, (void *)handle);
            return -1;
        }

        ret = AML_HEVCEncNAL(handle, out, (unsigned int *)&dataLength, &type);
		VLOG(INFO, "AML_HEVCEncNAL ret %d, type %d\n", ret, type);
        if (ret == AMVENC_PICTURE_READY) {
            if (type == AVC_NALTYPE_IDR) {
                if ((handle->mPrependSPSPPSToIDRFrames || handle->mNumInputFrames == 1) && (handle->mSPSPPSData)) {
                    memmove(out + handle->mSPSPPSDataSize, out, dataLength);
                    memcpy(out, handle->mSPSPPSData, handle->mSPSPPSDataSize);
                    dataLength += handle->mSPSPPSDataSize;
                    VLOG(INFO, "copy mSPSPPSData to buffer size= %d at line %d \n", handle->mSPSPPSDataSize, __LINE__);
                }
            }
        }
        else if ((ret == AMVENC_SKIPPED_PICTURE) || (ret == AMVENC_TIMEOUT)) {
            dataLength = 0;
            if (ret == AMVENC_TIMEOUT) {
                handle->mKeyFrameRequested = true;
                ret = AMVENC_SKIPPED_PICTURE;
            }
            VLOG(INFO, "ret = %d at line %d, handle: %p", ret, __LINE__, (void *)handle);
        } else if (ret != AMVENC_SUCCESS) {
            dataLength = 0;
        }

        if (ret < AMVENC_SUCCESS) {
            VLOG(INFO, "encoderStatus = %d at line %d, handle: %p", ret , __LINE__, (void *)handle);
            return -1;
        }
    }
    return dataLength;
}

int vl_video_encoder_destory(vl_codec_handle_t codec_handle) {
    AMVHEVCEncHandle *handle = (AMVHEVCEncHandle *)codec_handle;
    AML_HEVCRelease(handle);
    if (handle->mSPSPPSData)
        free(handle->mSPSPPSData);
    if (handle)
        delete handle;
    return 1;
}
