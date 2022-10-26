/*------------------------------------------------------------------------------
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                                                                            --
--                            Verisilicon.                                    --
--                                                                            --
--                   (C) COPYRIGHT 2017 VERISILICON                           --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
--------------------------------------------------------------------------------*/

#ifndef CRC_H
#define CRC_H

#include "base_type.h"

/* Polynomial for CRC32 */
#define QUOTIENT 0x04c11db7

typedef struct {
    unsigned int crctab[256];
    unsigned int crc;
} crc32_ctx;

/* rename functions when access in system library. */
#define crc32_init VSIAPI(crc32_init)
#define crc32 VSIAPI(crc32)
#define crc32_final VSIAPI(crc32_final)

#ifdef CHECKSUM_CRC_BUILD_SUPPORT
void crc32_init(crc32_ctx *ctx, unsigned int init_crc);
unsigned int crc32(crc32_ctx *ctx, unsigned char *data, int len);
unsigned int crc32_final(crc32_ctx *ctx);
#else
#ifndef SYSTEMSHARED
#define VSIAPIcrc32_init(ctx, init_crc)
#define VSIAPIcrc32(ctx, data, len) (0)
#define VSIAPIcrc32_final(ctx) (0)
#else
#define VSISYScrc32_init(ctx, init_crc)
#define VSISYScrc32(ctx, data, len) (0)
#define VSISYScrc32_final(ctx) (0)
#endif
#endif

#endif
