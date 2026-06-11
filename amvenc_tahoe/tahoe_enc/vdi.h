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
#ifndef _VDI_H_
#define _VDI_H_

#include <stdbool.h>
#include "tahoe.h"

typedef enum
{
    NONE=0, INFO, DEBUG, WARN, ERR, TRACE, MAX_LOG_LEVEL
} debug_log_level_t;

extern debug_log_level_t g_tahoe_log_level;

#ifdef __ANDROID__
#include <log/log.h>
#undef LOG_NDEBUG
#define LOG_NDEBUG 0
#undef LOG_TAG
#define LOG_TAG "AMLTahoeEncCodec"

#define VLOG(level, x...) \
    do { \
        if (level >= g_tahoe_log_level) { \
            if (level == NONE) \
                ALOGV(x); \
            if (level == INFO) \
                ALOGI(x); \
            else if (level == DEBUG) \
                ALOGD(x); \
            else if (level == WARN) \
                ALOGW(x); \
            else if (level >= ERR) \
                ALOGE(x); \
        } \
    }while(0)
#else
#define VLOG(level, fmt , var...) \
    do { \
        if (level >= g_tahoe_log_level) { \
            printf("[%s:%d] " fmt "\n", __FUNCTION__, __LINE__, ##var);\
        } \
    }while(0)
#endif

enum
{
    INIT_VDI_STAT_NULL = 0,
    INIT_VDI_STAT_INIT = 1,
    INIT_VDI_STAT_OPENING = 2,
    INIT_VDI_STAT_ALLOC = 3,
    INIT_VDI_STAT_DONE = 4,
};

void tahoe_set_log_level(debug_log_level_t level);
int venc_lock(void);
void venc_unlock(void);
int venc_init(void);
void safe_memcpy(void *dst, const void *src, size_t n);
int vdi_debug(unsigned int data);
int vdi_read_memory(ulong addr, unsigned char *data, int len);
int vdi_allocate_dma_memory(venc_buffer_t *vb);
int venc_destroy(void);
int vdi_config_dma(tahoe_dma_buf_info_t *info);
int vdi_unmap_dma(tahoe_dma_buf_info_t *info);
void set_instance_valid(int id,bool bvalid);
bool find_valid_id(int *id);
int get_cur_instance();
uint8_t find_valid_chn_cnt();
int vdi_alloc_common_buffer(int chnid);

#endif /* _VDI_H_ */
