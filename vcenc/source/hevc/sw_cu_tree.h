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

#ifndef SW_CU_TREE_H
#define SW_CU_TREE_H

#include "base_type.h"
#include "sw_put_bits.h"
#include "sw_picture.h"
#include "enccommon.h"
#include "math.h"

#include "osal.h"

#define CU_TREE_VERSION 1
#define CU_TREE_QP 10

#define AGOP_MOTION_TH 8

#define DEFAULT_MAX_HIE_DEPTH 2

#define CUTREE_BUFFER_NUM                                                                          \
    (MIN(2 * MAX_GOP_SIZE, 16)) // output ROI map buffr count limited by bits in command

#define VCENC_FRAME_TYPE_IDR 0x0001
#define VCENC_FRAME_TYPE_I 0x0002
#define VCENC_FRAME_TYPE_P 0x0003
#define VCENC_FRAME_TYPE_BREF 0x0004 /* Non-disposable B-frame */
#define VCENC_FRAME_TYPE_B 0x0005
#define VCENC_FRAME_TYPE_BLDY 0x0006
#define IS_CODING_TYPE_I(x) ((x) == VCENC_FRAME_TYPE_I || (x) == VCENC_FRAME_TYPE_IDR)
#define IS_CODING_TYPE_B(x) ((x) == VCENC_FRAME_TYPE_B || (x) == VCENC_FRAME_TYPE_BREF)
#define IS_CODING_TYPE_P(x) ((x) == VCENC_FRAME_TYPE_P || (x) == VCENC_FRAME_TYPE_BLDY)
#define VCENC_CODING_TYPE(x)                                                                       \
    (IS_CODING_TYPE_I(x)                                                                           \
         ? VCENC_INTRA_FRAME                                                                       \
         : (IS_CODING_TYPE_B(x) ? VCENC_BIDIR_PREDICTED_FRAME : VCENC_PREDICTED_FRAME))

/* Arbitrary limitations as a sanity check. */
#define MAX_FRAME_DURATION 1.00
#define MIN_FRAME_DURATION 0.01
#define MAX_FRAME_DURATION_FIX8 256
#define MIN_FRAME_DURATION_FIX8 3

#define CLIP_DURATION(f) CLIP3(MIN_FRAME_DURATION, MAX_FRAME_DURATION, f)
#define CLIP_DURATION_FIX8(f) CLIP3(MIN_FRAME_DURATION_FIX8, MAX_FRAME_DURATION_FIX8, f)

#define LOWRES_COST_SHIFT CU_INFO_COST_BITS
#define LOWRES_COST_MASK ((1 << LOWRES_COST_SHIFT) - 1)

#define VCENC_MIN(a, b) ((a) < (b) ? (a) : (b))
#define VCENC_MAX(a, b) ((a) > (b) ? (a) : (b))

#if defined(_MSC_VER)
#define LOG2F(x) (logf((float)(x)) * 1.44269504088896405f)
#define LOG2(x) (log((double)(x)) * 1.4426950408889640513713538072172)
#else
#define LOG2F(x) log2f(x)
#define LOG2(x) log2(x)
#endif
#define LOG2I(x) log2_fixpoint(x, 8) // Q24.8

#define INVALID_INDEX 0x3f

#define MAX_AGOPINFO_NUM 12

typedef enum thread_status_enum {
    THREAD_STATUS_OK = 0,
    THREAD_STATUS_MAIN_STOP,
    THREAD_STATUS_LOOKAHEAD_FLUSH,
    THREAD_STATUS_CUTREE_FLUSH,
    THREAD_STATUS_CUTREE_FLUSH_END,
    THREAD_STATUS_LOOKAHEAD_ERROR,
    THREAD_STATUS_CUTREE_ERROR,
    THREAD_STATUS_MAIN_ERROR,
} THREAD_STATUS;

struct MV {
    int16_t x;
    int16_t y;
};

/* output buffer */
typedef struct {
    struct node *next;
    u32 *pOutBuf;    /* Pointer to output stream buffer */
    ptr_t busOutBuf; /* Bus address of output stream buffer */
    u32 outBufSize;  /* Size of output stream buffer in bytes */
    EWLLinearMem_t mem;
} OutputBuffer;

