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
--------------------------------------------------------------------------------
--
--  Description : Preprocessor setup
--
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/
#include "encpreprocess.h"
#include "enccommon.h"

/*------------------------------------------------------------------------------
  EncPreProcessAlloc
------------------------------------------------------------------------------*/
i32 EncPreProcessAlloc(preProcess_s *preProcess, i32 mbPerPicture)
{
    i32 status = ENCHW_OK;
    i32 i;

    for (i = 0; i < 3; i++) {
        preProcess->roiSegmentMap[i] = (u8 *)EWLcalloc(mbPerPicture, sizeof(u8));
        if (preProcess->roiSegmentMap[i] == NULL)
            status = ENCHW_NOK;
    }

    for (i = 0; i < 2; i++) {
        preProcess->skinMap[i] = (u8 *)EWLcalloc(mbPerPicture, sizeof(u8));
        if (preProcess->skinMap[i] == NULL)
            status = ENCHW_NOK;
    }

    preProcess->mvMap = (i32 *)EWLcalloc(mbPerPicture, sizeof(i32));
    if (preProcess->mvMap == NULL)
        status = ENCHW_NOK;

    preProcess->scoreMap = (u8 *)EWLcalloc(mbPerPicture, sizeof(u8));
    if (preProcess->scoreMap == NULL)
        status = ENCHW_NOK;

    if (status != ENCHW_OK) {
        EncPreProcessFree(preProcess);
        return ENCHW_NOK;
    }

    return ENCHW_OK;
}

/*------------------------------------------------------------------------------
  EncPreProcessFree
------------------------------------------------------------------------------*/
void EncPreProcessFree(preProcess_s *preProcess)
{
    i32 i;

    for (i = 0; i < 3; i++) {
        if (preProcess->roiSegmentMap[i])
            EWLfree(preProcess->roiSegmentMap[i]);
        preProcess->roiSegmentMap[i] = NULL;
    }

    for (i = 0; i < 2; i++) {
        if (preProcess->skinMap[i])
            EWLfree(preProcess->skinMap[i]);
        preProcess->skinMap[i] = NULL;
    }

    if (preProcess->mvMap)
        EWLfree(preProcess->mvMap);
    preProcess->mvMap = NULL;

    if (preProcess->scoreMap)
        EWLfree(preProcess->scoreMap);
    preProcess->scoreMap = NULL;
}

/*------------------------------------------------------------------------------
    EncPreGetHwFormat
------------------------------------------------------------------------------*/
u32 EncPreGetHwFormat(u32 inputFormat)
{
    /* Input format mapping API values to SW/HW register values. */
    static const u32 inputFormatMapping[46] = {ASIC_INPUT_YUV420PLANAR,
                                               ASIC_INPUT_YUV420SEMIPLANAR,
                                               ASIC_INPUT_YUV420SEMIPLANAR,
                                               ASIC_INPUT_YUYV422INTERLEAVED,
                                               ASIC_INPUT_UYVY422INTERLEAVED,
                                               ASIC_INPUT_RGB565,
                                               ASIC_INPUT_RGB565,
                                               ASIC_INPUT_RGB555,
                                               ASIC_INPUT_RGB555,
                                               ASIC_INPUT_RGB444,
                                               ASIC_INPUT_RGB444,
                                               ASIC_INPUT_RGB888,
                                               ASIC_INPUT_RGB888,
                                               ASIC_INPUT_RGB101010,
                                               ASIC_INPUT_RGB101010,
                                               ASIC_INPUT_I010,
                                               ASIC_INPUT_P010,
                                               ASIC_INPUT_PACKED_10BIT_PLANAR,
                                               ASIC_INPUT_PACKED_10BIT_Y0L2,
                                               ASIC_INPUT_YUV420_TILE32,
                                               ASIC_INPUT_YUV420_TILE16_PCK4,
                                               ASIC_INPUT_YUV420_TILE4,
                                               ASIC_INPUT_YUV420_TILE4,
                                               ASIC_INPUT_P010_TILE4,
                                               ASIC_INPUT_SP_101010,
                                               ASIC_INPUT_YUV422_888,
                                               ASIC_INPUT_YUV420_TILE64,
                                               ASIC_INPUT_YUV420_TILE64,
                                               ASIC_INPUT_YUV420PCK16_TILE,
                                               ASIC_INPUT_YUV420PCK10_TILE,
                                               ASIC_INPUT_YUV420PCK10_TILE,
                                               ASIC_INPUT_YUV420_TILE128,
                                               ASIC_INPUT_YUV420_TILE128,
                                               ASIC_INPUT_YUV420PCK10_TILE96,
                                               ASIC_INPUT_YUV420PCK10_TILE96,
                                               ASIC_INPUT_YUV420SP_TILE8,
                                               ASIC_INPUT_P010_TILE8,
                                               ASIC_INPUT_YUV420PLANAR,
                                               ASIC_INPUT_YUV420_UV_TILE64x2,
                                               ASIC_INPUT_YUV420_10BIT_UV_TILE128x2,
                                               ASIC_INPUT_RGB888_24BIT,
                                               ASIC_INPUT_RGB888_24BIT,
                                               ASIC_INPUT_RGB888_24BIT,
                                               ASIC_INPUT_RGB888_24BIT,
                                               ASIC_INPUT_RGB888_24BIT,
                                               ASIC_INPUT_RGB888_24BIT};

    return inputFormatMapping[inputFormat];
}

static const u32 rgbMaskBits[46]
                            [3] = {
                                {0, 0, 0},   {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0},
                                {15, 10, 4}, /* RGB565 */
                                {4, 10, 15}, /* BGR565 */
                                {14, 9, 4},  /* RGB565 */
                                {4, 9, 14},  /* BGR565 */
                                {11, 7, 3},  /* RGB444 */
                                {3, 7, 11},  /* BGR444 */
                                {23, 15, 7}, /* RGB888 */
                                {7, 15, 23}, /* BGR888 */
                                {29, 19, 9}, /* RGB101010 */
                                {9, 19, 29}, /* BGR101010 */
                                {0, 0, 0},   {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0},
                                {0, 0, 0},   {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0},
                                {0, 0, 0},   {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0},
                                {0, 0, 0},   {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0},
                                {0, 0, 0},   {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0},
                                {7, 15, 23}, /* RGB888 24bit,  VCENC_RGB888_24BIT */
                                {23, 15, 7}, /* BGR888 24bit,  VCENC_BGR888_24BIT */
                                {7, 23, 15}, /* RBG888 24bit,  VCENC_RBG888_24BIT */
                                {23, 7, 15}, /* GBR888 24bit,  VCENC_GBR888_24BIT */
                                {15, 23, 7}, /* BRG888 24bit,  VCENC_BRG888_24BIT */
                                {15, 7, 23}  /* GRB888 24bit,  VCENC_GRB888_24BIT */

};

