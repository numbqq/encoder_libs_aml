#ifndef _INCLUDED_COM_VIDEOPHONE_CODEC
#define _INCLUDED_COM_VIDEOPHONE_CODEC

#ifdef __cplusplus
extern "C" {
#endif

#define vl_codec_handle_hevc_t long

typedef struct enc_frame_extra_hevc_info {
  int frame_type; /* encoded frame type as vl_frame_type_t */
  int average_qp_value; /* average qp value of the encoded frame */
  int intra_blocks;  /* intra blockes (in 8x8) of the encoded frame */
  int merged_blocks; /* merged blockes (in 8x8) of the encoded frame */
  int skipped_blocks; /* skipped blockes (in 8x8) of the encoded frame */
} enc_frame_extra_info_hevc_t;

/* encoded frame info */
typedef struct encoding_metadata_hevc_e {
  int encoded_data_length_in_bytes; /* size of the encoded buffer */
  bool is_key_frame; /* if true, the encoded frame is a keyframe */
  int timestamp_us;  /* timestamp in usec of the encode frame */
  bool is_valid;     /* if true, the encoding was successful */
  enc_frame_extra_info_hevc_t extra; /* extra info of encoded frame if is_valid true */
  int err_cod; /* error code if is_valid is false: >0 normal, others error */
} encoding_metadata_hevc_t;

typedef enum vl_codec_id_hevc_e {
  CODEC_ID_NONE,
  CODEC_ID_VP8,
  CODEC_ID_H261,
  CODEC_ID_H263,
  CODEC_ID_H264, /* must support */
  CODEC_ID_H265,
} vl_codec_id_hevc_t;

typedef enum vl_img_format_hevc_e {
  IMG_FMT_NONE,
  IMG_FMT_NV12, /* must support  */
  IMG_FMT_NV21,
  IMG_FMT_YUV420P,
  IMG_FMT_YV12,
  IMG_FMT_RGB888,
  IMG_FMT_RGBA8888,
} vl_img_format_hevc_t;

typedef enum vl_frame_type_hevc_e {
  FRAME_TYPE_NONE,
  FRAME_TYPE_AUTO, /* encoder self-adaptation(default) */
  FRAME_TYPE_IDR,
  FRAME_TYPE_I,
  FRAME_TYPE_P,
} vl_frame_type_hevc_t;

typedef enum vl_error_type_hevc_e {
  ERR_HARDWARE = -4,
  ERR_OVERFLOW = -3,
  ERR_NOTSUPPORT = -2,
  ERR_UNDEFINED = -1,
} vl_error_type_hevc_e;

/* buffer type*/
typedef enum {
  VMALLOC_TYPE = 0,
  CANVAS_TYPE = 1,
  PHYSICAL_TYPE = 2,
  DMA_TYPE = 3,
} vl_buffer_type_hevc_t;

typedef enum
{
    NONE_HEVC = 0,
    MAIN_HEVC = 1,
    MAINSTILLPICTURE_HEVC = 3,
} vl_profile_t;

//HEVCTier
typedef enum {
    HEVC_TIER_MAIN = 0,
    HEVC_TIER_HIGH = 1
} vl_tier_t;


typedef enum {
    LEVEL_HEVC_NONE = 0,
    LEVEL1_HEVC = 30,
    LEVEL2_HEVC = 60,
    LEVEL2_1_HEVC = 63,
    LEVEL3_HEVC = 90,
    LEVEL3_1_HEVC = 93,
    LEVEL4_HEVC = 120,
    LEVEL4_1_HEVC = 123,
    LEVEL5_HEVC = 150,
    LEVEL5_1_HEVC = 153,
    LEVEL5_2_HEVC = 156,
    LEVEL6_HEVC = 180,
    LEVEL6_1_HEVC = 183,
    LEVEL6_2_HEVC = 186,
    LEVEL8_5_HEVC = 255
} vl_level_t;



/* encoder features configure flags bit masks for enc_feature_opts */
/* Enable RIO feature.
        bit field value 1: enable, 0: disable (default) */
#define ENABLE_ROI_FEATURE      0x1
/* Encode parameter update on the fly.
        bit field value: 1: enable, 0: disable (default) */
#define ENABLE_PARA_UPDATE      0x2
/* Encode long term references feature.
        bit field value: 1 enable, 0: disable (default)*/
#define ENABLE_LONG_TERM_REF    0x80

typedef struct crop_info {
    int left;
    int top;
    int right;
    int bottom;
}crop_info_t;

/* encoder info*/
typedef struct vl_encode_info_hevc {
  int width;
  int height;
  int frame_rate;
  int bit_rate;
  int gop;
  bool prepend_spspps_to_idr_frames;
  vl_img_format_hevc_t img_format;
  int qp_mode; /* 1: use customer QP range, 0:use default QP range */
  int bitstream_buf_sz; /* the encoded bitstream buffer size in MB (range 1-32)*/
                        /* 0: use the default value 2MB */
  int bitstream_buf_sz_kb; /* the encoded bitstream buffer size in KB */
  int enc_feature_opts; /* option features flag settings.*/
                        /* See above for fields definition in detail */
                       /* bit 0: qp hint(roi) 0:disable (default) 1: enable */
                       /* bit 1: param_change 0:disable (default) 1: enable */
                       /* bit 2 to 6: gop_opt:0 (default), 1:I only 2:IP, */
                       /*                     3: IBBBP, 4: IP_SVC1, 5:IP_SVC2 */
                       /*                     6: IP_SVC3, 7: IP_SVC4,  8:CustP*/
                       /*                     see define of AMVGOPModeOPT */
                       /* bit 7:LTR control   0:disable (default) 1: enable*/

  bool vui_info_present;
  bool video_signal_type; /*video_signal_type_present_flag*/
  bool color_description; /*color_description_present_flag*/
  int primaries; /*color primaries*/
  int transfer; /*color transfer charicstics*/
  int matrix; /* color space matrix coefficients*/
  bool range; /*color range flag, 0:full, 1:limitedd*/
  bool crop_enable;
  crop_info_t crop;
  vl_profile_t profile;
  vl_tier_t tier;
  vl_level_t level;
} vl_encode_info_hevc_t;

/* dma buffer info*/
typedef struct vl_dma_info_hevc {
  int shared_fd[3];
  unsigned int num_planes;//for nv12/nv21, num_planes can be 1 or 2
} vl_dma_info_hevc_t;

typedef union {
  vl_dma_info_hevc_t dma_info;
  unsigned long in_ptr[3];
} vl_buf_info_hevc_u;

/* input buffer info
 * buf_type = VMALLOC_TYPE correspond to  buf_info.in_ptr
   buf_type = DMA_TYPE correspond to  buf_info.dma_info
 */
typedef struct vl_buffer_info_hevc {
  vl_buffer_type_hevc_t buf_type;
  vl_buf_info_hevc_u buf_info;
    int buf_stride; //buf stride for Y, if 0,use width as stride (default)
    vl_img_format_hevc_t buf_fmt;
} vl_buffer_info_hevc_t;

typedef struct qp_param_hevc_s {
  int qp_min;
  int qp_max;
  int qp_base;
} qp_param_hevc_t;

/**
 * Getting version information
 *
 *@return : version information
 */
const char *vl_get_version_hevc();

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
 *                                       to all idr frames (keyframes).
 *         img_format: image format
 *@return : if success return encoder handle,else return <= 0
 */
vl_codec_handle_hevc_t vl_video_encoder_init_hevc(vl_codec_id_hevc_t codec_id,
                                       vl_encode_info_hevc_t encode_info,
                                        qp_param_hevc_t* qp_tbl);
encoding_metadata_hevc_t vl_video_encoder_generate_header(vl_codec_handle_hevc_t handle,
                                                                   unsigned char *pHeader,
                                                                   unsigned int *pLength);

/**
 * encode video
 *
 *@param : handle
 *@param : type: frame type
 *@param : in: data to be encoded
 *@param : in_size: data size
 *@param : out: data output,HEVC need header(0x00，0x00，0x00，0x01),and format
 *must be I420(apk set param out，through jni,so modify "out" in the
 *function,don't change address point)
 *@param : buffer_info
 *         buf_type:
 *              VMALLOC_TYPE: need memcpy from input buf to encoder internal dma buffer.
 *              DMA_TYPE: input buf is dma buffer, encoder use input buf without memcopy.
 *         buf_info.dma_info: input buf dma info.
 *              num_planes:For nv12/nv21, num_planes can be 1 or 2.
                           For YV12, num_planes can be 1 or 3.
                shared_fd: DMA buffer fd.
 *         buf_info.in_ptr: input buf ptr.
 *@return ：if success return encoded data length,else return error
 */
encoding_metadata_hevc_t vl_video_encoder_encode_hevc(vl_codec_handle_hevc_t handle,
                           vl_frame_type_hevc_t frame_type,
                           unsigned char *out,
                           vl_buffer_info_hevc_t *in_buffer_info);
/**
 *
 * vl_video_encoder_change_bitrate_hevc
 * Change the taget bitrate in encoding
 *@param : handle
 *@param : bitRate: the new target encode bitrate
 *@return : if success return 0 ,else return <= 0
 */
 int vl_video_encoder_change_bitrate_hevc(vl_codec_handle_hevc_t codec_handle,
                            int bitRate);

int vl_video_encoder_change_framerate_hevc(vl_codec_handle_hevc_t codec_handle,
                            int frameRate,int bitRate);
int vl_video_encoder_getavgqp(vl_codec_handle_hevc_t handle, int *avg_qp);


/**
 * destroy encoder
 *
 *@param ：handle: encoder handle
 *@return ：if success return 1,else return 0
 */
int vl_video_encoder_destroy_hevc(vl_codec_handle_hevc_t handle);

#ifdef __cplusplus
}
#endif

#endif /* _INCLUDED_COM_VIDEOPHONE_CODEC */
