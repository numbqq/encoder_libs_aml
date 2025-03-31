#include <stdio.h>
#include <stdlib.h>

#ifdef MAKEANDROID
#include <utils/Log.h>
#define LOGCAT
#endif
#include "vp_hevc_codec_1_0.h"
#include "include/AML_HEVCEncoder.h"
#include "include/enc_define.h"
#include "./vpuapi/include/vdi_osal.h"
#include "include/h265bitstream.h"

const char version[] = "Amlogic libamvenc_265 version 1.0";

const char *vl_get_version() {
    return version;
}

typedef struct AMVHEVCEncHandle_s {
    AMVHEVCEncParams mEncParams;

    AMVEncBufferType bufType;
    bool mPrependSPSPPSToIDRFrames;
    bool mSpsPpsHeaderReceived;
    bool mKeyFrameRequested;
    bool mSkipFrameRequested;
    int mLongTermRefRequestFlags;
    int mNumInputFrames;
    AMVEncFrameFmt fmt;
    int mSPSPPSDataSize;
    char *mSPSPPSData;
    amv_enc_handle_hevc_t am_enc_handle;
    int shared_fd[3];
    uint32 mNumPlanes;
    int mSEIDataSize;
    char *mSEIData;
#if 0
    uint32 magic_num;
    uint32 instance_id;
    uint32 src_idx;
    uint32 enc_width;
    uint32 enc_height;
    uint32 frame_rate; /* frame rate */
    int32 idrPeriod; /* IDR period in number of frames */
    uint32 op_flag;

    uint32 src_num;
    uint32 fb_num;

    AMVEncFrameFmt fmt;

    AVCNalUnitType nal_unit_type;
    AMVEncBufferType bufType;
    int shared_fd[3];
    uint32 mNumPlanes;

    AMVHEVCEncParams mEncParams;
    amv_enc_handle_hevc_t am_enc_handle;
    EncHandle enchandle; /*VPU encoder handler */
    EncOpenParam encOpenParam; /* api open param */
    bool mSpsPpsHeaderReceived;
    int mSPSPPSDataSize;
    char *mSPSPPSData;
    int32_t mNumInputFrames;
    bool mKeyFrameRequested;
    bool mPrependSPSPPSToIDRFrames;
    uint32 mOutputBufferLen;
    Uint64                      startTimeout;
    Uint32                      frameIdx;
    EncParam                    encParam;
    Int32                       encodedSrcFrmIdxArr[ENC_SRC_BUF_NUM];
    Int32                       encMEMSrcFrmIdxArr[ENC_SRC_BUF_NUM];
    EncInitialInfo              initialInfo;
    vpu_buffer_t                vbVuiRbsp;
    vpu_buffer_t                vbHrdRbsp;
    vpu_buffer_t                bsBuffer[ENC_SRC_BUF_NUM];

    Uint32                      reconFbStride;
    Uint32                      reconFbHeight;
    Uint32                      srcFbStride;
    Uint32                      srcFbSize;
    FrameBufferAllocInfo        srcFbAllocInfo;
    BOOL                        fbAllocated;
    FrameBuffer                 pFbRecon[MAX_REG_FRAME];
    vpu_buffer_t                pFbReconMem[MAX_REG_FRAME];
    FrameBuffer                 pFbSrc[ENC_SRC_BUF_NUM];
    vpu_buffer_t                pFbSrcMem[ENC_SRC_BUF_NUM];
    vpu_buffer_t                vbCustomMap[MAX_REG_FRAME];
#endif
} AMVHEVCEncHandle;


