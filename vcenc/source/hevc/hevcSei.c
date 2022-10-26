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
-- Description : HEVC SEI Messages.
--
------------------------------------------------------------------------------*/
#ifdef SEI_BUILD_SUPPORT

#include "vsi_string.h"
#include "hevcSei.h"
#include "sw_put_bits.h"
#include "sw_nal_unit.h"
#include "enccommon.h"
#include "base_type.h"

#define SEI_BUFFERING_PERIOD 0
#define SEI_PIC_TIMING 1
#define SEI_FILLER_PAYLOAD 3
#define SEI_USER_DATA_UNREGISTERED 5
#define SEI_RECOVERY_POINT_PAYLOAD 6
#define SEI_ACTIVE_PARAMETER_SETS 129
#define SEI_SCALABILITY_INFO 24
#define SEI_MASTERING_DISPLAY_COLOR 137
#define SEI_CONTENT_LIGHT_LEVEL 144

/*------------------------------------------------------------------------------
  HevcInitSei()
------------------------------------------------------------------------------*/
void HevcInitSei(sei_s *sei, true_e byteStream, u32 hrd, u32 timeScale, u32 nuit)
{
    ASSERT(sei != NULL);

    sei->byteStream = byteStream;
    sei->hrd = hrd;
    sei->seqId = 0x0;
    sei->psp = (u32)ENCHW_YES;
    sei->cts = (u32)ENCHW_YES;
    /* sei->icrd = 0; */
    sei->icrdLen = 24;
    /* sei->icrdo = 0; */
    sei->icrdoLen = 24;
    /* sei->crd = 0; */
    sei->crdLen = 24;
    /* sei->dod = 0; */
    sei->dodLen = 24;
    sei->ps = 0;
    sei->cntType = 1;
    sei->cdf = 0;
    sei->nframes = 0;
    sei->toffs = 0;

    {
        u32 n = 1;

        while (nuit > (1U << n))
            n++;
        sei->toffsLen = n;
    }

    sei->ts.timeScale = timeScale;
    sei->ts.nuit = nuit;
    sei->ts.time = 0;
    sei->ts.sec = 0;
    sei->ts.min = 0;
    sei->ts.hr = 0;
    sei->ts.fts = (u32)ENCHW_YES;
    sei->ts.secf = (u32)ENCHW_NO;
    sei->ts.minf = (u32)ENCHW_NO;
    sei->ts.hrf = (u32)ENCHW_NO;

    sei->userDataEnabled = (u32)ENCHW_NO;
    sei->userDataSize = 0;
    sei->pUserData = NULL;
    sei->activated_sps = 0;
}

/*------------------------------------------------------------------------------
  HevcUpdateSeiPS()  Calculate new pic_struct.
------------------------------------------------------------------------------*/
void HevcUpdateSeiPS(sei_s *sei, u32 interlacedFrame, u32 bottomfield)
{
    //Table D 2 - Interpretation of pic_struct
    if (interlacedFrame == 0) {
        // progressive frame.
        sei->ps = 0;
    } else {
        if (bottomfield) {
            sei->ps = 2;
        } else
            sei->ps = 1;
    }
}

/*------------------------------------------------------------------------------
  HevcUpdateSeiTS()  Calculate new time stamp.
------------------------------------------------------------------------------*/
void HevcUpdateSeiTS(sei_s *sei, u32 timeInc)
{
    timeStamp_s *ts = &sei->ts;

    ASSERT(sei != NULL);
    timeInc += ts->time;

    while (timeInc >= ts->timeScale) {
        timeInc -= ts->timeScale;
        ts->sec++;
        if (ts->sec == 60) {
            ts->sec = 0;
            ts->min++;
            if (ts->min == 60) {
                ts->min = 0;
                ts->hr++;
                if (ts->hr == 32) {
                    ts->hr = 0;
                }
            }
        }
    }

    ts->time = timeInc;

    sei->nframes = ts->time / ts->nuit;
    sei->toffs = ts->time - sei->nframes * ts->nuit;

    ts->hrf = (ts->hr != 0);
    ts->minf = ts->hrf || (ts->min != 0);
    ts->secf = ts->minf || (ts->sec != 0);

#ifdef TRACE_PIC_TIMING
    DEBUG_PRINT(("Picture Timing: %02i:%02i:%02i  %6i ticks\n", ts->hr, ts->min, ts->sec,
                 (sei->nframes * ts->nuit + sei->toffs)));
#endif
}

/*------------------------------------------------------------------------------
  HevcFillerSei()  Filler payload SEI message. Requested filler payload size
  could be huge. Use of temporary stream buffer is not needed, because size is
  know.
------------------------------------------------------------------------------*/
void HevcFillerSei(struct buffer *sp, sei_s *sei, i32 cnt)
{
    i32 i = cnt;

    struct nal_unit nal_unit_val;

    ASSERT(sp != NULL && sei != NULL);
    if (sei->byteStream == ENCHW_YES) {
        put_bits_startcode(sp);
    }

    nal_unit_val.type = PREFIX_SEI_NUT;
    nal_unit_val.temporal_id = 0;
    nal_unit(sp, &nal_unit_val);

    put_bit(sp, SEI_FILLER_PAYLOAD, 8);
    COMMENT(sp, "last_payload_type_byte")

    while (cnt >= 255) {
        put_bit(sp, 0xFF, 0x8);
        COMMENT(sp, "ff_byte")
        cnt -= 255;
    }
    put_bit(sp, cnt, 8);
    COMMENT(sp, "last_payload_size_byte");

    for (; i > 0; i--) {
        put_bit(sp, 0xFF, 8);
        COMMENT(sp, "filler ff_byte");
    }
    rbsp_trailing_bits(sp);
}

