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
--  Abstract : HEVC Encoder testbench for linux
--
------------------------------------------------------------------------------*/
#ifndef _HEVCTESTBENCH_H_
#define _HEVCTESTBENCH_H_

#include "osal.h"
#include "base_type.h"

#include "hevcencapi.h"
#include "encinputlinebuffer.h"
#include "encasiccontroller.h"
#ifdef VMAF_SUPPORT
#include "vmaf.h"
#endif

/*--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/

#define DEFAULT -255
#define MAX_BPS_ADJUST 20
#define MAX_STREAMS 16
#define MAX_SCENE_CHANGE 20
#define MAX_CUTREE_DEPTH 64
#define MAX_DELAY_NUM (MAX_CORE_NUM + MAX_CUTREE_DEPTH)
#define MAX_BITS_STAT 1024

/* Structure for command line options */
typedef struct commandLine_s {
    char *input;
    char *output;
    char *recon;
    char *dec400CompTableinput;
    char *osdDec400CompTableInput;

    i32 useVcmd;
    i32 useDec400;
    i32 useL2Cache;
    i32 useAXIFE;

    char *test_data_files;
    i32 outputRateNumer; /* Output frame rate numerator */
    i32 outputRateDenom; /* Output frame rate denominator */
    i32 inputRateNumer;  /* Input frame rate numerator */
    i32 inputRateDenom;  /* Input frame rate denominator */
    i32 firstPic;
    i32 lastPic;

    i32 width;
    i32 height;
    i32 lumWidthSrc;
    i32 lumHeightSrc;

    i32 inputFormat;
    i32 formatCustomizedType; /*change general format to customized one*/

    i32 picture_cnt;
    i32 byteStream;
    i32 videoStab;

    i32 max_cu_size;                   /* Max coding unit size in pixels */
    i32 min_cu_size;                   /* Min coding unit size in pixels */
    i32 max_tr_size;                   /* Max transform size in pixels */
    i32 min_tr_size;                   /* Min transform size in pixels */
    i32 tr_depth_intra;                /* Max transform hierarchy depth */
    i32 tr_depth_inter;                /* Max transform hierarchy depth */
    VCEncVideoCodecFormat codecFormat; /* Video Codec Format: HEVC/H264/AV1 */

    i32 enableCabac; /* [0,1] H.264 entropy coding mode, 0 for CAVLC, 1 for CABAC */
    i32 cabacInitFlag;

    // intra setup
    u32 strong_intra_smoothing_enabled_flag;

    i32 cirStart;
    i32 cirInterval;

    i32 intraAreaEnable;
    i32 intraAreaTop;
    i32 intraAreaLeft;
    i32 intraAreaBottom;
    i32 intraAreaRight;

    i32 pcm_loop_filter_disabled_flag;

    i32 ipcm1AreaTop;
    i32 ipcm1AreaLeft;
    i32 ipcm1AreaBottom;
    i32 ipcm1AreaRight;

    i32 ipcm2AreaTop;
    i32 ipcm2AreaLeft;
    i32 ipcm2AreaBottom;
    i32 ipcm2AreaRight;

    i32 ipcm3AreaTop;
    i32 ipcm3AreaLeft;
    i32 ipcm3AreaBottom;
    i32 ipcm3AreaRight;

    i32 ipcm4AreaTop;
    i32 ipcm4AreaLeft;
    i32 ipcm4AreaBottom;
    i32 ipcm4AreaRight;

    i32 ipcm5AreaTop;
    i32 ipcm5AreaLeft;
    i32 ipcm5AreaBottom;
    i32 ipcm5AreaRight;

    i32 ipcm6AreaTop;
    i32 ipcm6AreaLeft;
    i32 ipcm6AreaBottom;
    i32 ipcm6AreaRight;

    i32 ipcm7AreaTop;
    i32 ipcm7AreaLeft;
    i32 ipcm7AreaBottom;
    i32 ipcm7AreaRight;

    i32 ipcm8AreaTop;
    i32 ipcm8AreaLeft;
    i32 ipcm8AreaBottom;
    i32 ipcm8AreaRight;
    i32 ipcmMapEnable;
    char *ipcmMapFile;

    char *skipMapFile;
    i32 skipMapEnable;
    i32 skipMapBlockUnit;

    i32 rdoqMapEnable;

    i32 roi1AreaEnable;
    i32 roi2AreaEnable;
    i32 roi3AreaEnable;
    i32 roi4AreaEnable;
    i32 roi5AreaEnable;
    i32 roi6AreaEnable;
    i32 roi7AreaEnable;
    i32 roi8AreaEnable;

    i32 roi1AreaTop;
    i32 roi1AreaLeft;
    i32 roi1AreaBottom;
    i32 roi1AreaRight;

    i32 roi2AreaTop;
    i32 roi2AreaLeft;
    i32 roi2AreaBottom;
    i32 roi2AreaRight;

    i32 roi1DeltaQp;
    i32 roi2DeltaQp;
    i32 roi1Qp;
    i32 roi2Qp;

    i32 roi3AreaTop;
    i32 roi3AreaLeft;
    i32 roi3AreaBottom;
    i32 roi3AreaRight;
    i32 roi3DeltaQp;
    i32 roi3Qp;

    i32 roi4AreaTop;
    i32 roi4AreaLeft;
    i32 roi4AreaBottom;
    i32 roi4AreaRight;
    i32 roi4DeltaQp;
    i32 roi4Qp;

    i32 roi5AreaTop;
    i32 roi5AreaLeft;
    i32 roi5AreaBottom;
    i32 roi5AreaRight;
    i32 roi5DeltaQp;
    i32 roi5Qp;

    i32 roi6AreaTop;
    i32 roi6AreaLeft;
    i32 roi6AreaBottom;
    i32 roi6AreaRight;
    i32 roi6DeltaQp;
    i32 roi6Qp;

    i32 roi7AreaTop;
    i32 roi7AreaLeft;
    i32 roi7AreaBottom;
    i32 roi7AreaRight;
    i32 roi7DeltaQp;
    i32 roi7Qp;

    i32 roi8AreaTop;
    i32 roi8AreaLeft;
    i32 roi8AreaBottom;
    i32 roi8AreaRight;
    i32 roi8DeltaQp;
    i32 roi8Qp;

    true_e reEncode;

    /* Rate control parameters */
    true_e fillerData; /* if enable/disable filler Data when HRD off */
    i32 hrdConformance;
    i32 cpbSize;
    i32 intraPicRate; /* IDR interval */

    i32 vbr; /* Variable Bit Rate Control by qpMin */
    i32 qpHdr;
    i32 qpMin;
    i32 qpMax;
    i32 qpMinI;
    i32 qpMaxI;
    i32 bitPerSecond;
    i32 cpbMaxRate;
    i32 crf; /*CRF constant*/

    /* rcMode */
    rcMode_e rcMode;

    i32 bitVarRangeI;

    i32 bitVarRangeP;

    i32 bitVarRangeB;
    u32 u32StaticSceneIbitPercent;

    i32 tolMovingBitRate; /*tolerance of max Moving bit rate */
    i32 monitorFrames;    /*monitor frame length for moving bit rate*/
    i32 picRc;
    i32 ctbRc;
    i32 blockRCSize;
    u32 rcQpDeltaRange;
    u32 rcBaseMBComplexity;
    i32 picSkip;
    i32 picQpDeltaMin;
    i32 picQpDeltaMax;
    i32 ctbRcRowQpStep;

    float tolCtbRcInter;
    float tolCtbRcIntra;

    i32 bitrateWindow;
    i32 intraQpDelta;
    i32 fixedIntraQp;
    i32 bFrameQpDelta;

    i32 disableDeblocking;

    i32 enableSao;

    i32 tc_Offset;
    i32 beta_Offset;

    i32 chromaQpOffset;

    i32 profile; /*main profile or main still picture profile*/
    i32 tier;    /*main tier or high tier*/
    i32 level;   /*main profile level*/

    i32 bpsAdjustFrame[MAX_BPS_ADJUST];
    i32 bpsAdjustBitrate[MAX_BPS_ADJUST];
    i32 smoothPsnrInGOP;

    i32 sliceSize;

    i32 testId;

    i32 rotation;
    i32 mirror;
    i32 horOffsetSrc;
    i32 verOffsetSrc;
    i32 colorConversion;
    i32 scaledWidth;
    i32 scaledHeight;
    i32 scaledOutputFormat;

    i32 enableDeblockOverride;
    i32 deblockOverride;

    i32 enableScalingList;

    u32 compressor;

    i32 interlacedFrame;
    i32 fieldOrder;
    i32 videoRange;
    i32 ssim;
    i32 psnr;
    i32 sei;
    char *userData;
    u32 gopSize;
    char *gopCfg;
    u32 numRefP; /**< number of reference frames for P frame. */
    u32 gopLowdelay;
    i32 outReconFrame;
    u32 longTermGap;
    u32 longTermGapOffset;
    u32 ltrInterval;
    i32 longTermQpDelta;

    char *flexRefs; /**< flexible reference description file. */

    i32 gdrDuration;
    u32 roiMapDeltaQpBlockUnit;
    u32 roiMapDeltaQpEnable;
    char *roiMapDeltaQpFile;
    char *roiMapDeltaQpBinFile;
    char *roiMapInfoBinFile;
    char *roiMapConfigFile;
    char *RoimapCuCtrlInfoBinFile;
    char *RoimapCuCtrlIndexBinFile;
    u32 RoiCuCtrlVer;
    u32 RoiQpDeltaVer;
    i32 outBufSizeMax;
    i32 multimode; // Multi-stream mode, 0--disable, 1--mult-thread, 2--multi-process
    char *streamcfg[MAX_STREAMS];
    struct {
        int pid[MAX_STREAMS];

        pthread_t *tid[MAX_STREAMS];
    };
    u32 layerInRefIdc;
    struct commandLine_s *streamcml[MAX_STREAMS];
    i32 nstream;

    char **argv; /* Command line parameter... */
    i32 argc;
    //WIENER_DENOISE
    i32 noiseReductionEnable;
    i32 noiseLow;
    i32 firstFrameSigma;

    i32 bitDepthLuma;
    i32 bitDepthChroma;

    u32 enableOutputCuInfo;
    u32 enableOutputCtbBits;

    u32 rdoLevel;
    /* dynamic rdo params */
    u32 dynamicRdoEnable;
    u32 dynamicRdoCu16Bias;
    u32 dynamicRdoCu16Factor;
    u32 dynamicRdoCu32Bias;
    u32 dynamicRdoCu32Factor;

    /* low latency */
    i32 inputLineBufMode;
    i32 inputLineBufDepth;
    i32 amountPerLoopBack;

    u32 hashtype;
    u32 verbose;

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

    i32 sceneChange[MAX_SCENE_CHANGE];

    /* for tile*/
    i32 tiles_enabled_flag;
    i32 num_tile_columns;
    i32 num_tile_rows;
    i32 loop_filter_across_tiles_enabled_flag;
    u32 tile_width[HEVC_MAX_TILE_COLS];
    u32 tile_height[HEVC_MAX_TILE_ROWS];
    i32 tileMvConstraint;

    /*for skip frame encoding ctr*/
    i32 skip_frame_enabled_flag;
    i32 skip_frame_poc;

    /*stride*/
    u32 exp_of_input_alignment;
    u32 exp_of_ref_alignment;
    u32 exp_of_ref_ch_alignment;
    u32 exp_of_aqinfo_alignment;
    u32 exp_of_tile_stream_alignment;

    /* ITU-T-T35 */
    u32 itu_t_t35_enable;
    u32 itu_t_t35_country_code;
    u32 itu_t_t35_country_code_extension_byte;
    char *p_t35_payload_file;

    /* HDR10 */
    u32 write_once_HDR10;
    u32 hdr10_display_enable;
    u32 hdr10_dx0;
    u32 hdr10_dy0;
    u32 hdr10_dx1;
    u32 hdr10_dy1;
    u32 hdr10_dx2;
    u32 hdr10_dy2;
    u32 hdr10_wx;
    u32 hdr10_wy;
    u32 hdr10_maxluma;
    u32 hdr10_minluma;

    u32 hdr10_lightlevel_enable;
    u32 hdr10_maxlight;
    u32 hdr10_avglight;

    u32 vuiColorDescripPresentFlag;
    u32 vuiColorPrimaries;
    u32 vuiTransferCharacteristics;
    u32 vuiMatrixCoefficients;
    u32 vuiVideoFormat;
    u32 vuiVideoSignalTypePresentFlag;
    u32 vuiAspectRatioWidth;
    u32 vuiAspectRatioHeight;

    u32 RpsInSliceHeader;
    u32 P010RefEnable;
    u32 vui_timing_info_enable;

    u32 picOrderCntType;
    u32 log2MaxPicOrderCntLsb;
    u32 log2MaxFrameNum;

    i16 gmv[2][2];
    char *gmvFileName[2];
    char *halfDsInput;
    u32 inLoopDSRatio;

    u32 parallelCoreNum;
    u32 dumpRegister;
    u32 rasterscan;

    u32 streamBufChain;
    u32 lookaheadDepth;
    u32 streamMultiSegmentMode;
    u32 streamMultiSegmentAmount;
    i32 cuInfoVersion;
    u32 enableRdoQuant;

    u32 extSramLumHeightBwd;
    u32 extSramChrHeightBwd;
    u32 extSramLumHeightFwd;
    u32 extSramChrHeightFwd;

    u64 AXIAlignment;

    u32 irqTypeMask;
    u32 irqTypeCutreeMask;

    /* used in multi-node server env. specify which node will be used. */
    u32 sliceNode;

    u32 ivf;

    u32 MEVertRange;

    /*Overlay*/
    u32 overlayEnables;
    char *olInput[MAX_OVERLAY_NUM];
    u32 olFormat[MAX_OVERLAY_NUM];
    u32 olAlpha[MAX_OVERLAY_NUM];
    u32 olWidth[MAX_OVERLAY_NUM];
    u32 olCropWidth[MAX_OVERLAY_NUM];
    u32 olScaleWidth[MAX_OVERLAY_NUM];
    u32 olHeight[MAX_OVERLAY_NUM];
    u32 olCropHeight[MAX_OVERLAY_NUM];
    u32 olScaleHeight[MAX_OVERLAY_NUM];
    u32 olXoffset[MAX_OVERLAY_NUM];
    u32 olCropXoffset[MAX_OVERLAY_NUM];
    u32 olYoffset[MAX_OVERLAY_NUM];
    u32 olCropYoffset[MAX_OVERLAY_NUM];
    u32 olYStride[MAX_OVERLAY_NUM];
    u32 olUVStride[MAX_OVERLAY_NUM];
    u32 olBitmapY[MAX_OVERLAY_NUM];
    u32 olBitmapU[MAX_OVERLAY_NUM];
    u32 olBitmapV[MAX_OVERLAY_NUM];
    u32 olSuperTile[MAX_OVERLAY_NUM];

    /* Mosaic parameters */
    u32 mosaicEnables;
    u32 mosXoffset[MAX_MOSAIC_NUM];
    u32 mosYoffset[MAX_MOSAIC_NUM];
    u32 mosWidth[MAX_MOSAIC_NUM];
    u32 mosHeight[MAX_MOSAIC_NUM];

    VCEncChromaIdcType codedChromaIdc;

    u32 aq_mode;
    float aq_strength;
    float psyFactor;

    u32 preset;
    u32 writeReconToDDR;

    u32 TxTypeSearchEnable;
    u32 av1InterFiltSwitch;

    char *extSEI;

    u32 tune;
    u32 b_vmaf_preprocess;

    u32 resendParamSet;

    u32 sendAUD;

    u32 sramPowerdownDisable;

    u32 insertIDR;

    i32 stopFrameNum;

    /* Internal use, to output some information */
    u32 bRdLog;
    u32 bRcLog;

    char *replaceMvFile;

    char *inputFileList;

    /*AXI max burst length */
    u32 burstMaxLength;

    u32 enableTMVP;
    /*disableEOS*/
    u32 disableEOS;

    /* trigger for coding parameter change */
    u32 trigger_pic_cnt; /* active the trigger when picture at "picture_cnt" will be encoded */
    u32 trigger_prm_cnt; /* where the new parameters will be start in the argv */

    /* modify space of av1's tile_group_size, just for verified when using ffmpeg, NOT using when encoding */
    u32 modifiedTileGroupSize;

    /* logmsg env parameter */
    u32 logOutDir;
    u32 logOutLevel;
    u32 logTraceMap;
    u32 logCheckMap;

    u32 refRingBufEnable;
} commandLine_s;

