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
--  Description : ASIC low level controller
--
------------------------------------------------------------------------------*/
#ifndef __ENC_ASIC_CONTROLLER_H__
#define __ENC_ASIC_CONTROLLER_H__

#include "base_type.h"
#include "enccfg.h"
#include "ewl.h"
#include "encswhwregisters.h"
#include "hevcencapi.h"

#define ASIC_STATUS_ENABLE 0x001

#define ASIC_VCENC_BYTE_STREAM 0x00
#define ASIC_VCENC_NAL_UNIT 0x01

#define ASIC_PENALTY_UNDEFINED -1

#define ASIC_PENALTY_TABLE_SIZE 128

#define ASIC_FRAME_BUF_LUM_MAX (VCENC_MAX_REF_FRAMES + MAX_CORE_NUM)
#define ASIC_FRAME_BUF_CHR_MAX (VCENC_MAX_REF_FRAMES + MAX_CORE_NUM)
#define ASIC_FRAME_BUF_CUINFO_MAX (VCENC_LOOKAHEAD_MAX + 2 * MAX_GOP_SIZE + MAX_CORE_NUM - 2)

#define MAX_CU_WIDTH 64
#define INVALID_AREA 0xfffff
#define INVALID_POS 1023

// sizeof(FRAME_CONTEXT)
#define FRAME_CONTEXT_LENGTH 21264
#define VP9_FRAME_CONTEXT_LENGTH 2074

#define VCMDBUFFER_SIZE 8192

#define REFRINGBUF_EXTEND_HEIGHT 128
typedef enum {
    IDLE = 0,   /* Initial state, both HW and SW disabled */
    HWON_SWOFF, /* HW processing, SW waiting for HW */
    HWON_SWON,  /* Both HW and SW processing */
    HWOFF_SWON, /* HW is paused or disabled, SW is processing */
    DONE
} bufferState_e;

typedef enum {
    ASIC_HEVC = 1,
    ASIC_H264 = 2,
    ASIC_AV1 = 3,
    ASIC_JPEG = 4,
    ASIC_MJPEG = 5,
    ASIC_CUTREE = 6,
    ASIC_VP9 = 7
} asicCodingType_e;

typedef enum {
    ASIC_P_16x16 = 0,
    ASIC_P_16x8 = 1,
    ASIC_P_8x16 = 2,
    ASIC_P_8x8 = 3,
    ASIC_I_4x4 = 4,
    ASIC_I_16x16 = 5
} asicMbType_e;

typedef enum {
    ASIC_INTER = 0,
    ASIC_INTRA = 1,
    ASIC_MVC = 2,
    ASIC_MVC_REF_MOD = 3
} asicFrameCodingType_e;

enum {
    ASIC_PENALTY_I16MODE0 = 0,
    ASIC_PENALTY_I16MODE1,
    ASIC_PENALTY_I16MODE2,
    ASIC_PENALTY_I16MODE3,
    ASIC_PENALTY_I4MODE0,
    ASIC_PENALTY_I4MODE1,
    ASIC_PENALTY_I4MODE2,
    ASIC_PENALTY_I4MODE3,
    ASIC_PENALTY_I4MODE4,
    ASIC_PENALTY_I4MODE5,
    ASIC_PENALTY_I4MODE6,
    ASIC_PENALTY_I4MODE7,
    ASIC_PENALTY_I4MODE8,
    ASIC_PENALTY_I4MODE9,
    ASIC_PENALTY_I16FAVOR,
    ASIC_PENALTY_I4_PREV_MODE_FAVOR,
    ASIC_PENALTY_COST_INTER,
    ASIC_PENALTY_DMV_COST_CONST,
    ASIC_PENALTY_INTER_FAVOR,
    ASIC_PENALTY_SKIP,
    ASIC_PENALTY_GOLDEN,
    ASIC_PENALTY_SPLIT4x4,
    ASIC_PENALTY_SPLIT8x4,
    ASIC_PENALTY_SPLIT8x8,
    ASIC_PENALTY_SPLIT16x8,
    ASIC_PENALTY_SPLIT_ZERO,
    ASIC_PENALTY_DMV_4P,
    ASIC_PENALTY_DMV_1P,
    ASIC_PENALTY_DMV_QP,
    ASIC_PENALTY_DZ_RATE0,
    ASIC_PENALTY_DZ_RATE1,
    ASIC_PENALTY_DZ_RATE2,
    ASIC_PENALTY_DZ_RATE3,
    ASIC_PENALTY_DZ_SKIP0,
    ASIC_PENALTY_DZ_SKIP1,

    ASIC_PENALTY_AMOUNT
};

