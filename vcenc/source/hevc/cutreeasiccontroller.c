/*------------------------------------------------------------------------------
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                                                                            --
--                            Hantro Products Oy.                             --
--                                                                            --
--                   (C) COPYRIGHT 2006 HANTRO PRODUCTS OY                    --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
--------------------------------------------------------------------------------
--
--  Abstract  :   CuTree process via HW
--
------------------------------------------------------------------------------*/
#ifdef CUTREE_BUILD_SUPPORT

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/
#include "vsi_string.h"
#include "enccommon.h"
#include "ewl.h"
#include "hevcencapi.h"
#include "instance.h"
#include "sw_cu_tree.h"
#include "enctrace.h"
#include "hevcencapi_utils.h"
/*------------------------------------------------------------------------------
    2. External compiler flags
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/
const struct Lowres *invalid_frame = NULL;

/*--------------------------------------------------------------------------------
    4. functions
------------------------------------------------------------------------------*/
void trace_sw_cutree_ctrl_flow(int size, int out_cnt, int pop_cnt, int *qpoutidx);

/*------------------------------------------------------------------------------

    VCEncCuTreeCodeFrame

------------------------------------------------------------------------------*/
#define MAKE_CMD(cur, last, middle, frame, frame_p0, frame_p1, cie, coe, qoe, aqoe, clear_first,   \
                 clear_last, referenced)                                                           \
    (((u64)frame->cuDataIdx << 32) | ((u64)frame->sliceType << 42) |                               \
     ((u64)(clear_first == 0) << 45) | ((u64)(clear_last == 0) << 46) |                            \
     ((((u64)frame->outRoiMapDeltaQpIdx >> 4) & 0x3) << 47) | ((u64)(referenced & 1) << 49) |      \
     ((u64)(cie & 0x01) << 28) | ((u64)(coe & 0x01) << 29) | ((u64)(qoe & 0x01) << 30) |           \
     ((u64)(aqoe & 0x01) << 31) | ((frame->outRoiMapDeltaQpIdx & 0xf) << 24) |                     \
     ((frame->inRoiMapDeltaBinIdx & 0x3f) << 18) |                                                 \
     ((frame_p1 ? (frame_p1->propagateCostIdx & 0x3f) : 0x3f) << 12) |                             \
     ((frame_p0 ? (frame_p0->propagateCostIdx & 0x3f) : 0x3f) << 6) |                              \
     (frame->propagateCostIdx & 0x3f))

