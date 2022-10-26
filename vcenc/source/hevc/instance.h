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
--------------------------------------------------------------------------------*/

#ifndef INSTANCE_H
#define INSTANCE_H

#include "container.h"
#include "enccommon.h"
#include "encpreprocess.h"
#include "encasiccontroller.h"

#include "hevcencapi.h" /* Callback type from API is reused */
#include "rate_control_picture.h"
#include "hash.h"
#include "sw_cu_tree.h"
#include "encdec400.h"

#ifdef SUPPORT_AV1
#include "av1instance.h"
#endif

#ifdef SUPPORT_VP9
#include "vp9instance.h"
#endif

#ifdef VIDEOSTAB_ENABLED
#include "vidstabcommon.h"
#endif

/*Coding ctrl parameter*/
typedef struct {
    struct node *next;
    VCEncCodingCtrl encCodingCtrl; /*codingCtrl Parameter*/
    i32 startPicCnt; /*the picture_cnt for when parameters in encode order start to go into force*/
    u32 refCnt;      /*number of this set of parameter in use*/
} EncCodingCtrlParam;

/*Coding ctrl*/
typedef struct {
    struct queue codingCtrlBufPool; /*codingCtrl parameter buffer pool*/
    struct queue codingCtrlQueue;   /*codingCtrl parameter queue*/
    EncCodingCtrlParam *
        pCodingCtrlParam; /*corresponding poniter in codingCtrlQueue of current enforced coding ctrl parameters */
} EncCodingCtrl;

typedef struct {
    struct node *next;
    VCEncIn encIn;
    EncCodingCtrlParam *pCodingCtrlParam; //pointer to coding ctrl parameters of this frame
} VCEncJob;

typedef struct {
    u32 tileLeft;  //based on ctb
    u32 tileRight; //based on ctb
    u32 startTileIdx;

    /* Buffer addresses and offsets */
    u32 *pOutBuf;
    ptr_t busOutBuf;
    u32 outBufSize;
    int input_offset;

    /* HW job */
    int job_id;

    /* output */
    u32 sumOfQP;
    u32 sumOfQPNumber;
    u32 picComplexity;

    /* input YUV */
    u32 input_luma_stride;
    u32 input_chroma_stride;
    ptr_t inputLumBase;
    ptr_t inputCbBase;
    ptr_t inputCrBase;
    u32 inputChromaBaseOffset;
    u32 inputLumaBaseOffset;
    ptr_t stabNextLumaBase;

    /* cuinfo */
    ptr_t cuinfoTableBase;
    ptr_t cuinfoDataBase;

    u32 streamSize;
    u32 numNalu;

    u32 intraCu8Num;
    u32 skipCu8Num;
    u32 PBFrame4NRdCost;
    u32 SSEDivide256;
    u32 lumSSEDivide256;
    u32 cbSSEDivide64;
    u32 crSSEDivide64;
    //compress_coeff_scan_base

    i64 ssim_numerator_y;
    i64 ssim_numerator_u;
    i64 ssim_numerator_v;
    u32 ssim_denominator_y;
    u32 ssim_denominator_uv;

    // for CtbRc
    i32 ctbRcX0;
    i32 ctbRcX1;
    i32 ctbRcFrameMad;
    i32 ctbRcQpSum;
} TileCtrl;

struct vcenc_instance {
    void *ctx;
    u32 slice_idx;

    u32 encStatus;
    asicData_s asic;
    regValues_s *regs_bak;
    VCDec400data *dec400_data_bak;
    VCDec400data *dec400_osd_bak;

    i32 vps_id; /* Video parameter set id */
    i32 sps_id; /* Sequence parameter set id */
    i32 pps_id; /* Picture parameter set id */
    i32 rps_id; /* Reference picture set id */

    struct buffer stream;
    struct buffer streams[MAX_CORE_NUM];

    EWLHwConfig_t featureToSupport;
    EWLHwConfig_t asic_core_cfg[MAX_SUPPORT_CORE_NUM];
    u32 asic_hw_id[MAX_SUPPORT_CORE_NUM];

    u32 reserve_core_info;

    u8 *temp_buffer; /* Data buffer, user set */
    u32 temp_size;   /* Size (bytes) of buffer */
    u32 temp_bufferBusAddress;

    // SPS&PPS parameters
    i32 max_cu_size;    /* Max coding unit size in pixels */
    i32 min_cu_size;    /* Min coding unit size in pixels */
    i32 max_tr_size;    /* Max transform size in pixels */
    i32 min_tr_size;    /* Min transform size in pixels */
    i32 tr_depth_intra; /* Max transform hierarchy depth */
    i32 tr_depth_inter; /* Max transform hierarchy depth */
    i32 width;
    i32 height;
    i32 ori_width;
    i32 ori_height;
    i32 enableScalingList;             /* */
    VCEncVideoCodecFormat codecFormat; /* Video Codec Format: HEVC/H264/AV1 */
    i32 pcm_enabled_flag;              /*enable pcm for HEVC*/
    i32 pcm_loop_filter_disabled_flag; /*pcm filter*/

