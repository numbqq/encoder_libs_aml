#include <string.h>
#include <dlfcn.h>
#include <pthread.h>

#include "vpcodec_1_0.h"
#include "amvenc_common.h"
#include "amvenc.h"

//_need_modify_待区分Android和Linux====
#define LIB_ENCODER_API_NAME "libamvenc_264.so"

typedef vl_codec_handle_t (*fn_init)(vl_codec_id_t codec_id,
                                        vl_init_params_t init_param);

typedef int (*fn_generate_header)(vl_codec_handle_t codec_handle,
                                            vl_vui_params_t vui,
                                            int in_size,
                                            unsigned char *out);
typedef int (*fn_encode)(vl_codec_handle_t codec_handle,
                                vl_frame_info_t frame_info,
                                unsigned char *out,
                                int *out_size,
                                vl_frame_type_t *frame_type);
//_need_modify_待实现
typedef int (*fn_change_bitrate)(vl_codec_handle_t handle, int bitRate);
typedef int (*fn_destroy)(vl_codec_handle_t handle);

typedef struct {
    int load_count;
    void *libHandle;
    fn_init init;
    fn_generate_header generate_header;
    fn_encode encode;
    fn_change_bitrate change_bitrate;
    fn_destroy destroy;
}vl_encoder_ops_s;

static vl_encoder_ops_s encoder_ops = {0};
static pthread_mutex_t encoder_ops_mutex = PTHREAD_MUTEX_INITIALIZER;

int amvenc_264_load_Module(void)
{
    VLOG(DEBUG, "amvenc_ports hcodec initModule!");
    pthread_mutex_lock(&encoder_ops_mutex);
    encoder_ops.load_count ++;
    if (encoder_ops.load_count == 1) {
        encoder_ops.libHandle = dlopen(LIB_ENCODER_API_NAME, RTLD_NOW | RTLD_NODELETE);
        if (encoder_ops.libHandle != NULL) {
            encoder_ops.init = (fn_init)dlsym(encoder_ops.libHandle, "vl_video_encoder_init");
            if (encoder_ops.init == NULL) {
                VLOG(ERR, "dlsym for vl_video_encoder_init failed");
                goto exit;
            }

            encoder_ops.generate_header = (fn_generate_header)dlsym(encoder_ops.libHandle, "vl_video_encode_header");
            if (encoder_ops.generate_header == NULL) {
                VLOG(ERR, "dlsym for vl_multi_encoder_generate_header failed");
                goto exit;
            }

            /*encoder_ops.change_bitrate = (fn_change_bitrate)dlsym(encoder_ops.libHandle, "vl_video_encoder_change_bitrate");
            if (encoder_ops.change_bitrate == NULL) {
                VLOG(ERR, "dlsym for vl_video_encoder_change_bitrate failed");
                goto exit;
            }*/

            encoder_ops.encode = (fn_encode)dlsym(encoder_ops.libHandle, "vl_video_encoder_encode_frame");
            if (encoder_ops.encode == NULL) {
                VLOG(ERR, "dlsym for vl_video_encoder_encode failed");
                goto exit;
            }
            encoder_ops.destroy = (fn_destroy)dlsym(encoder_ops.libHandle, "vl_video_encoder_destroy");
            if (encoder_ops.destroy == NULL) {
                VLOG(ERR, "dlsym for vl_video_encoder_destroy failed,err:%s",dlerror());
                goto exit;
            }
        } else {
            VLOG(ERR, "dlopen for %s failed,err:%s", LIB_ENCODER_API_NAME, dlerror());
            encoder_ops.load_count --;
            pthread_mutex_unlock(&encoder_ops_mutex);
            return -1;
        }
    }
    pthread_mutex_unlock(&encoder_ops_mutex);
    return 0;
exit:
    dlclose(encoder_ops.libHandle);
    encoder_ops.load_count --;
    pthread_mutex_unlock(&encoder_ops_mutex);
    return -1;
}

amvenc_handle_t amvenc_264_init(amvenc_codec_id_t vcodec_id,
                                       amvenc_info_t vcodec_info,
                                        amvenc_qp_param_t* qp_param)
{
    vl_init_params_t init_param;
    vl_codec_handle_t handle_enc = 0;

    if (qp_param == NULL) {
        VLOG(ERR, "invalid param, vqp_tbl is NULL!\n");
        return 0;
    }
    memset(&init_param, 0, sizeof(init_param));

    init_param.width = vcodec_info.width;
    init_param.height = vcodec_info.height;
    init_param.frame_rate = vcodec_info.frame_rate;
    init_param.bit_rate = vcodec_info.bit_rate;
    init_param.gop = vcodec_info.gop;
    init_param.i_qp_min = qp_param->qp_I_min;
    init_param.i_qp_max = qp_param->qp_I_max;
    init_param.p_qp_min = qp_param->qp_P_min;
    init_param.p_qp_max = qp_param->qp_P_max;

    //init_param.csc = 0;
    //init_param.profile = vcodec_info.profile;//待改
    //init_param.level = 0;

    if (encoder_ops.init)
        handle_enc = encoder_ops.init(CODEC_ID_H264, init_param);

    return (amvenc_handle_t)handle_enc;
}

