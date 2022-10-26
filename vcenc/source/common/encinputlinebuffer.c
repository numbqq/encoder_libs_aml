/*------------------------------------------------------------------------------
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                                                                            --
--                            Verisilicon.                                    --
--                                                                            --
--                   (C) COPYRIGHT 2014 VERISILICON                           --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
--------------------------------------------------------------------------------
--
--  Abstract :  For test/fpga_verification/example purpose. operations on Input line buffer.
--
------------------------------------------------------------------------------*/
#ifdef LOW_LATENCY_BUILD_SUPPORT

#include <stdio.h>
#include "encinputlinebuffer.h"
#include "encasiccontroller.h"
#include "instance.h"
#ifdef SUPPORT_JPEG
#include "EncJpegInstance.h"
#endif

static inputLineBufferCfg *pLineBufCfgS = NULL;
static const regField_s lineBufRegisterDesc[] = {
    /* HW ID register, read-only */
    {InputlineBufWrCntr, 0x000, 0x000001ff, 0, 0, RW, NONE,
     "slice_wr_cntr. +slice_depth when one slice is filled into slice_fifo"},
    {InputlineBufDepth, 0x000, 0x0003fe00, 9, 0, RW, NONE, "slice_depth. unit is MB line"},
    {InputlineBufHwHandshake, 0x000, 0x00040000, 18, 0, RW, NONE,
     "slice_hw_mode_en. active high. enable bit of slice_fifo hardware mode. "
     "should be disabled before the start of next frame."},
    {InputlineBufPicHeight, 0x000, 0x0ff80000, 19, 0, RW, NONE,
     "pic_height. same value of swreg14[18:10] in H1."},
    {InputlineBufRdCntr, 0x008, 0x000001ff, 0, 0, RO, NONE, "slice_rd_cntr. read only"},
};

/* FPGA verification. get/set register value of input-line-buffer */
static u32 getInputLineBufReg(u32 *reg, u32 name)
{
    u32 value = (reg[lineBufRegisterDesc[name].base / 4] & lineBufRegisterDesc[name].mask) >>
                lineBufRegisterDesc[name].lsb;
    return value;
}
static void setInputLineBufReg(u32 *reg, u32 name, u32 value)
{
    reg[lineBufRegisterDesc[name].base / 4] =
        (reg[lineBufRegisterDesc[name].base / 4] & ~(lineBufRegisterDesc[name].mask)) |
        ((value << lineBufRegisterDesc[name].lsb) & lineBufRegisterDesc[name].mask);
}

static u32 getMbLinesRdCnt(inputLineBufferCfg *cfg)
{
    u32 rdCnt = 0;

    if (cfg->hwHandShake && cfg->hwSyncReg) {
        rdCnt = getInputLineBufReg(cfg->hwSyncReg, InputlineBufRdCntr);

        /* frame end, disable hardware handshake */
        //if ((rdCnt*cfg->ctbSize) >= cfg->encHeight)
        //setInputLineBufReg(cfg->hwSyncReg,InputlineBufHwHandshake, 0);
    } else if (cfg->getMbLines)
        rdCnt = cfg->getMbLines(cfg->inst);

    return rdCnt;
}

static void DumpInputLineBufReg(inputLineBufferCfg *cfg)
{
    u32 *reg = cfg->hwSyncReg;
    i32 i;

    if (!reg)
        return;

    printf("==== SRAM HW-handshake REGISTERS ====\n");
    for (i = 0; i < LINE_BUF_SWREG_AMOUNT; i++)
        printf("     %08x: %08x\n", i * 4, reg[i]);

    printf("     rdCnt=%d, wrCnt=%d, depth=%d, hwMode=%d, picH=%d\n",
           getInputLineBufReg(reg, InputlineBufRdCntr), getInputLineBufReg(reg, InputlineBufWrCntr),
           getInputLineBufReg(reg, InputlineBufDepth),
           getInputLineBufReg(reg, InputlineBufHwHandshake),
           getInputLineBufReg(reg, InputlineBufPicHeight));
}

