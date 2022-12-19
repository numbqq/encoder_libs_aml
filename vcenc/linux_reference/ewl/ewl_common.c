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
--  Abstract : Encoder Wrapper Layer, common parts
--
------------------------------------------------------------------------------*/
#include "ewl.h"
#include "enc_log.h"

#undef PTRACE
#include <stdio.h>
static const char *busTypeName[7] = {"UNKNOWN", "AHB", "OCP", "AXI", "PCI", "AXIAHB", "AXIAPB"};
#define PTRACE(...)                                                                                \
    do {                                                                                           \
        VCEncTraceMsg(NULL, VCENC_LOG_INFO, VCENC_LOG_TRACE_EWL, ##__VA_ARGS__);                   \
    } while (0)
#include "ewl_common.h"

/**
 * Get Core Signature which can identify the feature of the core.
 *
 * \param [in] regs array which contain the register values read from certain core.
 * \param [out] signature filled with the signature register values.
 * \return EWL_OK valid signature is filled.
 * \return EWL_ERROR invalid signature
 */
i32 EWLGetCoreSignature(u32 *regs, EWLCoreSignature_t *signature)
{
    if (!regs || !signature)
        return EWL_ERROR;

    u32 hw_id = signature->hw_asic_id = regs[0];

    signature->hw_build_id = regs[HWIF_REG_BUILD_ID];
    signature->fuse[0] = regs[HWIF_REG_BUILD_REV];

    signature->fuse[1] = regs[HWIF_REG_CFG1];
    signature->fuse[2] = (hw_id >= 0x80006001) ? regs[HWIF_REG_CFG2] : 0;
    signature->fuse[3] = (hw_id >= 0x80006010) ? regs[HWIF_REG_CFG3] : 0;
    signature->fuse[4] = (hw_id >= 0x80006010) ? regs[HWIF_REG_CFG4] : 0;
    signature->fuse[5] = (hw_id >= 0x80006010) ? regs[HWIF_REG_CFG5] : 0;
    signature->fuse[6] = (hw_id >= 0x80006010) ? regs[HWIF_REG_CFGAXI] : 0;

    return EWL_OK;
}

/**
 * Parse the Core Configure according to the fuse registers signature.
 *
 * \pre signatrue->hw_build_id is zero, it indicates that the core is a legacy core
 *      while the BUILD_ID is not defined.
 * \param [in] signature filled with the signature register values.
 * \param [out] cfg_info the feature list will be filled into this data structure.
 * \return EWL_OK valid feature list is filled.
 * \return EWL_ERROR erros happens when get the feature list
 */
static i32 EWLParseCoreConfig(EWLCoreSignature_t *signature, EWLHwConfig_t *cfg_info)
{
    if (!signature || !cfg_info)
        return EWL_ERROR;

    assert(signature->hw_build_id == 0x0);

    memset(cfg_info, 0, sizeof(EWLHwConfig_t));

    u32 hw_id = signature->hw_asic_id;
    u32 cfgval;

    cfg_info->hw_asic_id = signature->hw_asic_id;

    if (!cfg_info->encSliceSizeBits)
        cfg_info->encSliceSizeBits = 7;
    cfg_info->saoSupport = 1;

    cfgval = signature->fuse[1];
    cfg_info->h264Enabled = (cfgval >> 31) & 1;
    cfg_info->scalingEnabled = (cfgval >> 30) & 1;
    cfg_info->bFrameEnabled = (cfgval >> 29) & 1;
    cfg_info->rgbEnabled = (cfgval >> 28) & 1;
    cfg_info->hevcEnabled = (cfgval >> 27) & 1;
    cfg_info->vp9Enabled = (cfgval >> 26) & 1;
    cfg_info->deNoiseEnabled = (cfgval >> 25) & 1;
    cfg_info->main10Enabled = (cfgval >> 24) & 1;
    cfg_info->busType = (cfgval >> 21) & 7;
    cfg_info->h264CavlcEnable = (cfgval >> 20) & 1;
    cfg_info->lineBufEnable = (cfgval >> 19) & 1;
    cfg_info->progRdoEnable = (cfgval >> 18) & 1;
    cfg_info->rfcEnable = (cfgval >> 17) & 1;
    cfg_info->tu32Enable = (cfgval >> 16) & 1;
    cfg_info->jpegEnabled = (cfgval >> 15) & 1;
    cfg_info->busWidth = (cfgval >> 13) & 3;
    cfg_info->maxEncodedWidthH264 = cfg_info->maxEncodedWidthJPEG = cfg_info->maxEncodedWidthHEVC =
        cfgval & ((1 << 13) - 1);

    if (hw_id >= 0x80006001) {
        cfgval = signature->fuse[2];
        cfg_info->ljpegSupport = (cfgval >> 31) & 1;
        cfg_info->roiAbsQpSupport = (cfgval >> 30) & 1;
        cfg_info->intraTU32Enable = (cfgval >> 29) & 1;
        cfg_info->roiMapVersion = (cfgval >> 26) & 7;
        if (hw_id >= 0x80006010) {
            cfg_info->maxEncodedWidthHEVC <<= 3;
            cfg_info->maxEncodedWidthH264 = ((cfgval >> 13) & ((1 << 13) - 1)) << 3;
            cfg_info->maxEncodedWidthJPEG = (cfgval & ((1 << 13) - 1)) << 3;
        }
    }

    if (hw_id >= 0x80006010) {
        cfgval = signature->fuse[3];
        cfg_info->ssimSupport = (cfgval >> 31) & 1;
        cfg_info->P010RefSupport = (cfgval >> 30) & 1;
        cfg_info->cuInforVersion = (cfgval >> 27) & 7;
        cfg_info->meVertSearchRangeHEVC = (cfgval >> 21) & 0x3f;
        cfg_info->meVertSearchRangeH264 = (cfgval >> 15) & 0x3f;
        cfg_info->ctbRcVersion = (cfgval >> 12) & 7;
        cfg_info->jpeg422Support = (cfgval >> 11) & 1;
        cfg_info->gmvSupport = (cfgval >> 10) & 1;
        cfg_info->ROI8Support = (cfgval >> 9) & 1;
        cfg_info->meHorSearchRange = (cfgval >> 7) & 3;
        cfg_info->RDOQSupportHEVC = (cfgval >> 6) & 1;
        cfg_info->bMultiPassSupport = (cfgval >> 5) & 1;
        cfg_info->inLoopDSRatio = (cfgval >> 4) & 1;
        cfg_info->streamBufferChain = (cfgval >> 3) & 1;
        cfg_info->streamMultiSegment = (cfgval >> 2) & 1;
        cfg_info->IframeOnly = (cfgval >> 1) & 1;
        cfg_info->dynamicMaxTuSize = (cfgval & 1);
    }

    if (hw_id >= 0x80006010) {
        cfgval = signature->fuse[4];
        cfg_info->videoHeightExt = (cfgval >> 31) & 1;
        cfg_info->cscExtendSupport = (cfgval >> 30) & 1;
        cfg_info->scaled420Support = (cfgval >> 29) & 1;
        cfg_info->cuTreeSupport = (cfgval >> 28) & 1;
        cfg_info->maxAXIAlignment = (cfgval >> 24) & 0xf;
        cfg_info->ctbRcMoreMode = (cfgval >> 23) & 1;
        cfg_info->meVertRangeProgramable = (cfgval >> 22) & 1;
        cfg_info->MonoChromeSupport = (cfgval >> 21) & 1;
        cfg_info->meExternSramSupport = (cfgval >> 20) & 1;
        cfg_info->vsSupport = (cfgval >> 19) & 1;
        cfg_info->RDOQSupportH264 = (cfgval >> 18) & 1;
        cfg_info->disableRecWtSupport = (cfgval >> 17) & 1;
        cfg_info->OSDSupport = (cfgval >> 16) & 1;
        cfg_info->h264NalRefIdc2bit = (cfgval >> 15) & 1;
        cfg_info->dynamicRdoSupport = (cfgval >> 14) & 1;
        cfg_info->av1Enabled = (cfgval >> 13) & 1;
        cfg_info->maxEncodedWidthAV1 = cfgval & 0x1fff;

        cfgval = signature->fuse[5];
        cfg_info->RDOQSupportAV1 = (cfgval >> 31) & 1;
        cfg_info->av1InterpFilterSwitchable = (cfgval >> 30) & 1;
        cfg_info->JpegRoiMapSupport = (cfgval >> 29) & 1;
        cfg_info->backgroundDetSupport = (cfgval >> 28) & 1;
        cfg_info->RDOQSupportVP9 = (cfgval >> 27) & 1;
        cfg_info->CtbBitsOutSupport = (cfgval >> 26) & 1;
        cfg_info->encVisualTuneSupport = (cfgval >> 25) & 1;
        cfg_info->encPsyTuneSupport = (cfgval >> 24) & 1;
        cfg_info->NonRotationSupport = (cfgval >> 23) & 1;
        cfg_info->NVFormatOnlySupport = (cfgval >> 22) & 1;
        cfg_info->MosaicSupport = (cfgval >> 21) & 1;
        cfg_info->IPCM8Support = (cfgval >> 20) & 1;
        cfg_info->psnrSupport = (cfgval >> 18) & 1;
        cfg_info->prpSbiSupport = (cfgval >> 17) & 1;

        cfgval = signature->fuse[6];
        cfg_info->axi_burst_align_wr_common = (cfgval >> 28) & 0xf;
        cfg_info->axi_burst_align_wr_stream = (cfgval >> 24) & 0xf;
        cfg_info->axi_burst_align_wr_chroma_ref = (cfgval >> 20) & 0xf;
        cfg_info->axi_burst_align_wr_luma_ref = (cfgval >> 16) & 0xf;
        cfg_info->axi_burst_align_rd_common = (cfgval >> 12) & 0xf;
        cfg_info->axi_burst_align_rd_prp = (cfgval >> 8) & 0xf;
        cfg_info->axi_burst_align_rd_ch_ref_prefetch = (cfgval >> 4) & 0xf;
        cfg_info->axi_burst_align_rd_lu_ref_prefetch = (cfgval)&0xf;
    }

    EWLShowHwConfig(cfg_info);

    return EWL_OK;
}

/**
 * Get the Core Configure according to the signature. The configure is a group of feature list.
 *
 * \param [in] signature filled with the signature register values.
 * \param [out] cfg_info the feature list will be filled into this data structure.
 * \return EWL_OK valid feature list is filled.
 * \return EWL_ERROR erros happens when get the feature list
 */
i32 EWLGetCoreConfig(EWLCoreSignature_t *signature, EWLHwConfig_t *cfg_info)
{
    static EWLHwConfig_t feature_list[] = {
#include EWLHWCFGFILE
    };
    EWLHwConfig_t *cfg = feature_list;
    int list_num = sizeof(feature_list) / sizeof(EWLHwConfig_t);
    u32 hw_build_id = signature->hw_build_id;
    int i;

/* current we still need to support legacy HW without BUILD_ID */
#define SUPPORT_NO_BUILD_ID

#ifdef SUPPORT_NO_BUILD_ID
    static EWLHwConfig_t legacy_list[EWL_MAX_CORES] = {{
                                                           .hw_build_id = 0,
                                                       },
                                                       {
                                                           .hw_build_id = 0,
                                                       },
                                                       {
                                                           .hw_build_id = 0,
                                                       },
                                                       {
                                                           .hw_build_id = 0,
                                                       }};
    static EWLCoreSignature_t signature_list[EWL_MAX_CORES] = {{
                                                                   .hw_build_id = 0,
                                                               },
                                                               {
                                                                   .hw_build_id = 0,
                                                               },
                                                               {
                                                                   .hw_build_id = 0,
                                                               },
                                                               {
                                                                   .hw_build_id = 0,
                                                               }};

    if (hw_build_id == 0) {
        //FIXME: protected this section with mutex!!!

        /* when BUILD_ID is not valid, need to parse the configure by fuse registers. */
        for (i = 0; i < EWL_MAX_CORES; i++) {
            if ((signature_list[i].hw_asic_id == signature->hw_asic_id) &&
                (0 ==
                 EWLmemcmp(signature_list[i].fuse, signature->fuse, sizeof(u32) * EWL_MAX_CORES))) {
                *cfg_info = legacy_list[i];
                return EWL_OK;
            }
        }

        assert(i == EWL_MAX_CORES);

        for (i = 0; i < EWL_MAX_CORES; i++) {
            if (signature_list[i].hw_build_id == 0)
                break;
        }

        if (i == EWL_MAX_CORES) {
            /* no empty slot for parsed configure */
            return EWL_ERROR;
        }

        EWLParseCoreConfig(signature, &legacy_list[i]);

        memcpy(&signature_list[i], signature, sizeof(*signature));
        signature_list[i].hw_build_id = VCE_BUILD_ID_PBASE + i;
        legacy_list[i].hw_build_id = VCE_BUILD_ID_PBASE + i;

        *cfg_info = legacy_list[i];
        return EWL_OK;
    }

#endif /* SUPPORT_NO_BUILD_ID */

    for (i = 0; i < list_num; i++) {
        if (hw_build_id == cfg->hw_build_id)
            break;
        cfg++;
    }

    if (i == list_num) {
        /* not found build id */
        return EWL_ERROR;
    }

    *cfg_info = *cfg;

    EWLShowHwConfig(cfg_info);

    return EWL_OK;
}
