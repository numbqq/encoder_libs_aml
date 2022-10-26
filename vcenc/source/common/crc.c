/*------------------------------------------------------------------------------
--                                                                                                                               --
--       This software is confidential and proprietary and may be used                                   --
--        only as expressly authorized by a licensing agreement from                                     --
--                                                                                                                               --
--                            Verisilicon.                                                                                    --
--                                                                                                                               --
--                   (C) COPYRIGHT 2017 VERISILICON                                                            --
--                            ALL RIGHTS RESERVED                                                                    --
--                                                                                                                               --
--                 The entire notice above must be reproduced                                                 --
--                  on all copies and should not be removed.                                                    --
--                                                                                                                               --
--------------------------------------------------------------------------------*/
#ifdef CHECKSUM_CRC_BUILD_SUPPORT

#include "crc.h"

/*------------------------------------------------------------------------------
  crc32_init
------------------------------------------------------------------------------*/
void crc32_init(crc32_ctx *ctx, unsigned int init_crc)
{
    int i, j;

    unsigned int crc;

    for (i = 0; i < 256; i++) {
        crc = i << 24;
        for (j = 0; j < 8; j++) {
            if (crc & 0x80000000)
                crc = (crc << 1) ^ QUOTIENT;
            else
                crc = crc << 1;
        }
        ctx->crctab[i] = crc;
    }
    ctx->crc = init_crc;
}

/*------------------------------------------------------------------------------
  crc32
------------------------------------------------------------------------------*/
unsigned int crc32(crc32_ctx *ctx, unsigned char *data, int len)
{
    unsigned int result = ctx->crc;
    int i;

    for (i = 0; i < len; i++) {
        result = (result << 8) ^ ctx->crctab[(result >> 24) ^ *data++];
    }
    ctx->crc = result;

    return result;
}

/*------------------------------------------------------------------------------
  crc32_final
------------------------------------------------------------------------------*/
unsigned int crc32_final(crc32_ctx *ctx)
{
    /* add final bit inversion per dahua's specification */
    ctx->crc = ~ctx->crc;
    return ctx->crc;
}

#endif