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
--  Description : Encoder common definitions for control code and system model
--
------------------------------------------------------------------------------*/

#ifndef __ENC_COMMON_H__
#define __ENC_COMMON_H__

#ifdef __cplusplus
extern "C" {
#endif

/*------------------------------------------------------------------------------
    1. External compiler flags
------------------------------------------------------------------------------*/

/* Encoder global definitions
 *
 * _ASSERT_USED     # Asserts enabled
 * _DEBUG_PRINT     # Prints debug information on stdout
 * TEST_DATA        # Creates test data files
 *
 * Can be defined here or using compiler flags */

/*------------------------------------------------------------------------------
    2. Include headers
------------------------------------------------------------------------------*/

#include "base_type.h"
#include "ewl.h"
#include "ewl_memsync.h"

/* Stream tracing requires encdebug.h */
#if 0

#ifdef H2_HAVE_ENCTRACE_H
#include "enctrace.h"
#endif
#endif

/*------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/

typedef enum { ENCHW_NOK = -1, ENCHW_OK = 0 } bool_e;

typedef enum { ENCHW_NO = 0, ENCHW_YES = 1 } true_e;

typedef enum { VCE_RC_CVBR, VCE_RC_CBR, VCE_RC_VBR, VCE_RC_ABR, VCE_RC_CRF, VCE_RC_CQP } rcMode_e;

typedef enum {
    CU_INFO_LOC_X_BITS = 3,
    CU_INFO_LOC_Y_BITS = 3,
    CU_INFO_SIZE_BITS = 2,
    CU_INFO_MODE_BITS = 1,
    CU_INFO_COST_BITS = 25,
    CU_INFO_INTER_PRED_IDC_BITS = 2,
    CU_INFO_MV_REF_IDX_BITS = 2,
    CU_INFO_MV_X_BITS = 14,
    CU_INFO_MV_Y_BITS = 14,
    CU_INFO_INTRA_PART_BITS = 1,
    CU_INFO_INTRA_PART_BITS_H264 = 2,
    CU_INFO_INTRA_PRED_MODE_BITS = 6,
    CU_INFO_INTRA_PRED_MODE_BITS_H264 = 4,
    CU_INFO_INTRA_DUMMY_BITS = 37,
    CU_INFO_MEAN_BITS = 10,
    CU_INFO_VAR_BITS = 18,
    CU_INFO_VAR_BITS_V3 = 26,
    CU_INFO_QP_BITS = 6,
    CU_INFO_OUTPUT_SIZE = 12,    //96 bits
    CU_INFO_TABLE_ITEM_SIZE = 4, //32 bits
    CU_INFO_OUTPUT_SIZE_V1 = 26, //208 bits
    CU_INFO_OUTPUT_SIZE_V2 = 16, //128 bits
    CU_INFO_OUTPUT_SIZE_V3 = 19, //152 bits
} cuInfoBits_e;

/* H.264 collocated mb */
struct h264_mb_col {
    u8 colZeroFlagStore; // per 2 mbs, bit[0..3] for 1st, bit[4..7] for second
};

/* VLC TABLE */
typedef struct {
    i32 value;  /* Value of bits  */
    i32 number; /* Number of bits */
} table_s;

/* used in stream buffer handling */
typedef struct {
#ifdef TEST_DATA
    struct stream_trace *stream_trace;
#endif
    u8 *stream;           /* Pointer to next byte of stream */
    u32 size;             /* Byte size of stream buffer */
    u32 byteCnt;          /* Byte counter */
    u32 bitCnt;           /* Bit counter */
    u32 byteBuffer;       /* Byte buffer */
    u32 bufferedBits;     /* Amount of bits in byte buffer, [0-7] */
    u32 zeroBytes;        /* Amount of consecutive zero bytes */
    i32 overflow;         /* This will signal a buffer overflow */
    u32 emulCnt;          /* Counter for emulation_3_byte, needed in SEI */
    i32 *table;           /* Video packet or Gob sizes */
    i32 tableSize;        /* Size of above table */
    i32 tableCnt;         /* Table counter of above table */
    u32 bufferedLeftBits; /* Amount of left bits in byte buffer, [0-7] */
} stream_s;

#if 0
/* General tools */
#define ABS(x) ((x) < (0) ? -(x) : (x))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define SIGN(a) ((a) < (0) ? (-1) : (1))
#define OUT_OF_RANGE(x, a, b) ((((i32)x) < (a) ? (1) : (0)) || ((x) > (b) ? (1) : (0)))
#define CLIP3(v, min, max) ((v) < (min) ? (min) : ((v) > (max) ? (max) : (v)))
#endif

/* Encoder MB output information defined as a struct.

   NOTE!    By defining this struct we rely that ASIC output endianess
            is configured properly to produce output that is identical
            to the way the compiler constructs this struct. */

typedef struct {
    u8 mode; /* refIdx 2b, chromaIntraMode 2b, mbMode 4b */
    u8 nonZeroCnt;
    u16 bitCount;
    i8 mvY[4];
    i16 mvX[4];
    u8 intra4x4[8]; /* 4 bits per block */
    u16 intraSad;
    u16 interSad;
    u8 inputMad_div128;
    u8 yMean;
    u8 cbMean;
    u8 crMean;
    u8 boostQp;      /* HEVC CTU autoboost qp */
    u8 dummy0;       /* Dummy (not used) place holder */
    u16 varRecon[3]; /* Mb variance after reconstruction but before DB-filter */
    u8 dummy1[2];    /* Dummy (not used) place holder */
    u16 varInput[3]; /* Mb variance before encoding loop */
} encOutputMbInfo_s;

typedef struct {
    u8 mode; /* refIdx 2b, chromaIntraMode 2b, mbMode 4b */
    u8 nonZeroCnt;
    u16 bitCount;
    i8 mvY[4];
    i16 mvX[4];
    u8 intra4x4[8]; /* 4 bits per block */
    u16 intraSad;
    u16 interSad;
    u8 inputMad_div128;
    u8 yMean;
    u8 cbMean;
    u8 crMean;
    u8 boostQp;      /* HEVC CTU autoboost qp */
    u8 dummy0;       /* Dummy (not used) place holder */
    u16 varRecon[3]; /* Mb variance after reconstruction but before DB-filter */
    u8 dummy1[2];    /* Dummy (not used) place holder */
    u16 varInput[3]; /* Mb variance before encoding loop */
    u8 data[8];
} encOutputMbInfoDebug_s;

typedef struct EncOutForCutree {
    int poc;
    int qp;
    u32 codingType;
    i32 hierarchical_bit_allocation_GOP_size;
    i32 encoded_frame_number;
    u32 dsRatio;
    u32 extDSRatio;
    i32 p0;
    i32 p1;
    u32 cuDataIdx;
    u32 roiMapEnable;
    i8 *pRoiMapDelta;
    ptr_t roiMapDeltaQpAddr;
    u32 roiMapDeltaSize;
    u32 motionScore[2][2];
} encOutForCutree_s;

typedef void (*EncInputLineBufCallBackFunc)(void *pAppData);

typedef struct {
    u32 inputLineBufEn;         /* enable input image control signals */
    u32 inputLineBufLoopBackEn; /* input buffer loopback mode enable */
    u32 inputLineBufDepth;      /* input loopback buffer size in mb lines */
    u32 inputLineBufHwModeEn;   /* hw handshake mode */
    u32 amountPerLoopBack;      /* Handshake sync amount for every loopback */
    u32 wrCnt;
    EncInputLineBufCallBackFunc cbFunc; /* call back function for line buffer interrupt */
    void *cbData;                       /* call back function data for line buffer interrupt */
    u32 initSegNum; /* The number of segments which input data is stored for SBI */
    u32 sbi_id_0;   /* flexa sbi id 0 */
    u32 sbi_id_1;   /* flexa sbi id 1 */
    u32 sbi_id_2;   /* flexa sbi id 2 */
} inputLineBuf_s;

typedef void (*EncStreamMultiSegCallBackFunc)(void *pAppData);

typedef struct {
    u32 streamMultiSegmentMode; /* 0:single segment 1:multi-segment with no sync 2:multi-segment with sw handshake mode*/
    u32 streamMultiSegmentAmount; /* total segment amount must be more than 1*/
    u32 rdCnt;
    EncStreamMultiSegCallBackFunc cbFunc; /* call back function for segment ready interrupt */
    void *cbData;                         /* call back function data for segment ready interrupt */
} streamMultiSeg_s;

/* Reference frame Mv info for tmv */
struct MvInfo {
    u8 pred_mode;
    u8 pred_dir;
    u8 partMode;
    u8 cuSize;
    u8 puIndex;
    short m_iHor[2];
    short m_iVer[2];
    i32 refIdx[2];
    u8 longTermRef[2];
};
#undef Pel
#if 0
typedef       unsigned char       Pel;        ///< 8-bit pixel type
#else
typedef unsigned short Pel; // 16-bit pixel type
#endif

/* deblock info when multi-tile */
typedef struct {
    //data
    Pel leftY[16];
    Pel leftC[16];

    //ctrl info
    i32 bmvy;
    i32 bmvx;
    i32 fmvy;
    i32 fmvx;
    u8 bmvid;
    u8 fmvid;
    u8 tile_border; //above or below
    /* HW-SW_size: 0-8, 1-16, 2-32, 3-64 */
    u8 cu_size;
    u8 pu_split_type;
    u8 tu_border;
    u8 pcm_flag;
    u8 block_coded_block_flag;
    u8 block_bi_dir;
    u8 block_qp;
    u8 block_intra;
    /* HW-SW_size: 0-4, 1-8, 2-16, 3-32 */
    u8 tu_size;
} debTileCtx;

typedef struct { // per CTB
    // control
    u8 sao_filter_type[2];      // luma/chroma; disable/EO/BO
    u8 sao_filter_info[3];      // yuv; eo_class or band position
    u8 left_sao_disable;        // disable sao for PCM
    i8 sao_filter_offset[3][4]; // yuv; offsets

    // data
    Pel leftY[16][4][5];   // [blkIdxY]
    Pel leftC[2][8][4][5]; // [cb/cr][blkIdxY]
} saoTileCtx;

#define DEB_TILE_SYNC_SIZE ((sizeof(debTileCtx)) * 16)
#define SAODEC_TILE_SYNC_SIZE (sizeof(saoTileCtx))
#define SAOPP_TILE_SYNC_SIZE (2 * sizeof(Pel) * 16)

/* the max ctb size allowed for hevc */
#define MAX_CTB_SIZE 64

/* Masks for encOutputMbInfo.mode */
#define MBOUT_9_MODE_MASK 0x0F
#define MBOUT_9_CHROMA_MODE_MASK 0x30
#define MBOUT_9_REFIDX_MASK 0xC0

/* MB output information. Amount of data/mb in 16-bit words / bytes.
   This is how things used to be before Foxtail release.
   Older models rely on these defines.
   For Foxtail data has increased to 48 bytes/MB, this is defined in model. */
#define MBOUT_16 16
#define MBOUT_8 32

/* 16-bit pointer offset to field. */
#define MBOUT_16_BITCOUNT 1
#define MBOUT_16_MV_B0_X 4
#define MBOUT_16_MV_B1_X 5
#define MBOUT_16_MV_B2_X 6
#define MBOUT_16_MV_B3_X 7
#define MBOUT_16_INTRA_SAD 12
#define MBOUT_16_INTER_SAD 13

/* 8-bit pointer offset to field. */
#define MBOUT_8_MODE 0
#define MBOUT_8_NONZERO_CNT 1
#define MBOUT_8_MV_B0_Y 4
#define MBOUT_8_MV_B1_Y 5
#define MBOUT_8_MV_B2_Y 6
#define MBOUT_8_MV_B3_Y 7
#define MBOUT_8_I4X4 16
#define MBOUT_8_INPUT_MAD 28
#define MBOUT_8_LUMA_MEAN 29
#define MBOUT_8_CB_MEAN 30
#define MBOUT_8_CR_MEAN 31

#define MBOUT_8_BOOS_QP 32
#define MBOUT_8_VAR_REC_LUM 34 /* varRecon[0] */
#define MBOUT_8_VAR_INP_LUM 42 /* varInput[0] */

/* General tools */
#define ABS(x) ((x) < (0) ? -(x) : (x))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define SIGN(a) ((a) < (0) ? (-1) : (1))
#define CLIP3(x, y, z) ((z) < (x) ? (x) : ((z) > (y) ? (y) : (z)))
#define WIDTH_64_ALIGN(w) (((w) + 63) & (~63))
#define RAND(a, b) (rand() % ((b) - (a) + 1) + (a))
#define SWAP(a, b, type)                                                                           \
    {                                                                                              \
        type tmp;                                                                                  \
        tmp = a;                                                                                   \
        a = b;                                                                                     \
        b = tmp;                                                                                   \
    }
#define STRIDE(variable, alignment) ((variable + alignment - 1) & (~(alignment - 1)))

#define WIENER_DENOISE (1)
#if WIENER_DENOISE
#define USE_TOP_CTRL_DENOISE (1)
#define FIX_POINT_BIT_WIDTH (10)
#define SIG_RECI_BIT_WIDTH (12)
#define MB_NUM_BIT_WIDTH (20)

#define SNR_MAX (2 << FIX_POINT_BIT_WIDTH)
#define SNR_MIN (1 << FIX_POINT_BIT_WIDTH)
#define SIGMA_RANGE (20)
#define SIGMA_MAX (30)
#define FILTER_STRENGTH_H (1 << FIX_POINT_BIT_WIDTH)
#define FILTER_STRENGTH_L (1 << (FIX_POINT_BIT_WIDTH - 1))
#define FILTER_STRENGTH_PARAM ((FILTER_STRENGTH_H - FILTER_STRENGTH_L) / SIGMA_RANGE)
#define SIGMA_SMOOTH_NUM (5)
#endif

#define MOTION_LAMBDA_SSE_SHIFT 6
#define MOTION_LAMBDA_SAD_SHIFT 5
#define IMS_LAMBDA_SAD_SHIFT 6
#define MOTION_LAMBDA_PSY_SHIFT 6

#define AQ_NONE 0
#define AQ_VARIANCE 1
#define AQ_AUTO_VARIANCE 2
#define AQ_AUTO_VARIANCE_BIASED 3

#define QPFACTOR_FIX_POINT 14
#define LAMBDA_FIX_POINT 10

#define SSIM_FIX_POINT_FOR_8BIT 16
#define SSIM_FIX_POINT_FOR_10BIT 24

#define DOWN_SCALING_MAX_WIDTH 8192
//#define SEARCH_RANGE_ROW_OFFSET_TEST

#define CTB_RC_BUF_NUM 4

#define GMV_STEP_X 64
#define GMV_STEP_Y 16

#define MAX_STRM_BUF_NUM 2

#define MAX_OVERLAY_NUM 12

#define MAX_MOSAIC_NUM 12

#define SUBJECTIVE_TUNING
#ifdef SUBJECTIVE_TUNING
#define SUBJECTIVE_LAMBDA_TUNING
#define LA_RC_VBV
#define H264_LA_INTRA_MODE_NON_4x4
#define H264_LA_INTRA_MODE_NON_8x8
#define LA_REFERENCE_USE_INPUT
#define H264_LA_BLOCK_SIZE 2
//#define CUTREE_FRAME_COST_NBORDER
//#define H264_DECIMATE
#define RDOQ_LAMBDA_ADJ
//#define DISABLE_LOWDELAYB_FOR_P
#define BILINEAR_DOWNSAMPLE

#define LA_HEVC_SIMPLE_RDO
#define SUBJECTIVE_INITIAL_QP_TUNING
#define LA_INTRA_BY_SATD
//#define INTRA_CU_MODE_REFINE_ON_RECON
//#define GLOBAL_MV_ON_SEARCH_RANGE
//#define PSY_RDOQ
#endif

// optimazation scabac performance for HW
// #define RDO_SCABAC_SIMPLE_EN

// #define TILE_CMODEL_CTB_NUM
#define TILE_CMODEL_SIMULATION

#define HEVC_AV1_TMVP_ENABLE 0

/* macros to improve hevc quality of psnr */
#define VCE_VIDEO_QUALITY_IMPROVE
#ifdef VCE_VIDEO_QUALITY_IMPROVE
/* tools tuning */
#define HEVC_QUALITY_LA_TUNE
#define RDO_CHECK_ZERO_TU
#define REFINE_MEXN_BINCOST

/* rate control */
#define VBR_RC 2
#define FIRST_I_TUNE 1
#define RC_MODEL_DECAY 1.0
#define RC_MODEL_MIN_DIV 2
#define RC_IFRAME_EST_WITH_DELTA 1

/* PASS1 Speed Up: CABIT not updated, no RDO */
#define VQ_PASS1_CABIT_ONCE 1
#define VQ_PASS1_NO_RDO 1

//#define HEVC_INTER_CBF0_CHECK
#else
#define VBR_RC 0
#define FIRST_I_TUNE 0
#define RC_MODEL_DECAY 0.5
#define RC_MODEL_MIN_DIV 4
#define RC_IFRAME_EST_WITH_DELTA 0
#endif

/**** below are const parameters, not for control ****/
#define RDOQ_LMD_INTRA_FACTOR 0.65
#define RDOQ_LMD_INTER_FACTOR 0.75
#define RDOQ_LMD_INTRA_FACTOR_SCALED                                                               \
    (u32)(RDOQ_LMD_INTRA_FACTOR * RDOQ_LMD_INTRA_FACTOR * (1 << QPFACTOR_FIX_POINT))
#define RDOQ_LMD_INTER_FACTOR_SCALED                                                               \
    (u32)(RDOQ_LMD_INTER_FACTOR * RDOQ_LMD_INTER_FACTOR * (1 << QPFACTOR_FIX_POINT))

#define MAX_SEGMENTS 8
static const int segment_delta_qp[] = {-8, -6, -4, -2, 0, 2, 4, 6};

#define CDEF_STRENGTH_NUM 8

#define VCENC_FREE(p)                                                                              \
    do {                                                                                           \
        if (p) {                                                                                   \
            EWLfree(p);                                                                            \
            p = NULL;                                                                              \
        }                                                                                          \
    } while (0)

#ifdef SUPPORT_AV1
#include "av1enccommon.h"
#endif

#ifdef SUPPORT_VP9
#include "vp9enccommon.h"
#endif

#define MC_SYNC_WORD_BYTES (64 * 2)

#ifdef __cplusplus
}
#endif

#endif
