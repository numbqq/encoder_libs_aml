/*
* Copyright (c) 2019 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in below
* which is part of this source code package.
*
* Description:
*/

// Copyright (C) 2019 Amlogic, Inc. All rights reserved.
//
// All information contained herein is Amlogic confidential.
//
// This software is provided to you pursuant to Software License
// Agreement (SLA) with Amlogic Inc ("Amlogic"). This software may be
// used only in accordance with the terms of this agreement.
//
// Redistribution and use in source and binary forms, with or without
// modification is strictly prohibited without prior written permission
// from Amlogic.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#ifndef _VP_CODEC_TAHOE
#define _VP_CODEC_TAHOE

#include <stdint.h>
#include <stdbool.h>

#define vl_codec_tahoe_handle_t long

typedef enum vt_img_format_e {
    IMG_FMT_NONE,
    IMG_FMT_NV12, /* must support  */
    IMG_FMT_NV21,
    IMG_FMT_YUV420P,
    IMG_FMT_YV12,
    IMG_FMT_RGB888,
    IMG_FMT_RGBA8888,
} vt_img_format_t;

typedef enum vl_frame_type_e {
  FRAME_TYPE_NONE,
  FRAME_TYPE_AUTO, /* encoder self-adaptation(default) */
  FRAME_TYPE_IDR,
  FRAME_TYPE_I,
  FRAME_TYPE_P,
  FRAME_TYPE_B,
  FRAME_TYPE_DROPPABLE_P,
} vt_frame_type_t;

typedef enum vt_codec_id_e {
    CODEC_ID_H264 = 0,
    CODEC_ID_H265 = 1,
} vt_codec_id_t;

typedef enum {
  VMALLOC_TYPE = 0,
  CANVAS_TYPE = 1,
  PHYSICAL_TYPE = 2,
  DMA_TYPE = 3,
} vt_buffer_type_t;



typedef struct crop_info_tahoe {
    int left;
    int top;
    int right;
    int bottom;
}crop_info_tahoe_t;


typedef struct vt_encode_info {
  int width;
  int height;
  int frame_rate;
  int bit_rate;
  int gop;
  bool prepend_spspps_to_idr_frames;
  vt_img_format_t img_format;
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
  int level;
  /*frame rotation angle before encoding, counter clock-wise
	0: no rotation
	90: rotate 90 degree
	180: rotate 180 degree
	270: rotate 270 degree*/
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
                             0 disabled max 120, larger value will be clipped*/
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
  crop_info_tahoe_t crop;
} vt_encode_info_t;

typedef struct vl_dma_info {
    int shared_fd[3];
    unsigned int num_planes;//for nv12/nv21, num_planes can be 1 or 2
} vt_dma_info_t;


typedef union {
  vt_dma_info_t dma_info;
  unsigned long in_ptr[3];
  uint32_t canvas;
} vt_buf_info_u;


typedef struct vl_buffer_info {
  vt_buffer_type_t buf_type;
  vt_buf_info_u buf_info;
  int buf_stride; //buf stride for Y, if 0,use width as stride (default)
  vt_img_format_t buf_fmt;
} vt_buffer_info_t;

 typedef struct vc_enc_frame_extra_info {
  int frame_type; /* encoded frame type as vl_frame_type_t */
  int average_qp_value; /* average qp value of the encoded frame */
  int intra_blocks;  /* intra blockes (in 8x8) of the encoded frame */
  int merged_blocks; /* merged blockes (in 8x8) of the encoded frame */
  int skipped_blocks; /* skipped blockes (in 8x8) of the encoded frame */
} vt_enc_frame_extra_info_t;

/* encoded frame info */
typedef struct vc_encoding_metadata_e {
  int encoded_data_length_in_bytes; /* size of the encoded buffer */
  bool is_key_frame; /* if true, the encoded frame is a keyframe */
  int timestamp_us;  /* timestamp in usec of the encode frame */
  bool is_valid;     /* if true, the encoding was successful */
  vt_enc_frame_extra_info_t extra; /* extra info of encoded frame if is_valid true */
  int err_cod; /* error code if is_valid is false: >0 normal, others error */
} vt_encoding_metadata_t;



vl_codec_tahoe_handle_t vt_tahoe_encoder_init(int codec_id, vt_encode_info_t encode_info);
void vt_tahoe_encoder_destroy(vl_codec_tahoe_handle_t codec_handle);
int vt_video_encoder_change_bitrate(vl_codec_tahoe_handle_t codec_handle, int bitRate);
int vt_video_encoder_change_gop(vl_codec_tahoe_handle_t codec_handle, int IntraQP, int IntraPeriod);
int vt_video_encoder_change_strict_rc(vl_codec_tahoe_handle_t codec_handle, int bitrate_window, int skip_threshold);
vt_encoding_metadata_t vt_tahoe_encoder_encode(vl_codec_tahoe_handle_t codec_handle,vt_frame_type_t type,vt_buffer_info_t *in_buffer_info,unsigned char* out);
int vt_tahoe_encoder_generate_header(vl_codec_tahoe_handle_t codec_handle, unsigned char *pHeader, unsigned int *pLength);

#endif
