/*------------------------------------------------------------------------------
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                                                                            --
--                            Verisilicon.                                    --
--                                                                            --
--      In the event of publication, the following notice is applicable:      --
--                                                                            --
--                   (C) COPYRIGHT 2014 VERISILICON                           --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
------------------------------------------------------------------------------*/

#include <math.h>
#include <stdarg.h>
#include "vsi_string.h"
#include "osal.h"

#include "base_type.h"
//#include "version.h"
#include "error.h"

#include "HevcTestBench.h"
#include "test_bench_utils.h"
#include "get_option.h"
#include "enccfg.h"
#define MAXARGS 128
#define CMDLENMAX 256
#define OFFSET(cml) offsetof(commandLine_s, cml)

static struct option options[] = {
    {"help", 'H', 2, 0, 4, {0}},
    {"firstPic", 'a', 1, I32_NUM_TYPE, 1, {OFFSET(firstPic)}},
    {"lastPic", 'b', 1, I32_NUM_TYPE, 1, {OFFSET(lastPic)}},
    {"width", 'x', 1, I32_NUM_TYPE, 1, {OFFSET(width)}},
    {"height", 'y', 1, I32_NUM_TYPE, 1, {OFFSET(height)}},
    {"lumWidthSrc", 'w', 1, I32_NUM_TYPE, 1, {OFFSET(lumWidthSrc)}},
    {"lumHeightSrc", 'h', 1, I32_NUM_TYPE, 1, {OFFSET(lumHeightSrc)}},
    {"horOffsetSrc", 'X', 1, I32_NUM_TYPE, 1, {OFFSET(horOffsetSrc)}},
    {"verOffsetSrc", 'Y', 1, I32_NUM_TYPE, 1, {OFFSET(verOffsetSrc)}},
    {"inputFormat", 'l', 1, I32_NUM_TYPE, 1, {OFFSET(inputFormat)}}, /* Input image format */
    {"colorConversion",
     'O',
     1,
     I32_NUM_TYPE,
     1,
     {OFFSET(colorConversion)}},                               /* RGB to YCbCr conversion type */
    {"rotation", 'r', 1, I32_NUM_TYPE, 1, {OFFSET(rotation)}}, /* Input image rotation */
    {"outputRateNumer", 'f', 1, I32_NUM_TYPE, 1, {OFFSET(outputRateNumer)}},
    {"outputRateDenom", 'F', 1, I32_NUM_TYPE, 1, {OFFSET(outputRateDenom)}},
    {"inputLineBufferMode", '0', 1, I32_NUM_TYPE, 1, {OFFSET(inputLineBufMode)}},
    {"inputLineBufferDepth", '0', 1, I32_NUM_TYPE, 1, {OFFSET(inputLineBufDepth)}},
    {"inputLineBufferAmountPerLoopback", '0', 1, I32_NUM_TYPE, 1, {OFFSET(amountPerLoopBack)}},
    {"inputRateNumer", 'j', 1, I32_NUM_TYPE, 1, {OFFSET(inputRateNumer)}},
    {"inputRateDenom", 'J', 1, I32_NUM_TYPE, 1, {OFFSET(inputRateDenom)}},
    {"inputFileList", '0', 1, STR_TYPE, 1, {OFFSET(inputFileList)}},
    /*stride*/
    {"inputAlignmentExp", '0', 1, U32_NUM_TYPE, 1, {OFFSET(exp_of_input_alignment)}},
    {"refAlignmentExp", '0', 1, U32_NUM_TYPE, 1, {OFFSET(exp_of_ref_alignment)}},
    {"refChromaAlignmentExp", '0', 1, U32_NUM_TYPE, 1, {OFFSET(exp_of_ref_ch_alignment)}},
    {"aqInfoAlignmentExp", '0', 1, U32_NUM_TYPE, 1, {OFFSET(exp_of_aqinfo_alignment)}},
    {"tileStreamAlignmentExp", '0', 1, U32_NUM_TYPE, 1, {OFFSET(exp_of_tile_stream_alignment)}},
    {"formatCustomizedType", '0', 1, I32_NUM_TYPE, 1, {OFFSET(formatCustomizedType)}},
    {"input", 'i', 1, STR_TYPE, 1, {OFFSET(input)}},
    {"output", 'o', 1, STR_TYPE, 1, {OFFSET(output)}},
    {"test_data_files", 'T', 1, STR_TYPE, 1, {OFFSET(test_data_files)}},
    {"cabacInitFlag", 'p', 1, I32_NUM_TYPE, 1, {OFFSET(cabacInitFlag)}},
    {"qp_size", 'Q', 1, TODO_TYPE, 1, {0}},
    {"qpMinI",
     '0',
     1,
     I32_NUM_TYPE,
     1,
     {OFFSET(qpMinI)}}, /* Minimum frame header qp for I picture */
    {"qpMaxI",
     '0',
     1,
     I32_NUM_TYPE,
     1,
     {OFFSET(qpMaxI)}}, /* Maximum frame header qp for I picture */
    {"qpMin",
     'n',
     1,
     I32_NUM_TYPE,
     1,
     {OFFSET(qpMin)}}, /* Minimum frame header qp for any picture */
    {"qpMax",
     'm',
     1,
     I32_NUM_TYPE,
     1,
     {OFFSET(qpMax)}}, /* Maximum frame header qp for any picture */
    {"qpHdr", 'q', 1, I32_NUM_TYPE, 1, {OFFSET(qpHdr)}},
    {"fillerData",
     '0',
     1,
     I32_NUM_TYPE,
     1,
     {OFFSET(fillerData)}}, /* if enable filler Data when HRD off */
    {"hrdConformance",
     'C',
     1,
     I32_NUM_TYPE,
     1,
     {OFFSET(hrdConformance)}},                              /* HDR Conformance (ANNEX C) */
    {"cpbSize", 'c', 1, I32_NUM_TYPE, 1, {OFFSET(cpbSize)}}, /* Coded Picture Buffer Size */
    {"vbr", '0', 1, I32_NUM_TYPE, 1, {OFFSET(vbr)}}, /* Variable Bit Rate Control by qpMin */
    {"intraQpDelta",
     'A',
     1,
     I32_NUM_TYPE,
     1,
     {OFFSET(intraQpDelta)}}, /* QP adjustment for intra frames */
    {"fixedIntraQp",
     'G',
     1,
     I32_NUM_TYPE,
     1,
     {OFFSET(fixedIntraQp)}}, /* Fixed QP for all intra frames */
    {"bFrameQpDelta",
     'V',
     1,
     I32_NUM_TYPE,
     1,
     {OFFSET(bFrameQpDelta)}}, /* QP adjustment for B frames */
    {"byteStream",
     'N',
     1,
     I32_NUM_TYPE,
     1,
     {OFFSET(byteStream)}}, /* Byte stream format (ANNEX B) */
    {"bitPerSecond", 'B', 1, I32_NUM_TYPE, 1, {OFFSET(bitPerSecond)}},
    {"picRc", 'U', 1, I32_NUM_TYPE, 1, {OFFSET(picRc)}},
    {"ctbRc", 'u', 1, I32_NUM_TYPE, 1, {OFFSET(ctbRc)}},
    {"picSkip", 's', 1, I32_NUM_TYPE, 1, {OFFSET(picSkip)}}, /* Frame skipping */
    {"profile",
     'P',
     1,
     I32_NUM_TYPE,
     1,
     {OFFSET(profile)}}, /* profile   (ANNEX A):support main and main still picture */
    {"level", 'L', 1, I32_NUM_TYPE, 1, {OFFSET(level)}},               /* Level * 30  (ANNEX A) */
    {"intraPicRate", 'R', 1, I32_NUM_TYPE, 1, {OFFSET(intraPicRate)}}, /* IDR interval */
    {"bpsAdjust", '1', 1, TODO_TYPE, 1, {0}}, /* Setting bitrate on the fly */
    {"bitrateWindow",
     'g',
     1,
     I32_NUM_TYPE,
     1,
     {OFFSET(bitrateWindow)}}, /* bitrate window of pictures length */
    {"disableDeblocking", 'D', 1, I32_NUM_TYPE, 1, {OFFSET(disableDeblocking)}},
    {"tc_Offset", 'W', 1, I32_NUM_TYPE, 1, {OFFSET(tc_Offset)}},
    {"beta_Offset", 'E', 1, I32_NUM_TYPE, 1, {OFFSET(beta_Offset)}},
    {"sliceSize", 'e', 1, I32_NUM_TYPE, 1, {OFFSET(sliceSize)}},
    {"chromaQpOffset",
     'I',
     1,
     I32_NUM_TYPE,
     1,
     {OFFSET(chromaQpOffset)}},                                  /* Chroma qp index offset */
    {"enableSao", 'M', 1, I32_NUM_TYPE, 1, {OFFSET(enableSao)}}, /* Enable or disable SAO */
    {"videoRange", 'k', 1, I32_NUM_TYPE, 1, {OFFSET(videoRange)}},
    {"sei", 'S', 1, I32_NUM_TYPE, 1, {OFFSET(sei)}}, /* SEI messages */
    {"codecFormat",
     '0',
     1,
     ENUM_STR_TYPE,
     1,
     {OFFSET(codecFormat)}}, /* select videoFormat: HEVC/H264/AV1 */
    {"enableCabac",
     'K',
     1,
     I32_NUM_TYPE,
     1,
     {OFFSET(enableCabac)}}, /* H.264 entropy coding mode, 0 for CAVLC, 1 for CABAC */
    {"userData", 'z', 1, STR_TYPE, 1, {OFFSET(userData)}},       /* SEI User data file */
    {"videoStab", 'Z', 1, I32_NUM_TYPE, 1, {OFFSET(videoStab)}}, /* video stabilization */

    /* Only long option can be used for all the following parameters because
   * we have no more letters to use. All shortOpt=0 will be identified by
   * long option. */
    {"cir", '0', 1, I32_NUM_TYPE, 2, {OFFSET(cirStart), OFFSET(cirInterval)}},
    {"cpbMaxRate", '0', 1, I32_NUM_TYPE, 1, {OFFSET(cpbMaxRate)}}, /* max bitrate for CPB VBR/CBR */
    {"intraArea",
     '0',
     1,
     I32_NUM_TYPE,
     4,
     {OFFSET(intraAreaLeft), OFFSET(intraAreaTop), OFFSET(intraAreaRight),
      OFFSET(intraAreaBottom)}},
    {"ipcm1Area",
     '0',
     1,
     I32_NUM_TYPE,
     4,
     {OFFSET(ipcm1AreaLeft), OFFSET(ipcm1AreaTop), OFFSET(ipcm1AreaRight),
      OFFSET(ipcm1AreaBottom)}},
    {"ipcm2Area",
     '0',
     1,
     I32_NUM_TYPE,
     4,
     {OFFSET(ipcm2AreaLeft), OFFSET(ipcm2AreaTop), OFFSET(ipcm2AreaRight),
      OFFSET(ipcm2AreaBottom)}},
    {"ipcm3Area",
     '0',
     1,
     I32_NUM_TYPE,
     4,
     {OFFSET(ipcm3AreaLeft), OFFSET(ipcm3AreaTop), OFFSET(ipcm3AreaRight),
      OFFSET(ipcm3AreaBottom)}},
    {"ipcm4Area",
     '0',
     1,
     I32_NUM_TYPE,
     4,
     {OFFSET(ipcm4AreaLeft), OFFSET(ipcm4AreaTop), OFFSET(ipcm4AreaRight),
      OFFSET(ipcm4AreaBottom)}},
    {"ipcm5Area",
     '0',
     1,
     I32_NUM_TYPE,
     4,
     {OFFSET(ipcm5AreaLeft), OFFSET(ipcm5AreaTop), OFFSET(ipcm5AreaRight),
      OFFSET(ipcm5AreaBottom)}},
    {"ipcm6Area",
     '0',
     1,
     I32_NUM_TYPE,
     4,
     {OFFSET(ipcm6AreaLeft), OFFSET(ipcm6AreaTop), OFFSET(ipcm6AreaRight),
      OFFSET(ipcm6AreaBottom)}},
    {"ipcm7Area",
     '0',
     1,
     I32_NUM_TYPE,
     4,
     {OFFSET(ipcm7AreaLeft), OFFSET(ipcm7AreaTop), OFFSET(ipcm7AreaRight),
      OFFSET(ipcm7AreaBottom)}},
    {"ipcm8Area",
     '0',
     1,
     I32_NUM_TYPE,
     4,
     {OFFSET(ipcm8AreaLeft), OFFSET(ipcm8AreaTop), OFFSET(ipcm8AreaRight),
      OFFSET(ipcm8AreaBottom)}},
    {"roi1Area",
     '0',
     1,
     I32_NUM_TYPE,
     4,
     {OFFSET(roi1AreaLeft), OFFSET(roi1AreaTop), OFFSET(roi1AreaRight), OFFSET(roi1AreaBottom)}},
    {"roi2Area",
     '0',
     1,
     I32_NUM_TYPE,
     4,
     {OFFSET(roi2AreaLeft), OFFSET(roi2AreaTop), OFFSET(roi2AreaRight), OFFSET(roi2AreaBottom)}},
    {"roi1DeltaQp", '0', 1, I32_NUM_TYPE, 1, {OFFSET(roi1DeltaQp)}},
    {"roi2DeltaQp", '0', 1, I32_NUM_TYPE, 1, {OFFSET(roi2DeltaQp)}},
    {"roi1Qp", '0', 1, I32_NUM_TYPE, 1, {OFFSET(roi1Qp)}},
    {"roi2Qp", '0', 1, I32_NUM_TYPE, 1, {OFFSET(roi2Qp)}},
    {"roi3Area",
     '0',
     1,
     I32_NUM_TYPE,
     4,
     {OFFSET(roi3AreaLeft), OFFSET(roi3AreaTop), OFFSET(roi3AreaRight), OFFSET(roi3AreaBottom)}},
    {"roi3DeltaQp", '0', 1, I32_NUM_TYPE, 1, {OFFSET(roi3DeltaQp)}},
    {"roi3Qp", '0', 1, I32_NUM_TYPE, 1, {OFFSET(roi3Qp)}},
    {"roi4Area",
     '0',
     1,
     I32_NUM_TYPE,
     4,
     {OFFSET(roi4AreaLeft), OFFSET(roi4AreaTop), OFFSET(roi4AreaRight), OFFSET(roi4AreaBottom)}},
    {"roi4DeltaQp", '0', 1, I32_NUM_TYPE, 1, {OFFSET(roi4DeltaQp)}},
    {"roi4Qp", '0', 1, I32_NUM_TYPE, 1, {OFFSET(roi4Qp)}},
    {"roi5Area",
     '0',
     1,
     I32_NUM_TYPE,
     4,
     {OFFSET(roi5AreaLeft), OFFSET(roi5AreaTop), OFFSET(roi5AreaRight), OFFSET(roi5AreaBottom)}},
    {"roi5DeltaQp", '0', 1, I32_NUM_TYPE, 1, {OFFSET(roi5DeltaQp)}},
    {"roi5Qp", '0', 1, I32_NUM_TYPE, 1, {OFFSET(roi5Qp)}},
    {"roi6Area",
     '0',
     1,
     I32_NUM_TYPE,
     4,
     {OFFSET(roi6AreaLeft), OFFSET(roi6AreaTop), OFFSET(roi6AreaRight), OFFSET(roi6AreaBottom)}},
    {"roi6DeltaQp", '0', 1, I32_NUM_TYPE, 1, {OFFSET(roi6DeltaQp)}},
    {"roi6Qp", '0', 1, I32_NUM_TYPE, 1, {OFFSET(roi6Qp)}},
    {"roi7Area",
     '0',
     1,
     I32_NUM_TYPE,
     4,
     {OFFSET(roi7AreaLeft), OFFSET(roi7AreaTop), OFFSET(roi7AreaRight), OFFSET(roi7AreaBottom)}},
    {"roi7DeltaQp", '0', 1, I32_NUM_TYPE, 1, {OFFSET(roi7DeltaQp)}},
    {"roi7Qp", '0', 1, I32_NUM_TYPE, 1, {OFFSET(roi7Qp)}},
    {"roi8Area",
     '0',
     1,
     I32_NUM_TYPE,
     4,
     {OFFSET(roi8AreaLeft), OFFSET(roi8AreaTop), OFFSET(roi8AreaRight), OFFSET(roi8AreaBottom)}},
    {"roi8DeltaQp", '0', 1, I32_NUM_TYPE, 1, {OFFSET(roi8DeltaQp)}},
    {"roi8Qp", '0', 1, I32_NUM_TYPE, 1, {OFFSET(roi8Qp)}},