#define GET_CUTREE_OUTPUT_REG(i)                                                                   \
    if (i < m_param->lastGopEnd) {                                                                 \
        m_param->lookaheadFrames[i]->cost =                                                        \
            (EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror, HWIF_ENC_CUTREE_COST##i) +   \
             ((u64)EncAsicGetRegisterValue(asic->ewl, asic->regs.regMirror,                        \
                                           HWIF_ENC_CUTREE_COST##i##_MSB)                          \
              << 32) +                                                                             \
             m_param->unitCount / 2) /                                                             \
            m_param->unitCount / 4;                                                                \
    }
#define SET_CUTREE_CMD(i)                                                                          \
    if (i < m_param->num_cmds) {                                                                   \
        EncAsicSetRegisterValue(regs->regMirror, HWIF_ENC_CUTREE_CMD##i,                           \
                                (u32)m_param->commands[i]);                                        \
        EncAsicSetRegisterValue(regs->regMirror, HWIF_ENC_CUTREE_CMD##i##_MSB,                     \
                                (u32)(m_param->commands[i] >> 32));                                \
    }

/*------------------------------------------------------------------------------
    Function name   : CuTreeAsicFrameStart
    Description     :
    Return type     : void
    Argument        : const void *ewl
    Argument        : regValues_s * val
------------------------------------------------------------------------------*/
#define HSWREG(n) ((n)*4)
void CuTreeAsicFrameStart(const void *ewl, regValues_s *val)
{
    i32 i;

    //val->asic_pic_swap = ((val->asic_pic_swap));

    EncAsicSetRegisterValue(val->regMirror, HWIF_ENC_AXI_WR_ID_E, 0);
    EncAsicSetRegisterValue(val->regMirror, HWIF_ENC_AXI_RD_ID_E, 0);

    /* clear all interrupt */
    EncAsicSetRegisterValue(val->regMirror, HWIF_ENC_SLICE_RDY_STATUS, 1);
    EncAsicSetRegisterValue(val->regMirror, HWIF_ENC_IRQ_LINE_BUFFER, 1);
    EncAsicSetRegisterValue(val->regMirror, HWIF_ENC_TIMEOUT, 1);
    EncAsicSetRegisterValue(val->regMirror, HWIF_ENC_BUFFER_FULL, 1);
    EncAsicSetRegisterValue(val->regMirror, HWIF_ENC_SW_RESET, 1);
    EncAsicSetRegisterValue(val->regMirror, HWIF_ENC_BUS_ERROR_STATUS, 1);
    EncAsicSetRegisterValue(val->regMirror, HWIF_ENC_FRAME_RDY_STATUS, 1);
    EncAsicSetRegisterValue(val->regMirror, HWIF_ENC_IRQ, 1);

    /* encoder interrupt */
    EncAsicSetRegisterValue(val->regMirror, HWIF_ENC_IRQ_DIS,
                            val->bVCMDEnable ? 0 : val->irqDisable);
    EncAsicSetRegisterValue(val->regMirror, HWIF_ENC_AXI_READ_ID, val->asic_axi_readID);
    EncAsicSetRegisterValue(val->regMirror, HWIF_ENC_AXI_WRITE_ID, val->asic_axi_writeID);
    EncAsicSetRegisterValue(val->regMirror, HWIF_ENC_CLOCK_GATE_ENCODER_E,
                            val->asic_clock_gating_encoder);
    EncAsicSetRegisterValue(val->regMirror, HWIF_ENC_CLOCK_GATE_ENCODER_H265_E,
                            val->asic_clock_gating_encoder_h265);
    EncAsicSetRegisterValue(val->regMirror, HWIF_ENC_CLOCK_GATE_ENCODER_H264_E,
                            val->asic_clock_gating_encoder_h264);
    EncAsicSetRegisterValue(val->regMirror, HWIF_ENC_CLOCK_GATE_INTER_E,
                            val->asic_clock_gating_inter);
    EncAsicSetRegisterValue(val->regMirror, HWIF_ENC_CLOCK_GATE_INTER_H265_E,
                            val->asic_clock_gating_inter_h265);
    EncAsicSetRegisterValue(val->regMirror, HWIF_ENC_CLOCK_GATE_INTER_H264_E,
                            val->asic_clock_gating_inter_h264);

    EncAsicSetRegisterValue(val->regMirror, HWIF_ENC_MODE, val->codingType);

    EncAsicSetRegisterValue(val->regMirror, HWIF_ENC_MAX_BURST, val->AXI_burst_max_length);
    EncAsicSetRegisterValue(
        val->regMirror, HWIF_ENC_ROI_MAP_QP_DELTA_MAP_SWAP,
        (~(ENCH2_ROIMAP_QPDELTA_SWAP_8 | (ENCH2_ROIMAP_QPDELTA_SWAP_16 << 1) |
           (ENCH2_ROIMAP_QPDELTA_SWAP_32 << 2) | (ENCH2_ROIMAP_QPDELTA_SWAP_64 << 3))) &
            0xf);
    EncAsicSetRegisterValue(val->regMirror, HWIF_ENC_AXI_WRITE_OUTSTANDING_NUM,
                            ENCH2_ASIC_AXI_WRITE_OUTSTANDING_NUM);
    EncAsicSetRegisterValue(val->regMirror, HWIF_ENC_AXI_READ_OUTSTANDING_NUM,
                            ENCH2_ASIC_AXI_READ_OUTSTANDING_NUM);

    if (!val->bVCMDEnable) {
        val->regMirror[HWIF_REG_CFG1] = EWLReadReg(ewl, HWIF_REG_CFG1 * 4);
        val->regMirror[HWIF_REG_CFG2] = EWLReadReg(ewl, HWIF_REG_CFG2 * 4);
        val->regMirror[HWIF_REG_CFG3] = EWLReadReg(ewl, HWIF_REG_CFG3 * 4);
        val->regMirror[HWIF_REG_CFG4] = EWLReadReg(ewl, HWIF_REG_CFG4 * 4);
        //val->regMirror[HWIF_REG_CFG5] = EWLReadReg(ewl,HWIF_REG_CFG5*4);

        for (i = 1; i < ASIC_SWREG_AMOUNT; i++)
            EWLWriteReg(ewl, HSWREG(i), val->regMirror[i]);

        EncTraceRegs(ewl, 0, 0, NULL);
        /* Register with enable bit is written last */
        val->regMirror[ASIC_REG_INDEX_STATUS] |= ASIC_STATUS_ENABLE;

        EWLEnableHW(ewl, HSWREG(ASIC_REG_INDEX_STATUS), val->regMirror[ASIC_REG_INDEX_STATUS]);
    } else {
        val->regMirror[HWIF_REG_CFG1] = EWLReadRegInit(ewl, HWIF_REG_CFG1 * 4);
        val->regMirror[HWIF_REG_CFG2] = EWLReadRegInit(ewl, HWIF_REG_CFG2 * 4);
        val->regMirror[HWIF_REG_CFG3] = EWLReadRegInit(ewl, HWIF_REG_CFG3 * 4);
        val->regMirror[HWIF_REG_CFG4] = EWLReadRegInit(ewl, HWIF_REG_CFG4 * 4);
        //val->regMirror[HWIF_REG_CFG5] = EWLReadRegInit(ewl,HWIF_REG_CFG5*4);

        val->regMirror[ASIC_REG_INDEX_STATUS] &= ~ASIC_STATUS_ENABLE;
        {
            u32 current_length = 0;
            EWLCollectWriteRegData(ewl, &val->regMirror[1], val->vcmdBuf + val->vcmdBufSize, 1,
                                   ASIC_SWREG_AMOUNT - 1, &current_length);
            val->vcmdBufSize += current_length;
        }
        //Flush MMU before enable IM
        {
            EWLCollectWriteMMURegData(ewl, val->vcmdBuf, &val->vcmdBufSize);
        }
        /* Register with enable bit is written last */
        val->regMirror[ASIC_REG_INDEX_STATUS] |= ASIC_STATUS_ENABLE;

        {
            u32 current_length = 0;
            EWLCollectWriteRegData(ewl, &val->regMirror[ASIC_REG_INDEX_STATUS],
                                   val->vcmdBuf + val->vcmdBufSize, ASIC_REG_INDEX_STATUS, 1,
                                   &current_length);
            val->vcmdBufSize += current_length;
        }
    }
}

// Get ROI buffer on demand
// return 0 on success, -1 on error;
static int PrepareRoiBuffer(struct cuTreeCtr *m_param, struct Lowres *pFrame)
{
    ptr_t busAddress = 0;
    if (pFrame->outRoiMapDeltaQpIdx != INVALID_INDEX)
        return 0;
    if (GetRoiMapBufferFromBufferPool(m_param, &busAddress) == NULL)
        return -1;
    pFrame->outRoiMapDeltaQpIdx =
        (busAddress - m_param->outRoiMapDeltaQp_Base) / m_param->outRoiMapDeltaQp_frame_size;
    return 0;
}

static void collectGopCmds(struct cuTreeCtr *m_param, struct Lowres **frames, i32 cur, i32 last,
                           i32 depth, bool *p_clear_first, bool *p_clear_last, bool *p_clear_middle,
                           bool coe, bool aq_output, int outGopSize)
{
    bool clear_first = *p_clear_first;
    bool clear_last = *p_clear_last;
    i32 bframes = last - cur - 1;
    i32 i = last - 1;
    int middle = (bframes + 1) / 2 + cur;
    bool cie;
    bool qoe;
    struct Lowres fake_frame;
    struct Lowres *p_fake_frame = &fake_frame;
    p_fake_frame->propagateCostIdx = m_param->lastGopEnd;
    if (cur < 0)
        return;
    if (depth == 0)
        m_param->maxHieDepth = ((frames[last]->gopSize == 8) && (frames[last]->aGopSize == 4))
                                   ? 3
                                   : DEFAULT_MAX_HIE_DEPTH;
    cie = (depth < m_param->maxHieDepth && bframes > 1);
    if (bframes > 1) {
        bool clear_middle = true;
        bool tmp;
        collectGopCmds(m_param, frames, middle, last, depth + 1, &clear_middle, &clear_last, &tmp,
                       coe, aq_output, outGopSize);
        collectGopCmds(m_param, frames, cur, middle, depth + 1, &clear_first, &clear_middle, &tmp,
                       coe, aq_output, outGopSize);
        if (clear_middle)
            cie = false;
    }

    if (bframes >= 1) {
        bool referenced = (bframes > 1);
        qoe = coe && (depth == 0);
        if (frames[middle]->p0 == (middle - cur) && frames[middle]->p1 == (last - middle) &&
            depth + 1 <= m_param->maxHieDepth) {
            if (qoe || ((m_param->aq_mode != AQ_NONE ||
                         frames[middle]->inRoiMapDeltaBinIdx != INVALID_INDEX) &&
                        aq_output))
                PrepareRoiBuffer(m_param, frames[middle]);
            m_param->commands[m_param->num_cmds++] =
                MAKE_CMD(cur, last, middle, frames[middle], frames[cur], frames[last], cie, true,
                         qoe, (aq_output && !qoe), clear_first, clear_last, referenced);
            clear_first = clear_last = false;
        } else {
            if ((m_param->aq_mode != AQ_NONE ||
                 frames[middle]->inRoiMapDeltaBinIdx != INVALID_INDEX) &&
                aq_output)
                PrepareRoiBuffer(m_param, frames[middle]);
            m_param->commands[m_param->num_cmds++] =
                MAKE_CMD(middle, middle, middle, frames[middle], p_fake_frame, p_fake_frame, cie,
                         true, false, aq_output, true, true, referenced);
        }
        ASSERT(m_param->num_cmds <= 56);
    }
    *p_clear_middle = !cie;
    if (depth == 0) {
        cie = !clear_last;
        qoe = coe || m_param->lastGopEnd <= outGopSize + 1;
        bool aqoe = (aq_output && (last != outGopSize));
        bool referenced = true;
        if (frames[last]->p0 == (last - cur)) {
            if (qoe || ((m_param->aq_mode != AQ_NONE ||
                         frames[last]->inRoiMapDeltaBinIdx != INVALID_INDEX) &&
                        aqoe))
                PrepareRoiBuffer(m_param, frames[last]);
            m_param->commands[m_param->num_cmds++] =
                MAKE_CMD(cur, last, last, frames[last], frames[cur], invalid_frame, cie,
                         (cur >= outGopSize || m_param->lastGopEnd <= outGopSize + 1), qoe,
                         (aqoe && !qoe), clear_first, clear_last, referenced);
            clear_first = false;
        } else {
            if ((m_param->aq_mode != AQ_NONE ||
                 frames[last]->inRoiMapDeltaBinIdx != INVALID_INDEX) &&
                aqoe)
                PrepareRoiBuffer(m_param, frames[last]);
            m_param->commands[m_param->num_cmds++] =
                MAKE_CMD(last, last, last, frames[last], p_fake_frame, p_fake_frame, cie, true,
                         false, aqoe, true, true, referenced);
        }
        clear_last = false;
    }
    *p_clear_first = clear_first;
    *p_clear_last = clear_last;
}
void processGopConvert_4to8_asic(struct cuTreeCtr *m_param, struct Lowres **frames);
static void collectCmds(struct cuTreeCtr *m_param, struct Lowres **frames, int numframes,
                        bool bFirst)
{
    int idx = !bFirst;
    int lastnonb, curnonb = 1;
    int bframes = 0;
    int i = numframes;
    m_param->num_cmds = 0;
    bool bAgop_4to8 =
        (m_param->lastGopEnd > 8 && (frames[4]->gopEncOrder == 0) && (frames[4]->gopSize == 4) &&
         (frames[4]->aGopSize == 8) && (frames[8]->gopEncOrder == 0) && (frames[8]->gopSize == 4) &&
         (frames[8]->aGopSize == 8));
    int outGopSize = (m_param->lastGopEnd > 1 ? (bAgop_4to8 ? 8 : frames[1]->gopSize) : 1);
    bool aq_output = true;

    if (!(bFirst && bAgop_4to8))
        processGopConvert_4to8_asic(m_param, frames);

    while (i > 0 && IS_CODING_TYPE_B(frames[i]->sliceType))
        i--;

    lastnonb = i;

    bool clear_last = true;
    bool clear_first = true;
    bool clear_middle = true;
    /* est one gop */
    while (i-- > 0) {
        curnonb = i;
        while (IS_CODING_TYPE_B(frames[curnonb]->sliceType) && curnonb > 0)
            curnonb--;

        bframes = lastnonb - curnonb - 1;
        clear_first = true;
        collectGopCmds(
            m_param, frames, curnonb, lastnonb, 0, &clear_first, &clear_last, &clear_middle, false,
            (aq_output && lastnonb <= outGopSize && !(bFirst && bAgop_4to8)), outGopSize);
        if (curnonb == outGopSize) {
            PrepareRoiBuffer(m_param, frames[curnonb]);
            m_param->commands[m_param->num_cmds++] =
                MAKE_CMD(curnonb, curnonb, curnonb, frames[curnonb], invalid_frame, invalid_frame,
                         (!clear_first), true, true, false, false, false, false);
        }
        lastnonb = i = curnonb;
        clear_last = clear_first;
    }
    if (bFirst) {
        PrepareRoiBuffer(m_param, frames[0]);
        m_param->commands[m_param->num_cmds++] =
            MAKE_CMD(0, 0, 0, frames[0], invalid_frame, invalid_frame, (!clear_first), true, true,
                     false, false, false, false);
    }
    if (bFirst && bAgop_4to8) {
        clear_first = clear_last = true;
        processGopConvert_4to8_asic(m_param, frames);
        collectGopCmds(m_param, frames, 0, 8, 0, &clear_first, &clear_last, &clear_middle, false,
                       aq_output, outGopSize);
    }
    if (outGopSize > 1) {
        PrepareRoiBuffer(m_param, frames[outGopSize / 2]);
        m_param->commands[m_param->num_cmds++] = MAKE_CMD(
            outGopSize / 2, outGopSize / 2, outGopSize / 2, frames[outGopSize / 2], invalid_frame,
            invalid_frame, (!clear_middle), true, true, false, false, false, false);
    }
    ASSERT(m_param->num_cmds > 0);
}

void EncMakeCmdbufDataCuTree(const void *ewl, regValues_s *val)
{
    //the first command is to read executing cmdbuf id into ddr for balancing load of cores.
    {
        u32 current_length = 0;
        EWLCollectReadVcmdRegData(ewl, val->vcmdBuf + val->vcmdBufSize, 26, 1, &current_length,
                                  val->cmdbufid);
        val->vcmdBufSize += current_length;
    }

    //write registers and run encoder.
    CuTreeAsicFrameStart(ewl, val);
    //stall
    {
        u32 current_length = 0;
        EWLCollectStallDataCuTree(ewl, val->vcmdBuf + val->vcmdBufSize, &current_length);
        val->vcmdBufSize += current_length;
    }
    //read registers necessary into status cmdbuf.
    if (0) {
        u32 current_length = 0;
        EWLCollectReadRegData(ewl, val->vcmdBuf + val->vcmdBufSize, 0, 6, &current_length,
                              val->cmdbufid);
        val->vcmdBufSize += current_length;
        current_length = 0;
        EWLCollectReadRegData(ewl, val->vcmdBuf + val->vcmdBufSize, 20, 132, &current_length,
                              val->cmdbufid);
        val->vcmdBufSize += current_length;
    } else {
        u32 current_length = 0;
        EWLCollectReadRegData(ewl, val->vcmdBuf + val->vcmdBufSize, 0, ASIC_SWREG_AMOUNT,
                              &current_length, val->cmdbufid);
        val->vcmdBufSize += current_length;
    }
    //stop encoder
    {
        u32 current_length = 0;
        EWLCollectStopHwData(ewl, val->vcmdBuf + val->vcmdBufSize, &current_length);
        val->vcmdBufSize += current_length;
    }
    //clear int status register
    {
        u32 current_length = 0;
        EWLCollectClrIntData(ewl, val->vcmdBuf + val->vcmdBufSize, &current_length);
        val->vcmdBufSize += current_length;
    }
#if 0
  //int cmdbuf
  {
    u32 current_length=0;
    EWLCollectIntData(ewl,val->vcmdBuf+val->vcmdBufSize,&current_length,val->cmdbufid);
    val->vcmdBufSize += current_length;
  }
#endif
    //read vcmd own registers. just occupy the position in cmdbuf here, driver will change these data because we don't know the core id currently.
    {
        u32 current_length = 0;
        EWLCollectReadVcmdRegData(ewl, val->vcmdBuf + val->vcmdBufSize, 0, 27, &current_length,
                                  val->cmdbufid);
        val->vcmdBufSize += current_length;
    }

    //JMP
    {
        u32 current_length = 0;
        EWLCollectJmpData(ewl, val->vcmdBuf + val->vcmdBufSize, &current_length, val->cmdbufid);
        val->vcmdBufSize += current_length;
    }
}

int CutreeGet7bitQp(int deltaQp)
{
    return (deltaQp < 0) ? (0x40 | ((-deltaQp) & 0x3F)) : (deltaQp & 0x3F);
}

VCEncRet VCEncCuTreeStart(struct cuTreeCtr *m_param)
{
    int i, j;
    VCEncRet ret;
    regValues_s *regs = &m_param->asic.regs;
    struct vcenc_instance *enc = (struct vcenc_instance *)m_param->pEncInst;
    i32 roiAbsQpSupport = ((struct vcenc_instance *)enc)->asic.regs.asicCfg.roiAbsQpSupport;
    i32 roiMapVersion = ((struct vcenc_instance *)enc)->asic.regs.asicCfg.roiMapVersion;
    i32 cuInforVersion = ((struct vcenc_instance *)enc)->asic.regs.cuInfoVersion;
    i32 roiQpDeltaVer = ((struct vcenc_instance *)enc)->asic.regs.RoiQpDelta_ver;
    ptr_t busAddress = 0;
    u8 *memory;
    struct Lowres **frames = m_param->lookaheadFrames;

    EWLmemset(regs->regMirror, 0, sizeof(regs->regMirror));

    int totalDuration = 0;
    for (j = 0; j < m_param->lastGopEnd; j++) {
        totalDuration += m_param->fpsDenom * 256 / m_param->fpsNum;
        m_param->lookaheadFrames[j]->propagateCostIdx = j;
    }
    int averageDuration = totalDuration / (m_param->lastGopEnd); // Q24.8
    int fpsFactor = (int)(CLIP_DURATION_FIX8(averageDuration) * 256 /
                          CLIP_DURATION_FIX8((m_param->fpsDenom << 8) / m_param->fpsNum)); // Q24.8

    /* set new frame encoding parameters */
    EncAsicSetRegisterValue(regs->regMirror, HWIF_TIMEOUT_OVERRIDE_E,
                            ENCH2_ASIC_TIMEOUT_OVERRIDE_ENABLE);
    EncAsicSetRegisterValue(regs->regMirror, HWIF_TIMEOUT_CYCLES,
                            ENCH2_ASIC_TIMEOUT_CYCLES & 0x7fffff);
    EncAsicSetRegisterValue(regs->regMirror, HWIF_ENC_CUTREE_TIMEOUT_CYCLES_MSB,
                            ENCH2_ASIC_TIMEOUT_CYCLES >> 23);
    EncAsicSetRegisterValue(regs->regMirror, HWIF_ENC_CUTREE_CODECH264,
                            IS_H264(m_param->codecFormat));
    EncAsicSetRegisterValue(regs->regMirror, HWIF_ENC_CUTREE_SEGMENT_COUNT_ENABLE,
                            IS_VP9(m_param->codecFormat));
    EncAsicSetRegisterValue(regs->regMirror, HWIF_ENC_CUTREE_HEIGHT,
                            m_param->height * m_param->dsRatio);
    EncAsicSetRegisterValue(regs->regMirror, HWIF_ENC_CUTREE_WIDTH,
                            m_param->width * m_param->dsRatio);
    EncAsicSetRegisterValue(regs->regMirror, HWIF_ENC_CUTREE_ROIMAPENABLE, m_param->roiMapEnable);
    EncAsicSetRegisterValue(regs->regMirror, HWIF_ENC_CUTREE_HWLOOKAHEADDEPTH, m_param->lastGopEnd);
    EncAsicSetRegisterValue(regs->regMirror, HWIF_ENC_CUTREE_INLOOPDSRATIO, m_param->dsRatio - 1);
    EncAsicSetRegisterValue(regs->regMirror, HWIF_ENC_CUTREE_BPASS1ADAPTIVEGOP,
                            m_param->bUpdateGop);
    EncAsicSetRegisterValue(regs->regMirror, HWIF_ENC_CUTREE_CUINFORVERSION, cuInforVersion);
    EncAsicSetRegisterValue(regs->regMirror, HWIF_ENC_CUTREE_ROIMAPVERSION, roiMapVersion);
    EncAsicSetRegisterValue(regs->regMirror, HWIF_ENC_CUTREE_ROIABSQPSUPPORT, roiAbsQpSupport);
    EncAsicSetRegisterValue(regs->regMirror, HWIF_ENC_CUTREE_ROIDELTAQPVER, roiQpDeltaVer);
    u32 cuTreeStrength =
        (((5 * (10000 - (int)(m_param->qCompress * 10000 + 0.5))) << 8) + 5000) / 10000; // Q24.8
    EncAsicSetRegisterValue(regs->regMirror, HWIF_ENC_CUTREE_STRENGTH, cuTreeStrength);
    EncAsicSetRegisterValue(regs->regMirror, HWIF_ENC_CUTREE_FPSFACTOR, fpsFactor);
    EncAsicSetRegisterValue(regs->regMirror, HWIF_ENC_CUTREE_INQPDELTABLKSIZE,
                            m_param->inQpDeltaBlkSize / m_param->dsRatio == 64
                                ? 2
                                : (m_param->inQpDeltaBlkSize / m_param->dsRatio == 32 ? 1 : 0));

    EncAsicSetRegisterValue(regs->regMirror, HWIF_ENC_CUTREE_IRQ_TYPE_BUS_ERROR,
                            m_param->asic.regs.irq_type_bus_error_mask);
    EncAsicSetRegisterValue(regs->regMirror, HWIF_ENC_CUTREE_IRQ_TYPE_TIMEOUT,
                            m_param->asic.regs.irq_type_timeout_mask);
    EncAsicSetRegisterValue(regs->regMirror, HWIF_ENC_CUTREE_IRQ_TYPE_FRAME_RDY,
                            m_param->asic.regs.irq_type_frame_rdy_mask);

    /* block unit err check */
    u32 cutreeBlkSize = m_param->unitSize * m_param->dsRatio;
    while (cutreeBlkSize > (1 << (6 - m_param->outRoiMapDeltaQpBlockUnit)))
        m_param->outRoiMapDeltaQpBlockUnit--;

    EncAsicSetRegisterValue(regs->regMirror, HWIF_ENC_CUTREE_OUTPUT_BLOCKUNITSIZE,
                            m_param->outRoiMapDeltaQpBlockUnit);
    EncAsicSetAddrRegisterValue(regs->regMirror, HWIF_ENC_CUTREE_CUDATA_BASE, m_param->cuData_Base);
    EncAsicSetRegisterValue(regs->regMirror, HWIF_ENC_CUTREE_CUDATA_FRAME_SIZE,
                            m_param->cuData_frame_size);
    EncAsicSetAddrRegisterValue(regs->regMirror, HWIF_ENC_CUTREE_INROIMAPDELTABIN_BASE,
                                m_param->inRoiMapDeltaBin_Base);
    EncAsicSetRegisterValue(regs->regMirror, HWIF_ENC_CUTREE_INROIMAPDELTABIN_FRAME_SIZE,
                            m_param->inRoiMapDeltaBin_frame_size);
    EncAsicSetAddrRegisterValue(regs->regMirror, HWIF_ENC_CUTREE_OUTROIMAPDELTAQP_BASE,
                                m_param->outRoiMapDeltaQp_Base);
    EncAsicSetRegisterValue(regs->regMirror, HWIF_ENC_CUTREE_OUTROIMAPDELTAQP_FRAME_SIZE,
                            m_param->outRoiMapDeltaQp_frame_size);
    EncAsicSetRegisterValue(regs->regMirror, HWIF_ENC_CUTREE_SEGMENT_COUNT_OFFSET,
                            m_param->outRoiMapSegmentCountOffset);
    EncAsicSetAddrRegisterValue(regs->regMirror, HWIF_ENC_CUTREE_PROPAGATECOST_BASE,
                                m_param->propagateCost_Base);
    EncAsicSetRegisterValue(regs->regMirror, HWIF_ENC_CUTREE_PROPAGATECOST_FRAME_SIZE,
                            m_param->propagateCost_frame_size);
    EncAsicSetRegisterValue(regs->regMirror, HWIF_ENC_CUTREE_FRAMECOSTREFINEENABLE,
                            m_param->imFrameCostRefineEn);
    EncAsicSetRegisterValue(regs->regMirror, HWIF_ENC_CUTREE_AQMODE, m_param->aq_mode);

    EncAsicSetRegisterValue(regs->regMirror, HWIF_ENC_CUTREE_VP9_SEGMENT1_QP,
                            CutreeGet7bitQp(m_param->segment_qp[0]));
    EncAsicSetRegisterValue(regs->regMirror, HWIF_ENC_CUTREE_VP9_SEGMENT2_QP,
                            CutreeGet7bitQp(m_param->segment_qp[1]));
    EncAsicSetRegisterValue(regs->regMirror, HWIF_ENC_CUTREE_VP9_SEGMENT3_QP,
                            CutreeGet7bitQp(m_param->segment_qp[2]));
    EncAsicSetRegisterValue(regs->regMirror, HWIF_ENC_CUTREE_VP9_SEGMENT4_QP,
                            CutreeGet7bitQp(m_param->segment_qp[3]));
    EncAsicSetRegisterValue(regs->regMirror, HWIF_ENC_CUTREE_VP9_SEGMENT5_QP,
                            CutreeGet7bitQp(m_param->segment_qp[4]));
    EncAsicSetRegisterValue(regs->regMirror, HWIF_ENC_CUTREE_VP9_SEGMENT6_QP,
                            CutreeGet7bitQp(m_param->segment_qp[5]));
    EncAsicSetRegisterValue(regs->regMirror, HWIF_ENC_CUTREE_VP9_SEGMENT7_QP,
                            CutreeGet7bitQp(m_param->segment_qp[6]));
    EncAsicSetRegisterValue(regs->regMirror, HWIF_ENC_CUTREE_VP9_SEGMENT8_QP,
                            CutreeGet7bitQp(m_param->segment_qp[7]));

    if (m_param->aq_mode) {
        u32 aqStrength = (u32)(m_param->aqStrength * (1 << AQ_STRENGTH_SCALE_BITS));
        EncAsicSetRegisterValue(regs->regMirror, HWIF_ENC_CUTREE_AQMODESTRENGTH, aqStrength);
        EncAsicSetAddrRegisterValue(regs->regMirror, HWIF_ENC_CUTREE_AQDATA_BASE,
                                    m_param->aqDataBase);
        EncAsicSetRegisterValue(regs->regMirror, HWIF_ENC_CUTREE_AQDATA_FRAME_SIZE,
                                m_param->aqDataFrameSize);
        EncAsicSetRegisterValue(regs->regMirror, HWIF_ENC_CUTREE_AQDATA_STRIDE,
                                m_param->aqDataStride);
    }
    EncAsicSetRegisterValue(regs->regMirror, HWIF_ENC_CUTREE_CUINFO_STRIDE,
                            m_param->asic.regs.cuinfoStride);

    collectCmds(m_param, m_param->lookaheadFrames, m_param->lastGopEnd - 1,
                IS_CODING_TYPE_I(frames[0]->sliceType));
    SET_CUTREE_CMD(0);
    SET_CUTREE_CMD(1);
    SET_CUTREE_CMD(2);
    SET_CUTREE_CMD(3);
    SET_CUTREE_CMD(4);
    SET_CUTREE_CMD(5);
    SET_CUTREE_CMD(6);
    SET_CUTREE_CMD(7);
    SET_CUTREE_CMD(8);
    SET_CUTREE_CMD(9);
    SET_CUTREE_CMD(10);
    SET_CUTREE_CMD(11);
    SET_CUTREE_CMD(12);
    SET_CUTREE_CMD(13);
    SET_CUTREE_CMD(14);
    SET_CUTREE_CMD(15);
    SET_CUTREE_CMD(16);
    SET_CUTREE_CMD(17);
    SET_CUTREE_CMD(18);
    SET_CUTREE_CMD(19);
    SET_CUTREE_CMD(20);
    SET_CUTREE_CMD(21);
    SET_CUTREE_CMD(22);
    SET_CUTREE_CMD(23);
    SET_CUTREE_CMD(24);
    SET_CUTREE_CMD(25);
    SET_CUTREE_CMD(26);
    SET_CUTREE_CMD(27);
    SET_CUTREE_CMD(28);
    SET_CUTREE_CMD(29);
    SET_CUTREE_CMD(30);
    SET_CUTREE_CMD(31);
    SET_CUTREE_CMD(32);
    SET_CUTREE_CMD(33);
    SET_CUTREE_CMD(34);
    SET_CUTREE_CMD(35);
    SET_CUTREE_CMD(36);
    SET_CUTREE_CMD(37);
    SET_CUTREE_CMD(38);
    SET_CUTREE_CMD(39);
    SET_CUTREE_CMD(40);
    SET_CUTREE_CMD(41);
    SET_CUTREE_CMD(42);
    SET_CUTREE_CMD(43);
    SET_CUTREE_CMD(44);
    SET_CUTREE_CMD(45);
    SET_CUTREE_CMD(46);
    SET_CUTREE_CMD(47);
    SET_CUTREE_CMD(48);
    SET_CUTREE_CMD(49);
    SET_CUTREE_CMD(50);
    SET_CUTREE_CMD(51);
    SET_CUTREE_CMD(52);
    SET_CUTREE_CMD(53);
    SET_CUTREE_CMD(54);
    SET_CUTREE_CMD(55);
    EncAsicSetRegisterValue(regs->regMirror, HWIF_ENC_CUTREE_NUM_CMDS, m_param->num_cmds);

    if (!regs->bVCMDEnable) {
        /* Try to reserve the HW resource */
        u32 reserve_core_info = 0;

        u32 core_info =
            0; /*mode[1bit](1:all 0:specified)+amount[3bit](the needing amount -1)+reserved+core_mapping[8bit]*/
        //u32 valid_num =0;
        //EWLHwConfig_t cfg;
        //for (i=0; i< EWLGetCoreNum(m_param->ctx); i++)
        //{
        // cfg = EncAsicGetAsicConfig(i, m_param->ctx);
        // if (cfg.cuTreeSupport == 1)
        //  {
        //  core_info |= 1<<i;
        //   valid_num++;
        //  }
        //  else
        //   continue;
        // }
        // if (valid_num == 0) /*none of cores is supported*/
        //   return VCENC_INVALID_ARGUMENT;

        core_info |= 1 << CORE_INFO_MODE_OFFSET; //now just support 1 core,so mode is all.

        core_info |= 0 << CORE_INFO_AMOUNT_OFFSET; //now just support 1 core

        reserve_core_info = core_info;
        if ((EWLReserveHw(m_param->asic.ewl, &reserve_core_info, NULL) == EWL_ERROR) ||
            (EWLCheckCutreeValid(m_param->asic.ewl) == EWL_ERROR)) {
            return VCENC_HW_RESERVED;
        }

        //AXIFE
        if (enc->axiFEEnable) {
            VCEncAxiFeEnable((void *)enc, (void *)m_param->asic.ewl, 0);
        }

        /* start hw encoding */
        CuTreeAsicFrameStart(m_param->asic.ewl, &m_param->asic.regs);
    } else {
        EncSetReseveInfo(m_param->asic.ewl, m_param->width * m_param->dsRatio,
                         m_param->height * m_param->dsRatio, 0, 0, 0, EWL_CLIENT_TYPE_CUTREE);
        EncReseveCmdbuf(m_param->asic.ewl, &m_param->asic.regs);
        m_param->asic.regs.vcmdBufSize = 0;
        EncMakeCmdbufDataCuTree(m_param->asic.ewl, &m_param->asic.regs);
        m_param->asic.regs.totalCmdbufSize = m_param->asic.regs.vcmdBufSize;
        EncCopyDataToCmdbuf(m_param->asic.ewl, &m_param->asic.regs);

        EncLinkRunCmdbuf(m_param->asic.ewl, &m_param->asic.regs);
    }
    return VCENC_OK;
}

VCEncRet VCEncCuTreeWait(struct cuTreeCtr *m_param)
{
    VCEncRet ret;
    asicData_s *asic = &m_param->asic;
    u32 status = ASIC_STATUS_ERROR;

    do {
        /* Encode one frame */
        i32 ewl_ret;
        if (!asic->regs.bVCMDEnable) {
            /* Wait for IRQ */
            ewl_ret = EWLWaitHwRdy(asic->ewl, NULL, NULL, &status);
        } else {
            ewl_ret = EncWaitCmdbuf(asic->ewl, asic->regs.cmdbufid, &status);
        }
        if (ewl_ret != EWL_OK) {
            status = ASIC_STATUS_ERROR;

            if (ewl_ret == EWL_ERROR) {
                /* IRQ error => Stop and release HW */
                ret = VCENC_SYSTEM_ERROR;
            } else /*if(ewl_ret == EWL_HW_WAIT_TIMEOUT) */
            {
                /* IRQ Timeout => Stop and release HW */
                ret = VCENC_HW_TIMEOUT;
            }

            EncAsicStop(asic->ewl);
            /* Release HW so that it can be used by other codecs */
            if (!asic->regs.bVCMDEnable) {
                EWLReleaseHw(asic->ewl);
            } else {
                EWLReleaseCmdbuf(asic->ewl, asic->regs.cmdbufid);
            }
        } else {
            status = EncAsicCheckStatus_V2(asic, status);

            switch (status) {
            case ASIC_STATUS_ERROR:
                if (!asic->regs.bVCMDEnable) {
                    EWLReleaseHw(asic->ewl);
                } else {
                    EWLReleaseCmdbuf(asic->ewl, asic->regs.cmdbufid);
                }
                ret = VCENC_ERROR;
                break;
            case ASIC_STATUS_HW_RESET:
                if (!asic->regs.bVCMDEnable) {
                    EWLReleaseHw(asic->ewl);
                } else {
                    EWLReleaseCmdbuf(asic->ewl, asic->regs.cmdbufid);
                }
                ret = VCENC_HW_RESET;
                break;
            case ASIC_STATUS_HW_TIMEOUT:
                if (!asic->regs.bVCMDEnable) {
                    EWLReleaseHw(asic->ewl);
                } else {
                    EWLReleaseCmdbuf(asic->ewl, asic->regs.cmdbufid);
                }
                ret = VCENC_HW_TIMEOUT;
                break;
            case ASIC_STATUS_FRAME_READY:
                if (m_param->lastGopEnd > 8 && ((m_param->lookaheadFrames[4]->gopEncOrder == 0) &&
                                                (m_param->lookaheadFrames[4]->gopSize == 4) &&
                                                (m_param->lookaheadFrames[4]->aGopSize == 8) &&
                                                (m_param->lookaheadFrames[8]->gopEncOrder == 0) &&
                                                (m_param->lookaheadFrames[8]->gopSize == 4) &&
                                                (m_param->lookaheadFrames[8]->aGopSize == 8)))
                    m_param->rem_frames = m_param->lastGopEnd - 8;
                else
                    m_param->rem_frames =
                        m_param->lastGopEnd -
                        (m_param->lastGopEnd > 1 ? m_param->lookaheadFrames[1]->gopSize : 0);
                GET_CUTREE_OUTPUT_REG(0);
                GET_CUTREE_OUTPUT_REG(1);
                GET_CUTREE_OUTPUT_REG(2);
                GET_CUTREE_OUTPUT_REG(3);
                GET_CUTREE_OUTPUT_REG(4);
                GET_CUTREE_OUTPUT_REG(5);
                GET_CUTREE_OUTPUT_REG(6);
                GET_CUTREE_OUTPUT_REG(7);
                GET_CUTREE_OUTPUT_REG(8);
                GET_CUTREE_OUTPUT_REG(9);
                GET_CUTREE_OUTPUT_REG(10);
                GET_CUTREE_OUTPUT_REG(11);
                GET_CUTREE_OUTPUT_REG(12);
                GET_CUTREE_OUTPUT_REG(13);
                GET_CUTREE_OUTPUT_REG(14);
                GET_CUTREE_OUTPUT_REG(15);
                GET_CUTREE_OUTPUT_REG(16);
                GET_CUTREE_OUTPUT_REG(17);
                GET_CUTREE_OUTPUT_REG(18);
                GET_CUTREE_OUTPUT_REG(19);
                GET_CUTREE_OUTPUT_REG(20);
                GET_CUTREE_OUTPUT_REG(21);
                GET_CUTREE_OUTPUT_REG(22);
                GET_CUTREE_OUTPUT_REG(23);
                GET_CUTREE_OUTPUT_REG(24);
                GET_CUTREE_OUTPUT_REG(25);
                GET_CUTREE_OUTPUT_REG(26);
                GET_CUTREE_OUTPUT_REG(27);
                GET_CUTREE_OUTPUT_REG(28);
                GET_CUTREE_OUTPUT_REG(29);
                GET_CUTREE_OUTPUT_REG(30);
                GET_CUTREE_OUTPUT_REG(31);
                GET_CUTREE_OUTPUT_REG(32);
                GET_CUTREE_OUTPUT_REG(33);
                GET_CUTREE_OUTPUT_REG(34);
                GET_CUTREE_OUTPUT_REG(35);
                GET_CUTREE_OUTPUT_REG(36);
                GET_CUTREE_OUTPUT_REG(37);
                GET_CUTREE_OUTPUT_REG(38);
                GET_CUTREE_OUTPUT_REG(39);
                GET_CUTREE_OUTPUT_REG(40);
                GET_CUTREE_OUTPUT_REG(41);
                GET_CUTREE_OUTPUT_REG(42);
                GET_CUTREE_OUTPUT_REG(43);
                GET_CUTREE_OUTPUT_REG(44);
                GET_CUTREE_OUTPUT_REG(45);
                GET_CUTREE_OUTPUT_REG(46);
                GET_CUTREE_OUTPUT_REG(47);
                ret = VCENC_OK;
                if (!asic->regs.bVCMDEnable) {
                    EWLReleaseHw(asic->ewl);
                } else {
                    EWLReleaseCmdbuf(asic->ewl, asic->regs.cmdbufid);
                }
                break;
            default:
                /* should never get here */
                ASSERT(0);
                ret = VCENC_ERROR;
            }
        }
    } while (status == ASIC_STATUS_LINE_BUFFER_DONE || status == ASIC_STATUS_SEGMENT_READY);

    return ret;
}

void setFrameTypeChar(struct Lowres *frame);
static void markBRef(struct cuTreeCtr *m_param, struct Lowres **frames, i32 cur, i32 last,
                     i32 depth)
{
    i32 bframes = last - cur - 1;
    i32 i = last - 1;
    if (cur < 0)
        return;
    if (bframes > 1) {
        int middle = (bframes + 1) / 2 + cur;
        markBRef(m_param, frames, middle, last, depth + 1);
        markBRef(m_param, frames, cur, middle, depth + 1);

        frames[middle]->sliceType = VCENC_FRAME_TYPE_BREF;
        setFrameTypeChar(frames[middle]);
        frames[middle]->predId = getFramePredId(frames[middle]->sliceType);
    }
}
void statisAheadData(struct cuTreeCtr *m_param, struct Lowres **frames, int numframes, bool bFirst);
void remove_one_frame(struct cuTreeCtr *m_param);
static void write_asic_gop_cutree(struct cuTreeCtr *m_param, struct Lowres **frame, i32 size,
                                  i32 base);
void processGopConvert_4to8_asic(struct cuTreeCtr *m_param, struct Lowres **frames)
{
    int i;
    if (m_param->lastGopEnd <= 8)
        return;

    /* convert 2xgop4 to gop8 */
    if ((frames[4]->gopEncOrder == 0) && (frames[4]->gopSize == 4) && (frames[4]->aGopSize == 8) &&
        (frames[8]->gopEncOrder == 0) && (frames[8]->gopSize == 4) && (frames[8]->aGopSize == 8)) {
        for (i = 1; i <= 8; i++)
            frames[i]->gopSize = 8;

        frames[4]->sliceType = VCENC_FRAME_TYPE_BREF;
        setFrameTypeChar(frames[4]);
        frames[4]->predId = getFramePredId(frames[4]->sliceType);

        frames[8]->gopEncOrder = 0;
        frames[4]->gopEncOrder = 1;
        frames[2]->gopEncOrder = 2;
        frames[1]->gopEncOrder = 3;
        frames[3]->gopEncOrder = 4;
        frames[6]->gopEncOrder = 5;
        frames[5]->gopEncOrder = 6;
        frames[7]->gopEncOrder = 7;

        for (i = 1; i <= 8; i++)
            frames[i]->aGopSize = 0;
    }
}
bool processGopConvert_8to4_asic(struct cuTreeCtr *m_param, struct Lowres **frames)
{
    i32 i;
    if (m_param->lastGopEnd <= 8)
        return false;

    /* convert gop8 to 2xgop4 */
    if ((frames[8]->gopEncOrder == 0) && (frames[8]->gopSize == 8) && (frames[8]->aGopSize == 4)) {
        for (i = 1; i <= 8; i++)
            frames[i]->gopSize = 4;

        frames[4]->sliceType = VCENC_FRAME_TYPE_P;
        setFrameTypeChar(frames[4]);
        frames[4]->predId = getFramePredId(frames[4]->sliceType);

        frames[4]->gopEncOrder = 0;
        frames[2]->gopEncOrder = 1;
        frames[1]->gopEncOrder = 2;
        frames[3]->gopEncOrder = 3;
        frames[8]->gopEncOrder = 0;
        frames[6]->gopEncOrder = 1;
        frames[5]->gopEncOrder = 2;
        frames[7]->gopEncOrder = 3;

        statisAheadData(m_param, frames, m_param->lastGopEnd - 1, HANTRO_FALSE);
        write_asic_gop_cutree(m_param, m_param->lookaheadFrames + 1, 4, 1);

        for (i = 1; i <= 8; i++)
            frames[i]->aGopSize = 0;

        for (i = 0; i < 4; i++)
            remove_one_frame(m_param);
        m_param->out_cnt += 4;
        m_param->pop_cnt += 4;

        return true;
    }
    return false;
}
static void write_asic_cutree_file(struct cuTreeCtr *m_param, struct Lowres *frame, int i)
{
    struct vcenc_instance *enc = (struct vcenc_instance *)m_param->pEncInst;
    VCEncLookaheadJob *out = frame->job;
    ptr_t busAddress = 0;
    ptr_t segBusAddress = 0;

    if (frame->outRoiMapDeltaQpIdx != INVALID_INDEX) {
        busAddress = m_param->outRoiMapDeltaQp_Base +
                     m_param->outRoiMapDeltaQp_frame_size * frame->outRoiMapDeltaQpIdx;
        //segBusAddress = busAddress + m_param->outRoiMapSegmentCountOffset;
        segBusAddress = (ptr_t)m_param->segmentCountVirtualBase +
                        m_param->outRoiMapDeltaQp_frame_size * frame->outRoiMapDeltaQpIdx +
                        m_param->outRoiMapSegmentCountOffset;
        EWLLinearMem_t roimapDeltaQpMem =
            m_param->roiMapDeltaQpMemFactory[frame->outRoiMapDeltaQpIdx];
        EWLSyncMemData(&roimapDeltaQpMem, (u32)m_param->outRoiMapSegmentCountOffset,
                       MAX_SEGMENTS * sizeof(u32), DEVICE_TO_HOST);
    }

    /* output frame info */
    out->pRoiMapDeltaQpAddr =
        out->encIn.roiMapDeltaQpAddr; //save original roiMapDeltaQpAddr to put it back to buffer pool
    out->encIn.roiMapDeltaQpAddr = busAddress;
    out->frame.segmentCountAddr = (u32 *)segBusAddress;
    out->frame.poc = frame->poc;
    out->frame.frameNum = frame->frameNum;
    out->frame.typeChar = frame->typeChar;
    out->frame.qp = frame->qp;
    out->frame.cost = (int)(m_param->lookaheadFrames[i]->cost / 256.0);
    out->frame.gopSize = frame->gopSize;
    for (i = 0; i < 4; i++) {
        out->frame.costGop[i] = m_param->costGopInt[i] / 256.0;
        out->frame.FrameNumGop[i] = m_param->FrameNumGop[i];
        out->frame.costAvg[i] = m_param->costAvgInt[i] / 256.0;
        out->frame.FrameTypeNum[i] = m_param->FrameTypeNum[i];
    }
    out->status = VCENC_FRAME_READY;
#ifdef GLOBAL_MV_ON_SEARCH_RANGE
    out->encIn.gmv[0][0] = out->frame.gmv[0][0];
    out->encIn.gmv[0][1] = out->frame.gmv[0][1];
    out->encIn.gmv[1][0] = out->frame.gmv[1][0];
    out->encIn.gmv[1][1] = out->frame.gmv[1][1];
    printf("    Lowres gmv (%d,%d),(%d,%d),(%d,%d)\n", out->encIn.gmv[0][0], out->encIn.gmv[0][1],
           out->encIn.gmv[1][0], out->encIn.gmv[1][1], out->frame.gmv[0][2], out->frame.gmv[1][2]);
#endif
    LookaheadEnqueueOutput(&enc->lookahead, out);
    frame->job = NULL;
}

static void write_asic_gop_cutree(struct cuTreeCtr *m_param, struct Lowres **frame, i32 size,
                                  i32 base)
{
    i32 i, j;

    markBRef(m_param, frame - 1, 0, size, 0);
    for (i = 0; i < size; i++) {
        for (j = 0; j < size; j++) {
            if (frame[j]->gopEncOrder == i)
                break;
        }
        write_asic_cutree_file(m_param, frame[j], base + j);
        m_param->qpOutIdx[m_param->out_cnt + i] = frame[j]->outRoiMapDeltaQpIdx;
    }
}

VCEncRet VCEncCuTreeInit(struct cuTreeCtr *m_param)
{
    VCEncRet ret = VCENC_OK;
    const void *ewl = NULL;
    EWLInitParam_t param;
    asicMemAlloc_s allocCfg;
    struct vcenc_instance *enc = (struct vcenc_instance *)m_param->pEncInst;

    param.clientType = EWL_CLIENT_TYPE_CUTREE;
    param.slice_idx = m_param->slice_idx;
    param.context = m_param->ctx;
    /* Init EWL */
    if ((ewl = EWLInit(&param)) == NULL) {
        return VCENC_EWL_ERROR;
    }

    m_param->asic.regs.bVCMDAvailable = EWLGetVCMDSupport() ? true : false;
    m_param->asic.regs.bVCMDEnable = EWLGetVCMDMode(ewl) ? true : false;

    if (m_param->asic.regs.bVCMDEnable) {
        if (!(m_param->asic.regs.vcmdBuf = (u32 *)EWLcalloc(1, VCMDBUFFER_SIZE))) {
            ret = VCENC_MEMORY_ERROR;
            goto error;
        }
    }
    m_param->asic.ewl = ewl;
    (void)EncAsicControllerInit(&m_param->asic, m_param->ctx, param.clientType);

    /* Allocate internal SW/HW shared memories */
    memset(&allocCfg, 0, sizeof(asicMemAlloc_s));
    allocCfg.width = m_param->width;
    allocCfg.height = m_param->height;
    allocCfg.encodingType = ASIC_CUTREE;
    allocCfg.is_malloc = 1;
    if (EncAsicMemAlloc_V2(&m_param->asic, &allocCfg) != ENCHW_OK) {
        ret = VCENC_EWL_MEMORY_ERROR;
        goto error;
    }
    // cuInfo buffer layout:
    //   cuTable (size = asic.cuInfoTableSize)
    //   aqData  (size = asic.aqInfoSize)
    //   cuData
    m_param->cuData_Base =
        enc->asic.cuInfoMem[0].busAddress + enc->asic.cuInfoTableSize + enc->asic.aqInfoSize;
    m_param->cuData_frame_size =
        enc->asic.cuInfoMem[1].busAddress - enc->asic.cuInfoMem[0].busAddress;
    m_param->aqDataBase = enc->asic.cuInfoMem[0].busAddress + enc->asic.cuInfoTableSize;
    m_param->aqDataFrameSize = m_param->cuData_frame_size;
    m_param->aqDataStride = enc->asic.aqInfoStride;
    m_param->asic.regs.cuinfoStride = enc->asic.regs.cuinfoStride;
    m_param->outRoiMapDeltaQp_Base = m_param->roiMapDeltaQpMemFactory[0].busAddress;
    m_param->outRoiMapDeltaQp_frame_size = m_param->roiMapDeltaQpMemFactory[1].busAddress -
                                           m_param->roiMapDeltaQpMemFactory[0].busAddress;
    if (IS_VP9(m_param->codecFormat)) {
        m_param->outRoiMapSegmentCountOffset =
            m_param->outRoiMapDeltaQp_frame_size - MAX_SEGMENTS * sizeof(u32);
        m_param->segmentCountVirtualBase = m_param->roiMapDeltaQpMemFactory[0].virtualAddress;
    }
    m_param->inRoiMapDeltaBin_Base = 0;
    m_param->inRoiMapDeltaBin_frame_size = 0;
    {
        //allocate propagate cost memory.
        //allocate 1 extra buffer as fake reference propagate buffer, for frames that only needs to calculate frame cost.
        //(frame beyond maxHieDepth, or frame missing reference frame due to GOP conversion)
        i32 num_buf = CUTREE_BUFFER_CNT(m_param->lookaheadDepth, m_param->gopSize) + 1;
        int block_size = m_param->unitCount * sizeof(uint32_t);
        int i;
        block_size = ((block_size + 63) & (~63));
        m_param->propagateCostMemFactory[0].mem_type =
            CPU_WR | VPU_WR | VPU_RD | EWL_MEM_TYPE_VPU_WORKING;
        if (EWLMallocLinear(m_param->asic.ewl, block_size * num_buf, 0,
                            &m_param->propagateCostMemFactory[0]) != EWL_OK) {
            for (i = 0; i < num_buf; i++) {
                m_param->propagateCostMemFactory[i].virtualAddress = NULL;
            }

            ret = VCENC_EWL_MEMORY_ERROR;
            goto error;
        } else {
            i32 total_size = m_param->propagateCostMemFactory[0].size;
            memset(m_param->propagateCostMemFactory[0].virtualAddress, 0, block_size * num_buf);
            EWLSyncMemData(&m_param->propagateCostMemFactory[0], 0, block_size * num_buf,
                           HOST_TO_DEVICE);

            for (i = 0; i < num_buf; i++) {
                m_param->propagateCostMemFactory[i].virtualAddress =
                    (u32 *)((ptr_t)m_param->propagateCostMemFactory[0].virtualAddress +
                            i * block_size);
                m_param->propagateCostMemFactory[i].busAddress =
                    m_param->propagateCostMemFactory[0].busAddress + i * block_size;
                m_param->propagateCostMemFactory[i].size =
                    (i < num_buf - 1 ? block_size : total_size - (num_buf - 1) * block_size);
                m_param->propagateCostRefCnt[i] = 0;
            }
        }
        m_param->propagateCost_Base = m_param->propagateCostMemFactory[0].busAddress;
        m_param->propagateCost_frame_size = m_param->propagateCostMemFactory[1].busAddress -
                                            m_param->propagateCostMemFactory[0].busAddress;
    }
    return VCENC_OK;

error:
    if (ewl != NULL) {
        (void)EWLRelease(ewl);
        m_param->asic.ewl = NULL;
    }

    return ret;
}

VCEncRet VCEncCuTreeProcessOneFrame(struct cuTreeCtr *m_param)
{
    // process until lastGopEnd, ignoring incomplete GOP.
    VCEncRet ret;
    int i;
    ret = VCEncCuTreeStart(m_param);
    if (ret != VCENC_OK)
        return ret;
    ret = VCEncCuTreeWait(m_param);
    if (ret != VCENC_OK)
        return ret;
    struct Lowres **frames = m_param->lookaheadFrames;
    struct Lowres *out_frame = *m_param->lookaheadFrames;
    i32 idx = 1;
    i32 size = m_param->lastGopEnd;
    m_param->out_cnt = m_param->pop_cnt = 0;
    i = 0;
    while (i + 1 < m_param->lastGopEnd) {
        markBRef(m_param, frames, i, i + frames[i + 1]->gopSize, 0);
        i += frames[i + 1]->gopSize;
    }
    if (IS_CODING_TYPE_I(out_frame->sliceType)) {
        statisAheadData(m_param, m_param->lookaheadFrames, m_param->lastGopEnd - 1, HANTRO_TRUE);
        write_asic_gop_cutree(m_param, &out_frame, 1, 0);
        m_param->out_cnt++;
    }
    if (processGopConvert_8to4_asic(m_param, frames))
        idx += 4;
    if (m_param->lastGopEnd > 1) {
        int gopSize =
            (m_param->lookaheadFrames[1]->poc == 0 ? 1 : m_param->lookaheadFrames[1]->gopSize);
        if (!IS_CODING_TYPE_I(m_param->lookaheadFrames[1]->sliceType)) {
            statisAheadData(m_param, m_param->lookaheadFrames, m_param->lastGopEnd - 1,
                            HANTRO_FALSE);
            write_asic_gop_cutree(m_param, m_param->lookaheadFrames + 1, gopSize, 1);
            m_param->out_cnt += gopSize;
        }
        for (i = 0; i < gopSize; i++)
            remove_one_frame(m_param);
        m_param->pop_cnt += gopSize;
    }
    pthread_mutex_lock(&m_param->cuinfobuf_mutex);
    ASSERT(m_param->cuInfoToRead >= m_param->out_cnt);
    m_param->cuInfoToRead -= m_param->out_cnt;
    pthread_mutex_unlock(&m_param->cuinfobuf_mutex);
    pthread_cond_signal(&m_param->cuinfobuf_cond);
#ifdef TEST_DATA
    trace_sw_cutree_ctrl_flow(size, m_param->out_cnt, m_param->pop_cnt, m_param->qpOutIdx);
#endif
    return VCENC_OK;
}

VCEncRet VCEncCuTreeRelease(struct cuTreeCtr *pEncInst)
{
    const void *ewl;

    ASSERT(pEncInst);
    ewl = pEncInst->asic.ewl;
    if (pEncInst->asic.regs.vcmdBuf)
        EWLfree(pEncInst->asic.regs.vcmdBuf);
    if (ewl == NULL)
        return VCENC_OK;

    EWLFreeLinear(pEncInst->asic.ewl, &pEncInst->propagateCostMemFactory[0]);

    if (ewl) {
        EncAsicMemFree_V2(&pEncInst->asic);
        (void)EWLRelease(ewl);
    }
    pEncInst->asic.ewl = NULL;
    return VCENC_OK;
}

#endif