/*------------------------------------------------------------------------------
  HevcBufferingSei()  Buffering period SEI message.
------------------------------------------------------------------------------*/
void HevcBufferingSei(struct buffer *sp, sei_s *sei, vui_t *vui)
{
    u8 *pPayloadSizePos;
    u32 startByteCnt;

    ASSERT(sei != NULL);

    if (sei->hrd == ENCHW_NO) {
        return;
    }

    put_bit(sp, SEI_BUFFERING_PERIOD, 8);
    COMMENT(sp, "last_payload_type_byte");
    //rbsp_flush_bits(sp);
    pPayloadSizePos = sp->stream + (sp->bit_cnt >> 3);

    put_bit(sp, 0xFF, 8); /* this will be updated after we know exact payload size */
    COMMENT(sp, "last_payload_size_byte");
    //rbsp_flush_bits(sp);
    //startByteCnt = *sp->cnt;
    sp->emulCnt = 0; /* count emul_3_byte for this payload */

    put_bit_ue(sp, sei->seqId);
    COMMENT(sp, "seq_parameter_set_id");

    put_bit(sp, 0, 1);
    COMMENT(sp, "irap_cpb_params_present_flag");
    //how to config this
    put_bit(sp, 0, 1);
    COMMENT(sp, "concatenation_flag");

    put_bit_32(sp, 0, vui->cpbRemovalDelayLength);
    COMMENT(sp, "au_cpb_removal_delay_delta_minus1");

    put_bit_32(sp, sei->icrd, vui->initialCpbRemovalDelayLength);
    COMMENT(sp, "nal_initial_cpb_removal_delay[ i ]");

    put_bit_32(sp, sei->icrdo, vui->initialCpbRemovalDelayLength);
    COMMENT(sp, "nal_initial_cpb_removal_offset[ i ]");
    //rbsp_flush_bits(sp);
    if (sp->bit_cnt) {
        rbsp_trailing_bits(sp);
    }

    {
        u32 payload_size;

        payload_size = (u32)(sp->stream - pPayloadSizePos) - 1 - sp->emulCnt;
        *pPayloadSizePos = payload_size;
    }

    /* reset cpb_removal_delay, for I frmae, the value is zero */
    sei->crd = 1;
}

/*------------------------------------------------------------------------------
  HevcPicTimingSei()  Picture timing SEI message.
------------------------------------------------------------------------------*/
void HevcPicTimingSei(struct buffer *sp, sei_s *sei, vui_t *vui)
{
    u8 *pPayloadSizePos;
    u32 startByteCnt;

    ASSERT(sei != NULL);

    put_bit(sp, SEI_PIC_TIMING, 8);
    COMMENT(sp, "last_payload_type_byte");
    //rbsp_flush_bits(sp);
    pPayloadSizePos = sp->stream + (sp->bit_cnt >> 3);

    put_bit(sp, 0xFF, 8); /* this will be updated after we know exact payload size */
    COMMENT(sp, "last_payload_size_byte");
    //rbsp_flush_bits(sp);
    //startByteCnt = *sp->cnt;
    sp->emulCnt = 0; /* count emul_3_byte for this payload */
    put_bit(sp, sei->ps, 4);
    COMMENT(sp, "pic_struct");
    put_bit(sp, !sei->ps, 2);
    COMMENT(sp, "source_scan_type");
    put_bit(sp, 0, 1);
    COMMENT(sp, "duplicate_flag");
    if (sei->hrd) {
        put_bit_32(sp, sei->crd - 1, vui->cpbRemovalDelayLength);
        COMMENT(sp, "au_cpb_removal_delay_minus1");
        put_bit_32(sp, sei->dod, vui->dpbOutputDelayLength);
        COMMENT(sp, "pic_dpb_output_delay");
    }

    if (sp->bit_cnt) {
        rbsp_trailing_bits(sp);
    }

    {
        u32 payload_size;

        payload_size = (u32)(sp->stream - pPayloadSizePos) - 1 - sp->emulCnt;
        *pPayloadSizePos = payload_size;
    }
}
/*------------------------------------------------------------------------------
  HevcPicTimingSei()  Picture timing SEI message.
------------------------------------------------------------------------------*/
void HevcActiveParameterSetsSei(struct buffer *sp, sei_s *sei, vui_t *vui)
{
    u8 *pPayloadSizePos;
    u32 startByteCnt;

    ASSERT(sei != NULL);

    put_bit(sp, SEI_ACTIVE_PARAMETER_SETS, 8);
    COMMENT(sp, "last_payload_type_byte");
    //rbsp_flush_bits(sp);
    pPayloadSizePos = sp->stream + (sp->bit_cnt >> 3);

    put_bit(sp, 0xFF, 8); /* this will be updated after we know exact payload size */
    COMMENT(sp, "last_payload_size_byte");
    //rbsp_flush_bits(sp);
    //startByteCnt = *sp->cnt;
    sp->emulCnt = 0; /* count emul_3_byte for this payload */
    put_bit(sp, sei->seqId, 4);
    COMMENT(sp, "active_video_parameter_set_id");
    put_bit(sp, 0, 1);
    COMMENT(sp, "self_contained_cvs_flag");
    put_bit(sp, 1, 1);
    COMMENT(sp, "no_parameter_set_update_flag");
    put_bit_ue(sp, 0);
    COMMENT(sp, "num_sps_ids_minus1");
    put_bit_ue(sp, 0);
    COMMENT(sp, "active_seq_parameter_set_id[ 0 ]");

    if (sp->bit_cnt) {
        rbsp_trailing_bits(sp);
    }

    {
        u32 payload_size;

        payload_size = (u32)(sp->stream - pPayloadSizePos) - 1 - sp->emulCnt;
        *pPayloadSizePos = payload_size;
    }
}