    {"layerInRefIdc", '0', 1, U32_NUM_TYPE, 1, {OFFSET(layerInRefIdc)}}, /*H264 2bit nal_ref_idc*/

    {"ipcmFilterDisable", '0', 1, I32_NUM_TYPE, 1, {OFFSET(pcm_loop_filter_disabled_flag)}},

    {"roiMapDeltaQpBlockUnit", '0', 1, U32_NUM_TYPE, 1, {OFFSET(roiMapDeltaQpBlockUnit)}},
    {"roiMapDeltaQpEnable", '0', 1, U32_NUM_TYPE, 1, {OFFSET(roiMapDeltaQpEnable)}},
    {"ipcmMapEnable", '0', 1, I32_NUM_TYPE, 1, {OFFSET(ipcmMapEnable)}},

    {"outBufSizeMax", '0', 1, I32_NUM_TYPE, 1, {OFFSET(outBufSizeMax)}},

    {"constrainIntra", '0', 1, TODO_TYPE, 1, {0}},
    {"smoothingIntra", '0', 1, U32_NUM_TYPE, 1, {OFFSET(strong_intra_smoothing_enabled_flag)}},
    {"scaledWidth", '0', 1, I32_NUM_TYPE, 1, {OFFSET(scaledWidth)}},
    {"scaledHeight", '0', 1, I32_NUM_TYPE, 1, {OFFSET(scaledHeight)}},
    {"scaledOutputFormat", '0', 1, I32_NUM_TYPE, 1, {OFFSET(scaledOutputFormat)}},
    {"enableDeblockOverride", '0', 1, I32_NUM_TYPE, 1, {OFFSET(enableDeblockOverride)}},
    {"deblockOverride", '0', 1, I32_NUM_TYPE, 1, {OFFSET(deblockOverride)}},
    {"enableScalingList", '0', 1, I32_NUM_TYPE, 1, {OFFSET(enableScalingList)}},
    {"compressor", '0', 1, U32_NUM_TYPE, 1, {OFFSET(compressor)}},
    {"testId", '0', 1, I32_NUM_TYPE, 1, {OFFSET(testId)}}, /* TestID for generate vector. */
    {"gopSize", '0', 1, U32_NUM_TYPE, 1, {OFFSET(gopSize)}},
    {"gopConfig", '0', 1, STR_TYPE, 1, {OFFSET(gopCfg)}},
    {"gopLowdelay", '0', 1, U32_NUM_TYPE, 1, {OFFSET(gopLowdelay)}},
    {"LTR",
     '0',
     1,
     MIX_TYPE,
     4,
     {OFFSET(ltrInterval),       //u32
      OFFSET(longTermGapOffset), //u32
      OFFSET(longTermGap),       //u32
      OFFSET(longTermQpDelta)}}, //i32   special: u32 and i32 are in the same one.
    {"flexRefs", '0', 1, STR_TYPE, 1, {OFFSET(flexRefs)}}, /* flexible reference list */
    {"numRefP",
     '0',
     1,
     U32_NUM_TYPE,
     1,
     {OFFSET(numRefP)}}, /* number of reference frames for P frame */
    {"interlacedFrame", '0', 1, I32_NUM_TYPE, 1, {OFFSET(interlacedFrame)}},
    {"fieldOrder", '0', 1, I32_NUM_TYPE, 1, {OFFSET(fieldOrder)}},
    {"outReconFrame", '0', 1, I32_NUM_TYPE, 1, {OFFSET(outReconFrame)}},
    {"tier", '0', 1, I32_NUM_TYPE, 1, {OFFSET(tier)}},

    {"gdrDuration", '0', 1, I32_NUM_TYPE, 1, {OFFSET(gdrDuration)}},
    {"bitVarRangeI", '0', 1, I32_NUM_TYPE, 1, {OFFSET(bitVarRangeI)}},
    {"bitVarRangeP", '0', 1, I32_NUM_TYPE, 1, {OFFSET(bitVarRangeP)}},
    {"bitVarRangeB", '0', 1, I32_NUM_TYPE, 1, {OFFSET(bitVarRangeB)}},
    {"smoothPsnrInGOP", '0', 1, I32_NUM_TYPE, 1, {OFFSET(smoothPsnrInGOP)}},
    {"staticSceneIbitPercent", '0', 1, U32_NUM_TYPE, 1, {OFFSET(u32StaticSceneIbitPercent)}},
    {"tolCtbRcInter", '0', 1, FLOAT_TYPE, 1, {OFFSET(tolCtbRcInter)}},
    {"tolCtbRcIntra", '0', 1, FLOAT_TYPE, 1, {OFFSET(tolCtbRcIntra)}},
    {"ctbRowQpStep", '0', 1, I32_NUM_TYPE, 1, {OFFSET(ctbRcRowQpStep)}},

    {"tolMovingBitRate", '0', 1, I32_NUM_TYPE, 1, {OFFSET(tolMovingBitRate)}},
    {"monitorFrames", '0', 1, I32_NUM_TYPE, 1, {OFFSET(monitorFrames)}},
    {"roiMapDeltaQpFile", '0', 1, STR_TYPE, 1, {OFFSET(roiMapDeltaQpFile)}},
    {"roiMapDeltaQpBinFile", '0', 1, STR_TYPE, 1, {OFFSET(roiMapDeltaQpBinFile)}},
    {"ipcmMapFile", '0', 1, STR_TYPE, 1, {OFFSET(ipcmMapFile)}},
    {"roiMapInfoBinFile", '0', 1, STR_TYPE, 1, {OFFSET(roiMapInfoBinFile)}},
    {"roiMapConfigFile", '0', 1, STR_TYPE, 1, {OFFSET(roiMapConfigFile)}},
    {"RoimapCuCtrlInfoBinFile", '0', 1, STR_TYPE, 1, {OFFSET(RoimapCuCtrlInfoBinFile)}},
    {"RoimapCuCtrlIndexBinFile", '0', 1, STR_TYPE, 1, {OFFSET(RoimapCuCtrlIndexBinFile)}},
    {"RoiCuCtrlVer", '0', 1, U32_NUM_TYPE, 1, {OFFSET(RoiCuCtrlVer)}},
    {"RoiQpDeltaVer", '0', 1, U32_NUM_TYPE, 1, {OFFSET(RoiQpDeltaVer)}},

    {"reEncode", '0', 1, I32_NUM_TYPE, 1, {OFFSET(reEncode)}},

    {"rcMode", '0', 1, ENUM_STR_TYPE, 1, {OFFSET(rcMode)}},

    //WIENER_DENOISE
    {"noiseReductionEnable", '0', 1, I32_NUM_TYPE, 1, {OFFSET(noiseReductionEnable)}},
    {"noiseLow", '0', 1, I32_NUM_TYPE, 1, {OFFSET(noiseLow)}},
    {"noiseFirstFrameSigma", '0', 1, I32_NUM_TYPE, 1, {OFFSET(firstFrameSigma)}},
    /* multi stream configs */
    {"multimode",
     '0',
     1,
     I32_NUM_TYPE,
     1,
     {OFFSET(multimode)}}, // Multi-stream mode, 0--disable, 1--mult-thread, 2--multi-process
    {"streamcfg", '0', 1, STR_TYPE, 1, {OFFSET(streamcfg)}},               // extra stream config.
    {"bitDepthLuma", '0', 1, I32_NUM_TYPE, 1, {OFFSET(bitDepthLuma)}},     /* luma bit depth */
    {"bitDepthChroma", '0', 1, I32_NUM_TYPE, 1, {OFFSET(bitDepthChroma)}}, /* chroma bit depth */
    {"blockRCSize", '0', 1, I32_NUM_TYPE, 1, {OFFSET(blockRCSize)}},       /*block rc size */
    {"rcQpDeltaRange",
     '0',
     1,
     U32_NUM_TYPE,
     1,
     {OFFSET(rcQpDeltaRange)}}, /* ctb rc qp delta range */
    {"rcBaseMBComplexity",
     '0',
     1,
     U32_NUM_TYPE,
     1,
     {OFFSET(rcBaseMBComplexity)}}, /* ctb rc mb complexity base */
    {"picQpDeltaRange", '0', 1, I32_NUM_TYPE, 2, {OFFSET(picQpDeltaMin), OFFSET(picQpDeltaMax)}},
    {"enableOutputCuInfo", '0', 1, U32_NUM_TYPE, 1, {OFFSET(enableOutputCuInfo)}},
    {"enableOutputCtbBits",
     '0',
     1,
     U32_NUM_TYPE,
     1,
     {OFFSET(enableOutputCtbBits)}}, /* enable output ctb encoded bits */
    {"enableP010Ref", '0', 1, U32_NUM_TYPE, 1, {OFFSET(P010RefEnable)}},
    {"rdoLevel", '0', 1, U32_NUM_TYPE, 1, {OFFSET(rdoLevel)}},
    {"enableDynamicRdo", '0', 1, U32_NUM_TYPE, 1, {OFFSET(dynamicRdoEnable)}},
    {"dynamicCu16Bias", '0', 1, U32_NUM_TYPE, 1, {OFFSET(dynamicRdoCu16Bias)}},
    {"dynamicCu16Factor", '0', 1, U32_NUM_TYPE, 1, {OFFSET(dynamicRdoCu16Factor)}},
    {"dynamicCu32Bias", '0', 1, U32_NUM_TYPE, 1, {OFFSET(dynamicRdoCu32Bias)}},
    {"dynamicCu32Factor", '0', 1, U32_NUM_TYPE, 1, {OFFSET(dynamicRdoCu32Factor)}},
    {"enableRdoQuant", '0', 1, U32_NUM_TYPE, 1, {OFFSET(enableRdoQuant)}},
    {"gmvList1File", '0', 1, STR_TYPE, 1, {OFFSET(gmvFileName[1])}},
    {"gmvFile", '0', 1, STR_TYPE, 1, {OFFSET(gmvFileName[0])}},
    {"gmvList1", '0', 1, I16_NUM_TYPE, 2, {OFFSET(gmv[1][0]), OFFSET(gmv[1][1])}},
    {"gmv", '0', 1, I16_NUM_TYPE, 2, {OFFSET(gmv[0][0]), OFFSET(gmv[0][1])}},

    {"enableSmartMode", '0', 1, I32_NUM_TYPE, 1, {OFFSET(smartModeEnable)}}, /* enable smart mode */
    //TODO
    {"smartConfig", '0', 1, TODO_TYPE, 1, {0}},            /* smart config file */
    {"mirror", '0', 1, I32_NUM_TYPE, 1, {OFFSET(mirror)}}, /* mirror */
    {"hashtype",
     '0',
     1,
     U32_NUM_TYPE,
     1,
     {OFFSET(hashtype)}}, /* hash frame data, 0--disable, 1--crc32, 2--checksum */
    {"verbose", '0', 1, U32_NUM_TYPE, 1, {OFFSET(verbose)}}, /* print log verbose or not */
    /* constant chroma control */
    {"enableConstChroma",
     '0',
     1,
     I32_NUM_TYPE,
     1,
     {OFFSET(constChromaEn)}}, /* enable constant chroma setting or not */
    {"constCb", '0', 1, U32_NUM_TYPE, 1, {OFFSET(constCb)}}, /* constant pixel value for CB */
    {"constCr", '0', 1, U32_NUM_TYPE, 1, {OFFSET(constCr)}}, /* constant pixel value for CR */