amvenc_metadata_t amvenc_264_generate_header(amvenc_handle_t handle,
                                                              unsigned char *pHeader,
                                                              unsigned int *pLength)
{
    int ret = -1;
    vl_vui_params_t vui;
    amvenc_metadata_t result;

    memset(&vui, 0, sizeof(vui));
    memset(&result, 0, sizeof(result));

    //_need_modify_
    //vui.primaries = 0;
    //vui.transfer = 0;
    //vui.matrixCoeffs = 0;
    //vui.range = 0;
    if (encoder_ops.generate_header)
        ret = encoder_ops.generate_header((vl_codec_handle_t)handle, vui, *pLength, pHeader);

    if (ret <= 0) {
        result.is_valid = false;
        return result;
    }
    result.encoded_data_length_in_bytes = *pLength;
    //result.is_key_frame = encoding_metadata.is_key_frame;
    //result.timestamp_us = encoding_metadata.timestamp_us;
    result.is_valid = true;
    //result.extra.frame_type = encoding_metadata.extra.frame_type;
    //result.extra.average_qp_value = encoding_metadata.extra.average_qp_value;
    //result.extra.intra_blocks = encoding_metadata.extra.intra_blocks;
    //result.extra.merged_blocks = encoding_metadata.extra.merged_blocks;
    //result.extra.skipped_blocks = encoding_metadata.extra.skipped_blocks;
    //result.err_cod = encoding_metadata.err_cod;

    return result;
}

