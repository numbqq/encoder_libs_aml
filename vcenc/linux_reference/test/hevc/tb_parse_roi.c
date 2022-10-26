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
 * @file    tb_parse_roi.c
 * @brief   parse the xxxx.roi file to get the roi data
 * @details tb_parse_roi.h
 * @date    2021-02-01
 * @version version-1
 * @par     Copyright (c): Verisilicon
*/
#include "HevcTestBench.h"
#include "vsi_string.h"
#include "tb_parse_roi.h"
#include "vcenc_fillroimap.h"
/**
 * @brief Print roiMapVersion and Format match rules
 * @param[in]    no input
 * @param[out]   void
*/
void help_Version_Format()
{
    printf("roiMapVersion and Format match rules\n");
    char *MatchTable[] = {
        "roiMapVersion        Available QP/CU Information Format",
        "      0                             0                  ",
        "      1                             1                  ",
        "      2                             2                  ",
        "      3                        1,2,3,4,5,6,7           ",
        NULL,
    };
    printf("%s\n", MatchTable[0]);
    printf("%s\n", MatchTable[1]);
    printf("%s\n", MatchTable[2]);
    printf("%s\n", MatchTable[3]);
    printf("%s\n", MatchTable[4]);
}
/**
 * @brief      Parse the picture number from the ROI description file
 * @param[in]  line: string, one line in the ROI description file
 * @return     picture number
 * - if an error occurs, returns -1
*/
i32 parse_pic_num(char *line)
{
    i32 poc = -1;

    if (line[0] == '#') {
        sscanf(line, "# pic=%d", &poc);
    }
    return poc;
}

/**
 * @brief      Parse inter direction from the ROI description file
 * @param[in]  string of ROI file
 * @return     ROI prediction direction
 * - if an error occurs, returns -1. An assert generates
*/
i32 parse_inter_dir(char *str)
{
    if (0 == strcmp("L0", str))
        return ROI_PRED_L0;
    if (0 == strcmp("L1", str))
        return ROI_PRED_L1;
    if (0 == strcmp("B0", str))
        return ROI_PRED_BI;
    if (0 == strcmp("B1", str))
        return ROI_PRED_BI;

    return -1;
}

/**
 * @brief      Parse intra cu part from the ROI description file
 * @param[in]  string of ROI file
 * @return     ROI intra part
 * - if an error occurs, returns -1.
*/
i8 parse_intra_cu_part(char *str)
{
    if (0 == strcmp("2Nx2N", str))
        return ROI_PART_2N_2N_I;
    if (0 == strcmp("NxN", str))
        return ROI_PART_N_N_I;
    return -1;
}

/**
 * @brief      Parse inter cu part
 * @param[in]  string of ROI file
 * @return     ROI inter part
 * - if an error occurs, returns -1. An assert generates
*/
i8 parse_inter_cu_part(char *str)
{
    if (0 == strcmp("any", str))
        return ROI_INTERNAL_AUTOMODE;
    if (0 == strcmp("2Nx2N", str))
        return ROI_PART_2N_2N;
    if (0 == strcmp("2NxN", str))
        return ROI_PART_2N_N;
    if (0 == strcmp("Nx2N", str))
        return ROI_PART_N_2N;
    if (0 == strcmp("2NxnU", str))
        return ROI_PART_2N_NU;
    if (0 == strcmp("2NxnD", str))
        return ROI_PART_2N_ND;
    if (0 == strcmp("nLx2N", str))
        return ROI_PART_NL_2N;
    if (0 == strcmp("nRx2N", str))
        return ROI_PART_NR_2N;

    return -1;
}

/**
 * @brief      parse cu mode
 * @param[in]  string of ROI file
 * @return     INTRA / INTER / SKIP / IPCM
 * - Default one is sINTRA.
*/
u8 parse_cu_mode(char *str)
{
    if (0 == strcmp("intra", str))
        return CU_MODE_INTRA;
    if (0 == strcmp("inter", str))
        return CU_MODE_INTER;
    if (0 == strcmp("skip", str))
        return CU_MODE_SKIP;
    if (0 == strcmp("ipcm", str))
        return CU_MODE_IPCM;
    return CU_MODE_INTRA;
}

/**
 * @brief      parse the roi file and fill the roi map
 * @details
 * - Parse the each rectangle description in roi file
 * - After parsing each rectangle description data, \n
 *   fill the roi map in memory.
 * @param[in]  inst               instance object
 * @param[in]  *roimapConfigFile  parsed ROI file
 * @param[in]  roimap_format      roimap format range from 1 to 7
 * @param[in]  *roimapDeltaQPmemory     store the QP and CU value
 * @param[in]  blk_side_length            block unit side length
 * @ref        vcenc_fillroimap.h
 * @return     picture number
 * - if errors occur, return -1
*/