typedef struct {
    u32 streamPos;
    u32 multislice_encoding;
    u32 output_byte_stream;
    FILE *outStreamFile;
} SliceCtl_s;

typedef struct {
    u32 streamRDCounter;
    u32 streamMultiSegEn;
    u8 *streamBase;
    u32 segmentSize;
    u32 segmentAmount;
    FILE *outStreamFile;
    u8 startCodeDone;
    i32 output_byte_stream;
} SegmentCtl_s;

typedef struct inputFileLinkList_s {
    char *in;
    char *out;
    u32 a;
    u32 b;
    u32 w;
    u32 h;
    struct inputFileLinkList_s *next;
} inputFileLinkList;

typedef struct {
    u32 gopBits[MAX_BITS_STAT]; /* record all gops encoded bits upto 1024 gops */
    u32 gopPos;
    u32 nGop;
    u8 nGopFrame;
    double sumDeviation;
    float maxMovingBitRate;
    float minMovingBitRate;
    u32 nStatFrame;
} statisticBits_s;

struct test_bench {
    char *input;
    char *halfDsInput;
    char *output;
    char *test_data_files;
    char *dec400TableFile;
    FILE *in;
    FILE *inDS;
    FILE *out;
    FILE *fmv;
    FILE *dec400Table;
    u32 hwid;            /* HW ID */
    EWLHwConfig_t hwCfg; /* HW fuse */
    i32 width;
    i32 height;
    i32 outputRateNumer; /* Output frame rate numerator */
    i32 outputRateDenom; /* Output frame rate denominator */
    i32 inputRateNumer;  /* Input frame rate numerator */
    i32 inputRateDenom;  /* Input frame rate denominator */
    i32 firstPic;
    i32 lastPic;
    i32 picture_enc_cnt;     /*count of frames add to encode API*/
    i32 picture_encoded_cnt; /*count of frame finish encoding*/
    i32 ivf_frame_cnt;       /* count of ivf frames*/
    i32 ivfSize;
    i32 idr_interval;
    i32 lastForceIntra; /* last force intra picture cnt */
    i32 byteStream;
    u8 *lum;
    u8 *cb;
    u8 *cr;
    u8 *lumDS;
    u8 *cbDS;
    u8 *crDS;
    u32 src_img_size_ds;
    i32 interlacedFrame;
    u32 input_alignment;
    u32 ref_alignment;
    u32 ref_ch_alignment;
    i32 formatCustomizedType;
    u32 lumaSize;
    u32 chromaSize;
    u32 transformedSize;
    u32 pictureDSSize;
    u32 extSramLumBwdSize;
    u32 extSramLumFwdSize;
    u32 extSramChrBwdSize;
    u32 extSramChrFwdSize;
    u32 dec400LumaTableSize;
    u32 dec400ChrTableSize;
    u32 dec400FrameTableSize;