/* Lookahead ctrl */
typedef struct {
    pthread_t *tid_lookahead;
    pthread_t tid_cutree;
    VCEncIn encIn;

    VCEncInst priv_inst;
    i32 enqueueJobcnt;
    struct queue jobs;
    struct queue output;
    pthread_mutex_t job_mutex;
    pthread_cond_t job_cond;
    pthread_mutex_t output_mutex;
    pthread_cond_t output_cond;
    bool bFlush;
    VCEncRet status;
    i32 lastPoc;
    i32 lastGopPicIdx;
    i32 lastGopSize;
    VCEncPictureCodingType lastCodingType;
    i32 picture_cnt;
    i32 last_idr_picture_cnt;
    OutputBuffer internal_mem;
    void *jobBufferPool; //pass2->pass1's job bufferpool
    i32 nextIdrCnt;      //2pass next idr, share lock job_mutex with queue jobs
    pthread_mutex_t stop_mutex;
    pthread_cond_t stop_cond;
    i32 bStop;
} VCEncLookahead;

/* frame level output of cu tree */
struct FrameOutput {
    //pixel *buffer[4];

    int frameNum;  // Presentation frame number
    int poc;       // Presentation frame number
    int sliceType; // Slice type decided by lookahead
    int qp;
    double cost;
    char typeChar;
    u32 gopSize;
    double costGop[4];
    i32 FrameNumGop[4];
    double costAvg[4];
    i32 FrameTypeNum[4];
    u32 *segmentCountAddr; /* Bus address of Segment counter */
#ifdef GLOBAL_MV_ON_SEARCH_RANGE
    i32 gmv[2][3]; // global mv for pass2
#endif
};
typedef struct {
    struct node *next;
    VCEncIn encIn;
    VCEncOut encOut;
    VCEncRet status;
    struct FrameOutput frame;
    char *qpDeltaBuf;
    encOutForCutree_s outForCutree;
    ptr_t pRoiMapDeltaQpAddr; //save pRoiMapDeltaQpAddr sent in by test bench
    void *pCodingCtrlParam;   //pointer to coding ctrl parameters of this frame
} VCEncLookaheadJob;

/* lowres buffers, sizes and strides */
struct Lowres {
    struct node *next;
    //pixel *buffer[4];

    int frameNum;  // Presentation frame number
    int poc;       // Presentation frame number
    int sliceType; // Slice type decided by lookahead
    int qp;
    //    int    width;            // width of lowres frame in pixels
    //    int    lines;            // height of lowres frame in pixel lines
    //    int    leadingBframes;   // number of leading B frames for P or I
    //
    //    bool   bScenecut;        // Set to false if the frame cannot possibly be part of a real scenecut.
    //    bool   bKeyframe;
    //    bool   bLastMiniGopBFrame;
    //
    //    double ipCostRatio;
    //
    //    /* lookahead output data */
    //    int64_t   costEst[MAX_GOP_SIZE + 2][MAX_GOP_SIZE + 2];
    //    int64_t   costEstAq[MAX_GOP_SIZE + 2][MAX_GOP_SIZE + 2];
    //    int32_t*  rowSatds[MAX_GOP_SIZE + 2][MAX_GOP_SIZE + 2];
    //    int       intraMbs[MAX_GOP_SIZE + 2];
    int32_t *intraCost;
    //    uint8_t*  intraMode;
    //    int64_t   satdCost;
    //    uint16_t* lowresCostForRc;
    uint32_t *lowresCosts[MAX_GOP_SIZE + 2][MAX_GOP_SIZE + 2];
    //    int32_t*  lowresMvCosts[2][MAX_GOP_SIZE + 2];
    struct MV *lowresMvs[2][MAX_GOP_SIZE + 2];
    //    uint32_t  maxBlocksInRow;
    //    uint32_t  maxBlocksInCol;
    uint32_t maxBlocksInRowFullRes;
    //    uint32_t  maxBlocksInColFullRes;
    //
    //    /* used for vbvLookahead */
    //    int       plannedType[VCENC_LOOKAHEAD_MAX + 1];
    //    int64_t   plannedSatd[VCENC_LOOKAHEAD_MAX + 1];
    //    int       indB;
    //    int       bframes;
    //
    //    /* rate control / adaptive quant data */
    int32_t *qpAqOffset;     // AQ QP offset values for each 16x16 CU
    int32_t *qpCuTreeOffset; // cuTree QP offset values for each 16x16 CU
                             //    double*   qpAqMotionOffset;
    int *invQscaleFactor;    // qScale values for qp Aq Offsets
    int *invQscaleFactor8x8; // temporary buffer for qg-size 8
                             //    uint32_t* blockVariance;
    //    uint64_t  wp_ssd[3];       // This is different than SSDY, this is sum(pixel^2) - sum(pixel)^2 for entire frame
    //    uint64_t  wp_sum[3];
    //    uint64_t  frameVariance;
    //
    //    /* cutree intermediate data */
    uint32_t *propagateCost;
    int weightedCostDelta[MAX_GOP_SIZE + 2];
    //    //ReferencePlanes weightedRef[MAX_GOP_SIZE + 2];
    //
    i32 p0;
    i32 p1;