    // intra setup
    u32 strong_intra_smoothing_enabled_flag;
    u32 constrained_intra_pred_flag;
    i32 enableDeblockOverride;
    i32 disableDeblocking;

    i32 tc_Offset;

    i32 beta_Offset;

    i32 ctbPerFrame;
    i32 ctbPerRow;
    i32 ctbPerCol;

    /* Minimum luma coding block size of coding units that convey
   * cu_qp_delta_abs and cu_qp_delta_sign and default quantization
   * parameter */
    i32 log2_qp_size;
    i32 qpHdr;

    i32 levelIdx;   /*level 5.1 =8*/
    i32 level;      /*level 5.1 =8*/
    i32 bAutoLevel; /* automatically calculate level */

    i32 profile; /**/
    i32 tier;

    preProcess_s preProcess;

    /* Rate control parameters */
    vcencRateControl_s rateControl;
    VCEncRateCtrl rateControl_ori;

    struct vps *vps;
    struct sps *sps;

    i32 poc; /* encoded Picture order count */
    i32 frameNum; /* frame number in decoding order, 0 for IDR, +1 for each reference frame; used for H.264 */
    i32 frameNumExt; /* frame number in decoding order, 0 for IDR, +1 for each reference frame; used for H.264 */
    i32 idrPicId; /* idrPicId in H264, to distinguish subsequent idr pictures */
    u8 *lum;
    u8 *cb;
    u8 *cr;
    i32 chromaQpOffset;
    i32 enableSao;
    i32 output_buffer_over_flow;
    const void *inst;

    /* H.264 MMO */
    i32 h264_mmo_nops;
    i32 h264_mmo_unref[VCENC_MAX_REF_FRAMES];
    i32 h264_mmo_ltIdx[VCENC_MAX_REF_FRAMES];
    i32 h264_mmo_long_term_flag[VCENC_MAX_REF_FRAMES];
    i32 h264_mmo_unref_ext[VCENC_MAX_REF_FRAMES];

    VCEncSliceReadyCallBackFunc sliceReadyCbFunc;
    VCEncTryNewParamsCallBackFunc cb_try_new_params;
    void *pAppData; /* User given application specific data */
    u32 frameCnt;
    u32 vuiVideoSignalTypePresentFlag;
    u32 vuiVideoFormat;
    u32 vuiVideoFullRange;
    u32 sarWidth;
    u32 sarHeight;
    i32 fieldOrder; /* 1=top first, 0=bottom first */
    u32 interlaced;
#ifdef VIDEOSTAB_ENABLED
    HWStabData vsHwData;
    SwStbData vsSwData;
#endif
    i32 gdrEnabled;
    i32 gdrStart;
    i32 gdrDuration;
    i32 gdrCount;
    i32 gdrAverageMBRows;
    i32 gdrMBLeft;
    i32 gdrFirstIntraFrame;
    u32 roi1Enable;
    u32 roi2Enable;
    u32 roi3Enable;
    u32 roi4Enable;
    u32 roi5Enable;
    u32 roi6Enable;
    u32 roi7Enable;
    u32 roi8Enable;
    u32 ctbRCEnable;
    i32 blockRCSize;
    u32 roiMapEnable;
    u32 RoimapCuCtrl_index_enable;
    u32 RoimapCuCtrl_enable; /* cu ctrl roi map enable (3 ~ 7 ) */
    u32 RoimapCuCtrl_ver;
    u32 RoiQpDelta_ver;
    u32 numNalus[MAX_CORE_NUM]; /* Amount of NAL units */
    u32 testId;
    i32 created_pic_num; /* number of pictures currently created */
    /* low latency */
    inputLineBuf_s inputLineBuf;
#if USE_TOP_CTRL_DENOISE
    unsigned int uiFrmNum;
    unsigned int uiNoiseReductionEnable;
    int FrmNoiseSigmaSmooth[5];
    int iFirstFrameSigma;
    int iNoiseL;
    int iSigmaCur;
    int iThreshSigmaCur;
    int iThreshSigmaPrev;
    int iSigmaCalcd;
    int iThreshSigmaCalcd;
    int iSliceQPPrev;
#endif
    i32 insertNewPPS;
    i32 insertNewPPSId;
    i32 maxPPSId;
    u32 maxTLayers; /*max temporal layers*/
    u32 rdoLevel;

    hashctx hashctx;

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

