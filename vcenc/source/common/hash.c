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

#include "hash.h"

/*------------------------------------------------------------------------------
  hash_init
------------------------------------------------------------------------------*/
void hash_init(hashctx *ctx, unsigned int hash_type)
{
    ctx->hash_type = hash_type;
    if (ctx->hash_type == 1) {
        crc32_init(&ctx->_ctx.crc32_ctx, (unsigned)-1);
    } else if (ctx->hash_type == 2) {
        checksum_init(&ctx->_ctx.checksum_ctx, 0, 0);
    }
}

/*------------------------------------------------------------------------------
  hash
------------------------------------------------------------------------------*/
unsigned int hash(hashctx *ctx, unsigned char *data, int len)
{
    if (ctx->hash_type == 1) {
        return crc32(&ctx->_ctx.crc32_ctx, data, len);
    } else if (ctx->hash_type == 2) {
        return checksum(&ctx->_ctx.checksum_ctx, data, len);
    }
    return 0;
}

/*------------------------------------------------------------------------------
  hash_finalize
------------------------------------------------------------------------------*/
unsigned int hash_finalize(hashctx *ctx)
{
    if (ctx->hash_type == 1) {
        return crc32_final(&ctx->_ctx.crc32_ctx);
    } else if (ctx->hash_type == 2) {
        return checksum_final(&ctx->_ctx.checksum_ctx);
    }
    return 0;
}
/*------------------------------------------------------------------------------
  hash_reset
------------------------------------------------------------------------------*/
void hash_reset(hashctx *ctx, int init_hash, int offset)
{
    if (ctx->hash_type == 1) {
        crc32_init(&ctx->_ctx.crc32_ctx, init_hash);
    } else if (ctx->hash_type == 2) {
        checksum_init(&ctx->_ctx.checksum_ctx, init_hash, offset);
    }
}
/*------------------------------------------------------------------------------
  hash_getstate
------------------------------------------------------------------------------*/
void hash_getstate(hashctx *ctx, unsigned int *hash, int *offset)
{
    if (ctx->hash_type == 1) {
        *hash = ctx->_ctx.crc32_ctx.crc;
        *offset = 0;
    } else if (ctx->hash_type == 2) {
        *hash = ctx->_ctx.checksum_ctx.chksum;
        *offset = ctx->_ctx.checksum_ctx.offset;
    } else {
        *hash = *offset = 0;
    }
}