    u32 cost;
    i32 predId;
    i32 gopEncOrder;
    char typeChar;
    i32 gopEnd;
    i32 gopSize;

    u32 motionScore[2][2];
    i32 motionNum[2];

    i32 aGopSize;
    i32 hieDepth;
    VCEncLookaheadJob *job;
    u32 cuDataIdx;
    u32 inRoiMapDeltaBinIdx;
    u32 outRoiMapDeltaQpIdx;
    u32 propagateCostIdx;
    u32 outSegmentCountIdx;
};

struct AGopInfo {
    int sliceType; // Slice type decided by lookahead
    i32 gopSize;
    i32 aGopSize;
    u32 motionScore[2][2];
    u32 poc;
};

struct agop_res {
    struct node *next;
    int agop_size;
};
struct cuTreeCtr {
    /* qComp sets the quantizer curve compression factor. It weights the frame
   * quantizer based on the complexity of residual (measured by lookahead).
   * Default value is 0.6. Increasing it to 1 will effectively generate CQP */
    double qCompress;

    /* Enable weighted prediction in B slices. Default is disabled */
    int bEnableWeightedBiPred;

    /* When enabled, the encoder will use the B frame in the middle of each
   * mini-GOP larger than 2 B frames as a motion reference for the surrounding
   * B frames.  This improves compression efficiency for a small performance
   * penalty.  Referenced B frames are treated somewhere between a B and a P
   * frame by rate control.  Default is enabled. */
    int bBPyramid;

    /* The number of frames that must be queued in the lookahead before it may
   * make slice decisions. Increasing this value directly increases the encode
   * latency. The longer the queue the more optimally the lookahead may make
   * slice decisions, particularly with b-adapt 2. When cu-tree is enabled,
   * the length of the queue linearly increases the effectiveness of the
   * cu-tree analysis. Default is 40 frames, maximum is 250 */
    int lookaheadDepth;

    /* Numerator and denominator of frame rate */
    uint32_t fpsNum;
    uint32_t fpsDenom;

    /* Sets the size of the VBV buffer in kilobits. Default is zero */
    int vbvBufferSize;

    /* pre-lookahead */
    int unitSize;
    int unitCount;
    int widthInUnit;
    int heightInUnit;
    int m_cuTreeStrength;
    u32 roiMapEnable;
    u32 width;
    u32 height;
    u32 max_cu_size;
    u32 bHWMultiPassSupport;

    int qgSize;

    int32_t *m_scratch; // temp buffer for cutree propagate
    int frameNum;

    int nLookaheadFrames; // frames in queue
    int lastGopEnd;       // num of frames until last gopEnd
    struct Lowres *lookaheadFramesBase[2 * VCENC_LOOKAHEAD_MAX];
    struct Lowres **lookaheadFrames;

    double costCur;

