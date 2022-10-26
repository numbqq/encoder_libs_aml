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

/*------------------------------------------------------------------------------
    Include headers
------------------------------------------------------------------------------*/
#include "encpreprocess.h"
#include "encasiccontroller.h"
#include "enccommon.h"
#include "ewl.h"
#include "buffer_info.h"

/*------------------------------------------------------------------------------
    External compiler flags
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    Module defines
------------------------------------------------------------------------------*/

/* Mask fields */
#define mask_2b (u32)0x00000003
#define mask_3b (u32)0x00000007
#define mask_4b (u32)0x0000000F
#define mask_5b (u32)0x0000001F
#define mask_6b (u32)0x0000003F
#define mask_11b (u32)0x000007FF
#define mask_14b (u32)0x00003FFF
#define mask_16b (u32)0x0000FFFF

#define HSWREG(n) ((n)*4)

/*------------------------------------------------------------------------------
    Local function prototypes
------------------------------------------------------------------------------*/
void EncAsicGetSizeAndSetRegs(asicData_s *asic, asicMemAlloc_s *allocCfg,
                              u32 *internalImageLumaSize, u32 *lumaSize, u32 *lumaSize4N,
                              u32 *chromaSize, u32 *u32FrameContextSize)
{
    u32 width = allocCfg->width;
    u32 height = allocCfg->height;
    bool codecH264 = allocCfg->encodingType == ASIC_H264;
    u32 alignment = allocCfg->ref_alignment;
    u32 alignment_ch = allocCfg->ref_ch_alignment;
    regValues_s *regs = &asic->regs;
    u32 width_4n, height_4n;

    width = ((width + 63) >> 6) << 6;
    height = ((height + 63) >> 6) << 6;
    //width_4n = width / 4;
    width_4n = ((allocCfg->width + 15) / 16) * 4;
    height_4n = height / 4;

    asic->regs.recon_chroma_half_size =
        ((width * height) + (width * height) * (allocCfg->bitDepthLuma - 8) / 8) / 4;

    /* 6.0 software:  merge*/
    *u32FrameContextSize = (regs->codingType == ASIC_AV1)
                               ? FRAME_CONTEXT_LENGTH
                               : (regs->codingType == ASIC_VP9) ? VP9_FRAME_CONTEXT_LENGTH : 0;
    if (HW_PRODUCT_SYSTEM60(asic->regs.asicHwId) ||
        HW_PRODUCT_VC9000LE(asic->regs.asicHwId)) /* VC8000E6.0 */
    {
        asic->regs.recon_chroma_half_size =
            (width * height + (width / 4) * height_4n) * (allocCfg->bitDepthLuma) / 8 / 4;
        asic->regs.ref_frame_stride =
            STRIDE(STRIDE(width * 4 * allocCfg->bitDepthLuma / 8, 16), alignment);
        *lumaSize = STRIDE(width * 4, alignment) * height / 4 +
                    (width * height) * (allocCfg->bitDepthLuma - 8) / 8;
        *lumaSize4N = STRIDE(width_4n * 4, alignment) * height_4n / 4 +
                      (width_4n * height_4n) * (allocCfg->bitDepthLuma - 8) / 8;
        *internalImageLumaSize = *lumaSize + *lumaSize4N;
        *chromaSize = (alignment == 1 ? *internalImageLumaSize / 2
                                      : STRIDE(width * 4, alignment) * height / 4);
    } else {
#ifdef RECON_REF_1KB_BURST_RW
        asic->regs.ref_frame_stride = STRIDE((STRIDE(width, 128) * 8), alignment);
        *lumaSize = asic->regs.ref_frame_stride * (height / (8 / (allocCfg->bitDepthLuma / 8)));
#else
        asic->regs.ref_frame_stride =
            STRIDE(STRIDE(width * 4 * allocCfg->bitDepthLuma / 8, 16), alignment);
        *lumaSize = asic->regs.ref_frame_stride * height / 4;
#endif
        *lumaSize4N = STRIDE(STRIDE(width_4n * 4 * allocCfg->bitDepthLuma / 8, 16), alignment) *
                      height_4n / 4;
        *internalImageLumaSize = ((*lumaSize + *lumaSize4N + 0xf) & (~0xf));
        *internalImageLumaSize = STRIDE(*internalImageLumaSize, alignment);
        *chromaSize = *lumaSize / 2;
#ifdef RECON_REF_1KB_BURST_RW
        if (allocCfg->compressor & 2) {
            *chromaSize =
                STRIDE((width * 4 * allocCfg->bitDepthChroma / 8), alignment_ch) * (height / 8);
            asic->regs.recon_chroma_half_size = *chromaSize / 2;
        }
#endif
    }
}

