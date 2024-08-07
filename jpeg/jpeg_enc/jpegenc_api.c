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
#define LOG_TAG "jenc_test"
#define LOG_LINE() printf("[%s:%d]\n", __FUNCTION__, __LINE__)
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/poll.h>
#include <stdint.h>

#include "jpegenc_api.h"
#include "jpegenc.h"

jpegenc_handle_t jpegenc_init() {
	jpegenc_handle_t handle = hw_encode_init(0);

	return handle;
}


jpegenc_result_e jpegenc_encode(jpegenc_handle_t handle, jpegenc_frame_info_t frame_info,	unsigned char *out, int *out_size) {
	int ret = -1;
	if (frame_info.iformat == 0)
		frame_info.iformat = FMT_YUV422_SINGLE;
	else if (frame_info.iformat == 1)
		frame_info.iformat = FMT_YUV444_SINGLE;
	else if (frame_info.iformat == 2)
		frame_info.iformat = FMT_NV21;
	else if (frame_info.iformat == 3)
		frame_info.iformat = FMT_NV12;
	else if (frame_info.iformat == 4)
		frame_info.iformat = FMT_YUV420;
	else if (frame_info.iformat == 5)
		frame_info.iformat = FMT_YUV444_PLANE;
	else if (frame_info.iformat == 6)
		frame_info.iformat = FMT_RGB888;

	if (frame_info.oformat == 0)
		frame_info.oformat = FMT_YUV420;     //4
	else if (frame_info.oformat == 1)
		frame_info.oformat = FMT_YUV422_SINGLE;     //0
	else if (frame_info.oformat == 2)
		frame_info.oformat = FMT_YUV444_SINGLE;     //1
	ret = hw_encode(handle, frame_info, out, out_size);
	if (ret != 0)
		return ENC_FAILED;
	return ENC_SUCCESS;
}

int jpegenc_destroy(jpegenc_handle_t handle) {
	hw_encode_uninit(handle);

	free((void *)handle);
	return 0;
}