    char **argv; /* Command line parameter... */
    i32 argc;
    /* Moved from global space */
    FILE *yuvFile;
    FILE *roiMapFile;
    FILE *roiMapBinFile;
    FILE *ipcmMapFile;
    FILE *skipMapFile;
    FILE *rdoqMapFile;
    FILE *roiMapInfoBinFile;
    FILE *roiMapConfigFile;
    FILE *RoimapCuCtrlInfoBinFile;
    FILE *RoimapCuCtrlIndexBinFile;

    /** flexible reference description file handle */
    FILE *flexRefsFile;

    /* user provided ME1N mv */
    FILE *replaceMvFile;

    /** For resolution change.*/
    FILE *inputFileList;
    int inputFileLinkListNum;
    inputFileLinkList *inputFileLinkListP;

    /* SW/HW shared memories for input/output buffers */
    EWLLinearMem_t *pictureMem;
    EWLLinearMem_t *pictureDSMem;
    EWLLinearMem_t *outbufMem[HEVC_MAX_TILE_COLS][MAX_STRM_BUF_NUM];
    EWLLinearMem_t *roiMapDeltaQpMem;
    EWLLinearMem_t *transformMem;
    EWLLinearMem_t *RoimapCuCtrlInfoMem;
    EWLLinearMem_t *RoimapCuCtrlIndexMem;
    EWLLinearMem_t *overlayInputMem[MAX_OVERLAY_NUM];
    EWLLinearMem_t *osdDec400TableMem;
    EWLLinearMem_t *Dec400CmpTableMem;

