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

#ifndef CHECKSUM_H
#define CHECKSUM_H

#include "base_type.h"

typedef struct {
    int offset;
    unsigned int chksum;
} checksum_ctx;

/* rename functions with prefix. */
#define checksum_init VSIAPI(checksum_init)
#define checksum VSIAPI(checksum)
#define checksum_final VSIAPI(checksum_final)

#ifdef CHECKSUM_CRC_BUILD_SUPPORT
void checksum_init(checksum_ctx *ctx, unsigned int chksum, int offset);
unsigned int checksum(checksum_ctx *ctx, unsigned char *data, int len);
unsigned int checksum_final(checksum_ctx *ctx);
#else
#ifndef SYSTEMSHARED
#define VSIAPIchecksum_init(ctx, chksum, offset)
#define VSIAPIchecksum(ctx, data, len) (0)
#define VSIAPIchecksum_final(ctx) (0)
#else
#define VSISYSchecksum_init(ctx, chksum, offset)
#define VSISYSchecksum(ctx, data, len) (0)
#define VSISYSchecksum_final(ctx) (0)
#endif
#endif

#endif