/*------------------------------------------------------------------------------
  HevcUserDataUnregSei()  User data unregistered SEI message.
------------------------------------------------------------------------------*/
void HevcUserDataUnregSei(struct buffer *sp, sei_s *sei)
{
    u32 i, cnt;
    const u8 *pUserData;

    ASSERT(sei != NULL);
    ASSERT(sei->pUserData != NULL);
    ASSERT(sei->userDataSize >= 16);

    pUserData = sei->pUserData;
    cnt = sei->userDataSize;
    if (sei->userDataEnabled == ENCHW_NO) {
        return;
    }
    put_bit(sp, SEI_USER_DATA_UNREGISTERED, 8);
    COMMENT(sp, "last_payload_type_byte");

    while (cnt >= 255) {
        put_bit(sp, 0xFF, 0x8);
        COMMENT(sp, "ff_byte");
        cnt -= 255;
    }

    put_bit(sp, cnt, 8);
    COMMENT(sp, "last_payload_size_byte");

    /* Write uuid */
    for (i = 0; i < 16; i++) {
        put_bit(sp, pUserData[i], 8);
        COMMENT(sp, "uuid_iso_iec_11578_byte");
    }

    /* Write payload */
    for (i = 16; i < sei->userDataSize; i++) {
        put_bit(sp, pUserData[i], 8);
        COMMENT(sp, "user_data_payload_byte");
    }
}

/*------------------------------------------------------------------------------
  HevcRecoveryPointSei()  recovery point SEI message.
------------------------------------------------------------------------------*/
void HevcRecoveryPointSei(struct buffer *sp, sei_s *sei)
{
    u8 *pPayloadSizePos;
    u32 startByteCnt;

    ASSERT(sei != NULL);

    put_bit(sp, SEI_RECOVERY_POINT_PAYLOAD, 8);
    COMMENT(sp, "last_payload_type_byte");
    //rbsp_flush_bits(sp);
    pPayloadSizePos = sp->stream + (sp->bit_cnt >> 3);

    put_bit(sp, 0xFF, 8); /* this will be updated after we know exact payload size */
    COMMENT(sp, "last_payload_size_byte");
    //rbsp_flush_bits(sp);
    //startByteCnt = *sp->cnt;
    sp->emulCnt = 0; /* count emul_3_byte for this payload */
    put_bit_se(sp, sei->recoveryFrameCnt);
    COMMENT(sp, "recovery_poc_cnt");
    put_bit(sp, 1, 1);
    COMMENT(sp, "exact_match_flag");
    put_bit(sp, 0, 1);
    COMMENT(sp, "broken_link_flag");

    if (sp->bit_cnt) {
        rbsp_trailing_bits(sp);
    }

    {
        u32 payload_size;

        payload_size = (u32)(sp->stream - pPayloadSizePos) - 1 - sp->emulCnt;
        *pPayloadSizePos = payload_size;
    }
}

