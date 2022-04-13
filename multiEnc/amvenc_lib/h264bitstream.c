/*
 **
 ** Copyright 2012 The Android Open Source Project
 **
 ** Licensed under the Apache License, Version 2.0 (the "License");
 ** you may not use this file except in compliance with the License.
 ** You may obtain a copy of the License at
 **
 **     http://www.apache.org/licenses/LICENSE-2.0
 **
 ** Unless required by applicable law or agreed to in writing, software
 ** distributed under the License is distributed on an "AS IS" BASIS,
 ** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 ** See the License for the specific language governing permissions and
 ** limitations under the License.
 */

#include "h264bitstream.h"

//7.3.2.11 RBSP trailing bits syntax
void read_rbsp_trailing_bits(bs_t* b)
{
    /* rbsp_stop_one_bit */ bs_skip_u(b, 1);

    while ( !bs_byte_aligned(b) )
    {
        /* rbsp_alignment_zero_bit */ bs_skip_u(b, 1);
    }
}

//7.3.2.11 RBSP trailing bits syntax
void write_rbsp_trailing_bits(bs_t* b)
{
    /* rbsp_stop_one_bit */ bs_write_u(b, 1, 1);

    while ( !bs_byte_aligned(b) )
    {
        /* rbsp_alignment_zero_bit */ bs_write_u(b, 1, 0);
    }
}

/**
 Calculate the log base 2 of the argument, rounded up.
 Zero or negative arguments return zero
 Idea from http://www.southwindsgames.com/blog/2009/01/19/fast-integer-log2-function-in-cc/
 */
int intlog2(int x)
{
    int log = 0;
    if (x < 0) { x = 0; }
    while ((x >> log) > 0)
    {
        log++;
    }
    if (log > 0 && x == 1<<(log-1)) { log--; }
    return log;
}


int more_rbsp_data(bs_t* bs)
{
    // TODO this version handles reading only. writing version?

    // no more data
    if (bs_eof(bs)) { return 0; }

    // no rbsp_stop_bit yet
    if (bs_peek_u1(bs) == 0) { return 1; }

    // next bit is 1, is it the rsbp_stop_bit? only if the rest of bits are 0
    bs_t bs_tmp;
    bs_clone(&bs_tmp, bs);
    bs_skip_u1(&bs_tmp);
    while (!bs_eof(&bs_tmp))
    {
        // A later bit was 1, it wasn't the rsbp_stop_bit
        if (bs_read_u1(&bs_tmp) == 1) { return 1; }
    }

    // All following bits were 0, it was the rsbp_stop_bit
    return 0;
}




//7.3.2.1.1 Scaling list syntax
void read_scaling_list(bs_t* b, int* scalingList, int sizeOfScalingList, int* useDefaultScalingMatrixFlag )
{
    // NOTE need to be able to set useDefaultScalingMatrixFlag when reading, hence passing as pointer
    int lastScale = 8;
    int nextScale = 8;
    int delta_scale;
    for ( int j = 0; j < sizeOfScalingList; j++ )
    {
        if ( nextScale != 0 )
        {
            if ( 0 )
            {
                nextScale = scalingList[ j ];
                if (useDefaultScalingMatrixFlag[0]) { nextScale = 0; }
                delta_scale = (nextScale - lastScale) % 256 ;
            }

            delta_scale = bs_read_se(b);

            if ( 1 )
            {
                nextScale = ( lastScale + delta_scale + 256 ) % 256;
                useDefaultScalingMatrixFlag[0] = ( j == 0 && nextScale == 0 );
            }
        }
        if ( 1 )
        {
            scalingList[ j ] = ( nextScale == 0 ) ? lastScale : nextScale;
        }
        lastScale = scalingList[ j ];
    }
}


//7.3.2.1.1 Scaling list syntax
void write_scaling_list(bs_t* b, int* scalingList, int sizeOfScalingList, int* useDefaultScalingMatrixFlag )
{
    // NOTE need to be able to set useDefaultScalingMatrixFlag when reading, hence passing as pointer
    int lastScale = 8;
    int nextScale = 8;
    int delta_scale;
    for ( int j = 0; j < sizeOfScalingList; j++ )
    {
        if ( nextScale != 0 )
        {
            if ( 1 )
            {
                nextScale = scalingList[ j ];
                if (useDefaultScalingMatrixFlag[0]) { nextScale = 0; }
                delta_scale = (nextScale - lastScale) % 256 ;
            }

            bs_write_se(b, delta_scale);

            if ( 0 )
            {
                nextScale = ( lastScale + delta_scale + 256 ) % 256;
                useDefaultScalingMatrixFlag[0] = ( j == 0 && nextScale == 0 );
            }
        }
        if ( 0 )
        {
            scalingList[ j ] = ( nextScale == 0 ) ? lastScale : nextScale;
        }
        lastScale = scalingList[ j ];
    }
}