typedef struct {
    u32 irqDisable;
    u32 inputReadChunk;
    u32 asic_axi_readID;
    u32 asic_axi_writeID;
    u32 asic_stream_swap;
    u32 asic_pic_swap;
    u32 asic_roi_map_qp_delta_swap;
    u32 asic_ctb_rc_mem_out_swap;
    u32 asic_burst_length;
    u32 asic_burst_scmd_disable;
    u32 asic_burst_incr;
    u32 asic_data_discard;
    u32 asic_clock_gating_encoder;
    u32 asic_clock_gating_encoder_h265;
    u32 asic_clock_gating_encoder_h264;
    u32 asic_clock_gating_inter;
    u32 asic_clock_gating_inter_h265;
    u32 asic_clock_gating_inter_h264;
    u32 asic_axi_dual_channel;
    u32 asic_cu_info_mem_out_swap;

    u32 enc_mode;
    u32 nal_unit_type;
    u32 nuh_temporal_id;
    u32 prefixnal_svc_ext;
    /* indicate hw has the feature to control if write obu extension,
     1-don't write obu extension */
    u32 av1_extension_flag;

    u32 codingType;
    u32 sliceSize;
    u32 sliceNum;
    ptr_t outputStrmBase[MAX_STRM_BUF_NUM];
    ptr_t Av1PreoutputStrmBase;
    u32 outputStrmSize[MAX_STRM_BUF_NUM];
    u32 Av1PreBufferSize;

    u32 frameCodingType;
    u32 poc;

    ptr_t inputLumBase;
    ptr_t inputCbBase;
    ptr_t inputCrBase;

    //current picture
    ptr_t reconLumBase;
    ptr_t reconCbBase;
    ptr_t reconCrBase;
    ptr_t reconL4nBase;

    u32 minCbSize;
    u32 maxCbSize; /* ctb size */

    u32 minTrbSize;
    u32 maxTrbSize;

    u32 picWidth;
    u32 picHeight;

    u32 pps_deblocking_filter_override_enabled_flag;
    u32 slice_deblocking_filter_override_flag;

    u32 qp;
    u32 qpMin;
    u32 qpMax;
    u32 rcQpDeltaRange;

    u32 picInitQp;
    u32 diffCuQpDeltaDepth;

    i32 cbQpOffset;
    i32 crQpOffset;

    u32 saoEnable;

    u32 maxTransHierarchyDepthInter;
    u32 maxTransHierarchyDepthIntra;

    u32 cuQpDeltaEnabled;
    u32 log2ParellelMergeLevel;
    u32 numShortTermRefPicSets;
    u32 rpsId;
    u32 numNegativePics;
    u32 numPositivePics;
    i32 delta_poc0;
    i32 l0_delta_poc[2];
    i32 l0_long_term_flag[2];
    i32 used_by_curr_pic0;
    i32 l0_used_by_curr_pic[2];
    i32 delta_poc1;
    i32 l1_delta_poc[2];
    i32 l1_long_term_flag[2];
    i32 used_by_curr_pic1;
    i32 l1_used_by_curr_pic[2];
    i32 long_term_ref_pics_present_flag;
    i32 num_long_term_pics;

    //reference and recon common buffer
    u8 refRingBufEnable;
    ptr_t pRefRingBuf_base;
    u32 refRingBuf_luma_rd_offset;
    u32 refRingBuf_luma_wr_offset;
    u32 refRingBuf_chroma_rd_offset;
    u32 refRingBuf_chroma_wr_offset;
    u32 refRingBuf_4n_rd_offset;
    u32 refRingBuf_4n_wr_offset;
    u32 refRingBuf_luma_size;
    u32 refRingBuf_chroma_size;
    u32 refRingBuf_4n_size;

    //reference picture list
    ptr_t pRefPic_recon_l0[3][2]; /* separate luma and chroma addresses */

    ptr_t pRefPic_recon_l0_4n[2];

    ptr_t pRefPic_recon_l1[3][2]; /* separate luma and chroma addresses */

    ptr_t pRefPic_recon_l1_4n[2];

    //compress
    u32 ref_l0_luma_compressed[2];
    u32 ref_l0_chroma_compressed[2];
    ptr_t ref_l0_luma_compress_tbl_base[2];
    ptr_t ref_l0_chroma_compress_tbl_base[2];

    u32 ref_l1_luma_compressed[2];
    u32 ref_l1_chroma_compressed[2];
    ptr_t ref_l1_luma_compress_tbl_base[2];
    ptr_t ref_l1_chroma_compress_tbl_base[2];
    ptr_t frameCtx_base;

    u32 recon_luma_compress;
    u32 recon_chroma_compress;
    ptr_t recon_luma_compress_tbl_base;
    ptr_t recon_chroma_compress_tbl_base;

    u32 active_l0_cnt;
    u32 active_l1_cnt;
    u32 active_override_flag;

    u32 streamMode; /* 0 - byte stream, 1 - NAL units */

    // intra setup
    u32 strong_intra_smoothing_enabled_flag;
    u32 constrained_intra_pred_flag;

    u32 scaling_list_enabled_flag;

    u32 cirStart;
    u32 cirInterval;

    u32 intraAreaTop;
    u32 intraAreaLeft;
    u32 intraAreaBottom;
    u32 intraAreaRight;

    u32 roi1Top;
    u32 roi1Left;
    u32 roi1Bottom;
    u32 roi1Right;

    u32 roi2Top;
    u32 roi2Left;
    u32 roi2Bottom;
    u32 roi2Right;

    u32 roi3Top;
    u32 roi3Left;
    u32 roi3Bottom;
    u32 roi3Right;

    u32 roi4Top;
    u32 roi4Left;
    u32 roi4Bottom;
    u32 roi4Right;

    u32 roi5Top;
    u32 roi5Left;
    u32 roi5Bottom;
    u32 roi5Right;

    u32 roi6Top;
    u32 roi6Left;
    u32 roi6Bottom;
    u32 roi6Right;

    u32 roi7Top;
    u32 roi7Left;
    u32 roi7Bottom;
    u32 roi7Right;

    u32 roi8Top;
    u32 roi8Left;
    u32 roi8Bottom;
    u32 roi8Right;

    i32 roi1DeltaQp;
    i32 roi2DeltaQp;
    i32 roi3DeltaQp;
    i32 roi4DeltaQp;
    i32 roi5DeltaQp;
    i32 roi6DeltaQp;
    i32 roi7DeltaQp;
    i32 roi8DeltaQp;
    i32 roi1Qp;
    i32 roi2Qp;
    i32 roi3Qp;
    i32 roi4Qp;
    i32 roi5Qp;
    i32 roi6Qp;
    i32 roi7Qp;
    i32 roi8Qp;

    u32 qpFactorSad;
    u32 qpFactorSse;
    u32 lambdaDepth;

    u32 rcRoiEnable;
    u32 ipcmMapEnable;
    ptr_t roiMapDeltaQpAddr;
    u32 RoimapCuCtrl_index_enable;
    u32 RoimapCuCtrl_enable;
    u32 RoimapCuCtrl_ver;
    u32 RoiQpDelta_ver;
    ptr_t RoimapCuCtrlIndexAddr;
    ptr_t RoimapCuCtrlAddr;

    u32 roiUpdate;
    u32 filterDisable;
    i32 tc_Offset;
    i32 beta_Offset;

    u32 intraPenaltyPic4x4;
    u32 intraPenaltyPic8x8;
    u32 intraPenaltyPic16x16;
    u32 intraPenaltyPic32x32;
    u32 intraMPMPenaltyPic1;
    u32 intraMPMPenaltyPic2;
    u32 intraMPMPenaltyPic3;

    u32 intraPenaltyRoi14x4;
    u32 intraPenaltyRoi18x8;
    u32 intraPenaltyRoi116x16;
    u32 intraPenaltyRoi132x32;
    u32 intraMPMPenaltyRoi11;
    u32 intraMPMPenaltyRoi12;
    u32 intraMPMPenaltyRoi13;

    u32 intraPenaltyRoi24x4;
    u32 intraPenaltyRoi28x8;
    u32 intraPenaltyRoi216x16;
    u32 intraPenaltyRoi232x32;
    u32 intraMPMPenaltyRoi21;
    u32 intraMPMPenaltyRoi22;
    u32 intraMPMPenaltyRoi23;

    ptr_t sizeTblBase;
    u32 lamda_SAO_luma;
    u32 lamda_SAO_chroma;
    u32 lamda_motion_sse;
    u32 lamda_motion_sse_roi1;
    u32 lamda_motion_sse_roi2;
    u32 skip_chroma_dc_threadhold;
    u32 bits_est_tu_split_penalty;
    u32 bits_est_bias_intra_cu_8;
    u32 bits_est_bias_intra_cu_16;
    u32 bits_est_bias_intra_cu_32;
    u32 bits_est_bias_intra_cu_64;
    u32 inter_skip_bias;
    u32 bits_est_1n_cu_penalty;
    u32 lambda_motionSAD;
    u32 lambda_motionSAD_ROI1;
    u32 lambda_motionSAD_ROI2;

    u32 recon_chroma_half_size;

    u32 inputImageFormat;

    u32 outputBitWidthLuma;
    u32 outputBitWidthChroma;

    u32 inputImageRotation;
    u32 inputImageMirror;

    u32 inputChromaBaseOffset;
    u32 inputLumaBaseOffset;

    u32 dummyReadEnable;
    u32 dummyReadAddr;

    u32 pixelsOnRow;

    u32 xFill;
    u32 yFill;

    u32 colorConversionCoeffA;
    u32 colorConversionCoeffB;
    u32 colorConversionCoeffC;
    u32 colorConversionCoeffE;
    u32 colorConversionCoeffF;
    u32 colorConversionCoeffG;
    u32 colorConversionCoeffH;
    u32 rMaskMsb;
    u32 gMaskMsb;
    u32 bMaskMsb;
    u32 colorConversionLumaOffset;

    u32 scaledOutputFormat;
    ptr_t scaledLumBase;
    u32 scaledWidth;
    u32 scaledHeight;
    u32 scaledWidthRatio;
    u32 scaledHeightRatio;
    u32 scaledSkipLeftPixelColumn;
    u32 scaledSkipTopPixelRow;
    u32 scaledVertivalWeightEn;
    u32 scaledHorizontalCopy;
    u32 scaledVerticalCopy;
    u32 scaledOutSwap;
    u32 chromaSwap;
    u32 nalUnitSizeSwap;
    u32 codedChromaIdc;

    ptr_t compress_coeff_scan_base;
    u32 buswidth;

    u32 cabac_init_flag;
    u32 buffer_full_continue;

    u32 sliceReadyInterrupt;

    u32 asicHwId;

    u32 targetPicSize;

    u32 minPicSize;

    u32 maxPicSize;

    u32 averageQP;

    u32 nonZeroCount;

    u32 intraCu8Num;
    u32 skipCu8Num;
    u32 PBFrame4NRdCost;
    u32 SSEDivide256;
    //for noise reduction
    u32 noiseReductionEnable;
    u32 noiseLow;
    //u32 firstFrameSigma;
    u32 nrMbNumInvert;
    u32 nrSliceQPPrev;
    u32 nrThreshSigmaCur;
    u32 nrSigmaCur;
    u32 nrThreshSigmaCalced;
    u32 nrFrameSigmaCalced;
// for HEVC
#ifndef CTBRC_STRENGTH
    u32 lambda_sse_me[16];
    u32 lambda_satd_me[16];
    u32 lambda_satd_ims[16];
#else
    u32 lambda_sse_me[32];
    u32 lambda_satd_me[32];
    u32 lambda_satd_ims[32];
#endif
    u32 intra_size_factor[4];
    u32 intra_mode_factor[3];

    ptr_t ctbRcMemAddrCur;
    ptr_t ctbRcMemAddrPre;
    u32 ctbRcQpDeltaReverse;
    u32 ctbRcThrdMin;
    u32 ctbRcThrdMax;
    u32 ctbBitsMin;
    u32 ctbBitsMax;
    u32 totalLcuBits;
    u32 bitsRatio;
#ifdef CTBRC_STRENGTH
    i32 qpfrac;
    u32 sumOfQP;
    u32 sumOfQPNumber;
    u32 rcBaseMBComplexity;
    u32 qpDeltaMBGain;
    u32 picComplexity;
    u32 rcBlockSize;
    i32 offsetSliceQp;
#endif

    //  for vp9
    u32 mvRefIdx[2];
    u32 ref2Enable;

    u32 ipolFilterMode;
    u32 disableQuarterPixelMv; //not add into registers

    u32 splitMvMode; //not add into registers
    u32 partitionBase[8];
    u32 qpY1QuantDc[4];
    u32 qpY1QuantAc[4];
    u32 qpY2QuantDc[4];
    u32 qpY2QuantAc[4];
    u32 qpChQuantDc[4];
    u32 qpChQuantAc[4];
    u32 qpY1ZbinDc[4];
    u32 qpY1ZbinAc[4];
    u32 qpY2ZbinDc[4];
    u32 qpY2ZbinAc[4];
    u32 qpChZbinDc[4];
    u32 qpChZbinAc[4];
    u32 qpY1RoundDc[4];
    u32 qpY1RoundAc[4];
    u32 qpY2RoundDc[4];
    u32 qpY2RoundAc[4];
    u32 qpChRoundDc[4];
    u32 qpChRoundAc[4];
    u32 qpY1DequantDc[4];
    u32 qpY1DequantAc[4];
    u32 qpY2DequantDc[4];
    u32 qpY2DequantAc[4];
    u32 qpChDequantDc[4];
    u32 qpChDequantAc[4];
    u32 filterLevel[4];
    u32 boolEncValue;
    u32 boolEncValueBits;
    u32 boolEncRange;
    u32 dctPartitions;
    u32 filterSharpness;
    u32 segmentEnable;
    u32 segmentMapUpdate;
    i32 lfRefDelta[4];
    i32 lfModeDelta[4];
    u32 avgVar;
    u32 invAvgVar;
    u32 cabacCtxBase;
    u32 probCountBase;

    // for h.264
    u32 frameNum;
    u32 idrPicId;
    u32 nalRefIdc;
    u32 nalRefIdc_2bit;
    u32 transform8x8Enable;
    u32 entropy_coding_mode_flag;
    ptr_t colctbs_load_base;
    ptr_t colctbs_store_base;
    i32 l0_delta_framenum[2];
    i32 l0_used_by_next_pic[2];
    i32 l1_delta_framenum[2];
    i32 l1_used_by_next_pic[2];
    u32 markCurrentLongTerm;
    i32 currentLongTermIdx;
    i32 l0_referenceLongTermIdx[2];
    i32 l1_referenceLongTermIdx[2];
    u32 max_long_term_frame_idx_plus1;
    u32 log2_max_pic_order_cnt_lsb;
    u32 log2_max_frame_num;
    u32 pic_order_cnt_type;

    //ref pic lists modification
    u32 lists_modification_present_flag;
    u32 ref_pic_list_modification_flag_l0;
    u32 list_entry_l0[2];
    u32 ref_pic_list_modification_flag_l1;
    u32 list_entry_l1[2];

    //cu info output
    u32 enableOutputCuInfo;
    ptr_t cuInfoTableBase;
    ptr_t cuInfoDataBase;
    u32 cuinfoStride;

    //aq info output
    u32 aqInfoOutputMode;
    ptr_t aqInfoOutputBase;
    u32 aqInfoOutputStride;
    u32 aqStrength;

    /* mv info */
    ptr_t mvInfoBase;

    /* ctb bits output*/
    u32 enableOutputCtbBits;
    ptr_t ctbBitsDataBase;

    u32 pps_id;

    u32 rdoLevel;
    // dynamic rdo setting
    u32 dynamicRdoEnable;
    u32 dynamicRdoCu16Bias;
    u32 dynamicRdoCu16Factor;
    u32 dynamicRdoCu32Bias;
    u32 dynamicRdoCu32Factor;
    // for jpeg
    u32 firstFreeBit;
    u32 strmStartMSB;
    u32 strmStartLSB;
    u32 jpegHeaderLength;
    u32 jpegMode;
    u32 jpegSliceEnable;
    u32 jpegRestartInterval;
    u32 jpegRestartMarker;
    u32 constrainedIntraPrediction;
    u32 roundingCtrl;
    u32 mbsInRow;
    u32 mbsInCol;
    ptr_t busRoiMap;
    u32 roiEnable;
    u8 quantTable[8 * 8 * 2];
    u8 quantTableNonRoi[8 * 8 * 2];
    // for lossless jpeg
    u32 ljpegEn;
    u32 ljpegFmt;
    u32 ljpegPsv;
    u32 ljpegPt;

    /* low latency */
    u32 mbWrPtr;
    u32 mbRdPtr;
    u32 lineBufferDepth;
    u32 lineBufferEn;
    u32 lineBufferHwHandShake;
    u32 lineBufferLoopBackEn;
    u32 lineBufferInterruptEn;
    u32 amountPerLoopBack;
    u32 initSegNum;
    u32 sbi_id_0;
    u32 sbi_id_1;
    u32 sbi_id_2;

    u32 ipcmFilterDisable;
    u32 ipcm1AreaTop;
    u32 ipcm1AreaLeft;
    u32 ipcm1AreaBottom;
    u32 ipcm1AreaRight;

    u32 ipcm2AreaTop;
    u32 ipcm2AreaLeft;
    u32 ipcm2AreaBottom;
    u32 ipcm2AreaRight;

    u32 ipcm3AreaTop;
    u32 ipcm3AreaLeft;
    u32 ipcm3AreaBottom;
    u32 ipcm3AreaRight;

    u32 ipcm4AreaTop;
    u32 ipcm4AreaLeft;
    u32 ipcm4AreaBottom;
    u32 ipcm4AreaRight;

    u32 ipcm5AreaTop;
    u32 ipcm5AreaLeft;
    u32 ipcm5AreaBottom;
    u32 ipcm5AreaRight;

    u32 ipcm6AreaTop;
    u32 ipcm6AreaLeft;
    u32 ipcm6AreaBottom;
    u32 ipcm6AreaRight;

    u32 ipcm7AreaTop;
    u32 ipcm7AreaLeft;
    u32 ipcm7AreaBottom;
    u32 ipcm7AreaRight;

    u32 ipcm8AreaTop;
    u32 ipcm8AreaLeft;
    u32 ipcm8AreaBottom;
    u32 ipcm8AreaRight;

    /* frame data hash */
    u32 hashtype;
    u32 hashval;
    i32 hashoffset;

    /* for smart */
    i32 smartModeEnable;
    i32 smartH264Qp;
    i32 smartHevcLumQp;
    i32 smartHevcChrQp;
    i32 smartH264LumDcTh;
    i32 smartH264CbDcTh;
    i32 smartH264CrDcTh;
    /* threshold for hevc cu8x8/16x16/32x32 */
    i32 smartHevcLumDcTh[3];
    i32 smartHevcChrDcTh[3];
    i32 smartHevcLumAcNumTh[3];
    i32 smartHevcChrAcNumTh[3];
    /* back ground */
    i32 smartMeanTh[4];
    /* foreground/background threashold: maximum foreground pixels in background block */
    i32 smartPixNumCntTh;

    /* constant chroma control */
    i32 constChromaEn;
    u32 constCb;
    u32 constCr;

    /* Tile */
    i32 tiles_enabled_flag;
    i32 num_tile_columns;
    i32 num_tile_rows;
    i32 loop_filter_across_tiles_enabled_flag;
    u32 tileLeftStart;
    u32 tileWidthIn8;
    u32 startTileIdx;
    u32 tileMvConstraint;
    u32 tileSyncReadAlignment;
    u32 tileSyncWriteAlignment;
    u32 tileStrmSizeAlignmentExp;
    ptr_t tileSyncReadBase;
    ptr_t tileSyncWriteBase;
    ptr_t tileHeightBase;

    /* frame encoded as skipframe*/
    u32 skip_frame_enabled;

    /* enable P010 tile-raster format for reference frame buffer */
    u32 P010RefEnable;

    EWLHwConfig_t asicCfg;
    /*stride*/
    u32 input_luma_stride;
    u32 input_chroma_stride;
    u32 ref_frame_stride;
    u32 ref_frame_stride_ch;
    u32 ref_ds_luma_stride;
    /* RPS in slice header */
    u32 short_term_ref_pic_set_sps_flag;
    u32 rps_neg_pic_num;
    u32 rps_pos_pic_num;
    u32 rps_delta_poc[VCENC_MAX_REF_FRAMES];
    u32 rps_used_by_cur[VCENC_MAX_REF_FRAMES];
    u32 ssim;
    u32 psnr;
    u32 current_max_tu_size_decrease;

    /* CTB QP adjustment for rate control */
    u32 ctbRcModelParam0;
    u32 ctbRcModelParam1;
    u32 ctbRcModelParamMin;
    u32 prevPicLumMad;
    u32 ctbQpSumForRc;
    u32 ctbRcQpStep;
    u32 ctbRcRowFactor;
    u32 ctbRcPrevMadValid;
    u32 ctbRcDelay;

    /* Global MV */
    i16 gmv[2][2];

    /* Assigned ME vertical search range */
    u32 meAssignedVertRange;

    /* SKIP map */
    u32 skipMapEnable;

    /* rdoq map */
    u32 rdoqMapEnable;

    /* SSE before DEB */
    u32 lumSSEDivide256;
    u32 cbSSEDivide64;
    u32 crSSEDivide64;

    /* multi-core sync */
    u32 mc_sync_enable;
    ptr_t mc_sync_l0_addr;
    ptr_t mc_sync_l1_addr;
    ptr_t mc_sync_rec_addr;
    u32 mc_ref_ready_threshold;
    u32 mc_ddr_polling_interval;

    /* multi-pass*/
    u32 bSkipCabacEnable;
    u32 bRDOQEnable;
    u32 inLoopDSRatio;
    u32 bMotionScoreEnable;
    u32 motionScore[2][2];
    u32 cuInfoVersion;

    /*stream multi-segment*/
    u32 streamMultiSegEn;
    u32 streamMultiSegSWSyncEn;
    u32 streamMultiSegSize;
    u32 streamMultiSegRD;
    u32 streamMultiSegIRQEn;

    /* external SRAM line buffer */
    ptr_t sram_base_lum_fwd;
    ptr_t sram_base_lum_bwd;
    ptr_t sram_base_chr_fwd;
    ptr_t sram_base_chr_bwd;
    u32 sram_linecnt_lum_fwd;
    u32 sram_linecnt_lum_bwd;
    u32 sram_linecnt_chr_fwd;
    u32 sram_linecnt_chr_bwd;
    u32 AXI_ENC_WR_ID_E;
    u32 AXI_ENC_RD_ID_E;

    /*AXI max burst length */
    u32 AXI_burst_max_length;

    /* AXI alignment */
    u32 AXI_burst_align_wr_cuinfo;
    u32 AXI_burst_align_wr_common;
    u32 AXI_burst_align_wr_stream;
    u32 AXI_burst_align_wr_chroma_ref;
    u32 AXI_burst_align_wr_luma_ref;
    u32 AXI_burst_align_rd_common;
    u32 AXI_burst_align_rd_prp;
    u32 AXI_burst_align_rd_ch_ref_prefetch;
    u32 AXI_burst_align_rd_lu_ref_prefetch;

    /* AV1 */
    u32 sw_av1_allow_intrabc;
    u32 sw_av1_coded_lossless;
    u32 sw_av1_delta_q_res;
    u32 sw_av1_enable_filter_intra;
    u32 sw_av1_tx_mode;
    u32 sw_av1_reduced_tx_set_used;
    u32 sw_av1_seg_enable;
    u32 sw_av1_allow_high_precision_mv;
    u32 sw_av1_skip_mode_flag;
    u32 sw_av1_reference_mode;
    i32 sw_av1_list0_ref_frame;
    i32 sw_av1_list1_ref_frame;
    u32 sw_av1_enable_interintra_compound;
    u32 sw_av1_enable_dual_filter;
    u32 sw_av1_cur_frame_force_integer_mv;
    u32 sw_av1_switchable_motion_mode;
    u32 sw_av1_interp_filter;
    u32 sw_av1_allow_update_cdf;
    u32 sw_av1_db_filter_lvl[2];
    u32 sw_av1_db_filter_lvl_u;
    u32 sw_av1_db_filter_lvl_v;
    u32 sw_av1_sharpness_lvl;
    u32 sw_cdef_damping;
    u32 sw_cdef_bits;
    u32 sw_cdef_strengths[CDEF_STRENGTH_NUM];
    u32 sw_cdef_uv_strengths[CDEF_STRENGTH_NUM];
    u32 sw_av1_enable_order_hint;
    u32 sw_primary_ref_frame;
    u32 av1_bTxTypeSearch;
    u32 av1_plane_rdmult[2][2];
    u32 av1_qp2qindex[52];

    /* VP9 */
    u32 sw_last_frame_type;
    u32 sw_vp9_refresh_frame_context;
    u32 sw_vp9_segmentation_enable;
    u32 sw_vp9_segment_qp[MAX_SEGMENTS];
    u32 sw_vp9_segment_lf[MAX_SEGMENTS];
    u32 sw_vp9_segment_skip[MAX_SEGMENTS];
    u32 sw_vp9_segment_abs;
    u32 sw_vp9_segment_tree_prob[MAX_SEGMENTS - 1];

    /* overlay regs */
    ptr_t overlayYAddr[MAX_OVERLAY_NUM];
    ptr_t overlayUAddr[MAX_OVERLAY_NUM];
    ptr_t overlayVAddr[MAX_OVERLAY_NUM];
    u32 overlayEnable[MAX_OVERLAY_NUM];
    u32 overlayFormat[MAX_OVERLAY_NUM];
    u32 overlayAlpha[MAX_OVERLAY_NUM];
    u32 overlayXoffset[MAX_OVERLAY_NUM];
    u32 overlayYoffset[MAX_OVERLAY_NUM];
    u32 overlayWidth[MAX_OVERLAY_NUM];
    u32 overlayHeight[MAX_OVERLAY_NUM];
    u32 overlayYStride[MAX_OVERLAY_NUM];
    u32 overlayUVStride[MAX_OVERLAY_NUM];
    u32 overlayBitmapY[MAX_OVERLAY_NUM];
    u32 overlayBitmapU[MAX_OVERLAY_NUM];
    u32 overlayBitmapV[MAX_OVERLAY_NUM];
    u32 overlayScaleWidth;
    u32 overlayScaleHeight;
    u32 overlaySuperTile;
    u32 overlayScaleStepW;
    u32 overlayScaleStepH;

    /* video stabilization */
    ptr_t stabNextLumaBase;
    u32 stabMode;
    u32 stabMotionMin;
    u32 stabMotionSum;
    u32 stabGmvX;
    u32 stabGmvY;
    u32 stabMatrix1;
    u32 stabMatrix2;
    u32 stabMatrix3;
    u32 stabMatrix4;
    u32 stabMatrix5;
    u32 stabMatrix6;
    u32 stabMatrix7;
    u32 stabMatrix8;
    u32 stabMatrix9;

    u32 writeReconToDDR;

    u32 CyclesDdrPollingCheck;

    /* subject visual tuning */
    u32 H264Intramode4x4Disable;
    u32 H264Intramode8x8Disable;
    u32 RefFrameUsingInputFrameEnable;
    u32 rdoqLambdaAdjustIntra;
    u32 rdoqLambdaAdjustInter;
    u32 InLoopDSBilinearEnable;
    u32 HevcSimpleRdoAssign;
    u32 PredModeBySatdEnable;
    u32 bRdoCheckChromaZeroTu;
    u32 lambdaCostScaleMExN[3];
    /* psy factor */
    u32 psyFactor;

    u32 regMirror[ASIC_SWREG_AMOUNT];
    u32 *vcmdBuf;
    u32 vcmdBufSize;
    u32 totalCmdbufSize;
    u16 cmdbufid;

    /* vcmd */
    bool bVCMDAvailable;
    bool bVCMDEnable;

    /*irq Type mask*/
    u8 irq_type_sw_reset_mask;
    u8 irq_type_fuse_error_mask;
    u8 irq_type_buffer_full_mask;
    u8 irq_type_bus_error_mask;
    u8 irq_type_timeout_mask;
    u8 irq_type_strm_segment_mask;
    u8 irq_type_line_buffer_mask;
    u8 irq_type_slice_rdy_mask;
    u8 irq_type_frame_rdy_mask;

    u32 bInitUpdate; //clean to 0 in EncAsicFrameStart(), set to 1 in VCEncInit()/JpegEncInit()
    u32 bCodingCtrlUpdate; //clean to 0 in EncAsicFrameStart(), set to 1 in VCEncSetCodingCtrl()
    u32 bRateCtrlUpdate;   //clean to 0 in EncAsicFrameStart(), set to 1 in VCEncSetRateCtrl()
    u32 bPreprocessUpdate; //clean to 0 in EncAsicFrameStart(), set to 1 in VCEncSetPreProcessing()
    u32 bStrmStartUpdate;  //clean to 0 in EncAsicFrameStart(), set to 1 in VCEncStrmStart()

    /* SRAM power down disable */
    u32 sramPowerdownDisable;

    /* TMVP related*/
    u32 sliceTmvpEnable;
    u32 spsTmvpEnable;
    ptr_t tmvpMvInfoBase;
    ptr_t tmvpRefMvInfoBaseL0;
    ptr_t tmvpRefMvInfoBaseL1;
    u32 writeTMVinfoDDR;
    u8 colFrameFromL0;
    u8 colFrameRefIdx;
    i32 rplL0DeltaPocL0[2];
    i32 rplL0DeltaPocL1[2];
    i32 rplL1DeltaPocL0[2];
    i32 rplL1DeltaPocL1[2];
    u32 av1LastAltRefOrderHint;
} regValues_s;