/*------------------------------------------------------------------------------
  H264ScalabilityInfoSei()  Scalability information SEI message.
------------------------------------------------------------------------------*/
void H264ScalabilityInfoSei(struct buffer *sp, struct sps *s, i32 svctLevel, i32 frameRate)
{
    u8 *pPayloadSizePos;
    u32 startByteCnt;
    i32 i;
    const bool priority_id_setting_flag = HANTRO_TRUE;
    const bool sub_pic_layer_flag = HANTRO_FALSE;
    const bool iroi_division_info_present_flag = HANTRO_FALSE;
    const bool bitrate_info_present_flag = HANTRO_FALSE;
    const bool frm_rate_info_present_flag = HANTRO_TRUE;
    const bool frm_size_info_present_flag = HANTRO_TRUE;
    const bool layer_dependency_info_present_flag = HANTRO_TRUE;
    bool sub_region_layer_flag;
    bool profile_level_info_present_flag;
    bool parameter_sets_info_present_flag;

    put_bit(sp, SEI_SCALABILITY_INFO, 8);
    COMMENT(sp, "last_payload_type_byte");

    pPayloadSizePos = sp->stream + (sp->bit_cnt >> 3);

    put_bit(sp, 0xFF, 8); /* this will be updated after we know exact payload size */
    COMMENT(sp, "last_payload_size_byte");

    sp->emulCnt = 0; /* count emul_3_byte for this payload */

    put_bit(sp, 0, 1);
    COMMENT(sp, "temporal_id_nesting_flag");
    put_bit(sp, 0, 1);
    COMMENT(sp, "priority_layer_info_present_flag");
    put_bit(sp, priority_id_setting_flag, 1);
    COMMENT(sp, "priority_id_setting_flag");
    put_bit_ue(sp, svctLevel);
    COMMENT(sp, "num_layers_minus1");

    for (i = 0; i < svctLevel + 1; i++) {
        put_bit_ue(sp, i);
        COMMENT(sp, "layer_id[i]");
        put_bit(sp, i, 6); /* 0/1 */
        COMMENT(sp, "priority_id[i]");
        put_bit(sp, 1, 1);
        COMMENT(sp, "discardable_flag[i]");
        put_bit(sp, 0, 3);
        COMMENT(sp, "dependency_id[i]");
        put_bit(sp, 0, 4);
        COMMENT(sp, "quality_id[i]");
        put_bit(sp, i, 3);
        COMMENT(sp, "temporal_id[i]");
        put_bit(sp, sub_pic_layer_flag, 1);
        COMMENT(sp, "sub_pic_layer_flag[i]");
        sub_region_layer_flag = 0; /*(i==0)?HANTRO_TRUE:HANTRO_FALSE; */
        put_bit(sp, sub_region_layer_flag, 1);
        COMMENT(sp, "sub_region_layer_flag[i]");
        put_bit(sp, iroi_division_info_present_flag, 1);
        COMMENT(sp, "iroi_division_info_present_flag[i]");
        profile_level_info_present_flag = 0; /* (i==0)?HANTRO_TRUE:HANTRO_FALSE; */
        put_bit(sp, profile_level_info_present_flag, 1);
        COMMENT(sp, "profile_level_info_present_flag[i]");
        put_bit(sp, bitrate_info_present_flag, 1);
        COMMENT(sp, "bitrate_info_present_flag[i]");
        put_bit(sp, frm_rate_info_present_flag, 1);
        COMMENT(sp, "frm_rate_info_present_flag[i]");
        put_bit(sp, frm_size_info_present_flag, 1);
        COMMENT(sp, "frm_size_info_present_flag[i]");
        put_bit(sp, layer_dependency_info_present_flag, 1); /* HANTRO_TRUE */
        COMMENT(sp, "layer_dependency_info_present_flag[i]");
        parameter_sets_info_present_flag = (i == 0) ? HANTRO_TRUE : HANTRO_FALSE;
        put_bit(sp, parameter_sets_info_present_flag, 1);
        COMMENT(sp, "parameter_sets_info_present_flag[i]");
        put_bit(sp, 0, 1);
        COMMENT(sp, "bitstream_restriction_info_present_flag[i]");
        put_bit(sp, 1, 1);
        COMMENT(sp, "exact_inter_layer_pred_flag[i]");
        if (sub_pic_layer_flag || iroi_division_info_present_flag) {
            put_bit(sp, 0, 1);
            COMMENT(sp, "exact_sample_value_match_flag[i]");
        }
        put_bit(sp, 0, 1);
        COMMENT(sp, "layer_conversion_flag[i]");
        put_bit(sp, 1, 1); /* output */
        COMMENT(sp, "layer_output_flag[i]");
        if (profile_level_info_present_flag) {
            put_bit(sp, 0, 8);
            put_bit(sp, 0, 8);
            put_bit(sp, 0, 8);
            COMMENT(sp, "layer_profile_level_idc[i]");
        }
        if (bitrate_info_present_flag) {
#if 0  /*       */
            avg_bitrate[ i ] 5 u(16)
            max_bitrate_layer[ i ] 5 u(16)
            max_bitrate_layer_representation[ i ] 5 u(16)
            max_bitrate_calc_window[ i ] 5 u(16)
#endif /*       */
        }
        if (frm_rate_info_present_flag) {
            u32 frame_rate;
            put_bit(sp, 0, 2); /* constant frame rate */
            COMMENT(sp, "constant_frm_rate_idc[i]");
            frame_rate = frameRate >> (svctLevel - i);
            put_bit_32(sp, frame_rate, 16); /* constant frame rate */
            COMMENT(sp, "avg_frm_rate[i]");
        }
        if (frm_size_info_present_flag || iroi_division_info_present_flag) {
            /* frm_width_in_mbs_minus1[ i ] 5 ue(v) */
            /* frm_height_in_mbs_minus1[ i ] 5 ue(v) */
            put_bit_ue(sp, s->picWidthInMbsMinus1);
            COMMENT(sp, "pic_width_in_mbs_minus1");

            put_bit_ue(sp, s->picHeightInMapUnitsMinus1);
            COMMENT(sp, "pic_height_in_map_units_minus1");
        }
#if 0
        if ( sub_region_layer_flag[ i ] ) {
            base_region_layer_id[ i ] 5 ue(v)
            dynamic_rect_flag[ i ] 5 u(1)
            if ( !dynamic_rect_flag[ i ] ) {
                horizontal_offset[ i ] 5 u(16)
                vertical_offset[ i ] 5 u(16)
                region_width[ i ] 5 u(16)
                region_height[ i ] 5 u(16)
            }
        }
        if ( sub_pic_layer_flag )
        {
            roi_id[ i ] 5 ue(v)
        }
        if ( iroi_division_info_present_flag)
        {
            iroi_grid_flag[ i ] 5 u(1)
            if ( iroi_grid_flag[ i ] ) {
              grid_width_in_mbs_minus1[ i ] 5 ue(v)
              grid_height_in_mbs_minus1[ i ] 5 ue(v)
            } else {
              num_rois_minus1[ i ] 5 ue(v)
                for (j = 0; j <= num_rois_minus1[ i ]; j++ ) {
                    first_mb_in_roi[ i ][ j ] 5 ue(v)
                    roi_width_in_mbs_minus1[ i ][ j ] 5 ue(v)
                    roi_height_in_mbs_minus1[ i ][ j ] 5 ue(v)
                }
            }
        }
#endif
        if (layer_dependency_info_present_flag) {
            u32 num_directly_dependent_layers;
            u32 j;
            num_directly_dependent_layers = (i == 0) ? 0 : 1;
            put_bit_ue(sp, num_directly_dependent_layers);
            COMMENT(sp, "num_directly_dependent_layers[i]");
            for (j = 0; j < num_directly_dependent_layers; j++) {
                put_bit_ue(sp, 0);
                COMMENT(sp, "directly_dependent_layer_id_delta_minus1[i]");
            }
        } else {
            put_bit_ue(sp, 0);
            COMMENT(sp, "layer_dependency_info_src_layer_id_delta[i]");
        }
        if (parameter_sets_info_present_flag) {
            u32 j;
            u32 num_seq_parameter_set_minus1 = 0;
            u32 num_subset_seq_parameter_set_minus1 = 0;
            u32 num_pic_parameter_set_minus1 = 0;
            put_bit_ue(sp, num_seq_parameter_set_minus1);
            COMMENT(sp, "num_seq_parameter_set_minus1[i]");
            for (j = 0; j <= num_seq_parameter_set_minus1; j++) {
                put_bit_ue(sp, 0);
                COMMENT(sp, "seq_parameter_set_id_delta[i]");
            }
            put_bit_ue(sp, num_subset_seq_parameter_set_minus1);
            COMMENT(sp, "num_subset_seq_parameter_set_minus1[i]");
            for (j = 0; j <= num_subset_seq_parameter_set_minus1; j++) {
                put_bit_ue(sp, 0);
                COMMENT(sp, "subset_seq_parameter_set_id_delta[i]");
            }
            put_bit_ue(sp, num_pic_parameter_set_minus1);
            COMMENT(sp, "num_pic_parameter_set_minus1[i]");
            for (j = 0; j <= num_pic_parameter_set_minus1; j++) {
                put_bit_ue(sp, 0);
                COMMENT(sp, "pic_parameter_set_id_delta[i]");
            }
        } else {
            put_bit_ue(sp, 0);
            COMMENT(sp, "parameter_sets_info_src_layer_id_delta[i]");
        }
#if 0
        if ( bitstream_restriction_info_present_flag[ i ] ) {
            motion_vectors_over_pic_boundaries_flag[ i ] 5 u(1)
            max_bytes_per_pic_denom[ i ] 5 ue(v)
            max_bits_per_mb_denom[ i ] 5 ue(v)
            log2_max_mv_length_horizontal[ i ] 5 ue(v)
            log2_max_mv_length_vertical[ i ] 5 ue(v)
            num_reorder_frames[ i ] 5 ue(v)
            max_dec_frame_buffering[ i ] 5 ue(v)
        }
        if ( layer_conversion_flag[ i ] ) {
            conversion_type_idc[ i ] 5 ue(v)
            for ( j=0; j < 2; j++ ) {
                rewriting_info_flag[ i ][ j ] 5 u(1)
                if ( rewriting_info_flag[ i ][ j ] ) {
                    rewriting_profile_level_idc[ i ][ j ] 5 u(24)
                    rewriting_avg_bitrate[ i ][ j ] 5 u(16)
                    rewriting_max_bitrate[ i ][ j ] 5 u(16)
                }
            }
        }
#endif
    }

#if 0
    if ( priority_layer_info_present_flag ) {
        pr_num_dId_minus1 5 ue(v)
        for ( i = 0; i <= pr_num_dId_minus1; i++ ) {
            pr_dependency_id[ i ] 5 u(3)
            pr_num_minus1[ i ] 5 ue(v)
            for ( j = 0; j <= pr_num_minus1[ i ]; j++ ) {
                pr_id[ i ][ j ] 5 ue(v)
                pr_profile_level_idc[ i ][ j ] 5 u(24)
                pr_avg_bitrate[ i ][ j ] 5 u(16)
                pr_max_bitrate[ i ][ j ] 5 u(16)
            }
        }
    }
#endif

    if (priority_id_setting_flag) {
        int PriorityIdSettingUriIdx = 0;
        char priority_id_setting_uri[20] = "http://svc.com";
        do {
            put_bit(sp, priority_id_setting_uri[PriorityIdSettingUriIdx], 8);
            COMMENT(sp, "priority_id_setting_uri[i]");
        } while (priority_id_setting_uri[PriorityIdSettingUriIdx++] != 0);
    }

    if (sp->bit_cnt) {
        rbsp_trailing_bits(sp);
    }

    {
        u32 payload_size;

        payload_size = (u32)(sp->stream - pPayloadSizePos) - 1 - sp->emulCnt;
        *pPayloadSizePos = payload_size;
    }
}