//Appendix E.1.2 HRD parameters syntax
void read_hrd_parameters(hrd_t* hrd, bs_t* b)
{
    hrd->cpb_cnt_minus1 = bs_read_ue(b);
    hrd->bit_rate_scale = bs_read_u(b, 4);
    hrd->cpb_size_scale = bs_read_u(b, 4);
    for ( int SchedSelIdx = 0; SchedSelIdx <= hrd->cpb_cnt_minus1; SchedSelIdx++ )
    {
        hrd->bit_rate_value_minus1[ SchedSelIdx ] = bs_read_ue(b);
        hrd->cpb_size_value_minus1[ SchedSelIdx ] = bs_read_ue(b);
        hrd->cbr_flag[ SchedSelIdx ] = bs_read_u1(b);
    }
    hrd->initial_cpb_removal_delay_length_minus1 = bs_read_u(b, 5);
    hrd->cpb_removal_delay_length_minus1 = bs_read_u(b, 5);
    hrd->dpb_output_delay_length_minus1 = bs_read_u(b, 5);
    hrd->time_offset_length = bs_read_u(b, 5);
}


//Appendix E.1.2 HRD parameters syntax
void write_hrd_parameters(hrd_t* hrd, bs_t* b)
{
    bs_write_ue(b, hrd->cpb_cnt_minus1);
    bs_write_u(b, 4, hrd->bit_rate_scale);
    bs_write_u(b, 4, hrd->cpb_size_scale);
    for ( int SchedSelIdx = 0; SchedSelIdx <= hrd->cpb_cnt_minus1; SchedSelIdx++ )
    {
        bs_write_ue(b, hrd->bit_rate_value_minus1[ SchedSelIdx ]);
        bs_write_ue(b, hrd->cpb_size_value_minus1[ SchedSelIdx ]);
        bs_write_u1(b, hrd->cbr_flag[ SchedSelIdx ]);
    }
    bs_write_u(b, 5, hrd->initial_cpb_removal_delay_length_minus1);
    bs_write_u(b, 5, hrd->cpb_removal_delay_length_minus1);
    bs_write_u(b, 5, hrd->dpb_output_delay_length_minus1);
    bs_write_u(b, 5, hrd->time_offset_length);
}


