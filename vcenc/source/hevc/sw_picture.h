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

#ifndef SW_PICTURE_H
#define SW_PICTURE_H

#include "base_type.h"
#include "vsi_queue.h"
#include "sw_parameter_set.h"
#include "container.h"
#include "encasiccontroller.h"

#define MAX_CU_WIDTH 64

#define RECON_PIPELINE_CTU_NUM 4

enum sw_image_type { SW_IMAGE_U8 = 0, SW_IMAGE_I32 };

struct sw_data {
    enum sw_image_type type;
    i32 lum_width;  /* Width of *lum */
    i32 lum_height; /* Height of *lum */
    i32 ch_width;   /* Width of *cb and *cr */
    i32 ch_height;  /* Height of *cb and *cr */
    i32 lum_pad_width;
    i32 lum_pad_height;
    i32 ch_pad_width;
    i32 ch_pad_height;
    ptr_t lum;
    ptr_t cb;
    ptr_t cr;
};

struct sw_buffer_s {
    u8 *stream;        /* Stream buffer */
    i32 startOffset;   /* Bytes in stream header remainder */
    u32 hdrRemainder1; /* Stream header remainder MSB */
    u32 hdrRemainder2; /* Stream header remainder LSB */
    i32 size;          /* Size of stream buffer (user set) */
    i32 *table;
    i32 tableSize;
    i32 tableCnt;
};

struct sw_compress {
    u32 lumaCompressed;
    u32 chromaCompressed;
    ptr_t lumaTblBase;
    ptr_t chromaTblBase;
};

struct sw_picture {
    struct node *next;
    struct queue memory;
    struct sw_data input[HEVC_MAX_TILE_COLS]; /* Input image store */
    struct sw_data recon;                     /* Recon image store */

    struct queue slice; /* Slice store */
    struct slice *sliceInst;
    struct vps *vps;          /* Active video parameter set */
    struct sps *sps;          /* Active sequence parameter set */
    struct pps *pps;          /* Active picture parameter set */
    struct rps *rps;          /* Active reference picture set */
    struct sw_picture ***rpl; /* Reference picture lists */
    ptr_t recon_4n_base;
    ptr_t mc_sync_addr;          /* multi-core sync address */
    ptr_t mc_sync_ptr;           /* multi-core sync virtual address */
    ptr_t framectx_base;         /* base adrress of frame context to AV1 */
    ptr_t framectx_virtual_addr; /* virtual adrress of frame context to AV1 */

    i32 poc; /* Picture order count */
    i32 frameNum; /* Frame number in decoding order, 0 for IDR, +1 for each reference frame; used for H.264 */
    i32 frameNumExt; /* Frame number given outside */
    u32 nalRefIdc;   /* current frame is used for reference */
    u32 nalRefIdc_2bit;
    u32 temporal_id;         /**< temproal layer ID */
    u32 markCurrentLongTerm; /* (H264 only) mark current frame as long term reference */
    i32 curLongTermIdx;      /* (H264 only) long term reference index of current frame */
    i32 ref_cnt;             /* Reference cnt (other thread may use) */
    i32 reference;           /* Picture is reference picture */
    i32 h264_no_ref;         /* H.264: cannot used as reference anymore (replaced ltr) */
    i32 long_term_flag;      /* Picture is long term reference picture */
    i32 encOrderInGop;       /* encoding order with in current gop */

    i32 column;
    i32 row;

    i32 ctb_num;

    i32 picture_memory_id;
    i32 picture_memory_init;
    i32 picture_cnt; /* encoded picture count */

    i32 slice_deblocking_filter_override_flag;
    i32 disableDeblocking;
    i32 sliceNum;
    i32 sliceSize;

    i32 trace_pic_cnt; /* for trace only */

    struct sw_buffer_s buffer;

    //compress
    struct sw_compress recon_compress;

    struct h264_mb_col *colctbs;
    struct h264_mb_col *colctbs_store;
    ptr_t colctbs_load_base;
    ptr_t colctbs_store_base;

    //cu information output
    ptr_t cuInfoTableBase;
    ptr_t cuInfoDataBase;

    //Ctb bits info output
    ptr_t ctbBitsDataBase;

    //crf information
    double curQpRc; //QP for this slice
    int predType;   //predId for this slice
    u64 cost;       //Cost for this slice
    u16 cmdbufid;

    /* mv info */
    ptr_t mvInfoBase;
    u32 deltaPocL0[2];
    u32 deltaPocL1[2];
    u32 av1OrderHint[8];

    //encoder output data for cutree
    encOutForCutree_s outForCutree;

    //parameters for av1 and vp9
    u32 strmHeaderLen;
    u32 av1Vp9FrameNotShow;

    //parameters for av1 only
    true_e av1_show_frame;
};

void create_slices_ctrlsw(struct sw_picture *p, struct pps *pps, u32 sliceSize);

struct vcenc_instance;
struct sw_picture *create_picture_ctrlsw(struct vcenc_instance *instance, struct vps *vps,
                                         struct sps *sps, struct pps *pps, u32 sliceSize);
struct sw_picture *get_picture(struct container *c, i32 poc);
void reference_picture_list(struct container *c, struct sw_picture *pic);

i32 sw_init_image(struct sw_picture *p, struct sw_data *image, i32 w, i32 h,
                  enum sw_image_type type, bool bPading, i32 padw, i32 padh);

void sw_free_pictures(struct container *c);
void sw_free_picture(struct sw_picture *p);

void sw_ref_cnt_increase(struct sw_picture *pic);
void sw_ref_cnt_decrease(struct sw_picture *pic);

#endif
