/*------------------------------------------------------------------------------
--                                                                                                                               --
--       This software is confidential and proprietary and may be used                                   --
--        only as expressly authorized by a licensing agreement from                                     --
--                                                                                                                               --
--                            Verisilicon.                                                                                    --
--                                                                                                                               --
--                   (C) COPYRIGHT 2014 VERISILICON                                                            --
--                            ALL RIGHTS RESERVED                                                                    --
--                                                                                                                               --
--                 The entire notice above must be reproduced                                                 --
--                  on all copies and should not be removed.                                                    --
--                                                                                                                               --
--------------------------------------------------------------------------------*/

#ifndef SW_PARAMETER_SET_H
#define SW_PARAMETER_SET_H

#include "base_type.h"
#include "container.h"
#include "vsi_queue.h"
#include "sw_put_bits.h"
#include "sw_nal_unit.h"
#include "hevcencapi.h"

#ifdef SUPPORT_AV1
#include "av1_sw_parameter_set.h"
#endif

#ifdef SUPPORT_VP9
#include "vp9_sw_parameter_set.h"
#endif

#define INVALID_LEVEL 0xFFFF

struct ps {
    struct node *next;
    struct queue memory;
    struct nal_unit nal_unit;
    struct buffer b;
    i32 id;
};

struct vps {
    struct ps ps;
    i32 max_num_sub_layers;
    i32 temporal_id_nesting_flag;
    i32 slo_info_present_flag;
    i32 max_dec_pic_buffering[8];
    i32 max_num_reorder_pics[8];
    i32 max_latency_increase[8];
    i32 streamMode; /* 0: byte_stream, 1: nal type */
    u32 general_level_idc;
    u32 general_tier_flag;
    u32 general_profile_idc;
};

struct ref_pic {
    i32 delta_poc;
    i32 used_by_curr_pic;
    i32 long_term_flag;
};

struct rps {
    struct ps ps;
    i32 sps_id;
    i32 idx;
    i32 num_negative_pics;
    i32 num_positive_pics;
    i32 num_lt_pics;
    struct ref_pic *ref_pic_s0;
    struct ref_pic *ref_pic_s1;
    struct ref_pic *ref_pic_lt;
    i32 long_term_ref_pic_poc[VCENC_MAX_LT_REF_FRAMES];

    i32 *before;
    i32 *after;
    i32 *follow;
    i32 *lt_current;
    i32 *lt_follow;
    i32 before_cnt;
    i32 after_cnt;
    i32 follow_cnt;
    i32 lt_current_cnt;
    i32 lt_follow_cnt;
};
typedef struct {
    u32 timeScale;
    u32 numUnitsInTick;
    u32 bitStreamRestrictionFlag;
    u32 vuiVideoFullRange;
    u32 sarWidth;
    u32 sarHeight;
    u32 nalHrdParametersPresentFlag;
    u32 vclHrdParametersPresentFlag;
    u32 field_seq_flag;
    u32 frame_field_info_present_flag;

    /** hrd_parameter */
    u32 initialCpbRemovalDelayLength;
    u32 cpbRemovalDelayLength;
    u32 dpbOutputDelayLength;
    u32 timeOffsetLength;
    u32 bitRate;
    u32 cpbSize;
    u32 cbr_flag;

    u32 vuiVideoSignalTypePresentFlag;
    u32 vuiVideoFormat;
    u32 vuiColorDescripPresentFlag;
    u32 vuiColorPrimaries;
    u32 vuiTransferCharacteristics;
    u32 vuiMatrixCoefficients;
} vui_t;