/*------------------------------------------------------------------------------
    Local function prototypes
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------

    EncAsicMemAlloc_V2

    Allocate HW/SW shared memory

    Input:
        asic        asicData structure
        width       width of encoded image, multiple of four
        height      height of encoded image
        numRefBuffs amount of reference luma frame buffers to be allocated

    Output:
        asic        base addresses point to allocated memory areas

    Return:
        ENCHW_OK        Success.
        ENCHW_NOK       Error: memory allocation failed, no memories allocated
                        and EWL instance released

------------------------------------------------------------------------------*/
i32 EncAsicMemAlloc_V2(asicData_s *asic, asicMemAlloc_s *allocCfg)
{
    u32 i;
    u32 width = allocCfg->width;
    u32 height = allocCfg->height;
    bool codecH264 = allocCfg->encodingType == ASIC_H264;
    u32 alignment = allocCfg->ref_alignment;
    u32 alignment_ch = allocCfg->ref_ch_alignment;
    regValues_s *regs;
    EWLLinearMem_t *buff = NULL;
    u32 internalImageLumaSize, u32FrameContextSize, RefRingBufSize;
    u32 total_allocated_buffers;
    u32 width_4n, height_4n;
    u32 block_unit, block_size;
    //u32 maxSliceNals;
    u32 log2_ctu_size = (codecH264 ? 4 : 6);
    u32 ctu_size = (1 << log2_ctu_size);
    i32 ctb_per_row = ((width + ctu_size - 1) / ctu_size);
    i32 ctb_per_column = ((height + ctu_size - 1) / ctu_size);
    i32 ctb_per_picture = ctb_per_row * ctb_per_column;

    ASSERT(asic != NULL);
    ASSERT(width != 0);
    ASSERT(height != 0);
    ASSERT((height % 2) == 0);
    ASSERT((width % 2) == 0);
    //scaledWidth=((scaledWidth+7)/8)*8;
    regs = &asic->regs;

    regs->codingType = allocCfg->encodingType;

    if (allocCfg->encodingType == ASIC_JPEG || allocCfg->encodingType == ASIC_CUTREE)
        return ENCHW_OK;

    /*
    allocate scan coeff buffer before reference frame to avoid roimap overeading(64~1536 bytes) hit reference frame:
    reference frame could be in DEC400(1024B burst size)
   */
    for (i = 0; i < MAX_CORE_NUM; i++) {
        asic->compress_coeff_SCAN[i].mem_type = CPU_RD | VPU_WR | VPU_RD | EWL_MEM_TYPE_VPU_WORKING;
        if (allocCfg->is_malloc) {
            if (EWLMallocLinear(asic->ewl, 288 * 1024 / 8, alignment,
                                &asic->compress_coeff_SCAN[i]) != EWL_OK) {
                EncAsicMemFree_V2(asic);
                return ENCHW_NOK;
            }
        }
    }

    if (regs->tiles_enabled_flag) {
        if (regs->num_tile_columns > 1) {
            asic->deblockCtx.mem_type = EWL_MEM_TYPE_VPU_ONLY;
            /* 4-byte syncword for each tile column sync buffer */
            if (EWLMallocLinear(
                    asic->ewl,
                    (regs->num_tile_columns - 1) *
                            (DEB_TILE_SYNC_SIZE + SAODEC_TILE_SYNC_SIZE + SAOPP_TILE_SYNC_SIZE) *
                            ctb_per_column +
                        4 * (regs->num_tile_columns - 1),
                    1, &asic->deblockCtx) != EWL_OK) {
                EncAsicMemFree_V2(asic);
                return ENCHW_NOK;
            }
        }

        asic->tileHeightMem.mem_type = EWL_MEM_TYPE_VPU_ONLY;
        if (EWLMallocLinear(asic->ewl, regs->num_tile_rows * sizeof(u16), 1,
                            &asic->tileHeightMem) != EWL_OK) {
            EncAsicMemFree_V2(asic);
            return ENCHW_NOK;
        }
    }

    ASSERT(allocCfg->numRefBuffsLum < ASIC_FRAME_BUF_LUM_MAX);
    total_allocated_buffers = allocCfg->numRefBuffsLum + 1;

    if (allocCfg->encodingType == ASIC_VP9 && allocCfg->tmvpEnable)
        total_allocated_buffers += 1;

    width = ((width + 63) >> 6) << 6;
    height = ((height + 63) >> 6) << 6;
    //width_4n = width / 4;
    width_4n = ((allocCfg->width + 15) / 16) * 4;
    height_4n = height / 4;

    /* 6.0 software:  merge*/
    u32 lumaSize = 0;
    u32 lumaSize4N = 0;
    u32 chromaSize = 0;

    EncAsicGetSizeAndSetRegs(asic, allocCfg, &internalImageLumaSize, &lumaSize, &lumaSize4N,
                             &chromaSize, &u32FrameContextSize);

    {
        for (i = 0; i < total_allocated_buffers; i++) {
            if (!allocCfg->exteralReconAlloc) {
                asic->internalreconLuma[i].mem_type = VPU_WR | VPU_RD | EWL_MEM_TYPE_DPB;
#ifdef TEST_DATA
                asic->internalreconLuma[i].mem_type |= CPU_RD;
#endif
#ifdef INTERNAL_TEST
                asic->internalreconLuma[i].mem_type |= CPU_WR;
#endif
                if (allocCfg->is_malloc) {
                    if (!allocCfg->refRingBufEnable) {
                        if (EWLMallocRefFrm(asic->ewl, internalImageLumaSize, alignment,
                                            &asic->internalreconLuma[i]) != EWL_OK)
                            goto mem_error;
                    }
                    /* internalFrameContext buffer */
                    if (u32FrameContextSize != 0) {
                        if (regs->codingType == ASIC_AV1)
                            asic->internalFrameContext[i].mem_type =
                                CPU_WR | VPU_WR | VPU_RD | EWL_MEM_TYPE_VPU_WORKING;
                        else if (regs->codingType == ASIC_VP9)
                            asic->internalFrameContext[i].mem_type =
                                CPU_WR | CPU_RD | VPU_WR | VPU_RD | EWL_MEM_TYPE_VPU_WORKING;
                        if (EWLMallocLinear(asic->ewl, u32FrameContextSize, alignment,
                                            &asic->internalFrameContext[i]) != EWL_OK)
                            goto mem_error;
                    }
                }
                if (!allocCfg->refRingBufEnable) {
                    asic->internalreconLuma_4n[i].busAddress =
                        asic->internalreconLuma[i].busAddress + lumaSize;
                    asic->internalreconLuma_4n[i].size = lumaSize4N;
                }
                asic->internalFrameContext[i].size = u32FrameContextSize;
            }
        }

        /* mc_sync_word buffer */
        if (asic->regs.mc_sync_enable) {
            u32 total_mc_sync_word = 0;
            if (!allocCfg->exteralReconAlloc) {
                if (allocCfg->is_malloc) {
                    /* do the unify allocation to save buffer asap */
                    total_mc_sync_word = total_allocated_buffers * MC_SYNC_WORD_BYTES;
                    asic->mc_sync_word[0].mem_type =
                        CPU_WR | VPU_WR | VPU_RD | EWL_MEM_TYPE_VPU_WORKING;
                    if (EWLMallocLinear(asic->ewl, total_mc_sync_word, alignment,
                                        &asic->mc_sync_word[0]) != EWL_OK)
                        goto mem_error;

                    for (i = 1; i < total_allocated_buffers; i++) {
                        asic->mc_sync_word[i].mem_type =
                            CPU_WR | VPU_WR | VPU_RD | EWL_MEM_TYPE_VPU_WORKING;
                        asic->mc_sync_word[i].size = MC_SYNC_WORD_BYTES;
                        asic->mc_sync_word[i].busAddress =
                            asic->mc_sync_word[0].busAddress + i * MC_SYNC_WORD_BYTES;
                        asic->mc_sync_word[i].virtualAddress =
                            (u32 *)((u8 *)asic->mc_sync_word[0].virtualAddress +
                                    i * MC_SYNC_WORD_BYTES);
                    }
                }
            }
        }

        for (i = 0; i < total_allocated_buffers; i++) {
            if (!allocCfg->exteralReconAlloc) {
                asic->internalreconChroma[i].mem_type = VPU_WR | VPU_RD | EWL_MEM_TYPE_DPB;
#ifdef TEST_DATA
                asic->internalreconChroma[i].mem_type |= CPU_RD;
#endif
#ifdef INTERNAL_TEST
                asic->internalreconChroma[i].mem_type |= CPU_WR;
#endif
                if (allocCfg->is_malloc) {
                    if (!allocCfg->refRingBufEnable) {
                        if (EWLMallocRefFrm(asic->ewl, chromaSize, alignment_ch,
                                            &asic->internalreconChroma[i]) != EWL_OK)
                            goto mem_error;
                    }
                }
            }
        }

        asic->regs.ref_ds_luma_stride =
            STRIDE(STRIDE(width_4n * 4 * allocCfg->bitDepthLuma / 8, 16), alignment);
        asic->regs.ref_frame_stride_ch =
            STRIDE((width * 4 * allocCfg->bitDepthChroma / 8), alignment_ch);

        if (allocCfg->refRingBufEnable) {
            u32 lumaBufSize =
                lumaSize + allocCfg->RefRingBufExtendHeight * asic->regs.ref_frame_stride / 4;
            u32 chromaBufSize = chromaSize + allocCfg->RefRingBufExtendHeight / 2 *
                                                 asic->regs.ref_frame_stride_ch / 4;
            u32 lumaBufSize4N = lumaSize4N + allocCfg->RefRingBufExtendHeight / 4 *
                                                 asic->regs.ref_ds_luma_stride / 4;
            RefRingBufSize = lumaBufSize + chromaBufSize + lumaBufSize4N;
            if (EWLMallocRefFrm(asic->ewl, RefRingBufSize, alignment, &asic->RefRingBuf) != EWL_OK)
                goto mem_error;
            if (EWLMallocRefFrm(asic->ewl, RefRingBufSize, alignment, &asic->RefRingBufBak) !=
                EWL_OK)
                goto mem_error;
            regs->refRingBuf_luma_size = lumaBufSize;
            regs->refRingBuf_chroma_size = chromaBufSize;
            regs->refRingBuf_4n_size = lumaBufSize4N;
        }

        /*  VP9: The table is used for probability tables, 1208 bytes. */
        if (regs->codingType == ASIC_VP9) {
            i = 8 * 55 + 8 * 96;
            asic->cabacCtx.mem_type = VPU_WR | VPU_RD | EWL_MEM_TYPE_VPU_WORKING;
            if (allocCfg->is_malloc) {
                if (EWLMallocLinear(asic->ewl, i, 0, &asic->cabacCtx) != EWL_OK)
                    goto mem_error;
            }
        }

        regs->cabacCtxBase = asic->cabacCtx.busAddress;

        if (regs->codingType == ASIC_VP9) {
            /* VP9: Table of counter for probability updates. */
            asic->probCount.mem_type = VPU_WR | VPU_RD | EWL_MEM_TYPE_VPU_WORKING;
            if (allocCfg->is_malloc) {
                if (EWLMallocLinear(asic->ewl, ASIC_VP9_PROB_COUNT_SIZE, 0, &asic->probCount) !=
                    EWL_OK)
                    goto mem_error;
            }
            regs->probCountBase = asic->probCount.busAddress;
        }

        /* NAL size table, table size must be 64-bit multiple,
     * space for SEI, MVC prefix, filler and zero at the end of table.
     * Atleast 1 macroblock row in every slice.
     */

        asic->sizeTblSize =
            EncGetSizeTblSize(height, allocCfg->tileEnable, allocCfg->numTileRows,
                              allocCfg->encodingType, allocCfg->maxTemporalLayers, alignment);
        u32 sizeTbleNum =
            allocCfg->tileEnable == 1 ? allocCfg->numTileColumns : MAX_CORE_NUM; //enable
        for (i = 0; i < sizeTbleNum; i++) {
            buff = &asic->sizeTbl[i];
            buff->mem_type = VPU_WR | CPU_WR | CPU_RD | EWL_MEM_TYPE_VPU_WORKING;
            if (allocCfg->is_malloc) {
                if (EWLMallocLinear(asic->ewl, asic->sizeTblSize, alignment, buff) != EWL_OK)
                    goto mem_error;
            }
        }

        /* Ctb RC */
        if (allocCfg->ctbRcMode & 2) {
            i32 ctbRcMadsize = width * height / ctu_size / ctu_size; // 1 byte per ctb
            /* for multi-core case, just allocate enough space for alignment requirement */
            if (allocCfg->tileEnable && allocCfg->numTileColumns)
                ctbRcMadsize += alignment * (allocCfg->numTileColumns - 1);
            if (EncAsicGetBuffers(asic, asic->ctbRcMem, CTB_RC_BUF_NUM, ctbRcMadsize, alignment) !=
                ENCHW_OK)
                goto mem_error;
        }

        //allocate compressor table
        if (allocCfg->compressor) {
            u32 tblLumaSize = 0;
            u32 tblChromaSize = 0;
            u32 tblSize = 0;
            EncGetCompressTableSize(allocCfg->compressor, width, height, &tblLumaSize,
                                    &tblChromaSize);

            tblSize = tblLumaSize + tblChromaSize;

            for (i = 0; i < total_allocated_buffers; i++) {
                if (!allocCfg->exteralReconAlloc) {
                    asic->compressTbl[i].mem_type = VPU_WR | VPU_RD | EWL_MEM_TYPE_VPU_WORKING;
#ifdef TEST_DATA
                    asic->compressTbl[i].mem_type |= CPU_RD;
#endif
#ifdef INTERNAL_TEST
                    asic->compressTbl[i].mem_type |= CPU_WR;
#endif
                    if (allocCfg->is_malloc) {
                        if (EWLMallocLinear(asic->ewl, tblSize, alignment, &asic->compressTbl[i]) !=
                            EWL_OK)
                            goto mem_error;
                    }
                }
            }
        }

        //allocate collocated buffer for H.264
        if (codecH264) {
            // 4 bits per mb, 2 mbs per struct h264_mb_col
            u32 bufSize = (ctb_per_picture + 1) / 2 * sizeof(struct h264_mb_col);

            for (i = 0; i < total_allocated_buffers; i++) {
                if (!allocCfg->exteralReconAlloc) {
                    asic->colBuffer[i].mem_type = VPU_WR | VPU_RD | EWL_MEM_TYPE_VPU_WORKING;
                    if (allocCfg->is_malloc) {
                        if (EWLMallocLinear(asic->ewl, bufSize, alignment, &asic->colBuffer[i]) !=
                            EWL_OK)
                            goto mem_error;
                    }
                }
            }
        }

        /* allocate mv info buffer for tmvp */
        if (allocCfg->tmvpEnable) {
            /* Aligned to CTB.
         For every 4x4:
         2 bits pred direction + 2 bits pred mode + mv(m_iHor + m_iVer)*/
            u32 bufSize =
                (STRIDE(width, 64) / 4) * (STRIDE(height, 64) / 4) * sizeof(struct MvInfo);
            for (i = 0; i < total_allocated_buffers; i++) {
                if (!allocCfg->exteralReconAlloc) {
                    asic->mvInfo[i].mem_type = VPU_WR | VPU_RD | EWL_MEM_TYPE_VPU_WORKING;
                    if (allocCfg->is_malloc) {
                        if (EWLMallocLinear(asic->ewl, bufSize, alignment, &asic->mvInfo[i]) !=
                            EWL_OK) {
                            goto mem_error;
                        }
                    }
                }
            }
        }

        /* allocation CU information memory */
        if (allocCfg->outputCuInfo) {
            i32 cuInfoSizes[] = {CU_INFO_OUTPUT_SIZE, CU_INFO_OUTPUT_SIZE_V1,
                                 CU_INFO_OUTPUT_SIZE_V2, CU_INFO_OUTPUT_SIZE_V3};
            i32 ctuNum = (width >> log2_ctu_size) * (height >> log2_ctu_size);
            i32 maxCuNum = ctuNum * (ctu_size / 8) * (ctu_size / 8);
            i32 cuInfoVersion = regs->asicCfg.cuInforVersion;
            u32 cuinfoStride =
                (((allocCfg->width + 15) >> 4 << 4) + ((1 << allocCfg->cuinfoAlignment) - 1)) >>
                allocCfg->cuinfoAlignment << allocCfg->cuinfoAlignment;
            asic->regs.cuinfoStride = cuinfoStride;
            if (cuInfoVersion == 2) {
                cuInfoVersion =
                    (regs->asicCfg.bMultiPassSupport && allocCfg->pass == 1 ? cuInfoVersion : 1);
                maxCuNum = (cuInfoVersion == 2)
                               ? ((cuinfoStride / 8) * ((allocCfg->height + 15) / 16 * 16 / 8))
                               : maxCuNum;
            }

            i32 infoSizePerCu = cuInfoSizes[cuInfoVersion];
            i32 cuInfoTblSize = ctuNum * CU_INFO_TABLE_ITEM_SIZE;
            i32 cuInfoSize = maxCuNum * infoSizePerCu;
            i32 cuInfoTotalSize = 0;
            i32 widthInUnit = (allocCfg->width + 15) / 16;
            i32 heightInUnit = (allocCfg->height + 15) / 16;
            i32 aqInfoStride = 0;
            i32 aqInfoSize = 0;
            if (cuInfoVersion == 2) {
                aqInfoStride = STRIDE(widthInUnit * sizeof(uint32_t), allocCfg->aqInfoAlignment);
                aqInfoSize = (heightInUnit + 1) * aqInfoStride; // extra row for avgAdj & strength
            }

            cuInfoTblSize = ((cuInfoTblSize + ctu_size - 1) >> log2_ctu_size) << log2_ctu_size;
            cuInfoSize = ((cuInfoSize + ctu_size - 1) >> log2_ctu_size) << log2_ctu_size;
            cuInfoTotalSize = cuInfoTblSize + aqInfoSize + cuInfoSize;
            asic->cuInfoTableSize = cuInfoTblSize;
            asic->aqInfoSize = aqInfoSize;
            asic->aqInfoStride = aqInfoStride;

            if (!allocCfg->exteralReconAlloc) {
                asic->cuInfoMem[0].mem_type = VPU_WR | VPU_RD | CPU_RD | EWL_MEM_TYPE_VPU_WORKING;
                if (allocCfg->is_malloc) {
                    if (EWLMallocLinear(asic->ewl, cuInfoTotalSize * allocCfg->numCuInfoBuf,
                                        alignment, &asic->cuInfoMem[0]) != EWL_OK)
                        goto mem_error;
                }
                i32 total_size = asic->cuInfoMem[0].size;
                for (i = 0; i < allocCfg->numCuInfoBuf; i++) {
                    asic->cuInfoMem[i].virtualAddress =
                        (u32 *)((ptr_t)asic->cuInfoMem[0].virtualAddress + i * cuInfoTotalSize);
                    asic->cuInfoMem[i].busAddress =
                        asic->cuInfoMem[0].busAddress + i * cuInfoTotalSize;
                    asic->cuInfoMem[i].size =
                        (i < allocCfg->numCuInfoBuf - 1
                             ? cuInfoTotalSize
                             : total_size - (allocCfg->numCuInfoBuf - 1) * cuInfoTotalSize);
                }
            }
        }

        /* Allocate ctb encoded bits memory */
        if (allocCfg->outputCtbBits) {
            /* 2bytes per ctb to store bits info */
            i32 ctbBitsSize = ctb_per_picture * 2;
            /* for multi-core case, just allocate enough space for alignment requirement */
            if (allocCfg->tileEnable && allocCfg->numTileColumns)
                ctbBitsSize += alignment * (allocCfg->numTileColumns - 1);

            for (i = 0; i < allocCfg->numCtbBitsBuf; i++) {
                buff = &asic->ctbBitsMem[i];
                buff->mem_type = VPU_WR | CPU_RD | EWL_MEM_TYPE_VPU_WORKING;
                if (allocCfg->is_malloc) {
                    if (EWLMallocLinear(asic->ewl, ctbBitsSize, alignment, buff) != EWL_OK)
                        goto mem_error;
                }
            }
        }

        /* Allocate pre carry buffer for av1*/
        if (ASIC_AV1 == regs->codingType) {
            i32 av1PreCarrybufSize = width * height * 3 / 2;
            av1PreCarrybufSize = (av1PreCarrybufSize << 1) + 64; // 64 for HW sync words
            for (i = 0; i < allocCfg->numAv1PreCarryBuf; i++) {
                buff = &asic->av1PreCarryBuf[i];
                buff->mem_type = VPU_WR | VPU_RD | EWL_MEM_TYPE_VPU_WORKING;
                if (EWLMallocLinear(asic->ewl, av1PreCarrybufSize, alignment, buff) != EWL_OK)
                    goto mem_error;
            }
        }
    }
    return ENCHW_OK;

mem_error:
    EncAsicMemFree_V2(asic);
    return ENCHW_NOK;
}