//Appendix E.1.1 VUI parameters syntax
void read_vui_parameters(sps_t* sps, bs_t* b)
{
    sps->vui.aspect_ratio_info_present_flag = bs_read_u1(b);
    if ( sps->vui.aspect_ratio_info_present_flag )
    {
        sps->vui.aspect_ratio_idc = bs_read_u8(b);
        if ( sps->vui.aspect_ratio_idc == SAR_Extended )
        {
            sps->vui.sar_width = bs_read_u(b, 16);
            sps->vui.sar_height = bs_read_u(b, 16);
        }
    }
    sps->vui.overscan_info_present_flag = bs_read_u1(b);
    if ( sps->vui.overscan_info_present_flag )
    {
        sps->vui.overscan_appropriate_flag = bs_read_u1(b);
    }
    sps->vui.video_signal_type_present_flag = bs_read_u1(b);
    if ( sps->vui.video_signal_type_present_flag )
    {
        sps->vui.video_format = bs_read_u(b, 3);
        sps->vui.video_full_range_flag = bs_read_u1(b);
        sps->vui.colour_description_present_flag = bs_read_u1(b);
        if ( sps->vui.colour_description_present_flag )
        {
            sps->vui.colour_primaries = bs_read_u8(b);
            sps->vui.transfer_characteristics = bs_read_u8(b);
            sps->vui.matrix_coefficients = bs_read_u8(b);
        }
    }
    sps->vui.chroma_loc_info_present_flag = bs_read_u1(b);
    if ( sps->vui.chroma_loc_info_present_flag )
    {
        sps->vui.chroma_sample_loc_type_top_field = bs_read_ue(b);
        sps->vui.chroma_sample_loc_type_bottom_field = bs_read_ue(b);
    }
    sps->vui.timing_info_present_flag = bs_read_u1(b);
    if ( sps->vui.timing_info_present_flag )
    {
        sps->vui.num_units_in_tick = bs_read_u(b, 32);
        sps->vui.time_scale = bs_read_u(b, 32);
        sps->vui.fixed_frame_rate_flag = bs_read_u1(b);
    }
    sps->vui.nal_hrd_parameters_present_flag = bs_read_u1(b);
    if ( sps->vui.nal_hrd_parameters_present_flag )
    {
        read_hrd_parameters(&sps->hrd_nal, b);
    }
    sps->vui.vcl_hrd_parameters_present_flag = bs_read_u1(b);
    if ( sps->vui.vcl_hrd_parameters_present_flag )
    {
        read_hrd_parameters(&sps->hrd_vcl, b);
    }
    if ( sps->vui.nal_hrd_parameters_present_flag || sps->vui.vcl_hrd_parameters_present_flag )
    {
        sps->vui.low_delay_hrd_flag = bs_read_u1(b);
    }
    sps->vui.pic_struct_present_flag = bs_read_u1(b);
    sps->vui.bitstream_restriction_flag = bs_read_u1(b);
    if ( sps->vui.bitstream_restriction_flag )
    {
        sps->vui.motion_vectors_over_pic_boundaries_flag = bs_read_u1(b);
        sps->vui.max_bytes_per_pic_denom = bs_read_ue(b);
        sps->vui.max_bits_per_mb_denom = bs_read_ue(b);
        sps->vui.log2_max_mv_length_horizontal = bs_read_ue(b);
        sps->vui.log2_max_mv_length_vertical = bs_read_ue(b);
        sps->vui.num_reorder_frames = bs_read_ue(b);
        sps->vui.max_dec_frame_buffering = bs_read_ue(b);
    }
}

//7.3.2.1 Sequence parameter set RBSP syntax
void read_seq_parameter_set_rbsp(sps_t* sps, bs_t* b)
{
    int i;

    if ( 1 )
    {
        memset(sps, 0, sizeof(sps_t));
        sps->chroma_format_idc = 1;
    }

    sps->profile_idc = bs_read_u8(b);
    sps->constraint_set0_flag = bs_read_u1(b);
    sps->constraint_set1_flag = bs_read_u1(b);
    sps->constraint_set2_flag = bs_read_u1(b);
    sps->constraint_set3_flag = bs_read_u1(b);
    sps->constraint_set4_flag = bs_read_u1(b);
    sps->constraint_set5_flag = bs_read_u1(b);
    /* reserved_zero_2bits */ bs_skip_u(b, 2);
    sps->level_idc = bs_read_u8(b);
    sps->seq_parameter_set_id = bs_read_ue(b);

    if ( sps->profile_idc == 100 || sps->profile_idc == 110 ||
        sps->profile_idc == 122 || sps->profile_idc == 244 ||
        sps->profile_idc == 44 || sps->profile_idc == 83 ||
        sps->profile_idc == 86 || sps->profile_idc == 118 ||
        sps->profile_idc == 128 || sps->profile_idc == 138 ||
        sps->profile_idc == 139 || sps->profile_idc == 134
       )
    {
        sps->chroma_format_idc = bs_read_ue(b);
        if ( sps->chroma_format_idc == 3 )
        {
            sps->residual_colour_transform_flag = bs_read_u1(b);
        }
        sps->bit_depth_luma_minus8 = bs_read_ue(b);
        sps->bit_depth_chroma_minus8 = bs_read_ue(b);
        sps->qpprime_y_zero_transform_bypass_flag = bs_read_u1(b);
        sps->seq_scaling_matrix_present_flag = bs_read_u1(b);
        if ( sps->seq_scaling_matrix_present_flag )
        {
            for ( i = 0; i < ((sps->chroma_format_idc != 3) ? 8 : 12); i++ )
            {
                sps->seq_scaling_list_present_flag[ i ] = bs_read_u1(b);
                if ( sps->seq_scaling_list_present_flag[ i ] )
                {
                    if ( i < 6 )
                    {
                        read_scaling_list( b, sps->ScalingList4x4[ i ], 16,
                                                 &( sps->UseDefaultScalingMatrix4x4Flag[ i ] ) );
                    }
                    else
                    {
                        read_scaling_list( b, sps->ScalingList8x8[ i - 6 ], 64,
                                                 &( sps->UseDefaultScalingMatrix8x8Flag[ i - 6 ] ) );
                    }
                }
            }
        }
    }
    sps->log2_max_frame_num_minus4 = bs_read_ue(b);
    sps->pic_order_cnt_type = bs_read_ue(b);
    if ( sps->pic_order_cnt_type == 0 )
    {
        sps->log2_max_pic_order_cnt_lsb_minus4 = bs_read_ue(b);
    }
    else if ( sps->pic_order_cnt_type == 1 )
    {
        sps->delta_pic_order_always_zero_flag = bs_read_u1(b);
        sps->offset_for_non_ref_pic = bs_read_se(b);
        sps->offset_for_top_to_bottom_field = bs_read_se(b);
        sps->num_ref_frames_in_pic_order_cnt_cycle = bs_read_ue(b);
        for ( i = 0; i < sps->num_ref_frames_in_pic_order_cnt_cycle; i++ )
        {
            sps->offset_for_ref_frame[ i ] = bs_read_se(b);
        }
    }
    sps->num_ref_frames = bs_read_ue(b);
    sps->gaps_in_frame_num_value_allowed_flag = bs_read_u1(b);
    sps->pic_width_in_mbs_minus1 = bs_read_ue(b);
    sps->pic_height_in_map_units_minus1 = bs_read_ue(b);
    sps->frame_mbs_only_flag = bs_read_u1(b);
    if ( !sps->frame_mbs_only_flag )
    {
        sps->mb_adaptive_frame_field_flag = bs_read_u1(b);
    }
    sps->direct_8x8_inference_flag = bs_read_u1(b);
    sps->frame_cropping_flag = bs_read_u1(b);
    if ( sps->frame_cropping_flag )
    {
        sps->frame_crop_left_offset = bs_read_ue(b);
        sps->frame_crop_right_offset = bs_read_ue(b);
        sps->frame_crop_top_offset = bs_read_ue(b);
        sps->frame_crop_bottom_offset = bs_read_ue(b);
    }
    sps->vui_parameters_present_flag = bs_read_u1(b);
    if ( sps->vui_parameters_present_flag )
    {
        read_vui_parameters(sps, b);
    }
}