static void DumpVCEncLowLatencyReg(asicData_s *asic)
{
    u32 mode = EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror, HWIF_ENC_MODE);
    u32 rdCnt = EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror, HWIF_CTB_ROW_RD_PTR);
    u32 wrCnt = EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror, HWIF_CTB_ROW_WR_PTR);

    if (mode == ASIC_JPEG) {
        rdCnt +=
            EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror, HWIF_CTB_ROW_RD_PTR_JPEG_MSB);
        wrCnt +=
            EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror, HWIF_CTB_ROW_WR_PTR_JPEG_MSB);
    }
    printf(
        "==== ASIC Low-Latency Regs: rdCnt=%d, wrCnt=%d, depth=%d, En=%d, "
        "hwMode=%d, loopback=%d, IrqEn=%d\n",
        rdCnt, wrCnt,
        (EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror, HWIF_NUM_CTB_ROWS_PER_SYNC) |
         (EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror, HWIF_NUM_CTB_ROWS_PER_SYNC_MSB)
          << 9)),
        EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror, HWIF_LOW_LATENCY_EN),
        EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror, HWIF_LOW_LATENCY_HW_SYNC_EN),
        EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror, HWIF_INPUT_BUF_LOOPBACK_EN),
        EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror, HWIF_ENC_LINE_BUFFER_INT));
}

static void setMbLinesWrCnt(inputLineBufferCfg *cfg)
{
    if (cfg->hwHandShake && cfg->hwSyncReg) {
        setInputLineBufReg(cfg->hwSyncReg, InputlineBufWrCntr, cfg->wrCnt);
    } else if (cfg->setMbLines)
        cfg->setMbLines(cfg->inst, cfg->wrCnt);
}

static u32 getBufStride(VCEncPictureType type, u32 pixW)
{
    return pixW;
    switch (type) {
    case VCENC_YUV420_PLANAR:
    case VCENC_YVU420_PLANAR:
    case VCENC_YUV420_SEMIPLANAR:
    case VCENC_YUV420_SEMIPLANAR_VU:
        return pixW;

    case VCENC_YUV422_INTERLEAVED_YUYV:
    case VCENC_YUV422_INTERLEAVED_UYVY:
    case VCENC_RGB565:
    case VCENC_BGR565:
    case VCENC_RGB555:
    case VCENC_BGR555:
    case VCENC_RGB444:
    case VCENC_BGR444:
    case VCENC_YUV420_10BIT_PACKED_Y0L2:
    case VCENC_YUV420_PLANAR_10BIT_I010:
    case VCENC_YUV420_PLANAR_10BIT_P010:
        return (pixW * 2);
    case VCENC_RGB888_24BIT:
    case VCENC_BGR888_24BIT:
    case VCENC_RBG888_24BIT:
    case VCENC_GBR888_24BIT:
    case VCENC_BRG888_24BIT:
    case VCENC_GRB888_24BIT:
        return (pixW * 3);
    case VCENC_RGB888:
    case VCENC_BGR888:
    case VCENC_RGB101010:
    case VCENC_BGR101010:
        return (pixW * 4);

    case VCENC_YUV420_PLANAR_10BIT_PACKED_PLANAR:
        return (pixW * 10 / 8);

    default:
        return pixW;
    }

    return pixW;
}

static i32 is420CbCrInterleave(VCEncPictureType inputFormat)
{
    return ((inputFormat == VCENC_YUV420_SEMIPLANAR) ||
            (inputFormat == VCENC_YUV420_SEMIPLANAR_VU) ||
            (inputFormat == VCENC_YUV420_PLANAR_10BIT_P010));
}

static i32 is420CbCrPlanar(VCEncPictureType inputFormat)
{
    return ((inputFormat == VCENC_YUV420_PLANAR) ||
            (inputFormat == VCENC_YUV420_PLANAR_10BIT_I010) ||
            (inputFormat == VCENC_YUV420_PLANAR_10BIT_PACKED_PLANAR) ||
            (inputFormat == VCENC_YUV420_PLANAR_8BIT_TILE_32_32) ||
            (inputFormat == VCENC_YUV420_PLANAR_8BIT_TILE_16_16_PACKED_4) ||
            (inputFormat == VCENC_YVU420_PLANAR));
}

