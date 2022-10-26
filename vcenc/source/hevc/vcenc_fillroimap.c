/*------------------------------------------------------------------------------
--                                                                                                                               --
--       This software is confidential and proprietary and may be used                                   --
--        only as expressly authorized by a licensing agreement from                                     --
--                                                                                                                               --
--                            Verisilicon.                                                                                    --
--                                                                                                                               --
--                   (C) COPYRIGHT 2021 VERISILICON                                                            --
--                            ALL RIGHTS RESERVED                                                                    --
--                                                                                                                               --
--                 The entire notice above must be reproduced                                                 --
--                  on all copies and should not be removed.                                                    --
--                                                                                                                               --
--------------------------------------------------------------------------------*/
/**
 * @file    vcenc_fillroimap.c
 * @brief   format QP&CU value getting and roi map filling
 * @details vcenc_fillroimap.h
 * @date    2021-02-01
 * @version version-1
 * @par     Copyright (c): Verisilicon
*/
#ifdef ROI_BUILD_SUPPORT

#include "ewl.h"
#include "hevcencapi.h"
#include "hevcencapi_utils.h"
#include "instance.h"
#include "vcenc_fillroimap.h"

/** @name    The number of bytes that a QP/CU control information data element occupies
*   @{
*/
#define FORMAT_BYTE_LEN 14
/** @ } */

/** @name    Caculate the specify length unit index in the whole picture frame by coordinate.
*   @{
*/
#define UNIT_INDEX(roi_coordinate, length)                                                         \
    (((roi_coordinate) & ((length)-1)) == 0 ? ((roi_coordinate) / (length)-1)                      \
                                            : (roi_coordinate) / (length))
/** @ } */

/** @name    First align the length by the specify unit length and segment with the segment length
*   @{
*/
#define ALIGN_AND_SEGMENT(length, align_unit_length, segment_length)                               \
    (((((length) + (align_unit_length)-1) & (~((align_unit_length)-1))) + (segment_length)-1) /    \
     (segment_length))
/** @ } */

/*
 * QP/CU control information data element size in bytes;
 */
static const u8 format_byte_len[] = {1, 1, 1, 1, 2, 6, 12, 14};