void read_debug_scaling_list(bs_t* b, int* scalingList, int sizeOfScalingList, int* useDefaultScalingMatrixFlag )
{
    // NOTE need to be able to set useDefaultScalingMatrixFlag when reading, hence passing as pointer
    int lastScale = 8;
    int nextScale = 8;
    int delta_scale;
    for ( int j = 0; j < sizeOfScalingList; j++ )
    {
        if ( nextScale != 0 )
        {
            if ( 0 )
            {
                nextScale = scalingList[ j ];
                if (useDefaultScalingMatrixFlag[0]) { nextScale = 0; }
                delta_scale = (nextScale - lastScale) % 256 ;
            }

            delta_scale = bs_read_se(b);

            if ( 1 )
            {
                nextScale = ( lastScale + delta_scale + 256 ) % 256;
                useDefaultScalingMatrixFlag[0] = ( j == 0 && nextScale == 0 );
            }
        }
        if ( 1 )
        {
            scalingList[ j ] = ( nextScale == 0 ) ? lastScale : nextScale;
        }
        lastScale = scalingList[ j ];
    }
}


//7.3.2.2 Picture parameter set RBSP syntax
void read_pic_parameter_set_rbsp(pps_t* pps, bs_t* b)
{
   // pps_t* pps = h->pps;
    if ( 1 )
    {
        memset(pps, 0, sizeof(pps_t));
    }

    pps->pic_parameter_set_id = bs_read_ue(b);
    pps->seq_parameter_set_id = bs_read_ue(b);
    pps->entropy_coding_mode_flag = bs_read_u1(b);
    pps->pic_order_present_flag = bs_read_u1(b);
    pps->num_slice_groups_minus1 = bs_read_ue(b);

    if ( pps->num_slice_groups_minus1 > 0 )
    {
        pps->slice_group_map_type = bs_read_ue(b);
       // printf("%ld.%d: ", (long int)(b->p - b->start), b->bits_left); pps->slice_group_map_type = bs_read_ue(b); printf("pps->slice_group_map_type: %d \n", pps->slice_group_map_type);
        if ( pps->slice_group_map_type == 0 )
        {
            for ( int i_group = 0; i_group <= pps->num_slice_groups_minus1; i_group++ )
            {
                pps->run_length_minus1[ i_group ] = bs_read_ue(b);
                //printf("%ld.%d: ", (long int)(b->p - b->start), b->bits_left); pps->run_length_minus1[ i_group ] = bs_read_ue(b); printf("pps->run_length_minus1[ i_group ]: %d \n", pps->run_length_minus1[ i_group ]);
            }
        }
        else if ( pps->slice_group_map_type == 2 )
        {
            for ( int i_group = 0; i_group < pps->num_slice_groups_minus1; i_group++ )
            {
                pps->top_left[ i_group ] = bs_read_ue(b);
                pps->bottom_right[ i_group ] = bs_read_ue(b);
                //printf("%ld.%d: ", (long int)(b->p - b->start), b->bits_left); pps->top_left[ i_group ] = bs_read_ue(b); printf("pps->top_left[ i_group ]: %d \n", pps->top_left[ i_group ]);
                //printf("%ld.%d: ", (long int)(b->p - b->start), b->bits_left); pps->bottom_right[ i_group ] = bs_read_ue(b); printf("pps->bottom_right[ i_group ]: %d \n", pps->bottom_right[ i_group ]);
            }
        }
        else if ( pps->slice_group_map_type == 3 ||
                 pps->slice_group_map_type == 4 ||
                 pps->slice_group_map_type == 5 )
        {
            pps->slice_group_change_direction_flag = bs_read_u1(b);
            pps->slice_group_change_rate_minus1 = bs_read_ue(b);
            //printf("%ld.%d: ", (long int)(b->p - b->start), b->bits_left); pps->slice_group_change_direction_flag = bs_read_u1(b); printf("pps->slice_group_change_direction_flag: %d \n", pps->slice_group_change_direction_flag);
            //printf("%ld.%d: ", (long int)(b->p - b->start), b->bits_left); pps->slice_group_change_rate_minus1 = bs_read_ue(b); printf("pps->slice_group_change_rate_minus1: %d \n", pps->slice_group_change_rate_minus1);
        }
        else if ( pps->slice_group_map_type == 6 )
        {
            pps->pic_size_in_map_units_minus1 = bs_read_ue(b);
            //printf("%ld.%d: ", (long int)(b->p - b->start), b->bits_left); pps->pic_size_in_map_units_minus1 = bs_read_ue(b); printf("pps->pic_size_in_map_units_minus1: %d \n", pps->pic_size_in_map_units_minus1);
            for ( int i = 0; i <= pps->pic_size_in_map_units_minus1; i++ )
            {
                int v = intlog2( pps->num_slice_groups_minus1 + 1 );
                pps->slice_group_id[ i ] = bs_read_u(b, v);
                //int v = intlog2( pps->num_slice_groups_minus1 + 1 );
                //printf("%ld.%d: ", (long int)(b->p - b->start), b->bits_left); pps->slice_group_id[ i ] = bs_read_u(b, v); printf("pps->slice_group_id[ i ]: %d \n", pps->slice_group_id[ i ]);
            }
        }
    }

    pps->num_ref_idx_l0_active_minus1 = bs_read_ue(b);
    pps->num_ref_idx_l1_active_minus1 = bs_read_ue(b);
    pps->weighted_pred_flag = bs_read_u1(b);
    pps->weighted_bipred_idc = bs_read_u(b, 2);
    pps->pic_init_qp_minus26 = bs_read_se(b);
    pps->pic_init_qs_minus26 = bs_read_se(b);
    pps->chroma_qp_index_offset = bs_read_se(b);
    pps->deblocking_filter_control_present_flag = bs_read_u1(b);
    pps->constrained_intra_pred_flag = bs_read_u1(b);
    pps->redundant_pic_cnt_present_flag = bs_read_u1(b);

    int have_more_data = 0;
    if ( 1 ) { have_more_data = more_rbsp_data(b); }
    if ( 0 )
    {
        have_more_data = pps->transform_8x8_mode_flag | pps->pic_scaling_matrix_present_flag | pps->second_chroma_qp_index_offset != 0;
    }

    if ( have_more_data )
    {
        pps->transform_8x8_mode_flag = bs_read_u1(b);
        pps->pic_scaling_matrix_present_flag = bs_read_u1(b);
        if ( pps->pic_scaling_matrix_present_flag )
        {
            for ( int i = 0; i < 6 + 2* pps->transform_8x8_mode_flag; i++ )
            {
                pps->pic_scaling_list_present_flag[ i ] = bs_read_u1(b);
                //printf("%ld.%d: ", (long int)(b->p - b->start), b->bits_left); pps->pic_scaling_list_present_flag[ i ] = bs_read_u1(b); printf("pps->pic_scaling_list_present_flag[ i ]: %d \n", pps->pic_scaling_list_present_flag[ i ]);
                if ( pps->pic_scaling_list_present_flag[ i ] )
                {
                    if ( i < 6 )
                    {
                        read_debug_scaling_list( b, pps->ScalingList4x4[ i ], 16,
                                                 &( pps->UseDefaultScalingMatrix4x4Flag[ i ] ) );
                    }
                    else
                    {
                        read_debug_scaling_list( b, pps->ScalingList8x8[ i - 6 ], 64,
                                                 &( pps->UseDefaultScalingMatrix8x8Flag[ i - 6 ] ) );
                    }
                }
            }
        }
        pps->second_chroma_qp_index_offset = bs_read_se(b);
    }
}