    {"sceneChange", '0', 1, I32_NUM_TYPE, 20, {OFFSET(sceneChange[0]),  OFFSET(sceneChange[1]),
                                               OFFSET(sceneChange[2]),  OFFSET(sceneChange[3]),
                                               OFFSET(sceneChange[4]),  OFFSET(sceneChange[5]),
                                               OFFSET(sceneChange[6]),  OFFSET(sceneChange[7]),
                                               OFFSET(sceneChange[8]),  OFFSET(sceneChange[9]),
                                               OFFSET(sceneChange[10]), OFFSET(sceneChange[11]),
                                               OFFSET(sceneChange[12]), OFFSET(sceneChange[13]),
                                               OFFSET(sceneChange[14]), OFFSET(sceneChange[15]),
                                               OFFSET(sceneChange[16]), OFFSET(sceneChange[17]),
                                               OFFSET(sceneChange[18]), OFFSET(sceneChange[19])}},
    {"enableVuiTimingInfo",
     '0',
     1,
     U32_NUM_TYPE,
     1,
     {OFFSET(vui_timing_info_enable)}}, /* Write VUI timing info in SPS */
    {"lookaheadDepth", '0', 1, U32_NUM_TYPE, 1, {OFFSET(lookaheadDepth)}},
    {"halfDsInput", '0', 1, STR_TYPE, 1, {OFFSET(halfDsInput)}}, /* external 1/2 DS input yuv */
    {"inLoopDSRatio", '0', 1, U32_NUM_TYPE, 1, {OFFSET(inLoopDSRatio)}}, /* in-loop DS ratio */
    {"cuInfoVersion", '0', 1, I32_NUM_TYPE, 1, {OFFSET(cuInfoVersion)}},

    {"ssim", '0', 1, I32_NUM_TYPE, 1, {OFFSET(ssim)}},
    {"psnr", '0', 1, I32_NUM_TYPE, 1, {OFFSET(psnr)}},
    {"tile",
     '0',
     1,
     I32_NUM_TYPE,
     3,
     {OFFSET(num_tile_columns), OFFSET(num_tile_rows),
      OFFSET(loop_filter_across_tiles_enabled_flag)}},
    {"skipFramePOC", '0', 1, I32_NUM_TYPE, 1, {OFFSET(skip_frame_poc)}},
    {"t35",
     '0',
     1,
     U32_NUM_TYPE,
     2,
     {OFFSET(itu_t_t35_country_code), OFFSET(itu_t_t35_country_code_extension_byte)}},
    {"payloadT35File", '0', 1, STR_TYPE, 1, {OFFSET(p_t35_payload_file)}},
    {"writeOnceHDR10", '0', 1, U32_NUM_TYPE, 1, {OFFSET(write_once_HDR10)}},
    {"HDR10_display",
     '0',
     1,
     U32_NUM_TYPE,
     10,
     {OFFSET(hdr10_dx0), OFFSET(hdr10_dy0), OFFSET(hdr10_dx1), OFFSET(hdr10_dy1), OFFSET(hdr10_dx2),
      OFFSET(hdr10_dy2), OFFSET(hdr10_wx), OFFSET(hdr10_wy), OFFSET(hdr10_maxluma),
      OFFSET(hdr10_minluma)}},
    {"vuiColordescription",
     '0',
     1,
     U32_NUM_TYPE,
     3,
     {OFFSET(vuiColorPrimaries), OFFSET(vuiTransferCharacteristics),
      OFFSET(vuiMatrixCoefficients)}},
    {"HDR10_lightlevel", '0', 1, U32_NUM_TYPE, 2, {OFFSET(hdr10_maxlight), OFFSET(hdr10_avglight)}},
    {"vuiVideoFormat",
     '0',
     1,
     U32_NUM_TYPE,
     1,
     {OFFSET(
         vuiVideoFormat)}}, /* videoformat, 0--component, 1--PAL, 2--NTSC, 3--SECAM, 4--MAC, 5--UNDEF */
    {"vuiVideosignalPresent",
     '0',
     1,
     U32_NUM_TYPE,
     1,
     {OFFSET(
         vuiVideoSignalTypePresentFlag)}}, /* video signal type Present in vui, 0--NOT present, 1--present */
    {"vuiAspectRatio",
     '0',
     1,
     U32_NUM_TYPE,
     2,
     {OFFSET(vuiAspectRatioWidth), OFFSET(vuiAspectRatioHeight)}},
    {"RPSInSliceHeader", '0', 1, U32_NUM_TYPE, 1, {OFFSET(RpsInSliceHeader)}},
    {"POCConfig",
     '0',
     1,
     U32_NUM_TYPE,
     3,
     {OFFSET(picOrderCntType), OFFSET(log2MaxPicOrderCntLsb), OFFSET(log2MaxFrameNum)}},
    /* skip map */
    {"skipMapEnable", '0', 1, I32_NUM_TYPE, 1, {OFFSET(skipMapEnable)}},
    {"skipMapFile", '0', 1, STR_TYPE, 1, {OFFSET(skipMapFile)}},
    {"skipMapBlockUnit", '0', 1, I32_NUM_TYPE, 1, {OFFSET(skipMapBlockUnit)}},

    /* rdoq map */
    {"rdoqMapEnable", '0', 1, I32_NUM_TYPE, 1, {OFFSET(rdoqMapEnable)}},

    /* multi-core */
    {"parallelCoreNum", '0', 1, U32_NUM_TYPE, 1, {OFFSET(parallelCoreNum)}}, /* parallel core num */

    /* stream buffer chain */
    {"streamBufChain", '0', 1, U32_NUM_TYPE, 1, {OFFSET(streamBufChain)}},

    /* stream multi-segment */
    {"streamMultiSegmentMode", '0', 1, U32_NUM_TYPE, 1, {OFFSET(streamMultiSegmentMode)}},
    {"streamMultiSegmentAmount", '0', 1, U32_NUM_TYPE, 1, {OFFSET(streamMultiSegmentAmount)}},

    /*external sram*/
    {"extSramLumHeightBwd", '0', 1, U32_NUM_TYPE, 1, {OFFSET(extSramLumHeightBwd)}},
    {"extSramChrHeightBwd", '0', 1, U32_NUM_TYPE, 1, {OFFSET(extSramChrHeightBwd)}},
    {"extSramLumHeightFwd", '0', 1, U32_NUM_TYPE, 1, {OFFSET(extSramLumHeightFwd)}},
    {"extSramChrHeightFwd", '0', 1, U32_NUM_TYPE, 1, {OFFSET(extSramChrHeightFwd)}},

    /* AXI alignment */
    {"AXIAlignment", '0', 1, U64_NUM_TYPE, 1, {OFFSET(AXIAlignment)}},

    /* Irq Type Mask */
    {"irqTypeMask", '0', 1, U32_NUM_TYPE, 1, {OFFSET(irqTypeMask)}},

    /* Irq Type Cutree Mask */
    {"irqTypeCutreeMask", '0', 1, U32_NUM_TYPE, 1, {OFFSET(irqTypeCutreeMask)}},

    /* sliceNode */
    {"sliceNode", '0', 1, U32_NUM_TYPE, 1, {OFFSET(sliceNode)}},

    /*register dump*/
    {"dumpRegister", '0', 1, U32_NUM_TYPE, 1, {OFFSET(dumpRegister)}},
    /* raster scan output for recon on FPGA & HW */
    {"rasterscan", '0', 1, U32_NUM_TYPE, 1, {OFFSET(rasterscan)}},

    /*CRF constant*/
    {"crf", '0', 1, I32_NUM_TYPE, 1, {OFFSET(crf)}},

    {"ivf", '0', 1, U32_NUM_TYPE, 1, {OFFSET(ivf)}},
    {"codedChromaIdc", '0', 1, I32_NUM_TYPE, 1, {OFFSET(codedChromaIdc)}},

    /*ME vertical search range*/
    {"MEVertRange", '0', 1, U32_NUM_TYPE, 1, {OFFSET(MEVertRange)}},

    {"dec400TableInput", '0', 1, STR_TYPE, 1, {OFFSET(dec400CompTableinput)}},
    {"osdDec400TableInput", '0', 1, STR_TYPE, 1, {OFFSET(osdDec400CompTableInput)}},

    {"psyFactor", '0', 1, FLOAT_TYPE, 1, {OFFSET(psyFactor)}},