/*------------------------------------------------------------------------------
HevcMasteringDisplayColourSei()  Mastering display colour volume SEI message.
------------------------------------------------------------------------------*/
void HevcMasteringDisplayColourSei(struct buffer *sp, Hdr10DisplaySei *pDisplaySei)
{
    ASSERT(sp != NULL);
    ASSERT(pDisplaySei != NULL);

    put_bit(sp, SEI_MASTERING_DISPLAY_COLOR, 8);
    COMMENT(sp, "mastering_display_colour_volume");

    put_bit(sp, 24, 8); /* this will be updated after we know exact payload size */
    COMMENT(sp, "last_payload_size_byte");

    put_bit_32(sp, pDisplaySei->hdr10_dx0, 16);
    COMMENT(sp, "display_primaries_x_c0");
    put_bit_32(sp, pDisplaySei->hdr10_dy0, 16);
    COMMENT(sp, "display_primaries_y_c0");

    put_bit_32(sp, pDisplaySei->hdr10_dx1, 16);
    COMMENT(sp, "display_primaries_x_c1");
    put_bit_32(sp, pDisplaySei->hdr10_dy1, 16);
    COMMENT(sp, "display_primaries_y_c1");
    put_bit_32(sp, pDisplaySei->hdr10_dx2, 16);
    COMMENT(sp, "display_primaries_x_c2");
    put_bit_32(sp, pDisplaySei->hdr10_dy2, 16);
    COMMENT(sp, "display_primaries_y_c2");

    put_bit_32(sp, pDisplaySei->hdr10_wx, 16);
    COMMENT(sp, "white_point_x");
    put_bit_32(sp, pDisplaySei->hdr10_wy, 16);
    COMMENT(sp, "white_point_y");

    put_bit_32(sp, pDisplaySei->hdr10_maxluma, 32);
    COMMENT(sp, "max_display_mastering_luminance");
    put_bit_32(sp, pDisplaySei->hdr10_minluma, 32);
    COMMENT(sp, "min_display_mastering_luminance");

    if (sp->bit_cnt) {
        rbsp_trailing_bits(sp);
    }
}

