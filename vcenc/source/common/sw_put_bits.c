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

#include "sw_put_bits.h"

#ifdef SUPPORT_AV1
#include "av1_enums.h"
#endif

#ifdef TEST_DATA
#include "enctrace.h"
#endif

/*------------------------------------------------------------------------------
  put_bits write bits to stream. For example (value=2, number=4) write
  0010 to the stream.
------------------------------------------------------------------------------*/
void put_bit(struct buffer *b, i32 value, i32 number)
{
    i32 left;

    ASSERT((number <= 8) && (number > 0));
    ASSERT(!(value & (~0 << number)));

#ifdef TEST_DATA
    Enc_add_comment(b, value, number, NULL);
#endif

    if (buffer_full(b))
        return;

    b->bit_cnt += number;
    left = 32 - b->bit_cnt;
    if (left > 0) {
        b->cache |= value << left;
        return;
    }

    /* Flush next byte to stream */
    if (b->cache & 0xFFFFFC00) /* No emulation prevent byte */
    {
#ifdef TEST_DATA
        Enc_add_comment(b, b->cache >> 24, 8, "write to stream");
#endif
        *b->stream++ = b->cache >> 24;
        (*b->cnt)++;
        b->cache <<= 8;
        b->cache |= value << (left + 8);
        b->bit_cnt -= 8;
    } else {
        *b->stream++ = 0;
        *b->stream++ = 0;
        *b->stream++ = 0x03;
        b->emulCnt++;
#ifdef TEST_DATA
        Enc_add_comment(b, 0, 8, "write to stream");
        Enc_add_comment(b, 0, 8, "write to stream");
        Enc_add_comment(b, 3, 8, "write to stream (emulation prevent)");
#endif
        (*b->cnt) += 3;
        b->cache <<= 16;
        b->cache |= value << (left + 16);
        b->bit_cnt -= 16;
    }
}

void put_bit_av1(struct buffer *b, i32 value, i32 number)
{
    i32 left;

    ASSERT((number <= 8) && (number > 0));
    ASSERT(!(value & (~0 << number)));

    //#ifdef TEST_DATA
    //    Enc_add_comment(b, value, number, NULL);
    //#endif

    if (buffer_full(b))
        return;

    b->bit_cnt += number;
    left = 32 - b->bit_cnt;
    if (left > 0) {
        b->cache |= value << left;
        return;
    }

    /* Flush next byte to stream */
    *b->stream++ = b->cache >> 24;
    (*b->cnt)++;
    b->cache <<= 8;
    b->cache |= value << (left + 8);
    b->bit_cnt -= 8;
}
/*------------------------------------------------------------------------------
  buffer_full
------------------------------------------------------------------------------*/
i32 buffer_full(struct buffer *b)
{
    if (b->size < *b->cnt + 8)
        return HANTRO_TRUE;

    return HANTRO_FALSE;
}

/*------------------------------------------------------------------------------
  rbsp_trailing_bits adds rbsp_stop_one_bit and rbsp_alignment_zero_bit
  until next byte boundary and flush bits to stream.
------------------------------------------------------------------------------*/
void rbsp_trailing_bits(struct buffer *b)
{
    if (buffer_full(b))
        return;

    COMMENT(b, "rbsp_stop_one_bit");
    put_bit(b, 1, 1);
    while (b->bit_cnt % 8) {
        COMMENT(b, "rbsp_alignment_zero_bit");
        put_bit(b, 0, 1);
    }

    while (b->bit_cnt) {
        /* Flush next byte to stream */
        if ((b->bit_cnt >= 24) && ((b->cache & 0xFFFFFC00) == 0)) {
            *b->stream++ = 0;
            *b->stream++ = 0;
            *b->stream++ = 0x03;
            b->emulCnt++;
#ifdef TEST_DATA
            Enc_add_comment(b, 0, 8, "write to stream");
            Enc_add_comment(b, 0, 8, "write to stream");
            Enc_add_comment(b, 3, 8, "write to stream (emulation prevent)");
#endif
            (*b->cnt) += 3;
            b->cache <<= 16;
            b->bit_cnt -= 16;
        } else {
            /* No emulation prevent byte */
#ifdef TEST_DATA
            Enc_add_comment(b, b->cache >> 24, 8, "write to stream");
#endif
            *b->stream++ = b->cache >> 24;
            (*b->cnt)++;
            b->cache <<= 8;
            b->bit_cnt -= 8;
        }
    }
}

