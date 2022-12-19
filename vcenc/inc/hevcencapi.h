/*------------------------------------------------------------------------------
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                                                                            --
--                            Verisilicon.                                    --
--                                                                            --
--                   (C) COPYRIGHT 2014 VERISILICON                           --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
--------------------------------------------------------------------------------
--
--  Abstract : VC8000 Encoder API
--
------------------------------------------------------------------------------*/

#ifndef API_H
#define API_H

#include "base_type.h"
#include "enccommon.h"
#include "enc_log.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * \defgroup api_video Video Encoder API
 *
 * @{
 */

#define VCENC_MAX_REF_FRAMES 8
#define MAX_GOP_PIC_CONFIG_NUM 48
#define MAX_GOP_SIZE 16
#define MAX_CORE_NUM 4
#define MAX_ADAPTIVE_GOP_SIZE 8
#define MAX_GOP_SPIC_CONFIG_NUM 16
#define VCENC_STREAM_MIN_BUF0_SIZE 1024 * 11
#define LONG_TERM_REF_ID2DELTAPOC(id) ((id) + 10000)
#define LONG_TERM_REF_DELTAPOC2ID(poc) ((poc)-10000)
#define IS_LONG_TERM_REF_DELTAPOC(poc) ((poc) >= 10000)
#define VCENC_MAX_LT_REF_FRAMES VCENC_MAX_REF_FRAMES
#define MMO_STR2LTR(poc) ((poc) | 0x40000000)
#define VCENC_LOOKAHEAD_MAX 64
#define ROIMAP_PREFETCH_EXT_SIZE 1536

/* define for HEVC*/
#define HEVC_MAX_TILE_COLS (20)
#define HEVC_MAX_TILE_ROWS (22)

/* define for special configure */
#define QPOFFSET_RESERVED -255
#define QPFACTOR_RESERVED -255
#define TEMPORALID_RESERVED -255
#define NUMREFPICS_RESERVED -255
#define FRAME_TYPE_RESERVED -255
#define INVALITED_POC -1

/* define for AV1 */
#define AV1_REF_FRAME_NUM 8
#define AV1_REFS_PER_FRAME 7
#define REGULAR_FRAME 0
/*#define LAST_REF_FRAME     1
#define LAST2_REF_FRAME    2
#define LAST3_REF_FRAME    3
#define GOLDEN_FRAME        4
#define BWDREF_FRAME        5
#define ALTREF2_FRAME       6
#define ALTREF_FRAME        7
#define SW_NONE_FRAME    (-1)
#define SW_INTRA_FRAME   0
#define SW_LAST_FRAME    1
#define SW_LAST2_FRAME   2
#define SW_LAST3_FRAME   3
#define SW_GOLDEN_FRAME  4
#define SW_BWDREF_FRAME  5
#define SW_ALTREF2_FRAME 6
#define SW_ALTREF_FRAME  7
*/

#define AQ_STRENGTH_SCALE_BITS 7
#define PSY_FACTOR_SCALE_BITS 8

/* define for video codec detection */
#define IS_HEVC(a) (a == VCENC_VIDEO_CODEC_HEVC)
#define IS_H264(a) (a == VCENC_VIDEO_CODEC_H264)
#define IS_AV1(a) (a == VCENC_VIDEO_CODEC_AV1)
#define IS_VP9(a) (a == VCENC_VIDEO_CODEC_VP9)

/** \brief MAX_ADAPTIVE_GOP_SIZE for gopSize<=MAX_ADAPTIVE_GOP_SIZE, or AGOP */
#define MAX_GOP_SIZE_INUSE(gopSize)                                                                \
    (gopSize <= MAX_ADAPTIVE_GOP_SIZE ? MAX_ADAPTIVE_GOP_SIZE : MAX_GOP_SIZE)

/* define for cutree buffer count */
/** \brief Cutree works on whole GOPs. so at most (MAX_GOP_SIZE-1) frames are needed to complete a GOP.
  for all gopSize, at most ((depth) + MAX_GOP_SIZE / 2) frames are queued in cutree;
  if current GOP is still incomplete, cutree will work on frames excluding current GOP
*/
#define CUTREE_MAX_LOOKAHEAD_FRAMES(depth, gopSize) ((depth) + MAX_GOP_SIZE_INUSE(gopSize) / 2)

/** \brief Extra 1 for first frame in cutree queue (already processed/output) */
#define CUTREE_BUFFER_CNT(depth, gopSize) ((depth) + MAX_GOP_SIZE_INUSE(gopSize) / 2 + 1)

/*------------------------------------------------------------------------------
      1. Type definition for encoder instance
  ------------------------------------------------------------------------------*/
typedef const void *VCEncInst;

/*------------------------------------------------------------------------------
      2. Enumerations for API parameters
  ------------------------------------------------------------------------------*/

/** Function return values */
typedef enum {
    /** Success */
    VCENC_OK = 0,
    /** A frame encoding was finished. */
    VCENC_FRAME_READY = 1,
    /** A frame inside the encoder's internal queue but not encoded yet, will be encoded and output in the following call to the encoder API */
    VCENC_FRAME_ENQUEUE = 2,
    /** Encoder error. */
    VCENC_ERROR = -1,
    /** Error, a pointer argument had an invalid NULL value. */
    VCENC_NULL_ARGUMENT = -2,
    /** Error, one of the arguments was invalid. None of the arguments was set. */
    VCENC_INVALID_ARGUMENT = -3,
    /** Error, the encoder was not able to allocate memory. */
    VCENC_MEMORY_ERROR = -4,
    /** Error, the encoder’s system interface failed to initialize. */
    VCENC_EWL_ERROR = -5,
    /** Error, the system interface failed to allocate memory. */
    VCENC_EWL_MEMORY_ERROR = -6,
    /** Error, the stream was started with Hypothetical Reference Decoder (HRD) enabled and consequently, the rate control parameters cannot be altered. */
    VCENC_INVALID_STATUS = -7,
    /** Error, the output buffer’s size was too small to fit the generated stream. Allocate a bigger buffer and try again. */
    VCENC_OUTPUT_BUFFER_OVERFLOW = -8,
    /** Error, this can be caused by invalid bus addresses that push the encoder to access an invalid memory area. New frame encoding must be started. */
    VCENC_HW_BUS_ERROR = -9,
    /** Error in the hardware data. */
    VCENC_HW_DATA_ERROR = -10,
    /** Error, the wait for a hardware finish has timed out. The current frame is lost. New frame encoding must be started. */
    VCENC_HW_TIMEOUT = -11,
    /** Error, the hardware could not be reserved for exclusive access. */
    VCENC_HW_RESERVED = -12,
    /** Error, a fatal system error occurred. The encoding can’t continue. The encoder instance must be released.*/
    VCENC_SYSTEM_ERROR = -13,
    /** Error, the encoder instance is invalid or corrupted. */
    VCENC_INSTANCE_ERROR = -14,
    /** Hypothetical Reference Decoder error. */
    VCENC_HRD_ERROR = -15,
    /** Error, the hardware was reset by external means. The whole frame is lost. */
    VCENC_HW_RESET = -16
} VCEncRet;

/** Video Codec Format */
typedef enum {
    /** Video codec format is HEVC */
    VCENC_VIDEO_CODEC_HEVC = 0,
    /** Video codec format is H.264 */
    VCENC_VIDEO_CODEC_H264 = 1,
    /** Video codec format is AV1 */
    VCENC_VIDEO_CODEC_AV1 = 2,
    /** Video codec format is VP9 */
    VCENC_VIDEO_CODEC_VP9 = 3
} VCEncVideoCodecFormat;

/** Stream type for initialization */
typedef enum {
    VCENC_BYTE_STREAM = 0,    /**< NAL unit starts with hex bytes '00 00 00 01' */
    VCENC_NAL_UNIT_STREAM = 1 /**< Plain NAL units without startcode */
} VCEncStreamType;

/** Level for initialization */
/**
  * <table>
  * <caption id='tab_vencLevel'>VCEncoder Level Enumeration:HEVC</caption>
  * <tr>
  *   <th align="center"> VCEncoder Level Values: HEVC
  *   <th align="center"> Encoded Picture Size
  *   <th align="center"> Frame Rate (fps)
  *   <th align="center"> Max luma picture size(samples)
  *   <th align="center"> Max luma sample rate(samples/sec)
  *   <th align="center"> Max Bit Rate(kbps)
  * <tr>
  *   <td align="center"> VCENC_HEVC\n _LEVEL_1
  *   <td align="center"> QCIF
  *   <td align="center"> 15
  *   <td align="center"> 36,864
  *   <td align="center"> 552,960
  *   <td align="center"> 128
  * <tr>
  *   <td align="center"> VCENC_HEVC\n _LEVEL_2
  *   <td align="center"> CIF
  *   <td align="center"> 30
  *   <td align="center"> 122,880
  *   <td align="center"> 3,686,400
  *   <td align="center"> 1500
  * <tr>
  *   <td align="center" > VCENC_HEVC\n _LEVEL_2_1
  *   <td align="center"> Q720p
  *   <td align="center"> 30
  *   <td align="center"> 245,760
  *   <td align="center"> 7,372,800
  *   <td align="center"> 3000
  * <tr>
  *   <td align="center"> VCENC_HEVC\n _LEVEL_3
  *   <td align="center"> QHD
  *   <td align="center"> 30
  *   <td align="center"> 552,960
  *   <td align="center"> 16,588,800
  *   <td align="center"> 6000
  * <tr>
  *   <td align="center"> VCENC_HEVC\n _LEVEL_3_1
  *   <td align="center"> 1280x720
  *   <td align="center"> 30
  *   <td align="center"> 983,040
  *   <td align="center"> 33,177,600
  *   <td align="center"> 10000
  * <tr>
  *   <td align="center"> VCENC_HEVC\n _LEVEL_4
  *   <td align="center"> 2Kx1080
  *   <td align="center"> 30
  *   <td align="center"> 2,228,224
  *   <td align="center"> 66,846,720
  *   <td align="center"> 12000
  * <tr>
  *   <td align="center" > VCENC_HEVC\n _LEVEL_4_1
  *   <td align="center"> 2Kx1080
  *   <td align="center"> 60
  *   <td align="center"> 2,228,224
  *   <td align="center"> 133,693,440
  *   <td align="center"> 20000
  * <tr>
  *   <td align="center" > VCENC_HEVC\n _LEVEL_5
  *   <td align="center"> 4096x2160
  *   <td align="center"> 30
  *   <td align="center"> 8,912,896
  *   <td align="center"> 267,386,880
  *   <td align="center"> 25000
  * <tr>
  *   <td align="center" > VCENC_HEVC\n _LEVEL_5_1
  *   <td align="center"> 4096x2160
  *   <td align="center"> 60
  *   <td align="center"> 8,912,896
  *   <td align="center"> 534,773,760
  *   <td align="center"> 40000
  * <tr>
  *   <td align="center" > VCENC_HEVC\n _LEVEL_5_2
  *   <td align="center"> 4096x2160
  *   <td align="center"> 120
  *   <td align="center"> 8,912,896
  *   <td align="center"> 1,069,547,520
  *   <td align="center"> 60000
  * <tr>
  *   <td align="center"> VCENC_HEVC\n _LEVEL_6
  *   <td align="center"> 8192x4320
  *   <td align="center"> 30
  *   <td align="center"> 35,651,584
  *   <td align="center"> 1,069,547,520
  *   <td align="center"> 60000
  * <tr>
  *   <td align="center"> VCENC_HEVC\n _LEVEL_6_1
  *   <td align="center"> 8192x4320
  *   <td align="center"> 60
  *   <td align="center"> 35,651,584
  *   <td align="center"> 2,139,095,040
  *   <td align="center"> 120000
  * <tr>
  *   <td align="center" > VCENC_HEVC\n _LEVEL_6_2
  *   <td align="center"> 8192x4320
  *   <td align="center"> 120
  *   <td align="center"> 35,651,584
  *   <td align="center"> 4,278,190,080
  *   <td align="center"> 240000
  * </table>
  *
  * <table>
  * <caption id='tab_vencLevel1'>VCEncoder Level Enumeration:H.264</caption>
  * <tr>
  *  <th align="center"> VCEncLevel Values : H.264
  *  <th align="center"> Encoded Picture Size
  *  <th align="center"> Frame Rate (fps)
  *  <th align="center"> Max luma picture size (samples)
  *  <th align="center"> Max luma sample rate (samples/sec)
  *  <th align="center"> Max Bit Rate (kbps)
  * <tr>
  *  <td align="center"> VCENC_H264\n _LEVEL_1
  *  <td align="center"> <table><tr><td> Sub-QCIF <tr><td> QCIF </table>
  *  <td align="center"> <table><tr><td> 15 <tr><td> 15 </table>
  *  <td align="center"> 99 (QCIF)
  *  <td align="center"> 1,485
  *  <td align="center"> 64
  * <tr>
  *  <td align="center"> VCENC_H264\n _LEVEL_1_b
  *  <td align="center"> <table><tr><td> Sub-QCIF <tr><td> QCIF </table>
  *  <td align="center"> <table><tr><td> 15 <tr><td> 15 </table>
  *  <td align="center"> 99 (QCIF)
  *  <td align="center"> 1,485
  *  <td align="center"> 128
  * <tr>
  *  <td align="center"> VCENC_H264\n _LEVEL_1_1
  *  <td align="center"> <table><tr><td> QCIF <tr><td> QVGA </table>
  *  <td align="center"> <table><tr><td> 30 <tr><td> 10 </table>
  *  <td align="center"> 396 (QCIF)
  *  <td align="center"> 3,000
  *  <td align="center"> 192
  * <tr>
  *  <td align="center"> VCENC_H264\n _LEVEL_1_2
  *  <td align="center"> <table><tr><td> QVGA <tr><td> CIF </table>
  *  <td align="center"> <table><tr><td> 20 <tr><td> 15 </table>
  *  <td align="center"> 396 (QCIF)
  *  <td align="center"> 6,000
  *  <td align="center"> 384
  * <tr>
  *  <td align="center"> VCENC_H264\n _LEVEL_1_3
  *  <td align="center"> <table><tr><td> QVGA <tr><td> CIF </table>
  *  <td align="center"> <table><tr><td> 30 <tr><td> 30 </table>
  *  <td align="center"> 396 (QCIF)
  *  <td align="center"> 11,880
  *  <td align="center"> 768
  * <tr>
  *  <td align="center"> VCENC_H264\n _LEVEL_2
  *  <td align="center"> CIF
  *  <td align="center"> 30
  *  <td align="center"> 396 (QCIF)
  *  <td align="center"> 11,880
  *  <td align="center"> 2000
  * <tr>
  *  <td align="center"> VCENC_H264\n _LEVEL_2_1
  *  <td align="center"> 512x384
  *  <td align="center"> 25
  *  <td align="center"> 792
  *  <td align="center"> 19,800
  *  <td align="center"> 4000
  * <tr>
  *  <td align="center"> VCENC_H264\n _LEVEL_2_2
  *  <td align="center"> <table><tr><td> VGA <tr><td> 720x480  </table>
  *  <td align="center"> <table><tr><td> 15 <tr><td> 15 </table>
  *  <td align="center"> 1,620 (PAL)
  *  <td align="center"> 20,250
  *  <td align="center"> 4,000
  * <tr>
  *  <td align="center"> VCENC_H264\n _LEVEL_3
  *  <td align="center"> <table><tr><td> VGA <tr><td> 720x576 <tr><td> 720x480 </table>
  *  <td align="center"> <table><tr><td> 30 <tr><td> 25 <tr><td> 30 </table>
  *  <td align="center"> 1,620 (PAL)
  *  <td align="center"> 40,500
  *  <td align="center"> 10,000
  * <tr>
  *  <td align="center"> VCENC_H264\n _LEVEL_3_1
  *  <td align="center"> 1280x720
  *  <td align="center"> 30
  *  <td align="center"> 3,600 (HD 720p)
  *  <td align="center"> 108,000
  *  <td align="center"> 14,000
  * <tr>
  *  <td align="center"> VCENC_H264\n _LEVEL_3_2
  *  <td align="center"> 1280x720
  *  <td align="center"> 60
  *  <td align="center"> 5,120 (SXGA)
  *  <td align="center"> 216,000
  *  <td align="center"> 20,000
  * <tr>
  *  <td align="center"> VCENC_H264\n  _LEVEL_4
  *  <td align="center"> 2048x1024
  *  <td align="center"> 30
  *  <td align="center"> 8,192 (1080p)
  *  <td align="center"> 245,760
  *  <td align="center"> 20,000
  * <tr>
  *  <td align="center"> VCENC_H264\n _LEVEL_4_1
  *  <td align="center"> 2048x1024
  *	 <td align="center"> 30
  *  <td align="center"> 8,192 (1080p)
  *  <td align="center"> 245,760
  *  <td align="center"> 50,000
  * <tr>
  *  <td align="center"> VCENC_H264\n _LEVEL_4_2
  *  <td align="center"> 2048x1024
  *  <td align="center"> 60
  *  <td align="center"> 8,192 (1080p)
  *  <td align="center"> 49,1520
  *  <td align="center"> 50,000
  * <tr>
  *  <td align="center"> VCENC_H264\n _LEVEL_5
  *  <td align="center"> 3680x1536
  *  <td align="center"> 25
  *  <td align="center"> 22,080
  *  <td align="center"> 58,9824
  *  <td align="center"> 135,000
  * <tr>
  *  <td align="center"> VCENC_H264\n _LEVEL_5_1
  *  <td align="center"> 4096x2048
  *  <td align="center"> 30
  *  <td align="center"> 36,864
  *  <td align="center"> 983,040
  *  <td align="center"> 240,000
  * <tr>
  *  <td align="center"> VCENC_H264\n _LEVEL_5_2
  *  <td align="center"> 4096x2160
  *  <td align="center"> 60
  *  <td align="center"> 36,864
  *  <td align="center"> 2,073,600
  *  <td align="center"> 240,000
  * <tr>
  *  <td align="center"> VCENC_H264\n _LEVEL_6
  *  <td align="center"> 8192x4320
  *  <td align="center"> 30
  *  <td align="center"> 35,651,584
  *  <td align="center"> 4,177,920
  *  <td align="center"> 240,000
  * <tr>
  *  <td align="center"> VCENC_H264\n _LEVEL_6_1
  *  <td align="center"> 8192x4320
  *  <td align="center"> 60
  *  <td align="center"> 35,651,584
  *  <td align="center"> 8,355,840
  *  <td align="center"> 480,000
  * <tr>
  *  <td align="center"> VCENC_H264\n _LEVEL_6_2
  *  <td align="center"> 8192x4320
  *  <td align="center"> 120
  *  <td align="center"> 35,651,584
  *  <td align="center"> 16,711,680
  *  <td align="center"> 800,000
  * </table>
  */
typedef enum {
    VCENC_HEVC_LEVEL_1 = 30,
    VCENC_HEVC_LEVEL_2 = 60,
    VCENC_HEVC_LEVEL_2_1 = 63,
    VCENC_HEVC_LEVEL_3 = 90,
    VCENC_HEVC_LEVEL_3_1 = 93,
    VCENC_HEVC_LEVEL_4 = 120,
    VCENC_HEVC_LEVEL_4_1 = 123,
    VCENC_HEVC_LEVEL_5 = 150,
    VCENC_HEVC_LEVEL_5_1 = 153,
    VCENC_HEVC_LEVEL_5_2 = 156,
    VCENC_HEVC_LEVEL_6 = 180,
    VCENC_HEVC_LEVEL_6_1 = 183,
    VCENC_HEVC_LEVEL_6_2 = 186,

    /* H264 Defination*/
    VCENC_H264_LEVEL_1 = 10,
    VCENC_H264_LEVEL_1_b = 99,
    VCENC_H264_LEVEL_1_1 = 11,
    VCENC_H264_LEVEL_1_2 = 12,
    VCENC_H264_LEVEL_1_3 = 13,
    VCENC_H264_LEVEL_2 = 20,
    VCENC_H264_LEVEL_2_1 = 21,
    VCENC_H264_LEVEL_2_2 = 22,
    VCENC_H264_LEVEL_3 = 30,
    VCENC_H264_LEVEL_3_1 = 31,
    VCENC_H264_LEVEL_3_2 = 32,
    VCENC_H264_LEVEL_4 = 40,
    VCENC_H264_LEVEL_4_1 = 41,
    VCENC_H264_LEVEL_4_2 = 42,
    VCENC_H264_LEVEL_5 = 50,
    VCENC_H264_LEVEL_5_1 = 51,
    VCENC_H264_LEVEL_5_2 = 52,
    VCENC_H264_LEVEL_6 = 60,
    VCENC_H264_LEVEL_6_1 = 61,
    VCENC_H264_LEVEL_6_2 = 62,

    VCENC_AUTO_LEVEL = 0xFFFF
} VCEncLevel;

#define VCENC_PROFILE_H264_CONSTRAINED (1 << 9) /** 8+1; constraint_set1_flag */
#define VCENC_PROFILE_H264_INTRA (1 << 11)      /** 8+3; constraint_set3_flag */
/** Profile for initialization
   * VCENC_PROFILE_xxx is based on the spec
   */
typedef enum {
    /** Allows for a bit depth of 8 bits per sample with 4:2:0 chroma sampling. */
    VCENC_HEVC_MAIN_PROFILE = 0,
    /** Allows for a single still picture to be encoded with the same constraints as the Main profile. */
    VCENC_HEVC_MAIN_STILL_PICTURE_PROFILE = 1,
    /** Allows for a bit depth of 8-bits to 10-bits per sample with 4:2:0 chroma sampling. */
    VCENC_HEVC_MAIN_10_PROFILE = 2,
    VCENC_HEVC_MAINREXT = 3,
    VCENC_PROFILE_HEVC_MAIN = 1,
    VCENC_PROFILE_HEVC_MAIN_10 = 2,
    VCENC_PROFILE_HEVC_MAIN_STILL_PICTURE = 3,
    VCENC_PROFILE_HEVC_REXT = 4,

    /* H264 Defination*/
    /** Used for H.264 Baseline Profile for videoconferencing and mobile applications. */
    VCENC_H264_BASE_PROFILE = 9,
    /** Used for standard-definition digital TV broadcasts that use the MPEG-4 format.*/
    VCENC_H264_MAIN_PROFILE = 10,
    /** The primary profile for broadcast and disc storage applications, particularly for high-definition TV applications. */
    VCENC_H264_HIGH_PROFILE = 11,
    /** This profile builds on top of the High Profile, adding support for up to 10 bits per sample. */
    VCENC_H264_HIGH_10_PROFILE = 12,
    VCENC_PROFILE_H264_BASELINE = 66,
    VCENC_PROFILE_H264_CONSTRAINED_BASELINE = (66 | VCENC_PROFILE_H264_CONSTRAINED),
    VCENC_PROFILE_H264_MAIN = 77,
    VCENC_PROFILE_H264_EXTENDED = 88,
    VCENC_PROFILE_H264_HIGH = 100,
    VCENC_PROFILE_H264_HIGH_10 = 110,
    VCENC_PROFILE_H264_HIGH_10_INTRA = (110 | VCENC_PROFILE_H264_INTRA),
    VCENC_PROFILE_H264_MULTIVIEW_HIGH = 118,
    VCENC_PROFILE_H264_HIGH_422 = 122,
    VCENC_PROFILE_H264_HIGH_422_INTRA = (122 | VCENC_PROFILE_H264_INTRA),
    VCENC_PROFILE_H264_STEREO_HIGH = 128,
    VCENC_PROFILE_H264_HIGH_444 = 144,
    VCENC_PROFILE_H264_HIGH_444_PREDICTIVE = 244,
    VCENC_PROFILE_H264_HIGH_444_INTRA = (244 | VCENC_PROFILE_H264_INTRA),
    VCENC_PROFILE_H264_CAVLC_444 = 44,

    /* AV1 Defination*/
    VCENC_AV1_MAIN_PROFILE = 0, /**< 4:2:0 8/10 bit */
    VCENC_AV1_HIGH_PROFILE = 1,
    VCENC_AV1_PROFESSIONAL_PROFILE = 2,

    /*Vp9 Defination*/
    VCENC_VP9_MAIN_PROFILE = 0,  /**< 4:2:0 8 bit No SRGB */
    VCENC_VP9_MSRGB_PROFILE = 1, /**< 4:2:2 4:4:0 4:4:4 8 bit SRGB */
    VCENC_VP9_HIGH_PROFILE = 2,  /**< 4:2:0 10/12 bit No SRGB */
    VCENC_VP9_HSRGB_PROFILE = 3, /**< 4:2:2 4:4:0 4:4:4 10 bit SRGB */
} VCEncProfile;

/** Tier for initialization */
typedef enum {
    VCENC_HEVC_MAIN_TIER = 0,
    VCENC_HEVC_HIGH_TIER = 1,
} VCEncTier;

/** Picture YUV type for initialization */
typedef enum {
    /** YYYY... UUUU... VVVV...  */
    VCENC_YUV420_PLANAR = 0,
    /** YYYY... UVUVUV...        */
    VCENC_YUV420_SEMIPLANAR = 1,
    /** YYYY... VUVUVU...        */
    VCENC_YUV420_SEMIPLANAR_VU = 2,
    /** YUYVYUYV...              */
    VCENC_YUV422_INTERLEAVED_YUYV = 3,
    /** UYVYUYVY...              */
    VCENC_YUV422_INTERLEAVED_UYVY = 4,
    /** 16-bit RGB 16bpp         */
    VCENC_RGB565 = 5,
    /** 16-bit RGB 16bpp         */
    VCENC_BGR565 = 6,
    /** 15-bit RGB 16bpp         */
    VCENC_RGB555 = 7,
    /** 15-bit RGB 16bpp         */
    VCENC_BGR555 = 8,
    /** 12-bit RGB 16bpp         */
    VCENC_RGB444 = 9,
    /** 12-bit RGB 16bpp         */
    VCENC_BGR444 = 10,
    /** 24-bit RGB 32bpp         */
    VCENC_RGB888 = 11,
    /** 24-bit RGB 32bpp         */
    VCENC_BGR888 = 12,
    /** 30-bit RGB 32bpp         */
    VCENC_RGB101010 = 13,
    /** 30-bit RGB 32bpp         */
    VCENC_BGR101010 = 14,
    /** YYYY... UUUU... VVVV...  */
    VCENC_YUV420_PLANAR_10BIT_I010 = 15,
    /** YYYY... UVUVUV...  */
    VCENC_YUV420_PLANAR_10BIT_P010 = 16,
    /** YYYY... UUUU... VVVV...  */
    VCENC_YUV420_PLANAR_10BIT_PACKED_PLANAR = 17,
    /** Y0U0Y1a0a1Y2V0Y3a2a3Y4U1Y5a4a5Y6V1Y7a6a7... */
    VCENC_YUV420_10BIT_PACKED_Y0L2 = 18,
    VCENC_YUV420_PLANAR_8BIT_TILE_32_32 = 19,
    VCENC_YUV420_PLANAR_8BIT_TILE_16_16_PACKED_4 = 20,
    /** YYYY... UVUVUV...        */
    VCENC_YUV420_SEMIPLANAR_8BIT_TILE_4_4 = 21,
    /** YYYY... VUVUVU...        */
    VCENC_YUV420_SEMIPLANAR_VU_8BIT_TILE_4_4 = 22,
    /** YYYY... UVUV... */
    VCENC_YUV420_PLANAR_10BIT_P010_TILE_4_4 = 23,
    /** YYYY... UVUV... */
    VCENC_YUV420_SEMIPLANAR_101010 = 24,
    /** YYYY... VUVU... */
    VCENC_YUV420_8BIT_TILE_64_4 = 26,
    /** YYYY... UVUV... */
    VCENC_YUV420_UV_8BIT_TILE_64_4 = 27,
    /** YYYY... UVUV... */
    VCENC_YUV420_10BIT_TILE_32_4 = 28,
    /** YYYY... UVUV... */
    VCENC_YUV420_10BIT_TILE_48_4 = 29,
    /** YYYY... VUVU... */
    VCENC_YUV420_VU_10BIT_TILE_48_4 = 30,
    /** YYYY... VUVU... */
    VCENC_YUV420_8BIT_TILE_128_2 = 31,
    /** YYYY... UVUV... */
    VCENC_YUV420_UV_8BIT_TILE_128_2 = 32,
    /** YYYY... UVUV... */
    VCENC_YUV420_10BIT_TILE_96_2 = 33,
    /** YYYY... VUVU... */
    VCENC_YUV420_VU_10BIT_TILE_96_2 = 34,
    /** YYYY... UVUV... */
    VCENC_YUV420_8BIT_TILE_8_8 = 35,
    /** YYYY... UVUV... */
    VCENC_YUV420_10BIT_TILE_8_8 = 36,
    /** YYYY... VVVV... UUUU... */
    VCENC_YVU420_PLANAR = 37,
    /** YYYY... UVUVUV   64x2 tile */
    VCENC_YUV420_UV_8BIT_TILE_64_2 = 38,
    /** YYYY... UVUVUV   64x2 tile */
    VCENC_YUV420_UV_10BIT_TILE_128_2 = 39,
    /** 24-bit RGB 24bpp    (MSB)BGR(LSB)     */
    VCENC_RGB888_24BIT = 40,
    /** 24-bit RGB 24bpp    (MSB)RGB(LSB)     */
    VCENC_BGR888_24BIT = 41,
    /** 24-bit RGB 24bpp    (MSB)GBR(LSB)     */
    VCENC_RBG888_24BIT = 42,
    /** 24-bit RGB 24bpp    (MSB)RBG(LSB)     */
    VCENC_GBR888_24BIT = 43,
    /** 24-bit RGB 24bpp    (MSB)GRB(LSB)     */
    VCENC_BRG888_24BIT = 44,
    /** 24-bit RGB 24bpp    (MSB)BRG(LSB)     */
    VCENC_GRB888_24BIT = 45,
    VCENC_FORMAT_MAX

} VCEncPictureType;

/** Picture rotation for pre-processing. Check HW if each feature is supported. */
typedef enum {
    /** No rotation. */
    VCENC_ROTATE_0 = 0,
    /** Rotate 90 degrees clockwise */
    VCENC_ROTATE_90R = 1,
    /** Rotate 90 degrees counter-clockwise */
    VCENC_ROTATE_90L = 2,
    /** Rotate 180 degrees clockwise */
    VCENC_ROTATE_180R = 3
} VCEncPictureRotation;

/** Picture mirror for pre-processing. Check HW if each feature is supported. */
typedef enum {
    /** no mirror */
    VCENC_MIRROR_NO = 0,
    /** mirror */
    VCENC_MIRROR_YES = 1
} VCEncPictureMirror;

/** Picture color space conversion (RGB input) for pre-processing */
typedef enum {
    /** Color conversion of limited range[16,235] according to BT.601 [3] */
    VCENC_RGBTOYUV_BT601 = 0,
    /** Color conversion of limited range[16,235] according to BT.709 [4] */
    VCENC_RGBTOYUV_BT709 = 1,
    /**  Conversion using user-defined coefficients coeffA, coeffB, coeffC, coeffE and coeffF in formula:
   * \f[
   *    Y=  (( coeffA * R + coeffB * G + coeffC * B ))/65536
   * \f]
   * \f[
   *    Cb = ((coeffE*( B-Y ) ))/(65536+128)
   * \f]
   * \f[
   *    Cr = ((coeffF*(R-Y) ))/(65536+128)
   * \f]
   */
    VCENC_RGBTOYUV_USER_DEFINED = 2,
    /** Conversion according to to ITU-R Recommendation ITU-R-REC-BT.2020 [5].*/
    VCENC_RGBTOYUV_BT2020 = 3,
    /** Color conversion of full range[0,255] according to BT.601 [3] */
    VCENC_RGBTOYUV_BT601_FULL_RANGE = 4,
    /** Color conversion of limited range[0,219] according to BT.601 [3] */
    VCENC_RGBTOYUV_BT601_LIMITED_RANGE = 5,
    /** Color conversion of full range[0,255] according to BT.709 [4] */
    VCENC_RGBTOYUV_BT709_FULL_RANGE = 6
} VCEncColorConversionType;

/** Picture type for encoding */
typedef enum {
    /** The picture should be INTRA coded. */
    VCENC_INTRA_FRAME = 0,
    /** The picture should be INTER coded using the previous picture as a predictor. */
    VCENC_PREDICTED_FRAME = 1,
    /** The picture should be INTER coded using the backward and/or forward pictures as predictors. */
    VCENC_BIDIR_PREDICTED_FRAME = 2,
    /** Used just as a return value */
    VCENC_NOTCODED_FRAME
} VCEncPictureCodingType;

/** AV1 frame type for encoding */
typedef enum {
    VCENC_AV1_KEY_FRAME = 0,
    VCENC_AV1_INTER_FRAME = 1,
    VCENC_AV1_INTRA_ONLY_FRAME = 2, // replaces intra-only
    VCENC_AV1_S_FRAME = 3,
    VCENC_AV1_FRAME_TYPES,
} VCENC_AV1_FRAME_TYPE;

/** HDR10, transfer funtion */
typedef enum {
    VCENC_HDR10_BT2020 = 14,
    VCENC_HDR10_ST2084 = 16,
    VCENC_HDR10_STDB67 = 18,
} VCEncHDRTransferType;

/** Chroma Idc type for encoding */
typedef enum {
    VCENC_CHROMA_IDC_400 = 0,
    VCENC_CHROMA_IDC_420 = 1,
    VCENC_CHROMA_IDC_422 = 2,
} VCEncChromaIdcType;

/** Tune video quality for different target. */
typedef enum {
    /** tune psnr:   aq_mode=0, psyFactor=0 */
    VCENC_TUNE_PSNR = 0,
    /** tune ssim:   aq_mode=2, psyFactor=0 */
    VCENC_TUNE_SSIM = 1,
    /** tune visual: aq_mode=2, psyFactor=0.75 */
    VCENC_TUNE_VISUAL = 2,
    /** tune sharpness visual: aq_mode=2, psyFactor=0.75, inLoopDSRatio=0, enableRdoQuant=0 */
    VCENC_TUNE_SHARP_VISUAL = 3
} VCENC_TuneType;

/** for AV1 */
typedef enum {
    /** update Key REF */
    KF_UPDATE = 0,
    /** update LAST/LAST2/LAST3 REF, after encoded , -----> LAST */
    LF_UPDATE = 1,
    /** update GOLDREF & LAST,       after encoded , -----> LAST/GOLD REF            */
    GF_UPDATE = 2,
    /** update ALTREF,               after encoded , -----> ARF                      */
    ARF_UPDATE = 3,
    /** update ALTREF2,              after encoded , -----> ARF2                     */
    ARF2_UPDATE = 4,
    /** update BWD,                  after encoded , -----> BWD                      */
    BRF_UPDATE = 5,
    /** common Frame, not update any reference idx                                   */
    COMMON_NO_UPDATE = 6,
    /** Last Frame just before BWD in the poc order, after encoded , BWD------> LAST */
    BWD_TO_LAST_UPDATE = 7,
    /** Last Frame just before ARF in the poc order, after encoded , ARF------> LAST */
    ARF_TO_LAST_UPDATE = 8,
    /** Last Frame just before ARF2 in the poc order,after encoded , ARF2-----> LAST */
    ARF2_TO_LAST_UPDATE = 9,
    /** Used just as a return value */
    FRAME_UPDATE_TYPES = 10
} VCENC_FrameUpdateType;

/*------------------------------------------------------------------------------
      3. Structures for API function parameters
  ------------------------------------------------------------------------------*/
/** \brief For 1pass agop */
typedef struct {
    int gop_frm_num;
    double sum_intra_vs_interskip;
    double sum_skip_vs_interskip;
    double sum_intra_vs_interskipP;
    double sum_intra_vs_interskipB;
    int sum_costP;
    int sum_costB;
    int last_gopsize;
} VCENCAdapGopCtr;

/** \brief Contains the Coding Unit (CU) information of a picture output by hardware. */
typedef struct {
    /** \brief Point to a memory containing the total CU number by the end of each CTU. */
    u32 *ctuOffset;
    /** \brief CU information of a picture output by HW. */
    u8 *cuData;
} VCEncCuOutData;

/** \brief Contains the motion information for the INTER Coding Unit. */
typedef struct {
    u8 refIdx; /**< \brief Reference idx in reference list */
    i16 mvX;   /**< \brief Horiazontal motion in 1/4 pixel */
    i16 mvY;   /**< \brief Vertical motion in 1/4 pixel */
} VCEncMv;

/** \brief T35 */
typedef struct {
    u8 t35_enable;
    u8 t35_country_code;
    u8 t35_country_code_extension_byte;
    const u8 *p_t35_payload_bytes;
    u32 t35_payload_size;
} ITU_T_T35;

/** \brief HDR10 */
typedef struct {
    u8 hdr10_display_enable;
    u16 hdr10_dx0;
    u16 hdr10_dy0;
    u16 hdr10_dx1;
    u16 hdr10_dy1;
    u16 hdr10_dx2;
    u16 hdr10_dy2;
    u16 hdr10_wx;
    u16 hdr10_wy;
    u32 hdr10_maxluma;
    u32 hdr10_minluma;
} Hdr10DisplaySei;

/**\brief  HDR10 Light Level */
typedef struct {
    u8 hdr10_lightlevel_enable;
    u16 hdr10_maxlight;
    u16 hdr10_avglight;
} Hdr10LightLevelSei;

/**
 * \brief Color description in the vui which coded in the sps(sequence parameter sets).
 * only valid when video signal type present flag in the vui is set.
 */
typedef struct {
    u8 vuiColorDescripPresentFlag; /**< \brief Color description present in the vui.0- not present, 1- present */
    u8 vuiColorPrimaries;          /**< \brief Color's Primaries */
    u8 vuiTransferCharacteristics; /**< \brief Transfer Characteristics */
    u8 vuiMatrixCoefficients;      /**< \brief Matrix Coefficients */
} VuiColorDescription;

/**
   * \brief CU Statistic Information output by Encoder. Different version and formats are supported.
   * Can be enable or disable by set VCEncConfig.enableOutputCuInfo. Only valid field will
   * be parsed and fill into this structure.
   * \n See \ref VCEncGetCuInfo()
   * \n See \ref VCEncConfig.enableOutputCuInfo
   */
typedef struct {
    u8 cuLocationX;       /**< \brief cu x coordinate relative to CTU */
    u8 cuLocationY;       /**< \brief cu y coordinate relative to CTU */
    u8 cuSize;            /**< \brief cu size. 8/16/32/64 */
    u8 cuMode;            /**< \brief cu mode. 0:INTER; 1:INTRA; 2:IPCM */
    u32 cost;             /**< \brief sse cost of cuMode*/
    u32 costOfOtherMode;  /**< \brief sse cost of the other cuMode */
    u32 costIntraSatd;    /**< \brief satd cost of intra mode */
    u32 costInterSatd;    /**< \brief satd cost of inter mode */
    u8 interPredIdc;      /**< \brief only for INTER cu. prediction direction.
                             * 0: by list0; 1: by list1; 2: bi-direction */
    VCEncMv mv[2];        /**< \brief only for INTER cu. motion information.
                             * mv[0] for list0 if it's valid; mv[1] for list1 if it's valid */
    u8 intraPartMode;     /**< \brief only for INTRA cu. partition mode. 0:2Nx2N; 1: NxN */
    u8 intraPredMode[16]; /**< \brief only for INTRA CU. prediction mode.
                             * 0: planar; 1: DC; 2-34: Angular in HEVC spec.
                             * intraPredMode[1~3] only valid for NxN Partition mode.
                             * intra y_mode in AV1 spec. */
    u8 qp;
    u32 mean;
    u32 variance;
} VCEncCuInfo;

typedef enum { NO_RE_ENCODE, NEW_QP = 1, NEW_OUTPUT_BUFFER = 2 } ReEncodeStrategy;

typedef struct {
    const void *kEwl;
    u32 avg_qp_y;
    u32 frame_target_bits;
    u32 frame_real_bits;
    double psnr_y;
    /** Any other useful statistic */
    u32 header_stream_byte;
    const void *kOutputbufferMem[MAX_STRM_BUF_NUM];
    u32 output_buffer_over_flow;

    /* store some parameters that needs to be reset when re-encode */
    u32 POCtobeDisplayAV1;
} VCEncStatisticOut;

typedef struct {
    ReEncodeStrategy strategy; /**< \brief 1-adjust qp 2-adjust output buffer*/
    EWLLinearMem_t output_buffer_mem[MAX_STRM_BUF_NUM];
    u32 qp;
    u32 try_counter;
} NewEncodeParams;

/**
 * \brief Callback function used as the argument of callback function. The callback is called
 * when occue OUTPUT_BUFFER_OVERFOLW after VCEncCollectEncodeStats that will collects some encode parameters needed.
 * 0-didn't re-encode
 * 1-adjust qp
 * 2-adjust output buffer
 */
typedef ReEncodeStrategy (*VCEncTryNewParamsCallBackFunc)(const VCEncStatisticOut *stat,
                                                          NewEncodeParams *new_params);

/**
 * \brief Configuration info for initialization
 * Width and height are picture dimensions after rotation
 * Width and height are restricted by level limitations
 */
typedef struct {
    VCEncStreamType streamType; /**< \brief Byte stream / Plain NAL units */

    VCEncProfile profile; /**< \brief Main, Main Still or main10 */

    /** \brief Level encoded in to the SPS. When set as zero, will select
   *         automatically. See \ref ss_autoLevel. */
    VCEncLevel level;

    VCEncTier tier; /**< \brief Main Tier or High Tier */

    u32 width;  /**< \brief Encoded picture width in pixels, multiple of 2 */
    u32 height; /**< \brief Encoded picture height in pixels, multiple of 2 */

    /** \brief The stream time scale, [1..1048575] */
    u32 frameRateNum;
    /** \brief Maximum frame rate is frameRateNum/frameRateDenom in frames/second.
   *     The actual frame rate will be defined by timeIncrement of encoded pictures,
   *     [1..frameRateNum] */
    u32 frameRateDenom;

    /** \brief Amount of reference frame buffers, [0..8]
   *\n 0 = only I frames are encoded.
   *\n 1 = gop size is 1 and interlacedFrame =0,
   *\n 2 = gop size is 1 and interlacedFrame =1,
   *\n 2 = gop size is 2 or 3,
   *\n 3 = gop size is 4,5,6, or 7,
   *\n 4 = gop size is 8
   *\n 8 = gop size is 8 svct hierach 7B+1p 4 layer, only libva support this config */
    u32 refFrameAmount;
    u32 strongIntraSmoothing; /**< \brief 0 = Normal smoothing, 1 = Strong smoothing. */

    /** \brief Enable/Disable Embedded Compression. See \ref ss_rfc
   *\n 0 = Disable Compression
   *\n 1 = Only Enable Luma Compression (not support)
   *\n 2 = Only Enable Chroma Compression (not support)
   *\n 3 = Enable Both Luma and Chroma Compression */
    u32 compressor;
    /** \brief Only HEVC supported interlace encoding by insert proper SEI information. See \ref ss_interlace
      * \n 0 = progressive frame; 1 = interlace frame
      */
    u32 interlacedFrame;

    u32 bitDepthLuma; /**< \brief Luma sample bit depth of encoded bit stream, 8 = 8 bits, 10 = 10 bits  */
    u32 bitDepthChroma; /**< \brief Chroma sample bit depth of encoded bit stream,  8 = 8 bits, 10 = 10 bits */

    /** \brief Control CU information dumping, 1: Enable, 0: Disable. See \ref ss_outCuInfo */
    u32 enableOutputCuInfo;

    u32 enableOutputCtbBits; /**< \brief 1 to enable CTB bits output. See \ref ss_outCtbBits */
    u32 enableSsim;          /**< \brief Enable/Disable SSIM calculation */
    u32 enablePsnr;          /**< \brief Enable/Disable PSNR calculation */
    u32 maxTLayers;          /**< \brief Max number Temporal layers*/

    /** \brief Control RDO hw runtime effort level, balance between quality and throughput
      * performance, [0..2]. See \ref ss_rdolevel
      *\n 0 = RDO run 1x candidates
      *\n 1 = RDO run 2x candidates
      *\n 2 = RDO run 3x candidates
      */
    u32 rdoLevel;
    /** \brief Video Codec Format: HEVC/H264/AV1 */
    VCEncVideoCodecFormat codecFormat;

    u32 verbose;                /**< \brief Log printing mode */
    u32 exp_of_input_alignment; /**< \brief Indicates the encoder's alignment,except input/ref frame*/
    u32 exp_of_ref_alignment;   /**< \brief Just affect luma reference frame config*/
    u32 exp_of_ref_ch_alignment;      /**< \brief Just affect chroma reference frame config*/
    u32 exp_of_aqinfo_alignment;      /**< \brief Just affect aqinfo output config*/
    u32 exp_of_tile_stream_alignment; /**< \brief Just affect tile stream config*/
    u32 exteralReconAlloc;            /**< \brief 0 = recon frame is allocated by encoder itself
                              1 = recon frame is allocated by APP*/
    u32 P010RefEnable; /**< \brief Enable P010 tile-raster format for reference frame buffer */

    /** \brief CTB QP adjustment mode for Rate Control and Subjective Quality.
    *\n 0 = No CTB QP adjustment.
    *\n 1 = CTB QP adjustment for Subjective Quality only.
    *\n 2 = CTB QP adjustment for Rate Control only.
    *\n 3 = CTB QP adjustment for both Subjective Quality and Rate Control. */
    u32 ctbRcMode;
    u32 picOrderCntType;
    u32 log2MaxPicOrderCntLsb;
    u32 log2MaxFrameNum;

    u32 dumpRegister; /**< \brief Enable dump register values for debug */
    u32 dumpCuInfo; /**< \brief Enable dump the Cu Information after encoding one frame for debug */
    u32 dumpCtbBits; /**< \brief Enable dump the CTB encoded bits information after encoding one frame */
    u32 rasterscan; /**< \brief Enable raster recon yuv dump on FPGA & HW for debug */

    u32 parallelCoreNum; /**< \brief Num of Core used by one encoder instance */

    u32 pass; /**< \brief Multipass coding config, currently only for internal test purpose */
    bool bPass1AdaptiveGop;
    u8 lookaheadDepth;
    u32 extDSRatio; /**< \brief External downsample ratio for first pass. 0=1/1 (no downsample), 1=1/2 downsample */
    u32 inLoopDSRatio; /**< \brief In-loop downsample ratio for first pass. 0=1/1 (no downsample), 1=1/2 downsample */
    i32 cuInfoVersion; /**< \brief CuInfo version; -1: decide by VC8000E and HW support; 0,1,2: different cuInfo version */
    u32 gopSize; /**< \brief Sequence level GOP size, set gopSize=0 for adaptive GOP size. */

    /** \brief Slice node */
    u32 slice_idx;

    /** \brief External SRAM*/
    u32 extSramLumHeightBwd;
    u32 extSramChrHeightBwd;
    u32 extSramLumHeightFwd;
    u32 extSramChrHeightFwd;

    /* AXI alignment */
    u64 AXIAlignment; /**< \brief swreg367 bit[29:26] AXI_burst_align_wr_cuinfo
                       *\n swreg320 bit[31:28] AXI_burst_align_wr_common
                       *\n swreg320 bit[27:24] AXI_burst_align_wr_stream
                       *\n swreg320 bit[23:20] AXI_burst_align_wr_chroma_ref
                       *\n swreg320 bit[19:16] AXI_burst_align_wr_luma_ref
                       *\n swreg320 bit[15:12] AXI_burst_align_rd_common
                       *\n swreg320 bit[11: 8] AXI_burst_align_rd_prp
                       *\n swreg320 bit[ 7: 4] AXI_burst_align_rd_ch_ref_prefetch
                       *\n swreg320 bit[ 3: 0] AXI_burst_align_rd_lu_ref_prefetch */

    /* Irq Type Mask */
    u32 irqTypeMask; /**< \brief swreg1 bit24 irq_type_sw_reset_mask,
                       *\n swreg1 bit23 irq_type_fuse_error_mask,
                       *\n swreg1 bit22 irq_type_buffer_full_mask,
                       *\n swreg1 bit21 irq_type_bus_error_mask,
                       *\n swreg1 bit20 irq_type_timeout_mask,
                       *\n swreg1 bit19 irq_type_strm_segment_mask,
                       *\n swreg1 bit18 irq_type_line_buffer_mask,
                       *\n swreg1 bit17 irq_type_slice_rdy_mask,
                       *\n swreg1 bit16 irq_type_frame_rdy_mask. */

    /* Irq Type Cutree Mask */
    u32 irqTypeCutreeMask; /**<  \brief        irq_type_sw_reset_mask,
                             *\n                irq_type_fuse_error_mask,
                             *\n                irq_type_buffer_full_mask,
                             *\n Imswreg1 bit21 irq_type_bus_error_mask,
                             *\n Imswreg1 bit20 irq_type_timeout_mask,
                             *\n                irq_type_strm_segment_mask,
                             *\n                irq_type_line_buffer_mask,
                             *\n                irq_type_slice_rdy_mask,
                             *\n Imswreg1 bit16 irq_type_frame_rdy_mask. */

    VCEncChromaIdcType codedChromaIdc; /**< \brief Select the chroma_format_idc coded in SPS.
                                              0:YUV400, 1:YUV420. See \ref ss_chmfmt */
    /**< \brief 0..9 Tune psnr/ssim/visual/sharpness_visual by forcing some encoding
                   parameters. Default 0. See \ref ss_tune
     \n 0 = tune psnr:   aq_mode=0, psyFactor=0
     \n 1 = tune ssim:   aq_mode=2, psyFactor=0
     \n 2 = tune visual: aq_mode=2, psyFactor=0.75
     \n 3 = tune sharpness visual: aq_mode=2, psyFactor=0.75, inLoopDSRatio=0, enableRdoQuant=0
     \n 4~9 = reserved */
    VCENC_TuneType tune;

    u32 writeReconToDDR; /**< \brief HW write recon to DDR or not if it's pure I-frame encoding*/

    u32 TxTypeSearchEnable; /**< \brief Av1 tx type search 1=enable 0=disable, enabled by default*/
    u32 av1InterFiltSwitch; /**< \brief Av1 interp filter switchable. 1=enable 0=disable, enabled by default*/

    u32 fileListExist; /**< \brief File list exist flag. 1=exist 0=not exist*/

    u32 burstMaxLength; /**< \brief AXI max burst length */

    u32 enableTMVP;

    u32 refRingBufEnable;
    /* tile */
    i32 tiles_enabled_flag;
    i32 num_tile_columns;
    i32 num_tile_rows;
    i32 loop_filter_across_tiles_enabled_flag;
    i32 *tile_width;
    i32 *tile_height;
    i32 tileMvConstraint;

    u32 bIOBufferBinding; /**< \brief If bind input buffer and output buffer 1->bind, 0 notbind*/

    /** \brief Callback to get new encode parameters when reEncode */
    VCEncTryNewParamsCallBackFunc cb_try_new_params;
} VCEncConfig;

/** \brief Contains one reference picture.s */
typedef struct {
    i32 ref_pic; /**< \brief  Delta_poc(Picture Order Count) of this short reference picture relative to the poc of current picture or index of LTR */
    u32 used_by_cur; /**< \brief  whether this reference picture used by current picture (1) or not (0). */
} VCEncGopPicRps;

/**
   * \brief The Reference description for one frame in a GOP Structure Configure
   */
typedef struct {
    u32 poc;      /**< \brief  Picture order count(POC) within a GOP, ranging from 1 to gopSize. */
    i32 QpOffset; /**< \brief  Quantization Parameter(QP) offset  */
    double QpFactor;                   /**< \brief  QP Factor  */
    i32 temporalId;                    /**< \brief  Temporal layer ID */
    VCEncPictureCodingType codingType; /**< \brief  Picture coding type  */
    u32 nonReference;                  /**< \brief  Frame not used for future reference */
    u32 numRefPics; /**< \brief  The number of reference pictures kept for this picture, the value should be within [0, VCENC_MAX_REF_FRAMES] */
    /** \brief  Short-term reference picture sets for this picture*/
    VCEncGopPicRps refPics[VCENC_MAX_REF_FRAMES];
} VCEncGopPicConfig;

/**
   * \brief The Reference description for one frame in a GOP Special Structure Configure
   */
typedef struct {
    u32 poc;                           /**< \brief Picture order count within a GOP  */
    i32 QpOffset;                      /**< \brief  QP offset  */
    double QpFactor;                   /**< \brief  QP Factor  */
    i32 temporalId;                    /**< \brief  Temporal layer ID */
    VCEncPictureCodingType codingType; /**< \brief  Picture coding type  */
    u32 nonReference;                  /**< \brief  Frame not used for future reference */
    /** \brief  The number of reference pictures kept for this picture, the value should be within [0, VCENC_MAX_REF_FRAMES] */
    u32 numRefPics;
    /** \brief Sshort-term reference picture sets for this picture*/
    VCEncGopPicRps refPics[VCENC_MAX_REF_FRAMES];
    i32 i32Ltr; /**< \brief Index of the long-term referencr frame,the value should be within [0, VCENC_MAX_LT_REF_FRAMES]. 0 -- not LTR; 1...VCENC_MAX_LT_REF_FRAMES--- LTR's index */
    i32 i32Offset;   /**< \brief Offset of the special pics, relative to start of ltrInterval*/
    i32 i32Interval; /**< \brief Interval between two pictures using LTR as referencr picture or  interval between two pictures coded as special frame */
    i32 i32short_change; /**< \brief Only change short term coding parameter. 0 - not change, 1- change */
} VCEncGopPicSpecialConfig;

/** \brief Configure the sequence property of one encoder instance. */
typedef struct {
    /** \brief Pointer to an array containing all used VCEncGopPicConfig */
    VCEncGopPicConfig *pGopPicCfg;
    u8 size; /**< \brief The number of VCEncGopPicConfig pointed by pGopPicCfg, the value should be within [0, MAX_GOP_PIC_CONFIG_NUM] */
    u8 id; /**< \brief The index of VCEncGopPicConfig in pGopPicCfg used by current picture, the value should be within [0, size-1] */
    u8 id_next; /**< \brief The index of VCEncGopPicConfig in pGopPicCfg used by next picture, the value should be within [0, size-1] */
    u8 special_size; /**< \brief Number of special configuration, the value should be within [0, MAX_GOP_PIC_CONFIG_NUM] */
    i32 delta_poc_to_next; /**< \brief The difference between poc of next picture and current picture */
    /** \brief Pointer to an array containing all used VCEncGopPicSpecialConfig */
    VCEncGopPicSpecialConfig *pGopPicSpecialCfg;
    u8 ltrcnt; /**< \brief Number of long-term ref pics used,the value should be within [0, VCENC_MAX_LT_REF_FRAMES]  */
    u32 u32LTR_idx[VCENC_MAX_LT_REF_FRAMES]; /**< \brief LTR's INDEX */
    i32 idr_interval;                        /**< \brief idr interval */
    i32 firstPic;                            /**< \brief First picture of input file to encode. */
    i32 lastPic;                             /**< \brief Last picture of input file to encode. */
    i32 outputRateNumer;                     /**< \brief Output frame rate numerator */
    i32 outputRateDenom;                     /**< \brief Output frame rate denominator */
    i32 inputRateNumer;                      /**< \brief Input frame rate numerator */
    i32 inputRateDenom;                      /**< \brief Input frame rate denominator */
    i32 gopLowdelay;

    /** \brief interlace frame will be encoded. for HEVC gopSize=1 only now. See \ref ss_interlace */
    i32 interlacedFrame;

    /** \brief Specify the index of different GOP size. */
    u8 gopCfgOffset[MAX_GOP_SIZE + 1];
    /** \brief Pointer to an array containing all used VCEncGopPicConfig, used for pass1 */
    VCEncGopPicConfig *pGopPicCfgPass1;
    /** \brief Pointer to an array containing all used VCEncGopPicConfig, used for pass2 */
    VCEncGopPicConfig *pGopPicCfgPass2;
} VCEncGopConfig;

/** \brief Defining rectangular macroblock area in encoder picture */
typedef struct {
    u32 enable; /**< \brief [0,1] Enables this area */
    u32 top;    /**< \brief Top macroblock row inside area [0..heightMbs-1] */
    u32 left;   /**< \brief Left macroblock row inside area [0..widthMbs-1] */
    u32 bottom; /**< \brief Bottom macroblock row inside area [top..heightMbs-1] */
    u32 right;  /**< \brief Right macroblock row inside area [left..widthMbs-1] */
} VCEncPictureArea;

/** \brief Defining overlay block area in encoder picture. See \ref ss_osd */
typedef struct {
    /** \brief [0, 1] Enables this area */
    u32 enable;
    /** \brief Input format for this osd area
    *\n 0: ARGB8888
    *\n 1: NV12
    *\n 2: BITMAP
    */
    u32 format;
    /** \brief Global alpha value for this osd area, not used for ARGB8888*/
    u32 alpha;
    /** \brief Top left column offset of this osd area */
    u32 xoffset;
    /** \brief Osd input cropping top left column offset */
    u32 cropXoffset;
    /** \brief Top left row offset of this osd area */
    u32 yoffset;
    /** \brief Osd input cropping top left row offset */
    u32 cropYoffset;
    /** \brief Input source width of this osd area */
    u32 width;
    /** \brief Cropping width of this osd area(final used width) */
    u32 cropWidth;
    /** \brief Osd upscale width(Only for special useage) */
    u32 scaleWidth;
    /** \brief Input source height of this osd area */
    u32 height;
    /** \brief Cropping height of this osd area(final used height) */
    u32 cropHeight;
    /** \brief Osd upscale height(Only for special useage) */
    u32 scaleHeight;
    /** \brief Luma stride of input and to store for this osd area */
    u32 Ystride;
    /** \brief Chroma stride of input and to store for this osd area */
    u32 UVstride;
    /** \brief Global luma Y value for bitmap format */
    u32 bitmapY;
    /** \brief Global chroma cb value for bitmap format */
    u32 bitmapU;
    /** \brief Global chroma cr value for bitmap format */
    u32 bitmapV;
    /** \brief Is input in super tile ordered. Only support for ARGB8888 format
    *\n 0: not superTile format
    *\n 1: x marjor superTile format
    *\n 2: y major superTile format
    */
    u32 superTile;
} VCEncOverlayArea;

/** \brief Stream Control Parameters */
typedef struct {
    u32 width;  /**< \brief Width of new stream, must smaller than initial config value */
    u32 height; /**< \brief Height of new stream, must smaller than initial config value */
} VCEncStrmCtrl;

/** \brief Coding control parameters */
typedef struct {
    u32 sliceSize;         /**< \brief Slice size in CTB/MB rows,
                              * when the value is 0, whole frame will be encoded as one slice,
                              * otherwise (height+ctu_size-1)/ctu_size slices will be created.
                              * The height of one slice will be ctu_size*sliceSize except
                              * the last slice.
                              */
    u32 seiMessages;       /**< \brief Enable insert picture timing and buffering
                          * period SEI messages into the stream, [0,1] */
    u32 vuiVideoFullRange; /**< \brief  Input video signal sample range, [0,1]
                              * 0 = Y range in [16..235], Cb&Cr range in [16..240];
                              * 1 = Y, Cb and Cr range in [0..255]
                              */
    /** \brief 0 = Filter enabled,
      *\n 1 = Filter disabled,
      *\n 2 = Filter disabled on slice edges. */
    u32 disableDeblockingFilter;
    /** \brief  Sram power down mode disable
      *\n 0 = go into power down mode when clock is gated
      *\n 1 = disable; power always on */
    u32 sramPowerdownDisable;

    i32 tc_Offset; /**< \brief Deblock parameter, tc_offset */

    i32 beta_Offset; /**< \brief Deblock parameter, beta_offset */

    /** \brief Flag to indicate whether enable deblock override between frames.
       * when this flag is 1, the deblockOverride can be enable, and then deblock parameters can be
       * changed for each frame.
       * For HEVC, it will be used to generate pps.deblocking_filter_override_enabled_flag.
       * For H264, it must be "1" for H264 haven't such syntax in pps.
       **/
    u32 enableDeblockOverride;

    /** \brief Flag to indicate whether deblock override between slice valid for HEVC, corresponding to
      * deblocking_filter_override_flag syntax in slice header. Valid only when enable DeblockOverride
      * is 1. When set as 1, the deblocking _filter_disabled_flag (for HEVC), tc_offset and beta_offset
      * will be included in the slice hader. When set as 0, the values in pps will be used.
      */
    u32 deblockOverride;

    u32 enableSao; /**< \brief Enable SAO */

    u32 enableScalingList; /**< \brief Enabled ScalingList */

    u32 sampleAspectRatioWidth;  /**< \brief Horizontal size of the sample aspect
                    * ratio (in arbitrary units), 0 for unspecified, [0..65535]
                    */
    u32 sampleAspectRatioHeight; /**< \brief Vertical size of the sample aspect ratio
                      * (in same units as sampleAspectRatioWidth)
                      * 0 for unspecified, [0..65535]
                      */
    u32 enableCabac;   /**< \brief [0,1] H.264 entropy coding mode, 0 for CAVLC, 1 for CABAC */
    u32 cabacInitFlag; /**< \brief [0,1] CABAC table initial flag */
    u32 cirStart;      /**< \brief [0..mbTotal] First macroblock for Cyclic Intra Refresh */
    u32 cirInterval; /**< \brief [0..mbTotal] Macroblock interval for Cyclic Intra Refresh, 0=disabled */

    /** \brief Enable PCM encoding.
      * PCM encoding can be lossless. It is specified in sps. */
    /* TODO: move it to VCEncConfig */
    i32 pcm_enabled_flag;
    /** \brief Disable deblock filter for IPCM, 0=enabled, 1=disabled.
      * Enable this flag will disable the deblock filter around the PCM block. This
      * can keep the lossless content in the block unchanged by the filters. */
    /* TODO: move it to VCEncConfig */
    i32 pcm_loop_filter_disabled_flag;
    VCEncPictureArea intraArea; /**< \brief Area for forcing intra macroblocks */
    /** \brief 1st Area for forcing IPCM macroblocks */
    VCEncPictureArea ipcm1Area;
    /** \brief 2nd Area for forcing IPCM macroblocks */
    VCEncPictureArea ipcm2Area;
    /** \brief 3rd Area for forcing IPCM macroblocks */
    VCEncPictureArea ipcm3Area;
    /** \brief 4th Area for forcing IPCM macroblocks */
    VCEncPictureArea ipcm4Area;
    /** \brief 5th Area for forcing IPCM macroblocks */
    VCEncPictureArea ipcm5Area;
    /** \brief 6th Area for forcing IPCM macroblocks */
    VCEncPictureArea ipcm6Area;
    /** \brief 7th Area for forcing IPCM macroblocks */
    VCEncPictureArea ipcm7Area;
    /** \brief 8th Area for forcing IPCM macroblocks */
    VCEncPictureArea ipcm8Area;

    /**
      * \brief Area for 1st Region-Of-Interest. ROI Area is defined by registers. The supported area number is determined by HW.
      * When roixQp is valid (in range of [0..51]), absolute QP will be used. So valid
      * absolution QP has higher priority.
      */
    VCEncPictureArea roi1Area; /* Area for 1st Region-Of-Interest */
    VCEncPictureArea roi2Area; /**< \brief Area for 2nd Region-Of-Interest */
    VCEncPictureArea roi3Area; /**< \brief Area for 3rd Region-Of-Interest */
    VCEncPictureArea roi4Area; /**< \brief Area for 4th Region-Of-Interest */
    VCEncPictureArea roi5Area; /**< \brief Area for 5th Region-Of-Interest */
    VCEncPictureArea roi6Area; /**< \brief Area for 6th Region-Of-Interest */
    VCEncPictureArea roi7Area; /**< \brief Area for 7th Region-Of-Interest */
    VCEncPictureArea roi8Area; /**< \brief Area for 8th Region-Of-Interest */
    i32 roi1DeltaQp;           /**< \brief [-30..0] QP delta value for 1st ROI */
    i32 roi2DeltaQp;           /**< \brief [-30..0] QP delta value for 2nd ROI */
    i32 roi3DeltaQp;           /**< \brief [-30..0] QP delta value for 3rd ROI */
    i32 roi4DeltaQp;           /**< \brief [-30..0] QP delta value for 4th ROI */
    i32 roi5DeltaQp;           /**< \brief [-30..0] QP delta value for 5th ROI */
    i32 roi6DeltaQp;           /**< \brief [-30..0] QP delta value for 6th ROI */
    i32 roi7DeltaQp;           /**< \brief [-30..0] QP delta value for 7th ROI */
    i32 roi8DeltaQp;           /**< \brief [-30..0] QP delta value for 8th ROI */
    i32 roi1Qp;                /**< \brief [-1..51] QP value for 1st ROI, -1 means invalid. */
    i32 roi2Qp;                /**< \brief [-1..51] QP value for 2nd ROI, -1 means invalid. */
    i32 roi3Qp;                /**< \brief [-1..51] QP value for 3rd ROI, -1 means invalid. */
    i32 roi4Qp;                /**< \brief [-1..51] QP value for 4th ROI, -1 means invalid. */
    i32 roi5Qp;                /**< \brief [-1..51] QP value for 5th ROI, -1 means invalid. */
    i32 roi6Qp;                /**< \brief [-1..51] QP value for 6th ROI, -1 means invalid. */
    i32 roi7Qp;                /**< \brief [-1..51] QP value for 7th ROI, -1 means invalid. */
    i32 roi8Qp;                /**< \brief [-1..51] QP value for 8th ROI, -1 means invalid. */

    u32 fieldOrder; /**< \brief Field order for HEVC interlaced coding, 0 = bottom field first, 1 = top field first */
    i32 chroma_qp_offset; /**< \brief [-12...12] chroma qp offset */

    u32 roiMapDeltaQpEnable; /**< \brief0 = ROI map disabled, 1 = ROI map enabled. Forbidden change between frames. */
    u32 roiMapDeltaQpBlockUnit; /**< \brief 0-64x64,1-32x32,2-16x16,3-8x8*/
    u32 ipcmMapEnable;          /**< \brief 0 = IPCM map disabled,  1 = IPCM map enabled */
    u32 RoimapCuCtrl_index_enable; /**< \brief 0 = ROI map cu ctrl index disabled,  1 = ROI map cu ctrl index enabled */
    u32 RoimapCuCtrl_enable; /**< \brief 0 = ROI map cu ctrl disabled,  1 = ROI map cu ctrl enabled. Forbidden change between frames.  */
    u32 RoimapCuCtrl_ver;    /**< \brief ROI map cu ctrl version number */
    u32 RoiQpDelta_ver;      /**< \brief QpDelta map version number */

    u32 skipMapEnable; /**< \brief 0 = SKIP map disabled, 1 = SKIP map enabled */
    u32 rdoqMapEnable; /**< \brief 0 = RDOQ map disabled, 1 = RDOQ map enabled */

    //wiener denoise parameters

    u32 noiseReductionEnable; /**< \brief 0 = disable noise reduction; 1 = enable noise reduction */
    u32 noiseLow;             /**< \brief valid value range :[1...30] , default: 10 */
    u32 firstFrameSigma;      /**< \brief valid value range :[1...30] , default :11*/

    u32 gdrDuration; /**< \brief [0...0xFFFFFFFF] how many pictures it will take to do GDR, if 0, not do GDR*/

    /* for low latency */
    u32 inputLineBufEn;         /**< \brief enable input image control signals */
    u32 inputLineBufLoopBackEn; /**< \brief input buffer loopback mode enable */
    u32 inputLineBufDepth;      /**< \brief input buffer depth in mb lines */
    u32 inputLineBufHwModeEn;   /**< \brief hw handshake*/
    u32 amountPerLoopBack;      /**< \brief Handshake sync amount for every loopback */
    EncInputLineBufCallBackFunc inputLineBufCbFunc; /**< \brief callback function */
    void *inputLineBufCbData;                       /**< \brief callback function data */
    u32 sbi_id_0;                                   /**< \brief flexa sbi id 0 */
    u32 sbi_id_1;                                   /**< \brief flexa sbi id 1 */
    u32 sbi_id_2;                                   /**< \brief flexa sbi id 2 */

    /*stream multi-segment*/
    u32 streamMultiSegmentMode; /**< \brief 0: single segment \n 1: multi-segment with no sync \n 2: multi-segment with sw handshake mode*/
    u32 streamMultiSegmentAmount; /**< \brief must be >= 2*/
    /** \brief Callback function*/
    EncStreamMultiSegCallBackFunc streamMultiSegCbFunc;
    void *streamMultiSegCbData; /**< \brief Callback data*/

    /* for smart (obsolete) */
    i32 smartModeEnable;  /* (obsolete) */
    i32 smartH264Qp;      /* (obsolete) */
    i32 smartHevcLumQp;   /* (obsolete) */
    i32 smartHevcChrQp;   /* (obsolete) */
    i32 smartH264LumDcTh; /* (obsolete) */
    i32 smartH264CbDcTh;  /*(obsolete) */
    i32 smartH264CrDcTh;  /* (obsolete) */
    /* threshold for hevc cu8x8/16x16/32x32 */
    i32 smartHevcLumDcTh[3];    /* (obsolete) */
    i32 smartHevcChrDcTh[3];    /* (obsolete) */
    i32 smartHevcLumAcNumTh[3]; /* (obsolete) */
    i32 smartHevcChrAcNumTh[3]; /* (obsolete) */
    /* back ground */
    i32 smartMeanTh[4]; /* (obsolete) */
    /* foreground/background threashold: maximum foreground pixels in background block */
    i32 smartPixNumCntTh; /* (obsolete) */

    /* for T35 */
    ITU_T_T35
    itu_t_t35; /**< \brief T35's codes and payload data information in the hevc's sei */
    /* for HDR10 */
    u32 write_once_HDR10;
    /** \brief HDR10's display information in the hevc's sei */
    Hdr10DisplaySei Hdr10Display;
    /** \brief HDR10's light level information in the hevc's sei */
    Hdr10LightLevelSei Hdr10LightLevel;

    /** \brief Color description in the vui, see \ref ss_vui */
    VuiColorDescription vuiColorDescription;
    /** \brief 0..1 video signal type present in the vui flag.default 0.
       *\n 0 -- video signal type not present in	the vui
       *\n 1 -- video signal type present in the vui
       *\n See \ref ss_vui */
    u32 vuiVideoSignalTypePresentFlag;
    /** \brief 0..5 video format in the vui. Default 5.
       *\n 0 -- component.
       *\n 1 -- PAL.
       *\n 2 -- NTSC.
       *\n 3 -- SECAM.
       *\n 4 -- MAC.
       *\n 5 -- UNDEF.
       *\n See \ref ss_vui */
    u32 vuiVideoFormat;

    u32 RpsInSliceHeader; /**< \brief Specify the RPS in slice header instead of select pre-defined RPS from sps */

    u32 enableRdoQuant; /**< \brief RDO quant enable. See \ref ss_rdoq */

    /** \brief Dynamic Rdo enable */
    u32 enableDynamicRdo;
    u32 dynamicRdoCu16Bias;
    u32 dynamicRdoCu16Factor;
    u32 dynamicRdoCu32Bias;
    u32 dynamicRdoCu32Factor;

    /** \brief Me vertical search range */
    u32 meVertSearchRange;

    u32 layerInRefIdcEnable;

    u32 aq_mode; /**< \brief Mode for Adaptive Quantization, Default none. See \ref ss_aqmode
                               * \n 0: none
                               * \n 1: uniform AQ
                               * \n 2: auto variance
                               * \n 3: auto variance with bias to dark scenes.  */

    /** \brief Reduces blocking and blurring in flat and textured areas
   *        (0 to 3.0). Default 1.00. See \ref ss_aqmode */
    float aq_strength;
    float psyFactor; /**< \brief 0..4 Weight of psycho-visual encoding. Default 0
                           * \n 0 = disable psy-rd.
                           * \n 1..4 = encode psy-rd, and set strength of psyFactor, larger favor better subjective quality. */
} VCEncCodingCtrl;

/** @} */ /* end of def api_video */

/**
   * \defgroup api_rc Video Rate Control API
   *
   * @{
   */
/** \brief Rate control parameters */
typedef struct {
    i32 crf;       /**< \brief CRF constant [0...51]*/
    u32 pictureRc; /**< \brief Adjust QP between pictures, [0,1] */
    u32 ctbRc; /**< \brief Adjust QP between Lcus, [0,3] */ //CTB_RC
    u32 blockRCSize;  /**< \brief Size of block rate control : 2=16x16,1= 32x32, 0=64x64*/
    u32 pictureSkip;  /**< \brief Allow rate control to skip pictures, [0,1] */
    i32 qpHdr;        /**< \brief QP for next encoded picture, [-1..51].
                 *\n -1 = Let rate control calculate initial QP.
                 *\n This QP is used for all pictures if
                 * HRD and pictureRc and mbRc are disabled.
                 * If HRD is enabled it may override this QP.
                 */
    u32 qpMinPB;      /**< \brief  Minimum QP for any P/B picture, [0..51] */
    u32 qpMaxPB;      /**< \brief  Maximum QP for any P/B picture, [0..51] */
    u32 qpMinI;       /**< \brief	Minimum QP for any I picture, [0..51] */
    u32 qpMaxI;       /**< \brief	Maximum QP for any I picture, [0..51] */
    u32 bitPerSecond; /**< \brief Target bitrate in bits/second, this is
                 * needed if pictureRc, mbRc, pictureSkip or
                 * hrd is enabled. Max bitrate is constrained by corresponding
                 * profile and level of current VideoFormat [10000..MaxByLevel]
                 */

    u32 cpbMaxRate;    /**< \brief Cpb buffer max bitrate in bits/second, this is
                 * needed if CPB based CBR/VBR mode
                 */
    true_e fillerData; /**< \brief enable/disable filler Data when HRD off*/
    u32 hrd;           /**< \brief	Hypothetical Reference Decoder model, [0,1]
                 * restricts the instantaneous bitrate and
                 * total bit amount of every coded picture.
                 * Enabling HRD will cause tight constrains
                 * on the operation of the rate control
                 */
    u32 hrdCpbSize;    /**< \brief Size of Coded Picture Buffer in HRD (bits)
               * Suggest to use 2x bit rate.
               */
    u32 bitrateWindow; /**< \brief	Length for Group of Pictures, indicates
                 * the distance of two intra pictures,
                 * including first intra [1..300]
                 */
    i32 intraQpDelta;  /**< \brief Intra QP delta. intraQP = QP + intraQpDelta
                 * This can be used to change the relative quality
                 * of the Intra pictures or to lower the size
                 * of Intra pictures. [-12..12]
                 */
    u32 fixedIntraQp;  /**< \brief Fixed QP value for all Intra pictures, [0..51]
                 * 0 = Rate control calculates intra QP.
                 */
    i32 bitVarRangeI;  /**< \brief (obsolete) variations over average bits per frame for I frame*/

    i32 bitVarRangeP; /**< \brief variations over average bits per frame for P frame*/

    i32 bitVarRangeB; /**< \brief variations over average bits per frame for B frame*/
    i32 tolMovingBitRate; /**< \brief tolerance of max Moving bit rate. (Limitation of instantaneous bit rate) */
    i32 monitorFrames; /*< monitor frame length for moving bit rate*/
    i32 targetPicSize;
    i32 smoothPsnrInGOP; /**< \brief  dynamic bits allocation among GOP using SSE feed back for smooth psnr, 1 - enable, 0 - disable */
    u32 u32StaticSceneIbitPercent; /**< \brief	I frame bits percent in static scene */
    u32 rcQpDeltaRange;            /**< \brief  ctb rc qp delta range */
    u32 rcBaseMBComplexity;        /**< \brief	ctb rc mb complexity base */
    i32 picQpDeltaMin;             /**< \brief minimum pic qp delta */
    i32 picQpDeltaMax;             /**< \brief maximum pic qp delta */
    i32 longTermQpDelta;           /**< \brief QP delta of the frame using long term reference */
    i32 vbr;                       /**< \brief	Variable Bit Rate Control by qpMin */
    rcMode_e rcMode;               /**< \brief	RC mode (CVBR, ABR, VBR, CBR, CRF, CQP) */
    float tolCtbRcInter;           /**< \brief Tolerance of Ctb Rate Control for INTER frames*/
    float tolCtbRcIntra;           /**< \brief Tolerance of Ctb Rate Control for INTRA frames*/
    i32 ctbRcRowQpStep; /**< \brief	max accumulated QP adjustment step per CTB Row by Ctb Rate Control.
                       QP adjustment step per CTB is ctbRcRowQpStep/ctb_per_row. */
    u32 ctbRcQpDeltaReverse; /**< \brief ctbrc qpdelta value reversed*/

    u32 frameRateNum;   /**< \brief The stream time scale, [1..1048575] */
    u32 frameRateDenom; /**< \brief Maximum frame rate is frameRateNum/frameRateDenom
                 * in frames/second. The actual frame rate will be
                 * defined by timeIncrement of encoded pictures,
                 * [1..frameRateNum] */
} VCEncRateCtrl;

/** @} */ /* end of def api_rc*/

/**
  * \addtogroup api_video
  *
  * @{
  */

/** \brief External SEI info */
typedef struct {
    u8 nalType;          /**< \brief External SEI NAL type,
              * For HEVC, this should be PREFIX_SEI_NUT or SUFFIX_SEI_NUT
              * For H264, this will always be treated as PREFIX_SEI_NUT
              */
    u8 payloadType;      /**< \brief External SEI payload type, e.g. 1 for picture timing.
              * For HEVC, please see ITU-T Rec. H.265 (11/2019), Annex D.2.1 for reference
              * For H264, please see ITU-T Rec. H.264 (06/2019), Annex D.1.1 for reference
              */
    u32 payloadDataSize; /**< \brief External SEI payload data length */
    u8 *pPayloadData;    /**< \brief External SEI payload data */
} ExternalSEI;

/** \brief Picture configs */
typedef struct {
    // GOP config related
    VCEncPictureCodingType codingType; /**< \brief Proposed picture coding type,  INTRA/PREDICTED */
    i32 poc;                           /**< \brief Picture display order count */
    VCEncGopConfig gopConfig;          /**< \brief GOP configuration*/
    i32 gopSize;                       /**< \brief Current GOP size*/
    i32 gopPicIdx; /**< \brief Encoded order count of current picture within its GOP, should be in the range of [0, gopSize-1] */
    i32 picture_cnt;          /**< \brief Encoded picture count */
    i32 last_idr_picture_cnt; /**< \brief Encoded picture count of last IDR */
    u32 bIsIDR; /**< \brief Coded current picture as IDR frame ,: false - not coded as IDR, true-coded as IDR */

    // long-term reference relatived info, filled by GenNextPicConfig
    u32 bIsPeriodUsingLTR; /**< \brief Use LTR as referencr picture periodly: false - not periodly used, true - periodly used */
    u32 bIsPeriodUpdateLTR; /**< \brief Update LTR periodly:false - not periodly update, true - periodly update */
    VCEncGopPicConfig gopCurrPicConfig; /**< \brief Current pic running configuration */
    i32 long_term_ref_pic
        [VCENC_MAX_LT_REF_FRAMES]; /**< \brief LTR's poc:-1 - not a valid poc, >= long-term referencr's poc */
    u32 bLTR_used_by_cur
        [VCENC_MAX_LT_REF_FRAMES]; /**< \brief LTR used by current picture: false- not used as reference, true - used as reference by current picture */
    u32 bLTR_need_update
        [VCENC_MAX_LT_REF_FRAMES]; /**< \brief LTR's poc should update: false - not need updated, true- need updated */
    i8 i8SpecialRpsIdx; /**< \brief Special RPS index selected by current coded picture, -1 - NOT, >= 0 - special RPS */
    i8 i8SpecialRpsIdx_next; /**< \brief Special RPS index selected by nextbeing coded picture, -1 - NOT, >= 0 - special RPS,for h264 */
    u8 u8IdxEncodedAsLTR; /**< \brief Current picture coded as LTR : 0 - not a LTR, 1... VCENC_MAX_LT_REF_FRAMES -- idx of LTR */
} VCEncPicConfig;

/** \brief Input YUV parameters and output stream buffer parameters for the second tile to the last tile when multi-tile */
typedef struct {
    ptr_t busLuma;    /**< \brief Bus address for input picture
                              * planar format: luminance component
                              * semiplanar format: luminance component
                              * interleaved format: whole picture
                              */
    ptr_t busChromaU; /**< \brief Bus address for input chrominance
                              * planar format: cb component
                              * semiplanar format: both chrominance
                              * interleaved format: not used
                              */
    ptr_t busChromaV; /**< \brief Bus address for input chrominance
                              * planar format: cr component
                              * semiplanar format: not used
                              * interleaved format: not used
                              */

    u32 *pOutBuf[MAX_STRM_BUF_NUM];    /**< \brief Pointer to output stream buffer */
    ptr_t busOutBuf[MAX_STRM_BUF_NUM]; /**< \brief Bus address of output stream buffer */
    u32 outBufSize[MAX_STRM_BUF_NUM];  /**< \brief Size of output stream buffer in bytes */
    void *cur_out_buffer
        [MAX_STRM_BUF_NUM]; /**< \brief Used to re-encode when output buffer overflow, app need to replace */
} VCEncInTileExtra;

/**
   * \brief Encoder input structure
   */
typedef struct {
    ptr_t busLuma;    /**< \brief Bus address for input picture
                              * planar format: luminance component
                              * semiplanar format: luminance component
                              * interleaved format: whole picture
                              */
    ptr_t busChromaU; /**< \brief Bus address for input chrominance
                              * planar format: cb component
                              * semiplanar format: both chrominance
                              * interleaved format: not used
                              */
    ptr_t busChromaV; /**< \brief Bus address for input chrominance
                              * planar format: cr component
                              * semiplanar format: not used
                              * interleaved format: not used
                              */
    ptr_t
        busLumaOrig; /**< \brief Bus address for original input picture when first-pass input is down-scaled
                              * planar format: luminance component
                              * semiplanar format: luminance component
                              * interleaved format: whole picture
                              */
    ptr_t
        busChromaUOrig; /**< \brief Bus address for original input chrominance when first-pass input is down-scaled
                              * planar format: cb component
                              * semiplanar format: both chrominance
                              * interleaved format: not used
                              */
    ptr_t
        busChromaVOrig; /**< \brief Bus address for original input chrominance when first-pass input is down-scaled
                              * planar format: cr component
                              * semiplanar format: not used
                              * interleaved format: not used
                              */
    u32 timeIncrement;          /**< \brief The previous picture duration in units
                              * of 1/frameRateNum. 0 for the very first picture
                              * and typically equal to frameRateDenom for the rest.
                              */
    u32 vui_timing_info_enable; /**< \brief Write VUI timing info in SPS: 1=enable, 0=disable. */
    u32 *pOutBuf[MAX_STRM_BUF_NUM];    /**< \brief Pointer to output stream buffer */
    ptr_t busOutBuf[MAX_STRM_BUF_NUM]; /**< \brief Bus address of output stream buffer */
    u32 outBufSize[MAX_STRM_BUF_NUM];  /**< \brief Size of output stream buffer in bytes */
    void *cur_out_buffer
        [MAX_STRM_BUF_NUM]; /**< \brief just as a buffer identifier to re-encode when output buffer overflow,
                                             * app needs to check this buffer to replace the output buffer maintained by app layer,
                                             * internal api shouldn't use it
                                             */
    VCEncPictureCodingType codingType; /**< \brief Proposed picture coding type, INTRA/PREDICTED */
    ptr_t
        busLumaStab; /**< \brief Bus address of next picture to stabilize (luma). See \ref ss_videoStab */

    //VCEncPicConfig picConfig;
    i32 poc;                  /**< \brief Picture display order count */
    VCEncGopConfig gopConfig; /**< \brief GOP configuration*/
    i32 gopSize;              /**< \brief current GOP size*/
    i32 gopPicIdx; /**< \brief Encoded order count of current picture within its GOP, should be in the range of [0, gopSize-1] */

    i8 *pRoiMapDelta;        /**< \brief  ROI Map address for CPU access. */
    u32 roiMapDeltaSize;     /**< \brief ROI Map size for one frame. */
    ptr_t roiMapDeltaQpAddr; /**< \brief ROI Map Address for encoder access. */

    ptr_t RoimapCuCtrlAddr;      /**< \brief Cu Ctrl Map address for encoder access */
    ptr_t RoimapCuCtrlIndexAddr; /**< \brief Cu Ctrl Index address for encoder access (obsolete) */

    ptr_t extSRAMLumBwdBase;
    ptr_t extSRAMLumFwdBase;
    ptr_t extSRAMChrBwdBase;
    ptr_t extSRAMChrFwdBase;

    ptr_t dec400LumTableBase;
    ptr_t dec400CbTableBase;
    ptr_t dec400CrTableBase;

    i32 picture_cnt;          /**< \brief Encoded picture count */
    i32 picture_cnt_next;     /**< \brief Encoded picture count */
    i32 last_idr_picture_cnt; /**< \brief Encoded picture count of last IDR */

    /* for low latency */
    u32 lineBufWrCnt; /**< \brief The number of MB lines already in input MB line buffer */
    u32 initSegNum;   /**< \brief The number of segments which input data is stored for SBI */

    /* for crc32/checksum */
    u32 hashType; /**< \brief 0=disable, 1=crc32, 2=checksum32 */

    u32 sceneChange; /**< \brief Flag to indicate that current frame is a scene change frame. See \ref sss_extern_detect */

    i16 gmv[2][2]; /**< \brief global mv */

    /* long-term reference relatived info */
    u32 bIsIDR; /**< \brief Coded current picture as IDR frame ,: false - not coded as IDR, true-coded as IDR */
    u32 bIsPeriodUsingLTR; /**< \brief Use LTR as referencr picture periodly: false - not periodly used, true - periodly used */
    u32 bIsPeriodUpdateLTR; /**< \brief Update LTR periodly:false - not periodly update, true - periodly update */
    VCEncGopPicConfig gopCurrPicConfig; /**< \brief Current pic running configuration */
    VCEncGopPicConfig gopNextPicConfig; /**< \brief Current pic running configuration */
    i32 long_term_ref_pic
        [VCENC_MAX_LT_REF_FRAMES]; /**< \brief LTR's poc:-1 - not a valid poc, >= long-term referencr's poc */
    u32 bLTR_used_by_cur
        [VCENC_MAX_LT_REF_FRAMES]; /**< \brief LTR used by current picture: false- not used as reference, true - used as reference by current picture */
    u32 bLTR_need_update
        [VCENC_MAX_LT_REF_FRAMES]; /**< \brief LTR's poc should update: false - not need updated, true- need updated */
    i8 i8SpecialRpsIdx; /**< \brief special RPS index selected by current coded picture, -1 - NOT, >= 0 - special RPS */
    i8 i8SpecialRpsIdx_next; /**< \brief special RPS index selected by nextbeing coded picture, -1 - NOT, >= 0 - special RPS,for h264 */
    u8 u8IdxEncodedAsLTR; /**< \brief current picture coded as LTR : 0 - not a LTR, 1... VCENC_MAX_LT_REF_FRAMES -- idx of LTR */

    u32 resendVPS; /**< \brief 0=NOT send VPS, 1= send VPS, see \ref ss_resendHeaders */
    u32 resendSPS; /**< \brief 0=NOT send SPS, 1= send SPS, see \ref ss_resendHeaders */
    u32 resendPPS; /**< \brief 0=NOT send PPS, 1= send PPS, see \ref ss_resendHeaders */

    u32 sendAUD; /**<\brief  0=NOT send AUD, 1= send AUD */

    u32 bSkipFrame; /**< \brief code all MB/CTB in current frame as SKIP Mode. 0=No, 1=Yes, see \ref ss_skipFrame */

    u32 flexRefsEnable; /**< \brief Enable external reference frame selection or not, see section \ref ss_flexRefs */

    u32 dec400Enable; /**< \brief 1: bypass 2: enable */
    u32 axiFEEnable; /**< \brief 0: axiFE disable   1:axiFE normal mode enable   2:axiFE bypass  3.security mode*/
    u32 apbFTEnable; /**< \brief 0: apbfilter disable   1:apbfilter enable*/

    /*  Overlay */
    ptr_t overlayInputYAddr[MAX_OVERLAY_NUM]; /**< \brief OSD input buffer luma base address*/
    ptr_t overlayInputUAddr[MAX_OVERLAY_NUM]; /**< \brief OSD input buffer chroma cb base address*/
    ptr_t overlayInputVAddr[MAX_OVERLAY_NUM]; /**< \brief OSD input buffer chroma cr base address*/
    u32 overlayEnable[MAX_OVERLAY_NUM];       /**< \brief OSD channel enable */
    u32 osdDec400Enable; /**< \brief Only for superTile format. 0: osd dec400 disable   1:osd dec400 enable*/
    ptr_t osdDec400TableBase;

    u32 externalSEICount;      /**< \brief External SEI count, see \ref ss_externSEI */
    ExternalSEI *pExternalSEI; /**< \brief External SEI info array address, see \ref ss_externSEI */

    /** \brief
     * Ctb encoded bits output enable
     */
    u32 enableOutputCtbBits;

    VCEncInTileExtra
        *tileExtra; /**< \brief Parameter for the second tile to the last tile when multi-tile.
                                  * When the busLuma of the second tile is not filled, input parameters including
                                  * busLuma, busChromaU and busChromaV for the second tile to the last tile will
                                  * be filled according to tile distribution.
                                  */
} VCEncIn;

/** \brief LTR configure structure */
typedef struct {
    int encoded_as_ltr;
    u32 bLTR_used_by_cur
        [VCENC_MAX_LT_REF_FRAMES]; /**< \brief LTR used by current picture: false- not used as reference, true - used as reference by current picture */
    u32 bLTR_need_update[VCENC_MAX_LT_REF_FRAMES];
} ltr_struct;

#define SHORT_TERM_REFERENCE 0x01
#define LONG_TERM_REFERENCE 0x02
/** \brief All physical addresses related to recon frame*/
typedef struct {
    /*recon buffer*/

    i32 poc; /**< \brief pic order count*/

    i32 frame_num;
    i32 frame_idx;
    i32 flags; /**< \brief Specify reference flags, 1: short term, 2: long term */
    i32 temporalId;
    u32 *mc_sync_va;        /**< \brief va of muti core sync word */
    ptr_t mc_sync_pa;       /**< \brief pa of muti core sync word */
    ptr_t busReconLuma;     /**< \brief Bus address for recon picture
                                * semiplanar format: luminance component
                                */
    ptr_t busReconChromaUV; /**< \brief Bus address for recon chrominance
                                * semiplanar format: both chrominance
                                */

    /* private buffers with recon frame*/
    ptr_t reconLuma_4n;           /**< \brief Bus address for 4n */
    ptr_t compressTblReconLuma;   /**< \brief Bus address for compress table */
    ptr_t compressTblReconChroma; /**< \brief Bus address for compress table  */
    ptr_t colBufferH264Recon;     /**< \brief Bus address for colomn buffer of H264 */
    ptr_t cuInfoMemRecon;         /**< \brief Bus address for cu info memory */
} VCEncReconPara;

/** \brief Structure contains parameters of H.264 */
typedef struct {
    //recon :    short or long term ref
    //ref list0 :    short or long term ref
    //ref list1 :    short or long term ref

    /** \brief Is picture an IDR picture? */
    unsigned int idr_pic_flag;
    /** \brief Is picture a reference picture? */
    unsigned int reference_pic_flag;

    /* \brief The picture identifier.
     *   Range: 0 to \f$2^{log2\_max\_frame\_num\_minus4 + 4} - 1\f$, inclusive.
     */
    // unsigned short  frame_num;

    /** \brief Slice type.
     *  \n Range: 0..2, 5..7, i.e. no switching slices.
     */
    unsigned char slice_type;
    /* \brief Same as the H.264 bitstream syntax element. */
    /*unsigned char   pic_parameter_set_id;*/
    /** \brief Same as the H.264 bitstream syntax element. */
    unsigned short idr_pic_id;

    /* \brief Specifies if
     * see \ref _VAEncPictureParameterBufferH264::num_ref_idx_l0_active_minus1 or
     * see \ref _VAEncPictureParameterBufferH264::num_ref_idx_l1_active_minus1 are
     * overridden by the values for this slice.
     */
    //unsigned char   num_ref_idx_active_override_flag;

    /** \brief Maximum reference index for reference picture list 0.
     *  Range: 0 to 31, inclusive.
     */
    unsigned char num_ref_idx_l0_active_minus1;
    /** \brief Maximum reference index for reference picture list 1.
     *  Range: 0 to 31, inclusive.
     */
    unsigned char num_ref_idx_l1_active_minus1;

    /** @name If pic_order_cnt_type == 0 */
    /**@{*/
    /** \brief The picture order count modulo MaxPicOrderCntLsb. */
    unsigned short pic_order_cnt_lsb;
    /**@}*/

} VCEncH264Para;

/** \brief A structure which contains parameters of HEVC.*/
typedef struct {
    //recon :    short or long term ref
    //ref list0 :    short or long term ref
    //ref list1 :    short or long term ref

    /** \brief Is picture an IDR picture? */
    unsigned int idr_pic_flag;
    /** \brief Is picture a reference picture? */
    unsigned int reference_pic_flag;

    /*  The picture identifier.
     *   Range: 0 to \f$2^{log2\_max\_frame\_num\_minus4 + 4} - 1\f$, inclusive.
     */
    //unsigned short  frame_num;

    /** \brief Slice type.
     *  \n Range: 0..2, 5..7, i.e. No switching slices.
     */
    unsigned char slice_type;
    /* Same as the H.264 bitstream syntax element. */
    /*unsigned char   pic_parameter_set_id;*/
    /** \brief Same as the H.264 bitstream syntax element. */
    unsigned short idr_pic_id;

    /** \brief Maximum reference index for reference picture list 0.
     *  Range: 0 to 31, inclusive.
     */
    unsigned char num_ref_idx_l0_active_minus1;
    /** \brief Maximum reference index for reference picture list 1.
     *  Range: 0 to 31, inclusive.
     */
    unsigned char num_ref_idx_l1_active_minus1;

    /** @name If pic_order_cnt_type == 0 */
    /**@{*/
    /** \brief The picture order count modulo MaxPicOrderCntLsb. */
    unsigned short pic_order_cnt_lsb;
    /**@}*/
    /** \brief slice data*/
    unsigned char rps_neg_pic_num;
    unsigned char rps_pos_pic_num;
    signed int rps_delta_poc[VCENC_MAX_REF_FRAMES];
    unsigned char rps_used_by_cur[VCENC_MAX_REF_FRAMES];

} VCEncHevcPara;

/** \brief Encoder libva private input structure */
typedef struct {
    struct node *next;
    /** \brief Parameters for recon frame. */
    VCEncReconPara recon;
    /** \brief Parameters for reference list0. */
    VCEncReconPara reflist0[2];
    /** \brief Parameters for reference list1. */
    VCEncReconPara reflist1[2];
    /** \brief Parameters for H.264 or HEVC. */
    union _params {
        VCEncH264Para h264Para;
        VCEncHevcPara hevcPara;
    } params;

} VCEncExtParaIn;

/** \brief Three regs value of encoder for adaptive gop. */
typedef struct {
    u32 intraCu8Num;     /**< \brief The number of block8x8 with type INTRA. */
    u32 skipCu8Num;      /**< \brief The number of block8x8 with type SKIP. */
    u32 PBFrame4NRdCost; /**< \brief */
} VCEncCuStatis;

/** \brief Encoder consumed buffers' bus addresses  structure */
typedef struct {
    ptr_t inputbufBusAddr;                      /**< \brief input buffer bus address*/
    ptr_t outbufBusAddr;                        /**< \brief output buffer bus address*/
    ptr_t dec400TableBusAddr;                   /**< \brief dec400 Table buffer bus address*/
    ptr_t roiMapDeltaQpBusAddr;                 /**< \brief roi Map Delta Qp bus address*/
    ptr_t roimapCuCtrlInfoBusAddr;              /**< \brief roi map CuCtrl Info bus address*/
    ptr_t overlayInputBusAddr[MAX_OVERLAY_NUM]; /**< \brief overlay Input bus address*/
} VCEncConsumedAddr;

/** \brief Output Parameter for the second tile column to the last tile column when multi-tile */
typedef struct {
    u32 streamSize; /**< \brief Size of output stream in bytes */
    /** \brief Output buffer for NAL unit sizes
     * pNaluSizeBuf[0] = NALU 0 size in bytes
     * pNaluSizeBuf[1] = NALU 1 size in bytes
     * etc
     * Zero value is written after last NALU.
     */
    u32 *pNaluSizeBuf;
    u32 numNalus;             /**< \brief Amount of NAL units */
    VCEncCuOutData cuOutData; /**< \brief Pointer for cu information output by HW */
} VCEncOutTileExtra;

/** \brief Encoder output structure */
typedef struct {
    VCEncPictureCodingType
        codingType; /**< \brief Realized picture coding type, INTRA/PREDICTED/NOTCODED */
    u32 streamSize; /**< \brief Size of output stream in bytes */

    /** \brief Output buffer for NAL unit sizes
      * pNaluSizeBuf[0] = NALU 0 size in bytes
      * pNaluSizeBuf[1] = NALU 1 size in bytes
      * etc
      * Zero value is written after last NALU.
      */
    u32 *pNaluSizeBuf;
    u32 numNalus; /**< \brief Amount of NAL units */
    /** \brief 32 bit flag used for av1/vp9 codec IVF timestamp,
      each bit indicates whether a frame
      (obu frame for av1, raw frame data for vp9) in output buffer
      is a not-show frame. 1: not show frame; 0: show frame.
      Bit 0 of av1Vp9FrameNotShow: av1/vp9 first output frame
      (obu frame for av1, raw frame data for vp9) is not-show frame;
      Bit 1 of av1Vp9FrameNotShow: av1/vp9 second output frame
      (obu frame for av1, raw frame data for vp9) is not-show frame;
      Etc
      */
    u32 av1Vp9FrameNotShow;
    u32 maxSliceStreamSize; /**< \brief max size of output slice in bytes*/

    ptr_t busScaledLuma; /**< \brief Bus address for scaled encoder picture luma   */
    u8 *scaledPicture;   /**< \brief Pointer for scaled encoder picture            */

    VCEncCuOutData cuOutData; /**< \brief Pointer for cu information output by HW */

    u16 *ctbBitsData; /**< \brief Pointer for ctb encoded bits information output by HW */

    u8 boolCurIsLongTermRef; /**< \brief Flag for current frame will be used as LongTermReference for future frame */

    double ssim
        [3]; /**< \brief Y/Cb/Cr SSIM values calculated by HW. Valid only if SSIM calculation feature is supported by HW. */

    double psnr
        [3]; /**< \brief Y/Cb/Cr PSNR values calculated by HW. Valid only if PSNR calculation feature is supported by HW. */

    double
        psnr_y; /**< \brief An approximate PSNR_Y value not including the 8 bottom lines of each CTB, HW calculate SSE, SW calculate log10f*/

    double
        psnr_y_predeb; /**< \brief PSNR_Y value (before deblock), HW calculate SSE, SW calculate log10f*/
    double
        psnr_cb_predeb; /**< \brief PSNR_CB value (before deblock), HW calculate SSE, SW calculate log10f*/
    double
        psnr_cr_predeb; /**< \brief PSNR_CR value (before deblock), HW calculate SSE, SW calculate log10f*/

    u32 sliceHeaderSize; /**< \brief Size of output slice header in multi-tile case */

    VCEncCuStatis cuStatis; /**< \brief The statistics data of cus. */

    u32 nextSceneChanged; /**< \brief Scene change detection result for next frame. Filled by
                              * stabilization module. \n See \ref sss_stab_detect */

    i32 picture_cnt; /**< \brief Encoded picture count */

    u32 PreDataSize;  /**< \brief The btye number of data in stream before HW encoding.
                          Not including the initial stream header data. */
    u32 PreNaluNum;   /**< \brief The number of "PreDataSize" NAL Unit */
    u32 PostDataSize; /**< \brief The btye number of data in stream after HW encoding.*/

    i32 poc; /**< \brief Picture display order count */

    VCEncConsumedAddr
        consumedAddr; /**< \brief Consumed bus addresses which should be put back to bufferpool after finish encoding this frame  */

    VCEncOutTileExtra *
        tileExtra; /**< \brief Parameter for the second tile column to the last tile column when multi-tile */
} VCEncOut;

/* Input pre-processing */
/** \brief RGB to YUV color conversion coeeficients, see \ref ss_csc for detail usage */
typedef struct {
    VCEncColorConversionType type;
    u16 coeffA;     /**< \brief User defined color conversion coefficient */
    u16 coeffB;     /* User defined color conversion coefficient */
    u16 coeffC;     /* User defined color conversion coefficient */
    u16 coeffE;     /* User defined color conversion coefficient */
    u16 coeffF;     /* User defined color conversion coefficient */
    u16 coeffG;     /* User defined color conversion coefficient */
    u16 coeffH;     /* User defined color conversion coefficient */
    u16 LumaOffset; /* User defined color conversion coefficient */
} VCEncColorConversion;

/* Input pre-processing */
/** \brief Input YUV parameters for the second tile to the last tile when multi-tile under the condition of several input YUV */
typedef struct {
    u32 origWidth;  /**< \brief Input camera picture width */
    u32 origHeight; /**< \brief Input camera picture height*/
    u32 xOffset;    /**< \brief Horizontal offset          */
    u32 yOffset;    /**< \brief Vertical offset            */
} VCEncPrpTileExtra;

/** \brief Pre-Processing Configure */
typedef struct {
    u32 origWidth;                 /**< \brief Input camera picture width */
    u32 origHeight;                /**< \brief Input camera picture height*/
    u32 xOffset;                   /**< \brief Horizontal offset          */
    u32 yOffset;                   /**< \brief Vertical offset            */
    VCEncPictureType inputType;    /**< \brief Input picture color format */
    VCEncPictureRotation rotation; /**< \brief Input picture rotation     */
    VCEncPictureMirror mirror;     /**< \brief Input picture mirror     */
    u32 videoStabilization;        /**< \brief Enable video stabilization: 0=disabled, 1=enabled. */
    VCEncColorConversion colorConversion; /**< \brief Define color conversion
                                                   parameters for RGB input   */

    u32 scaledWidth; /**< \brief Optional down-scaled output picture width, multiple of 4. 0=disabled. [16..width] */
    u32 scaledHeight; /**< \brief Optional down-scaled output picture height, multiple of 2. [96..height]  */

    u32 scaledOutput;       /**< \brief Enable output of down-scaled encoder picture.  */
    u32 scaledOutputFormat; /**< \brief 0:YUV422   1:YUV420SP*/

    u32 *
        virtualAddressScaledBuff; /**< \brief Virtual address of  allocated buffer in application for scaled picture.*/
    ptr_t
        busAddressScaledBuff; /**< \brief Phyical address of  allocated buffer in application for scaled picture.*/
    u32 sizeScaledBuff; /**< \brief Size of allocated buffer in application for scaled picture. The size is not less than scaledWidth*scaledOutput*2 bytes */
    /**\brief On-the-fly downscaler used for lookahead feature in first pass, only used for getting parameter in API VCEncGetPreProcessing().
     * downscale ratio is difeined as 0=1:1(no downsample), 1=1:2 (1/2 downsample) */
    u32 inLoopDSRatio;

    i32 constChromaEn; /**< \brief Constant chroma control */
    u32 constCb;
    u32 constCr;

    u32 input_alignment; /**< \brief Just affect prp config of input frame*/

    /* Overlay area */
    VCEncOverlayArea overlayArea[MAX_OVERLAY_NUM]; /**< \brief Config for each OSD channel */

    /* Mosaic region parameters */
    u32 mosEnable[MAX_MOSAIC_NUM];  /**< \brief Mosaic region enable */
    u32 mosXoffset[MAX_MOSAIC_NUM]; /**< \brief Mosaic region top left horizontal offset */
    u32 mosYoffset[MAX_MOSAIC_NUM]; /**< \brief Mosaic region top left vertical offset */
    u32 mosWidth[MAX_MOSAIC_NUM];   /**< \brief Mosaic region width */
    u32 mosHeight[MAX_MOSAIC_NUM];  /**< \brief Mosaic region height */
    VCEncPrpTileExtra *
        tileExtra; /**< \brief Input YUV parameters for the second tile to the last tile when multi-tile under the condition of several input YUV */
} VCEncPreProcessingCfg;

/**@}*/

/**
   * \defgroup api_pps Picture Parameter Set Config
   *
   * @{
   */

/** \brief A structure that contains the user-provided Picture Parameter Set (PPS) parameters. */
typedef struct {
    i32 chroma_qp_offset; /**< \brief Chroma qp offset */
    i32 tc_Offset;        /**< \brief Deblock parameter, tc_offset */
    i32 beta_Offset;      /**< \brief Ceblock parameter, beta_offset */
} VCEncPPSCfg;
/**@}*/

/**
   * \addtogroup api_video
   *
   * @{
   */
/** \brief Output stream buffers */
typedef struct {
    u8 *buf[MAX_STRM_BUF_NUM];       /**< \brief Pointers to beginning of output stream buffer. */
    u32 bufLen[MAX_STRM_BUF_NUM];    /**< \brief Length of output stream buffer */
    u32 bufOffset[MAX_STRM_BUF_NUM]; /**< \brief Offset of output stream buffer*/
} VCEncStrmBufs;

/**
   * \brief Callback struct used as the argument of callback function. See \ref ss_multislice
   */
typedef struct {
    u32 slicesReadyPrev;      /**< \brief Indicates how many slices were completed at
                               previous callback. This is given because
                               several slices can be completed between
                               the callbacks. */
    u32 slicesReady;          /**< \brief Indicates how many slices are completed. */
    u32 nalUnitInfoNum;       /**< \brief Indicates how many information nal units are
                               completed, including all kinds of sei information.*/
    u32 nalUnitInfoNumPrev;   /**< \brief Indicates how many information nal units
                               are completed at previous callback, including
                               all kinds of sei information.*/
    u32 *sliceSizes;          /**< \brief Holds the size (bytes) of every completed slice. */
    VCEncStrmBufs streamBufs; /**< \brief Output stream buffers */
    void *pAppData;           /**< \brief Pointer to application data. */
    EWLLinearMem_t *outbufMem[MAX_STRM_BUF_NUM]; /**< \brief Output memory*/
    u32 PreDataSize;    /**< \brief The btye number of data bytes in stream before HW encoding.
                          Not including the initial stream header data. */
    u32 PreNaluNum;     /**< \brief The number of "PreDataSize" NAL Unit */
    u32 streamBufChain; /**< \brief StreamBufChain flag*/
} VCEncSliceReady;

/**
   * \brief Callback function used as the argument of callback function. The callback is called
   * when one or some slices bitstream is generated by HW and SW can process its data in
   * the stream output buffer without wait whole frame encoding done.
   */
typedef void (*VCEncSliceReadyCallBackFunc)(VCEncSliceReady *sliceReady);

/** \brief Version information */
typedef struct {
    u32 major; /**< \brief Encoder API major version */
    u32 minor; /**< \brief Encoder API minor version */
    u32 clnum; /**< \brief CL# */
} VCEncApiVersion;

/** \brief Containing the encoder‘s build information. */
typedef struct {
    u32 swBuild; /**< \brief Software build ID */
    u32 hwBuild; /**< \brief Hardware build ID */
} VCEncBuild;

/** \brief A linked list linear data structure which stores encoder buffer information. */
struct vcenc_buffer {
    struct vcenc_buffer *next;
    u8 *buffer;    /**< \brief Data store */
    u32 cnt;       /**< \brief Data byte cnt */
    ptr_t busaddr; /**< \brief Data store bus address */
};

/** \brief Specifies the encoder status. */
enum VCEncStatus {
    VCENCSTAT_INIT = 0xA1,
    VCENCSTAT_START_STREAM,
    VCENCSTAT_START_FRAME,
    VCENCSTAT_ERROR
};

/** \brief Used for RPS in slice header.*/ /* begin*/
typedef struct {
    u8 u1short_term_ref_pic_set_sps_flag;
    u8 u3NegPicNum; /**< \brief  0...7, num_negative_pics ,3bits */
    u8 u3PosPicNum; /**< \brief   0...7, num_positive_pics ,3bits */
    u8 u1DeltaPocS0Used[VCENC_MAX_REF_FRAMES];
    u32 u20DeltaPocS0[VCENC_MAX_REF_FRAMES]; /**< \brief   {20'bDeltaPoc}, 20bits */
    u8 u1DeltaPocS1Used[VCENC_MAX_REF_FRAMES];
    u32 u20DeltaPocS1[VCENC_MAX_REF_FRAMES]; /**<  \brief  {20'bDeltaPoc}, 20bits */
} HWRPS_CONFIG;
/* used for RPS in slice header , end */

/*------------------------------------------------------------------------------
      4. Encoder API function prototypes
  ------------------------------------------------------------------------------*/
/**
   * Return the API version information
   *
   * \param [in] None
   * \return VCEncApiVersion
   */
VCEncApiVersion VCEncGetApiVersion(void);
/**
   * Return the SW and HW build information. Does not require encoder initialization.
   *
   * \param [in] Client_type Used to select available HW engine when initializing the EWL
   * \return VCEncBuild
   */
VCEncBuild VCEncGetBuild(u32 client_type);
u32 VCEncGetRoiMapVersion(u32 core_id, void *ctx);

/**
   * Returns the amount of bits per pixel for input picture format type.
   *
   * \param [in] Type VCEncPictureType: Input picture format type.
   * \return Number of bits per pixel.
   */
u32 VCEncGetBitsPerPixel(VCEncPictureType type);

/**
   * Returns the stride in byte by given aligment and format.
   *
   * \param [in] width	Width of the input image.
   * \param [in] input_format	Format of the input image.
   * \param [in] input_alignment Input alignment.
   * \param [out] luma_stride Luma stride.
   * \param [out] chroma_stride	 Chroma stride.
   * \return Number of bytes per pixel.
   */
u32 VCEncGetAlignedStride(int width, i32 input_format, u32 *luma_stride, u32 *chroma_stride,
                          u32 input_alignment);

/**
   * Create and Initialize Video Encoder Intstance.
   *
   * Initializes the encoder and returns an instance of it. The configuration contains stream
   * parameters that can’t be altered during encoding.
   *
   * \pre EWL layer and Kernel driver has finish porting according to your platform.
   * \post an instance will be created according to the config
   * \param [in] config Points to the VCEncConfig structure that contains the encoder’s initial
   *             configuration parameters. Generally, the configure parameter cannot be changed
   *             when encoding.
   * \param [inout] instAddr Points to a space where the new encoder instance pointer will be stored.
   * \param [in] ctx The context is initialized in application and maybe used by EWL. For example,
   *             when the device is open before initialize, the ctx can be use to transfer this
   *             data. It can be NULL if application don't use it.
   * \return VCENC_OK initialize successfully;
   * \return others see \ref VCEncRet
   */
VCEncRet VCEncInit(const VCEncConfig *config, VCEncInst *instAddr, void *ctx);

/**
   *  Releases encoder instance and all associated resource. This will free all the resources allocated at the encoder initialization phase.
   *
   * \param [in] inst The encoder instance to be released. This instance was created earlier with a call to VCEncInit().
   * \return VCENC_OK initialize successfully;
   * \return others see \ref VCEncRet
   */
VCEncRet VCEncRelease(VCEncInst inst);

/**
   * To get HW cycle number use to encode one frame.
   *
   * \param [in] inst Pointer to the encoder instance to be freed. After this the pointer is no longer valid.
   * \return    The number of clock cycles used to encode the previous frame.
   */
u32 VCEncGetPerformance(VCEncInst inst);

/**
   *  Set Encoder configuration. All the parameters can be adjusted before the stream is started. The following parameters can also
   *  be altered between frames:
   *   -   sliceSize
   *   -   cirStart
   *   -   cirInterval
   *   -   intraArea
   *   -   roiDeltaQp
   *
   *   If the following parameters need to be altered between frames,
   *   -   roiArea
   *   -   rdoqMapEnable
   *   -   gdrDuration
   *
   *   cu_qp_delta_enabled_flag in PPS will be reconfigured, so the following steps should be done:
   *   -   VCEncStrmEnd() :       to end the last sequence
   *   -   VCEncSetCodingCtrl() :  to change parameters
   *   -   VCEncStrmStart():      to output a new Picture Parameter Set (PPS)
   *
   *   then, call
   *   -   VCEncStrmEncode():      to encode the next frame
   *
   * \param [in] instAddr The instance that defines the encoder in use.
   * \param [in] *pCodeParams Pointer to the VCEncCodingCtrl structure that contains the encoder’s coding parameters.
   * \return VCENC_OK successful,
   * \return other unsuccessful.
   */
VCEncRet VCEncSetCodingCtrl(VCEncInst instAddr, const VCEncCodingCtrl *pCodeParams);
/**
   *  Get Encoder configuration before stream generation.
   *
   * \param [in] inst The instance that defines the encoder in use.
   * \param [in] *pCodeParams Pointer to the VCEncCodingCtrl structure where the encoder’s coding parameters will be saved.
   * \return VCENC_OK successful,
   * \return other unsuccessful.
   */
VCEncRet VCEncGetCodingCtrl(VCEncInst inst, VCEncCodingCtrl *pCodeParams);

/**
   * Set stream control parameters.
   *
   * The stream parameter can only change after end the previous stream or start
   * a stream.
   *
   * \param inst encoder instance.
   * \param *params stream control params.
   * \return VCENC_OK successful,
   * \return other unsuccessful.
   */
#ifndef STREAM_CTRL_BUILD_SUPPORT
#define VCEncSetStrmCtrl(inst, params) (-1)
#else
VCEncRet VCEncSetStrmCtrl(VCEncInst inst, VCEncStrmCtrl *params);
#endif

/**@}*/ /* end of addto api_video */

/**
  * \addtogroup api_rc
  *
  * @{
  */

/**
    * Set Rate Control configuration parameters
  *
	The function should be called before starting the stream.

	If the Hypothetical Reference Decoder (HRD) is disabled, the function can also be called between frames to set new rate control parameters except the ctbRc parameter. This however will reset the rate control and is recommended to do so at the end of Group of Pictures (GOP). To modify ctbRc between frames, cu_qp_delta_enabled_flag in the Picture Parameter Set (PPS) will be reconfigured, so the following steps should be done:

  -	VCEncStrmEnd(): to end the last sequence
  -	VCEncSetRateCtrl(): 	to change ctbRc
  -	VCEncStrmStart():	to output a new PPS

	Then, call
  -	VCEncStrmEncode():	to encode the next frame

	In general, ctbRc should not be changed between frames.
  * \param [in] inst the instance in use.
  * \param [in] pRateCtrl user provided parameters
  * \return VCEncRet
    */
VCEncRet VCEncSetRateCtrl(VCEncInst inst, const VCEncRateCtrl *pRateCtrl);
/**
   *  Get Rate Control configuration, return current rate control parameters.
   * \param [in] inst the instance in use.
   * \param [out] pRateCtrl Place where parameters are returned
   * \return VCEncRet
   */
VCEncRet VCEncGetRateCtrl(VCEncInst inst, VCEncRateCtrl *pRateCtrl);
/**@}*/

/**
   * \addtogroup api_video
   *
   * @{
   */

/**
   *  Set PreProcessing configuration before and during stream generation
   * \param [in] inst The instance that defines the encoder in use.
   * \param [in] pRateCtrl Pointer to the VCEncPreProcessingCfg structure where the new parameters are set.
   * \return VCEncRet
   */
VCEncRet VCEncSetPreProcessing(VCEncInst inst, const VCEncPreProcessingCfg *pPreProcCfg);

/**
   *  Get PreProcessing configuration before and during stream generation
   * \param [in] inst The instance that defines the encoder in use.
   * \param [out] pRateCtrl Pointer to the VCEncPreProcessingCfg structure where the new parameters are set.
   * \return VCEncRet
   */
VCEncRet VCEncGetPreProcessing(VCEncInst inst, VCEncPreProcessingCfg *pPreProcCfg);

/**
   * Insert User Data SEI in stream
   */
VCEncRet VCEncSetSeiUserData(VCEncInst inst, const u8 *pUserData, u32 userDataSize);

/* Stream generation API */

/**
   * Generates the SPS and PPS.
   *
   * SPS is the first NAL unit and PPS is the second NAL unit.
   *
   * \param [in] inst The instance that defines the encoder in use.
   * \param [in] pEncIn Pointer to user provided input parameters.
   * \param [out] pEncOut->NaluSizeBuf indicates the size of NAL units.
   * \return VCEncRet
   */
VCEncRet VCEncStrmStart(VCEncInst inst, const VCEncIn *pEncIn, VCEncOut *pEncOut);

/**
   * Encodes one video frame.
   *
   * If SEI messages are enabled the first NAL unit is a SEI message.
   *
   * \param [in] inst The instance that defines the encoder in use.
   * \param [in] pEncIn Pointer to user provided input parameters.
   * \param [in] sliceReadyCbFunc Pointer to the VCEncSliceReadyCallBackFunc callback function that will be called after a slice encoding has been
   *             finished by the hardware.
   * \param [in] pAppdata Pointer to application-specific data to be passed on to the callback function.
   * \param [out] pEncOut Pointer to the VCEncOut structure where the output parameters will be saved.
   * \return VCEncRet
   */
VCEncRet VCEncStrmEncode(VCEncInst inst, VCEncIn *pEncIn, VCEncOut *pEncOut,
                         VCEncSliceReadyCallBackFunc sliceReadyCbFunc, void *pAppData);

/** VCEncStrmEncode encodes one video frame. If SEI messages are enabled the
   * first NAL unit is a SEI message. When MVC mode is selected first encoded
   * frame belongs to view=0 and second encoded frame belongs to view=1 and so on.
   * When MVC mode is selected a prefix NAL unit is generated before view=0 frames.
   *
   * \param [in] inst The instance that defines the encoder in use.
   * \param [in] pEncIn Pointer to user provided input parameters.
   * \param [in] sliceReadyCbFunc Pointer to the VCEncSliceReadyCallBackFunc callback function that will be called after a slice encoding has been
   *             finished by the hardware.
   * \param [in] pAppdata Pointer to application-specific data to be passed on to the callback function.
   * \param [in] useExtFlag Specify whether to use the extra parameter or not.
   * \param [out] pEncOut Pointer to the VCEncOut structure where the output parameters will be saved.
   * \return VCEncRet
   */
VCEncRet VCEncStrmEncodeExt(VCEncInst inst, VCEncIn *pEncIn, const VCEncExtParaIn *pEncExtParaIn,
                            VCEncOut *pEncOut, VCEncSliceReadyCallBackFunc sliceReadyCbFunc,
                            void *pAppData, i32 useExtFlag);

/** Ends a stream with an EOS code.
  * \param [in] inst The instance that defines the encoder in use.
  * \param [in] pEncIn Pointer to user provided input parameters.
  * \param [out] pEncOut Pointer to the VCEncOut structure where the output parameters will be saved.
  * \return VCEncRet
  */
VCEncRet VCEncStrmEnd(VCEncInst inst, const VCEncIn *pEncIn, VCEncOut *pEncOut);

/**
   * Flushes out remaining frames after all the frames to be encoded have been send into encoder.
   *
   * \param [in] inst The instance that defines the encoder in use.
   * \param [in] pEncIn Pointer to user provided input parameters.
   * \param [in] sliceReadyCbFunc Pointer to the VCEncSliceReadyCallBackFunc callback function that will be called after a slice encoding has been
   *             finished by the hardware.
   * \param [in] pAppdata Pointer to application-specific data to be passed on to the callback function.
   * \param [out] pEncOut Pointer to the VCEncOut structure where the output parameters will be saved.
   * \return VCEncRet
   */
VCEncRet VCEncFlush(VCEncInst inst, VCEncIn *pEncIn, VCEncOut *pEncOut,
                    VCEncSliceReadyCallBackFunc sliceReadyCbFunc, void *pAppData);
/**
   * Flush remaining frames at the end of multi-core encoding.
   *
   * \param [in] inst The instance that defines the encoder in use.
   * \param [in] pEncIn Pointer to user provided input parameters.
   * \param [in] sliceReadyCbFunc Pointer to the VCEncSliceReadyCallBackFunc callback function that will be called after a slice encoding has been
   *             finished by the hardware.
   * \param [out] pEncOut Pointer to the VCEncOut structure where the output parameters will be saved.
   * \return VCEncRet
   */
VCEncRet VCEncMultiCoreFlush(VCEncInst inst, VCEncIn *pEncIn, VCEncOut *pEncOut,
                             VCEncSliceReadyCallBackFunc sliceReadyCbFunc);

/* Internal encoder testing */
/**
   * Sets the encoder configuration according to a test vector.
   *
   * \param [in] inst The instance that defines the encoder in use.
   * \param [in] testId The test vector ID.
   * \return VCEncRet
   */
VCEncRet VCEncSetTestId(VCEncInst inst, u32 testId);

/**@}*/

/**
   * \addtogroup api_pps
   *
   * @{
   */

/* PPS config*/
/**
   * create new Creates a new Picture Parameter Set (PPS).
   * \param [in] inst The instance that defines the encoder in use.
   * \param [in] pPPSCfg The user provided parameters.
   * \param [out] newPPSId The new PPS id for user.
   * \return VCEncRet
   */
VCEncRet VCEncCreateNewPPS(VCEncInst inst, const VCEncPPSCfg *pPPSCfg, i32 *newPPSId);

/**
   * Modifies an existing Picture Parameter Set (PPS).
   *
   * \param [in] inst The instance that defines the encoder in use.
   * \param [in] pPPSCfg The user provided parameters.
   * \param [in] ppsId The old PPS id provided by user.
   * \return VCEncRet
   */
VCEncRet VCEncModifyOldPPS(VCEncInst inst, const VCEncPPSCfg *pPPSCfg, i32 ppsId);

/**
   *  Get PPS parameters for user
   *
   * \param [in] inst The instance that defines the encoder in use.
   * \param [in] ppsId The PPS id provided by user.
   * \param [out] pPPSCfg Place where the parameters are returned
   * \return VCEncRet
   */
VCEncRet VCEncGetPPSData(VCEncInst inst, VCEncPPSCfg *pPPSCfg, i32 ppsId);

/**
   *  Active another PPS
   *
   * \param [in] inst The instance that defines the encoder in use.
   * \param [in] ppsId The old PPS id provided by user.
   * \return VCEncRet
   */
VCEncRet VCEncActiveAnotherPPS(VCEncInst inst, i32 ppsId);

/**
   *  Get the ID of the active Picture Parameter Set (PPS).
   *
   * \param [in] inst The instance that defines the encoder in use.The instance was created earlier with a call to VCEncInit().
   * \param [out] ppsId PPS ID returned to user.
   * \return VCEncRet
   */
VCEncRet VCEncGetActivePPSId(VCEncInst inst, i32 *ppsId);
/**@}*/

/**
   * \addtogroup api_video
   *
   * @{
   */
/* low latency */
/** Low latency:  sets the valid input MB lines for the encoder to work.
  *
  * \param [in] inst The instance that defines the encoder in use.The instance was created earlier with a call to VCEncInit().
  * \param [in] lines Valid input MB lines to set..
  * \return VCEncRet
  */
VCEncRet VCEncSetInputMBLines(VCEncInst inst, u32 lines);

/** Get encoded lines information from encoder
  *
  * \param [in] inst The instance that defines the encoder in use.
  * \return int The encoded lines information from the encoder.
  */
u32 VCEncGetEncodedMbLines(VCEncInst inst);

/**
   * Set error status for multi-pass.
   *
   * \param [in] inst The instance that defines the encoder in use.
   * \return none.
   */
void VCEncSetError(VCEncInst inst);

/*------------------------------------------------------------------------------
       API Functions only valid for HEVC.
   ------------------------------------------------------------------------------*/
/**
  * Get the encoding information of a specified CU in a specified CTU, only valid for HEVC.
  *
  * \param [in]  inst The instance that defines the encoder in use.
  * \param [in]  pEncCuOutData Structure containing ctu table and cu information stream output by HW
  * \param [in]  ctuNum Ctu number within picture
  * \param [in]  cuNum Cu number within ctu
  * \param [out]	pEncCuInfo Structure returns the parsed cu information
  * \return VCENC_INVALID_ARGUMENT invalid argument
  * \return VCENC_ERROR error version number for cu information
  * \return VCENC_OK success
  */
#ifndef CUINFO_BUILD_SUPPORT
#define VCEncGetCuInfo(inst, pEncCuOutData, ctuNum, cuNum, pEncCuInfo) (0)
#else
VCEncRet VCEncGetCuInfo(VCEncInst inst, VCEncCuOutData *pEncCuOutData, u32 ctuNum, u32 cuNum,
                        VCEncCuInfo *pEncCuInfo);
#endif

/*------------------------------------------------------------------------------
      5. Encoder API tracing callback function
  ------------------------------------------------------------------------------*/
/**
   * Encoder API tracing callback function.
   *
   * \params [in] msg Null terminated tracing message
   * \return None.
   */
void VCEncTrace(const char *msg);

/**
   * Tracing PSNR profile result.
   *
   * \param [in]  inst The instance that defines the encoder in use.
   * \return None.
   */
void VCEncTraceProfile(VCEncInst inst);

/**
   * Get HW configuration.
   *
   * \param [in]  codecFormat The codec format.
   * \return  EWLHwConfig_t structure containing the ASIC hardware configuration.
   *  Refer to the Hantro EWL API document for the definition of EWLHwConfig_t.
   */
EWLHwConfig_t VCEncGetAsicConfig(VCEncVideoCodecFormat codecFormat, void *ctx);

/** Get pass1 updated GopSize*/
i32 VCEncGetPass1UpdatedGopSize(VCEncInst inst);

/** Get encoding buffer delay */
i32 VCEncGetEncodeMaxDelay(VCEncInst inst);

/**@}*/

#ifdef __cplusplus
}
#endif

#endif