static void getAlignedPicSizebyFormat(u32 type, u32 luma_stride, u32 chroma_stride, u32 height,
                                      u32 *luma_Size, u32 *chroma_Size, u32 *picture_Size)
{
    u32 lumaSize = 0, chromaSize = 0, pictureSize = 0;

    switch (type) {
    case VCENC_YUV420_PLANAR:
    case VCENC_YVU420_PLANAR:
        lumaSize = luma_stride * height;
        chromaSize = chroma_stride * height / 2 * 2;
        break;
    case VCENC_YUV420_SEMIPLANAR:
    case VCENC_YUV420_SEMIPLANAR_VU:
        lumaSize = luma_stride * height;
        chromaSize = chroma_stride * height / 2;
        break;
    case VCENC_YUV422_INTERLEAVED_YUYV:
    case VCENC_YUV422_INTERLEAVED_UYVY:
    case VCENC_RGB565:
    case VCENC_BGR565:
    case VCENC_RGB555:
    case VCENC_BGR555:
    case VCENC_RGB444:
    case VCENC_BGR444:
    case VCENC_RGB888:
    case VCENC_BGR888:
    case VCENC_RGB888_24BIT:
    case VCENC_BGR888_24BIT:
    case VCENC_RBG888_24BIT:
    case VCENC_GBR888_24BIT:
    case VCENC_BRG888_24BIT:
    case VCENC_GRB888_24BIT:
    case VCENC_RGB101010:
    case VCENC_BGR101010:
        lumaSize = luma_stride * height;
        chromaSize = 0;
        break;
    case VCENC_YUV420_PLANAR_10BIT_I010:
        lumaSize = luma_stride * height;
        chromaSize = chroma_stride * height / 2 * 2;
        break;
    case VCENC_YUV420_PLANAR_10BIT_P010:
        lumaSize = luma_stride * height;
        chromaSize = chroma_stride * height / 2;
        break;
    case VCENC_YUV420_PLANAR_10BIT_PACKED_PLANAR:
        lumaSize = luma_stride * 10 / 8 * height;
        chromaSize = chroma_stride * 10 / 8 * height / 2 * 2;
        break;
    case VCENC_YUV420_10BIT_PACKED_Y0L2:
        lumaSize = luma_stride * 2 * 2 * height / 2;
        chromaSize = 0;
        break;
    case VCENC_YUV420_PLANAR_8BIT_TILE_32_32:
        lumaSize = luma_stride * ((height + 32 - 1) & (~(32 - 1)));
        chromaSize = lumaSize / 2;
        break;
    case VCENC_YUV420_PLANAR_8BIT_TILE_16_16_PACKED_4:
        lumaSize = luma_stride * height * 2 * 12 / 8;
        chromaSize = 0;
        break;
    case VCENC_YUV420_SEMIPLANAR_8BIT_TILE_4_4:
    case VCENC_YUV420_SEMIPLANAR_VU_8BIT_TILE_4_4:
        lumaSize = luma_stride * ((height + 3) / 4);
        chromaSize = chroma_stride * (((height / 2) + 3) / 4);
        break;
    case VCENC_YUV420_PLANAR_10BIT_P010_TILE_4_4:
        lumaSize = luma_stride * ((height + 3) / 4);
        chromaSize = chroma_stride * (((height / 2) + 3) / 4);
        break;
    case VCENC_YUV420_SEMIPLANAR_101010:
        lumaSize = luma_stride * height;
        chromaSize = chroma_stride * height / 2;
        break;
    case VCENC_YUV420_8BIT_TILE_64_4:
    case VCENC_YUV420_UV_8BIT_TILE_64_4:
        lumaSize = luma_stride * ((height + 3) / 4);
        chromaSize = chroma_stride * (((height / 2) + 3) / 4);
        break;
    case VCENC_YUV420_10BIT_TILE_32_4:
        lumaSize = luma_stride * ((height + 3) / 4);
        chromaSize = chroma_stride * (((height / 2) + 3) / 4);
        break;
    case VCENC_YUV420_10BIT_TILE_48_4:
    case VCENC_YUV420_VU_10BIT_TILE_48_4:
        lumaSize = luma_stride * ((height + 3) / 4);
        chromaSize = chroma_stride * (((height / 2) + 3) / 4);
        break;
    case VCENC_YUV420_8BIT_TILE_128_2:
    case VCENC_YUV420_UV_8BIT_TILE_128_2:
        lumaSize = luma_stride * ((height + 1) / 2);
        chromaSize = chroma_stride * (((height / 2) + 1) / 2);
        break;
    case VCENC_YUV420_10BIT_TILE_96_2:
    case VCENC_YUV420_VU_10BIT_TILE_96_2:
        lumaSize = luma_stride * ((height + 1) / 2);
        chromaSize = chroma_stride * (((height / 2) + 1) / 2);
        break;
    case VCENC_YUV420_8BIT_TILE_8_8:
        lumaSize = luma_stride * ((height + 7) / 8);
        chromaSize = chroma_stride * (((height / 2) + 3) / 4);
        break;
    case VCENC_YUV420_10BIT_TILE_8_8:
        lumaSize = luma_stride * ((height + 7) / 8);
        chromaSize = chroma_stride * (((height / 2) + 3) / 4);
        break;
    case VCENC_YUV420_UV_8BIT_TILE_64_2:
        lumaSize = luma_stride * ((height + 1) / 2);
        chromaSize = chroma_stride * (((height / 2) + 1) / 2);
        break;
    default:
        printf("not support this format\n");
        chromaSize = lumaSize = 0;
        break;
    }

    pictureSize = lumaSize + chromaSize;
    if (luma_Size != NULL)
        *luma_Size = lumaSize;
    if (chroma_Size != NULL)
        *chroma_Size = chromaSize;
    if (picture_Size != NULL)
        *picture_Size = pictureSize;
}

