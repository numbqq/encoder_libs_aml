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
--  Description : AV1 VP9 common structure
--
------------------------------------------------------------------------------*/
#ifndef __AV1_VP9_COMMON_TYPE_H__
#define __AV1_VP9_COMMON_TYPE_H__

#include "hevcencapi.h"
#include "av1_vp9_common.h"

typedef struct _SW_RefCntBuffer {
    i32 poc;
    i32 ref_count;
    u32 frame_num;
    u32 ref_frame_num[SW_INTER_REFS_PER_FRAME];
    i32 showable_frame; // frame can be used as show existing frame in future
    VCEncPictureCodingType codingType;
    i8 temporalId; // temporal ID
} SW_RefCntBuffer;

#endif
