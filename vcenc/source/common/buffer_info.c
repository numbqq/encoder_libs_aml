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

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/
#include "buffer_info.h"
#include "encasiccontroller.h"

void EncGetCompressTableSize(u32 compressor, u32 width, u32 height, u32 *lumaCompressTblSize,
                             u32 *chromaCompressTblSize)
{
    u32 tblLumaSize = 0;
    u32 tblChromaSize = 0;
    u32 tblSize = 0;

    if (compressor & 1) {
        tblLumaSize = ((width + 63) / 64) * ((height + 63) / 64) * 8; //ctu_num * 8
        tblLumaSize = ((tblLumaSize + 15) >> 4) << 4;
        *lumaCompressTblSize = tblLumaSize;
    }
    if (compressor & 2) {
        int cbs_w = ((width >> 1) + 7) / 8;
        int cbs_h = ((height >> 1) + 3) / 4;
        int cbsg_w = (cbs_w + 15) / 16;
        tblChromaSize = cbsg_w * cbs_h * 16;
        *chromaCompressTblSize = tblChromaSize;
    }
}

u32 EncGetSizeTblSize(u32 height, u32 tileEnable, u32 numTileRows, u32 codingType,
                      u32 maxTemporalLayers, u32 alignment)
{
    u32 maxSliceNals;
    u32 sizeTblSize = 0;
    maxSliceNals = (height + 15) / 16;
    if (codingType == ASIC_H264 && maxTemporalLayers > 1) //allocCfg->maxTemporalLayers > 1
        maxSliceNals *= 2;
    maxSliceNals = tileEnable == 1 ? numTileRows : maxSliceNals; //enable  several buffer
    sizeTblSize =
        STRIDE((((sizeof(u32) * (maxSliceNals + 1) + 7) & (~7)) + (sizeof(u32) * 10)), alignment);
    return sizeTblSize;
}