/*------------------------------------------------------------------------------

  EncPreProcessCheck

  Check image size: Cropped frame _must_ fit inside of source image

  Input preProcess Pointer to preProcess_s structure.

  Return  ENCHW_OK  No errors.
    ENCHW_NOK Error condition.

------------------------------------------------------------------------------*/
i32 EncPreProcessCheck(const preProcess_s *preProcess, i32 tileColumnNum)
{
    i32 status = ENCHW_OK;
    u32 tmp;
    u32 width, height;
    u32 tileId;

#if 0 /* These limits apply for input stride but camstab needs the
  actual pixels without padding. */
  u32 w_mask;

  /* Check width limits: lum&ch strides must be full 64-bit addresses */
  if (preProcess->inputFormat == 0)       /* YUV 420 planar */
    w_mask = 0x0F;                      /* 16 multiple */
  else if (preProcess->inputFormat <= 1)  /* YUV 420 semiplanar */
    w_mask = 0x07;                      /* 8 multiple  */
  else if (preProcess->inputFormat <= 9)  /* YUYV 422 or 16-bit RGB */
    w_mask = 0x03;                      /* 4 multiple  */
  else                                    /* 32-bit RGB */
    w_mask = 0x01;                      /* 2 multiple  */

  if (preProcess->lumWidthSrc & w_mask)
  {
    status = ENCHW_NOK;
  }
#endif
    for (tileId = 0; tileId < tileColumnNum; tileId++) {
        if (preProcess->lumHeightSrc[tileId] & 0x01) {
            status = ENCHW_NOK;
        }

        if (preProcess->lumWidthSrc[tileId] > MAX_INPUT_IMAGE_WIDTH) {
            status = ENCHW_NOK;
        }
    }

    width = preProcess->lumWidth;
    height = preProcess->lumHeight;
    if (preProcess->rotation && preProcess->rotation != 3) {
        u32 tmp;

        tmp = width;
        width = height;
        height = tmp;
    }

    if (tileColumnNum > 1 && preProcess->rotation)
        status = ENCHW_NOK;

    tmp = MAX(preProcess->horOffsetSrc[0] + width, preProcess->horOffsetSrc[0]);
    if (tmp > preProcess->lumWidthSrc[0]) {
        status = ENCHW_NOK;
    }

    tmp = MAX(preProcess->verOffsetSrc[0] + height, preProcess->verOffsetSrc[0]);
    if (tmp > preProcess->lumHeightSrc[0]) {
        status = ENCHW_NOK;
    }

    return status;
}