/*------------------------------------------------------------------------------

    EncAsicGetBuffers

    Get HW/SW shared memory

    Input:
        asic        asicData structure
        buf         Allocated linear memory area information
        bufNum      the number of buffers
        bufSize     buffer size in bytes
        alignment   alignment of buffer address

    Output:
        buf         Allocated linear memory area information

    Return:
        ENCHW_OK        Success.
        ENCHW_NOK       Error: memory allocation failed, no memories allocated
                        and EWL instance released

------------------------------------------------------------------------------*/
i32 EncAsicGetBuffers(asicData_s *asic, EWLLinearMem_t *buf, i32 bufNum, i32 bufSize, i32 alignment)
{
    for (i32 i = 0; i < bufNum; i++) {
        buf[i].mem_type = VPU_WR | VPU_RD | EWL_MEM_TYPE_VPU_WORKING;

        if (buf[i].busAddress == 0) {
            if (EWLMallocLinear(asic->ewl, bufSize, alignment, &(buf[i])) != EWL_OK) {
                for (i32 j = 0; j < i; j++)
                    EWLFreeLinear(asic->ewl, &(buf[j]));

                return ENCHW_NOK;
            }
        }
    }
    return ENCHW_OK;
}

/*------------------------------------------------------------------------------

    EncAsicMemFree_V2

    Free HW/SW shared memory

------------------------------------------------------------------------------*/
void EncAsicMemFree_V2(asicData_s *asic)
{
    i32 i;

    ASSERT(asic != NULL);
    ASSERT(asic->ewl != NULL);

    for (i = 0; i < ASIC_FRAME_BUF_LUM_MAX; i++) {
        EWLFreeRefFrm(asic->ewl, &asic->internalreconLuma[i]);

        EWLFreeLinear(asic->ewl, &asic->internalFrameContext[i]);

        EWLFreeRefFrm(asic->ewl, &asic->internalreconChroma[i]);

        EWLFreeRefFrm(asic->ewl, &asic->compressTbl[i]);

        EWLFreeRefFrm(asic->ewl, &asic->colBuffer[i]);

        EWLFreeRefFrm(asic->ewl, &asic->mvInfo[i]);
    }

    EWLFreeRefFrm(asic->ewl, &asic->RefRingBuf);

    EWLFreeRefFrm(asic->ewl, &asic->RefRingBufBak);

    EWLFreeLinear(asic->ewl, &asic->mc_sync_word[0]);

    EWLFreeRefFrm(asic->ewl, &asic->cuInfoMem[0]);

    for (i = 0; i < MAX_CORE_NUM; i++) {
        EWLFreeRefFrm(asic->ewl, &asic->ctbBitsMem[i]);
    }

    EWLFreeLinear(asic->ewl, &asic->cabacCtx);

    EWLFreeLinear(asic->ewl, &asic->probCount);

    for (i = 0; i < MAX_CORE_NUM; i++) {
        EWLFreeLinear(asic->ewl, &asic->compress_coeff_SCAN[i]);
    }

    for (i = 0; i < HEVC_MAX_TILE_COLS; i++)
        EWLFreeLinear(asic->ewl, &asic->sizeTbl[i]);

    for (i = 0; i < 4; i++) {
        EWLFreeLinear(asic->ewl, &(asic->ctbRcMem[i]));
    }

    EWLFreeLinear(asic->ewl, &asic->loopbackLineBufMem);

    asic->cabacCtx.virtualAddress = NULL;
    asic->mvOutput.virtualAddress = NULL;
    asic->probCount.virtualAddress = NULL;
    asic->segmentMap.virtualAddress = NULL;

    for (i = 0; i < MAX_CORE_NUM; i++) {
        EWLFreeRefFrm(asic->ewl, &asic->av1PreCarryBuf[i]);
    }

    EWLFreeLinear(asic->ewl, &asic->deblockCtx);
    EWLFreeLinear(asic->ewl, &asic->tileHeightMem);
}