    /*Overlay*/
    {"overlayEnables", '0', 1, U32_NUM_TYPE, 1, {OFFSET(overlayEnables)}},
    {"olInput01", '0', 1, STR_TYPE, 1, {OFFSET(olInput[0])}},
    {"olFormat01", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olFormat[0])}},
    {"olAlpha01", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olAlpha[0])}},
    {"olWidth01", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olWidth[0])}},
    {"olHeight01", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olHeight[0])}},
    {"olXoffset01", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olXoffset[0])}},
    {"olYoffset01", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olYoffset[0])}},
    {"olYStride01", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olYStride[0])}},
    {"olUVStride01", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olUVStride[0])}},
    {"olCropWidth01", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olCropWidth[0])}},
    {"olCropHeight01", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olCropHeight[0])}},
    {"olCropXoffset01", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olCropXoffset[0])}},
    {"olCropYoffset01", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olCropYoffset[0])}},
    {"olBitmapY01", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olBitmapY[0])}},
    {"olBitmapU01", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olBitmapU[0])}},
    {"olBitmapV01", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olBitmapV[0])}},
    {"olSuperTile01", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olSuperTile[0])}},
    {"olScaleWidth01", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olScaleWidth[0])}},
    {"olScaleHeight01", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olScaleHeight[0])}},

    {"olInput02", '0', 1, STR_TYPE, 1, {OFFSET(olInput[1])}},
    {"olFormat02", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olFormat[1])}},
    {"olAlpha02", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olAlpha[1])}},
    {"olWidth02", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olWidth[1])}},
    {"olHeight02", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olHeight[1])}},
    {"olXoffset02", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olXoffset[1])}},
    {"olYoffset02", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olYoffset[1])}},
    {"olYStride02", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olYStride[1])}},
    {"olUVStride02", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olUVStride[1])}},
    {"olCropWidth02", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olCropWidth[1])}},
    {"olCropHeight02", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olCropHeight[1])}},
    {"olCropXoffset02", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olCropXoffset[1])}},
    {"olCropYoffset02", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olCropYoffset[1])}},
    {"olBitmapY02", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olBitmapY[1])}},
    {"olBitmapU02", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olBitmapU[1])}},
    {"olBitmapV02", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olBitmapV[1])}},

    {"olInput03", '0', 1, STR_TYPE, 1, {OFFSET(olInput[2])}},
    {"olFormat03", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olFormat[2])}},
    {"olAlpha03", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olAlpha[2])}},
    {"olWidth03", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olWidth[2])}},
    {"olHeight03", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olHeight[2])}},
    {"olXoffset03", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olXoffset[2])}},
    {"olYoffset03", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olYoffset[2])}},
    {"olYStride03", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olYStride[2])}},
    {"olUVStride03", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olUVStride[2])}},
    {"olCropWidth03", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olCropWidth[2])}},
    {"olCropHeight03", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olCropHeight[2])}},
    {"olCropXoffset03", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olCropXoffset[2])}},
    {"olCropYoffset03", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olCropYoffset[2])}},
    {"olBitmapY03", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olBitmapY[2])}},
    {"olBitmapU03", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olBitmapU[2])}},
    {"olBitmapV03", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olBitmapV[2])}},

    {"olInput04", '0', 1, STR_TYPE, 1, {OFFSET(olInput[3])}},
    {"olFormat04", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olFormat[3])}},
    {"olAlpha04", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olAlpha[3])}},
    {"olWidth04", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olWidth[3])}},
    {"olHeight04", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olHeight[3])}},
    {"olXoffset04", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olXoffset[3])}},
    {"olYoffset04", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olYoffset[3])}},
    {"olYStride04", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olYStride[3])}},
    {"olUVStride04", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olUVStride[3])}},
    {"olCropWidth04", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olCropWidth[3])}},
    {"olCropHeight04", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olCropHeight[3])}},
    {"olCropXoffset04", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olCropXoffset[3])}},
    {"olCropYoffset04", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olCropYoffset[3])}},
    {"olBitmapY04", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olBitmapY[3])}},
    {"olBitmapU04", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olBitmapU[3])}},
    {"olBitmapV04", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olBitmapV[3])}},

    {"olInput05", '0', 1, STR_TYPE, 1, {OFFSET(olInput[4])}},
    {"olFormat05", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olFormat[4])}},
    {"olAlpha05", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olAlpha[4])}},
    {"olWidth05", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olWidth[4])}},
    {"olHeight05", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olHeight[4])}},
    {"olXoffset05", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olXoffset[4])}},
    {"olYoffset05", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olYoffset[4])}},
    {"olYStride05", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olYStride[4])}},
    {"olUVStride05", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olUVStride[4])}},
    {"olCropWidth05", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olCropWidth[4])}},
    {"olCropHeight05", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olCropHeight[4])}},
    {"olCropXoffset05", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olCropXoffset[4])}},
    {"olCropYoffset05", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olCropYoffset[4])}},
    {"olBitmapY05", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olBitmapY[4])}},
    {"olBitmapU05", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olBitmapU[4])}},
    {"olBitmapV05", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olBitmapV[4])}},

    {"olInput06", '0', 1, STR_TYPE, 1, {OFFSET(olInput[5])}},
    {"olFormat06", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olFormat[5])}},
    {"olAlpha06", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olAlpha[5])}},
    {"olWidth06", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olWidth[5])}},
    {"olHeight06", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olHeight[5])}},
    {"olXoffset06", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olXoffset[5])}},
    {"olYoffset06", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olYoffset[5])}},
    {"olYStride06", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olYStride[5])}},
    {"olUVStride06", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olUVStride[5])}},
    {"olCropWidth06", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olCropWidth[5])}},
    {"olCropHeight06", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olCropHeight[5])}},
    {"olCropXoffset06", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olCropXoffset[5])}},
    {"olCropYoffset06", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olCropYoffset[5])}},
    {"olBitmapY06", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olBitmapY[5])}},
    {"olBitmapU06", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olBitmapU[5])}},
    {"olBitmapV06", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olBitmapV[5])}},

    {"olInput07", '0', 1, STR_TYPE, 1, {OFFSET(olInput[6])}},
    {"olFormat07", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olFormat[6])}},
    {"olAlpha07", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olAlpha[6])}},
    {"olWidth07", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olWidth[6])}},
    {"olHeight07", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olHeight[6])}},
    {"olXoffset07", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olXoffset[6])}},
    {"olYoffset07", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olYoffset[6])}},
    {"olYStride07", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olYStride[6])}},
    {"olUVStride07", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olUVStride[6])}},
    {"olCropWidth07", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olCropWidth[6])}},
    {"olCropHeight07", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olCropHeight[6])}},
    {"olCropXoffset07", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olCropXoffset[6])}},
    {"olCropYoffset07", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olCropYoffset[6])}},
    {"olBitmapY07", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olBitmapY[6])}},
    {"olBitmapU07", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olBitmapU[6])}},
    {"olBitmapV07", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olBitmapV[6])}},

    {"olInput08", '0', 1, STR_TYPE, 1, {OFFSET(olInput[7])}},
    {"olFormat08", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olFormat[7])}},
    {"olAlpha08", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olAlpha[7])}},
    {"olWidth08", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olWidth[7])}},
    {"olHeight08", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olHeight[7])}},
    {"olXoffset08", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olXoffset[7])}},
    {"olYoffset08", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olYoffset[7])}},
    {"olYStride08", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olYStride[7])}},
    {"olUVStride08", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olUVStride[7])}},
    {"olCropWidth08", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olCropWidth[7])}},
    {"olCropHeight08", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olCropHeight[7])}},
    {"olCropXoffset08", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olCropXoffset[7])}},
    {"olCropYoffset08", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olCropYoffset[7])}},
    {"olBitmapY08", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olBitmapY[7])}},
    {"olBitmapU08", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olBitmapU[7])}},
    {"olBitmapV08", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olBitmapV[7])}},

    {"olInput09", '0', 1, STR_TYPE, 1, {OFFSET(olInput[8])}},
    {"olFormat09", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olFormat[8])}},
    {"olAlpha09", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olAlpha[8])}},
    {"olWidth09", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olWidth[8])}},
    {"olHeight09", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olHeight[8])}},
    {"olXoffset09", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olXoffset[8])}},
    {"olYoffset09", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olYoffset[8])}},
    {"olYStride09", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olYStride[8])}},
    {"olUVStride09", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olUVStride[8])}},
    {"olCropWidth09", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olCropWidth[8])}},
    {"olCropHeight09", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olCropHeight[8])}},
    {"olCropXoffset09", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olCropXoffset[8])}},
    {"olCropYoffset09", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olCropYoffset[8])}},
    {"olBitmapY09", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olBitmapY[8])}},
    {"olBitmapU09", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olBitmapU[8])}},
    {"olBitmapV09", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olBitmapV[8])}},

    {"olInput10", '0', 1, STR_TYPE, 1, {OFFSET(olInput[9])}},
    {"olFormat10", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olFormat[9])}},
    {"olAlpha10", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olAlpha[9])}},
    {"olWidth10", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olWidth[9])}},
    {"olHeight10", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olHeight[9])}},
    {"olXoffset10", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olXoffset[9])}},
    {"olYoffset10", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olYoffset[9])}},
    {"olYStride10", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olYStride[9])}},
    {"olUVStride10", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olUVStride[9])}},
    {"olCropWidth10", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olCropWidth[9])}},
    {"olCropHeight10", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olCropHeight[9])}},
    {"olCropXoffset10", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olCropXoffset[9])}},
    {"olCropYoffset10", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olCropYoffset[9])}},
    {"olBitmapY10", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olBitmapY[9])}},
    {"olBitmapU10", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olBitmapU[9])}},
    {"olBitmapV10", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olBitmapV[9])}},

    {"olInput11", '0', 1, STR_TYPE, 1, {OFFSET(olInput[10])}},
    {"olFormat11", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olFormat[10])}},
    {"olAlpha11", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olAlpha[10])}},
    {"olWidth11", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olWidth[10])}},
    {"olHeight11", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olHeight[10])}},
    {"olXoffset11", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olXoffset[10])}},
    {"olYoffset11", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olYoffset[10])}},
    {"olYStride11", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olYStride[10])}},
    {"olUVStride11", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olUVStride[10])}},
    {"olCropWidth11", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olCropWidth[10])}},
    {"olCropHeight11", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olCropHeight[10])}},
    {"olCropXoffset11", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olCropXoffset[10])}},
    {"olCropYoffset11", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olCropYoffset[10])}},
    {"olBitmapY11", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olBitmapY[10])}},
    {"olBitmapU11", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olBitmapU[10])}},
    {"olBitmapV11", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olBitmapV[10])}},

    {"olInput12", '0', 1, STR_TYPE, 1, {OFFSET(olInput[11])}},
    {"olFormat12", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olFormat[11])}},
    {"olAlpha12", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olAlpha[11])}},
    {"olWidth12", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olWidth[11])}},
    {"olHeight12", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olHeight[11])}},
    {"olXoffset12", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olXoffset[11])}},
    {"olYoffset12", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olYoffset[11])}},
    {"olYStride12", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olYStride[11])}},
    {"olUVStride12", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olUVStride[11])}},
    {"olCropWidth12", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olCropWidth[11])}},
    {"olCropHeight12", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olCropHeight[11])}},
    {"olCropXoffset12", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olCropXoffset[11])}},
    {"olCropYoffset12", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olCropYoffset[11])}},
    {"olBitmapY12", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olBitmapY[11])}},
    {"olBitmapU12", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olBitmapU[11])}},
    {"olBitmapV12", '0', 1, I32_NUM_TYPE, 1, {OFFSET(olBitmapV[11])}},

    /* Mosaic */
    {"mosaicEnables", '0', 1, U32_NUM_TYPE, 1, {OFFSET(mosaicEnables)}},
    {"mosArea01",
     '0',
     1,
     U32_NUM_TYPE,
     4,
     {OFFSET(mosXoffset[0]), OFFSET(mosYoffset[0]), OFFSET(mosWidth[0]), OFFSET(mosHeight[0])}},
    {"mosArea02",
     '0',
     1,
     U32_NUM_TYPE,
     4,
     {OFFSET(mosXoffset[1]), OFFSET(mosYoffset[1]), OFFSET(mosWidth[1]), OFFSET(mosHeight[1])}},
    {"mosArea03",
     '0',
     1,
     U32_NUM_TYPE,
     4,
     {OFFSET(mosXoffset[2]), OFFSET(mosYoffset[2]), OFFSET(mosWidth[2]), OFFSET(mosHeight[2])}},
    {"mosArea04",
     '0',
     1,
     U32_NUM_TYPE,
     4,
     {OFFSET(mosXoffset[3]), OFFSET(mosYoffset[3]), OFFSET(mosWidth[3]), OFFSET(mosHeight[3])}},
    {"mosArea05",
     '0',
     1,
     U32_NUM_TYPE,
     4,
     {OFFSET(mosXoffset[4]), OFFSET(mosYoffset[4]), OFFSET(mosWidth[4]), OFFSET(mosHeight[4])}},
    {"mosArea06",
     '0',
     1,
     U32_NUM_TYPE,
     4,
     {OFFSET(mosXoffset[5]), OFFSET(mosYoffset[5]), OFFSET(mosWidth[5]), OFFSET(mosHeight[5])}},
    {"mosArea07",
     '0',
     1,
     U32_NUM_TYPE,
     4,
     {OFFSET(mosXoffset[6]), OFFSET(mosYoffset[6]), OFFSET(mosWidth[6]), OFFSET(mosHeight[6])}},
    {"mosArea08",
     '0',
     1,
     U32_NUM_TYPE,
     4,
     {OFFSET(mosXoffset[7]), OFFSET(mosYoffset[7]), OFFSET(mosWidth[7]), OFFSET(mosHeight[7])}},
    {"mosArea09",
     '0',
     1,
     U32_NUM_TYPE,
     4,
     {OFFSET(mosXoffset[8]), OFFSET(mosYoffset[8]), OFFSET(mosWidth[8]), OFFSET(mosHeight[8])}},
    {"mosArea10",
     '0',
     1,
     U32_NUM_TYPE,
     4,
     {OFFSET(mosXoffset[9]), OFFSET(mosYoffset[9]), OFFSET(mosWidth[9]), OFFSET(mosHeight[9])}},
    {"mosArea11",
     '0',
     1,
     U32_NUM_TYPE,
     4,
     {OFFSET(mosXoffset[10]), OFFSET(mosYoffset[10]), OFFSET(mosWidth[10]), OFFSET(mosHeight[10])}},
    {"mosArea12",
     '0',
     1,
     U32_NUM_TYPE,
     4,
     {OFFSET(mosXoffset[11]), OFFSET(mosYoffset[11]), OFFSET(mosWidth[11]), OFFSET(mosHeight[11])}},

    {"aq_mode", '0', 1, U32_NUM_TYPE, 1, {OFFSET(aq_mode)}},
    {"aq_strength", '0', 1, FLOAT_TYPE, 1, {OFFSET(aq_strength)}},

    {"tune", '0', 1, ENUM_STR_TYPE, 1, {OFFSET(tune)}},
    {"preset", '0', 1, U32_NUM_TYPE, 1, {OFFSET(preset)}},

    /*HW write recon to DDR or not if it's pure I-frame encoding*/
    {"writeReconToDDR", '0', 1, U32_NUM_TYPE, 1, {OFFSET(writeReconToDDR)}},

    /*AV1 tx type search*/
    {"TxTypeSearchEnable", '0', 1, U32_NUM_TYPE, 1, {OFFSET(TxTypeSearchEnable)}},
    {"av1InterFiltSwitch", '0', 1, U32_NUM_TYPE, 1, {OFFSET(av1InterFiltSwitch)}},

    {"sendAUD", '0', 1, U32_NUM_TYPE, 1, {OFFSET(sendAUD)}},

    /*external SEI*/
    {"extSEI", '0', 1, STR_TYPE, 1, {OFFSET(extSEI)}},

    {"useVcmd", '3', 1, I32_NUM_TYPE, 1, {OFFSET(useVcmd)}},
    {"useDec400", '3', 1, I32_NUM_TYPE, 1, {OFFSET(useDec400)}},
    {"useL2Cache", '3', 1, I32_NUM_TYPE, 1, {OFFSET(useL2Cache)}},
    {"useAXIFE", '3', 1, I32_NUM_TYPE, 1, {OFFSET(useAXIFE)}},

    /* Resend high level headers(VPS/SPS/PPS) */
    {"resendParamSet", '0', 1, U32_NUM_TYPE, 1, {OFFSET(resendParamSet)}},

    {"sramPowerdownDisable", '0', 1, U32_NUM_TYPE, 1, {OFFSET(sramPowerdownDisable)}},

    {"insertIDR", '0', 1, U32_NUM_TYPE, 1, {OFFSET(insertIDR)}},
    {"stopFrameNum", '0', 1, I32_NUM_TYPE, 1, {OFFSET(stopFrameNum)}},

    /* output rd log in profile.log regardless of tb.cfg */
    {"rdLog", '0', 1, U32_NUM_TYPE, 1, {OFFSET(bRdLog)}},

    /* print RC statistic Info in test bench */
    {"rcLog", '0', 1, U32_NUM_TYPE, 1, {OFFSET(bRcLog)}},

    {"replaceMvFile", '0', 1, STR_TYPE, 1, {OFFSET(replaceMvFile)}},

    /*AXI max burst length */
    {"burstMaxLength", '0', 1, U32_NUM_TYPE, 1, {OFFSET(burstMaxLength)}},

    /* TMVP enable */
    {"enableTMVP", '0', 1, U32_NUM_TYPE, 1, {OFFSET(enableTMVP)}},

    /* disable EOS */
    {"disableEOS", '0', 1, U32_NUM_TYPE, 1, {OFFSET(disableEOS)}},

    /* modify space of av1's tile_group_size, just for verified when using ffmpeg, NOT using when encoding */
    {"modifiedTileGroupSize", '0', 1, U32_NUM_TYPE, 1, {OFFSET(modifiedTileGroupSize)}},

    /* logmsg env parametet api which can be setting by command line or getting from env */
    {"logOutDir", '0', 1, U32_NUM_TYPE, 1, {OFFSET(logOutDir)}},
    {"logOutLevel", '0', 1, U32_NUM_TYPE, 1, {OFFSET(logOutLevel)}},
    {"logTraceMap", '0', 1, U32_NUM_TYPE, 1, {OFFSET(logTraceMap)}},
    {"logCheckMap", '0', 1, U32_NUM_TYPE, 1, {OFFSET(logCheckMap)}},

    {"refRingBufEnable", '0', 1, U32_NUM_TYPE, 1, {OFFSET(refRingBufEnable)}},
    {NULL, 0, 0, 0, 1, {0}} /* Format of last line */
};

static struct enum_str_option enum_str_options[] = {
    {"codecFormat", {"hevc", "h264", "av1", "vp9"}},
    {"tune", {"psnr", "ssim", "visual", "sharpness_visual", "vmaf"}},
    {"rcMode", {"cvbr", "cbr", "vbr", "abr", "crf", "cqp"}},
    {NULL, {NULL}}};

static i32 long_option(i32 argc, char **argv, struct option *option, struct parameter *parameter,
                       char **p);
static i32 short_option(i32 argc, char **argv, struct option *option, struct parameter *parameter,
                        char **p);
static i32 parse(i32 argc, char **argv, struct option *option, struct parameter *parameter,
                 char **p, u32 length);
static i32 get_next(i32 argc, char **argv, struct parameter *parameter, char **p);

static i8 cml_print(struct option *option, commandLine_s *cml, char *cml_all_log,
                    int cml_all_log_size);
static void logstr(char *cml_all_log, char *cml_log_buffer, int cml_all_log_size, const char *fmt,
                   ...);