/*------------------------------------------------------------------------------
    copyLineBuf
------------------------------------------------------------------------------*/
static void copyLineBuf(u8 *dst, u8 *src, i32 width, i32 height, u32 offset_src, u32 offset_dst,
                        u32 depth)
{
    i32 i, j;
    if (dst && src) {
        for (i = 0; i < height; i++) {
            i32 srcOff = i + offset_src;
            i32 dstOff = (i + offset_dst) % depth;
            u32 *dst32 = (u32 *)(dst + dstOff * width);
            u32 *src32 = (u32 *)(src + srcOff * width);
            for (j = 0; j < (width / 4); j++)
                dst32[j] = src32[j];
        }
    }
}

static asicData_s *getInstance_asicData(inputLineBufferCfg *lineBufCfg)
{
    asicData_s *asic = NULL;
    if (lineBufCfg->client_type == EWL_CLIENT_TYPE_HEVC_ENC ||
        lineBufCfg->client_type == EWL_CLIENT_TYPE_H264_ENC)
        asic = &(((struct vcenc_instance *)(lineBufCfg->inst))->asic);
#ifdef SUPPORT_JPEG
    else if (lineBufCfg->client_type == EWL_CLIENT_TYPE_JPEG_ENC)
        asic = &(((jpegInstance_s *)(lineBufCfg->inst))->asic);
#endif
    return asic;
}

/*------------------------------------------------------------------------------
    writeInputLineBuf
------------------------------------------------------------------------------*/
static void writeInputLineBuf(inputLineBufferCfg *cfg, i32 lines)
{
    u8 *lumSrc = cfg->lumSrc;
    u8 *cbSrc = cfg->cbSrc;
    u8 *crSrc = cfg->crSrc;
    u8 *lumDst = cfg->lumBuf.buf;
    u8 *cbDst = cfg->cbBuf.buf;
    u8 *crDst = cfg->crBuf.buf;
    u32 format = cfg->inputFormat;
    u32 depth = cfg->depth;
    u32 wrCnt = cfg->wrCnt;
    u32 offset_src = wrCnt * cfg->ctbSize;
    u32 maxLine;
    u32 offset_dst = 0;
    asicData_s *asic = NULL;

    asic = getInstance_asicData(cfg);

    u32 prpSbiSupport = asic->regs.asicCfg.prpSbiSupport;

    if ((lumSrc == NULL) || (lumDst == NULL))
        return;

    offset_dst = prpSbiSupport ? (offset_src + cfg->initSegNum * cfg->ctbSize) : offset_src;

    if (cfg->loopBackEn)
        maxLine = depth * cfg->ctbSize * cfg->amountPerLoopBack; /* low-latency buffer */
    else
        maxLine = cfg->encHeight;

    copyLineBuf(lumDst, lumSrc, cfg->lumaStride, lines, offset_src, offset_dst, maxLine);

    if (is420CbCrPlanar(format)) {
        copyLineBuf(cbDst, cbSrc, cfg->chromaStride, lines / 2, offset_src / 2, offset_dst / 2,
                    maxLine / 2);
        copyLineBuf(crDst, crSrc, cfg->chromaStride, lines / 2, offset_src / 2, offset_dst / 2,
                    maxLine / 2);
    } else if (is420CbCrInterleave(format)) {
        copyLineBuf(cbDst, cbSrc, cfg->chromaStride, lines / 2, offset_src / 2, offset_dst / 2,
                    maxLine / 2);
    }
}