u32 EncGetAlignedByteStride(int width, i32 input_format, u32 *luma_stride, u32 *chroma_stride,
                            u32 input_alignment)
{
    u32 alignment = input_alignment == 0 ? 1 : input_alignment;
    u32 pixelByte = 1;

    if (luma_stride == NULL || chroma_stride == NULL)
        return 1;

    switch (input_format) {
    case 0:  //VCENC_YUV420_PLANAR
    case 37: //VCENC_YVU420_PLANAR
        *luma_stride = STRIDE(width, alignment);
        *chroma_stride = STRIDE(width / 2, alignment);
        break;
    case 1: //VCENC_YUV420_SEMIPLANAR
    case 2: //VCENC_YUV420_SEMIPLANAR_VU
        *luma_stride = STRIDE(width, alignment);
        *chroma_stride = STRIDE(width, alignment);
        break;
    case 3: //VCENC_YUV422_INTERLEAVED_YUYV
    case 4: //VCENC_YUV422_INTERLEAVED_UYVY
        *luma_stride = STRIDE(width * 2, alignment);
        *chroma_stride = 0;
        pixelByte = 2;
        break;
    case 5:  //VCENC_RGB565
    case 6:  //VCENC_BGR565
    case 7:  //VCENC_RGB555
    case 8:  //VCENC_BGR555
    case 9:  //VCENC_RGB444
    case 10: //VCENC_BGR444
        *luma_stride = STRIDE(width * 2, alignment);
        *chroma_stride = 0;
        pixelByte = 2;
        break;
    case 11: //VCENC_RGB888
    case 12: //VCENC_BGR888
    case 13: //VCENC_RGB101010
    case 14: //VCENC_BGR101010
        *luma_stride = STRIDE(width * 4, alignment);
        *chroma_stride = 0;
        pixelByte = 4;
        break;
    case 15: //VCENC_YUV420_PLANAR_10BIT_I010
        *luma_stride = STRIDE(width * 2, alignment);
        *chroma_stride = STRIDE(width / 2 * 2, alignment);
        pixelByte = 2;
        break;
    case 16: //VCENC_YUV420_PLANAR_10BIT_P010
        *luma_stride = STRIDE(width * 2, alignment);
        *chroma_stride = STRIDE(width * 2, alignment);
        pixelByte = 2;
        break;
    case 17: //VCENC_YUV420_PLANAR_10BIT_PACKED_PLANAR
        *luma_stride = STRIDE(width, 64);
        *chroma_stride = STRIDE(width, 64) / 2;
        break;
    case 18: //VCENC_YUV420_10BIT_PACKED_Y0L2
        *luma_stride = STRIDE(width, 4);
        *chroma_stride = 0;
        break;
    case 19: //VCENC_YUV420_PLANAR_8BIT_TILE_32_32
        *luma_stride = STRIDE(width, 32);
        *chroma_stride = STRIDE(width, 32) / 2;
        break;
    case 20: //VCENC_YUV420_PLANAR_8BIT_TILE_16_16_PACKED_4
        *luma_stride = STRIDE(width, 16);
        *chroma_stride = 0;
        break;
    case 21: //VCENC_YUV420_SEMIPLANAR_8BIT_TILE_4_4
    case 22: //VCENC_YUV420_SEMIPLANAR_VU_8BIT_TILE_4_4
        *luma_stride = STRIDE(width * 4, alignment);
        *chroma_stride = STRIDE(width * 4, alignment);
        break;
    case 23: //VCENC_YUV420_PLANAR_10BIT_P010_TILE_4_4
        *luma_stride = STRIDE(width * 4 * 2, alignment);
        *chroma_stride = STRIDE(width * 4 * 2, alignment);
        pixelByte = 2;
        break;
    case 24: //VCENC_YUV420_SEMIPLANAR_101010
        *luma_stride = STRIDE((width + 2) / 3 * 4, alignment);
        *chroma_stride = STRIDE((width + 2) / 3 * 4, alignment);
        break;
    case 25: //JPEGENC_YUV422_888
        *luma_stride = STRIDE(width, alignment);
        *chroma_stride = STRIDE(width, alignment);
        break;
    case 26: //VCENC_YUV420_8BIT_TILE_64_4
    case 27: //VCENC_YUV420_UV_8BIT_TILE_64_4
        *luma_stride = STRIDE(STRIDE(width, 64) * 4, alignment);
        *chroma_stride = STRIDE(STRIDE(width, 64) * 4, alignment);
        break;
    case 28: //VCENC_YUV420_10BIT_TILE_32_4
        *luma_stride = STRIDE(STRIDE(width, 32) * 2 * 4, alignment);
        *chroma_stride = STRIDE(STRIDE(width, 32) * 2 * 4, alignment);
        pixelByte = 2;
        break;
    case 29: //VCENC_YUV420_10BIT_TILE_48_4
    case 30: //VCENC_YUV420_VU_10BIT_TILE_48_4
        *luma_stride = STRIDE((width + 47) / 48 * 48 / 3 * 4 * 4, alignment);
        *chroma_stride = STRIDE((width + 47) / 48 * 48 / 3 * 4 * 4, alignment);
        break;
    case 31: //VCENC_YUV420_8BIT_TILE_128_2
    case 32: //VCENC_YUV420_UV_8BIT_TILE_128_2
        *luma_stride = STRIDE(STRIDE(width, 128) * 2, alignment);
        *chroma_stride = STRIDE(STRIDE(width, 128) * 2, alignment);
        break;
    case 33: //VCENC_YUV420_10BIT_TILE_96_2
    case 34: //VCENC_YUV420_VU_10BIT_TILE_96_2
        *luma_stride = STRIDE((width + 95) / 96 * 96 / 3 * 4 * 2, alignment);
        *chroma_stride = STRIDE((width + 95) / 96 * 96 / 3 * 4 * 2, alignment);
        break;
    case 35: //VCENC_YUV420_8BIT_TILE_8_8
        *luma_stride = STRIDE(STRIDE(width, 8) * 8, alignment);
        *chroma_stride = STRIDE(STRIDE(width, 16) * 4, alignment);
        break;
    case 36: //VCENC_YUV420_10BIT_TILE_8_8
        *luma_stride = STRIDE(STRIDE(width, 8) * 8 * 2, alignment);
        *chroma_stride = STRIDE(STRIDE(width, 16) * 4 * 2, alignment);
        break;
    case 38: //VCENC_YUV420_UV_8BIT_TILE_64_2
        *luma_stride = STRIDE(STRIDE(width, 64) * 2, alignment);
        *chroma_stride = STRIDE(STRIDE(width, 64) * 2, alignment);
        break;
    case 39: //VCENC_YUV420_UV_10BIT_TILE_128_2
        *luma_stride = STRIDE(STRIDE(width, 128) * 2 * 2, alignment);
        *chroma_stride = STRIDE(STRIDE(width, 128) * 2 * 2, alignment);
        pixelByte = 2;
        break;
    case 40: //VCENC_RGB888_24BIT
    case 41: //VCENC_BGR888_24BIT
    case 42: //VCENC_RBG888_24BIT
    case 43: //VCENC_GBR888_24BIT
    case 44: //VCENC_BRG888_24BIT
    case 45: //VCENC_GRB888_24BIT
        *luma_stride = STRIDE(width * 3, alignment);
        *chroma_stride = 0;
        break;
    default:
        *luma_stride = 0;
        *chroma_stride = 0;
        break;
    }

    return pixelByte;
}