i32 fill_pic_roimap(const void *inst, FILE *roimapConfigFile, u8 roimap_format,
                    u8 *roimapDeltaQPmemory, u32 blk_side_length)
{
    static i32 pic_this = -1;
    RoiRect roi;
    u8 roi_valid = 0;
    char buffer[MAX_LINE_LENGTH_BLOCK];
    char *line;
    i32 left = 0, width = 0, top = 0, height = 0, last_left = 0, last_width = 0, last_top = 0,
        last_height = 0;
    char str0[MAX_LINE_LENGTH_BLOCK], str1[MAX_LINE_LENGTH_BLOCK];
    i32 value = 0;
    u32 rect_num = -1; //rect_num is used to count the reading steps of rectangle coordinate.
    while (pic_this == -1) {
        /* first entry */
        buffer[0] = '\0';

        /* Read one line */
        line = fgets((char *)buffer, MAX_LINE_LENGTH_BLOCK, roimapConfigFile);
        printf("line content is %s \n", line);
        if (line == NULL) {
            printf("ERROR: can't get line \n");
            return NOK;
        }

        pic_this = parse_pic_num(line);
        if (pic_this == -1) {
            printf("ERROR: file not start from # pic=xx\n");
            return NOK;
        }
        printf("#pic = %d \n", pic_this);
    }

    memset(&roi, 0, sizeof(roi));

    while (1) {
        buffer[0] = '\0';

        /* Read one line */
        line = fgets((char *)buffer, MAX_LINE_LENGTH_BLOCK, roimapConfigFile);

        /* end of description file */
        if (line == NULL)
            return NOK;

        if (line[0] == 'r' || line[0] == '#' || line[0] == 'f') {
            //last_xxx keeps the last rectangle location
            last_left = left;
            last_top = top;
            last_width = width;
            last_height = height;
        }

        if ((roi_valid && rect_num > 0) ||
            (rect_num == 0 && (line[0] == 'r' || line[0] == '#' || line[0] == 'f'))) {
            //update the roi coordinate into the one of last time's
            roi.x0 = last_left;
            roi.y0 = last_top;
            roi.dx = last_width;
            roi.dy = last_height;
            if ((VCEncFillRoiMap(inst, &roi, roimap_format, roimapDeltaQPmemory,
                                 blk_side_length)) == NOK)
                return NOK;
            memset(&roi, 0, sizeof(roi));
            printf("VCEncFillRoiMap called ! \n");
            rect_num--;
        }

        /* picture end */
        if ((line[0] == 'f') || (line[0] == '#')) /* 'f' is in 'final'. */
            break;

        switch (line[0]) {
        case 'r':
            printf("%s", line);
            sscanf(line, "rect=(%d,%d,%d,%d)", &left, &top, &width, &height);
            roi.x0 = left;
            roi.y0 = top;
            roi.dx = width;
            roi.dy = height;
            roi_valid = 1;
            rect_num++;
            break;

        case ' ':
            switch (line[2]) {
            case 'q':
                /* qp */
                sscanf(line, "  qp=(%[^,], %d)", str0, &value);
                if (0 == strncmp(str0, "abs", strlen("abs"))) {
                    roi.qp_type = QP_TYPE_ABS;
                } else if (0 == strncmp(str0, "delta", strlen("delta"))) {
                    roi.qp_type = QP_TYPE_DELTA;
                } else {
                    printf("ERROR: invalid QP type in line\n %s\n", line);
                    return NOK;
                }
                roi.qp_valid = 1;
                roi.qp = value;
                break;

            case 's':
                /* size */
                sscanf(line, "  size=%d", &value);
                roi.cu_size_valid = 1;
                roi.cu_size = value;
                break;

            case 'm':
                /* mode */
                sscanf(line, "  mode=(%[^,], %[^)])", str0, str1);
                roi.mode = parse_cu_mode(str0);
                roi.mode_valid = 1;
                if (roi.mode == CU_MODE_INTRA) {
                    roi.part = parse_intra_cu_part(str1);
                    if (roi.part == -1) {
                        roi.part = 0;
                        roi.intra_part_valid = 0;
                    } else {
                        roi.intra_part_valid = 1;
                    }
                    if (((roi.part == ROI_PART_2N_2N_I) || (roi.part == ROI_PART_N_N_I)) == 0) {
                        return NOK;
                    }
                } else if (roi.mode == CU_MODE_INTER) {
                    roi.part = parse_inter_cu_part(str1);
                } else if ((roi.mode == CU_MODE_IPCM) || (roi.mode == CU_MODE_SKIP)) {
                    roi.intra_part_valid = 0;
                } else {
                    printf("Error: Invalid cu mode: %s.\n", str0);
                    return NOK;
                }
                break;

            case 'i':
                /* intra modes */
                sscanf(line, "  intra=(%d, %d, %d, %d, %d)", &roi.info.intra.luma_mode[0],
                       &roi.info.intra.luma_mode[1], &roi.info.intra.luma_mode[2],
                       &roi.info.intra.luma_mode[3], &roi.info.intra.chroma_mode);
                break;
            case 'p':
                /* intra modes */
                {
                    int32_t ref, mvx, mvy, index, part_idx;
                    if (line[6] == '0') {
                        part_idx = 0;
                        sscanf(line, "  part0=(%[^,], %d, %d, %d)", str0, &ref, &mvx, &mvy);
                    } else {
                        part_idx = 1;
                        sscanf(line, "  part1=(%[^,], %d, %d, %d)", str0, &ref, &mvx, &mvy);
                    }
                    roi.info.inter[part_idx].dir = parse_inter_dir(str0);
                    if (str0[1] == '0')
                        index = 0;
                    else
                        index = 1;
                    roi.info.inter[part_idx].ref_idx[index] = ref;
                    roi.info.inter[part_idx].mvx[index] = mvx;
                    roi.info.inter[part_idx].mvy[index] = mvy;
                }
                break;

            default:
                printf("roi file format wrong !\n");
                return NOK;
            }
            break;
        default:
            printf("roi file format wrong !\n");
            return NOK;
        }
    }
    return OK;
}