    u32 verbose;      /* Log printing mode */
    u32 dumpRegister; /**< enable dump register values for debug */
    u32 dumpCuInfo;   /**< enable dump Cu Information after encoding one frame for debug */
    u32 dumpCtbBits; /**< enable dump Ctb encoded bits Information after encoding one frame for debug */

    /* for tile*/
    i32 tiles_enabled_flag;
    i32 num_tile_columns;
    i32 num_tile_rows;
    i32 loop_filter_across_tiles_enabled_flag;
    i32 tileMvConstraint;
    u16 *tileHeights;
    TileCtrl tileCtrl[HEVC_MAX_TILE_COLS];

    /* L2-cache */
    void *cache;
    u32 channel_idx;

    u32 input_alignment;
    u32 ref_alignment;
    u32 ref_ch_alignment;
    u32 aqInfoAlignment;
    u32 tileStrmSizeAlignmentExp;

    u32 compressor;
    u32 ctbRcMode;
    u32 outputCuInfo;
    u32 outputCtbBits;
    u32 numCtbBitsBuf;

    /* AXIFE*/
    u32 axiFEEnable; /*0: axiFE disable   1:axiFE normal mode enable   2:axiFE bypass  3.security mode*/

    ITU_T_T35 itu_t_t35;
    u32 write_once_HDR10;
    Hdr10DisplaySei Hdr10Display;
    Hdr10LightLevelSei Hdr10LightLevel;
    VuiColorDescription vuiColorDescription;

    u32 RpsInSliceHeader;
    HWRPS_CONFIG sHwRps;

    bool rasterscan;

    /* cu tree look ahead queue */
    u32 pass;
    struct cuTreeCtr cuTreeCtl;
    bool bSkipCabacEnable;
    u32 lookaheadDepth;
    u32 idelay;
    u32 lookaheadDelay;
    VCEncLookahead lookahead;
    u32 numCuInfoBuf;
    u32 cuInfoBufIdx;
    bool bMotionScoreEnable;
    u32 extDSRatio;            /*0=1:1, 1=1:2*/
    void *cutreeJobBufferPool; /*cutree job buffer pool*/
    struct AGopInfo *gopInfoAddr[MAX_AGOPINFO_NUM];
    u32 nGopInfo; //number of agopInfo

    /* Multi-core parallel ctr */
    struct sw_picture *pic[MAX_CORE_NUM];
    u32 parallelCoreNum;
    u32 jobCnt;
    u32 reservedCore;
    VCEncStrmBufs streamBufs[MAX_CORE_NUM]; /* output stream buffers */
    u32 waitJobid[MAX_CORE_NUM];
    VCEncOut EncOut[MAX_CORE_NUM];
    VCEncIn EncIn[MAX_CORE_NUM];
    VCEncLookaheadJob *lookaheadJob[MAX_CORE_NUM];
    encOutForCutree_s outForCutree;

    /*stream Multi-segment ctrl*/
    streamMultiSeg_s streamMultiSegment;

    /* psy factor */
    float psyFactor;

#ifdef SUPPORT_AV1
    /* AV1 frame header */
    vcenc_instance_av1 av1_inst;
#endif

    u32 strmHeaderLen;

#ifdef SUPPORT_VP9
    /*VP9 inst*/
    vcenc_instance_vp9 vp9_inst;
#endif

    //for extra parameters.
    struct queue extraParaQ;

    u32 writeReconToDDR;

    u32 layerInRefIdc;
    u8 bRdoqLambdaAdjust;

    u8 bInputfileList;

    /*single pass manange encode order*/
    void *jobBufferPool;   /* buffer pool for job */
    struct queue jobQueue; /* job queue for reorder */
    VCEncIn encInFor1pass; /* maintain encode order information*/
    i32 gopSize;           /* gop size set by cmdline*/
    i32 nextGopSize;       /* next gop size*/
    VCENCAdapGopCtr agop;
    VCEncPicConfig lastPicCfg;
    i32 enqueueJobNum; /* single pass enqueue job number */

    /* */
    i8 bFlush;      /* flush flag, 1->flush*/
    i32 nextIdrCnt; /* next idr picture count*/

    u8 enableTMVP; /* Enable tmvp */

    //ncodeOrderMode_E encodeOrderMode; /* 0->init status; 1->encode in Frame input order; 2->adjust encode order inside API*/
    i32 bIOBufferBinding; /* if bind input buffer and output buffer 1->bind, 0 notbind */

    u8 flexRefsEnable;

    /*codingCtrl parameter*/
    EncCodingCtrl codingCtrl;

    //reference and recon common buffer
    u32 RefRingBufExtendHeight;
    u8 refRingBufEnable;
};

struct instance {
    struct vcenc_instance vcenc_instance; /* Visible to user */
    struct vcenc_instance *instance;      /* Instance sanity check */
    struct container container;           /* Encoder internal store */
};

struct container *get_container(struct vcenc_instance *instance);

#endif
