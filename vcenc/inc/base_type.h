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

#ifndef BASE_TYPE_H
#define BASE_TYPE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdio.h>
#ifndef NDEBUG
#include <assert.h>
#endif
#include <stdbool.h>

#ifndef SYSTEMSHARED
#define VSIAPI(x) VSIAPI##x /* redefine API with prefix "VSIAPI" */
#else
#define VSIAPI(x) VSISYS##x /* redefine API with prefix "VSISYS" */
#endif

/**
 * \defgroup base_type Numeric Data Type
 *
 * \brief Numeric data type definition.
 *
 * @{
 */

typedef int8_t i8;    /**< \brief signed 8-bit integer */
typedef uint8_t u8;   /**< \brief unsigned 8-bit integer */
typedef int16_t i16;  /**< \brief signed 16-bit integer */
typedef uint16_t u16; /**< \brief unsigned 16-bit integer */
typedef int32_t i32;  /**< \brief signed 32-bit integer */
typedef uint32_t u32; /**< \brief unsigned 32-bit integer */
typedef int64_t i64;  /**< \brief signed 64-bit integer */
typedef uint64_t u64; /**< \brief unsigned 64-bit integer */

/**@}*/

#ifdef __FREERTOS__

#ifndef FREERTOS_SIMULATOR
typedef unsigned long long ptr_t; //Now the FreeRTOS Simulator just support the 64bit env
#else
typedef size_t ptr_t;
#endif

#else

typedef size_t ptr_t;
#endif

#ifndef INLINE
#define INLINE inline
#endif

#ifndef NULL
#ifdef __cplusplus
#define NULL 0
#else /*  */
#define NULL ((void *)0)
#endif /*  */
#endif

typedef short Short;
typedef int Int;
typedef unsigned int UInt;

#ifndef __cplusplus

#define HANTRO_FALSE 0
#define HANTRO_TRUE 1

enum { OK = 0, NOK = -1 };
#endif

/* ASSERT */
#ifndef NDEBUG
#define ASSERT(x) assert(x)
#else
#define ASSERT(x)
#endif

#ifdef __cplusplus
}
#endif

#endif