/**
  get_option parses command line options. This function should be called
  with argc and argv values that are parameters of main(). The function
  will parse the next parameter from the command line. The function
  returns the next option character and stores the current place to
  structure argument. Structure option contain valid options
  and structure parameter contains parsed option.

  option options[] = {
    {"help",           'H', 0}, // No argument
    {"input",          'i', 1}, // Argument is compulsory
    {"output",         'o', 2}, // Argument is optional
    {NULL,              0,  0}  // Format of last line
  }

  Command line format can be
  --input filename
  --input=filename
  --inputfilename
  -i filename
  -i=filename
  -ifilename

  Input argc  Argument count as passed to main().
    argv  Argument values as passed to main().
    option  Valid options and argument requirements structure.
    parameter Option and argument return structure.

  \return  0 Option and argument are OK.
  \return  1 Unknown option.
  \return  2 a trigger is met
  \return -1 No more options.
  \return -2 Option match but argument is missing.
 */
i32 get_option(i32 argc, char **argv, struct option *option, struct parameter *parameter)
{
    char *p = NULL;
    i32 ret;

    parameter->argument = "?";
    parameter->short_opt = '?';
    parameter->enable = 0;

    if (get_next(argc, argv, parameter, &p)) {
        return -1; /* End of options */
    }

    /* Long option */
    ret = long_option(argc, argv, option, parameter, &p);
    if (ret != 1)
        return ret;

    /* Short option */
    ret = short_option(argc, argv, option, parameter, &p);
    if (ret != 1)
        return ret;

    /* This is unknown option but option anyway so argument must return */
    parameter->argument = p;

    return 1;
}

/**
 * check if the option is long_option
 *
 * \return  0 valid long option;
 * \return  1 not a valid long option or not found matched long option
 * \return -2 argument is invalid for a valid long option
 */
i32 long_option(i32 argc, char **argv, struct option *option, struct parameter *parameter, char **p)
{
    i32 j = 0;
    u32 index = 0xffffffff;
    u32 substr_len = 0, max_substr_len = 0;
    if (strncmp("--", *p, 2) != 0) {
        return 1;
    }

    while (option[j].long_opt != NULL) {
        if (strstr(*p + 2, option[j].long_opt)) {
            substr_len = strlen(option[j].long_opt);
            if (substr_len > max_substr_len) {
                index = j;
                max_substr_len = substr_len;
            }
        }
        j++;
    }
    if (index == 0xffffffff) //index no changing, no matching.
        return 1;
    max_substr_len += 2; /* Because option start -- */
    parameter->match_index = index;
    if (parse(argc, argv, &option[index], parameter, p, max_substr_len) != 0) {
        return -2;
    }
    return 0;
}

/**
 * check if the option is short_option (single charactor option)
 *
 * \return  0 valid short option;
 * \return  1 not a valid short option for wrong prefix or not found matched short option
 * \return  2 meet a trigger option;
 * \return -2 argument is invalid for a valid short option
 */
i32 short_option(i32 argc, char **argv, struct option *option, struct parameter *parameter,
                 char **p)
{
    i32 i = 0;
    char short_opt;

    if (strncmp("@", *p, 1) == 0) {
        if (parse(argc, argv, &option[i], parameter, p, 1) != 0) {
            return -2;
        }
        return 2;
    }

    if (strncmp("-", *p, 1) != 0) {
        return 1;
    }

    //strncpy(&short_opt, *p + 1, 1);
    short_opt = *(*p + 1);
    while (option[i].long_opt != NULL) {
        if (option[i].short_opt == short_opt) {
            goto match;
        }
        i++;
    }
    return 1;

match:
    parameter->match_index = i;
    if (parse(argc, argv, &option[i], parameter, p, 2) != 0) {
        return -2;
    }

    return 0;
}

/*------------------------------------------------------------------------------
  parse
------------------------------------------------------------------------------*/
i32 parse(i32 argc, char **argv, struct option *option, struct parameter *parameter, char **p,
          u32 length)
{
    char *arg;

    parameter->short_opt = option->short_opt;
    parameter->longOpt = option->long_opt;
    arg = *p + length;

    /* Argument and option are together */
    if (strlen(arg) != 0) {
        /* There should be no argument */
        if (option->enable == 0) {
            return -1;
        }
        /*--option=value;  and : --option=    value;*/
        /* Remove = */
        if (strncmp("=", arg, 1) == 0) {
            arg++;
            if (strcmp(arg, "\0") == 0) {
                if (get_next(argc, argv, parameter, &arg))
                    return -1;
            }
        }
        parameter->enable = 1;
        parameter->argument = arg;
        return 0;
    }
    /*--option   value; --option   =   value;*/
    /* Argument and option are separately */
    if (get_next(argc, argv, parameter, p)) {
        /* There is no more parameters */
        if (option->enable == 1) {
            return -1;
        }
        return 0;
    }
    /*--option   =   value*/
    if (strcmp("=", *p) == 0) {
        if (get_next(argc, argv, parameter, p))
            return -1;
    }
    /*--option   =value*/
    if (strncmp("=", *p, 1) == 0) {
        (*p)++;
    }

    /* Parameter is missing if next start with "-" but next time this
   * option is OK so we must fix parameter->cnt */
    if (strncmp("-", *p, 1) == 0) {
        parameter->cnt--;
        if (option->enable == 1) {
            return -1;
        }
        return 0;
    }

    /* There should be no argument */
    if (option->enable == 0) {
        return -1;
    }

    parameter->enable = 1;
    parameter->argument = *p;

    return 0;
}

/*------------------------------------------------------------------------------
  get_next
------------------------------------------------------------------------------*/
i32 get_next(i32 argc, char **argv, struct parameter *parameter, char **p)
{
    /* End of options */
    if ((parameter->cnt >= argc) || (parameter->cnt < 0)) {
        return -1;
    }
    *p = argv[parameter->cnt];
    parameter->cnt++;

    return 0;
}

/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
int ParseDelim(char *optArg, char delim)
{
    i32 i;

    for (i = 0; i < (i32)strlen(optArg); i++)
        if (optArg[i] == delim) {
            optArg[i] = 0;
            return i;
        }

    return -1;
}

int HasDelim(char *optArg, char delim)
{
    i32 i;

    for (i = 0; i < (i32)strlen(optArg); i++)
        if (optArg[i] == delim) {
            return 1;
        }

    return 0;
}

/*------------------------------------------------------------------------------
  help
------------------------------------------------------------------------------*/
void help(char *test_app)
{
    char helptext[] = {
#include "../vcenc_help.dat"
        , '\0'};

    //fprintf(stdout, "Version: %s\n", git_version);
    fprintf(stdout, "Usage:  %s [options] -i inputfile\n\n", test_app);
    fprintf(stdout, "Default parameters are marked inside []. More detailed descriptions of\n"
                    "C-Model command line parameters can be found from C-Model Command Line "
                    "API Manual.\n\n");

    fprintf(stdout, "  -H --help                        This help\n\n");

    fprintf(stdout, "%s", helptext);

    exit(OK);
}

/* developing options */
void help_dev()
{
    fprintf(stdout, "===================\n");
    fprintf(stdout, "Developing Options:\n");
    fprintf(stdout, "\n Flex\n"
                    "    --flexRefs refs.txt        Read flexible reference list from "
                    "following file.\n"
                    "\n");
}

/**
 * \page preset Preset Option Description

 The <b>\-\-preset</b> option is used to explicitly defines preset encoding parameters. Preset settings
 shows the balance between  performance and compression efficiency. A higher preset value means
 high quality but lower performance. When the <b>\-\-preset</b> option is specified, the default
 parameters defined in the table \ref tab_preset are loaded.

<table>
<caption id='tab_preset'>Preset Definition</caption>
<tr>
  <th align="center">Preset: H264
  <th align="center"> Preset: HEVC
  <th align="center"> \-\-lookaheadDepth
  <th align="center"> \-\-rdoLevel
  <th align="center"> \-\-enableRdoQuant
  <th align="center"> Performance
<tr>
  <td align="center"> N/A
  <td align="center"> 4
  <td align="center"> 40
  <td align="center"> 3
  <td align="center"> 1
  <td align="center"> 4K12fps
<tr>
  <td align="center"> N/A
  <td align="center"> 3
  <td align="center"> 40
  <td align="center"> 3
  <td align="center"> 0
  <td align="center"> 4K18fps
<tr>
  <td align="center"> N/A
  <td align="center"> 2
  <td align="center"> 40
  <td align="center"> 2
  <td align="center"> 0
  <td align="center"> 4K30fps
<tr>
  <td align="center"> 1
  <td align="center"> 1
  <td align="center"> 40
  <td align="center"> 1
  <td align="center"> 0
  <td align="center"> 4K60fps
<tr>
  <td align="center"> 0
  <td align="center"> 0
  <td align="center"> 0
  <td align="center"> 1
  <td align="center"> 0
  <td align="center"> 4K75fps
</table>

 When preset are specified, the default parameters defined in above table are loaded. The later
 options will overwrite the previous values for the option parser is working one by one.

 The performance data is an approximate value in general condition, may change according to
 actual stream condition. Assume the core work frequency is 650MHz.
 */
void Parameter_Preset(commandLine_s *cml)
{
    if (cml->preset >= 4) {
        cml->lookaheadDepth = 40;
        cml->rdoLevel = 3;
        cml->enableRdoQuant = 1;
    } else if (cml->preset == 3) {
        cml->lookaheadDepth = 40;
        cml->rdoLevel = 3;
        cml->enableRdoQuant = 0;
    } else if (cml->preset == 2) {
        cml->lookaheadDepth = 40;
        cml->rdoLevel = 2;
        cml->enableRdoQuant = 0;
    } else if (cml->preset == 1) {
        cml->lookaheadDepth = 40;
        cml->rdoLevel = 1;
        cml->enableRdoQuant = 0;
    } else if (cml->preset == 0) {
        cml->lookaheadDepth = 0;
        cml->rdoLevel = 1;
        cml->enableRdoQuant = 0;
    }
}

