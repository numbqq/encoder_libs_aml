/*------------------------------------------------------------------------------
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                                                                            --
--                            Verisilicon.                                    --
--                                                                            --
--      In the event of publication, the following notice is applicable:      --
--                                                                            --
--                   (C) COPYRIGHT 2020 HANTRO PRODUCTS OY                    --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--         The entire notice above must be reproduced on all copies.          --
--                                                                            --
--------------------------------------------------------------------------------
--
--  Description : Encoder HW Configure features.
--
------------------------------------------------------------------------------*/

/* Genenrated on 2022-03-25, not modify. */

#ifndef EWL_HWCFG_H
#define EWL_HWCFG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "base_type.h"

/** Hardware configuration description. Software will work accordingly. */
typedef struct EWLHwConfig {
    //Sub-System
    /** \brief ASIC_ID to identify VCE/VCEJ/IM */
    u32 hw_asic_id;
    /** \brief BUILD_ID to identify VCE/VCEJ/IM */
    u32 hw_build_id;
    //Format
    /** \brief HEVC encoding supported by HW. 0=not supported. 1=supported */
    u32 hevcEnabled;
    /** \brief H264 encoding supported by HW. 0=not supported. 1=supported */
    u32 h264Enabled;
    /** \brief JPEG encoding supported by HW. 0=not supported. 1=supported */
    u32 jpegEnabled;
    /** \brief VP9 encoding supported by HW. 0=not supported. 1=supported */
    u32 vp9Enabled;
    /** \brief AV1 encoding supported by HW. 0=not supported. 1=supported */
    u32 av1Enabled;
    //IM
    /** \brief Support of CuTree Lookahead. 0=not supported. 1=supported */
    u32 cuTreeSupport;
    //Resolution
    /** \brief Maximum video width supported by HW for HEVC encoding (unit pixels) */
    u32 maxEncodedWidthHEVC;
    /** \brief Maximum video width supported by HW for H264 encoding (unit pixels) */
    u32 maxEncodedWidthH264;
    /** \brief Maximum video width supported by HW for AV1 encoding (unit pixels) */
    u32 maxEncodedWidthAV1;
    /** \brief Maximum video width supported by HW for VP9 encoding (unit pixels) */
    u32 maxEncodedWidthVP9;
    /** \brief Maximum allowed video height extended from 8192 to 8640. 0=Not. 1=Yes. */
    u32 videoHeightExt;
    /** \brief Maximum video width supported by HW for JPEG encoding (unit pixels) */
    u32 maxEncodedWidthJPEG;
    //Major
    /** \brief HW bframe support. 0=not support bframe. 1=support bframe */
    u32 bFrameEnabled;
    /** \brief main10 supported by HW. 0=main8 supported. 1=main10 supported */
    u32 main10Enabled;
    /** \brief H264 RDOQ supported by HW. 0=not supported. 1=supported */
    u32 RDOQSupportH264;
    /** \brief HEVC RDOQ supported by HW. 0=not supported. 1=supported */
    u32 RDOQSupportHEVC;
    /** \brief AV1 RDOQ supported by HW. 0=not supported. 1=supported */
    u32 RDOQSupportAV1;
    u32 RDOQSupportVP9;
    /** \brief 0: Pframe 128, Bframe 64
1: Pframe 128, Bframe 112
2: Pframe 192, Bframe 112
3: Pframe 240, Bframe 240
4: Pframe 112, Bframe 56 */
    u32 meHorSearchRange;
    /** \brief ME Vertical search range of H264 */
    u32 meVertSearchRangeH264;
    /** \brief ME Vertical Search Range of HEVC/AV1/VP9 */
    u32 meVertSearchRangeHEVC;
    /** \brief SSIM calculation supported by HW. 0=not supported. 1=supported */
    u32 ssimSupport;
    u32 psnrSupport;
    /** \brief HEVC Tile supported by HW. 0=not supported. 1=supported */
    u32 hevcTileSupport;
    /** \brief HEVC SAO supported by HW. 0=not supported. 1=supported */
    u32 saoSupport;
    //Minor
    u32 rfcEnable;
    u32 roiMapVersion;
    u32 cuInforVersion;
    u32 ctbRcVersion;
    u32 progRdoEnable;
    u32 ljpegSupport;
    u32 JpegRoiMapSupport;
    u32 jpeg400Support;
    u32 jpeg422Support;
    u32 meVertRangeProgramable;
    u32 MonoChromeSupport;
    u32 intraTU32Enable;
    u32 bMultiPassSupport;
    u32 tu32Enable;
    u32 dynamicMaxTuSize;
    u32 CtbBitsOutSupport;
    u32 encVisualTuneSupport;
    /** \brief more ctbrc mode supported by HW */
    u32 ctbRcMoreMode;
    u32 encPsyTuneSupport;
    /** \brief Ref frame number minus 1 for L0
0=1 ref frame 1=2 ref frame */
    u32 maxRefNumList0Minus1;
    /** \brief determine the max slice height in CTB/MB */
    u32 encSliceSizeBits;
    /** \brief HEVC support Asymmetric PU partation 0=not supported. 1=supported */
    u32 asymPuSupport;
    /** \brief HEVC/H264 support the 4th skip candinate. 0=not supported. 1=supported */
    u32 maxMergeCand4Support;
    u32 encStreamSegmentSupport;
    u32 encAQInfoOut;
    //PRP
    /** \brief RGB to YUV conversion supported by HW. 0=not supported. 1=supported */
    u32 rgbEnabled;
    u32 NonRotationSupport;
    u32 NVFormatOnlySupport;
    u32 lineBufEnable;
    u32 inLoopDSRatio;
    u32 cscExtendSupport;
    /** \brief Down-scaling supported by HW. 0=not supported. 1=supported */
    u32 scalingEnabled;
    /** \brief out-loop scaler output YUV420SP. 0=not supported. 1=supported */
    u32 scaled420Support;
    /** \brief HW supports video stabilization or not. 0=not supported. 1=supported */
    u32 vsSupport;
    u32 OSDSupport;
    /** \brief How many OSD rectangles are supported by HW */
    u32 maxOsdNum;
    u32 MosaicSupport;
    /** \brief How many Mosaic rectangles are supported by HW */
    u32 maxMosaicNum;
    /** \brief HW supports tile8x8 related special input formats or not. 35,36 */
    u32 tile8x8FormatSupport;
    /** \brief HW supports tile4x4 related special input formats or not. 21,22,23 */
    u32 tile4x4FormatSupport;
    /** \brief HW supports customized special tile input formats or not. 26,27,28,29,30,33,34, */
    u32 customTileFormatSupport;
    /** \brief HW supports RGB888/BGR888 24bit input formats or not. */
    u32 rgb24bitFormatSupport;
    u32 prpSbiSupport;
    /** \brief Prp support mirror or not. */
    u32 prpMirrorSupport;
    //HW
    /** \brief External SRAM supported by HW. 0=not supported. 1=supported */
    u32 meExternSramSupport;
    /** \brief Bus connection of HW. 1=AHB. 2=OCP. 3=AXI. 4=PCI. 5=AXIAHB. 6=AXIAPB. */
    u32 busType;
    /** \brief Bus width fix 128bit. 0=32B 1=64B 2=128B 3=256B */
    u32 busWidth;
    /** \brief Maximum DDR alignment supported by HW */
    u32 maxAXIAlignment;
    /** \brief AXI alignment setting in heradecimal format, wr_common. 6=align to 64. 8=align to 256 */
    u32 axi_burst_align_wr_common;
    /** \brief AXI alignment setting in heradecimal format, wr_stream. 6=align to 64. 8=align to 256 */
    u32 axi_burst_align_wr_stream;
    /** \brief AXI alignment setting in heradecimal format, wr_chroma_ref. 6=align to 64. 8=align to 256 */
    u32 axi_burst_align_wr_chroma_ref;
    /** \brief AXI alignment setting in heradecimal format, wr_luma_ref. 6=align to 64. 8=align to 256 */
    u32 axi_burst_align_wr_luma_ref;
    /** \brief AXI alignment setting in heradecimal format, rd_common. 6=align to 64. 8=align to 256 */
    u32 axi_burst_align_rd_common;
    /** \brief AXI alignment setting in heradecimal format, rd_prp. 6=align to 64. 8=align to 256 */
    u32 axi_burst_align_rd_prp;
    /** \brief AXI alignment setting in heradecimal format, rd_ch_ref_prefetch. 6=align to 64. 8=align to 256 */
    u32 axi_burst_align_rd_ch_ref_prefetch;
    /** \brief AXI alignment setting in heradecimal format, rd_lu_ref_prefetch. 6=align to 64. 8=align to 256 */
    u32 axi_burst_align_rd_lu_ref_prefetch;
    /** \brief AXI alignment setting in heradecimal format, wr_cuinfo. 6=align to 64. 8=align to 256 */
    u32 axi_burst_align_wr_cuinfo;
    //supported in most HW (keep them always supported in future HW)
    /** \brief support write 2 bits for h264 nal_ref_idc syntax or not.
1=write 2 bits, 0=only write 1 bit. */
    u32 h264NalRefIdc2bit;
    /** \brief support av1_extension_flag register or not. It control the realted syntax in OBU_Header. */
    u32 av1ExtensionFlag;
    /** \brief support absolution QP or not for ROI. */
    u32 roiAbsQpSupport;
    /** \brief support 8 ROI areas or 2 ROI areas.
1=support 8 ROI Areas; 0=support 2 ROI Areas; */
    u32 ROI8Support;
    /** \brief CAVLC supported by HW. 0=not supported. 1=supported */
    u32 h264CavlcEnable;
    /** \brief Use linear or ring structure for ref and recon buffers (only available for P frame encoding now)
0: only support linear buffer.
1: support both linear and ring buffer. */
    u32 refRingBufEnable;
    u32 disableRecWtSupport;
    //Not supported in most HW
    u32 backgroundDetSupport;
    u32 P010RefSupport;
    /** \brief Denoise supported by HW (obsolete), 0=not supported. 1=supported */
    u32 deNoiseEnabled;
    /** \brief Stream Buffer Chain supported by HW. 0=not supported. 1=supported */
    u32 streamBufferChain;
    u32 gmvSupport;
    u32 IPCM8Support;
    //Not implemented in HW
    u32 av1InterpFilterSwitchable;
    /** \brief only support encoding I frame. */
    u32 IframeOnly;
    u32 dynamicRdoSupport;
    u32 tuneToolsSet2Support;
    u32 temporalMvpSupport;
    u32 streamMultiSegment;
} EWLHwConfig_t;