//7.3.2.2 Picture parameter set RBSP syntax
void write_pic_parameter_set_rbsp(pps_t* pps, bs_t* b)
{
    //pps_t* pps = h->pps;
    if ( 0 )
    {
        memset(pps, 0, sizeof(pps_t));
    }

    bs_write_ue(b, pps->pic_parameter_set_id);
    bs_write_ue(b, pps->seq_parameter_set_id);
    bs_write_u1(b, pps->entropy_coding_mode_flag);
    bs_write_u1(b, pps->pic_order_present_flag);
    bs_write_ue(b, pps->num_slice_groups_minus1);

    if ( pps->num_slice_groups_minus1 > 0 )
    {
        bs_write_ue(b, pps->slice_group_map_type);
        if ( pps->slice_group_map_type == 0 )
        {
            for ( int i_group = 0; i_group <= pps->num_slice_groups_minus1; i_group++ )
            {
                bs_write_ue(b, pps->run_length_minus1[ i_group ]);
            }
        }
        else if ( pps->slice_group_map_type == 2 )
        {
            for ( int i_group = 0; i_group < pps->num_slice_groups_minus1; i_group++ )
            {
                bs_write_ue(b, pps->top_left[ i_group ]);
                bs_write_ue(b, pps->bottom_right[ i_group ]);
            }
        }
        else if ( pps->slice_group_map_type == 3 ||
                 pps->slice_group_map_type == 4 ||
                 pps->slice_group_map_type == 5 )
        {
            bs_write_u1(b, pps->slice_group_change_direction_flag);
            bs_write_ue(b, pps->slice_group_change_rate_minus1);
        }
        else if ( pps->slice_group_map_type == 6 )
        {
            bs_write_ue(b, pps->pic_size_in_map_units_minus1);
            for ( int i = 0; i <= pps->pic_size_in_map_units_minus1; i++ )
            {
                int v = intlog2( pps->num_slice_groups_minus1 + 1 );
                bs_write_u(b, v, pps->slice_group_id[ i ]);
            }
        }
    }
    bs_write_ue(b, pps->num_ref_idx_l0_active_minus1);
    bs_write_ue(b, pps->num_ref_idx_l1_active_minus1);
    bs_write_u1(b, pps->weighted_pred_flag);
    bs_write_u(b, 2, pps->weighted_bipred_idc);
    bs_write_se(b, pps->pic_init_qp_minus26);
    bs_write_se(b, pps->pic_init_qs_minus26);
    bs_write_se(b, pps->chroma_qp_index_offset);
    bs_write_u1(b, pps->deblocking_filter_control_present_flag);
    bs_write_u1(b, pps->constrained_intra_pred_flag);
    bs_write_u1(b, pps->redundant_pic_cnt_present_flag);

    int have_more_data = 0;
    if ( 0 ) { have_more_data = more_rbsp_data(b); }
    if ( 1 )
    {
        have_more_data = pps->transform_8x8_mode_flag | pps->pic_scaling_matrix_present_flag | pps->second_chroma_qp_index_offset != 0;
    }

    if ( have_more_data )
    {
        bs_write_u1(b, pps->transform_8x8_mode_flag);
        bs_write_u1(b, pps->pic_scaling_matrix_present_flag);
        if ( pps->pic_scaling_matrix_present_flag )
        {
            for ( int i = 0; i < 6 + 2* pps->transform_8x8_mode_flag; i++ )
            {
                bs_write_u1(b, pps->pic_scaling_list_present_flag[ i ]);
                if ( pps->pic_scaling_list_present_flag[ i ] )
                {
                    if ( i < 6 )
                    {
                        write_scaling_list( b, pps->ScalingList4x4[ i ], 16,
                                                 &( pps->UseDefaultScalingMatrix4x4Flag[ i ] ) );
                    }
                    else
                    {
                        write_scaling_list( b, pps->ScalingList8x8[ i - 6 ], 64,
                                                 &( pps->UseDefaultScalingMatrix8x8Flag[ i - 6 ] ) );
                    }
                }
            }
        }
        bs_write_se(b, pps->second_chroma_qp_index_offset);
    }
}