/*------------------------------------------------------------------------------

  EncPreProcess

  Preform cropping

  Input asic  Pointer to asicData_s structure
    preProcess Pointer to preProcess_s structure.

------------------------------------------------------------------------------*/
void EncPreProcess(asicData_s *asic, preProcess_s *preProcess, void *ctx, u32 tileId)
{
    u32 tmp, i;
    u32 width, height;
    regValues_s *regs;
    u32 luma_stride, chroma_stride, pixelByte;
    u32 horOffsetSrc = preProcess->horOffsetSrc[tileId];
    u32 alignment = preProcess->input_alignment;
    u32 height64;
    u32 client_type;

    ASSERT(asic != NULL && preProcess != NULL);

    regs = &asic->regs;

    pixelByte = EncGetAlignedByteStride(preProcess->lumWidthSrc[tileId], preProcess->inputFormat,
                                        &luma_stride, &chroma_stride, alignment);
    luma_stride /= pixelByte;
    chroma_stride /= pixelByte;

    regs->input_luma_stride = luma_stride;
    regs->input_chroma_stride = chroma_stride;

    regs->pixelsOnRow = luma_stride;
    height64 = ((preProcess->lumHeight + 63) & ~63);
    regs->dummyReadEnable = 0;

    /* cropping */
    if (preProcess->inputFormat <= 2 ||
        preProcess->inputFormat == VCENC_YVU420_PLANAR) /* YUV 420 planar/semiplanar */
    {
        /* Input image position after crop and stabilization */
        tmp = preProcess->verOffsetSrc[tileId];
        tmp *= luma_stride;
        tmp += horOffsetSrc;
        regs->inputLumBase += (tmp & (~15));
        regs->inputLumaBaseOffset = tmp & 15;

        if (preProcess->videoStab)
            regs->stabNextLumaBase += (tmp & (~15));

        /* Chroma */
        if (preProcess->inputFormat == VCENC_YUV420_PLANAR ||
            preProcess->inputFormat == VCENC_YVU420_PLANAR) {
            tmp = preProcess->verOffsetSrc[tileId] / 2;
            tmp *= chroma_stride;
            tmp += horOffsetSrc / 2;
            if (((chroma_stride) % 16) == 0) {
                regs->inputCbBase += (tmp & (~15));
                regs->inputCrBase += (tmp & (~15));
                regs->inputChromaBaseOffset = tmp & 15;
            } else {
                regs->inputCbBase += (tmp & (~7));
                regs->inputCrBase += (tmp & (~7));
                regs->inputChromaBaseOffset = tmp & 7;
            }
            regs->dummyReadAddr = regs->inputCrBase + regs->inputChromaBaseOffset +
                                  height64 / 2 * chroma_stride + preProcess->lumWidth / 2 - 1;
        } else {
            tmp = preProcess->verOffsetSrc[tileId] / 2;
            tmp *= chroma_stride / 2;
            tmp += horOffsetSrc / 2;
            tmp *= 2;

            regs->inputCbBase += (tmp & (~15));
            regs->inputChromaBaseOffset = tmp & 15;
            regs->dummyReadAddr = regs->inputCbBase + regs->inputChromaBaseOffset +
                                  height64 / 2 * chroma_stride + preProcess->lumWidth - 1;
        }
    } else if (preProcess->inputFormat <= 10) /* YUV 422 / RGB 16bpp */
    {
        /* Input image position after crop and stabilization */
        tmp = preProcess->verOffsetSrc[tileId];
        tmp *= luma_stride;
        regs->inputImageFormat = EncPreGetHwFormat(preProcess->inputFormat);
        if (regs->inputImageFormat == ASIC_INPUT_YUYV422INTERLEAVED ||
            regs->inputImageFormat == ASIC_INPUT_UYVY422INTERLEAVED)
            horOffsetSrc = (horOffsetSrc / 2) * 2;
        tmp += horOffsetSrc;
        tmp *= 2;

        regs->inputLumBase += (tmp & (~15));
        regs->inputLumaBaseOffset = tmp & 15;
        regs->inputChromaBaseOffset = (regs->inputLumaBaseOffset / 4) * 4;
        regs->dummyReadAddr = regs->inputLumBase + regs->inputLumaBaseOffset +
                              height64 * luma_stride * 2 + preProcess->lumWidth * 2 - 1;

        if (preProcess->videoStab)
            regs->stabNextLumaBase += (tmp & (~15));
    } else if (preProcess->inputFormat <= 14) /* RGB 32bpp */
    {
        /* Input image position after crop and stabilization */
        tmp = preProcess->verOffsetSrc[tileId];
        tmp *= luma_stride;
        regs->inputImageFormat = EncPreGetHwFormat(preProcess->inputFormat);
        if (regs->inputImageFormat == ASIC_INPUT_YUYV422INTERLEAVED ||
            regs->inputImageFormat == ASIC_INPUT_UYVY422INTERLEAVED)
            horOffsetSrc = (horOffsetSrc / 2) * 2;
        tmp += horOffsetSrc;
        tmp *= 4;

        regs->inputLumBase += (tmp & (~15));
        /* Note: HW does the cropping AFTER RGB to YUYV conversion
     * so the offset is calculated using 16bpp */
        regs->inputLumaBaseOffset = (tmp & 15);
        regs->inputChromaBaseOffset = (regs->inputLumaBaseOffset / 4) * 4;
        regs->dummyReadAddr = regs->inputLumBase + regs->inputLumaBaseOffset +
                              height64 * luma_stride + preProcess->lumWidth * 4 - 1;

        if (preProcess->videoStab)
            regs->stabNextLumaBase += (tmp & (~15));
    } else if (preProcess->inputFormat == 15) //I010
    {
        /* Input image position after crop and stabilization */
        tmp = preProcess->verOffsetSrc[tileId];
        tmp *= luma_stride;
        tmp += horOffsetSrc;
        tmp *= 2;
        regs->inputLumBase += (tmp & (~15));
        regs->inputLumaBaseOffset = tmp & 15;

        /* Chroma */
        {
            tmp = preProcess->verOffsetSrc[tileId] / 2;
            tmp *= chroma_stride;
            tmp += horOffsetSrc / 2;
            tmp *= 2;
            if (((chroma_stride * 2) % 16) == 0) {
                regs->inputCbBase += (tmp & (~15));
                regs->inputCrBase += (tmp & (~15));
                regs->inputChromaBaseOffset = tmp & 15;
            } else {
                regs->inputCbBase += (tmp & (~7));
                regs->inputCrBase += (tmp & (~7));
                regs->inputChromaBaseOffset = tmp & 7;
            }
        }
        regs->dummyReadAddr = regs->inputCrBase + regs->inputChromaBaseOffset +
                              height64 / 2 * chroma_stride * 2 + preProcess->lumWidth - 1;
    } else if (preProcess->inputFormat == 16) //P010
    {
        /* Input image position after crop and stabilization */
        tmp = preProcess->verOffsetSrc[tileId];
        tmp *= luma_stride;
        tmp += horOffsetSrc;
        tmp *= 2;
        regs->inputLumBase += (tmp & (~15));
        regs->inputLumaBaseOffset = tmp & 15;

        /* Chroma */
        {
            tmp = preProcess->verOffsetSrc[tileId] / 2;
            tmp *= chroma_stride / 2;
            tmp += horOffsetSrc / 2;
            tmp *= 2 * 2;

            if (((luma_stride) % 16) == 0) {
                regs->inputCbBase += (tmp & (~15));
                regs->inputChromaBaseOffset = tmp & 15;
            } else {
                regs->inputCbBase += (tmp & (~7));
                regs->inputChromaBaseOffset = tmp & 7;
            }
        }
        regs->dummyReadAddr = regs->inputCbBase + regs->inputChromaBaseOffset +
                              (height64 / 2 * luma_stride / 2 + preProcess->lumWidth / 2) * 4 - 1;
    } else if (preProcess->inputFormat == 17) {
        /* Input image position after crop and stabilization */
        tmp = preProcess->verOffsetSrc[tileId];
        tmp *= luma_stride * 10 / 8;
        tmp += horOffsetSrc * 10 / 8;
        regs->inputLumBase += (tmp & (~15));
        regs->inputLumaBaseOffset = tmp & 15;

        /* Chroma */
        {
            tmp = preProcess->verOffsetSrc[tileId] / 2;
            tmp *= luma_stride * 5 / 8;
            tmp += horOffsetSrc * 5 / 8;
            if (((luma_stride * 5 / 8) % 16) == 0) {
                regs->inputCbBase += (tmp & (~15));
                regs->inputCrBase += (tmp & (~15));
                regs->inputChromaBaseOffset = tmp & 15;
            } else {
                regs->inputCbBase += (tmp & (~7));
                regs->inputCrBase += (tmp & (~7));
                regs->inputChromaBaseOffset = tmp & 7;
            }
        }
        regs->dummyReadAddr = regs->inputCrBase + regs->inputChromaBaseOffset +
                              height64 / 2 * luma_stride * 5 / 8 + (preProcess->lumWidth) * 5 / 8 -
                              1;
    } else if (preProcess->inputFormat == 18) {
        /* Input image position after crop and stabilization */
        tmp = preProcess->verOffsetSrc[tileId] / 2;
        tmp *= luma_stride / 2;
        tmp *= 8;
        tmp += horOffsetSrc / 2 * 8;
        regs->inputLumBase += (tmp & (~15));
        regs->inputLumaBaseOffset = tmp & 15;
    } else if (preProcess->inputFormat == 19) /* VCENC_YUV420_PLANAR_8BIT_TILE_32_32 */
    {
        /* Input image position after crop and stabilization */
        /*crop alignment for 32x32*/
        tmp = preProcess->verOffsetSrc[tileId];
        tmp *= luma_stride;
        tmp += horOffsetSrc * 32;
        regs->inputLumBase += (tmp & (~15));
        regs->inputLumaBaseOffset = tmp & 15;

        /* Chroma */
        tmp = preProcess->verOffsetSrc[tileId] / 2;
        tmp *= luma_stride / 2;
        tmp += (horOffsetSrc / 2) * 16;
        tmp *= 2;

        regs->inputCbBase += (tmp & (~15));
        regs->inputChromaBaseOffset = tmp & 15;
    } else if (preProcess->inputFormat == 20) /* VCENC_YUV420_PLANAR_8BIT_TILE_16_16_PACKED_4 */
    {
        /* Input image position after crop and stabilization */
        /*crop alignment for 32x32*/
        u32 mb_per_row = preProcess->lumWidthSrc[tileId] / 16;
        u32 mb_cropped = mb_per_row * preProcess->verOffsetSrc[tileId] / 16 + horOffsetSrc / 16;

        tmp = mb_cropped / 5 * 2048 + mb_cropped % 5 * 400;

        if (preProcess->sliced_frame) //jpeg
        {
            u32 slice_cropped = mb_per_row * preProcess->verOffsetSrc[tileId] / 16;
            u32 slice_cropped_size = slice_cropped / 5 * 2048 + slice_cropped % 5 * 400;
            tmp = tmp - slice_cropped_size;
        }

        regs->inputLumBase += (tmp & (~15));
        regs->inputLumaBaseOffset = tmp & 15;
        regs->inputCbBase = horOffsetSrc / 16;
        regs->inputCrBase = preProcess->verOffsetSrc[tileId] / 16;

        if (preProcess->sliced_frame) //jpeg
        {
            preProcess->verOffsetSrc[tileId] = 0;
        }
    } else if (preProcess->inputFormat == 21 || preProcess->inputFormat == 22 ||
               preProcess->inputFormat == 23) {
        regs->dummyReadAddr = regs->inputCbBase +
                              (preProcess->verOffsetSrc[tileId] + height64) / 8 * luma_stride +
                              ((preProcess->horOffsetSrc[tileId] + preProcess->lumWidth) * 4 - 1) *
                                  (1 + (preProcess->inputFormat == 23));
    } else if (preProcess->inputFormat == 24) /* P101010*/
    {
        /* Input image position after crop and stabilization */
        tmp = preProcess->verOffsetSrc[tileId];
        tmp *= luma_stride;
        tmp += horOffsetSrc / 3 * 4;
        regs->inputLumBase += (tmp & (~15));
        regs->inputLumaBaseOffset = tmp & 15;

        /* Chroma */
        tmp = preProcess->verOffsetSrc[tileId] / 2;
        tmp *= chroma_stride;
        tmp += horOffsetSrc / 3 * 4;
        regs->inputCbBase += (tmp & (~15));
        regs->inputChromaBaseOffset = tmp & 15;
    } else if (preProcess->inputFormat == 25) /* 422_888*/
    {
        /* Input image position after crop and stabilization */
        tmp = preProcess->verOffsetSrc[tileId];
        tmp *= luma_stride;
        tmp += horOffsetSrc;
        regs->inputLumBase += (tmp & (~15));
        regs->inputLumaBaseOffset = tmp & 15;

        /* Chroma */
        regs->inputCbBase += (tmp & (~15));
        regs->inputChromaBaseOffset = tmp & 15;
    } else if (preProcess->inputFormat >= 26 && preProcess->inputFormat <= 36) /* 420_tile*/
    {
        regs->input_luma_stride = luma_stride;
        regs->input_chroma_stride = chroma_stride;
    } else if (preProcess->inputFormat == 38) /* VCENC_YUV420_UV_8BIT_TILE_64_2 */
    {
        if (preProcess->verOffsetSrc[tileId] % 4 != 0 || preProcess->horOffsetSrc[tileId] % 64 != 0)
            printf("FBC cropping need to be aligned to 64x4 tile");
        else {
            tmp = preProcess->verOffsetSrc[tileId] / 2;
            tmp *= luma_stride;
            tmp += horOffsetSrc * 2;

            regs->inputLumBase += (tmp & (~15));
            regs->inputLumaBaseOffset = tmp & 15;

            /* Chroma */
            tmp = preProcess->verOffsetSrc[tileId] / 4;
            tmp *= chroma_stride;
            tmp += horOffsetSrc * 2;
            regs->inputCbBase += (tmp & (~15));
            regs->inputChromaBaseOffset = tmp & 15;
        }
    } else if ((preProcess->inputFormat <= 40) &&
               (preProcess->inputFormat <=
                45)) /* RGB 24bpp , VCENC_RGB888_24BIT ~ VCENC_GRB888_24BIT*/
    {
        /* Input image position after crop and stabilization */
        tmp = preProcess->verOffsetSrc[tileId];
        tmp *= luma_stride;
        regs->inputImageFormat = EncPreGetHwFormat(preProcess->inputFormat);
        tmp += horOffsetSrc * 3;

        regs->inputLumBase += (tmp & (~15));
        regs->inputLumaBaseOffset = (tmp & 15);
        regs->inputChromaBaseOffset = (regs->inputLumaBaseOffset / 4) * 4;
        regs->dummyReadAddr = regs->inputLumBase + regs->inputLumaBaseOffset +
                              height64 * luma_stride + preProcess->lumWidth * 3 - 1;

        if (preProcess->videoStab)
            regs->stabNextLumaBase += (tmp & (~15));
    }

    if (!regs->dummyReadEnable)
        regs->dummyReadAddr = 0;

    /* YUV subsampling, map API values into HW reg values */
    regs->inputImageFormat = EncPreGetHwFormat(preProcess->inputFormat);

    if (preProcess->inputFormat == 2 || preProcess->inputFormat == 22 ||
        preProcess->inputFormat == 26 || preProcess->inputFormat == 30 ||
        preProcess->inputFormat == 31 || preProcess->inputFormat == 34)
        regs->chromaSwap = 1;

    regs->inputImageRotation = preProcess->rotation;
    regs->inputImageMirror = preProcess->mirror;

    /* source image setup, size and fill */
    width = preProcess->lumWidth;
    height = preProcess->lumHeight;
    regs->scaledVertivalWeightEn = 1;
    regs->scaledHorizontalCopy = 0;
    regs->scaledVerticalCopy = 0;
    client_type =
        IS_H264(preProcess->codecFormat) ? EWL_CLIENT_TYPE_H264_ENC : EWL_CLIENT_TYPE_HEVC_ENC;
    EWLHwConfig_t config = EncAsicGetAsicConfig(client_type, ctx);
    /* Scaling ratio for down-scaling, fixed point 1.16, calculate from
      rotated dimensions. */
    if (preProcess->scaledWidth * preProcess->scaledHeight > 0 && preProcess->scaledOutput) {
        i32 width8 = 0;
        i32 height8 = 0;

        u32 width = preProcess->lumWidth * (preProcess->inLoopDSRatio + 1);
        u32 height = preProcess->lumHeight * (preProcess->inLoopDSRatio + 1);

        if ((regs->codingType == ASIC_JPEG) || (regs->codingType == ASIC_H264)) //aligned to 16
        {
            width8 = (width + 15) / 16 * 16;
            height8 = (height + 15) / 16 * 16;
        } else {
            width8 = (width + 7) / 8 * 8;
            height8 = (height + 7) / 8 * 8;
        }

        regs->scaledOutputFormat = preProcess->scaledOutputFormat;
        regs->scaledWidth = preProcess->scaledWidth;
        regs->scaledHeight = preProcess->scaledHeight;
        regs->scaledSkipLeftPixelColumn = 0;
        regs->scaledSkipTopPixelRow = 0;
        regs->scaledWidthRatio = (u32)(preProcess->scaledWidth << 16) / width8 + 1;
        regs->scaledHeightRatio = (u32)(preProcess->scaledHeight << 16) / height8 + 1;
        regs->scaledHeightRatio = MIN(65535, regs->scaledHeightRatio);
        regs->scaledWidthRatio = MIN(65535, regs->scaledWidthRatio);

        if (config.scaled420Support == 1) {
            if (width8 == preProcess->scaledWidth) {
                regs->scaledHorizontalCopy = 1;
            }
            if (height8 == preProcess->scaledHeight) {
                regs->scaledVerticalCopy = 1;
            }
        } else {
            if (width == preProcess->scaledWidth) {
                regs->scaledHorizontalCopy = 1;
            }
            if (height == preProcess->scaledHeight) {
                regs->scaledVerticalCopy = 1;
            }

            if (preProcess->rotation == 2) {
                regs->scaledSkipTopPixelRow =
                    (((height8 * regs->scaledHeightRatio) >> 16) - preProcess->scaledHeight) / 2;

                if (preProcess->scaledHeight == height) {
                    regs->scaledSkipTopPixelRow = (8 - (height & 0x07)) / 2;
                    if (!(height & 0x07))
                        regs->scaledSkipTopPixelRow = 0;
                }
            }
            if (preProcess->rotation == 1) {
                regs->scaledSkipLeftPixelColumn =
                    (((width8 * regs->scaledWidthRatio) >> 16) - preProcess->scaledWidth) / 2;
                if (preProcess->scaledWidth == width) {
                    regs->scaledSkipLeftPixelColumn = (8 - (width & 0x07)) / 2;
                    if (!(width & 0x07))
                        regs->scaledSkipLeftPixelColumn = 0;
                }
            }

            if (preProcess->rotation == 3) {
                regs->scaledSkipLeftPixelColumn =
                    (((width8 * regs->scaledWidthRatio) >> 16) - preProcess->scaledWidth) / 2;
                if (preProcess->scaledWidth == width) {
                    regs->scaledSkipLeftPixelColumn = (8 - (width & 0x07)) / 2;
                    if (!(width & 0x07))
                        regs->scaledSkipLeftPixelColumn = 0;
                }
                regs->scaledSkipTopPixelRow =
                    (((height8 * regs->scaledHeightRatio) >> 16) - preProcess->scaledHeight) / 2;

                if (preProcess->scaledHeight == height) {
                    regs->scaledSkipTopPixelRow = (8 - (height & 0x07)) / 2;
                    if (!(height & 0x07))
                        regs->scaledSkipTopPixelRow = 0;
                }
            }
        }
    } else {
        regs->scaledWidth = regs->scaledHeight = 0;
        regs->scaledWidthRatio = regs->scaledHeightRatio = 0;
    }

    /* For rotated image, swap dimensions back to normal. */
    if (preProcess->rotation && preProcess->rotation != 3) {
        u32 tmp;

        tmp = width;
        width = height;
        height = tmp;
    }

    /* Set mandatory input parameters in asic structure */
    //regs->picWidth = (width + 7) / 8;
    //regs->picHeight = (height + 7) / 8;
    //asic->regs.picWidth = pic->sps->width_min_cbs;
    //asic->regs.picHeight = pic->sps->height_min_cbs;
    u32 mcuh =
        (regs->codingType == ASIC_JPEG && regs->jpegMode == 1 && regs->ljpegFmt == 1 ? 8 : 16);
    regs->mbsInRow = (width + 15) / 16;
    regs->mbsInCol = (height + mcuh - 1) / mcuh;

    /* Set the overfill values */
    if (width & 0x07)
        regs->xFill = (8 - (width & 0x07)) / 2;
    else
        regs->xFill = 0;

    if (height & 0x07)
        regs->yFill = 8 - (height & 0x07);
    else
        regs->yFill = 0;

    if ((regs->codingType == ASIC_JPEG) || (regs->codingType == ASIC_H264)) {
        if (width & 0x0f) {
            //TODO: H1 prp is doing /4 crop for both JPEG/H.264
            if (regs->codingType == ASIC_JPEG)
                regs->xFill = (16 - (width & 0x0f)) / 2;
            else if (regs->codingType == ASIC_H264)
                regs->xFill = (16 - (width & 0x0f)) / 2;
        } else
            regs->xFill = 0;
    }

    if ((regs->codingType == ASIC_JPEG && !(regs->jpegMode == 1 && regs->ljpegFmt == 1)) ||
        (regs->codingType == ASIC_H264)) {
        if (height & 0x0f)
            regs->yFill = (16 - (height & 0x0f));
        else
            regs->yFill = 0;
    }

    if (preProcess->inputFormat == 20 || preProcess->inputFormat == 19) {
        regs->xFill = 0;
        regs->yFill = 0;
    }

    regs->constChromaEn = preProcess->constChromaEn;
    regs->constCb = preProcess->constCb;
    regs->constCr = preProcess->constCr;

    /* video stabilization */
    if (regs->codingType != ASIC_JPEG && preProcess->videoStab != 0)
        regs->stabMode = 2; /* stab + encode */
    else
        regs->stabMode = 0;

    //Perform OSD cropping
    for (i = 0; i < MAX_OVERLAY_NUM; i++) {
        if (regs->overlayEnable[i]) {
            switch (regs->overlayFormat[i]) {
            case 0: //ARGB
                if (regs->overlaySuperTile) {
                    /* For super tile, offset has to be 64 aligned */
                    tmp = STRIDE(preProcess->overlayVerOffset[i], 64) / 64;
                    tmp *= preProcess->overlayYStride[i];
                    tmp += ((preProcess->overlayCropXoffset[i] / 64) * (64 * 64) +
                            preProcess->overlayCropXoffset[i] % 64) *
                           4;
                } else {
                    tmp = preProcess->overlayVerOffset[i];
                    tmp *= preProcess->overlayYStride[i];
                    tmp += preProcess->overlayCropXoffset[i] * 4;
                }
                regs->overlayYAddr[i] += tmp;
                regs->overlayWidth[i] = preProcess->overlayCropWidth[i];
                regs->overlayHeight[i] = preProcess->overlaySliceHeight[i];
                break;
            case 1: //NV12
                //Luma
                tmp = preProcess->overlayVerOffset[i];
                tmp *= preProcess->overlayYStride[i];
                tmp += preProcess->overlayCropXoffset[i];
                regs->overlayYAddr[i] += tmp;
                regs->overlayWidth[i] = preProcess->overlayCropWidth[i];
                regs->overlayHeight[i] = preProcess->overlaySliceHeight[i];

                //Chroma
                tmp = preProcess->overlayVerOffset[i] / 2;
                tmp *= preProcess->overlayUVStride[i];
                tmp += preProcess->overlayCropXoffset[i];
                regs->overlayUAddr[i] += tmp;
                break;
            case 2: //Bitmap
                tmp = preProcess->overlayVerOffset[i];
                tmp *= preProcess->overlayYStride[i];
                tmp += preProcess->overlayCropXoffset[i] / 8;
                regs->overlayYAddr[i] += tmp;
                regs->overlayWidth[i] = preProcess->overlayCropWidth[i];
                regs->overlayHeight[i] = preProcess->overlaySliceHeight[i];
                break;
            default:
                break;
            }
        }
    }

#ifdef TRACE_PREPROCESS
    EncTracePreProcess(preProcess);
#endif

    return;
}

