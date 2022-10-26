#ifdef CUTREE_BUILD_SUPPORT

#include "hevcencapi.h"
#include "sw_cu_tree.h"
#include "instance.h"
#include "cutree_stream_ctrl.h"

/**
 * @brief cuTreeChangeResolution(), cutree change to the new resolution.
 * @param struct vcenc_instance *vcenc_instance: encoder instance.
 * @param struct cuTreeCtr *m_param: cutree params.
 * @return VCEncRet: VCENC_OK successful, other unsuccessful.
 */
VCEncRet cuTreeChangeResolution(struct vcenc_instance *vcenc_instance, struct cuTreeCtr *m_param)
{
    VCEncRet ret = VCENC_OK;
    asicMemAlloc_s allocCfg;
    m_param->pEncInst = (void *)vcenc_instance;
    struct vcenc_instance *enc = (struct vcenc_instance *)m_param->pEncInst;
    int i;

    //alg default parameter
    m_param->vbvBufferSize = 0;
    m_param->bEnableWeightedBiPred = 0;
    m_param->bBPyramid = 1;
    m_param->bBHierarchy = 1;
    m_param->lookaheadDepth = enc->lookaheadDepth;
    m_param->qgSize = 32;
    m_param->qCompress = 0.6;
    m_param->m_cuTreeStrength = (int)(5.0 * (1.0 - m_param->qCompress) * 256.0 + 0.5);
    //m_param->gopSize = config->gopSize;

    //from encoder
    m_param->unitSize = 16;
    m_param->widthInUnit = (vcenc_instance->width + m_param->unitSize - 1) / m_param->unitSize;
    m_param->heightInUnit = (vcenc_instance->height + m_param->unitSize - 1) / m_param->unitSize;
    m_param->unitCount = (m_param->widthInUnit) * (m_param->heightInUnit);
    m_param->fpsNum = vcenc_instance->rateControl.outRateNum;
    m_param->fpsDenom = vcenc_instance->rateControl.outRateDenom;
    m_param->width = vcenc_instance->width;
    m_param->height = vcenc_instance->height;
    m_param->max_cu_size = vcenc_instance->max_cu_size;
    m_param->roiMapEnable = vcenc_instance->roiMapEnable;
    m_param->codecFormat = vcenc_instance->codecFormat;
    m_param->imFrameCostRefineEn = vcenc_instance->rateControl.pass1EstCostAll;

    //temp buffer for cutree propagate
    //m_param->m_scratch = EWLmalloc(sizeof(int64_t) * m_param->widthInUnit);
    m_param->nLookaheadFrames = 0;
    m_param->lastGopEnd = 0;
    m_param->lookaheadFrames = m_param->lookaheadFramesBase;
    m_param->frameNum = 0;
    for (i32 i = 0; i < 4; i++) {
        m_param->FrameTypeNum[i] = 0;
        m_param->costAvgInt[i] = 0;
        m_param->FrameNumGop[i] = 0;
        m_param->costGopInt[i] = 0;
    }
    m_param->bUpdateGop = 0;
    m_param->latestGopSize = 0;
    m_param->maxHieDepth = DEFAULT_MAX_HIE_DEPTH;
    m_param->bHWMultiPassSupport = vcenc_instance->asic.regs.asicCfg.bMultiPassSupport;

    m_param->asic.regs.totalCmdbufSize = 0;
    m_param->asic.regs.vcmdBufSize = 0;

    /* Init segment qps */
    m_param->segmentCountEnable = IS_VP9(vcenc_instance->codecFormat);
    for (i = 0; i < MAX_SEGMENTS; i++) {
        m_param->segment_qp[i] = segment_delta_qp[i];
    }

    {
        //allocate delta qp map memory.
        // 4 bits per block.
        int block_size = ((vcenc_instance->width + vcenc_instance->max_cu_size - 1) &
                          (~(vcenc_instance->max_cu_size - 1))) *
                         ((vcenc_instance->height + vcenc_instance->max_cu_size - 1) &
                          (~(vcenc_instance->max_cu_size - 1))) /
                         (8 * 8 * 2);
        // 8 bits per block if ipcm map/absolute roi qp is supported
        if (vcenc_instance->asic.regs.asicCfg.roiMapVersion >= 1)
            block_size *= 2;
        i32 in_loop_ds_ratio = vcenc_instance->preProcess.inLoopDSRatio + 1;
        block_size *= in_loop_ds_ratio * in_loop_ds_ratio;

        /* Put VP9 Segment count buffer at end of each roi buffer */
        if (IS_VP9(vcenc_instance->codecFormat)) {
            block_size += 8 * sizeof(u32);
        }
        block_size = ((block_size + 63) & (~63));

        i32 total_size = m_param->roiMapDeltaQpMemFactory[0].size;
        memset(m_param->roiMapDeltaQpMemFactory[0].virtualAddress, 0,
               block_size * CUTREE_BUFFER_NUM);
        EWLSyncMemData(&m_param->roiMapDeltaQpMemFactory[0], 0, block_size * CUTREE_BUFFER_NUM,
                       HOST_TO_DEVICE);
        for (i = 0; i < CUTREE_BUFFER_NUM; i++) {
            m_param->roiMapDeltaQpMemFactory[i].virtualAddress =
                (u32 *)((ptr_t)m_param->roiMapDeltaQpMemFactory[0].virtualAddress + i * block_size);
            m_param->roiMapDeltaQpMemFactory[i].busAddress =
                m_param->roiMapDeltaQpMemFactory[0].busAddress + i * block_size;
            m_param->roiMapDeltaQpMemFactory[i].size =
                (i < CUTREE_BUFFER_NUM - 1 ? block_size
                                           : total_size - (CUTREE_BUFFER_NUM - 1) * block_size);
            m_param->roiMapRefCnt[i] = 0;
        }
        m_param->outRoiMapSegmentCountOffset = m_param->roiMapDeltaQpMemFactory[1].busAddress -
                                               m_param->roiMapDeltaQpMemFactory[0].busAddress -
                                               MAX_SEGMENTS * sizeof(u32);
    }

    m_param->ctx = vcenc_instance->ctx;
    m_param->slice_idx = vcenc_instance->slice_idx;
    m_param->bStatus = THREAD_STATUS_OK;

    /*init*/
    {
        EWLInitParam_t param;
        param.clientType = EWL_CLIENT_TYPE_CUTREE;
        param.slice_idx = m_param->slice_idx;
        param.context = m_param->ctx;

        (void)EncAsicControllerInit(&m_param->asic, m_param->ctx, param.clientType);

        /* Allocate internal SW/HW shared memories */
        memset(&allocCfg, 0, sizeof(asicMemAlloc_s));
        allocCfg.width = m_param->width;
        allocCfg.height = m_param->height;
        allocCfg.encodingType = ASIC_CUTREE;
        allocCfg.is_malloc = 0;
        if (EncAsicMemAlloc_V2(&m_param->asic, &allocCfg) != ENCHW_OK) {
            ret = VCENC_EWL_MEMORY_ERROR;
            return ret;
            ;
        } //*/

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
        if (IS_VP9(m_param->codecFormat))
            m_param->outRoiMapSegmentCountOffset =
                m_param->outRoiMapDeltaQp_frame_size - MAX_SEGMENTS * sizeof(u32);
        m_param->inRoiMapDeltaBin_Base = 0;
        m_param->inRoiMapDeltaBin_frame_size = 0;

        //allocate propagate cost memory.
        i32 num_buf = CUTREE_BUFFER_CNT(m_param->lookaheadDepth, m_param->gopSize);
        int block_size = m_param->unitCount * sizeof(uint32_t);
        block_size = ((block_size + 63) & (~63));

        i32 total_size = m_param->propagateCostMemFactory[0].size;
        memset(m_param->propagateCostMemFactory[0].virtualAddress, 0, block_size * num_buf);
        EWLSyncMemData(&m_param->propagateCostMemFactory[0], 0, block_size * num_buf,
                       HOST_TO_DEVICE);
        for (i = 0; i < num_buf; i++) {
            m_param->propagateCostMemFactory[i].virtualAddress =
                (u32 *)((ptr_t)m_param->propagateCostMemFactory[0].virtualAddress + i * block_size);
            m_param->propagateCostMemFactory[i].busAddress =
                m_param->propagateCostMemFactory[0].busAddress + i * block_size;
            m_param->propagateCostMemFactory[i].size =
                (i < num_buf - 1 ? block_size : total_size - (num_buf - 1) * block_size);
            m_param->propagateCostRefCnt[i] = 0;
        }

        m_param->propagateCost_Base = m_param->propagateCostMemFactory[0].busAddress;
        m_param->propagateCost_frame_size = m_param->propagateCostMemFactory[1].busAddress -
                                            m_param->propagateCostMemFactory[0].busAddress;
    }

    queue_init(&m_param->jobs);
    queue_init(&m_param->agop);
    m_param->job_cnt = 0;
    m_param->output_cnt = 0;
    m_param->total_frames = 0;
    vcenc_instance->asic.regs.bInitUpdate = 1;

    memset(m_param->asic.regs.regMirror, 0, sizeof(u32) * ASIC_SWREG_AMOUNT);
    m_param->num_cmds = 0;
    m_param->out_cnt = 0;
    m_param->pop_cnt = 0;
    m_param->rem_frames = 0;
    memset(m_param->qpOutIdx, 0, sizeof(i32) * CUTREE_BUFFER_NUM);
    memset(m_param->commands, 0,
           sizeof(u64) * CUTREE_BUFFER_CNT(VCENC_LOOKAHEAD_MAX, MAX_GOP_SIZE) + MAX_GOP_SIZE);
    m_param->outRoiMapDeltaQpBlockUnit = 0;
    m_param->dsRatio = 0;
    m_param->roiMapEnable = 0;
    m_param->inQpDeltaBlkSize = 0;

    StartCuTreeThread(m_param);
    return VCENC_OK;
}

#endif
