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
--------------------------------------------------------------------------------*/

#include "tools.h"
#include "ewl.h"
#include "vsi_string.h"

#ifdef SYSTEMSHARED
#define EWLmalloc malloc
#define EWLcalloc calloc
#define EWLfree free
#endif

struct memory {
    struct node *next;
    void *p;
};

/*------------------------------------------------------------------------------
  log2i
------------------------------------------------------------------------------*/
i32 log2i(i32 x, i32 *result)
{
    i32 tmp = 0;

    if (x < 0)
        return NOK;

    while (x >> ++tmp)
        ;

    *result = tmp - 1;

    return (x == 1 << (tmp - 1)) ? OK : NOK;
}

/*------------------------------------------------------------------------------
  checkRange
------------------------------------------------------------------------------*/
i32 checkRange(i32 x, i32 min, i32 max)
{
    if ((x >= min) && (x <= max))
        return OK;

    return NOK;
}

/*------------------------------------------------------------------------------
  qalloc
------------------------------------------------------------------------------*/
void *qalloc(struct queue *q, i32 nmemb, i32 size)
{
    struct memory *m;
    char *p;

    if (nmemb == 0)
        return NULL;

    m = EWLmalloc(sizeof(struct memory));
    p = EWLcalloc(nmemb, size);
    if (!m || !p) {
        EWLfree(m);
        EWLfree(p);
        return NULL;
    }

    m->p = p;
    queue_put(q, (struct node *)m);

    return p;
}

/*------------------------------------------------------------------------------
  qfree
------------------------------------------------------------------------------*/
void qfree(struct queue *q)
{
    struct node *n;
    struct memory *m;

    while ((n = queue_get(q))) {
        m = (struct memory *)n;
        if (m->p) {
            EWLfree(m->p);
            m->p = NULL;
        }
        EWLfree(m);
        m = NULL;
    }
}

/*------------------------------------------------------------------------------
  malloc_array
  For example array size [5][5]:
  char **c = (char **)malloc_array(5, 5, sizeof(char));
  struct foo **f = (struct foo **)malloc_array(5, 5, sizeof(struct foo));
------------------------------------------------------------------------------*/
void **malloc_array(struct queue *q, i32 r, i32 c, i32 size)
{
    char **p;
    i32 i;

    ASSERT(sizeof(void *) == sizeof(char *));

    if (!(p = qalloc(q, r, sizeof(char *))))
        return NULL;
    ;
    for (i = 0; i < r; i++) {
        if (!(p[i] = qalloc(q, c, size)))
            return NULL;
    }
    return (void **)p;
}

/*------------------------------------------------------------------------------
  get_value. get signed value of number bits from stream
------------------------------------------------------------------------------*/
i32 get_value(bits_buffer_s *b, i32 number, bool bSigned)
{
    u32 nBits = number;
    u32 value = 0;
    u32 out_bit_cnt = 0;
    u32 tmp;

    ASSERT((number <= 32) && (number > 0));

    /* Fill byte from stream to cache */
    while (b->bit_cnt < (u32)number) {
        /* if cache is going to be full */
        if (b->bit_cnt > 24) {
            tmp = b->cache & 0xFF;
            value |= (tmp << out_bit_cnt);
            out_bit_cnt += 8;
            b->bit_cnt -= 8;
            number -= 8;
            b->cache >>= 8;
        }
        b->cache |= (*b->stream++) << b->bit_cnt;
        b->bit_cnt += 8;
    }

    tmp = b->cache & ((1 << number) - 1);
    value |= (tmp << out_bit_cnt);
    b->bit_cnt -= number;
    b->cache >>= number;

    /* check signed flag and add signed flag */
    if (bSigned) {
        bool signedFlag = value >> (nBits - 1);
        value |= (signedFlag) ? (((1 << (32 - nBits)) - 1)) << nBits : 0;
    }
    b->accu_bits += nBits;
    return (i32)value;
}

/*------------------------------------------------------------------------------
  get_align. get bytes alignment for the stream
------------------------------------------------------------------------------*/
void get_align(bits_buffer_s *b, u32 bytes)
{
    u32 bitsEnd = bytes * 8;
    i32 tail = b->accu_bits & 7;

    if (tail)
        get_value(b, 8 - tail, HANTRO_FALSE);

    while (b->accu_bits < bitsEnd) {
        get_value(b, 8, HANTRO_FALSE);
    }
}

char *nextIntToken(char *str, i16 *ret)
{
    char *p = str;
    i32 tmp;

    if (!p)
        return NULL;

    while ((*p < '0') || (*p > '9')) {
        if (*p == '\0')
            return NULL;
        p++;
    }
    sscanf(p, "%d", &tmp);
    if ((p != str) && (*(p - 1) == '-'))
        tmp = -tmp;

    while ((*p >= '0') && (*p <= '9'))
        p++;

    *ret = tmp;
    return p;
}

i32 getGMVRange(i16 *maxX, i16 *maxY, i32 rangeCfg, i32 isH264, i32 isBSlice)
{
#if 0
  i32 rangeX = isBSlice ? 64 : 128;
  i32 rangeY = isH264 ? 16 : 32;
  i32 paddingX = 64;
  i32 paddingY = 32;

  if (!maxX || !maxY) return -1;

  if (rangeCfg)
    rangeY = (rangeCfg << 3) - 8;

  *maxX = rangeX + paddingX;
  *maxY = rangeY + paddingY;
#else
    *maxX = 128;
    *maxY = 128;
#endif
    return 0;
}

i32 tile_log2(i32 blk_size, i32 target)
{
    i32 k;
    for (k = 0; (blk_size << k) < target; k++) {
    }
    return k;
}

int clamp(int value, int low, int high)
{
    return value < low ? low : (value > high ? high : value);
}

int64_t clamp64(int64_t value, int64_t low, int64_t high)
{
    return value < low ? low : (value > high ? high : value);
}

double fclamp(double value, double low, double high)
{
    return value < low ? low : (value > high ? high : value);
}

void FreeBuffer(void **buf)
{
    if (*buf) {
        EWLfree(*buf);
        *buf = NULL;
    }
    return;
}