AMVEnc_Status initEncParams(AMVHEVCEncHandle *handle,
                        vl_encode_info_hevc_t encode_info,
                        qp_param_hevc_t* qp_tbl) {
    int width = encode_info.width;
    int height = encode_info.height;
    memset(&(handle->mEncParams), 0, sizeof(AMVHEVCEncParams));
    VLOG(DEBUG, "bit_rate:%d", encode_info.bit_rate);
    if ((width % 16 != 0 || height % 2 != 0)) {
        VLOG(DEBUG, "Video frame size %dx%d must be a multiple of 16", width, height);
        //return -1;
    } else if (height % 16 != 0) {
        VLOG(DEBUG, "Video frame height is not standard:%d", height);
    } else {
        VLOG(DEBUG, "Video frame size is %d x %d", width, height);
    }
    handle->mEncParams.rate_control = HEVC_ON;

    handle->mEncParams.initQP = 0;
    handle->mEncParams.initQP = qp_tbl->qp_base;
    handle->mEncParams.maxDeltaQP = qp_tbl->qp_max - qp_tbl->qp_min;
    handle->mEncParams.maxQP = qp_tbl->qp_max;
    handle->mEncParams.minQP = qp_tbl->qp_min;
    handle->mEncParams.qp_mode = encode_info.qp_mode;

    handle->mEncParams.init_CBP_removal_delay = 1600;
    handle->mEncParams.auto_scd = HEVC_OFF;
    handle->mEncParams.out_of_band_param_set = HEVC_OFF;
    handle->mEncParams.num_ref_frame = 1;
    handle->mEncParams.num_slice_group = 1;
    //handle->mEncParams.nSliceHeaderSpacing = 0;
    handle->mEncParams.fullsearch = HEVC_OFF;
    handle->mEncParams.search_range = 16;
    //handle->mEncParams.sub_pel = HEVC_OFF;
    //handle->mEncParams.submb_pred = HEVC_OFF;
    handle->mEncParams.width = width;
    handle->mEncParams.height = height;
    handle->mEncParams.src_width = encode_info.width;
    handle->mEncParams.src_height = encode_info.height;
    handle->mEncParams.bitrate = encode_info.bit_rate;
    handle->mEncParams.frame_rate = encode_info.frame_rate;
    handle->mEncParams.CPB_size = (uint32)(encode_info.bit_rate >> 1);
    handle->mEncParams.FreeRun = HEVC_OFF;

    if (encode_info.bitstream_buf_sz_kb > 0 && encode_info.bitstream_buf_sz_kb <= (32 * 1024)) {
        handle->mEncParams.es_buf_sz =  encode_info.bitstream_buf_sz_kb*1024;
    } else {
        if (encode_info.bitstream_buf_sz > 0 && encode_info.bitstream_buf_sz <= 32)
            handle->mEncParams.es_buf_sz =  encode_info.bitstream_buf_sz*1024*1024;
        else  //default value
            handle->mEncParams.es_buf_sz = 0; //out of range, use default
    }

    handle->mEncParams.MBsIntraRefresh = 0;
    handle->mEncParams.MBsIntraOverlap = 0;
    handle->mEncParams.encode_once = 1;

    handle->mEncParams.crop_enable = encode_info.crop_enable;
    if (handle->mEncParams.crop_enable) {
        handle->mEncParams.crop_left = encode_info.crop.left;
        handle->mEncParams.crop_top = encode_info.crop.top;
        handle->mEncParams.crop_right = encode_info.crop.right;
        handle->mEncParams.crop_bottom = encode_info.crop.bottom;
    }

    if (encode_info.enc_feature_opts & ENABLE_PARA_UPDATE)
      handle->mEncParams.param_change_enable = 1;

    if (encode_info.img_format == IMG_FMT_NV12) {
      VLOG(INFO, "[%s %d] img_format is IMG_FMT_NV12 \n", __func__, __LINE__);
      handle->fmt = AMVENC_NV12;
    } else if (encode_info.img_format == IMG_FMT_NV21) {
      VLOG(INFO, "[%s %d] img_format is IMG_FMT_NV21 \n", __func__, __LINE__);
      handle->fmt = AMVENC_NV21;
    } else if (encode_info.img_format == IMG_FMT_YUV420P) {
      VLOG(INFO, "[%s %d] img_format is IMG_FMT_YUV420 \n", __func__, __LINE__);
      handle->fmt = AMVENC_YUV420;
    } else if (encode_info.img_format == IMG_FMT_RGB888) {
      handle->fmt = AMVENC_RGB888;
    } else if (encode_info.img_format == IMG_FMT_RGBA8888) {
      VLOG(INFO, "img_format is IMG_FMT_RGBA8888 \n");
      handle->fmt = AMVENC_RGBA8888;
    } else {
      VLOG(ERR, "img_format %d not supprot\n",encode_info.img_format);
      return AMVENC_FAIL;
    }

    handle->mEncParams.fmt = handle->fmt;
    if ((encode_info.img_format == IMG_FMT_RGB888) || (encode_info.img_format == IMG_FMT_RGBA8888)) {
      handle->mEncParams.fmt = AMVENC_NV21; //ge2d dst format PIXEL_FORMAT_YCrCb_420_SP
    }
    // Set IDR frame refresh interval
    if (encode_info.gop <= 0) {
        handle->mEncParams.idr_period = 0;   //an infinite period, only one I frame
    } else {
        handle->mEncParams.idr_period = encode_info.gop; //period of I frame, 1 means all frames are I type.
    }

    VLOG(DEBUG, "idrPeriod: %d, gop %d\n", handle->mEncParams.idr_period, encode_info.gop);
    // Set profile and level
    handle->mEncParams.profile = HEVC_MAIN;
    handle->mEncParams.level = HEVC_LEVEL_NONE; // firmware determines a level.
    handle->mEncParams.tier = HEVC_TIER_MAIN;
    handle->mEncParams.initQP = 30;
    handle->mEncParams.BitrateScale = HEVC_OFF;
    handle->mEncParams.refresh_type = HEVC_CRA;
    handle->mEncParams.vui_info_present = encode_info.vui_info_present;
    handle->mEncParams.video_signal_type = encode_info.video_signal_type;
    handle->mEncParams.color_description = encode_info.color_description;
    handle->mEncParams.primaries = encode_info.primaries;
    handle->mEncParams.transfer = encode_info.transfer;
    handle->mEncParams.matrix = encode_info.matrix;
    handle->mEncParams.range = encode_info.range;
    if (encode_info.profile)
        handle->mEncParams.profile = (HEVCProfile)encode_info.profile;
    if (encode_info.tier)
        handle->mEncParams.tier = encode_info.tier;
    if (encode_info.level)
        handle->mEncParams.level = (HEVCLevel)encode_info.level;
    return AMVENC_SUCCESS;
}