struct sps {
    struct ps ps;
    i32 vps_id;
    i32 max_num_sub_layers;
    i32 temporal_id_nesting_flag;
    i32 slo_info_present_flag;
    i32 chroma_format_idc;
    i32 pcm_enabled_flag;
    i32 pcm_sample_bit_depth_luma_minus1;
    i32 pcm_sample_bit_depth_chroma_minus1;
    i32 log2_min_pcm_luma_cb_size_minus3;
    i32 log2_diff_max_min_pcm_luma_cb_size;
    i32 pcm_loop_filter_disabled_flag;
    i32 log2_max_pic_order_cnt_lsb;
    i32 max_dec_pic_buffering[8];
    i32 max_num_reorder_pics[8];
    i32 max_latency_increase[8];
    i32 log2_min_cb_size;
    i32 min_cb_size;
    i32 log2_diff_cb_size;
    i32 log2_min_tr_size;
    i32 log2_diff_tr_size;
    i32 amp_enabled_flag;
    i32 sao_enabled_flag;
    i32 sps_temporal_id_nesting_flag;
    i32 max_tr_hierarchy_depth_inter;
    i32 max_tr_hierarchy_depth_intra;
    i32 scaling_list_enable_flag;
    i32 scaling_list_data_present_flag;
    i32 num_short_term_ref_pic_sets;
    i32 long_term_ref_pics_present_flag;
    i32 long_term_ref_pics_sps;
    i32 temporal_mvp_enable_flag;
    i32 strong_intra_smoothing_enabled_flag;
    i32 width;          /* Width of original imaga */
    i32 height;         /* Height of original imaga */
    i32 width_min_cbs;  /* Width rounded to next n*min_cb_size */
    i32 height_min_cbs; /* Height rounded to next n*min_cb_size */

    true_e vui_parameters_present_flag;
    vui_t vui;

    i32 frameCropping;
    u32 frameCropLeftOffset;
    u32 frameCropRightOffset;
    u32 frameCropTopOffset;
    u32 frameCropBottomOffset;
    i32 streamMode; /* 0: byte_stream, 1: nal type */

    u32 general_level_idc;

    u32 general_tier_flag;

    u32 general_profile_idc;

    u32 bit_depth_luma_minus8;
    u32 bit_depth_chroma_minus8;

    /* H264 definitions*/
    true_e constraintSet0;
    true_e constraintSet1;
    true_e constraintSet2;
    true_e constraintSet3;
    i32 log2MaxFrameNumMinus4;
    i32 log2MaxpicOrderCntLsbMinus4;
    u32 picOrderCntType;
    true_e gapsInFrameNumValueAllowed;
    i32 picWidthInMbsMinus1;
    i32 picHeightInMapUnitsMinus1;
    true_e frameMbsOnly;
    true_e direct8x8Inference;
    u32 numRefFrames;

#ifdef SUPPORT_AV1
    /* AV1 definitions*/
    AV1_SPS_Parameters av1SpsParam;
#endif

#ifdef SUPPORT_VP9
    /* VP9 definitions*/
    VP9_SPS_Parameters vp9SpsParam;
#endif
};

struct pps {
    struct ps ps;
    i32 sps_id;
    i32 sign_data_hiding_flag;
    i32 entropy_coding_mode_flag;
    i32 cabac_init_present_flag;
    i32 num_ref_idx_l0_default_active;
    i32 num_ref_idx_l1_default_active;
    i32 init_qp;
    i32 constrained_intra_pred_flag;
    i32 transform_skip_enabled_flag;
    i32 cu_qp_delta_enabled_flag;
    i32 diff_cu_qp_delta_depth;
    i32 cb_qp_offset;
    i32 cr_qp_offset;
    i32 slice_chroma_qp_offsets_present_flag;
    i32 weighted_pred_flag;
    i32 weighted_bipred_flag;
    i32 output_flag_present_flag;
    i32 transquant_bypass_enable_flag;
    i32 dependent_slice_enabled_flag;
    i32 entropy_coding_sync_enabled_flag;
    i32 loop_filter_across_slices_enabled_flag;
    i32 deblocking_filter_control_present_flag;
    i32 deblocking_filter_override_enabled_flag;
    i32 beta_offset;
    i32 tc_offset;
    i32 deblocking_filter_disabled_flag;
    i32 scaling_list_data_present_flag;
    i32 lists_modification_present_flag;
    i32 log2_parallel_merge_level;

    /* Tiles, scanning and scaling */
    i32 tiles_enabled_flag;
    i32 loop_filter_across_tiles_enabled_flag;
    i32 num_tile_columns;
    i32 num_tile_rows;
    i32 uniform_spacing_flag;
    i32 *col_width;
    i32 *row_height;
    i32 *addr_rs_to_ts;
    i32 *addr_ts_to_rs;
    i32 *tile_id;
    i32 **min_tb_addr_zs;
    u8 *scaling_factor[6][6];  /* [log2_size][matrix_id] */
    i32 *scaling[6][6][6];     /* [log2_size][matrix_id][6 (index = qp%6)] */
    i32 *inv_scaling[6][6][6]; /* [log2_size][matrix_id][6 (index = qp%6)] */