    EWLLinearMem_t pictureMemFactory[MAX_DELAY_NUM];
    EWLLinearMem_t pictureDSMemFactory[MAX_DELAY_NUM];
    EWLLinearMem_t outbufMemFactory[MAX_CORE_NUM][HEVC_MAX_TILE_COLS]
                                   [MAX_STRM_BUF_NUM]; /* [frameIdx][tileIdx][bufIdx] */
    EWLLinearMem_t roiMapDeltaQpMemFactory[MAX_DELAY_NUM];
    EWLLinearMem_t transformMemFactory[MAX_DELAY_NUM];
    EWLLinearMem_t RoimapCuCtrlInfoMemFactory[MAX_DELAY_NUM];
    EWLLinearMem_t RoimapCuCtrlIndexMemFactory[MAX_DELAY_NUM];
    EWLLinearMem_t extSRAMMemFactory[MAX_CORE_NUM];
    EWLLinearMem_t Dec400CmpTableMemFactory[MAX_DELAY_NUM];
    EWLLinearMem_t overlayInputMemFactory[MAX_DELAY_NUM][MAX_OVERLAY_NUM];
    EWLLinearMem_t osdDec400CmpTableMemFactory[MAX_DELAY_NUM];
    EWLLinearMem_t mvReplaceBuffer;