static u8 QPCU_Ctrl_Infor[FORMAT_BYTE_LEN] = {0};
/**
 * @brief         format 1 data generate
 * @param[in]     rect                Parsed roi rectangle information
 * @param[out]    QPCU_Ctrl_Infor     QP_CU value and data element
 * @return        returns 0
*/
static u8 fill_roimap_fmt1(const RoiRect *rect, u8 *QPCU_Ctrl_Infor)
{
    u8 is_abs = (u8)(rect->qp_type == QP_TYPE_ABS);
    u8 qp = rect->qp;
    u8 is_ipcm = (u8)(rect->mode == CU_MODE_IPCM);
    QPCU_Ctrl_Infor[0] = ((is_ipcm << 7) | ((qp & 0x3f) << 1) | (is_abs << 0));
    return 0;
}
/**
 * @brief         format 2 data generate
 * @param[in]     rect                Parsed roi rectangle information
 * @param[out]    QPCU_Ctrl_Infor     QP_CU value and data element
 * @return        returns 0
*/
static u8 fill_roimap_fmt2(const RoiRect *rect, u8 *QPCU_Ctrl_Infor)
{
    u8 is_delta = (u8)(rect->qp_type == QP_TYPE_DELTA);
    u8 qp = rect->qp;
    u8 is_skip = (u8)(rect->mode == CU_MODE_SKIP);
    QPCU_Ctrl_Infor[0] = ((is_skip << 7) | ((qp & 0x3f) << 0) | (is_delta << 6));
    return 0;
}
/**
 * @brief         format 3 data generate
 * @param[in]     rect                Parsed roi rectangle information
 * @param[out]    QPCU_Ctrl_Infor     QP_CU value and data element
 * @return        returns 0
*/
static u8 fill_roimap_fmt3(const RoiRect *rect, u8 *QPCU_Ctrl_Infor)
{
    if (rect->qp_valid) {
        int32_t is_abs = (rect->qp_type == QP_TYPE_ABS);
        int32_t qp = rect->qp;
        QPCU_Ctrl_Infor[0] = ((qp & 0x3f) << 2) | (is_abs << 1) | (1 << 0);
    }
    return 0;
}
/**
 * @brief         format 4 data generate
 * @param[in]     rect                Parsed roi rectangle information
 * @param[out]    QPCU_Ctrl_Infor     QP_CU value and data element
 * @return        returns 0
*/
static u8 fill_roimap_fmt4(const RoiRect *rect, u8 *QPCU_Ctrl_Infor)
{
    fill_roimap_fmt3(rect, QPCU_Ctrl_Infor);
    if (rect->cu_size_valid) {
        u8 cu_size = (rect->cu_size == 8)
                         ? 0
                         : ((rect->cu_size == 16) ? 1 : ((rect->cu_size == 32) ? 2 : 3));
        QPCU_Ctrl_Infor[1] |= ((cu_size & 0x3) << 1) | (1 << 0);
    }
    if (rect->mode_valid) {
        QPCU_Ctrl_Infor[1] |= (1 << 3);
        if (rect->mode == CU_MODE_INTER) {
            QPCU_Ctrl_Infor[1] |= ((rect->part & 0x7) << 5) | (1 << 4);
        } else if (rect->mode == CU_MODE_INTRA) {
            if (rect->intra_part_valid) {
                QPCU_Ctrl_Infor[1] |= (rect->part << 7);
                QPCU_Ctrl_Infor[1] |= (1 << 5);
            }
        } else if (rect->mode == CU_MODE_SKIP) {
            QPCU_Ctrl_Infor[1] |= (2 << 5);
        } else if (rect->mode == CU_MODE_IPCM) {
            QPCU_Ctrl_Infor[1] |= (3 << 5);
        }
    }
    return 0;
}
/**
 * @brief         format 5 data generate
 * @param[in]     rect                Parsed roi rectangle information
 * @param[out]    QPCU_Ctrl_Infor       QP_CU value and data element
 * @return        returns 0
*/
static u8 fill_roimap_fmt5(const RoiRect *rect, u8 *QPCU_Ctrl_Infor)
{
    fill_roimap_fmt4(rect, QPCU_Ctrl_Infor);
    if (rect->mode != CU_MODE_INTRA)
        return 1;
    QPCU_Ctrl_Infor[2] |=
        (rect->info.intra.luma_mode[0] & 0x3f) | ((rect->info.intra.luma_mode[1] & 0x3) << 6);
    QPCU_Ctrl_Infor[3] |= ((rect->info.intra.luma_mode[1] & 0x3c) >> 2) |
                          ((rect->info.intra.luma_mode[2] & 0xf) << 4);
    QPCU_Ctrl_Infor[4] |= ((rect->info.intra.luma_mode[2] & 0x3) >> 4) |
                          ((rect->info.intra.luma_mode[3] & 0x3f) << 2);
    QPCU_Ctrl_Infor[5] |= (rect->info.intra.chroma_mode & 0x3f);
    return 0;
}
/**
 * @brief         format 6 data generate
 * @param[in]     rect                Parsed roi rectangle information
 * @param[out]    QPCU_Ctrl_Infor     QP_CU value and data element
 * @return        returns 0
*/
static u8 fill_roimap_fmt6(const RoiRect *rect, u8 *QPCU_Ctrl_Infor)
{
    u8 ret = fill_roimap_fmt5(rect, QPCU_Ctrl_Infor);
    if (ret == 1)
        return 1;
#if 0
  if (rect->mode != CU_MODE_INTER)
    return 1;
  if (rect->part == ROI_INTERNAL_AUTOMODE)
    return 1;

  roi->data[6] |= (rect->info.inter[0].dir&0x3) | ((rect->info.inter[0].ref_idx[0]&0x3)<<2)
                  | ((rect->info.inter[0].mvx[0]&0xf)<<2);
  roi->data[7] |=  ((rect->info.inter[0].mvx[0]&0xff0)>>4);
  roi->data[8] |=  ((rect->info.inter[0].mvx[0]&0x3000)>>12)|((rect->info.inter[0].mvy[0]&0x3f)<<0);
  roi->data[9] |=  ((rect->info.inter[0].mvy[0]&0x3fc0)>>6);
  roi->data[10] |=  ((rect->info.inter[0].ref_idx[1]&0x3)<<0)
                  | ((rect->info.inter[0].mvx[1]&0x3f)<<2);
  roi->data[11] |=  ((rect->info.inter[0].mvx[1]&0x3fc0)<<6);
  roi->data[12] |=  ((rect->info.inter[0].mvy[1]&0xff)<<0);
  roi->data[13] |=  ((rect->info.inter[0].mvy[1]&0x3f00)>>8);
#else
    int mvxi[2], mvyi[2], mvxf_p0[2], mvyf_p0[2];
    int dir;

    if (rect->mode != CU_MODE_INTER)
        return 1;
    if (rect->part == ROI_INTERNAL_AUTOMODE)
        return 1;

    dir = rect->info.inter[0].dir;
    if (rect->part == ROI_PART_2N_2N) {
        /* one partition */
        /* L0 */
        mvxi[0] = rect->info.inter[0].mvx[0] >> 2;
        mvxf_p0[0] = rect->info.inter[0].mvx[0] & 0x3;
        mvyi[0] = rect->info.inter[0].mvy[0] >> 2;
        mvyf_p0[0] = rect->info.inter[0].mvy[0] & 0x3;
        /* L1 */
        mvxi[1] = rect->info.inter[0].mvx[1] >> 2;
        mvyi[1] = rect->info.inter[0].mvy[1] >> 2;
        mvxf_p0[1] = rect->info.inter[0].mvx[1] & 0x3;
        mvyf_p0[1] = rect->info.inter[0].mvy[1] & 0x3;
        /* part 2 */
    } else {
        /* two partitions */
        if (rect->info.inter[0].dir != rect->info.inter[1].dir) {
            APITRACEERR("QP_CU roimap format 6 error: rect->info.inter[0].dir != "
                        "rect->info.inter[1].dir\n");
            return 1;
        }
        /* L0 */
        mvxi[0] = (rect->info.inter[0].mvx[0] + rect->info.inter[1].mvx[0] + 2) >> 3;
        mvxf_p0[0] = rect->info.inter[0].mvx[0] - (mvxi[0] << 2);
        mvyi[0] = (rect->info.inter[0].mvy[0] + rect->info.inter[1].mvy[0] + 2) >> 3;
        mvyf_p0[0] = rect->info.inter[0].mvy[0] - (mvyi[0] << 2);
        if (((mvxf_p0[0] < 4) && (mvxf_p0[0] >= -4)) || ((mvyf_p0[0] < 4) && (mvyf_p0[0] >= -4))) {
            APITRACEERR("QP_CU roimap format 6 error: (mvxf_p0[0]<4)&&(mvxf_p0[0]>=-4) || "
                        "(mvyf_p0[0]<4)&&(mvyf_p0[0]>=-4)\n");
            return 1;
        }
        /* L1 */
        mvxi[1] = (rect->info.inter[0].mvx[1] + rect->info.inter[1].mvx[1] + 2) >> 3;
        mvxf_p0[1] = rect->info.inter[0].mvx[1] - (mvxi[1] << 2);
        mvyi[1] = (rect->info.inter[0].mvy[1] + rect->info.inter[1].mvy[1] + 2) >> 3;
        mvyf_p0[1] = rect->info.inter[0].mvy[1] - (mvyi[1] << 2);
        if (((mvxf_p0[1] < 4) && (mvxf_p0[1] >= -4)) || ((mvyf_p0[1] < 4) && (mvyf_p0[1] >= -4))) {
            APITRACEERR("QP_CU roimap format 6 error: ((mvxf_p0[1]<4)&&(mvxf_p0[1]>=-4)) || "
                        "((mvyf_p0[1]<4)&&(mvyf_p0[1]>=-4))\n");
            return 1;
        }
    }

    QPCU_Ctrl_Infor[6] |= (dir & 0x3)                                           /* 2bit dir */
                          | (((mvxi[0]) & 0x3f) << 2);                          /* 9bit mvxi */
    QPCU_Ctrl_Infor[7] |= (((mvxi[0] >> 6) & 0x7)) | (((mvyi[0]) & 0x1f) << 3); /* 7bit mvyi */
    QPCU_Ctrl_Infor[8] |= (((mvyi[0] >> 5) & 0x3)) | (((mvxi[1]) & 0x3f) << 2); /* 9bit mvxi */
    QPCU_Ctrl_Infor[9] |= (((mvxi[1] >> 6) & 0x7)) | (((mvyi[1]) & 0x1f) << 3); /* 7bit mvyi */
    QPCU_Ctrl_Infor[10] |=
        (((mvyi[1] >> 5) & 0x3) << 0) | (((mvxf_p0[0])) << 2) | (((mvyf_p0[0])) << 5);
    QPCU_Ctrl_Infor[11] |= (((mvxf_p0[1])) << 0) | (((mvyf_p0[1])) << 3);
#endif
    return 0;
}
/**
 * @brief         format 7 data generate
 * @param[in]     rect                Parsed roi rectangle information
 * @param[out]    QPCU_Ctrl_Infor     QP_CU value and data element
 * @return        returns 0
*/
static u8 fill_roimap_fmt7(const RoiRect *rect, u8 *QPCU_Ctrl_Infor)
{
    u8 ret = fill_roimap_fmt6(rect, QPCU_Ctrl_Infor);
    if (ret == 1)
        return 1;
    if (rect->mode != CU_MODE_INTER)
        return 1;
    if ((rect->part == ROI_PART_2N_2N) || (rect->part == ROI_INTERNAL_AUTOMODE))
        return 1;

    int mvxi[2], mvyi[2], mvxf_p1[2], mvyf_p1[2];

    if (rect->mode != CU_MODE_INTER)
        return 1;
    if (rect->part == ROI_INTERNAL_AUTOMODE)
        return 1;

    if (rect->part == ROI_PART_2N_2N) {
        /* one partition */
        /* L0 */
        mvxi[0] = rect->info.inter[0].mvx[0] >> 2;
        mvyi[0] = rect->info.inter[0].mvy[0] >> 2;
        /* L1 */
        mvxi[1] = rect->info.inter[0].mvx[1] >> 2;
        mvyi[1] = rect->info.inter[0].mvy[1] >> 2;
        /* part 2 */
        mvxf_p1[0] = mvyf_p1[0] = mvxf_p1[1] = mvyf_p1[1] = 0;
    } else {
        /* two partitions */
        if (rect->info.inter[0].dir != rect->info.inter[1].dir) {
            APITRACEERR("QP_CU roimap format 6 error: rect->info.inter[0].dir != "
                        "rect->info.inter[1].dir\n");
            return 1;
        }
        /* L0 */
        mvxi[0] = (rect->info.inter[0].mvx[0] + rect->info.inter[1].mvx[0] + 2) >> 3;
        mvxf_p1[0] = rect->info.inter[1].mvx[0] - (mvxi[0] << 2);
        mvyi[0] = (rect->info.inter[0].mvy[0] + rect->info.inter[1].mvy[0] + 2) >> 3;
        mvyf_p1[0] = rect->info.inter[1].mvy[0] - (mvyi[0] << 2);
        if (((mvxf_p1[0] < 4) && (mvxf_p1[0] >= -4)) || ((mvyf_p1[0] < 4) && (mvyf_p1[0] >= -4))) {
            APITRACEERR("QP_CU roimap format 6 error: (mvxf_p0[0]<4)&&(mvxf_p0[0]>=-4) || "
                        "(mvyf_p0[0]<4)&&(mvyf_p0[0]>=-4)\n");
            return 1;
        }
        /* L1 */
        mvxi[1] = (rect->info.inter[0].mvx[1] + rect->info.inter[1].mvx[1] + 2) >> 3;
        mvxf_p1[1] = rect->info.inter[1].mvx[1] - (mvxi[1] << 2);
        mvyi[1] = (rect->info.inter[0].mvy[1] + rect->info.inter[1].mvy[1] + 2) >> 3;
        mvyf_p1[1] = rect->info.inter[1].mvy[1] - (mvyi[1] << 2);
        if (((mvxf_p1[1] < 4) && (mvxf_p1[1] >= -4)) || ((mvyf_p1[1] < 4) && (mvyf_p1[1] >= -4))) {
            APITRACEERR("QP_CU roimap format 6 error: ((mvxf_p0[1]<4)&&(mvxf_p0[1]>=-4)) || "
                        "((mvyf_p0[1]<4)&&(mvyf_p0[1]>=-4))\n");
            return 1;
        }
    }
    QPCU_Ctrl_Infor[12] |= (((mvxf_p1[0])) << 0) | (((mvyf_p1[0])) << 3);
    QPCU_Ctrl_Infor[13] |= (((mvxf_p1[1])) << 0) | (((mvyf_p1[1])) << 3);
#if 0
  roi->data[12] |=  (((rect->info.inter.mvxf1[0]))<<0)
                  | (((rect->info.inter.mvyf1[0]))<<3);
  roi->data[13] |=  (((rect->info.inter.mvxf1[1]))<<0)
                  | (((rect->info.inter.mvyf1[1]))<<3);
#endif
    return 0;
}