i32 Parameter_Check(commandLine_s *cml)
{
    EWLAttach(NULL, 0, cml->useVcmd);

    EWLHwConfig_t asic_cfg = VCEncGetAsicConfig(cml->codecFormat, NULL); //EncAsicGetAsicConfig(0);

    if (0) { // (asic_cfg.hw_asic_id == 0) {
        printf("Cannot Get Valid HW Configure!\n");
        return NOK;
    }

    if (cml->lookaheadDepth > 0 &&
        ((!asic_cfg.bFrameEnabled && cml->gopSize != 1) || asic_cfg.cuInforVersion < 1)) {
        // lookahead needs bFrame support & cuInfo version 1
        printf("Lookahead not supported, disable lookahead!\n");
        cml->lookaheadDepth = 0;
    }

    if (cml->lookaheadDepth > 0 && asic_cfg.cuInforVersion != 2 && asic_cfg.bMultiPassSupport) {
        printf("IM only support cuInfo version 2!\n");
        return NOK;
    }

    if (((i32)asic_cfg.cuInforVersion < cml->cuInfoVersion) ||
        (cml->cuInfoVersion == 0 && asic_cfg.cuInforVersion != 0)) {
        printf("CuInfoVersion %d is not supported\n", cml->cuInfoVersion);
        return NOK;
    }

    if (cml->inLoopDSRatio > 0 && (cml->lookaheadDepth == 0 || asic_cfg.inLoopDSRatio == 0))
        cml->inLoopDSRatio = 0;

    /* We check OSD FUSE here to avoid testbench level osd working(test OSD input existence) */
    if (cml->overlayEnables > 0 && !asic_cfg.OSDSupport) {
        printf("OSD not supported\n");
        return NOK;
    }

    if (cml->layerInRefIdc == 1 && !asic_cfg.h264NalRefIdc2bit) {
        printf("h264NalRefIdc2bit not supported, disable 2bitNalRefIdc!\n");
        cml->layerInRefIdc = 0;
    }

    if (cml->lookaheadDepth) {
        cml->gopLowdelay = 0; /* lookahead not work well with lowdelay configurations */
    }

    if ((IS_AV1(cml->codecFormat) || IS_VP9(cml->codecFormat)) && cml->sliceSize != 0) {
        printf("WARNING: No multi slice support in AV1 or VP9\n");
        cml->sliceSize = 0;
    }

    if (cml->tiles_enabled_flag && cml->sliceSize != 0) {
        printf("WARNING:sliceSize NOT supported when multi tiles\n");
        cml->sliceSize = 0;
    }

    if ((IS_AV1(cml->codecFormat) || IS_VP9(cml->codecFormat)) && !cml->byteStream) {
        printf("WARNING: AV1 and VP9 only supports byte stream mode\n");
        cml->byteStream = 1;
    }

    if (IS_H264(cml->codecFormat) && cml->rdoLevel > 1) {
        printf("WARNING: rdoLevel 2/3 NOT support in H264, set rdoLevel to 1\n");
        cml->rdoLevel = 1;
    }

    true_e secondIpcmSkipMapValid = (asic_cfg.roiMapVersion >= 3) &&
                                    (cml->RoimapCuCtrlInfoBinFile != NULL) &&
                                    (cml->RoiCuCtrlVer >= 4);
    true_e firstIpcmMapValid =
        ((cml->ipcmMapFile != NULL) ||
         ((asic_cfg.roiMapVersion >= 3) && (cml->roiMapInfoBinFile != NULL)) ||
         (((asic_cfg.roiMapVersion >= 3) && (cml->roiMapConfigFile != NULL)))) &&
        (cml->RoiQpDeltaVer == 1);
    if (cml->ipcmMapEnable && (ENCHW_NO == secondIpcmSkipMapValid) &&
        (ENCHW_NO == firstIpcmMapValid)) {
        printf("Error: ipcmMapEnable NOT valid configure\n");
        return NOK;
    }

    true_e onePassQpMapValid =
        (0 == cml->lookaheadDepth) &&
        ((cml->roiMapDeltaQpFile != NULL) ||
         ((asic_cfg.roiMapVersion >= 3) &&
          ((cml->roiMapInfoBinFile != NULL) || (cml->roiMapConfigFile != NULL))));
    true_e twoPassQpMapValid = (0 != cml->lookaheadDepth) && (cml->roiMapDeltaQpBinFile != NULL);

    true_e firstSkipMapValid =
        ((((cml->skipMapFile != NULL) && (asic_cfg.roiMapVersion >= 2)) ||
          ((asic_cfg.roiMapVersion >= 3) &&
           ((cml->roiMapInfoBinFile != NULL) || (cml->roiMapConfigFile != NULL)))) &&
         (cml->RoiQpDeltaVer == 2)) ||
        ((asic_cfg.roiMapVersion == 4) && (cml->RoiQpDeltaVer == 4) &&
         (onePassQpMapValid || twoPassQpMapValid));
    if (cml->skipMapEnable && (ENCHW_NO == secondIpcmSkipMapValid) &&
        (ENCHW_NO == firstSkipMapValid)) {
        printf("Error: skipMapEnable NOT valid configure\n");
        return NOK;
    }

    if (cml->roiMapDeltaQpEnable && (ENCHW_NO == onePassQpMapValid) &&
        (ENCHW_NO == twoPassQpMapValid)) {
        printf("Error: roiMapDeltaQpEnable NOT valid configure\n");
        return NOK;
    }

    true_e onePassRdoqMapValid =
        (asic_cfg.roiMapVersion == 4) && (cml->RoiQpDeltaVer == 4) && (cml->lookaheadDepth == 0) &&
        cml->roiMapDeltaQpEnable &&
        ((cml->roiMapInfoBinFile != NULL) || (cml->roiMapConfigFile != NULL));
    true_e twoPassRdoqMapValid = (asic_cfg.roiMapVersion == 4) && (cml->RoiQpDeltaVer == 4) &&
                                 (cml->lookaheadDepth > 0) && (cml->roiMapDeltaQpBinFile != NULL);
    if (cml->rdoqMapEnable && (ENCHW_NO == onePassRdoqMapValid) &&
        (ENCHW_NO == twoPassRdoqMapValid)) {
        printf("Error: rdoqMapEnable NOT valid configure\n");
        return NOK;
    }

#ifdef VMAF_SUPPORT
    if (cml->tune == 4 && cml->inputFormat != 0) {
        printf("Error: tune=vmaf only supports YUV420P format\n");
        return NOK;
    }
#endif

    if (cml->num_tile_columns > 1 && cml->parallelCoreNum > 1) {
        printf("Warning: multi-tile-columns encoding only support tile-columns "
               "parallel when num_tile_columns > 1, so disable frame level parallel "
               "\n");
        cml->parallelCoreNum = 1;
    }

    /* disable DEB & SAO for InputAsRef test */
    if (cml->testId == 45) {
        cml->disableDeblocking = 1;
        cml->enableSao = 0;
    }

    /* check RC Mode with other parameters */
    if (cml->rcMode == VCE_RC_CVBR) {
        /* if rcMode uses default CVBR, below options can auto-change rcMode,
    * otherwise rcMode has higher priority
    */
        if (cml->vbr == 1 && cml->crf < 0)
            cml->rcMode = VCE_RC_VBR;
        else if (cml->cpbSize > 0)
            cml->rcMode = VCE_RC_CBR;
        else if (cml->crf >= 0)
            cml->rcMode = VCE_RC_CRF;
        else if (cml->picRc <= 0)
            cml->rcMode = VCE_RC_CQP;
    }

    if (cml->rcMode == VCE_RC_CVBR) {
        cml->vbr = 0;
        cml->crf = -1;
        cml->picRc = 1;
        cml->cpbSize = 0;
        cml->cpbMaxRate = 0;
    } else if (cml->rcMode == VCE_RC_CBR) {
        if (cml->cpbSize <= 0) {
            /* set to 2 second size by default */
            cml->cpbSize = 2 * cml->bitPerSecond;
        }
        cml->vbr = 0;
        cml->crf = -1;
        cml->picRc = 1;
    } else if (cml->rcMode == VCE_RC_VBR) {
        cml->vbr = 1;
        cml->crf = -1;
        cml->picRc = 1;
        cml->cpbSize = 0;
        cml->cpbMaxRate = 0;
    } else if (cml->rcMode == VCE_RC_ABR) {
        cml->vbr = 0;
        cml->crf = -1;
        cml->picRc = 1;
        cml->cpbSize = 0;
        cml->cpbMaxRate = 0;
    } else if (cml->rcMode == VCE_RC_CRF) {
        cml->vbr = 0;
        cml->picRc = 0;
        cml->cpbSize = 0;
        cml->cpbMaxRate = 0;
        if (cml->crf < 0) {
            printf("Error: CRF mode sets invalid crf value %d\n", cml->crf);
            return NOK;
        }
    } else if (cml->rcMode == VCE_RC_CQP) {
        cml->vbr = 0;
        cml->crf = -1;
        cml->picRc = 0;
        cml->cpbSize = 0;
        cml->cpbMaxRate = 0;
    } else {
        printf("Error: Not support such RC mode %d\n", cml->rcMode);
        return NOK;
    }

    return OK;
}

/**
 * Parse input options and arguments and save the parameters
 *
 * \param [out] cml save the parsed result
 * \return NOK parameters has error.
 * \return OK  parameters are parsed and filled;
 */
i32 Parameter_Get(i32 argc, char **argv, commandLine_s *cml)
{
    struct parameter prm;
    i32 status = OK;
    i32 ret, i, j;
    u32 cml_num, m = 0, pos = 0;
    char *p;
    i32 bpsAdjustCount = 0;

    if (cml->trigger_prm_cnt == 0)
        prm.cnt = 1;
    else
        prm.cnt = cml->trigger_prm_cnt;

    while (1) {
        p = argv[prm.cnt];
        ret = get_option(argc, argv, options, &prm);
        if (ret == -1)
            break;

        if (ret == 2) {
            cml->trigger_pic_cnt = atoi(prm.argument);
            cml->trigger_prm_cnt = prm.cnt;
            break;
        }

        if (ret != 0) {
            status = NOK;
            fprintf(stderr, "Error: Invalid Option %s\n", p);
            return NOK;
        }

        p = prm.argument;
        cml_num = prm.match_index;
        // Begin to parse the specify options.
        if (strcmp(&options[cml_num].short_opt, "H") == 0) {
            help(argv[0]);
            if (strcmp(p, "dev") == 0)
                help_dev();
            continue; //If the prm matched option, contine the next prm.
        }
        if (strcmp(&options[cml_num].short_opt, "1") == 0) {
            if (bpsAdjustCount == MAX_BPS_ADJUST)
                continue;
            /* Argument must be "xx:yy", replace ':' with 0 */
            /* --option=xx:yy:zz */
            if ((i = ParseDelim(p, ':')) == -1)
                continue;
            /* xx is frame number */
            cml->bpsAdjustFrame[bpsAdjustCount] = atoi(p);
            /* yy is new target bitrate */
            cml->bpsAdjustBitrate[bpsAdjustCount] = atoi(p + i + 1);
            bpsAdjustCount++;
            continue;
        }
        if (strcmp(prm.longOpt, "AXIAlignment") == 0) {
            cml->AXIAlignment = strtoull(p, NULL, 16);
            continue;
        }
        if (strcmp(options[cml_num].long_opt, "irqTypeMask") == 0) {
            cml->irqTypeMask = strtoul(p, NULL, 2);
            continue;
        }
        if (strcmp(options[cml_num].long_opt, "irqTypeCutreeMask") == 0) {
            cml->irqTypeCutreeMask = strtoul(p, NULL, 2);
            continue;
        }
        if (strcmp(options[cml_num].long_opt, "smartConfig") == 0) {
            ParsingSmartConfig(p, cml);
            continue;
        }
        if (strcmp(options[cml_num].long_opt, "streamcfg") == 0) {
            cml->streamcfg[cml->nstream++] = p;
            continue;
        }
        // Loop is used to parse the common options.
        for (m = 0; m < options[cml_num].multiple_num; m++) {
            if (options[cml_num].data_type == I32_NUM_TYPE ||
                options[cml_num].data_type == MIX_TYPE)
                *(i32 *)(((i8 *)cml) + options[cml_num].offset[m]) = atoi(p);
            else if (options[cml_num].data_type == U32_NUM_TYPE)
                *(u32 *)(((i8 *)cml) + options[cml_num].offset[m]) = atoi(p);
            else if (options[cml_num].data_type == U64_NUM_TYPE)
                *(u64 *)(((i8 *)cml) + options[cml_num].offset[m]) = atoi(p);
            else if (options[cml_num].data_type == I16_NUM_TYPE)
                *(i16 *)(((i8 *)cml) + options[cml_num].offset[m]) = atoi(p);
            else if (options[cml_num].data_type == STR_TYPE)
                *(char **)(((i8 *)cml) + options[cml_num].offset[m]) = p;
            else if (options[cml_num].data_type == FLOAT_TYPE)
                *(float *)(((i8 *)cml) + options[cml_num].offset[m]) = atof(p);
            else if (options[cml_num].data_type == ENUM_STR_TYPE) {
                if (strcmp(options[cml_num].long_opt, "codecFormat") == 0) {
                    if (strcmp(p, "hevc") == 0)
                        cml->codecFormat = VCENC_VIDEO_CODEC_HEVC;
                    else if (strcmp(p, "h264") == 0)
                        cml->codecFormat = VCENC_VIDEO_CODEC_H264;
                    else if (strcmp(p, "av1") == 0)
                        cml->codecFormat = VCENC_VIDEO_CODEC_AV1;
                    else if (strcmp(p, "vp9") == 0)
                        cml->codecFormat = VCENC_VIDEO_CODEC_VP9;
                    else if (p[0] >= '0' && p[0] <= '9')
                        cml->codecFormat = atoi(p);
                    else {
                        ASSERT(0 && "unknown codecFormat");
                    }
                    if (IS_H264(cml->codecFormat)) {
                        cml->max_cu_size = 16;
                        cml->min_cu_size = 8;
                        cml->max_tr_size = 16;
                        cml->min_tr_size = 4;
                        cml->tr_depth_intra = 1;
                        cml->tr_depth_inter = 2;
                    }
                } else if (strcmp(options[cml_num].long_opt, "tune") == 0) {
                    if (strcmp(p, "psnr") == 0)
                        cml->tune = VCENC_TUNE_PSNR;
                    else if (strcmp(p, "ssim") == 0)
                        cml->tune = VCENC_TUNE_SSIM;
                    else if (strcmp(p, "visual") == 0)
                        cml->tune = VCENC_TUNE_VISUAL;
                    else if (strcmp(p, "sharpness_visual") == 0)
                        cml->tune = VCENC_TUNE_SHARP_VISUAL;
#ifdef VMAF_SUPPORT
                    else if (strcmp(p, "vmaf") == 0)
                        cml->tune = 4;
#endif
                    else if (p[0] >= '0' && p[0] <= '9')
                        cml->tune = atoi(p);
                    else {
                        fprintf(stderr, "unknown tuning option\n");
                        status = NOK;
                    }
                } else if (strcmp(options[cml_num].long_opt, "rcMode") == 0) {
                    if (strcmp(p, "cvbr") == 0 || strcmp(p, "CVBR") == 0)
                        cml->rcMode = VCE_RC_CVBR;
                    else if (strcmp(p, "cbr") == 0 || strcmp(p, "CBR") == 0)
                        cml->rcMode = VCE_RC_CBR;
                    else if (strcmp(p, "vbr") == 0 || strcmp(p, "VBR") == 0)
                        cml->rcMode = VCE_RC_VBR;
                    else if (strcmp(p, "abr") == 0 || strcmp(p, "ABR") == 0)
                        cml->rcMode = VCE_RC_ABR;
                    else if (strcmp(p, "crf") == 0 || strcmp(p, "CRF") == 0)
                        cml->rcMode = VCE_RC_CRF;
                    else if (strcmp(p, "cqp") == 0 || strcmp(p, "CQP") == 0)
                        cml->rcMode = VCE_RC_CQP;
                    else if (p[0] >= '0' && p[0] <= '5')
                        cml->rcMode = atoi(p);
                    else {
                        printf("unknown rcMode %s, change to default CVBR\n", p);
                        cml->rcMode = VCE_RC_CVBR;
                        ASSERT(0 && "unknown rcMode");
                    }
                }
            }
            if (options[cml_num].multiple_num > 1) {
                /* Argument must be "xx:yy:zz", replace ':' with 0 */
                if ((pos = ParseDelim(p, ':')) == -1)
                    break;    // End the for loop:
                p += pos + 1; // for (m = 0; m < options[cml_num].multiple_num; m++)
            }
        }
        //After cml->value set value,
        //supplement the additional cml->value, which doesn't exist in option array.
        if (strcmp(options[cml_num].long_opt, "intraArea") == 0)
            cml->intraAreaEnable = 1;
        if (strcmp(options[cml_num].long_opt, "roi1Area") == 0)
            cml->roi1AreaEnable = 1;
        if (strcmp(options[cml_num].long_opt, "roi2Area") == 0)
            cml->roi2AreaEnable = 1;
        if (strcmp(options[cml_num].long_opt, "roi3Area") == 0)
            cml->roi3AreaEnable = 1;
        if (strcmp(options[cml_num].long_opt, "roi4Area") == 0)
            cml->roi4AreaEnable = 1;
        if (strcmp(options[cml_num].long_opt, "roi5Area") == 0)
            cml->roi5AreaEnable = 1;
        if (strcmp(options[cml_num].long_opt, "roi6Area") == 0)
            cml->roi6AreaEnable = 1;
        if (strcmp(options[cml_num].long_opt, "roi7Area") == 0)
            cml->roi7AreaEnable = 1;
        if (strcmp(options[cml_num].long_opt, "roi8Area") == 0)
            cml->roi8AreaEnable = 1;
        if (strcmp(options[cml_num].long_opt, "tile") == 0)
            cml->tiles_enabled_flag = ((cml->num_tile_columns * cml->num_tile_rows) > 1);
        if (strcmp(options[cml_num].long_opt, "skipFramePOC") == 0)
            cml->skip_frame_enabled_flag = (cml->skip_frame_poc != 0);
        if (strcmp(options[cml_num].long_opt, "mirror") == 0)
            cml->mirror = CLIP3(0, 1, cml->mirror);
        if (strcmp(options[cml_num].long_opt, "t35") == 0) {
            if (cml->itu_t_t35_country_code == -1)
                cml->itu_t_t35_enable = ENCHW_NO;
            else
                cml->itu_t_t35_enable = ENCHW_YES;
        }
        if (strcmp(options[cml_num].long_opt, "HDR10_display") == 0) {
            if (cml->hdr10_dx0 == -1)
                cml->hdr10_display_enable = ENCHW_NO;
            else
                cml->hdr10_display_enable = ENCHW_YES;
        }
        if (strcmp(options[cml_num].long_opt, "HDR10_lightlevel") == 0) {
            if (cml->hdr10_maxlight == -1)
                cml->hdr10_lightlevel_enable = ENCHW_NO;
            else
                cml->hdr10_lightlevel_enable = ENCHW_YES;
        }
        if (strcmp(options[cml_num].long_opt, "vuiColordescription") == 0)
            cml->vuiColorDescripPresentFlag = ENCHW_YES;
        if (strcmp(options[cml_num].long_opt, "preset") == 0)
            Parameter_Preset(cml);
        if (strcmp(options[cml_num].long_opt, "smoothingIntra") == 0) {
            ASSERT(cml->strong_intra_smoothing_enabled_flag < 2);
        }
        if (strncmp("mosArea", options[cml_num].long_opt, strlen("mosArea")) == 0) {
            //update offset[2]
            *(u32 *)(((i8 *)cml) + options[cml_num].offset[2]) =
                *(u32 *)(((i8 *)cml) + options[cml_num].offset[2]) -
                *(u32 *)(((i8 *)cml) + options[cml_num].offset[0]);
            //update offset[3]
            *(u32 *)(((i8 *)cml) + options[cml_num].offset[3]) =
                *(u32 *)(((i8 *)cml) + options[cml_num].offset[3]) -
                *(u32 *)(((i8 *)cml) + options[cml_num].offset[1]);
        }
    }

#ifdef __FREERTOS__
#ifndef FREERTOS_SIMULATOR
#include "user_freertos.h"
    extern u8 user_freertos_vcmd_en;
    if (cml->sliceSize > 1) { //VCMD HW not support the multi slice, so here bypass vcmd
        user_freertos_vcmd_en = 0;
    }
    Platform_init();
#endif
#endif
    VCEncLogInit(cml->logOutDir, cml->logOutLevel, cml->logTraceMap, cml->logCheckMap);
    if (Parameter_Check(cml) != OK)
        status = NOK;
    return status;
}