/*------------------------------------------------------------------------------
HevcContentLightLevelSei()  Content light level info SEI message.
------------------------------------------------------------------------------*/
void HevcContentLightLevelSei(struct buffer *sp, Hdr10LightLevelSei *pLightSei)
{
    ASSERT(sp != NULL);
    ASSERT(pLightSei != NULL);

    put_bit(sp, SEI_CONTENT_LIGHT_LEVEL, 8);
    COMMENT(sp, "content_light_level_info");

    put_bit(sp, 4, 8);
    COMMENT(sp, "last_payload_size_byte");

    put_bit_32(sp, pLightSei->hdr10_maxlight, 16);
    COMMENT(sp, "max_content_light_level");
    put_bit_32(sp, pLightSei->hdr10_avglight, 16);
    COMMENT(sp, "max_pic_average_light_level");

    if (sp->bit_cnt) {
        rbsp_trailing_bits(sp);
    }
}

/*------------------------------------------------------------------------------
  HevcExternalSei()  external SEI message.
------------------------------------------------------------------------------*/
void HevcExternalSei(struct buffer *sp, u8 type, u8 *content, u32 size)
{
    u32 i, cnt;

    ASSERT(sp != NULL);
    cnt = size;

    /* Write payload type*/
    put_bit(sp, type, 8);
    COMMENT(sp, "last_payload_type_byte");

    /* Write payload size*/
    while (cnt >= 255) {
        put_bit(sp, 0xFF, 0x8);
        COMMENT(sp, "ff_byte");
        cnt -= 255;
    }
    put_bit(sp, cnt, 8);
    COMMENT(sp, "last_payload_size_byte");

    /* Write payload content*/
    for (i = 0; i < size; i++) {
        put_bit(sp, content[i], 8);
        COMMENT(sp, "external_payload_byte");
    }
}

/*------------------------------------------------------------------------------
  H264InitSei()
------------------------------------------------------------------------------*/
void H264InitSei(sei_s *sei, true_e byteStream, u32 hrd, u32 timeScale, u32 nuit)
{
    ASSERT(sei != NULL);

    sei->byteStream = byteStream;
    sei->hrd = hrd;
    sei->seqId = 0x0;
    sei->psp = (u32)ENCHW_YES;
    sei->cts = (u32)ENCHW_YES;
    /* sei->icrd = 0; */
    sei->icrdLen = 24;
    /* sei->icrdo = 0; */
    sei->icrdoLen = 24;
    /* sei->crd = 0; */
    sei->crdLen = 24;
    /* sei->dod = 0; */
    sei->dodLen = 24;
    sei->ps = 0;
    sei->cntType = 1;
    sei->cdf = 0;
    sei->nframes = 0;
    sei->toffs = 0;

    {
        u32 n = 1;

        while (nuit > (1U << n))
            n++;
        sei->toffsLen = n;
    }

    sei->ts.timeScale = timeScale;
    sei->ts.nuit = nuit;
    sei->ts.time = 0;
    sei->ts.sec = 0;
    sei->ts.min = 0;
    sei->ts.hr = 0;
    sei->ts.fts = (u32)ENCHW_YES;
    sei->ts.secf = (u32)ENCHW_NO;
    sei->ts.minf = (u32)ENCHW_NO;
    sei->ts.hrf = (u32)ENCHW_NO;

    sei->userDataEnabled = (u32)ENCHW_NO;
    sei->userDataSize = 0;
    sei->pUserData = NULL;
}

/*------------------------------------------------------------------------------
  H264UpdateSeiTS()  Calculate new time stamp.
------------------------------------------------------------------------------*/
void H264UpdateSeiTS(sei_s *sei, u32 timeInc)
{
    timeStamp_s *ts = &sei->ts;

    ASSERT(sei != NULL);
    timeInc += ts->time;

    while (timeInc >= ts->timeScale) {
        timeInc -= ts->timeScale;
        ts->sec++;
        if (ts->sec == 60) {
            ts->sec = 0;
            ts->min++;
            if (ts->min == 60) {
                ts->min = 0;
                ts->hr++;
                if (ts->hr == 32) {
                    ts->hr = 0;
                }
            }
        }
    }

    ts->time = timeInc;

    sei->nframes = ts->time / ts->nuit;
    sei->toffs = ts->time - sei->nframes * ts->nuit;

    ts->hrf = (ts->hr != 0);
    ts->minf = ts->hrf || (ts->min != 0);
    ts->secf = ts->minf || (ts->sec != 0);

#ifdef TRACE_PIC_TIMING
    DEBUG_PRINT(("Picture Timing: %02i:%02i:%02i  %6i ticks\n", ts->hr, ts->min, ts->sec,
                 (sei->nframes * ts->nuit + sei->toffs)));
#endif
}

