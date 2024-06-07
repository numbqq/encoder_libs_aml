#ifndef __AMVENC_H__
#define __AMVENC_H__

#ifdef __cplusplus
extern "C" {
#endif
#include <stdbool.h>
#include <stdint.h>

#define amvenc_handle_t long

typedef struct amvenc_frame_extra_info {
    int frame_type; /* encoded frame type as vl_frame_type_t */
    int average_qp_value; /* average qp value of the encoded frame */
    int intra_blocks;  /* intra blockes (in 8x8) of the encoded frame */
    int merged_blocks; /* merged blockes (in 8x8) of the encoded frame */
    int skipped_blocks; /* skipped blockes (in 8x8) of the encoded frame */
} amvenc_frame_extra_info_t;

/* encoded frame info */
typedef struct amvenc_metadata_e {
    int encoded_data_length_in_bytes; /* size of the encoded buffer */
    bool is_key_frame; /* if true, the encoded frame is a keyframe */
    int timestamp_us;  /* timestamp in usec of the encode frame */
    bool is_valid;     /* if true, the encoding was successful */
    amvenc_frame_extra_info_t extra; /* extra info of encoded frame if is_valid true */
    int err_cod; /* error code if is_valid is false: >0 normal, others error */
} amvenc_metadata_t;

typedef enum amvenc_codec_id_e {
    AML_CODEC_ID_NONE,
    AML_CODEC_ID_VP8,
    AML_CODEC_ID_H261,
    AML_CODEC_ID_H263,
    AML_CODEC_ID_H264, /* must support */
    AML_CODEC_ID_H265,
} amvenc_codec_id_t;

typedef enum amvenc_img_format_e {
    AML_IMG_FMT_NONE,
    AML_IMG_FMT_NV12, /* must support  */
    AML_IMG_FMT_NV21,
    AML_IMG_FMT_YUV420P,
    AML_IMG_FMT_YV12,
    AML_IMG_FMT_RGB888,
    AML_IMG_FMT_RGBA8888,
} amvenc_img_format_t;

typedef enum amvenc_frame_type_e {
    AML_FRAME_TYPE_NONE,
    AML_FRAME_TYPE_AUTO, /* encoder self-adaptation(default) */
    AML_FRAME_TYPE_IDR,
    AML_FRAME_TYPE_I,
    AML_FRAME_TYPE_P,
    AML_FRAME_TYPE_B,
    AML_FRAME_TYPE_DROPPABLE_P,
} amvenc_frame_type_t;

/* buffer type*/
typedef enum {
    AML_VMALLOC_TYPE = 0,
    AML_CANVAS_TYPE = 1,
    AML_PHYSICAL_TYPE = 2,
    AML_DMA_TYPE = 3,
} amvenc_buffer_type_t;
/* encoder info config */

typedef struct crop_info {
    int left;
    int top;
    int right;
    int bottom;
}crop_info_t;

typedef struct amvenc_info {
    int width;
    int height;
    int frame_rate;
    int bit_rate;
    int gop;
    bool prepend_spspps_to_idr_frames;
    amvenc_img_format_t img_format;
    int qp_mode; /* 1: use customer QP range, 0:use default QP range */
    int forcePicQpEnable;
    int forcePicQpI;
    int forcePicQpP;
    int forcePicQpB;
    int enc_feature_opts; /* option features flag settings.*/
                        /* See above for fields definition in detail */
                       /* bit 0: qp hint(roi) 0:disable (default) 1: enable */
                       /* bit 1: param_change 0:disable (default) 1: enable */
                       /* bit 2 to 6: gop_opt:0 (default), 1:I only 2:IP, */
                       /*                     3: IBBBP, 4: IP_SVC1, 5:IP_SVC2 */
                       /*                     6: IP_SVC3, 7: IP_SVC4,  8:CustP*/
                       /*                     see define of AMVGOPModeOPT */
                       /* bit 7:LTR control   0:disable (default) 1: enable*/
    int intra_refresh_mode; /* refresh mode select */
                       /* 0: no refresh, 1:row 2:column, 3: step size */
                       /* 4 (for HEVC only): adaptive */
    int intra_refresh_arg; /* number of MB(CTU) rows, columns, MB(CTU)s */
    int profile; /* encoding profile: 0 auto (H.264 high, H.265 main profile) */
               /* H.264 1: baseline 2: Main, 3 High profile*/
    /*frame rotation angle before encoding, counter clock-wise
    0: no rotation
    90: rotate 90 degree
    180: rotate 180 degree
    270: rotate 270 degree*/
    int level;
    uint32_t frame_rotation;
    /*frame mirroring before encoding
    0: no mirroring
    1: vertical mirroring
    2: horizontal mirroring
    3: both v and h mirroring*/
    uint32_t frame_mirroring;
    int bitstream_buf_sz; /* the encoded bitstream buffer size in MB (range 1-32)*/
                        /* 0: use the default value 2MB */
    int multi_slice_mode; /* init multi-slice control */
                        /*  0: only one slice per frame (default)
                            1. multiple slice by MB(H.264)/CTU (H.265)
                            2: by encoded size(dependant Slice) and
                              CTU (independent Slice) combination (H.265 only)*/
    int multi_slice_arg;  /*  when multi_slice_mode ==1:
                                numbers of MB(16x16 blocks)/ CTU (64x64 blocks)
                           when: multi_slice_mode ==2
                           bit 0-15: CTUs per independent Slices
                           bit 16-31: size of dependent slices in bytes */
    int cust_gop_qp_delta;   /* an qp delta for P  frames
                            apply to cust GOP mode (>=IP_SVC1)           */
    int strict_rc_window;     /* strict bitrate control window (frames count)
                             0 disbled max 120, larger value will be clipped*/
    int strict_rc_skip_thresh;/* threshold of actual bitrate in compare with target
                              bitrate to trigger skip frame (in percentage)  */
    int bitstream_buf_sz_kb; /* the encoded bitstream buffer size in KB */
    uint8_t vui_parameters_present_flag;
    uint8_t video_full_range_flag;
    uint8_t video_signal_type_present_flag;
    uint8_t colour_description_present_flag;
    uint8_t colour_primaries;
    uint8_t transfer_characteristics;
    uint8_t matrix_coefficients;
    bool crop_enable;
    crop_info_t crop;
} amvenc_info_t;

/* dma buffer info*/
/*for nv12/nv21, num_planes can be 1 or 2
  for yv12, num_planes can be 1 or 3
 */
typedef struct amvenc_dma_info {
    int shared_fd[3];
    unsigned int num_planes;//for nv12/nv21, num_planes can be 1 or 2
} amvenc_dma_info_t;

/*When the memory type is V4L2_MEMORY_DMABUF, dma_info.shared_fd is a
  file descriptor associated with a DMABUF buffer.
  When the memory type is V4L2_MEMORY_USERPTR, in_ptr is a userspace
  pointer to the memory allocated by an application.
*/

typedef union {
    amvenc_dma_info_t dma_info;
    unsigned long in_ptr[3];
    uint32_t canvas;
} amvenc_buf_info_u;

/* input buffer info
 * buf_type = VMALLOC_TYPE correspond to  buf_info.in_ptr
   buf_type = DMA_TYPE correspond to  buf_info.dma_info
 */
typedef struct amvenc_buffer_info {
    amvenc_buffer_type_t buf_type;
    amvenc_buf_info_u buf_info;
    int buf_stride; //buf stride for Y, if 0,use width as stride (default)
    amvenc_img_format_t buf_fmt;
} amvenc_buffer_info_t;

typedef struct amvenc_qp_param_s {
    int qp_min;
    int qp_max;
    int qp_I_base;
    int qp_P_base;
    int qp_B_base;
    int qp_I_min;
    int qp_I_max;
    int qp_P_min;
    int qp_P_max;
    int qp_B_min;
    int qp_B_max;
} amvenc_qp_param_t;

typedef struct amvenc_sei_payload
{
    int payload_size;
    int payload_type;
    unsigned char *payload;
} amvenc_sei_payload_t;

typedef struct amvenc_frame_info
{
    unsigned long frame_size;
    int pitch;
    int height;
    unsigned int coding_timestamp;
    unsigned int scale_width;
    unsigned int scale_height;
    unsigned int crop_left;
    unsigned int crop_right;
    unsigned int crop_top;
    unsigned int crop_bottom;
    unsigned int bitrate;
    unsigned int frame_rate;
    amvenc_sei_payload_t sei_payload;
} amvenc_frame_info_t;

/**
 * init encoder
 *
 *@param : codec_id: codec type
 *@param : vl_encode_info_t: encode info
 *         width:      video width
 *         height:     video height
 *         frame_rate: framerate
 *         bit_rate:   bitrate
 *         gop GOP:    max I frame interval
 *         prepend_spspps_to_idr_frames: if true, adds spspps header
 *         to all idr frames (keyframes).
 *         buf_type:
 *         0: need memcpy from input buf to encoder internal dma buffer.
 *         3: input buf is dma buffer, encoder use input buf without memcopy.
 *img_format: image format
 *@return : if success return encoder handle,else return <= 0
 */
amvenc_handle_t amvenc_init(amvenc_codec_id_t codec_id,
                                    amvenc_info_t encode_info,
                                    amvenc_qp_param_t* qp_tbl);


/**
 * generate header
 *
 *@param : codec_handle: handle
 *@param : vl_encode_info_t: encode info
 *         width:      video width
 *         height:     video height
 *         frame_rate: framerate
 *         bit_rate:   bitrate
 *         gop GOP:    max I frame interval
 *         prepend_spspps_to_idr_frames: if true, adds spspps header
 *         to all idr frames (keyframes).
 *         buf_type:
 *         0: need memcpy from input buf to encoder internal dma buffer.
 *         3: input buf is dma buffer, encoder use input buf without memcopy.
 *img_format: image format
 *@return : if success return encoder handle,else return <= 0
 */
amvenc_metadata_t amvenc_generate_header(amvenc_handle_t handle,
                                                     unsigned char *pHeader,
                                                     unsigned int *pLength);


/**
 * encode video
 *
 *@param : handle
 *@param : type: frame type
 *@param : in: data to be encoded
 *@param : in_size: data size
 *@param : out: data output,need header(0x00,0x00,0x00,0x01),and format
 *         must be I420(apk set param out, through jni,so modify "out" in the
 *         function,don't change address point)
 *@param : in_buffer_info
 *         buf_type:
 *              VMALLOC_TYPE: need memcpy from input buf to encoder internal dma buffer.
 *              DMA_TYPE: input buf is dma buffer, encoder use input buf without memcopy.
 *         buf_info.dma_info: input buf dma info.
 *              num_planes:For nv12/nv21, num_planes can be 1 or 2.
 *                         For YV12, num_planes can be 1 or 3.
 *              shared_fd: DMA buffer fd.
 *         buf_info.in_ptr: input buf ptr.
 *@param : ret_buffer_info
 *         buf_type:
 *              VMALLOC_TYPE: need memcpy from input buf to encoder internal dma buffer.
 *              DMA_TYPE: input buf is dma buffer, encoder use input buf without memcopy.
 *              due to references and reordering, the DMA buffer may not return immedialtly.
 *         buf_info.dma_info: returned buf dma info if any.
 *              num_planes:For nv12/nv21, num_planes can be 1 or 2.
 *                         For YV12, num_planes can be 1 or 3.
 *              shared_fd: DMA buffer fd.
 *         buf_info.in_ptr: returned input buf ptr.
 *@return: if success return encoded data length,else return <= 0
 */
amvenc_metadata_t amvenc_encode(amvenc_handle_t handle,
                                           amvenc_frame_info_t frame_info,
                                           amvenc_frame_type_t type,
                                           unsigned char* out,
                                           amvenc_buffer_info_t *in_buffer_info,
                                           amvenc_buffer_info_t *ret_buffer_info);

/**
 *
 * vl_video_encoder_change_bitrate
 * Change the taget bitrate in encoding
 *@param : handle
 *@param : bitRate: the new target encode bitrate
 *@return : if success return 0 ,else return <= 0
 */
int amvenc_change_bitrate(amvenc_handle_t handle, int bitRate);

/**
 * destroy encoder
 *
 *@param  :handle: encoder handle
 *@return :if success return 1,else return 0
 */
int amvenc_destroy(amvenc_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif /* __AMVENC_H__ */