void logstr(char *cml_all_log, char *cml_log_buffer, int cml_all_log_size, const char *fmt, ...)
{
    va_list arg;
    va_start(arg, fmt);
    vsnprintf(cml_log_buffer, MAX_NUM_ALINE, fmt, arg);
    strncat(cml_all_log, cml_log_buffer, cml_all_log_size - strlen(cml_all_log) - 1);
    va_end(arg);
}

i8 cml_print(struct option *option, commandLine_s *cml, char *cml_all_log, int cml_all_log_size)
{
    u8 str_index;
    i16 i = 0, j;
    char cml_log_buffer[MAX_NUM_ALINE] = {0};
    if (!option || !cml || !cml_all_log)
        return NOK;
    while (option[i].long_opt) {
        if (option[i].data_type == TODO_TYPE) {
            i++;
            continue;
        }
        if (option[i].enable != 2) {
            if (option[i].data_type == I32_NUM_TYPE)
                CML_OUT(i32);
            else if (option[i].data_type == U32_NUM_TYPE)
                CML_OUT(u32);
            else if (option[i].data_type == I16_NUM_TYPE)
                CML_OUT(i16);
            else if (option[i].data_type == U64_NUM_TYPE) {
                logstr(cml_all_log, cml_log_buffer, cml_all_log_size, "--%s=", option[i].long_opt);
                for (int j = 0; j < option[i].multiple_num; j++) {
                    if (j == option[i].multiple_num - 1) {
                        logstr(cml_all_log, cml_log_buffer, cml_all_log_size, "%ld \\\n",
                               *(u64 *)(((i8 *)cml) + option[i].offset[j]));
                        break;
                    }
                    logstr(cml_all_log, cml_log_buffer, cml_all_log_size,
                           "%ld:", *(u64 *)(((i8 *)cml) + option[i].offset[j]));
                }
            } else if (option[i].data_type == STR_TYPE) {
                if (*((char **)(((i8 *)cml) + option[i].offset[0])) != NULL) {
                    logstr(cml_all_log, cml_log_buffer, cml_all_log_size, "--%s=%s \\\n",
                           option[i].long_opt, *((char **)(((i8 *)cml) + option[i].offset[0])));
                }
            } else if (option[i].data_type == FLOAT_TYPE) {
                logstr(cml_all_log, cml_log_buffer, cml_all_log_size, "--%s=%.1f \\\n",
                       option[i].long_opt, *(float *)(((i8 *)cml) + option[i].offset[0]));
            } else if (option[i].data_type == ENUM_STR_TYPE) //eg: --codecFormat --tune
            {
                j = 0;
                while (enum_str_options[j].long_opt) {
                    if (strcmp(option[i].long_opt, enum_str_options[j].long_opt) == 0) {
                        str_index = *(u32 *)(((i8 *)cml) + option[i].offset[0]);
                        if (str_index > (sizeof(enum_str_options[j].enum_str) /
                                         sizeof(enum_str_options[j].enum_str[0])) &&
                            str_index <= 9)
                            logstr(cml_all_log, cml_log_buffer, cml_all_log_size, "--%s=%d \\\n",
                                   option[i].long_opt, *(u32 *)(((i8 *)cml) + option[i].offset[0]));
                        else
                            logstr(cml_all_log, cml_log_buffer, cml_all_log_size, "--%s=%s \\\n",
                                   option[i].long_opt, enum_str_options[j].enum_str[str_index]);
                    }
                    j++;
                }
            } else if (option[i].data_type == MIX_TYPE) {
                if (strcmp(option[i].long_opt, "LTR") == 0) {
                    logstr(cml_all_log, cml_log_buffer, cml_all_log_size, "--%s=%d:%d:%d:%d \\\n",
                           option[i].long_opt, *(u32 *)(((i8 *)cml) + option[i].offset[0]),
                           *(u32 *)(((i8 *)cml) + option[i].offset[1]),
                           *(u32 *)(((i8 *)cml) + option[i].offset[2]),
                           *(i32 *)(((i8 *)cml) + option[i].offset[3]));
                }
            }
        }
        i++;
    }
    return OK;
}

i8 cml_log(char *p_argv, commandLine_s *cml)
{
    int cml_all_log_size;
    cml_all_log_size = LOGSIZEBUFFER * sizeof(char);
    cml_all_log = malloc(cml_all_log_size);
    if (!cml_all_log)
        return NOK;
    memset(cml_all_log, 0, cml_all_log_size);
    if (cml_print(options, cml, cml_all_log, cml_all_log_size) == NOK)
        return NOK;
    else {
        CMLTRACE("VC8000E Test Bench Command Line:%s \\\n%s", p_argv, cml_all_log);
        free(cml_all_log);
        return OK;
    }
}

/*------------------------------------------------------------------------------
  change_input
------------------------------------------------------------------------------*/
i32 change_input(struct test_bench *tb)
{
    struct parameter prm;
    i32 enable = HANTRO_FALSE;
    i32 ret;

    prm.cnt = 1;
    while ((ret = get_option(tb->argc, tb->argv, options, &prm)) != -1) {
        if ((ret == 1) && enable) {
            tb->input = prm.argument;
            printf("Next input file %s\n", tb->input);
            return OK;
        }
        if (prm.argument == tb->input) {
            enable = HANTRO_TRUE;
        }
    }

    return NOK;
}

