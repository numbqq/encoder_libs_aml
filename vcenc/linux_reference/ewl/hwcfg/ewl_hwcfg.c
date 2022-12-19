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
--  Abstract : EWL Hardware Config for different target.
--
------------------------------------------------------------------------------*/

{
    .hw_build_id = 0x2110,
    .hw_asic_id = 0x90001000,
    .hevcEnabled = 1,
    .h264Enabled = 1,
    .jpegEnabled = 0,
    .vp9Enabled = 0,
    .av1Enabled = 0,
    .cuTreeSupport = 0,
    .maxEncodedWidthHEVC = 4096,
    .maxEncodedWidthH264 = 4096,
    .maxEncodedWidthAV1 = 0,
    .maxEncodedWidthVP9 = 0,
    .videoHeightExt = 0,
    .maxEncodedWidthJPEG = 16384,
    .bFrameEnabled = 0,
    .main10Enabled = 0,
    .RDOQSupportH264 = 0,
    .RDOQSupportHEVC = 0,
    .RDOQSupportAV1 = 0,
    .RDOQSupportVP9 = 0,
    .meHorSearchRange = 0,
    .meVertSearchRangeH264 = 3,
    .meVertSearchRangeHEVC = 5,
    .ssimSupport = 0,
    .psnrSupport = 0,
    .hevcTileSupport = 0,
    .saoSupport = 1,
    .rfcEnable = 1,
    .roiMapVersion = 2,
    .cuInforVersion = 1,
    .ctbRcVersion = 1,
    .progRdoEnable = 1,
    .ljpegSupport = 0,
    .JpegRoiMapSupport = 0,
    .jpeg400Support = 0,
    .jpeg422Support = 0,
    .meVertRangeProgramable = 0,
    .MonoChromeSupport = 0,
    .intraTU32Enable = 0,
    .bMultiPassSupport = 0,
    .tu32Enable = 1,
    .dynamicMaxTuSize = 1,
    .CtbBitsOutSupport = 0,
    .encVisualTuneSupport = 0,
    .ctbRcMoreMode = 1,
    .encPsyTuneSupport = 0,
    .maxRefNumList0Minus1 = 0,
    .encSliceSizeBits = 7,
    .asymPuSupport = 1,
    .maxMergeCand4Support = 0,
    .encStreamSegmentSupport = 0,
    .encAQInfoOut = 0,
    .rgbEnabled = 1,
    .NonRotationSupport = 0,
    .NVFormatOnlySupport = 0,
    .lineBufEnable = 1,
    .inLoopDSRatio = 1,
    .cscExtendSupport = 1,
    .scalingEnabled = 1,
    .scaled420Support = 1,
    .vsSupport = 0,
    .OSDSupport = 1,
    .maxOsdNum = 12,
    .MosaicSupport = 1,
    .maxMosaicNum = 12,
    .tile8x8FormatSupport = 1,
    .tile4x4FormatSupport = 1,
    .customTileFormatSupport = 0,
    .rgb24bitFormatSupport = 1,
    .prpSbiSupport = 1,
    .prpMirrorSupport = 0,
    .meExternSramSupport = 0,
    .busType = 6,
    .busWidth = 2,
    .maxAXIAlignment = 0,
    .axi_burst_align_wr_common = 0,
    .axi_burst_align_wr_stream = 0,
    .axi_burst_align_wr_chroma_ref = 0,
    .axi_burst_align_wr_luma_ref = 0,
    .axi_burst_align_rd_common = 0,
    .axi_burst_align_rd_prp = 0,
    .axi_burst_align_rd_ch_ref_prefetch = 0,
    .axi_burst_align_rd_lu_ref_prefetch = 0,
    .axi_burst_align_wr_cuinfo = 0,
    .h264NalRefIdc2bit = 1,
    .av1ExtensionFlag = 0,
    .roiAbsQpSupport = 1,
    .ROI8Support = 1,
    .h264CavlcEnable = 1,
    .refRingBufEnable = 1,
    .disableRecWtSupport = 1,
    .backgroundDetSupport = 0,
    .P010RefSupport = 0,
    .deNoiseEnabled = 0,
    .streamBufferChain = 1,
    .gmvSupport = 0,
    .IPCM8Support = 0,
    .av1InterpFilterSwitchable = 0,
    .IframeOnly = 0,
    .dynamicRdoSupport = 0,
    .tuneToolsSet2Support = 0,
    .temporalMvpSupport = 0,
    .streamMultiSegment = 0,
},