bool check_qp_tbl(const qp_param_hevc_t* qp_tbl) {
  if (qp_tbl == NULL) {
    return false;
  }
  if (qp_tbl->qp_min < 0 || qp_tbl->qp_min > 51 || qp_tbl->qp_max < 0 ||
      qp_tbl->qp_max > 51) {
    VLOG(ERR,"qp_min or qp_max out of range [0, 51]\n");
    return false;
  }
  if (qp_tbl->qp_base < 0 || qp_tbl->qp_base > 51) {
    VLOG(ERR,"qp_base out of range [0, 51]\n");
    return false;
  }
  return true;
}

vl_codec_handle_hevc_t vl_video_encoder_init_hevc(vl_codec_id_hevc_t codec_id,
                                       vl_encode_info_hevc_t encode_info,
                                        qp_param_hevc_t* qp_tbl) {
    AMVEnc_Status ret;
    AMVHEVCEncHandle *mHandle = new AMVHEVCEncHandle;
    bool has_mix = false;
    (void)codec_id;

    if (mHandle == NULL)
        goto exit;
    memset(mHandle, 0, sizeof(AMVHEVCEncHandle));
    if (!check_qp_tbl(qp_tbl)) {
      goto exit;
    }

    vp4_set_log_level(ERR);

    ret = initEncParams(mHandle, encode_info, qp_tbl);
    if (ret < AMVENC_SUCCESS)
        goto exit;
    mHandle->am_enc_handle = AML_HEVCInitialize(&(mHandle->mEncParams));
    if (mHandle->am_enc_handle == 0)
        goto exit;

    mHandle->mPrependSPSPPSToIDRFrames =
                  encode_info.prepend_spspps_to_idr_frames;

    mHandle->mSpsPpsHeaderReceived = false;
    mHandle->mNumInputFrames = -1;  // 1st two buffers contain SPS and PPS

    return (vl_codec_handle_hevc_t) mHandle;
exit:
    if (mHandle != NULL)
        delete mHandle;
    return (vl_codec_handle_hevc_t) NULL;
}

#define H265_HEADER_LEN 6

