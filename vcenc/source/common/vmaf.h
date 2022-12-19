/*------------------------------------------------------------------------------
--                                                                                                                               --
--       This software is confidential and proprietary and may be used                                   --
--        only as expressly authorized by a licensing agreement from                                     --
--                                                                                                                               --
--                            Verisilicon.                                                                                    --
--                                                                                                                               --
--                   (C) COPYRIGHT 2020 VERISILICON                                                            --
--                            ALL RIGHTS RESERVED                                                                    --
--                                                                                                                               --
--                 The entire notice above must be reproduced                                                 --
--                  on all copies and should not be removed.                                                    --
--                                                                                                                               --
--------------------------------------------------------------------------------*/

#ifndef VMAF_H
#define VMAF_H

#define SUBPEL_BITS 4
#define SUBPEL_MASK ((1 << SUBPEL_BITS) - 1)
#define SUBPEL_SHIFTS (1 << SUBPEL_BITS)
#define SUBPEL_TAPS 8
#define FILTER_BITS 7
#define ROUND0_BITS 3
#define COMPOUND_ROUND1_BITS 7
#define WIENER_ROUND0_BITS 3
#define MAX_FILTER_TAP 8
#define YV12_FLAG_HIGHBITDEPTH 8

typedef struct yv12_buffer_config {
    int y_width;
    int uv_width;
    int y_height;
    int uv_height;
    int y_stride;
    int uv_stride;
    uint8_t *y_buffer;
    uint8_t *u_buffer;
    uint8_t *v_buffer;
    int border;
    EWLLinearMem_t mem;
} YV12_BUFFER_CONFIG;

typedef enum ATTRIBUTE_PACKED {
    EIGHTTAP_REGULAR,
    EIGHTTAP_SMOOTH,
    MULTITAP_SHARP,
    BILINEAR,
    INTERP_FILTERS_ALL,
    SWITCHABLE_FILTERS = BILINEAR,
    SWITCHABLE = SWITCHABLE_FILTERS + 1, /* the last switchable one */
    EXTRA_FILTERS = INTERP_FILTERS_ALL - SWITCHABLE_FILTERS,
    EIGHTTAP_REGULAR_VP9 = 6,
    EIGHTTAP_SMOOTH_VP9,
    MULTITAP_SHARP_VP9,
    BILINEAR_VP9,
    INTERP_FILTERS_ALL_VP9,
} InterpFilter;

typedef uint8_t CONV_BUF_TYPE;

typedef struct ConvolveParams {
    int do_average;
    CONV_BUF_TYPE *dst;
    int dst_stride;
    int round_0;
    int round_1;
    int plane;
    int is_compound;
    int use_jnt_comp_avg;
    int fwd_offset;
    int bck_offset;
} ConvolveParams;

typedef struct InterpFilterParams {
    const int16_t *filter_ptr;
    uint16_t taps;
    InterpFilter interp_filter;
} InterpFilterParams;

typedef struct FrameData {
    const YV12_BUFFER_CONFIG *source;
    const YV12_BUFFER_CONFIG *distorted;
    int frame_set;
    int bit_depth;
} FrameData;

struct buf_2d {
    uint8_t *buf;
    uint8_t *buf0;
    int width;
    int height;
    int stride;
};

typedef struct {
    YV12_BUFFER_CONFIG source_extended;
    YV12_BUFFER_CONFIG blurred;
    YV12_BUFFER_CONFIG sharpened;
    double last_frame_unsharp_amount;
} VMAFCtrl;

void vmaf_alloc_frame_buffer(const void *encoder, YV12_BUFFER_CONFIG *frame, int width, int height);
void vmaf_free_frame_buffer(const void *encoder, YV12_BUFFER_CONFIG *frame);
void vmaf_frame_preprocessing(const void *pEncInst, const int bit_depth, u8 *lum, u8 *cb, u8 *cr,
                              int width, int height, VMAFCtrl *ctl);
int vmaf_prepare_model();
#endif /* VMAF_H */