/*------------------------------------------------------------------------------
    VCEncInitInputLineBufSrcPtr
    -Initialize src picture related pointers
------------------------------------------------------------------------------*/
void VCEncInitInputLineBufSrcPtr(inputLineBufferCfg *lineBufCfg)
{
    u32 format = lineBufCfg->inputFormat;
    u32 verOffset = lineBufCfg->srcVerOffset;

    if (!lineBufCfg->lumSrc)
        return;

    lineBufCfg->lumSrc += verOffset * lineBufCfg->lumaStride;
    if (is420CbCrInterleave(format)) {
        lineBufCfg->cbSrc += (verOffset / 2) * lineBufCfg->chromaStride;
    } else if (is420CbCrPlanar(format)) {
        lineBufCfg->cbSrc += (verOffset / 2) * (lineBufCfg->chromaStride);
        lineBufCfg->crSrc += (verOffset / 2) * (lineBufCfg->chromaStride);
    }
}

/*------------------------------------------------------------------------------
    VCEncInitInputLineBufPtr
    -Initialize line buffer related pointers
------------------------------------------------------------------------------*/
i32 VCEncInitInputLineBufPtr(inputLineBufferCfg *lineBufCfg)
{
    u32 lumaBufSize, chrBufSize, bufSize;
    u32 height = lineBufCfg->amountPerLoopBack;
    asicData_s *asic = NULL;
    asic = getInstance_asicData(lineBufCfg);

    getAlignedPicSizebyFormat(
        lineBufCfg->inputFormat, lineBufCfg->lumaStride, lineBufCfg->chromaStride,
        lineBufCfg->depth * lineBufCfg->ctbSize * height, &lumaBufSize, &chrBufSize, &bufSize);

    /* if there is enough sram */
    if (lineBufCfg->sram && (lineBufCfg->sramSize >= bufSize)) {
        lineBufCfg->lumBuf.buf = lineBufCfg->sram;
        lineBufCfg->lumBuf.busAddress = lineBufCfg->sramBusAddr;
    } else {
        asic->loopbackLineBufMem.mem_type = EXT_WR | VPU_RD | EWL_MEM_TYPE_DPB;
        if (EWLMallocLoopbackLineBuf(asic->ewl, bufSize, &(asic->loopbackLineBufMem)) != EWL_OK)
            return -1;

        lineBufCfg->lumBuf.buf = (u8 *)asic->loopbackLineBufMem.virtualAddress;
        lineBufCfg->lumBuf.busAddress = asic->loopbackLineBufMem.busAddress;
    }

    if (!(lineBufCfg->lumBuf.buf))
        return 0;

    if (is420CbCrInterleave(lineBufCfg->inputFormat) || is420CbCrPlanar(lineBufCfg->inputFormat)) {
        lineBufCfg->cbBuf.buf = lineBufCfg->lumBuf.buf + lumaBufSize;
        lineBufCfg->cbBuf.busAddress = lineBufCfg->lumBuf.busAddress + lumaBufSize;
    }

    if (is420CbCrPlanar(lineBufCfg->inputFormat)) {
        lineBufCfg->crBuf.buf = lineBufCfg->cbBuf.buf + chrBufSize / 2;
        lineBufCfg->crBuf.busAddress = lineBufCfg->cbBuf.busAddress + chrBufSize / 2;
    }

    return 0;
}