void vl_encoder_adjust_h265_header(vl_codec_handle_hevc_t handle,char *header,int *dataLength)
{
    bs_t bs;
    bs_t bs_v;
    uint32_t i = 0;
    int vps_nalu_size = 0;
    int sps_nalu_size = 0;
    int pps_nalu_size = 0;
    int new_sps_size = 0;
    int new_vps_size = 0;
    int vps_start = -1;
    int sps_start = -1;
    int pps_start = -1;
    int ret = 0;
    h265_stream_t *pstream_handle = NULL;
    AMVHEVCEncHandle *codec_handle = (AMVHEVCEncHandle *)handle;

    uint8_t *vps_nalu = (uint8_t *) malloc(sizeof(uint8_t) * (*dataLength));
    uint8_t *sps_nalu = (uint8_t *) malloc(sizeof(uint8_t) * (*dataLength));
    uint8_t *pps_nalu = (uint8_t *) malloc(sizeof(uint8_t) * (*dataLength));
    if (sps_nalu == NULL || pps_nalu == NULL || vps_nalu == NULL) {
        VLOG(ERR,"hack_sps malloc for sps or pps failed");
        return;
    }

    for (i=0;i<*dataLength-H265_HEADER_LEN;i++) {
        if ((uint8_t)header[i+0] == 0 && (uint8_t)header[i+1] == 0 && (uint8_t)header[i+2] == 0 && (uint8_t)header[i+3] == 1 &&
            (((uint8_t)header[i+4]) & 0x7e) == 0x40) {
            vps_start = i;
            VLOG(INFO,"hack_sps vps_start=%d\n", vps_start);
            //break;
        }
        if ((uint8_t)header[i+0] == 0 && (uint8_t)header[i+1] == 0 && (uint8_t)header[i+2] == 0 && (uint8_t)header[i+3] == 1 &&
            (((uint8_t)header[i+4]) & 0x7e) == 0x42) {
            sps_start = i;
            VLOG(INFO,"hack_sps pps_start=%d\n", sps_start);
        }
        if ((uint8_t)header[i+0] == 0 && (uint8_t)header[i+1] == 0 && (uint8_t)header[i+2] == 0 && (uint8_t)header[i+3] == 1 &&
        (((uint8_t)header[i+4]) & 0x7e) == 0x44) {
            pps_start = i;
            VLOG(INFO,"hack_sps pps_start=%d\n", pps_start);
            break;
        }
    }

    vps_nalu_size = sps_start;
    sps_nalu_size = pps_start - sps_start;
    pps_nalu_size = *dataLength - pps_start;
    VLOG(INFO,"hack_sps old vps_nalu_size=%d,sps_nalu_size=%d,pps_nalu_size=%d",vps_nalu_size, sps_nalu_size, pps_nalu_size);

    memcpy(vps_nalu, header, vps_nalu_size);
    memcpy(sps_nalu, header + sps_start, sps_nalu_size);
    memcpy(pps_nalu, header + pps_start, pps_nalu_size);
#if 0
    char tmp[1024] = {0};
    char tmp1[128] = {0};
    for (int i = 0;i < vps_nalu_size;i++) {
        sprintf(tmp1," %x",header[i]);
        strcat(tmp,tmp1);
    }
    VLOG(ERR,"old header data:%s",tmp);
#endif
    sps_nalu_size = EBSPtoRBSP(sps_nalu,H265_HEADER_LEN,sps_nalu_size);

    vps_nalu_size = EBSPtoRBSP(vps_nalu,H265_HEADER_LEN,vps_nalu_size);

    pstream_handle = h265bitstream_init();
    bs_init(&bs_v,vps_nalu + H265_HEADER_LEN, vps_nalu_size - H265_HEADER_LEN);

    read_debug_video_parameter_set_rbsp(pstream_handle, &bs_v);
    read_debug_rbsp_trailing_bits(pstream_handle,&bs_v);

    pstream_handle->ptl->general_profile_idc = codec_handle->mEncParams.profile;//3;
    pstream_handle->ptl->general_tier_flag = codec_handle->mEncParams.tier;
    pstream_handle->ptl->general_level_idc = codec_handle->mEncParams.level;//120;

    bs_init(&bs_v,vps_nalu + H265_HEADER_LEN, vps_nalu_size - H265_HEADER_LEN);
    write_debug_video_parameter_set_rbsp(pstream_handle, &bs_v);

    new_vps_size = vps_nalu_size;
    new_vps_size = RBSPtoEBSP(vps_nalu,H265_HEADER_LEN,new_vps_size,0);

    bs_init(&bs, sps_nalu + H265_HEADER_LEN, sps_nalu_size - H265_HEADER_LEN);
    read_debug_seq_parameter_set_rbsp(pstream_handle, &bs);
    read_debug_rbsp_trailing_bits(pstream_handle,&bs);

    pstream_handle->ptl->general_profile_idc = codec_handle->mEncParams.profile;//3;
    pstream_handle->ptl->general_tier_flag = codec_handle->mEncParams.tier;
    pstream_handle->ptl->general_level_idc = codec_handle->mEncParams.level;//120;

    VLOG(INFO,"write profile :%d,level:%d",pstream_handle->ptl->general_profile_idc,pstream_handle->ptl->general_level_idc);

    pstream_handle->sps->conformance_window_flag = codec_handle->mEncParams.crop_enable;//1;
    if (pstream_handle->sps->conformance_window_flag) {
        pstream_handle->sps->conf_win_top_offse = codec_handle->mEncParams.crop_top / 2;
        pstream_handle->sps->conf_win_left_offset = codec_handle->mEncParams.crop_left / 2;
        pstream_handle->sps->conf_win_right_offset = (codec_handle->mEncParams.width - codec_handle->mEncParams.crop_right) / 2;//0;//(256 - 176)/2;
        pstream_handle->sps->conf_win_bottom_offset = (codec_handle->mEncParams.height - codec_handle->mEncParams.crop_bottom) / 2;
        VLOG(INFO,"crop top:%d,left:%d,right:%d,bottom:%d,enable:%d",pstream_handle->sps->conf_win_top_offse,
                                                                    pstream_handle->sps->conf_win_left_offset,
                                                                    pstream_handle->sps->conf_win_right_offset,
                                                                    pstream_handle->sps->conf_win_bottom_offset,
                                                                    pstream_handle->sps->conformance_window_flag);
    }

    memset(sps_nalu + H265_HEADER_LEN, 0, *dataLength - H265_HEADER_LEN);

    bs_init(&bs, sps_nalu + H265_HEADER_LEN, *dataLength - H265_HEADER_LEN);
    write_debug_seq_parameter_set_rbsp(pstream_handle, &bs);
    write_debug_rbsp_trailing_bits(&bs);
    new_sps_size = bs.p - bs.start + H265_HEADER_LEN;

    memset(header, 0, new_vps_size + new_sps_size + pps_nalu_size);

    memcpy(header,vps_nalu, new_vps_size);

    new_sps_size = RBSPtoEBSP(sps_nalu,H265_HEADER_LEN,new_sps_size,0);
    memcpy(header + new_vps_size,sps_nalu,new_sps_size);
    memcpy(header + new_sps_size + new_vps_size, pps_nalu, pps_nalu_size);
    *dataLength = new_sps_size + pps_nalu_size + new_vps_size;
#if 0
    memset(tmp,0,sizeof(tmp));
    memset(tmp1,0,sizeof(tmp1));
    for (int i = 0;i < *dataLength;i++) {
        sprintf(tmp1," %x",header[i]);
        strcat(tmp,tmp1);
    }
    VLOG(ERR,"new header:%s",tmp);
#endif
    if (vps_nalu)
        free(vps_nalu);
    if (sps_nalu)
        free(sps_nalu);
    if (pps_nalu)
        free(pps_nalu);
    h265_free(pstream_handle);
}

