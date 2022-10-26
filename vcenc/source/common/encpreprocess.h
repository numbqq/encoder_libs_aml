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
#ifndef __ENC_PRE_PROCESS_H__
#define __ENC_PRE_PROCESS_H__

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/
#include "base_type.h"
#include "encasiccontroller.h"

/*------------------------------------------------------------------------------
    2. External compiler flags
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/

#define ROTATE_0 0U
#define ROTATE_90R 1U  /* Rotate 90 degrees clockwise */
#define ROTATE_90L 2U  /* Rotate 90 degrees counter-clockwise */
#define ROTATE_180R 3U /* Rotate 180 degrees clockwise */

/* maximum input picture width set by the available bits in ASIC regs */
#define MAX_INPUT_IMAGE_WIDTH (32768)

typedef struct {
    u32 lumWidthSrc[HEVC_MAX_TILE_COLS];  /* Source input image width */
    u32 lumHeightSrc[HEVC_MAX_TILE_COLS]; /* Source input image height */
    u32 lumWidth;                         /* Encoded image width */
    u32 lumHeight;                        /* Encoded image height */
    u32 scaledWidth;                      /* Scaled output image width */
    u32 scaledHeight;                     /* Scaled output image height */
    u32 horOffsetSrc[HEVC_MAX_TILE_COLS]; /* Encoded frame offset, reference is ... */
    u32 verOffsetSrc[HEVC_MAX_TILE_COLS]; /* ...top  left corner of source image */
    u32 input_alignment;
    u32 inputFormat;
    u32 rotation;
    u32 mirror;
    u32 sliced_frame; /*this is used for jpeg tile_32_32/tile_16_16_packed_4 format*/
    u32 videoStab;
    u32 scaledOutput;
    u32 scaledOutputFormat; /*0:YUV422   1:YUV420SP*/
    u32 inLoopDSRatio; /* down-scaling ratio, set by SetParameter(), 0=1:1(no downsample), 1=1:2 (1/2 downsample) */
    u32 colorConversionType; /* 0 = bt601, 1 = bt709, 2 = user defined */
    u32 colorConversionCoeffA;
    u32 colorConversionCoeffB;
    u32 colorConversionCoeffC;
    u32 colorConversionCoeffE;
    u32 colorConversionCoeffF;
    u32 colorConversionCoeffG;
    u32 colorConversionCoeffH;
    u32 colorConversionLumaOffset;
    i32 adaptiveRoi;
    i32 adaptiveRoiColor;  /* Color temperature -10..10 = 2000K..5000K */
    i32 adaptiveRoiMotion; /* Motion sensitivity -10..10 */
    u32 prevMapCount;      /* How many frames previous map is used in. */
    u32 intra;
    u32 mvFrames; /* How many frames mvMap is counted for. */
    u32 cbComp;   /* Chroma compensation for white balance fix. */
    u32 crComp;
    u32 boxw; /* Filter box dimensions. */
    u32 boxh;
    u8 *skinMap[2];       /* Skin MBs before/after filtering */
    u8 *roiSegmentMap[3]; /* MB maps, 0=curr, 1=prev, 2=curr expanded */
    u32 roiMbCount[3];    /* Amount of MBs in each of above maps */
    i32 *mvMap;           /* Motion map based on MB MVs */
    u8 *scoreMap;         /* Skin + motion score for each MB */
    u32 roiUpdate;        /* AROI calculation has updated roiSegmentMap[2] */
    u32 roiCoded;         /* The new ROI map has been coded to stream */
    u32 interlacedFrame;  /* Enable interlaced frame input */
    u32 bottomField;      /* Current picture is interlaced bottom field */

    i32 frameCropping;
    u32 frameCropLeftOffset;
    u32 frameCropRightOffset;
    u32 frameCropTopOffset;
    u32 frameCropBottomOffset;

    /* constant chroma control */
    i32 constChromaEn;
    u32 constCb;
    u32 constCr;

    /* overlay control*/
    u32 overlayEnable[MAX_OVERLAY_NUM];
    u32 overlayFormat[MAX_OVERLAY_NUM];
    u32 overlayAlpha[MAX_OVERLAY_NUM];
    u32 overlayXoffset[MAX_OVERLAY_NUM];
    u32 overlayCropXoffset[MAX_OVERLAY_NUM];
    u32 overlayYoffset[MAX_OVERLAY_NUM];
    u32 overlayCropYoffset[MAX_OVERLAY_NUM];

    u32 overlayWidth[MAX_OVERLAY_NUM];
    u32 overlayCropWidth[MAX_OVERLAY_NUM];
    u32 overlayScaleWidth[MAX_OVERLAY_NUM];
    u32 overlayHeight[MAX_OVERLAY_NUM];
    u32 overlayCropHeight[MAX_OVERLAY_NUM];
    u32 overlayScaleHeight[MAX_OVERLAY_NUM];
    u32 overlayYStride[MAX_OVERLAY_NUM];
    u32 overlayUVStride[MAX_OVERLAY_NUM];
    u32 overlayBitmapY[MAX_OVERLAY_NUM];
    u32 overlayBitmapU[MAX_OVERLAY_NUM];
    u32 overlayBitmapV[MAX_OVERLAY_NUM];
    u32 overlaySuperTile[MAX_OVERLAY_NUM];
    //To support jpeg slice
    u32 overlaySliceHeight[MAX_OVERLAY_NUM];  //Height for OSD region in a slice
    u32 overlayVerOffset[MAX_OVERLAY_NUM];    //Vertical Offset for overlay input
    u32 overlaySliceYoffset[MAX_OVERLAY_NUM]; //Top left offset to place OSD in a slice

    /* Mosaic control */
    u32 mosEnable[MAX_MOSAIC_NUM];
    u32 mosXoffset[MAX_MOSAIC_NUM];
    u32 mosYoffset[MAX_MOSAIC_NUM];
    u32 mosWidth[MAX_MOSAIC_NUM];
    u32 mosHeight[MAX_MOSAIC_NUM];

    // subsample
    u32 subsample_x;
    u32 subsample_y;

    VCEncVideoCodecFormat codecFormat;
} preProcess_s;

/*------------------------------------------------------------------------------
    4. Function prototypes
------------------------------------------------------------------------------*/
i32 EncPreProcessAlloc(preProcess_s *preProcess, i32 mbPerPicture);
void EncPreProcessFree(preProcess_s *preProcess);
i32 EncPreProcessCheck(const preProcess_s *preProcess, i32 tileColumnNum);
void EncPreProcess_60(asicData_s *asic, preProcess_s *preProcess);
void EncPreProcess(asicData_s *asic, preProcess_s *preProcess, void *ctx, u32 tileId);
void EncSetColorConversion(preProcess_s *preProcess, asicData_s *asic);
u32 EncGetAlignedByteStride(int width, i32 input_format, u32 *luma_stride, u32 *chroma_stride,
                            u32 input_alignment);
u32 EncPreGetHwFormat(u32 inputFormat);

#endif