typedef struct {
    const void *ewl;
    regValues_s regs;
    //source image

    //reconstructed  image
    EWLLinearMem_t internalreconLuma[ASIC_FRAME_BUF_LUM_MAX];
    EWLLinearMem_t internalreconLuma_4n[ASIC_FRAME_BUF_LUM_MAX];
    EWLLinearMem_t internalreconChroma[ASIC_FRAME_BUF_LUM_MAX];
    //EWLLinearMem_t internalreconCr[ASIC_FRAME_BUF_LUM_MAX];
    /**< \brief Frame context buffer for AV1 and VP9 */
    EWLLinearMem_t internalFrameContext[ASIC_FRAME_BUF_LUM_MAX];
    /**< sync word buffer for multi core */
    EWLLinearMem_t mc_sync_word[ASIC_FRAME_BUF_LUM_MAX];
    EWLLinearMem_t RefRingBuf;
    EWLLinearMem_t RefRingBufBak;

    EWLLinearMem_t scaledImage;
    EWLLinearMem_t cabacCtx;
    EWLLinearMem_t mvOutput;
    EWLLinearMem_t probCount;
    EWLLinearMem_t segmentMap;
    u32 sizeTblSize;
    EWLLinearMem_t sizeTbl[HEVC_MAX_TILE_COLS];
    EWLLinearMem_t compress_coeff_SCAN[MAX_CORE_NUM];
    EWLLinearMem_t ctbRcMem[4];
    EWLLinearMem_t deblockCtx;    //work when multi-tile
    EWLLinearMem_t tileHeightMem; // tile heights, only allocated when multi-tile

    u32 traceRecon;
    u32 dumpRegister;

    //compressor table
    EWLLinearMem_t compressTbl[ASIC_FRAME_BUF_LUM_MAX];

    //H.264 collocated buffer
    EWLLinearMem_t colBuffer[ASIC_FRAME_BUF_LUM_MAX];

    //CU information memory
    EWLLinearMem_t cuInfoMem[ASIC_FRAME_BUF_CUINFO_MAX];
    u32 cuInfoTableSize;
    //AQ information
    u32 aqInfoSize;
    u32 aqInfoStride;

    /*Ctb bit info memory */
    EWLLinearMem_t ctbBitsMem[MAX_CORE_NUM];

    EWLLinearMem_t loopbackLineBufMem;

    /* To store rpl mv info for TMVP */
    EWLLinearMem_t mvInfo[ASIC_FRAME_BUF_LUM_MAX];

    EWLLinearMem_t av1PreCarryBuf[MAX_CORE_NUM];
} asicData_s;