    u32 tileColumnNum;
    u32 tileRowNum;
    u32 tileEnable;

    i32 *tile_width;
    i32 *tile_height;
    VCEncInTileExtra *vcencInTileExtra;
    VCEncPrpTileExtra *vcencPrpTileExtra;
    u32 inputMemFlags[MAX_DELAY_NUM];            //flags for input picture buffer
    u32 outputMemFlags[MAX_CORE_NUM];            //flags for output picture buffer
    u32 roiMapDeltaQpMemFlags[MAX_DELAY_NUM];    //flags for input roiMapDeltaQp buffers
    u32 roiMapCuCtrlInfoMemFlags[MAX_DELAY_NUM]; //flags for input roiMapCuCtrlInfo buffers
    u32 overlayInputFlags[MAX_DELAY_NUM];        //flags for overlayInput buffers

    EWLLinearMem_t scaledPictureMem;
    EWLLinearMem_t pictureStabMem;

    u32 gopSize;
    i32 nextGopSize;
    VCEncIn encIn;

    inputLineBufferCfg inputCtbLineBuf;

    FILE *gmvFile[2];

    u32 parallelCoreNum;
    SliceCtl_s sliceCtl;

    i32 streamBufNum;
    u32 inputInfoBuf_cnt;
    u32 buffer_cnt;
    u32 outbuf_cnt; //count of out buffers
    SegmentCtl_s streamSegCtl;