/*------------------------------------------------------------------------------
  put_bit_ue
------------------------------------------------------------------------------*/
void put_bit_ue(struct buffer *b, i32 val)
{
    i32 tmp = 0;

    ASSERT(val >= 0);
    COMMENT(b, " ue(%i)", val);

    val++;
    while (val >> ++tmp)
        ;

    put_bit_32(b, val, tmp * 2 - 1);
}

/*------------------------------------------------------------------------------
  put_bit_se
------------------------------------------------------------------------------*/
void put_bit_se(struct buffer *b, i32 val)
{
    i32 tmp = 0;

    COMMENT(b, " se(%i)", val);

    if (val > 0) {
        val = 2 * val;
    } else {
        val = -2 * val + 1;
    }

    while (val >> ++tmp)
        ;

    put_bit_32(b, val, tmp * 2 - 1);
}

/*------------------------------------------------------------------------------
  put_bit_32 flush "number" of bits from "value" because put_bit() can
  handle only 8 bits at time.
------------------------------------------------------------------------------*/
void put_bit_32(struct buffer *b, i32 value, i32 number)
{
    i32 tmp, bits_left = 24;

    ASSERT(number <= 32);
    while (number) {
        if (number > bits_left) {
            tmp = number - bits_left;
            put_bit(b, (value >> bits_left) & 0xFF, tmp);
            number -= tmp;
        }
        bits_left -= 8;
    }
}

void put_bit_av1_32(struct buffer *b, i32 value, i32 number)
{
    i32 tmp, bits_left = 24;

    ASSERT(number <= 32);
    while (number) {
        if (number > bits_left) {
            tmp = number - bits_left;
            put_bit_av1(b, (value >> bits_left) & 0xFF, tmp);
            number -= tmp;
        }
        bits_left -= 8;
    }
}

/*------------------------------------------------------------------------------
  put_bits write bits to stream. For example (value=2, number=4) write
  0010 to the stream

  only for slice header and start code
------------------------------------------------------------------------------*/
void put_bits_startcode(struct buffer *b)
{
    if (buffer_full(b))
        return;

    COMMENT(b, "BYTE STREAM: leadin_zero_8bits");
    *b->stream++ = 0;
    COMMENT(b, "BYTE STREAM: Start_code_prefix");
    *b->stream++ = 0;
    COMMENT(b, "BYTE STREAM: Start_code_prefix");
    *b->stream++ = 0;
    COMMENT(b, "BYTE STREAM: Start_code_prefix");
    *b->stream++ = 1;
    (*b->cnt) += 4;

    return;
}
/*------------------------------------------------------------------------------
  rbsp_trailing_bits adds rbsp_stop_one_bit and rbsp_alignment_zero_bit
  until next byte boundary and flush bits to stream.
------------------------------------------------------------------------------*/
void rbsp_flush_bits(struct buffer *b)
{
    while (b->bit_cnt >= 8) {
#ifdef TEST_DATA
        Enc_add_comment(b, b->cache >> 24, 8, "write to stream");
#endif
        *b->stream++ = b->cache >> 24;
        (*b->cnt)++;
        b->cache <<= 8;
        b->bit_cnt -= 8;
    }
}

//TODO: To be remove
void rbsp_flush_bits_av1(struct buffer *b)
{
    while (b->bit_cnt >= 8) {
        //#ifdef TEST_DATA
        //    Enc_add_comment(b, b->cache >> 24, 8, "write to stream");
        //#endif
        *b->stream++ = b->cache >> 24;
        (*b->cnt)++;
        b->cache <<= 8;
        b->bit_cnt -= 8;
    }
}

void add_trailing_bits_av1(struct buffer *b)
{
    if (buffer_full(b))
        return;

    //COMMENT(b, "rbsp_stop_one_bit");
    put_bit_av1(b, 1, 1);
    while (b->bit_cnt % 8) {
        //COMMENT(b, "rbsp_alignment_zero_bit");
        put_bit_av1(b, 0, 1);
    }

    while (b->bit_cnt) {
        *b->stream++ = b->cache >> 24;
        (*b->cnt)++;
        b->cache <<= 8;
        b->bit_cnt -= 8;
    }
}