typedef struct {
    u32 width;
    u32 height;
    u32 scaledWidth;
    u32 scaledHeight;
    u32 encodingType;
    u32 numRefBuffsLum;
    u32 numRefBuffsChr;
    u32 compressor;
    u32 outputCuInfo;
    u32 outputCtbBits;
    u32 bitDepthLuma;
    u32 bitDepthChroma;
    u32 input_alignment;
    u32 ref_alignment;
    u32 ref_ch_alignment;
    u32 aqInfoAlignment;
    u32 cuinfoAlignment;
    u32 exteralReconAlloc;
    u32 maxTemporalLayers;
    u32 ctbRcMode;
    u32 numCuInfoBuf;
    u32 numCtbBitsBuf;
    u32 pass;
    u32 codedChromaIdc;
    u32 tmvpEnable;
    u32 is_malloc;
    u32 numAv1PreCarryBuf;
    u32 numTileColumns;
    u32 numTileRows;
    u32 tileEnable;
    u32 RefRingBufExtendHeight;
    u32 refRingBufEnable;
} asicMemAlloc_s;

/*------------------------------------------------------------------------------
    4. Function prototypes
------------------------------------------------------------------------------*/
i32 EncAsicControllerInit(asicData_s *asic, void *ctx, u32 client_type);