void vl_encoder_adjust_h265_sei(AMVHEVCEncHandle* handle, uint32 op_flag)
{
    int ret;
    bs_t bs = {0};
    int sei_nalu_size = handle->mSPSPPSDataSize;
    char *sei_nalu = handle->mSEIData;
    char seiUserData[35] = {0};
    char startCodePrefix[] = {0, 0, 0, 1};

    if (op_flag & AMVEncFrameIO_FORCE_IDR_FLAG)
        ret = snprintf(seiUserData, sizeof(seiUserData), "W4IF%dB%dX%dI%d", handle->mEncParams.frame_rate / 5, handle->mEncParams.bitrate / 1000, handle->mEncParams.maxQP, handle->mEncParams.minQP);
    else
        ret = snprintf(seiUserData, sizeof(seiUserData), "W4F%dB%dX%dI%d", handle->mEncParams.frame_rate / 5, handle->mEncParams.bitrate / 1000, handle->mEncParams.maxQP, handle->mEncParams.minQP);
    if (ret >= sizeof(seiUserData))
        VLOG(ERR, "sei info exceeds the userdatasize.\n");

    memcpy(sei_nalu, startCodePrefix, sizeof(startCodePrefix));
    bs_init(&bs, (uint8_t*)sei_nalu + sizeof(startCodePrefix), sei_nalu_size - sizeof(startCodePrefix));
    write_sei_userdataunreg(&bs, (unsigned char *)seiUserData);
    write_debug_rbsp_trailing_bits(&bs);
    handle->mSEIDataSize = bs.p - bs.start + sizeof(startCodePrefix);
}


encoding_metadata_hevc_t vl_video_encoder_generate_header(vl_codec_handle_hevc_t codec_handle,
                                                                   unsigned char *pHeader,
                                                                   unsigned int *pLength) {
   int ret;
   AMVHEVCEncHandle *handle = (AMVHEVCEncHandle *)codec_handle;
   encoding_metadata_hevc_t result;
   if (!handle->mSpsPpsHeaderReceived) {
        ret = AML_HEVCEncHeader(handle->am_enc_handle, (unsigned char *)pHeader, (unsigned int *)pLength);
        if (ret == AMVENC_SUCCESS) {
            vl_encoder_adjust_h265_header(codec_handle, (char *)pHeader,(int *)pLength);
            handle->mSPSPPSDataSize = 0;
            handle->mSPSPPSData = (char *)malloc(*pLength);
            if (handle->mSPSPPSData) {
                handle->mSPSPPSDataSize = *pLength;
                memcpy(handle->mSPSPPSData, (unsigned char *)pHeader, handle->mSPSPPSDataSize);
                VLOG(DEBUG, "get mSPSPPSData size= %d at line %d \n", handle->mSPSPPSDataSize, __LINE__);
            }
            handle->mSEIData = (char *)malloc(*pLength);
            handle->mNumInputFrames = 0;
            handle->mSpsPpsHeaderReceived = true;
            result.is_valid = true;
            result.err_cod = AMVENC_SUCCESS;
        } else {
            *pLength = 0;
            result.is_valid = false;
            VLOG(ERR, "Encode SPS and PPS error, encoderStatus = %d. handle: %p\n", ret, (void *)handle);
            if (ret == AMVENC_FAIL)
               result.err_cod = AMVENC_INVALID_PARAM;
            else
                result.err_cod = ret;
            return result;
        }
    }
    return result;
}

