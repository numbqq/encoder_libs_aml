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
 * @file    vcenc_fillroimap.h
 * @brief   format QP&CU value getting and roi map filling class&function declaration
 * @date    2021-02-01
 * @version version-1
 * @par     Copyright (c): Verisilicon
*/
#ifndef __VCENCFILLROIMAP_H
#define __VCENCFILLROIMAP_H

#ifdef __cplusplus
extern "C" {
#endif
/**
 * \page chapter 6
 *
 * ROI Map has different formats from 1 to 7. Each format owns different bits
 * that correspond to Qp_info, roiAbsQq_flag, qp_valid.
 * Qp_info affects every CU encode quality in frames.
 * QP buffer storages specific value of CU control bits information by binary file.
 * In the past, the binary file generated by the gen_roimap_bin.c file. After that operation,
 * the binary file was put into hevc_testenc by --roiMapInfoBinFile option.
 * But now, not a binary file but a script file which involves CU control information is put into hevc_testenc.
 * So in test_bench_utils.c, parameters from the script need to
 * be parsed and at the same time, CU control information for ROI Buffer should be added.

 *
 * Details of the roimap fill process
 *  - First, parse the roi file parameters
 *  - second, find the roi location, fill the data into memory
 *
 * Command line of ROI :
 * -HEVC
 * ./hevc_testencDEV -i /your_path/BQSquare_416x240_60.yuv -w 416 -h 240 -q 25 -b1 -o BQSquare_416x240_cu_block32_roiCUFile.hevc
 * --roiMapDeltaQpBlockUnit=2 --roiMapDeltaQpEnable=0 --roiMapConfigFile=./../cfg/roicfg/vsi_par_test6.roi
 * --RoiCuCtrlVer=4 --RoiQpDeltaVer=0 --enableOutputCuInfo=2
 *
 * -H264
 * ./h264_testencDEV -i /your_path/BQSquare_416x240_60.yuv -w 416 -h 240 -q 25 -b1 -o BQSquare_416x240_cu_block32_roiCUFile.hevc
 * --roiMapDeltaQpBlockUnit=2 --roiMapDeltaQpEnable=0 --roiMapConfigFile=./../cfg/roicfg/vsi_par_test6.roi
 * --RoiCuCtrlVer=4 --RoiQpDeltaVer=0 --enableOutputCuInfo=2
 */

/** @name    QP mode, INTER, INTRA, SKIP, IPCM
*   @{
*/
#define QP_TYPE_DELTA 0 ///< Delta QP
#define QP_TYPE_ABS 1   ///< ABS   QP
#define CU_MODE_INTER 0 ///< INTER MODE
#define CU_MODE_INTRA 1 ///< INTRA MODE
#define CU_MODE_SKIP 2  ///< SKIP  MODE
#define CU_MODE_IPCM 3  ///< IPCM  MODE
#define CU8_SIDE_LENGTH 8
/** @ } */

/** @enum  ROI INTRA Part
*/
enum RoiIntraPartType_e {
    ROI_PART_2N_2N_I = 0, ///< attribute 2Nx2N for INTRA
    ROI_PART_N_N_I,       ///< attribute NxN   for INTRA
};

/** @enum  RoiInterPartType_e
 *  @brief Roi Inter Part types*/
enum RoiInterPartType_e {
    ROI_INTERNAL_AUTOMODE = 0, ///< AUTOMODE
    ROI_PART_2N_2N = 1,        ///< 2Nx2N
    ROI_PART_2N_N,             ///< 2NxN
    ROI_PART_N_2N,             ///< Nx2N
    ROI_PART_2N_NU,            ///< 2NxNU
    ROI_PART_2N_ND,            ///< 2NxND
    ROI_PART_NL_2N,            ///< NLx2N
    ROI_PART_NR_2N,            ///< NRx2N
};

/** @struct RoiRect
 *  @brief  Roi rectangle element attributes
*/
typedef struct {
    u16 x0;
    u16 y0; ///< the coordinates of the top left corner of the ROI region, unit: pixel
    u16 dx;
    u16 dy; ///< the width and height of the ROI region, unit: pixel
    u8 qp;
    u8 qp_type;
    u8 qp_valid; /// < qp, qp type and qp validation
    u8 cu_size;
    u8 cu_size_valid; ///< CU size and CU size validation
    u8 mode;
    i8 part;
    u8 mode_valid;
    u8 intra_part_valid; ///< Mode and its attributes
    union {
        struct {
            i32 dir;
            u8 ref_idx[2]; ///< L0 or L1 in inter
            u8 mvx[2];     ///< 0 for L0, 1 for L1 in inter
            u8 mvy[2];     ///< 0 for L0, 1 for L1 in inter
        } inter[2];
        struct {
            i32 luma_mode[4]; ///< Luma mode in intra
            i32 chroma_mode;  ///< Chroma mode in intra
        } intra;
    } info;
} RoiRect;

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

#ifndef ROI_BUILD_SUPPORT
#define VCEncFillRoiMap(inst, roi, roimap_format, roimap_delta_qpmemory, blk_side_length) (0)
#else
i8 VCEncFillRoiMap(const void *inst, const RoiRect *roi, u8 roimap_format,
                   u8 *roimap_delta_qpmemory, u32 blk_side_length);
#endif

#ifdef __cplusplus
}
#endif
#endif