/*------------------------------------------------------------------------------
  H264FillerSei()  Filler payload SEI message. Requested filler payload size
  could be huge. Use of temporary stream buffer is not needed, because size is
  know.
------------------------------------------------------------------------------*/
void H264FillerSei(struct buffer *sp, sei_s *sei, i32 cnt)
{
    i32 i = cnt;

    ASSERT(sp != NULL && sei != NULL);

    H264NalUnitHdr(sp, 0, H264_SEI, sei->byteStream);

    put_bit(sp, SEI_FILLER_PAYLOAD, 8);
    COMMENT(sp, "last_payload_type_byte");

    while (cnt >= 255) {
        put_bit(sp, 0xFF, 0x8);
        COMMENT(sp, "ff_byte");
        cnt -= 255;
    }
    put_bit(sp, cnt, 8);
    COMMENT(sp, "last_payload_size_byte");

    for (; i > 0; i--) {
        put_bit(sp, 0xFF, 8);
        COMMENT(sp, "filler ff_byte");
    }
    rbsp_trailing_bits(sp);
}

/*------------------------------------------------------------------------------
  H264BufferingSei()  Buffering period SEI message.
------------------------------------------------------------------------------*/
void H264BufferingSei(struct buffer *sp, sei_s *sei)
{
    u8 *pPayloadSizePos;
    u32 startByteCnt;

    ASSERT(sei != NULL);

    if (sei->hrd == ENCHW_NO) {
        return;
    }

    put_bit(sp, SEI_BUFFERING_PERIOD, 8);
    COMMENT(sp, "last_payload_type_byte");

    pPayloadSizePos = sp->stream + (sp->bit_cnt >> 3);

    put_bit(sp, 0xFF, 8); /* this will be updated after we know exact payload size */
    COMMENT(sp, "last_payload_size_byte");

    //startByteCnt = sp->byteCnt;
    sp->emulCnt = 0; /* count emul_3_byte for this payload */

    put_bit_ue(sp, sei->seqId);
    COMMENT(sp, "seq_parameter_set_id");

    put_bit_32(sp, sei->icrd, sei->icrdLen);
    COMMENT(sp, "initial_cpb_removal_delay");

    put_bit_32(sp, sei->icrdo, sei->icrdoLen);
    COMMENT(sp, "initial_cpb_removal_delay_offset");

    if (sp->bit_cnt) {
        rbsp_trailing_bits(sp);
    }

    {
        u32 payload_size;

        payload_size = (u32)(sp->stream - pPayloadSizePos) - 1 - sp->emulCnt;
        *pPayloadSizePos = payload_size;
    }

    /* reset cpb_removal_delay */
    sei->crd = 0;
}

/*------------------------------------------------------------------------------
  TimeStamp()
------------------------------------------------------------------------------*/
static void TimeStamp(struct buffer *sp, timeStamp_s *ts)
{
    if (ts->fts) {
        put_bit(sp, ts->sec, 6);
        COMMENT(sp, "seconds_value");
        put_bit(sp, ts->min, 6);
        COMMENT(sp, "minutes_value");
        put_bit(sp, ts->hr, 5);
        COMMENT(sp, "hours_value");
    } else {
        put_bit(sp, ts->secf, 1);
        COMMENT(sp, "seconds_flag");
        if (ts->secf) {
            put_bit(sp, ts->sec, 6);
            COMMENT(sp, "seconds_value");
            put_bit(sp, ts->minf, 1);
            COMMENT(sp, "minutes_flag");
            if (ts->minf) {
                put_bit(sp, ts->min, 6);
                COMMENT(sp, "minutes_value");
                put_bit(sp, ts->hrf, 1);
                COMMENT(sp, "hours_flag");
                if (ts->hrf) {
                    put_bit(sp, ts->hr, 5);
                    COMMENT(sp, "hours_value");
                }
            }
        }
    }
}