    /* Picture/ctb/etc... sizes derived from sps */
    i32 ctb_per_picture;
    i32 ctb_per_column;
    i32 ctb_per_row;
    i32 log2_ctb_size;
    i32 ctb_size;
    i32 mcb_per_column; /* TODO needed ? */
    i32 mcb_per_row;
    i32 log2_max_tr_size;
    i32 log2_qp_size;
    i32 qp_size;
    i32 qp_per_row;
    i32 qp_per_column;

    /* Slice header uses these */
    i32 no_output_of_prior_pics_flag;
    i32 short_term_ref_pic_set_sps_flag;

    i32 qp;
    i32 qpMin;
    i32 qpMax;

    i32 streamMode; /* 0: byte_stream, 1: nal type */

    true_e transform8x8Mode; /*H264 8x8 transform, 0:1 disable/enable 8x8 transform*/
};

void *create_parameter_set(i32 type);
struct ps *get_parameter_set(struct container *, enum nal_type type, i32 id);
void free_parameter_sets(struct container *c);
void free_parameter_set(struct ps *ps);
void free_parameter_set_queue(struct container *c);
void remove_parameter_set(struct container *c, enum nal_type type, i32 id);

void video_parameter_set(struct vps *v, VCEncInst inst);
void sequence_parameter_set(struct container *c, struct sps *s, VCEncInst inst);
void cnt_ref_pic_set(struct container *c, struct sps *s);
void picture_parameter_set(struct pps *p, VCEncInst inst);
i32 init_parameter_set(struct sps *s, struct pps *p);
i32 set_reference_pic_set(struct rps *r);
i32 get_reference_pic_set(struct rps *r);
void init_poc_list(struct rps *r, i32 poc, bool is_ltr_cur, u32 codingType, struct container *c,
                   bool bH264, const u32 *pLTR_used, u8 *rpsModify);
i32 ref_pic_marking(struct container *c, struct rps *r, int saveExtPoc);

i32 scaling_lists(struct sps *s, struct pps *p);
i32 tile_init(struct pps *p, i32 demo_id, i32 c, i32 r);

i32 rt_scan(struct pps *p);
i32 z_scan(struct pps *p, i32 log2_min_size);

void HEVCEndOfSequence(struct buffer *b, u32 byte_stream);

void VCEncSpsSetVuiVideoInfo(struct sps *sps, u32 videoFullRange);
void VCEncSpsSetVuiAspectRatio(struct sps *sps, u32 sampleAspectRatioWidth,
                               u32 sampleAspectRatioHeight);
void VCEncSpsSetVuiTimigInfo(struct sps *sps, u32 timeScale, u32 numUnitsInTick);

void VCEncSpsSetVuiHrd(struct sps *sps, u32 present);
u32 VCEncSpsGetVuiHrdBitRate(struct sps *sps);
u32 VCEncSpsGetVuiHrdCpbSize(struct sps *sps);
void VCEncSpsSetVuiPictStructPresentFlag(struct sps *sps, u32 flag);
void VCEncSpsSetVui_field_seq_flag(struct sps *sps, u32 flag);
void VCEncSpsSetVuiHrdBitRate(struct sps *sps, u32 bitRate);
void VCEncSpsSetVuiHrdCpbSize(struct sps *sps, u32 cpbSize);
void VCEncSpsSetVuiHrdCbrFlag(struct sps *sps, u32 cbr_flag);

void VCEncSpsSetVuiSignalType(struct sps *sps, u32 VideoSignalTypePresentFlag, u32 video_format,
                              u32 videoFullRange, u32 ColorDescripPresentFlag, u32 ColorPrimaries,
                              u32 TransferCharacteristics, u32 MatrixCoefficients);
void HEVCAccessUnitDelimiter(struct buffer *b, u32 byte_stream, u32 pic_type);

/*-----------------------------------
                 H264 Defination
-------------------------------------*/
u32 H264GetLevelIndex(u32 levelIdc);
void H264EndOfSequence(struct buffer *b, u32 byte_stream);
void H264AccessUnitDelimiter(struct buffer *b, u32 byte_stream, u32 primary_pic_type);

#endif
