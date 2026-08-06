#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "amvenc_common.h"
#include "amvenc.h"

#include "amvenc_multi.h"
#include "amvenc_264.h"
#include "amvenc_265.h"
#include "amvenc_vers.h"
#include "amvenc_tahoe.h"


#define AMVENC_MULTI_DEVICE "/dev/amvenc_multi"
#define AMVENC_264_DEVICE   "/dev/amvenc_avc"
#define AMVENC_265_DEVICE   "/dev/HevcEnc"
#define AMVENC_VERS_DEVICE   "/dev/vc8000"
#define AMVENC_TAHOE_DEVICE  "/dev/tahoe"
#define AMVENC_TAHOE_V4L_DEVICE  "/dev/video90"

typedef enum amvenc_hw_e {
    AMVENC_UNKNOWN,
    AMVENC_MULTI,
    AMVENC_264,
    AMVENC_265,
    AMVENC_VERS,
    AMVENC_TAHOE,
    AMVENC_MAX,
} amvenc_hw_t;

struct amvenc_manager_s {
    int (*loadModule) (void);
    amvenc_handle_t (*init) (amvenc_codec_id_t codec_id,
                                    amvenc_info_t encode_info,
                                    amvenc_qp_param_t* qp_tbl);
    amvenc_metadata_t (*generate_header) (amvenc_handle_t handle,
                                     unsigned char *pHeader,
                                     unsigned int *pLength);
    amvenc_metadata_t (*encode) (amvenc_handle_t handle,
                           amvenc_frame_info_t frame_info,
                           amvenc_frame_type_t type,
                           unsigned char* out,
                           amvenc_buffer_info_t *in_buffer_info,
                           amvenc_buffer_info_t *ret_buffer_info);
    int (*change_bitrate) (amvenc_handle_t handle, int bitRate);
    int (*destroy) (amvenc_handle_t handle);
    int (*unloadModule) (void);
};

static const struct amvenc_manager_s amvenc_multi_data = {
    .loadModule = amvenc_multi_load_Module,
    .init = amvenc_multi_init,
    .generate_header = amvenc_multi_generate_header,
    .encode = amvenc_multi_encode,
    .change_bitrate = amvenc_multi_change_bitrate,
    .destroy = amvenc_multi_destroy,
    .unloadModule = amvenc_multi_unload_Module,
};

static const struct amvenc_manager_s amvenc_264_data = {
    .loadModule = amvenc_264_load_Module,
    .init = amvenc_264_init,
    .generate_header = amvenc_264_generate_header,
    .encode = amvenc_264_encode,
    .change_bitrate = amvenc_264_change_bitrate,
    .destroy = amvenc_264_destroy,
    .unloadModule = amvenc_264_unload_Module,
};

static const struct amvenc_manager_s amvenc_265_data = {
    .loadModule = amvenc_265_load_Module,
    .init = amvenc_265_init,
    .generate_header = amvenc_265_generate_header,
    .encode = amvenc_265_encode,
    .change_bitrate = amvenc_265_change_bitrate,
    .destroy = amvenc_265_destroy,
    .unloadModule = amvenc_265_unload_Module,
};

static const struct amvenc_manager_s amvenc_vers_data = {
    .loadModule = amvenc_vers_load_Module,
    .init = amvenc_vers_init,
    .generate_header = amvenc_vers_generate_header,
    .encode = amvenc_vers_encode,
    .change_bitrate = amvenc_vers_change_bitrate,
    .destroy = amvenc_vers_destroy,
    .unloadModule = amvenc_vers_unload_Module,
};

static const struct amvenc_manager_s amvenc_tahoe_data = {
    .loadModule = amvenc_tahoe_load_Module,
    .init = amvenc_tahoe_init,
    .generate_header = amvenc_tahoe_generate_header,
    .encode = amvenc_tahoe_encode,
    .change_bitrate = amvenc_tahoe_change_bitrate,
    .destroy = amvenc_tahoe_destroy,
    .unloadModule = amvenc_tahoe_unload_Module,
};

const static struct amvenc_manager_s *amvenc_manager[] = {
    NULL,
    &amvenc_multi_data,
    &amvenc_264_data,
    &amvenc_265_data,
    &amvenc_vers_data,
    &amvenc_tahoe_data,
    NULL,
};

typedef struct amvencHandle_s {
    amvenc_hw_t hw_type;
    amvenc_handle_t codec_handle;
} amvencHandle;


