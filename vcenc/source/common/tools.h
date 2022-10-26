/*------------------------------------------------------------------------------
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                                                                            --
--                            Verisilicon.                                    --
--                                                                            --
--                   (C) COPYRIGHT 2014 VERISILICON                           --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
--------------------------------------------------------------------------------*/

#ifndef TOOLS_H
#define TOOLS_H

#include "base_type.h"
#include "vsi_queue.h"
#include <stdlib.h>

typedef struct {
    u8 *stream;
    u32 cache;
    u32 bit_cnt;
    u32 accu_bits;
} bits_buffer_s;

/* rename functions with prefix. */
#define log2i VSIAPI(log2i)
#define checkRange VSIAPI(checkRange)
#define malloc_array VSIAPI(malloc_array)
#define qalloc VSIAPI(qalloc)
#define qfree VSIAPI(qfree)
#define get_value VSIAPI(get_value)
#define get_align VSIAPI(get_align)
#define nextIntToken VSIAPI(nextIntToken)
#define getGMVRange VSIAPI(getGMVRange)
#define tile_log2 VSIAPI(tile_log2)
#define clamp VSIAPI(clamp)
#define clamp64 VSIAPI(clamp64)
#define fclamp VSIAPI(fclamp)
#define FreeBuffer VSIAPI(FreeBuffer)

#ifndef SYSTEMSHARED
extern void *EWLcalloc(u32 n, u32 s);
extern void *EWLmalloc(u32 n);
extern void EWLfree(void *p);
#endif

i32 log2i(i32 x, i32 *result);
i32 checkRange(i32 x, i32 min, i32 max);
void **malloc_array(struct queue *q, i32 r, i32 c, i32 size);
void *qalloc(struct queue *q, i32 nmemb, i32 size);
void qfree(struct queue *q);
i32 get_value(bits_buffer_s *b, i32 number, bool bSigned);
void get_align(bits_buffer_s *b, u32 bytes);
char *nextIntToken(char *str, i16 *ret);
i32 getGMVRange(i16 *maxX, i16 *maxY, i32 rangeCfg, i32 isH264, i32 isBSlice);
i32 tile_log2(i32 blk_size, i32 target);
int clamp(int value, int low, int high);
int64_t clamp64(int64_t value, int64_t low, int64_t high);
double fclamp(double value, double low, double high);
void FreeBuffer(void **buf);

#endif