void EncAsicSetQuantTable(asicData_s *asic, const u8 *lumTable, const u8 *chTable);

void EncAsicSetNonRoiQuantTable(asicData_s *asic, const u8 *lumTable, const u8 *chTable,
                                const u8 *filter);

i32 EncAsicMemAlloc_V2(asicData_s *asic, asicMemAlloc_s *allocCfg);
void EncAsicGetSizeAndSetRegs(asicData_s *asic, asicMemAlloc_s *allocCfg,
                              u32 *internalImageLumaSize, u32 *lumaSize, u32 *lumaSize4N,
                              u32 *chromaSize, u32 *u32FrameContextSize);
i32 EncAsicGetBuffers(asicData_s *asic, EWLLinearMem_t *buf, i32 bufNum, i32 bufSize,
                      i32 alignment);

void EncAsicMemFree_V2(asicData_s *asic);

/* Functions for controlling ASIC */

u32 EncAsicGetPerformance(const void *ewl);

void EncAsicGetRegisters(const void *ewl, regValues_s *val, u32 dumpRegister, u32 rdReg);
u32 EncAsicGetStatus(const void *ewl);
void EncAsicClearStatus(const void *ewl, u32 value);

void EncAsicFrameStart(const void *ewl, regValues_s *val, u32 dumpRegister);

