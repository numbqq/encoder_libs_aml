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

#include "checksum.h"

/*------------------------------------------------------------------------------
  checksum_init
------------------------------------------------------------------------------*/
void checksum_init(checksum_ctx *ctx, unsigned int chksum, int offset)
{
    ctx->offset = offset;
    ctx->chksum = chksum;
}

/*------------------------------------------------------------------------------
  checksum
------------------------------------------------------------------------------*/
unsigned int checksum(checksum_ctx *ctx, unsigned char *data, int len)
{
    unsigned int result = ctx->chksum;
    int i = 0;

    while ((ctx->offset & 3) && i < len) {
#if 1
        result += (data[i]) << (ctx->offset * 8);
#else
        result += (data[i]) << ((3 - ctx->offset) * 8);
#endif
        ctx->offset++;
        i++;
        ctx->offset &= 0x3;
    }
    for (; i + 4 <= len; i += 4) {
#if 1
        result += (data[i] | (data[i + 1] << 8) | (data[i + 2] << 16) | (data[i + 3] << 24));
#else
        result += ((data[i] << 24) | (data[i + 1] << 16) | (data[i + 2] << 8) | data[i + 3]);
#endif
    }
    while (i < len) {
#if 1
        result += (data[i]) << (ctx->offset * 8);
#else
        result += (data[i]) << ((3 - ctx->offset) * 8);
#endif
        ctx->offset++;
        i++;
        ctx->offset &= 0x3;
    }
    ctx->chksum = result;

    return result;
}

/*------------------------------------------------------------------------------
  checksum_final
------------------------------------------------------------------------------*/
unsigned int checksum_final(checksum_ctx *ctx)
{
    return ctx->chksum;
}

#endif