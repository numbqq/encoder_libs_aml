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
--  Abstract : Hantro H1 JPEG Encoder API
--
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------

    Table of contents

    1. Include headers
    2. Module defines
    3. Data types
    4. Function prototypes

------------------------------------------------------------------------------*/

#ifndef __MJPEGENCAPI_H__
#define __MJPEGENCAPI_H__

#ifdef __cplusplus
extern "C" {
#endif

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/

#include "base_type.h"

/*------------------------------------------------------------------------------
    2. Module defines
------------------------------------------------------------------------------*/
typedef struct {
    u32 id;
    u32 flags;  //fixed to 0x10 to indicate key frame
    u32 offset; //offset to movi
    u32 length; //length of chunk
} IDX_CHUNK;

/*------------------------------------------------------------------------------
    4. Function prototypes
------------------------------------------------------------------------------*/
void MjpegAVIchunkheader(u8 **buf, char *type, char *name, u32 length);
u32 MjpegEncodeAVIHeader(u32 *outbuf, u32 width, u32 height, u32 frameRateNum, u32 frameRateDenom,
                         u32 frameNum);
u32 MjpegEncodeAVIidx(u32 *outbuf, IDX_CHUNK *idx, u32 movi_idx, u32 frameNum);

#ifdef __cplusplus
}
#endif

#endif