//Appendix E.1.1 VUI parameters syntax
void write_vui_parameters(sps_t* sps, bs_t* b)
{
    bs_write_u1(b, sps->vui.aspect_ratio_info_present_flag);
    if ( sps->vui.aspect_ratio_info_present_flag )
    {
        bs_write_u8(b, sps->vui.aspect_ratio_idc);
        if ( sps->vui.aspect_ratio_idc == SAR_Extended )
        {
            bs_write_u(b, 16, sps->vui.sar_width);
            bs_write_u(b, 16, sps->vui.sar_height);
        }
    }
    bs_write_u1(b, sps->vui.overscan_info_present_flag);
    if ( sps->vui.overscan_info_present_flag )
    {
        bs_write_u1(b, sps->vui.overscan_appropriate_flag);
    }
    bs_write_u1(b, sps->vui.video_signal_type_present_flag);
    if ( sps->vui.video_signal_type_present_flag )
    {
        bs_write_u(b, 3, sps->vui.video_format);
        bs_write_u1(b, sps->vui.video_full_range_flag);
        bs_write_u1(b, sps->vui.colour_description_present_flag);
        if ( sps->vui.colour_description_present_flag )
        {
            bs_write_u8(b, sps->vui.colour_primaries);
            bs_write_u8(b, sps->vui.transfer_characteristics);
            bs_write_u8(b, sps->vui.matrix_coefficients);
        }
    }
    bs_write_u1(b, sps->vui.chroma_loc_info_present_flag);
    if ( sps->vui.chroma_loc_info_present_flag )
    {
        bs_write_ue(b, sps->vui.chroma_sample_loc_type_top_field);
        bs_write_ue(b, sps->vui.chroma_sample_loc_type_bottom_field);
    }
    bs_write_u1(b, sps->vui.timing_info_present_flag);
    if ( sps->vui.timing_info_present_flag )
    {
        bs_write_u(b, 32, sps->vui.num_units_in_tick);
        bs_write_u(b, 32, sps->vui.time_scale);
        bs_write_u1(b, sps->vui.fixed_frame_rate_flag);
    }
    bs_write_u1(b, sps->vui.nal_hrd_parameters_present_flag);
    if ( sps->vui.nal_hrd_parameters_present_flag )
    {
        write_hrd_parameters(&sps->hrd_nal, b);
    }
    bs_write_u1(b, sps->vui.vcl_hrd_parameters_present_flag);
    if ( sps->vui.vcl_hrd_parameters_present_flag )
    {
        write_hrd_parameters(&sps->hrd_vcl, b);
    }
    if ( sps->vui.nal_hrd_parameters_present_flag || sps->vui.vcl_hrd_parameters_present_flag )
    {
        bs_write_u1(b, sps->vui.low_delay_hrd_flag);
    }
    bs_write_u1(b, sps->vui.pic_struct_present_flag);
    bs_write_u1(b, sps->vui.bitstream_restriction_flag);
    if ( sps->vui.bitstream_restriction_flag )
    {
        bs_write_u1(b, sps->vui.motion_vectors_over_pic_boundaries_flag);
        bs_write_ue(b, sps->vui.max_bytes_per_pic_denom);
        bs_write_ue(b, sps->vui.max_bits_per_mb_denom);
        bs_write_ue(b, sps->vui.log2_max_mv_length_horizontal);
        bs_write_ue(b, sps->vui.log2_max_mv_length_vertical);
        bs_write_ue(b, sps->vui.num_reorder_frames);
        bs_write_ue(b, sps->vui.max_dec_frame_buffering);
    }
}