static void fill_data(u8 *p, u8 roimap_format, u8 temp_start_x, u8 temp_end_x, u8 temp_start_y,
                      u8 temp_end_y, u8 temp_start_x_offset, u8 temp_start_y_offset,
                      u8 ctb_side_len_steps)
{
    u32 i, j, k;
    u8 *p_start;
    u8 tmp = format_byte_len[roimap_format];
    p += temp_start_y_offset + temp_start_x_offset;
    p_start = p;
    for (i = temp_start_y; i <= temp_end_y; i++) {
        for (j = temp_start_x; j <= temp_end_x; j++) {
            for (k = 0; k < tmp; k++) {
                *p++ = QPCU_Ctrl_Infor[k];
            }
        }
        p_start += ctb_side_len_steps;
        p = p_start;
    }
}

/* use the function pointer array to point the fill_roimap_fmtx() to reduce the "if" choice.
* In relationship table, format boundary value has been checked in above code,
* checking process is very important in relationship table.
*/

static u8 (*fill_roimap_fmt[])(const RoiRect *, u8 *) = {
    fill_roimap_fmt1, fill_roimap_fmt2, fill_roimap_fmt3, fill_roimap_fmt4,
    fill_roimap_fmt5, fill_roimap_fmt6, fill_roimap_fmt7};

