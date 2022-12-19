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
--  Description : Preprocessor setup
--
------------------------------------------------------------------------------*/
#ifndef __BUFFER_INFO_H__
#define __BUFFER_INFO_H__

#ifdef __cplusplus
extern "C" {
#endif

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/
#include "base_type.h"

#ifdef SYSTEMSHARED
/* rename common functions for access in c-model library. */
#define EncGetCompressTableSize CoreEncGetCompressTableSize
#define EncGetSizeTblSize CoreEncGetSizeTblSize
#endif

/*------------------------------------------------------------------------------
    2. External compiler flags
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    4. Function prototypes
------------------------------------------------------------------------------*/
void EncGetCompressTableSize(u32 compressor, u32 width, u32 height, u32 *lumaCompressTblSize,
                             u32 *chromaCompressTblSize);
u32 EncGetSizeTblSize(u32 height, u32 tileEnable, u32 numTileRows, u32 codingType,
                      u32 maxTemporalLayers, u32 alignment);

#ifdef __cplusplus
}
#endif

#endif //__BUFFER_INFO_H__
