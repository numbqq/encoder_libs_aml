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
--------------------------------------------------------------------------------*/

#ifndef TEST_DATA_DEFINE_H
#define TEST_DATA_DEFINE_H

#include "vsi_queue.h"
#include "base_type.h"
#include "osal.h"
#ifndef TEST_DATA
#define COMMENT(b, ...)
#else
#define COMMENT(b, ...)                                                                            \
    if ((b)->stream_trace) {                                                                       \
        char buffer[128];                                                                          \
        snprintf(buffer, sizeof(buffer), ##__VA_ARGS__);                                           \
        ASSERT(strlen((b)->stream_trace->comment) + strlen(buffer) <                               \
               sizeof((b)->stream_trace->comment));                                                \
        strcat((b)->stream_trace->comment, buffer);                                                \
    }
#endif

#endif