/**
 * @brief         Fill the QP&CU value into the memory region of specific positon \n
 *                First, judge whether roimap format matches block side length or not.\n
 *                Second, expand the roi in the four points of specify size. \n
 *                Third, fill the QP or CU value in the memory region. \n
 *                Formula: DataMemory[f(top, left, width, height)] = QP or CU
 * @param[in]     *inst               instance of encoder
 * @param[in]     roi                 roi rectangle
 * @param[in]     roimap_format           roimap range from format 1 to 7
 * @param[in]     *roimap_delta_qpmemory  memory storing the QP&CU information
 * @param[in]     blk_side_length         block unit side length
 * @return        returns VCENC_ERROR if error occurs
 */
i8 VCEncFillRoiMap(const void *inst, const RoiRect *roi, u8 roimap_format,
                   u8 *roimap_delta_qpmemory, u32 blk_side_length)
{
    struct vcenc_instance *pEncInst = (struct vcenc_instance *)inst;

    if (pEncInst->codecFormat == VCENC_VIDEO_CODEC_AV1) {
        APITRACEERR("CODEC_AV1 is not supported in the --roiConfigFile option.\n");
        return VCENC_ERROR;
    }
    if (pEncInst->codecFormat == VCENC_VIDEO_CODEC_VP9) {
        APITRACEERR("CODEC_VP9 is not supported in the --roiConfigFile option.\n");
        return VCENC_ERROR;
    }
    /* check if the option values are valid */
    if ((roimap_format == 0) || (roimap_format > 7)) {
        APITRACEPARAM("ERROR: format = %d. Invalid!\n", roimap_format);
        return VCENC_ERROR;
    }
    memset(QPCU_Ctrl_Infor, 0, FORMAT_BYTE_LEN);
    fill_roimap_fmt[roimap_format - 1](roi, QPCU_Ctrl_Infor);
    if (pEncInst->codecFormat == VCENC_VIDEO_CODEC_HEVC) {
        // --If the IPCM_flag == 1, the software forces the hardware to
        //   code the corresponding 64 x 64 CTU (HEVC) or 16 x 16 macroblock (H.264) in IPCM mode.
        // --When skip_flag ==1, the software forces the hardware to code
        //   the corresponding 32 x 32 CU (HEVC) or 16 x 16 macroblock (H.264) in skip mode.
        if (roimap_format == 1 && (QPCU_Ctrl_Infor[0] & 0x80) == 0x80 &&
            pEncInst->asic.regs.ipcmMapEnable == 1) {
            if (blk_side_length != 64) {
                APITRACEWRN("Warning: In the IPCM mode, Treat Block Unit as 64 x 64 for HEVC.");
                blk_side_length = 64;
            }
        }
        if (roimap_format == 2 && (QPCU_Ctrl_Infor[0] & 0x80) == 0x80 &&
            pEncInst->asic.regs.skipMapEnable == 1) {
            if (blk_side_length != 32) {
                APITRACEWRN("Warning: In the SKIP mode, Treat Block Unit as 32 x 32 for H264.");
                blk_side_length = 32;
            }
        }
    } else if (pEncInst->codecFormat == VCENC_VIDEO_CODEC_H264) {
        if (blk_side_length != 16) {
            APITRACEWRN("Warning: Treat Block Unit as 16 x 16 for H264.");
            blk_side_length = 16;
        }
    }

    // pEncInst->max_cu_size means length of max CU : HEVC--64; H264--16
    // calculation example  ( ((416 + 64 - 1) & (~(64 - 1))) + 32 - 1 ) / 32 = ( ((416 + 64 - 1) & (1111 1111 1100 0000) + 32 - 1 ) / 32
    //                                                          =  ( 479 & (1111 1111 1100 0000) + 32 -1 ) / 32
    // Fill a full max_cu_size length at rows trail for new length, then cut off with max_cu_size in new length. Finally separates
    // the rows with block length.
    // In the Frames, count the block numbers in rows and columns direction
    // block number in ctb side and cu8 number in block side
    u8 block_num_in_ctb_side = pEncInst->max_cu_size / blk_side_length;
    u8 cu8_num_in_block_side = blk_side_length / CU8_SIDE_LENGTH;
    u8 cu8_num_in_ctb_side = pEncInst->max_cu_size / CU8_SIDE_LENGTH;
    u8 cu8_num_in_ctb = cu8_num_in_ctb_side * cu8_num_in_ctb_side; // cu8 number in ctb.
    u32 cu8_num_in_pic_CtbRows =
        ((ALIGN_AND_SEGMENT(pEncInst->width, pEncInst->max_cu_size, blk_side_length)) /
         block_num_in_ctb_side) *
        cu8_num_in_ctb;
    // rect= {top, left, width, height} = {x0, y0, dx, dy}
    // Expand the roi coordinates into the block unit indexes
    // block index means the coordinate of each block in the whole pciture frame.
    u16 ctb_index_of_left_top_x = (roi->x0 / blk_side_length) / block_num_in_ctb_side;
    u16 ctb_index_of_left_top_y = (roi->y0 / blk_side_length) / block_num_in_ctb_side;
    u16 ctb_index_of_right_buttom_x =
        (UNIT_INDEX(roi->x0 + roi->dx, blk_side_length)) / block_num_in_ctb_side;
    u16 ctb_index_of_right_buttom_y =
        (UNIT_INDEX(roi->y0 + roi->dy, blk_side_length)) / block_num_in_ctb_side;
    u8 *ctb_x_start_p = &roimap_delta_qpmemory[(ctb_index_of_left_top_y * cu8_num_in_pic_CtbRows +
                                                ctb_index_of_left_top_x * cu8_num_in_ctb) *
                                               format_byte_len[roimap_format]];
    u8 *ctb_x_start_p_origin = ctb_x_start_p;
    u8 ctb_start_x = ((roi->x0 / blk_side_length) % block_num_in_ctb_side) * cu8_num_in_block_side;
    u8 ctb_start_x_offset = ctb_start_x * format_byte_len[roimap_format];
    u8 ctb_start_y = ((roi->y0 / blk_side_length) % block_num_in_ctb_side) * cu8_num_in_block_side;
    u8 ctb_start_y_offset = ctb_start_y * cu8_num_in_ctb_side * format_byte_len[roimap_format];
    u8 ctb_end_x = ((UNIT_INDEX(roi->x0 + roi->dx, blk_side_length)) % block_num_in_ctb_side) *
                       cu8_num_in_block_side +
                   cu8_num_in_block_side - 1;
    u8 ctb_end_y = ((UNIT_INDEX(roi->y0 + roi->dy, blk_side_length)) % block_num_in_ctb_side) *
                       cu8_num_in_block_side +
                   cu8_num_in_block_side - 1;
    u32 steps_column = cu8_num_in_pic_CtbRows * format_byte_len[roimap_format];
    u32 steps_row = cu8_num_in_ctb * format_byte_len[roimap_format];
    u32 move_stride = cu8_num_in_ctb_side * format_byte_len[roimap_format];
    u8 ctb_temp_y = 0;
    u8 ctb_temp_x = 0;
    u8 temp_start_y = 0;
    u8 temp_start_y_offset = 0;
    u8 temp_end_y = 0;
    u8 temp_start_x = 0;
    u8 temp_start_x_offset = 0;
    u8 temp_end_x = 0;

    for (ctb_temp_y = ctb_index_of_left_top_y; ctb_temp_y <= ctb_index_of_right_buttom_y;
         ctb_temp_y++) {
        temp_start_y = 0;
        temp_start_y_offset = 0;
        temp_end_y = cu8_num_in_ctb_side - 1;

        if (ctb_temp_y == ctb_index_of_left_top_y) {
            temp_start_y = ctb_start_y;
            temp_start_y_offset = ctb_start_y_offset;
        }
        if (ctb_temp_y == ctb_index_of_right_buttom_y) {
            temp_end_y = ctb_end_y;
        }
        for (ctb_temp_x = ctb_index_of_left_top_x; ctb_temp_x <= ctb_index_of_right_buttom_x;
             ctb_temp_x++) {
            temp_start_x = 0;
            temp_start_x_offset = 0;
            temp_end_x = cu8_num_in_ctb_side - 1;

            if (ctb_temp_x == ctb_index_of_left_top_x) {
                temp_start_x = ctb_start_x;
                temp_start_x_offset = ctb_start_x_offset;
            }
            if (ctb_temp_x == ctb_index_of_right_buttom_x) {
                temp_end_x = ctb_end_x;
            }
            fill_data(ctb_x_start_p, roimap_format, temp_start_x, temp_end_x, temp_start_y,
                      temp_end_y, temp_start_x_offset, temp_start_y_offset, move_stride);
            ctb_x_start_p += steps_row;
        }
        ctb_x_start_p_origin += steps_column;
        ctb_x_start_p = ctb_x_start_p_origin;
    }
    return VCENC_OK;
}

#endif
