#ifndef _INCLUDED_COM_VIDEOPHONE_CODEC
#define _INCLUDED_COM_VIDEOPHONE_CODEC

#ifdef __cplusplus
extern "C" {
#endif

#define vl_codec_handle_t long

typedef enum vl_codec_id {
    CODEC_ID_NONE,
    CODEC_ID_VP8,
    CODEC_ID_H261,
    CODEC_ID_H263,
    CODEC_ID_H264, /* must support */
    CODEC_ID_H265,

} vl_codec_id_t;

typedef enum vl_img_format
{
    IMG_FMT_NONE,
    IMG_FMT_NV12, /* must support  */
    IMG_FMT_NV21,
    IMG_FMT_YV12,
    IMG_FMT_RGB888,
    IMG_FMT_BGR888,
    IMG_FMT_RGBA8888,
} vl_img_format_t;

typedef enum vl_frame_type
{
    FRAME_TYPE_NONE,
    FRAME_TYPE_AUTO, /* encoder self-adaptation(default) */
    FRAME_TYPE_IDR,
    FRAME_TYPE_I,
    FRAME_TYPE_P,
} vl_frame_type_t;

typedef enum vl_buffer_type
{
    VMALLOC_BUFFER_TYPE = 0,
    CANVAS_BUFFER_TYPE = 1,
    PHYSICAL_BUFF_TYPE = 2,
    DMA_BUFF_TYPE = 3,
    MAX_TYPE_TYPE = 4,
} vl_buffer_type_e;

typedef enum vl_h_enc_result
{
    ENC_TIMEOUT = -3,
    ENC_SKIPPED_PICTURE,
    ENC_FAILED,
    ENC_SUCCESS,
    ENC_IDR_FRAME,
}vl_enc_result_e;

typedef enum vl_h_enc_csc {
    ENC_CSC_BT601,
    ENC_CSC_BT709
}vl_h_enc_csc_e;

typedef enum vl_h_enc_profile{
    ENC_AVC_BASELINE = 66,
    ENC_AVC_MAIN = 77,
    ENC_AVC_EXTENDED = 88,
    ENC_AVC_HIGH = 100,
    ENC_AVC_HIGH10 = 110,
    ENC_AVC_HIGH422 = 122,
    ENC_AVC_HIGH444 = 144
} vl_h_enc_profile_e;

typedef enum vl_h_enc_level{
    ENC_AVC_LEVEL_AUTO = 0,
    ENC_AVC_LEVEL1_B = 9,
    ENC_AVC_LEVEL1 = 10,
    ENC_AVC_LEVEL1_1 = 11,
    ENC_AVC_LEVEL1_2 = 12,
    ENC_AVC_LEVEL1_3 = 13,
    ENC_AVC_LEVEL2 = 20,
    ENC_AVC_LEVEL2_1 = 21,
    ENC_AVC_LEVEL2_2 = 22,
    ENC_AVC_LEVEL3 = 30,
    ENC_AVC_LEVEL3_1 = 31,
    ENC_AVC_LEVEL3_2 = 32,
    ENC_AVC_LEVEL4 = 40,
    ENC_AVC_LEVEL4_1 = 41,
    ENC_AVC_LEVEL4_2 = 42,
    ENC_AVC_LEVEL5 = 50,
    ENC_AVC_LEVEL5_1 = 51
} vl_h_enc_level_e;


typedef struct vl_vui_params {
    int primaries;
    int transfer;
    int matrixCoeffs;
    unsigned char range;
}vl_vui_params_t;

typedef struct vl_init_params
{
    int width;
    int height;
    int frame_rate;
    int bit_rate;
    int gop;
    int i_qp_min;
    int i_qp_max;
    int p_qp_min;
    int p_qp_max;
    vl_h_enc_csc_e csc;
    vl_h_enc_profile_e profile;
    vl_h_enc_level_e level;
}vl_init_params_t;

typedef struct vl_sei_payload_t
{
    int payload_size;
    int payload_type;
    unsigned char *payload;
} vl_sei_payload_t;

typedef struct vl_frame_info
{
    unsigned long YCbCr[3];
    vl_buffer_type_e type;
    vl_frame_type_t frame_type;
    unsigned long frame_size;
    vl_img_format_t fmt; //1:nv12 2:nv21 3:YUV420 4:rgb888 5:bgr888
    int pitch;
    int height;
    unsigned int coding_timestamp;
    unsigned int canvas;
    unsigned int scale_width;
    unsigned int scale_height;
    unsigned int crop_left;
    unsigned int crop_right;
    unsigned int crop_top;
    unsigned int crop_bottom;
    unsigned int bitrate;
    unsigned int plane_num;
    unsigned int frame_rate;
    vl_sei_payload_t sei_payload;
    vl_sei_payload_t sei_userdata;
} vl_frame_info_t;

typedef struct vl_dma_info {
    int shared_fd[3];
    unsigned int num_planes;//for nv12/nv21, num_planes can be 1 or 2
} vl_dma_info_t;

/**
 * Getting version information
 *
 *@return : version information
 */
const char *vl_get_version();

/**
 * init encoder
 *
 *@param : codec_id: codec type
 *@param : width: video width
 *@param : height: video height
 *@param : frame_rate: framerate
 *@param : bit_rate: bitrate
 *@param : gop GOP: max I frame interval
 *@param : img_format: image format
 *@param : i_qp_min
 *@param : i_qp_max
 *@param : p_qp_min
 *@param : p_qp_max
 *@return : if success return encoder handle,else return <= 0
 */
vl_codec_handle_t vl_video_encoder_init(vl_codec_id_t codec_id, vl_init_params_t init_param);
/**
 * init encoder with const qp mode
 *
 *@param : codec_id: codec type
 *@param : width: video width
 *@param : height: video height
 *@param : frame_rate: framerate
 *@param : bit_rate: bitrate
 *@param : gop GOP: max I frame interval
 *@param : img_format: image format
 *@param : fix_qp: fix qp
 *@return : if success return encoder handle,else return <= 0
 */
vl_codec_handle_t vl_video_encoder_init_fix_qp(vl_codec_id_t codec_id, int width, int height, int frame_rate, int bit_rate, int gop, vl_img_format_t img_format, int fix_qp);

/**
 * encode video header
 *
 *@param : handle
 *@param : in_size: data size
 *@param : out: data output,H.264 need header(0x00，0x00，0x00，0x01),and format must be I420(apk set param out，through jni,so modify "out" in the function,don't change address point)
 *@return ：if success return encoded data length,else return <= 0
 */
int vl_video_encode_header(vl_codec_handle_t codec_handle, vl_vui_params_t vui,int in_size, unsigned char *out);

/**
 * encode video
 *
 *@param : handle
 *@param : type: frame type
 *@param : in: data to be encoded
 *@param : in_size: data size
 *@param : out: data output,H.264 need header(0x00，0x00，0x00，0x01),and format must be I420(apk set param out，through jni,so modify "out" in the function,don't change address point)
 *@return ：if success return encoded data length,else return <= 0
 */

int vl_video_encoder_encode(vl_codec_handle_t handle, vl_frame_type_t type, unsigned char *in, int in_size, unsigned char *out, int format);

/**
 * encode video frame
 *
 *@param : handle
 *@param : frame_info:frame info encoder need
 *@param : out_size: data size
 *@param : out: data output,H.264 need header(0x00，0x00，0x00，0x01),and format must be I420(apk set param out，through jni,so modify "out" in the function,don't change address point)
 *@return ：if success return 0,else return <= 0
 */
vl_enc_result_e vl_video_encoder_encode_frame(vl_codec_handle_t codec_handle, vl_frame_info_t frame_info, unsigned char *out,int *out_size,vl_frame_type_t *frame_type);


vl_enc_result_e vl_video_encoder_getavgqp(vl_codec_handle_t handle, float *avg_qp);



/**
 * destroy encoder
 *
 *@param ：handle: encoder handle
 *@return ：if success return 1,else return 0
 */
int vl_video_encoder_destroy(vl_codec_handle_t handle);

/**
 * init decoder
 *
 *@param : codec_id: decoder type
 *@return : if success return decoder handle,else return <= 0
 */
//    vl_codec_handle_t vl_video_decoder_init(vl_codec_id_t codec_id);

/**
 * decode video
 *
 *@param : handle: decoder handle
 *@param : in: data to be decoded
 *@param : in_size: data size
 *@param : out: data output, intenal set
 *@return ：if success return decoded data length, else return <= 0
 */
//    int vl_video_decoder_decode(vl_codec_handle_t handle, char *in, int in_size, char **out);

/**
 * destroy decoder
 *@param : handle: decoderhandle
 *@return ：if success return 1, else return 0
 */
//    int vl_video_decoder_destroy(vl_codec_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif /* _INCLUDED_COM_VIDEOPHONE_CODEC */