amvenc_metadata_t amvenc_264_encode(amvenc_handle_t handle,
                                           amvenc_frame_info_t frame_info,
                                           amvenc_frame_type_t type,
                                           unsigned char* out,
                                           amvenc_buffer_info_t *in_buffer_info,
                                           amvenc_buffer_info_t *ret_buffer_info)
{
    int datalen = 0;
    vl_frame_info_t enc_frame_info;
    vl_frame_type_t frame_type;
    vl_enc_result_e enc_result = ENC_FAILED;

    amvenc_metadata_t result;

    memset(&result, 0, sizeof(result));
    memset(&enc_frame_info, 0, sizeof(enc_frame_info));

    if ((in_buffer_info == NULL) || (ret_buffer_info == NULL)) {
      VLOG(ERR, "invalid input buffer_info\n");
      result.is_valid = false;
      return result;
    }

    enc_frame_info.frame_type = FRAME_TYPE_AUTO;
    switch (type)
    {
        case AML_FRAME_TYPE_AUTO:
            enc_frame_info.frame_type = FRAME_TYPE_AUTO;
            break;
        case AML_FRAME_TYPE_I:
            enc_frame_info.frame_type = FRAME_TYPE_I;
            break;
        default:
            break;
    }

    enc_frame_info.frame_size = frame_info.frame_size;

    enc_frame_info.pitch = frame_info.pitch;
    enc_frame_info.height = frame_info.height;

    enc_frame_info.coding_timestamp = frame_info.coding_timestamp;
    enc_frame_info.scale_width = frame_info.scale_width;
    enc_frame_info.scale_height = frame_info.scale_height;
    enc_frame_info.crop_left = frame_info.crop_left;
    enc_frame_info.crop_right = frame_info.crop_right;
    enc_frame_info.crop_top = frame_info.crop_top;
    enc_frame_info.crop_bottom = frame_info.crop_bottom;
    enc_frame_info.bitrate = frame_info.bitrate;
    enc_frame_info.frame_rate = frame_info.frame_rate * 1000;
    enc_frame_info.sei_payload.payload_size = frame_info.sei_payload.payload_size;
    enc_frame_info.sei_payload.payload_type = frame_info.sei_payload.payload_type;
    enc_frame_info.sei_payload.payload = frame_info.sei_payload.payload;

    if (in_buffer_info->buf_type == AML_CANVAS_TYPE) {
        enc_frame_info.type = CANVAS_BUFFER_TYPE;
        enc_frame_info.canvas = in_buffer_info->buf_info.canvas;
    }else if (in_buffer_info->buf_type == AML_DMA_TYPE) {
        enc_frame_info.type = DMA_BUFF_TYPE;
        enc_frame_info.YCbCr[0] = in_buffer_info->buf_info.dma_info.shared_fd[0];
        enc_frame_info.YCbCr[1] = in_buffer_info->buf_info.dma_info.shared_fd[1];
        enc_frame_info.YCbCr[2] = in_buffer_info->buf_info.dma_info.shared_fd[2];
        enc_frame_info.plane_num = in_buffer_info->buf_info.dma_info.num_planes;
    } else {
        enc_frame_info.type = VMALLOC_BUFFER_TYPE;
        enc_frame_info.YCbCr[0] = in_buffer_info->buf_info.in_ptr[0];
        enc_frame_info.YCbCr[1] = in_buffer_info->buf_info.in_ptr[1];
        enc_frame_info.YCbCr[2] = in_buffer_info->buf_info.in_ptr[2];
    }

    switch (in_buffer_info->buf_fmt)
    {
        case AML_IMG_FMT_NV12:
            enc_frame_info.fmt = IMG_FMT_NV12;
            break;
        case AML_IMG_FMT_NV21:
            enc_frame_info.fmt = IMG_FMT_NV21;
            break;
        case AML_IMG_FMT_YUV420P:
            enc_frame_info.fmt = IMG_FMT_YV12;
            break;
        case AML_IMG_FMT_RGB888:
            enc_frame_info.fmt = IMG_FMT_RGB888;
            break;
        default:
            break;
    }

    if (encoder_ops.encode)
        enc_result = encoder_ops.encode((vl_codec_handle_t)handle, enc_frame_info, out, &datalen, &frame_type);

    if (enc_result == ENC_FAILED) {
        result.is_valid = false;
        return result;
    }
    ret_buffer_info->buf_type = in_buffer_info->buf_type;

    ret_buffer_info->buf_info.dma_info.shared_fd[0] = in_buffer_info->buf_info.dma_info.shared_fd[0];
    ret_buffer_info->buf_info.dma_info.shared_fd[1] = in_buffer_info->buf_info.dma_info.shared_fd[1];
    ret_buffer_info->buf_info.dma_info.shared_fd[2] = in_buffer_info->buf_info.dma_info.shared_fd[2];
    ret_buffer_info->buf_info.dma_info.num_planes = in_buffer_info->buf_info.dma_info.num_planes;

    ret_buffer_info->buf_info.in_ptr[0] = in_buffer_info->buf_info.in_ptr[0];
    ret_buffer_info->buf_info.in_ptr[1] = in_buffer_info->buf_info.in_ptr[1];
    ret_buffer_info->buf_info.in_ptr[2] = in_buffer_info->buf_info.in_ptr[2];
    ret_buffer_info->buf_info.canvas = in_buffer_info->buf_info.canvas;

    ret_buffer_info->buf_stride = in_buffer_info->buf_stride;

    ret_buffer_info->buf_fmt = in_buffer_info->buf_fmt;

    result.encoded_data_length_in_bytes = datalen;
    //result.is_key_frame = encoding_metadata.is_key_frame;
    //result.timestamp_us = encoding_metadata.timestamp_us;
    result.is_valid = true;
    result.extra.frame_type = frame_type;
    //result.extra.average_qp_value = encoding_metadata.extra.average_qp_value;
    //result.extra.intra_blocks = encoding_metadata.extra.intra_blocks;
    //result.extra.merged_blocks = encoding_metadata.extra.merged_blocks;
    //result.extra.skipped_blocks = encoding_metadata.extra.skipped_blocks;
    //result.err_cod = encoding_metadata.err_cod;

    return result;
}

int amvenc_264_change_bitrate(amvenc_handle_t handle,          int bitRate)
{
    if (encoder_ops.change_bitrate)
        return encoder_ops.change_bitrate((vl_codec_handle_t)handle, bitRate);
    return 0;
}

int amvenc_264_destroy(amvenc_handle_t handle)
{
    if (encoder_ops.destroy)
        return encoder_ops.destroy((vl_codec_handle_t)handle);
    return 0;
}

int amvenc_264_unload_Module(void)
{
    pthread_mutex_lock(&encoder_ops_mutex);
    encoder_ops.load_count --;
    if (encoder_ops.load_count == 0) {
        if (encoder_ops.libHandle) {
            VLOG(DEBUG, "Unloading encoder api lib\n");
            dlclose(encoder_ops.libHandle);
        }
    }
    pthread_mutex_unlock(&encoder_ops_mutex);
    return 0;
}