    double costGop[4];
    i32 FrameNumGop[4];
    double costAvg[4];
    i32 FrameTypeNum[4];
    u32 costGopInt[4];
    u32 costAvgInt[4];

    i32 curTypeChar;
    i32 gopSize;
    i32 nextGopSize;
    i32 bBHierarchy;
    bool bUpdateGop;
    i32 latestGopSize;
    i32 lastGopSize;
    i32 updateAgopFlag;
    i32 maxHieDepth;
    u32 aq_mode;
    float aqStrength;
    i32 cuInfoToRead;

    i32 inQpDeltaBlkSize;
    i32 outRoiMapDeltaQpBlockUnit;
    u32 dsRatio;
    VCEncInst pEncInst;
    EWLLinearMem_t roiMapDeltaQpMemFactory[CUTREE_BUFFER_NUM];
    u32 roiMapRefCnt[CUTREE_BUFFER_NUM];
    EWLLinearMem_t propagateCostMemFactory[CUTREE_BUFFER_CNT(VCENC_LOOKAHEAD_MAX, MAX_GOP_SIZE)];
    u32 propagateCostRefCnt[CUTREE_BUFFER_CNT(VCENC_LOOKAHEAD_MAX, MAX_GOP_SIZE)];
    VCEncVideoCodecFormat codecFormat; /* Video Codec Format: HEVC/H264/AV1 */

    pthread_t *tid_cutree;

    pthread_mutex_t cutree_mutex;
    pthread_cond_t cutree_cond;
    pthread_mutex_t roibuf_mutex;
    pthread_cond_t roibuf_cond;
    pthread_mutex_t cuinfobuf_mutex;
    pthread_cond_t cuinfobuf_cond;
    pthread_mutex_t agop_mutex;
    pthread_cond_t agop_cond;
    pthread_mutex_t stop_mutex;
    pthread_cond_t stop_cond;
    pthread_mutex_t status_mutex;
    THREAD_STATUS bStatus; /* Global status for all thread */
    struct queue jobs;
    struct queue agop;
    i32 job_cnt;
    i32 output_cnt;
    i32 total_frames;
    i32 bStop;

    // for cutree asic
    asicData_s asic;
    ptr_t cuData_Base;
    size_t cuData_frame_size;
    ptr_t inRoiMapDeltaBin_Base;
    size_t inRoiMapDeltaBin_frame_size;
    ptr_t outRoiMapDeltaQp_Base;
    size_t outRoiMapDeltaQp_frame_size;
    ptr_t propagateCost_Base;
    size_t propagateCost_frame_size;
    struct FrameOutput output[MAX_GOP_SIZE + 1];
    ptr_t aqDataBase;
    size_t aqDataFrameSize;
    u32 aqDataStride;
    i32 rem_frames;
    u64 commands[CUTREE_BUFFER_CNT(VCENC_LOOKAHEAD_MAX, MAX_GOP_SIZE) +
                 MAX_GOP_SIZE]; // extra commands for IBBBPBBBP with GOP4 to GOP8 convertion
    i32 num_cmds;

    u32 imFrameCostRefineEn;

    void *ctx;
    u32 slice_idx;
    u32 out_cnt; /* output frame count */
    u32 pop_cnt; /* number of frames to remove from queue */
    i32 qpOutIdx[CUTREE_BUFFER_NUM];

    /* Segment count */
    u32 segmentCountEnable;
    size_t outRoiMapSegmentCountOffset; /* Offset inside roibuffer for vp9 segment */
    int segment_qp[MAX_SEGMENTS];
    u32 *segmentCountVirtualBase; /* Virtual base address for vp9 segment count */
};