/*------------------------------------------------------------------------------

    VCEncInitInputLineBuffer
    -get line buffer params for IRQ handle
    -get address of input line buffer
------------------------------------------------------------------------------*/
i32 VCEncInitInputLineBuffer(inputLineBufferCfg *lineBufCfg)
{
    if (!lineBufCfg)
        return -1;
    EWLLinearMem_t lineBufSRAM;
    u32 syncDepth = lineBufCfg->depth;
    asicData_s *asic = NULL;
    asic = getInstance_asicData(lineBufCfg);

    if (!asic)
        return -1;

    if (lineBufCfg->depth == 0)
        lineBufCfg->depth = 1;

    /* setup addresses of line buffer */
    if (EWLGetLineBufSram(asic->ewl, &lineBufSRAM))
        return -1;
    lineBufCfg->sram = (u8 *)(lineBufSRAM.virtualAddress);
    lineBufCfg->sramBusAddr = lineBufSRAM.busAddress;
    lineBufCfg->sramSize = lineBufSRAM.size;

    lineBufCfg->hwSyncReg = NULL;
#ifdef PCIE_FPGA_VERI_LINEBUF
    if (lineBufCfg->sram) {
        /* some registers are in the head of sram for fpga verification */
        u32 regsBytes = LINE_BUF_SWREG_AMOUNT * 4;
        i32 i;

        lineBufCfg->hwSyncReg = (u32 *)lineBufCfg->sram;
        /* clean sram registers */
        for (i = 0; i < LINE_BUF_SWREG_AMOUNT; i++)
            lineBufCfg->hwSyncReg[i] = 0;

        lineBufCfg->sram += regsBytes;
        lineBufCfg->sramBusAddr += regsBytes;
        lineBufCfg->sramSize -= regsBytes;
    }
#endif

    /* in loop back mode, testbench needs to copy source data to line buffer on IRQ */
    if (lineBufCfg->loopBackEn) {
        /* setup pointers of source picture */
        VCEncInitInputLineBufSrcPtr(lineBufCfg);

        /* setup pointers of loopback line buffer */
        if (VCEncInitInputLineBufPtr(lineBufCfg))
            return -1;
    }

    /* Only for verification purpose:
           Set func and variable to test hardware handshake mode or IRQ is disabled by depth==0.
           This test doesn't work in multi-instance case.*/
    pLineBufCfgS = lineBufCfg;
    if (lineBufCfg->hwHandShake || (syncDepth == 0))
        pollInputLineBufTestFunc = &VCEncInputLineBufPolling;

#if 0
    /* To test sram */
    if (lineBufSRAM.virtualAddress)
    {
        u32 *sram = (u32 *)(lineBufSRAM.virtualAddress + LINE_BUF_SWREG_AMOUNT);
        i32 size = lineBufSRAM.size/4 - LINE_BUF_SWREG_AMOUNT;
        i32 i;
        //write
        for (i=0; i<size; i++)
            *sram++ = i;
        //read
        sram = (u32 *)(lineBufSRAM.virtualAddress + LINE_BUF_SWREG_AMOUNT);
        for (i=0; i<size; i++)
        {
            i32 r = *sram++;
            if (r != i)
                printf ("====== SRAM Test Error at %d: w=%d, r=%d\n", i, i, r);
        }
    }
#endif

    return 0;
}

/*------------------------------------------------------------------------------

    VCEncStartInputLineBuffer
    -setup inputLineBufferCfg
    -initialize line buffer
    -Input:
      bSrcPtrUpd: The input image pointers have been changed by application, only useful in loopback mode.

------------------------------------------------------------------------------*/
u32 VCEncStartInputLineBuffer(inputLineBufferCfg *lineBufCfg, bool bSrcPtrUpd)
{
    /* write line buffer to start */
    u32 wrCnt;
    u32 lines = MIN(lineBufCfg->depth * lineBufCfg->amountPerLoopBack * lineBufCfg->ctbSize,
                    lineBufCfg->encHeight);
    asicData_s *asic = NULL;
    asic = getInstance_asicData(lineBufCfg);

    u32 prpSbiSupport = asic->regs.asicCfg.prpSbiSupport;

    if (lineBufCfg->loopBackEn) {
        if (bSrcPtrUpd == HANTRO_TRUE)
            VCEncInitInputLineBufSrcPtr(lineBufCfg);
        writeInputLineBuf(lineBufCfg, lines);
    }

    /* init sram regs */
    if (lineBufCfg->hwSyncReg) {
        i32 i;
        for (i = 0; i < LINE_BUF_SWREG_AMOUNT; i++)
            lineBufCfg->hwSyncReg[i] = 0;
        lineBufCfg->wrCnt = ((lines + (lineBufCfg->ctbSize - 1)) / lineBufCfg->ctbSize);
        if (lineBufCfg->hwHandShake) {
            setInputLineBufReg(lineBufCfg->hwSyncReg, InputlineBufPicHeight,
                               (lineBufCfg->encHeight + (lineBufCfg->ctbSize - 1)) /
                                   lineBufCfg->ctbSize);
            setInputLineBufReg(lineBufCfg->hwSyncReg, InputlineBufDepth, lineBufCfg->depth);
            setInputLineBufReg(lineBufCfg->hwSyncReg, InputlineBufWrCntr, lineBufCfg->wrCnt);
            setInputLineBufReg(lineBufCfg->hwSyncReg, InputlineBufHwHandshake, 1);

            //DumpInputLineBufReg(lineBufCfg);
        }
    }
    lineBufCfg->wrCnt = ((lines + (lineBufCfg->ctbSize - 1)) / lineBufCfg->ctbSize);
    lineBufCfg->wrCnt = prpSbiSupport
                            ? ((lineBufCfg->wrCnt + lineBufCfg->depth - 1) / lineBufCfg->depth)
                            : lineBufCfg->wrCnt;
    wrCnt = lineBufCfg->hwHandShake ? 0 : lineBufCfg->wrCnt;
    return wrCnt;
}