/*------------------------------------------------------------------------------
  H264PicTimingSei()  Picture timing SEI message.
------------------------------------------------------------------------------*/
void H264PicTimingSei(struct buffer *sp, sei_s *sei)
{
    u8 *pPayloadSizePos;
    u32 startByteCnt;

    ASSERT(sei != NULL);

    put_bit(sp, SEI_PIC_TIMING, 8);
    COMMENT(sp, "last_payload_type_byte");

    pPayloadSizePos = sp->stream + (sp->bit_cnt >> 3);

    put_bit(sp, 0xFF, 8); /* this will be updated after we know exact payload size */
    COMMENT(sp, "last_payload_size_byte");

    //startByteCnt = sp->byteCnt;
    sp->emulCnt = 0; /* count emul_3_byte for this payload */

    if (sei->hrd) {
        u32 DeltaTfiDivisor = 1;
        /* progressive frame use 2x timing scale. see Table E-6 & Table D-1 */
        if (0 == sei->psp) {
            DeltaTfiDivisor = 2;
        }
        put_bit_32(sp, sei->crd * DeltaTfiDivisor, sei->crdLen);
        COMMENT(sp, "cpb_removal_delay");
        put_bit_32(sp, sei->dod, sei->dodLen);
        COMMENT(sp, "dpb_output_delay");
    }

    if (sei->psp) {
        put_bit(sp, sei->ps, 4);
        COMMENT(sp, "pic_struct");
        put_bit(sp, sei->cts, 1);
        COMMENT(sp, "clock_timestamp_flag");
        if (sei->cts) {
            put_bit(sp, 0, 2);
            COMMENT(sp, "ct_type");
            put_bit(sp, 0, 1);
            COMMENT(sp, "nuit_field_based_flag");
            put_bit(sp, sei->cntType, 5);
            COMMENT(sp, "counting_type");
            put_bit(sp, sei->ts.fts, 1);
            COMMENT(sp, "full_timestamp_flag");
            put_bit(sp, 0, 1);
            COMMENT(sp, "discontinuity_flag");
            put_bit(sp, sei->cdf, 1);
            COMMENT(sp, "cnt_dropped_flag");
            put_bit(sp, sei->nframes, 8);
            COMMENT(sp, "n_frames");
            TimeStamp(sp, &sei->ts);
            if (sei->toffsLen > 0) {
                put_bit_32(sp, sei->toffs, sei->toffsLen);
                COMMENT(sp, "time_offset");
            }
        }
    }

    if (sp->bit_cnt) {
        rbsp_trailing_bits(sp);
    }

    {
        u32 payload_size;

        payload_size = (u32)(sp->stream - pPayloadSizePos) - 1 - sp->emulCnt;
        *pPayloadSizePos = payload_size;
    }
}

/*------------------------------------------------------------------------------
  H264UserDataUnregSei()  User data unregistered SEI message.
------------------------------------------------------------------------------*/
void H264UserDataUnregSei(struct buffer *sp, sei_s *sei)
{
    u32 i, cnt;
    const u8 *pUserData;
    ASSERT(sei != NULL);
    ASSERT(sei->pUserData != NULL);
    ASSERT(sei->userDataSize >= 16);

    pUserData = sei->pUserData;
    cnt = sei->userDataSize;
    if (sei->userDataEnabled == ENCHW_NO) {
        return;
    }

    put_bit(sp, SEI_USER_DATA_UNREGISTERED, 8);
    COMMENT(sp, "last_payload_type_byte");

    while (cnt >= 255) {
        put_bit(sp, 0xFF, 0x8);
        COMMENT(sp, "ff_byte");
        cnt -= 255;
    }

    put_bit(sp, cnt, 8);
    COMMENT(sp, "last_payload_size_byte");

    /* Write uuid */
    for (i = 0; i < 16; i++) {
        put_bit(sp, pUserData[i], 8);
        COMMENT(sp, "uuid_iso_iec_11578_byte");
    }

    /* Write payload */
    for (i = 16; i < sei->userDataSize; i++) {
        put_bit(sp, pUserData[i], 8);
        COMMENT(sp, "user_data_payload_byte");
    }
}
/*------------------------------------------------------------------------------
  HevcRecoveryPointSei()  recovery point SEI message.
------------------------------------------------------------------------------*/
void H264RecoveryPointSei(struct buffer *sp, sei_s *sei)
{
    u8 *pPayloadSizePos;
    u32 startByteCnt;

    ASSERT(sei != NULL);

    put_bit(sp, SEI_RECOVERY_POINT_PAYLOAD, 8);
    COMMENT(sp, "last_payload_type_byte");

    pPayloadSizePos = sp->stream + (sp->bit_cnt >> 3);

    put_bit(sp, 0xFF, 8); /* this will be updated after we know exact payload size */
    COMMENT(sp, "last_payload_size_byte");

    //startByteCnt = sp->byteCnt;
    sp->emulCnt = 0; /* count emul_3_byte for this payload */

    put_bit_ue(sp, sei->recoveryFrameCnt - 1);
    COMMENT(sp, "recovery_frame_cnt");

    put_bit(sp, 1, 1);
    COMMENT(sp, "exact_match_flag");

    put_bit(sp, 0, 1);
    COMMENT(sp, "broken_link_flag");

    put_bit(sp, 0, 2);
    COMMENT(sp, "changing_slice_group_idc");

    if (sp->bit_cnt) {
        rbsp_trailing_bits(sp);
    }

    {
        u32 payload_size;

        payload_size = (u32)(sp->stream - pPayloadSizePos) - 1 - sp->emulCnt;
        *pPayloadSizePos = payload_size;
    }
}

/*------------------------------------------------------------------------------
  H264ExternalSei()  external SEI message.
------------------------------------------------------------------------------*/
void H264ExternalSei(struct buffer *sp, u8 type, u8 *content, u32 size)
{
    u32 i, cnt;
    ASSERT(content != NULL);

    cnt = size;

    /* Write payload type*/
    put_bit(sp, type, 8);
    COMMENT(sp, "last_payload_type_byte");

    /* Write payload size*/
    while (cnt >= 255) {
        put_bit(sp, 0xFF, 0x8);
        COMMENT(sp, "ff_byte");
        cnt -= 255;
    }
    put_bit(sp, cnt, 8);
    COMMENT(sp, "last_payload_size_byte");

    /* Write payload content*/
    for (i = 0; i < size; i++) {
        put_bit(sp, content[i], 8);
        COMMENT(sp, "external_payload_byte");
    }
}

#endif