#ifndef CUTREE_BUILD_SUPPORT
//Public API
#define cuTreeInit(m_param, inst, config) (0) //VCENC_OK=0
#define cuTreeAddFrame(inst, job, agopInfoBuf, gopSizeFromUser) (0)
#define PutRoiMapBufferToBufferPool(m_param, addr)
#define cuTreeRelease(m_param, error)
#define cuTreeFlush(m_param)
#define getFramePredId(type) (0)
#define getPass1UpdatedGopSize(inst) (0)
#define waitCuInfoBufPass1(vcenc_instance) (0)
/*Lookahead releated */
/* Initialization & release */
#define GetRoiMapBufferFromBufferPool(m_param, busAddr) (NULL)
#define StartLookaheadThread(lookahead) (0)
#define TerminateLookaheadThread(lookahead, error) (0)
#define AddPictureToLookahead(vcenc_instance, pEncIn, pEncOut) (0)
#define GetLookaheadOutput(lookahead, bFlush) (NULL)
#define ReleaseLookaheadPicture(lookahead, output, inputbufBusAddr)
#define LookaheadEnqueueOutput(lookahead, output) (1) //HANTRO_TRUE=1
#define DestroyThread(lookahead, m_param)
#define StopLookaheadThread(p2_lookahead, error) (0)
#define StopCuTreeThread(m_param, error) (0)
#define lookaheadClear(lookahead, pendingJob) (0)
#define cuTreeClear(m_param) (0)
/* cu tree asic */
#define VCEncCuTreeInit(m_param) (0)
#define VCEncCuTreeProcessOneFrame(m_param) (0)
#define VCEncCuTreeRelease(pEncInst) (0)
#define TerminateCuTreeThread(m_param, error) (0)
#define StartCuTreeThread(m_param) (0)
#else
//Public API
/*
  called by VCEncStrmStart()
 */
VCEncRet cuTreeInit(struct cuTreeCtr *m_param, VCEncInst inst, const VCEncConfig *config);

/*
  called by VCEncStrmEncode()
 */
VCEncRet cuTreeAddFrame(VCEncInst inst, VCEncLookaheadJob *job, void *agopInfoBuf,
                        i32 gopSizeFromUser);

/*
  called by VCEncStrmEnd()
 */
void PutRoiMapBufferToBufferPool(struct cuTreeCtr *m_param, ptr_t addr);
void cuTreeRelease(struct cuTreeCtr *m_param, u8 error);
void cuTreeFlush(struct cuTreeCtr *m_param);
i32 getFramePredId(i32 type);
i32 getPass1UpdatedGopSize(VCEncInst inst);
VCEncRet waitCuInfoBufPass1(struct vcenc_instance *vcenc_instance);

/*
 * Lookahead releated
 */

/* Initialization & release */
u8 *GetRoiMapBufferFromBufferPool(struct cuTreeCtr *m_param, ptr_t *busAddr);
VCEncRet StartLookaheadThread(VCEncLookahead *lookahead);
VCEncRet TerminateLookaheadThread(VCEncLookahead *lookahead, u8 error);
VCEncRet AddPictureToLookahead(struct vcenc_instance *vcenc_instance, const VCEncIn *pEncIn,
                               VCEncOut *pEncOut);
VCEncLookaheadJob *GetLookaheadOutput(VCEncLookahead *lookahead, bool bFlush);
void ReleaseLookaheadPicture(VCEncLookahead *lookahead, VCEncLookaheadJob *output,
                             ptr_t inputbufBusAddr);
bool LookaheadEnqueueOutput(VCEncLookahead *lookahead, VCEncLookaheadJob *output);
void DestroyThread(VCEncLookahead *lookahead, struct cuTreeCtr *m_param);
VCEncRet StopLookaheadThread(VCEncLookahead *p2_lookahead, u8 error);
VCEncRet StopCuTreeThread(struct cuTreeCtr *m_param, u8 error);
VCEncRet lookaheadClear(VCEncLookahead *lookahead, VCEncLookaheadJob **pendingJob);
VCEncRet cuTreeClear(struct cuTreeCtr *m_param);

/* cu tree asic */
VCEncRet VCEncCuTreeInit(struct cuTreeCtr *m_param);
VCEncRet VCEncCuTreeProcessOneFrame(struct cuTreeCtr *m_param);
VCEncRet VCEncCuTreeRelease(struct cuTreeCtr *pEncInst);
VCEncRet TerminateCuTreeThread(struct cuTreeCtr *m_param, u8 error);
VCEncRet StartCuTreeThread(struct cuTreeCtr *m_param);
#endif

#endif