/*------------------------------------------------------------------------------
    An example of Line buffer Callback function
    called by the encoder SW after receive "input line buffer done" interruption from HW.
    Used for test/fpga_verification currently.
------------------------------------------------------------------------------*/
void VCEncInputLineBufDone(void *pAppData)
{
    if (pAppData) {
        inputLineBufferCfg *cfg = (inputLineBufferCfg *)pAppData;
        asicData_s *asic = NULL;
        asic = getInstance_asicData(cfg);

        u32 prpSbiSupport = asic->regs.asicCfg.prpSbiSupport;
        i32 rdCnt = 0;
        i32 wrCnt = cfg->wrCnt;
        i32 depth = cfg->depth;
        i32 height = cfg->encHeight;
        i32 lines = depth * cfg->ctbSize;
        i32 offset = wrCnt * cfg->ctbSize;

        /* get rd counter */
        rdCnt = getMbLinesRdCnt(cfg);

        /* write line buffer */
        lines = MIN(lines, height - offset);
        if ((lines > 0) && (cfg->wrCnt <= (rdCnt + depth))) {
            if (cfg->loopBackEn)
                writeInputLineBuf(cfg, lines);
            cfg->wrCnt +=
                (prpSbiSupport ? ((((lines + (cfg->ctbSize - 1)) / cfg->ctbSize) + cfg->depth - 1) /
                                  cfg->depth)
                               : ((lines + (cfg->ctbSize - 1)) / cfg->ctbSize));
        }

        /* update write counter */
        setMbLinesWrCnt(cfg);

        if ((rdCnt * cfg->ctbSize) < height)
            printf("    #<---- Line_Buf_Done:  encHeight=%d, depth=%d, rdCnt=%d, "
                   "wrCnt=%d-->%d\n",
                   height, depth, rdCnt, wrCnt, cfg->wrCnt);

        //DumpInputLineBufReg(cfg);
        //DumpVCEncLowLatencyReg(cfg->asic);
    }
}

/*------------------------------------------------------------------------------
    Test input_line_buffer hw-handshake mode for FPGA verification.
    Polling input line buffer status and process it.
------------------------------------------------------------------------------*/
u32 VCEncInputLineBufPolling(void)
{
    if (!pLineBufCfgS)
        return 0;

    VCEncInputLineBufDone(pLineBufCfgS);
    return pLineBufCfgS->wrCnt;
}

void VCEncUpdateInitSegNum(inputLineBufferCfg *lineBufCfg)
{
    if (lineBufCfg == NULL)
        return;

    asicData_s *asic = NULL;
    asic = getInstance_asicData(lineBufCfg);

    u32 prpSbiSupport = asic->regs.asicCfg.prpSbiSupport;

    if (prpSbiSupport == 0)
        return;

    lineBufCfg->initSegNum =
        (lineBufCfg->initSegNum +
         (lineBufCfg->encHeight + lineBufCfg->depth * lineBufCfg->ctbSize - 1) /
             (lineBufCfg->depth * lineBufCfg->ctbSize)) %
        lineBufCfg->amountPerLoopBack;

    return;
}

#endif