/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
i32 EncAsicCheckStatus_V2(asicData_s *asic, u32 status)
{
    i32 ret;
    u32 dumpRegister = asic->dumpRegister;

    status &= ASIC_STATUS_ALL;

    if (status & ASIC_STATUS_ERROR) {
        /* Get registers for debugging */
        EncAsicGetRegisters(asic->ewl, &asic->regs, dumpRegister, 1);
        ret = ASIC_STATUS_ERROR;
    } else if (status & ASIC_STATUS_FUSE_ERROR) {
        /* Get registers for debugging */
        EncAsicGetRegisters(asic->ewl, &asic->regs, dumpRegister, 1);
        ret = ASIC_STATUS_ERROR;
    } else if (status & ASIC_STATUS_HW_TIMEOUT) {
        /* Get registers for debugging */
        EncAsicGetRegisters(asic->ewl, &asic->regs, dumpRegister, 1);
        ret = ASIC_STATUS_HW_TIMEOUT;
    } else if (status & ASIC_STATUS_FRAME_READY) {
        /* read out all register */
        EncAsicGetRegisters(asic->ewl, &asic->regs, dumpRegister, 1);
        ret = ASIC_STATUS_FRAME_READY;
    } else if (status & ASIC_STATUS_BUFF_FULL) {
        /* ASIC doesn't support recovery from buffer full situation,
     * at the same time with buff full ASIC also resets itself. */
        ret = ASIC_STATUS_BUFF_FULL;
    } else if (status & ASIC_STATUS_HW_RESET) {
        ret = ASIC_STATUS_HW_RESET;
    } else if (status & ASIC_STATUS_SEGMENT_READY) {
        ret = ASIC_STATUS_SEGMENT_READY;
    } else {
        ret = status;
    }

    return ret;
}
