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

#ifndef TEST_BENCH_STATISTIC_H
#define TEST_BENCH_STATISTIC_H

#include "base_type.h"

/* store the bits consumed by each GOP */
void StatisticFrameBits(VCEncInst inst, struct test_bench *tb, ma_s *ma, u32 frameBits,
                        u64 totalBits);

/* calculate bits deviation from the stored GOP bits */
float GetBitsDeviation(struct test_bench *tb);

/* print the distortion & bits data per frame, only valid for c-model. */
void EncTraceProfile(VCEncInst inst, struct test_bench *tb, VCEncOut *pEncOut, u64 bits, u8 force);

#endif