#define EWLShowHwConfig(info)                                                                      \
    do {                                                                                           \
        PTRACE("Get Hantro Encoder Hardware Configure:\n");                                        \
        PTRACE("                               hw_asic_id = %d\n", info->hw_asic_id);              \
        PTRACE("                              hw_build_id = %d\n", info->hw_build_id);             \
        PTRACE("                              hevcEnabled = %d\n", info->hevcEnabled);             \
        PTRACE("                              h264Enabled = %d\n", info->h264Enabled);             \
        PTRACE("                              jpegEnabled = %d\n", info->jpegEnabled);             \
        PTRACE("                               vp9Enabled = %d\n", info->vp9Enabled);              \
        PTRACE("                               av1Enabled = %d\n", info->av1Enabled);              \
        PTRACE("                            cuTreeSupport = %d\n", info->cuTreeSupport);           \
        PTRACE("                      maxEncodedWidthHEVC = %d\n", info->maxEncodedWidthHEVC);     \
        PTRACE("                      maxEncodedWidthH264 = %d\n", info->maxEncodedWidthH264);     \
        PTRACE("                       maxEncodedWidthAV1 = %d\n", info->maxEncodedWidthAV1);      \
        PTRACE("                       maxEncodedWidthVP9 = %d\n", info->maxEncodedWidthVP9);      \
        PTRACE("                           videoHeightExt = %d\n", info->videoHeightExt);          \
        PTRACE("                      maxEncodedWidthJPEG = %d\n", info->maxEncodedWidthJPEG);     \
        PTRACE("                            bFrameEnabled = %d\n", info->bFrameEnabled);           \
        PTRACE("                            main10Enabled = %d\n", info->main10Enabled);           \
        PTRACE("                          RDOQSupportH264 = %d\n", info->RDOQSupportH264);         \
        PTRACE("                          RDOQSupportHEVC = %d\n", info->RDOQSupportHEVC);         \
        PTRACE("                           RDOQSupportAV1 = %d\n", info->RDOQSupportAV1);          \
        PTRACE("                           RDOQSupportVP9 = %d\n", info->RDOQSupportVP9);          \
        PTRACE("                         meHorSearchRange = %d\n", info->meHorSearchRange);        \
        PTRACE("                    meVertSearchRangeH264 = %d\n", info->meVertSearchRangeH264);   \
        PTRACE("                    meVertSearchRangeHEVC = %d\n", info->meVertSearchRangeHEVC);   \
        PTRACE("                              ssimSupport = %d\n", info->ssimSupport);             \
        PTRACE("                              psnrSupport = %d\n", info->psnrSupport);             \
        PTRACE("                          hevcTileSupport = %d\n", info->hevcTileSupport);         \
        PTRACE("                               saoSupport = %d\n", info->saoSupport);              \
        PTRACE("                                rfcEnable = %d\n", info->rfcEnable);               \
        PTRACE("                            roiMapVersion = %d\n", info->roiMapVersion);           \
        PTRACE("                           cuInforVersion = %d\n", info->cuInforVersion);          \
        PTRACE("                             ctbRcVersion = %d\n", info->ctbRcVersion);            \
        PTRACE("                            progRdoEnable = %d\n", info->progRdoEnable);           \
        PTRACE("                             ljpegSupport = %d\n", info->ljpegSupport);            \
        PTRACE("                        JpegRoiMapSupport = %d\n", info->JpegRoiMapSupport);       \
        PTRACE("                           jpeg400Support = %d\n", info->jpeg400Support);          \
        PTRACE("                           jpeg422Support = %d\n", info->jpeg422Support);          \
        PTRACE("                   meVertRangeProgramable = %d\n", info->meVertRangeProgramable);  \
        PTRACE("                        MonoChromeSupport = %d\n", info->MonoChromeSupport);       \
        PTRACE("                          intraTU32Enable = %d\n", info->intraTU32Enable);         \
        PTRACE("                        bMultiPassSupport = %d\n", info->bMultiPassSupport);       \
        PTRACE("                               tu32Enable = %d\n", info->tu32Enable);              \
        PTRACE("                         dynamicMaxTuSize = %d\n", info->dynamicMaxTuSize);        \
        PTRACE("                        CtbBitsOutSupport = %d\n", info->CtbBitsOutSupport);       \
        PTRACE("                     encVisualTuneSupport = %d\n", info->encVisualTuneSupport);    \
        PTRACE("                            ctbRcMoreMode = %d\n", info->ctbRcMoreMode);           \
        PTRACE("                        encPsyTuneSupport = %d\n", info->encPsyTuneSupport);       \
        PTRACE("                     maxRefNumList0Minus1 = %d\n", info->maxRefNumList0Minus1);    \
        PTRACE("                         encSliceSizeBits = %d\n", info->encSliceSizeBits);        \
        PTRACE("                            asymPuSupport = %d\n", info->asymPuSupport);           \
        PTRACE("                     maxMergeCand4Support = %d\n", info->maxMergeCand4Support);    \
        PTRACE("                  encStreamSegmentSupport = %d\n", info->encStreamSegmentSupport); \
        PTRACE("                             encAQInfoOut = %d\n", info->encAQInfoOut);            \
        PTRACE("                               rgbEnabled = %d\n", info->rgbEnabled);              \
        PTRACE("                       NonRotationSupport = %d\n", info->NonRotationSupport);      \
        PTRACE("                      NVFormatOnlySupport = %d\n", info->NVFormatOnlySupport);     \
        PTRACE("                            lineBufEnable = %d\n", info->lineBufEnable);           \
        PTRACE("                            inLoopDSRatio = %d\n", info->inLoopDSRatio);           \
        PTRACE("                         cscExtendSupport = %d\n", info->cscExtendSupport);        \
        PTRACE("                           scalingEnabled = %d\n", info->scalingEnabled);          \
        PTRACE("                         scaled420Support = %d\n", info->scaled420Support);        \
        PTRACE("                                vsSupport = %d\n", info->vsSupport);               \
        PTRACE("                               OSDSupport = %d\n", info->OSDSupport);              \
        PTRACE("                                maxOsdNum = %d\n", info->maxOsdNum);               \
        PTRACE("                            MosaicSupport = %d\n", info->MosaicSupport);           \
        PTRACE("                             maxMosaicNum = %d\n", info->maxMosaicNum);            \
        PTRACE("                     tile8x8FormatSupport = %d\n", info->tile8x8FormatSupport);    \
        PTRACE("                     tile4x4FormatSupport = %d\n", info->tile4x4FormatSupport);    \
        PTRACE("                  customTileFormatSupport = %d\n", info->customTileFormatSupport); \
        PTRACE("                    rgb24bitFormatSupport = %d\n", info->rgb24bitFormatSupport);   \
        PTRACE("                            prpSbiSupport = %d\n", info->prpSbiSupport);           \
        PTRACE("                         prpMirrorSupport = %d\n", info->prpMirrorSupport);        \
        PTRACE("                      meExternSramSupport = %d\n", info->meExternSramSupport);     \
        PTRACE("                                  busType = %d\n", info->busType);                 \
        PTRACE("                                 busWidth = %d\n", info->busWidth);                \
        PTRACE("                          maxAXIAlignment = %d\n", info->maxAXIAlignment);         \
        PTRACE("                axi_burst_align_wr_common = %d\n",                                 \
               info->axi_burst_align_wr_common);                                                   \
        PTRACE("                axi_burst_align_wr_stream = %d\n",                                 \
               info->axi_burst_align_wr_stream);                                                   \
        PTRACE("            axi_burst_align_wr_chroma_ref = %d\n",                                 \
               info->axi_burst_align_wr_chroma_ref);                                               \
        PTRACE("              axi_burst_align_wr_luma_ref = %d\n",                                 \
               info->axi_burst_align_wr_luma_ref);                                                 \
        PTRACE("                axi_burst_align_rd_common = %d\n",                                 \
               info->axi_burst_align_rd_common);                                                   \
        PTRACE("                   axi_burst_align_rd_prp = %d\n", info->axi_burst_align_rd_prp);  \
        PTRACE("       axi_burst_align_rd_ch_ref_prefetch = %d\n",                                 \
               info->axi_burst_align_rd_ch_ref_prefetch);                                          \
        PTRACE("       axi_burst_align_rd_lu_ref_prefetch = %d\n",                                 \
               info->axi_burst_align_rd_lu_ref_prefetch);                                          \
        PTRACE("                axi_burst_align_wr_cuinfo = %d\n",                                 \
               info->axi_burst_align_wr_cuinfo);                                                   \
        PTRACE("                        h264NalRefIdc2bit = %d\n", info->h264NalRefIdc2bit);       \
        PTRACE("                         av1ExtensionFlag = %d\n", info->av1ExtensionFlag);        \
        PTRACE("                          roiAbsQpSupport = %d\n", info->roiAbsQpSupport);         \
        PTRACE("                              ROI8Support = %d\n", info->ROI8Support);             \
        PTRACE("                          h264CavlcEnable = %d\n", info->h264CavlcEnable);         \
        PTRACE("                         refRingBufEnable = %d\n", info->refRingBufEnable);        \
        PTRACE("                      disableRecWtSupport = %d\n", info->disableRecWtSupport);     \
        PTRACE("                     backgroundDetSupport = %d\n", info->backgroundDetSupport);    \
        PTRACE("                           P010RefSupport = %d\n", info->P010RefSupport);          \
        PTRACE("                           deNoiseEnabled = %d\n", info->deNoiseEnabled);          \
        PTRACE("                        streamBufferChain = %d\n", info->streamBufferChain);       \
        PTRACE("                               gmvSupport = %d\n", info->gmvSupport);              \
        PTRACE("                             IPCM8Support = %d\n", info->IPCM8Support);            \
        PTRACE("                av1InterpFilterSwitchable = %d\n",                                 \
               info->av1InterpFilterSwitchable);                                                   \
        PTRACE("                               IframeOnly = %d\n", info->IframeOnly);              \
        PTRACE("                        dynamicRdoSupport = %d\n", info->dynamicRdoSupport);       \
        PTRACE("                     tuneToolsSet2Support = %d\n", info->tuneToolsSet2Support);    \
        PTRACE("                       temporalMvpSupport = %d\n", info->temporalMvpSupport);      \
        PTRACE("                       streamMultiSegment = %d\n", info->streamMultiSegment);      \
    } while (0)

#ifdef __cplusplus
}
#endif

#endif /* EWL_HWCFG_H */