encoding_metadata_hevc_t vl_video_encoder_encode_hevc(vl_codec_handle_hevc_t codec_handle,
                                    vl_frame_type_hevc_t frame_type,
                                    unsigned char* out,
                                    vl_buffer_info_hevc_t* in_buffer_info) {
    AMVEnc_Status ret;
    int i;
    uint32_t dataLength = 0;
    int type = 0;
    AMVHEVCEncHandle *handle = (AMVHEVCEncHandle *)codec_handle;
    encoding_metadata_hevc_t result;
    memset(&result, 0, sizeof(encoding_metadata_hevc_t));
    if (in_buffer_info == NULL) {
        VLOG(ERR, "invalid buffer_info\n");
        result.is_valid = false;
        result.err_cod = AMVENC_UNINITIALIZED;
        return result;
    }

    handle->bufType = (AMVEncBufferType)(in_buffer_info->buf_type);
    if (handle->bufType == DMA_BUFF) {
        if (in_buffer_info->buf_fmt == IMG_FMT_NV12 || in_buffer_info->buf_fmt == IMG_FMT_NV21 || in_buffer_info->buf_fmt == IMG_FMT_YUV420P) {
            if ((handle->mEncParams.width % 16 && in_buffer_info->buf_stride == 0) ||
                in_buffer_info->buf_stride % 16) {
               VLOG(ERR, "dma buffer stride must be multiple of 16!");
               result.is_valid = false;
               result.err_cod = AMVENC_ENCPARAM_MEM_FAIL;
               return result;
            }
        }
    }

    if (!handle->mSpsPpsHeaderReceived) {
        ret = AML_HEVCEncHeader(handle->am_enc_handle, (unsigned char *)out, (unsigned int *)&dataLength);
        if (ret == AMVENC_SUCCESS) {
            vl_encoder_adjust_h265_header(codec_handle, (char *)out,(int *)&dataLength);
            handle->mSPSPPSDataSize = 0;
            handle->mSPSPPSData = (char *)malloc(dataLength);
            if (handle->mSPSPPSData) {
                handle->mSPSPPSDataSize = dataLength;
                memcpy(handle->mSPSPPSData, (unsigned char *)out, handle->mSPSPPSDataSize);
                VLOG(DEBUG, "get mSPSPPSData size= %d at line %d \n", handle->mSPSPPSDataSize, __LINE__);
            }
            handle->mSEIData = (char *)malloc(dataLength);
            handle->mNumInputFrames = 0;
            handle->mSpsPpsHeaderReceived = true;
        } else {
            VLOG(ERR, "Encode SPS and PPS error, encoderStatus = %d. handle: %p\n", ret, (void *)handle);
            if (ret == AMVENC_FAIL)
               result.err_cod = AMVENC_INVALID_PARAM;
            else
                result.err_cod = ret;
            return result;
        }
    }

    if (frame_type == FRAME_TYPE_I || frame_type == FRAME_TYPE_IDR)
      handle->mKeyFrameRequested = true;

    if (handle->mNumInputFrames >= 0) {
        AMVHEVCEncFrameIO videoInput;
        memset(&videoInput, 0, sizeof(videoInput));
        videoInput.height = handle->mEncParams.height;
        if (!in_buffer_info->buf_stride)
            videoInput.pitch = handle->mEncParams.width; //((handle->mEncParams.width + 15) >> 4) << 4;
        else
            videoInput.pitch = in_buffer_info->buf_stride;
        /* TODO*/
        videoInput.bitrate = handle->mEncParams.bitrate;
        videoInput.frame_rate = handle->mEncParams.frame_rate / 1000;
        videoInput.coding_timestamp = handle->mNumInputFrames * 1000 / videoInput.frame_rate;  // in ms
        handle->shared_fd[0] = -1;
        handle->shared_fd[1] = -1;
        handle->shared_fd[2] = -1;

        videoInput.fmt = handle->fmt;

        VLOG(INFO, "in_buffer_info->buf_fmt=%d", in_buffer_info->buf_fmt);

        if (in_buffer_info->buf_fmt == IMG_FMT_NV12) {
          VLOG(INFO, "img_format is IMG_FMT_NV12 \n");
          videoInput.fmt = AMVENC_NV12;
        } else if (in_buffer_info->buf_fmt == IMG_FMT_NV21) {
          VLOG(INFO, "img_format is IMG_FMT_NV21 \n");
          videoInput.fmt = AMVENC_NV21;
        } else if (in_buffer_info->buf_fmt == IMG_FMT_YUV420P) {
          VLOG(INFO, "img_format is IMG_FMT_YUV420P \n");
          videoInput.fmt = AMVENC_YUV420;
        } else if (in_buffer_info->buf_fmt == IMG_FMT_RGB888) {
          VLOG(INFO, "img_format is IMG_FMT_RGB888 \n");
          videoInput.fmt = AMVENC_RGB888;
        } else if (in_buffer_info->buf_fmt == IMG_FMT_RGBA8888) {
          VLOG(INFO, "img_format is IMG_FMT_RGBA8888 \n");
          videoInput.fmt = AMVENC_RGBA8888;
        }

        if (handle->bufType == DMA_BUFF) {
            vl_dma_info_hevc_t *dma_info;
            dma_info = &(in_buffer_info->buf_info.dma_info);
            if (videoInput.fmt == AMVENC_NV21 || videoInput.fmt == AMVENC_NV12) {
                if (dma_info->num_planes != 1
                    && dma_info->num_planes != 2) {
                    VLOG(ERR, "invalid num_planes %d\n", dma_info->num_planes);
                    result.is_valid = false;
                    result.err_cod = AMVENC_ENCPARAM_MEM_FAIL;

                    return result;
                }
            } else if (videoInput.fmt == AMVENC_YUV420) {
                if (dma_info->num_planes != 1
                    && dma_info->num_planes != 3) {
                    VLOG(ERR, "YV12 invalid num_planes %d\n", dma_info->num_planes);
                    result.is_valid = false;
                    result.err_cod = AMVENC_ENCPARAM_MEM_FAIL;

                    return result;
                }
            }

            handle->mNumPlanes = dma_info->num_planes;

            for (uint32 i = 0; i < dma_info->num_planes; i++) {
                if (dma_info->shared_fd[i] < 0) {
                    VLOG(ERR, "invalid dma_fd %d\n", dma_info->shared_fd[i]);
                    result.is_valid = false;
                    result.err_cod = AMVENC_ENCPARAM_MEM_FAIL;
                    return result;
                }
                handle->shared_fd[i] = dma_info->shared_fd[i];
                VLOG(INFO, "**shared_fd %d\n", handle->shared_fd[i]);
                videoInput.shared_fd[i] = dma_info->shared_fd[i];
            }

            videoInput.num_planes = handle->mNumPlanes;
            VLOG(INFO, "videoInput.num_planes=%d", videoInput.num_planes);
        } else {
            unsigned long* in = in_buffer_info->buf_info.in_ptr;
            videoInput.YCbCr[0] = (unsigned long)in[0];
            videoInput.YCbCr[1] = (unsigned long)(videoInput.YCbCr[0] + videoInput.height * videoInput.pitch);
            if (videoInput.fmt == AMVENC_NV21 || videoInput.fmt == AMVENC_NV12) {
                videoInput.YCbCr[2] = 0;
            } else if (videoInput.fmt == AMVENC_RGB888) {
                videoInput.YCbCr[1] = 0;
                videoInput.YCbCr[2] = 0;
            } else if (videoInput.fmt == AMVENC_YUV420) {
                videoInput.YCbCr[2] = (unsigned long)(videoInput.YCbCr[1] + videoInput.height * videoInput.pitch / 4);
            }
        }
        //videoInput.fmt = handle->fmt;
        videoInput.canvas = 0xffffffff;
        videoInput.type = handle->bufType;
        videoInput.disp_order = handle->mNumInputFrames;
        videoInput.op_flag = 0;

        if (handle->mKeyFrameRequested == true) {
            videoInput.op_flag = AMVEncFrameIO_FORCE_IDR_FLAG;
            handle->mKeyFrameRequested = false;
            VLOG(INFO, "Force encode a IDR frame at %d frame", handle->mNumInputFrames);
        }
        //if (handle->idrPeriod == 0) {
            //videoInput.op_flag |= AMVEncFrameIO_FORCE_IDR_FLAG;
        //}
        ret = AML_HEVCSetInput(handle->am_enc_handle, &videoInput);
        ++(handle->mNumInputFrames);

        VLOG(DEBUG, "AML_HEVCSetInput ret %d\n", ret);
        if (ret < AMVENC_SUCCESS) {
            VLOG(ERR, "encoderStatus = %d at line %d, handle: %p", ret, __LINE__, (void *)handle);
            result.is_valid = false;
            if (ret == AMVENC_FAIL)
               result.err_cod = AMVENC_INVALID_PARAM;
            else
                result.err_cod = ret;
            return result;
        }

        ret = AML_HEVCEncNAL(handle->am_enc_handle, out, (unsigned int *)&dataLength, &type);
        if (ret == AMVENC_PICTURE_READY) {
            if ((type == 0) || handle->mNumInputFrames == 1) {
                if ((handle->mPrependSPSPPSToIDRFrames ||
                     handle->mNumInputFrames == 1) &&
                    (handle->mSPSPPSData)) {
                    vl_encoder_adjust_h265_sei(handle, videoInput.op_flag);
                    memmove(out + handle->mSPSPPSDataSize + handle->mSEIDataSize, out, dataLength);
                    memcpy(out, handle->mSPSPPSData, handle->mSPSPPSDataSize);
                    memcpy(out + handle->mSPSPPSDataSize, handle->mSEIData, handle->mSEIDataSize);
                    dataLength += (handle->mSPSPPSDataSize + handle->mSEIDataSize);
                    VLOG(DEBUG, "copy mSPSPPSData to buffer size= %d at line %d \n", handle->mSPSPPSDataSize, __LINE__);
                } else {
                    vl_encoder_adjust_h265_sei(handle, videoInput.op_flag);
                    memmove(out + handle->mSEIDataSize, out, dataLength);
                    memcpy(out, handle->mSEIData, handle->mSEIDataSize);
                    dataLength += handle->mSEIDataSize;
                }
                result.is_key_frame = true;
            }
        } else if ((ret == AMVENC_SKIPPED_PICTURE) || (ret == AMVENC_TIMEOUT)) {
            dataLength = 0;
            if (ret == AMVENC_TIMEOUT) {
                handle->mKeyFrameRequested = true;
                ret = AMVENC_SKIPPED_PICTURE;
            }
            VLOG(DEBUG, "ret = %d at line %d, handle: %p", ret, __LINE__, (void *)handle);
        } else if (ret != AMVENC_SUCCESS) {
            dataLength = 0;
            VLOG(ERR, "encoderStatus = %d at line %d, handle: %p", ret , __LINE__, (void *)handle);
        }
        if (ret < AMVENC_SUCCESS) {
            VLOG(ERR, "encoderStatus = %d, handle: %p", ret,(void*)handle);
            result.is_valid = false;
            if (ret == AMVENC_FAIL)
                result.err_cod = AMVENC_INVALID_PARAM;
            else
                result.err_cod = ret;
            return result;
        }
    }
    result.is_valid = true;
    result.encoded_data_length_in_bytes = dataLength;
    return result;
}

