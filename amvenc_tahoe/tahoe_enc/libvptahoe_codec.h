/*
* Copyright (c) 2019 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in below
* which is part of this source code package.
*
* Description:
*/

// Copyright (C) 2019 Amlogic, Inc. All rights reserved.
//
// All information contained herein is Amlogic confidential.
//
// This software is provided to you pursuant to Software License
// Agreement (SLA) with Amlogic Inc ("Amlogic"). This software may be
// used only in accordance with the terms of this agreement.
//
// Redistribution and use in source and binary forms, with or without
// modification is strictly prohibited without prior written permission
// from Amlogic.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#ifndef _VP_CODEC_TAHOE_H_
#define _VP_CODEC_TAHOE_H_

#include "vp_codec_tahoe.h"
#include <linux/videodev2.h>

void encoder_open(char *fwfile);
void encoder_close();
void encoder_unmap(int inst_id);
int vl_tahoe_encoder_init(int codec_id, handle_t encode_info);
int vl_tahoe_encoder_destroy(handle_t encode_info);
int vl_video_encoder_change_bitrate(handle_t *encode_info, int bitRate);
int vl_video_encoder_change_gop(handle_t *encode_info, int IntraQP, int IntraPeriod);
int vl_video_encoder_change_strict_rc(handle_t *encode_info, int bitrate_window, int skip_threshold);
int vl_video_encoder_change_qp(handle_t *encode_info,
                                int minQpI, int maxQpI, int maxDeltaQp,
                                int minQpP, int maxQpP,
                                int minQpB, int maxQpB);
int vl_tahoe_encoder_encode(handle_t *encode_info,struct v4l2_buffer *src_buf);
int get_dsframe(handle_t *handle, unsigned char *buffer, unsigned int *buf_nal_size);
int get_bsbframe(handle_t *handle, unsigned char *buffer, unsigned int *buf_nal_size, signed char *frame_type);
int get_header(handle_t *handle, unsigned char *pHeader, unsigned int *pLength);
int check_initparam(handle_t handle);
int vl_tahoe_encoder_generate_header(handle_t *encode_info);
bool get_input_buf_forv4l(ulong *pInput,struct v4l2_buffer *src_buf, handle_t *encode_info);
void detectV4l2Support(handle_t *encode_info);
int vl_v4l2_switch_to_dmamode( handle_t encode_info,int plane_num) ;

#endif