int selectHW(amvenc_codec_id_t codec_id, int *hw_type)
{
    int ret = -1;
    VLOG(DEBUG, "selectEncoder");
    if (AML_CODEC_ID_H264 == codec_id) {
        if (access(AMVENC_MULTI_DEVICE, F_OK ) != -1) {
            *hw_type = AMVENC_MULTI;
            VLOG(DEBUG, "amvenc_multi 264 present");
            ret = 0;
        }
        else if (access(AMVENC_264_DEVICE, F_OK ) != -1) {
            *hw_type = AMVENC_264;
            VLOG(DEBUG, "amvenc_264 present");
            ret = 0;
        }
        else if (access(AMVENC_VERS_DEVICE, F_OK ) != -1) {
            *hw_type = AMVENC_VERS;
            VLOG(DEBUG, "amvenc_vers present");
            ret = 0;
        }
        else if ((access(AMVENC_TAHOE_DEVICE, F_OK ) != -1) || (access(AMVENC_TAHOE_V4L_DEVICE, F_OK )!= -1)) {
            *hw_type = AMVENC_TAHOE;
            VLOG(DEBUG, "amvenc_tahoe present");
            ret = 0;
        }
        else {
            VLOG(ERR, "can not find available h.264 driver!!!,please check!");
        }
    }
    if (AML_CODEC_ID_H265 == codec_id) {
        if (access(AMVENC_MULTI_DEVICE, F_OK ) != -1) {
            *hw_type = AMVENC_MULTI;
            VLOG(DEBUG, "amvenc_multi 265 present");
            ret = 0;
        }
        else if (access(AMVENC_265_DEVICE, F_OK ) != -1) {
            *hw_type = AMVENC_265;
            VLOG(DEBUG, "amvenc_265 present");
            ret = 0;
        }
        else if (access(AMVENC_VERS_DEVICE, F_OK ) != -1) {
            *hw_type = AMVENC_VERS;
            VLOG(DEBUG, "amvenc_vers present");
            ret = 0;
        }
        else if ((access(AMVENC_TAHOE_DEVICE, F_OK ) != -1) || (access(AMVENC_TAHOE_V4L_DEVICE, F_OK )!= -1)) {
            *hw_type = AMVENC_TAHOE;
            VLOG(DEBUG, "amvenc_tahoe present");
            ret = 0;
        }
        else {
            VLOG(ERR, "can not find available h.265 driver!!!,please check!");
        }
    }
    return ret;
}

amvenc_handle_t amvenc_init(amvenc_codec_id_t codec_id,
                                    amvenc_info_t encode_info,
                                    amvenc_qp_param_t* qp_tbl)
{
    int ret = -1;
    int hw_type = 0;
    amvencHandle* mHandle = (amvencHandle *) malloc(sizeof(amvencHandle));

    if (mHandle == NULL)
        goto exit;

    amvenc_set_log_level(INFO);
    ret = selectHW(codec_id, &hw_type);
    if (ret != 0)
        goto exit;
    mHandle->hw_type = hw_type;
    if (NULL == amvenc_manager[mHandle->hw_type])
        goto exit;
    if (NULL == amvenc_manager[mHandle->hw_type]->loadModule)
        goto exit;
    ret = amvenc_manager[mHandle->hw_type]->loadModule();
    if (ret != 0)
        goto exit;

    if (NULL == amvenc_manager[mHandle->hw_type]->init)
        goto exit;
    mHandle->codec_handle = amvenc_manager[mHandle->hw_type]->init(codec_id, encode_info, qp_tbl);
    if (mHandle->codec_handle == 0)
        goto exit;

    return (amvenc_handle_t)mHandle;
exit:
    if (mHandle != NULL)
        free(mHandle);
    return (amvenc_handle_t)NULL;
}

amvenc_metadata_t amvenc_generate_header(amvenc_handle_t handle,
                                                     unsigned char *pHeader,
                                                     unsigned int *pLength)
{
    amvencHandle* mHandle = (amvencHandle *)handle;
    amvenc_metadata_t result;

    if (mHandle == NULL)
        goto exit;
    memset(&result, 0, sizeof(result));

    if (amvenc_manager[mHandle->hw_type]) {
        if (amvenc_manager[mHandle->hw_type]->generate_header)
            return amvenc_manager[mHandle->hw_type]->generate_header(mHandle->codec_handle, pHeader, pLength);
    }
exit:
    result.is_valid = false;
    return result;
}


amvenc_metadata_t amvenc_encode(amvenc_handle_t handle,
                                           amvenc_frame_info_t frame_info,
                                           amvenc_frame_type_t type,
                                           unsigned char* out,
                                           amvenc_buffer_info_t *in_buffer_info,
                                           amvenc_buffer_info_t *ret_buffer_info)
{
    amvencHandle* mHandle = (amvencHandle *)handle;
    amvenc_metadata_t result;

    if (mHandle == NULL)
        goto exit;
    memset(&result, 0, sizeof(result));
    if (amvenc_manager[mHandle->hw_type]) {
        if (amvenc_manager[mHandle->hw_type]->encode)
            return amvenc_manager[mHandle->hw_type]->encode(mHandle->codec_handle, frame_info, type, out, in_buffer_info, ret_buffer_info);
    }
exit:
    result.is_valid = false;
    return result;
}

int amvenc_change_bitrate(amvenc_handle_t handle,
                            int bitRate)
{
    amvencHandle* mHandle = (amvencHandle *)handle;
    if (mHandle == NULL)
        return -1;
    if (amvenc_manager[mHandle->hw_type]) {
        if (amvenc_manager[mHandle->hw_type]->change_bitrate)
            return amvenc_manager[mHandle->hw_type]->change_bitrate(mHandle->codec_handle, bitRate);
    }
    return -1;
}

int amvenc_destroy(amvenc_handle_t handle)
{
    amvencHandle* mHandle = (amvencHandle *)handle;
    if (mHandle == NULL)
        return -1;
    if (amvenc_manager[mHandle->hw_type]) {
        if (amvenc_manager[mHandle->hw_type]->destroy)
            amvenc_manager[mHandle->hw_type]->destroy(mHandle->codec_handle);

        if (amvenc_manager[mHandle->hw_type]->unloadModule)
            amvenc_manager[mHandle->hw_type]->unloadModule();
    }

    if (mHandle)
        free(mHandle);
    return 0;
}