/*------------------------------------------------------------------------------

  EncSetColorConversion

  Set color conversion coefficients and RGB input mask

  Input asic  Pointer to asicData_s structure
    preProcess Pointer to preProcess_s structure.

------------------------------------------------------------------------------*/
void EncSetColorConversion(preProcess_s *preProcess, asicData_s *asic)
{
    regValues_s *regs;

    ASSERT(asic != NULL && preProcess != NULL);

    regs = &asic->regs;
    regs->colorConversionLumaOffset = 0;

    /* Y  = A R + B G + C B + offset
       * Cb = E B - G Y + 128
       * Cr = F R - H Y + 128
       */
    switch (preProcess->colorConversionType) {
    case 0: /* BT.601 */
    default:
        /* Y  = 0.2989 R + 0.5866 G + 0.1145 B
       * Cb = 0.5647 B - 0.5647 Y + 128
       * Cr = 0.7132 R - 0.7132 Y + 128
       */
        preProcess->colorConversionType = 0;
        regs->colorConversionCoeffA = preProcess->colorConversionCoeffA = 19589;
        regs->colorConversionCoeffB = preProcess->colorConversionCoeffB = 38443;
        regs->colorConversionCoeffC = preProcess->colorConversionCoeffC = 7504;
        regs->colorConversionCoeffE = preProcess->colorConversionCoeffE = 37008;
        regs->colorConversionCoeffF = preProcess->colorConversionCoeffF = 46740;
        regs->colorConversionCoeffG = preProcess->colorConversionCoeffG = 37008;
        regs->colorConversionCoeffH = preProcess->colorConversionCoeffH = 46740;
        break;

    case 1: /* BT.709 */
        /* Y  = 0.2126 R + 0.7152 G + 0.0722 B
       * Cb = 0.5389 B - 0.5389 Y + 128
       * Cr = 0.6350 R - 0.6350 Y + 128
       */
        regs->colorConversionCoeffA = preProcess->colorConversionCoeffA = 13933;
        regs->colorConversionCoeffB = preProcess->colorConversionCoeffB = 46871;
        regs->colorConversionCoeffC = preProcess->colorConversionCoeffC = 4732;
        regs->colorConversionCoeffE = preProcess->colorConversionCoeffE = 35317;
        regs->colorConversionCoeffF = preProcess->colorConversionCoeffF = 41615;
        regs->colorConversionCoeffG = preProcess->colorConversionCoeffG = 35317;
        regs->colorConversionCoeffH = preProcess->colorConversionCoeffH = 41615;
        break;

    case 2: /* User defined */
        /* Limitations for coefficients: A+B+C <= 65536 */
        regs->colorConversionCoeffA = preProcess->colorConversionCoeffA;
        regs->colorConversionCoeffB = preProcess->colorConversionCoeffB;
        regs->colorConversionCoeffC = preProcess->colorConversionCoeffC;
        regs->colorConversionCoeffE = preProcess->colorConversionCoeffE;
        regs->colorConversionCoeffF = preProcess->colorConversionCoeffF;
        regs->colorConversionCoeffG = preProcess->colorConversionCoeffG;
        regs->colorConversionCoeffH = preProcess->colorConversionCoeffH;
        regs->colorConversionLumaOffset = preProcess->colorConversionLumaOffset;
        break;

    case 3: // BT.2020
            // 10bits
        /* Y  = 0.2627 R + 0.678 G + 0.0593 B
         * Cb = 0.531519 B - 0.531519 Y  + 512
         * Cr = 0.67815 R - 0.67815 Y   + 512
        */
        regs->colorConversionCoeffA = preProcess->colorConversionCoeffA = 17216;
        regs->colorConversionCoeffB = preProcess->colorConversionCoeffB = 44433;
        regs->colorConversionCoeffC = preProcess->colorConversionCoeffC = 3886;
        regs->colorConversionCoeffE = preProcess->colorConversionCoeffE = 34834;
        regs->colorConversionCoeffF = preProcess->colorConversionCoeffF = 44443;
        regs->colorConversionCoeffG = preProcess->colorConversionCoeffG = 34834;
        regs->colorConversionCoeffH = preProcess->colorConversionCoeffH = 44443;
        break;

    case 4: /* BT.601 of full range[0,255]*/
        /* Y' = 0.257 R + 0.504 G + 0.098 B
             * Y  = Y' + 16
             * Cb = 0.495 B - 0.576 Y' + 128
             * Cr = 0.627 R - 0.73Y' + 128
             */
        regs->colorConversionCoeffA = preProcess->colorConversionCoeffA = 16843;
        regs->colorConversionCoeffB = preProcess->colorConversionCoeffB = 33030;
        regs->colorConversionCoeffC = preProcess->colorConversionCoeffC = 6423;
        regs->colorConversionCoeffE = preProcess->colorConversionCoeffE = 32440;
        regs->colorConversionCoeffF = preProcess->colorConversionCoeffG = 41091;
        regs->colorConversionCoeffG = preProcess->colorConversionCoeffF = 37749;
        regs->colorConversionCoeffH = preProcess->colorConversionCoeffH = 47841;
        regs->colorConversionLumaOffset = 16;
        break;

    case 5: /* BT.601 of limited range[0,219]*/
        /* Y' = 0.299 R + 0.587 G + 0.114 B
             * Y  = Y' + 16
             * Cb = 0.579 B - 0.579 Y' + 128
             * Cr = 0.728 R - 0.728 Y' + 128
             */
        regs->colorConversionCoeffA = preProcess->colorConversionCoeffA = 19595;
        regs->colorConversionCoeffB = preProcess->colorConversionCoeffB = 38470;
        regs->colorConversionCoeffC = preProcess->colorConversionCoeffC = 7471;
        regs->colorConversionCoeffE = preProcess->colorConversionCoeffE = 37945;
        regs->colorConversionCoeffF = preProcess->colorConversionCoeffF = 47710;
        regs->colorConversionCoeffG = preProcess->colorConversionCoeffG = 37945;
        regs->colorConversionCoeffH = preProcess->colorConversionCoeffH = 47710;
        regs->colorConversionLumaOffset = 16;
        break;

    case 6: /* BT.709 of full range[0,255]*/
        /* Y' = 0.1826 R + 0.6142 G + 0.062 B
             * Y  = Y' + 16
             * Cb = 0.4732 B - 0.5509 Y' + 128
             * Cr = 0.5579 R - 0.6495 Y' + 128
             */
        regs->colorConversionCoeffA = preProcess->colorConversionCoeffA = 11967;
        regs->colorConversionCoeffB = preProcess->colorConversionCoeffB = 40252;
        regs->colorConversionCoeffC = preProcess->colorConversionCoeffC = 4063;
        regs->colorConversionCoeffE = preProcess->colorConversionCoeffE = 31012;
        regs->colorConversionCoeffF = preProcess->colorConversionCoeffG = 36563;
        regs->colorConversionCoeffG = preProcess->colorConversionCoeffF = 36104;
        regs->colorConversionCoeffH = preProcess->colorConversionCoeffH = 42566;
        regs->colorConversionLumaOffset = 16;
        break;
    }

    /* Setup masks to separate R, G and B from RGB */
    regs->rMaskMsb = rgbMaskBits[preProcess->inputFormat][0];
    regs->gMaskMsb = rgbMaskBits[preProcess->inputFormat][1];
    regs->bMaskMsb = rgbMaskBits[preProcess->inputFormat][2];
}
