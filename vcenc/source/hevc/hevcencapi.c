/*------------------------------------------------------------------------------
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                                                                            --
--                            Verisilicon.                                    --
--                                                                            --
--                   (C) COPYRIGHT 2019 VERISILICON                           --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
--------------------------------------------------------------------------------
--
--  Abstract : VC Encoder API
--
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
       Version Information
------------------------------------------------------------------------------*/
#include <math.h> /* for pow */
#include "vsi_string.h"
#include "osal.h"

#include "vc8000e_version.h"

#include "hevcencapi.h"
#include "hevcencapi_utils.h"

#include "base_type.h"
#include "error.h"
#include "sw_parameter_set.h"
#include "rate_control_picture.h"
#include "sw_slice.h"
#include "tools.h"
#include "pool.h"
#include "enccommon.h"
#include "hevcApiVersion.h"
#include "enccfg.h"
#include "hevcenccache.h"
#include "encdec400.h"
#include "axife.h"
#include "apbfilter.h"
#include "ewl.h"

#include "sw_put_bits.h"

#ifdef SUPPORT_VP9
#include "vp9encapi.h"
#endif

#ifdef SUPPORT_AV1
#include "av1encapi.h"
#endif

#ifdef INTERNAL_TEST
#include "sw_test_id.h"
#endif

#ifdef TEST_DATA
#include "enctrace.h"
#endif

#include "enctrace.h"

#ifdef VIDEOSTAB_ENABLED
#include "vidstabcommon.h"
#endif

typedef enum {
    RCROIMODE_RC_ROIMAP_ROIAREA_DIABLE = 0,
    RCROIMODE_ROIAREA_ENABLE = 1,
    RCROIMODE_ROIMAP_ENABLE = 2,
    RCROIMODE_ROIMAP_ROIAREA_ENABLE = 3,
    RCROIMODE_RC_ENABLE = 4,
    RCROIMODE_RC_ROIAREA_ENABLE = 5,
    RCROIMODE_RC_ROIMAP_ENABLE = 6,
    RCROIMODE_RC_ROIMAP_ROIAREA_ENABLE = 7

} RCROIMODEENABLE;

const u64 VCEncMaxSBPSHevc[HEVC_LEVEL_NUM] = {
    552960,    3686400,   7372800,    16588800,   33177600,   66846720,  133693440,
    267386880, 534773760, 1069547520, 1069547520, 2139095040, 4278190080};
const u64 VCEncMaxSBPSH264[H264_LEVEL_NUM] = {
    1485 * 256,   1485 * 256,    3000 * 256,    6000 * 256,    11880 * 256,
    11880 * 256,  19800 * 256,   20250 * 256,   40500 * 256,   108000 * 256,
    216000 * 256, 245760 * 256,  245760 * 256,  522240 * 256,  589824 * 256,
    983040 * 256, 2073600 * 256, 4177920 * 256, 8355840 * 256, 16711680ULL * 256};
const u64 VCEncMaxSBPSAV1[AV1_LEVEL_NUM] = {
    5529600,    10454400,   0,          0,          24969600,  39938400,  0,          0,
    77856768,   155713536,  0,          0,          273715200, 547430400, 1094860800, 1176502272,
    1176502272, 2189721600, 4379443200, 4706009088, 0,         0,         0,          0};
const u64 VCEncMaxSBPSVP9[VP9_LEVEL_NUM] = {
    829440,    2764800,   4608000,   9216000,    20736000,   36864000,   83558400,
    160432128, 311951360, 588251136, 1176502272, 1176502272, 2353004544, 4706009088};

const u32 VCEncMaxPicSizeHevc[HEVC_LEVEL_NUM] = {36864,    122880,   245760,  552960,  983040,
                                                 2228224,  2228224,  8912896, 8912896, 8912896,
                                                 35651584, 35651584, 35651584};
const u32 VCEncMaxFSH264[H264_LEVEL_NUM] = {
    99 * 256,    99 * 256,    396 * 256,   396 * 256,    396 * 256,    396 * 256,   792 * 256,
    1620 * 256,  1620 * 256,  3600 * 256,  5120 * 256,   8192 * 256,   8192 * 256,  8704 * 256,
    22080 * 256, 36864 * 256, 36864 * 256, 139264 * 256, 139264 * 256, 139264 * 256};
const u32 VCEncMaxPicSizeAV1[AV1_LEVEL_NUM] = {
    147456,   278784,   0,        0,        665856,  1065024, 0,       0,
    2359296,  2359296,  0,        0,        8912896, 8912896, 8912896, 8912896,
    35651584, 35651584, 35651584, 35651584, 0,       0,       0,       0};
const u32 VCEncMaxPicSizeVP9[VP9_LEVEL_NUM] = {36864,   73728,    122880,   245760,  552960,
                                               983040,  2228224,  2228224,  8912896, 8912896,
                                               8912896, 35651584, 35651584, 35651584};

const u32 VCEncMaxCPBSHevc[HEVC_LEVEL_NUM] = {350000,   1500000,   3000000,  6000000,  10000000,
                                              12000000, 20000000,  25000000, 40000000, 60000000,
                                              60000000, 120000000, 240000000};
const u32 VCEncMaxCPBSH264[H264_LEVEL_NUM] = {
    175000,    350000,    500000,    1000000,   2000000,   2000000,  4000000,
    4000000,   10000000,  14000000,  20000000,  25000000,  62500000, 62500000,
    135000000, 240000000, 240000000, 240000000, 480000000, 800000000};
const u32 VCEncMaxCPBSHighTierHevc[HEVC_LEVEL_NUM] = {
    350000,    1500000,   3000000,   6000000,   10000000,  30000000, 50000000,
    100000000, 160000000, 240000000, 240000000, 480000000, 800000000};
const u32 VCEncMaxCPBSAV1[AV1_LEVEL_NUM] = {
    1500000,  3000000,   0,         0,         6000000,  10000000, 0,        0,
    12000000, 20000000,  0,         0,         30000000, 40000000, 60000000, 60000000,
    60000000, 100000000, 160000000, 160000000, 0,        0,        0,        0};
const u32 VCEncMaxCPBSHighTierAV1[AV1_LEVEL_NUM] = {
    0,         0,         0,         0,         0,         0,         0,         0,
    30000000,  50000000,  0,         0,         100000000, 160000000, 240000000, 240000000,
    240000000, 480000000, 800000000, 800000000, 0,         0,         0,         0};
const u32 VCEncMaxCPBSVP9[VP9_LEVEL_NUM] = {400000,   1000000,  1500000,  2800000,  6000000,
                                            10000000, 16000000, 18000000, 36000000, 46000000,
                                            0,        0,        0,        0};

const u32 VCEncMaxBRHevc[HEVC_LEVEL_NUM] = {128000,   1500000,   3000000,  6000000,  10000000,
                                            12000000, 20000000,  25000000, 40000000, 60000000,
                                            60000000, 120000000, 240000000};

const u32 VCEncMaxBRHighTierHevc[HEVC_LEVEL_NUM] = {
    128000,    1500000,   3000000,   6000000,   10000000,  30000000, 50000000,
    100000000, 160000000, 240000000, 240000000, 480000000, 800000000};

const u32 VCEncMaxBRH264[H264_LEVEL_NUM] = {64000,     128000,    192000,    384000,    768000,
                                            2000000,   4000000,   4000000,   10000000,  14000000,
                                            20000000,  20000000,  50000000,  50000000,  135000000,
                                            240000000, 240000000, 240000000, 480000000, 800000000};
const u32 VCEncMaxBRVP9[VP9_LEVEL_NUM] = {200000,    800000,    1800000,   3600000,  7200000,
                                          12000000,  18000000,  30000000,  60000000, 120000000,
                                          180000000, 180000000, 240000000, 480000000};

static const u32 VCEncIntraPenalty[52] = {
    24,  24,   24,   26,   27,   30,   32,   35,   39,   43,   48,   53,    58,
    64,  71,   78,   85,   93,   102,  111,  121,  131,  142,  154,  167,   180,
    195, 211,  229,  248,  271,  296,  326,  361,  404,  457,  523,  607,   714,
    852, 1034, 1272, 1588, 2008, 2568, 3318, 4323, 5672, 7486, 9928, 13216, 17648}; //max*3=16bit

static const u32 lambdaSadDepth0Tbl[] = {
    0x00000100, 0x0000011f, 0x00000143, 0x0000016a, 0x00000196, 0x000001c8, 0x00000200, 0x0000023f,
    0x00000285, 0x000002d4, 0x0000032d, 0x00000390, 0x00000400, 0x0000047d, 0x0000050a, 0x000005a8,
    0x00000659, 0x00000721, 0x00000800, 0x000008fb, 0x00000a14, 0x00000b50, 0x00000cb3, 0x00000e41,
    0x00001000, 0x000011f6, 0x00001429, 0x000016a1, 0x00001966, 0x00001c82, 0x00002000, 0x000023eb,
    0x00002851, 0x00002d41, 0x000032cc, 0x00003904, 0x00004000, 0x000047d6, 0x000050a3, 0x00005a82,
    0x00006598, 0x00007209, 0x00008000, 0x00008fad, 0x0000a145, 0x0000b505, 0x0000cb30, 0x0000e412,
    0x00010000, 0x00011f5a, 0x0001428a, 0x00016a0a};

static const u32 lambdaSseDepth0Tbl[] = {
    0x00000040, 0x00000051, 0x00000066, 0x00000080, 0x000000a1, 0x000000cb, 0x00000100, 0x00000143,
    0x00000196, 0x00000200, 0x00000285, 0x0000032d, 0x00000400, 0x0000050a, 0x00000659, 0x00000800,
    0x00000a14, 0x00000cb3, 0x00001000, 0x00001429, 0x00001966, 0x00002000, 0x00002851, 0x000032cc,
    0x00004000, 0x000050a3, 0x00006598, 0x00008000, 0x0000a145, 0x0000cb30, 0x00010000, 0x0001428a,
    0x00019660, 0x00020000, 0x00028514, 0x00032cc0, 0x00040000, 0x00050a29, 0x00065980, 0x00080000,
    0x000a1451, 0x000cb2ff, 0x00100000, 0x001428a3, 0x001965ff, 0x00200000, 0x00285146, 0x0032cbfd,
    0x00400000, 0x0050a28c, 0x006597fb, 0x00800000};

static const u32 lambdaSadDepth1Tbl[] = {
    0x0000016a, 0x00000196, 0x000001c8, 0x00000200, 0x0000023f, 0x00000285, 0x000002d4, 0x0000032d,
    0x00000390, 0x00000400, 0x0000047d, 0x0000050a, 0x000005a8, 0x00000659, 0x00000721, 0x00000800,
    0x000008fb, 0x00000a14, 0x00000b50, 0x00000cb3, 0x00000e41, 0x00001000, 0x000011f6, 0x00001429,
    0x000016a1, 0x00001a6f, 0x00001ecb, 0x000023c7, 0x0000297a, 0x00002ffd, 0x0000376d, 0x00003feb,
    0x0000499c, 0x000054aa, 0x00006145, 0x00006fa2, 0x00008000, 0x00008fad, 0x0000a145, 0x0000b505,
    0x0000cb30, 0x0000e412, 0x00010000, 0x00011f5a, 0x0001428a, 0x00016a0a, 0x00019660, 0x0001c824,
    0x00020000, 0x00023eb3, 0x00028514, 0x0002d414};

static const u32 lambdaSseDepth1Tbl[] = {
    0x00000080, 0x000000a1, 0x000000cb, 0x00000100, 0x00000143, 0x00000196, 0x00000200, 0x00000285,
    0x0000032d, 0x00000400, 0x0000050a, 0x00000659, 0x00000800, 0x00000a14, 0x00000cb3, 0x00001000,
    0x00001429, 0x00001966, 0x00002000, 0x00002851, 0x000032cc, 0x00004000, 0x000050a3, 0x00006598,
    0x00008000, 0x0000aeb6, 0x0000ed0d, 0x00014000, 0x0001ae0e, 0x00023fb3, 0x00030000, 0x0003fd60,
    0x00054a95, 0x00070000, 0x00093d4b, 0x000c2b8a, 0x00100000, 0x001428a3, 0x001965ff, 0x00200000,
    0x00285146, 0x0032cbfd, 0x00400000, 0x0050a28c, 0x006597fb, 0x00800000, 0x00a14518, 0x00cb2ff5,
    0x01000000, 0x01428a30, 0x01965fea, 0x02000000};

static const float sqrtPow2QpDiv3[] = {
    /* for computing lambda_sad
   * for(int qp = 0; qp <= 63; qp++)
   *   sqrtPow2QpDiv3[i] = sqrt(pow(2.0, (qp-12)/3.0));
   */
    0.250000000,   0.280615512,   0.314980262,   0.353553391,   0.396850263,   0.445449359,
    0.500000000,   0.561231024,   0.629960525,   0.707106781,   0.793700526,   0.890898718,
    1.000000000,   1.122462048,   1.259921050,   1.414213562,   1.587401052,   1.781797436,
    2.000000000,   2.244924097,   2.519842100,   2.828427125,   3.174802104,   3.563594873,
    4.000000000,   4.489848193,   5.039684200,   5.656854249,   6.349604208,   7.127189745,
    8.000000000,   8.979696386,   10.079368399,  11.313708499,  12.699208416,  14.254379490,
    16.000000000,  17.959392773,  20.158736798,  22.627416998,  25.398416831,  28.508758980,
    32.000000000,  35.918785546,  40.317473597,  45.254833996,  50.796833663,  57.017517961,
    64.000000000,  71.837571092,  80.634947193,  90.509667992,  101.593667326, 114.035035922,
    128.000000000, 143.675142184, 161.269894387, 181.019335984, 203.187334652, 228.070071844,
    256.000000000, 287.350284367, 322.539788773, 362.038671968};
static const float sqrtQpDiv6[] = {
    /* for computing lambda_sad
   * for(int qp = 0; qp <= 63; qp++)
   *   sqrtQpDiv6[i] = sqrt(CLIP3(2.0, 4.0, (qp-12) / 6.0));
   */
    1.414213562, 1.414213562, 1.414213562, 1.414213562, 1.414213562, 1.414213562, 1.414213562,
    1.414213562, 1.414213562, 1.414213562, 1.414213562, 1.414213562, 1.414213562, 1.414213562,
    1.414213562, 1.414213562, 1.414213562, 1.414213562, 1.414213562, 1.414213562, 1.414213562,
    1.414213562, 1.414213562, 1.414213562, 1.414213562, 1.471960144, 1.527525232, 1.581138830,
    1.632993162, 1.683250823, 1.732050808, 1.779513042, 1.825741858, 1.870828693, 1.914854216,
    1.957890021, 2.000000000, 2.000000000, 2.000000000, 2.000000000, 2.000000000, 2.000000000,
    2.000000000, 2.000000000, 2.000000000, 2.000000000, 2.000000000, 2.000000000, 2.000000000,
    2.000000000, 2.000000000, 2.000000000, 2.000000000, 2.000000000, 2.000000000, 2.000000000,
    2.000000000, 2.000000000, 2.000000000, 2.000000000, 2.000000000, 2.000000000, 2.000000000,
    2.000000000};

bool multipass_enabled = HANTRO_FALSE;

#ifdef CDEF_BLOCK_STRENGTH_ENABLE
// [pass][luma/chroma][QP][strength]
u32 cdef_stats[2][2][64];

// [pass][luma_strength][chroma_strength]
u32 cdef_union[2][64][64];

// [pass][index]
//u8 cdef_y_strength[2][8];
//u8 cdef_uv_strength[2][8];
#endif

#define SLICE_TYPE_P 0
#define SLICE_TYPE_B 1
#define SLICE_TYPE_I 2
#define SLICE_TYPE_SP 3
#define SLICE_TYPE_SI 4

/*------------------------------------------------------------------------------
    4. Local function prototypes
------------------------------------------------------------------------------*/
static i32 out_of_memory(struct vcenc_buffer *n, u32 size);
static i32 get_buffer(struct buffer *, struct vcenc_buffer *, i32 size, i32 reset);
static i32 init_buffer(struct vcenc_buffer *, struct vcenc_instance *);
static i32 set_parameter(struct vcenc_instance *vcenc_instance, const VCEncIn *pEncIn,
                         struct vps *v, struct sps *s, struct pps *p);
static bool_e VCEncCheckCfg(const VCEncConfig *enc_cfg, const EWLHwConfig_t *asic_cfg, void *ctx);
static bool_e SetParameter(struct vcenc_instance *inst, const VCEncConfig *pEncCfg);
static i32 vcenc_set_ref_pic_set(struct vcenc_instance *vcenc_instance);
static i32 vcenc_ref_pic_sets(struct vcenc_instance *vcenc_instance, const VCEncIn *pEncIn);
static i32 VCEncGetAllowedWidth(i32 width, VCEncPictureType inputType);
static u32 VCEncGetMaxWidth(const EWLHwConfig_t *asic_cfg, VCEncVideoCodecFormat codec_format);

static void IntraLamdaQpFixPoint(int qp, u32 *lamda_sad, enum slice_type type, int poc,
                                 u32 qpFactorSad, bool depth0);
static void InterLamdaQpFixPoint(int qp, unsigned int *lamda_sse, unsigned int *lamda_sad,
                                 enum slice_type type, u32 qpFactorSad, u32 qpFactorSse,
                                 bool depth0, u32 asicId);
static void IntraLamdaQp(int qp, u32 *lamda_sad, enum slice_type type, int poc, float dQPFactor,
                         bool depth0);
static void InterLamdaQp(int qp, unsigned int *lamda_sse, unsigned int *lamda_sad,
                         enum slice_type type, float dQPFactor, bool depth0, u32 asicId);
static void FillIntraFactor(regValues_s *regs);
static void LamdaTableQp(regValues_s *regs, int qp, enum slice_type type, int poc, double dQPFactor,
                         bool depth0, u32 ctbRc);
static void VCEncSetQuantParameters(struct vcenc_instance *vcenc_instance, struct sw_picture *pic,
                                    const VCEncIn *pEncIn, double qp_factor, bool is_depth0);
static void VCEncSetNewFrame(VCEncInst inst);

/* DeNoise Filter */
static void VCEncHEVCDnfInit(struct vcenc_instance *vcenc_instance);
static void VCEncHEVCDnfSetParameters(struct vcenc_instance *vcenc_instance,
                                      const VCEncCodingCtrl *pCodeParams);
static void VCEncHEVCDnfGetParameters(struct vcenc_instance *vcenc_instance,
                                      VCEncCodingCtrl *pCodeParams);
static void VCEncHEVCDnfPrepare(struct vcenc_instance *vcenc_instance);
static void VCEncHEVCDnfUpdate(struct vcenc_instance *vcenc_instance);

/* RPS in slice header*/
static void VCEncGenSliceHeaderRps(struct vcenc_instance *vcenc_instance,
                                   VCEncPictureCodingType codingType, VCEncGopPicConfig *cfg);
static void VCEncGenPicRefConfig(struct container *c, VCEncGopPicConfig *cfg,
                                 struct sw_picture *pCurPic, i32 curPicPoc);

/* Parameter set send handler */
static void VCEncParameterSetHandle(struct vcenc_instance *vcenc_instance, VCEncInst inst,
                                    const VCEncIn *pEncIn, VCEncOut *pEncOut, struct container *c);

static i32 VCEncWriteFillerDataForCBR(struct vcenc_instance *vcenc, i32 stat);
static void VCEncAddFillerNALU(struct vcenc_instance *vcenc, i32 cnt, true_e has_startcode);

static VCEncLevel getLevel(VCEncVideoCodecFormat codecFormat, i32 levelIdx);
static VCEncLevel getLevelHevc(i32 levelIdx);
static VCEncLevel getLevelH264(i32 levelIdx);
static i32 getLevelIdx(VCEncVideoCodecFormat codecFormat, VCEncLevel levelIdx);
static i32 getlevelIdxHevc(VCEncLevel level);
static i32 getlevelIdxH264(VCEncLevel level);
static u64 getMaxSBPS(VCEncVideoCodecFormat codecFormat, i32 levelIdx);
static u32 getMaxPicSize(VCEncVideoCodecFormat codecFormat, i32 levelIdx);
static u32 getMaxCPBS(VCEncVideoCodecFormat codecFormat, i32 levelIdx, i32 profile, i32 tier);
static u32 getMaxBR(VCEncVideoCodecFormat codecFormat, i32 levelIdx, i32 profile, i32 tier);
static void VCEncCollectEncodeStats(struct vcenc_instance *vcenc_instance, VCEncIn *pEncIn,
                                    VCEncOut *pEncOut, VCEncPictureCodingType codingType,
                                    VCEncStatisticOut *enc_statistic);
static VCEncRet VCEncResetEncodeStatus(struct vcenc_instance *vcenc_instance, struct container *c,
                                       VCEncIn *pEncIn, struct sw_picture *pic, VCEncOut *pEncOut,
                                       const VCEncStatisticOut *enc_statistic);
static void VCEncRertryNewParameters(struct vcenc_instance *vcenc_instance, VCEncIn *pEncIn,
                                     VCEncOut *pEncOut, VCEncSliceReady *slice_callback,
                                     NewEncodeParams *new_params, regValues_s *regs_for2nd_encode);

/*------------------------------------------------------------------------------

    Function name : VCEncGetApiVersion
    Description   : Return the API version info

    Return type   : VCEncApiVersion
    Argument      : void
------------------------------------------------------------------------------*/
VCEncApiVersion VCEncGetApiVersion(void)
{
    VCEncApiVersion ver;

    ver.major = VCENC_MAJOR_VERSION;
    ver.minor = VCENC_MINOR_VERSION;
    ver.clnum = VCENC_BUILD_CLNUM;

    // APITRACE("VCEncGetApiVersion# OK\n");
    return ver;
}

/*------------------------------------------------------------------------------
    Function name : VCEncGetBuild
    Description   : Return the SW and HW build information.

    Return type   : VCEncBuild
    Argument      : client_type
------------------------------------------------------------------------------*/
VCEncBuild VCEncGetBuild(u32 client_type)
{
    VCEncBuild ver;

    ver.swBuild = VCENC_SW_BUILD;
    ver.hwBuild = EncAsicGetAsicHWid(client_type, NULL);

    // APITRACE("VCEncGetBuild# OK\n");

    return (ver);
}

/*------------------------------------------------------------------------------
  VCEncInit initializes the encoder and creates new encoder instance.
------------------------------------------------------------------------------*/
VCEncRet VCEncInit(const VCEncConfig *config, VCEncInst *instAddr, void *ctx)
{
    struct instance *instance;
    const void *ewl = NULL;
    EWLInitParam_t param;
    struct vcenc_instance *vcenc_inst = NULL;
    struct container *c;
    u32 client_type;
    asicMemAlloc_s allocCfg;
    i32 parallelCoreNum = config->parallelCoreNum;
    u32 exp_of_input_alignment = config->exp_of_input_alignment;
    u32 exp_of_ref_alignment = config->exp_of_ref_alignment;
    u32 exp_of_ref_ch_alignment = config->exp_of_ref_ch_alignment;
    u32 exp_of_aqinfo_alignment = config->exp_of_aqinfo_alignment;
    u32 exp_of_tile_stream_alignment = config->exp_of_tile_stream_alignment;
    VCEncTryNewParamsCallBackFunc cb_try_new_params = config->cb_try_new_params;

    u32 hw_id;

    EWLHwConfig_t asic_cfg;

    regValues_s *regs = NULL;
    preProcess_s *pPP;
    struct vps *v;
    struct sps *s;
    struct pps *p;
    VCEncRet ret;
    int i;
    u32 coreId, tileId;

    // VCEncLogInit();
    APITRACE("VCEncInit#\n");

    APITRACEPARAM(" %s : %d\n", "streamType", config->streamType);
    APITRACEPARAM(" %s : %d\n", "profile", config->profile);
    APITRACEPARAM(" %s : %d\n", "level", config->level);
    APITRACEPARAM(" %s : %d\n", "width", config->width);
    APITRACEPARAM(" %s : %d\n", "height", config->height);
    APITRACEPARAM(" %s : %d\n", "frameRateNum", config->frameRateNum);
    APITRACEPARAM(" %s : %d\n", "frameRateDenom", config->frameRateDenom);
    APITRACEPARAM(" %s : %d\n", "refFrameAmount", config->refFrameAmount);
    APITRACEPARAM(" %s : %d\n", "strongIntraSmoothing", config->strongIntraSmoothing);
    APITRACEPARAM(" %s : %d\n", "compressor", config->compressor);
    APITRACEPARAM(" %s : %d\n", "interlacedFrame", config->interlacedFrame);
    APITRACEPARAM(" %s : %d\n", "bitDepthLuma", config->bitDepthLuma);
    APITRACEPARAM(" %s : %d\n", "bitDepthChroma", config->bitDepthChroma);
    APITRACEPARAM(" %s : %d\n", "outputCuInfo", config->enableOutputCuInfo);
    APITRACEPARAM(" %s : %d\n", "SSIM", config->enableSsim);
    APITRACEPARAM(" %s : %d\n", "PSNR", config->enablePsnr);
    APITRACEPARAM(" %s : %d\n", "ctbRcMode", config->ctbRcMode);

    /* check that right shift on negative numbers is performed signed */
    /*lint -save -e* following check causes multiple lint messages */
#if (((-1) >> 1) != (-1))
#error Right bit-shifting (>>) does not preserve the sign
#endif
    /*lint -restore */

    /* Check for illegal inputs */
    if (config == NULL || instAddr == NULL) {
        APITRACEERR("VCEncInit: ERROR Null argument\n");
        return VCENC_NULL_ARGUMENT;
    }

    for (tileId = 0; tileId < config->num_tile_columns; tileId++) {
        if (config->tiles_enabled_flag == 1 && config->tile_width[tileId] * 64 < 256) {
            APITRACEERR("VCEncInit: Tile column width needs to be greater than or equal to "
                        "256\n");
            return VCENC_INVALID_ARGUMENT;
        }
    }

    for (tileId = 0; tileId < config->num_tile_rows; tileId++) {
        if (config->tiles_enabled_flag == 1 && config->tile_height[tileId] * 64 < 64) {
            APITRACEERR("VCEncInit: Tile row height needs to be greater than or equal to 64 "
                        "\n");
            return VCENC_INVALID_ARGUMENT;
        }
    }
    if (config->tiles_enabled_flag == 1 && config->num_tile_columns > 1 && cb_try_new_params) {
        APITRACE("VCEncInit: Disable re-encode for multi-tile\n");
        cb_try_new_params = NULL;
    }

    if (parallelCoreNum > 1 && config->width * config->height < 256 * 256) {
        APITRACE("VCEncInit: Use single core for small resolution\n");
        parallelCoreNum = 1;
    }

    if (exp_of_input_alignment < 6) {
        APITRACE("VCEncInit: HW request alignment 64\n");
        exp_of_input_alignment = 6;
    }

    if (exp_of_ref_alignment < 6) {
        APITRACE("VCEncInit: HW request alignment 64\n");
        exp_of_ref_alignment = 6;
    }

    if (exp_of_ref_ch_alignment < 6) {
        APITRACE("VCEncInit: HW request alignment 64\n");
        exp_of_ref_ch_alignment = 6;
    }

    if (exp_of_aqinfo_alignment < 6) {
        APITRACE("VCEncInit: HW request alignment 64\n");
        exp_of_aqinfo_alignment = 6;
    }

    if (exp_of_tile_stream_alignment > 15) {
        APITRACE("VCEncInit: HW request alignment 2^15 bytes\n");
        exp_of_tile_stream_alignment = 15;
    }

    /* Init EWL */
    param.context = ctx;
    param.clientType = VCEncGetClientType(config->codecFormat);
    param.slice_idx = config->slice_idx;
    if ((ewl = EWLInit(&param)) == NULL) {
        APITRACEERR("VCEncInit: EWL Initialization failed\n");
        return VCENC_EWL_ERROR;
    }

    *instAddr = NULL;
    if (!(instance = (struct instance *)EWLcalloc(1, sizeof(struct instance)))) {
        ret = VCENC_MEMORY_ERROR;
        goto error;
    }

    instance->instance = (struct vcenc_instance *)instance;
    *instAddr = (VCEncInst)instance;
    ASSERT(instance == (struct instance *)&instance->vcenc_instance);
    vcenc_inst = (struct vcenc_instance *)instance;
    for (coreId = 0; coreId < MAX_CORE_NUM; coreId++) {
        if (config->num_tile_columns > 1) {
            vcenc_inst->EncIn[coreId].tileExtra =
                (VCEncInTileExtra *)calloc(config->num_tile_columns - 1, sizeof(VCEncInTileExtra));
            vcenc_inst->EncOut[coreId].tileExtra = (VCEncOutTileExtra *)calloc(
                config->num_tile_columns - 1, sizeof(VCEncOutTileExtra));
        } else
            vcenc_inst->EncOut[coreId].tileExtra = NULL;
    }

    vcenc_inst->asic.regs.bVCMDAvailable = EWLGetVCMDSupport() ? true : false;
    vcenc_inst->asic.regs.bVCMDEnable = EWLGetVCMDMode(ewl) ? true : false;
    if (vcenc_inst->asic.regs.bVCMDAvailable) {
        if (!(vcenc_inst->asic.regs.vcmdBuf = (u32 *)EWLcalloc(1, VCMDBUFFER_SIZE))) {
            ret = VCENC_MEMORY_ERROR;
            goto error;
        }
    }

    if (cb_try_new_params) {
        if (!(vcenc_inst->regs_bak =
                  (regValues_s *)EWLcalloc(1, parallelCoreNum * sizeof(regValues_s)))) {
            ret = VCENC_MEMORY_ERROR;
            goto error;
        }
    }

    if (!(vcenc_inst->dec400_data_bak =
              (VCDec400data *)EWLcalloc(1, parallelCoreNum * sizeof(VCDec400data)))) {
        ret = VCENC_MEMORY_ERROR;
        goto error;
    }

    if (!(vcenc_inst->dec400_osd_bak =
              (VCDec400data *)EWLcalloc(1, parallelCoreNum * sizeof(VCDec400data)))) {
        ret = VCENC_MEMORY_ERROR;
        goto error;
    }

    vcenc_inst->ctx = ctx;
    vcenc_inst->slice_idx = config->slice_idx;

    if (!(c = get_container(vcenc_inst))) {
        ret = VCENC_MEMORY_ERROR;
        goto error;
    }

    /* Check for correct HW */
    client_type = VCEncGetClientType(config->codecFormat);
    hw_id = HW_ID_PRODUCT(EncAsicGetAsicHWid(client_type, ctx));
    if ((hw_id != HW_ID_PRODUCT_H2) && (hw_id != HW_ID_PRODUCT_VC8000E) &&
        (hw_id != HW_ID_PRODUCT_VC9000E) && (hw_id != HW_ID_PRODUCT_VC9000LE)) {
        if (instance != NULL)
            EWLfree(instance);
        if (ewl != NULL)
            (void)EWLRelease(ewl);
        APITRACEERR("VCEncInit: ERROR codecFormat NOT support by HW\n");
        return VCENC_ERROR;
    }

    /* Check that configuration is valid */
    asic_cfg = EncAsicGetAsicConfig(param.clientType, ctx);
    if (VCEncCheckCfg(config, &asic_cfg, ctx) == ENCHW_NOK) {
        if (instance != NULL)
            EWLfree(instance);
        if (ewl != NULL)
            (void)EWLRelease(ewl);
        APITRACEERR("VCEncInit: ERROR Invalid configuration\n");
        return VCENC_INVALID_ARGUMENT;
    }

    /* Create parameter sets */
    v = (struct vps *)create_parameter_set(VPS_NUT);
    s = (struct sps *)create_parameter_set(SPS_NUT);
    p = (struct pps *)create_parameter_set(PPS_NUT);

    /* Replace old parameter set with new ones */
    remove_parameter_set(c, VPS_NUT, 0);
    remove_parameter_set(c, SPS_NUT, 0);
    remove_parameter_set(c, PPS_NUT, 0);

    queue_put(&c->parameter_set, (struct node *)v);
    queue_put(&c->parameter_set, (struct node *)s);
    queue_put(&c->parameter_set, (struct node *)p);
    vcenc_inst->sps = s;
    vcenc_inst->vps = v;
    vcenc_inst->interlaced = config->interlacedFrame;
    vcenc_inst->insertNewPPS = 0;
    vcenc_inst->maxPPSId = 0;

#ifdef RECON_REF_1KB_BURST_RW
    /*
   1KB_BURST_RW feature enabled, aligment is fixed not changeable
   User must set the config correct, API internal not trick to change it
  */
    ASSERT((config->exp_of_input_alignment == 10) && (config->exp_of_ref_alignment == 10) &&
           (config->exp_of_ref_ch_alignment == 10) && (config->compressor == 2));
#endif

    /* Initialize ASIC */

    vcenc_inst->asic.ewl = ewl;
    (void)EncAsicControllerInit(&vcenc_inst->asic, ctx, param.clientType);

    u32 lookaheadDepth = config->lookaheadDepth;
    u32 pass = config->pass;
    if (lookaheadDepth > 0 && (asic_cfg.cuInforVersion < 1)) {
        // lookahead needs bFrame support & cuInfo version 1
        lookaheadDepth = 0;
        pass = 0;
    }

    vcenc_inst->ori_width = vcenc_inst->width = config->width;
    vcenc_inst->ori_height = vcenc_inst->height = config->height / (vcenc_inst->interlaced + 1);

    vcenc_inst->input_alignment = 1 << exp_of_input_alignment;
    vcenc_inst->ref_alignment = 1 << exp_of_ref_alignment;
    vcenc_inst->ref_ch_alignment = 1 << exp_of_ref_ch_alignment;
    vcenc_inst->aqInfoAlignment = 1 << exp_of_aqinfo_alignment;
    vcenc_inst->tileStrmSizeAlignmentExp = exp_of_tile_stream_alignment;

    /* debug options */
    vcenc_inst->dumpRegister = config->dumpRegister;
    vcenc_inst->dumpCuInfo = config->dumpCuInfo;
    vcenc_inst->dumpCtbBits = config->dumpCtbBits;
    vcenc_inst->rasterscan = config->rasterscan;

    vcenc_inst->writeReconToDDR = config->writeReconToDDR;

    /* register VCEncReEncodeCallBackFunc to vcenc_inst */
    vcenc_inst->cb_try_new_params = cb_try_new_params;

    /* tile */
    vcenc_inst->tiles_enabled_flag = vcenc_inst->asic.regs.tiles_enabled_flag =
        config->tiles_enabled_flag;
    vcenc_inst->num_tile_columns = vcenc_inst->asic.regs.num_tile_columns =
        config->num_tile_columns;
    vcenc_inst->num_tile_rows = vcenc_inst->asic.regs.num_tile_rows = config->num_tile_rows;
    vcenc_inst->loop_filter_across_tiles_enabled_flag =
        vcenc_inst->asic.regs.loop_filter_across_tiles_enabled_flag =
            config->loop_filter_across_tiles_enabled_flag;
    vcenc_inst->tileMvConstraint = vcenc_inst->asic.regs.tileMvConstraint =
        config->tileMvConstraint;
    for (tileId = 0; tileId < vcenc_inst->num_tile_columns; tileId++) {
        vcenc_inst->tileCtrl[tileId].tileLeft =
            tileId ? vcenc_inst->tileCtrl[tileId - 1].tileRight + 1 : 0;
        vcenc_inst->tileCtrl[tileId].tileRight =
            vcenc_inst->tileCtrl[tileId].tileLeft + config->tile_width[tileId] - 1;
        vcenc_inst->tileCtrl[tileId].startTileIdx = tileId;
    }

    /* Allocate internal SW/HW shared memories */
    memset(&allocCfg, 0, sizeof(asicMemAlloc_s));
    allocCfg.width = config->width;
    allocCfg.height = config->height / (vcenc_inst->interlaced + 1);
    allocCfg.encodingType =
        (IS_H264(config->codecFormat)
             ? ASIC_H264
             : (IS_HEVC(config->codecFormat) ? ASIC_HEVC
                                             : IS_VP9(config->codecFormat) ? ASIC_VP9 : ASIC_AV1));
    allocCfg.numRefBuffsLum = allocCfg.numRefBuffsChr =
        config->refFrameAmount + parallelCoreNum - 1;
    allocCfg.compressor = config->compressor;
    allocCfg.outputCuInfo = config->enableOutputCuInfo;
    allocCfg.outputCtbBits = config->enableOutputCtbBits && pass != 1;
    allocCfg.bitDepthLuma = config->bitDepthLuma;
    allocCfg.bitDepthChroma = config->bitDepthChroma;
    allocCfg.input_alignment = vcenc_inst->input_alignment;
    allocCfg.ref_alignment = vcenc_inst->ref_alignment;
    allocCfg.ref_ch_alignment = vcenc_inst->ref_ch_alignment;
    allocCfg.aqInfoAlignment = vcenc_inst->aqInfoAlignment;
    allocCfg.cuinfoAlignment = (config->AXIAlignment >> 32) & 0x000f;
    allocCfg.exteralReconAlloc = config->exteralReconAlloc;
    allocCfg.maxTemporalLayers = config->maxTLayers;
    allocCfg.ctbRcMode = config->ctbRcMode;
    allocCfg.numTileColumns = config->num_tile_columns;
    allocCfg.numTileRows = config->num_tile_rows;
    allocCfg.tileEnable = config->tiles_enabled_flag;

    i32 lookAheadBufCnt = EncInitLookAheadBufCnt(config, lookaheadDepth);
    //multicore delay should be added to lookahead delay to make IM work
    vcenc_inst->lookaheadDelay = lookAheadBufCnt - 1 + parallelCoreNum - 1;
    allocCfg.numCuInfoBuf = vcenc_inst->numCuInfoBuf =
        (pass == 1 && asic_cfg.bMultiPassSupport ? lookAheadBufCnt : 3) + parallelCoreNum - 1;
    allocCfg.numCtbBitsBuf = parallelCoreNum;
    allocCfg.pass = pass;
    //pre-carry buffer for av1
    allocCfg.numAv1PreCarryBuf = parallelCoreNum;
    vcenc_inst->cuInfoBufIdx = 0;
    vcenc_inst->asic.regs.P010RefEnable = config->P010RefEnable;
    if (config->P010RefEnable) {
        if (config->bitDepthLuma > 8)
            allocCfg.bitDepthLuma = 16;
        if (config->bitDepthChroma > 8 && (config->compressor & 2) == 0)
            allocCfg.bitDepthChroma = 16;
    }

    vcenc_inst->compressor = config->compressor; //save these value
    vcenc_inst->maxTLayers = config->maxTLayers;
    vcenc_inst->ctbRcMode = config->ctbRcMode;
    vcenc_inst->outputCuInfo = config->enableOutputCuInfo;
    vcenc_inst->outputCtbBits = config->enableOutputCtbBits && pass != 1;
    vcenc_inst->numCtbBitsBuf = allocCfg.numCtbBitsBuf;

    // TODO,internal memory size alloc according to codedChromaIdc
    allocCfg.codedChromaIdc =
        (vcenc_inst->asic.regs.asicCfg.MonoChromeSupport == EWL_HW_CONFIG_NOT_SUPPORTED)
            ? VCENC_CHROMA_IDC_420
            : config->codedChromaIdc;
    allocCfg.tmvpEnable = config->enableTMVP;
    allocCfg.is_malloc = 1;
    vcenc_inst->asic.regs.mc_sync_enable = (parallelCoreNum > 1);
    vcenc_inst->refRingBufEnable = config->refRingBufEnable;
    vcenc_inst->RefRingBufExtendHeight = REFRINGBUF_EXTEND_HEIGHT;
    allocCfg.refRingBufEnable = config->refRingBufEnable;
    allocCfg.RefRingBufExtendHeight = REFRINGBUF_EXTEND_HEIGHT;
    if (EncAsicMemAlloc_V2(&vcenc_inst->asic, &allocCfg) != ENCHW_OK) {
        ret = VCENC_EWL_MEMORY_ERROR;
        goto error;
    }
    if (config->tiles_enabled_flag) {
        vcenc_inst->tileHeights = (u16 *)vcenc_inst->asic.tileHeightMem.virtualAddress;
        vcenc_inst->asic.regs.tileHeightBase = vcenc_inst->asic.tileHeightMem.busAddress;
        for (tileId = 0; tileId < vcenc_inst->num_tile_rows; tileId++) {
            vcenc_inst->tileHeights[tileId] = config->tile_height[tileId];
        }
    }

    /* Set parameters depending on user config */
    if (SetParameter(vcenc_inst, config) != ENCHW_OK) {
        ret = VCENC_INVALID_ARGUMENT;
        goto error;
    }

    /* tuning video quality configuration */
    VideoTuneConfig(config, vcenc_inst);

    vcenc_inst->rateControl.codingType = ASIC_HEVC;
    vcenc_inst->rateControl.monitorFrames =
        MAX(LEAST_MONITOR_FRAME, config->frameRateNum / config->frameRateDenom);
    vcenc_inst->rateControl.picRc = ENCHW_NO;
    vcenc_inst->rateControl.picSkip = ENCHW_NO;
    vcenc_inst->rateControl.qpHdr = -1 << QP_FRACTIONAL_BITS;
    vcenc_inst->rateControl.ctbRcQpDeltaReverse = 0;
    vcenc_inst->rateControl.qpMin = vcenc_inst->rateControl.qpMinI =
        vcenc_inst->rateControl.qpMinPB = 0 << QP_FRACTIONAL_BITS;
    vcenc_inst->rateControl.qpMax = vcenc_inst->rateControl.qpMaxI =
        vcenc_inst->rateControl.qpMaxPB = 51 << QP_FRACTIONAL_BITS;
    vcenc_inst->rateControl.hrd = ENCHW_NO;
    vcenc_inst->rateControl.virtualBuffer.bitRate = 1000000;
    vcenc_inst->rateControl.virtualBuffer.bufferSize = 0;
    vcenc_inst->rateControl.bitrateWindow = 150;
    vcenc_inst->rateControl.vbr = ENCHW_NO;
    vcenc_inst->rateControl.crf = -1;
    vcenc_inst->rateControl.pbOffset = 6.0 * 0.378512;
    vcenc_inst->rateControl.ipOffset = 6.0 * 0.485427;
    vcenc_inst->rateControl.rcMode = VCE_RC_CVBR;

    vcenc_inst->rateControl.intraQpDelta = INTRA_QPDELTA_DEFAULT << QP_FRACTIONAL_BITS;
    vcenc_inst->rateControl.fixedIntraQp = 0 << QP_FRACTIONAL_BITS;
    vcenc_inst->rateControl.tolMovingBitRate = 100;
    vcenc_inst->rateControl.tolCtbRcInter = -1.0;
    vcenc_inst->rateControl.tolCtbRcIntra = -1.0;
    vcenc_inst->rateControl.longTermQpDelta = 0;
    vcenc_inst->rateControl.f_tolMovingBitRate = 100.0;
    vcenc_inst->rateControl.smooth_psnr_in_gop = 0;

    vcenc_inst->rateControl.maxPicSizeI =
        MIN((((i64)rcCalculate(vcenc_inst->rateControl.virtualBuffer.bitRate,
                               vcenc_inst->rateControl.outRateDenom,
                               vcenc_inst->rateControl.outRateNum)) *
             (1 + 20)),
            I32_MAX);
    vcenc_inst->rateControl.maxPicSizeP =
        MIN((((i64)rcCalculate(vcenc_inst->rateControl.virtualBuffer.bitRate,
                               vcenc_inst->rateControl.outRateDenom,
                               vcenc_inst->rateControl.outRateNum)) *
             (1 + 20)),
            I32_MAX);
    vcenc_inst->rateControl.maxPicSizeB =
        MIN((((i64)rcCalculate(vcenc_inst->rateControl.virtualBuffer.bitRate,
                               vcenc_inst->rateControl.outRateDenom,
                               vcenc_inst->rateControl.outRateNum)) *
             (1 + 20)),
            I32_MAX);

    vcenc_inst->rateControl.minPicSizeI =
        rcCalculate(vcenc_inst->rateControl.virtualBuffer.bitRate,
                    vcenc_inst->rateControl.outRateDenom, vcenc_inst->rateControl.outRateNum) /
        (1 + 20);
    vcenc_inst->rateControl.minPicSizeP =
        rcCalculate(vcenc_inst->rateControl.virtualBuffer.bitRate,
                    vcenc_inst->rateControl.outRateDenom, vcenc_inst->rateControl.outRateNum) /
        (1 + 20);
    vcenc_inst->rateControl.minPicSizeB =
        rcCalculate(vcenc_inst->rateControl.virtualBuffer.bitRate,
                    vcenc_inst->rateControl.outRateDenom, vcenc_inst->rateControl.outRateNum) /
        (1 + 20);
    vcenc_inst->blockRCSize = 0;
    vcenc_inst->rateControl.rcQpDeltaRange = 10;
    vcenc_inst->rateControl.rcBaseMBComplexity = 15;
    vcenc_inst->rateControl.picQpDeltaMin = -4;
    vcenc_inst->rateControl.picQpDeltaMax = 4;
    for (i = 0; i < 3; i++) {
        vcenc_inst->rateControl.ctbRateCtrl.models[i].ctbMemPreAddr =
            vcenc_inst->asic.ctbRcMem[i].busAddress;
        vcenc_inst->rateControl.ctbRateCtrl.models[i].ctbMemPreVirtualAddr =
            vcenc_inst->asic.ctbRcMem[i].virtualAddress;
    }
    vcenc_inst->rateControl.ctbRateCtrl.ctbMemCurAddr = vcenc_inst->asic.ctbRcMem[3].busAddress;
    vcenc_inst->rateControl.ctbRateCtrl.ctbMemCurVirtualAddr =
        vcenc_inst->asic.ctbRcMem[3].virtualAddress;
    vcenc_inst->rateControl.ctbRateCtrl.qpStep =
        ((4 << CTB_RC_QP_STEP_FIXP) + vcenc_inst->rateControl.ctbCols / 2) /
        vcenc_inst->rateControl.ctbCols;

    vcenc_inst->rateControl.pass = vcenc_inst->pass = pass;
    multipass_enabled = (pass > 0);

    if (VCEncInitRc(&vcenc_inst->rateControl, 1) != ENCHW_OK) {
        ret = VCENC_INVALID_ARGUMENT;
        goto error;
    }

    vcenc_inst->rateControl.sei.enabled = ENCHW_NO;
    vcenc_inst->sps->vui.vuiVideoFullRange = ENCHW_NO;
    vcenc_inst->disableDeblocking = 0;
    vcenc_inst->tc_Offset = 0;
    vcenc_inst->beta_Offset = 0;
    vcenc_inst->enableSao = 1;
    vcenc_inst->enableScalingList = 0;
    vcenc_inst->sarWidth = 0;
    vcenc_inst->sarHeight = 0;

    vcenc_inst->codecFormat = config->codecFormat;
    if (IS_H264(vcenc_inst->codecFormat)) {
        vcenc_inst->max_cu_size = 16;   /* Max coding unit size in pixels */
        vcenc_inst->min_cu_size = 8;    /* Min coding unit size in pixels */
        vcenc_inst->max_tr_size = 16;   /* Max transform size in pixels */
        vcenc_inst->min_tr_size = 4;    /* Min transform size in pixels */
        vcenc_inst->tr_depth_intra = 1; /* Max transform hierarchy depth */
        vcenc_inst->tr_depth_inter = 2; /* Max transform hierarchy depth */
        vcenc_inst->log2_qp_size = 4;
    } else {
        vcenc_inst->max_cu_size = 64; /* Max coding unit size in pixels */
        vcenc_inst->min_cu_size = 8;  /* Min coding unit size in pixels */
        vcenc_inst->max_tr_size = 16;
        /* Max transform size in pixels */ /*Default max 16, assume no TU32 support*/
        vcenc_inst->min_tr_size = 4;       /* Min transform size in pixels */
        vcenc_inst->tr_depth_intra = 2;    /* Max transform hierarchy depth */
        vcenc_inst->tr_depth_inter =
            (vcenc_inst->max_cu_size == 64) ? 4 : 3; /* Max transform hierarchy depth */
        vcenc_inst->log2_qp_size = 6;
        if (asic_cfg.tu32Enable) {
            vcenc_inst->max_tr_size = 32;
            vcenc_inst->tr_depth_inter =
                (asic_cfg.tu32Enable && asic_cfg.dynamicMaxTuSize && vcenc_inst->rdoLevel == 0 ? 3
                                                                                               : 2);
        }
    }

    VCEncHEVCDnfInit(vcenc_inst);

    vcenc_inst->fieldOrder = 0;
    vcenc_inst->chromaQpOffset = 0;
    vcenc_inst->rateControl.sei.insertRecoveryPointMessage = 0;
    vcenc_inst->rateControl.sei.recoveryFrameCnt = 0;

    /* low latency */
    vcenc_inst->inputLineBuf.inputLineBufDepth = 1;

    /* smart */
    vcenc_inst->smartModeEnable = 0;
    vcenc_inst->smartH264LumDcTh = 5;
    vcenc_inst->smartH264CbDcTh = 1;
    vcenc_inst->smartH264CrDcTh = 1;
    vcenc_inst->smartHevcLumDcTh[0] = 2;
    vcenc_inst->smartHevcLumDcTh[1] = 2;
    vcenc_inst->smartHevcLumDcTh[2] = 2;
    vcenc_inst->smartHevcChrDcTh[0] = 2;
    vcenc_inst->smartHevcChrDcTh[1] = 2;
    vcenc_inst->smartHevcChrDcTh[2] = 2;
    vcenc_inst->smartHevcLumAcNumTh[0] = 12;
    vcenc_inst->smartHevcLumAcNumTh[1] = 51;
    vcenc_inst->smartHevcLumAcNumTh[2] = 204;
    vcenc_inst->smartHevcChrAcNumTh[0] = 3;
    vcenc_inst->smartHevcChrAcNumTh[1] = 12;
    vcenc_inst->smartHevcChrAcNumTh[2] = 51;
    vcenc_inst->smartH264Qp = 30;
    vcenc_inst->smartHevcLumQp = 30;
    vcenc_inst->smartHevcChrQp = 30;
    vcenc_inst->smartMeanTh[0] = 5;
    vcenc_inst->smartMeanTh[1] = 5;
    vcenc_inst->smartMeanTh[2] = 5;
    vcenc_inst->smartMeanTh[3] = 5;
    vcenc_inst->smartPixNumCntTh = 0;

    vcenc_inst->verbose = config->verbose;

    regs = &vcenc_inst->asic.regs;

    regs->CyclesDdrPollingCheck = 0;
    regs->cabac_init_flag = 0;
    regs->entropy_coding_mode_flag = 1;
    regs->cirStart = 0;
    regs->cirInterval = 0;
    regs->intraAreaTop = regs->intraAreaLeft = regs->intraAreaBottom = regs->intraAreaRight =
        INVALID_POS;

    regs->ipcm1AreaTop = regs->ipcm1AreaLeft = regs->ipcm1AreaBottom = regs->ipcm1AreaRight =
        INVALID_POS;
    regs->ipcm2AreaTop = regs->ipcm2AreaLeft = regs->ipcm2AreaBottom = regs->ipcm2AreaRight =
        INVALID_POS;
    regs->ipcm3AreaTop = regs->ipcm3AreaLeft = regs->ipcm3AreaBottom = regs->ipcm3AreaRight =
        INVALID_POS;
    regs->ipcm4AreaTop = regs->ipcm4AreaLeft = regs->ipcm4AreaBottom = regs->ipcm4AreaRight =
        INVALID_POS;
    regs->ipcm5AreaTop = regs->ipcm5AreaLeft = regs->ipcm5AreaBottom = regs->ipcm5AreaRight =
        INVALID_POS;
    regs->ipcm6AreaTop = regs->ipcm6AreaLeft = regs->ipcm6AreaBottom = regs->ipcm6AreaRight =
        INVALID_POS;
    regs->ipcm7AreaTop = regs->ipcm7AreaLeft = regs->ipcm7AreaBottom = regs->ipcm7AreaRight =
        INVALID_POS;
    regs->ipcm8AreaTop = regs->ipcm8AreaLeft = regs->ipcm8AreaBottom = regs->ipcm8AreaRight =
        INVALID_POS;

    regs->roi1Top = regs->roi1Left = regs->roi1Bottom = regs->roi1Right = INVALID_POS;
    vcenc_inst->roi1Enable = 0;

    regs->roi2Top = regs->roi2Left = regs->roi2Bottom = regs->roi2Right = INVALID_POS;
    vcenc_inst->roi2Enable = 0;

    regs->roi3Top = regs->roi3Left = regs->roi3Bottom = regs->roi3Right = INVALID_POS;
    vcenc_inst->roi3Enable = 0;

    regs->roi4Top = regs->roi4Left = regs->roi4Bottom = regs->roi4Right = INVALID_POS;
    vcenc_inst->roi4Enable = 0;

    regs->roi5Top = regs->roi5Left = regs->roi5Bottom = regs->roi5Right = INVALID_POS;
    vcenc_inst->roi5Enable = 0;

    regs->roi6Top = regs->roi6Left = regs->roi6Bottom = regs->roi6Right = INVALID_POS;
    vcenc_inst->roi6Enable = 0;

    regs->roi7Top = regs->roi7Left = regs->roi7Bottom = regs->roi7Right = INVALID_POS;
    vcenc_inst->roi7Enable = 0;

    regs->roi8Top = regs->roi8Left = regs->roi8Bottom = regs->roi8Right = INVALID_POS;
    vcenc_inst->roi8Enable = 0;

    regs->roi1DeltaQp = 0;
    regs->roi2DeltaQp = 0;
    regs->roi3DeltaQp = 0;
    regs->roi4DeltaQp = 0;
    regs->roi5DeltaQp = 0;
    regs->roi6DeltaQp = 0;
    regs->roi7DeltaQp = 0;
    regs->roi8DeltaQp = 0;
    regs->roi1Qp = -1;
    regs->roi2Qp = -1;
    regs->roi3Qp = -1;
    regs->roi4Qp = -1;
    regs->roi5Qp = -1;
    regs->roi6Qp = -1;
    regs->roi7Qp = -1;
    regs->roi8Qp = -1;

    /* currently HW SSIM calculation is not supported for AV1 */
    regs->ssim = config->enableSsim;
    if (regs->ssim) {
        if (!regs->asicCfg.ssimSupport) {
            APITRACEERR("VCEncInit: WARNING SSIM calculation not supported by HW, force it "
                        "disable\n");
            regs->ssim = 0;
        } else if (IS_AV1(config->codecFormat)) {
            APITRACEERR("VCEncInit: WARNING SSIM calculation not supported for AV1, force it "
                        "disable\n");
            regs->ssim = 0;
        }
    }

    /*configure enablePsnr*/
    regs->psnr = config->enablePsnr;
    if (regs->psnr) {
        if (!regs->asicCfg.psnrSupport) {
            APITRACEERR("VCEncInit: WARNING PSNR calculation not supported by HW, force it "
                        "disable\n");
            regs->psnr = 0;
        } else if (IS_AV1(config->codecFormat)) {
            APITRACEERR("VCEncInit: WARNING PSNR calculation not supported for AV1, force it "
                        "disable\n");
            regs->psnr = 0;
        }
    }

    regs->totalCmdbufSize = 0;

    /* External line buffer settings */
    regs->sram_linecnt_lum_fwd = config->extSramLumHeightFwd;
    regs->sram_linecnt_lum_bwd = config->extSramLumHeightBwd;
    regs->sram_linecnt_chr_fwd = config->extSramChrHeightFwd;
    regs->sram_linecnt_chr_bwd = config->extSramChrHeightBwd;

    /*AXI max burst length */
    if (0 == config->burstMaxLength)
        regs->AXI_burst_max_length = ENCH2_DEFAULT_BURST_LENGTH; //default
    else
        regs->AXI_burst_max_length = config->burstMaxLength;

    /* AXI alignment */
    regs->AXI_burst_align_wr_cuinfo = (config->AXIAlignment >> 32) & 0xf;
    regs->AXI_burst_align_wr_common = (config->AXIAlignment >> 28) & 0xf;
    regs->AXI_burst_align_wr_stream = (config->AXIAlignment >> 24) & 0xf;
    regs->AXI_burst_align_wr_chroma_ref = (config->AXIAlignment >> 20) & 0xf;
    regs->AXI_burst_align_wr_luma_ref = (config->AXIAlignment >> 16) & 0xf;
    regs->AXI_burst_align_rd_common = (config->AXIAlignment >> 12) & 0xf;
    regs->AXI_burst_align_rd_prp = (config->AXIAlignment >> 8) & 0xf;
    regs->AXI_burst_align_rd_ch_ref_prefetch = (config->AXIAlignment >> 4) & 0xf;
    regs->AXI_burst_align_rd_lu_ref_prefetch = (config->AXIAlignment) & 0xf;

    /* Irq type mask */
    regs->irq_type_sw_reset_mask = (config->irqTypeMask >> 8) & 0x01;
    regs->irq_type_fuse_error_mask = (config->irqTypeMask >> 7) & 0x01;
    regs->irq_type_buffer_full_mask = (config->irqTypeMask >> 6) & 0x01;
    regs->irq_type_bus_error_mask = (config->irqTypeMask >> 5) & 0x01;
    regs->irq_type_timeout_mask = (config->irqTypeMask >> 4) & 0x01;
    regs->irq_type_strm_segment_mask = (config->irqTypeMask >> 3) & 0x01;
    regs->irq_type_line_buffer_mask = (config->irqTypeMask >> 2) & 0x01;
    regs->irq_type_slice_rdy_mask = (config->irqTypeMask >> 1) & 0x01;
    regs->irq_type_frame_rdy_mask = config->irqTypeMask & 0x01;

    /* Check AXI alignment */
    if (config->AXIAlignment != 0) {
        u32 axi_align_fuse =
            asic_cfg.axi_burst_align_wr_common + asic_cfg.axi_burst_align_wr_stream +
            asic_cfg.axi_burst_align_wr_chroma_ref + asic_cfg.axi_burst_align_wr_luma_ref +
            asic_cfg.axi_burst_align_rd_common + asic_cfg.axi_burst_align_rd_prp +
            asic_cfg.axi_burst_align_rd_ch_ref_prefetch +
            asic_cfg.axi_burst_align_rd_lu_ref_prefetch + asic_cfg.axi_burst_align_wr_cuinfo;

        if (axi_align_fuse == 0) {
            APITRACEERR("VCEncInit: Error, configured AXI is not supported by hardware\n");
            return VCENC_INVALID_ARGUMENT;
        } else {
            if ((asic_cfg.axi_burst_align_wr_common < regs->AXI_burst_align_wr_common) ||
                (asic_cfg.axi_burst_align_wr_stream < regs->AXI_burst_align_wr_stream) ||
                (asic_cfg.axi_burst_align_wr_chroma_ref < regs->AXI_burst_align_wr_chroma_ref) ||
                (asic_cfg.axi_burst_align_wr_luma_ref < regs->AXI_burst_align_wr_luma_ref) ||
                (asic_cfg.axi_burst_align_rd_common < regs->AXI_burst_align_rd_common) ||
                (asic_cfg.axi_burst_align_rd_prp < regs->AXI_burst_align_rd_prp) ||
                (asic_cfg.axi_burst_align_rd_ch_ref_prefetch <
                 regs->AXI_burst_align_rd_ch_ref_prefetch) ||
                (asic_cfg.axi_burst_align_rd_lu_ref_prefetch <
                 regs->AXI_burst_align_rd_lu_ref_prefetch) ||
                (asic_cfg.axi_burst_align_wr_cuinfo < regs->AXI_burst_align_wr_cuinfo)) {
                APITRACEERR("VCEncInit: Error, configured AXI is not matched with hardware\n");
                return VCENC_INVALID_ARGUMENT;
            }
        }
    }

    /* Check HEVC Tile */
    if (asic_cfg.hevcTileSupport == 0 && vcenc_inst->tiles_enabled_flag == 1) {
        APITRACEERR("VCEncInit: Error, HEVC TILE is not supported by hardware\n");
        return VCENC_INVALID_ARGUMENT;
    }

    if (vcenc_inst->tiles_enabled_flag == 1 && !IS_HEVC(config->codecFormat)) {
        APITRACEERR("VCEncInit: Tile is not supported by H264/av1/vp9 \n");
        return VCENC_INVALID_ARGUMENT;
    }

    /* Check multi-core */
    if (IS_VP9(config->codecFormat) && config->parallelCoreNum > 1) {
        APITRACEERR("VCEncInit: Multi-core is not supported by vp9 \n");
        return VCENC_INVALID_ARGUMENT;
    }

    vcenc_inst->pcm_enabled_flag = 0;
    vcenc_inst->pcm_loop_filter_disabled_flag = 0;

    vcenc_inst->roiMapEnable = 0;
    vcenc_inst->RoimapCuCtrl_index_enable = 0;
    vcenc_inst->RoimapCuCtrl_enable = 0;
    vcenc_inst->RoimapCuCtrl_ver = 0;
    vcenc_inst->RoiQpDelta_ver = 0;
    vcenc_inst->ctbRCEnable = 0;
    regs->rcRoiEnable = 0x00;
    regs->diffCuQpDeltaDepth = 0;
    /* Status == INIT   Initialization successful */
    vcenc_inst->encStatus = VCENCSTAT_INIT;
    vcenc_inst->created_pic_num = 0;

    pPP = &(vcenc_inst->preProcess);
    pPP->constChromaEn = 0;
    pPP->constCb = pPP->constCr = 0x80 << (config->bitDepthChroma - 8);

    /* t35 */
    EWLmemset(&vcenc_inst->itu_t_t35, 0, sizeof(ITU_T_T35));

    /* HDR10 */
    EWLmemset(&vcenc_inst->Hdr10Display, 0, sizeof(Hdr10DisplaySei));
    vcenc_inst->write_once_HDR10 = 0;
    vcenc_inst->Hdr10Display.hdr10_display_enable = (u8)HDR10_NOCFGED;

    vcenc_inst->vuiColorDescription.vuiColorDescripPresentFlag = 0;
    vcenc_inst->vuiColorDescription.vuiColorPrimaries = 9;
    vcenc_inst->vuiColorDescription.vuiTransferCharacteristics = 0;
    vcenc_inst->vuiColorDescription.vuiMatrixCoefficients = 9;

    vcenc_inst->sarHeight = 0;
    vcenc_inst->sarWidth = 0;

    vcenc_inst->vuiVideoFormat = 5;
    vcenc_inst->vuiVideoSignalTypePresentFlag = 0;

    vcenc_inst->Hdr10LightLevel.hdr10_lightlevel_enable = (u8)HDR10_NOCFGED;
    vcenc_inst->Hdr10LightLevel.hdr10_maxlight = 0;
    vcenc_inst->Hdr10LightLevel.hdr10_avglight = 0;

    vcenc_inst->RpsInSliceHeader = 0;

    /* Multi-core */
    vcenc_inst->parallelCoreNum = parallelCoreNum;
    vcenc_inst->reservedCore = 0;

    /* RDOQ. enable RDOQ by default if HW supported */
    if ((vcenc_inst->pass != 1) && (config->tune != VCENC_TUNE_SHARP_VISUAL)) {
        regs->bRDOQEnable = (IS_H264(config->codecFormat) && regs->asicCfg.RDOQSupportH264) ||
                            (IS_AV1(config->codecFormat) && regs->asicCfg.RDOQSupportAV1) ||
                            (IS_HEVC(config->codecFormat) && regs->asicCfg.RDOQSupportHEVC) ||
                            (IS_VP9(config->codecFormat) && regs->asicCfg.RDOQSupportVP9);
    }

    /* Multi-pass */
    vcenc_inst->lookaheadDepth = lookaheadDepth;
    vcenc_inst->idelay = 0;
    vcenc_inst->bSkipCabacEnable = 0;
    vcenc_inst->bMotionScoreEnable = 0;
    vcenc_inst->extDSRatio = config->extDSRatio;
    vcenc_inst->ctx = ctx;
    vcenc_inst->slice_idx = config->slice_idx;

    vcenc_inst->bInputfileList = config->fileListExist;

    vcenc_inst->gopSize = config->gopSize;
    //if io buffer binding
    vcenc_inst->bIOBufferBinding = (1 == config->bIOBufferBinding) ? 1 : 0;

    if (vcenc_inst->pass == 1) {
        /* init pass1 -> cutree job' bufferpool */
        u32 nodeNum = VCEncGetEncodeMaxDelay(vcenc_inst) + 1;
        ret = InitBufferPool(&vcenc_inst->cutreeJobBufferPool, sizeof(struct Lowres), nodeNum);
        if (VCENC_OK != ret) {
            APITRACEERR("VCEncInit: Init cutreeJobBufferPool failed\n");
            goto error;
        }

        ret = cuTreeInit(&vcenc_inst->cuTreeCtl, vcenc_inst, config);
        if (ret != VCENC_OK) {
            APITRACEERR("VCEncInit: cuTreeInit failed\n");
            goto error;
        }

        /* skip CABAC engine for better througput in first pass */
        vcenc_inst->bSkipCabacEnable = regs->asicCfg.bMultiPassSupport;
        /* enable motion score computing for agop decision in first pass */
        vcenc_inst->bMotionScoreEnable = regs->asicCfg.bMultiPassSupport;
    } else if (vcenc_inst->pass == 2) {
        VCEncConfig new_cfg;
        u32 inLoopDsRatio, dsRation;

        // init pass2 -> pass1 job' bufferpool
        u32 nodeNum = VCEncGetEncodeMaxDelay(vcenc_inst) + 1;
        u32 nodeSize = sizeof(VCEncLookaheadJob);
        if (vcenc_inst->num_tile_columns > 1)
            nodeSize += (vcenc_inst->num_tile_columns - 1) * sizeof(VCEncInTileExtra);
        ret = InitBufferPool(&vcenc_inst->lookahead.jobBufferPool, nodeSize, nodeNum);
        if (ret != VCENC_OK) {
            APITRACEERR("VCEncInit: Init pass1JobBufferPool failed\n");
            goto error;
        }

        memcpy(&new_cfg, config, sizeof(VCEncConfig));
        /*config pass 1 encoder cfg*/
        new_cfg.pass = 1;
        new_cfg.lookaheadDepth = lookaheadDepth;
        new_cfg.enableOutputCuInfo = 1;

        /*pass-1 cutree downscaler config */
        inLoopDsRatio = config->inLoopDSRatio;
        if (vcenc_inst->extDSRatio) {
            inLoopDsRatio = 0;
        } else if (inLoopDsRatio) {
            if (config->tune == VCENC_TUNE_SHARP_VISUAL) {
                inLoopDsRatio = 0;
                APITRACE("VCEncInit: Disable input downsampe in look-ahead encoding for "
                         "tune=sharpness_visual\n");
            }

            /* Down-sampled input shouldn't be smaller than the size supported by HW */
            if (config->width < 272 || config->height < 256) {
                inLoopDsRatio = 0;
                APITRACE("VCEncInit: Disable input downsampe in look-ahead encoding for "
                         "small video size\n");
            }

            if (regs->asicCfg.inLoopDSRatio == 0)
                inLoopDsRatio = 0;
        }

        // if extDSRatio, inLoopDsRation should be 0.
        dsRation = vcenc_inst->extDSRatio ? 2 : (inLoopDsRatio + 1);
        new_cfg.inLoopDSRatio = inLoopDsRatio;
        new_cfg.width /= dsRation;
        new_cfg.height /= dsRation;
        new_cfg.width = (new_cfg.width >> 1) << 1;
        new_cfg.height = (new_cfg.height >> 1) << 1;

        new_cfg.exteralReconAlloc = 0; //this should alloc internarlly when in the first pass.

        ret = VCEncInit(&new_cfg, &vcenc_inst->lookahead.priv_inst, ctx);
        if (ret != VCENC_OK) {
            vcenc_inst->lookahead.priv_inst = NULL;
            APITRACEERR("VCEncInit: LookaheadInit failed\n");
            goto error;
        }
        /* init qpDeltaBlockSize for 2nd pass */
        u32 cuTreeOutQpBlockUnit = ((struct vcenc_instance *)vcenc_inst->lookahead.priv_inst)
                                       ->cuTreeCtl.outRoiMapDeltaQpBlockUnit;
        vcenc_inst->log2_qp_size = MIN(vcenc_inst->log2_qp_size, 6 - cuTreeOutQpBlockUnit);

        /* for pass1 cutree put buffer back to bufferpool */
        ((struct vcenc_instance *)vcenc_inst->lookahead.priv_inst)->lookahead.jobBufferPool =
            vcenc_inst->lookahead.jobBufferPool;
    } else if (vcenc_inst->pass == 0) {
        // signal pass
        u32 nodeNum = MAX_GOP_SIZE;
        u32 nodeSize = sizeof(VCEncJob);
        if (vcenc_inst->num_tile_columns > 1)
            nodeSize += (vcenc_inst->num_tile_columns - 1) * sizeof(VCEncInTileExtra);
        ret = InitBufferPool(&vcenc_inst->jobBufferPool, nodeSize, nodeNum);
        if (ret != VCENC_OK) {
            APITRACEERR("VCEncInit: Init single pass JobBufferPool failed\n");
            goto error;
        }
    }

    /* AV1 or VP9 specific params init */
#ifdef SUPPORT_AV1
    if (IS_AV1(config->codecFormat))
        VCEncInitAV1(config, vcenc_inst);
#endif

#ifdef SUPPORT_VP9
    if (IS_VP9(config->codecFormat))
        VCEncInitVP9(config, vcenc_inst);
#endif

    vcenc_inst->asic.regs.bInitUpdate = 1;
    vcenc_inst->inst = vcenc_inst;

    /*init coding ctrl buffer pool & queue */
    queue_init(&vcenc_inst->codingCtrl.codingCtrlBufPool);
    queue_init(&vcenc_inst->codingCtrl.codingCtrlQueue);
    vcenc_inst->codingCtrl.pCodingCtrlParam = NULL;

    APITRACE("VCEncInit: OK\n");
    return VCENC_OK;

error:

    APITRACEERR("VCEncInit: ERROR Initialization failed\n");

    //release all resource(instance/ewl/memory/ewl_mem) if vcenc_inst created but error happen
    if (vcenc_inst != NULL) {
        vcenc_inst->inst = vcenc_inst;
        VCEncRelease(vcenc_inst);
        vcenc_inst = NULL;
        return ret;
    }
    //release ewl resource if vcenc_inst not created yet
    if (instance != NULL)
        EWLfree(instance);
    if (ewl != NULL)
        (void)EWLRelease(ewl);

    return ret;
}

/*------------------------------------------------------------------------------

    Function name : VCEncRelease
    Description   : Releases encoder instance and all associated resource

    Return type   : VCEncRet
    Argument      : inst - the instance to be released
------------------------------------------------------------------------------*/
VCEncRet VCEncRelease(VCEncInst inst)
{
    struct vcenc_instance *pEncInst = (struct vcenc_instance *)inst;
    struct container *c;
    VCEncRet ret = VCENC_OK;
    u32 coreId;

    APITRACE("VCEncRelease#\n");

    /* Check for illegal inputs */
    if (pEncInst == NULL) {
        APITRACEERR("VCEncRelease: ERROR Null argument\n");
        return VCENC_NULL_ARGUMENT;
    }

    /* Check for existing instance */
    if (pEncInst->inst != inst) {
        APITRACEERR("VCEncRelease: ERROR Invalid instance\n");
        return VCENC_INSTANCE_ERROR;
    }

    /* stop encoder first, then release encoder*/
    VCEncStop(inst);

    if (pEncInst->pass == 0) {
        ReleaseBufferPool(&pEncInst->jobBufferPool);
        //release coding ctrl buffer
        DynamicReleasePool(&pEncInst->codingCtrl.codingCtrlBufPool,
                           &pEncInst->codingCtrl.codingCtrlQueue);
    }

    if (pEncInst->pass == 2 && pEncInst->lookahead.priv_inst) {
        struct vcenc_instance *pEncInst_priv =
            (struct vcenc_instance *)pEncInst->lookahead.priv_inst;

        /* Terminate lookahead thread first, put correct status */
        ret =
            TerminateLookaheadThread(&pEncInst->lookahead, pEncInst->encStatus == VCENCSTAT_ERROR);

        ret = TerminateCuTreeThread(&pEncInst_priv->cuTreeCtl,
                                    pEncInst->encStatus == VCENCSTAT_ERROR);

        DestroyThread(&pEncInst->lookahead, &(pEncInst_priv->cuTreeCtl));

        /* Release pass 1 instance */
        if ((c = get_container(pEncInst_priv))) {
            sw_free_pictures(c); /* Call this before free_parameter_sets() */
            free_parameter_sets(c);

            /* release pass1 internal buffer only if lookahead thread is killed. Maybe it's runing after strm_end*/
            EWLFreeLinear(pEncInst_priv->asic.ewl, &(pEncInst_priv->lookahead.internal_mem.mem));
            free_parameter_set_queue(c);

            VCEncShutdown((struct instance *)pEncInst_priv);
        } else
            ret = VCENC_ERROR;

        pEncInst_priv = NULL;

        DynamicReleasePool(&pEncInst->codingCtrl.codingCtrlBufPool,
                           &pEncInst->codingCtrl.codingCtrlQueue);
    }

    for (coreId = 0; coreId < MAX_CORE_NUM; coreId++) {
        if (pEncInst->num_tile_columns > 1) {
            free(pEncInst->EncIn[coreId].tileExtra);
            free(pEncInst->EncOut[coreId].tileExtra);
            pEncInst->EncOut[coreId].tileExtra = NULL;
        }
    }

    if ((c = get_container(pEncInst))) {
        sw_free_pictures(c); /* Call this before free_parameter_sets() */
        free_parameter_sets(c);

        free_parameter_set_queue(c);

        VCEncShutdown((struct instance *)pEncInst);
    } else
        ret = VCENC_ERROR;

    EwlReleaseCoreWait(NULL);
    pEncInst = NULL;

    if (ret >= VCENC_OK) {
        APITRACE("VCEncRelease: OK\n");
    } else {
        APITRACE("VCEncRelease: NOK\n");
    }
    VCEncLogDestroy();

    return ret;
}

/*------------------------------------------------------------------------------

    VCEncGetPerformance

    Function frees the encoder instance.

    Input   inst    Pointer to the encoder instance to be freed.
                            After this the pointer is no longer valid.

------------------------------------------------------------------------------*/
u32 VCEncGetPerformance(VCEncInst inst)
{
    struct vcenc_instance *pEncInst = (struct vcenc_instance *)inst;
    const void *ewl;
    u32 performanceData;
    ASSERT(inst);
    APITRACE("VCEncGetPerformance#\n");
    /* Check for illegal inputs */
    if (pEncInst == NULL) {
        APITRACEERR("VCEncGetPerformance: ERROR Null argument\n");
        return VCENC_NULL_ARGUMENT;
    }

    /* Check for existing instance */
    if (pEncInst->inst != inst) {
        APITRACEERR("VCEncGetPerformance: ERROR Invalid instance\n");
        return VCENC_INSTANCE_ERROR;
    }

    ewl = pEncInst->asic.ewl;
    performanceData = EncAsicGetPerformance(ewl);
    APITRACE("VCEncGetPerformance:OK\n");
    return performanceData;
}

/*------------------------------------------------------------------------------
  get_container
------------------------------------------------------------------------------*/
struct container *get_container(struct vcenc_instance *vcenc_instance)
{
    struct instance *instance = (struct instance *)vcenc_instance;

    if (!instance || (instance->instance != vcenc_instance))
        return NULL;

    return &instance->container;
}

/*------------------------------------------------------------------------------
  VCEncSetCodingCtrl
------------------------------------------------------------------------------*/
VCEncRet VCEncSetCodingCtrl(VCEncInst instAddr, const VCEncCodingCtrl *pCodeParams)
{
    struct vcenc_instance *pEncInst = (struct vcenc_instance *)instAddr;
    regValues_s *regs = &pEncInst->asic.regs;
    u32 i;
    u32 client_type;
    EWLHwConfig_t cfg;
    APITRACE("VCEncSetCodingCtrl#\n");
    APITRACEPARAM(" %s : %d\n", "sliceSize", pCodeParams->sliceSize);
    APITRACEPARAM(" %s : %d\n", "seiMessages", pCodeParams->seiMessages);
    APITRACEPARAM(" %s : %d\n", "vuiVideoFullRange", pCodeParams->vuiVideoFullRange);
    APITRACEPARAM(" %s : %d\n", "disableDeblockingFilter", pCodeParams->disableDeblockingFilter);
    APITRACEPARAM(" %s : %d\n", "tc_Offset", pCodeParams->tc_Offset);
    APITRACEPARAM(" %s : %d\n", "beta_Offset", pCodeParams->beta_Offset);
    APITRACEPARAM(" %s : %d\n", "enableDeblockOverride", pCodeParams->enableDeblockOverride);
    APITRACEPARAM(" %s : %d\n", "deblockOverride", pCodeParams->deblockOverride);
    APITRACEPARAM(" %s : %d\n", "enableSao", pCodeParams->enableSao);
    APITRACEPARAM(" %s : %d\n", "enableScalingList", pCodeParams->enableScalingList);
    APITRACEPARAM(" %s : %d\n", "sampleAspectRatioWidth", pCodeParams->sampleAspectRatioWidth);
    APITRACEPARAM(" %s : %d\n", "sampleAspectRatioHeight", pCodeParams->sampleAspectRatioHeight);
    APITRACEPARAM(" %s : %d\n", "enableCabac", pCodeParams->enableCabac);
    APITRACEPARAM(" %s : %d\n", "cabacInitFlag", pCodeParams->cabacInitFlag);
    APITRACEPARAM(" %s : %d\n", "cirStart", pCodeParams->cirStart);
    APITRACEPARAM(" %s : %d\n", "cirInterval", pCodeParams->cirInterval);
    APITRACEPARAM(" %s : %d\n", "intraArea.enable", pCodeParams->intraArea.enable);
    APITRACEPARAM(" %s : %d\n", "intraArea.top", pCodeParams->intraArea.top);
    APITRACEPARAM(" %s : %d\n", "intraArea.bottom", pCodeParams->intraArea.bottom);
    APITRACEPARAM(" %s : %d\n", "intraArea.left", pCodeParams->intraArea.left);
    APITRACEPARAM(" %s : %d\n", "intraArea.right", pCodeParams->intraArea.right);
    APITRACEPARAM(" %s : %d\n", "ipcm1Area.enable", pCodeParams->ipcm1Area.enable);
    APITRACEPARAM(" %s : %d\n", "ipcm1Area.top", pCodeParams->ipcm1Area.top);
    APITRACEPARAM(" %s : %d\n", "ipcm1Area.bottom", pCodeParams->ipcm1Area.bottom);
    APITRACEPARAM(" %s : %d\n", "ipcm1Area.left", pCodeParams->ipcm1Area.left);
    APITRACEPARAM(" %s : %d\n", "ipcm1Area.right", pCodeParams->ipcm1Area.right);
    APITRACEPARAM(" %s : %d\n", "ipcm2Area.enable", pCodeParams->ipcm2Area.enable);
    APITRACEPARAM(" %s : %d\n", "ipcm2Area.top", pCodeParams->ipcm2Area.top);
    APITRACEPARAM(" %s : %d\n", "ipcm2Area.bottom", pCodeParams->ipcm2Area.bottom);
    APITRACEPARAM(" %s : %d\n", "ipcm2Area.left", pCodeParams->ipcm2Area.left);
    APITRACEPARAM(" %s : %d\n", "ipcm2Area.right", pCodeParams->ipcm2Area.right);
    APITRACEPARAM(" %s : %d\n", "ipcm3Area.enable", pCodeParams->ipcm3Area.enable);
    APITRACEPARAM(" %s : %d\n", "ipcm3Area.top", pCodeParams->ipcm3Area.top);
    APITRACEPARAM(" %s : %d\n", "ipcm3Area.bottom", pCodeParams->ipcm3Area.bottom);
    APITRACEPARAM(" %s : %d\n", "ipcm3Area.left", pCodeParams->ipcm3Area.left);
    APITRACEPARAM(" %s : %d\n", "ipcm3Area.right", pCodeParams->ipcm3Area.right);
    APITRACEPARAM(" %s : %d\n", "ipcm4Area.enable", pCodeParams->ipcm4Area.enable);
    APITRACEPARAM(" %s : %d\n", "ipcm4Area.top", pCodeParams->ipcm4Area.top);
    APITRACEPARAM(" %s : %d\n", "ipcm4Area.bottom", pCodeParams->ipcm4Area.bottom);
    APITRACEPARAM(" %s : %d\n", "ipcm4Area.left", pCodeParams->ipcm4Area.left);
    APITRACEPARAM(" %s : %d\n", "ipcm4Area.right", pCodeParams->ipcm4Area.right);
    APITRACEPARAM(" %s : %d\n", "ipcm5Area.enable", pCodeParams->ipcm5Area.enable);
    APITRACEPARAM(" %s : %d\n", "ipcm5Area.top", pCodeParams->ipcm5Area.top);
    APITRACEPARAM(" %s : %d\n", "ipcm5Area.bottom", pCodeParams->ipcm5Area.bottom);
    APITRACEPARAM(" %s : %d\n", "ipcm5Area.left", pCodeParams->ipcm5Area.left);
    APITRACEPARAM(" %s : %d\n", "ipcm5Area.right", pCodeParams->ipcm5Area.right);
    APITRACEPARAM(" %s : %d\n", "ipcm6Area.enable", pCodeParams->ipcm6Area.enable);
    APITRACEPARAM(" %s : %d\n", "ipcm6Area.top", pCodeParams->ipcm6Area.top);
    APITRACEPARAM(" %s : %d\n", "ipcm6Area.bottom", pCodeParams->ipcm6Area.bottom);
    APITRACEPARAM(" %s : %d\n", "ipcm6Area.left", pCodeParams->ipcm6Area.left);
    APITRACEPARAM(" %s : %d\n", "ipcm6Area.right", pCodeParams->ipcm6Area.right);
    APITRACEPARAM(" %s : %d\n", "ipcm7Area.enable", pCodeParams->ipcm7Area.enable);
    APITRACEPARAM(" %s : %d\n", "ipcm7Area.top", pCodeParams->ipcm7Area.top);
    APITRACEPARAM(" %s : %d\n", "ipcm7Area.bottom", pCodeParams->ipcm7Area.bottom);
    APITRACEPARAM(" %s : %d\n", "ipcm7Area.left", pCodeParams->ipcm7Area.left);
    APITRACEPARAM(" %s : %d\n", "ipcm7Area.right", pCodeParams->ipcm7Area.right);
    APITRACEPARAM(" %s : %d\n", "ipcm8Area.enable", pCodeParams->ipcm8Area.enable);
    APITRACEPARAM(" %s : %d\n", "ipcm8Area.top", pCodeParams->ipcm8Area.top);
    APITRACEPARAM(" %s : %d\n", "ipcm8Area.bottom", pCodeParams->ipcm8Area.bottom);
    APITRACEPARAM(" %s : %d\n", "ipcm8Area.left", pCodeParams->ipcm8Area.left);
    APITRACEPARAM(" %s : %d\n", "ipcm8Area.right", pCodeParams->ipcm8Area.right);
    APITRACEPARAM(" %s : %d\n", "roi1Area.enable", pCodeParams->roi1Area.enable);
    APITRACEPARAM(" %s : %d\n", "roi1Area.top", pCodeParams->roi1Area.top);
    APITRACEPARAM(" %s : %d\n", "roi1Area.bottom", pCodeParams->roi1Area.bottom);
    APITRACEPARAM(" %s : %d\n", "roi1Area.left", pCodeParams->roi1Area.left);
    APITRACEPARAM(" %s : %d\n", "roi1Area.right", pCodeParams->roi1Area.right);
    APITRACEPARAM(" %s : %d\n", "roi2Area.enable", pCodeParams->roi2Area.enable);
    APITRACEPARAM(" %s : %d\n", "roi2Area.top", pCodeParams->roi2Area.top);
    APITRACEPARAM(" %s : %d\n", "roi2Area.bottom", pCodeParams->roi2Area.bottom);
    APITRACEPARAM(" %s : %d\n", "roi2Area.left", pCodeParams->roi2Area.left);
    APITRACEPARAM(" %s : %d\n", "roi2Area.right", pCodeParams->roi2Area.right);
    APITRACEPARAM(" %s : %d\n", "roi3Area.enable", pCodeParams->roi3Area.enable);
    APITRACEPARAM(" %s : %d\n", "roi3Area.top", pCodeParams->roi3Area.top);
    APITRACEPARAM(" %s : %d\n", "roi3Area.bottom", pCodeParams->roi3Area.bottom);
    APITRACEPARAM(" %s : %d\n", "roi3Area.left", pCodeParams->roi3Area.left);
    APITRACEPARAM(" %s : %d\n", "roi3Area.right", pCodeParams->roi3Area.right);
    APITRACEPARAM(" %s : %d\n", "roi4Area.enable", pCodeParams->roi4Area.enable);
    APITRACEPARAM(" %s : %d\n", "roi4Area.top", pCodeParams->roi4Area.top);
    APITRACEPARAM(" %s : %d\n", "roi4Area.bottom", pCodeParams->roi4Area.bottom);
    APITRACEPARAM(" %s : %d\n", "roi4Area.left", pCodeParams->roi4Area.left);
    APITRACEPARAM(" %s : %d\n", "roi4Area.right", pCodeParams->roi4Area.right);
    APITRACEPARAM(" %s : %d\n", "roi5Area.enable", pCodeParams->roi5Area.enable);
    APITRACEPARAM(" %s : %d\n", "roi5Area.top", pCodeParams->roi5Area.top);
    APITRACEPARAM(" %s : %d\n", "roi5Area.bottom", pCodeParams->roi5Area.bottom);
    APITRACEPARAM(" %s : %d\n", "roi5Area.left", pCodeParams->roi5Area.left);
    APITRACEPARAM(" %s : %d\n", "roi5Area.right", pCodeParams->roi5Area.right);
    APITRACEPARAM(" %s : %d\n", "roi6Area.enable", pCodeParams->roi6Area.enable);
    APITRACEPARAM(" %s : %d\n", "roi6Area.top", pCodeParams->roi6Area.top);
    APITRACEPARAM(" %s : %d\n", "roi6Area.bottom", pCodeParams->roi6Area.bottom);
    APITRACEPARAM(" %s : %d\n", "roi6Area.left", pCodeParams->roi6Area.left);
    APITRACEPARAM(" %s : %d\n", "roi6Area.right", pCodeParams->roi6Area.right);
    APITRACEPARAM(" %s : %d\n", "roi7Area.enable", pCodeParams->roi7Area.enable);
    APITRACEPARAM(" %s : %d\n", "roi7Area.top", pCodeParams->roi7Area.top);
    APITRACEPARAM(" %s : %d\n", "roi7Area.bottom", pCodeParams->roi7Area.bottom);
    APITRACEPARAM(" %s : %d\n", "roi7Area.left", pCodeParams->roi7Area.left);
    APITRACEPARAM(" %s : %d\n", "roi7Area.right", pCodeParams->roi7Area.right);
    APITRACEPARAM(" %s : %d\n", "roi8Area.enable", pCodeParams->roi8Area.enable);
    APITRACEPARAM(" %s : %d\n", "roi8Area.top", pCodeParams->roi8Area.top);
    APITRACEPARAM(" %s : %d\n", "roi8Area.bottom", pCodeParams->roi8Area.bottom);
    APITRACEPARAM(" %s : %d\n", "roi8Area.left", pCodeParams->roi8Area.left);
    APITRACEPARAM(" %s : %d\n", "roi8Area.right", pCodeParams->roi8Area.right);
    APITRACEPARAM(" %s : %d\n", "roi1DeltaQp", pCodeParams->roi1DeltaQp);
    APITRACEPARAM(" %s : %d\n", "roi2DeltaQp", pCodeParams->roi2DeltaQp);
    APITRACEPARAM(" %s : %d\n", "roi3DeltaQp", pCodeParams->roi3DeltaQp);
    APITRACEPARAM(" %s : %d\n", "roi4DeltaQp", pCodeParams->roi4DeltaQp);
    APITRACEPARAM(" %s : %d\n", "roi5DeltaQp", pCodeParams->roi5DeltaQp);
    APITRACEPARAM(" %s : %d\n", "roi6DeltaQp", pCodeParams->roi6DeltaQp);
    APITRACEPARAM(" %s : %d\n", "roi7DeltaQp", pCodeParams->roi7DeltaQp);
    APITRACEPARAM(" %s : %d\n", "roi8DeltaQp", pCodeParams->roi8DeltaQp);
    APITRACEPARAM(" %s : %d\n", "roi1Qp", pCodeParams->roi1Qp);
    APITRACEPARAM(" %s : %d\n", "roi2Qp", pCodeParams->roi2Qp);
    APITRACEPARAM(" %s : %d\n", "roi3Qp", pCodeParams->roi3Qp);
    APITRACEPARAM(" %s : %d\n", "roi4Qp", pCodeParams->roi4Qp);
    APITRACEPARAM(" %s : %d\n", "roi5Qp", pCodeParams->roi5Qp);
    APITRACEPARAM(" %s : %d\n", "roi6Qp", pCodeParams->roi6Qp);
    APITRACEPARAM(" %s : %d\n", "roi7Qp", pCodeParams->roi7Qp);
    APITRACEPARAM(" %s : %d\n", "roi8Qp", pCodeParams->roi8Qp);
    APITRACEPARAM(" %s : %d\n", "chroma_qp_offset", pCodeParams->chroma_qp_offset);
    APITRACEPARAM(" %s : %d\n", "fieldOrder", pCodeParams->fieldOrder);
    APITRACEPARAM(" %s : %d\n", "gdrDuration", pCodeParams->gdrDuration);
    APITRACEPARAM(" %s : %d\n", "roiMapDeltaQpEnable", pCodeParams->roiMapDeltaQpEnable);
    APITRACEPARAM(" %s : %d\n", "roiMapDeltaQpBlockUnit", pCodeParams->roiMapDeltaQpBlockUnit);
    APITRACEPARAM(" %s : %d\n", "RoimapCuCtrl_index_enable",
                  pCodeParams->RoimapCuCtrl_index_enable);
    APITRACEPARAM(" %s : %d\n", "RoimapCuCtrl_enable", pCodeParams->RoimapCuCtrl_enable);
    APITRACEPARAM(" %s : %d\n", "RoimapCuCtrl_ver", pCodeParams->RoimapCuCtrl_ver);
    APITRACEPARAM(" %s : %d\n", "RoiQpDelta_ver", pCodeParams->RoiQpDelta_ver);
    APITRACEPARAM(" %s : %d\n", "pcm_enabled_flag", pCodeParams->pcm_enabled_flag);
    APITRACEPARAM(" %s : %d\n", "pcm_loop_filter_disabled_flag",
                  pCodeParams->pcm_loop_filter_disabled_flag);
    APITRACEPARAM(" %s : %d\n", "ipcmMapEnable", pCodeParams->ipcmMapEnable);
    APITRACEPARAM(" %s : %d\n", "skipMapEnable", pCodeParams->skipMapEnable);
    APITRACEPARAM(" %s : %d\n", "enableRdoQuant", pCodeParams->enableRdoQuant);
    APITRACEPARAM(" %s : %d\n", "rdoqMapEnable", pCodeParams->rdoqMapEnable);
    APITRACEPARAM(" %s : %d\n", "aq_mode", pCodeParams->aq_mode);
    APITRACEPARAM(" %s : %d\n", "aq_strength", pCodeParams->aq_strength);
    APITRACEPARAM(" %s : %d\n", "psyFactor", pCodeParams->psyFactor);

    /* Check for illegal inputs */
    if ((pEncInst == NULL) || (pCodeParams == NULL)) {
        APITRACEERR("VCEncSetCodingCtrl: ERROR Null argument\n");
        return VCENC_NULL_ARGUMENT;
    }

    /* Check for existing instance */
    if (pEncInst->inst != pEncInst) {
        APITRACEERR("VCEncSetCodingCtrl: ERROR Invalid instance\n");
        return VCENC_INSTANCE_ERROR;
    }

    EncCodingCtrlParam *pEncCodingCtrlParam = NULL;
    VCEncCodingCtrl *pCodingCtrl = NULL;
    VCEncRet ret = VCENC_ERROR;
    do {
        /*head of parameter queue's refCnt need be current realRefCnt + 1,
    to avoid being removed from queue before new frame is sent in.*/

        //before new parameter enqueue as new head of queue, adjust refCnt of queue's last head
        EncCodingCtrlParam *pLastCodingCtrlParam =
            (EncCodingCtrlParam *)queue_head(&pEncInst->codingCtrl.codingCtrlQueue);
        if (pLastCodingCtrlParam) {
            if (pLastCodingCtrlParam->refCnt > 0)
                pLastCodingCtrlParam->refCnt--;
            //remove useless parameters from queue
            if (0 == pLastCodingCtrlParam->refCnt) {
                queue_remove(&pEncInst->codingCtrl.codingCtrlQueue,
                             (struct node *)pLastCodingCtrlParam);
                DynamicPutBufferToPool(&pEncInst->codingCtrl.codingCtrlBufPool,
                                       (void *)pLastCodingCtrlParam);
            }
        }

        //get buffer
        pEncCodingCtrlParam = (EncCodingCtrlParam *)DynamicGetBufferFromPool(
            &pEncInst->codingCtrl.codingCtrlBufPool, sizeof(EncCodingCtrlParam));
        if (!pEncCodingCtrlParam) {
            APITRACEERR("VCEncSetCodingCtrl: ERROR Get coding ctrl buffer failed\n");
            break;
        }
        pCodingCtrl = &pEncCodingCtrlParam->encCodingCtrl;
        //copy parameters to buffer
        memcpy(pCodingCtrl, pCodeParams, sizeof(VCEncCodingCtrl));

        //check and modify parameters
        if (VCENC_OK != EncCheckCodingCtrlParam(pEncInst, pCodingCtrl))
            break;

        /*The newest parameter's refCnt need be realRefCnt+1,
      so that next frame can also use these parameters if no new parameters are set*/
        pEncCodingCtrlParam->refCnt = 1;
        pEncCodingCtrlParam->startPicCnt = -1;
        queue_put(&pEncInst->codingCtrl.codingCtrlQueue, (struct node *)pEncCodingCtrlParam);

        /*when init, set parameters into instance*/
        if (pEncInst->encStatus == VCENCSTAT_INIT) {
            EncUpdateCodingCtrlParam(pEncInst, pEncCodingCtrlParam, -1);
            if (2 == pEncInst->pass) //a few coding ctrl parameters need to be set in pass1/cutree
                EncUpdateCodingCtrlForPass1(pEncInst->lookahead.priv_inst, pEncCodingCtrlParam);
        }

        ret = VCENC_OK;
    } while (0);

    if (VCENC_OK != ret && pEncCodingCtrlParam) {
        DynamicPutBufferToPool(&pEncInst->codingCtrl.codingCtrlBufPool,
                               (void *)pEncCodingCtrlParam);
        return ret;
    }

    APITRACE("VCEncSetCodingCtrl: OK\n");
    return ret;
}

/*-----------------------------------------------------------------------------
check reference lists modification
-------------------------------------------------------------------------------*/
i32 check_ref_lists_modification(const VCEncIn *pEncIn)
{
    int i;
    bool lowdelayB = HANTRO_FALSE;
    VCEncGopConfig *gopCfg = (VCEncGopConfig *)(&(pEncIn->gopConfig));

    for (i = 0; i < gopCfg->size; i++) {
        VCEncGopPicConfig *cfg = &(gopCfg->pGopPicCfg[i]);
        if (cfg->codingType == VCENC_BIDIR_PREDICTED_FRAME) {
            u32 iRefPic;
            lowdelayB = HANTRO_TRUE;
            for (iRefPic = 0; iRefPic < cfg->numRefPics; iRefPic++) {
                if (cfg->refPics[iRefPic].used_by_cur && cfg->refPics[iRefPic].ref_pic > 0)
                    lowdelayB = HANTRO_FALSE;
            }
            if (lowdelayB)
                break;
        }
    }
    return (lowdelayB || pEncIn->bIsPeriodUpdateLTR || pEncIn->flexRefsEnable) ? 1 : 0;
}

/*------------------------------------------------------------------------------
  set_parameter
------------------------------------------------------------------------------*/
i32 set_parameter(struct vcenc_instance *vcenc_instance, const VCEncIn *pEncIn, struct vps *v,
                  struct sps *s, struct pps *p)
{
    struct container *c;
    struct vcenc_buffer source;
    i32 tmp;
    i32 i;

    if (!(c = get_container(vcenc_instance)))
        goto out;
    if (!v || !s || !p)
        goto out;

    /* Initialize stream buffers */
    if (init_buffer(&source, vcenc_instance))
        goto out;
    if (get_buffer(&v->ps.b, &source, PRM_SET_BUFF_SIZE, HANTRO_TRUE))
        goto out;
#ifdef TEST_DATA
    Enc_open_stream_trace(&v->ps.b);
#endif

    if (get_buffer(&s->ps.b, &source, PRM_SET_BUFF_SIZE, HANTRO_TRUE))
        goto out;
#ifdef TEST_DATA
    Enc_open_stream_trace(&s->ps.b);
#endif

    if (get_buffer(&p->ps.b, &source, PRM_SET_BUFF_SIZE, HANTRO_TRUE))
        goto out;
#ifdef TEST_DATA
    Enc_open_stream_trace(&p->ps.b);
#endif

    /* Coding unit sizes */
    if (log2i(vcenc_instance->min_cu_size, &tmp))
        goto out;
    if (checkRange(tmp, 3, 3))
        goto out;
    s->log2_min_cb_size = tmp;

    if (log2i(vcenc_instance->max_cu_size, &tmp))
        goto out;
    i32 log2_ctu_size = (IS_H264(vcenc_instance->codecFormat) ? 4 : 6);
    if (checkRange(tmp, log2_ctu_size, log2_ctu_size))
        goto out;
    s->log2_diff_cb_size = tmp - s->log2_min_cb_size;
    p->log2_ctb_size = s->log2_min_cb_size + s->log2_diff_cb_size;
    p->ctb_size = 1 << p->log2_ctb_size;
    ASSERT(p->ctb_size == vcenc_instance->max_cu_size);

    /* Transform size */
    if (log2i(vcenc_instance->min_tr_size, &tmp))
        goto out;
    if (checkRange(tmp, 2, 2))
        goto out;
    s->log2_min_tr_size = tmp;

    if (log2i(vcenc_instance->max_tr_size, &tmp))
        goto out;
    if (checkRange(tmp, s->log2_min_tr_size, MIN(p->log2_ctb_size, 5)))
        goto out;
    s->log2_diff_tr_size = tmp - s->log2_min_tr_size;
    p->log2_max_tr_size = s->log2_min_tr_size + s->log2_diff_tr_size;
    ASSERT(1 << p->log2_max_tr_size == vcenc_instance->max_tr_size);

    /* Max transform hierarchy depth intra/inter */
    tmp = p->log2_ctb_size - s->log2_min_tr_size;
    if (checkRange(vcenc_instance->tr_depth_intra, 0, tmp))
        goto out;
    s->max_tr_hierarchy_depth_intra = vcenc_instance->tr_depth_intra;

    if (checkRange(vcenc_instance->tr_depth_inter, 0, tmp))
        goto out;
    s->max_tr_hierarchy_depth_inter = vcenc_instance->tr_depth_inter;

    s->scaling_list_enable_flag = vcenc_instance->enableScalingList;

    /* Parameter set id */
    if (checkRange(vcenc_instance->vps_id, 0, 15))
        goto out;
    v->ps.id = vcenc_instance->vps_id;

    if (checkRange(vcenc_instance->sps_id, 0, 15))
        goto out;
    s->ps.id = vcenc_instance->sps_id;
    s->vps_id = v->ps.id;

    if (checkRange(vcenc_instance->pps_id, 0, 63))
        goto out;
    p->ps.id = vcenc_instance->pps_id;
    p->sps_id = s->ps.id;

    /* TODO image cropping parameters */
    if (!(vcenc_instance->width > 0))
        goto out;
    if (!(vcenc_instance->height > 0))
        goto out;
    s->width = vcenc_instance->width;
    s->height = vcenc_instance->height;

    /* strong_intra_smoothing_enabled_flag */
    //if (checkRange(s->strong_intra_smoothing_enabled_flag, 0, 1)) goto out;
    s->strong_intra_smoothing_enabled_flag = vcenc_instance->strong_intra_smoothing_enabled_flag;
    ASSERT((s->strong_intra_smoothing_enabled_flag == 0) ||
           (s->strong_intra_smoothing_enabled_flag == 1));

    /* Default quantization parameter */
    if (checkRange(vcenc_instance->rateControl.qpHdr >> QP_FRACTIONAL_BITS, 0, 51))
        goto out;
    p->init_qp = vcenc_instance->rateControl.qpHdr >> QP_FRACTIONAL_BITS;

    /* Picture level rate control TODO... */
    if (checkRange(vcenc_instance->rateControl.picRc, 0, 1))
        goto out;

    /* block qp enable flag */
    p->cu_qp_delta_enabled_flag = 0;
    if (vcenc_instance->asic.regs.rcRoiEnable || vcenc_instance->gdrDuration ||
        vcenc_instance->rateControl.ctbRc
#ifdef INTERNAL_TEST
        || vcenc_instance->testId == TID_ROI
#endif
    ) {
        p->cu_qp_delta_enabled_flag = 1;
    }
    vcenc_instance->asic.regs.cuQpDeltaEnabled = p->cu_qp_delta_enabled_flag;
    p->log2_qp_size = vcenc_instance->log2_qp_size;
    p->diff_cu_qp_delta_depth = p->log2_ctb_size - p->log2_qp_size;

    if (vcenc_instance->asic.regs.RefFrameUsingInputFrameEnable) {
        vcenc_instance->disableDeblocking = 1;
        vcenc_instance->enableSao = 0;
    }

    //tile
    p->tiles_enabled_flag = vcenc_instance->tiles_enabled_flag;
    p->num_tile_columns = vcenc_instance->num_tile_columns;
    p->num_tile_rows = vcenc_instance->num_tile_rows;
    p->loop_filter_across_tiles_enabled_flag =
        vcenc_instance->loop_filter_across_tiles_enabled_flag;

    /* Init the rest. TODO: tiles are not supported yet. How to get
   * slice/tiles from test_bench.c ? */
    if (init_parameter_set(s, p))
        goto out;
    p->deblocking_filter_disabled_flag = vcenc_instance->disableDeblocking;
    p->tc_offset = vcenc_instance->tc_Offset * 2;
    p->beta_offset = vcenc_instance->beta_Offset * 2;
    p->deblocking_filter_override_enabled_flag = vcenc_instance->enableDeblockOverride;

    p->cb_qp_offset = vcenc_instance->chromaQpOffset;
    p->cr_qp_offset = vcenc_instance->chromaQpOffset;

    s->sao_enabled_flag = vcenc_instance->enableSao;

    s->frameCropLeftOffset = vcenc_instance->preProcess.frameCropLeftOffset;
    s->frameCropRightOffset = vcenc_instance->preProcess.frameCropRightOffset;
    s->frameCropTopOffset = vcenc_instance->preProcess.frameCropTopOffset;
    s->frameCropBottomOffset = vcenc_instance->preProcess.frameCropBottomOffset;

    v->streamMode = vcenc_instance->asic.regs.streamMode;
    s->streamMode = vcenc_instance->asic.regs.streamMode;
    p->streamMode = vcenc_instance->asic.regs.streamMode;

    v->general_level_idc = vcenc_instance->level;
    s->general_level_idc = vcenc_instance->level;

    v->general_profile_idc = vcenc_instance->profile;
    s->general_profile_idc = vcenc_instance->profile;

    s->chroma_format_idc = vcenc_instance->asic.regs.codedChromaIdc;

    v->general_tier_flag = vcenc_instance->tier;
    s->general_tier_flag = vcenc_instance->tier;

    s->pcm_enabled_flag = vcenc_instance->pcm_enabled_flag;
    s->pcm_sample_bit_depth_luma_minus1 = vcenc_instance->sps->bit_depth_luma_minus8 + 7;
    s->pcm_sample_bit_depth_chroma_minus1 = vcenc_instance->sps->bit_depth_chroma_minus8 + 7;
    s->pcm_loop_filter_disabled_flag = vcenc_instance->pcm_loop_filter_disabled_flag;

    /* Set H264 sps*/
    if (IS_H264(vcenc_instance->codecFormat)) {
        /* Internal images, next macroblock boundary */
        i32 width = 16 * ((s->width + 15) / 16);
        i32 height = 16 * ((s->height + 15) / 16);
        i32 mbPerFrame = (width / 16) * (height / 16);
        if (vcenc_instance->interlaced) {
            s->frameMbsOnly = ENCHW_NO;
            /* Map unit 32-pixels high for fields */
            height = 32 * ((s->height + 31) / 32);
        }
        s->picWidthInMbsMinus1 = width / 16 - 1;
        s->picHeightInMapUnitsMinus1 = height / (16 * (1 + vcenc_instance->interlaced)) - 1;

        /* Set cropping parameters if required */
        if (s->width % 16 || s->height % 16 || (vcenc_instance->interlaced && s->height % 32)) {
            s->frameCropping = ENCHW_YES;
        } else {
            s->frameCropping = ENCHW_NO;
        }

        /* Level 1b is indicated with levelIdc == 11 (later) and constraintSet3 */
        if (vcenc_instance->level == VCENC_H264_LEVEL_1_b) {
            s->constraintSet3 = ENCHW_YES;
        }

        /* Level 1b is indicated with levelIdc == 11 (constraintSet3) */
        if (vcenc_instance->level == VCENC_H264_LEVEL_1_b) {
            s->general_level_idc = 11;
        }

        /* Interlaced => main profile */
        if (vcenc_instance->interlaced >= 1 && s->general_profile_idc == 66)
            s->general_profile_idc = 77;

        p->transform8x8Mode = (s->general_profile_idc >= 100 ? ENCHW_YES : ENCHW_NO);
        p->entropy_coding_mode_flag = vcenc_instance->asic.regs.entropy_coding_mode_flag;

        /* always CABAC => main profile */
        if (s->general_profile_idc == 66) {
            ASSERT(p->entropy_coding_mode_flag == 0);
        }

        /*  always 8x8 transform enabled => high profile */
        ASSERT(p->transform8x8Mode == ENCHW_NO || s->general_profile_idc >= 100);

        /* set numRefFrame*/
        s->numRefFrames = s->max_dec_pic_buffering[0] - 1;
        vcenc_instance->frameNum = 0;
        vcenc_instance->idrPicId = 0;
        vcenc_instance->h264_mmo_nops = 0;
    }

    vcenc_instance->asic.regs.outputBitWidthLuma = s->bit_depth_luma_minus8;
    vcenc_instance->asic.regs.outputBitWidthChroma = s->bit_depth_chroma_minus8;

    p->lists_modification_present_flag = check_ref_lists_modification(pEncIn);
    if (pEncIn->gopConfig.ltrcnt > 0)
        s->long_term_ref_pics_present_flag = 1;

    s->temporal_mvp_enable_flag = vcenc_instance->enableTMVP;

#ifdef SUPPORT_AV1
    if (IS_AV1(vcenc_instance->codecFormat)) {
        AV1SetParameter(vcenc_instance, s, p, v);
    }
#endif

#ifdef SUPPORT_VP9
    /*TODO: VP9 read profile from user*/
    if (IS_VP9(vcenc_instance->codecFormat)) {
        VP9SetParameter(vcenc_instance, v, s);
    }
#endif

    return VCENC_OK;

out:
    /*
    The memory instances are allocated in VCEncInit()
    should be released in VCEncRelease()
    */
    //free_parameter_set((struct ps *)v);
    //free_parameter_set((struct ps *)s);
    //free_parameter_set((struct ps *)p);

    return VCENC_ERROR;
}

VCEncRet VCEncGetCodingCtrl(VCEncInst inst, VCEncCodingCtrl *pCodeParams)
{
    struct vcenc_instance *pEncInst = (struct vcenc_instance *)inst;

    /* Check for illegal inputs */
    if ((pEncInst == NULL) || (pCodeParams == NULL)) {
        APITRACEERR("VCEncGetCodingCtrl: ERROR Null argument\n");
        return VCENC_NULL_ARGUMENT;
    }

    /* Check for existing instance */
    if (pEncInst->inst != pEncInst) {
        APITRACEERR("VCEncGetCodingCtrl: ERROR Invalid instance\n");
        return VCENC_INSTANCE_ERROR;
    }

    // get last coding ctrl parameter set
    EncCodingCtrlParam *pLastCodingCtrlParam =
        (EncCodingCtrlParam *)queue_head(&pEncInst->codingCtrl.codingCtrlQueue);
    if (pLastCodingCtrlParam) {
        memcpy(pCodeParams, &pLastCodingCtrlParam->encCodingCtrl, sizeof(VCEncCodingCtrl));
        return VCENC_OK;
    }

    //if queue is empty, get codingctrl from instance
    regValues_s *regs = &pEncInst->asic.regs;
    i32 i;
    APITRACE("VCEncGetCodingCtrl#\n");

    pCodeParams->seiMessages = (pEncInst->rateControl.sei.enabled == ENCHW_YES) ? 1 : 0;

    pCodeParams->disableDeblockingFilter = pEncInst->disableDeblocking;
    pCodeParams->fieldOrder = pEncInst->fieldOrder;

    pCodeParams->sramPowerdownDisable = regs->sramPowerdownDisable;

    pCodeParams->tc_Offset = pEncInst->tc_Offset;
    pCodeParams->beta_Offset = pEncInst->beta_Offset;
    pCodeParams->enableSao = pEncInst->enableSao;
    pCodeParams->enableScalingList = pEncInst->enableScalingList;
    pCodeParams->vuiVideoFullRange = (pEncInst->sps->vui.vuiVideoFullRange == ENCHW_YES) ? 1 : 0;
    pCodeParams->sampleAspectRatioWidth = pEncInst->sarWidth;
    pCodeParams->sampleAspectRatioHeight = pEncInst->sarHeight;
    pCodeParams->sliceSize = regs->sliceSize;

    pCodeParams->cabacInitFlag = regs->cabac_init_flag;
    pCodeParams->enableCabac = regs->entropy_coding_mode_flag;

    pCodeParams->cirStart = regs->cirStart;
    pCodeParams->cirInterval = regs->cirInterval;
    pCodeParams->intraArea.top = regs->intraAreaTop;
    pCodeParams->intraArea.left = regs->intraAreaLeft;
    pCodeParams->intraArea.bottom = regs->intraAreaBottom;
    pCodeParams->intraArea.right = regs->intraAreaRight;
    if ((pCodeParams->intraArea.top <= pCodeParams->intraArea.bottom &&
         pCodeParams->intraArea.bottom < (u32)pEncInst->ctbPerCol &&
         pCodeParams->intraArea.left <= pCodeParams->intraArea.right &&
         pCodeParams->intraArea.right < (u32)pEncInst->ctbPerRow)) {
        pCodeParams->intraArea.enable = 1;
    } else {
        pCodeParams->intraArea.enable = 0;
    }
    //GDR will use this area, so we need to turn it off when get from instance
    if (pEncInst->gdrDuration > 0)
        pCodeParams->intraArea.enable = 0;

    pCodeParams->ipcm1Area.top = regs->ipcm1AreaTop;
    pCodeParams->ipcm1Area.left = regs->ipcm1AreaLeft;
    pCodeParams->ipcm1Area.bottom = regs->ipcm1AreaBottom;
    pCodeParams->ipcm1Area.right = regs->ipcm1AreaRight;
    if ((pCodeParams->ipcm1Area.top <= pCodeParams->ipcm1Area.bottom &&
         pCodeParams->ipcm1Area.bottom < (u32)pEncInst->ctbPerCol &&
         pCodeParams->ipcm1Area.left <= pCodeParams->ipcm1Area.right &&
         pCodeParams->ipcm1Area.right < (u32)pEncInst->ctbPerRow)) {
        pCodeParams->ipcm1Area.enable = 1;
    } else {
        pCodeParams->ipcm1Area.enable = 0;
    }
    //GDR will use this area, so we need to turn it off when get from instance
    if (pEncInst->gdrDuration > 0)
        pCodeParams->ipcm1Area.enable = 0;

    pCodeParams->ipcm2Area.top = regs->ipcm2AreaTop;
    pCodeParams->ipcm2Area.left = regs->ipcm2AreaLeft;
    pCodeParams->ipcm2Area.bottom = regs->ipcm2AreaBottom;
    pCodeParams->ipcm2Area.right = regs->ipcm2AreaRight;
    if ((pCodeParams->ipcm2Area.top <= pCodeParams->ipcm2Area.bottom &&
         pCodeParams->ipcm2Area.bottom < (u32)pEncInst->ctbPerCol &&
         pCodeParams->ipcm2Area.left <= pCodeParams->ipcm2Area.right &&
         pCodeParams->ipcm2Area.right < (u32)pEncInst->ctbPerRow)) {
        pCodeParams->ipcm2Area.enable = 1;
    } else {
        pCodeParams->ipcm2Area.enable = 0;
    }
    //GDR will use this area, so we need to turn it off when get from instance
    if (pEncInst->gdrDuration > 0)
        pCodeParams->ipcm2Area.enable = 0;

    pCodeParams->ipcm3Area.top = regs->ipcm3AreaTop;
    pCodeParams->ipcm3Area.left = regs->ipcm3AreaLeft;
    pCodeParams->ipcm3Area.bottom = regs->ipcm3AreaBottom;
    pCodeParams->ipcm3Area.right = regs->ipcm3AreaRight;
    if ((pCodeParams->ipcm3Area.top <= pCodeParams->ipcm3Area.bottom &&
         pCodeParams->ipcm3Area.bottom < (u32)pEncInst->ctbPerCol &&
         pCodeParams->ipcm3Area.left <= pCodeParams->ipcm3Area.right &&
         pCodeParams->ipcm3Area.right < (u32)pEncInst->ctbPerRow)) {
        pCodeParams->ipcm3Area.enable = 1;
    } else {
        pCodeParams->ipcm3Area.enable = 0;
    }
    //GDR will use this area, so we need to turn it off when get from instance
    if (pEncInst->gdrDuration > 0)
        pCodeParams->ipcm3Area.enable = 0;

    pCodeParams->ipcm4Area.top = regs->ipcm4AreaTop;
    pCodeParams->ipcm4Area.left = regs->ipcm4AreaLeft;
    pCodeParams->ipcm4Area.bottom = regs->ipcm4AreaBottom;
    pCodeParams->ipcm4Area.right = regs->ipcm4AreaRight;
    if ((pCodeParams->ipcm4Area.top <= pCodeParams->ipcm4Area.bottom &&
         pCodeParams->ipcm4Area.bottom < (u32)pEncInst->ctbPerCol &&
         pCodeParams->ipcm4Area.left <= pCodeParams->ipcm4Area.right &&
         pCodeParams->ipcm4Area.right < (u32)pEncInst->ctbPerRow)) {
        pCodeParams->ipcm4Area.enable = 1;
    } else {
        pCodeParams->ipcm4Area.enable = 0;
    }
    //GDR will use this area, so we need to turn it off when get from instance
    if (pEncInst->gdrDuration > 0)
        pCodeParams->ipcm4Area.enable = 0;

    pCodeParams->ipcm5Area.top = regs->ipcm5AreaTop;
    pCodeParams->ipcm5Area.left = regs->ipcm5AreaLeft;
    pCodeParams->ipcm5Area.bottom = regs->ipcm5AreaBottom;
    pCodeParams->ipcm5Area.right = regs->ipcm5AreaRight;
    if ((pCodeParams->ipcm5Area.top <= pCodeParams->ipcm5Area.bottom &&
         pCodeParams->ipcm5Area.bottom < (u32)pEncInst->ctbPerCol &&
         pCodeParams->ipcm5Area.left <= pCodeParams->ipcm5Area.right &&
         pCodeParams->ipcm5Area.right < (u32)pEncInst->ctbPerRow)) {
        pCodeParams->ipcm5Area.enable = 1;
    } else {
        pCodeParams->ipcm5Area.enable = 0;
    }
    //GDR will use this area, so we need to turn it off when get from instance
    if (pEncInst->gdrDuration > 0)
        pCodeParams->ipcm5Area.enable = 0;

    pCodeParams->ipcm6Area.top = regs->ipcm6AreaTop;
    pCodeParams->ipcm6Area.left = regs->ipcm6AreaLeft;
    pCodeParams->ipcm6Area.bottom = regs->ipcm6AreaBottom;
    pCodeParams->ipcm6Area.right = regs->ipcm6AreaRight;
    if ((pCodeParams->ipcm6Area.top <= pCodeParams->ipcm6Area.bottom &&
         pCodeParams->ipcm6Area.bottom < (u32)pEncInst->ctbPerCol &&
         pCodeParams->ipcm6Area.left <= pCodeParams->ipcm6Area.right &&
         pCodeParams->ipcm6Area.right < (u32)pEncInst->ctbPerRow)) {
        pCodeParams->ipcm6Area.enable = 1;
    } else {
        pCodeParams->ipcm6Area.enable = 0;
    }
    //GDR will use this area, so we need to turn it off when get from instance
    if (pEncInst->gdrDuration > 0)
        pCodeParams->ipcm6Area.enable = 0;

    pCodeParams->ipcm7Area.top = regs->ipcm7AreaTop;
    pCodeParams->ipcm7Area.left = regs->ipcm7AreaLeft;
    pCodeParams->ipcm7Area.bottom = regs->ipcm7AreaBottom;
    pCodeParams->ipcm7Area.right = regs->ipcm7AreaRight;
    if ((pCodeParams->ipcm7Area.top <= pCodeParams->ipcm7Area.bottom &&
         pCodeParams->ipcm7Area.bottom < (u32)pEncInst->ctbPerCol &&
         pCodeParams->ipcm7Area.left <= pCodeParams->ipcm7Area.right &&
         pCodeParams->ipcm7Area.right < (u32)pEncInst->ctbPerRow)) {
        pCodeParams->ipcm7Area.enable = 1;
    } else {
        pCodeParams->ipcm7Area.enable = 0;
    }
    //GDR will use this area, so we need to turn it off when get from instance
    if (pEncInst->gdrDuration > 0)
        pCodeParams->ipcm7Area.enable = 0;

    pCodeParams->ipcm8Area.top = regs->ipcm8AreaTop;
    pCodeParams->ipcm8Area.left = regs->ipcm8AreaLeft;
    pCodeParams->ipcm8Area.bottom = regs->ipcm8AreaBottom;
    pCodeParams->ipcm8Area.right = regs->ipcm8AreaRight;
    if ((pCodeParams->ipcm8Area.top <= pCodeParams->ipcm8Area.bottom &&
         pCodeParams->ipcm8Area.bottom < (u32)pEncInst->ctbPerCol &&
         pCodeParams->ipcm8Area.left <= pCodeParams->ipcm8Area.right &&
         pCodeParams->ipcm8Area.right < (u32)pEncInst->ctbPerRow)) {
        pCodeParams->ipcm8Area.enable = 1;
    } else {
        pCodeParams->ipcm8Area.enable = 0;
    }
    //GDR will use this area, so we need to turn it off when get from instance
    if (pEncInst->gdrDuration > 0)
        pCodeParams->ipcm8Area.enable = 0;

    pCodeParams->roi1Area.top = regs->roi1Top;
    pCodeParams->roi1Area.left = regs->roi1Left;
    pCodeParams->roi1Area.bottom = regs->roi1Bottom;
    pCodeParams->roi1Area.right = regs->roi1Right;
    if (pCodeParams->roi1Area.top <= pCodeParams->roi1Area.bottom &&
        pCodeParams->roi1Area.bottom < (u32)pEncInst->ctbPerCol &&
        pCodeParams->roi1Area.left <= pCodeParams->roi1Area.right &&
        pCodeParams->roi1Area.right < (u32)pEncInst->ctbPerRow) {
        pCodeParams->roi1Area.enable = 1;
    } else {
        pCodeParams->roi1Area.enable = 0;
    }
    //GDR will use this area, so we need to turn it off when get from instance
    if (pEncInst->gdrDuration > 0)
        pCodeParams->roi1Area.enable = 0;

    pCodeParams->roi2Area.top = regs->roi2Top;
    pCodeParams->roi2Area.left = regs->roi2Left;
    pCodeParams->roi2Area.bottom = regs->roi2Bottom;
    pCodeParams->roi2Area.right = regs->roi2Right;
    if ((pCodeParams->roi2Area.top <= pCodeParams->roi2Area.bottom &&
         pCodeParams->roi2Area.bottom < (u32)pEncInst->ctbPerCol &&
         pCodeParams->roi2Area.left <= pCodeParams->roi2Area.right &&
         pCodeParams->roi2Area.right < (u32)pEncInst->ctbPerRow)) {
        pCodeParams->roi2Area.enable = 1;
    } else {
        pCodeParams->roi2Area.enable = 0;
    }

    pCodeParams->roi1DeltaQp = -regs->roi1DeltaQp;
    pCodeParams->roi2DeltaQp = -regs->roi2DeltaQp;
    pCodeParams->roi1Qp = regs->roi1Qp;
    pCodeParams->roi2Qp = regs->roi2Qp;

    pCodeParams->roi3Area.top = regs->roi3Top;
    pCodeParams->roi3Area.left = regs->roi3Left;
    pCodeParams->roi3Area.bottom = regs->roi3Bottom;
    pCodeParams->roi3Area.right = regs->roi3Right;
    if ((pCodeParams->roi3Area.top <= pCodeParams->roi3Area.bottom &&
         pCodeParams->roi3Area.bottom < (u32)pEncInst->ctbPerCol &&
         pCodeParams->roi3Area.left <= pCodeParams->roi3Area.right &&
         pCodeParams->roi3Area.right < (u32)pEncInst->ctbPerRow)) {
        pCodeParams->roi3Area.enable = 1;
    } else {
        pCodeParams->roi3Area.enable = 0;
    }

    pCodeParams->roi4Area.top = regs->roi4Top;
    pCodeParams->roi4Area.left = regs->roi4Left;
    pCodeParams->roi4Area.bottom = regs->roi4Bottom;
    pCodeParams->roi4Area.right = regs->roi4Right;
    if ((pCodeParams->roi4Area.top <= pCodeParams->roi4Area.bottom &&
         pCodeParams->roi4Area.bottom < (u32)pEncInst->ctbPerCol &&
         pCodeParams->roi4Area.left <= pCodeParams->roi4Area.right &&
         pCodeParams->roi4Area.right < (u32)pEncInst->ctbPerRow)) {
        pCodeParams->roi4Area.enable = 1;
    } else {
        pCodeParams->roi4Area.enable = 0;
    }

    pCodeParams->roi5Area.top = regs->roi5Top;
    pCodeParams->roi5Area.left = regs->roi5Left;
    pCodeParams->roi5Area.bottom = regs->roi5Bottom;
    pCodeParams->roi5Area.right = regs->roi5Right;
    if ((pCodeParams->roi5Area.top <= pCodeParams->roi5Area.bottom &&
         pCodeParams->roi5Area.bottom < (u32)pEncInst->ctbPerCol &&
         pCodeParams->roi5Area.left <= pCodeParams->roi5Area.right &&
         pCodeParams->roi5Area.right < (u32)pEncInst->ctbPerRow)) {
        pCodeParams->roi5Area.enable = 1;
    } else {
        pCodeParams->roi5Area.enable = 0;
    }

    pCodeParams->roi6Area.top = regs->roi6Top;
    pCodeParams->roi6Area.left = regs->roi6Left;
    pCodeParams->roi6Area.bottom = regs->roi6Bottom;
    pCodeParams->roi6Area.right = regs->roi6Right;
    if ((pCodeParams->roi6Area.top <= pCodeParams->roi6Area.bottom &&
         pCodeParams->roi6Area.bottom < (u32)pEncInst->ctbPerCol &&
         pCodeParams->roi6Area.left <= pCodeParams->roi6Area.right &&
         pCodeParams->roi6Area.right < (u32)pEncInst->ctbPerRow)) {
        pCodeParams->roi6Area.enable = 1;
    } else {
        pCodeParams->roi6Area.enable = 0;
    }

    pCodeParams->roi7Area.top = regs->roi7Top;
    pCodeParams->roi7Area.left = regs->roi7Left;
    pCodeParams->roi7Area.bottom = regs->roi7Bottom;
    pCodeParams->roi7Area.right = regs->roi7Right;
    if ((pCodeParams->roi7Area.top <= pCodeParams->roi7Area.bottom &&
         pCodeParams->roi7Area.bottom < (u32)pEncInst->ctbPerCol &&
         pCodeParams->roi7Area.left <= pCodeParams->roi7Area.right &&
         pCodeParams->roi7Area.right < (u32)pEncInst->ctbPerRow)) {
        pCodeParams->roi7Area.enable = 1;
    } else {
        pCodeParams->roi7Area.enable = 0;
    }

    pCodeParams->roi8Area.top = regs->roi8Top;
    pCodeParams->roi8Area.left = regs->roi8Left;
    pCodeParams->roi8Area.bottom = regs->roi8Bottom;
    pCodeParams->roi8Area.right = regs->roi8Right;
    if ((pCodeParams->roi8Area.top <= pCodeParams->roi8Area.bottom &&
         pCodeParams->roi8Area.bottom < (u32)pEncInst->ctbPerCol &&
         pCodeParams->roi8Area.left <= pCodeParams->roi8Area.right &&
         pCodeParams->roi8Area.right < (u32)pEncInst->ctbPerRow)) {
        pCodeParams->roi8Area.enable = 1;
    } else {
        pCodeParams->roi8Area.enable = 0;
    }

    pCodeParams->roi3DeltaQp = -regs->roi3DeltaQp;
    pCodeParams->roi4DeltaQp = -regs->roi4DeltaQp;
    pCodeParams->roi5DeltaQp = -regs->roi5DeltaQp;
    pCodeParams->roi6DeltaQp = -regs->roi6DeltaQp;
    pCodeParams->roi7DeltaQp = -regs->roi7DeltaQp;
    pCodeParams->roi8DeltaQp = -regs->roi8DeltaQp;
    pCodeParams->roi3Qp = regs->roi3Qp;
    pCodeParams->roi4Qp = regs->roi4Qp;
    pCodeParams->roi5Qp = regs->roi5Qp;
    pCodeParams->roi6Qp = regs->roi6Qp;
    pCodeParams->roi7Qp = regs->roi7Qp;
    pCodeParams->roi8Qp = regs->roi8Qp;

    pCodeParams->pcm_enabled_flag = pEncInst->pcm_enabled_flag;
    pCodeParams->pcm_loop_filter_disabled_flag = pEncInst->pcm_loop_filter_disabled_flag;

    pCodeParams->enableDeblockOverride = pEncInst->enableDeblockOverride;

    pCodeParams->deblockOverride = regs->slice_deblocking_filter_override_flag;

    pCodeParams->chroma_qp_offset = pEncInst->chromaQpOffset;

    pCodeParams->roiMapDeltaQpEnable = pEncInst->roiMapEnable;
    pCodeParams->roiMapDeltaQpBlockUnit = regs->diffCuQpDeltaDepth;
    pCodeParams->ipcmMapEnable = regs->ipcmMapEnable;
    pCodeParams->RoimapCuCtrl_index_enable = pEncInst->RoimapCuCtrl_index_enable;
    pCodeParams->RoimapCuCtrl_enable = pEncInst->RoimapCuCtrl_enable;
    pCodeParams->RoimapCuCtrl_ver = pEncInst->RoimapCuCtrl_ver;
    pCodeParams->RoiQpDelta_ver = pEncInst->RoiQpDelta_ver;

    /* SKIP map */
    pCodeParams->skipMapEnable = regs->skipMapEnable;

    /* RDOQ map */
    pCodeParams->rdoqMapEnable = regs->rdoqMapEnable;

    pCodeParams->gdrDuration = pEncInst->gdrEnabled ? pEncInst->gdrDuration : 0;

    VCEncHEVCDnfGetParameters(pEncInst, pCodeParams);

    /* low latency */
    pCodeParams->inputLineBufEn = pEncInst->inputLineBuf.inputLineBufEn;
    pCodeParams->inputLineBufLoopBackEn = pEncInst->inputLineBuf.inputLineBufLoopBackEn;
    pCodeParams->inputLineBufDepth = pEncInst->inputLineBuf.inputLineBufDepth;
    pCodeParams->amountPerLoopBack = pEncInst->inputLineBuf.amountPerLoopBack;
    pCodeParams->inputLineBufHwModeEn = pEncInst->inputLineBuf.inputLineBufHwModeEn;
    pCodeParams->inputLineBufCbFunc = pEncInst->inputLineBuf.cbFunc;
    pCodeParams->inputLineBufCbData = pEncInst->inputLineBuf.cbData;

    /*stream multi-segment*/
    pCodeParams->streamMultiSegmentMode = pEncInst->streamMultiSegment.streamMultiSegmentMode;
    pCodeParams->streamMultiSegmentAmount = pEncInst->streamMultiSegment.streamMultiSegmentAmount;
    pCodeParams->streamMultiSegCbFunc = pEncInst->streamMultiSegment.cbFunc;
    pCodeParams->streamMultiSegCbData = pEncInst->streamMultiSegment.cbData;

    /* smart */
    pCodeParams->smartModeEnable = pEncInst->smartModeEnable;
    pCodeParams->smartH264LumDcTh = pEncInst->smartH264LumDcTh;
    pCodeParams->smartH264CbDcTh = pEncInst->smartH264CbDcTh;
    pCodeParams->smartH264CrDcTh = pEncInst->smartH264CrDcTh;
    for (i = 0; i < 3; i++) {
        pCodeParams->smartHevcLumDcTh[i] = pEncInst->smartHevcLumDcTh[i];
        pCodeParams->smartHevcChrDcTh[i] = pEncInst->smartHevcChrDcTh[i];
        pCodeParams->smartHevcLumAcNumTh[i] = pEncInst->smartHevcLumAcNumTh[i];
        pCodeParams->smartHevcChrAcNumTh[i] = pEncInst->smartHevcChrAcNumTh[i];
    }
    pCodeParams->smartH264Qp = pEncInst->smartH264Qp;
    pCodeParams->smartHevcLumQp = pEncInst->smartHevcLumQp;
    pCodeParams->smartHevcChrQp = pEncInst->smartHevcChrQp;
    for (i = 0; i < 4; i++)
        pCodeParams->smartMeanTh[i] = pEncInst->smartMeanTh[i];
    pCodeParams->smartPixNumCntTh = pEncInst->smartPixNumCntTh;

    pCodeParams->itu_t_t35 = pEncInst->itu_t_t35;

    pCodeParams->write_once_HDR10 = pEncInst->write_once_HDR10;
    pCodeParams->Hdr10Display = pEncInst->Hdr10Display;
    pCodeParams->Hdr10Display.hdr10_display_enable =
        (pEncInst->Hdr10Display.hdr10_display_enable == (u8)HDR10_NOCFGED) ? 0 : 1;
    pCodeParams->vuiColorDescription = pEncInst->vuiColorDescription;

    pCodeParams->Hdr10LightLevel = pEncInst->Hdr10LightLevel;
    pCodeParams->Hdr10LightLevel.hdr10_lightlevel_enable =
        (pEncInst->Hdr10LightLevel.hdr10_lightlevel_enable == (u8)HDR10_NOCFGED) ? 0 : 1;

    pCodeParams->vuiVideoSignalTypePresentFlag = pEncInst->vuiVideoSignalTypePresentFlag;
    pCodeParams->vuiVideoFormat = pEncInst->vuiVideoFormat;

    pCodeParams->RpsInSliceHeader = pEncInst->RpsInSliceHeader;

    pCodeParams->enableRdoQuant = regs->bRDOQEnable;

    pCodeParams->enableDynamicRdo = regs->dynamicRdoEnable;
    pCodeParams->dynamicRdoCu16Bias = regs->dynamicRdoCu16Bias;
    pCodeParams->dynamicRdoCu16Factor = regs->dynamicRdoCu16Factor;
    pCodeParams->dynamicRdoCu32Bias = regs->dynamicRdoCu32Bias;
    pCodeParams->dynamicRdoCu32Factor = regs->dynamicRdoCu32Factor;
    /* Assigned ME vertical search range */
    pCodeParams->meVertSearchRange = regs->meAssignedVertRange << 3;

    /* some subjective quality params: aq_mode and psyFactor */
    pCodeParams->aq_mode = pEncInst->cuTreeCtl.aq_mode;
    pCodeParams->aq_strength = pEncInst->cuTreeCtl.aqStrength;
    pCodeParams->psyFactor = pEncInst->psyFactor;

    APITRACE("VCEncGetCodingCtrl: OK\n");
    return VCENC_OK;
}

/* recalculate level based on RC parameters */
static VCEncLevel rc_recalculate_level(struct vcenc_instance *inst, u32 cpbSize, u32 bps)
{
    i32 i = 0, j = 0, levelIdx = inst->levelIdx;
    i32 maxLevel = 0;
    switch (inst->codecFormat) {
    case VCENC_VIDEO_CODEC_HEVC:
        maxLevel = HEVC_LEVEL_NUM - 1;
        break;
    case VCENC_VIDEO_CODEC_H264:
        maxLevel = H264_LEVEL_NUM - 1;
        break;

    case VCENC_VIDEO_CODEC_AV1:
        maxLevel = AV1_VALID_MAX_LEVEL - 1;
        break;

    case VCENC_VIDEO_CODEC_VP9:
        maxLevel = VP9_VALID_MAX_LEVEL - 1;
        break;

    default:
        break;
    }

    if (cpbSize > getMaxCPBS(inst->codecFormat, maxLevel, inst->profile, inst->tier)) {
        APITRACEERR("rc_recalculate_level: WARNING Invalid cpbSize.\n");
        i = j = maxLevel;
    }
    if (bps > getMaxBR(inst->codecFormat, maxLevel, inst->profile, inst->tier)) {
        APITRACEERR("rc_recalculate_level: WARNING Invalid bitsPerSecond.\n");
        i = j = maxLevel;
    }
    for (i = 0; i < maxLevel; i++) {
        if (cpbSize <= getMaxCPBS(inst->codecFormat, i, inst->profile, inst->tier))
            break;
    }
    for (j = 0; j < maxLevel; j++) {
        if (bps <= getMaxBR(inst->codecFormat, j, inst->profile, inst->tier))
            break;
    }

    levelIdx = MAX(levelIdx, MAX(i, j));
    return (getLevel(inst->codecFormat, levelIdx));
}

/*------------------------------------------------------------------------------

    Function name : VCEncSetRateCtrl
    Description   : Sets rate control parameters

    Return type   : VCEncRet
    Argument      : inst - the instance in use
                    pRateCtrl - user provided parameters
------------------------------------------------------------------------------*/
VCEncRet VCEncSetRateCtrl(VCEncInst inst, const VCEncRateCtrl *pRateCtrl)
{
    struct vcenc_instance *vcenc_instance = (struct vcenc_instance *)inst;
    vcencRateControl_s *rc;
    u32 i, tmp;
    i32 prevBitrate;
    regValues_s *regs = &vcenc_instance->asic.regs;
    i32 bitrateWindow;
    i32 rate_control_mode_change = 0;
    i32 frame_rate_change = 0;
    i32 ctbRcAllowed = vcenc_instance->encStatus < VCENCSTAT_START_STREAM ||
                       vcenc_instance->asic.regs.cuQpDeltaEnabled;

    APITRACE("VCEncSetRateCtrl#\n");
    APITRACEPARAM(" %s : %d\n", "pictureRc", pRateCtrl->pictureRc);
    APITRACEPARAM(" %s : %d\n", "pictureSkip", pRateCtrl->pictureSkip);
    APITRACEPARAM(" %s : %d\n", "qpHdr", pRateCtrl->qpHdr);
    APITRACEPARAM(" %s : %d\n", "qpMinPB", pRateCtrl->qpMinPB);
    APITRACEPARAM(" %s : %d\n", "qpMaxPB", pRateCtrl->qpMaxPB);
    APITRACEPARAM(" %s : %d\n", "qpMinI", pRateCtrl->qpMinI);
    APITRACEPARAM(" %s : %d\n", "qpMaxI", pRateCtrl->qpMaxI);
    APITRACEPARAM(" %s : %d\n", "bitPerSecond", pRateCtrl->bitPerSecond);
    APITRACEPARAM(" %s : %d\n", "rcMode", pRateCtrl->rcMode);
    APITRACEPARAM(" %s : %d\n", "hrd", pRateCtrl->hrd);
    APITRACEPARAM(" %s : %d\n", "hrdCpbSize", pRateCtrl->hrdCpbSize);
    APITRACEPARAM(" %s : %d\n", "bitrateWindow", pRateCtrl->bitrateWindow);
    APITRACEPARAM(" %s : %d\n", "intraQpDelta", pRateCtrl->intraQpDelta);
    APITRACEPARAM(" %s : %d\n", "fixedIntraQp", pRateCtrl->fixedIntraQp);
    APITRACEPARAM(" %s : %d\n", "bitVarRangeI", pRateCtrl->bitVarRangeI);
    APITRACEPARAM(" %s : %d\n", "bitVarRangeP", pRateCtrl->bitVarRangeP);
    APITRACEPARAM(" %s : %d\n", "bitVarRangeB", pRateCtrl->bitVarRangeB);
    APITRACEPARAM(" %s : %d\n", "staticSceneIbitPercent", pRateCtrl->u32StaticSceneIbitPercent);
    APITRACEPARAM(" %s : %d\n", "ctbRc", pRateCtrl->ctbRc);
    APITRACEPARAM(" %s : %d\n", "blockRCSize", pRateCtrl->blockRCSize);
    APITRACEPARAM(" %s : %d\n", "rcQpDeltaRange", pRateCtrl->rcQpDeltaRange);
    APITRACEPARAM(" %s : %d\n", "rcBaseMBComplexity", pRateCtrl->rcBaseMBComplexity);
    APITRACEPARAM(" %s : %d\n", "picQpDeltaMin", pRateCtrl->picQpDeltaMin);
    APITRACEPARAM(" %s : %d\n", "picQpDeltaMax", pRateCtrl->picQpDeltaMax);
    APITRACEPARAM(" %s : %d\n", "vbr", pRateCtrl->vbr);
    APITRACEPARAM(" %s : %d\n", "ctbRcQpDeltaReverse", pRateCtrl->ctbRcQpDeltaReverse);

    /* Check for illegal inputs */
    if ((vcenc_instance == NULL) || (pRateCtrl == NULL)) {
        APITRACEERR("VCEncSetRateCtrl: ERROR Null argument\n");
        return VCENC_NULL_ARGUMENT;
    }

    /* Check for existing instance */
    if (vcenc_instance->inst != vcenc_instance) {
        APITRACEERR("VCEncSetRateCtrl: ERROR Invalid instance\n");
        return VCENC_INSTANCE_ERROR;
    }

    rc = &vcenc_instance->rateControl;
    if (pRateCtrl->ctbRcQpDeltaReverse > 1) {
        APITRACEERR("VCEncSetRateCtrl: ERROR ctbRcQpDeltaReverse out of range\n");
        return VCENC_INVALID_ARGUMENT;
    }
    rc->ctbRcQpDeltaReverse = pRateCtrl->ctbRcQpDeltaReverse;

    /* after stream was started with HRD ON,
   * it is not allowed to change RC params */
    if (vcenc_instance->encStatus == VCENCSTAT_START_FRAME && rc->hrd == ENCHW_YES) {
        APITRACEERR("VCEncSetRateCtrl: ERROR Stream started with HRD ON. Not allowed to "
                    "change any parameters\n");
        return VCENC_INVALID_STATUS;
    }

    /* check ctbRc setting */
    if (pRateCtrl->ctbRc) {
        if (pRateCtrl->ctbRc > 3) {
            APITRACEERR("VCEncSetCodingCtrl: ERROR Invalid ctbRc mode\n");
            return VCENC_INVALID_ARGUMENT;
        }
        if (ctbRcAllowed == 0) {
            APITRACEERR("VCEncSetCodingCtrl: ERROR Invalid encoding status to enable "
                        "ctbRc\n");
            return VCENC_INVALID_ARGUMENT;
        }
        if (HW_ID_MAJOR_NUMBER(regs->asicHwId) < 2 /* H2V1 */) {
            APITRACEERR("VCEncSetRateCtrl: ERROR CTB RC not supported\n");
            return VCENC_INVALID_ARGUMENT;
        }
        if (pRateCtrl->blockRCSize > 2 || (IS_AV1(vcenc_instance->codecFormat) &&
                                           pRateCtrl->blockRCSize > 0)) //AV1 qpDelta in CTU level
        {
            APITRACEERR("VCEncSetRateCtrl: ERROR Invalid blockRCSize\n");
            return VCENC_INVALID_ARGUMENT;
        }
        if (pRateCtrl->ctbRc >= 2 && pRateCtrl->crf >= 0) {
            APITRACEERR("VCEncSetRateCtrl: ERROR crf is set with ctbRc>=2\n");
            return VCENC_INVALID_ARGUMENT;
        }
    }

    /* Check for invalid input values */
    if (pRateCtrl->pictureRc > 1 || pRateCtrl->pictureSkip > 1 || pRateCtrl->hrd > 1) {
        APITRACEERR("VCEncSetRateCtrl: ERROR Invalid enable/disable value\n");
        return VCENC_INVALID_ARGUMENT;
    }

    /* Check the fillData when hrd off */
    if (pRateCtrl->fillerData && (pRateCtrl->hrdCpbSize == 0 || pRateCtrl->pictureRc != 1)) {
        APITRACEERR("VCEncSetRateCtrl: ERROR Invalid cpbSize value or didn't open the RC "
                    "when fillerData is 1\n");
        return VCENC_INVALID_ARGUMENT;
    }

    /* rate control mode has been changed. Only consider change between cbr and vbr. Constrain couple (hrd=1,vbr=0),(hrd=0,vbr=1)*/
    if (rc->hrd != (true_e)pRateCtrl->hrd || rc->vbr != (true_e)pRateCtrl->vbr) {
        rate_control_mode_change = 1;
    }

    if (pRateCtrl->qpHdr > 51 || pRateCtrl->qpMinPB > 51 || pRateCtrl->qpMaxPB > 51 ||
        pRateCtrl->qpMaxPB < pRateCtrl->qpMinPB || pRateCtrl->qpMinI > 51 ||
        pRateCtrl->qpMaxI > 51 || pRateCtrl->qpMaxI < pRateCtrl->qpMinI) {
        APITRACEERR("VCEncSetRateCtrl: ERROR Invalid QP\n");
        return VCENC_INVALID_ARGUMENT;
    }

    if ((u32)(pRateCtrl->intraQpDelta + 51) > 102) {
        APITRACEERR("VCEncSetRateCtrl: ERROR intraQpDelta out of range\n");
        return VCENC_INVALID_ARGUMENT;
    }

    if (pRateCtrl->fixedIntraQp > 51) {
        APITRACEERR("VCEncSetRateCtrl: ERROR fixedIntraQp out of range\n");
        return VCENC_INVALID_ARGUMENT;
    }
    if (pRateCtrl->bitrateWindow < 1 || pRateCtrl->bitrateWindow > 300) {
        APITRACEERR("VCEncSetRateCtrl: ERROR Invalid GOP length\n");
        return VCENC_INVALID_ARGUMENT;
    }
    if (pRateCtrl->monitorFrames < LEAST_MONITOR_FRAME || pRateCtrl->monitorFrames > 120) {
        APITRACEERR("VCEncSetRateCtrl: ERROR Invalid monitorFrames\n");
        return VCENC_INVALID_ARGUMENT;
    }

    if (pRateCtrl->blockRCSize > 2 || pRateCtrl->ctbRc > 3 ||
        (IS_AV1(vcenc_instance->codecFormat) &&
         pRateCtrl->blockRCSize > 0)) //AV1 qpDelta in CTU level
    {
        APITRACEERR("VCEncSetRateCtrl: ERROR Invalid blockRCSize\n");
        return VCENC_INVALID_ARGUMENT;
    }

    /* Frame rate may change. */
    if (pRateCtrl->frameRateDenom == 0 || pRateCtrl->frameRateNum == 0) {
        APITRACEERR("VCEncSetRateCtrl: ERROR Invalid frameRateDenom, frameRateNum\n");
        return VCENC_INVALID_ARGUMENT;
    }

    if (rc->outRateNum != (i32)pRateCtrl->frameRateNum ||
        rc->outRateDenom != (i32)pRateCtrl->frameRateDenom) {
        /*Though only when ((rc->outRateNum/rc->outRateDenom) != (i32)(pRateCtrl->frameRateNum/pRateCtrl->frameRateDenom)) the frame rate is changed,
    we consider all the situation as frame rate is changed, and sps will change.*/
        frame_rate_change = 1;
        rc->outRateNum = pRateCtrl->frameRateNum;
        rc->outRateDenom = pRateCtrl->frameRateDenom;
    }

    /* Bitrate affects only when rate control is enabled */
    if ((pRateCtrl->pictureRc || pRateCtrl->pictureSkip || pRateCtrl->hrd) &&
        (((pRateCtrl->bitPerSecond < 10000) && (rc->outRateNum > rc->outRateDenom)) ||
         ((((pRateCtrl->bitPerSecond * rc->outRateDenom) / rc->outRateNum) < 10000) &&
          (rc->outRateNum < rc->outRateDenom)) ||
         pRateCtrl->bitPerSecond > VCENC_MAX_BITRATE)) {
        APITRACEERR("VCEncSetRateCtrl: ERROR Invalid bitPerSecond\n");
        return VCENC_INVALID_ARGUMENT;
    }

    /* HRD and VBR are conflict */
    if (pRateCtrl->hrd && pRateCtrl->vbr) {
        APITRACEERR("VCEncSetRateCtrl: ERROR HRD and VBR can not be enabled at the same "
                    "time\n");
        return VCENC_INVALID_ARGUMENT;
    }

    if ((pRateCtrl->picQpDeltaMin > -1) || (pRateCtrl->picQpDeltaMin < -10) ||
        (pRateCtrl->picQpDeltaMax < 1) || (pRateCtrl->picQpDeltaMax > 10)) {
        APITRACEERR("VCEncSetRateCtrl: ERROR picQpRange out of range. Min:Max should be in "
                    "[-1,-10]:[1,10]\n");
        return VCENC_INVALID_ARGUMENT;
    }

    if (pRateCtrl->ctbRcRowQpStep < 0) {
        APITRACEERR("VCEncSetRateCtrl: ERROR ctbRowQpStep out of range\n");
        return VCENC_INVALID_ARGUMENT;
    }

    if (pRateCtrl->ctbRc >= 2 && pRateCtrl->crf >= 0) {
        APITRACEERR("VCEncSetRateCtrl: ERROR crf is set with ctbRc>=2\n");
        return VCENC_INVALID_ARGUMENT;
    }

    if (vcenc_instance->pass == 0 && pRateCtrl->crf >= 0) {
        APITRACEERR("VCEncSetRateCtrl: ERROR crf is set when one-pass encoding\n");
        return VCENC_INVALID_ARGUMENT;
    }

    {
        u32 cpbSize = pRateCtrl->hrdCpbSize;
        u32 bps = pRateCtrl->bitPerSecond;
        u32 level = vcenc_instance->levelIdx;
        u32 cpbMaxRate = pRateCtrl->cpbMaxRate;

        /* Limit maximum bitrate based on resolution and frame rate */
        /* Saturates really high settings */
        /* bits per unpacked frame */
        tmp = (vcenc_instance->sps->bit_depth_chroma_minus8 / 2 +
               vcenc_instance->sps->bit_depth_luma_minus8 + 12) *
              vcenc_instance->ctbPerFrame * vcenc_instance->max_cu_size *
              vcenc_instance->max_cu_size;
        rc->i32MaxPicSize = tmp;
        /* bits per second */
        tmp = MIN((((i64)rcCalculate(tmp, rc->outRateNum, rc->outRateDenom)) * 5 / 3), I32_MAX);
        if (bps > (u32)tmp)
            bps = (u32)tmp;

        if (vcenc_instance->bAutoLevel) {
            vcenc_instance->level = rc_recalculate_level(vcenc_instance, cpbSize, bps);
            level = vcenc_instance->levelIdx =
                getLevelIdx(vcenc_instance->codecFormat, vcenc_instance->level);
            if (vcenc_instance->level == -1) {
                return VCENC_INVALID_ARGUMENT;
            }
        }
        /* if HRD is ON we have to obay all its limits */
        if (pRateCtrl->hrd != 0) {
            if (cpbSize == 0)
                cpbSize = getMaxCPBS(vcenc_instance->codecFormat, level, vcenc_instance->profile,
                                     vcenc_instance->tier);
            else if (cpbSize == (u32)(-1))
                cpbSize = bps;
            if (cpbSize > 0x7FFFFFFF)
                cpbSize = 0x7FFFFFFF;

            if (cpbMaxRate == 0)
                cpbMaxRate = bps;

            /* Limit minimum CPB size based on average bits per frame */
            tmp = rcCalculate(bps, rc->outRateDenom, rc->outRateNum);
            cpbSize = MAX(cpbSize, tmp);

            /* cpbSize must be rounded so it is exactly the size written in stream */
            i = 0;
            tmp = cpbSize;
            while (4095 < (tmp >> (4 + i++)))
                ;

            cpbSize = (tmp >> (4 + i)) << (4 + i);

            if (cpbSize > getMaxCPBS(vcenc_instance->codecFormat, level, vcenc_instance->profile,
                                     vcenc_instance->tier)) {
                APITRACEERR("VCEncSetRateCtrl: ERROR. HRD is ON. hrdCpbSize higher than "
                            "maximum allowed for stream level\n");
                return VCENC_INVALID_ARGUMENT;
            }

            if (bps > getMaxBR(vcenc_instance->codecFormat, level, vcenc_instance->profile,
                               vcenc_instance->tier)) {
                APITRACEERR("VCEncSetRateCtrl: ERROR. HRD is ON. bitPerSecond higher than "
                            "maximum allowed for stream level\n");
                return VCENC_INVALID_ARGUMENT;
            }
        }

        if (cpbMaxRate || cpbSize) {
            /* if cpbMaxRate or cpbSize is set, cpbMaxRate can't < bps */
            cpbMaxRate = MAX(bps, cpbMaxRate);

            /* if cpbMaxRate is not 0, cpbSize mustn't be 0 and is set to default 2*bps*/
            if (cpbSize == 0)
                cpbSize = 2 * bps;
        }

        rc->virtualBuffer.bufferSize = cpbSize;
        rc->virtualBuffer.maxBitRate = cpbMaxRate;
        rc->virtualBuffer.bufferRate = cpbMaxRate * rc->outRateDenom / rc->outRateNum;

        if (cpbMaxRate > bps)
            rc->cbr_flag = 0;
        else
            rc->cbr_flag = 1;

        /* Set the parameters to rate control */
        if (pRateCtrl->pictureRc != 0)
            rc->picRc = ENCHW_YES;
        else
            rc->picRc = ENCHW_NO;

        /* CTB_RC */
        rc->rcQpDeltaRange = pRateCtrl->rcQpDeltaRange;
        rc->rcBaseMBComplexity = pRateCtrl->rcBaseMBComplexity;
        rc->picQpDeltaMin = pRateCtrl->picQpDeltaMin;
        rc->picQpDeltaMax = pRateCtrl->picQpDeltaMax;
        rc->ctbRc = pRateCtrl->ctbRc;
        rc->tolCtbRcInter = pRateCtrl->tolCtbRcInter;
        rc->tolCtbRcIntra = pRateCtrl->tolCtbRcIntra;
        /* look-ahead always use fixed-QP; enable ctbRc if cpb is set */
        if (vcenc_instance->pass == 1)
            rc->ctbRc = 0;
        else if (rc->virtualBuffer.bufferSize && ctbRcAllowed &&
                 IS_CTBRC_FOR_BITRATE(rc->ctbRc) == 0 && vcenc_instance->pass != 2) {
            /* With cpb, ctbRc mode 2 is enabled for 1-pass encoding */
            rc->ctbRc |= 2;
            rc->tolCtbRcInter = rc->tolCtbRcIntra = -1.0;
        }

        if (rc->ctbRc) {
            u32 maxCtbRcQpDelta = (regs->asicCfg.ctbRcVersion > 1) ? 51 : 15;

            /* If CTB QP adjustment for Rate Control not supported, just disable it, not return error. */
            if (IS_CTBRC_FOR_BITRATE(rc->ctbRc) && (regs->asicCfg.ctbRcVersion == 0)) {
                CLR_CTBRC_FOR_BITRATE(rc->ctbRc);
                APITRACEERR("VCEncSetRateCtrl: ERROR CTB QP adjustment for Rate Control not "
                            "supported, Disabled it\n");
            }

            if (rc->rcQpDeltaRange > maxCtbRcQpDelta) {
                rc->rcQpDeltaRange = maxCtbRcQpDelta;
                APITRACEERR("VCEncSetRateCtrl: rcQpDeltaRange too big, Clipped it into valid "
                            "range\n");
            }

            if (IS_CTBRC_FOR_BITRATE(rc->ctbRc))
                rc->picRc = ENCHW_YES;

            if (IS_CTBRC_FOR_QUALITY(rc->ctbRc)) {
                vcenc_instance->blockRCSize = pRateCtrl->blockRCSize;
                vcenc_instance->log2_qp_size =
                    MIN(6 - vcenc_instance->blockRCSize, vcenc_instance->log2_qp_size);
            }
        }

        if (pRateCtrl->pictureSkip != 0)
            rc->picSkip = ENCHW_YES;
        else
            rc->picSkip = ENCHW_NO;

        rc->fillerData = pRateCtrl->fillerData;
        if (pRateCtrl->hrd != 0) {
            rc->hrd = ENCHW_YES;
            rc->picRc = ENCHW_YES;
        } else
            rc->hrd = ENCHW_NO;

        rc->vbr = pRateCtrl->vbr ? ENCHW_YES : ENCHW_NO;
        rc->qpHdr = pRateCtrl->qpHdr << QP_FRACTIONAL_BITS;
        rc->qpMinPB = pRateCtrl->qpMinPB << QP_FRACTIONAL_BITS;
        rc->qpMaxPB = pRateCtrl->qpMaxPB << QP_FRACTIONAL_BITS;
        rc->qpMinI = pRateCtrl->qpMinI << QP_FRACTIONAL_BITS;
        rc->qpMaxI = pRateCtrl->qpMaxI << QP_FRACTIONAL_BITS;
        rc->rcMode = pRateCtrl->rcMode;
        prevBitrate = rc->virtualBuffer.bitRate;
        rc->virtualBuffer.bitRate = bps;
        bitrateWindow = rc->bitrateWindow;
        rc->bitrateWindow = pRateCtrl->bitrateWindow;
        rc->maxPicSizeI = MIN(((((i64)rcCalculate(bps, rc->outRateDenom, rc->outRateNum)) / 100) *
                               (100 + pRateCtrl->bitVarRangeI)),
                              rc->i32MaxPicSize);
        rc->maxPicSizeP = MIN(((((i64)rcCalculate(bps, rc->outRateDenom, rc->outRateNum)) / 100) *
                               (100 + pRateCtrl->bitVarRangeP)),
                              rc->i32MaxPicSize);
        rc->maxPicSizeB = MIN(((((i64)rcCalculate(bps, rc->outRateDenom, rc->outRateNum)) / 100) *
                               (100 + pRateCtrl->bitVarRangeB)),
                              rc->i32MaxPicSize);

        rc->minPicSizeI = (i32)(((i64)rcCalculate(bps, rc->outRateDenom, rc->outRateNum)) * 100 /
                                (100 + pRateCtrl->bitVarRangeI));
        rc->minPicSizeP = (i32)(((i64)rcCalculate(bps, rc->outRateDenom, rc->outRateNum)) * 100 /
                                (100 + pRateCtrl->bitVarRangeP));
        rc->minPicSizeB = (i32)(((i64)rcCalculate(bps, rc->outRateDenom, rc->outRateNum)) * 100 /
                                (100 + pRateCtrl->bitVarRangeB));

        rc->tolMovingBitRate = pRateCtrl->tolMovingBitRate;
        rc->f_tolMovingBitRate = (float)rc->tolMovingBitRate;
        rc->monitorFrames = pRateCtrl->monitorFrames;
        rc->u32StaticSceneIbitPercent = pRateCtrl->u32StaticSceneIbitPercent;
        rc->tolCtbRcInter = pRateCtrl->tolCtbRcInter;
        rc->tolCtbRcIntra = pRateCtrl->tolCtbRcIntra;
        rc->ctbRateCtrl.qpStep =
            ((pRateCtrl->ctbRcRowQpStep << CTB_RC_QP_STEP_FIXP) + rc->ctbCols / 2) / rc->ctbCols;
    }

    rc->intraQpDelta = pRateCtrl->intraQpDelta << QP_FRACTIONAL_BITS;
    rc->fixedIntraQp = pRateCtrl->fixedIntraQp << QP_FRACTIONAL_BITS;
    rc->frameQpDelta = 0 << QP_FRACTIONAL_BITS;
    rc->smooth_psnr_in_gop = pRateCtrl->smoothPsnrInGOP;
    rc->longTermQpDelta = pRateCtrl->longTermQpDelta << QP_FRACTIONAL_BITS;

    /*CRF parameters: pb and ip offset is set to default value
    Todo: set it available to user*/
    rc->crf = pRateCtrl->crf;
    //rc->pbOffset = 6.0 * LOG2(1.3f);
    //rc->ipOffset = 6.0 * LOG2(1.4f);
    rc->pbOffset = 6.0 * 0.378512;
    rc->ipOffset = 6.0 * 0.485427;

    /* New parameters checked already so ignore return value.
  * Reset RC bit counters when changing bitrate. */
    (void)VCEncInitRc(rc, (vcenc_instance->encStatus == VCENCSTAT_INIT) ||
                              (rc->virtualBuffer.bitRate != prevBitrate) ||
                              (bitrateWindow != rc->bitrateWindow) || rate_control_mode_change ||
                              frame_rate_change);

    if (vcenc_instance->encStatus <= VCENCSTAT_START_STREAM && vcenc_instance->bInputfileList) {
        memcpy(&vcenc_instance->rateControl_ori, pRateCtrl, sizeof(*pRateCtrl));
    }

    if (vcenc_instance->pass == 2) {
        VCEncRet ret = VCEncSetRateCtrl(vcenc_instance->lookahead.priv_inst, pRateCtrl);
        if (ret != VCENC_OK) {
            APITRACE("VCEncSetRateCtrl: LookaheadSetRateCtrl failed\n");
            return ret;
        }
    }

    vcenc_instance->asic.regs.bRateCtrlUpdate = 1;
    APITRACE("VCEncSetRateCtrl: OK\n");
    return VCENC_OK;
}

/*------------------------------------------------------------------------------

    Function name : VCEncGetRateCtrl
    Description   : Return current rate control parameters

    Return type   : VCEncRet
    Argument      : inst - the instance in use
                    pRateCtrl - place where parameters are returned
------------------------------------------------------------------------------*/
VCEncRet VCEncGetRateCtrl(VCEncInst inst, VCEncRateCtrl *pRateCtrl)
{
    struct vcenc_instance *vcenc_instance = (struct vcenc_instance *)inst;
    vcencRateControl_s *rc;

    APITRACE("VCEncGetRateCtrl#\n");

    /* Check for illegal inputs */
    if ((vcenc_instance == NULL) || (pRateCtrl == NULL)) {
        APITRACEERR("VCEncGetRateCtrl: ERROR Null argument\n");
        return VCENC_NULL_ARGUMENT;
    }

    /* Check for existing instance */
    if (vcenc_instance->inst != vcenc_instance) {
        APITRACEERR("VCEncGetRateCtrl: ERROR Invalid instance\n");
        return VCENC_INSTANCE_ERROR;
    }

    /* Get the values */
    rc = &vcenc_instance->rateControl;

    pRateCtrl->pictureRc = rc->picRc == ENCHW_NO ? 0 : 1;
    /* CTB RC */
    pRateCtrl->ctbRc = rc->ctbRc;
    pRateCtrl->pictureSkip = rc->picSkip == ENCHW_NO ? 0 : 1;
    pRateCtrl->qpHdr = rc->qpHdr >> QP_FRACTIONAL_BITS;
    pRateCtrl->qpMinPB = rc->qpMinPB >> QP_FRACTIONAL_BITS;
    pRateCtrl->qpMaxPB = rc->qpMaxPB >> QP_FRACTIONAL_BITS;
    pRateCtrl->qpMinI = rc->qpMinI >> QP_FRACTIONAL_BITS;
    pRateCtrl->qpMaxI = rc->qpMaxI >> QP_FRACTIONAL_BITS;
    pRateCtrl->bitPerSecond = rc->virtualBuffer.bitRate;
    pRateCtrl->cpbMaxRate = rc->virtualBuffer.maxBitRate;

    /* get the outputRateNum */
    pRateCtrl->frameRateNum = rc->outRateNum;
    pRateCtrl->frameRateDenom = rc->outRateDenom;
    if (rc->virtualBuffer.bitPerPic) {
        pRateCtrl->bitVarRangeI =
            (i32)(((i64)rc->maxPicSizeI) * 100 / rc->virtualBuffer.bitPerPic - 100);
        pRateCtrl->bitVarRangeP =
            (i32)(((i64)rc->maxPicSizeP) * 100 / rc->virtualBuffer.bitPerPic - 100);
        pRateCtrl->bitVarRangeB =
            (i32)(((i64)rc->maxPicSizeB) * 100 / rc->virtualBuffer.bitPerPic - 100);
    } else {
        pRateCtrl->bitVarRangeI = 10000;
        pRateCtrl->bitVarRangeP = 10000;
        pRateCtrl->bitVarRangeB = 10000;
    }
    pRateCtrl->fillerData = rc->fillerData;
    pRateCtrl->hrd = rc->hrd == ENCHW_NO ? 0 : 1;
    pRateCtrl->bitrateWindow = rc->bitrateWindow;
    pRateCtrl->targetPicSize = rc->targetPicSize;
    pRateCtrl->rcMode = rc->rcMode;

    pRateCtrl->hrdCpbSize = (u32)rc->virtualBuffer.bufferSize;
    pRateCtrl->intraQpDelta = rc->intraQpDelta >> QP_FRACTIONAL_BITS;
    pRateCtrl->fixedIntraQp = rc->fixedIntraQp >> QP_FRACTIONAL_BITS;
    pRateCtrl->monitorFrames = rc->monitorFrames;
    pRateCtrl->u32StaticSceneIbitPercent = rc->u32StaticSceneIbitPercent;
    pRateCtrl->tolMovingBitRate = rc->tolMovingBitRate;
    pRateCtrl->tolCtbRcInter = rc->tolCtbRcInter;
    pRateCtrl->tolCtbRcIntra = rc->tolCtbRcIntra;
    pRateCtrl->ctbRcRowQpStep =
        rc->ctbRateCtrl.qpStep
            ? ((rc->ctbRateCtrl.qpStep * rc->ctbCols + (1 << (CTB_RC_QP_STEP_FIXP - 1))) >>
               CTB_RC_QP_STEP_FIXP)
            : 0;
    pRateCtrl->longTermQpDelta = rc->longTermQpDelta >> QP_FRACTIONAL_BITS;

    pRateCtrl->blockRCSize = vcenc_instance->blockRCSize;
    pRateCtrl->rcQpDeltaRange = rc->rcQpDeltaRange;
    pRateCtrl->rcBaseMBComplexity = rc->rcBaseMBComplexity;
    pRateCtrl->picQpDeltaMin = rc->picQpDeltaMin;
    pRateCtrl->picQpDeltaMax = rc->picQpDeltaMax;
    pRateCtrl->vbr = rc->vbr == ENCHW_NO ? 0 : 1;
    pRateCtrl->ctbRcQpDeltaReverse = rc->ctbRcQpDeltaReverse;
    pRateCtrl->crf = rc->crf;

    APITRACE("VCEncGetRateCtrl: OK\n");
    return VCENC_OK;
}

/*------------------------------------------------------------------------------
    Function name   : VSCheckSize
    Description     :
    Return type     : i32
    Argument        : u32 inputWidth
    Argument        : u32 inputHeight
    Argument        : u32 stabilizedWidth
    Argument        : u32 stabilizedHeight
------------------------------------------------------------------------------*/
i32 VSCheckSize(u32 inputWidth, u32 inputHeight, u32 stabilizedWidth, u32 stabilizedHeight)
{
    /* Input picture minimum dimensions */
    if ((inputWidth < 104) || (inputHeight < 104))
        return 1;

    /* Stabilized picture minimum  values */
    if ((stabilizedWidth < 96) || (stabilizedHeight < 96))
        return 1;

    /* Stabilized dimensions multiple of 4 */
    if (((stabilizedWidth & 3) != 0) || ((stabilizedHeight & 3) != 0))
        return 1;

    /* Edge >= 4 pixels, not checked because stabilization can be
     * used without cropping for scene change detection
    if ((inputWidth < (stabilizedWidth + 8)) ||
       (inputHeight < (stabilizedHeight + 8)))
        return 1; */

    return 0;
}

/*------------------------------------------------------------------------------
    Function name   : osd_overlap
    Description     : check osd input overlap
    Return type     : VCEncRet
    Argument        : pPreProcCfg - user provided parameters
    Argument        : i - osd channel to check
------------------------------------------------------------------------------*/
VCEncRet osd_overlap(const VCEncPreProcessingCfg *pPreProcCfg, u8 id, VCEncVideoCodecFormat format)
{
    int i, tmpx, tmpy;
    int blockW = 64;
    int blockH = (format == VCENC_VIDEO_CODEC_H264) ? 16 : 64;
    VCEncOverlayArea area0 = pPreProcCfg->overlayArea[id];
    for (i = 0; i < MAX_OVERLAY_NUM; i++) {
        if (!pPreProcCfg->overlayArea[i].enable || i == id)
            continue;

        VCEncOverlayArea area1 = pPreProcCfg->overlayArea[i];

        if (!((area0.xoffset + area0.cropWidth) <= area1.xoffset ||
              (area0.yoffset + area0.cropHeight) <= area1.yoffset ||
              area0.xoffset >= (area1.xoffset + area1.cropWidth) ||
              area0.yoffset >= (area1.yoffset + area1.cropHeight))) {
            return VCENC_ERROR;
        }

        /* Check not share CTB: avoid loop all ctb */
        if ((area0.xoffset + area0.cropWidth) <= area1.xoffset &&
            (area0.yoffset + area0.cropHeight) <= area1.yoffset) {
            tmpx = ((area0.xoffset + area0.cropWidth - 1) / blockW) * blockW;
            tmpy = ((area0.yoffset + area0.cropHeight - 1) / blockH) * blockH;

            if (tmpx + blockW > area1.xoffset && tmpy + blockH > area1.yoffset) {
                return VCENC_ERROR;
            }
        } else if ((area0.xoffset + area0.cropWidth) <= area1.xoffset &&
                   area0.yoffset >= (area1.yoffset + area1.cropHeight)) {
            tmpx = ((area0.xoffset + area0.cropWidth - 1) / blockW) * blockW;
            tmpy = ((area1.yoffset + area1.cropHeight - 1) / blockH) * blockH;
            if (tmpx + blockW > area1.xoffset && tmpy + blockH > area0.yoffset) {
                return VCENC_ERROR;
            }
        } else if (area0.xoffset >= (area1.xoffset + area1.cropWidth) &&
                   (area0.yoffset + area0.cropHeight) <= area1.yoffset) {
            tmpx = ((area1.xoffset + area1.cropWidth - 1) / blockW) * blockW;
            tmpy = ((area0.yoffset + area0.cropHeight - 1) / blockH) * blockH;
            if (tmpx + blockW > area0.xoffset && tmpy + blockH > area1.yoffset) {
                return VCENC_ERROR;
            }
        } else if (area0.xoffset >= (area1.xoffset + area1.cropWidth) &&
                   area0.yoffset >= (area1.yoffset + area1.cropHeight)) {
            tmpx = ((area1.xoffset + area1.cropWidth - 1) / blockW) * blockW;
            tmpy = ((area1.yoffset + area1.cropHeight - 1) / blockH) * blockH;
            if (tmpx + blockW > area0.xoffset && tmpy + blockH > area0.yoffset) {
                return VCENC_ERROR;
            }
        } else if ((area0.xoffset + area0.cropWidth) <= area1.xoffset) {
            tmpx = ((area0.xoffset + area0.cropWidth - 1) / blockW) * blockW;
            if (tmpx + blockW > area1.xoffset)
                return VCENC_ERROR;
        } else if ((area0.yoffset + area0.cropHeight) <= area1.yoffset) {
            tmpy = ((area0.yoffset + area0.cropHeight - 1) / blockH) * blockH;
            if (tmpy + blockH > area1.yoffset)
                return VCENC_ERROR;
        } else if (area0.xoffset >= (area1.xoffset + area1.cropWidth)) {
            tmpx = ((area1.xoffset + area1.cropWidth - 1) / blockW) * blockW;
            if (tmpx + blockW > area0.xoffset)
                return VCENC_ERROR;
        } else if (area0.yoffset >= (area1.yoffset + area1.cropHeight)) {
            tmpy = ((area1.yoffset + area1.cropHeight - 1) / blockH) * blockH;
            if (tmpy + blockH > area0.yoffset)
                return VCENC_ERROR;
        }
    }

    return VCENC_OK;
}

/*------------------------------------------------------------------------------
    Function name   : VCEncSetPreProcessing
    Description     : Sets the preprocessing parameters
    Return type     : VCEncRet
    Argument        : inst - encoder instance in use
    Argument        : pPreProcCfg - user provided parameters
------------------------------------------------------------------------------*/
VCEncRet VCEncSetPreProcessing(VCEncInst inst, const VCEncPreProcessingCfg *pPreProcCfg)
{
    struct vcenc_instance *vcenc_instance = (struct vcenc_instance *)inst;
    preProcess_s pp_tmp;
    asicData_s *asic = &vcenc_instance->asic;
    u32 client_type;
    u32 tileId;
    u32 origWidth;
    u32 origHeight;
    u32 xOffset;
    u32 yOffset;

    APITRACE("VCEncSetPreProcessing#\n");
    for (tileId = 0; tileId < vcenc_instance->num_tile_columns; tileId++) {
        origWidth =
            (tileId == 0) ? pPreProcCfg->origWidth : pPreProcCfg->tileExtra[tileId - 1].origWidth;
        origHeight =
            (tileId == 0) ? pPreProcCfg->origHeight : pPreProcCfg->tileExtra[tileId - 1].origHeight;
        xOffset = (tileId == 0) ? pPreProcCfg->xOffset : pPreProcCfg->tileExtra[tileId - 1].xOffset;
        yOffset = (tileId == 0) ? pPreProcCfg->yOffset : pPreProcCfg->tileExtra[tileId - 1].yOffset;
        APITRACEPARAM(" %s : %d\n", "origWidth", origWidth);
        APITRACEPARAM(" %s : %d\n", "origHeight", origHeight);
        APITRACEPARAM(" %s : %d\n", "xOffset", xOffset);
        APITRACEPARAM(" %s : %d\n", "yOffset", yOffset);
    }
    APITRACEPARAM(" %s : %d\n", "inputType", pPreProcCfg->inputType);
    APITRACEPARAM(" %s : %d\n", "rotation", pPreProcCfg->rotation);
    APITRACEPARAM(" %s : %d\n", "mirror", pPreProcCfg->mirror);
    APITRACEPARAM(" %s : %d\n", "colorConversion.type", pPreProcCfg->colorConversion.type);
    APITRACEPARAM(" %s : %d\n", "colorConversion.coeffA", pPreProcCfg->colorConversion.coeffA);
    APITRACEPARAM(" %s : %d\n", "colorConversion.coeffB", pPreProcCfg->colorConversion.coeffB);
    APITRACEPARAM(" %s : %d\n", "colorConversion.coeffC", pPreProcCfg->colorConversion.coeffC);
    APITRACEPARAM(" %s : %d\n", "colorConversion.coeffE", pPreProcCfg->colorConversion.coeffE);
    APITRACEPARAM(" %s : %d\n", "colorConversion.coeffF", pPreProcCfg->colorConversion.coeffF);
    APITRACEPARAM(" %s : %d\n", "scaledWidth", pPreProcCfg->scaledWidth);
    APITRACEPARAM(" %s : %d\n", "scaledHeight", pPreProcCfg->scaledHeight);
    APITRACEPARAM(" %s : %d\n", "scaledOutput", pPreProcCfg->scaledOutput);
    APITRACEPARAM_X(" %s : %p\n", "virtualAddressScaledBuff",
                    pPreProcCfg->virtualAddressScaledBuff);
    APITRACEPARAM(" %s : %d\n", "busAddressScaledBuff", pPreProcCfg->busAddressScaledBuff);
    APITRACEPARAM(" %s : %d\n", "sizeScaledBuff", pPreProcCfg->sizeScaledBuff);
    APITRACEPARAM(" %s : %d\n", "constChromaEn", pPreProcCfg->constChromaEn);
    APITRACEPARAM(" %s : %d\n", "constCb", pPreProcCfg->constCb);
    APITRACEPARAM(" %s : %d\n", "constCr", pPreProcCfg->constCr);

    /* Check for illegal inputs */
    if ((inst == NULL) || (pPreProcCfg == NULL)) {
        APITRACEERR("VCEncSetPreProcessing: ERROR Null argument\n");
        return VCENC_NULL_ARGUMENT;
    }

    /* Check for existing instance */
    if (vcenc_instance->inst != vcenc_instance) {
        APITRACEERR("VCEncSetPreProcessing: ERROR Invalid instance\n");
        return VCENC_INSTANCE_ERROR;
    }

    /* check HW limitations */
    u32 i = 0;
    i32 core_id = -1;

    client_type = VCEncGetClientType(vcenc_instance->codecFormat);
    EWLHwConfig_t cfg = EncAsicGetAsicConfig(client_type, vcenc_instance->ctx);

    if (client_type == EWL_CLIENT_TYPE_H264_ENC && cfg.h264Enabled == EWL_HW_CONFIG_NOT_SUPPORTED)
        return VCENC_INVALID_ARGUMENT;

    if (client_type == EWL_CLIENT_TYPE_HEVC_ENC && cfg.hevcEnabled == EWL_HW_CONFIG_NOT_SUPPORTED)
        return VCENC_INVALID_ARGUMENT;

    if (client_type == EWL_CLIENT_TYPE_AV1_ENC && cfg.av1Enabled == EWL_HW_CONFIG_NOT_SUPPORTED)
        return VCENC_INVALID_ARGUMENT;

    if (client_type == EWL_CLIENT_TYPE_VP9_ENC && cfg.vp9Enabled == EWL_HW_CONFIG_NOT_SUPPORTED)
        return VCENC_INVALID_ARGUMENT;

    if (cfg.rgbEnabled == EWL_HW_CONFIG_NOT_SUPPORTED && pPreProcCfg->inputType >= VCENC_RGB565 &&
        pPreProcCfg->inputType <= VCENC_BGR101010) {
        APITRACEERR("VCEncSetPreProcessing: ERROR RGB input core not supported\n");
        return VCENC_INVALID_ARGUMENT;
    }
    if (cfg.scalingEnabled == EWL_HW_CONFIG_NOT_SUPPORTED && pPreProcCfg->scaledOutput != 0) {
        APITRACEERR("VCEncSetPreProcessing: WARNING Scaling core not supported, disabling "
                    "output\n");
        return VCENC_INVALID_ARGUMENT;
    }
    if (cfg.cscExtendSupport == EWL_HW_CONFIG_NOT_SUPPORTED &&
        (pPreProcCfg->colorConversion.type >= VCENC_RGBTOYUV_BT601_FULL_RANGE) &&
        (pPreProcCfg->colorConversion.type <= VCENC_RGBTOYUV_BT709_FULL_RANGE)) {
        APITRACEERR("VCEncSetPreProcessing: WARNING color conversion extend not supported, "
                    "set to BT601_l.mat\n");
        return VCENC_INVALID_ARGUMENT;
    }

    if ((cfg.NVFormatOnlySupport == EWL_HW_CONFIG_ENABLED) &&
        ((pPreProcCfg->inputType < 1) || (pPreProcCfg->inputType > 2))) {
        APITRACEERR("VCEncSetPreProcessing: ERROR input format only support NV12 or NV21 "
                    "by HW\n");
        return VCENC_INVALID_ARGUMENT;
    }

    if ((cfg.NonRotationSupport == EWL_HW_CONFIG_ENABLED) && (pPreProcCfg->rotation != 0)) {
        APITRACEERR("VCEncSetPreProcessing: ERROR rotation not supported by HW\n");
        return VCENC_INVALID_ARGUMENT;
    }

    if ((pPreProcCfg->colorConversion.type == VCENC_RGBTOYUV_USER_DEFINED) &&
        (cfg.cscExtendSupport == EWL_HW_CONFIG_NOT_SUPPORTED) &&
        ((pPreProcCfg->colorConversion.coeffG != pPreProcCfg->colorConversion.coeffE) ||
         (pPreProcCfg->colorConversion.coeffH != pPreProcCfg->colorConversion.coeffF) ||
         (pPreProcCfg->colorConversion.LumaOffset != 0))) {
        APITRACEERR("VCEncSetPreProcessing: ERROR color conversion extend not supported, "
                    "invalid coefficient\n");
        return VCENC_INVALID_ARGUMENT;
    }

    if (cfg.scaled420Support == EWL_HW_CONFIG_NOT_SUPPORTED && pPreProcCfg->scaledOutput != 0 &&
        pPreProcCfg->scaledOutputFormat == 1) {
        APITRACEERR("VCEncSetPreProcessing: ERROR Scaling to 420SP core not supported\n");
        return VCENC_INVALID_ARGUMENT;
    }

    if (cfg.inLoopDSRatio == EWL_HW_CONFIG_ENABLED && vcenc_instance->pass == 1 &&
        pPreProcCfg->inputType >= VCENC_YUV420_10BIT_PACKED_Y0L2 &&
        pPreProcCfg->inputType < VCENC_YUV420_SEMIPLANAR_8BIT_TILE_4_4 &&
        pPreProcCfg->inputType > VCENC_YUV420_PLANAR_10BIT_P010_TILE_4_4 &&
        pPreProcCfg->inputType <= VCENC_YUV420_VU_10BIT_TILE_96_2) {
        APITRACEERR("VCEncSetPreProcessing: ERROR in-loop down-scaler not supported by "
                    "this format\n");
        return VCENC_INVALID_ARGUMENT;
    }

    if (cfg.scaled420Support == EWL_HW_CONFIG_ENABLED && pPreProcCfg->scaledOutput != 0 &&
        (!((((double)vcenc_instance->width / pPreProcCfg->scaledWidth) >= 1) &&
           (((double)vcenc_instance->width / pPreProcCfg->scaledWidth) <= 32) &&
           (((double)vcenc_instance->height / pPreProcCfg->scaledHeight) >= 1) &&
           (((double)vcenc_instance->height / pPreProcCfg->scaledHeight) <= 32)))) {
        APITRACEERR("VCEncSetPreProcessing: ERROR Scaling ratio not supported\n");
        return VCENC_INVALID_ARGUMENT;
    }

    if (pPreProcCfg->inputType >= VCENC_RGB565 && pPreProcCfg->inputType <= VCENC_BGR101010)
        vcenc_instance->featureToSupport.rgbEnabled = 1;
    else
        vcenc_instance->featureToSupport.rgbEnabled = 0;

    if (pPreProcCfg->scaledOutput != 0)
        vcenc_instance->featureToSupport.scalingEnabled = 1;
    else
        vcenc_instance->featureToSupport.scalingEnabled = 0;

    if (vcenc_instance->pass != 1) {
        /* HW limit: width/height should be 8-pixel aligned for AV1 or VP9+rotation */
        if (IS_AV1(vcenc_instance->codecFormat) && ((vcenc_instance->preProcess.lumWidth & 7) ||
                                                    (vcenc_instance->preProcess.lumHeight & 7))) {
            APITRACEERR("VCEncSetPreProcessing: ERROR support only 8-pixel aligned "
                        "width/height for AV1\n");
            return VCENC_INVALID_ARGUMENT;
        } else if (IS_VP9(vcenc_instance->codecFormat)) {
            true_e picWidthInvalidInVp9 = ((pPreProcCfg->rotation != VCENC_ROTATE_0) ||
                                           (pPreProcCfg->mirror != VCENC_MIRROR_NO)) &&
                                          (0 != (vcenc_instance->preProcess.lumWidth & 7));
            true_e picHeightInvalidInVp9 = (pPreProcCfg->rotation != VCENC_ROTATE_0) &&
                                           (0 != (vcenc_instance->preProcess.lumHeight & 7));
            if (picWidthInvalidInVp9 || picHeightInvalidInVp9) {
                APITRACEERR("VCEncSetPreProcessing: ERROR support only 8-pixel aligned "
                            "width/height for VP9 when mirror or rotation\n");
                return VCENC_INVALID_ARGUMENT;
            }
        }
    }
    for (tileId = 0; tileId < vcenc_instance->num_tile_columns; tileId++) {
        origWidth =
            (tileId == 0) ? pPreProcCfg->origWidth : pPreProcCfg->tileExtra[tileId - 1].origWidth;
        if (origWidth > VCENC_MAX_PP_INPUT_WIDTH) {
            APITRACEERR("VCEncSetPreProcessing: ERROR Input image width is too big\n");
            return VCENC_INVALID_ARGUMENT;
        }
    }
#if 1
    /* Scaled image width limits, multiple of 4 (YUYV) and smaller than input */
    if ((pPreProcCfg->scaledWidth > (u32)vcenc_instance->width) ||
        ((pPreProcCfg->scaledWidth & 0x1) != 0) ||
        (pPreProcCfg->scaledWidth > DOWN_SCALING_MAX_WIDTH)) {
        APITRACEERR("VCEncSetPreProcessing: Invalid scaledWidth\n");
        return VCENC_INVALID_ARGUMENT;
    }

    if (((i32)pPreProcCfg->scaledHeight > vcenc_instance->height) ||
        (pPreProcCfg->scaledHeight & 0x1) != 0) {
        APITRACEERR("VCEncSetPreProcessing: Invalid scaledHeight\n");
        return VCENC_INVALID_ARGUMENT;
    }
#endif
    if (pPreProcCfg->inputType >= VCENC_FORMAT_MAX) {
        APITRACEERR("VCEncSetPreProcessing: ERROR Invalid YUV type\n");
        return VCENC_INVALID_ARGUMENT;
    }
    if ((pPreProcCfg->inputType == VCENC_YUV420_PLANAR_10BIT_I010 ||
         pPreProcCfg->inputType == VCENC_YUV420_PLANAR_10BIT_P010 ||
         pPreProcCfg->inputType == VCENC_YUV420_PLANAR_10BIT_PACKED_PLANAR ||
         pPreProcCfg->inputType == VCENC_YUV420_10BIT_PACKED_Y0L2 ||
         pPreProcCfg->inputType == VCENC_YUV420_PLANAR_10BIT_P010_TILE_4_4 ||
         pPreProcCfg->inputType == VCENC_YUV420_PLANAR_8BIT_TILE_32_32 ||
         pPreProcCfg->inputType == VCENC_YUV420_PLANAR_8BIT_TILE_16_16_PACKED_4) &&
        ((HW_ID_MAJOR_NUMBER(asic->regs.asicHwId) < 4) && HW_PRODUCT_H2(asic->regs.asicHwId))) {
        APITRACEERR("VCEncSetPreProcessing: ERROR Invalid YUV type, HW not support I010 "
                    "format.\n");
        return VCENC_INVALID_ARGUMENT;
    }

    /* Check for special input format support */
    if (asic->regs.asicHwId >= MIN_ASIC_ID_WITH_BUILD_ID) {
        if (!cfg.tile8x8FormatSupport && (pPreProcCfg->inputType == VCENC_YUV420_8BIT_TILE_8_8 ||
                                          pPreProcCfg->inputType == VCENC_YUV420_10BIT_TILE_8_8)) {
            APITRACEERR("VCEncSetPreProcessing: ERROR Invalid YUV type, HW not support "
                        "tile8x8 format.\n");
            return VCENC_INVALID_ARGUMENT;
        }

        if (!cfg.tile4x4FormatSupport &&
            (pPreProcCfg->inputType == VCENC_YUV420_SEMIPLANAR_8BIT_TILE_4_4 ||
             pPreProcCfg->inputType == VCENC_YUV420_SEMIPLANAR_VU_8BIT_TILE_4_4 ||
             pPreProcCfg->inputType == VCENC_YUV420_PLANAR_10BIT_P010_TILE_4_4)) {
            APITRACEERR("VCEncSetPreProcessing: ERROR Invalid YUV type, HW not support "
                        "tile4x4 format.\n");
            return VCENC_INVALID_ARGUMENT;
        }

        if (!cfg.customTileFormatSupport &&
            (pPreProcCfg->inputType == VCENC_YUV420_8BIT_TILE_64_4 ||
             pPreProcCfg->inputType == VCENC_YUV420_UV_8BIT_TILE_64_4 ||
             pPreProcCfg->inputType == VCENC_YUV420_10BIT_TILE_32_4 ||
             pPreProcCfg->inputType == VCENC_YUV420_10BIT_TILE_48_4 ||
             pPreProcCfg->inputType == VCENC_YUV420_VU_10BIT_TILE_48_4 ||
             pPreProcCfg->inputType == VCENC_YUV420_10BIT_TILE_96_2 ||
             pPreProcCfg->inputType == VCENC_YUV420_VU_10BIT_TILE_96_2)) {
            APITRACEERR("VCEncSetPreProcessing: ERROR Invalid YUV type, HW not support "
                        "customized tile format.\n");
            return VCENC_INVALID_ARGUMENT;
        }
    }

    if ((pPreProcCfg->inputType == VCENC_YUV420_UV_8BIT_TILE_64_2 ||
         pPreProcCfg->inputType == VCENC_YUV420_UV_8BIT_TILE_128_2 ||
         pPreProcCfg->inputType == VCENC_YUV420_UV_10BIT_TILE_128_2) &&
        pPreProcCfg->rotation > VCENC_ROTATE_0) {
        APITRACEERR("VCEncSetPreProcessing: ERROR Invalid rotation, tile FBC format does "
                    "not support rotate.\n");
        return VCENC_INVALID_ARGUMENT;
    }

    if (pPreProcCfg->inputType == VCENC_YUV420_UV_8BIT_TILE_64_2 &&
        pPreProcCfg->mirror == VCENC_MIRROR_YES) {
        APITRACEERR("VCEncSetPreProcessing: ERROR Invalid mirror, tile FBC format does not "
                    "support rotate.\n");
        return VCENC_INVALID_ARGUMENT;
    }

    if (pPreProcCfg->rotation > VCENC_ROTATE_180R) {
        APITRACEERR("VCEncSetPreProcessing: ERROR Invalid rotation\n");
        return VCENC_INVALID_ARGUMENT;
    }

    if (pPreProcCfg->mirror > VCENC_MIRROR_YES) {
        APITRACEERR("VCEncSetPreProcessing: ERROR Invalid mirror\n");
        return VCENC_INVALID_ARGUMENT;
    }

    if (vcenc_instance->num_tile_rows * vcenc_instance->num_tile_columns > 1 &&
        pPreProcCfg->rotation > VCENC_ROTATE_0) {
        APITRACEERR("VCEncSetPreProcessing: ERROR Invalid rotation, multi-tile does not "
                    "support rotate.\n");
        return VCENC_INVALID_ARGUMENT;
    }

    if ((vcenc_instance->inputLineBuf.inputLineBufEn) &&
        ((pPreProcCfg->rotation != 0) || (pPreProcCfg->mirror != 0) ||
         (pPreProcCfg->videoStabilization != 0))) {
        APITRACE("VCEncSetPreProcessing: ERROR Cannot do rotation, stabilization, and "
                 "mirror in low latency mode\n");
        return VCENC_INVALID_ARGUMENT;
    }

    for (tileId = 0; tileId < vcenc_instance->num_tile_columns; tileId++) {
        origWidth =
            (tileId == 0) ? pPreProcCfg->origWidth : pPreProcCfg->tileExtra[tileId - 1].origWidth;
        origHeight =
            (tileId == 0) ? pPreProcCfg->origHeight : pPreProcCfg->tileExtra[tileId - 1].origHeight;
        xOffset = (tileId == 0) ? pPreProcCfg->xOffset : pPreProcCfg->tileExtra[tileId - 1].xOffset;
        yOffset = (tileId == 0) ? pPreProcCfg->yOffset : pPreProcCfg->tileExtra[tileId - 1].yOffset;
#if 0
    if (((pPreProcCfg->xOffset & 1) != 0) || ((pPreProcCfg->yOffset & 1) != 0))
    {
      APITRACEERR("VCEncSetPreProcessing: ERROR Invalid offset\n");
      return VCENC_INVALID_ARGUMENT;
    }
#endif

        if (pPreProcCfg->inputType == VCENC_YUV420_SEMIPLANAR_101010 && (xOffset % 6) != 0) {
            APITRACEERR("VCEncSetPreProcessing: ERROR Invalid offset for this format\n");
            return VCENC_INVALID_ARGUMENT;
        }

        if ((pPreProcCfg->inputType == VCENC_YUV420_8BIT_TILE_64_4 ||
             pPreProcCfg->inputType == VCENC_YUV420_UV_8BIT_TILE_64_4) &&
            (((xOffset % 64) != 0) || ((yOffset % 4) != 0))) {
            APITRACEERR("VCEncSetPreProcessing: ERROR Invalid offset for this format\n");
            return VCENC_INVALID_ARGUMENT;
        }

        if (pPreProcCfg->inputType == VCENC_YUV420_10BIT_TILE_32_4 &&
            (((xOffset % 32) != 0) || ((yOffset % 4) != 0))) {
            APITRACEERR("VCEncSetPreProcessing: ERROR Invalid offset for this format\n");
            return VCENC_INVALID_ARGUMENT;
        }

        if ((pPreProcCfg->inputType == VCENC_YUV420_10BIT_TILE_48_4 ||
             pPreProcCfg->inputType == VCENC_YUV420_VU_10BIT_TILE_48_4) &&
            (((xOffset % 48) != 0) || ((yOffset % 4) != 0))) {
            APITRACEERR("VCEncSetPreProcessing: ERROR Invalid offset for this format\n");
            return VCENC_INVALID_ARGUMENT;
        }

        if ((pPreProcCfg->inputType == VCENC_YUV420_8BIT_TILE_128_2 ||
             pPreProcCfg->inputType == VCENC_YUV420_UV_8BIT_TILE_128_2) &&
            ((xOffset % 128) != 0)) {
            APITRACEERR("VCEncSetPreProcessing: ERROR Invalid offset for this format\n");
            return VCENC_INVALID_ARGUMENT;
        }

        if ((pPreProcCfg->inputType == VCENC_YUV420_10BIT_TILE_96_2 ||
             pPreProcCfg->inputType == VCENC_YUV420_VU_10BIT_TILE_96_2) &&
            ((xOffset % 96) != 0)) {
            APITRACEERR("VCEncSetPreProcessing: ERROR Invalid offset for this format\n");
            return VCENC_INVALID_ARGUMENT;
        }
        if (((pPreProcCfg->inputType == VCENC_YUV420_SEMIPLANAR_8BIT_TILE_4_4) ||
             (pPreProcCfg->inputType == VCENC_YUV420_SEMIPLANAR_VU_8BIT_TILE_4_4) ||
             (pPreProcCfg->inputType == VCENC_YUV420_PLANAR_10BIT_P010_TILE_4_4)) &&
            ((origWidth & 0x3) != 0 || (origHeight & 0x3) != 0)) {
            APITRACEERR("VCEncSetPreProcessing: the source size not supported by the 4x4 "
                        "tile format\n");
            return VCENC_INVALID_ARGUMENT;
        }
    }

    if ((pPreProcCfg->inputType >= VCENC_YUV420_SEMIPLANAR_8BIT_TILE_4_4 &&
         pPreProcCfg->inputType <= VCENC_YUV420_VU_10BIT_TILE_96_2) &&
        pPreProcCfg->rotation != 0) {
        APITRACEERR("VCEncSetPreProcessing: rotation not supported by this format\n");
        return VCENC_INVALID_ARGUMENT;
    }

    if ((pPreProcCfg->inputType > VCENC_BGR101010) && pPreProcCfg->videoStabilization != 0) {
        APITRACEERR("VCEncSetPreProcessing: stabilization not supported by this format\n");
        return VCENC_INVALID_ARGUMENT;
    }

    if (pPreProcCfg->constChromaEn) {
        u32 maxCh = (1 << (8 + vcenc_instance->sps->bit_depth_chroma_minus8)) - 1;
        if ((pPreProcCfg->constCb > maxCh) || (pPreProcCfg->constCr > maxCh)) {
            APITRACEERR("VCEncSetPreProcessing: ERROR Invalid constant chroma pixel value\n");
            return VCENC_INVALID_ARGUMENT;
        }
    }

    if ((IS_HEVC(vcenc_instance->codecFormat)) &&
        ((vcenc_instance->Hdr10Display.hdr10_display_enable == (u8)HDR10_CFGED) ||
         (vcenc_instance->Hdr10LightLevel.hdr10_lightlevel_enable == (u8)HDR10_CFGED))) {
        if ((pPreProcCfg->inputType >= VCENC_RGB565) &&
            (pPreProcCfg->inputType <= VCENC_BGR101010)) {
            if (pPreProcCfg->colorConversion.type != VCENC_RGBTOYUV_BT2020) {
                APITRACEERR("VCEncSetPreProcessing: Invalid color conversion in HDR10\n\n");
                return VCENC_INVALID_ARGUMENT;
            }
        }

        if ((vcenc_instance->sps->bit_depth_luma_minus8 != 2) ||
            (vcenc_instance->sps->bit_depth_chroma_minus8 != 2)) {
            APITRACEERR("VCEncSetPreProcessing: Invalid bit depth for main10 profile in "
                        "HDR10\n\n");
            return VCENC_INVALID_ARGUMENT;
        }
    }

    /*Overlay Area check*/
    for (i = 0; i < MAX_OVERLAY_NUM; i++) {
        if (!pPreProcCfg->overlayArea[i].enable)
            continue;
        if (!cfg.OSDSupport) {
            APITRACEERR("VCEncSetPreProcessing: OSD not supported\n\n");
            return VCENC_INVALID_ARGUMENT;
        }
        if (asic->regs.asicHwId >= MIN_ASIC_ID_WITH_BUILD_ID && i >= cfg.maxOsdNum) {
            APITRACEERR("VCEncSetPreProcessing: OSD region exceed supported number\n\n");
            return VCENC_INVALID_ARGUMENT;
        }
        if (vcenc_instance->pass != 1) {
            if (pPreProcCfg->overlayArea[i].xoffset + pPreProcCfg->overlayArea[i].cropWidth >
                    vcenc_instance->width ||
                pPreProcCfg->overlayArea[i].yoffset + pPreProcCfg->overlayArea[i].cropHeight >
                    vcenc_instance->height) {
                APITRACEERR("VCEncSetPreProcessing: ERROR Invalid overlay area\n\n");
                return VCENC_INVALID_ARGUMENT;
            }

            if (pPreProcCfg->overlayArea[i].cropWidth == 0 ||
                pPreProcCfg->overlayArea[i].cropHeight == 0) {
                APITRACEERR("VCEncSetPreProcessing: ERROR Invalid overlay size\n\n");
                return VCENC_INVALID_ARGUMENT;
            }

            if (pPreProcCfg->overlayArea[i].cropXoffset + pPreProcCfg->overlayArea[i].cropWidth >
                    pPreProcCfg->overlayArea[i].width ||
                pPreProcCfg->overlayArea[i].cropYoffset + pPreProcCfg->overlayArea[i].cropHeight >
                    pPreProcCfg->overlayArea[i].height) {
                APITRACEERR("VCEncSetPreProcessing: ERROR Invalid overlay cropping size\n\n");
                return VCENC_INVALID_ARGUMENT;
            }

            if (pPreProcCfg->overlayArea[i].format == 2 &&
                (pPreProcCfg->overlayArea[i].cropWidth % 8 != 0)) {
                APITRACEERR("VCEncSetPreProcessing: ERROR Invalid overlay cropping width for "
                            "bitmap \n\n");
                return VCENC_INVALID_ARGUMENT;
            }

            if (osd_overlap(pPreProcCfg, i, vcenc_instance->codecFormat)) {
                APITRACEERR("VCEncSetPreProcessing: ERROR overlay area overlapping \n\n");
                return VCENC_INVALID_ARGUMENT;
            }

            if (pPreProcCfg->overlayArea[i].superTile != 0 &&
                pPreProcCfg->overlayArea[i].format != 0) {
                APITRACEERR("VCEncSetPreProcessing: ERROR Super tile only support ARGB8888 "
                            "format \n\n");
                return VCENC_INVALID_ARGUMENT;
            }

            if (pPreProcCfg->overlayArea[i].superTile != 0 && i != 0) {
                APITRACEERR("VCEncSetPreProcessing: ERROR Super tile only support for channel "
                            "1 \n\n");
                return VCENC_INVALID_ARGUMENT;
            }

            if ((pPreProcCfg->overlayArea[i].superTile == 0 &&
                 pPreProcCfg->overlayArea[i].scaleWidth != pPreProcCfg->overlayArea[i].cropWidth) ||
                (pPreProcCfg->overlayArea[i].superTile == 0 &&
                 pPreProcCfg->overlayArea[i].scaleHeight !=
                     pPreProcCfg->overlayArea[i].cropHeight) ||
                (pPreProcCfg->overlayArea[i].scaleWidth != pPreProcCfg->overlayArea[i].cropWidth &&
                 i != 0) ||
                (pPreProcCfg->overlayArea[i].scaleHeight !=
                     pPreProcCfg->overlayArea[i].cropHeight &&
                 i != 0)) {
                APITRACEERR("VCEncSetPreProcessing: ERROR Up scale only work for special usage "
                            "\n\n");
                return VCENC_INVALID_ARGUMENT;
            }

            if (pPreProcCfg->overlayArea[i].superTile != 0 &&
                (pPreProcCfg->overlayArea[i].cropXoffset % 64 != 0 ||
                 pPreProcCfg->overlayArea[i].cropYoffset % 64 != 0)) {
                APITRACEERR("VCEncSetPreProcessing: ERROR super tile cropping offset must be "
                            "64 aligned \n\n");
                return VCENC_INVALID_ARGUMENT;
            }

            if (pPreProcCfg->overlayArea[i].superTile != 0 &&
                (pPreProcCfg->overlayArea[i].scaleWidth < pPreProcCfg->overlayArea[i].cropWidth ||
                 pPreProcCfg->overlayArea[i].scaleHeight <
                     pPreProcCfg->overlayArea[i].cropHeight)) {
                APITRACEERR("VCEncSetPreProcessing: ERROR osd only support upscaling but no "
                            "downscaling \n\n");
                return VCENC_INVALID_ARGUMENT;
            }

            if (pPreProcCfg->overlayArea[i].superTile != 0 &&
                ((pPreProcCfg->overlayArea[i].scaleWidth & 1) ||
                 pPreProcCfg->overlayArea[i].scaleWidth >
                     (pPreProcCfg->overlayArea[i].cropWidth * 2) ||
                 (pPreProcCfg->overlayArea[i].scaleHeight & 1) ||
                 pPreProcCfg->overlayArea[i].scaleHeight >
                     (pPreProcCfg->overlayArea[i].cropHeight * 2))) {
                APITRACEERR("VCEncSetPreProcessing: ERROR osd upscaling width or height "
                            "constraint \n\n");
                return VCENC_INVALID_ARGUMENT;
            }

            if (pPreProcCfg->overlayArea[i].superTile != 0 &&
                (pPreProcCfg->overlayArea[i].scaleWidth > vcenc_instance->width ||
                 pPreProcCfg->overlayArea[i].scaleHeight > vcenc_instance->height)) {
                APITRACEERR("VCEncSetPreProcessing: ERROR osd upscaling size has to be less "
                            "than background size \n\n");
                return VCENC_INVALID_ARGUMENT;
            }

            /* Check whether stride is valid */
            u8 strideError = 0;
            switch (pPreProcCfg->overlayArea[i].format) {
            case 0:
                if (pPreProcCfg->overlayArea[i].superTile != 0)
                    strideError = pPreProcCfg->overlayArea[i].Ystride <
                                  STRIDE(pPreProcCfg->overlayArea[i].width, 64) * 64 * 4;
                else
                    strideError =
                        pPreProcCfg->overlayArea[i].Ystride < pPreProcCfg->overlayArea[i].width * 4;
                break;
            case 1:
                strideError =
                    pPreProcCfg->overlayArea[i].Ystride < pPreProcCfg->overlayArea[i].width;
                strideError +=
                    pPreProcCfg->overlayArea[i].UVstride < pPreProcCfg->overlayArea[i].width;
                break;
            case 2:
                strideError =
                    pPreProcCfg->overlayArea[i].Ystride < pPreProcCfg->overlayArea[i].width / 8;
                break;
            }
            if (strideError) {
                APITRACEERR("VCEncSetPreProcessing: ERROR invalid osd stride \n\n");
                return VCENC_INVALID_ARGUMENT;
            }
        }
    }

    /* Mosaic checker */
    for (i = 0; i < MAX_MOSAIC_NUM; i++) {
        if (!pPreProcCfg->mosEnable[i])
            continue;

        int mbSize = IS_H264(vcenc_instance->codecFormat) ? 16 : 64;
        if (pPreProcCfg->mosEnable[i] && !cfg.MosaicSupport) {
            APITRACEERR("VCEncSetPreProcessing: Mosaic not supported\n\n");
            return VCENC_INVALID_ARGUMENT;
        }
        if (asic->regs.asicHwId >= MIN_ASIC_ID_WITH_BUILD_ID && i >= cfg.maxMosaicNum) {
            APITRACEERR("VCEncSetPreProcessing: Mosaic region exceed supported number\n\n");
            return VCENC_INVALID_ARGUMENT;
        }
        u32 alignedPicWidth = STRIDE(vcenc_instance->width, mbSize);
        u32 alignedPicHeight = STRIDE(vcenc_instance->height, mbSize);
        if (pPreProcCfg->mosXoffset[i] + pPreProcCfg->mosWidth[i] > alignedPicWidth ||
            pPreProcCfg->mosYoffset[i] + pPreProcCfg->mosHeight[i] > alignedPicHeight) {
            APITRACEERR("VCEncSetPreProcessing: ERROR Invalid mosaic area\n\n");
            return VCENC_INVALID_ARGUMENT;
        }

        if (pPreProcCfg->mosXoffset[i] % mbSize || pPreProcCfg->mosYoffset[i] % mbSize ||
            pPreProcCfg->mosWidth[i] % mbSize || pPreProcCfg->mosHeight[i] % mbSize) {
            APITRACEERR("VCEncSetPreProcessing: ERROR mosaic region should be ctb/mb "
                        "alligned \n\n");
            return VCENC_INVALID_ARGUMENT;
        }
    }

    pp_tmp = vcenc_instance->preProcess;

    for (tileId = 0; tileId < vcenc_instance->num_tile_columns; tileId++) {
        if (pPreProcCfg->videoStabilization == 0) {
            pp_tmp.horOffsetSrc[tileId] =
                (tileId == 0) ? pPreProcCfg->xOffset : pPreProcCfg->tileExtra[tileId - 1].xOffset;
            pp_tmp.verOffsetSrc[tileId] =
                (tileId == 0) ? pPreProcCfg->yOffset : pPreProcCfg->tileExtra[tileId - 1].yOffset;
        } else {
            pp_tmp.horOffsetSrc[tileId] = 0;
            pp_tmp.verOffsetSrc[tileId] = 0;
        }
        pp_tmp.lumWidthSrc[tileId] =
            (tileId == 0) ? pPreProcCfg->origWidth : pPreProcCfg->tileExtra[tileId - 1].origWidth;
        pp_tmp.lumHeightSrc[tileId] =
            (tileId == 0) ? pPreProcCfg->origHeight : pPreProcCfg->tileExtra[tileId - 1].origHeight;
    }

    pp_tmp.subsample_x = (vcenc_instance->asic.regs.codedChromaIdc == VCENC_CHROMA_IDC_420) ? 1 : 0;
    pp_tmp.subsample_y = (vcenc_instance->asic.regs.codedChromaIdc == VCENC_CHROMA_IDC_420) ? 1 : 0;

    /* Encoded frame resolution as set in Init() */
    //pp_tmp = pEncInst->preProcess;    //here is a bug? if enabling this line.

    cfg = EncAsicGetAsicConfig(client_type, vcenc_instance->ctx);

    pp_tmp.rotation = pPreProcCfg->rotation;
    pp_tmp.mirror = pPreProcCfg->mirror;
    pp_tmp.inputFormat = pPreProcCfg->inputType;
    pp_tmp.videoStab = (pPreProcCfg->videoStabilization != 0) ? 1 : 0;
    pp_tmp.scaledWidth = pPreProcCfg->scaledWidth;
    pp_tmp.scaledHeight = pPreProcCfg->scaledHeight;
    asic->scaledImage.busAddress = pPreProcCfg->busAddressScaledBuff;
    asic->scaledImage.virtualAddress = pPreProcCfg->virtualAddressScaledBuff;
    asic->scaledImage.size = pPreProcCfg->sizeScaledBuff;
    asic->regs.scaledLumBase = asic->scaledImage.busAddress;
    pp_tmp.input_alignment = pPreProcCfg->input_alignment;

    pp_tmp.scaledOutputFormat = pPreProcCfg->scaledOutputFormat;
    pp_tmp.scaledOutput = (pPreProcCfg->scaledOutput) ? 1 : 0;
    if (pPreProcCfg->scaledWidth * pPreProcCfg->scaledHeight == 0 || vcenc_instance->pass == 1)
        pp_tmp.scaledOutput = 0;
    /* Check for invalid values */
    if (EncPreProcessCheck(&pp_tmp, vcenc_instance->num_tile_columns) != ENCHW_OK) {
        APITRACEERR("VCEncSetPreProcessing: ERROR Invalid cropping values\n");
        return VCENC_INVALID_ARGUMENT;
    }
#if 1
    pp_tmp.frameCropping = ENCHW_NO;
    pp_tmp.frameCropLeftOffset = 0;
    pp_tmp.frameCropRightOffset = 0;
    pp_tmp.frameCropTopOffset = 0;
    pp_tmp.frameCropBottomOffset = 0;
    /* Set cropping parameters if required */
    u32 alignment = (IS_H264(vcenc_instance->codecFormat) ? 16 : 8);
    if (vcenc_instance->preProcess.lumWidth % alignment ||
        vcenc_instance->preProcess.lumHeight % alignment) {
        u32 fillRight =
            (vcenc_instance->preProcess.lumWidth + alignment - 1) / alignment * alignment -
            vcenc_instance->preProcess.lumWidth;
        u32 fillBottom =
            (vcenc_instance->preProcess.lumHeight + alignment - 1) / alignment * alignment -
            vcenc_instance->preProcess.lumHeight;

        pp_tmp.frameCropping = ENCHW_YES;

        if (pPreProcCfg->rotation == VCENC_ROTATE_0) /* No rotation */
        {
            if (pp_tmp.mirror)
                pp_tmp.frameCropLeftOffset = fillRight >> pp_tmp.subsample_x;
            else
                pp_tmp.frameCropRightOffset = fillRight >> pp_tmp.subsample_x;

            pp_tmp.frameCropBottomOffset = fillBottom >> pp_tmp.subsample_y;
        } else if (pPreProcCfg->rotation == VCENC_ROTATE_90R) /* Rotate right */
        {
            pp_tmp.frameCropLeftOffset = fillRight >> pp_tmp.subsample_x;

            if (pp_tmp.mirror)
                pp_tmp.frameCropTopOffset = fillBottom >> pp_tmp.subsample_y;
            else
                pp_tmp.frameCropBottomOffset = fillBottom >> pp_tmp.subsample_y;
        } else if (pPreProcCfg->rotation == VCENC_ROTATE_90L) /* Rotate left */
        {
            pp_tmp.frameCropRightOffset = fillRight >> pp_tmp.subsample_x;

            if (pp_tmp.mirror)
                pp_tmp.frameCropBottomOffset = fillBottom >> pp_tmp.subsample_y;
            else
                pp_tmp.frameCropTopOffset = fillBottom >> pp_tmp.subsample_y;
        } else if (pPreProcCfg->rotation == VCENC_ROTATE_180R) /* Rotate 180 degree left */
        {
            if (pp_tmp.mirror)
                pp_tmp.frameCropRightOffset = fillRight >> pp_tmp.subsample_x;
            else
                pp_tmp.frameCropLeftOffset = fillRight >> pp_tmp.subsample_x;

            pp_tmp.frameCropTopOffset = fillBottom >> pp_tmp.subsample_y;
        }
    }
#endif

    if (pp_tmp.videoStab != 0) {
        u32 width = pp_tmp.lumWidth;
        u32 height = pp_tmp.lumHeight;
        u32 heightSrc = pp_tmp.lumHeightSrc[0];

        if (pp_tmp.rotation) {
            u32 tmp;

            tmp = width;
            width = height;
            height = tmp;
        }

        if (VSCheckSize(pp_tmp.lumWidthSrc[0], heightSrc, width, height) != 0) {
            APITRACE("VCEncSetPreProcessing: ERROR Invalid size for stabilization\n");
            return VCENC_INVALID_ARGUMENT;
        }

#ifdef VIDEOSTAB_ENABLED
        VSAlgInit(&vcenc_instance->vsSwData, pp_tmp.lumWidthSrc[0], heightSrc, width, height);

        VSAlgGetResult(&vcenc_instance->vsSwData, &pp_tmp.horOffsetSrc[0], &pp_tmp.verOffsetSrc[0]);
#endif
    }

    pp_tmp.colorConversionType = pPreProcCfg->colorConversion.type;
    pp_tmp.colorConversionCoeffA = pPreProcCfg->colorConversion.coeffA;
    pp_tmp.colorConversionCoeffB = pPreProcCfg->colorConversion.coeffB;
    pp_tmp.colorConversionCoeffC = pPreProcCfg->colorConversion.coeffC;
    pp_tmp.colorConversionCoeffE = pPreProcCfg->colorConversion.coeffE;
    pp_tmp.colorConversionCoeffF = pPreProcCfg->colorConversion.coeffF;
    pp_tmp.colorConversionCoeffG = pPreProcCfg->colorConversion.coeffG;
    pp_tmp.colorConversionCoeffH = pPreProcCfg->colorConversion.coeffH;
    pp_tmp.colorConversionLumaOffset = pPreProcCfg->colorConversion.LumaOffset;
    EncSetColorConversion(&pp_tmp, &vcenc_instance->asic);

    /* constant chroma control */
    pp_tmp.constChromaEn = pPreProcCfg->constChromaEn;
    pp_tmp.constCb = pPreProcCfg->constCb;
    pp_tmp.constCr = pPreProcCfg->constCr;

    /* overlay control*/
    for (i = 0; i < MAX_OVERLAY_NUM; i++) {
        pp_tmp.overlayEnable[i] = pPreProcCfg->overlayArea[i].enable;
        pp_tmp.overlayAlpha[i] = pPreProcCfg->overlayArea[i].alpha;
        pp_tmp.overlayFormat[i] = pPreProcCfg->overlayArea[i].format;
        pp_tmp.overlayHeight[i] = pPreProcCfg->overlayArea[i].height;
        pp_tmp.overlayCropHeight[i] = pPreProcCfg->overlayArea[i].cropHeight;
        pp_tmp.overlayScaleHeight[i] = pPreProcCfg->overlayArea[i].scaleHeight;
        pp_tmp.overlayWidth[i] = pPreProcCfg->overlayArea[i].width;
        pp_tmp.overlayCropWidth[i] = pPreProcCfg->overlayArea[i].cropWidth;
        pp_tmp.overlayScaleWidth[i] = pPreProcCfg->overlayArea[i].scaleWidth;
        pp_tmp.overlayXoffset[i] = pPreProcCfg->overlayArea[i].xoffset;
        pp_tmp.overlayCropXoffset[i] = pPreProcCfg->overlayArea[i].cropXoffset;
        pp_tmp.overlayYoffset[i] = pPreProcCfg->overlayArea[i].yoffset;
        pp_tmp.overlayCropYoffset[i] = pPreProcCfg->overlayArea[i].cropYoffset;
        pp_tmp.overlayYStride[i] = pPreProcCfg->overlayArea[i].Ystride;
        pp_tmp.overlayUVStride[i] = pPreProcCfg->overlayArea[i].UVstride;
        pp_tmp.overlayBitmapY[i] = pPreProcCfg->overlayArea[i].bitmapY;
        pp_tmp.overlayBitmapU[i] = pPreProcCfg->overlayArea[i].bitmapU;
        pp_tmp.overlayBitmapV[i] = pPreProcCfg->overlayArea[i].bitmapV;
        //HEVC handles PRP per frame, no need to do slice
        pp_tmp.overlaySliceHeight[i] = pPreProcCfg->overlayArea[i].cropHeight;
        pp_tmp.overlayVerOffset[i] = pPreProcCfg->overlayArea[i].cropYoffset;
        pp_tmp.overlaySuperTile[i] = pPreProcCfg->overlayArea[i].superTile;
    }

    /* Mosaic control */
    for (i = 0; i < MAX_MOSAIC_NUM; i++) {
        pp_tmp.mosEnable[i] = pPreProcCfg->mosEnable[i];
        pp_tmp.mosXoffset[i] = pPreProcCfg->mosXoffset[i];
        pp_tmp.mosYoffset[i] = pPreProcCfg->mosYoffset[i];
        pp_tmp.mosWidth[i] = pPreProcCfg->mosWidth[i];
        pp_tmp.mosHeight[i] = pPreProcCfg->mosHeight[i];
    }

    pp_tmp.codecFormat = vcenc_instance->codecFormat;

    vcenc_instance->preProcess = pp_tmp;
    if (vcenc_instance->pass == 2) {
        VCEncPreProcessingCfg new_cfg;
        u32 ds_ratio = 1;

        memcpy(&new_cfg, pPreProcCfg, sizeof(VCEncPreProcessingCfg));

        /*pass-1 cutree downscaler*/
        if (vcenc_instance->extDSRatio) {
            ds_ratio = vcenc_instance->extDSRatio + 1;
            for (tileId = 0; tileId < vcenc_instance->num_tile_columns; tileId++) {
                if (tileId == 0) {
                    new_cfg.origWidth /= ds_ratio;
                    new_cfg.origHeight /= ds_ratio;
                    new_cfg.origWidth = (new_cfg.origWidth >> 1) << 1;
                    new_cfg.origHeight = (new_cfg.origHeight >> 1) << 1;
                    new_cfg.xOffset /= ds_ratio;
                    new_cfg.yOffset /= ds_ratio;
                } else {
                    new_cfg.tileExtra[tileId - 1].origWidth /= ds_ratio;
                    new_cfg.tileExtra[tileId - 1].origHeight /= ds_ratio;
                    new_cfg.tileExtra[tileId - 1].origWidth =
                        (new_cfg.tileExtra[tileId - 1].origWidth >> 1) << 1;
                    new_cfg.tileExtra[tileId - 1].origHeight =
                        (new_cfg.tileExtra[tileId - 1].origHeight >> 1) << 1;
                    new_cfg.tileExtra[tileId - 1].xOffset /= ds_ratio;
                    new_cfg.tileExtra[tileId - 1].yOffset /= ds_ratio;
                }
            }
        }

        VCEncRet ret = VCEncSetPreProcessing(vcenc_instance->lookahead.priv_inst, &new_cfg);
        if (ret != VCENC_OK) {
            APITRACE("VCEncSetPreProcessing: LookaheadSetPreProcessing failed\n");
            return ret;
        }
    }

    vcenc_instance->asic.regs.bPreprocessUpdate = 1;
    APITRACE("VCEncSetPreProcessing: OK\n");

    return VCENC_OK;
}

/*------------------------------------------------------------------------------
    Function name   : VCEncGetPreProcessing
    Description     : Returns current preprocessing parameters
    Return type     : VCEncRet
    Argument        : inst - encoder instance
    Argument        : pPreProcCfg - place where the parameters are returned
------------------------------------------------------------------------------*/
VCEncRet VCEncGetPreProcessing(VCEncInst inst, VCEncPreProcessingCfg *pPreProcCfg)
{
    struct vcenc_instance *vcenc_instance = (struct vcenc_instance *)inst;
    preProcess_s *pPP;
    asicData_s *asic = &vcenc_instance->asic;
    i32 i;
    u32 tileId;
    APITRACE("VCEncGetPreProcessing#\n");
    /* Check for illegal inputs */
    if ((inst == NULL) || (pPreProcCfg == NULL)) {
        APITRACEERR("VCEncGetPreProcessing: ERROR Null argument\n");
        return VCENC_NULL_ARGUMENT;
    }

    /* Check for existing instance */
    if (vcenc_instance->inst != vcenc_instance) {
        APITRACEERR("VCEncGetPreProcessing: ERROR Invalid instance\n");
        return VCENC_INSTANCE_ERROR;
    }

    pPP = &vcenc_instance->preProcess;

    for (tileId = 0; tileId < vcenc_instance->num_tile_columns; tileId++) {
        if (tileId == 0) {
            pPreProcCfg->origHeight = pPP->lumHeightSrc[tileId];
            pPreProcCfg->origWidth = pPP->lumWidthSrc[tileId];
            pPreProcCfg->xOffset = pPP->horOffsetSrc[tileId];
            pPreProcCfg->yOffset = pPP->verOffsetSrc[tileId];
        } else {
            pPreProcCfg->tileExtra[tileId - 1].origHeight = pPP->lumHeightSrc[tileId];
            pPreProcCfg->tileExtra[tileId - 1].origWidth = pPP->lumWidthSrc[tileId];
            pPreProcCfg->tileExtra[tileId - 1].xOffset = pPP->horOffsetSrc[tileId];
            pPreProcCfg->tileExtra[tileId - 1].yOffset = pPP->verOffsetSrc[tileId];
        }
    }
    pPreProcCfg->rotation = (VCEncPictureRotation)pPP->rotation;
    pPreProcCfg->mirror = (VCEncPictureMirror)pPP->mirror;
    pPreProcCfg->inputType = (VCEncPictureType)pPP->inputFormat;

    pPreProcCfg->busAddressScaledBuff = asic->scaledImage.busAddress;
    pPreProcCfg->virtualAddressScaledBuff = asic->scaledImage.virtualAddress;
    pPreProcCfg->sizeScaledBuff = asic->scaledImage.size;

    pPreProcCfg->scaledOutput = pPP->scaledOutput;
    pPreProcCfg->scaledWidth = pPP->scaledWidth;
    pPreProcCfg->scaledHeight = pPP->scaledHeight;
    pPreProcCfg->input_alignment = pPP->input_alignment;
    if (vcenc_instance->pass == 2) {
        struct vcenc_instance *vcenc_instance1 =
            (struct vcenc_instance *)vcenc_instance->lookahead.priv_inst;
        pPreProcCfg->inLoopDSRatio = vcenc_instance1->preProcess.inLoopDSRatio;
    } else
        pPreProcCfg->inLoopDSRatio = pPP->inLoopDSRatio;
    pPreProcCfg->videoStabilization = pPP->videoStab;
    pPreProcCfg->scaledOutputFormat = pPP->scaledOutputFormat;

    pPreProcCfg->colorConversion.type = (VCEncColorConversionType)pPP->colorConversionType;
    pPreProcCfg->colorConversion.coeffA = pPP->colorConversionCoeffA;
    pPreProcCfg->colorConversion.coeffB = pPP->colorConversionCoeffB;
    pPreProcCfg->colorConversion.coeffC = pPP->colorConversionCoeffC;
    pPreProcCfg->colorConversion.coeffE = pPP->colorConversionCoeffE;
    pPreProcCfg->colorConversion.coeffF = pPP->colorConversionCoeffF;
    pPreProcCfg->colorConversion.coeffG = pPP->colorConversionCoeffG;
    pPreProcCfg->colorConversion.coeffH = pPP->colorConversionCoeffH;
    pPreProcCfg->colorConversion.LumaOffset = pPP->colorConversionLumaOffset;

    /* constant chroma control */
    pPreProcCfg->constChromaEn = pPP->constChromaEn;
    pPreProcCfg->constCb = pPP->constCb;
    pPreProcCfg->constCr = pPP->constCr;

    /* OSD related parameters */
    for (i = 0; i < MAX_OVERLAY_NUM; i++) {
        pPreProcCfg->overlayArea[i].xoffset = pPP->overlayXoffset[i];
        pPreProcCfg->overlayArea[i].cropXoffset = pPP->overlayCropXoffset[i];
        pPreProcCfg->overlayArea[i].yoffset = pPP->overlayYoffset[i];
        pPreProcCfg->overlayArea[i].cropYoffset = pPP->overlayCropYoffset[i];
        pPreProcCfg->overlayArea[i].width = pPP->overlayWidth[i];
        pPreProcCfg->overlayArea[i].scaleWidth = pPP->overlayScaleWidth[i];
        pPreProcCfg->overlayArea[i].cropWidth = pPP->overlayCropWidth[i];
        pPreProcCfg->overlayArea[i].height = pPP->overlayHeight[i];
        pPreProcCfg->overlayArea[i].cropHeight = pPP->overlayCropHeight[i];
        pPreProcCfg->overlayArea[i].format = pPP->overlayFormat[i];
        pPreProcCfg->overlayArea[i].superTile = pPP->overlaySuperTile[i];
        pPreProcCfg->overlayArea[i].alpha = pPP->overlayAlpha[i];
        pPreProcCfg->overlayArea[i].enable = pPP->overlayEnable[i];
        pPreProcCfg->overlayArea[i].Ystride = pPP->overlayYStride[i];
        pPreProcCfg->overlayArea[i].UVstride = pPP->overlayUVStride[i];
        pPreProcCfg->overlayArea[i].bitmapY = pPP->overlayBitmapY[i];
        pPreProcCfg->overlayArea[i].bitmapU = pPP->overlayBitmapU[i];
        pPreProcCfg->overlayArea[i].bitmapV = pPP->overlayBitmapV[i];
        pPreProcCfg->overlayArea[i].superTile = pPP->overlaySuperTile[i];
        pPreProcCfg->overlayArea[i].scaleWidth = pPP->overlayScaleWidth[i];
        pPreProcCfg->overlayArea[i].scaleHeight = pPP->overlayScaleHeight[i];
    }

    /* Get mosaic region parameters */
    for (i = 0; i < MAX_MOSAIC_NUM; i++) {
        pPreProcCfg->mosEnable[i] = pPP->mosEnable[i];
        pPreProcCfg->mosXoffset[i] = pPP->mosXoffset[i];
        pPreProcCfg->mosYoffset[i] = pPP->mosYoffset[i];
        pPreProcCfg->mosWidth[i] = pPP->mosWidth[i];
        pPreProcCfg->mosHeight[i] = pPP->mosHeight[i];
    }

    APITRACE("VCEncGetPreProcessing: OK\n");
    return VCENC_OK;
}

/*------------------------------------------------------------------------------
    Function name   : VCEncCreateNewPPS
    Description     : create new PPS
    Return type     : VCEncRet
    Argument        : inst - encoder instance in use
    Argument        : pPPSCfg - user provided parameters
    Argument        : newPPSId - new PPS id for user
------------------------------------------------------------------------------*/
VCEncRet VCEncCreateNewPPS(VCEncInst inst, const VCEncPPSCfg *pPPSCfg, i32 *newPPSId)
{
    struct vcenc_instance *vcenc_instance = (struct vcenc_instance *)inst;
    preProcess_s pp_tmp;
    asicData_s *asic = &vcenc_instance->asic;
    struct pps *p;
    struct pps *p0;
    i32 ppsId;

    struct container *c;

    APITRACE("VCEncCreateNewPPS#\n");
    APITRACEPARAM(" %s : %d\n", "chroma_qp_offset", pPPSCfg->chroma_qp_offset);
    APITRACEPARAM(" %s : %d\n", "tc_Offset", pPPSCfg->tc_Offset);
    APITRACEPARAM(" %s : %d\n", "beta_Offset", pPPSCfg->beta_Offset);

    /* Check for illegal inputs */
    if ((inst == NULL) || (pPPSCfg == NULL)) {
        APITRACEERR("VCEncCreateNewPPS: ERROR Null argument\n");
        return VCENC_NULL_ARGUMENT;
    }

    /* Check for existing instance */
    if (vcenc_instance->inst != vcenc_instance) {
        APITRACEERR("VCEncCreateNewPPS: ERROR Invalid instance\n");
        return VCENC_INSTANCE_ERROR;
    }

    if (pPPSCfg->chroma_qp_offset > 12 || pPPSCfg->chroma_qp_offset < -12) {
        APITRACEERR("VCEncCreateNewPPS: ERROR chroma_qp_offset out of range\n");
        return VCENC_INVALID_ARGUMENT;
    }
    if (pPPSCfg->tc_Offset > 6 || pPPSCfg->tc_Offset < -6) {
        APITRACEERR("VCEncCreateNewPPS: ERROR tc_Offset out of range\n");
        return VCENC_INVALID_ARGUMENT;
    }
    if (pPPSCfg->beta_Offset > 6 || pPPSCfg->beta_Offset < -6) {
        APITRACEERR("VCEncCreateNewPPS: ERROR beta_Offset out of range\n");
        return VCENC_INVALID_ARGUMENT;
    }
    c = get_container(vcenc_instance);

    p0 = (struct pps *)get_parameter_set(c, PPS_NUT, 0);
    /*get ppsId*/
    ppsId = 0;
    while (1) {
        p = (struct pps *)get_parameter_set(c, PPS_NUT, ppsId);
        if (p == NULL) {
            break;
        }
        ppsId++;
    }
    *newPPSId = ppsId;

    if (ppsId > 63) {
        APITRACEERR("VCEncCreateNewPPS: ERROR PPS id is greater than 63\n");
        return VCENC_INVALID_ARGUMENT;
    }
    p = (struct pps *)create_parameter_set(PPS_NUT);
    {
        struct ps pstemp = p->ps;
        *p = *p0;
        p->ps = pstemp;
    }
    p->cb_qp_offset = p->cr_qp_offset = pPPSCfg->chroma_qp_offset;

    p->tc_offset = pPPSCfg->tc_Offset * 2;

    p->beta_offset = pPPSCfg->beta_Offset * 2;

    p->ps.id = ppsId;

    queue_put(&c->parameter_set, (struct node *)p);

    vcenc_instance->insertNewPPS = 1;
    vcenc_instance->insertNewPPSId = ppsId;

    vcenc_instance->maxPPSId++;

    /*create new PPS with ppsId*/
    APITRACE("VCEncCreateNewPPS: OK\n");

    return VCENC_OK;
}

/*------------------------------------------------------------------------------
    Function name   : VCEncModifyOldPPS
    Description     : modify old PPS
    Return type     : VCEncRet
    Argument        : inst - encoder instance in use
    Argument        : pPPSCfg - user provided parameters
    Argument        : newPPSId - old PPS id provided by user
------------------------------------------------------------------------------*/
VCEncRet VCEncModifyOldPPS(VCEncInst inst, const VCEncPPSCfg *pPPSCfg, i32 ppsId)
{
    struct vcenc_instance *vcenc_instance = (struct vcenc_instance *)inst;
    preProcess_s pp_tmp;
    asicData_s *asic = &vcenc_instance->asic;
    struct pps *p;
    struct container *c;

    APITRACE("VCEncModifyOldPPS#\n");
    APITRACEPARAM(" %s : %d\n", "chroma_qp_offset", pPPSCfg->chroma_qp_offset);
    APITRACEPARAM(" %s : %d\n", "tc_Offset", pPPSCfg->tc_Offset);
    APITRACEPARAM(" %s : %d\n", "beta_Offset", pPPSCfg->beta_Offset);

    /* Check for illegal inputs */
    if ((inst == NULL) || (pPPSCfg == NULL)) {
        APITRACEERR("VCEncModifyOldPPS: ERROR Null argument\n");
        return VCENC_NULL_ARGUMENT;
    }

    /* Check for existing instance */
    if (vcenc_instance->inst != vcenc_instance) {
        APITRACEERR("VCEncModifyOldPPS: ERROR Invalid instance\n");
        return VCENC_INSTANCE_ERROR;
    }

    if (pPPSCfg->chroma_qp_offset > 12 || pPPSCfg->chroma_qp_offset < -12) {
        APITRACEERR("VCEncModifyOldPPS: ERROR chroma_qp_offset out of range\n");
        return VCENC_INVALID_ARGUMENT;
    }
    if (pPPSCfg->tc_Offset > 6 || pPPSCfg->tc_Offset < -6) {
        APITRACEERR("VCEncModifyOldPPS: ERROR tc_Offset out of range\n");
        return VCENC_INVALID_ARGUMENT;
    }
    if (pPPSCfg->beta_Offset > 6 || pPPSCfg->beta_Offset < -6) {
        APITRACEERR("VCEncModifyOldPPS: ERROR beta_Offset out of range\n");
        return VCENC_INVALID_ARGUMENT;
    }

    if (vcenc_instance->maxPPSId < ppsId || ppsId < 0) {
        APITRACEERR("VCEncModifyOldPPS: ERROR Invalid ppsId\n");
        return VCENC_INVALID_ARGUMENT;
    }

    c = get_container(vcenc_instance);

    p = (struct pps *)get_parameter_set(c, PPS_NUT, ppsId);

    if (p == NULL) {
        APITRACEERR("VCEncModifyOldPPS: ERROR Invalid ppsId\n");
        return VCENC_INVALID_ARGUMENT;
    }

    p->cb_qp_offset = p->cr_qp_offset = pPPSCfg->chroma_qp_offset;

    p->tc_offset = pPPSCfg->tc_Offset * 2;

    p->beta_offset = pPPSCfg->beta_Offset * 2;

    vcenc_instance->insertNewPPS = 1;
    vcenc_instance->insertNewPPSId = ppsId;

    /*create new PPS with ppsId*/
    APITRACE("VCEncModifyOldPPS: OK\n");

    return VCENC_OK;
}

/*------------------------------------------------------------------------------
    Function name   : VCEncGetPPSData
    Description     : get PPS parameters for user
    Return type     : VCEncRet
    Argument        : inst - encoder instance
    Argument        : pPPSCfg - place where the parameters are returned
    Argument        : ppsId - PPS id provided by user
------------------------------------------------------------------------------*/
VCEncRet VCEncGetPPSData(VCEncInst inst, VCEncPPSCfg *pPPSCfg, i32 ppsId)
{
    struct vcenc_instance *vcenc_instance = (struct vcenc_instance *)inst;
    preProcess_s *pPP;
    struct pps *p;
    struct pps *p0;
    struct container *c;

    APITRACE("VCEncGetPPSData#\n");
    /* Check for illegal inputs */
    if ((inst == NULL) || (pPPSCfg == NULL)) {
        APITRACEERR("VCEncGetPPSData: ERROR Null argument\n");
        return VCENC_NULL_ARGUMENT;
    }

    /* Check for existing instance */
    if (vcenc_instance->inst != vcenc_instance) {
        APITRACEERR("VCEncGetPPSData: ERROR Invalid instance\n");
        return VCENC_INSTANCE_ERROR;
    }

    if (vcenc_instance->maxPPSId < ppsId || ppsId < 0) {
        APITRACEERR("VCEncGetPPSData: ERROR Invalid ppsId\n");
        return VCENC_INVALID_ARGUMENT;
    }

    c = get_container(vcenc_instance);

    p = (struct pps *)get_parameter_set(c, PPS_NUT, ppsId);

    if (p == NULL) {
        APITRACEERR("VCEncGetPPSData: ERROR Invalid ppsId\n");
        return VCENC_INVALID_ARGUMENT;
    }

    pPPSCfg->chroma_qp_offset = p->cb_qp_offset;

    pPPSCfg->tc_Offset = p->tc_offset / 2;

    pPPSCfg->beta_Offset = p->beta_offset / 2;

    APITRACE("VCEncGetPPSData: OK\n");
    return VCENC_OK;
}

/*------------------------------------------------------------------------------
    Function name   : VCEncActiveAnotherPPS
    Description     : active another PPS
    Return type     : VCEncRet
    Argument        : inst - encoder instance in use
    Argument        : ppsId - PPS id provided by user
------------------------------------------------------------------------------*/
VCEncRet VCEncActiveAnotherPPS(VCEncInst inst, i32 ppsId)
{
    struct vcenc_instance *vcenc_instance = (struct vcenc_instance *)inst;
    preProcess_s pp_tmp;
    asicData_s *asic = &vcenc_instance->asic;
    struct pps *p;

    struct container *c;

    APITRACE("VCEncActiveAnotherPPS#\n");
    APITRACEPARAM(" %s : %d\n", "ppsId", ppsId);

    /* Check for illegal inputs */
    if (inst == NULL) {
        APITRACEERR("VCEncActiveAnotherPPS: ERROR Null argument\n");
        return VCENC_NULL_ARGUMENT;
    }

    /* Check for existing instance */
    if (vcenc_instance->inst != vcenc_instance) {
        APITRACEERR("VCEncActiveAnotherPPS: ERROR Invalid instance\n");
        return VCENC_INSTANCE_ERROR;
    }

    if (vcenc_instance->maxPPSId < ppsId || ppsId < 0) {
        APITRACEERR("VCEncActiveAnotherPPS: ERROR Invalid ppsId\n");
        return VCENC_INVALID_ARGUMENT;
    }

    c = get_container(vcenc_instance);

    p = (struct pps *)get_parameter_set(c, PPS_NUT, ppsId);

    if (p == NULL) {
        APITRACEERR("VCEncActiveAnotherPPS: ERROR Invalid ppsId\n");
        return VCENC_INVALID_ARGUMENT;
    }

    vcenc_instance->pps_id = ppsId;

    /*create new PPS with ppsId*/
    APITRACE("VCEncActiveAnotherPPS: OK\n");

    return VCENC_OK;
}

/*------------------------------------------------------------------------------
    Function name   : VCEncGetActivePPSId
    Description     : get PPS parameters for user
    Return type     : VCEncRet
    Argument        : inst - encoder instance
    Argument        : ppsId - PPS id provided by user
------------------------------------------------------------------------------*/
VCEncRet VCEncGetActivePPSId(VCEncInst inst, i32 *ppsId)
{
    struct vcenc_instance *vcenc_instance = (struct vcenc_instance *)inst;
    preProcess_s *pPP;
    struct pps *p;
    struct container *c;

    APITRACE("VCEncGetPPSData#\n");
    /* Check for illegal inputs */
    if ((inst == NULL) || (ppsId == NULL)) {
        APITRACEERR("VCEncGetActivePPSId: ERROR Null argument\n");
        return VCENC_NULL_ARGUMENT;
    }

    /* Check for existing instance */
    if (vcenc_instance->inst != vcenc_instance) {
        APITRACEERR("VCEncGetActivePPSId: ERROR Invalid instance\n");
        return VCENC_INSTANCE_ERROR;
    }

    *ppsId = vcenc_instance->pps_id;

    APITRACE("VCEncGetActivePPSId: OK\n");
    return VCENC_OK;
}

/**
 * \addtogroup api_video
 *
 * @{
 */

/**
 * Enables or disables writing user data to the encoded stream. The user data will be written in
 * the stream as a Supplemental Enhancement Information (SEI) message connected to all the
 * following encoded frames. The SEI message payload type is marked as "user_data_unregistered".
 *
 * \param [in] inst The instance that defines the encoder in use.
 * \param [in] pUserData Pointer to a buffer containing the user data. The encoder stores a
 *             pointer to this buffer and reads data from the buffer during encoding of the
 *             following frames. Because the encoder reads data straight from this buffer,
 *             it must not be freed before disabling the user data writing.
 * \param [in] userDataSize  Size of the data in the pUserData buffer in bytes. If a zero value
 *             is given, the user data writing is disabled. Invalid value disables user data
 *             writing. Valid value range: 0 or [16, 2048]
 * \return VCENC_OK setup sucessfully, the user data SEI will be inserted when call
 *             VCEncStrmEncode() or the user data has been disabled;
 * \return VCENC_NULL_ARGUMENT pUserData is invalid;
 * \return VCENC_INSTANCE_ERROR inst is invalid;
 */
VCEncRet VCEncSetSeiUserData(VCEncInst inst, const u8 *pUserData, u32 userDataSize)
{
    struct vcenc_instance *vcenc_instance = (struct vcenc_instance *)inst;

    APITRACE("VCEncSetSeiUserData#\n");
    APITRACEPARAM(" %s : %d\n", "userDataSize", userDataSize);

    /* Check for illegal inputs */
    if ((vcenc_instance == NULL) || (userDataSize != 0 && pUserData == NULL)) {
        APITRACEERR("VCEncSetSeiUserData: ERROR Null argument\n");
        return VCENC_NULL_ARGUMENT;
    }

    /* Check for existing instance */
    if (vcenc_instance->inst != vcenc_instance) {
        APITRACEERR("VCEncSetSeiUserData: ERROR Invalid instance\n");
        return VCENC_INSTANCE_ERROR;
    }

    /* Disable user data */
    if ((userDataSize < 16) || (userDataSize > VCENC_MAX_USER_DATA_SIZE)) {
        vcenc_instance->rateControl.sei.userDataEnabled = ENCHW_NO;
        vcenc_instance->rateControl.sei.pUserData = NULL;
        vcenc_instance->rateControl.sei.userDataSize = 0;
    } else {
        vcenc_instance->rateControl.sei.userDataEnabled = ENCHW_YES;
        vcenc_instance->rateControl.sei.pUserData = pUserData;
        vcenc_instance->rateControl.sei.userDataSize = userDataSize;
    }

    APITRACE("VCEncSetSeiUserData: OK\n");
    return VCENC_OK;
}
/** @} */

#define MAX_TLAYER 7

i32 calculate_gop_reorder_frames(struct sps *sps, VCEncGopPicConfig *pcfg, int gopSize)
{
    for (int i = 0; i < gopSize; i++) {
        int highestDecodingNumberWithLowerPOC = 0;
        for (int j = 0; j < gopSize; j++) {
            if (pcfg[j].poc <= pcfg[i].poc) {
                highestDecodingNumberWithLowerPOC = j;
            }
        }
        int numReorder = 0;
        for (int j = 0; j < highestDecodingNumberWithLowerPOC; j++) {
            if (pcfg[j].temporalId <= pcfg[i].temporalId && pcfg[j].poc > pcfg[i].poc) {
                numReorder++;
            }
        }
        if (numReorder > sps->max_num_reorder_pics[pcfg[i].temporalId]) {
            sps->max_num_reorder_pics[pcfg[i].temporalId] = numReorder;
        }
    }
    return OK;
}

/*------------------------------------------------------------------------------
  vcenc_calculate_num_reorder_frames
------------------------------------------------------------------------------*/
i32 vcenc_calculate_num_reorder_frames(struct vcenc_instance *inst, const VCEncIn *pEncIn)
{
    const VCEncGopConfig *gopCfg = &pEncIn->gopConfig;
    int i, ret;
    int numReorder = 0;

    for (int i = 0; i < inst->sps->max_num_sub_layers; i++) {
        inst->sps->max_num_reorder_pics[i] = 0;
    }
    /* calculate per-gop num reordering frames */
    for (i = 1; i <= MAX_GOP_SIZE; i++) {
        if (gopCfg->gopCfgOffset[i] == 0 && i > 1)
            continue;
        calculate_gop_reorder_frames(inst->sps, gopCfg->pGopPicCfg + gopCfg->gopCfgOffset[i], i);
    }
    /* update max_num_reorder_pics & max_dec_pic_buffering to full layer param */
    for (int i = 1; i < inst->sps->max_num_sub_layers; i++) {
        inst->sps->max_num_reorder_pics[0] =
            MAX(inst->sps->max_num_reorder_pics[0], inst->sps->max_num_reorder_pics[i]);
        if (inst->sps->max_dec_pic_buffering[0] < inst->vps->max_num_reorder_pics[i] + 1)
            inst->sps->max_dec_pic_buffering[0] = inst->vps->max_num_reorder_pics[i] + 1;
    }
    for (int i = 0; i < inst->sps->max_num_sub_layers; i++) {
        inst->vps->max_dec_pic_buffering[i] = inst->sps->max_dec_pic_buffering[i] =
            inst->sps->max_dec_pic_buffering[0];
        inst->vps->max_num_reorder_pics[i] = inst->sps->max_num_reorder_pics[i] =
            inst->sps->max_num_reorder_pics[0];
    }

    return OK;
}

/*------------------------------------------------------------------------------
  parameter_set

  RPS:
  poc[n*2  ] = delta_poc
  poc[n*2+1] = used_by_curr_pic
  poc[n*2  ] = 0, termination
  For example:
  { 0, 0},    No reference pictures
  {-1, 1},    Only one previous pictures
  {-1, 1, -2, 1},   Two previous pictures
  {-1, 1, -2, 0},   Two previous ictures, 2'nd not used
  {-1, 1, -2, 1, 1, 1}, Two previous and one future picture
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------

    Function name : VCEncStrmStart
    Description   : Starts a new stream
    Return type   : VCEncRet
    Argument      : inst - encoder instance
    Argument      : pEncIn - user provided input parameters
                    pEncOut - place where output info is returned
------------------------------------------------------------------------------*/
VCEncRet VCEncStrmStart(VCEncInst inst, const VCEncIn *pEncIn, VCEncOut *pEncOut)
{
    struct vcenc_instance *vcenc_instance = (struct vcenc_instance *)inst;
    struct container *c;
    struct vps *v;
    struct sps *s;
    struct pps *p;
    i32 i, ret = VCENC_ERROR;
    u32 client_type;

    APITRACE("VCEncStrmStart#\n");
    APITRACEPARAM_X(" %s : %p\n", "busLuma", pEncIn->busLuma);
    APITRACEPARAM_X(" %s : %p\n", "busChromaU", pEncIn->busChromaU);
    APITRACEPARAM_X(" %s : %p\n", "busChromaV", pEncIn->busChromaV);
    APITRACEPARAM(" %s : %d\n", "timeIncrement", pEncIn->timeIncrement);
    APITRACEPARAM_X(" %s : %p\n", "pOutBuf", pEncIn->pOutBuf[0]);
    APITRACEPARAM_X(" %s : %p\n", "busOutBuf", pEncIn->busOutBuf[0]);
    APITRACEPARAM(" %s : %d\n", "outBufSize", pEncIn->outBufSize[0]);
    APITRACEPARAM(" %s : %d\n", "codingType", pEncIn->codingType);
    APITRACEPARAM(" %s : %d\n", "poc", pEncIn->poc);
    APITRACEPARAM(" %s : %d\n", "gopSize", pEncIn->gopSize);
    APITRACEPARAM(" %s : %d\n", "gopPicIdx", pEncIn->gopPicIdx);
    APITRACEPARAM_X(" %s : %p\n", "roiMapDeltaQpAddr", pEncIn->roiMapDeltaQpAddr);

    /* Check for illegal inputs */
    if ((vcenc_instance == NULL) || (pEncIn == NULL) || (pEncOut == NULL)) {
        APITRACEERR("VCEncStrmStart: ERROR Null argument\n");
        return VCENC_NULL_ARGUMENT;
    }

    /* Check for existing instance */
    if (vcenc_instance->inst != vcenc_instance) {
        APITRACEERR("VCEncStrmStart: ERROR Invalid instance\n");
        return VCENC_INSTANCE_ERROR;
    }

    /* Check status */
    if ((vcenc_instance->encStatus != VCENCSTAT_INIT) &&
        (vcenc_instance->encStatus != VCENCSTAT_START_FRAME)) {
        APITRACEERR("VCEncStrmStart: ERROR Invalid status\n");
        return VCENC_INVALID_STATUS;
    }

    /* Check output stream buffers */
    if (pEncIn->pOutBuf[0] == NULL) {
        APITRACEERR("VCEncStrmStart: ERROR Invalid output stream buffer\n");
        return VCENC_INVALID_ARGUMENT;
    }

    if (vcenc_instance->streamMultiSegment.streamMultiSegmentMode == 0 &&
        (pEncIn->outBufSize[0] < VCENC_STREAM_MIN_BUF0_SIZE)) {
        APITRACEERR("VCEncStrmStart: ERROR Too small output stream buffer\n");
        return VCENC_INVALID_ARGUMENT;
    }

    /* Check idr_interval and gdrDuration */
    if (pEncIn->gopConfig.idr_interval < vcenc_instance->gdrDuration ||
        (pEncIn->gopConfig.idr_interval == 1 && vcenc_instance->gdrDuration > 0)) {
        APITRACEERR("VCEncStrmStart: ERROR intraPicRate and gdrDuration\n");
        return VCENC_INVALID_ARGUMENT;
    }

    /* Check for invalid input values */
    client_type = VCEncGetClientType(vcenc_instance->codecFormat);
    u32 asicHwId = EncAsicGetAsicHWid(client_type, vcenc_instance->ctx);
    if (((HW_ID_MAJOR_NUMBER(asicHwId) <= 1) && HW_PRODUCT_H2(asicHwId)) &&
        (pEncIn->gopConfig.size > 1)) {
        APITRACEERR("VCEncStrmStart: ERROR Invalid input. gopConfig\n");
        return VCENC_INVALID_ARGUMENT;
    }
    if (!(c = get_container(vcenc_instance)))
        return VCENC_ERROR;

    vcenc_instance->bFlush = HANTRO_FALSE;
    vcenc_instance->nextIdrCnt = 0;

    vcenc_instance->vps_id = 0;
    vcenc_instance->sps_id = 0;
    vcenc_instance->pps_id = 0;

    if (vcenc_instance->pass == 1) {
        vcenc_instance->lookahead.internal_mem.mem.mem_type =
            CPU_WR | VPU_WR | EWL_MEM_TYPE_VPU_WORKING;
        /* if the buffer is valid, no need to malloc it again and it will be released in VCEncRelease */
        if (vcenc_instance->lookahead.internal_mem.mem.busAddress == 0) {
            ret = EWLMallocLinear(vcenc_instance->asic.ewl, pEncIn->outBufSize[0], 0,
                                  &(vcenc_instance->lookahead.internal_mem.mem));
            if (ret != EWL_OK)
                return VCENC_ERROR;
        }
        vcenc_instance->stream.stream =
            (u8 *)vcenc_instance->lookahead.internal_mem.mem.virtualAddress;
        vcenc_instance->lookahead.internal_mem.pOutBuf =
            vcenc_instance->lookahead.internal_mem.mem.virtualAddress;
        vcenc_instance->stream.stream_bus = vcenc_instance->lookahead.internal_mem.busOutBuf =
            vcenc_instance->lookahead.internal_mem.mem.busAddress;
        vcenc_instance->stream.size = vcenc_instance->lookahead.internal_mem.outBufSize =
            vcenc_instance->lookahead.internal_mem.mem.size;
    } else {
        vcenc_instance->stream.stream = (u8 *)pEncIn->pOutBuf[0];
        vcenc_instance->stream.stream_bus = pEncIn->busOutBuf[0];
        vcenc_instance->stream.size = pEncIn->outBufSize[0];
    }

    pEncOut->pNaluSizeBuf = (u32 *)vcenc_instance->asic.sizeTbl[0].virtualAddress;

    hash_init(&vcenc_instance->hashctx, pEncIn->hashType);

    vcenc_instance->temp_size = 1024 * 10;
    vcenc_instance->temp_buffer =
        vcenc_instance->stream.stream + vcenc_instance->stream.size - vcenc_instance->temp_size;

    pEncOut->streamSize = 0;
    pEncOut->sliceHeaderSize = 0;
    pEncOut->numNalus = 0;
    pEncOut->maxSliceStreamSize = 0;

    /* set RPS */
    if (vcenc_ref_pic_sets(vcenc_instance, pEncIn)) {
        Error(2, ERR, "vcenc_ref_pic_sets() fails");
        return NOK;
    }

    v = (struct vps *)get_parameter_set(c, VPS_NUT, 0);
    s = (struct sps *)get_parameter_set(c, SPS_NUT, 0);
    p = (struct pps *)get_parameter_set(c, PPS_NUT, 0);

    /* Initialize parameter sets */
    if (set_parameter(vcenc_instance, pEncIn, v, s, p))
        goto out;

    /* Workaround for temporal nesting flag for P only multi-layer */
    if ((pEncIn->gopConfig.size == 1) && (pEncIn->gopConfig.pGopPicCfg[0].temporalId > 0)) {
        v->temporal_id_nesting_flag = 0;
        s->temporal_id_nesting_flag = 0;
    }

    /* calculate sps->sps_max_num_reorder_pics from pEncIn->gopConfig */
    if (vcenc_calculate_num_reorder_frames(vcenc_instance, pEncIn) != OK) {
        Error(2, ERR, "vcenc_calculate_num_reorder_frames() fails");
        return NOK;
    }

    /* init VUI */

    VCEncSpsSetVuiAspectRatio(s, vcenc_instance->sarWidth, vcenc_instance->sarHeight);
    VCEncSpsSetVuiVideoInfo(s, vcenc_instance->vuiVideoFullRange);
    if (pEncIn->vui_timing_info_enable) {
        const rcVirtualBuffer_s *vb = &vcenc_instance->rateControl.virtualBuffer;

        VCEncSpsSetVuiTimigInfo(s, vb->timeScale, vb->unitsInTic);
    } else {
        VCEncSpsSetVuiTimigInfo(s, 0, 0);
    }

    if (vcenc_instance->vuiVideoSignalTypePresentFlag)
        VCEncSpsSetVuiSignalType(s, vcenc_instance->vuiVideoSignalTypePresentFlag,
                                 vcenc_instance->vuiVideoFormat, vcenc_instance->vuiVideoFullRange,
                                 vcenc_instance->vuiColorDescription.vuiColorDescripPresentFlag,
                                 vcenc_instance->vuiColorDescription.vuiColorPrimaries,
                                 vcenc_instance->vuiColorDescription.vuiTransferCharacteristics,
                                 vcenc_instance->vuiColorDescription.vuiMatrixCoefficients);

    /* update VUI */
    if (vcenc_instance->rateControl.sei.enabled == ENCHW_YES) {
        VCEncSpsSetVuiPictStructPresentFlag(s, 1);
    }

    VCEncSpsSetVui_field_seq_flag(s, vcenc_instance->preProcess.interlacedFrame);

    if (vcenc_instance->rateControl.hrd == ENCHW_YES) {
        rcVirtualBuffer_s *vb = &vcenc_instance->rateControl.virtualBuffer;

        VCEncSpsSetVuiHrd(s, 1);

        VCEncSpsSetVuiHrdBitRate(s, vb->bitRate);

        VCEncSpsSetVuiHrdCpbSize(s, vb->bufferSize);

        VCEncSpsSetVuiHrdCbrFlag(s, (vb->maxBitRate > vb->bitRate) ? 0 : 1);
    }

    /* Init SEI */
    HevcInitSei(&vcenc_instance->rateControl.sei, s->streamMode == ASIC_VCENC_BYTE_STREAM,
                vcenc_instance->rateControl.hrd, vcenc_instance->rateControl.outRateNum,
                vcenc_instance->rateControl.outRateDenom);

    /* Create parameter set nal units */
    if (IS_HEVC(vcenc_instance->codecFormat)) {
        v->ps.b.stream = vcenc_instance->stream.stream;
        video_parameter_set(v, inst);
        APITRACEPARAM("vps size=%d\n", *v->ps.b.cnt);
        pEncOut->streamSize += *v->ps.b.cnt;
        VCEncAddNaluSize(pEncOut, *v->ps.b.cnt, 0);
        hash(&vcenc_instance->hashctx, vcenc_instance->stream.stream, *v->ps.b.cnt);
        vcenc_instance->stream.stream += *v->ps.b.cnt;
    }

    //H264 SVCT enable: insert H264Scalability SEI
    /* For VC8000E 6.0, need minor version >= 6010 */
    if (IS_H264(vcenc_instance->codecFormat) && s->max_num_sub_layers > 1 &&
        (vcenc_instance->asic.regs.asicHwId >= 0x80006010)) {
        //Code SVCT SEI
        v->ps.b.stream = vcenc_instance->stream.stream;
        struct buffer *b = &v->ps.b;

        H264NalUnitHdr(b, 0, H264_SEI, (v->streamMode == VCENC_BYTE_STREAM) ? ENCHW_YES : ENCHW_NO);
        H264ScalabilityInfoSei(b, s, s->max_num_sub_layers - 1,
                               vcenc_instance->rateControl.outRateNum * 256 /
                                   vcenc_instance->rateControl.outRateDenom);
        rbsp_trailing_bits(b);

        APITRACEPARAM("H264Scalability SEI size=%d\n", *v->ps.b.cnt);
        pEncOut->streamSize += *v->ps.b.cnt;
        VCEncAddNaluSize(pEncOut, *v->ps.b.cnt, 0);
        hash(&vcenc_instance->hashctx, vcenc_instance->stream.stream, *v->ps.b.cnt);
        vcenc_instance->stream.stream += *v->ps.b.cnt;
    }

    if (!IS_HEVC(vcenc_instance->codecFormat))
        cnt_ref_pic_set(c, s);

    if ((IS_HEVC(vcenc_instance->codecFormat) || IS_H264(vcenc_instance->codecFormat))) {
        s->ps.b.stream = vcenc_instance->stream.stream;
        sequence_parameter_set(c, s, inst);
        APITRACEPARAM("sps size=%d\n", *s->ps.b.cnt);
        pEncOut->streamSize += *s->ps.b.cnt;
        VCEncAddNaluSize(pEncOut, *s->ps.b.cnt, 0);
        hash(&vcenc_instance->hashctx, vcenc_instance->stream.stream, *s->ps.b.cnt);
        vcenc_instance->stream.stream += *s->ps.b.cnt;

        p->ps.b.stream = vcenc_instance->stream.stream;
        picture_parameter_set(p, inst);
        //APITRACEPARAM("pps size", *p->ps.b.cnt);
        APITRACEPARAM("pps size=%d\n", *p->ps.b.cnt);
        pEncOut->streamSize += *p->ps.b.cnt;
        VCEncAddNaluSize(pEncOut, *p->ps.b.cnt, 0);
        hash(&vcenc_instance->hashctx, vcenc_instance->stream.stream, *p->ps.b.cnt);
        vcenc_instance->stream.stream += *p->ps.b.cnt;

        if (vcenc_instance->write_once_HDR10 == 1) {
            if ((IS_HEVC(vcenc_instance->codecFormat) || IS_H264(vcenc_instance->codecFormat)) &&
                (vcenc_instance->Hdr10Display.hdr10_display_enable == (u8)HDR10_CFGED ||
                 vcenc_instance->Hdr10LightLevel.hdr10_lightlevel_enable == (u8)HDR10_CFGED)) {
                VCEncEncodeSeiHdr10(vcenc_instance, pEncOut);
                vcenc_instance->Hdr10Display.hdr10_display_enable =
                    vcenc_instance->Hdr10Display.hdr10_display_enable ? ((u8)HDR10_CODED)
                                                                      : ((u8)HDR10_NOCFGED);
                vcenc_instance->Hdr10LightLevel.hdr10_lightlevel_enable =
                    vcenc_instance->Hdr10LightLevel.hdr10_lightlevel_enable ? ((u8)HDR10_CODED)
                                                                            : ((u8)HDR10_NOCFGED);
            }
        }
    }

    /* Status == START_STREAM   Stream started */
    vcenc_instance->encStatus = VCENCSTAT_START_STREAM;

    if (vcenc_instance->rateControl.hrd == ENCHW_YES) {
        /* Update HRD Parameters to RC if needed */
        u32 bitrate = VCEncSpsGetVuiHrdBitRate(s);
        u32 cpbsize = VCEncSpsGetVuiHrdCpbSize(s);

        if ((vcenc_instance->rateControl.virtualBuffer.bitRate != (i32)bitrate) ||
            (vcenc_instance->rateControl.virtualBuffer.bufferSize != (i32)cpbsize)) {
            vcenc_instance->rateControl.virtualBuffer.bitRate = bitrate;
            vcenc_instance->rateControl.virtualBuffer.bufferSize = cpbsize;
            (void)VCEncInitRc(&vcenc_instance->rateControl, 1);
        }
    }
    if (vcenc_instance->pass == 2) {
        VCEncOut encOut;
        VCEncIn encIn;
        memcpy(&encIn, pEncIn, sizeof(VCEncIn));
        if (vcenc_instance->num_tile_columns > 1) {
            encOut.tileExtra = vcenc_instance->EncOut[0].tileExtra;
            if (encOut.tileExtra == NULL)
                goto out;
        } else
            encOut.tileExtra = NULL;
        encIn.gopConfig.pGopPicCfg = pEncIn->gopConfig.pGopPicCfgPass1;
        VCEncRet ret = VCEncStrmStart(vcenc_instance->lookahead.priv_inst, &encIn, &encOut);
        if (ret != VCENC_OK) {
            APITRACEERR("VCEncStrmStart: LookaheadStrmStart failed\n");
            goto out;
        }
        memcpy(&vcenc_instance->lookahead.encIn, &encIn,
               sizeof(VCEncIn)); //copy encIn to pass1
        //init gopsize , 2pass agop init gopsize as 4
        vcenc_instance->lookahead.encIn.gopSize =
            (vcenc_instance->gopSize ? vcenc_instance->gopSize : 4);

        //don't start lookahead thread if it's already running
        if (vcenc_instance->lookahead.tid_lookahead == NULL) {
            ret = StartLookaheadThread(&vcenc_instance->lookahead);
            if (ret != VCENC_OK) {
                APITRACEERR("VCEncStrmStart: StartLookaheadThread failed\n");
                goto out;
            }
        }
        queue_init(&vcenc_instance->extraParaQ); //for extra parameters.
    } else if (vcenc_instance->pass == 0) {
        memset(&vcenc_instance->lastPicCfg, 0, sizeof(vcenc_instance->lastPicCfg));
        //To handle situation that fail to read first frame
        vcenc_instance->lastPicCfg.poc = -1;
        vcenc_instance->enqueueJobNum = 0;
        memcpy(&vcenc_instance->encInFor1pass, pEncIn,
               sizeof(VCEncIn)); //copy encIn to 1pass
        //init gopSize, 1pass agop init gopsize as 1
        vcenc_instance->encInFor1pass.gopSize =
            (vcenc_instance->gopSize ? vcenc_instance->gopSize : 1);
        vcenc_instance->nextGopSize = vcenc_instance->encInFor1pass.gopSize; //init next gop
        InitAgop(&vcenc_instance->agop);                                     //init agop
    }

    if (vcenc_instance->pass != 1) {
        //set consumed buffer
        pEncOut->consumedAddr.inputbufBusAddr = 0;
        pEncOut->consumedAddr.dec400TableBusAddr = 0;
        pEncOut->consumedAddr.roiMapDeltaQpBusAddr = 0;
        pEncOut->consumedAddr.roimapCuCtrlInfoBusAddr = 0;
        memset(pEncOut->consumedAddr.overlayInputBusAddr, 0, MAX_OVERLAY_NUM * sizeof(ptr_t));
        pEncOut->consumedAddr.outbufBusAddr = pEncIn->busOutBuf[0];

        //Init coding ctrl parameter queue
        EncInitCodingCtrlQueue(inst);
    }

#ifdef TEST_DATA
    if ((IS_HEVC(vcenc_instance->codecFormat) || IS_H264(vcenc_instance->codecFormat)) &&
        (vcenc_instance->pass != 1)) {
        Enc_add_stream_header_trace(vcenc_instance->stream.stream - pEncOut->streamSize,
                                    pEncOut->streamSize);
    }
#endif

    APITRACE("VCEncStrmStart: OK\n");
    ret = VCENC_OK;

    vcenc_instance->asic.regs.bStrmStartUpdate = 1;

out:
#ifdef TEST_DATA
    Enc_close_stream_trace();
#endif

    return ret;
}

/*------------------------------------------------------------------------------
  vcenc_ref_pic_sets
------------------------------------------------------------------------------*/
i32 vcenc_replace_rps(struct vcenc_instance *vcenc_instance, const VCEncIn *pEncIn,
                      VCEncGopPicConfig *cfg, i32 rps_id)
{
#define RPS_TEMP_BUF_SIZE (RPS_SET_BUFF_SIZE + 60)
    struct vcenc_buffer *vcenc_buffer;
    i32 *poc;
    i32 ret = OK;
    u8 temp_buf[RPS_TEMP_BUF_SIZE];
    const VCEncGopConfig *gopCfg = NULL;
    vcenc_instance->temp_buffer = temp_buf;
    vcenc_instance->temp_size = RPS_TEMP_BUF_SIZE;
    vcenc_instance->temp_bufferBusAddress = 0;
    if (pEncIn)
        gopCfg = &pEncIn->gopConfig;

    /* Initialize tb->instance->buffer so that we can cast it to vcenc_buffer
   * by calling hevc_set_parameter(), see api.c TODO...*/
    vcenc_instance->rps_id = -1;
    vcenc_set_ref_pic_set(vcenc_instance);
    vcenc_buffer = (struct vcenc_buffer *)vcenc_instance->temp_buffer;
    poc = (i32 *)vcenc_buffer->buffer;

    u32 iRefPic, idx = 0;
    for (iRefPic = 0; iRefPic < cfg->numRefPics; iRefPic++) {
        poc[idx++] = cfg->refPics[iRefPic].ref_pic;
        poc[idx++] = cfg->refPics[iRefPic].used_by_cur;
    }
    if (gopCfg) {
        for (u8 j = 0; j < gopCfg->ltrcnt; j++) {
            poc[idx++] = gopCfg->u32LTR_idx[j];
            poc[idx++] = 0;
        }
    }
    poc[idx] = 0; //end
    vcenc_instance->rps_id = rps_id;
    ret = vcenc_set_ref_pic_set(vcenc_instance);

    vcenc_instance->temp_buffer = NULL;
    vcenc_instance->temp_size = 0;
    return ret;
}

struct sw_picture *get_ref_picture(struct vcenc_instance *vcenc_instance, VCEncGopPicConfig *cfg,
                                   i32 idx, bool bRecovery, bool *error)
{
    struct node *n;
    struct sw_picture *p, *ref = NULL;
    struct container *c = get_container(vcenc_instance);
    i32 curPoc = vcenc_instance->poc;
    i32 deltaPoc = cfg->refPics[idx].ref_pic;
    i32 poc = curPoc + deltaPoc;
    i32 iDelta, i;
    *error = HANTRO_TRUE;
    bRecovery = bRecovery && cfg->refPics[idx].used_by_cur;

    for (n = c->picture.tail; n; n = n->next) {
        p = (struct sw_picture *)n;
        if (p->reference) {
            iDelta = p->poc - curPoc;
            if (p->poc == poc) {
                ref = p;
                *error = HANTRO_FALSE;
                break;
            }
            /*when current reference frame is not match with the default rps, and it can be recovered,
        only if it meets the follow conditions, can be used as a reference candidate:
        1. it is not a long term reference frame
        2. it is the first encoded frame in a gop
        3. it is at the same side with ref in the default rps of current frame
      */
            if (bRecovery && (!p->long_term_flag) && (p->encOrderInGop == 0) &&
                ((deltaPoc * iDelta) > 0)) {
                bool bCand = 1;
                for (i = 0; i < (i32)cfg->numRefPics; i++) {
                    if ((p->poc == (cfg->refPics[i].ref_pic + curPoc)) &&
                        cfg->refPics[i].used_by_cur) {
                        bCand = 0;
                        break;
                    }
                }

                if (!bCand)
                    continue;

                if (!ref)
                    ref = p;
                else {
                    i32 lastDelta = ref->poc - curPoc;
                    //the closest reference candidate will be chose.
                    if (ABS(iDelta) < ABS(lastDelta))
                        ref = p;
                }
            }
        }
    }
    return ref;
}

static i32 check_multi_references(struct vcenc_instance *vcenc_instance, struct sps *s,
                                  VCEncGopPicConfig *pConfig, const VCEncIn *pEncIn)
{
    struct sw_picture *p;
    struct container *c = get_container(vcenc_instance);
    i32 i, j;
    bool error = HANTRO_FALSE;
    VCEncGopPicConfig tmpConfig = *pConfig;
    VCEncGopPicConfig *cfg = &tmpConfig;

    for (i = 0; i < (i32)cfg->numRefPics; i++) {
        bool iErr;
        if (IS_LONG_TERM_REF_DELTAPOC(cfg->refPics[i].ref_pic)) // skip long term reference check
            continue;
        i32 refPoc = vcenc_instance->poc + cfg->refPics[i].ref_pic;
        p = get_ref_picture(vcenc_instance, cfg, i, (pEncIn->gopPicIdx == 0), &iErr);

        if (iErr)
            error = HANTRO_TRUE;
        else
            continue;

        bool remove = HANTRO_FALSE;
        if (!p)
            remove = HANTRO_TRUE;
        else if (p->poc != refPoc) {
            for (j = 0; j < (i32)cfg->numRefPics; j++) {
                cfg->refPics[i].ref_pic = p->poc - vcenc_instance->poc;
                if ((j != i) && (cfg->refPics[j].ref_pic == cfg->refPics[i].ref_pic)) {
                    if (cfg->refPics[i].used_by_cur)
                        cfg->refPics[j].used_by_cur = 1;
                    remove = HANTRO_TRUE;
                    break;
                }
            }
        }

        if (remove) {
            for (j = i; j < (i32)cfg->numRefPics - 1; j++) {
                cfg->refPics[j] = cfg->refPics[j + 1];
            }
            cfg->numRefPics--;
            i--;
        }
    }

    vcenc_instance->RpsInSliceHeader = HANTRO_FALSE;
    if (error) {
        if (cfg->numRefPics == 0) {
            vcenc_instance->rps_id = s->num_short_term_ref_pic_sets;
        } else {
            i32 rps_id = s->num_short_term_ref_pic_sets - 1;
            vcenc_replace_rps(vcenc_instance, pEncIn, cfg, rps_id);
            vcenc_instance->rps_id = rps_id;
            if (!IS_H264(vcenc_instance->codecFormat))
                vcenc_instance->RpsInSliceHeader = HANTRO_TRUE;
        }
    }

    return cfg->numRefPics;
}

static VCEncPictureCodingType check_references(struct vcenc_instance *vcenc_instance,
                                               const VCEncIn *pEncIn, struct sps *s,
                                               VCEncGopPicConfig *cfg,
                                               VCEncPictureCodingType codingType)
{
    u32 i;
    /* check references if exist, if not exist need to write new rps in slice header*/
    if (1) {
        i32 numRef = check_multi_references(vcenc_instance, s, cfg, pEncIn);
        if (numRef == 0)
            codingType = VCENC_INTRA_FRAME;
        else if (numRef == 1)
            codingType = VCENC_PREDICTED_FRAME;
    }
    //  else
    //  {
    //    for (i = 0; i < cfg->numRefPics; i ++)
    //    {
    //      if (cfg->refPics[i].used_by_cur && (vcenc_instance->poc + cfg->refPics[i].ref_pic) < 0)
    //      {
    //        vcenc_instance->rps_id = s->num_short_term_ref_pic_sets - 1; //IPPPPPP
    //        codingType = VCENC_PREDICTED_FRAME;
    //      }
    //    }
    //  }
    return codingType;
}

/*------------------------------------------------------------------------------
select RPS according to coding type and interlace option
--------------------------------------------------------------------------------*/
static VCEncPictureCodingType get_rps_id(struct vcenc_instance *vcenc_instance,
                                         const VCEncIn *pEncIn, struct sps *s, u8 *rpsModify)
{
    VCEncGopConfig *gopCfg = (VCEncGopConfig *)(&(pEncIn->gopConfig));
    VCEncPictureCodingType codingType = pEncIn->codingType;
    i32 bak = vcenc_instance->RpsInSliceHeader;

    if ((vcenc_instance->gdrEnabled == 1) && (vcenc_instance->encStatus == VCENCSTAT_START_FRAME) &&
        (vcenc_instance->gdrFirstIntraFrame == 0)) {
        if (pEncIn->codingType == VCENC_INTRA_FRAME) {
            vcenc_instance->gdrStart++;
            codingType = VCENC_PREDICTED_FRAME;
        }
    }
    //Intra
    if (codingType == VCENC_INTRA_FRAME && vcenc_instance->poc == 0) {
        vcenc_instance->rps_id = s->num_short_term_ref_pic_sets;
    }
    //else if ((codingType == VCENC_INTRA_FRAME) && ((vcenc_instance->u32IsPeriodUsingLTR != 0 && vcenc_instance->u8IdxEncodedAsLTR != 0)
    //    ||  vcenc_instance->poc > 0))
    else if (pEncIn->i8SpecialRpsIdx >= 0) {
        vcenc_instance->rps_id = s->num_short_term_ref_pic_sets - 1 -
                                 pEncIn->gopConfig.special_size + pEncIn->i8SpecialRpsIdx;
    } else {
        vcenc_instance->rps_id = gopCfg->id;
        // check reference validness
        VCEncGopPicConfig *cfg = &(gopCfg->pGopPicCfg[vcenc_instance->rps_id]);
        codingType = check_references(vcenc_instance, pEncIn, s, cfg, codingType);
    }

    *rpsModify = vcenc_instance->RpsInSliceHeader;
    vcenc_instance->RpsInSliceHeader = bak;

    return codingType;
}

/*------------------------------------------------------------------------------
calculate the val of H264 nal_ref_idc
--------------------------------------------------------------------------------*/

static u32 h264_refIdc_val(const VCEncIn *pEncIn, struct sw_picture *pic)
{
    u32 nalRefIdc_2bit = 0;
    u8 idx = pEncIn->gopConfig.id;

    if (pEncIn->codingType == VCENC_INTRA_FRAME) {
        nalRefIdc_2bit = 3;
        return nalRefIdc_2bit;
    }
    for (int i = 1; i < pEncIn->gopConfig.special_size; i++) {
        if (pEncIn->gopConfig.special_size != 0 && (pEncIn->poc >= 0) &&
            (pEncIn->poc % pEncIn->gopConfig.pGopPicSpecialCfg[i].i32Interval == 0)) {
            if (pEncIn->gopConfig.pGopPicSpecialCfg[i].temporalId == -255)
                nalRefIdc_2bit = 2;
            else {
                if (pEncIn->gopConfig.pGopPicSpecialCfg[i].temporalId == 0)
                    nalRefIdc_2bit = pic->nalRefIdc;
                else
                    nalRefIdc_2bit = pEncIn->gopConfig.pGopPicSpecialCfg[i].temporalId;
            }
            return nalRefIdc_2bit;
        }
    }
    if (pEncIn->gopConfig.size != 0) {
        if (pEncIn->gopConfig.pGopPicCfg[idx].temporalId == -255 ||
            pEncIn->gopConfig.pGopPicCfg[idx].temporalId == 0)
            nalRefIdc_2bit = pic->nalRefIdc;
        else
            nalRefIdc_2bit = pEncIn->gopConfig.pGopPicCfg[idx].temporalId;
    }
    return nalRefIdc_2bit;
}

/*------------------------------------------------------------------------------
check if delta_poc is in rps
(0 for unused, 1 for short term ref, 2+LongTermFrameIdx for long term ref)
--------------------------------------------------------------------------------*/
static int h264_ref_in_use(int delta_poc, int poc, struct rps *r, const i32 *long_term_ref_pic_poc)
{
    struct ref_pic *p;
    int i;
    for (i = 0; i < r->num_lt_pics; i++) {
        int id = r->ref_pic_lt[i].delta_poc;
        if (id >= 0 && (poc == long_term_ref_pic_poc[id]) &&
            (INVALITED_POC != long_term_ref_pic_poc[id]))
            return 2 + id;
    }

    for (i = 0; i < r->num_negative_pics; i++) {
        p = &r->ref_pic_s0[i];
        if (delta_poc == p->delta_poc)
            return 1;
    }

    for (i = 0; i < r->num_positive_pics; i++) {
        p = &r->ref_pic_s1[i];
        if (delta_poc == p->delta_poc)
            return 1;
    }
    return 0;
}
/*------------------------------------------------------------------------------
collect unused ref frames for H.264 MMO
--------------------------------------------------------------------------------*/
bool h264_mmo_collect(struct vcenc_instance *vcenc_instance, struct sw_picture *pic,
                      const VCEncIn *pEncIn, struct rps *r_next, int delta_poc)
{
    struct container *c = get_container(vcenc_instance);
    if (!c)
        return HANTRO_FALSE;
    const VCEncGopConfig *gopCfg = (VCEncGopConfig *)(&(pEncIn->gopConfig));

    struct rps *r2 = NULL;
    int id;

    /* get the next coded pic's info */
    int poc2 = vcenc_instance->poc + gopCfg->delta_poc_to_next;
    if (pEncIn->flexRefsEnable) {
        r2 = r_next;
        poc2 = vcenc_instance->poc + delta_poc;
    } else {
        id = gopCfg->id_next;
        if (id == 255)
            id = pic->sps->num_short_term_ref_pic_sets - 1 -
                 ((pEncIn->gopConfig.special_size == 0) ? 1 : pEncIn->gopConfig.special_size);
        if (-1 != pEncIn->i8SpecialRpsIdx_next)
            id = pic->sps->num_short_term_ref_pic_sets - 1 - pEncIn->gopConfig.special_size +
                 pEncIn->i8SpecialRpsIdx_next;
        r2 = (struct rps *)get_parameter_set(c, RPS, id);
        if (!r2)
            return HANTRO_FALSE;
    }
    struct rps *r = pic->rps;
    int i, j;

    if (pic->sliceInst->type == I_SLICE) {
        pic->nalRefIdc = 1;

        if (1 == vcenc_instance->layerInRefIdc)
            pic->nalRefIdc_2bit = h264_refIdc_val(pEncIn, pic);
        else
            pic->nalRefIdc_2bit = pic->nalRefIdc;

        pic->markCurrentLongTerm = (pEncIn->u8IdxEncodedAsLTR > 0 ? 1 : 0);
        pic->curLongTermIdx = (pic->markCurrentLongTerm ? (pEncIn->u8IdxEncodedAsLTR - 1) : -1);

        if (vcenc_instance->poc == 0) {
            vcenc_instance->h264_mmo_nops = 0;
            return HANTRO_TRUE;
        }
    }

    /* marking the pic in the ref list */
    for (i = 0; i < r->before_cnt + r->after_cnt + r->lt_current_cnt; i++) {
        struct sw_picture *ref = pic->rpl[0][i];
        int inuse =
            h264_ref_in_use(ref->poc - poc2, ref->poc, r2,
                            pEncIn->long_term_ref_pic); /* whether used as ref of next pic */
        if (inuse == 0) {
            vcenc_instance->h264_mmo_unref[vcenc_instance->h264_mmo_nops] = ref->frameNum;
            vcenc_instance->h264_mmo_unref_ext[vcenc_instance->h264_mmo_nops] = ref->frameNumExt;
            bool is_long =
                (h264_ref_in_use(ref->poc - pic->poc, ref->poc, r, pEncIn->long_term_ref_pic) >= 2);
            vcenc_instance->h264_mmo_long_term_flag[vcenc_instance->h264_mmo_nops] = is_long;
            vcenc_instance->h264_mmo_ltIdx[vcenc_instance->h264_mmo_nops++] =
                -1; // unref short/long
        } else if (inuse > 1 &&
                   h264_ref_in_use(ref->poc - pic->poc, ref->poc, r, pEncIn->long_term_ref_pic) ==
                       1 &&
                   (0 == pEncIn->bIsPeriodUsingLTR)) {
            vcenc_instance->h264_mmo_unref[vcenc_instance->h264_mmo_nops] = ref->frameNum;
            vcenc_instance->h264_mmo_unref_ext[vcenc_instance->h264_mmo_nops] = ref->frameNumExt;
            vcenc_instance->h264_mmo_long_term_flag[vcenc_instance->h264_mmo_nops] = 0;
            vcenc_instance->h264_mmo_ltIdx[vcenc_instance->h264_mmo_nops++] =
                inuse - 2; // str-> ltr
            ref->curLongTermIdx = inuse - 2;
        }
    }

    for (i = 0; i < r->lt_follow_cnt; i++) {
        struct sw_picture *ref = get_picture(c, r->lt_follow[i]);

        int inuse = h264_ref_in_use(ref->poc - poc2, ref->poc, r2, pEncIn->long_term_ref_pic);
        if (inuse == 0) {
            vcenc_instance->h264_mmo_unref[vcenc_instance->h264_mmo_nops] = ref->frameNum;
            vcenc_instance->h264_mmo_unref_ext[vcenc_instance->h264_mmo_nops] = ref->frameNumExt;
            vcenc_instance->h264_mmo_long_term_flag[vcenc_instance->h264_mmo_nops] = 1;
            vcenc_instance->h264_mmo_ltIdx[vcenc_instance->h264_mmo_nops++] = -1; // unref long
        } else if (inuse > 1 &&
                   h264_ref_in_use(ref->poc - pic->poc, ref->poc, r, pEncIn->long_term_ref_pic) ==
                       1 &&
                   (0 == pEncIn->bIsPeriodUsingLTR)) {
            vcenc_instance->h264_mmo_unref[vcenc_instance->h264_mmo_nops] = ref->frameNum;
            vcenc_instance->h264_mmo_unref_ext[vcenc_instance->h264_mmo_nops] = ref->frameNumExt;
            vcenc_instance->h264_mmo_long_term_flag[vcenc_instance->h264_mmo_nops] = 0;
            vcenc_instance->h264_mmo_ltIdx[vcenc_instance->h264_mmo_nops++] =
                inuse - 2; // short -> long
            ref->curLongTermIdx = inuse - 2;
        }
    }

    /* marking cur coded pic */
    i32 long_term_ref_pic_poc_2[VCENC_MAX_LT_REF_FRAMES];
    for (i = 0; i < VCENC_MAX_LT_REF_FRAMES; i++)
        long_term_ref_pic_poc_2[i] = INVALITED_POC;
    for (i = 0; i < pEncIn->gopConfig.ltrcnt; i++) {
        if (HANTRO_TRUE == pEncIn->bLTR_need_update[i])
            long_term_ref_pic_poc_2[i] = pic->poc;
        else
            long_term_ref_pic_poc_2[i] = pEncIn->long_term_ref_pic[i];
    }
    pic->nalRefIdc = h264_ref_in_use(pic->poc - poc2, pic->poc, r2, long_term_ref_pic_poc_2);
    pic->markCurrentLongTerm = 0;
    pic->curLongTermIdx = -1;
    if (pic->nalRefIdc >= 2) {
        pic->markCurrentLongTerm = 1;
        pic->curLongTermIdx = pic->nalRefIdc - 2;
        pic->nalRefIdc = 1;
    }
    if ((0 != pEncIn->bIsPeriodUsingLTR) && (0 != pEncIn->u8IdxEncodedAsLTR)) {
        pic->markCurrentLongTerm = 1;
        pic->curLongTermIdx = pEncIn->u8IdxEncodedAsLTR - 1;
        pic->nalRefIdc = 1;
    }
    if (1 == vcenc_instance->layerInRefIdc) {
        if (1 == pic->nalRefIdc)
            pic->nalRefIdc_2bit = h264_refIdc_val(pEncIn, pic);
        else
            pic->nalRefIdc_2bit = 0;
    } else
        pic->nalRefIdc_2bit = pic->nalRefIdc;
    return HANTRO_TRUE;
}

/*------------------------------------------------------------------------------
mark unused ref frames in lX_used_by_next_pic for H.264 MMO
--------------------------------------------------------------------------------*/
void h264_mmo_mark_unref(regValues_s *regs, int frame_num, int ltrflag, int ltIdx, int cnt[2],
                         struct sw_picture *pic)
{
    int i;

    // search frame_num in current reference list
    for (i = 0; i < pic->sliceInst->active_l0_cnt; i++) {
        if (frame_num == pic->rpl[0][i]->frameNum) {
            regs->l0_used_by_next_pic[i] = 0;
            regs->l0_long_term_flag[i] = ltrflag;
            regs->l0_referenceLongTermIdx[i] = ltIdx;
            return;
        }
    }

    if (pic->sliceInst->type == B_SLICE) {
        for (i = 0; i < pic->sliceInst->active_l1_cnt; i++) {
            if (frame_num == pic->rpl[1][i]->frameNum) {
                regs->l1_used_by_next_pic[i] = 0;
                regs->l1_long_term_flag[i] = ltrflag;
                regs->l1_referenceLongTermIdx[i] = ltIdx;
                return;
            }
        }
    }

    // insert in free slot
    ASSERT(cnt[0] + cnt[1] < 4);
    if (cnt[0] < 2) {
        i = cnt[0]++;
        regs->l0_delta_framenum[i] = pic->frameNum - frame_num;
        regs->l0_used_by_next_pic[i] = 0;
        regs->l0_long_term_flag[i] = ltrflag;
        regs->l0_referenceLongTermIdx[i] = ltIdx;
    } else {
        i = cnt[1]++;
        regs->l1_delta_framenum[i] = pic->frameNum - frame_num;
        regs->l1_used_by_next_pic[i] = 0;
        regs->l1_long_term_flag[i] = ltrflag;
        regs->l1_referenceLongTermIdx[i] = ltIdx;
    }
}

/*------------------------------------------------------------------------------
mark unused ref frames using frame_num given from app
--------------------------------------------------------------------------------*/
void h264_mmo_mark_unref_ext(regValues_s *regs, int frame_num, int ltrflag, int ltIdx, int cnt[2],
                             const VCEncExtParaIn *pEncExtParaIn)
{
    int i;

    // search frame_num in current reference list
    for (i = 0; i < pEncExtParaIn->params.h264Para.num_ref_idx_l0_active_minus1 + 1; i++) {
        if (frame_num == pEncExtParaIn->reflist0[i].frame_num) {
            regs->l0_used_by_next_pic[i] = 0;
            regs->l0_long_term_flag[i] = ltrflag;
            regs->l0_referenceLongTermIdx[i] = ltIdx;
            return;
        }
    }

    if (pEncExtParaIn->params.h264Para.slice_type == SLICE_TYPE_B) {
        for (i = 0; i < pEncExtParaIn->params.h264Para.num_ref_idx_l1_active_minus1 + 1; i++) {
            if (frame_num == pEncExtParaIn->reflist1[i].frame_num) {
                regs->l1_used_by_next_pic[i] = 0;
                regs->l1_long_term_flag[i] = ltrflag;
                regs->l1_referenceLongTermIdx[i] = ltIdx;
                return;
            }
        }
    }

    // insert in free slot
    ASSERT(cnt[0] + cnt[1] < 4);
    if (cnt[0] < 2) {
        i = cnt[0]++;
        regs->l0_delta_framenum[i] = regs->frameNum - frame_num;
        regs->l0_used_by_next_pic[i] = 0;
        regs->l0_long_term_flag[i] = ltrflag;
        regs->l0_referenceLongTermIdx[i] = ltIdx;
    } else {
        i = cnt[1]++;
        regs->l1_delta_framenum[i] = regs->frameNum - frame_num;
        regs->l1_used_by_next_pic[i] = 0;
        regs->l1_long_term_flag[i] = ltrflag;
        regs->l1_referenceLongTermIdx[i] = ltIdx;
    }
} //*/

#ifdef CTBRC_STRENGTH

static i32 float2fixpoint(float data)
{
    i32 i = 0;
    i32 result = 0;
    float pow2 = 2.0;
    /*0.16 format*/
    float base = 0.5;
    for (i = 0; i < 16; i++) {
        result <<= 1;
        if (data >= base) {
            result |= 1;
            data -= base;
        }

        pow2 *= 2;
        base = 1.0 / pow2;
    }
    return result;
}
static i32 float2fixpoint8(float data)
{
    i32 i = 0;
    i32 result = 0;
    float pow2 = 2.0;
    /*0.16 format*/
    float base = 0.5;
    for (i = 0; i < 8; i++) {
        result <<= 1;
        if (data >= base) {
            result |= 1;
            data -= base;
        }

        pow2 *= 2;
        base = 1.0 / pow2;
    }
    return result;
}
#endif

static VCEncRet getCtbRcParamsInMultiTiles(struct vcenc_instance *inst, enum slice_type type)
{
    if (inst == NULL)
        return VCENC_ERROR;

    ctbRateControl_s *pCtbRateCtrl = &(inst->rateControl.ctbRateCtrl);
    asicData_s *asic = &(inst->asic);

    if ((!asic->regs.asicCfg.ctbRcVersion) || (!(asic->regs.rcRoiEnable & 0x08)))
        return VCENC_ERROR;

    i32 ctbRcX0;
    i32 ctbRcX1;
    i32 ctbRcFrameMad;
    i32 ctbRcQpSum;

    ctbRcX0 = ctbRcX1 = ctbRcFrameMad = ctbRcQpSum = 0;
    for (i32 tileId = 0; tileId < inst->num_tile_columns; tileId++) {
        ctbRcX0 += inst->tileCtrl[tileId].ctbRcX0;
        ctbRcX1 += inst->tileCtrl[tileId].ctbRcX1;
        ctbRcFrameMad += inst->tileCtrl[tileId].ctbRcFrameMad;
        ctbRcQpSum += inst->tileCtrl[tileId].ctbRcQpSum;
    }

    pCtbRateCtrl->models[type].x0 = asic->regs.ctbRcModelParam0 = ctbRcX0 / inst->num_tile_columns;
    pCtbRateCtrl->models[type].x1 = asic->regs.ctbRcModelParam1 = ctbRcX1 / inst->num_tile_columns;
    pCtbRateCtrl->models[type].preFrameMad = asic->regs.prevPicLumMad = ctbRcFrameMad;
    pCtbRateCtrl->qpSumForRc = asic->regs.ctbQpSumForRc = ctbRcQpSum;
    pCtbRateCtrl->models[type].started = 1;

    return VCENC_OK;
}

static VCEncRet getCtbRcParams(struct vcenc_instance *inst, enum slice_type type)
{
    if (inst == NULL)
        return VCENC_ERROR;

    ctbRateControl_s *pCtbRateCtrl = &(inst->rateControl.ctbRateCtrl);
    asicData_s *asic = &(inst->asic);

    if ((!asic->regs.asicCfg.ctbRcVersion) || (!(asic->regs.rcRoiEnable & 0x08)))
        return VCENC_ERROR;

    if (inst->tiles_enabled_flag)
        return VCENC_OK;

    pCtbRateCtrl->models[type].x0 = asic->regs.ctbRcModelParam0 =
        EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror, HWIF_ENC_CTB_RC_MODEL_PARAM0);

    pCtbRateCtrl->models[type].x1 = asic->regs.ctbRcModelParam1 =
        EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror, HWIF_ENC_CTB_RC_MODEL_PARAM1);

    pCtbRateCtrl->models[type].preFrameMad = asic->regs.prevPicLumMad =
        EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror, HWIF_ENC_PREV_PIC_LUM_MAD);

    pCtbRateCtrl->qpSumForRc = asic->regs.ctbQpSumForRc =
        EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror, HWIF_ENC_CTB_QP_SUM_FOR_RC) |
        (EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror, HWIF_ENC_CTB_QP_SUM_FOR_RC_MSB)
         << 24);

    pCtbRateCtrl->models[type].started = 1;

    return VCENC_OK;
}

static void onSliceReady(struct vcenc_instance *inst, VCEncSliceReady *slice_callback)
{
    if ((!inst) || (!slice_callback))
        return;

    asicData_s *asic = &inst->asic;
    i32 sliceInc = slice_callback->slicesReady - slice_callback->slicesReadyPrev;
    if (sliceInc > 0) {
        slice_callback->nalUnitInfoNum += sliceInc;
        if (asic->regs.prefixnal_svc_ext)
            slice_callback->nalUnitInfoNum += sliceInc;
    }

    if (inst->sliceReadyCbFunc && (slice_callback->slicesReadyPrev < slice_callback->slicesReady) &&
        (inst->rateControl.hrd == ENCHW_NO)) {
        inst->sliceReadyCbFunc(slice_callback);
    }

    slice_callback->slicesReadyPrev = slice_callback->slicesReady;
    slice_callback->nalUnitInfoNumPrev = slice_callback->nalUnitInfoNum;
}

VCEncRet VCEncStrmWaitReady(VCEncInst inst, const VCEncIn *pEncIn, VCEncOut *pEncOut,
                            struct sw_picture *pic, VCEncSliceReady *slice_callback,
                            struct container *c, u32 waitId)
{
    struct vcenc_instance *vcenc_instance = (struct vcenc_instance *)inst;
    asicData_s *asic = &vcenc_instance->asic;
    struct node *n;
    struct sps *s = pic->sps;

    i32 ret = VCENC_ERROR;
    u32 status = ASIC_STATUS_ERROR;
    int sliceNum;
    i32 i;
    EWLCoreWaitJob_t *out = NULL;
    EWLLinearMem_t *sizeTable = NULL;
    u32 MemSyncOffset = 0;
    u32 MemSyncDataSize = 0;

    do {
        /* Encode one frame */
        i32 ewl_ret = EWL_OK;

        /* Wait for IRQ for every slice or for complete frame */
        if (!asic->regs.bVCMDEnable) {
            if ((out = (EWLCoreWaitJob_t *)EWLDequeueCoreOutJob(asic->ewl, waitId)) == NULL)
                break;

            if ((asic->regs.sliceNum > 1) && vcenc_instance->sliceReadyCbFunc) {
                status = out->out_status;
                slice_callback->slicesReady = out->slices_rdy;
            } else
                status = out->out_status;

            memcpy(asic->regs.regMirror, out->VC8000E_reg, sizeof(u32) * ASIC_SWREG_AMOUNT);
            EWLPutJobtoPool(asic->ewl, (struct node *)out);
        } else {
            ewl_ret = EncWaitCmdbuf(asic->ewl, (u16)waitId, &status);
            EncGetRegsByCmdbuf(asic->ewl, (u16)waitId, asic->regs.regMirror, asic->dumpRegister);
        }

        if (ewl_ret != EWL_OK && asic->regs.bVCMDEnable) {
            status = ASIC_STATUS_ERROR;

            if (ewl_ret == EWL_ERROR) {
                /* IRQ error => Stop and release HW */
                ret = VCENC_SYSTEM_ERROR;
            } else /*if(ewl_ret == EWL_HW_WAIT_TIMEOUT) */
            {
                /* IRQ Timeout => Stop and release HW */
                ret = VCENC_HW_TIMEOUT;
            }
            APITRACEERR("VCEncStrmEncode: ERROR Fatal system error ewl_ret != EWL_OK.\n");
            EncAsicStop(asic->ewl);
            /* Release HW so that it can be used by other codecs */
            EWLReleaseCmdbuf(asic->ewl, (u16)waitId);
        } else {
            /* Check ASIC status bits and possibly release HW */
            if (vcenc_instance->asic.regs.bVCMDEnable)
                status = EncAsicCheckStatus_V2(asic, status);
            else if (!(status & (ASIC_STATUS_SLICE_READY | ASIC_STATUS_LINE_BUFFER_DONE |
                                 ASIC_STATUS_SEGMENT_READY)))
                EncAsicGetRegisters(asic->ewl, &asic->regs, asic->dumpRegister, 0);

#ifdef INTERNAL_TEST
            if (vcenc_instance->cb_try_new_params && vcenc_instance->pass != 1) {
                if (vcenc_instance->testId == TID_RE_ENCODE) {
                    status = VCEncReEncodeTest(vcenc_instance, pic, status);
                }
            }
#endif
            switch (status) {
            case ASIC_STATUS_ERROR:
                APITRACEERR("VCEncStrmEncode: ERROR HW bus access error\n");
                if (vcenc_instance->asic.regs.bVCMDEnable)
                    EWLReleaseCmdbuf(asic->ewl, (u16)waitId);
                ret = VCENC_ERROR;
                break;
            case ASIC_STATUS_FUSE_ERROR:
                if (vcenc_instance->asic.regs.bVCMDEnable)
                    EWLReleaseCmdbuf(asic->ewl, (u16)waitId);
                ret = VCENC_ERROR;
                break;
            case ASIC_STATUS_HW_TIMEOUT:
                APITRACEERR("VCEncStrmEncode: ERROR HW/IRQ timeout\n");
                if (vcenc_instance->asic.regs.bVCMDEnable)
                    EWLReleaseCmdbuf(asic->ewl, (u16)waitId);
                ret = VCENC_HW_TIMEOUT;
                break;
            case ASIC_STATUS_SLICE_READY:
            case ASIC_STATUS_LINE_BUFFER_DONE: /* low latency */
            case (ASIC_STATUS_LINE_BUFFER_DONE | ASIC_STATUS_SLICE_READY):
            case ASIC_STATUS_SEGMENT_READY:
                sizeTable =
                    &asic->sizeTbl[(vcenc_instance->jobCnt + 1) % vcenc_instance->parallelCoreNum];
                MemSyncOffset = pEncOut->PreNaluNum * sizeof(u32);
                MemSyncDataSize = asic->sizeTblSize;
                if (EWLSyncMemData(sizeTable, MemSyncOffset, MemSyncDataSize, DEVICE_TO_HOST) !=
                    EWL_OK) {
                    APITRACEERR("sizeTbl sync memory data failed: ERROR HW error");
                    EWLReleaseHw(asic->ewl);
                    ret = VCENC_ERROR;
                    break;
                }
                if (status & ASIC_STATUS_LINE_BUFFER_DONE) {
                    APITRACE("VCEncStrmEncode: IRQ Line Buffer INT\n");
                    /* clear the interrupt */
                    if (!vcenc_instance->inputLineBuf
                             .inputLineBufHwModeEn) { /* SW handshaking: Software will clear the line buffer interrupt and then update the
               *   line buffer write pointer, when the next line buffer is ready. The encoder will
               *   continue to run when detected the write pointer is updated.  */
                        if (vcenc_instance->inputLineBuf.cbFunc)
                            vcenc_instance->inputLineBuf.cbFunc(
                                vcenc_instance->inputLineBuf.cbData);
                    }
                }
                if (status & ASIC_STATUS_SLICE_READY) {
                    APITRACE("VCEncStrmEncode: IRQ Slice Ready\n");
                    /* Issue callback to application telling how many slices
             * are available. */
                    onSliceReady(vcenc_instance, slice_callback);
                }

                if (status & ASIC_STATUS_SEGMENT_READY) {
                    APITRACE("VCEncStrmEncode: IRQ stream segment INT\n");
                    while (vcenc_instance->streamMultiSegment.streamMultiSegmentMode != 0 &&
                           vcenc_instance->streamMultiSegment.rdCnt <
                               EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror,
                                                       HWIF_ENC_STRM_SEGMENT_WR_PTR)) {
                        if (vcenc_instance->streamMultiSegment.cbFunc)
                            vcenc_instance->streamMultiSegment.cbFunc(
                                vcenc_instance->streamMultiSegment.cbData);
                        /*note: must make sure the data of one segment is read by app then rd counter can increase*/
                        vcenc_instance->streamMultiSegment.rdCnt++;
                    }
                    EncAsicWriteRegisterValue(asic->ewl, asic->regs.regMirror,
                                              HWIF_ENC_STRM_SEGMENT_RD_PTR,
                                              vcenc_instance->streamMultiSegment.rdCnt);
                }
                ret = VCENC_OK;
                break;
            case ASIC_STATUS_BUFF_FULL:
                APITRACEERR("VCEncStrmEncode: ERROR buffer full.\n");
                if (vcenc_instance->asic.regs.bVCMDEnable)
                    EWLReleaseCmdbuf(asic->ewl, (u16)waitId);
                vcenc_instance->output_buffer_over_flow = 1;
                //inst->stream.overflow = ENCHW_YES;
                ret = VCENC_OK;
                break;
            case ASIC_STATUS_HW_RESET:
                APITRACEERR("VCEncStrmEncode: ERROR HW abnormal\n");
                if (vcenc_instance->asic.regs.bVCMDEnable)
                    EWLReleaseCmdbuf(asic->ewl, (u16)waitId);
                ret = VCENC_HW_RESET;
                break;
            case ASIC_STATUS_FRAME_READY:
                APITRACE("VCEncStrmEncode: IRQ Frame Ready\n");
                sizeTable =
                    &asic->sizeTbl[(vcenc_instance->jobCnt + 1) % vcenc_instance->parallelCoreNum];
                MemSyncOffset = pEncOut->PreNaluNum * sizeof(u32);
                MemSyncDataSize = asic->sizeTblSize;
                if (EWLSyncMemData(sizeTable, MemSyncOffset, MemSyncDataSize, DEVICE_TO_HOST) !=
                    EWL_OK) {
                    APITRACEERR("sizeTbl sync memory data failed: ERROR HW error");
                    EWLReleaseHw(asic->ewl);
                    ret = VCENC_ERROR;
                    break;
                }

                /* clear all interrupt */
                if (asic->regs.sliceReadyInterrupt) {
                    //this is used for hardware interrupt.
                    slice_callback->slicesReady = asic->regs.sliceNum;
                    onSliceReady(vcenc_instance, slice_callback);
                }

                asic->regs.frameCodingType = EncAsicGetRegisterValue(
                    asic->ewl, asic->regs.regMirror, HWIF_ENC_FRAME_CODING_TYPE);
                asic->regs.outputStrmSize[0] = EncAsicGetRegisterValue(
                    asic->ewl, asic->regs.regMirror, HWIF_ENC_OUTPUT_STRM_BUFFER_LIMIT);
                vcenc_instance->stream.byteCnt += asic->regs.outputStrmSize[0];
                vcenc_instance->stream.stream += asic->regs.outputStrmSize[0];
                pEncOut->sliceHeaderSize = EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror,
                                                                   HWIF_ENC_SLICE_HEADER_SIZE);

                i32 temporalDelimiterSize = 0;
#ifdef SUPPORT_AV1
                /* fill av1 parameters */
                if (IS_AV1(vcenc_instance->codecFormat)) {
                    vcenc_instance->strmHeaderLen = pic->strmHeaderLen;
                    pEncOut->av1Vp9FrameNotShow = pic->av1Vp9FrameNotShow;
                    vcenc_instance->av1_inst.show_frame = pic->av1_show_frame;
                    temporalDelimiterSize = AV1AddTemporalDelimiter(vcenc_instance);
                }
#endif

                u32 tileIndex = FindIndexBywaitCoreJobid(vcenc_instance, waitId);
                sliceNum = 0;
                u32 *sizeTbl = vcenc_instance->asic.regs.tiles_enabled_flag
                                   ? asic->sizeTbl[tileIndex].virtualAddress
                                   : asic->sizeTbl[(vcenc_instance->jobCnt + 1) %
                                                   vcenc_instance->parallelCoreNum]
                                         .virtualAddress;
                u32 naluIndex = vcenc_instance->asic.regs.tiles_enabled_flag ? tileIndex : 0;
                u32 numNalus =
                    naluIndex == 0 ? pEncOut->numNalus : pEncOut->tileExtra[naluIndex - 1].numNalus;
                if (sizeTbl) {
                    /* if size table wasn't written, set it encoded frame size for later use */
                    if ((EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror,
                                                 HWIF_ENC_NAL_SIZE_WRITE) == 0))
                        sizeTbl[numNalus] = asic->regs.outputStrmSize[0];

                    // for ivf file format, freame header should be in one NAL with stream coded by hw
                    if (IS_VP9(vcenc_instance->codecFormat) || IS_AV1(vcenc_instance->codecFormat))
                        sizeTbl[numNalus] +=
                            (vcenc_instance->strmHeaderLen + temporalDelimiterSize);

                    i = 0;
                    n = pic->slice.tail;
                    while (HANTRO_TRUE) {
                        if ((vcenc_instance->tiles_enabled_flag &&
                             i >= vcenc_instance->num_tile_rows) ||
                            !n) {
                            break;
                        }
                        u32 sliceBytes = sizeTbl[numNalus];

                        numNalus++;
                        if (asic->regs.prefixnal_svc_ext) {
                            sliceBytes += sizeTbl[numNalus];
                            numNalus++;
                        }

                        APIPRINT(vcenc_instance->verbose, "POC %3d slice %d size=%d\n",
                                 vcenc_instance->poc, sliceNum, sliceBytes);
                        sliceNum++;
                        pEncOut->maxSliceStreamSize = MAX(pEncOut->maxSliceStreamSize, sliceBytes);
                        if (vcenc_instance->tiles_enabled_flag)
                            i++;
                        else
                            n = n->next;
                    }
                    sizeTbl[numNalus] = 0;
                }
                if (naluIndex == 0)
                    pEncOut->numNalus = numNalus;
                else
                    pEncOut->tileExtra[naluIndex - 1].numNalus = numNalus;
                u32 hashval =
                    EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror, HWIF_ENC_HASH_VAL);
                u32 hashoffset =
                    EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror, HWIF_ENC_HASH_OFFSET);
                hash_reset(&vcenc_instance->hashctx, hashval, hashoffset);
                hashval = hash_finalize(&vcenc_instance->hashctx);
                if (vcenc_instance->hashctx.hash_type == 1) {
                    APITRACE("POC %3d crc32 %08x\n", vcenc_instance->poc, hashval);
                } else if (vcenc_instance->hashctx.hash_type == 2) {
                    APITRACE("POC %3d checksum %08x\n", vcenc_instance->poc, hashval);
                }
                hash_init(&vcenc_instance->hashctx, vcenc_instance->hashctx.hash_type);
                vcenc_instance->streamMultiSegment.rdCnt = 0;

#ifdef CTBRC_STRENGTH
                asic->regs.sumOfQP =
                    EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror, HWIF_ENC_QP_SUM) |
                    (EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror, HWIF_ENC_QP_SUM_MSB)
                     << 26);
                asic->regs.sumOfQPNumber =
                    EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror, HWIF_ENC_QP_NUM) |
                    (EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror, HWIF_ENC_QP_NUM_MSB)
                     << 20);
                asic->regs.picComplexity = EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror,
                                                                   HWIF_ENC_PIC_COMPLEXITY) |
                                           (EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror,
                                                                    HWIF_ENC_PIC_COMPLEXITY_MSB)
                                            << 23);
#else
                asic->regs.averageQP =
                    EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror, HWIF_ENC_AVERAGEQP);
                asic->regs.nonZeroCount =
                    EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror, HWIF_ENC_NONZEROCOUNT);
#endif

                //for adaptive GOP
                vcenc_instance->rateControl.intraCu8Num = asic->regs.intraCu8Num =
                    EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror, HWIF_ENC_INTRACU8NUM) |
                    (EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror,
                                             HWIF_ENC_INTRACU8NUM_MSB)
                     << 20);

                vcenc_instance->rateControl.skipCu8Num = asic->regs.skipCu8Num =
                    EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror, HWIF_ENC_SKIPCU8NUM) |
                    (EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror,
                                             HWIF_ENC_SKIPCU8NUM_MSB)
                     << 20);

                vcenc_instance->rateControl.PBFrame4NRdCost = asic->regs.PBFrame4NRdCost =
                    EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror,
                                            HWIF_ENC_PBFRAME4NRDCOST);

                // video stabilization
                asic->regs.stabMotionMin =
                    EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror, HWIF_ENC_STAB_MINIMUM);
                asic->regs.stabMotionSum = EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror,
                                                                   HWIF_ENC_STAB_MOTION_SUM);
                asic->regs.stabGmvX =
                    EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror, HWIF_ENC_STAB_GMVX);
                asic->regs.stabGmvY =
                    EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror, HWIF_ENC_STAB_GMVY);
                asic->regs.stabMatrix1 =
                    EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror, HWIF_ENC_STAB_MATRIX1);
                asic->regs.stabMatrix2 =
                    EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror, HWIF_ENC_STAB_MATRIX2);
                asic->regs.stabMatrix3 =
                    EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror, HWIF_ENC_STAB_MATRIX3);
                asic->regs.stabMatrix4 =
                    EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror, HWIF_ENC_STAB_MATRIX4);
                asic->regs.stabMatrix5 =
                    EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror, HWIF_ENC_STAB_MATRIX5);
                asic->regs.stabMatrix6 =
                    EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror, HWIF_ENC_STAB_MATRIX6);
                asic->regs.stabMatrix7 =
                    EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror, HWIF_ENC_STAB_MATRIX7);
                asic->regs.stabMatrix8 =
                    EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror, HWIF_ENC_STAB_MATRIX8);
                asic->regs.stabMatrix9 =
                    EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror, HWIF_ENC_STAB_MATRIX9);

                //CTB_RC
                asic->regs.ctbBitsMin =
                    EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror, HWIF_ENC_CTBBITSMIN);
                asic->regs.ctbBitsMax =
                    EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror, HWIF_ENC_CTBBITSMAX);
                asic->regs.totalLcuBits =
                    EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror, HWIF_ENC_TOTALLCUBITS);
                vcenc_instance->iThreshSigmaCalcd = asic->regs.nrThreshSigmaCalced =
                    EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror,
                                            HWIF_ENC_THRESH_SIGMA_CALCED);
                vcenc_instance->iSigmaCalcd = asic->regs.nrFrameSigmaCalced =
                    EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror,
                                            HWIF_ENC_FRAME_SIGMA_CALCED);

                /* motion score for 2-pass Agop */
                if (asic->regs.asicCfg.bMultiPassSupport) {
                    asic->regs.motionScore[0][0] = EncAsicGetRegisterValue(
                        asic->ewl, asic->regs.regMirror, HWIF_ENC_MOTION_SCORE_L0_0);
                    ;
                    asic->regs.motionScore[0][1] = EncAsicGetRegisterValue(
                        asic->ewl, asic->regs.regMirror, HWIF_ENC_MOTION_SCORE_L0_1);
                    ;
                    asic->regs.motionScore[1][0] = EncAsicGetRegisterValue(
                        asic->ewl, asic->regs.regMirror, HWIF_ENC_MOTION_SCORE_L1_0);
                    ;
                    asic->regs.motionScore[1][1] = EncAsicGetRegisterValue(
                        asic->ewl, asic->regs.regMirror, HWIF_ENC_MOTION_SCORE_L1_1);
                    ;
                }

                if (asic->regs.asicCfg.ssimSupport && asic->regs.ssim)
                    EncGetSSIM(vcenc_instance, pEncOut);

                /*calculate PSNR under several conditions */
                EncGetPSNR(vcenc_instance, pEncOut);

                if ((asic->regs.asicCfg.ctbRcVersion > 0) &&
                    IS_CTBRC_FOR_BITRATE(vcenc_instance->rateControl.ctbRc))
                    getCtbRcParams(vcenc_instance, pic->sliceInst->type);

                /* collect data to tileCtrl */
                TileInfoCollect(vcenc_instance, tileIndex, numNalus);

                /* should before Release */
                if (vcenc_instance->pass != 1) {
                    void *prof_data = (void *)EncAsicGetAddrRegisterValue(
                        asic->ewl, asic->regs.regMirror, HWIF_ENC_COMPRESSEDCOEFF_BASE);
                    i32 qp =
                        EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror, HWIF_ENC_PIC_QP);
                    i32 poc =
                        EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror, HWIF_ENC_POC);

                    EWLTraceProfile(asic->ewl, prof_data, qp, poc);
                }

                if (vcenc_instance->pass == 1 &&
                    vcenc_instance->asic.regs.asicCfg.bMultiPassSupport) {
                    pic->outForCutree.motionScore[0][0] =
                        vcenc_instance->asic.regs.motionScore[0][0];
                    pic->outForCutree.motionScore[0][1] =
                        vcenc_instance->asic.regs.motionScore[0][1];
                    pic->outForCutree.motionScore[1][0] =
                        vcenc_instance->asic.regs.motionScore[1][0];
                    pic->outForCutree.motionScore[1][1] =
                        vcenc_instance->asic.regs.motionScore[1][1];
                }

                if (vcenc_instance->asic.regs.bVCMDEnable) {
                    EWLReleaseCmdbuf(asic->ewl, (u16)waitId);
                }
                if (s->long_term_ref_pics_present_flag) {
                    if (0 != pEncIn->u8IdxEncodedAsLTR) {
                        if (IS_H264(vcenc_instance->codecFormat)) {
                            // H.264 ltr can not be reused after replaced
                            struct sw_picture *p_old;
                            i32 old_poc = pEncIn->long_term_ref_pic[pEncIn->u8IdxEncodedAsLTR - 1];
                            if (old_poc >= 0 && (p_old = get_picture(c, old_poc))) {
                                p_old->h264_no_ref = HANTRO_TRUE;
                            }
                        }
                    }
#if 0 /* Not needed (mmco=4, set max long-term index) */
            if (codingType == VCENC_INTRA_FRAME && pic->poc == 0 && vcenc_instance->rateControl.ltrCnt > 0)
              asic->regs.max_long_term_frame_idx_plus1 = vcenc_instance->rateControl.ltrCnt;
            else
#endif
                    asic->regs.max_long_term_frame_idx_plus1 = 0;
                }
                ret = VCENC_OK;
                break;

            default:
                APITRACEERR("VCEncStrmEncode: ERROR Fatal system error\n");
                /* should never get here */
                ASSERT(0);
                ret = VCENC_ERROR;
            }
        }
    } while (status &
             (ASIC_STATUS_SLICE_READY | ASIC_STATUS_LINE_BUFFER_DONE | ASIC_STATUS_SEGMENT_READY));

    return ret;
}

static u32 round_this_value(u32 value)
{
    u32 i = 0, tmp = 0;
    while (4095 < (value >> (6 + i++)))
        ;
    tmp = (value >> (6 + i)) << (6 + i);
    return tmp;
}
void useExtPara(const VCEncExtParaIn *pEncExtParaIn, asicData_s *asic, i32 h264Codec,
                struct vcenc_instance *vcenc_instance)
{
    i32 i, j;
    regValues_s *regs = &asic->regs;

    regs->poc = pEncExtParaIn->recon.poc;
    regs->frameNum = pEncExtParaIn->recon.frame_num;

    //                = pEncExtParaIn->recon.flags;
    regs->reconLumBase = pEncExtParaIn->recon.busReconLuma;
    regs->reconCbBase = pEncExtParaIn->recon.busReconChromaUV;

    regs->reconL4nBase = pEncExtParaIn->recon.reconLuma_4n;

    regs->recon_luma_compress_tbl_base = pEncExtParaIn->recon.compressTblReconLuma;
    regs->recon_chroma_compress_tbl_base = pEncExtParaIn->recon.compressTblReconChroma;

    regs->colctbs_store_base = pEncExtParaIn->recon.colBufferH264Recon;

    //cu info output
    regs->cuInfoTableBase = pEncExtParaIn->recon.cuInfoMemRecon;
    regs->cuInfoDataBase = pEncExtParaIn->recon.cuInfoMemRecon + asic->cuInfoTableSize;

    if (h264Codec) {
        //regs->frameNum = pEncExtParaIn->params.h264Para.frame_num;
        if (regs->frameCodingType == 1) //I
        {
            if (pEncExtParaIn->params.h264Para.idr_pic_flag) {
                asic->regs.nal_unit_type = H264_IDR;
            } else
                asic->regs.nal_unit_type = H264_NONIDR;
        }

        regs->idrPicId =
            pEncExtParaIn->params.h264Para.idr_pic_id &
            1; //(pEncExtParaIn->params.h264Para.idr_pic_flag)? (pEncExtParaIn->params.h264Para.idr_pic_id):0;

        if (!vcenc_instance->flexRefsEnable)
            regs->nalRefIdc_2bit = regs->nalRefIdc =
                pEncExtParaIn->params.h264Para.reference_pic_flag;
        else
            regs->nalRefIdc_2bit =
                regs->nalRefIdc; //h264_mmo_collect decide according to gopCurrPicConfig and gopNextPicConfig
        //  regs->transform8x8Enable
        //  regs->entropy_coding_mode_flag
        regs->colctbs_load_base = 0;
        if (pEncExtParaIn->params.h264Para.slice_type == SLICE_TYPE_I ||
            pEncExtParaIn->params.h264Para.slice_type == SLICE_TYPE_SI) {
            if ((pEncExtParaIn->params.h264Para.idr_pic_flag)) {
                //not need curLongTermIdx
                regs->currentLongTermIdx = 0;
                regs->markCurrentLongTerm = !!(pEncExtParaIn->recon.flags & LONG_TERM_REFERENCE);
            } else {
                //curLongTermIdx
                regs->markCurrentLongTerm = !!(pEncExtParaIn->recon.flags & LONG_TERM_REFERENCE);
                regs->currentLongTermIdx = pEncExtParaIn->recon.frame_idx;
            }
        }

        regs->l0_delta_framenum[0] = 0;
        regs->l0_delta_framenum[1] = 0;
        regs->l1_delta_framenum[0] = 0;
        regs->l1_delta_framenum[1] = 0;

        regs->l0_used_by_curr_pic[0] = 0;
        regs->l0_used_by_curr_pic[1] = 0;
        regs->l1_used_by_curr_pic[0] = 0;
        regs->l1_used_by_curr_pic[1] = 0;
        regs->l0_long_term_flag[0] = 0;
        regs->l0_long_term_flag[1] = 0;
        regs->l1_long_term_flag[0] = 0;
        regs->l1_long_term_flag[1] = 0;
        regs->num_long_term_pics = 0;

        if ((pEncExtParaIn->params.h264Para.slice_type == SLICE_TYPE_P) ||
            (pEncExtParaIn->params.h264Para.slice_type == SLICE_TYPE_B)) {
            for (i = 0; i < (pEncExtParaIn->params.h264Para.num_ref_idx_l0_active_minus1 + 1);
                 i++) {
                regs->l0_delta_framenum[i] = regs->frameNum - pEncExtParaIn->reflist0[i].frame_num;
                regs->l0_referenceLongTermIdx[i] = pEncExtParaIn->reflist0[i].frame_idx;
            }
            //regs->l0_referenceLongTermIdx[i] = (pic->rpl[0][i]->long_term_flag ? pic->rpl[0][i]->curLongTermIdx : 0);
            //libva not support mmco so
            if (pEncExtParaIn->recon.flags & LONG_TERM_REFERENCE) {
                //curLongTermIdx
                regs->markCurrentLongTerm = 1;
            } else {
                regs->markCurrentLongTerm = 0;
            }

            regs->currentLongTermIdx = pEncExtParaIn->recon.frame_idx;

            for (i = 0; i < (pEncExtParaIn->params.h264Para.num_ref_idx_l0_active_minus1 + 1); i++)
                regs->l0_used_by_curr_pic[i] = 1;

            for (i = 0; i < (pEncExtParaIn->params.h264Para.num_ref_idx_l0_active_minus1 + 1);
                 i++) {
                regs->pRefPic_recon_l0[0][i] = pEncExtParaIn->reflist0[i].busReconLuma;
                regs->pRefPic_recon_l0[1][i] = pEncExtParaIn->reflist0[i].busReconChromaUV;
                regs->pRefPic_recon_l0_4n[i] = pEncExtParaIn->reflist0[i].reconLuma_4n;

                regs->l0_delta_poc[i] = pEncExtParaIn->reflist0[i].poc - pEncExtParaIn->recon.poc;
                regs->l0_long_term_flag[i] =
                    !!(pEncExtParaIn->reflist0[i].flags & LONG_TERM_REFERENCE);
                regs->num_long_term_pics += regs->l0_long_term_flag[i];

                //compress
                regs->ref_l0_luma_compressed[i] = regs->recon_luma_compress;
                regs->ref_l0_luma_compress_tbl_base[i] =
                    pEncExtParaIn->reflist0[i].compressTblReconLuma;

                regs->ref_l0_chroma_compressed[i] = regs->recon_chroma_compress;
                regs->ref_l0_chroma_compress_tbl_base[i] =
                    pEncExtParaIn->reflist0[i].compressTblReconChroma;
            }
        }

        if (pEncExtParaIn->params.h264Para.slice_type == SLICE_TYPE_B) {
            regs->colctbs_load_base = pEncExtParaIn->reflist1[0].colBufferH264Recon;

            for (i = 0; i < (pEncExtParaIn->params.h264Para.num_ref_idx_l1_active_minus1 + 1);
                 i++) {
                regs->l1_delta_framenum[i] = regs->frameNum - pEncExtParaIn->reflist1[i].frame_num;
                regs->l1_referenceLongTermIdx[i] = pEncExtParaIn->reflist1[i].frame_idx;
            }
            //libva not support mmco so
            if (pEncExtParaIn->recon.flags & LONG_TERM_REFERENCE) {
                //curLongTermIdx
                regs->markCurrentLongTerm = 1;
            } else
                regs->markCurrentLongTerm = 0;

            regs->currentLongTermIdx = pEncExtParaIn->recon.frame_idx;

            for (i = 0; i < (pEncExtParaIn->params.h264Para.num_ref_idx_l1_active_minus1 + 1); i++)
                regs->l1_used_by_curr_pic[i] = 1;

            for (i = 0; i < (pEncExtParaIn->params.h264Para.num_ref_idx_l1_active_minus1 + 1);
                 i++) {
                regs->pRefPic_recon_l1[0][i] = pEncExtParaIn->reflist1[i].busReconLuma;
                regs->pRefPic_recon_l1[1][i] = pEncExtParaIn->reflist1[i].busReconChromaUV;
                regs->pRefPic_recon_l1_4n[i] = pEncExtParaIn->reflist1[i].reconLuma_4n;

                regs->l1_delta_poc[i] = pEncExtParaIn->reflist1[i].poc - pEncExtParaIn->recon.poc;
                regs->l1_long_term_flag[i] =
                    !!(pEncExtParaIn->reflist1[i].flags & LONG_TERM_REFERENCE);
                regs->num_long_term_pics += regs->l1_long_term_flag[i];

                //compress
                regs->ref_l1_luma_compressed[i] = regs->recon_luma_compress;
                regs->ref_l1_luma_compress_tbl_base[i] =
                    pEncExtParaIn->reflist1[i].compressTblReconLuma;

                regs->ref_l1_chroma_compressed[i] = regs->recon_chroma_compress;
                regs->ref_l1_chroma_compress_tbl_base[i] =
                    pEncExtParaIn->reflist1[i].compressTblReconChroma;
            }
        }

        /* H.264 MMO  libva not support this feature*/
        regs->l0_used_by_next_pic[0] = 1;
        regs->l0_used_by_next_pic[1] = 1;
        regs->l1_used_by_next_pic[0] = 1;
        regs->l1_used_by_next_pic[1] = 1;

        if (IS_H264(vcenc_instance->codecFormat) && regs->nalRefIdc &&
            vcenc_instance->h264_mmo_nops > 0) {
            int cnt[2] = {pEncExtParaIn->params.h264Para.num_ref_idx_l0_active_minus1 + 1,
                          pEncExtParaIn->params.h264Para.num_ref_idx_l1_active_minus1 + 1};
            if (!vcenc_instance->flexRefsEnable) {
                for (i = 0; i < vcenc_instance->h264_mmo_nops; i++)
                    h264_mmo_mark_unref_ext(&asic->regs, vcenc_instance->h264_mmo_unref_ext[i],
                                            vcenc_instance->h264_mmo_long_term_flag[i],
                                            vcenc_instance->h264_mmo_ltIdx[i], cnt, pEncExtParaIn);
                vcenc_instance->h264_mmo_nops = 0;
            } else {
                if (pEncExtParaIn->params.h264Para.slice_type == SLICE_TYPE_P) {
                    cnt[0] = 1;
                    cnt[1] = 0;
                } else if (pEncExtParaIn->params.h264Para.slice_type == SLICE_TYPE_B) {
                    cnt[0] = 1;
                    cnt[1] = 1;
                } else {
                    cnt[0] = 0;
                    cnt[1] = 0;
                }
                for (i = 0; i < vcenc_instance->h264_mmo_nops; i++)
                    h264_mmo_mark_unref_ext(&asic->regs, vcenc_instance->h264_mmo_unref[i],
                                            vcenc_instance->h264_mmo_long_term_flag[i],
                                            vcenc_instance->h264_mmo_ltIdx[i], cnt, pEncExtParaIn);
                vcenc_instance->h264_mmo_nops = 0;
            }
        }
    } else {
        //hevc
        //regs->frameNum = pEncExtParaIn->params.hevcPara.frame_num;
        if (regs->frameCodingType == 1) //I
        {
            if (pEncExtParaIn->params.hevcPara.idr_pic_flag) {
                asic->regs.nal_unit_type = IDR_W_RADL;
            } else
                asic->regs.nal_unit_type = TRAIL_R;
        }

        //can del this row, this register is only used for h264
        regs->idrPicId =
            pEncExtParaIn->params.hevcPara.idr_pic_id &
            1; //s(regs->frameCodingType == 1)? (pEncExtParaIn->params.hevcPara.idr_pic_id & 1):0;

        //can del this row, this register is only used for h264
        regs->nalRefIdc = pEncExtParaIn->params.hevcPara.reference_pic_flag;
        //  regs->transform8x8Enable
        //  regs->entropy_coding_mode_flag
        regs->colctbs_load_base = 0;
        //
        if (pEncExtParaIn->params.hevcPara.slice_type == SLICE_TYPE_I ||
            pEncExtParaIn->params.hevcPara.slice_type == SLICE_TYPE_SI) {
            //can del this row, this register is only used for h264
            regs->markCurrentLongTerm = !!(pEncExtParaIn->recon.flags & LONG_TERM_REFERENCE);
        }

        //not used for hevc
        regs->l0_delta_framenum[0] = 0;
        regs->l0_delta_framenum[1] = 0;
        regs->l1_delta_framenum[0] = 0;
        regs->l1_delta_framenum[1] = 0;

        regs->l0_used_by_curr_pic[0] = 0;
        regs->l0_used_by_curr_pic[1] = 0;
        regs->l1_used_by_curr_pic[0] = 0;
        regs->l1_used_by_curr_pic[1] = 0;
        regs->l0_long_term_flag[0] = 0;
        regs->l0_long_term_flag[1] = 0;
        regs->l1_long_term_flag[0] = 0;
        regs->l1_long_term_flag[1] = 0;
        regs->num_long_term_pics = 0;

        if ((pEncExtParaIn->params.hevcPara.slice_type == SLICE_TYPE_P) ||
            (pEncExtParaIn->params.hevcPara.slice_type == SLICE_TYPE_B)) {
            for (i = 0; i < (pEncExtParaIn->params.hevcPara.num_ref_idx_l0_active_minus1 + 1);
                 i++) {
                regs->l0_delta_framenum[i] = regs->frameNum - pEncExtParaIn->reflist0[i].frame_num;
                regs->l0_referenceLongTermIdx[i] = pEncExtParaIn->reflist0[i].frame_idx;
            }
            //libva not support mmco so
            regs->markCurrentLongTerm = 0;

            for (i = 0; i < (pEncExtParaIn->params.hevcPara.num_ref_idx_l0_active_minus1 + 1); i++)
                regs->l0_used_by_curr_pic[i] = 1;

            for (i = 0; i < (pEncExtParaIn->params.hevcPara.num_ref_idx_l0_active_minus1 + 1);
                 i++) {
                regs->pRefPic_recon_l0[0][i] = pEncExtParaIn->reflist0[i].busReconLuma;
                regs->pRefPic_recon_l0[1][i] = pEncExtParaIn->reflist0[i].busReconChromaUV;
                regs->pRefPic_recon_l0_4n[i] = pEncExtParaIn->reflist0[i].reconLuma_4n;

                regs->l0_delta_poc[i] = pEncExtParaIn->reflist0[i].poc - pEncExtParaIn->recon.poc;
                regs->l0_long_term_flag[i] =
                    !!(pEncExtParaIn->reflist0[i].flags & LONG_TERM_REFERENCE);
                regs->num_long_term_pics += regs->l0_long_term_flag[i];

                //compress
                regs->ref_l0_luma_compressed[i] = regs->recon_luma_compress;
                regs->ref_l0_luma_compress_tbl_base[i] =
                    pEncExtParaIn->reflist0[i].compressTblReconLuma;

                regs->ref_l0_chroma_compressed[i] = regs->recon_chroma_compress;
                regs->ref_l0_chroma_compress_tbl_base[i] =
                    pEncExtParaIn->reflist0[i].compressTblReconChroma;
            }
        }

        if (pEncExtParaIn->params.hevcPara.slice_type == SLICE_TYPE_B) {
            regs->colctbs_load_base = pEncExtParaIn->reflist1[0].colBufferH264Recon;

            for (i = 0; i < (pEncExtParaIn->params.hevcPara.num_ref_idx_l1_active_minus1 + 1);
                 i++) {
                regs->l1_delta_framenum[i] = regs->frameNum - pEncExtParaIn->reflist1[i].frame_num;
                regs->l1_referenceLongTermIdx[i] = pEncExtParaIn->reflist1[i].frame_idx;
            }
            //libva not support mmco so
            regs->markCurrentLongTerm = 0;

            for (i = 0; i < (pEncExtParaIn->params.hevcPara.num_ref_idx_l1_active_minus1 + 1); i++)
                regs->l1_used_by_curr_pic[i] = 1;

            for (i = 0; i < (pEncExtParaIn->params.hevcPara.num_ref_idx_l1_active_minus1 + 1);
                 i++) {
                regs->pRefPic_recon_l1[0][i] = pEncExtParaIn->reflist1[i].busReconLuma;
                regs->pRefPic_recon_l1[1][i] = pEncExtParaIn->reflist1[i].busReconChromaUV;
                regs->pRefPic_recon_l1_4n[i] = pEncExtParaIn->reflist1[i].reconLuma_4n;

                regs->l1_delta_poc[i] = pEncExtParaIn->reflist1[i].poc - pEncExtParaIn->recon.poc;
                regs->l1_long_term_flag[i] =
                    !!(pEncExtParaIn->reflist1[i].flags & LONG_TERM_REFERENCE);
                regs->num_long_term_pics += regs->l1_long_term_flag[i];

                //compress
                regs->ref_l1_luma_compressed[i] = regs->recon_luma_compress;
                regs->ref_l1_luma_compress_tbl_base[i] =
                    pEncExtParaIn->reflist1[i].compressTblReconLuma;

                regs->ref_l1_chroma_compressed[i] = regs->recon_chroma_compress;
                regs->ref_l1_chroma_compress_tbl_base[i] =
                    pEncExtParaIn->reflist1[i].compressTblReconChroma;
            }
        }
#if 0
    //used for long term ref?
    {
      for (i = (pEncExtParaIn->params.hevcPara.num_ref_idx_l0_active_minus1+1), j = 0; i < 2 && j < r->lt_follow_cnt; i++, j++) {
        asic->regs.l0_delta_poc[i] = r->lt_follow[j] - pic->poc;
        regs->l0_delta_poc[i] = pEncExtParaIn->reflist0[i].poc - pEncExtParaIn->recon.poc;
        asic->regs.l0_long_term_flag[i] = 1;
      }
      for (i = (pEncExtParaIn->params.hevcPara.num_ref_idx_l1_active_minus1+1); i < 2 && j < r->lt_follow_cnt; i++, j++) {
        asic->regs.l1_delta_poc[i] = r->lt_follow[j] - pic->poc;
        asic->regs.l1_long_term_flag[i] = 1;
      }
    }
#endif
        /* H.264 MMO  libva not support this feature*/
        regs->l0_used_by_next_pic[0] = 1;
        regs->l0_used_by_next_pic[1] = 1;
        regs->l1_used_by_next_pic[0] = 1;
        regs->l1_used_by_next_pic[1] = 1;

        if (regs->num_long_term_pics)
            regs->long_term_ref_pics_present_flag = 1;

        regs->rps_neg_pic_num = pEncExtParaIn->params.hevcPara.rps_neg_pic_num;
        regs->rps_pos_pic_num = pEncExtParaIn->params.hevcPara.rps_pos_pic_num;
        ASSERT(regs->rps_neg_pic_num + regs->rps_pos_pic_num <= VCENC_MAX_REF_FRAMES);
        for (i = 0; i < (i32)(regs->rps_neg_pic_num + regs->rps_pos_pic_num); i++) {
            regs->rps_delta_poc[i] = pEncExtParaIn->params.hevcPara.rps_delta_poc[i];
            regs->rps_used_by_cur[i] = pEncExtParaIn->params.hevcPara.rps_used_by_cur[i];
        }
        for (i = regs->rps_neg_pic_num + regs->rps_pos_pic_num; i < VCENC_MAX_REF_FRAMES; i++) {
            regs->rps_delta_poc[i] = 0;
            regs->rps_used_by_cur[i] = 0;
        }
    }
}

/*------------------------------------------------------------------------------
    Function name : LambdaTuningSubjective
    Description   : Tuning lambda for subjective quality
    Argument      : asic - asicData_s structure holding asic settings
                  : pEncIn - user provided input parameters
------------------------------------------------------------------------------*/
static void LambdaTuningSubjective(asicData_s *asic, struct sw_picture *pic, const VCEncIn *pEncIn)
{
    i32 gopDepth = 0;
    i32 gopSize = pEncIn->gopSize;
    if (pic->sliceInst->type != I_SLICE) {
        i32 gopPoc = pEncIn->gopCurrPicConfig.poc;
        while (gopPoc % gopSize) {
            gopDepth++;

            i32 mid = gopSize >> 1;
            if (gopPoc > mid) {
                gopPoc -= mid;
                gopSize -= mid;
            } else
                gopSize = mid;
        }
    }

    double factorSad = 1.0;
    double factorSse = 0.9;
    double factorDepth = 1.0;
    if (asic->regs.asicCfg.tuneToolsSet2Support) {
        factorSad = 0.9;
        factorSse = 0.6;
    }

    if (gopDepth) {
        const double factors[4] = {1.0, 1.15, 1.2, 1.2};
        factorDepth = factors[MIN(3, gopDepth)];
    }
    factorSad *= factorDepth;
    factorSse *= factorDepth * factorDepth;

    asic->regs.qpFactorSad = (u32)((factorSad * (1 << QPFACTOR_FIX_POINT)) + 0.5);
    asic->regs.qpFactorSse = (u32)((factorSse * (1 << QPFACTOR_FIX_POINT)) + 0.5);
    asic->regs.qpFactorSad = MIN(asic->regs.qpFactorSad, 0x7FFF);
    asic->regs.qpFactorSse = MIN(asic->regs.qpFactorSse, 0x7FFF);
    asic->regs.lambdaDepth = 0;
}

/*------------------------------------------------------------------------------

    Function name : VCEncStrmEncode
    Description   : Encodes a new picture
    Return type   : VCEncRet
    Argument      : inst - encoder instance
    Argument      : pEncIn - user provided input parameters
    Argument      : pEncOut - place where output info is returned
    Argument      : sliceReadyCbFunc - slice ready callback function
    Argument      : pAppData - needed by sliceReadyCbFunc
------------------------------------------------------------------------------*/
VCEncRet VCEncStrmEncode(VCEncInst inst, VCEncIn *pEncIn, VCEncOut *pEncOut,
                         VCEncSliceReadyCallBackFunc sliceReadyCbFunc, void *pAppData)
{
    VCEncRet ret = VCENC_ERROR;
    struct vcenc_instance *vcenc_instance = (struct vcenc_instance *)inst;

    //single pass & disable flexRefs
    if (0 == vcenc_instance->pass && !pEncIn->flexRefsEnable) {
        VCEncJob *job = NULL;
        VCEncIn *pEncInFor1pass =
            &vcenc_instance->encInFor1pass; //maintain single pass encode order
        i32 *pNextGopSize = &vcenc_instance->nextGopSize;
        i32 gopSizeFromUser = pEncIn->gopSize;
        VCENCAdapGopCtr *pAgop = &vcenc_instance->agop;
        VCEncPicConfig *pLastPicCfg = &vcenc_instance->lastPicCfg;
        if (!vcenc_instance->bFlush) { //enqueue job
            ret = SinglePassEnqueueJob(vcenc_instance, pEncIn);
            if (VCENC_OK != ret) {
                APITRACEERR("SinglePassEnqueueJob Failed. \n\n");
                return ret;
            }
            /*if there's an IDR frame before next frame to encode
      or gopSize specified by user is not equal to NextGopSize, refind next picture */
            if ((vcenc_instance->nextIdrCnt > 0 &&
                 vcenc_instance->nextIdrCnt <= pEncInFor1pass->picture_cnt) ||
                (0 != pEncIn->picture_cnt && 0 != gopSizeFromUser &&
                 gopSizeFromUser != *pNextGopSize)) {
                if (0 != gopSizeFromUser && gopSizeFromUser != *pNextGopSize)
                    *pNextGopSize = gopSizeFromUser; //reset next gop size

                SetPicCfgToEncIn(pLastPicCfg, pEncInFor1pass);
                //find next picture
                FindNextPic(inst, pEncInFor1pass, *pNextGopSize,
                            pEncInFor1pass->gopConfig.gopCfgOffset, vcenc_instance->nextIdrCnt);
            }
        } else //flush
        {
            i32 enqueueNum = vcenc_instance->enqueueJobNum;
            u64 numer, denom;
            denom = (u64)pEncInFor1pass->gopConfig.inputRateNumer *
                    (u64)pEncInFor1pass->gopConfig.outputRateDenom;
            numer = (u64)pEncInFor1pass->gopConfig.inputRateDenom *
                    (u64)pEncInFor1pass->gopConfig.outputRateNumer;
            i32 bNornalEnd =
                (((pEncInFor1pass->gopConfig.lastPic - pEncInFor1pass->gopConfig.firstPic + 1) *
                  numer / denom) <= enqueueNum)
                    ? 1
                    : 0;
            if (!bNornalEnd) //early end need to refind next picture
            {
                if (pLastPicCfg->poc == -1)
                    return VCENC_ERROR; //if lastPicCfg is invalid, return error.
                SetPicCfgToEncIn(pLastPicCfg, pEncInFor1pass);
                pEncInFor1pass->gopConfig.lastPic =
                    pEncInFor1pass->gopConfig.firstPic + enqueueNum * denom / numer - 1;
                //find next picture
                FindNextPic(inst, pEncInFor1pass, *pNextGopSize,
                            pEncInFor1pass->gopConfig.gopCfgOffset, vcenc_instance->nextIdrCnt);
            }
        }

        //get next picture according to encode order
        job = SinglePassGetNextJob(vcenc_instance, pEncInFor1pass->picture_cnt);
        if (job == NULL) //no job to process, return VCENC_FRAME_ENQUEUE
        {
            //need to return buffer to buffer pool
            pEncOut->consumedAddr.inputbufBusAddr = 0;
            pEncOut->consumedAddr.dec400TableBusAddr = 0;
            pEncOut->consumedAddr.roiMapDeltaQpBusAddr = 0;
            pEncOut->consumedAddr.roimapCuCtrlInfoBusAddr = 0;
            memset(pEncOut->consumedAddr.overlayInputBusAddr, 0, MAX_OVERLAY_NUM * sizeof(ptr_t));
            if (vcenc_instance->bIOBufferBinding) {
                pEncOut->consumedAddr.outbufBusAddr = 0;
            } else {
                pEncOut->consumedAddr.outbufBusAddr = pEncIn->busOutBuf[0];
            }

            if (vcenc_instance->bFlush) //end encode
            {
                return VCENC_OK;
            } else
                return VCENC_FRAME_ENQUEUE;
        } else {
            SetPictureCfgToJob(pEncInFor1pass, &job->encIn, vcenc_instance->gdrDuration);
            if (job->encIn.bIsIDR) {
                //updata next idr frame count
                i32 nextIdrCnt =
                    vcenc_instance->nextIdrCnt + pEncInFor1pass->gopConfig.idr_interval;
                i32 nextForceIdrCnt = FindNextForceIdr(&vcenc_instance->jobQueue);
                i32 curIdrCnt = vcenc_instance->nextIdrCnt;
                if (curIdrCnt <= job->encIn.picture_cnt) {
                    if (pEncInFor1pass->gopConfig.idr_interval > 0) // exist default idr interval
                    {
                        if (nextForceIdrCnt > curIdrCnt && nextForceIdrCnt < nextIdrCnt)
                            vcenc_instance->nextIdrCnt = nextForceIdrCnt;
                        else
                            vcenc_instance->nextIdrCnt = nextIdrCnt;
                    } else {
                        if (nextForceIdrCnt > curIdrCnt)
                            vcenc_instance->nextIdrCnt = nextForceIdrCnt;
                        else
                            vcenc_instance->nextIdrCnt = -1;
                    }
                }
            }

            if (!vcenc_instance->bIOBufferBinding) {
                job->encIn.pOutBuf[0] = pEncIn->pOutBuf[0];
                job->encIn.busOutBuf[0] = pEncIn->busOutBuf[0];
                job->encIn.outBufSize[0] = pEncIn->outBufSize[0];
                job->encIn.cur_out_buffer[0] = pEncIn->cur_out_buffer[0];
            }
            //set coding ctrl parameters into instance
            EncUpdateCodingCtrlParam(vcenc_instance, job->pCodingCtrlParam, job->encIn.picture_cnt);
            //process picture
            ret =
                VCEncStrmEncodeExt(inst, &job->encIn, NULL, pEncOut, sliceReadyCbFunc, pAppData, 0);

            PutBufferToPool(vcenc_instance->jobBufferPool, (void **)&job);

            /* pic skip by RC or buffer overflow */
            if ((ret != VCENC_FRAME_ENQUEUE && pEncOut->streamSize == 0) ||
                pEncOut->codingType == VCENC_NOTCODED_FRAME ||
                ret == VCENC_OUTPUT_BUFFER_OVERFLOW) {
                //save last encIn
                SavePicCfg(pEncInFor1pass, pLastPicCfg);
                pEncInFor1pass->picture_cnt++;
                if (vcenc_instance->nextIdrCnt > 0) //if overflow, idr frame need to be put off
                {
                    vcenc_instance->nextIdrCnt++;
                }
            } else {
                //Adaptive GOP size decision
                if (0 == vcenc_instance->gopSize &&
                    pEncInFor1pass->codingType != VCENC_INTRA_FRAME && 0 == gopSizeFromUser)
                    AGopDecision(vcenc_instance, pEncInFor1pass, pEncOut, pNextGopSize, pAgop);
                //save last encIn
                SavePicCfg(pEncInFor1pass, pLastPicCfg);
                //find next picture
                FindNextPic(inst, pEncInFor1pass, *pNextGopSize,
                            pEncInFor1pass->gopConfig.gopCfgOffset, vcenc_instance->nextIdrCnt);
            }
        }

    }

    //2pass: manager encode order in pass 1
    //flexRef: not need to manage encode order
    else {
        if ((!pEncIn->flexRefsEnable) || (!vcenc_instance->bFlush))
            ret = VCEncStrmEncodeExt(inst, pEncIn, NULL, pEncOut, sliceReadyCbFunc, pAppData, 0);
        else
            ret = VCENC_OK;
    }

    return ret;
}
/*------------------------------------------------------------------------------

    Function name : VCEncStrmEncode
    Description   : Encodes a new picture
    Return type   : VCEncRet
    Argument      : inst - encoder instance
    Argument      : pEncIn - user provided input parameters
    Argument      : pEncOut - place where output info is returned
    Argument      : sliceReadyCbFunc - slice ready callback function
    Argument      : pAppData - needed by sliceReadyCbFunc
------------------------------------------------------------------------------*/

VCEncRet VCEncStrmEncodeExt(VCEncInst inst, VCEncIn *pEncIn, const VCEncExtParaIn *pEncExtParaIn,
                            VCEncOut *pEncOut, VCEncSliceReadyCallBackFunc sliceReadyCbFunc,
                            void *pAppData, i32 useExtFlag)

{
    struct vcenc_instance *vcenc_instance = (struct vcenc_instance *)inst;
    asicData_s *asic = &vcenc_instance->asic;
    VCEncGopPicConfig sSliceRpsCfg;
    struct container *c;
    struct vcenc_buffer source;
    struct vps *v;
    struct sps *s;
    struct pps *p;
    struct rps *r;
    struct rps *r2 = NULL;
    struct sw_picture *pic;
    struct slice *slice;
    i32 bSkipFrame = 0;
    i32 tmp, i, j;
    i32 ret = VCENC_ERROR;
    VCEncSliceReady slice_callback;
    VCEncPictureCodingType codingType;
    u32 client_type;
    i32 core_id = -1;
    const void *ewl = NULL;
    u8 rpsInSliceHeader = vcenc_instance->RpsInSliceHeader;
    VCDec400data *dec400_data;
    VCDec400data *dec400_osd;
    u32 waitCoreJobid = 0;
    u32 byteCntSw = 0;
    u32 *segcounts = NULL;
    u32 re_encode_counter = 0;
    VCEncStatisticOut enc_statistic;
    NewEncodeParams new_params;
    ReEncodeStrategy use_new_parameters = 0;
    regValues_s *regs_bak_for2nd_encode;
    u32 cur_core_index;
    u32 next_core_index;
    int delta_poc = 0;
    VCEncInTileExtra *tmpTileExtra = NULL;
    u32 iBuf, tileId;

    cur_core_index = vcenc_instance->jobCnt % vcenc_instance->parallelCoreNum;
    next_core_index = (vcenc_instance->jobCnt + 1) % vcenc_instance->parallelCoreNum;

    memset(&new_params, 0, sizeof(NewEncodeParams));
    StrmEncodeTraceEncInPara(pEncIn, vcenc_instance);

    VCEncLookaheadJob *outframe = NULL;
    if (vcenc_instance->pass == 2) {
        if (pEncExtParaIn != NULL) { //for extra parameters.
            VCEncExtParaIn *pExtParaInBuf = (VCEncExtParaIn *)malloc(sizeof(VCEncExtParaIn));
            memcpy(pExtParaInBuf, pEncExtParaIn, sizeof(VCEncExtParaIn));
            queue_put(&vcenc_instance->extraParaQ, (struct node *)pExtParaInBuf);
        }

        if (!vcenc_instance->bFlush) {
            if (AddPictureToLookahead(vcenc_instance, pEncIn, pEncOut) != VCENC_OK)
                return VCENC_ERROR;
        }

        /* Flush with idelay == 0 indicate failure of reading first frame which should be error */
        if (vcenc_instance->bFlush && vcenc_instance->idelay == 0)
            return VCENC_ERROR;

        if (vcenc_instance->idelay <= vcenc_instance->lookaheadDelay)
            vcenc_instance->idelay++;
        if (vcenc_instance->idelay > vcenc_instance->lookaheadDelay || vcenc_instance->bFlush) {
            outframe = GetLookaheadOutput(&vcenc_instance->lookahead, vcenc_instance->bFlush);
            if (outframe == NULL) {
                //before return, set consumedAddrs
                pEncOut->consumedAddr.inputbufBusAddr = 0;
                pEncOut->consumedAddr.dec400TableBusAddr = 0;
                pEncOut->consumedAddr.roiMapDeltaQpBusAddr = 0;
                pEncOut->consumedAddr.roimapCuCtrlInfoBusAddr = 0;
                memset(pEncOut->consumedAddr.overlayInputBusAddr, 0,
                       MAX_OVERLAY_NUM * sizeof(ptr_t));
                if (vcenc_instance->bIOBufferBinding) //binding io
                {
                    pEncOut->consumedAddr.outbufBusAddr = 0;
                } else //not binding io
                {
                    // if not getLookaheadOutput, output buffer is useless, so need to return it to memory factory.
                    pEncOut->consumedAddr.outbufBusAddr = pEncIn->busOutBuf[0];
                }
                return VCENC_OK;
            }
            EncUpdateCodingCtrlParam(vcenc_instance,
                                     (EncCodingCtrlParam *)(outframe->pCodingCtrlParam),
                                     outframe->encIn.picture_cnt);
        } else {
            pEncOut->consumedAddr.inputbufBusAddr = 0;
            pEncOut->consumedAddr.dec400TableBusAddr = 0;
            pEncOut->consumedAddr.roiMapDeltaQpBusAddr = 0;
            pEncOut->consumedAddr.roimapCuCtrlInfoBusAddr = 0;
            memset(pEncOut->consumedAddr.overlayInputBusAddr, 0, MAX_OVERLAY_NUM * sizeof(ptr_t));
            if (vcenc_instance->bIOBufferBinding) //binding io
            {
                pEncOut->consumedAddr.outbufBusAddr = 0;
            } else //not binding io
            {
                // if not getLookaheadOutput, output buffer is useless, so need to return it to memory factory.
                pEncOut->consumedAddr.outbufBusAddr = pEncIn->busOutBuf[0];
            }
            return VCENC_FRAME_ENQUEUE;
        }

        if (outframe->status != VCENC_FRAME_READY) {
            VCEncRet status = outframe->status;
            if (status == VCENC_ERROR) {
                ReleaseLookaheadPicture(&vcenc_instance->lookahead, outframe,
                                        pEncOut->consumedAddr.inputbufBusAddr);
            } else
                PutBufferToPool(vcenc_instance->lookahead.jobBufferPool, (void **)&outframe);
            return status;
        }
        if (vcenc_instance->bIOBufferBinding) {
            pEncIn = &outframe->encIn;
        } else {
            u32 *pOutBuf = pEncIn->pOutBuf[0];      /* Pointer to output stream buffer */
            ptr_t busOutBuf = pEncIn->busOutBuf[0]; /* Bus address of output stream buffer */
            u32 outBufSize = pEncIn->outBufSize[0]; /* Size of output stream buffer in bytes */
            void *cur_out_buffer = pEncIn->cur_out_buffer[0];

            pEncIn = &outframe->encIn;

            pEncIn->pOutBuf[0] = pOutBuf;
            pEncIn->busOutBuf[0] = busOutBuf;
            pEncIn->outBufSize[0] = outBufSize;
            pEncIn->cur_out_buffer[0] = cur_out_buffer;
        }
        if (pEncIn->roiMapDeltaQpAddr == 0) {
            // disable pass2 ROI map
            vcenc_instance->roiMapEnable = 0;
            asic->regs.rcRoiEnable &= ~2;
        } else {
            // enable pass2 ROI map
            vcenc_instance->roiMapEnable = 1;
            asic->regs.rcRoiEnable |= 2;
        }
        i32 nextGopSize = outframe->frame.gopSize;
        if (outframe->frame.frameNum != 0)
            outframe->encIn.gopSize = vcenc_instance->lookahead.lastGopSize;
        if (1 == outframe->encIn
                     .bIsIDR) //if bIsIDR(gdr's poc is not 0), set next idr frame as current frame
            vcenc_instance->nextIdrCnt = outframe->encIn.picture_cnt;
        outframe->encIn.picture_cnt = vcenc_instance->lookahead.picture_cnt;
        outframe->encIn.last_idr_picture_cnt = vcenc_instance->lookahead.last_idr_picture_cnt;

        /* encIn codingType/poc used here is from last frame, to find correct info for current frame
       But bIsIDR is for current frame, so we can use bIsIDR to decide whether to start GDR.
       The reason to do that is pass 1 may change coding type from INTRA to PREDICT */
        outframe->encIn.codingType =
            (outframe->frame.frameNum == 0)
                ? VCENC_INTRA_FRAME
                : FindNextPic(inst, &outframe->encIn, nextGopSize, pEncIn->gopConfig.gopCfgOffset,
                              vcenc_instance->nextIdrCnt);

        /* We may already changed codingType or other info for last pic here
       Need to refresh last info in lookahead*/
        vcenc_instance->lookahead.lastPoc = outframe->encIn.poc;
        vcenc_instance->lookahead.lastGopPicIdx = outframe->encIn.gopPicIdx;
        vcenc_instance->lookahead.lastCodingType = outframe->encIn.codingType;

        vcenc_instance->lookahead.picture_cnt = outframe->encIn.picture_cnt;
        vcenc_instance->lookahead.last_idr_picture_cnt = outframe->encIn.last_idr_picture_cnt;
        if (vcenc_instance->extDSRatio) {
            /* switch to full resolution yuv for 2nd pass */
            outframe->encIn.busLuma = outframe->encIn.busLumaOrig;
            outframe->encIn.busChromaU = outframe->encIn.busChromaUOrig;
            outframe->encIn.busChromaV = outframe->encIn.busChromaVOrig;
        }

        /* For stable RC, if inloop-down-sample is supported by HW
       but small video isn't down-sampled in pass1 encoding because of HW limitation,
       increase the cost manually to be close to the cost in the down-sampled case. */
        struct vcenc_instance *instPass1 =
            (struct vcenc_instance *)vcenc_instance->lookahead.priv_inst;
        i32 costScale =
            (instPass1 && !instPass1->preProcess.inLoopDSRatio && asic->regs.asicCfg.inLoopDSRatio)
                ? 2
                : 1;

        vcenc_instance->cuTreeCtl.costCur = outframe->frame.cost * costScale;
        vcenc_instance->cuTreeCtl.curTypeChar = outframe->frame.typeChar;
        vcenc_instance->lookahead.lastGopPicIdx = outframe->encIn.gopPicIdx;
        vcenc_instance->lookahead.lastGopSize = outframe->encIn.gopSize;
        for (i = 0; i < 4; i++) {
            vcenc_instance->cuTreeCtl.costAvg[i] = outframe->frame.costAvg[i] * costScale;
            vcenc_instance->cuTreeCtl.FrameTypeNum[i] = outframe->frame.FrameTypeNum[i];
            vcenc_instance->cuTreeCtl.costGop[i] = outframe->frame.costGop[i] * costScale;
            vcenc_instance->cuTreeCtl.FrameNumGop[i] = outframe->frame.FrameNumGop[i];
        }

        /* VP9 Segmentation counts */
        segcounts = outframe->frame.segmentCountAddr;
    }
    pEncOut->picture_cnt = pEncIn->picture_cnt;
    pEncOut->poc = pEncIn->poc;

    /* ErrorChecker */
    client_type = VCEncGetClientType(vcenc_instance->codecFormat);
    if ((ret = StrmEncodeCheckPara(vcenc_instance, pEncIn, pEncOut, asic, client_type)) !=
        VCENC_OK) {
        return ret;
    }

    if (pEncIn->bSkipFrame) {
        //TODO: add this feature into EWLHwConfig
        if ((vcenc_instance->asic.regs.asicCfg.roiMapVersion >= 2) &&
            (IS_H264(vcenc_instance->codecFormat) || IS_HEVC(vcenc_instance->codecFormat))) {
            bSkipFrame = pEncIn->bSkipFrame;
        } else if (asic->regs.asicHwId >= 0x80006010 && asic->regs.asicHwId < 0x80006200) {
            bSkipFrame = pEncIn->bSkipFrame;
        } else {
            APITRACEWRN("FRAME_SKIP not supported by HW, Disable it forcely.\n");
        }

        if (pEncIn->bIsIDR) {
            APITRACEWRN("Disable Frame Skip setting for current frame when conflict with IDR "
                        "setting.\n");
            bSkipFrame = pEncIn->bSkipFrame = HANTRO_FALSE;
        }
    }

#ifdef INTERNAL_TEST
    /* Configure the encoder instance according to the test vector */
    HevcConfigureTestBeforeFrame(vcenc_instance);
#endif

    EWLHwConfig_t cfg = EncAsicGetAsicConfig(client_type, vcenc_instance->ctx);

    if (client_type == EWL_CLIENT_TYPE_H264_ENC && cfg.h264Enabled == EWL_HW_CONFIG_NOT_SUPPORTED)
        return VCENC_INVALID_ARGUMENT;

    if (client_type == EWL_CLIENT_TYPE_HEVC_ENC && cfg.hevcEnabled == EWL_HW_CONFIG_NOT_SUPPORTED)
        return VCENC_INVALID_ARGUMENT;

    if (client_type == EWL_CLIENT_TYPE_AV1_ENC && cfg.av1Enabled == EWL_HW_CONFIG_NOT_SUPPORTED)
        return VCENC_INVALID_ARGUMENT;

    if (client_type == EWL_CLIENT_TYPE_VP9_ENC && cfg.vp9Enabled == EWL_HW_CONFIG_NOT_SUPPORTED)
        return VCENC_INVALID_ARGUMENT;

    if ((cfg.bFrameEnabled == EWL_HW_CONFIG_NOT_SUPPORTED) &&
        ((pEncIn->codingType) == VCENC_BIDIR_PREDICTED_FRAME)) {
        APITRACEERR("VCEncStrmEncode: Invalid coding format, B frame not supported by "
                    "core\n");
        return VCENC_INVALID_ARGUMENT;
    }

    if ((cfg.IframeOnly == EWL_HW_CONFIG_ENABLED) && ((pEncIn->codingType) != VCENC_INTRA_FRAME)) {
        APITRACEERR("VCEncStrmEncode: Invalid coding format, only I frame supported by "
                    "core\n");
        return VCENC_INVALID_ARGUMENT;
    }

    if (pEncIn->codingType == VCENC_BIDIR_PREDICTED_FRAME)
        vcenc_instance->featureToSupport.bFrameEnabled = 1;

    if (client_type == EWL_CLIENT_TYPE_H264_ENC) {
        vcenc_instance->featureToSupport.h264Enabled = 1;
        vcenc_instance->featureToSupport.hevcEnabled = 0;
        vcenc_instance->featureToSupport.av1Enabled = 0;
        vcenc_instance->featureToSupport.vp9Enabled = 0;
    } else if (client_type == EWL_CLIENT_TYPE_AV1_ENC) {
        vcenc_instance->featureToSupport.h264Enabled = 0;
        vcenc_instance->featureToSupport.hevcEnabled = 0;
        vcenc_instance->featureToSupport.av1Enabled = 1;
        vcenc_instance->featureToSupport.vp9Enabled = 0;
    } else if (client_type == EWL_CLIENT_TYPE_VP9_ENC) {
        vcenc_instance->featureToSupport.h264Enabled = 0;
        vcenc_instance->featureToSupport.hevcEnabled = 0;
        vcenc_instance->featureToSupport.av1Enabled = 0;
        vcenc_instance->featureToSupport.vp9Enabled = 1;
    } else {
        vcenc_instance->featureToSupport.h264Enabled = 0;
        vcenc_instance->featureToSupport.hevcEnabled = 1;
        vcenc_instance->featureToSupport.av1Enabled = 0;
        vcenc_instance->featureToSupport.vp9Enabled = 0;
    }
    memset(&sSliceRpsCfg, 0, sizeof(VCEncGopPicConfig));

    asic->regs.roiMapDeltaQpAddr = pEncIn->roiMapDeltaQpAddr;
    asic->regs.RoimapCuCtrlAddr = pEncIn->RoimapCuCtrlAddr;
    asic->regs.RoimapCuCtrlIndexAddr = pEncIn->RoimapCuCtrlIndexAddr;
    asic->regs.RoimapCuCtrl_enable = vcenc_instance->RoimapCuCtrl_enable;
    asic->regs.RoimapCuCtrl_index_enable = vcenc_instance->RoimapCuCtrl_index_enable;
    asic->regs.RoimapCuCtrl_ver = vcenc_instance->RoimapCuCtrl_ver;
    asic->regs.RoiQpDelta_ver = vcenc_instance->RoiQpDelta_ver;

    true_e secondMap = (vcenc_instance->RoimapCuCtrl_ver >= 4) &&
                       vcenc_instance->RoimapCuCtrl_enable && (pEncIn->RoimapCuCtrlAddr != 0);
    if ((((asic->regs.rcRoiEnable & 2) && (vcenc_instance->RoimapCuCtrl_enable == 0)) ||
         ((ENCHW_NO == secondMap) && (asic->regs.ipcmMapEnable || asic->regs.skipMapEnable)) ||
         asic->regs.rdoqMapEnable) &&
        (pEncIn->roiMapDeltaQpAddr == 0) && (2 != vcenc_instance->pass)) {
        APITRACEERR("VCEncStrmEncode: Invalid roi map\n");
        return VCENC_INVALID_ARGUMENT;
    }

    if (vcenc_instance->RoimapCuCtrl_enable && (pEncIn->RoimapCuCtrlAddr == 0)) {
        APITRACEERR("VCEncStrmEncode: Invalid second roi map\n");
        return VCENC_INVALID_ARGUMENT;
    }

    asic->regs.sram_base_lum_fwd = pEncIn->extSRAMLumFwdBase;
    asic->regs.sram_base_lum_bwd = pEncIn->extSRAMLumBwdBase;
    asic->regs.sram_base_chr_fwd = pEncIn->extSRAMChrFwdBase;
    asic->regs.sram_base_chr_bwd = pEncIn->extSRAMChrBwdBase;

    if (vcenc_instance->pass == 1) {
        vcenc_instance->stream.stream = (u8 *)vcenc_instance->lookahead.internal_mem.pOutBuf;
        vcenc_instance->stream.stream_bus = vcenc_instance->lookahead.internal_mem.busOutBuf;
        vcenc_instance->stream.size = vcenc_instance->lookahead.internal_mem.outBufSize;
    } else {
        vcenc_instance->stream.stream = (u8 *)pEncIn->pOutBuf[0];
        vcenc_instance->stream.stream_bus = pEncIn->busOutBuf[0];
        vcenc_instance->stream.size = pEncIn->outBufSize[0];
    }
    vcenc_instance->stream.av1pre_size = asic->av1PreCarryBuf[cur_core_index].size;
    vcenc_instance->stream.stream_av1pre_bus = asic->av1PreCarryBuf[cur_core_index].busAddress;
    vcenc_instance->stream.cnt = &vcenc_instance->stream.byteCnt;

    pEncOut->tileExtra = vcenc_instance->EncOut[cur_core_index].tileExtra;
    pEncOut->pNaluSizeBuf = (u32 *)vcenc_instance->asic
                                .sizeTbl[vcenc_instance->num_tile_columns > 1 ? 0 : cur_core_index]
                                .virtualAddress;
    pEncOut->streamSize = 0;
    pEncOut->numNalus = 0;

    /* Check input bus address
     when pEncIn->tileExtra[0].busLuma is filled, we default that user fills in all input addresses. */
    if (vcenc_instance->num_tile_columns > 1 && pEncIn->tileExtra[0].busLuma) {
        for (tileId = 1; tileId < vcenc_instance->num_tile_columns; tileId++) {
            u32 lumaAddress = pEncIn->tileExtra[tileId - 1].busLuma ? 1 : 0;
            u32 chromaUAddress = pEncIn->tileExtra[tileId - 1].busChromaU ? 1 : 0;
            u32 chromaVAddress = pEncIn->tileExtra[tileId - 1].busChromaV ? 1 : 0;
            //Not all addresses are filled in the tile column
            if ((lumaAddress + chromaUAddress + chromaVAddress) != 3) {
                APITRACEERR("VCEncStrmStart: ERROR Too few input address for multi-tile\n");
                return VCENC_INVALID_ARGUMENT;
            }
        }
    }

    u32 defaultInputFlag =
        (vcenc_instance->num_tile_columns > 1 && pEncIn->tileExtra[0].busLuma) ? 0 : 1;
    for (tileId = 1; tileId < vcenc_instance->num_tile_columns; tileId++) {
        u32 index = asic->regs.tiles_enabled_flag ? tileId : cur_core_index;
        pEncOut->tileExtra[tileId - 1].pNaluSizeBuf =
            (u32 *)vcenc_instance->asic.sizeTbl[index].virtualAddress;

        pEncOut->tileExtra[tileId - 1].streamSize = 0;
        pEncOut->tileExtra[tileId - 1].numNalus = 0;

        /* when pEncIn->tileExtra[0].busLuma is NULL, we have to fill in input bus address and preprocess paremeters
       for the second tile to the last tile*/
        if (vcenc_instance->num_tile_columns > 1 && defaultInputFlag) {
            pEncIn->tileExtra[tileId - 1].busLuma = pEncIn->busLuma;
            pEncIn->tileExtra[tileId - 1].busChromaU = pEncIn->busChromaU;
            pEncIn->tileExtra[tileId - 1].busChromaV = pEncIn->busChromaV;
            vcenc_instance->preProcess.horOffsetSrc[tileId] =
                vcenc_instance->preProcess.horOffsetSrc[tileId - 1] +
                64 * (vcenc_instance->tileCtrl[tileId - 1].tileRight -
                      vcenc_instance->tileCtrl[tileId - 1].tileLeft + 1);
            vcenc_instance->preProcess.verOffsetSrc[tileId] =
                vcenc_instance->preProcess.verOffsetSrc[0];
            vcenc_instance->preProcess.lumWidthSrc[tileId] =
                vcenc_instance->preProcess.lumWidthSrc[0];
            vcenc_instance->preProcess.lumHeightSrc[tileId] =
                vcenc_instance->preProcess.lumHeightSrc[0];
        }
    }
    pEncOut->maxSliceStreamSize = 0;
    pEncOut->codingType = VCENC_NOTCODED_FRAME;

    vcenc_instance->sliceReadyCbFunc = sliceReadyCbFunc;
    vcenc_instance->pAppData = pAppData;
    for (i = 0; i < MAX_STRM_BUF_NUM; i++) {
        vcenc_instance->streamBufs[cur_core_index].buf[i] = (u8 *)pEncIn->pOutBuf[i];
        vcenc_instance->streamBufs[cur_core_index].bufLen[i] = pEncIn->outBufSize[i];
    }

    /* low latency */
    vcenc_instance->inputLineBuf.wrCnt = pEncIn->lineBufWrCnt;
    vcenc_instance->inputLineBuf.initSegNum = pEncIn->initSegNum;

    /* Clear the NAL unit size table */
    if (pEncOut->pNaluSizeBuf != NULL)
        pEncOut->pNaluSizeBuf[0] = 0;
    for (tileId = 1; tileId < vcenc_instance->num_tile_columns; tileId++) {
        if (pEncOut->tileExtra[tileId - 1].pNaluSizeBuf != NULL)
            pEncOut->tileExtra[tileId - 1].pNaluSizeBuf[0] = 0;
    }

    vcenc_instance->stream.byteCnt = 0;
    pEncOut->sliceHeaderSize = 0;
    pEncOut->PreDataSize = 0;
    pEncOut->PreNaluNum = 0;

    asic->regs.outputStrmBase[0] = (ptr_t)vcenc_instance->stream.stream_bus;
    asic->regs.outputStrmSize[0] = vcenc_instance->stream.size;
    asic->regs.Av1PreoutputStrmBase = (ptr_t)vcenc_instance->stream.stream_av1pre_bus;
    asic->regs.Av1PreBufferSize = vcenc_instance->stream.av1pre_size;
    if (asic->regs.asicCfg.streamBufferChain) {
        asic->regs.outputStrmBase[1] = pEncIn->busOutBuf[1];
        asic->regs.outputStrmSize[1] = pEncIn->outBufSize[1];
    }

    /* load output buf address to tileCtrl */
    for (tileId = 0; tileId < vcenc_instance->num_tile_columns; tileId++) {
        vcenc_instance->tileCtrl[tileId].busOutBuf =
            (tileId == 0) ? pEncIn->busOutBuf[0] : pEncIn->tileExtra[tileId - 1].busOutBuf[0];
        vcenc_instance->tileCtrl[tileId].outBufSize =
            (tileId == 0) ? pEncIn->outBufSize[0] : pEncIn->tileExtra[tileId - 1].outBufSize[0];
        vcenc_instance->tileCtrl[tileId].pOutBuf =
            (tileId == 0) ? pEncIn->pOutBuf[0] : pEncIn->tileExtra[tileId - 1].pOutBuf[0];
    }

    if (!(c = get_container(vcenc_instance)))
        return VCENC_ERROR;
    if ((IS_HEVC(vcenc_instance->codecFormat) || IS_H264(vcenc_instance->codecFormat))) {
        VCEncParameterSetHandle(vcenc_instance, inst, pEncIn, pEncOut, c);
    }

    vcenc_instance->poc = pEncIn->poc;

    pEncOut->busScaledLuma = vcenc_instance->asic.scaledImage.busAddress;
    pEncOut->scaledPicture = (u8 *)vcenc_instance->asic.scaledImage.virtualAddress;
    /* Get parameter sets */
    v = (struct vps *)get_parameter_set(c, VPS_NUT, 0);
    s = (struct sps *)get_parameter_set(c, SPS_NUT, vcenc_instance->sps_id);
    p = (struct pps *)get_parameter_set(c, PPS_NUT, vcenc_instance->pps_id);

    bool use_ltr_cur; // = pEncOut->boolCurIsLongTermRef = (pEncIn->u8IdxEncodedAsLTR != 0);
    if (!v || !s || !p)
        return VCENC_ERROR;

    u32 ltr_used_by_cur[VCENC_MAX_LT_REF_FRAMES];

    if (pEncIn->flexRefsEnable) {
        vcenc_instance->flexRefsEnable = pEncIn->flexRefsEnable;
        if (IS_H264(vcenc_instance->codecFormat))
            rpsInSliceHeader = 0;
        else if (IS_HEVC(vcenc_instance->codecFormat))
            rpsInSliceHeader = 1;
        codingType = pEncIn->codingType;
        vcenc_instance->rps_id = s->num_short_term_ref_pic_sets;
        vcenc_replace_rps(vcenc_instance, NULL, (VCEncGopPicConfig *)&(pEncIn->gopCurrPicConfig),
                          0);
        r = (struct rps *)get_parameter_set(c, RPS, vcenc_instance->rps_id);
        use_ltr_cur = (pEncOut->boolCurIsLongTermRef = (r->num_lt_pics != 0));
        if (use_ltr_cur) {
            for (i = 0; i < r->num_lt_pics; i++) {
                r->long_term_ref_pic_poc[i] = r->ref_pic_lt[i].delta_poc;
                r->ref_pic_lt[i].delta_poc = i;
                ltr_used_by_cur[i] = r->ref_pic_lt[i].used_by_curr_pic;
            }
            for (i = r->num_lt_pics; i < VCENC_MAX_LT_REF_FRAMES; i++) {
                r->long_term_ref_pic_poc[i] = -1;
                ltr_used_by_cur[i] = -1;
            }
        } else {
            for (i = 0; i < VCENC_MAX_LT_REF_FRAMES; i++) {
                r->long_term_ref_pic_poc[i] = -1;
                ltr_used_by_cur[i] = 0;
            }
        }

        if (IS_H264(vcenc_instance->codecFormat)) {
            vcenc_replace_rps(vcenc_instance, NULL,
                              (VCEncGopPicConfig *)&(pEncIn->gopNextPicConfig), 1);
            r2 = (struct rps *)get_parameter_set(c, RPS, vcenc_instance->rps_id);

            if (r2->num_lt_pics != 0) {
                for (i = 0; i < r2->num_lt_pics; i++) {
                    r2->long_term_ref_pic_poc[i] = r2->ref_pic_lt[i].delta_poc;
                    r2->ref_pic_lt[i].delta_poc = i;
                    //ltr_used_by_cur[i] = r2->ref_pic_lt[i].used_by_curr_pic;
                }
                for (i = r2->num_lt_pics; i < VCENC_MAX_LT_REF_FRAMES; i++) {
                    r2->long_term_ref_pic_poc[i] = -1;
                    //ltr_used_by_cur[i] = -1;
                }
            } else {
                for (i = 0; i < VCENC_MAX_LT_REF_FRAMES; i++) {
                    r2->long_term_ref_pic_poc[i] = -1;
                    //ltr_used_by_cur[i] = -1;
                }
            }

            delta_poc = pEncIn->gopNextPicConfig.poc - pEncIn->gopCurrPicConfig.poc;
        }
    } else {
        use_ltr_cur = (pEncOut->boolCurIsLongTermRef = (pEncIn->u8IdxEncodedAsLTR != 0));
        codingType = get_rps_id(vcenc_instance, pEncIn, s, &rpsInSliceHeader);
        r = (struct rps *)get_parameter_set(c, RPS, vcenc_instance->rps_id);

        /* Initialize before/after/follow poc list */
        if ((codingType == VCENC_INTRA_FRAME) && (vcenc_instance->poc == 0)) /* IDR */
            j = 0;
        else {
            for (i = 0, j = 0; i < VCENC_MAX_LT_REF_FRAMES; i++) {
                if (pEncIn->long_term_ref_pic[i] != INVALITED_POC)
                    r->long_term_ref_pic_poc[j++] = pEncIn->long_term_ref_pic[i];
            }
        }
        for (i = j; i < VCENC_MAX_LT_REF_FRAMES; i++)
            r->long_term_ref_pic_poc[i] = -1;

        for (i = 0; i < VCENC_MAX_LT_REF_FRAMES; i++) {
            ltr_used_by_cur[i] = pEncIn->bLTR_used_by_cur[i];
        }
    }

    if (!r)
        return VCENC_ERROR;

    init_poc_list(r, vcenc_instance->poc, use_ltr_cur, codingType, c,
                  IS_H264(vcenc_instance->codecFormat), ltr_used_by_cur, &rpsInSliceHeader);
    /* 6.0 not support */
    if (vcenc_instance->asic.regs.asicHwId < 0x80006010)
        r->lt_follow_cnt = 0;

    if (r->before_cnt + r->after_cnt + r->lt_current_cnt == 1)
        codingType = VCENC_PREDICTED_FRAME;

    /* mark unused picture, so that you can reuse those. TODO: IDR flush */
    if (GenralRefPicMarking(vcenc_instance, c, r, codingType)) {
        Error(2, ERR, "RPS error: reference picture(s) not available");
        return VCENC_ERROR;
    }

    /* Get free picture (if any) and remove it from picture store */
    if (!(pic = get_picture(c, -1))) {
        if (!(pic = create_picture_ctrlsw(vcenc_instance, v, s, p, asic->regs.sliceSize)))
            return VCENC_ERROR;
    }
    create_slices_ctrlsw(pic, p, asic->regs.sliceSize);
    queue_remove(&c->picture, (struct node *)pic);

    pic->encOrderInGop = pEncIn->gopPicIdx;
    pic->h264_no_ref = HANTRO_FALSE;
    if (pic->pps != p)
        pic->pps = p;

    /* store input addr */
    for (tileId = 0; tileId < vcenc_instance->num_tile_columns; tileId++) {
        pic->input[tileId].lum =
            (tileId == 0) ? pEncIn->busLuma : pEncIn->tileExtra[tileId - 1].busLuma;
        pic->input[tileId].cb =
            (tileId == 0) ? pEncIn->busChromaU : pEncIn->tileExtra[tileId - 1].busChromaU;
        pic->input[tileId].cr =
            (tileId == 0) ? pEncIn->busChromaV : pEncIn->tileExtra[tileId - 1].busChromaV;
    }

    if (pic->picture_memory_init == 0) {
        pic->recon.lum = asic->internalreconLuma[pic->picture_memory_id].busAddress;
        pic->recon.cb = asic->internalreconChroma[pic->picture_memory_id].busAddress;
        pic->recon.cr = pic->recon.cb + asic->regs.recon_chroma_half_size;
        pic->recon_4n_base = asic->internalreconLuma_4n[pic->picture_memory_id].busAddress;
        pic->framectx_base = asic->internalFrameContext[pic->picture_memory_id].busAddress;
        pic->framectx_virtual_addr =
            (ptr_t)asic->internalFrameContext[pic->picture_memory_id].virtualAddress;
        pic->mc_sync_addr = asic->mc_sync_word[pic->picture_memory_id].busAddress;
        pic->mc_sync_ptr = (ptr_t)asic->mc_sync_word[pic->picture_memory_id].virtualAddress;

        if (useExtFlag && vcenc_instance->parallelCoreNum > 1) {
            pic->mc_sync_addr = pEncExtParaIn->recon.mc_sync_pa;
            pic->mc_sync_ptr = (ptr_t)pEncExtParaIn->recon.mc_sync_va;
        }
        /* for compress */
        {
            u32 lumaTblSize =
                (vcenc_instance->compressor & 1)
                    ? (((pic->sps->width + 63) / 64) * ((pic->sps->height + 63) / 64) * 8)
                    : 0;
            lumaTblSize = ((lumaTblSize + 15) >> 4) << 4;
            pic->recon_compress.lumaTblBase = asic->compressTbl[pic->picture_memory_id].busAddress;
            pic->recon_compress.chromaTblBase = (pic->recon_compress.lumaTblBase + lumaTblSize);
        }

        /* for H.264 collocated mb */
        pic->colctbs_store =
            (struct h264_mb_col *)asic->colBuffer[pic->picture_memory_id].virtualAddress;
        pic->colctbs_store_base = asic->colBuffer[pic->picture_memory_id].busAddress;

        /* To collect MV info for TMVP */
        pic->mvInfoBase = asic->mvInfo[pic->picture_memory_id].busAddress;
        pic->picture_memory_init = 1;
    }

    // cuInfo buffer layout:
    //   cuTable (size = asic.cuInfoTableSize)
    //   aqData  (size = asic.aqInfoSize)
    //   cuData

    /* cu info output */
    pic->cuInfoTableBase = asic->cuInfoMem[vcenc_instance->cuInfoBufIdx].busAddress;
    pic->cuInfoDataBase = pic->cuInfoTableBase + asic->cuInfoTableSize + asic->aqInfoSize;

    if (vcenc_instance->pass == 1)
        waitCuInfoBufPass1(vcenc_instance);
    if (asic->regs.enableOutputCuInfo) {
        i32 cuInfoSizes[] = {CU_INFO_OUTPUT_SIZE, CU_INFO_OUTPUT_SIZE_V1, CU_INFO_OUTPUT_SIZE_V2,
                             CU_INFO_OUTPUT_SIZE_V3};
        i32 cuInfoVersion = asic->regs.asicCfg.cuInforVersion;
        i32 infoSizePerCu = cuInfoSizes[cuInfoVersion];
        u32 *tableBase = (u32 *)asic->cuInfoMem[vcenc_instance->cuInfoBufIdx].virtualAddress;
        u8 *dataBase = (u8 *)asic->cuInfoMem[vcenc_instance->cuInfoBufIdx].virtualAddress +
                       asic->cuInfoTableSize + asic->aqInfoSize;
        for (tileId = 0; tileId < vcenc_instance->num_tile_columns; tileId++) {
            if (tileId == 0) {
                pEncOut->cuOutData.ctuOffset =
                    tableBase + ((vcenc_instance->tileCtrl[tileId].tileLeft *
                                  (vcenc_instance->height + 63) / 64) *
                                 CU_INFO_TABLE_ITEM_SIZE) /
                                    sizeof(u32);
                pEncOut->cuOutData.cuData =
                    dataBase + ((vcenc_instance->tileCtrl[tileId].tileLeft * 64 / 8) *
                                ((vcenc_instance->height + 63) / 64 * 64 / 8)) *
                                   infoSizePerCu / sizeof(u8);
            } else {
                pEncOut->tileExtra[tileId - 1].cuOutData.ctuOffset =
                    tableBase + ((vcenc_instance->tileCtrl[tileId].tileLeft *
                                  (vcenc_instance->height + 63) / 64) *
                                 CU_INFO_TABLE_ITEM_SIZE) /
                                    sizeof(u32);
                pEncOut->tileExtra[tileId - 1].cuOutData.cuData =
                    dataBase + ((vcenc_instance->tileCtrl[tileId].tileLeft * 64 / 8) *
                                ((vcenc_instance->height + 63) / 64 * 64 / 8)) *
                                   infoSizePerCu / sizeof(u8);
            }

            vcenc_instance->tileCtrl[tileId].cuinfoTableBase =
                pic->cuInfoTableBase +
                (vcenc_instance->tileCtrl[tileId].tileLeft * (vcenc_instance->height + 63) / 64) *
                    CU_INFO_TABLE_ITEM_SIZE;
            vcenc_instance->tileCtrl[tileId].cuinfoDataBase =
                pic->cuInfoDataBase + ((vcenc_instance->tileCtrl[tileId].tileLeft * 64 / 8) *
                                       ((vcenc_instance->height + 63) / 64 * 64 / 8)) *
                                          infoSizePerCu;
        }
    } else {
        memset(&pEncOut->cuOutData, 0, sizeof(VCEncCuOutData));
        for (tileId = 1; tileId < vcenc_instance->num_tile_columns; tileId++)
            memset(&pEncOut->tileExtra[tileId - 1].cuOutData, 0, sizeof(VCEncCuOutData));
    }
    vcenc_instance->cuInfoBufIdx++;
    if (vcenc_instance->cuInfoBufIdx == vcenc_instance->numCuInfoBuf)
        vcenc_instance->cuInfoBufIdx = 0;

    //aq information output
    asic->regs.aqInfoOutputMode =
        (vcenc_instance->pass == 1 ? vcenc_instance->cuTreeCtl.aq_mode : AQ_NONE);
    if (asic->regs.aqInfoOutputMode != AQ_NONE) {
        u32 aqStrength =
            (u32)(vcenc_instance->cuTreeCtl.aqStrength * (1 << AQ_STRENGTH_SCALE_BITS)); // to Q2.7
        asic->regs.aqStrength = aqStrength;
        asic->regs.aqInfoOutputBase = pic->cuInfoTableBase + asic->cuInfoTableSize;
        asic->regs.aqInfoOutputStride = asic->aqInfoStride;
    }
    //ctb bits info output; Pass 1 turn off ctb bits output enable
    /* Ctb encoded bits output */
    if (asic->regs.enableOutputCtbBits) {
        pic->ctbBitsDataBase = asic->ctbBitsMem[cur_core_index].busAddress;
        pEncOut->ctbBitsData = (u16 *)asic->ctbBitsMem[cur_core_index].virtualAddress;
    } else {
        pic->ctbBitsDataBase = 0;
        pEncOut->ctbBitsData = NULL;
    }

    /* Activate reference paremater sets and reference picture set,
   * TODO: if parameter sets if id differs... */
    pic->rps = r;
    ASSERT(pic->vps == v);
    ASSERT(pic->sps == s);
    ASSERT(pic->pps == p);
    pic->poc = vcenc_instance->poc;

    /* configure GDR parameters */
    StrmEncodeGradualDecoderRefresh(vcenc_instance, asic, pEncIn, &codingType, cfg);

    /* setting TRAIL_N/TRAIL_R based on nonReference setting */
    pic->temporal_id = pEncIn->gopCurrPicConfig.temporalId;
    u32 hevc_nal_unit_type = pEncIn->gopCurrPicConfig.nonReference ? TRAIL_N : TRAIL_R;
    if (codingType == VCENC_INTRA_FRAME) {
        asic->regs.frameCodingType = 1;
        asic->regs.nal_unit_type = (IS_H264(vcenc_instance->codecFormat) ? H264_IDR : IDR_W_RADL);
        if (vcenc_instance->poc > 0 && !pEncIn->bIsIDR)
            asic->regs.nal_unit_type =
                (IS_H264(vcenc_instance->codecFormat) ? H264_NONIDR : TRAIL_R);
        pic->sliceInst->type = I_SLICE;
        pEncOut->codingType = VCENC_INTRA_FRAME;
    } else if (codingType == VCENC_PREDICTED_FRAME) {
        asic->regs.frameCodingType = 0;
        asic->regs.nal_unit_type =
            (IS_H264(vcenc_instance->codecFormat) ? H264_NONIDR : hevc_nal_unit_type);
        pic->sliceInst->type = P_SLICE;
        pEncOut->codingType = VCENC_PREDICTED_FRAME;
    } else if (codingType == VCENC_BIDIR_PREDICTED_FRAME) {
        asic->regs.frameCodingType = 2;
        asic->regs.nal_unit_type =
            (IS_H264(vcenc_instance->codecFormat) ? H264_NONIDR : hevc_nal_unit_type);
        pic->sliceInst->type = B_SLICE;
        pEncOut->codingType = VCENC_BIDIR_PREDICTED_FRAME;
    }

    asic->regs.hashtype = vcenc_instance->hashctx.hash_type;
    hash_getstate(&vcenc_instance->hashctx, &asic->regs.hashval, &asic->regs.hashoffset);

    /* Generate reference picture list of current picture using active
   * reference picture set */
    reference_picture_list(c, pic);

    if (pic->sliceInst->active_l0_cnt > 1) {
        //check buildId
        if (cfg.maxRefNumList0Minus1 < 1) {
            APITRACEERR("VCEncStrmEncodeExt: The buildID[0x%x] not support multi-reference "
                        "for P.\n",
                        cfg.hw_build_id);
            return VCENC_ERROR;
        }
        //Check codec format. Only hevc and h264 support multi-reference for P now.
        if ((vcenc_instance->codecFormat != VCENC_VIDEO_CODEC_HEVC) &&
            (vcenc_instance->codecFormat != VCENC_VIDEO_CODEC_H264)) {
            APITRACEERR("VCEncStrmEncodeExt: The codecFormat not support multi-reference for "
                        "P now.\n");
            return VCENC_ERROR;
        }
    }
    //Check reference frame number. support at most 2 reference frame now
    if ((pic->sliceInst->active_l0_cnt + pic->sliceInst->active_l1_cnt) > 2) {
        APITRACEERR("VCEncStrmEncodeExt: Illegal reference frame numbers\n");
        return VCENC_ERROR;
    }

    vcenc_instance->sHwRps.u1short_term_ref_pic_set_sps_flag = rpsInSliceHeader ? 0 : 1;
    vcenc_instance->asic.regs.short_term_ref_pic_set_sps_flag =
        vcenc_instance->sHwRps.u1short_term_ref_pic_set_sps_flag;
    if (rpsInSliceHeader) {
        VCEncGenPicRefConfig(c, &sSliceRpsCfg, pic, vcenc_instance->poc);
        VCEncGenSliceHeaderRps(vcenc_instance, codingType, &sSliceRpsCfg);
    }

    pic->colctbs = NULL;
    pic->colctbs_load_base = 0;
    if (IS_H264(vcenc_instance->codecFormat) && codingType == VCENC_BIDIR_PREDICTED_FRAME) {
        pic->colctbs = pic->rpl[1][0]->colctbs_store;
        pic->colctbs_load_base = pic->rpl[1][0]->colctbs_store_base;
    }
    if (pic->sliceInst->type != I_SLICE ||
        ((vcenc_instance->poc > 0) && (!IS_H264(vcenc_instance->codecFormat)))) // check only hevc?
    {
        int refCnt = pic->sliceInst->active_l0_cnt + pic->sliceInst->active_l1_cnt;
        int used_cnt = 0;
        //H2V2 limit:
        //List lengh is no more than 2
        //number of active ref is no more than 1
        ASSERT(refCnt + r->lt_follow_cnt <= VCENC_MAX_REF_FRAMES);

        asic->regs.l0_used_by_curr_pic[0] = 0;
        asic->regs.l0_used_by_curr_pic[1] = 0;
        asic->regs.l1_used_by_curr_pic[0] = 0;
        asic->regs.l1_used_by_curr_pic[1] = 0;
        asic->regs.l0_long_term_flag[0] = 0;
        asic->regs.l0_long_term_flag[1] = 0;
        asic->regs.l1_long_term_flag[0] = 0;
        asic->regs.l1_long_term_flag[1] = 0;
        asic->regs.num_long_term_pics = 0;

        /* 6.0 use ref_cnt */
        used_cnt = (vcenc_instance->asic.regs.asicHwId < 0x80006010)
                       ? refCnt
                       : pic->sliceInst->active_l0_cnt;
        for (i = 0; i < used_cnt; i++)
            asic->regs.l0_used_by_curr_pic[i] = 1;

        for (i = 0; i < pic->sliceInst->active_l0_cnt; i++) {
            asic->regs.pRefPic_recon_l0[0][i] = pic->rpl[0][i]->recon.lum;
            asic->regs.pRefPic_recon_l0[1][i] = pic->rpl[0][i]->recon.cb;
            asic->regs.pRefPic_recon_l0[2][i] = pic->rpl[0][i]->recon.cr;
            asic->regs.pRefPic_recon_l0_4n[i] = pic->rpl[0][i]->recon_4n_base;

            asic->regs.l0_delta_poc[i] = pic->rpl[0][i]->poc - pic->poc;
            asic->regs.l0_long_term_flag[i] = pic->rpl[0][i]->long_term_flag;
            asic->regs.num_long_term_pics += asic->regs.l0_long_term_flag[i];

            //compress
            asic->regs.ref_l0_luma_compressed[i] = pic->rpl[0][i]->recon_compress.lumaCompressed;
            asic->regs.ref_l0_luma_compress_tbl_base[i] =
                pic->rpl[0][i]->recon_compress.lumaTblBase;

            asic->regs.ref_l0_chroma_compressed[i] =
                pic->rpl[0][i]->recon_compress.chromaCompressed;
            asic->regs.ref_l0_chroma_compress_tbl_base[i] =
                pic->rpl[0][i]->recon_compress.chromaTblBase;
        }

        if (pic->sliceInst->type == B_SLICE) {
            /* 6.0 use ref_cnt */
            used_cnt = (vcenc_instance->asic.regs.asicHwId < 0x80006010)
                           ? refCnt
                           : pic->sliceInst->active_l1_cnt;
            for (i = 0; i < used_cnt; i++)
                asic->regs.l1_used_by_curr_pic[i] = 1;

            for (i = 0; i < pic->sliceInst->active_l1_cnt; i++) {
                asic->regs.pRefPic_recon_l1[0][i] = pic->rpl[1][i]->recon.lum;
                asic->regs.pRefPic_recon_l1[1][i] = pic->rpl[1][i]->recon.cb;
                asic->regs.pRefPic_recon_l1[2][i] = pic->rpl[1][i]->recon.cr;
                asic->regs.pRefPic_recon_l1_4n[i] = pic->rpl[1][i]->recon_4n_base;

                asic->regs.l1_delta_poc[i] = pic->rpl[1][i]->poc - pic->poc;
                asic->regs.l1_long_term_flag[i] = pic->rpl[1][i]->long_term_flag;
                asic->regs.num_long_term_pics += asic->regs.l1_long_term_flag[i];

                //compress
                asic->regs.ref_l1_luma_compressed[i] =
                    pic->rpl[1][i]->recon_compress.lumaCompressed;
                asic->regs.ref_l1_luma_compress_tbl_base[i] =
                    pic->rpl[1][i]->recon_compress.lumaTblBase;

                asic->regs.ref_l1_chroma_compressed[i] =
                    pic->rpl[1][i]->recon_compress.chromaCompressed;
                asic->regs.ref_l1_chroma_compress_tbl_base[i] =
                    pic->rpl[1][i]->recon_compress.chromaTblBase;
            }
        }
        if (!IS_H264(vcenc_instance->codecFormat)) {
            for (i = pic->sliceInst->active_l0_cnt, j = 0; i < 2 && j < r->lt_follow_cnt;
                 i++, j++) {
                asic->regs.l0_delta_poc[i] = r->lt_follow[j] - pic->poc;
                asic->regs.l0_long_term_flag[i] = 1;
            }
            for (i = pic->sliceInst->active_l1_cnt; i < 2 && j < r->lt_follow_cnt; i++, j++) {
                asic->regs.l1_delta_poc[i] = r->lt_follow[j] - pic->poc;
                asic->regs.l1_long_term_flag[i] = 1;
            }
        }
        asic->regs.num_long_term_pics += r->lt_follow_cnt;
    }
    if (IS_H264(vcenc_instance->codecFormat)) {
        asic->regs.colctbs_load_base = (ptr_t)pic->colctbs_load_base;
        asic->regs.colctbs_store_base = (ptr_t)pic->colctbs_store_base;
    }

    asic->regs.recon_luma_compress_tbl_base = pic->recon_compress.lumaTblBase;
    asic->regs.recon_chroma_compress_tbl_base = pic->recon_compress.chromaTblBase;

    asic->regs.active_l0_cnt = pic->sliceInst->active_l0_cnt;
    asic->regs.active_l1_cnt = pic->sliceInst->active_l1_cnt;
    asic->regs.active_override_flag = pic->sliceInst->active_override_flag;

    asic->regs.lists_modification_present_flag = pic->pps->lists_modification_present_flag;
    asic->regs.ref_pic_list_modification_flag_l0 =
        pic->sliceInst->ref_pic_list_modification_flag_l0;
    asic->regs.list_entry_l0[0] = pic->sliceInst->list_entry_l0[0];
    asic->regs.list_entry_l0[1] = pic->sliceInst->list_entry_l0[1];
    asic->regs.ref_pic_list_modification_flag_l1 =
        pic->sliceInst->ref_pic_list_modification_flag_l1;
    asic->regs.list_entry_l1[0] = pic->sliceInst->list_entry_l1[0];
    asic->regs.list_entry_l1[1] = pic->sliceInst->list_entry_l1[1];

    asic->regs.log2_max_pic_order_cnt_lsb = pic->sps->log2_max_pic_order_cnt_lsb;
    asic->regs.log2_max_frame_num = pic->sps->log2MaxFrameNumMinus4 + 4;
    asic->regs.pic_order_cnt_type = pic->sps->picOrderCntType;

    //cu info output
    asic->regs.cuInfoTableBase = pic->cuInfoTableBase;
    asic->regs.cuInfoDataBase = pic->cuInfoDataBase;

    //register dump
    asic->dumpRegister = vcenc_instance->dumpRegister;

    //write recon to ddr
    asic->regs.writeReconToDDR = vcenc_instance->writeReconToDDR;

    vcenc_instance->rateControl.hierarchical_bit_allocation_GOP_size = pEncIn->gopSize;
    /* Rate control */
    if (pic->sliceInst->type == I_SLICE) {
        vcenc_instance->rateControl.gopPoc = 0;
        vcenc_instance->rateControl.encoded_frame_number = 0;
    } else {
        vcenc_instance->rateControl.frameQpDelta = pEncIn->gopCurrPicConfig.QpOffset
                                                   << QP_FRACTIONAL_BITS;
        vcenc_instance->rateControl.gopPoc = pEncIn->gopCurrPicConfig.poc;
        vcenc_instance->rateControl.encoded_frame_number = pEncIn->gopPicIdx;
        if (vcenc_instance->rateControl.gopPoc > 0)
            vcenc_instance->rateControl.gopPoc -= 1;
    }
    /* picSkip not supported for 2-pass or gopSize > 1 or multi-core */
    vcenc_instance->rateControl.picSkipAllowed =
        (vcenc_instance->pass == 0) && (vcenc_instance->parallelCoreNum == 1) &&
        ((pic->sliceInst->type == I_SLICE) || (pEncIn->gopSize == 1));

    //CTB_RC
    vcenc_instance->rateControl.ctbRcBitsMin = asic->regs.ctbBitsMin;
    vcenc_instance->rateControl.ctbRcBitsMax = asic->regs.ctbBitsMax;
    vcenc_instance->rateControl.ctbRctotalLcuBit = asic->regs.totalLcuBits;

    if (codingType == VCENC_INTRA_FRAME) {
        vcenc_instance->rateControl.qpMin = vcenc_instance->rateControl.qpMinI;
        vcenc_instance->rateControl.qpMax = vcenc_instance->rateControl.qpMaxI;
    } else {
        vcenc_instance->rateControl.qpMin = vcenc_instance->rateControl.qpMinPB;
        vcenc_instance->rateControl.qpMax = vcenc_instance->rateControl.qpMaxPB;
    }
    vcenc_instance->rateControl.inputSceneChange = pEncIn->sceneChange;

    asic->regs.ctbRcQpDeltaReverse = vcenc_instance->rateControl.ctbRcQpDeltaReverse;
    asic->regs.rcQpDeltaRange = vcenc_instance->rateControl.rcQpDeltaRange;
    asic->regs.rcBaseMBComplexity = vcenc_instance->rateControl.rcBaseMBComplexity;

    if (vcenc_instance->pass == 2) {
        i32 i;
        i32 nBlk = (vcenc_instance->width / 8) * (vcenc_instance->height / 8);
        i32 costScale = 4;
        for (i = 0; i < 4; i++) {
            vcenc_instance->rateControl.pass1CurCost =
                vcenc_instance->cuTreeCtl.costCur * nBlk / costScale;
            vcenc_instance->rateControl.pass1AvgCost[i] =
                vcenc_instance->cuTreeCtl.costAvg[i] * nBlk / costScale;
            vcenc_instance->rateControl.pass1FrameNum[i] =
                vcenc_instance->cuTreeCtl.FrameTypeNum[i];

            vcenc_instance->rateControl.pass1GopCost[i] =
                vcenc_instance->cuTreeCtl.costGop[i] * nBlk / costScale;
            vcenc_instance->rateControl.pass1GopFrameNum[i] =
                vcenc_instance->cuTreeCtl.FrameNumGop[i];
        }
        vcenc_instance->rateControl.predId = getFramePredId(vcenc_instance->cuTreeCtl.curTypeChar);
    }

    asic->regs.bSkipCabacEnable = vcenc_instance->bSkipCabacEnable;
    asic->regs.inLoopDSRatio = vcenc_instance->preProcess.inLoopDSRatio;
    asic->regs.bMotionScoreEnable = vcenc_instance->bMotionScoreEnable;

    VCEncBeforePicRc(&vcenc_instance->rateControl, pEncIn->timeIncrement, pic->sliceInst->type,
                     use_ltr_cur, pic);

    asic->regs.bitsRatio = vcenc_instance->rateControl.bitsRatio;

    asic->regs.ctbRcThrdMin = vcenc_instance->rateControl.ctbRcThrdMin;
    asic->regs.ctbRcThrdMax = vcenc_instance->rateControl.ctbRcThrdMax;
    asic->regs.rcRoiEnable |= ((vcenc_instance->rateControl.ctbRc & 3) << 2);

#if 0
  printf("/***********sw***********/\nvcenc_instance->rateControl.ctbRc=%d,asic->regs.ctbRcBitMemAddrCur=0x%x,asic->regs.ctbRcBitMemAddrPre=0x%x\n", vcenc_instance->rateControl.ctbRc, asic->regs.ctbRcBitMemAddrCur, asic->regs.ctbRcBitMemAddrPre);
  printf("asic->regs.bitsRatio=%d,asic->regs.ctbRcThrdMin=%d,asic->regs.ctbRcThrdMax=%d,asic->regs.rcRoiEnable=%d\n", asic->regs.bitsRatio, asic->regs.ctbRcThrdMin, asic->regs.ctbRcThrdMax, asic->regs.rcRoiEnable);
  printf("asic->regs.ctbBitsMin=%d,asic->regs.ctbBitsMax=%d,asic->regs.totalLcuBits=%d\n", asic->regs.ctbBitsMin, asic->regs.ctbBitsMax, asic->regs.totalLcuBits);
#endif

    vcenc_instance->strmHeaderLen = 0;

    /* time stamp updated */
    HevcUpdateSeiTS(&vcenc_instance->rateControl.sei, pEncIn->timeIncrement);
    /* Rate control may choose to skip the frame */
    if (vcenc_instance->rateControl.frameCoded == ENCHW_NO) {
        APITRACE("VCEncStrmEncode: OK, frame skipped\n");

        //pSlice->frameNum = pSlice->prevFrameNum;    /* restore frame_num */
        pEncOut->codingType = VCENC_NOTCODED_FRAME;
        vcenc_instance->stream.byteCnt = 0;
        pEncOut->pNaluSizeBuf[0] = 0;
        pEncOut->numNalus = 0;
        pEncOut->streamSize = 0;
        queue_put(&c->picture, (struct node *)pic);
        if (vcenc_instance->pass == 2) {
            ReleaseLookaheadPicture(&vcenc_instance->lookahead, outframe,
                                    pEncOut->consumedAddr.inputbufBusAddr);
        }

        return VCENC_FRAME_READY;
    }
    pEncOut->av1Vp9FrameNotShow = 0;
    if (VCENC_OK !=
        VCEncCodecPrepareEncode(vcenc_instance, pEncIn, pEncOut, codingType, pic, c, segcounts))
        return VCENC_ERROR;

#ifdef TEST_DATA
    EncTraceReferences(c, pic, vcenc_instance->pass);
#endif

    VCEncExtParaIn *pExtParaQ = NULL;
    if (vcenc_instance->pass == 2) {
        pExtParaQ = (VCEncExtParaIn *)queue_get(
            &vcenc_instance->extraParaQ); //this queue will be empty after flush.
    }
    if (vcenc_instance->pass == 2 && pExtParaQ != NULL) {
        vcenc_instance->frameNumExt = pExtParaQ->recon.frame_num;
        pic->frameNumExt = vcenc_instance->frameNumExt;
    }

    if (IS_H264(vcenc_instance->codecFormat)) {
        if (!h264_mmo_collect(vcenc_instance, pic, pEncIn, r2, delta_poc))
            return VCENC_ERROR;

        /* use default sliding window mode for P-only ltr gap pattern for compatibility */
        if (codingType == VCENC_INTRA_FRAME && (vcenc_instance->poc == 0)) {
            vcenc_instance->frameNum = 0;
            ++vcenc_instance->idrPicId;
        } else if (pic->nalRefIdc)
            vcenc_instance->frameNum++;
        pic->frameNum = vcenc_instance->frameNum;

        if (pic->sliceInst->type != I_SLICE) {
            for (i = 0; i < pic->sliceInst->active_l0_cnt; i++) {
                asic->regs.l0_delta_framenum[i] = pic->frameNum - pic->rpl[0][i]->frameNum;
                asic->regs.l0_referenceLongTermIdx[i] =
                    (pic->rpl[0][i]->long_term_flag ? pic->rpl[0][i]->curLongTermIdx : 0);
            }

            if (pic->sliceInst->type == B_SLICE) {
                for (i = 0; i < pic->sliceInst->active_l1_cnt; i++) {
                    asic->regs.l1_delta_framenum[i] = pic->frameNum - pic->rpl[1][i]->frameNum;
                    asic->regs.l1_referenceLongTermIdx[i] =
                        (pic->rpl[1][i]->long_term_flag ? pic->rpl[1][i]->curLongTermIdx : 0);
                }
            }
        }
    } else {
        pic->markCurrentLongTerm = (pEncIn->u8IdxEncodedAsLTR != 0);
    }

    if (asic->regs.asicCfg.ctbRcVersion &&
        IS_CTBRC_FOR_BITRATE(vcenc_instance->rateControl.ctbRc) &&
        asic->ctbRcMem[0].busAddress == 0) {
        vcencRateControl_s *rc = &(vcenc_instance->rateControl);
        if (EncAsicGetBuffers(asic, asic->ctbRcMem, CTB_RC_BUF_NUM, vcenc_instance->ctbPerFrame,
                              vcenc_instance->ref_alignment) != ENCHW_OK) {
            APITRACEERR("VCEncStrmEncode: ERROR. ctbRc mem allocation failed!\n");
            return VCENC_EWL_MEMORY_ERROR;
        }
        for (i = 0; i < 3; i++) {
            rc->ctbRateCtrl.models[i].ctbMemPreAddr = asic->ctbRcMem[i].busAddress;
            rc->ctbRateCtrl.models[i].ctbMemPreVirtualAddr = asic->ctbRcMem[i].virtualAddress;
        }
        rc->ctbRateCtrl.ctbMemCurAddr = asic->ctbRcMem[3].busAddress;
        rc->ctbRateCtrl.ctbMemCurVirtualAddr = asic->ctbRcMem[3].virtualAddress;
    }

    asic->regs.inputLumBase = pEncIn->busLuma;
    asic->regs.inputCbBase = pEncIn->busChromaU;
    asic->regs.inputCrBase = pEncIn->busChromaV;
    /* setup stabilization */
    if (vcenc_instance->preProcess.videoStab) {
        asic->regs.stabNextLumaBase = pEncIn->busLumaStab;
    }

    asic->regs.minCbSize = pic->sps->log2_min_cb_size;
    asic->regs.maxCbSize = pic->pps->log2_ctb_size;
    asic->regs.minTrbSize = pic->sps->log2_min_tr_size;
    asic->regs.maxTrbSize = pic->pps->log2_max_tr_size;

    asic->regs.picWidth = pic->sps->width_min_cbs;
    asic->regs.picHeight = pic->sps->height_min_cbs;

    asic->regs.pps_deblocking_filter_override_enabled_flag =
        pic->pps->deblocking_filter_override_enabled_flag;

    /* Enable slice ready interrupts if defined by config and slices in use */
    asic->regs.sliceReadyInterrupt = ENCH2_SLICE_READY_INTERRUPT & ((asic->regs.sliceNum > 1));
    asic->regs.picInitQp = pic->pps->init_qp;
#ifndef CTBRC_STRENGTH
    asic->regs.qp = vcenc_instance->rateControl.qpHdr;
#else
    //printf("float qp=%f\n",vcenc_instance->rateControl.qpHdr);
    //printf("float qp=%f\n",inst->rateControl.qpHdr);
    asic->regs.qp = (vcenc_instance->rateControl.qpHdr >> QP_FRACTIONAL_BITS);
    asic->regs.qpfrac = (vcenc_instance->rateControl.qpHdr - (asic->regs.qp << QP_FRACTIONAL_BITS))
                        << (16 - QP_FRACTIONAL_BITS);
    //printf("float qp=%f, int qp=%d,qpfrac=0x%x\n",vcenc_instance->rateControl.qpHdr,asic->regs.qp, asic->regs.qpfrac);
    //#define BLOCK_SIZE_MIN_RC  32 , 2=16x16,1= 32x32, 0=64x64
    asic->regs.rcBlockSize = vcenc_instance->blockRCSize;
    {
        i32 qpDeltaGain;
        float strength = 1.6f;
        float qpDeltaGainFloat;

        qpDeltaGainFloat = vcenc_instance->rateControl.complexity;
        if (qpDeltaGainFloat < 14.0) {
            qpDeltaGainFloat = 14.0;
        }
        qpDeltaGainFloat = qpDeltaGainFloat * qpDeltaGainFloat * strength;
        qpDeltaGain = (i32)(qpDeltaGainFloat);

        asic->regs.qpDeltaMBGain = qpDeltaGain;
    }
#endif
    if (asic->regs.asicCfg.tu32Enable && asic->regs.asicCfg.dynamicMaxTuSize &&
        vcenc_instance->rdoLevel == 0 && asic->regs.qp <= 30 &&
        (IS_HEVC(vcenc_instance->codecFormat) || IS_H264(vcenc_instance->codecFormat)))
        asic->regs.current_max_tu_size_decrease = 1;
    else
        asic->regs.current_max_tu_size_decrease = 0;

    asic->regs.qpMax = vcenc_instance->rateControl.qpMax >> QP_FRACTIONAL_BITS;
    asic->regs.qpMin = vcenc_instance->rateControl.qpMin >> QP_FRACTIONAL_BITS;
    if (IS_CTBRC_FOR_QUALITY(vcenc_instance->rateControl.ctbRc)) {
        /* if ctbRc can tune subjective quality, not to increase block qp bigger than a big enough frame qp,
       otherwise it will hurt subjective quality. */
        const u32 worstQualityQp = 42;
        u32 qpMaxLimit = MAX(worstQualityQp, asic->regs.qp);
        asic->regs.qpMax = MIN(qpMaxLimit, asic->regs.qpMax);
    }

    if (vcenc_instance->pass == 1) {
        pic->outForCutree.poc = vcenc_instance->poc;
        pic->outForCutree.qp = vcenc_instance->rateControl.qpHdr;
        pic->outForCutree.codingType = pEncOut->codingType;
        pic->outForCutree.hierarchical_bit_allocation_GOP_size =
            vcenc_instance->rateControl.hierarchical_bit_allocation_GOP_size;
        pic->outForCutree.encoded_frame_number = vcenc_instance->rateControl.encoded_frame_number;
        pic->outForCutree.dsRatio = vcenc_instance->preProcess.inLoopDSRatio;
        pic->outForCutree.extDSRatio = vcenc_instance->extDSRatio;
        pic->outForCutree.p0 = ABS(asic->regs.l0_delta_poc[0]);
        pic->outForCutree.p1 = ABS(asic->regs.l1_delta_poc[0]);
        pic->outForCutree.roiMapEnable = vcenc_instance->roiMapEnable && pEncIn->pRoiMapDelta;

        if (vcenc_instance->asic.regs.asicCfg.bMultiPassSupport) {
            pic->outForCutree.cuDataIdx =
                ((ptr_t)pEncOut->cuOutData.cuData -
                 ((ptr_t)vcenc_instance->asic.cuInfoMem[0].virtualAddress +
                  vcenc_instance->asic.cuInfoTableSize)) /
                (vcenc_instance->asic.cuInfoMem[1].busAddress -
                 vcenc_instance->asic.cuInfoMem[0].busAddress);
            pic->outForCutree.roiMapEnable =
                pic->outForCutree.roiMapEnable && pEncIn->roiMapDeltaQpAddr;
            pic->outForCutree.pRoiMapDelta = pEncIn->pRoiMapDelta;
            pic->outForCutree.roiMapDeltaQpAddr = pEncIn->roiMapDeltaQpAddr;
            pic->outForCutree.roiMapDeltaSize = pEncIn->roiMapDeltaSize;
        }

        asic->regs.qpMax = 51;
        asic->regs.qpMin = 0;
    }

    pic->pps->qp = asic->regs.qp;
    pic->pps->qpMin = asic->regs.qpMin;
    pic->pps->qpMax = asic->regs.qpMax;

    asic->regs.diffCuQpDeltaDepth = pic->pps->diff_cu_qp_delta_depth;

    asic->regs.cbQpOffset = pic->pps->cb_qp_offset;
    asic->regs.saoEnable = pic->sps->sao_enabled_flag;
    asic->regs.maxTransHierarchyDepthInter = pic->sps->max_tr_hierarchy_depth_inter;
    asic->regs.maxTransHierarchyDepthIntra = pic->sps->max_tr_hierarchy_depth_intra;
    asic->regs.log2ParellelMergeLevel = pic->pps->log2_parallel_merge_level;
    if (rpsInSliceHeader || IS_H264(vcenc_instance->codecFormat)) {
        asic->regs.numShortTermRefPicSets = 0;
    } else {
        asic->regs.numShortTermRefPicSets = pic->sps->num_short_term_ref_pic_sets;
    }
    asic->regs.long_term_ref_pics_present_flag = pic->sps->long_term_ref_pics_present_flag;

    //ring buffer for reference and recon data using common buffer.
    if (vcenc_instance->refRingBufEnable) {
        VCEncSetRingBuffer(vcenc_instance, asic, pic);
    }

    asic->regs.reconLumBase = pic->recon.lum;
    asic->regs.reconCbBase = pic->recon.cb;
    asic->regs.reconCrBase = pic->recon.cr;

    asic->regs.reconL4nBase = pic->recon_4n_base;

    /* Map the reset of instance parameters */
    asic->regs.poc = pic->poc;
    asic->regs.frameNum = pic->frameNum;
    asic->regs.idrPicId = (codingType == VCENC_INTRA_FRAME ? (vcenc_instance->idrPicId & 1) : 0);
    asic->regs.nalRefIdc = pic->nalRefIdc;
    asic->regs.nalRefIdc_2bit = pic->nalRefIdc_2bit;
    asic->regs.markCurrentLongTerm = pic->markCurrentLongTerm;
    asic->regs.currentLongTermIdx = pic->curLongTermIdx;
    asic->regs.transform8x8Enable = pic->pps->transform8x8Mode;
    asic->regs.entropy_coding_mode_flag = pic->pps->entropy_coding_mode_flag;
    if (rpsInSliceHeader || IS_H264(vcenc_instance->codecFormat)) {
        asic->regs.rpsId = 0;
    } else {
        asic->regs.rpsId = pic->rps->ps.id;
    }
    // numNegativePics / numNegativePics is only used when short_term_ref_pic_set_sps_flag == 0
    // this relaxes bits limitation for GOP16.
    if (!vcenc_instance->sHwRps.u1short_term_ref_pic_set_sps_flag) {
        asic->regs.numNegativePics = pic->rps->num_negative_pics;
        asic->regs.numPositivePics = pic->rps->num_positive_pics;
    } else {
        asic->regs.numNegativePics = 0;
        asic->regs.numPositivePics = 0;
    }
    /* H.264 MMO */
    asic->regs.l0_used_by_next_pic[0] = 1;
    asic->regs.l0_used_by_next_pic[1] = 1;
    asic->regs.l1_used_by_next_pic[0] = 1;
    asic->regs.l1_used_by_next_pic[1] = 1;
    if (IS_H264(vcenc_instance->codecFormat) && pic->nalRefIdc &&
        vcenc_instance->h264_mmo_nops > 0 && (pExtParaQ == NULL) && !useExtFlag) {
        int cnt[2] = {pic->sliceInst->active_l0_cnt, pic->sliceInst->active_l1_cnt};
#if 1 // passing all unreferenced frame to hw
        for (i = 0; i < vcenc_instance->h264_mmo_nops; i++)
            h264_mmo_mark_unref(&asic->regs, vcenc_instance->h264_mmo_unref[i],
                                vcenc_instance->h264_mmo_long_term_flag[i],
                                vcenc_instance->h264_mmo_ltIdx[i], cnt, pic);
        vcenc_instance->h264_mmo_nops = 0;
#else // passing at most 1 unreferenced frame to hw
        h264_mmo_mark_unref(
            &asic->regs, vcenc_instance->h264_mmo_unref[--vcenc_instance->h264_mmo_nops], cnt, pic);
#endif
    }

    asic->regs.pps_id = pic->pps->ps.id;

    asic->regs.filterDisable = vcenc_instance->disableDeblocking;
    asic->regs.tc_Offset = vcenc_instance->tc_Offset * 2;
    asic->regs.beta_Offset = vcenc_instance->beta_Offset * 2;

    /* strong_intra_smoothing_enabled_flag */
    asic->regs.strong_intra_smoothing_enabled_flag = pic->sps->strong_intra_smoothing_enabled_flag;

    /* constrained_intra_pred_flag */
    asic->regs.constrained_intra_pred_flag = pic->pps->constrained_intra_pred_flag;

    /* skip frame flag */
    asic->regs.skip_frame_enabled = bSkipFrame;

    /*  update rdoLevel from instance per-frame:
    *  if skip_frame_enabled, config rdoLevel=0
    *  otherwise restore instance's rdoLevel setting
    */
    asic->regs.rdoLevel = vcenc_instance->rdoLevel;
    if (asic->regs.skip_frame_enabled)
        asic->regs.rdoLevel = 0;

    /* set rdoLevel=0 when HW not support progRdo */
    if (!asic->regs.asicCfg.progRdoEnable)
        asic->regs.rdoLevel = 0;

    /* HW base address for NAL unit sizes is affected by start offset
   * and SW created NALUs. */
    asic->regs.sizeTblBase = asic->sizeTbl[cur_core_index].busAddress;
    asic->regs.compress_coeff_scan_base = asic->compress_coeff_SCAN[cur_core_index].busAddress;
    /*clip qp*/
#ifdef CTBRC_STRENGTH

    /* configure ROI parameters */
    StrmEncodeRegionOfInterest(vcenc_instance, asic);

#endif

    /* HW Base must be 64-bit aligned */
    ASSERT(asic->regs.sizeTblBase % 8 == 0);

    /*inter prediction parameters*/
    double dQPFactor =
        (HW_PRODUCT_SYSTEM60(asic->regs.asicHwId) || HW_PRODUCT_VC9000LE(asic->regs.asicHwId))
            ? 0.894
            : 0.387298335; // sqrt(0.15);
    bool depth0 = HANTRO_TRUE;

    asic->regs.nuh_temporal_id = 0;

    if (pic->sliceInst->type != I_SLICE) {
        dQPFactor = pEncIn->gopCurrPicConfig.QpFactor;
        depth0 = (pEncIn->gopCurrPicConfig.poc % pEncIn->gopSize) ? HANTRO_FALSE : HANTRO_TRUE;
        asic->regs.nuh_temporal_id = pEncIn->gopCurrPicConfig.temporalId;
        if (useExtFlag && vcenc_instance->pass == 0)
            asic->regs.nuh_temporal_id = pEncExtParaIn->recon.temporalId;
        else if (useExtFlag && vcenc_instance->pass == 2 && pExtParaQ != NULL)
            asic->regs.nuh_temporal_id = pExtParaQ->recon.temporalId;
        //HEVC support temporal svc
        if ((asic->regs.nuh_temporal_id) && (!IS_H264(vcenc_instance->codecFormat))) {
            if (pic->rpl[0][0]->temporal_id < pic->temporal_id) {
                if ((pic->sliceInst->type == P_SLICE) ||
                    ((pic->sliceInst->type == B_SLICE) &&
                     (pic->rpl[1][0]->temporal_id < pic->temporal_id))) {
                    asic->regs.nal_unit_type = TSA_R;
                }
            }
        }
    }

    /* flag for HW to code PrefixNAL */
    /* For VC8000E 6.0, minor version >= 6010 */
    asic->regs.prefixnal_svc_ext = IS_H264(vcenc_instance->codecFormat) &&
                                   (s->max_num_sub_layers > 1) &&
                                   (vcenc_instance->asic.regs.asicHwId >= 0x80006010);

    asic->regs.av1_extension_flag = (asic->regs.nuh_temporal_id > 0) ? 1 : 0;
    if (vcenc_instance->rateControl.qpFactor > 0.0)
        dQPFactor = vcenc_instance->rateControl.qpFactor;
    VCEncSetQuantParameters(vcenc_instance, pic, pEncIn, dQPFactor, depth0);

    asic->regs.skip_chroma_dc_threadhold = 2;

    /*tuning on intra cu cost bias*/
    asic->regs.bits_est_bias_intra_cu_8 = 22;      //25;
    asic->regs.bits_est_bias_intra_cu_16 = 40;     //48;
    asic->regs.bits_est_bias_intra_cu_32 = 86;     //108;
    asic->regs.bits_est_bias_intra_cu_64 = 38 * 8; //48*8;

    asic->regs.bits_est_1n_cu_penalty = 5;
    asic->regs.bits_est_tu_split_penalty = 3;
    asic->regs.inter_skip_bias = 124;

    /*small resolution, smaller CU/TU prefered*/
    if (pic->sps->width <= 832 && pic->sps->height <= 480) {
        asic->regs.bits_est_1n_cu_penalty = 0;
        asic->regs.bits_est_tu_split_penalty = 2;
    }

    if (IS_H264(vcenc_instance->codecFormat))
        asic->regs.bits_est_1n_cu_penalty = 15;

    // scaling list enable
    asic->regs.scaling_list_enabled_flag = pic->sps->scaling_list_enable_flag;

    /*stream multi-segment*/
    asic->regs.streamMultiSegEn = vcenc_instance->streamMultiSegment.streamMultiSegmentMode != 0;
    asic->regs.streamMultiSegSWSyncEn =
        vcenc_instance->streamMultiSegment.streamMultiSegmentMode == 2;
    asic->regs.streamMultiSegRD = 0;
    asic->regs.streamMultiSegIRQEn = 0;
    vcenc_instance->streamMultiSegment.rdCnt = 0;
    if (asic->regs.streamMultiSegEn) {
        //make sure the stream size is equal to N*segment size
        asic->regs.streamMultiSegSize = asic->regs.outputStrmSize[0] /
                                        vcenc_instance->streamMultiSegment.streamMultiSegmentAmount;
        asic->regs.streamMultiSegSize = ((asic->regs.streamMultiSegSize + 16 - 1) &
                                         (~(16 - 1))); //segment size must be aligned to 16byte
        asic->regs.outputStrmSize[0] = asic->regs.streamMultiSegSize *
                                       vcenc_instance->streamMultiSegment.streamMultiSegmentAmount;
        asic->regs.streamMultiSegIRQEn = 1;
        APITRACE("segment size = %d\n", asic->regs.streamMultiSegSize);
    }

    /* smart */
    asic->regs.smartModeEnable = vcenc_instance->smartModeEnable;
    asic->regs.smartH264LumDcTh = vcenc_instance->smartH264LumDcTh;
    asic->regs.smartH264CbDcTh = vcenc_instance->smartH264CbDcTh;
    asic->regs.smartH264CrDcTh = vcenc_instance->smartH264CrDcTh;
    for (i = 0; i < 3; i++) {
        asic->regs.smartHevcLumDcTh[i] = vcenc_instance->smartHevcLumDcTh[i];
        asic->regs.smartHevcChrDcTh[i] = vcenc_instance->smartHevcChrDcTh[i];
        asic->regs.smartHevcLumAcNumTh[i] = vcenc_instance->smartHevcLumAcNumTh[i];
        asic->regs.smartHevcChrAcNumTh[i] = vcenc_instance->smartHevcChrAcNumTh[i];
    }
    asic->regs.smartH264Qp = vcenc_instance->smartH264Qp;
    asic->regs.smartHevcLumQp = vcenc_instance->smartHevcLumQp;
    asic->regs.smartHevcChrQp = vcenc_instance->smartHevcChrQp;
    for (i = 0; i < 4; i++)
        asic->regs.smartMeanTh[i] = vcenc_instance->smartMeanTh[i];
    asic->regs.smartPixNumCntTh = vcenc_instance->smartPixNumCntTh;

    /* Global MV */
    StrmEncodeGlobalmvConfig(asic, pic, pEncIn, vcenc_instance);

    /* multi-core sync */
    asic->regs.mc_sync_enable = (vcenc_instance->parallelCoreNum > 1);
    if (asic->regs.mc_sync_enable) {
        asic->regs.mc_sync_l0_addr =
            (codingType == VCENC_INTRA_FRAME ? 0 : pic->rpl[0][0]->mc_sync_addr);
        asic->regs.mc_sync_l1_addr =
            (codingType != VCENC_BIDIR_PREDICTED_FRAME ? 0 : pic->rpl[1][0]->mc_sync_addr);
        asic->regs.mc_sync_rec_addr = pic->mc_sync_addr;
        asic->regs.mc_ref_ready_threshold = 0;
        asic->regs.mc_ddr_polling_interval =
            (IS_H264(vcenc_instance->codecFormat) ? 32 : 128) * pic->pps->ctb_per_row;
        /* clear sync address */
        memset((u32 *)pic->mc_sync_ptr, 0,
               MC_SYNC_WORD_BYTES); //hw read 16Byte a time
        EWLSyncMemData(&asic->mc_sync_word[pic->picture_memory_id], 0, MC_SYNC_WORD_BYTES,
                       HOST_TO_DEVICE);
    }

    //set/clear picture compress flag
    pic->recon_compress.lumaCompressed = asic->regs.recon_luma_compress ? 1 : 0;
    pic->recon_compress.chromaCompressed = asic->regs.recon_chroma_compress ? 1 : 0;

    VCEncHEVCDnfPrepare(vcenc_instance);

#ifdef INTERNAL_TEST
    /* Configure the ASIC penalties according to the test vector */
    HevcConfigureTestBeforeStart(vcenc_instance);
#endif

    vcenc_instance->preProcess.bottomField =
        (vcenc_instance->poc % 2) == vcenc_instance->fieldOrder;
    HevcUpdateSeiPS(&vcenc_instance->rateControl.sei, vcenc_instance->preProcess.interlacedFrame,
                    vcenc_instance->preProcess.bottomField);

    /* configure OSD and mosaic parameter */
    StrmEncodeOverlayConfig(asic, pEncIn, vcenc_instance);

    u32 reservedLumWidth = vcenc_instance->preProcess.lumWidth;
    for (tileId = 0; tileId < vcenc_instance->num_tile_columns; tileId++) {
        asic->regs.inputLumBase =
            (tileId == 0) ? pEncIn->busLuma : pEncIn->tileExtra[tileId - 1].busLuma;
        asic->regs.inputCbBase =
            (tileId == 0) ? pEncIn->busChromaU : pEncIn->tileExtra[tileId - 1].busChromaU;
        asic->regs.inputCrBase =
            (tileId == 0) ? pEncIn->busChromaV : pEncIn->tileExtra[tileId - 1].busChromaV;
        ///lumaWidth->dummyAddress+scaled+xFill,so
        // vcenc_instance->preProcess.lumWidth = (vcenc_instance->tileCtrl[tileId].tileRight -
        //                                       vcenc_instance->tileCtrl[tileId].tileLeft + 1) * 64;
        EncPreProcess(asic, &vcenc_instance->preProcess, vcenc_instance->ctx, tileId);
        vcenc_instance->tileCtrl[tileId].input_luma_stride = asic->regs.input_luma_stride;
        vcenc_instance->tileCtrl[tileId].input_chroma_stride = asic->regs.input_chroma_stride;
        vcenc_instance->tileCtrl[tileId].inputLumBase = asic->regs.inputLumBase;
        vcenc_instance->tileCtrl[tileId].inputCbBase = asic->regs.inputCbBase;
        vcenc_instance->tileCtrl[tileId].inputCrBase = asic->regs.inputCrBase;
        vcenc_instance->tileCtrl[tileId].inputChromaBaseOffset = asic->regs.inputChromaBaseOffset;
        vcenc_instance->tileCtrl[tileId].inputLumaBaseOffset = asic->regs.inputLumaBaseOffset;
        vcenc_instance->tileCtrl[tileId].stabNextLumaBase = asic->regs.stabNextLumaBase;
    }
    vcenc_instance->preProcess.lumWidth = reservedLumWidth;

    /* PREFIX SEI */
    StrmEncodePrefixSei(vcenc_instance, s, pEncOut, pic, pEncIn);

    vcenc_instance->numNalus[cur_core_index] = pEncOut->numNalus;
    VCEncResetCallback(&slice_callback, vcenc_instance, pEncIn, pEncOut, next_core_index, 0);

    VCEncSetNewFrame(vcenc_instance);
    vcenc_instance->output_buffer_over_flow = 0;

    if (useExtFlag) {
        if (vcenc_instance->pass == 2 && pExtParaQ != NULL)
            useExtPara(pExtParaQ, asic, IS_H264(vcenc_instance->codecFormat), vcenc_instance);
        else if (vcenc_instance->pass == 0)
            useExtPara(pEncExtParaIn, asic, IS_H264(vcenc_instance->codecFormat), vcenc_instance);
    }

    //if current frame will be used as reference frame of follow stream, set reference as true.
    if (((!IS_H264(vcenc_instance->codecFormat)) &&
         (!pEncIn->gopCurrPicConfig.nonReference || codingType == VCENC_INTRA_FRAME)) ||
        pic->nalRefIdc)
        pic->reference = HANTRO_TRUE;

    dec400_data = &vcenc_instance->dec400_data_bak[cur_core_index];
    dec400_osd = &vcenc_instance->dec400_osd_bak[cur_core_index];
    VCEncCfgDec400(pEncIn, dec400_data, dec400_osd, vcenc_instance, asic, 0);
    TemporalMvpGenConfig(vcenc_instance, pEncIn, c, pic, codingType);

    vcenc_instance->asic.regs.tileSyncReadBase = 0;
    vcenc_instance->asic.regs.tileSyncWriteBase =
        (vcenc_instance->num_tile_columns > 1) ? asic->deblockCtx.busAddress : 0;
    vcenc_instance->asic.regs.tileSyncReadAlignment =
        vcenc_instance->asic.regs.tileSyncWriteAlignment = 4; // 16 bytes align
    vcenc_instance->asic.regs.tileStrmSizeAlignmentExp = vcenc_instance->tileStrmSizeAlignmentExp;
    if (vcenc_instance->num_tile_columns > 1) {
        u32 ctu_size = (1 << (IS_H264(vcenc_instance->codecFormat) ? 4 : 6));
        int tile_sync_size = (vcenc_instance->num_tile_columns - 1) *
                             (DEB_TILE_SYNC_SIZE + SAODEC_TILE_SYNC_SIZE + SAOPP_TILE_SYNC_SIZE) *
                             ((vcenc_instance->height + ctu_size - 1) / ctu_size);
        vcenc_instance->asic.regs.mc_sync_l0_addr = 0;
        vcenc_instance->asic.regs.mc_sync_l1_addr = 0;
        vcenc_instance->asic.regs.mc_sync_rec_addr =
            vcenc_instance->asic.deblockCtx.busAddress + tile_sync_size;
        memset((u8 *)vcenc_instance->asic.deblockCtx.virtualAddress + tile_sync_size, 0,
               4 * (vcenc_instance->num_tile_columns - 1));
        vcenc_instance->asic.regs.mc_ref_ready_threshold =
            vcenc_instance->compressor
                ? 64
                : 0; //if tile enabled and rfc enabled, extra lines will be needed
    }

    u32 xfillTmp = vcenc_instance->asic.regs.xFill;
    do {
        for (tileId = 0; tileId < vcenc_instance->num_tile_columns; tileId++) {
            if (!vcenc_instance->cb_try_new_params || re_encode_counter == 0) {
                dec400_data->lumWidthSrc = vcenc_instance->preProcess.lumWidthSrc[tileId];
                dec400_data->lumHeightSrc = vcenc_instance->preProcess.lumHeightSrc[tileId];
                TileInfoConfig(vcenc_instance, pic, tileId, codingType, pEncOut);
                vcenc_instance->asic.regs.xFill =
                    (tileId == vcenc_instance->num_tile_columns - 1) ? xfillTmp : 0;
            }
            if (vcenc_instance->cb_try_new_params && re_encode_counter == 0 && tileId == 0) {
                memcpy(&vcenc_instance->regs_bak[cur_core_index], &asic->regs, sizeof(regValues_s));
            }
            if (!vcenc_instance->asic.regs.bVCMDEnable) {
                /* Try to reserve the HW resource */
                if (EWLReserveHw(vcenc_instance->asic.ewl, &vcenc_instance->reserve_core_info,
                                 &waitCoreJobid) == EWL_ERROR) {
                    APITRACEERR("VCEncStrmEncode: ERROR HW unavailable\n");
                    return VCENC_HW_RESERVED;
                }

                /* Record coreJobid for tile */
                vcenc_instance->tileCtrl[tileId].job_id = waitCoreJobid;

                /* Enable L2Cache/dec400/AXIFE/APBFilter */
                VCEncSetSubsystem(vcenc_instance, pEncIn, asic, dec400_data, dec400_osd, pic,
                                  tileId);

                EncAsicFrameStart(asic->ewl, &asic->regs, asic->dumpRegister);

                byteCntSw = vcenc_instance->stream.byteCnt;
                VCEncSetwaitJobCfg(pEncIn, vcenc_instance, asic, waitCoreJobid);
            } else {
                //priority = 0, will be set in command line.
                EncSetReseveInfo(asic->ewl, vcenc_instance->width, vcenc_instance->height,
                                 vcenc_instance->rdoLevel, vcenc_instance->asic.regs.bRDOQEnable, 0,
                                 client_type);
                EncReseveCmdbuf(asic->ewl, &asic->regs);
                pic->cmdbufid = asic->regs.cmdbufid;
                /* Record cmdbufid for tile */
                vcenc_instance->tileCtrl[tileId].job_id = pic->cmdbufid;
                vcenc_instance->asic.regs.vcmdBufSize = 0;
                if (EncMakeCmdbufData(asic, &asic->regs, dec400_data, dec400_osd) ==
                    VCENC_INVALID_ARGUMENT) {
                    APITRACEERR("VCEncStrmEncode: DEC400 doesn't exist or format not "
                                "supported\n");
                    EWLReleaseCmdbuf(asic->ewl, pic->cmdbufid);
                    return VCENC_INVALID_ARGUMENT;
                }
                vcenc_instance->asic.regs.totalCmdbufSize = vcenc_instance->asic.regs.vcmdBufSize;
                EncCopyDataToCmdbuf(asic->ewl, &asic->regs);

                EncLinkRunCmdbuf(asic->ewl, &asic->regs);
            }
        }
        if (re_encode_counter == 0) {
            sw_ref_cnt_increase(pic);
        }

        // picture instance switch for multi-core encoding
        if (vcenc_instance->parallelCoreNum > 1) {
            if (re_encode_counter == 0) {
                pic->picture_cnt = pEncOut->picture_cnt;
                memcpy(&vcenc_instance->streams[cur_core_index], &vcenc_instance->stream,
                       sizeof(struct buffer));
                vcenc_instance->pic[cur_core_index] = pic;
                vcenc_instance->waitJobid[cur_core_index] = waitCoreJobid;
                /*buffer EncIn and EncOut of pass 1 in encoding order for they will be used to assemble job to Cutree*/
                tmpTileExtra = vcenc_instance->EncIn[cur_core_index].tileExtra;
                if (vcenc_instance->pass == 1) {
                    ASSERT(vcenc_instance->EncOut[cur_core_index].tileExtra == pEncOut->tileExtra);
                    vcenc_instance->EncOut[cur_core_index] = *pEncOut;
                    for (tileId = 1; tileId < vcenc_instance->num_tile_columns; tileId++) {
                        vcenc_instance->EncOut[cur_core_index].tileExtra[tileId - 1].pNaluSizeBuf =
                            pEncOut->tileExtra[tileId - 1].pNaluSizeBuf;
                        vcenc_instance->EncOut[cur_core_index].tileExtra[tileId - 1].streamSize =
                            pEncOut->tileExtra[tileId - 1].streamSize;
                        vcenc_instance->EncOut[cur_core_index].tileExtra[tileId - 1].numNalus =
                            pEncOut->tileExtra[tileId - 1].numNalus;
                        vcenc_instance->EncOut[cur_core_index].tileExtra[tileId - 1].cuOutData =
                            pEncOut->tileExtra[tileId - 1].cuOutData;
                    }

                    vcenc_instance->EncIn[cur_core_index] = *pEncIn;
                } else {
                    if (vcenc_instance->pass != 2)
                        vcenc_instance->EncIn[cur_core_index] = *pEncIn;
                    else {
                        vcenc_instance->EncIn[cur_core_index] = outframe->encIn;
                        vcenc_instance->lookaheadJob[cur_core_index] = outframe;
                    }
                }
                vcenc_instance->EncIn[cur_core_index].tileExtra = tmpTileExtra;
                for (tileId = 1; tileId < vcenc_instance->num_tile_columns; tileId++) {
                    for (iBuf = 0; iBuf < MAX_STRM_BUF_NUM; iBuf++) {
                        vcenc_instance->EncIn[cur_core_index].tileExtra[tileId - 1].pOutBuf[iBuf] =
                            vcenc_instance->pass == 2
                                ? outframe->encIn.tileExtra[tileId - 1].pOutBuf[iBuf]
                                : pEncIn->tileExtra[tileId - 1].pOutBuf[iBuf];
                        vcenc_instance->EncIn[cur_core_index]
                            .tileExtra[tileId - 1]
                            .busOutBuf[iBuf] =
                            vcenc_instance->pass == 2
                                ? outframe->encIn.tileExtra[tileId - 1].busOutBuf[iBuf]
                                : pEncIn->tileExtra[tileId - 1].busOutBuf[iBuf];
                        vcenc_instance->EncIn[cur_core_index]
                            .tileExtra[tileId - 1]
                            .outBufSize[iBuf] =
                            vcenc_instance->pass == 2
                                ? outframe->encIn.tileExtra[tileId - 1].outBufSize[iBuf]
                                : pEncIn->tileExtra[tileId - 1].outBufSize[iBuf];
                        vcenc_instance->EncIn[cur_core_index]
                            .tileExtra[tileId - 1]
                            .cur_out_buffer[iBuf] =
                            vcenc_instance->pass == 2
                                ? outframe->encIn.tileExtra[tileId - 1].cur_out_buffer[iBuf]
                                : pEncIn->tileExtra[tileId - 1].cur_out_buffer[iBuf];
                    }
                }

                if (vcenc_instance->reservedCore >= vcenc_instance->parallelCoreNum - 1) {
                    queue_put(&c->picture, (struct node *)pic);
                    pic = vcenc_instance->pic[next_core_index];
                    queue_remove(&c->picture, (struct node *)pic);

                    memcpy(&vcenc_instance->stream, &vcenc_instance->streams[next_core_index],
                           sizeof(struct buffer));
                    waitCoreJobid = vcenc_instance->waitJobid[next_core_index];

                    tmpTileExtra = pEncIn->tileExtra;

                    if (vcenc_instance->pass == 1) {
                        *pEncOut = vcenc_instance->EncOut[next_core_index];
                        for (tileId = 1; tileId < vcenc_instance->num_tile_columns; tileId++) {
                            pEncOut->tileExtra[tileId - 1].pNaluSizeBuf =
                                vcenc_instance->EncOut[next_core_index]
                                    .tileExtra[tileId - 1]
                                    .pNaluSizeBuf;
                            pEncOut->tileExtra[tileId - 1].streamSize =
                                vcenc_instance->EncOut[next_core_index]
                                    .tileExtra[tileId - 1]
                                    .streamSize;
                            pEncOut->tileExtra[tileId - 1].numNalus =
                                vcenc_instance->EncOut[next_core_index]
                                    .tileExtra[tileId - 1]
                                    .numNalus;
                            pEncOut->tileExtra[tileId - 1].cuOutData =
                                vcenc_instance->EncOut[next_core_index]
                                    .tileExtra[tileId - 1]
                                    .cuOutData;
                        }
                        *pEncIn = vcenc_instance->EncIn[next_core_index];

                    } else {
                        if (vcenc_instance->pass != 2)
                            pEncIn = &vcenc_instance->EncIn[next_core_index];
                        else
                            pEncIn = vcenc_instance->EncIn + next_core_index;

                        pEncOut->consumedAddr.inputbufBusAddr =
                            vcenc_instance->EncIn[next_core_index].busLuma;
                        pEncOut->consumedAddr.dec400TableBusAddr =
                            vcenc_instance->EncIn[next_core_index].dec400LumTableBase;
                        pEncOut->consumedAddr.roiMapDeltaQpBusAddr =
                            vcenc_instance->EncIn[next_core_index].roiMapDeltaQpAddr;
                        pEncOut->consumedAddr.roimapCuCtrlInfoBusAddr =
                            vcenc_instance->EncIn[next_core_index].RoimapCuCtrlAddr;
                        memcpy(pEncOut->consumedAddr.overlayInputBusAddr,
                               vcenc_instance->EncIn[next_core_index].overlayInputYAddr,
                               MAX_OVERLAY_NUM * sizeof(ptr_t));
                        pEncOut->consumedAddr.outbufBusAddr =
                            vcenc_instance->EncIn[next_core_index].busOutBuf[0];
                        if (vcenc_instance->pass == 2) {
                            outframe = vcenc_instance->lookaheadJob[next_core_index];
                        }
                    }
                    if (vcenc_instance->pass != 2)
                        pEncIn->tileExtra = tmpTileExtra;
                    for (tileId = 1; tileId < vcenc_instance->num_tile_columns; tileId++)
                        for (iBuf = 0; iBuf < MAX_STRM_BUF_NUM; iBuf++) {
                            pEncIn->tileExtra[tileId - 1].pOutBuf[iBuf] =
                                vcenc_instance->EncIn[cur_core_index]
                                    .tileExtra[tileId - 1]
                                    .pOutBuf[iBuf];
                            pEncIn->tileExtra[tileId - 1].busOutBuf[iBuf] =
                                vcenc_instance->EncIn[cur_core_index]
                                    .tileExtra[tileId - 1]
                                    .busOutBuf[iBuf];
                            pEncIn->tileExtra[tileId - 1].outBufSize[iBuf] =
                                vcenc_instance->EncIn[cur_core_index]
                                    .tileExtra[tileId - 1]
                                    .outBufSize[iBuf];
                            pEncIn->tileExtra[tileId - 1].cur_out_buffer[iBuf] =
                                vcenc_instance->EncIn[cur_core_index]
                                    .tileExtra[tileId - 1]
                                    .cur_out_buffer[iBuf];
                        }
                }
                pEncOut->picture_cnt = pic->picture_cnt;
                pEncOut->poc = pic->poc;

                // roiMapDeltaQpBusAddr will be returned when job is received by pass2 from cutree
                if (vcenc_instance->pass == 2 && outframe)
                    pEncOut->consumedAddr.roiMapDeltaQpBusAddr = outframe->pRoiMapDeltaQpAddr;
            } else {
                pEncOut->consumedAddr.outbufBusAddr = pEncIn->busOutBuf[0];
            }
        } else {
            if (vcenc_instance->pass !=
                1) //single pass and pass2 need to record outputbufferAddr in pEncout
            {
                pEncOut->consumedAddr.inputbufBusAddr = pEncIn->busLuma;
#if (!defined SYSTEM_BUILD) && (!defined SYSTEM60_BUILD)
                //[fpga]when bufLoopBack enable, don't need to manage input buffer inside api.
                if (vcenc_instance->inputLineBuf.inputLineBufLoopBackEn)
                    pEncOut->consumedAddr.inputbufBusAddr = 0;
#endif
                pEncOut->consumedAddr.dec400TableBusAddr = pEncIn->dec400LumTableBase;
                if (vcenc_instance->pass == 2 && outframe)
                    pEncOut->consumedAddr.roiMapDeltaQpBusAddr = outframe->pRoiMapDeltaQpAddr;
                else
                    pEncOut->consumedAddr.roiMapDeltaQpBusAddr = pEncIn->roiMapDeltaQpAddr;
                pEncOut->consumedAddr.roimapCuCtrlInfoBusAddr = pEncIn->RoimapCuCtrlAddr;
                memcpy(pEncOut->consumedAddr.overlayInputBusAddr, pEncIn->overlayInputYAddr,
                       MAX_OVERLAY_NUM * sizeof(ptr_t));
                pEncOut->consumedAddr.outbufBusAddr = pEncIn->busOutBuf[0];
            }
        }

        if (re_encode_counter == 0) {
            vcenc_instance->reservedCore++;
            vcenc_instance->reservedCore =
                MIN(vcenc_instance->reservedCore, vcenc_instance->parallelCoreNum);
            pEncOut->numNalus = vcenc_instance->numNalus[next_core_index];
        }

        if (vcenc_instance->reservedCore == vcenc_instance->parallelCoreNum) {
            //wait all tile job_id
            for (tileId = 0; tileId < asic->regs.num_tile_columns; tileId++) {
                i32 retRecord = VCENC_OK;
                /* select a waitId
           single tile:
           choose pic->cmdbufid when vcmd enable, otherwise choose waitCoreJobid
           multi-tile:
           choose vcenc_instance->tileCtrl[tileId].job_id
        */
                u32 waitId = asic->regs.bVCMDEnable ? pic->cmdbufid : waitCoreJobid;
                waitId = vcenc_instance->num_tile_columns > 1
                             ? vcenc_instance->tileCtrl[tileId].job_id
                             : waitId;
                ret = VCEncStrmWaitReady(inst, pEncIn, pEncOut, pic, &slice_callback, c, waitId);
                retRecord = (retRecord == VCENC_OK) ? ret : retRecord;
                ret = retRecord;
            }

        } else {
            if (vcenc_instance->pass != 1) {
                pEncOut->consumedAddr.inputbufBusAddr = 0;
                pEncOut->consumedAddr.dec400TableBusAddr = 0;
                pEncOut->consumedAddr.roiMapDeltaQpBusAddr = 0;
                pEncOut->consumedAddr.roimapCuCtrlInfoBusAddr = 0;
                memset(pEncOut->consumedAddr.overlayInputBusAddr, 0,
                       MAX_OVERLAY_NUM * sizeof(ptr_t));
                pEncOut->consumedAddr.outbufBusAddr = 0;
            }
            ret = VCENC_FRAME_ENQUEUE;
            vcenc_instance->frameCnt++; // update frame count after one frame is ready.
            vcenc_instance->jobCnt++;   // update job count after one frame is ready.
            queue_put(&c->picture, (struct node *)pic);
            return ret;
        }

        use_new_parameters = 0;
        if (vcenc_instance->cb_try_new_params && vcenc_instance->pass != 1) {
            regs_bak_for2nd_encode = &vcenc_instance->regs_bak[next_core_index];
            dec400_data = &vcenc_instance->dec400_data_bak[next_core_index];
            dec400_osd = &vcenc_instance->dec400_data_bak[next_core_index];

            VCEncCollectEncodeStats(vcenc_instance, pEncIn, pEncOut, codingType, &enc_statistic);
            use_new_parameters = vcenc_instance->cb_try_new_params(&enc_statistic, &new_params);
            if (use_new_parameters) {
                VCEncResetEncodeStatus(vcenc_instance, c, pEncIn, pic, pEncOut, &enc_statistic);
                VCEncRertryNewParameters(vcenc_instance, pEncIn, pEncOut, &slice_callback,
                                         &new_params, regs_bak_for2nd_encode);
            }
            re_encode_counter++;
        }
    } while (re_encode_counter && use_new_parameters);

    vcenc_instance->jobCnt++; // update job count after one frame is ready.
    pEncOut->pNaluSizeBuf = (u32 *)vcenc_instance->asic
                                .sizeTbl[vcenc_instance->jobCnt % vcenc_instance->parallelCoreNum]
                                .virtualAddress;

    if (ret != VCENC_OK)
        goto out;

    if (vcenc_instance->output_buffer_over_flow == 1) {
        vcenc_instance->stream.byteCnt = 0;
        pEncOut->streamSize = vcenc_instance->stream.byteCnt;
        pEncOut->pNaluSizeBuf[0] = 0;
        pEncOut->numNalus = 0;
        for (tileId = 1; tileId < asic->regs.num_tile_columns; tileId++) {
            pEncOut->tileExtra[tileId - 1].pNaluSizeBuf[0] = 0;
            pEncOut->tileExtra[tileId - 1].numNalus = 0;
        }
        vcenc_instance->encStatus = VCENCSTAT_START_FRAME;
        ret = VCENC_OUTPUT_BUFFER_OVERFLOW;
        /* revert frameNum update on output buffer overflow */
        if (IS_H264(vcenc_instance->codecFormat) && pic->nalRefIdc)
            vcenc_instance->frameNum--;
        APITRACEERR("VCEncStrmEncode: ERROR Output buffer too small\n");
        goto out;
    }

    if (vcenc_instance->num_tile_columns > 1) {
        asic->regs.sumOfQP = 0;
        asic->regs.sumOfQPNumber = 0;
        asic->regs.picComplexity = 0;
        for (tileId = 0; tileId < vcenc_instance->num_tile_columns; tileId++) {
            asic->regs.sumOfQP += vcenc_instance->tileCtrl[tileId].sumOfQP;
            asic->regs.sumOfQPNumber += vcenc_instance->tileCtrl[tileId].sumOfQPNumber;
            asic->regs.picComplexity += vcenc_instance->tileCtrl[tileId].picComplexity;
        }

        getCtbRcParamsInMultiTiles(vcenc_instance, pic->sliceInst->type);
    }
    i32 stat;
#ifdef CTBRC_STRENGTH
    vcenc_instance->rateControl.rcPicComplexity = asic->regs.picComplexity;
    vcenc_instance->rateControl.complexity = (float)vcenc_instance->rateControl.rcPicComplexity *
                                             (vcenc_instance->rateControl.reciprocalOfNumBlk8);
#endif

    const enum slice_type _slctypes[3] = {P_SLICE, I_SLICE, B_SLICE};
    vcenc_instance->rateControl.sliceTypeCur = _slctypes[asic->regs.frameCodingType];
    stat = VCEncAfterPicRc(&vcenc_instance->rateControl, 0, vcenc_instance->stream.byteCnt,
                           asic->regs.sumOfQP, asic->regs.sumOfQPNumber);

    /* After HRD overflow discard the coded frame and go back old time,
      * just like not coded frame. But if only one reference frame
      * buffer is in use we can't discard the frame unless the next frame
      * is coded as intra. */
    if (stat == VCENCRC_OVERFLOW && vcenc_instance->pass != 2) {
        vcenc_instance->stream.byteCnt = 0;

        pEncOut->pNaluSizeBuf[0] = 0;
        pEncOut->numNalus = 0;
        for (tileId = 1; tileId < asic->regs.num_tile_columns; tileId++) {
            pEncOut->tileExtra[tileId - 1].pNaluSizeBuf[0] = 0;
            pEncOut->tileExtra[tileId - 1].numNalus = 0;
        }

        /* revert frameNum update on HRD overflow */
        if (IS_H264(vcenc_instance->codecFormat) && pic->nalRefIdc)
            vcenc_instance->frameNum--;
        //queue_put(&c->picture, (struct node *)pic);
        APITRACE("VCEncStrmEncode: OK, Frame discarded (HRD overflow)\n");
        //return VCENC_FRAME_READY;
    } else
        vcenc_instance->rateControl.sei.activated_sps = 1;

#ifdef INTERNAL_TEST
    if (vcenc_instance->testId == TID_RFD) {
        HevcRFDTest(vcenc_instance, pic);
    }
#endif
    vcenc_instance->frameCnt++; // update frame count after one frame is ready.

    // add filler when
    //  - vbv buffer underflow (stat > 0)
    //  - HRD is enable and in cbr mode or force to add fillerData by app
    pEncOut->PostDataSize = 0;
    if ((stat > 0) && ((vcenc_instance->rateControl.hrd && vcenc_instance->rateControl.cbr_flag) ||
                       vcenc_instance->rateControl.fillerData)) {
        i32 nalu_length = VCEncWriteFillerDataForCBR(vcenc_instance, stat);
        pEncOut->PostDataSize = nalu_length;
        if (nalu_length)
            VCEncAddNaluSize(pEncOut, nalu_length, asic->regs.num_tile_columns - 1);
    }

    if (stat != VCENCRC_OVERFLOW) {
        if (vcenc_instance->gdrFirstIntraFrame != 0) {
            vcenc_instance->gdrFirstIntraFrame--;
        }

        if (vcenc_instance->gdrEnabled == 1) {
            asic->regs.rcRoiEnable = 0;
            if (vcenc_instance->gdrStart)
                vcenc_instance->gdrCount++;
            if (vcenc_instance->gdrCount ==
                (vcenc_instance->gdrDuration * (1 + (i32)vcenc_instance->interlaced))) {
                vcenc_instance->gdrStart--;
                vcenc_instance->gdrCount = 0;
                asic->regs.rcRoiEnable = 0x00;
            }
        }
    }

    pEncOut->nextSceneChanged = 0;

#ifdef VIDEOSTAB_ENABLED
    /* Finalize video stabilization */
    if (vcenc_instance->preProcess.videoStab) {
        u32 no_motion;

        VSReadStabData(vcenc_instance->asic.regs.regMirror, &vcenc_instance->vsHwData);

        no_motion = VSAlgStabilize(&vcenc_instance->vsSwData, &vcenc_instance->vsHwData);
        if (no_motion) {
            VSAlgReset(&vcenc_instance->vsSwData);
        }

        /* update offset after stabilization */
        VSAlgGetResult(&vcenc_instance->vsSwData, &vcenc_instance->preProcess.horOffsetSrc[0],
                       &vcenc_instance->preProcess.verOffsetSrc[0]);

        /* output scene change result */
        pEncOut->nextSceneChanged = vcenc_instance->vsSwData.sceneChange;
    }
#endif

    VCEncHEVCDnfUpdate(vcenc_instance);

    /* SUFFIX SEI  */
    StrmEncodeSuffixSei(vcenc_instance, pEncIn, pEncOut);

    vcenc_instance->encStatus = VCENCSTAT_START_FRAME;
    ret = VCENC_FRAME_READY;
    vcenc_instance->outForCutree = pic->outForCutree;

    APITRACE("VCEncStrmEncode: OK\n");

out:
    pEncOut->streamSize = vcenc_instance->num_tile_columns > 1
                              ? vcenc_instance->tileCtrl[0].streamSize
                              : vcenc_instance->stream.byteCnt;
    for (tileId = 1; tileId < asic->regs.num_tile_columns; tileId++) {
        pEncOut->tileExtra[tileId - 1].streamSize =
            asic->regs.tiles_enabled_flag ? vcenc_instance->tileCtrl[tileId].streamSize
                                          : vcenc_instance->stream.byteCnt;
    }
    if (VCENC_OK != VCEncCodecPostEncodeUpdate(vcenc_instance, pEncIn, pEncOut, codingType, pic))
        return VCENC_ERROR;

    sw_ref_cnt_decrease(pic);
    if (vcenc_instance->reservedCore)
        vcenc_instance->reservedCore--;
    if (vcenc_instance->pass == 2) {
        ReleaseLookaheadPicture(&vcenc_instance->lookahead, outframe,
                                pEncOut->consumedAddr.inputbufBusAddr);

        if (ret != VCENC_FRAME_READY) {
            VCEncLookahead *lookahead = &vcenc_instance->lookahead;
            VCEncLookahead *lookahead2 =
                &((struct vcenc_instance *)(lookahead->priv_inst))->lookahead;
            lookahead->status = lookahead2->status = ret;
            lookahead->bFlush = true;
        }
    }

    /* Put picture back to store */
    queue_put(&c->picture, (struct node *)pic);

    /* fill VCEncout */
    FillVCEncout(vcenc_instance, pEncOut);

    return ret;
}

/*------------------------------------------------------------------------------

    Function name : VCEncMultiCoreFlush
    Description   : Flush remaining frames at the end of multi-core encoding.
    Return type   : VCEncRet
    Argument      : inst - encoder instance
    Argument      : pEncIn - user provided input parameters
                    pEncOut - place where output info is returned
------------------------------------------------------------------------------*/

VCEncRet VCEncMultiCoreFlush(VCEncInst inst, VCEncIn *pEncIn, VCEncOut *pEncOut,
                             VCEncSliceReadyCallBackFunc sliceReadyCbFunc)
{
    struct vcenc_instance *vcenc_instance = (struct vcenc_instance *)inst;
    asicData_s *asic = &vcenc_instance->asic;
    struct container *c;
    struct sw_picture *pic = NULL;
    i32 ret = VCENC_ERROR;
    VCEncSliceReady slice_callback;
    VCEncLookaheadJob *outframe = NULL;
    i32 i;
    u32 waitCoreJobid;
    u32 cur_core_index, next_core_index;
    u32 iBuf, tileId;
    VCEncInTileExtra *tmpTileExtra;
    VCDec400data *dec400_data = NULL;
    VCDec400data *dec400_osd = NULL;
    u32 re_encode_counter = 0;
    VCEncStatisticOut enc_statistic;
    NewEncodeParams new_params;
    ReEncodeStrategy use_new_parameters = 0;
    regValues_s *regs_bak_for2nd_encode;
    u32 client_type;

    APITRACE("VCEncStrmEncode#\n");

    /* Check for illegal inputs */
    if ((vcenc_instance == NULL) || (pEncIn == NULL) || (pEncOut == NULL)) {
        APITRACEERR("VCEncStrmEncode: ERROR Null argument\n");
        return VCENC_NULL_ARGUMENT;
    }

    /* Check for existing instance */
    if (vcenc_instance->inst != vcenc_instance) {
        APITRACEERR("VCEncStrmEncode: ERROR Invalid instance\n");
        return VCENC_INSTANCE_ERROR;
    }
    /* Check status, INIT and ERROR not allowed */
    if ((vcenc_instance->encStatus != VCENCSTAT_START_STREAM) &&
        (vcenc_instance->encStatus != VCENCSTAT_START_FRAME)) {
        APITRACEERR("VCEncStrmEncode: ERROR Invalid status\n");
        return VCENC_INVALID_STATUS;
    }

    if (vcenc_instance->reservedCore == 0)
        return VCENC_OK;

    if (!(c = get_container(vcenc_instance)))
        return VCENC_ERROR;

    cur_core_index = vcenc_instance->jobCnt % vcenc_instance->parallelCoreNum;
    next_core_index = (vcenc_instance->jobCnt + 1) % vcenc_instance->parallelCoreNum;
    VCEncResetCallback(&slice_callback, vcenc_instance, pEncIn, pEncOut, next_core_index, 1);

    do {
        if (re_encode_counter != 0) {
            if (!vcenc_instance->asic.regs
                     .bVCMDEnable) //multi-tile do not support re-encode in VCEncMultiCoreFlush
            { //vcenc_instance->waitJobid[current_core_index]==vcenc_instance->tileCtrl[n].job_id -> n=tileId
                /* Try to reserve the HW resource */
                if (EWLReserveHw(vcenc_instance->asic.ewl, &vcenc_instance->reserve_core_info,
                                 &waitCoreJobid) == EWL_ERROR) {
                    ASSERT(0);
                    APITRACEERR("VCEncStrmEncode: ERROR HW unavailable\n");
                    return VCENC_HW_RESERVED;
                }
                /* Enable L2Cache/dec400/AXIFE/APBFilter */
                VCEncSetSubsystem(vcenc_instance, pEncIn, asic, dec400_data, dec400_osd, pic, 0);

                EncAsicFrameStart(asic->ewl, &asic->regs, asic->dumpRegister);

                VCEncSetwaitJobCfg(pEncIn, vcenc_instance, asic, waitCoreJobid);
            } else {
                client_type = VCEncGetClientType(vcenc_instance->codecFormat);
                //priority = 0, will be set in command line.
                EncSetReseveInfo(asic->ewl, vcenc_instance->width, vcenc_instance->height,
                                 vcenc_instance->rdoLevel, vcenc_instance->asic.regs.bRDOQEnable, 0,
                                 client_type);
                EncReseveCmdbuf(asic->ewl, &asic->regs);
                pic->cmdbufid = asic->regs.cmdbufid;
                vcenc_instance->asic.regs.vcmdBufSize = 0;
                if (EncMakeCmdbufData(asic, &asic->regs, dec400_data, dec400_osd) ==
                    VCENC_INVALID_ARGUMENT) {
                    APITRACEERR("VCEncStrmEncode: DEC400 doesn't exist or format not "
                                "supported\n");
                    EWLReleaseCmdbuf(asic->ewl, pic->cmdbufid);
                    return VCENC_INVALID_ARGUMENT;
                }
                vcenc_instance->asic.regs.totalCmdbufSize = vcenc_instance->asic.regs.vcmdBufSize;
                EncCopyDataToCmdbuf(asic->ewl, &asic->regs);

                EncLinkRunCmdbuf(asic->ewl, &asic->regs);
            }
            pEncOut->consumedAddr.outbufBusAddr = pEncIn->busOutBuf[0];
        } else {
            // picture instance switch for multi-core encoding
            pic = vcenc_instance->pic[next_core_index];
            waitCoreJobid = vcenc_instance->waitJobid[next_core_index];

            if (vcenc_instance->pass == 1) {
                tmpTileExtra = pEncIn->tileExtra;

                *pEncOut = vcenc_instance->EncOut[next_core_index];
                *pEncIn = vcenc_instance->EncIn[next_core_index];

                pEncIn->tileExtra = tmpTileExtra;

                for (tileId = 1; tileId < vcenc_instance->num_tile_columns; tileId++) {
                    for (iBuf = 0; iBuf < MAX_STRM_BUF_NUM; iBuf++) {
                        pEncIn->tileExtra[tileId - 1].pOutBuf[iBuf] =
                            vcenc_instance->EncIn[cur_core_index]
                                .tileExtra[tileId - 1]
                                .pOutBuf[iBuf];
                        pEncIn->tileExtra[tileId - 1].busOutBuf[iBuf] =
                            vcenc_instance->EncIn[cur_core_index]
                                .tileExtra[tileId - 1]
                                .busOutBuf[iBuf];
                        pEncIn->tileExtra[tileId - 1].outBufSize[iBuf] =
                            vcenc_instance->EncIn[cur_core_index]
                                .tileExtra[tileId - 1]
                                .outBufSize[iBuf];
                        pEncIn->tileExtra[tileId - 1].cur_out_buffer[iBuf] =
                            vcenc_instance->EncIn[cur_core_index]
                                .tileExtra[tileId - 1]
                                .cur_out_buffer[iBuf];
                    }
                    if (tileId > 0) {
                        pEncOut->tileExtra[tileId - 1].pNaluSizeBuf =
                            vcenc_instance->EncOut[next_core_index]
                                .tileExtra[tileId - 1]
                                .pNaluSizeBuf;
                        pEncOut->tileExtra[tileId - 1].streamSize =
                            vcenc_instance->EncOut[next_core_index]
                                .tileExtra[tileId - 1]
                                .streamSize;
                        pEncOut->tileExtra[tileId - 1].numNalus =
                            vcenc_instance->EncOut[next_core_index].tileExtra[tileId - 1].numNalus;
                        pEncOut->tileExtra[tileId - 1].cuOutData =
                            vcenc_instance->EncOut[next_core_index].tileExtra[tileId - 1].cuOutData;
                    }
                }
            } else {
                pEncIn = &vcenc_instance->EncIn[next_core_index];

                pEncOut->consumedAddr.inputbufBusAddr =
                    vcenc_instance->EncIn[next_core_index].busLuma;
                pEncOut->consumedAddr.dec400TableBusAddr =
                    vcenc_instance->EncIn[next_core_index].dec400LumTableBase;
                if (vcenc_instance->pass == 2) {
                    //in 2pass, roiMapDeltaQpBusAddr had been return when pass2 get the job from cutree
                    pEncOut->consumedAddr.roiMapDeltaQpBusAddr = 0;
                    //need to return roibuffer (buffer inside API with deltaQp data generated by cutree) to buffer pool
                    outframe = vcenc_instance->lookaheadJob[next_core_index];
                } else
                    pEncOut->consumedAddr.roiMapDeltaQpBusAddr =
                        vcenc_instance->EncIn[next_core_index].roiMapDeltaQpAddr;
                pEncOut->consumedAddr.roimapCuCtrlInfoBusAddr =
                    vcenc_instance->EncIn[next_core_index].RoimapCuCtrlAddr;
                memcpy(pEncOut->consumedAddr.overlayInputBusAddr,
                       vcenc_instance->EncIn[next_core_index].overlayInputYAddr,
                       MAX_OVERLAY_NUM * sizeof(ptr_t));
                pEncOut->consumedAddr.outbufBusAddr =
                    vcenc_instance->EncIn[next_core_index].busOutBuf[0];
            }
            queue_remove(&c->picture, (struct node *)pic);
            pEncOut->numNalus = vcenc_instance->numNalus[next_core_index];
            memcpy(&vcenc_instance->stream, &vcenc_instance->streams[next_core_index],
                   sizeof(struct buffer));
            pEncOut->picture_cnt = pic->picture_cnt;
            pEncOut->poc = pic->poc;
        }

        for (tileId = 0; tileId < vcenc_instance->asic.regs.num_tile_columns; tileId++) {
            i32 retRecord = VCENC_OK;
            /* select a waitId
         single tile:
         choose pic->cmdbufid when vcmd enable, otherwise choose waitCoreJobid
         multi-tile:
         choose vcenc_instance->tileCtrl[tileId].job_id
      */
            u32 waitId = asic->regs.bVCMDEnable ? pic->cmdbufid : waitCoreJobid;
            waitId = vcenc_instance->num_tile_columns > 1 ? vcenc_instance->tileCtrl[tileId].job_id
                                                          : waitId;
            ret = VCEncStrmWaitReady(inst, pEncIn, pEncOut, pic, &slice_callback, c, waitId);
            retRecord = (retRecord == VCENC_OK) ? ret : retRecord;
            ret = retRecord;
            use_new_parameters = 0;
            if (vcenc_instance->cb_try_new_params && vcenc_instance->pass != 1) {
                regs_bak_for2nd_encode = &vcenc_instance->regs_bak[next_core_index];
                dec400_data = &vcenc_instance->dec400_data_bak[next_core_index];
                dec400_osd = &vcenc_instance->dec400_data_bak[next_core_index];
                VCEncCollectEncodeStats(vcenc_instance, pEncIn, pEncOut, pEncOut->codingType,
                                        &enc_statistic);
                use_new_parameters = vcenc_instance->cb_try_new_params(&enc_statistic, &new_params);
                if (use_new_parameters) {
                    VCEncResetEncodeStatus(vcenc_instance, c, pEncIn, pic, pEncOut, &enc_statistic);
                    VCEncRertryNewParameters(vcenc_instance, pEncIn, pEncOut, &slice_callback,
                                             &new_params, regs_bak_for2nd_encode);
                }
                re_encode_counter++;
            }
        }
    } while (re_encode_counter && use_new_parameters);

    if (ret != VCENC_OK)
        goto out;

    //set/clear picture compress flag
    pic->recon_compress.lumaCompressed = asic->regs.recon_luma_compress ? 1 : 0;
    pic->recon_compress.chromaCompressed = asic->regs.recon_chroma_compress ? 1 : 0;

    if (vcenc_instance->output_buffer_over_flow == 1) {
        vcenc_instance->stream.byteCnt = 0;
        pEncOut->streamSize = vcenc_instance->stream.byteCnt;
        pEncOut->pNaluSizeBuf[0] = 0;
        pEncOut->numNalus = 0;
        for (tileId = 1; tileId < asic->regs.num_tile_columns; tileId++) {
            pEncOut->tileExtra[tileId - 1].pNaluSizeBuf[0] = 0;
            pEncOut->tileExtra[tileId - 1].numNalus = 0;
        }
        vcenc_instance->encStatus = VCENCSTAT_START_FRAME;
        ret = VCENC_OUTPUT_BUFFER_OVERFLOW;
        /* revert frameNum update on output buffer overflow */
        if (IS_H264(vcenc_instance->codecFormat) && pic->nalRefIdc)
            vcenc_instance->frameNum--;
        APITRACEERR("VCEncStrmEncode: ERROR Output buffer too small\n");
        goto out;
    }

    i32 stat;
    {
        if (vcenc_instance->num_tile_columns > 1) {
            asic->regs.sumOfQP = 0;
            asic->regs.sumOfQPNumber = 0;
            asic->regs.picComplexity = 0;
            vcenc_instance->rateControl.ctbRateCtrl.qpSumForRc = 0;
            for (tileId = 0; tileId < vcenc_instance->num_tile_columns; tileId++) {
                asic->regs.sumOfQP += vcenc_instance->tileCtrl[tileId].sumOfQP;
                asic->regs.sumOfQPNumber += vcenc_instance->tileCtrl[tileId].sumOfQPNumber;
                asic->regs.picComplexity += vcenc_instance->tileCtrl[tileId].picComplexity;
                vcenc_instance->rateControl.ctbRateCtrl.qpSumForRc +=
                    vcenc_instance->tileCtrl[tileId].ctbRcQpSum;
            }
        }
#ifdef CTBRC_STRENGTH
        vcenc_instance->rateControl.rcPicComplexity = asic->regs.picComplexity;
        vcenc_instance->rateControl.complexity =
            (float)vcenc_instance->rateControl.rcPicComplexity *
            (vcenc_instance->rateControl.reciprocalOfNumBlk8);
#endif

        const enum slice_type _slctypes[3] = {P_SLICE, I_SLICE, B_SLICE};
        vcenc_instance->rateControl.sliceTypeCur = _slctypes[asic->regs.frameCodingType];
        stat = VCEncAfterPicRc(&vcenc_instance->rateControl, 0, vcenc_instance->stream.byteCnt,
                               asic->regs.sumOfQP, asic->regs.sumOfQPNumber);

        /* After HRD overflow discard the coded frame and go back old time,
        * just like not coded frame. But if only one reference frame
        * buffer is in use we can't discard the frame unless the next frame
        * is coded as intra. */
        if (stat == VCENCRC_OVERFLOW) {
            vcenc_instance->stream.byteCnt = 0;

            pEncOut->pNaluSizeBuf[0] = 0;
            pEncOut->numNalus = 0;
            for (tileId = 1; tileId < asic->regs.num_tile_columns; tileId++) {
                pEncOut->tileExtra[tileId - 1].pNaluSizeBuf[0] = 0;
                pEncOut->tileExtra[tileId - 1].numNalus = 0;
            }

            /* revert frameNum update on HRD overflow */
            if (IS_H264(vcenc_instance->codecFormat) && pic->nalRefIdc)
                vcenc_instance->frameNum--;
            //queue_put(&c->picture, (struct node *)pic);
            APITRACE("VCEncStrmEncode: OK, Frame discarded (HRD overflow)\n");
            //return VCENC_FRAME_READY;
        } else
            vcenc_instance->rateControl.sei.activated_sps = 1;
    }
#ifdef INTERNAL_TEST
    if (vcenc_instance->testId == TID_RFD) {
        HevcRFDTest(vcenc_instance, pic);
    }
#endif
    vcenc_instance->jobCnt++; // update frame count after one frame is ready.
    pEncOut->pNaluSizeBuf = (u32 *)vcenc_instance->asic
                                .sizeTbl[vcenc_instance->jobCnt % vcenc_instance->parallelCoreNum]
                                .virtualAddress;

    if (vcenc_instance->gdrEnabled == 1 && stat != VCENCRC_OVERFLOW) {
        if (vcenc_instance->gdrFirstIntraFrame != 0) {
            vcenc_instance->gdrFirstIntraFrame--;
        }
        if (vcenc_instance->gdrStart)
            vcenc_instance->gdrCount++;

        if (vcenc_instance->gdrCount ==
            (vcenc_instance->gdrDuration * (1 + (i32)vcenc_instance->interlaced))) {
            vcenc_instance->gdrStart--;
            vcenc_instance->gdrCount = 0;
            asic->regs.rcRoiEnable = 0x00;
        }
    }

    VCEncHEVCDnfUpdate(vcenc_instance);

    vcenc_instance->encStatus = VCENCSTAT_START_FRAME;
    ret = VCENC_FRAME_READY;
    vcenc_instance->outForCutree = pic->outForCutree;

    APITRACE("VCEncStrmEncode: OK\n");

out:
    sw_ref_cnt_decrease(pic);
    if (vcenc_instance->reservedCore)
        vcenc_instance->reservedCore--;
    if (vcenc_instance->pass == 2) {
        ReleaseLookaheadPicture(&vcenc_instance->lookahead, outframe,
                                pEncOut->consumedAddr.inputbufBusAddr);
    }

    if (VCENC_OK !=
        VCEncCodecPostEncodeUpdate(vcenc_instance, pEncIn, pEncOut, pEncOut->codingType, pic))
        return VCENC_ERROR;

    /* Put picture back to store */
    queue_put(&c->picture, (struct node *)pic);
    pEncOut->streamSize = vcenc_instance->num_tile_columns > 1
                              ? vcenc_instance->tileCtrl[0].streamSize
                              : vcenc_instance->stream.byteCnt;
    for (tileId = 1; tileId < asic->regs.num_tile_columns; tileId++) {
        pEncOut->tileExtra[tileId - 1].streamSize =
            asic->regs.tiles_enabled_flag ? vcenc_instance->tileCtrl[tileId].streamSize
                                          : vcenc_instance->stream.byteCnt;
    }

    return ret;
}
/*------------------------------------------------------------------------------

    Function name : VCEncFlush
    Description   : Flush remaining frames at the end of encoding.
    Return type   : VCEncRet
    Argument      : inst - encoder instance
    Argument      : pEncIn - user provided input parameters
    Argument      : pEncOut - place where output info is returned
    Argument      : sliceReadyCbFunc - slice ready callback function
    Argument      : pAppData - needed by sliceReadyCbFunc
------------------------------------------------------------------------------*/
VCEncRet VCEncFlush(VCEncInst inst, VCEncIn *pEncIn, VCEncOut *pEncOut,
                    VCEncSliceReadyCallBackFunc sliceReadyCbFunc, void *pAppData)
{
    VCEncRet ret = VCENC_OK;
    struct vcenc_instance *vcenc_instance = (struct vcenc_instance *)inst;
    if (vcenc_instance->pass == 0) {
        // single pass flush
        vcenc_instance->bFlush = HANTRO_TRUE;
        ret = VCEncStrmEncode(inst, pEncIn, pEncOut, sliceReadyCbFunc, pAppData);
        if (ret != VCENC_OK)
            return ret;
    } else if (vcenc_instance->pass == 2) {
        // Lookahead flush
        vcenc_instance->bFlush = HANTRO_TRUE;
        ret = VCEncStrmEncodeExt(inst, pEncIn, NULL, pEncOut, NULL, NULL, 0);
        if (ret != VCENC_OK)
            return ret;
    }
    // Multicore flush
    if (vcenc_instance->reservedCore > 0)
        return VCEncMultiCoreFlush(inst, pEncIn, pEncOut, sliceReadyCbFunc);

    return VCENC_OK;
}

/*------------------------------------------------------------------------------

    Function name : VCEncStrmEnd
    Description   : Ends a stream
    Return type   : VCEncRet
    Argument      : inst - encoder instance
    Argument      : pEncIn - user provided input parameters
                    pEncOut - place where output info is returned
------------------------------------------------------------------------------*/
VCEncRet VCEncStrmEnd(VCEncInst inst, const VCEncIn *pEncIn, VCEncOut *pEncOut)
{
    /* Status == INIT   Stream ended, next stream can be started */
    struct vcenc_instance *vcenc_instance = (struct vcenc_instance *)inst;
    APITRACE("VCEncStrmEnd#\n");
    APITRACEPARAM_X(" %s : %p\n", "busLuma", pEncIn->busLuma);
    APITRACEPARAM_X(" %s : %p\n", "busChromaU", pEncIn->busChromaU);
    APITRACEPARAM_X(" %s : %p\n", "busChromaV", pEncIn->busChromaV);
    APITRACEPARAM(" %s : %d\n", "timeIncrement", pEncIn->timeIncrement);
    APITRACEPARAM_X(" %s : %p\n", "pOutBuf", pEncIn->pOutBuf[0]);
    APITRACEPARAM_X(" %s : %p\n", "busOutBuf", pEncIn->busOutBuf[0]);
    APITRACEPARAM(" %s : %d\n", "outBufSize", pEncIn->outBufSize[0]);
    APITRACEPARAM(" %s : %d\n", "codingType", pEncIn->codingType);
    APITRACEPARAM(" %s : %d\n", "poc", pEncIn->poc);
    APITRACEPARAM(" %s : %d\n", "gopSize", pEncIn->gopSize);
    APITRACEPARAM(" %s : %d\n", "gopPicIdx", pEncIn->gopPicIdx);
    APITRACEPARAM_X(" %s : %p\n", "roiMapDeltaQpAddr", pEncIn->roiMapDeltaQpAddr);

    struct container *c;
    /* Check for illegal inputs */
    if ((vcenc_instance == NULL) || (pEncIn == NULL) || (pEncOut == NULL)) {
        APITRACEERR("VCEncStrmEnd: ERROR Null argument\n");
        return VCENC_NULL_ARGUMENT;
    }

    /* Check for existing instance */
    if (vcenc_instance->inst != vcenc_instance) {
        APITRACEERR("VCEncStrmEnd: ERROR Invalid instance\n");
        return VCENC_INSTANCE_ERROR;
    }

    /* Check status, this also makes sure that the instance is valid */
    if ((vcenc_instance->encStatus != VCENCSTAT_START_FRAME) &&
        (vcenc_instance->encStatus != VCENCSTAT_START_STREAM)) {
        APITRACEERR("VCEncStrmEnd: ERROR Invalid status\n");
        return VCENC_INVALID_STATUS;
    }

    if (vcenc_instance->pass == 1) {
        vcenc_instance->stream.stream = (u8 *)vcenc_instance->lookahead.internal_mem.pOutBuf;
        vcenc_instance->stream.stream_bus = vcenc_instance->lookahead.internal_mem.busOutBuf;
        vcenc_instance->stream.size = vcenc_instance->lookahead.internal_mem.outBufSize;
    } else {
        vcenc_instance->stream.stream = (u8 *)pEncIn->pOutBuf[0];
        vcenc_instance->stream.stream_bus = pEncIn->busOutBuf[0];
        vcenc_instance->stream.size = pEncIn->outBufSize[0];
    }
    vcenc_instance->stream.cnt = &vcenc_instance->stream.byteCnt;

    pEncOut->streamSize = 0;
    vcenc_instance->stream.byteCnt = 0;

    /* Set pointer to the beginning of NAL unit size buffer */
    pEncOut->pNaluSizeBuf = (u32 *)vcenc_instance->asic.sizeTbl[0].virtualAddress;
    pEncOut->numNalus = 0;

    /* Clear the NAL unit size table */
    if (pEncOut->pNaluSizeBuf != NULL)
        pEncOut->pNaluSizeBuf[0] = 0;

    EndOfSequence(vcenc_instance, pEncIn, pEncOut);

    pEncOut->streamSize = vcenc_instance->stream.byteCnt;
    if ((IS_HEVC(vcenc_instance->codecFormat) || IS_H264(vcenc_instance->codecFormat))) {
        pEncOut->numNalus = 1;
        pEncOut->pNaluSizeBuf[0] = vcenc_instance->stream.byteCnt;
        pEncOut->pNaluSizeBuf[1] = 0;
    }

    if (vcenc_instance->pass == 2 && vcenc_instance->lookahead.priv_inst) {
        VCEncRet ret = VCENC_OK;
        VCEncOut encOut;
        VCEncIn encIn;
        memcpy(&encIn, pEncIn, sizeof(VCEncIn));
        if (vcenc_instance->num_tile_columns > 1) {
            encOut.tileExtra = vcenc_instance->EncOut[0].tileExtra;
            if (encOut.tileExtra == NULL)
                return VCENC_ERROR;
        } else
            encOut.tileExtra = NULL;
        encIn.gopConfig.pGopPicCfg = pEncIn->gopConfig.pGopPicCfgPass1;
        ret = VCEncStrmEnd(vcenc_instance->lookahead.priv_inst, &encIn, &encOut);
        if (ret != VCENC_OK) {
            APITRACE("VCEncStrmEnd: LookaheadStrmEnd failed\n");
            return ret;
        }
        struct vcenc_instance *vcenc_instance_priv =
            (struct vcenc_instance *)vcenc_instance->lookahead.priv_inst;
    }

    /* Status == INIT   Stream ended, next stream can be started */
    vcenc_instance->encStatus = VCENCSTAT_INIT;

    //set consumed buffer
    pEncOut->consumedAddr.inputbufBusAddr = 0;
    pEncOut->consumedAddr.dec400TableBusAddr = 0;
    pEncOut->consumedAddr.roiMapDeltaQpBusAddr = 0;
    pEncOut->consumedAddr.roimapCuCtrlInfoBusAddr = 0;
    memset(pEncOut->consumedAddr.overlayInputBusAddr, 0, MAX_OVERLAY_NUM * sizeof(ptr_t));
    pEncOut->consumedAddr.outbufBusAddr = pEncIn->busOutBuf[0];

    APITRACE("VCEncStrmEnd: OK\n");
    return VCENC_OK;
}

/*------------------------------------------------------------------------------
    Function name : VCEncSetError
    Description   : set error flag for thared flushing
    Return type   : NULL
------------------------------------------------------------------------------*/
void VCEncSetError(VCEncInst inst)
{
    struct vcenc_instance *vcenc_instance = (struct vcenc_instance *)inst;
    vcenc_instance->encStatus = VCENCSTAT_ERROR;
}

/*------------------------------------------------------------------------------
    Function name : VCEncGetBitsPerPixel
    Description   : Returns the amount of bits per pixel for given format.
    Return type   : u32
------------------------------------------------------------------------------*/
u32 VCEncGetBitsPerPixel(VCEncPictureType type)
{
    switch (type) {
    case VCENC_YUV420_PLANAR:
    case VCENC_YVU420_PLANAR:
    case VCENC_YUV420_SEMIPLANAR:
    case VCENC_YUV420_SEMIPLANAR_VU:
        return 12;
    case VCENC_YUV422_INTERLEAVED_YUYV:
    case VCENC_YUV422_INTERLEAVED_UYVY:
    case VCENC_RGB565:
    case VCENC_BGR565:
    case VCENC_RGB555:
    case VCENC_BGR555:
    case VCENC_RGB444:
    case VCENC_BGR444:
        return 16;
    case VCENC_RGB888:
    case VCENC_BGR888:
    case VCENC_RGB101010:
    case VCENC_BGR101010:
        return 32;
    case VCENC_RGB888_24BIT:
    case VCENC_BGR888_24BIT:
    case VCENC_RBG888_24BIT:
    case VCENC_GBR888_24BIT:
    case VCENC_BRG888_24BIT:
    case VCENC_GRB888_24BIT:
    case VCENC_YUV420_PLANAR_10BIT_I010:
    case VCENC_YUV420_PLANAR_10BIT_P010:
        return 24;
    case VCENC_YUV420_PLANAR_10BIT_PACKED_PLANAR:
        return 15;
    case VCENC_YUV420_10BIT_PACKED_Y0L2:
        return 16;
    default:
        return 0;
    }
}

/*------------------------------------------------------------------------------
    Function name : VCEncGetAlignedStride
    Description   : Returns the stride in byte by given aligment and format.
    Return type   : u32
------------------------------------------------------------------------------*/
u32 VCEncGetAlignedStride(int width, i32 input_format, u32 *luma_stride, u32 *chroma_stride,
                          u32 input_alignment)
{
    return EncGetAlignedByteStride(width, input_format, luma_stride, chroma_stride,
                                   input_alignment);
}

/*------------------------------------------------------------------------------
    Function name : VCEncSetTestId
    Description   : Sets the encoder configuration according to a test vector
    Return type   : VCEncRet
    Argument      : inst - encoder instance
    Argument      : testId - test vector ID
------------------------------------------------------------------------------*/
VCEncRet VCEncSetTestId(VCEncInst inst, u32 testId)
{
    struct vcenc_instance *pEncInst = (struct vcenc_instance *)inst;
    (void)pEncInst;
    (void)testId;

    APITRACE("VCEncSetTestId#\n");

#ifdef INTERNAL_TEST
    pEncInst->testId = testId;
    /* some tests need to update related params at once */
    EncConfigureTestId(pEncInst);

    APITRACE("VCEncSetTestId# OK\n");
    return VCENC_OK;
#else
    /* Software compiled without testing support, return error always */
    APITRACEERR("VCEncSetTestId# ERROR, testing disabled at compile time\n");
    return VCENC_ERROR;
#endif
}

/*------------------------------------------------------------------------------
  out_of_memory
------------------------------------------------------------------------------*/
i32 out_of_memory(struct vcenc_buffer *n, u32 size)
{
    return (size > n->cnt);
}

/*------------------------------------------------------------------------------
  init_buffer
------------------------------------------------------------------------------*/
i32 init_buffer(struct vcenc_buffer *vcenc_buffer, struct vcenc_instance *vcenc_instance)
{
    if (!vcenc_instance->temp_buffer)
        return NOK;
    vcenc_buffer->next = NULL;
    vcenc_buffer->buffer = vcenc_instance->temp_buffer;
    vcenc_buffer->cnt = vcenc_instance->temp_size;
    vcenc_buffer->busaddr = vcenc_instance->temp_bufferBusAddress;
    return OK;
}

/*------------------------------------------------------------------------------
  get_buffer
------------------------------------------------------------------------------*/
i32 get_buffer(struct buffer *buffer, struct vcenc_buffer *source, i32 size, i32 reset)
{
    struct vcenc_buffer *vcenc_buffer;

    if (size < 0)
        return NOK;
    memset(buffer, 0, sizeof(struct buffer));

    /* New vcenc_buffer node */
    if (out_of_memory(source, sizeof(struct vcenc_buffer)))
        return NOK;
    vcenc_buffer = (struct vcenc_buffer *)source->buffer;
    if (reset)
        memset(vcenc_buffer, 0, sizeof(struct vcenc_buffer));
    source->buffer += sizeof(struct vcenc_buffer);
    source->busaddr += sizeof(struct vcenc_buffer);
    source->cnt -= sizeof(struct vcenc_buffer);

    /* Connect previous vcenc_buffer node */
    if (source->next)
        source->next->next = vcenc_buffer;
    source->next = vcenc_buffer;

    /* Map internal buffer and user space vcenc_buffer, NOTE that my chunk
   * size is sizeof(struct vcenc_buffer) */
    size = (size / sizeof(struct vcenc_buffer)) * sizeof(struct vcenc_buffer);
    if (out_of_memory(source, size))
        return NOK;
    vcenc_buffer->buffer = source->buffer;
    vcenc_buffer->busaddr = source->busaddr;
    buffer->stream = source->buffer;
    buffer->stream_bus = source->busaddr;
    buffer->size = size;
    buffer->cnt = &vcenc_buffer->cnt;
    source->buffer += size;
    source->busaddr += size;
    source->cnt -= size;

    return OK;
}

/*------------------------------------------------------------------------------
  vcenc_set_ref_pic_set
------------------------------------------------------------------------------*/
i32 vcenc_set_ref_pic_set(struct vcenc_instance *vcenc_instance)
{
    struct container *c;
    struct vcenc_buffer source;
    struct rps *r;

    if (!(c = get_container(vcenc_instance)))
        return NOK;

    /* Create new reference picture set */
    if (!(r = (struct rps *)create_parameter_set(RPS)))
        return NOK;

    /* We will read vcenc_instance->buffer */
    if (init_buffer(&source, vcenc_instance))
        goto out;
    if (get_buffer(&r->ps.b, &source, RPS_SET_BUFF_SIZE, HANTRO_FALSE))
        goto out;

    /* Initialize parameter set id */
    r->ps.id = vcenc_instance->rps_id;
    r->sps_id = vcenc_instance->sps_id;

    if (set_reference_pic_set(r))
        goto out;

    /* Replace old reference picture set with new one */
    remove_parameter_set(c, RPS, vcenc_instance->rps_id);
    queue_put(&c->parameter_set, (struct node *)r);

    return OK;

out:
    free_parameter_set((struct ps *)r);
    return NOK;
}

/*------------------------------------------------------------------------------
  vcenc_ref_pic_sets
------------------------------------------------------------------------------*/
i32 vcenc_ref_pic_sets(struct vcenc_instance *vcenc_instance, const VCEncIn *pEncIn)
{
    struct vcenc_buffer *vcenc_buffer;
    const VCEncGopConfig *gopCfg;
    i32 *poc;
    i32 rps_id = 0;
    struct container *c;

    /* Initialize tb->instance->buffer so that we can cast it to vcenc_buffer
   * by calling hevc_set_parameter(), see api.c TODO...*/
    // free parameter set if any exists
    if (!(c = get_container(vcenc_instance)))
        return NOK;
    free_parameter_set_queue(c);

    vcenc_instance->rps_id = -1;
    vcenc_set_ref_pic_set(vcenc_instance);
    vcenc_buffer = (struct vcenc_buffer *)vcenc_instance->temp_buffer;
    poc = (i32 *)vcenc_buffer->buffer;

    gopCfg = &pEncIn->gopConfig;

    //RPS based on user GOP configuration
    int i, j;
    rps_id = 0;

    for (i = 0; i < gopCfg->size; i++) {
        VCEncGopPicConfig *cfg = &(gopCfg->pGopPicCfg[i]);
        u32 iRefPic, idx = 0;
        for (iRefPic = 0; iRefPic < cfg->numRefPics; iRefPic++) {
            if (IS_LONG_TERM_REF_DELTAPOC(cfg->refPics[iRefPic].ref_pic))
                continue;
            poc[idx++] = cfg->refPics[iRefPic].ref_pic;
            poc[idx++] = cfg->refPics[iRefPic].used_by_cur;
        }
        for (j = 0; j < gopCfg->ltrcnt; j++) {
            poc[idx++] = gopCfg->u32LTR_idx[j];
            poc[idx++] = 0;
        }
        poc[idx] = 0; //end
        vcenc_instance->rps_id = rps_id;
        if (vcenc_set_ref_pic_set(vcenc_instance)) {
            Error(2, ERR, "vcenc_set_ref_pic_set() fails");
            return NOK;
        }
        rps_id++;
    }

    if (0 != gopCfg->special_size) { //
        for (i = 0; i < gopCfg->special_size; i++) {
            VCEncGopPicSpecialConfig *cfg = &(gopCfg->pGopPicSpecialCfg[i]);
            u32 iRefPic, idx = 0;
            if (((i32)cfg->numRefPics) != NUMREFPICS_RESERVED) {
                for (iRefPic = 0; iRefPic < cfg->numRefPics; iRefPic++) {
                    if (IS_LONG_TERM_REF_DELTAPOC(cfg->refPics[iRefPic].ref_pic))
                        continue;
                    poc[idx++] = cfg->refPics[iRefPic].ref_pic;
                    poc[idx++] = cfg->refPics[iRefPic].used_by_cur;
                }
            }

            // add LTR idx to default GOP setting auto loaded
            for (j = 0; j < gopCfg->ltrcnt; j++) {
                poc[idx++] = gopCfg->u32LTR_idx[j];
                poc[idx++] = 0;
            }
            poc[idx] = 0; //end
            vcenc_instance->rps_id = rps_id;
            if (vcenc_set_ref_pic_set(vcenc_instance)) {
                Error(2, ERR, "vcenc_set_ref_pic_set() fails");
                return NOK;
            }
            rps_id++;
        }
    }

    //add some special RPS configuration
    //P frame for sequence starting
    poc[0] = -1;
    poc[1] = 1;
    for (i = 1; i < gopCfg->ltrcnt; i++) {
        poc[2 * i] = gopCfg->u32LTR_idx[i - 1];
        poc[2 * i + 1] = 0;
    }
    poc[2 * i] = 0;

    vcenc_instance->rps_id = rps_id;
    if (vcenc_set_ref_pic_set(vcenc_instance)) {
        Error(2, ERR, "vcenc_set_ref_pic_set() fails");
        return NOK;
    }
    rps_id++;
    //Intra
    for (i = 0; i < gopCfg->ltrcnt; i++) {
        poc[2 * i] = gopCfg->u32LTR_idx[i];
        poc[2 * i + 1] = 0;
    }
    poc[2 * i] = 0;

    vcenc_instance->rps_id = rps_id;
    if (vcenc_set_ref_pic_set(vcenc_instance)) {
        Error(2, ERR, "vcenc_set_ref_pic_set() fails");
        return NOK;
    }
    return OK;
}

/*------------------------------------------------------------------------------
  vcenc_get_ref_pic_set
------------------------------------------------------------------------------*/
i32 vcenc_get_ref_pic_set(struct vcenc_instance *vcenc_instance)
{
    struct container *c;
    struct vcenc_buffer source;
    struct ps *p;

    if (!(c = get_container(vcenc_instance)))
        return NOK;
    if (!(p = get_parameter_set(c, RPS, vcenc_instance->rps_id)))
        return NOK;
    if (init_buffer(&source, vcenc_instance))
        return NOK;
    if (get_buffer(&p->b, &source, PRM_SET_BUFF_SIZE, HANTRO_TRUE))
        return NOK;
    if (get_reference_pic_set((struct rps *)p))
        return NOK;

    return OK;
}

u32 VCEncGetRoiMapVersion(u32 core_id, void *ctx)
{
    EWLHwConfig_t cfg = EWLReadAsicConfig(core_id, ctx);

    return cfg.roiMapVersion;
}

/**
 * \addtogroup api_video
 *
 * @{
 */
/**
 * Checks if the configuration is valid or match with hardware feature.
 *
 * \param enc_cfg [in] Pointer to configure for current encoder instance
 * \param asic_cfg [in] Pointer to configure of current hardware.
 *
 * \return ENCHW_OK      The configuration is valid.
 * \return ENCHW_NOK     Some of the parameters in configuration are not valid.
 */
bool_e VCEncCheckCfg(const VCEncConfig *enc_cfg, const EWLHwConfig_t *asic_cfg, void *ctx)
{
    i32 height = enc_cfg->height;
    u32 client_type, asicHwId;

    ASSERT(enc_cfg);

    if ((enc_cfg->streamType != VCENC_BYTE_STREAM) &&
        (enc_cfg->streamType != VCENC_NAL_UNIT_STREAM)) {
        APITRACEERR("VCEncCheckCfg: Invalid stream type\n");
        return ENCHW_NOK;
    }

/* Supported codec check */
#ifndef SUPPORT_HEVC
    if (IS_HEVC(enc_cfg->codecFormat)) {
        APITRACEERR("VCEncCheckCfg: codecFormat=hevc not supported.\n");
        return ENCHW_NOK;
    }
#endif
#ifndef SUPPORT_H264
    if (IS_H264(enc_cfg->codecFormat)) {
        APITRACEERR("VCEncCheckCfg: codecFormat=h264 not supported.\n");
        return ENCHW_NOK;
    }
#endif
#ifndef SUPPORT_AV1
    if (IS_AV1(enc_cfg->codecFormat)) {
        APITRACEERR("VCEncCheckCfg: codecFormat=av1 not supported.\n");
        return ENCHW_NOK;
    }
#endif
#ifndef SUPPORT_VP9
    if (IS_VP9(enc_cfg->codecFormat)) {
        APITRACEERR("VCEncCheckCfg: codecFormat=vp9 not supported.\n");
        return ENCHW_NOK;
    }
#endif

    if (IS_AV1(enc_cfg->codecFormat) || IS_VP9(enc_cfg->codecFormat)) {
        if (enc_cfg->streamType != VCENC_BYTE_STREAM) {
            APITRACEERR("VCEncCheckCfg: Invalid stream type, need byte stream when AV1 or "
                        "VP9\n");
            return ENCHW_NOK;
        }

        if (IS_AV1(enc_cfg->codecFormat)) {
            if (enc_cfg->width > VCENC_AV1_MAX_ENC_WIDTH) {
                APITRACEERR("VCEncCheckCfg: Invalid width, need 4096 or smaller when AV1\n");
                return ENCHW_NOK;
            }

            if (enc_cfg->width * enc_cfg->height > VCENC_AV1_MAX_ENC_AREA) {
                APITRACEERR("VCEncCheckCfg: Invalid area, need 4096*2304 or below when AV1\n");
                return ENCHW_NOK;
            }
        } else {
            /* In the VP9's spec, the picture's width should not be larger than 8384,
      * but when only ONE tile, the picture's width should NOT be larger than 4096.
      */
            if (enc_cfg->width > VCENC_VP9_MAX_ENC_WIDTH) {
                APITRACEERR("VCEncCheckCfg: Invalid width, need 4096 or smaller when only one "
                            "tile in the VP9\n");
                return ENCHW_NOK;
            } else if (enc_cfg->height > VCENC_VP9_MAX_ENC_HEIGHT) {
                APITRACEERR("VCEncCheckCfg: Invalid height, need 8384 or smaller when VP9\n");
                return ENCHW_NOK;
            }

            if (enc_cfg->width * enc_cfg->height > VCENC_VP9_MAX_ENC_AREA) {
                APITRACEERR("VCEncCheckCfg: Invalid picture size, need 4096*2176 or below when "
                            "VP9\n");
                return ENCHW_NOK;
            }
        }
    }

    /* Encoded image width limits, multiple of 2 */
    if (enc_cfg->num_tile_columns == 1 &&
        ((enc_cfg->width < VCENC_MIN_ENC_WIDTH) ||
         (enc_cfg->width > VCEncGetMaxWidth(asic_cfg, enc_cfg->codecFormat)) ||
         (enc_cfg->width & 0x1) != 0)) {
        APITRACEERR("VCEncCheckCfg: Invalid width\n");
        return ENCHW_NOK;
    }

    /* Encoded image height limits, multiple of 2 */
    if (enc_cfg->num_tile_rows == 1 && (height < VCENC_MIN_ENC_HEIGHT ||
                                        height > VCENC_MAX_ENC_HEIGHT_EXT || (height & 0x1) != 0)) {
        APITRACEERR("VCEncCheckCfg: Invalid height\n");
        return ENCHW_NOK;
    }

    /* Check frame rate */
    if (enc_cfg->frameRateNum < 1 || enc_cfg->frameRateNum > ((1 << 20) - 1)) {
        APITRACEERR("VCEncCheckCfg: Invalid frameRateNum\n");
        return ENCHW_NOK;
    }

    if (enc_cfg->frameRateDenom < 1) {
        APITRACEERR("VCEncCheckCfg: Invalid frameRateDenom\n");
        return ENCHW_NOK;
    }

    /* check profile */
    {
        client_type = VCEncGetClientType(enc_cfg->codecFormat);
        asicHwId = EncAsicGetAsicHWid(client_type, ctx);
        u8 h264Constrain =
            (IS_H264(enc_cfg->codecFormat) && (enc_cfg->profile > VCENC_H264_HIGH_10_PROFILE ||
                                               enc_cfg->profile < VCENC_H264_BASE_PROFILE));
        u8 h2Constrain = (((HW_ID_MAJOR_NUMBER(asicHwId)) < 4) && HW_PRODUCT_H2(asicHwId) &&
                          (enc_cfg->profile == VCENC_HEVC_MAIN_10_PROFILE));
        u8 hevcConstrain =
            (IS_HEVC(enc_cfg->codecFormat) && (enc_cfg->profile > VCENC_HEVC_MAINREXT));
        if ((i32)enc_cfg->profile < VCENC_HEVC_MAIN_PROFILE || h264Constrain || hevcConstrain ||
            h2Constrain) {
            APITRACEERR("VCEncCheckCfg: Invalid profile\n");
            return ENCHW_NOK;
        }
    }

    /* check bit depth for main8 and main still profile */
    if (enc_cfg->profile < (IS_H264(enc_cfg->codecFormat) ? VCENC_H264_HIGH_10_PROFILE
                                                          : VCENC_HEVC_MAIN_10_PROFILE) &&
        (enc_cfg->bitDepthLuma != 8 || enc_cfg->bitDepthChroma != 8) &&
        (IS_HEVC(enc_cfg->codecFormat) || IS_H264(enc_cfg->codecFormat))) {
        APITRACEERR("VCEncCheckCfg: Invalid bit depth for the profile\n");
        return ENCHW_NOK;
    }

    /* check if hardware support 10 bit encoding. */
    if ((enc_cfg->bitDepthLuma != 8 || enc_cfg->bitDepthChroma != 8) &&
        (asic_cfg->main10Enabled == 0)) {
        APITRACEERR("VCEncCheckCfg: Not support higher bit depth in current hardware\n");
        return ENCHW_NOK;
    }

    if (enc_cfg->codedChromaIdc > VCENC_CHROMA_IDC_420) {
        APITRACEERR("VCEncCheckCfg: Invalid codedChromaIdc, it's should be 0 ~ 1\n");
        return ENCHW_NOK;
    }

    /* check bit depth for main10 profile */
    if (((enc_cfg->profile == VCENC_HEVC_MAIN_10_PROFILE) ||
         (enc_cfg->profile == VCENC_H264_HIGH_10_PROFILE)) &&
        ((enc_cfg->bitDepthLuma < 8) || (enc_cfg->bitDepthLuma > 10) ||
         (enc_cfg->bitDepthChroma < 8) || (enc_cfg->bitDepthChroma > 10))) {
        APITRACEERR("VCEncCheckCfg: Invalid bit depth for main10 profile\n");
        return ENCHW_NOK;
    }

    /* check level, if the level==VCENC_AUTO_LEVEL, don't check. */
    if (((IS_HEVC(enc_cfg->codecFormat) &&
          ((enc_cfg->level > VCENC_HEVC_MAX_LEVEL) || (enc_cfg->level < VCENC_HEVC_LEVEL_1))) ||
         (IS_H264(enc_cfg->codecFormat) &&
          ((enc_cfg->level > VCENC_H264_MAX_LEVEL) || (enc_cfg->level < VCENC_H264_LEVEL_1)) &&
          (enc_cfg->level != VCENC_H264_LEVEL_1_b))) &&
        (enc_cfg->level != VCENC_AUTO_LEVEL)) {
        APITRACEERR("VCEncCheckCfg: Invalid level\n");
        return ENCHW_NOK;
    }

    if (((i32)enc_cfg->tier > (i32)VCENC_HEVC_HIGH_TIER) ||
        ((i32)enc_cfg->tier < (i32)VCENC_HEVC_MAIN_TIER)) {
        APITRACEERR("VCEncCheckCfg: Invalid tier\n");
        return ENCHW_NOK;
    }

    if ((enc_cfg->tier == VCENC_HEVC_HIGH_TIER) &&
        (IS_H264(enc_cfg->codecFormat) || enc_cfg->level < VCENC_HEVC_LEVEL_4)) {
        APITRACEERR("VCEncCheckCfg: Invalid codec/level for chosen tier\n");
        return ENCHW_NOK;
    }

    if (enc_cfg->refFrameAmount > VCENC_MAX_REF_FRAMES) {
        APITRACEERR("VCEncCheckCfg: Invalid refFrameAmount\n");
        return ENCHW_NOK;
    }

    if ((enc_cfg->exp_of_input_alignment < 4 && enc_cfg->exp_of_input_alignment > 0) ||
        (enc_cfg->exp_of_ref_alignment < 4 && enc_cfg->exp_of_ref_alignment > 0) ||
        (enc_cfg->exp_of_ref_ch_alignment < 4 && enc_cfg->exp_of_ref_ch_alignment > 0)) {
        APITRACEERR("VCEncCheckCfg: Invalid alignment value\n");
        return ENCHW_NOK;
    }

    if (IS_H264(enc_cfg->codecFormat) &&
        (enc_cfg->picOrderCntType != 2 && (enc_cfg->picOrderCntType != 0))) {
        APITRACEERR("VCEncCheckCfg: H264 POCCntType support 0 or 2\n");
        return ENCHW_NOK;
    }

    if (IS_HEVC(enc_cfg->codecFormat) && enc_cfg->num_tile_columns > 1 &&
        enc_cfg->parallelCoreNum > 1) {
        APITRACEERR("VCEncCheckCfg: Invalid parallelCoreNum, multi-tile-columns encoding "
                    "only support parallelCoreNum = 1 when num_tile_columns > 1. \n");
        return ENCHW_NOK;
    }

    EWLHwConfig_t cfg = EncAsicGetAsicConfig(client_type, ctx);
    /* is HEVC encoding supported */
    if (IS_HEVC(enc_cfg->codecFormat) && cfg.hevcEnabled == EWL_HW_CONFIG_NOT_SUPPORTED) {
        APITRACEERR("VCEncCheckCfg: Invalid format, hevc not supported by HW coding "
                    "core\n");
        return ENCHW_NOK;
    }

    /* is H264 encoding supported */
    if (IS_H264(enc_cfg->codecFormat) && cfg.h264Enabled == EWL_HW_CONFIG_NOT_SUPPORTED) {
        APITRACEERR("VCEncCheckCfg: Invalid format, h264 not supported by HW coding "
                    "core\n");
        return ENCHW_NOK;
    }

    /* is AV1 encoding supported */
    if (IS_AV1(enc_cfg->codecFormat) && cfg.av1Enabled == EWL_HW_CONFIG_NOT_SUPPORTED) {
        APITRACEERR("VCEncCheckCfg: Invalid format, av1 not supported by HW coding core\n");
        return ENCHW_NOK;
    }

    /* is AV1 encoding supported */
    if (IS_VP9(enc_cfg->codecFormat) && cfg.vp9Enabled == EWL_HW_CONFIG_NOT_SUPPORTED) {
        APITRACEERR("VCEncCheckCfg: Invalid format, VP9 not supported by HW coding core\n");
        return ENCHW_NOK;
    }

    if ((cfg.cuInforVersion == 0x07) && enc_cfg->enableOutputCuInfo) {
        APITRACEERR("VCEncCheckCfg: Invalid enableOutputCuInfo, not supported by HW coding "
                    "core\n");
        return ENCHW_NOK;
    }

    /* max width supported */
    if (enc_cfg->num_tile_columns == 1 &&
        ((IS_H264(enc_cfg->codecFormat) ? cfg.maxEncodedWidthH264 : cfg.maxEncodedWidthHEVC) <
         enc_cfg->width)) {
        APITRACEERR("VCEncCheckCfg: Invalid width, not supported by HW coding core\n");
        return ENCHW_NOK;
    }

    /* P010 Ref format */
    if (enc_cfg->P010RefEnable && cfg.P010RefSupport == EWL_HW_CONFIG_NOT_SUPPORTED) {
        APITRACEERR("VCEncCheckCfg: Invalid format, P010Ref not supported by HW coding "
                    "core\n");
        return ENCHW_NOK;
    }

    /*extend video coding height */
    if (enc_cfg->num_tile_rows == 1 && (height > VCENC_MAX_ENC_HEIGHT) &&
        (cfg.videoHeightExt == EWL_HW_CONFIG_NOT_SUPPORTED)) {
        APITRACEERR("VCEncCheckCfg: Invalid height, height extension not supported by HW "
                    "coding core\n");
        return ENCHW_NOK;
    }

    /*disable recon write to DDR*/
    if (enc_cfg->writeReconToDDR == 0 && cfg.disableRecWtSupport == EWL_HW_CONFIG_NOT_SUPPORTED) {
        APITRACEERR("VCEncCheckCfg: disable recon write to DDR not supported by HW coding "
                    "core\n");
        return ENCHW_NOK;
    }

    /* Check ctb bits  */
    if (enc_cfg->enableOutputCtbBits && cfg.CtbBitsOutSupport == EWL_HW_CONFIG_NOT_SUPPORTED) {
        APITRACEERR("VCEncCheckCfg: ctb output encoded bits not supported by HW coding "
                    "core\n");
        return ENCHW_NOK;
    }

    if (enc_cfg->tune > VCENC_TUNE_SHARP_VISUAL) {
        APITRACEERR("VCEncCheckCfg: INVALID tune, it should be within [0..3]\n");
        return ENCHW_NOK;
    }

    if (enc_cfg->enableTMVP) {
        i32 tmvpSupport = IS_VP9(enc_cfg->codecFormat) ||
                          (asic_cfg->temporalMvpSupport && !IS_H264(enc_cfg->codecFormat));
        if (!tmvpSupport) {
            APITRACEERR("VCEncCheckCfg: TMVP is not supported for this codec format\n");
            return ENCHW_NOK;
        }
    }

    if (enc_cfg->refRingBufEnable && (asic_cfg->refRingBufEnable == EWL_HW_CONFIG_NOT_SUPPORTED)) {
        APITRACEERR("VCEncCheckCfg: ringbuffer is not supported\n");
        return ENCHW_NOK;
    }

    if (enc_cfg->refRingBufEnable && (enc_cfg->gopSize != 1)) {
        APITRACEERR("VCEncCheckCfg: ringbuffer is only supported for pFrame\n");
        return ENCHW_NOK;
    }

    return ENCHW_OK;
}

static i32 getLevelIdx(VCEncVideoCodecFormat codecFormat, VCEncLevel level)
{
    switch (codecFormat) {
    case VCENC_VIDEO_CODEC_HEVC:
        return getlevelIdxHevc(level);

    case VCENC_VIDEO_CODEC_H264:
        return getlevelIdxH264(level);

    case VCENC_VIDEO_CODEC_AV1:
        return CLIP3(0, AV1_VALID_MAX_LEVEL - 1, (i32)level);

    case VCENC_VIDEO_CODEC_VP9:
        return CLIP3(0, VP9_VALID_MAX_LEVEL - 1, (i32)level);

    default:
        ASSERT(0);
    }

    return -1;
}

static i32 getlevelIdxHevc(VCEncLevel level)
{
    switch (level) {
    case VCENC_HEVC_LEVEL_1:
        return 0;
    case VCENC_HEVC_LEVEL_2:
        return 1;
    case VCENC_HEVC_LEVEL_2_1:
        return 2;
    case VCENC_HEVC_LEVEL_3:
        return 3;
    case VCENC_HEVC_LEVEL_3_1:
        return 4;
    case VCENC_HEVC_LEVEL_4:
        return 5;
    case VCENC_HEVC_LEVEL_4_1:
        return 6;
    case VCENC_HEVC_LEVEL_5:
        return 7;
    case VCENC_HEVC_LEVEL_5_1:
        return 8;
    case VCENC_HEVC_LEVEL_5_2:
        return 9;
    case VCENC_HEVC_LEVEL_6:
        return 10;
    case VCENC_HEVC_LEVEL_6_1:
        return 11;
    case VCENC_HEVC_LEVEL_6_2:
        return 12;
    default:
        ASSERT(0);
    }
    return 0;
}
static i32 getlevelIdxH264(VCEncLevel level)
{
    switch (level) {
    case VCENC_H264_LEVEL_1:
        return 0;
    case VCENC_H264_LEVEL_1_b:
        return 1;
    case VCENC_H264_LEVEL_1_1:
        return 2;
    case VCENC_H264_LEVEL_1_2:
        return 3;
    case VCENC_H264_LEVEL_1_3:
        return 4;
    case VCENC_H264_LEVEL_2:
        return 5;
    case VCENC_H264_LEVEL_2_1:
        return 6;
    case VCENC_H264_LEVEL_2_2:
        return 7;
    case VCENC_H264_LEVEL_3:
        return 8;
    case VCENC_H264_LEVEL_3_1:
        return 9;
    case VCENC_H264_LEVEL_3_2:
        return 10;
    case VCENC_H264_LEVEL_4:
        return 11;
    case VCENC_H264_LEVEL_4_1:
        return 12;
    case VCENC_H264_LEVEL_4_2:
        return 13;
    case VCENC_H264_LEVEL_5:
        return 14;
    case VCENC_H264_LEVEL_5_1:
        return 15;
    case VCENC_H264_LEVEL_5_2:
        return 16;
    case VCENC_H264_LEVEL_6:
        return 17;
    case VCENC_H264_LEVEL_6_1:
        return 18;
    case VCENC_H264_LEVEL_6_2:
        return 19;
    default:
        ASSERT(0);
    }
    return 0;
}

static VCEncLevel getLevelHevc(i32 levelIdx)
{
    switch (levelIdx) {
    /*Hevc*/
    case 0:
        return VCENC_HEVC_LEVEL_1;
    case 1:
        return VCENC_HEVC_LEVEL_2;
    case 2:
        return VCENC_HEVC_LEVEL_2_1;
    case 3:
        return VCENC_HEVC_LEVEL_3;
    case 4:
        return VCENC_HEVC_LEVEL_3_1;
    case 5:
        return VCENC_HEVC_LEVEL_4;
    case 6:
        return VCENC_HEVC_LEVEL_4_1;
    case 7:
        return VCENC_HEVC_LEVEL_5;
    case 8:
        return VCENC_HEVC_LEVEL_5_1;
    case 9:
        return VCENC_HEVC_LEVEL_5_2;
    case 10:
        return VCENC_HEVC_LEVEL_6;
    case 11:
        return VCENC_HEVC_LEVEL_6_1;
    default:
        return VCENC_HEVC_LEVEL_6_2;
    }
}

static VCEncLevel getLevelH264(i32 levelIdx)
{
    switch (levelIdx) {
    case 0:
        return VCENC_H264_LEVEL_1;
    case 1:
        return VCENC_H264_LEVEL_1_b;
    case 2:
        return VCENC_H264_LEVEL_1_1;
    case 3:
        return VCENC_H264_LEVEL_1_2;
    case 4:
        return VCENC_H264_LEVEL_1_3;
    case 5:
        return VCENC_H264_LEVEL_2;
    case 6:
        return VCENC_H264_LEVEL_2_1;
    case 7:
        return VCENC_H264_LEVEL_2_2;
    case 8:
        return VCENC_H264_LEVEL_3;
    case 9:
        return VCENC_H264_LEVEL_3_1;
    case 10:
        return VCENC_H264_LEVEL_3_2;
    case 11:
        return VCENC_H264_LEVEL_4;
    case 12:
        return VCENC_H264_LEVEL_4_1;
    case 13:
        return VCENC_H264_LEVEL_4_2;
    case 14:
        return VCENC_H264_LEVEL_5;
    case 15:
        return VCENC_H264_LEVEL_5_1;
    case 16:
        return VCENC_H264_LEVEL_5_2;
    case 17:
        return VCENC_H264_LEVEL_6;
    case 18:
        return VCENC_H264_LEVEL_6_1;
    default:
        return VCENC_H264_LEVEL_6_2;
    }
}

static VCEncLevel getLevel(VCEncVideoCodecFormat codecFormat, i32 levelIdx)
{
    switch (codecFormat) {
    case VCENC_VIDEO_CODEC_HEVC:
        return getLevelHevc(levelIdx);

    case VCENC_VIDEO_CODEC_H264:
        return getLevelH264(levelIdx);

    case VCENC_VIDEO_CODEC_AV1:
        return (u32)CLIP3(0, AV1_VALID_MAX_LEVEL - 1, levelIdx);

    case VCENC_VIDEO_CODEC_VP9:
        return (u32)CLIP3(0, VP9_VALID_MAX_LEVEL - 1, levelIdx);

    default:
        ASSERT(0);
    }
    return -1;
}
static VCEncLevel calculate_level(const VCEncConfig *pEncCfg)
{
    u32 sample_per_picture = pEncCfg->width * pEncCfg->height;
    u64 sample_per_second = sample_per_picture * pEncCfg->frameRateNum / pEncCfg->frameRateDenom;
    i32 i = 0, j = 0, leveIdx = 0;
    i32 maxLevel = 0;
    switch (pEncCfg->codecFormat) {
    case VCENC_VIDEO_CODEC_HEVC:
        maxLevel = HEVC_LEVEL_NUM - 1;
        break;
    case VCENC_VIDEO_CODEC_H264:
        maxLevel = H264_LEVEL_NUM - 1;
        break;

    case VCENC_VIDEO_CODEC_AV1:
        maxLevel = AV1_VALID_MAX_LEVEL - 1;
        break;

    case VCENC_VIDEO_CODEC_VP9:
        maxLevel = VP9_VALID_MAX_LEVEL - 1;
        break;

    default:
        break;
    }

    if (sample_per_picture > getMaxPicSize(pEncCfg->codecFormat, maxLevel) ||
        sample_per_second > getMaxSBPS(pEncCfg->codecFormat, maxLevel)) {
        APITRACEERR("calculate_level: WARNING Invalid parameter.\n");
        i = j = maxLevel;
    }
    for (i = 0; i < maxLevel; i++) {
        if (sample_per_picture <= getMaxPicSize(pEncCfg->codecFormat, i))
            break;
    }
    for (j = 0; j < maxLevel; j++) {
        if (sample_per_second <= getMaxSBPS(pEncCfg->codecFormat, j))
            break;
    }

    leveIdx = MAX(i, j);
    return (getLevel(pEncCfg->codecFormat, leveIdx));
}
/*------------------------------------------------------------------------------

    SetParameter

    Set all parameters in instance to valid values depending on user config.

------------------------------------------------------------------------------*/
bool_e SetParameter(struct vcenc_instance *inst, const VCEncConfig *pEncCfg)
{
    i32 width, height, bps;
    int ctb_size;
    true_e bCorrectProfile;
    u32 tileId;

    ASSERT(inst);

    //inst->width = pEncCfg->width;
    //inst->height = pEncCfg->height;

    inst->codecFormat = pEncCfg->codecFormat;
    ctb_size = (IS_H264(inst->codecFormat) ? 16 : 64);
    /* Internal images, next macroblock boundary */
    width = ctb_size * ((inst->width + ctb_size - 1) / ctb_size);
    height = ctb_size * ((inst->height + ctb_size - 1) / ctb_size);

    /* stream type */
    if (pEncCfg->streamType == VCENC_BYTE_STREAM) {
        inst->asic.regs.streamMode = ASIC_VCENC_BYTE_STREAM;
        //        inst->rateControl.sei.byteStream = ENCHW_YES;
    } else {
        inst->asic.regs.streamMode = ASIC_VCENC_NAL_UNIT;
        //        inst->rateControl.sei.byteStream = ENCHW_NO;
    }
    if ((IS_AV1(pEncCfg->codecFormat) || IS_VP9(pEncCfg->codecFormat)) &&
        pEncCfg->streamType != VCENC_BYTE_STREAM) {
        APITRACEERR("WARNING: AV1 and VP9 only supports byte stream mode\n");
        inst->asic.regs.streamMode = ASIC_VCENC_BYTE_STREAM;
        //        inst->rateControl.sei.byteStream = ENCHW_YES;
    }

    /* ctb */
    inst->ctbPerRow = width / ctb_size;
    inst->ctbPerCol = height / ctb_size;
    inst->ctbPerFrame = inst->ctbPerRow * inst->ctbPerCol;

    /* Disable intra and ROI areas by default */
    inst->asic.regs.intraAreaTop = inst->asic.regs.intraAreaBottom = INVALID_POS;
    inst->asic.regs.intraAreaLeft = inst->asic.regs.intraAreaRight = INVALID_POS;

    inst->asic.regs.roi1Top = inst->asic.regs.roi1Bottom = INVALID_POS;
    inst->asic.regs.roi1Left = inst->asic.regs.roi1Right = INVALID_POS;
    inst->asic.regs.roi2Top = inst->asic.regs.roi2Bottom = INVALID_POS;
    inst->asic.regs.roi2Left = inst->asic.regs.roi2Right = INVALID_POS;
    inst->asic.regs.roi3Top = inst->asic.regs.roi3Bottom = INVALID_POS;
    inst->asic.regs.roi3Left = inst->asic.regs.roi3Right = INVALID_POS;
    inst->asic.regs.roi4Top = inst->asic.regs.roi4Bottom = INVALID_POS;
    inst->asic.regs.roi4Left = inst->asic.regs.roi4Right = INVALID_POS;
    inst->asic.regs.roi5Top = inst->asic.regs.roi5Bottom = INVALID_POS;
    inst->asic.regs.roi5Left = inst->asic.regs.roi5Right = INVALID_POS;
    inst->asic.regs.roi6Top = inst->asic.regs.roi6Bottom = INVALID_POS;
    inst->asic.regs.roi6Left = inst->asic.regs.roi6Right = INVALID_POS;
    inst->asic.regs.roi7Top = inst->asic.regs.roi7Bottom = INVALID_POS;
    inst->asic.regs.roi7Left = inst->asic.regs.roi7Right = INVALID_POS;
    inst->asic.regs.roi8Top = inst->asic.regs.roi8Bottom = INVALID_POS;
    inst->asic.regs.roi8Left = inst->asic.regs.roi8Right = INVALID_POS;

    /* Disable IPCM areas by default */
    inst->asic.regs.ipcm1AreaTop = inst->asic.regs.ipcm1AreaBottom = INVALID_POS;
    inst->asic.regs.ipcm1AreaLeft = inst->asic.regs.ipcm1AreaRight = INVALID_POS;
    inst->asic.regs.ipcm2AreaTop = inst->asic.regs.ipcm2AreaBottom = INVALID_POS;
    inst->asic.regs.ipcm2AreaLeft = inst->asic.regs.ipcm2AreaRight = INVALID_POS;
    inst->asic.regs.ipcm3AreaTop = inst->asic.regs.ipcm3AreaBottom = INVALID_POS;
    inst->asic.regs.ipcm3AreaLeft = inst->asic.regs.ipcm3AreaRight = INVALID_POS;
    inst->asic.regs.ipcm4AreaTop = inst->asic.regs.ipcm4AreaBottom = INVALID_POS;
    inst->asic.regs.ipcm4AreaLeft = inst->asic.regs.ipcm4AreaRight = INVALID_POS;
    inst->asic.regs.ipcm5AreaTop = inst->asic.regs.ipcm5AreaBottom = INVALID_POS;
    inst->asic.regs.ipcm5AreaLeft = inst->asic.regs.ipcm5AreaRight = INVALID_POS;
    inst->asic.regs.ipcm6AreaTop = inst->asic.regs.ipcm6AreaBottom = INVALID_POS;
    inst->asic.regs.ipcm6AreaLeft = inst->asic.regs.ipcm6AreaRight = INVALID_POS;
    inst->asic.regs.ipcm7AreaTop = inst->asic.regs.ipcm7AreaBottom = INVALID_POS;
    inst->asic.regs.ipcm7AreaLeft = inst->asic.regs.ipcm7AreaRight = INVALID_POS;
    inst->asic.regs.ipcm8AreaTop = inst->asic.regs.ipcm8AreaBottom = INVALID_POS;
    inst->asic.regs.ipcm8AreaLeft = inst->asic.regs.ipcm8AreaRight = INVALID_POS;

    /* Slice num */
    inst->asic.regs.sliceSize = 0;
    inst->asic.regs.sliceNum = 1;

    /* compressor */
    inst->asic.regs.recon_luma_compress = (pEncCfg->compressor & 1) ? 1 : 0;
    inst->asic.regs.recon_chroma_compress =
        ((pEncCfg->compressor & 2) &&
         (!((VCENC_CHROMA_IDC_400 == pEncCfg->codedChromaIdc) &&
            (inst->asic.regs.asicCfg.MonoChromeSupport == EWL_HW_CONFIG_ENABLED))))
            ? 1
            : 0;

    /* CU Info output*/
    inst->asic.regs.enableOutputCuInfo = pEncCfg->enableOutputCuInfo;
    /* Check cuinfo version */
    if (((i32)inst->asic.regs.asicCfg.cuInforVersion < pEncCfg->cuInfoVersion) ||
        (pEncCfg->cuInfoVersion == 0 && inst->asic.regs.asicCfg.cuInforVersion != 0)) {
        APITRACEERR("CuInfoVersion %d is not supported\n", pEncCfg->cuInfoVersion);
        return ENCHW_NOK;
    }
    if (inst->asic.regs.asicCfg.cuInforVersion >= 2) {
        /* HW IM only support cuinfo version 2; SW cutree only support cuinfo version 1 */
        if (pEncCfg->pass == 1)
            inst->asic.regs.cuInfoVersion = inst->asic.regs.asicCfg.bMultiPassSupport
                                                ? inst->asic.regs.asicCfg.cuInforVersion
                                                : 1;
        else
            inst->asic.regs.cuInfoVersion =
                pEncCfg->cuInfoVersion != -1 ? pEncCfg->cuInfoVersion : 1;
    } else
        inst->asic.regs.cuInfoVersion = inst->asic.regs.asicCfg.cuInforVersion;

    /* Ctb encoded bits output */
    inst->asic.regs.enableOutputCtbBits = pEncCfg->enableOutputCtbBits && pEncCfg->pass != 1;

    /* Sequence parameter set */
    inst->bAutoLevel = false;
    if (pEncCfg->level == VCENC_AUTO_LEVEL) {
        inst->level = calculate_level(pEncCfg);
        inst->bAutoLevel = true;
        if (inst->level == -1) {
            return ENCHW_NOK;
        }
    } else {
        inst->level = pEncCfg->level;
    }

    inst->levelIdx = getLevelIdx(inst->codecFormat, inst->level);
    inst->tier = pEncCfg->tier;

    /* RDO effort level */
    inst->rdoLevel = inst->asic.regs.rdoLevel = pEncCfg->rdoLevel;
    if (IS_H264(inst->codecFormat) && pEncCfg->rdoLevel > 0) {
        APITRACEERR("WARNING: rdoLevel 1/2 NOT support in H264, set rdoLevel to 0\n");
        inst->rdoLevel = inst->asic.regs.rdoLevel = 0;
    }

    inst->asic.regs.codedChromaIdc =
        (inst->asic.regs.asicCfg.MonoChromeSupport == EWL_HW_CONFIG_NOT_SUPPORTED)
            ? VCENC_CHROMA_IDC_420
            : pEncCfg->codedChromaIdc;
    bCorrectProfile = (VCENC_CHROMA_IDC_400 == pEncCfg->codedChromaIdc) &&
                      (inst->asic.regs.asicCfg.MonoChromeSupport == EWL_HW_CONFIG_ENABLED);
    switch (pEncCfg->profile) {
    case 0:
        //main profile
        inst->profile = bCorrectProfile ? VCENC_PROFILE_HEVC_REXT : VCENC_PROFILE_HEVC_MAIN;
        break;
    case 1:
        //main still picture profile.
        inst->profile = VCENC_PROFILE_HEVC_MAIN_STILL_PICTURE;
        break;
    case 2:
        //main 10 profile.
        inst->profile = bCorrectProfile ? VCENC_PROFILE_HEVC_REXT : VCENC_PROFILE_HEVC_MAIN_10;
        break;
    case 3:
        //main ext profile.
        inst->profile = bCorrectProfile ? VCENC_PROFILE_HEVC_REXT : VCENC_PROFILE_HEVC_MAIN_10;
        break;
    case 9:
        //H.264 baseline profile
        inst->profile = bCorrectProfile ? VCENC_PROFILE_H264_HIGH : VCENC_PROFILE_H264_BASELINE;
        break;
    case 10:
        //H.264 main profile
        inst->profile = bCorrectProfile ? VCENC_PROFILE_H264_HIGH : VCENC_PROFILE_H264_MAIN;
        break;
    case 11:
        //H.264 high profile
        inst->profile = VCENC_PROFILE_H264_HIGH;
        break;
    case 12:
        //H.264 high 10 profile
        inst->profile = VCENC_PROFILE_H264_HIGH_10;
        break;
    default:
        break;
    }
    /* enforce maximum frame size in level */
    if ((u32)(inst->width * inst->height) > getMaxPicSize(inst->codecFormat, inst->levelIdx)) {
        APITRACEERR("WARNING: MaxFS violates level limit\n");
    }

    /* enforce Max luma sample rate in level */
    {
        u32 sample_rate =
            (pEncCfg->frameRateNum * inst->width * inst->height) / pEncCfg->frameRateDenom;

        if (sample_rate > getMaxSBPS(inst->codecFormat, inst->levelIdx)) {
            APITRACEERR("WARNING: MaxSBPS violates level limit\n");
        }
    }
    /* intra */
    inst->constrained_intra_pred_flag = 0;
    inst->strong_intra_smoothing_enabled_flag = pEncCfg->strongIntraSmoothing;

    inst->vps->max_dec_pic_buffering[0] = pEncCfg->refFrameAmount + 1;
    inst->sps->max_dec_pic_buffering[0] = pEncCfg->refFrameAmount + 1;

    inst->sps->bit_depth_luma_minus8 = pEncCfg->bitDepthLuma - 8;
    inst->sps->bit_depth_chroma_minus8 = pEncCfg->bitDepthChroma - 8;

    inst->vps->max_num_sub_layers = pEncCfg->maxTLayers;
    inst->vps->temporal_id_nesting_flag = 1;
    for (int i = 0; i < inst->vps->max_num_sub_layers; i++) {
        inst->vps->max_dec_pic_buffering[i] = inst->vps->max_dec_pic_buffering[0];
    }

    inst->sps->max_num_sub_layers = pEncCfg->maxTLayers;
    inst->sps->temporal_id_nesting_flag = 1;
    for (int i = 0; i < inst->sps->max_num_sub_layers; i++) {
        inst->sps->max_dec_pic_buffering[i] = inst->sps->max_dec_pic_buffering[0];
    }

    inst->sps->picOrderCntType = pEncCfg->picOrderCntType;
    inst->sps->log2_max_pic_order_cnt_lsb = pEncCfg->log2MaxPicOrderCntLsb;
    inst->sps->log2MaxFrameNumMinus4 = (pEncCfg->log2MaxFrameNum - 4);
    inst->sps->log2MaxpicOrderCntLsbMinus4 = (pEncCfg->log2MaxPicOrderCntLsb - 4);
    if (HW_PRODUCT_SYSTEM60(inst->asic.regs.asicHwId) ||
        HW_PRODUCT_VC9000LE(inst->asic.regs.asicHwId)) /* VC8000E6.0 */
    {
        int log2MaxPicOrderCntLsb = (IS_H264(pEncCfg->codecFormat) ? 16 : 8);
        inst->sps->log2_max_pic_order_cnt_lsb = log2MaxPicOrderCntLsb;
        inst->sps->log2MaxFrameNumMinus4 = 12;
        inst->sps->log2MaxpicOrderCntLsbMinus4 = (log2MaxPicOrderCntLsb - 4);
    }

    inst->enableTMVP = pEncCfg->enableTMVP;

    /* Rate control setup */

    /* Maximum bitrate for the specified level */
    bps = getMaxBR(inst->codecFormat, inst->levelIdx, inst->profile, inst->tier);

    {
        vcencRateControl_s *rc = &inst->rateControl;

        rc->outRateDenom = pEncCfg->frameRateDenom;
        rc->outRateNum = pEncCfg->frameRateNum;
        rc->picArea = ((inst->width + 7) & (~7)) * ((inst->height + 7) & (~7));
        rc->ctbPerPic = inst->ctbPerFrame;
        rc->reciprocalOfNumBlk8 = 1.0 / (rc->ctbPerPic * (ctb_size / 8) * (ctb_size / 8));
        rc->ctbRows = inst->ctbPerCol;
        rc->ctbCols = inst->ctbPerRow;
        rc->ctbSize = ctb_size;

        {
            rcVirtualBuffer_s *vb = &rc->virtualBuffer;

            vb->bitRate = bps;
            vb->unitsInTic = pEncCfg->frameRateDenom;
            vb->timeScale = pEncCfg->frameRateNum * (inst->interlaced + 1);
            vb->bufferSize =
                getMaxCPBS(inst->codecFormat, inst->levelIdx, inst->profile, inst->tier);
        }

        rc->hrd = ENCHW_NO;
        rc->picRc = ENCHW_NO;
        rc->ctbRc = 0;
        rc->picSkip = ENCHW_NO;
        rc->vbr = ENCHW_NO;

        rc->qpHdr = VCENC_DEFAULT_QP << QP_FRACTIONAL_BITS;
        rc->qpMin = 0 << QP_FRACTIONAL_BITS;
        rc->qpMax = 51 << QP_FRACTIONAL_BITS;

        rc->frameCoded = ENCHW_YES;
        rc->sliceTypeCur = I_SLICE;
        rc->sliceTypePrev = P_SLICE;
        rc->bitrateWindow = 150;

        /* Default initial value for intra QP delta */
        rc->intraQpDelta = INTRA_QPDELTA_DEFAULT << QP_FRACTIONAL_BITS;
        rc->fixedIntraQp = 0 << QP_FRACTIONAL_BITS;
        rc->frameQpDelta = 0 << QP_FRACTIONAL_BITS;
        rc->gopPoc = 0;
    }

    /* no SEI by default */
    inst->rateControl.sei.enabled = ENCHW_NO;

    /* Pre processing */
    inst->preProcess.lumWidth = pEncCfg->width;
    inst->preProcess.lumHeight = pEncCfg->height / ((inst->interlaced + 1));
    for (tileId = 0; tileId < inst->num_tile_columns; tileId++) {
        inst->preProcess.lumWidthSrc[tileId] =
            VCEncGetAllowedWidth(pEncCfg->width, VCENC_YUV420_PLANAR);
        inst->preProcess.lumHeightSrc[tileId] = pEncCfg->height;
        inst->preProcess.horOffsetSrc[tileId] = 0;
        inst->preProcess.verOffsetSrc[tileId] = 0;
    }

    inst->preProcess.rotation = ROTATE_0;
    inst->preProcess.mirror = VCENC_MIRROR_NO;
    inst->preProcess.inputFormat = VCENC_YUV420_PLANAR;
    inst->preProcess.videoStab = 0;

    inst->preProcess.scaledWidth = 0;
    inst->preProcess.scaledHeight = 0;
    inst->preProcess.scaledOutput = 0;
    inst->preProcess.interlacedFrame = inst->interlaced;
    inst->asic.scaledImage.busAddress = 0;
    inst->asic.scaledImage.virtualAddress = NULL;
    inst->asic.scaledImage.size = 0;
    inst->preProcess.adaptiveRoi = 0;
    inst->preProcess.adaptiveRoiColor = 0;
    inst->preProcess.adaptiveRoiMotion = -5;

    inst->preProcess.input_alignment = inst->input_alignment;
    inst->preProcess.colorConversionType = 0;

    inst->preProcess.inLoopDSRatio = (pEncCfg->pass == 1) ? pEncCfg->inLoopDSRatio : 0;

    EncSetColorConversion(&inst->preProcess, &inst->asic);

    return ENCHW_OK;
}

/*------------------------------------------------------------------------------

    Round the width to the next multiple of 8 or 16 depending on YUV type.

------------------------------------------------------------------------------*/
i32 VCEncGetAllowedWidth(i32 width, VCEncPictureType inputType)
{
    if (inputType == VCENC_YUV420_PLANAR) {
        /* Width must be multiple of 16 to make
     * chrominance row 64-bit aligned */
        return ((width + 15) / 16) * 16;
    } else {
        /* VCENC_YUV420_SEMIPLANAR */
        /* VCENC_YUV422_INTERLEAVED_YUYV */
        /* VCENC_YUV422_INTERLEAVED_UYVY */
        return ((width + 7) / 8) * 8;
    }
}

/** get max width the hw supported for specified format. */
u32 VCEncGetMaxWidth(const EWLHwConfig_t *asic_cfg, VCEncVideoCodecFormat codec_format)
{
    if (IS_HEVC(codec_format))
        return asic_cfg->maxEncodedWidthHEVC;
    if (IS_H264(codec_format))
        return asic_cfg->maxEncodedWidthH264;
    if (IS_AV1(codec_format))
        return asic_cfg->maxEncodedWidthHEVC; /* FIXME: use HEVC now */
    if (IS_VP9(codec_format))
        return asic_cfg->maxEncodedWidthHEVC; /* FIXME: use HEVC now */

    return 0;
}

void IntraLamdaQp(int qp, u32 *lamda_sad, enum slice_type type, int poc, float dQPFactor,
                  bool depth0)
{
    float recalQP = (float)qp;
    float lambda;

    // pre-compute lambda and QP values for all possible QP candidates
    {
        // compute lambda value
        //Int    NumberBFrames = 0;
        //double dLambda_scale = 1.0 - MAX( 0.0, MIN( 0.5, 0.05*(double)NumberBFrames ));
        //double qp_temp = (double) recalQP + bitdepth_luma_qp_scale - SHIFT_QP;
        // Case #1: I or P-slices (key-frame)
        //double dQPFactor = type == B_SLICE ? 0.68 : 0.8;//0.442;//0.576;//0.8;
        //lambda = dQPFactor * pow(2.0, qp_temp / 3.0);
        //if (type == B_SLICE)
        //lambda *= CLIP3(2.0, 4.0, (qp - 12) / 6.0); //* 0.8/0.576;

        //clip
        //    if (((unsigned int)lambda) > ((1<<14)-1))
        //      lambda = (double)((1<<14)-1);

        Int SHIFT_QP = 12;
        Int g_bitDepth = 8;
        Int bitdepth_luma_qp_scale = 6 * (g_bitDepth - 8);
        Int qp_temp = qp + bitdepth_luma_qp_scale;

        lambda = (float)dQPFactor * sqrtPow2QpDiv3[qp_temp];
        if (!depth0)
            lambda *= sqrtQpDiv6[qp];
    }

    // store lambda
    //lambda = sqrt(lambda);

    ASSERT(lambda < 255);

    *lamda_sad = (u32)(lambda * (1 << IMS_LAMBDA_SAD_SHIFT) + 0.5f);
}

void InterLamdaQp(int qp, unsigned int *lamda_sse, unsigned int *lamda_sad, enum slice_type type,
                  float dQPFactor, bool depth0, u32 asicId)
{
    float recalQP = (float)qp;
    float lambda;

    // pre-compute lambda and QP values for all possible QP candidates
    {
        // compute lambda value
        //Int    NumberBFrames = 0;
        //double dLambda_scale = 1.0 - MAX( 0.0, MIN( 0.5, 0.05*(double)NumberBFrames ));
        //double qp_temp = (double) recalQP + bitdepth_luma_qp_scale - SHIFT_QP;
        // Case #1: I or P-slices (key-frame)
        //double dQPFactor = type == B_SLICE ? 0.68 : 0.8;//0.442;//0.576;//0.8;
        //lambda = dQPFactor * pow(2.0, qp_temp / 3.0);
        //if (type == B_SLICE)
        //lambda *= CLIP3(2.0, 4.0, (qp - 12) / 6.0); //* 0.8/0.576;

        Int SHIFT_QP = 12;
        Int g_bitDepth = 8;
        Int bitdepth_luma_qp_scale = 6 * (g_bitDepth - 8);
        Int qp_temp = qp + bitdepth_luma_qp_scale;

        lambda = (float)dQPFactor * sqrtPow2QpDiv3[qp_temp];
        if (!depth0)
            lambda *= sqrtQpDiv6[qp];
    }

    u32 lambdaSSEShift = 0;
    u32 lambdaSADShift = 0;
    if ((HW_ID_MAJOR_NUMBER(asicId)) >= 5 || !HW_PRODUCT_H2(asicId)) /* H2V5 rev01 */
    {
        lambdaSSEShift = MOTION_LAMBDA_SSE_SHIFT;
        lambdaSADShift = MOTION_LAMBDA_SAD_SHIFT;
    }

    //keep accurate for small QP
    u32 lambdaSSE = (u32)(lambda * lambda * (1 << lambdaSSEShift) + 0.5f);
    u32 lambdaSAD = (u32)(lambda * (1 << lambdaSADShift) + 0.5f);

    //clip
    u32 maxLambdaSSE = (1 << (15 + lambdaSSEShift)) - 1;
    u32 maxLambdaSAD = (1 << (8 + lambdaSADShift)) - 1;
    lambdaSSE = MIN(lambdaSSE, maxLambdaSSE);
    lambdaSAD = MIN(lambdaSAD, maxLambdaSAD);

    // store lambda
    *lamda_sse = lambdaSSE;
    *lamda_sad = lambdaSAD;
}

void IntraLamdaQpFixPoint(int qp, u32 *lamda_sad, enum slice_type type, int poc, u32 qpFactorSad,
                          bool depth0)
{
    i32 shiftSad = LAMBDA_FIX_POINT + QPFACTOR_FIX_POINT - IMS_LAMBDA_SAD_SHIFT;
    u32 roundSad = 1 << (shiftSad - 1);
    u32 maxLambdaSAD = (1 << (8 + IMS_LAMBDA_SAD_SHIFT)) - 1;
    const u32 *lambdaSadTbl = (!depth0) ? lambdaSadDepth1Tbl : lambdaSadDepth0Tbl;

    u64 lambdaSad = ((u64)qpFactorSad * lambdaSadTbl[qp] + roundSad) >> shiftSad;
    *lamda_sad = MIN(lambdaSad, maxLambdaSAD);
}

void InterLamdaQpFixPoint(int qp, unsigned int *lamda_sse, unsigned int *lamda_sad,
                          enum slice_type type, u32 qpFactorSad, u32 qpFactorSse, bool depth0,
                          u32 asicId)
{
    i32 shiftSad = LAMBDA_FIX_POINT + QPFACTOR_FIX_POINT - MOTION_LAMBDA_SAD_SHIFT;
    i32 shiftSse = LAMBDA_FIX_POINT + QPFACTOR_FIX_POINT - MOTION_LAMBDA_SSE_SHIFT;
    u32 roundSad = 1 << (shiftSad - 1);
    u32 roundSse = 1 << (shiftSse - 1);
    u32 maxLambdaSAD = (1 << (8 + MOTION_LAMBDA_SAD_SHIFT)) - 1;
    u32 maxLambdaSSE = (1 << (15 + MOTION_LAMBDA_SSE_SHIFT)) - 1;
    const u32 *lambdaSadTbl = (!depth0) ? lambdaSadDepth1Tbl : lambdaSadDepth0Tbl;
    const u32 *lambdaSseTbl = (!depth0) ? lambdaSseDepth1Tbl : lambdaSseDepth0Tbl;

    u64 lambdaSad = ((u64)qpFactorSad * lambdaSadTbl[qp] + roundSad) >> shiftSad;
    u64 lambdaSse = ((u64)qpFactorSse * lambdaSseTbl[qp] + roundSse) >> shiftSse;
    lambdaSse = MIN(lambdaSse, maxLambdaSSE);
    lambdaSad = MIN(lambdaSad, maxLambdaSAD);
    if ((HW_ID_MAJOR_NUMBER(asicId)) < 5 && HW_PRODUCT_H2(asicId)) /* H2V5 rev01 */
    {
        lambdaSse >>= MOTION_LAMBDA_SSE_SHIFT;
        lambdaSad >>= MOTION_LAMBDA_SAD_SHIFT;
    }

    // store lambda
    *lamda_sse = lambdaSse;
    *lamda_sad = lambdaSad;
}

//Set LamdaTable for all qp
void LamdaTableQp(regValues_s *regs, int qp, enum slice_type type, int poc, double dQPFactor,
                  bool depth0, u32 ctbRc)
{
    int qpoffset, lamdaIdx;
#ifdef FIX_POINT_LAMBDA
    u32 qpFactorSad = (u32)((dQPFactor * (1 << QPFACTOR_FIX_POINT)) + 0.5f);
    u32 qpFactorSse = (u32)((dQPFactor * dQPFactor * (1 << QPFACTOR_FIX_POINT)) + 0.5f);
#endif
#ifndef CTBRC_STRENGTH

    //setup LamdaTable dependent on ctbRc enable
    if (ctbRc) {
        for (qpoffset = -2, lamdaIdx = 14; qpoffset <= +2; qpoffset++, lamdaIdx++) {
#ifdef FIX_POINT_LAMBDA
            IntraLamdaQpFixPoint(qp - qpoffset, regs->lambda_satd_ims + (lamdaIdx & 0xf), type, poc,
                                 qpFactorSad, depth0);
            InterLamdaQpFixPoint(qp - qpoffset, regs->lambda_sse_me + (lamdaIdx & 0xf),
                                 regs->lambda_satd_me + (lamdaIdx & 0xf), type, qpFactorSad,
                                 qpFactorSse, depth0, regs->asicHwId);
#else
            IntraLamdaQp(qp - qpoffset, regs->lambda_satd_ims + (lamdaIdx & 0xf), type, poc,
                         dQPFactor, depth0);
            InterLamdaQp(qp - qpoffset, regs->lambda_sse_me + (lamdaIdx & 0xf),
                         regs->lambda_satd_me + (lamdaIdx & 0xf), type, dQPFactor, depth0,
                         regs->asicHwId);
#endif
        }

    } else {
        //for ROI-window, ROI-Map
        for (qpoffset = 0, lamdaIdx = 0; qpoffset <= 15; qpoffset++, lamdaIdx++) {
#ifdef FIX_POINT_LAMBDA
            IntraLamdaQpFixPoint(qp - qpoffset, regs->lambda_satd_ims + lamdaIdx, type, poc,
                                 qpFactorSad, depth0);
            InterLamdaQpFixPoint(qp - qpoffset, regs->lambda_sse_me + lamdaIdx,
                                 regs->lambda_satd_me + lamdaIdx, type, qpFactorSad, qpFactorSse,
                                 depth0, regs->asicHwId);
#else
            IntraLamdaQp(qp - qpoffset, regs->lambda_satd_ims + lamdaIdx, type, poc, dQPFactor,
                         depth0);
            InterLamdaQp(qp - qpoffset, regs->lambda_sse_me + lamdaIdx,
                         regs->lambda_satd_me + lamdaIdx, type, dQPFactor, depth0, regs->asicHwId);
#endif
        }
    }
#else

    regs->offsetSliceQp = 0;
    if (qp >= 35) {
        regs->offsetSliceQp = 35 - qp;
    }
    if (qp <= 15) {
        regs->offsetSliceQp = 15 - qp;
    }

    for (qpoffset = -16, lamdaIdx = 16 /* - regs->offsetSliceQp*/; qpoffset <= +15;
         qpoffset++, lamdaIdx++) {
#ifdef FIX_POINT_LAMBDA
        IntraLamdaQpFixPoint(CLIP3(0, 51, qp - qpoffset + regs->offsetSliceQp),
                             regs->lambda_satd_ims + (lamdaIdx & 0x1f), type, poc, qpFactorSad,
                             depth0);
        InterLamdaQpFixPoint(CLIP3(0, 51, qp - qpoffset + regs->offsetSliceQp),
                             regs->lambda_sse_me + (lamdaIdx & 0x1f),
                             regs->lambda_satd_me + (lamdaIdx & 0x1f), type, qpFactorSad,
                             qpFactorSse, depth0, regs->asicHwId);
#else
        IntraLamdaQp(CLIP3(0, 51, qp - qpoffset + regs->offsetSliceQp),
                     regs->lambda_satd_ims + (lamdaIdx & 0x1f), type, poc, dQPFactor, depth0);
        InterLamdaQp(CLIP3(0, 51, qp - qpoffset + regs->offsetSliceQp),
                     regs->lambda_sse_me + (lamdaIdx & 0x1f),
                     regs->lambda_satd_me + (lamdaIdx & 0x1f), type, dQPFactor, depth0,
                     regs->asicHwId);
#endif
    }

    if (regs->asicCfg.roiAbsQpSupport) {
        regs->qpFactorSad = (u32)((dQPFactor * (1 << QPFACTOR_FIX_POINT)) + 0.5);
        regs->qpFactorSse = (u32)((dQPFactor * dQPFactor * (1 << QPFACTOR_FIX_POINT)) + 0.5);
        regs->lambdaDepth = !depth0;
    }
#endif
}

void FillIntraFactor(regValues_s *regs)
{
#if 0
  const double weight = 0.57 / 0.8;
  const double size_coeff[4] =
  {
    30.0,
    30.0,
    42.0,
    42.0
  };
  const u32 mode_coeff[3] =
  {
    0x6276 + 1 * 0x8000,
    0x6276 + 2 * 0x8000,
    0x6276 + 5 * 0x8000,
  };

  regs->intra_size_factor[0] = double_to_u6q4(sqrt(weight) * (1 / 0.8) * size_coeff[0]);
  regs->intra_size_factor[1] = double_to_u6q4(sqrt(weight) * (1 / 0.8) * size_coeff[1]);
  regs->intra_size_factor[2] = double_to_u6q4(sqrt(weight) * (1 / 0.8) * size_coeff[2]);
  regs->intra_size_factor[3] = double_to_u6q4(sqrt(weight) * (1 / 0.8) * size_coeff[3]);

  regs->intra_mode_factor[0] = double_to_u6q4(sqrt(weight) * (double) mode_coeff[0] / 32768.0);
  regs->intra_mode_factor[1] = double_to_u6q4(sqrt(weight) * (double) mode_coeff[1] / 32768.0);
  regs->intra_mode_factor[2] = double_to_u6q4(sqrt(weight) * (double) mode_coeff[2] / 32768.0);
#else
    regs->intra_size_factor[0] = 506;
    regs->intra_size_factor[1] = 506;
    regs->intra_size_factor[2] = 709;
    regs->intra_size_factor[3] = 709;

    if (regs->codingType == ASIC_H264) {
        regs->intra_mode_factor[0] = 24;
        regs->intra_mode_factor[1] = 12;
        regs->intra_mode_factor[2] = 48;
    } else {
        regs->intra_mode_factor[0] = 24;
        regs->intra_mode_factor[1] = 37;
        regs->intra_mode_factor[2] = 78;
    }
#endif

    return;
}

/*------------------------------------------------------------------------------

    Set QP related parameters at the beginning of a new frame.

------------------------------------------------------------------------------*/
static void VCEncSetQuantParameters(struct vcenc_instance *vcenc_instance, struct sw_picture *pic,
                                    const VCEncIn *pEncIn, double qp_factor, bool is_depth0)
{
    if (!vcenc_instance)
        return;

    asicData_s *asic = &vcenc_instance->asic;
    u32 enable_ctu_rc = vcenc_instance->rateControl.ctbRc;
    enum slice_type type = pic->sliceInst->type;

    /* Intra Penalty for TU 4x4 vs. 8x8 */
    static unsigned short VCEncIntraPenaltyTu4[52] = {
        7,   7,   8,   10,  11,   12,   14,   15,   17,   20,   22,   25,   28,
        31,  35,  40,  44,  50,   56,   63,   71,   80,   89,   100,  113,  127,
        142, 160, 179, 201, 226,  254,  285,  320,  359,  403,  452,  508,  570,
        640, 719, 807, 905, 1016, 1141, 1281, 1438, 1614, 1811, 2033, 2282, 2562}; //max*3=13bit

    /* Intra Penalty for TU 8x8 vs. 16x16 */
    static unsigned short VCEncIntraPenaltyTu8[52] = {
        7,   7,   8,   10,  11,   12,   14,   15,   17,   20,   22,   25,   28,
        31,  35,  40,  44,  50,   56,   63,   71,   80,   89,   100,  113,  127,
        142, 160, 179, 201, 226,  254,  285,  320,  359,  403,  452,  508,  570,
        640, 719, 807, 905, 1016, 1141, 1281, 1438, 1614, 1811, 2033, 2282, 2562}; //max*3=13bit

    /* Intra Penalty for TU 16x16 vs. 32x32 */
    static unsigned short VCEncIntraPenaltyTu16[52] = {
        9,   11,   12,   14,   15,   17,   19,   22,   24,   28,   31,   35,   39,
        44,  49,   56,   62,   70,   79,   88,   99,   112,  125,  141,  158,  177,
        199, 224,  251,  282,  317,  355,  399,  448,  503,  564,  634,  711,  799,
        896, 1006, 1129, 1268, 1423, 1598, 1793, 2013, 2259, 2536, 2847, 3196, 3587}; //max*3=14bit

    /* Intra Penalty for TU 32x32 vs. 64x64 */
    static unsigned short VCEncIntraPenaltyTu32[52] = {
        9,   11,   12,   14,   15,   17,   19,   22,   24,   28,   31,   35,   39,
        44,  49,   56,   62,   70,   79,   88,   99,   112,  125,  141,  158,  177,
        199, 224,  251,  282,  317,  355,  399,  448,  503,  564,  634,  711,  799,
        896, 1006, 1129, 1268, 1423, 1598, 1793, 2013, 2259, 2536, 2847, 3196, 3587}; //max*3=14bit

    /* Intra Penalty for Mode a */
    static unsigned short VCEncIntraPenaltyModeA[52] = {
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  1,   1,  2,  2,
        2,  2,  3,  3,  4,  4,  5,  5,  6,  7,  8,  9,  10, 11, 13,  15, 16, 19,
        21, 23, 26, 30, 33, 38, 42, 47, 53, 60, 67, 76, 85, 95, 107, 120}; //max*3=9bit

    /* Intra Penalty for Mode b */
    static unsigned short VCEncIntraPenaltyModeB[52] = {
        0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  1,   1,   2,   2,   2,   2,  3,  3,
        4,  4,  5,  5,  6,  7,  8,  9,  10, 11, 13,  14,  16,  18,  21,  23, 26, 29,
        33, 37, 42, 47, 53, 59, 66, 75, 84, 94, 106, 119, 133, 150, 168, 189}; ////max*3=10bit

    /* Intra Penalty for Mode c */
    static unsigned short VCEncIntraPenaltyModeC[52] = {
        1,  1,  1,  1,  1,   1,   2,   2,   2,   3,   3,   3,   4,   4,   5,   6,  6,  7,
        8,  9,  10, 12, 13,  15,  17,  19,  21,  24,  27,  31,  34,  39,  43,  49, 55, 62,
        69, 78, 87, 98, 110, 124, 139, 156, 175, 197, 221, 248, 278, 312, 351, 394}; //max*3=11bit

    switch (HW_ID_MAJOR_NUMBER(asic->regs.asicHwId)) {
    case 1: /* H2V1 rev01 */
        asic->regs.intraPenaltyPic4x4 = VCEncIntraPenaltyTu4[asic->regs.qp];
        asic->regs.intraPenaltyPic8x8 = VCEncIntraPenaltyTu8[asic->regs.qp];
        asic->regs.intraPenaltyPic16x16 = VCEncIntraPenaltyTu16[asic->regs.qp];
        asic->regs.intraPenaltyPic32x32 = VCEncIntraPenaltyTu32[asic->regs.qp];

        asic->regs.intraMPMPenaltyPic1 = VCEncIntraPenaltyModeA[asic->regs.qp];
        asic->regs.intraMPMPenaltyPic2 = VCEncIntraPenaltyModeB[asic->regs.qp];
        asic->regs.intraMPMPenaltyPic3 = VCEncIntraPenaltyModeC[asic->regs.qp];

        u32 roi1_qp = CLIP3(0, 51, ((int)asic->regs.qp - (int)asic->regs.roi1DeltaQp));
        asic->regs.intraPenaltyRoi14x4 = VCEncIntraPenaltyTu4[roi1_qp];
        asic->regs.intraPenaltyRoi18x8 = VCEncIntraPenaltyTu8[roi1_qp];
        asic->regs.intraPenaltyRoi116x16 = VCEncIntraPenaltyTu16[roi1_qp];
        asic->regs.intraPenaltyRoi132x32 = VCEncIntraPenaltyTu32[roi1_qp];

        asic->regs.intraMPMPenaltyRoi11 = VCEncIntraPenaltyModeA[roi1_qp];
        asic->regs.intraMPMPenaltyRoi12 = VCEncIntraPenaltyModeB[roi1_qp];
        asic->regs.intraMPMPenaltyRoi13 = VCEncIntraPenaltyModeC[roi1_qp];

        u32 roi2_qp = CLIP3(0, 51, ((int)asic->regs.qp - (int)asic->regs.roi2DeltaQp));
        asic->regs.intraPenaltyRoi24x4 = VCEncIntraPenaltyTu4[roi2_qp];
        asic->regs.intraPenaltyRoi28x8 = VCEncIntraPenaltyTu8[roi2_qp];
        asic->regs.intraPenaltyRoi216x16 = VCEncIntraPenaltyTu16[roi2_qp];
        asic->regs.intraPenaltyRoi232x32 = VCEncIntraPenaltyTu32[roi2_qp];

        asic->regs.intraMPMPenaltyRoi21 = VCEncIntraPenaltyModeA[roi2_qp]; //need to change
        asic->regs.intraMPMPenaltyRoi22 = VCEncIntraPenaltyModeB[roi2_qp]; //need to change
        asic->regs.intraMPMPenaltyRoi23 = VCEncIntraPenaltyModeC[roi2_qp]; //need to change

        /*inter prediction parameters*/
#ifndef FIX_POINT_LAMBDA
        InterLamdaQp(asic->regs.qp, &asic->regs.lamda_motion_sse, &asic->regs.lambda_motionSAD,
                     type, qp_factor, is_depth0, asic->regs.asicHwId);
        InterLamdaQp(asic->regs.qp - asic->regs.roi1DeltaQp, &asic->regs.lamda_motion_sse_roi1,
                     &asic->regs.lambda_motionSAD_ROI1, type, qp_factor, is_depth0,
                     asic->regs.asicHwId);
        InterLamdaQp(asic->regs.qp - asic->regs.roi2DeltaQp, &asic->regs.lamda_motion_sse_roi2,
                     &asic->regs.lambda_motionSAD_ROI2, type, qp_factor, is_depth0,
                     asic->regs.asicHwId);
#else
        u32 qpFactorSad = (u32)((qp_factor * (1 << QPFACTOR_FIX_POINT)) + 0.5f);
        u32 qpFactorSse = (u32)((qp_factor * qp_factor * (1 << QPFACTOR_FIX_POINT)) + 0.5f);
        InterLamdaQpFixPoint(asic->regs.qp, &asic->regs.lamda_motion_sse,
                             &asic->regs.lambda_motionSAD, type, qpFactorSad, qpFactorSse,
                             is_depth0, asic->regs.asicHwId);
        InterLamdaQpFixPoint(asic->regs.qp - asic->regs.roi1DeltaQp,
                             &asic->regs.lamda_motion_sse_roi1, &asic->regs.lambda_motionSAD_ROI1,
                             type, qpFactorSad, qpFactorSse, is_depth0, asic->regs.asicHwId);
        InterLamdaQpFixPoint(asic->regs.qp - asic->regs.roi2DeltaQp,
                             &asic->regs.lamda_motion_sse_roi2, &asic->regs.lambda_motionSAD_ROI2,
                             type, qpFactorSad, qpFactorSse, is_depth0, asic->regs.asicHwId);
#endif

        asic->regs.lamda_SAO_luma = asic->regs.lamda_motion_sse;
        asic->regs.lamda_SAO_chroma = 0.75 * asic->regs.lamda_motion_sse;

        break;
    case 2: /* H2V2 rev01 */
    case 3: /* H2V3 rev01 */
        LamdaTableQp(&asic->regs, asic->regs.qp, type, asic->regs.poc, qp_factor, is_depth0,
                     enable_ctu_rc);
        FillIntraFactor(&asic->regs);
        asic->regs.lamda_SAO_luma = asic->regs.lambda_sse_me[0];
        asic->regs.lamda_SAO_chroma = 0.75 * asic->regs.lambda_sse_me[0];
        break;
    case 4: /* H2V4 rev01 */
        LamdaTableQp(&asic->regs, asic->regs.qp, type, asic->regs.poc, qp_factor, is_depth0,
                     enable_ctu_rc);
        FillIntraFactor(&asic->regs);
        asic->regs.lamda_SAO_luma = asic->regs.lambda_sse_me[0];
        asic->regs.lamda_SAO_chroma = 0.75 * asic->regs.lambda_sse_me[0];
        break;
    case 5:    /* H2V5 rev01 */
    case 6:    /* H2V6 rev01 */
    case 7:    /* H2V7 rev01 */
    case 0x60: /* VC8000E 6.0 */
    case 0x61: /* VC8000E 6.1 */
    case 0x62:
    case 0x81:
    case 0x82:
    default:
        LamdaTableQp(&asic->regs, asic->regs.qp, type, asic->regs.poc, qp_factor, is_depth0,
                     enable_ctu_rc);
        FillIntraFactor(&asic->regs);
        asic->regs.lamda_SAO_luma = asic->regs.lambda_sse_me[0];
        asic->regs.lamda_SAO_chroma = 0.75 * asic->regs.lambda_sse_me[0];
        // sao lambda registers are not expanded
        asic->regs.lamda_SAO_luma >>= MOTION_LAMBDA_SSE_SHIFT;
        asic->regs.lamda_SAO_chroma >>= MOTION_LAMBDA_SSE_SHIFT;
        break;
    }

    {
        u32 maxSaoLambda = (1 << 14) - 1;
        asic->regs.lamda_SAO_luma = MIN(asic->regs.lamda_SAO_luma, maxSaoLambda);
        asic->regs.lamda_SAO_chroma = MIN(asic->regs.lamda_SAO_chroma, maxSaoLambda);
    }

    /* For experiment, lambda setting for subjective quality */
    if (vcenc_instance->rateControl.visualLmdTuning && asic->regs.asicCfg.roiAbsQpSupport)
        LambdaTuningSubjective(asic, pic, pEncIn);

    /* For experiment, tuning rdoq lambda for subjective quality */
    true_e rdoqMapEnable = asic->regs.rdoqMapEnable && (asic->regs.asicCfg.roiMapVersion == 4) &&
                           (asic->regs.RoiQpDelta_ver == 4);
    if (vcenc_instance->bRdoqLambdaAdjust && (asic->regs.bRDOQEnable || rdoqMapEnable) &&
        (IS_H264(vcenc_instance->codecFormat) || IS_HEVC(vcenc_instance->codecFormat)) &&
        asic->regs.asicCfg.roiAbsQpSupport && asic->regs.asicCfg.tuneToolsSet2Support) {
        const u32 factorIntra = RDOQ_LMD_INTRA_FACTOR_SCALED;
        const u32 factorInter = RDOQ_LMD_INTER_FACTOR_SCALED;
        vcenc_instance->asic.regs.av1_plane_rdmult[0][0] =
            vcenc_instance->asic.regs.av1_plane_rdmult[0][1] =
                vcenc_instance->asic.regs.av1_plane_rdmult[1][0] =
                    vcenc_instance->asic.regs.av1_plane_rdmult[1][1] = 0;
        if (factorIntra < asic->regs.qpFactorSse) {
            vcenc_instance->asic.regs.av1_plane_rdmult[0][0] =
                vcenc_instance->asic.regs.av1_plane_rdmult[0][1] =
                    ((factorIntra << 4) + asic->regs.qpFactorSse / 2) / asic->regs.qpFactorSse;
        }

        if (factorInter < asic->regs.qpFactorSse) {
            vcenc_instance->asic.regs.av1_plane_rdmult[1][0] =
                vcenc_instance->asic.regs.av1_plane_rdmult[1][1] =
                    ((factorInter << 4) + asic->regs.qpFactorSse / 2) / asic->regs.qpFactorSse;
        }
    }
}

/*------------------------------------------------------------------------------

    Set encoding parameters at the beginning of a new frame.

------------------------------------------------------------------------------*/
void VCEncSetNewFrame(VCEncInst inst)
{
    struct vcenc_instance *vcenc_instance = (struct vcenc_instance *)inst;
    asicData_s *asic = &vcenc_instance->asic;
    regValues_s *regs = &vcenc_instance->asic.regs;
    i32 qpHdr, qpMin, qpMax;
    i32 qp[4];
    i32 s, i, aroiDeltaQ = 0, tmp;

    regs->outputStrmSize[0] -= vcenc_instance->stream.byteCnt;
    /* stream base address */
    regs->outputStrmBase[0] += vcenc_instance->stream.byteCnt;

    /* Enable slice ready interrupts if defined by config and slices in use */
    regs->sliceReadyInterrupt = ENCH2_SLICE_READY_INTERRUPT & (regs->sliceNum > 1);

    /* HW base address for NAL unit sizes is affected by start offset
   * and SW created NALUs. */
    regs->sizeTblBase =
        asic->sizeTbl[vcenc_instance->jobCnt % vcenc_instance->parallelCoreNum].busAddress +
        vcenc_instance->numNalus[vcenc_instance->jobCnt % vcenc_instance->parallelCoreNum] * 4;

    /* HW Base must be 64-bit aligned */
    //ASSERT(regs->sizeTblBase%8 == 0);

    /* low latency: configure related register.*/
    regs->lineBufferEn = vcenc_instance->inputLineBuf.inputLineBufEn;
    regs->lineBufferHwHandShake = vcenc_instance->inputLineBuf.inputLineBufHwModeEn;
    regs->lineBufferLoopBackEn = vcenc_instance->inputLineBuf.inputLineBufLoopBackEn;
    regs->lineBufferDepth = vcenc_instance->inputLineBuf.inputLineBufDepth;
    regs->amountPerLoopBack = vcenc_instance->inputLineBuf.amountPerLoopBack;
    regs->mbWrPtr = vcenc_instance->inputLineBuf.wrCnt;
    regs->mbRdPtr = 0;
    regs->lineBufferInterruptEn = ENCH2_INPUT_BUFFER_INTERRUPT & regs->lineBufferEn &
                                  (regs->lineBufferHwHandShake == 0) & (regs->lineBufferDepth > 0);
    regs->initSegNum = vcenc_instance->inputLineBuf.initSegNum;
    regs->sbi_id_0 = vcenc_instance->inputLineBuf.sbi_id_0;
    regs->sbi_id_1 = vcenc_instance->inputLineBuf.sbi_id_1;
    regs->sbi_id_2 = vcenc_instance->inputLineBuf.sbi_id_2;
}

/* DeNoise Filter */
static void VCEncHEVCDnfInit(struct vcenc_instance *vcenc_instance)
{
    vcenc_instance->uiNoiseReductionEnable = 0;

#if USE_TOP_CTRL_DENOISE
    vcenc_instance->iNoiseL = 10 << FIX_POINT_BIT_WIDTH;
    vcenc_instance->iSigmaCur = vcenc_instance->iFirstFrameSigma = 11 << FIX_POINT_BIT_WIDTH;
    //printf("init seq noiseL = %d, SigCur = %d, first = %d\n\n\n\n\n",pEncInst->iNoiseL, regs->nrSigmaCur,  pCodeParams->firstFrameSigma);
#endif
}

static void VCEncHEVCDnfGetParameters(struct vcenc_instance *inst, VCEncCodingCtrl *pCodeParams)
{
    pCodeParams->noiseReductionEnable = inst->uiNoiseReductionEnable;
#if USE_TOP_CTRL_DENOISE
    pCodeParams->noiseLow = inst->iNoiseL >> FIX_POINT_BIT_WIDTH;
    pCodeParams->firstFrameSigma = inst->iFirstFrameSigma >> FIX_POINT_BIT_WIDTH;
#endif
}

// run before HW run, set register's value according to coding settings
static void VCEncHEVCDnfPrepare(struct vcenc_instance *vcenc_instance)
{
    asicData_s *asic = &vcenc_instance->asic;
#if USE_TOP_CTRL_DENOISE
    asic->regs.nrSigmaCur = vcenc_instance->iSigmaCur;
    asic->regs.nrThreshSigmaCur = vcenc_instance->iThreshSigmaCur;
    asic->regs.nrSliceQPPrev = vcenc_instance->iSliceQPPrev;
#endif
}

// run after HW finish one frame, update collected parameters
static void VCEncHEVCDnfUpdate(struct vcenc_instance *vcenc_instance)
{
#if USE_TOP_CTRL_DENOISE
    const int KK = 102;
    unsigned int j = 0;
    int CurSigmaTmp = 0;
    unsigned int SigmaSmoothFactor[5] = {1024, 512, 341, 256, 205};
    int dSumFrmNoiseSigma = 0;
    int QpSlc = vcenc_instance->asic.regs.qp;
    int FrameCodingType = vcenc_instance->asic.regs.frameCodingType;
    unsigned int uiFrmNum = vcenc_instance->uiFrmNum++;

    // check pre-conditions
    if (vcenc_instance->uiNoiseReductionEnable == 0 || vcenc_instance->stream.byteCnt == 0)
        return;

    if (uiFrmNum == 0)
        vcenc_instance->FrmNoiseSigmaSmooth[0] =
            vcenc_instance
                ->iFirstFrameSigma; //(pvcenc_instance->asic.regs.firstFrameSigma<< FIX_POINT_BIT_WIDTH);//pvcenc_instance->iFirstFrmSigma;//(double)((51-QpSlc-5)/2);
    int iFrmSigmaRcv = vcenc_instance->iSigmaCalcd;
    //int iThSigmaRcv = vcenc_instance->iThreshSigmaCalcd;
    int iThSigmaRcv = (FrameCodingType != 1) ? vcenc_instance->iThreshSigmaCalcd
                                             : vcenc_instance->iThreshSigmaPrev;
    //printf("iFrmSigRcv = %d\n\n\n", iFrmSigmaRcv);
    if (iFrmSigmaRcv == 0xFFFF) {
        iFrmSigmaRcv = vcenc_instance->iFirstFrameSigma;
        //printf("initial to %d\n", iFrmSigmaRcv);
    }
    iFrmSigmaRcv = (iFrmSigmaRcv * KK) >> 7;
    if (iFrmSigmaRcv < vcenc_instance->iNoiseL)
        iFrmSigmaRcv = 0;
    vcenc_instance->FrmNoiseSigmaSmooth[(uiFrmNum + 1) % SIGMA_SMOOTH_NUM] = iFrmSigmaRcv;
    for (j = 0; j < (MIN(SIGMA_SMOOTH_NUM - 1, uiFrmNum + 1) + 1); j++) {
        dSumFrmNoiseSigma += vcenc_instance->FrmNoiseSigmaSmooth[j];
        //printf("FrmNoiseSig %d = %d\n", j, pvcenc_instance->FrmNoiseSigmaSmooth[j]);
    }
    CurSigmaTmp =
        (dSumFrmNoiseSigma * SigmaSmoothFactor[MIN(SIGMA_SMOOTH_NUM - 1, uiFrmNum + 1)]) >> 10;
    vcenc_instance->iSigmaCur = CLIP3(0, (SIGMA_MAX << FIX_POINT_BIT_WIDTH), CurSigmaTmp);
    vcenc_instance->iThreshSigmaCur = vcenc_instance->iThreshSigmaPrev = iThSigmaRcv;
    vcenc_instance->iSliceQPPrev = QpSlc;
    //printf("TOP::uiFrmNum = %d, FrmSig=%d, Th=%d, QP=%d, QP2=%d\n", uiFrmNum,pvcenc_instance->asic.regs.nrSigmaCur, iThSigmaRcv, QpSlc, QpSlc2 );
#endif
}

/*------------------------------------------------------------------------------
    Function name : VCEncGetEncodedMbLines
    Description   : Get how many MB lines has been encoded by encoder.
    Return type   : int
    Argument      : inst - encoder instance
------------------------------------------------------------------------------*/
u32 VCEncGetEncodedMbLines(VCEncInst inst)
{
    struct vcenc_instance *vcenc_instance = (struct vcenc_instance *)inst;
    u32 lines;

    APITRACE("VCEncGetEncodedMbLines#\n");

    /* Check for illegal inputs */
    if (!vcenc_instance) {
        APITRACE("VCEncGetEncodedMbLines: ERROR Null argument\n");
        return VCENC_NULL_ARGUMENT;
    }

    if (!vcenc_instance->inputLineBuf.inputLineBufEn) {
        APITRACE("VCEncGetEncodedMbLines: ERROR Invalid mode for input control\n");
        return VCENC_INVALID_ARGUMENT;
    }

    if (vcenc_instance->inputLineBuf.inputLineBufHwModeEn) {
        lines = EWLReadReg(vcenc_instance->asic.ewl, 196 * 4);
        lines = ((lines & 0x000ffc00) >> 10);
    } else
        lines = EncAsicGetRegisterValue(vcenc_instance->asic.ewl,
                                        vcenc_instance->asic.regs.regMirror, HWIF_CTB_ROW_RD_PTR);

    return lines;
}

/*------------------------------------------------------------------------------
    Function name : VCEncSetInputMBLines
    Description   : Low latency:  sets the valid input MB lines for the encoder to work. This function is only valid for HEVC.
    Return type   : VCEncRet
    Argument      : inst - encoder instance
    Argument      : lines - macroblock number
------------------------------------------------------------------------------*/
VCEncRet VCEncSetInputMBLines(VCEncInst inst, u32 lines)
{
    struct vcenc_instance *vcenc_instance = (struct vcenc_instance *)inst;

    APITRACE("VCEncSetInputMBLines#\n");

    /* Check for illegal inputs */
    if (!vcenc_instance) {
        APITRACE("VCEncSetInputMBLines: ERROR Null argument\n");
        return VCENC_NULL_ARGUMENT;
    }

    if (!vcenc_instance->inputLineBuf.inputLineBufEn) {
        APITRACE("VCEncSetInputMBLines: ERROR Invalid mode for input control\n");
        return VCENC_INVALID_ARGUMENT;
    }
    EncAsicWriteRegisterValue(vcenc_instance->asic.ewl, vcenc_instance->asic.regs.regMirror,
                              HWIF_CTB_ROW_WR_PTR, lines);
    return VCENC_OK;
}

/*------------------------------------------------------------------------------
Function name : VCEncGenSliceHeaderRps
Description   : cal RPS parameter in the slice header.
Return type   : void
Argument      : VCEncPictureCodingType codingType
Argument      : VCEncGopPicConfig *cfg
------------------------------------------------------------------------------*/
void VCEncGenSliceHeaderRps(struct vcenc_instance *vcenc_instance,
                            VCEncPictureCodingType codingType, VCEncGopPicConfig *cfg)
{
    regValues_s *regs = &vcenc_instance->asic.regs;
    HWRPS_CONFIG *pHwRps = &vcenc_instance->sHwRps;
    i32 i, j, tmp, i32TempPoc;
    i32 i32NegDeltaPoc[VCENC_MAX_REF_FRAMES], i32PosDeltaPoc[VCENC_MAX_REF_FRAMES];
    i32 i32NegDeltaPocUsed[VCENC_MAX_REF_FRAMES], i32PosDeltaPocUsed[VCENC_MAX_REF_FRAMES];
    u32 used_by_cur;

    ASSERT(cfg != NULL);
    ASSERT(codingType < VCENC_NOTCODED_FRAME);

    for (i = 0; i < VCENC_MAX_REF_FRAMES; i++) {
        pHwRps->u20DeltaPocS0[i] = 0;
        pHwRps->u20DeltaPocS1[i] = 0;

        i32NegDeltaPoc[i] = 0;
        i32PosDeltaPoc[i] = 0;
        i32NegDeltaPocUsed[i] = 0;
        i32PosDeltaPocUsed[i] = 0;
    }

    pHwRps->u3NegPicNum = 0;
    pHwRps->u3PosPicNum = 0;

    for (i = 0; i < (i32)cfg->numRefPics; i++) {
        if (cfg->refPics[i].ref_pic < 0) {
            i32NegDeltaPoc[pHwRps->u3NegPicNum] = cfg->refPics[i].ref_pic;
            i32NegDeltaPocUsed[pHwRps->u3NegPicNum] = cfg->refPics[i].used_by_cur;

            pHwRps->u3NegPicNum++;
        } else {
            i32PosDeltaPoc[pHwRps->u3PosPicNum] = cfg->refPics[i].ref_pic;
            i32PosDeltaPocUsed[pHwRps->u3PosPicNum] = cfg->refPics[i].used_by_cur;

            pHwRps->u3PosPicNum++;
        }
    }

    for (i = 0; i < pHwRps->u3NegPicNum; i++) {
        i32TempPoc = i32NegDeltaPoc[i];
        for (j = i; j < (i32)pHwRps->u3NegPicNum; j++) {
            if (i32NegDeltaPoc[j] > i32TempPoc) {
                i32NegDeltaPoc[i] = i32NegDeltaPoc[j];
                i32NegDeltaPoc[j] = i32TempPoc;
                i32TempPoc = i32NegDeltaPoc[i];

                used_by_cur = i32NegDeltaPocUsed[i];
                i32NegDeltaPocUsed[i] = i32NegDeltaPocUsed[j];
                i32NegDeltaPocUsed[j] = used_by_cur;
            }
        }
    }

    for (i = 0; i < pHwRps->u3PosPicNum; i++) {
        i32TempPoc = i32PosDeltaPoc[i];
        for (j = i; j < (i32)pHwRps->u3PosPicNum; j++) {
            if (i32PosDeltaPoc[j] < i32TempPoc) {
                i32PosDeltaPoc[i] = i32PosDeltaPoc[j];
                i32PosDeltaPoc[j] = i32TempPoc;
                i32TempPoc = i32PosDeltaPoc[i];

                used_by_cur = i32PosDeltaPocUsed[i];
                i32PosDeltaPocUsed[i] = i32PosDeltaPocUsed[j];
                i32PosDeltaPocUsed[j] = used_by_cur;
            }
        }
    }

    tmp = 0;
    for (i = 0; i < pHwRps->u3NegPicNum; i++) {
        ASSERT((-i32NegDeltaPoc[i] - 1 + tmp) < 0x100000);
        pHwRps->u1DeltaPocS0Used[i] = i32NegDeltaPocUsed[i] & 0x01;
        pHwRps->u20DeltaPocS0[i] = -i32NegDeltaPoc[i] - 1 + tmp;
        tmp = i32NegDeltaPoc[i];
    }
    tmp = 0;
    for (i = 0; i < pHwRps->u3PosPicNum; i++) {
        ASSERT((i32PosDeltaPoc[i] - 1 - tmp) < 0x100000);
        pHwRps->u1DeltaPocS1Used[i] = i32PosDeltaPocUsed[i] & 0x01;
        pHwRps->u20DeltaPocS1[i] = i32PosDeltaPoc[i] - 1 - tmp;
        tmp = i32PosDeltaPoc[i];
    }

    regs->short_term_ref_pic_set_sps_flag = pHwRps->u1short_term_ref_pic_set_sps_flag;
    regs->rps_neg_pic_num = pHwRps->u3NegPicNum;
    regs->rps_pos_pic_num = pHwRps->u3PosPicNum;
    ASSERT(pHwRps->u3NegPicNum + pHwRps->u3PosPicNum <= VCENC_MAX_REF_FRAMES);
    for (i = 0, j = 0; i < pHwRps->u3NegPicNum; i++, j++) {
        regs->rps_delta_poc[j] = pHwRps->u20DeltaPocS0[i];
        regs->rps_used_by_cur[j] = pHwRps->u1DeltaPocS0Used[i];
    }
    for (i = 0; i < pHwRps->u3PosPicNum; i++, j++) {
        regs->rps_delta_poc[j] = pHwRps->u20DeltaPocS1[i];
        regs->rps_used_by_cur[j] = pHwRps->u1DeltaPocS1Used[i];
    }
    for (; j < VCENC_MAX_REF_FRAMES; j++) {
        regs->rps_delta_poc[j] = 0;
        regs->rps_used_by_cur[j] = 0;
    }
}

/*------------------------------------------------------------------------------
Function name : VCEncGenPicRefConfig
Description   : cal the ref pic of current pic.
Return type   : void
Argument      : struct container *c
Argument      : VCEncGopPicConfig *cfg
Argument      : struct sw_picture *pCurPic
Argument      : i32 curPicPoc
------------------------------------------------------------------------------*/
void VCEncGenPicRefConfig(struct container *c, VCEncGopPicConfig *cfg, struct sw_picture *pCurPic,
                          i32 curPicPoc)
{
    struct sw_picture *p;
    struct node *n;
    i32 i, j;
    i32 i32PicPoc[VCENC_MAX_REF_FRAMES];

    ASSERT(c != NULL);
    ASSERT(cfg != NULL);
    ASSERT(pCurPic != NULL);
    ASSERT(curPicPoc >= 0);

    for (n = c->picture.tail; n; n = n->next) {
        p = (struct sw_picture *)n;
        if ((p->long_term_flag == HANTRO_FALSE) && (p->reference == HANTRO_TRUE) && (p->poc > -1) &&
            cfg->numRefPics <= VCENC_MAX_REF_FRAMES) {
            i32PicPoc[cfg->numRefPics] = p->poc; /* looking short_term ref */
            cfg->numRefPics++;
        }
    }

    for (i = 0; i < (i32)cfg->numRefPics; i++) {
        cfg->refPics[i].used_by_cur = 0;

        for (j = 0; j < pCurPic->sliceInst->active_l0_cnt; j++) {
            //ASSERT(pCurPic->rpl[0][j]->long_term_flag == HANTRO_FALSE );
            if (pCurPic->rpl[0][j]->poc == i32PicPoc[i]) {
                cfg->refPics[i].used_by_cur = 1;
                break;
            }
        }

        if (j == pCurPic->sliceInst->active_l0_cnt) {
            for (j = 0; j < pCurPic->sliceInst->active_l1_cnt; j++) {
                //ASSERT(pCurPic->rpl[1][j]->long_term_flag == HANTRO_FALSE);
                if (pCurPic->rpl[1][j]->poc == i32PicPoc[i]) {
                    cfg->refPics[i].used_by_cur = 1;
                    break;
                }
            }
        }
    }

    for (i = 0; i < (i32)cfg->numRefPics; i++) {
        cfg->refPics[i].ref_pic = i32PicPoc[i] - curPicPoc;
    }
}

/*------------------------------------------------------------------------------

    API tracing
        TRacing as defined by the API.
    Params:
        msg - null terminated tracing message
------------------------------------------------------------------------------*/
void VCEncTrace(const char *msg)
{
#ifdef APITRC_STDOUT
    FILE *fp = stdout;
#else
    static FILE *fp = NULL;

    if (fp == NULL)
        fp = fopen("api.trc", "wt");
#endif
    if (fp)
        fprintf(fp, "%s\n", msg);
}

/*------------------------------------------------------------------------------
    Function name : VCEncTraceProfile
    Description   : Tracing PSNR profile result
    Return type   : void
    Argument      : inst - encoder instance
------------------------------------------------------------------------------*/
void VCEncTraceProfile(VCEncInst inst)
{
    ASSERT(inst != NULL);
    struct vcenc_instance *vcenc_instance = (struct vcenc_instance *)inst;
    EWLTraceProfile(vcenc_instance->asic.ewl, NULL, 0, 0);
}

/*------------------------------------------------------------------------------
    Function name : VCEncGetAsicConfig
    Description   : Get HW configuration.
    Return type   : EWLHwConfig_t
    Argument      : codecFormat - codec format
------------------------------------------------------------------------------*/
EWLHwConfig_t VCEncGetAsicConfig(VCEncVideoCodecFormat codecFormat, void *ctx)
{
    u32 clientType = VCEncGetClientType(codecFormat);
    return EncAsicGetAsicConfig(clientType, ctx);
}

/**
 * add filler data NULU for CBR Mode.
 *
 * \param [in] inst encoder instance
 * \param [in] length total filler byte length to avoid CPB underflow
 * \return the byte number of added filler data. zero means that the filler data is not added.
 *
 * \note work for HEVC/H264 only, ignore for other formats now.
 */
static i32 VCEncWriteFillerDataForCBR(struct vcenc_instance *vcenc, i32 length)
{
    i32 has_startcode = (vcenc->asic.regs.streamMode == VCENC_BYTE_STREAM) ? ENCHW_YES : ENCHW_NO;
    i32 nalu_overhead;
    i32 nalu_length = 0;

    i32 tmp = vcenc->stream.byteCnt;

    /* consider the NALU overhead for different formats. */
    if (vcenc->codecFormat == VCENC_VIDEO_CODEC_HEVC)
        nalu_overhead = has_startcode ? 7 : 3;
    else if (vcenc->codecFormat == VCENC_VIDEO_CODEC_H264)
        nalu_overhead = has_startcode ? 6 : 2;
    else
        return 0;

    if (length < nalu_overhead) {
        VCEncAddFillerNALU(vcenc, 0, has_startcode);
        vcenc->rateControl.virtualBuffer.bucketFullness += (nalu_overhead - length) * 8;
        vcenc->rateControl.virtualBuffer.realBitCnt += (nalu_overhead - length) * 8;
        vcenc->rateControl.virtualBuffer.bucketLevel += (nalu_overhead - length) * 8;
    } else {
        VCEncAddFillerNALU(vcenc, length - nalu_overhead, has_startcode);
    }

    nalu_length = vcenc->stream.byteCnt - tmp;

    return nalu_length;
}

/**
 * Add filler nalu with specified payload size.
 *
 * \param [in] inst encoder instance
 * \param [in] cnt payload size
 * \param [in] has_startcode has start code prefix or not
 * \return none.
 */
static void VCEncAddFillerNALU(struct vcenc_instance *vcenc, i32 cnt, true_e has_startcode)
{
    ASSERT(vcenc != NULL);
    ASSERT(&vcenc->stream != NULL);

    i32 i = cnt;

    if (vcenc->codecFormat == VCENC_VIDEO_CODEC_H264)
        H264NalUnitHdr(&vcenc->stream, 0, H264_FILLERDATA, has_startcode);
    else if (vcenc->codecFormat == VCENC_VIDEO_CODEC_HEVC) {
        HevcNalUnitHdr(&vcenc->stream, FD_NUT, has_startcode);
    }

    for (; i > 0; i--) {
        put_bit(&vcenc->stream, 0xFF, 0x8);
    }

    rbsp_trailing_bits(&vcenc->stream);
}
/** @} */

/* Parameter set send handle */
static void VCEncParameterSetHandle(struct vcenc_instance *vcenc_instance, VCEncInst inst,
                                    const VCEncIn *pEncIn, VCEncOut *pEncOut, struct container *c)
{
    i32 tmp, i; /* tmp is to store temp byteCnt */

    if ((pEncIn->resendVPS) && (vcenc_instance->codecFormat == VCENC_VIDEO_CODEC_HEVC)) {
        struct vps *v;

        v = (struct vps *)get_parameter_set(c, VPS_NUT, vcenc_instance->vps_id);
        v->ps.b = vcenc_instance->stream;
        tmp = vcenc_instance->stream.byteCnt;
        //Generate vps
        video_parameter_set(v, inst);

        //Update recorded streamSize and add new nal into buffer
        pEncOut->streamSize += (vcenc_instance->stream.byteCnt - tmp);
        hash(&vcenc_instance->hashctx, vcenc_instance->stream.stream,
             vcenc_instance->stream.byteCnt - tmp);
        VCEncAddNaluSize(pEncOut, vcenc_instance->stream.byteCnt - tmp, 0);
        pEncOut->PreDataSize += (vcenc_instance->stream.byteCnt - tmp);
        pEncOut->PreNaluNum++;

        tmp = vcenc_instance->stream.byteCnt;
        vcenc_instance->stream = v->ps.b;
        vcenc_instance->stream.byteCnt = tmp;
    }

    if (pEncIn->sendAUD) {
        tmp = vcenc_instance->stream.byteCnt;
        if (IS_H264(vcenc_instance->codecFormat)) {
            H264AccessUnitDelimiter(&vcenc_instance->stream, vcenc_instance->asic.regs.streamMode,
                                    pEncIn->codingType);
        } else {
            HEVCAccessUnitDelimiter(&vcenc_instance->stream, vcenc_instance->asic.regs.streamMode,
                                    pEncIn->codingType);
        }
        pEncOut->streamSize += vcenc_instance->stream.byteCnt - tmp;
        hash(&vcenc_instance->hashctx, vcenc_instance->stream.stream,
             vcenc_instance->stream.byteCnt - tmp);
        VCEncAddNaluSize(pEncOut, vcenc_instance->stream.byteCnt - tmp, 0);
        pEncOut->PreDataSize += (vcenc_instance->stream.byteCnt - tmp);
        pEncOut->PreNaluNum++;
    }

    bool update_pps_after_sps = 0;
    struct sps *s_br = (struct sps *)get_parameter_set(c, SPS_NUT, vcenc_instance->sps_id);

    //BitRate has been rounded in VCEncStrmStart().[WriteVui(),or H264WriteVui()] when hrd=1; BitRate is 0 when hrd = 0.
    u32 old_bitrate = s_br->vui.bitRate;

    //need to round new bitrate
    u32 new_bitrate = round_this_value((u32)vcenc_instance->rateControl.virtualBuffer.bitRate);

    i32 old_frame_rate_num = vcenc_instance->rateControl.virtualBuffer.timeScale;
    i32 old_frame_rate_denom = vcenc_instance->rateControl.virtualBuffer.unitsInTic;
    i32 new_frame_rate_num =
        vcenc_instance->rateControl.outRateNum * (vcenc_instance->interlaced + 1);
    i32 new_frame_rate_denom = vcenc_instance->rateControl.outRateDenom;
    u32 old_hrd = vcenc_instance->rateControl.sei.hrd; //This hrd has not been update.
    u32 new_hrd =
        vcenc_instance->rateControl.hrd; //This value already has been update in VCEncSetRateCtrl.

    if (old_bitrate == 0) //hrd == 0
    {
        s_br->vui.bitRate = new_bitrate; //save this value.
    }
    if (pEncIn->resendSPS ||
        ((new_bitrate != old_bitrate) && (old_bitrate != 0)) || //bitrate change
        (new_frame_rate_num != old_frame_rate_num ||
         new_frame_rate_denom != old_frame_rate_denom) || //framerate change
        (new_hrd != old_hrd))                             //hrd conformance change
    {
        s_br->ps.b = vcenc_instance->stream;
        tmp = vcenc_instance->stream.byteCnt;

        VCEncSpsSetVuiAspectRatio(s_br, vcenc_instance->sarWidth, vcenc_instance->sarHeight);
        VCEncSpsSetVuiVideoInfo(s_br, vcenc_instance->vuiVideoFullRange);

        if (vcenc_instance->vuiVideoSignalTypePresentFlag)
            VCEncSpsSetVuiSignalType(s_br, vcenc_instance->vuiVideoSignalTypePresentFlag,
                                     vcenc_instance->vuiVideoFormat,
                                     vcenc_instance->vuiVideoFullRange,
                                     vcenc_instance->vuiColorDescription.vuiColorDescripPresentFlag,
                                     vcenc_instance->vuiColorDescription.vuiColorPrimaries,
                                     vcenc_instance->vuiColorDescription.vuiTransferCharacteristics,
                                     vcenc_instance->vuiColorDescription.vuiMatrixCoefficients);

        if ((new_bitrate != old_bitrate) && (old_bitrate != 0)) //bitrate change, update vui.
        {
            VCEncSpsSetVuiHrd(s_br, vcenc_instance->rateControl.hrd);
            VCEncSpsSetVuiHrdBitRate(s_br, new_bitrate);
            VCEncSpsSetVuiHrdCpbSize(s_br, vcenc_instance->rateControl.virtualBuffer.bufferSize);
            update_pps_after_sps = 1;
        }

        if (new_frame_rate_num != old_frame_rate_num ||
            new_frame_rate_denom != old_frame_rate_denom) //frame rate change, update vui.
        {
            rcVirtualBuffer_s *vb = &vcenc_instance->rateControl.virtualBuffer;
            vb->timeScale = new_frame_rate_num;
            vb->unitsInTic = new_frame_rate_denom;
            if (pEncIn->vui_timing_info_enable) {
                VCEncSpsSetVuiTimigInfo(s_br, vb->timeScale, vb->unitsInTic);
            } else {
                VCEncSpsSetVuiTimigInfo(s_br, 0, 0);
            }
            update_pps_after_sps = 1;
        }
        if (new_hrd != old_hrd) //hrd change, update vui.
        {
            vcenc_instance->rateControl.sei.hrd = (u32)vcenc_instance->rateControl.hrd;
            VCEncSpsSetVuiHrd(s_br, vcenc_instance->rateControl.hrd);
            update_pps_after_sps = 1;
        }
        sequence_parameter_set(c, s_br, inst);

        pEncOut->streamSize += vcenc_instance->stream.byteCnt - tmp;
        hash(&vcenc_instance->hashctx, vcenc_instance->stream.stream,
             vcenc_instance->stream.byteCnt - tmp);
        VCEncAddNaluSize(pEncOut, vcenc_instance->stream.byteCnt - tmp, 0);
        pEncOut->PreDataSize += (vcenc_instance->stream.byteCnt - tmp);
        pEncOut->PreNaluNum++;

        tmp = vcenc_instance->stream.byteCnt;
        vcenc_instance->stream = s_br->ps.b;
        vcenc_instance->stream.byteCnt = tmp;
    }

    if (pEncIn->resendPPS || update_pps_after_sps) {
        if (update_pps_after_sps)
            update_pps_after_sps = 0;
        struct pps *p;
        for (i = 0; i <= vcenc_instance->maxPPSId; i++) {
            p = (struct pps *)get_parameter_set(c, PPS_NUT, i);
            p->ps.b = vcenc_instance->stream;
            tmp = vcenc_instance->stream.byteCnt;
            //printf("byteCnt=%d\n",vcenc_instance->stream.byteCnt);
            picture_parameter_set(p, inst);

            //printf("pps inserted size=%d\n", vcenc_instance->stream.byteCnt-tmp);
            pEncOut->streamSize += vcenc_instance->stream.byteCnt - tmp;
            hash(&vcenc_instance->hashctx, vcenc_instance->stream.stream,
                 vcenc_instance->stream.byteCnt - tmp);
            VCEncAddNaluSize(pEncOut, vcenc_instance->stream.byteCnt - tmp, 0);
            pEncOut->PreDataSize += (vcenc_instance->stream.byteCnt - tmp);
            pEncOut->PreNaluNum++;

            tmp = vcenc_instance->stream.byteCnt;
            vcenc_instance->stream = p->ps.b;
            vcenc_instance->stream.byteCnt = tmp;
        }
    }

    if (vcenc_instance->insertNewPPS) {
        struct pps *p;

        p = (struct pps *)get_parameter_set(c, PPS_NUT, vcenc_instance->insertNewPPSId);
        p->ps.b = vcenc_instance->stream;
        tmp = vcenc_instance->stream.byteCnt;
        //printf("byteCnt=%d\n",vcenc_instance->stream.byteCnt);
        picture_parameter_set(p, inst);

        //printf("pps inserted size=%d\n", vcenc_instance->stream.byteCnt-tmp);
        pEncOut->streamSize += vcenc_instance->stream.byteCnt - tmp;
        hash(&vcenc_instance->hashctx, vcenc_instance->stream.stream,
             vcenc_instance->stream.byteCnt - tmp);
        VCEncAddNaluSize(pEncOut, vcenc_instance->stream.byteCnt - tmp, 0);
        pEncOut->PreDataSize += (vcenc_instance->stream.byteCnt - tmp);
        pEncOut->PreNaluNum++;

        //printf("pps size=%d\n", *p->ps.b.cnt);
        tmp = vcenc_instance->stream.byteCnt;
        vcenc_instance->stream = p->ps.b;
        vcenc_instance->stream.byteCnt = tmp;

        //printf("byteCnt=%d\n",vcenc_instance->stream.byteCnt);
        vcenc_instance->insertNewPPS = 0;
    }
}

u64 getMaxSBPS(VCEncVideoCodecFormat codecFormat, i32 levelIdx)
{
    ASSERT(levelIdx >= 0);
    i32 level = MAX(0, levelIdx);
    u64 maxSBPS = 0;

    switch (codecFormat) {
    case VCENC_VIDEO_CODEC_HEVC:
        ASSERT(level < HEVC_LEVEL_NUM);
        level = MIN(level, (HEVC_LEVEL_NUM - 1));
        maxSBPS = VCEncMaxSBPSHevc[level];
        break;

    case VCENC_VIDEO_CODEC_H264:
        ASSERT(level < H264_LEVEL_NUM);
        level = MIN(level, (H264_LEVEL_NUM - 1));
        maxSBPS = VCEncMaxSBPSH264[level];
        break;

    case VCENC_VIDEO_CODEC_AV1:
        level = MIN(level, (AV1_VALID_MAX_LEVEL - 1));
        maxSBPS = VCEncMaxSBPSAV1[level];
        break;

    case VCENC_VIDEO_CODEC_VP9:
        level = MIN(level, (VP9_VALID_MAX_LEVEL - 1));
        maxSBPS = VCEncMaxSBPSVP9[level];
        break;

    default:
        break;
    }

    return maxSBPS;
}

u32 getMaxPicSize(VCEncVideoCodecFormat codecFormat, i32 levelIdx)
{
    ASSERT(levelIdx >= 0);
    i32 level = MAX(0, levelIdx);
    u32 maxPicSize = 0;

    switch (codecFormat) {
    case VCENC_VIDEO_CODEC_HEVC:
        ASSERT(level < HEVC_LEVEL_NUM);
        level = MIN(level, (HEVC_LEVEL_NUM - 1));
        maxPicSize = VCEncMaxPicSizeHevc[level];
        break;

    case VCENC_VIDEO_CODEC_H264:
        ASSERT(level < H264_LEVEL_NUM);
        level = MIN(level, (H264_LEVEL_NUM - 1));
        maxPicSize = VCEncMaxFSH264[level];
        break;

    case VCENC_VIDEO_CODEC_AV1:
        level = MIN(level, (AV1_VALID_MAX_LEVEL - 1));
        maxPicSize = VCEncMaxPicSizeAV1[level];
        break;

    case VCENC_VIDEO_CODEC_VP9:
        level = MIN(level, (VP9_VALID_MAX_LEVEL - 1));
        maxPicSize = VCEncMaxPicSizeVP9[level];
        break;

    default:
        break;
    }

    return maxPicSize;
}

u32 getMaxCPBS(VCEncVideoCodecFormat codecFormat, i32 levelIdx, i32 profile, i32 tier)
{
    ASSERT(levelIdx >= 0);
    i32 level = MAX(0, levelIdx);
    u32 maxCBPS = 0;

    switch (codecFormat) {
    case VCENC_VIDEO_CODEC_HEVC:
        ASSERT(level < HEVC_LEVEL_NUM);
        level = MIN(level, (HEVC_LEVEL_NUM - 1));
        maxCBPS = (tier == VCENC_HEVC_HIGH_TIER) ? VCEncMaxCPBSHighTierHevc[level]
                                                 : VCEncMaxCPBSHevc[level];
        break;

    case VCENC_VIDEO_CODEC_H264:
        ASSERT(level < H264_LEVEL_NUM);
        level = MIN(level, (H264_LEVEL_NUM - 1));
        float h264_high10_factor = 1;
        if (profile == 100)
            h264_high10_factor = 1.25;
        else if (profile == 110)
            h264_high10_factor = 3.0;
        maxCBPS = VCEncMaxCPBSH264[level] * h264_high10_factor;
        break;

    case VCENC_VIDEO_CODEC_AV1:
        level = MIN(level, (AV1_VALID_MAX_LEVEL - 1));
        maxCBPS = ((tier == VCENC_HEVC_HIGH_TIER) || (level > 7)) ? VCEncMaxCPBSHighTierAV1[level]
                                                                  : VCEncMaxCPBSAV1[level];
        break;

    case VCENC_VIDEO_CODEC_VP9:
        level = MIN(level, (VP9_VALID_MAX_LEVEL - 1));
        maxCBPS = VCEncMaxCPBSVP9[level];
        break;

    default:
        break;
    }

    return maxCBPS;
}

u32 getMaxBR(VCEncVideoCodecFormat codecFormat, i32 levelIdx, i32 profile, i32 tier)
{
    ASSERT(levelIdx >= 0);
    i32 level = MAX(0, levelIdx);
    u32 maxBR = 0;

    switch (codecFormat) {
    case VCENC_VIDEO_CODEC_HEVC:
        ASSERT(level < HEVC_LEVEL_NUM);
        level = MIN(level, (HEVC_LEVEL_NUM - 1));
        maxBR =
            (tier == VCENC_HEVC_HIGH_TIER) ? VCEncMaxBRHighTierHevc[level] : VCEncMaxBRHevc[level];
        break;

    case VCENC_VIDEO_CODEC_H264:
        ASSERT(level < H264_LEVEL_NUM);
        level = MIN(level, (H264_LEVEL_NUM - 1));
        float h264_high10_factor = 1;
        if (profile == 100)
            h264_high10_factor = 1.25;
        else if (profile == 110)
            h264_high10_factor = 3.0;
        maxBR = VCEncMaxBRH264[level] * h264_high10_factor;
        break;

    case VCENC_VIDEO_CODEC_AV1:
        level = MIN(level, (AV1_VALID_MAX_LEVEL - 1));
        maxBR = ((tier == VCENC_HEVC_HIGH_TIER) || (level > 7)) ? VCEncMaxCPBSHighTierAV1[level]
                                                                : VCEncMaxCPBSAV1[level];
        break;

    case VCENC_VIDEO_CODEC_VP9:
        level = MIN(level, (VP9_VALID_MAX_LEVEL - 1));
        maxBR = VCEncMaxBRVP9[level];
        break;

    default:
        break;
    }

    return maxBR;
}

/* Get encoding buffer delay */
i32 VCEncGetEncodeMaxDelay(VCEncInst inst)
{
    struct vcenc_instance *pEncInst = (struct vcenc_instance *)inst;
    i32 delay = 0;
    if (pEncInst->pass)
        delay = pEncInst->lookaheadDelay;
    else
        delay = pEncInst->gopSize ? pEncInst->gopSize - 1 : MAX_ADAPTIVE_GOP_SIZE - 1;

    /* add delay caused by multi-core */
    delay += pEncInst->parallelCoreNum - 1;
    return delay;
}

/*----------------------------------------------------------------------------------
    Function name : VCEncCollectEncodeStats
    Description   : collect some encode parameters needed after encoding a frame
    Return type   : void
    Argument      : vcenc_instance - encoder instance
    Argument      : pEncIn - user provided input parameters
    Argument      : pEncOut - place where output info is returned
    Argument      : codingType - current encoding frame type, e.g. I or B or P
    Argument      : enc_statistic - the container used to filler encode parameters
------------------------------------------------------------------------------------*/
void VCEncCollectEncodeStats(struct vcenc_instance *vcenc_instance, VCEncIn *pEncIn,
                             VCEncOut *pEncOut, VCEncPictureCodingType codingType,
                             VCEncStatisticOut *enc_statistic)
{
    asicData_s *asic = &vcenc_instance->asic;

    enc_statistic->kEwl = asic->ewl;
    enc_statistic->avg_qp_y = asic->regs.qp;
    if (IS_CTBRC_FOR_BITRATE(vcenc_instance->rateControl.ctbRc)) {
        float f_tolCtbRc = (codingType == VCENC_INTRA_FRAME)
                               ? vcenc_instance->rateControl.tolCtbRcIntra
                               : vcenc_instance->rateControl.tolCtbRcInter;
        if (f_tolCtbRc >= 0)
            enc_statistic->avg_qp_y = (vcenc_instance->rateControl.ctbRateCtrl.qpSumForRc +
                                       vcenc_instance->rateControl.ctbPerPic / 2) /
                                      vcenc_instance->rateControl.ctbPerPic;
    }
    enc_statistic->frame_target_bits = asic->regs.targetPicSize;
    enc_statistic->frame_real_bits = vcenc_instance->stream.byteCnt * 8;
    enc_statistic->psnr_y = pEncOut->psnr_y_predeb;
    enc_statistic->header_stream_byte = pEncOut->PreDataSize;
    enc_statistic->kOutputbufferMem[0] = pEncIn->cur_out_buffer[0];
    enc_statistic->output_buffer_over_flow = vcenc_instance->output_buffer_over_flow;
#ifdef SUPPORT_AV1
    /* Some parameters need to be reset for AV1 */
    if (IS_AV1(vcenc_instance->codecFormat)) {
        if (pEncIn->bIsIDR)
            enc_statistic->POCtobeDisplayAV1 = 0;
        else {
            vcenc_instance_av1 *av1_inst = &vcenc_instance->av1_inst;
            enc_statistic->POCtobeDisplayAV1 =
                (av1_inst->POCtobeDisplayAV1 + av1_inst->MaxPocDisplayAV1 - 1) %
                av1_inst->MaxPocDisplayAV1;
        }
    }
#endif
}

/*----------------------------------------------------------------------------------
    Function name : VCEncResetEncodeStatus
    Description   : replace the encode parameters with new parameters
    Return type   : void
    Argument      : vcenc_instance - encoder instance
    Argument      : c - Encoder internal store
    Argument      : pEncIn - user provided input parameters
    Argument      : pic - current encoding frame
    Argument      : pEncOut - place where output info is returned
    Argument      : enc_statistic - some parameters need to be reset
------------------------------------------------------------------------------------*/
VCEncRet VCEncResetEncodeStatus(struct vcenc_instance *vcenc_instance, struct container *c,
                                VCEncIn *pEncIn, struct sw_picture *pic, VCEncOut *pEncOut,
                                const VCEncStatisticOut *enc_statistic)
{
#ifdef SUPPORT_AV1
    if (IS_AV1(vcenc_instance->codecFormat)) {
        if (vcenc_instance->lookaheadDepth)
            vcenc_instance->av1_inst.POCtobeDisplayAV1 =
                enc_statistic->POCtobeDisplayAV1; /* must be called firstly */
        if (VCENC_OK != VCEncGenAV1Config(vcenc_instance, c, pEncIn, pic, pEncOut->codingType))
            return VCENC_ERROR;
    }
#endif
    vcenc_instance->output_buffer_over_flow = 0;

    return VCENC_OK;
}

/*----------------------------------------------------------------------------------
    Function name : VCEncRertryNewParameters
    Description   : replace the encode parameters with new parameters
    Return type   : void
    Argument      : vcenc_instance - encoder instance
    Argument      : pEncIn - user provided input parameters
    Argument      : pEncOut - place where output info is returned
    Argument      : slice_callback - the callback of slice ready
    Argument      : new_params - the new encode parameters
    Argument      : regs_for2nd_encode - the regs backed up for reEncode
------------------------------------------------------------------------------------*/
void VCEncRertryNewParameters(struct vcenc_instance *vcenc_instance, VCEncIn *pEncIn,
                              VCEncOut *pEncOut, VCEncSliceReady *slice_callback,
                              NewEncodeParams *new_params, regValues_s *regs_for2nd_encode)
{
    u32 i = 0;
    asicData_s *asic = &vcenc_instance->asic;
    i32 coreIdx = vcenc_instance->jobCnt % vcenc_instance->parallelCoreNum;

    vcenc_instance->stream.byteCnt = pEncOut->PreDataSize;

    if (new_params->strategy == NEW_QP) {
        if (vcenc_instance->parallelCoreNum > 1) {
            APITRACEERR("new QP didn't support multi core when re-encode\n");
            ASSERT(0);
        }
        regs_for2nd_encode->qp = new_params->qp;
    } else if (new_params->strategy == NEW_OUTPUT_BUFFER) {
        vcenc_instance->stream.stream = ((u8 *)new_params->output_buffer_mem[0].virtualAddress) +
                                        vcenc_instance->stream.byteCnt;
        vcenc_instance->stream.stream_bus = new_params->output_buffer_mem[0].busAddress;
        vcenc_instance->stream.size = new_params->output_buffer_mem[0].size;

        pEncIn->pOutBuf[0] = new_params->output_buffer_mem[0].virtualAddress;
        pEncIn->busOutBuf[0] = new_params->output_buffer_mem[0].busAddress;
        pEncIn->outBufSize[0] = new_params->output_buffer_mem[0].size;
        for (i = 0; i < MAX_STRM_BUF_NUM; i++) {
            vcenc_instance->streamBufs[coreIdx].buf[i] = (u8 *)pEncIn->pOutBuf[i];
            vcenc_instance->streamBufs[coreIdx].bufLen[i] = pEncIn->outBufSize[i];
        }
        slice_callback->streamBufs =
            vcenc_instance
                ->streamBufs[(vcenc_instance->jobCnt + 1) % vcenc_instance->parallelCoreNum];

        regs_for2nd_encode->outputStrmBase[0] =
            (ptr_t)vcenc_instance->stream.stream_bus + vcenc_instance->stream.byteCnt;
        regs_for2nd_encode->outputStrmSize[0] =
            vcenc_instance->stream.size - vcenc_instance->stream.byteCnt;
    }

    asic->regs = *regs_for2nd_encode;
}
