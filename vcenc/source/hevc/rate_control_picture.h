
#ifndef RATE_CONTROL_PICTURE_H
#define RATE_CONTROL_PICTURE_H
#include "base_type.h"
#include "sw_picture.h"
#include "enccommon.h"
#include "hevcSei.h"
#ifdef VSB_TEMP_TEST
#include "video_statistic.h"
#endif
#ifdef __cplusplus
extern "C" {
#endif
#define zec761a150a
#define zea077a043d
enum { VCENCRC_OVERFLOW = -(0x1b0 + 7660 - 0x1f9b) };
#define RC_CBR_HRD (0x171 + 9153 - 0x2531)
#define zb4f7b36dcd (0x17ca + 2633 - 0x220c)
#define z9ae471aa41 (0x774 + 1240 - 0xc42)
#define zdb70b37658 (0x145 + 5084 - 0x1517)
#define INTRA_QPDELTA_DEFAULT (-(0x175b + 2325 - 0x206d))
#ifndef CTBRC_STRENGTH
#define QP_FRACTIONAL_BITS (0x154c + 1926 - 0x1cd2)
#else
#define QP_FRACTIONAL_BITS (0x8cc + 565 - 0xaf9)
#endif
#define QP_DEFAULT (0x127f + 3405 - 0x1fb2)
#define TOL_CTB_RC_FIX_POINT (0x914 + 5883 - 0x2008)
#define z2caa41c7fe (0x4e3 + 1203 - 0x95a)
#define LEAST_MONITOR_FRAME (0x1b14 + 2530 - 0x24f3)
#define I32_MAX ((i32)2147483647)
#define CLIP_I32_ADD(a, b) ((a) > (I32_MAX - (b)) ? I32_MAX : (a) + (b))
#define IS_CTBRC(x) ((x) & (0x62a + 2162 - 0xe99))
#define IS_CTBRC_FOR_QUALITY(x) ((x) & (0x161c + 200 - 0x16e3))
#define IS_CTBRC_FOR_BITRATE(x) ((x) & (0x461 + 5382 - 0x1965))
#define CLR_CTBRC_FOR_BITRATE(x)                                                                   \
    {                                                                                              \
        (x) &= (~(0x56b + 1092 - 0x9ad));                                                          \
    }
#define CTB_RC_QP_STEP_FIXP (0x393 + 957 - 0x740)
#define CTB_RC_ROW_FACTOR_FIXP (0x980 + 7398 - 0x2656)
enum {
    SIMPLE_SCENE,
    NORMAL_SCENE,
    COMPLEX_SCENE,
};
typedef struct {
    i32 frame[(0x21a + 3455 - 0xf21)];
    i32 length;
    i32 count;
    i32 zff13b54c4f;
    i32 z57f8526067;
    i32 zb2975ef616;
} zcf4bacd786;
typedef struct {
    u32 intraCu8Num[(0x142 + 220 - 0x1a6)];
    u32 skipCu8Num[(0x41a + 193 - 0x463)];
    u32 PBFrame4NRdCost[(0x6cf + 1889 - 0xdb8)];
    i32 length;
    i32 count;
    i32 zff13b54c4f;
    u32 zc09ddd71e6;
    u32 z59fac279de;
    u32 zf374f6a90b;
} z5a5950b51a;
typedef struct {
    i64 a1;
    i64 zca076e6f67;
    i32 z8ad09e4259;
    i32 zcb89df56bf[zdb70b37658 + (0x683 + 1372 - 0xbde)];
    i32 zdfcc1a3d2b[zdb70b37658 + (0x2ab + 7186 - 0x1ebc)];
    i32 zff13b54c4f;
    i32 len;
    i32 z8bdaa35e64;
    i32 z9f6f1ccdd6;
    i32 z8043ecc287;
    i32 z922e2b332f;
    i32 z47856f228a;
} za506561fab;
typedef struct {
    i32 ze60c2c5fdb[zb4f7b36dcd];
    i32 z0b244d785d[zb4f7b36dcd];
    i32 zf596d61938[z9ae471aa41];
    i32 z6b3312b96b[z9ae471aa41];
    i32 z31c3ec84f4;
    i32 zf8a3f36c10;
} z6783136e61;
typedef struct {
    i32 bufferSize;
    i32 z8bbd1e30ab;
    i32 ze8bd0d9c56;
    i32 maxBitRate;
    i32 bufferRate;
    i32 bitRate;
    i32 bitPerPic;
    i32 z1f9e750b2c;
    i32 timeScale;
    i32 unitsInTic;
    i32 zbe9fd58c6a;
    i32 realBitCnt;
    i32 z87a5b08757;
    i32 z9b20c7aeb4;
    i32 zd5c2c0ded9;
    i32 z2bdef80297;
    i32 bucketFullness;
    i32 bucketLevel;
    i32 zc0a193821d;
    i32 z11b549eb01;
    i32 z0d777460a4;
} rcVirtualBuffer_s;
typedef struct {
    i32 x0;
    i32 x1;
    i32 xMin;
    i32 started;
    i32 preFrameMad;
    ptr_t ctbMemPreAddr;
    u32 *ctbMemPreVirtualAddr;
} ctbRcModel_s;
typedef struct {
    ctbRcModel_s models[(0xd19 + 5260 - 0x21a2)];
    i32 qpSumForRc;
    i32 qpStep;
    i32 rowFactor;
    ptr_t ctbMemCurAddr;
    u32 *ctbMemCurVirtualAddr;
} ctbRateControl_s;
typedef struct {
    double z712cfca15b;
    double z497bd1f85d;
    double count;
    double z01bc086aef;
    double offset;
    double z861e0d65a9;
    double z3af6f68c9f;
    double z1b1f42bca4;
    double zca8d2a0001;
    double z255fad4eff;
    double z80c9bc5d34;
    double z38e5d4431c;
    i32 z2fd9172e8b;
    i32 z9b4d1c2963;
    i32 z922e2b332f;
    i32 z47856f228a;
} zba7eff2a5d;
typedef struct {
    true_e picRc;
    u32 ctbRc;
    true_e picSkip;
    true_e fillerData;
    true_e hrd;
    true_e vbr;
    i32 zeb1320bacc;
    i32 rcMode;
    u32 zab659326f4;
    i32 picArea;
    i32 ctbPerPic;
    i32 ctbRows;
    i32 ctbCols;
    i32 ctbSize;
    i32 zea3862b025;
    i32 nonZeroCnt;
    i32 z199dc1d4e8;
    i32 qpSum;
    i32 z1217f7c48b;
    float zecc8b26b90;
    u32 sliceTypeCur;
    u32 sliceTypePrev;
    true_e frameCoded;
    i32 fixedQp;
    i32 qpHdr;
    i32 qpMin;
    i32 qpMax;
    i32 qpMinI;
    i32 qpMaxI;
    i32 qpMinPB;
    i32 qpMaxPB;
    i32 qpHdrPrev;
    i32 qpLastCoded;
    i32 qpTarget;
    i32 z8fd7bcaec1;
    u32 zc97e82b7f1;
    i32 outRateNum;
    i32 outRateDenom;
    i32 zcbf5f92f04;
    i32 zdeb37127ce;
    i32 z29350a2390;
    z6783136e61 z1e137903f2;
    rcVirtualBuffer_s virtualBuffer;
    sei_s sei;
    i32 z792101456d, z6e3dcebb58;
    za506561fab za8b95563b2;
    i32 targetPicSize;
    i32 z3eab678f73;
    i32 zc7d8469149;
    i32 zc7008a3cc6;
    i32 z8e4b0bbb41;
#if VBR_RC
    i32 zf066713d8c;
    i32 zd99b97e997;
    i32 zbd6d50ffdd;
    i32 zc9ed7a8697;
    i32 z5f310a70ab;
#endif
    i32 minPicSizeI;
    i32 maxPicSizeI;
    i32 minPicSizeP;
    i32 maxPicSizeP;
    i32 minPicSizeB;
    i32 maxPicSizeB;
    i32 ze5fe0d2d6b;
    i32 tolMovingBitRate;
    float f_tolMovingBitRate;
    i32 monitorFrames;
    float tolCtbRcInter;
    float tolCtbRcIntra;
    float qpFactor;
    i32 ze173381e96;
    i32 ze41cb9116e;
    i32 za25e347240;
    i32 zf0e7d7a43a;
    i32 ze84c721aa9;
    i32 zabbbd97b5e;
    u32 z3c9e80fceb;
    u32 z1c7bacc73e;
    i32 bitrateWindow;
    i32 zf9d3566790;
    i32 z3c5c469fd0;
    i32 z29e41b4870;
    i32 intraQpDelta;
    i32 longTermQpDelta;
    i32 frameQpDelta;
    u32 fixedIntraQp;
    i32 zdfb346795b;
    i32 hierarchical_sse[(0x1e08 + 2318 - 0x270e)][(0x13dd + 4128 - 0x23f5)];
    i32 smooth_psnr_in_gop;
    i32 hierarchical_bit_allocation_GOP_size;
    i32 z45813bc4d2;
    i32 encoded_frame_number;
    u32 gopPoc;
    u32 picSkipAllowed;
    i32 minPicSize;
    i32 maxPicSize;
    i32 z0a1ab5b842;
    u32 ctbRcQpDeltaReverse;
    u32 ctbRcBitsMin;
    u32 ctbRcBitsMax;
    u32 ctbRctotalLcuBit;
    u32 bitsRatio;
    u32 ctbRcThrdMin;
    u32 ctbRcThrdMax;
    i32 z2aab5d849e;
    u32 rcQpDeltaRange;
    u32 rcBaseMBComplexity;
    i32 picQpDeltaMin;
    i32 picQpDeltaMax;
    i32 z9942fdcdc6;
    i32 inputSceneChange;
    zcf4bacd786 zafb762023b;
    z5a5950b51a z5bd36220d1;
    u32 rcPicComplexity;
    float complexity;
    i32 z257688f63c;
    i32 z44de60b69c;
    i32 zc3e0c52022;
    i32 z6f73eb8ac3;
    i32 zf062627859;
    u32 intraCu8Num;
    u32 skipCu8Num;
    u32 PBFrame4NRdCost;
    double reciprocalOfNumBlk8;
    u32 codingType;
    i32 i32MaxPicSize;
    u32 z52fad3c87c;
    i32 z75b9ee3750;
    i32 z2b59435d98;
    u32 z93e1c9ca47;
    u32 u32StaticSceneIbitPercent;
    ctbRateControl_s ctbRateCtrl;
    i32 crf;
    double z247948ead7;
    double pbOffset;
    double ipOffset;
    u64 z2b0accbf8b;
    double z37c52f3b35;
    int z02f70a8bf0;
    i32 pass;
    double pass1CurCost;
    double pass1GopCost[(0x11e3 + 397 - 0x136c)];
    double pass1AvgCost[(0xed2 + 5934 - 0x25fc)];
    i32 pass1GopFrameNum[(0x13ba + 797 - 0x16d3)];
    i32 pass1FrameNum[(0x1412 + 2582 - 0x1e24)];
    zba7eff2a5d z3a2cd96342[(0x1a96 + 161 - 0x1b33)];
    i32 predId;
    true_e cbr_flag;
    u8 IFrameQpVisualTuning;
    u8 pass1EstCostAll;
    u8 visualLmdTuning;
    u8 z11fe80aba6;
    u8 initialQpTuning;
} vcencRateControl_s;
#ifndef RATE_CONTROL_BUILD_SUPPORT
#define VCEncInitRc za96ee99723
#define VCEncBeforePicRc za87ad661eb
#define VCEncAfterPicRc(zca7520bb04, nonZeroCnt, ze3947b4c8e, qpSum, z1217f7c48b)                  \
    ((0x20ef + 341 - 0x2244))
#define z580c5b15ed(zca7520bb04, z1c7bacc73e) ((0x12f1 + 920 - 0x1689))
#define rcCalculate(a, b, c)                                                                       \
    ((c) == (0x9f1 + 44 - 0xa1d) ? (0x2 + 5202 - 0x1454) : ((a) * (b) / (c)))
#else
bool_e VCEncInitRc(vcencRateControl_s *zca7520bb04, u32 zcb3d5e8dbc);
void VCEncBeforePicRc(vcencRateControl_s *zca7520bb04, u32 zeabf8072f6, u32 z9acc17595d,
                      bool z2f7fa6af59, struct sw_picture *);
i32 VCEncAfterPicRc(vcencRateControl_s *zca7520bb04, u32 nonZeroCnt, u32 ze3947b4c8e, u32 qpSum,
                    u32 z1217f7c48b);
u32 z580c5b15ed(vcencRateControl_s *zca7520bb04, u32 z1c7bacc73e);
i32 rcCalculate(i32 a, i32 b, i32 c);
#endif
#ifdef __cplusplus
}
#endif
#endif
