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
 * @file    tb_parse_roi.h
 * @brief   parse the xxxx.roi file to get the roi data
 * @date    2021-02-01
 * @version version-1
 * @par     Copyright (c): Verisilicon
*/
#ifndef _TB_PARSE_ROI_H
#define _TB_PARSE_ROI_H
#ifdef __cplusplus
extern "C" {
#endif
#ifndef MAX_LINE_LENGTH_BLOCK
/** @name  Maximum line length blocks
*   @{
*/
#define MAX_LINE_LENGTH_BLOCK (512 * 8)
/** @ } */
#endif

/** @enum  ROI prediction direction
*/
enum inter_pred_idc { ROI_PRED_L0 = 0, ROI_PRED_L1 = 1, ROI_PRED_BI = 2 };

/**
 * @brief      Parse the picture number from the ROI description file
 * @param[in]  line: string, one line in the ROI description file
 * @return     picture number
 * - if an error occurs, returns -1
*/
i32 parse_pic_num(char *line);

/**
 * @brief      Parse inter direction from the ROI description file
 * @param[in]  string of ROI file
 * @return     ROI prediction direction
 * - if an error occurs, returns -1. An assert generates
*/
i32 parse_inter_dir(char *str);

/**
 * @brief      Parse intra cu part from the ROI description file
 * @param[in]  string of ROI file
 * @return     ROI intra part
 * - if an error occurs, returns -1.
*/
i8 parse_intra_cu_part(char *str);

/**
 * @brief      Parse inter cu part
 * @param[in]  string of ROI file
 * @return     ROI inter part
 * - if an error occurs, returns -1. An assert generates
*/
i8 parse_inter_cu_part(char *str);

/**
 * @brief      parse cu mode
 * @param[in]  string of ROI file
 * @return     INTRA / INTER / SKIP / IPCM
 * - Default one is sINTRA.
*/
u8 parse_cu_mode(char *str);

/**
 * @brief      parse the roi file and fill the roi map
 * @details
 * - Parse the each rectangle description in roi file
 * - After parsing each rectangle description data, \n
 *   fill the roi map in memory.
 * @param[in]  inst               instance object
 * @param[in]  *roimapConfigFile    parsed ROI file
 * @param[in]  roimap_format      format range from 1 to 7
 * @param[in]  *roimapDeltaQPmemory    store the QP and CU value
 * @param[in]  blk_side_length            block unit side length
 * @ref        vcenc_fillroimap.h
 * @return     picture number
 * - if errors occur, return -1
*/

i32 fill_pic_roimap(const void *inst, FILE *roimapConfigFile, u8 roimap_format,
                    u8 *roimapDeltaQPmemory, u32 blk_side_length);

/**
 * @brief Print roiMapVersion and Format match rules
 * @param[in]    no input
 * @param[out]   void
*/
void help_Version_Format();
#ifdef __cplusplus
}
#endif
#endif