    struct timeval timeFrameStart;
    struct timeval timeFrameEnd;

    /* Overlay */
    FILE *overlayFile[MAX_OVERLAY_NUM];
    FILE *osdDec400TableFile;
    u32 olFrameNume[MAX_OVERLAY_NUM];
    u8 olEnable[MAX_OVERLAY_NUM];
    u32 osdDec400TableSize;

    /* output log for psnr/ssim/bits, only valid for c-model, mainly used for internal dev */
    FILE *rdLogFile;
    u64 totalBits;

    FILE *extSEIFile;

    statisticBits_s bitStat; /* For statistic of GOP bits */

    FILE *p_t35_file;
#ifdef VMAF_SUPPORT
    VMAFCtrl vmafctrl;
#endif

    const void *ewlInst;
};

#define MOVING_AVERAGE_FRAMES 120

typedef struct {
    i32 frame[MOVING_AVERAGE_FRAMES];
    i32 length;
    i32 count;
    i32 pos;
    i32 frameRateNumer;
    i32 frameRateDenom;
} ma_s;

#define CMLTRACE(...)                                                                              \
    do {                                                                                           \
        VCEncTraceMsg(NULL, VCENC_LOG_INFO, VCENC_LOG_TRACE_CML, ##__VA_ARGS__);                   \
    } while (0)

#endif