void EncAsicStop(const void *ewl);

void EncAsicRecycleInternalImage(asicData_s *asic, u32 numViews, u32 viewId, u32 anchor,
                                 u32 numRefBuffsLum, u32 numRefBuffsChr);

i32 EncAsicCheckStatus_V2(asicData_s *asic, u32 status);
u32 *EncAsicGetMvOutput(asicData_s *asic, u32 mbNum);
u32 EncAsicGetAsicHWid(u32 client_type, void *ctx);
u32 EncAsicGetCoreIdByFormat(u32 client_type, void *ctx);
EWLHwConfig_t EncAsicGetAsicConfig(u32 client_type, void *ctx);
void EncSetReseveInfo(const void *ewl, u32 width, u32 height, u32 rdoLevel, u32 bRDOQEnable,
                      u16 priority, u32 client_type);

i32 EncMakeCmdbufData(asicData_s *asic, regValues_s *val, void *dec400_data, void *dec400_osd);
i32 EncReseveCmdbuf(const void *ewl, regValues_s *val);
void EncCopyDataToCmdbuf(const void *ewl, regValues_s *val);
i32 EncLinkRunCmdbuf(const void *ewl, regValues_s *val);
i32 EncWaitCmdbuf(const void *ewl, u16 cmdbufid, u32 *status);
void EncGetRegsByCmdbuf(const void *ewl, u16 cmdbufid, u32 *regMirror, u32 dumpRegister);

#endif