//7.3.2.1 Sequence parameter set RBSP syntax
void write_seq_parameter_set_rbsp(sps_t* sps, bs_t* b)
{
    int i;

    if ( 0 )
    {
        memset(sps, 0, sizeof(sps_t));
        sps->chroma_format_idc = 1;
    }

    bs_write_u8(b, sps->profile_idc);
    bs_write_u1(b, sps->constraint_set0_flag);
    bs_write_u1(b, sps->constraint_set1_flag);
    bs_write_u1(b, sps->constraint_set2_flag);
    bs_write_u1(b, sps->constraint_set3_flag);
    bs_write_u1(b, sps->constraint_set4_flag);
    bs_write_u1(b, sps->constraint_set5_flag);
    /* reserved_zero_2bits */ bs_write_u(b, 2, 0);
    bs_write_u8(b, sps->level_idc);
    bs_write_ue(b, sps->seq_parameter_set_id);

    if ( sps->profile_idc == 100 || sps->profile_idc == 110 ||
        sps->profile_idc == 122 || sps->profile_idc == 244 ||
        sps->profile_idc == 44 || sps->profile_idc == 83 ||
        sps->profile_idc == 86 || sps->profile_idc == 118 ||
        sps->profile_idc == 128 || sps->profile_idc == 138 ||
        sps->profile_idc == 139 || sps->profile_idc == 134
       )
    {
        bs_write_ue(b, sps->chroma_format_idc);
        if ( sps->chroma_format_idc == 3 )
        {
            bs_write_u1(b, sps->residual_colour_transform_flag);
        }
        bs_write_ue(b, sps->bit_depth_luma_minus8);
        bs_write_ue(b, sps->bit_depth_chroma_minus8);
        bs_write_u1(b, sps->qpprime_y_zero_transform_bypass_flag);
        bs_write_u1(b, sps->seq_scaling_matrix_present_flag);
        if ( sps->seq_scaling_matrix_present_flag )
        {
            for ( i = 0; i < 8; i++ )
            {
                bs_write_u1(b, sps->seq_scaling_list_present_flag[ i ]);
                if ( sps->seq_scaling_list_present_flag[ i ] )
                {
                    if ( i < 6 )
                    {
                        write_scaling_list( b, sps->ScalingList4x4[ i ], 16,
                                                 &( sps->UseDefaultScalingMatrix4x4Flag[ i ] ) );
                    }
                    else
                    {
                        write_scaling_list( b, sps->ScalingList8x8[ i - 6 ], 64,
                                                 &( sps->UseDefaultScalingMatrix8x8Flag[ i - 6 ] ) );
                    }
                }
            }
        }
    }
    bs_write_ue(b, sps->log2_max_frame_num_minus4);
    bs_write_ue(b, sps->pic_order_cnt_type);
    if ( sps->pic_order_cnt_type == 0 )
    {
        bs_write_ue(b, sps->log2_max_pic_order_cnt_lsb_minus4);
    }
    else if ( sps->pic_order_cnt_type == 1 )
    {
        bs_write_u1(b, sps->delta_pic_order_always_zero_flag);
        bs_write_se(b, sps->offset_for_non_ref_pic);
        bs_write_se(b, sps->offset_for_top_to_bottom_field);
        bs_write_ue(b, sps->num_ref_frames_in_pic_order_cnt_cycle);
        for ( i = 0; i < sps->num_ref_frames_in_pic_order_cnt_cycle; i++ )
        {
            bs_write_se(b, sps->offset_for_ref_frame[ i ]);
        }
    }
    bs_write_ue(b, sps->num_ref_frames);
    bs_write_u1(b, sps->gaps_in_frame_num_value_allowed_flag);
    bs_write_ue(b, sps->pic_width_in_mbs_minus1);
    bs_write_ue(b, sps->pic_height_in_map_units_minus1);
    bs_write_u1(b, sps->frame_mbs_only_flag);
    if ( !sps->frame_mbs_only_flag )
    {
        bs_write_u1(b, sps->mb_adaptive_frame_field_flag);
    }
    bs_write_u1(b, sps->direct_8x8_inference_flag);
    bs_write_u1(b, sps->frame_cropping_flag);
    if ( sps->frame_cropping_flag )
    {
        bs_write_ue(b, sps->frame_crop_left_offset);
        bs_write_ue(b, sps->frame_crop_right_offset);
        bs_write_ue(b, sps->frame_crop_top_offset);
        bs_write_ue(b, sps->frame_crop_bottom_offset);
    }
    bs_write_u1(b, sps->vui_parameters_present_flag);
    if ( sps->vui_parameters_present_flag )
    {
        write_vui_parameters(sps, b);
    }
}