int vl_video_encoder_change_bitrate_hevc(vl_codec_handle_hevc_t codec_handle,
                            int bitRate)
{
    int ret;
    AMVHEVCEncHandle *handle = (AMVHEVCEncHandle *)codec_handle;

    if (handle->am_enc_handle == 0) //not init the encoder yet
        return -1;
    if (handle->mEncParams.param_change_enable == 0) //no change enabled
        return -2;
    ret = AML_HEVCEncChangeBitRate(handle->am_enc_handle, bitRate);
    if (ret != AMVENC_SUCCESS)
        return -3;
    //hist_reset(handle); //bitrate target change reset the hist
    handle->mEncParams.bitrate = bitRate;
    return 0;
}

int vl_video_encoder_getavgqp(vl_codec_handle_hevc_t codec_handle, int *avg_qp) {
    int ret;
    AMVHEVCEncHandle *handle = (AMVHEVCEncHandle *)codec_handle;
    int avgqp;

    ret = AML_HEVCEncGetAvgQp(handle->am_enc_handle, &avgqp);
    if (ret == AMVENC_SUCCESS) {
        *avg_qp = avgqp;
    } else {
        VLOG(ERR, "get avgqp fail! ret = %d at line %d", ret, __LINE__);
    }
    return ret;
}

int vl_video_encoder_change_framerate_hevc(vl_codec_handle_hevc_t codec_handle,
                            int frameRate,int bitRate)
{
    int ret;
    AMVHEVCEncHandle *handle = (AMVHEVCEncHandle *)codec_handle;

    if (handle->am_enc_handle == 0) //not init the encoder yet
        return -1;
    if (handle->mEncParams.param_change_enable == 0) //no change enabled
        return -2;
    ret = AML_HEVCEncChangeFrameRate(handle->am_enc_handle, frameRate, bitRate);
    if (ret != AMVENC_SUCCESS)
        return -3;
    //hist_reset(handle); //bitrate target change reset the hist
    handle->mEncParams.frame_rate = frameRate;
    return 0;
}


int vl_video_encoder_destroy_hevc(vl_codec_handle_hevc_t codec_handle) {
    AMVHEVCEncHandle *handle = (AMVHEVCEncHandle *)codec_handle;
    AML_HEVCRelease(handle->am_enc_handle);
    if (handle->mSPSPPSData)
        free(handle->mSPSPPSData);
    if (handle->mSEIData)
        free(handle->mSEIData);
    if (handle)
        delete handle;
    return 1;
}