/*------------------------------------------------------------------------------
  _get_log_env
------------------------------------------------------------------------------*/
static void _get_log_env(log_env_setting *env_log)
{
    char *env_log_tmp = NULL;
    unsigned int tmp_val_1;

    env_log_tmp = getenv("VCENC_LOG_OUTPUT");
    if (env_log_tmp) {
        tmp_val_1 = atoi(env_log_tmp);
        if (tmp_val_1 < LOG_COUNT) {
            env_log->out_dir = (vcenc_log_output)tmp_val_1;
        }
    }

    env_log_tmp = getenv("VCENC_LOG_LEVEL");
    if (env_log_tmp) {
        tmp_val_1 = atoi(env_log_tmp);
        if (tmp_val_1 < VCENC_LOG_COUNT) {
            env_log->out_level = (vcenc_log_level)tmp_val_1;
        }
    }

    env_log_tmp = getenv("VCENC_LOG_TRACE");
    if (env_log_tmp) {
        env_log->k_trace_map = atoi(env_log_tmp);
    }

    env_log_tmp = getenv("VCENC_LOG_CHECK");
    if (env_log_tmp) {
        env_log->k_check_map = atoi(env_log_tmp);
    }
}
/*------------------------------------------------------------------------------
  default_parameter
------------------------------------------------------------------------------*/
void default_parameter(commandLine_s *cml)
{
    int i;
    memset(cml, 0, sizeof(commandLine_s));

    cml->useVcmd = -1;
    cml->useDec400 = 0;
    cml->useL2Cache = 0;

    cml->input = "input.yuv";
    cml->output = "stream.hevc";
    cml->recon = "deblock.yuv";
    cml->firstPic = 0;
    cml->lastPic = 100;
    cml->inputRateNumer = 30;
    cml->inputRateDenom = 1;
    cml->outputRateNumer = DEFAULT;
    cml->outputRateDenom = DEFAULT;
    cml->test_data_files = getenv("TEST_DATA_FILES");
    cml->lumWidthSrc = DEFAULT;
    cml->lumHeightSrc = DEFAULT;
    cml->horOffsetSrc = DEFAULT;
    cml->verOffsetSrc = DEFAULT;
    cml->videoStab = DEFAULT;
    cml->rotation = 0;
    cml->inputFormat = 0;
    cml->formatCustomizedType = -1;

    cml->width = DEFAULT;
    cml->height = DEFAULT;
    cml->max_cu_size = 64;
    cml->min_cu_size = 8;
    cml->max_tr_size = 16;
    cml->min_tr_size = 4;
    cml->tr_depth_intra = 2; //mfu =>0
    cml->tr_depth_inter = (cml->max_cu_size == 64) ? 4 : 3;
    cml->rdoLevel = 3;
    cml->intraPicRate = 0; // only first is IDR.
    cml->codecFormat = VCENC_VIDEO_CODEC_HEVC;
#ifdef CODEC_FORMAT
    cml->codecFormat = CODEC_FORMAT;
#endif
    if (IS_H264(cml->codecFormat)) {
        cml->max_cu_size = 16;
        cml->min_cu_size = 8;
        cml->max_tr_size = 16;
        cml->min_tr_size = 4;
        cml->tr_depth_intra = 1;
        cml->tr_depth_inter = 2;
        cml->layerInRefIdc = 0;
        cml->rdoLevel = 1;
    }

    cml->bitPerSecond = 1000000;
    cml->cpbMaxRate = 0;
    cml->bitVarRangeI = 10000;
    cml->bitVarRangeP = 10000;
    cml->bitVarRangeB = 10000;
    cml->tolMovingBitRate = 100;
    cml->monitorFrames = DEFAULT;
    cml->tolCtbRcInter = DEFAULT;
    cml->tolCtbRcIntra = DEFAULT;
    cml->u32StaticSceneIbitPercent = 80;
    cml->intraQpDelta = DEFAULT;
    cml->bFrameQpDelta = -1;

    cml->disableDeblocking = 0;
    cml->tc_Offset = 0;
    cml->beta_Offset = 0;

    cml->qpHdr = DEFAULT;
    cml->qpMin = DEFAULT;
    cml->qpMax = DEFAULT;
    cml->qpMinI = DEFAULT;
    cml->qpMaxI = DEFAULT;
    cml->picRc = DEFAULT;
    cml->ctbRc = DEFAULT; //CTB_RC
    cml->cpbSize = DEFAULT;
    cml->bitrateWindow = DEFAULT;
    cml->fixedIntraQp = 0;
    cml->fillerData = 0;
    cml->hrdConformance = 0;
    cml->smoothPsnrInGOP = 0;
    cml->vbr = 0;
    cml->rcMode = VCE_RC_CVBR;

    cml->byteStream = 1;

    cml->chromaQpOffset = 0;

    cml->enableSao = 1;

    cml->strong_intra_smoothing_enabled_flag = 0;

    cml->pcm_loop_filter_disabled_flag = 0;

    cml->intraAreaLeft = cml->intraAreaRight = cml->intraAreaTop = cml->intraAreaBottom =
        -1; /* Disabled */
    cml->ipcm1AreaLeft = cml->ipcm1AreaRight = cml->ipcm1AreaTop = cml->ipcm1AreaBottom =
        -1; /* Disabled */
    cml->ipcm2AreaLeft = cml->ipcm2AreaRight = cml->ipcm2AreaTop = cml->ipcm2AreaBottom =
        -1; /* Disabled */

    cml->ipcm3AreaLeft = cml->ipcm3AreaRight = cml->ipcm3AreaTop = cml->ipcm3AreaBottom =
        -1; /* Disabled */
    cml->ipcm4AreaLeft = cml->ipcm4AreaRight = cml->ipcm4AreaTop = cml->ipcm4AreaBottom =
        -1; /* Disabled */
    cml->ipcm5AreaLeft = cml->ipcm5AreaRight = cml->ipcm5AreaTop = cml->ipcm5AreaBottom =
        -1; /* Disabled */
    cml->ipcm6AreaLeft = cml->ipcm6AreaRight = cml->ipcm6AreaTop = cml->ipcm6AreaBottom =
        -1; /* Disabled */
    cml->ipcm7AreaLeft = cml->ipcm7AreaRight = cml->ipcm7AreaTop = cml->ipcm7AreaBottom =
        -1; /* Disabled */
    cml->ipcm8AreaLeft = cml->ipcm8AreaRight = cml->ipcm8AreaTop = cml->ipcm8AreaBottom =
        -1; /* Disabled */
    cml->gdrDuration = 0;

    cml->picSkip = 0;

    cml->sliceSize = 0;

    cml->enableCabac = 1;
    cml->cabacInitFlag = 0;
    cml->cirStart = 0;
    cml->cirInterval = 0;
    cml->enableDeblockOverride = 0;
    cml->deblockOverride = 0;

    cml->enableScalingList = 0;

    cml->compressor = 0;
    cml->sei = 0;
    cml->videoRange = 0;
    cml->level = DEFAULT;
    cml->profile = DEFAULT;
    cml->tier = DEFAULT;
    cml->bitDepthLuma = DEFAULT;
    cml->bitDepthChroma = DEFAULT;
    cml->blockRCSize = DEFAULT;
    cml->rcQpDeltaRange = DEFAULT;
    cml->rcBaseMBComplexity = DEFAULT;
    cml->picQpDeltaMin = DEFAULT;
    cml->picQpDeltaMax = DEFAULT;
    cml->ctbRcRowQpStep = DEFAULT;

    cml->gopSize = 0;
    cml->gopCfg = NULL;
    cml->gopLowdelay = 0;
    cml->longTermGap = 0;
    cml->longTermGapOffset = 0;
    cml->longTermQpDelta = 0;
    cml->ltrInterval = DEFAULT;

    cml->flexRefs = NULL;

    cml->numRefP = 1;

    cml->outReconFrame = 1;

    cml->roiMapDeltaQpBlockUnit = 0;
    cml->roiMapDeltaQpEnable = 0;
    cml->roiMapDeltaQpFile = NULL;
    cml->roiMapDeltaQpBinFile = NULL;
    cml->roiMapInfoBinFile = NULL;
    cml->roiMapConfigFile = NULL;
    cml->RoimapCuCtrlInfoBinFile = NULL;
    cml->RoimapCuCtrlIndexBinFile = NULL;
    cml->RoiCuCtrlVer = 0;
    cml->RoiQpDeltaVer = 1;
    cml->ipcmMapEnable = 0;
    cml->ipcmMapFile = NULL;
    cml->roi1Qp = DEFAULT;
    cml->roi2Qp = DEFAULT;
    cml->roi3Qp = DEFAULT;
    cml->roi4Qp = DEFAULT;
    cml->roi5Qp = DEFAULT;
    cml->roi6Qp = DEFAULT;
    cml->roi7Qp = DEFAULT;
    cml->roi8Qp = DEFAULT;

    cml->reEncode = 0;

    cml->interlacedFrame = 0;
    cml->noiseReductionEnable = 0;

    /* low latency */
    cml->inputLineBufMode = 0;
    cml->inputLineBufDepth = DEFAULT;
    cml->amountPerLoopBack = 0;

    /*stride*/
    cml->exp_of_input_alignment = 6;
    cml->exp_of_ref_alignment = 6;
    cml->exp_of_ref_ch_alignment = 6;
    cml->exp_of_aqinfo_alignment = 6;
    cml->exp_of_tile_stream_alignment = 0;

    cml->multimode = 0;
    cml->nstream = 0;
    for (i = 0; i < MAX_STREAMS; i++)
        cml->streamcfg[i] = NULL;

    cml->enableOutputCuInfo = 0;
    cml->enableOutputCtbBits = 0;
    cml->P010RefEnable = 0;

    cml->dynamicRdoCu16Bias = 3;
    cml->dynamicRdoCu16Factor = 80;
    cml->dynamicRdoCu32Bias = 2;
    cml->dynamicRdoCu32Factor = 32;
    cml->dynamicRdoEnable = 0;
    cml->hashtype = 0;
    cml->verbose = 0;

    /* smart */
    cml->smartModeEnable = 0;
    cml->smartH264LumDcTh = 5;
    cml->smartH264CbDcTh = 1;
    cml->smartH264CrDcTh = 1;
    cml->smartHevcLumDcTh[0] = 2;
    cml->smartHevcLumDcTh[1] = 2;
    cml->smartHevcLumDcTh[2] = 2;
    cml->smartHevcChrDcTh[0] = 2;
    cml->smartHevcChrDcTh[1] = 2;
    cml->smartHevcChrDcTh[2] = 2;
    cml->smartHevcLumAcNumTh[0] = 12;
    cml->smartHevcLumAcNumTh[1] = 51;
    cml->smartHevcLumAcNumTh[2] = 204;
    cml->smartHevcChrAcNumTh[0] = 3;
    cml->smartHevcChrAcNumTh[1] = 12;
    cml->smartHevcChrAcNumTh[2] = 51;
    cml->smartH264Qp = 30;
    cml->smartHevcLumQp = 30;
    cml->smartHevcChrQp = 30;
    cml->smartMeanTh[0] = 5;
    cml->smartMeanTh[1] = 5;
    cml->smartMeanTh[2] = 5;
    cml->smartMeanTh[3] = 5;
    cml->smartPixNumCntTh = 0;

    /* constant chroma control */
    cml->constChromaEn = 0;
    cml->constCb = DEFAULT;
    cml->constCr = DEFAULT;

    for (i = 0; i < MAX_SCENE_CHANGE; i++)
        cml->sceneChange[i] = 0;

    cml->tiles_enabled_flag = 0;
    cml->num_tile_columns = 1;
    cml->num_tile_rows = 1;
    cml->loop_filter_across_tiles_enabled_flag = 1;
    cml->tileMvConstraint = 0;

    cml->skip_frame_enabled_flag = 0;
    cml->skip_frame_poc = 0;

    /* T35 */
    cml->itu_t_t35_enable = 0;
    cml->itu_t_t35_country_code = 0;
    cml->itu_t_t35_country_code_extension_byte = 0;
    cml->p_t35_payload_file = NULL;

    /* HDR10 */
    cml->write_once_HDR10 = 0;
    cml->hdr10_display_enable = 0;
    cml->hdr10_dx0 = 0;
    cml->hdr10_dy0 = 0;
    cml->hdr10_dx1 = 0;
    cml->hdr10_dy1 = 0;
    cml->hdr10_dx2 = 0;
    cml->hdr10_dy2 = 0;
    cml->hdr10_wx = 0;
    cml->hdr10_wy = 0;
    cml->hdr10_maxluma = 0;
    cml->hdr10_minluma = 0;

    cml->hdr10_lightlevel_enable = 0;
    cml->hdr10_maxlight = 0;
    cml->hdr10_avglight = 0;

    cml->vuiColorDescripPresentFlag = 0;
    cml->vuiColorPrimaries = 9;
    cml->vuiTransferCharacteristics = 0;
    cml->vuiMatrixCoefficients = 9;
    cml->vuiVideoFormat = 5;
    cml->vuiVideoSignalTypePresentFlag = 0;
    cml->vuiAspectRatioWidth = 0;
    cml->vuiAspectRatioHeight = 0;

    cml->picOrderCntType = 0;
    cml->log2MaxPicOrderCntLsb = 16;
    cml->log2MaxFrameNum = 12;

    cml->RpsInSliceHeader = 0;
    cml->ssim = 1;
    cml->psnr = 1;
    cml->vui_timing_info_enable = 1;
    cml->halfDsInput = NULL;
    cml->inLoopDSRatio = 1;

    /* skip mode */
    cml->skipMapEnable = 0;
    cml->skipMapFile = NULL;
    cml->skipMapBlockUnit = 0;

    /* rdoq mode */
    cml->rdoqMapEnable = 0;

    /* Frame-level core parallelism option */
    cml->parallelCoreNum = 1;

    /* two stream buffer */
    cml->streamBufChain = 0;

    /*multi-segment of stream buffer*/
    cml->streamMultiSegmentMode = 0;
    cml->streamMultiSegmentAmount = 4;

    /*dump register*/
    cml->dumpRegister = 0;

    cml->rasterscan = 0;
    cml->cuInfoVersion = -1;

#ifdef RECON_REF_1KB_BURST_RW
    cml->exp_of_input_alignment = 10;
    cml->exp_of_ref_alignment = 10;
    cml->exp_of_ref_ch_alignment = 10;
    cml->compressor = 2;
#endif
#ifdef RECON_REF_ALIGN64
    cml->exp_of_ref_alignment = 6;
    cml->exp_of_ref_ch_alignment = 6;
#endif

    cml->enableRdoQuant = DEFAULT;

    /*CRF constant*/
    cml->crf = -1;

    /*external SRAM*/
    cml->extSramLumHeightBwd =
        IS_H264(cml->codecFormat) ? 12 : (IS_HEVC(cml->codecFormat) ? 16 : 0);
    cml->extSramChrHeightBwd = IS_H264(cml->codecFormat) ? 6 : (IS_HEVC(cml->codecFormat) ? 8 : 0);
    cml->extSramLumHeightFwd =
        IS_H264(cml->codecFormat) ? 12 : (IS_HEVC(cml->codecFormat) ? 16 : 0);
    cml->extSramChrHeightFwd = IS_H264(cml->codecFormat) ? 6 : (IS_HEVC(cml->codecFormat) ? 8 : 0);

    /* AXI alignment */
    cml->AXIAlignment = 0;

    /* irq type mask, 0:normal interrupt, 1:abnormal interrupt */
    cml->irqTypeMask = 0x01f0;
    cml->irqTypeCutreeMask = 0x01f0;

    /* sliceNode */
    cml->sliceNode = 0;

    /*Ivf support*/
    cml->ivf = 1;

    /*DEC400 compress table*/
    cml->dec400CompTableinput = "dec400CompTableinput.bin";
    cml->osdDec400CompTableInput = NULL;

    /*PSY factor*/
    cml->psyFactor = DEFAULT;

    /*Overlay*/
    cml->overlayEnables = 0;
    for (i = 0; i < MAX_OVERLAY_NUM; i++) {
        cml->olInput[i] = "olInput.yuv";
        cml->olFormat[i] = 0;
        cml->olAlpha[i] = 0;
        cml->olWidth[i] = 0;
        cml->olCropWidth[i] = 0;
        cml->olScaleWidth[i] = 0;
        cml->olHeight[i] = 0;
        cml->olCropHeight[i] = 0;
        cml->olScaleHeight[i] = 0;
        cml->olXoffset[i] = 0;
        cml->olCropXoffset[i] = 0;
        cml->olYoffset[i] = 0;
        cml->olCropYoffset[i] = 0;
        cml->olYStride[i] = 0;
        cml->olUVStride[i] = 0;
        cml->olBitmapY[i] = 0;
        cml->olBitmapU[i] = 0;
        cml->olBitmapV[i] = 0;
        cml->olSuperTile[i] = 0;
    }

    /* mosaic */
    cml->mosaicEnables = 0;
    for (i = 0; i < MAX_MOSAIC_NUM; i++) {
        cml->mosHeight[i] = 0;
        cml->mosWidth[i] = 0;
        cml->mosXoffset[i] = 0;
        cml->mosYoffset[i] = 0;
    }

    cml->codedChromaIdc = VCENC_CHROMA_IDC_420;
    cml->aq_mode = DEFAULT;
    cml->aq_strength = DEFAULT;

    cml->preset = DEFAULT;

    cml->writeReconToDDR = 1;

    cml->TxTypeSearchEnable = 0;
    cml->av1InterFiltSwitch = 1;

    cml->sendAUD = 0;

    cml->extSEI = NULL;
    cml->tune = VCENC_TUNE_PSNR;
    cml->b_vmaf_preprocess = 0;

    cml->resendParamSet = 0;

    cml->sramPowerdownDisable = 0;

    cml->insertIDR = 0;

    cml->stopFrameNum = -1;

    cml->replaceMvFile = NULL;

    cml->inputFileList = NULL;

    /*AXI max burst length */
    cml->burstMaxLength = ENCH2_DEFAULT_BURST_LENGTH;

    cml->enableTMVP = DEFAULT;
    /*disable EOS*/
    cml->disableEOS = 0;

    /* modify space of av1's tile_group_size, just for verified when using ffmpeg, NOT using when encoding */
    cml->modifiedTileGroupSize = 0;

    cml->trigger_pic_cnt = DEFAULT;

    /* logmsg env default settting */
    log_env_setting env_log = {LOG_STDOUT, VCENC_LOG_WARN, 0x003F,
                               0x0001}; //enable API/REGS/EWL/MEM/RC/CML enable RECON
    _get_log_env(&env_log);
    cml->logOutDir = env_log.out_dir;
    cml->logOutLevel = env_log.out_level;
    cml->logTraceMap = env_log.k_trace_map;
    cml->logCheckMap = env_log.k_check_map;
    cml->refRingBufEnable = 0;
}

int parse_stream_cfg(const char *streamcfg, commandLine_s *pcml)
{
    i32 ret, i;
    char *p;
    FILE *fp = fopen(streamcfg, "r");
    default_parameter(pcml);
    if (fp == NULL)
        return NOK;
    if ((ret = fseek(fp, 0, SEEK_END)) < 0)
        return NOK;
    i32 fsize = ftell(fp);
    if (fsize < 0)
        return NOK;
    if ((ret = fseek(fp, 0, SEEK_SET)) < 0)
        return NOK;
    pcml->argv = (char **)malloc(MAXARGS * sizeof(char *));
    pcml->argv[0] = (char *)malloc(fsize + 1);
    ret = fread(pcml->argv[0], 1, fsize, fp);
    if (ret < 0)
        return ret;
    fclose(fp);

    p = pcml->argv[0];
    pcml->argv[0][fsize] = 0;
    for (i = 1; i < MAXARGS; i++) {
        while (p - pcml->argv[0] < fsize && *p && *p <= 32)
            ++p;
        if (p - pcml->argv[0] >= fsize || !*p)
            break;
        pcml->argv[i] = p;
        while (*p > 32)
            ++p;
        *p = 0;
        ++p;
    }
    pcml->argc = i;
    if (Parameter_Get(pcml->argc, pcml->argv, pcml)) {
        Error(2, ERR, "Input parameter error");
        return NOK;
    }
    return OK;
}
