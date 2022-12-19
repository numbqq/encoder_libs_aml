/*------------------------------------------------------------------------------
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                                                                            --
--                            Verisilicon.                                    --
--                                                                            --
--                   (C) COPYRIGHT 2020 VERISILICON                           --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
------------------------------------------------------------------------------*/

#ifndef __VCMDSTRUCT_H__
#define __VCMDSTRUCT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include "base_type.h"
#include "vsi_queue.h"

/* Maximum number of chunks allowed. */

/********variables declaration related with race condition**********/
#define ASIC_VCMD_SWREG_AMOUNT 27
#define VCMD_REGISTER_CONTROL_OFFSET 0X40
#define VCMD_REGISTER_INT_STATUS_OFFSET 0X44
#define VCMD_REGISTER_INT_CTL_OFFSET 0X48

#define MAX_SAME_MODULE_TYPE_CORE_NUMBER 4

#define CMDBUF_MAX_SIZE (512 * 4 * 4)

#define CMDBUF_POOL_TOTAL_SIZE                                                                     \
    (2 * 1024 * 1024) //approximately=128x(320x240)=128x2k=128x8kbyte=1Mbytes
#define TOTAL_DISCRETE_CMDBUF_NUM (CMDBUF_POOL_TOTAL_SIZE / CMDBUF_MAX_SIZE)
#define MAX_CMDBUF_INT_NUMBER 1
#define INT_MIN_SUM_OF_IMAGE_SIZE                                                                  \
    (4096 * 2160 * MAX_SAME_MODULE_TYPE_CORE_NUMBER * MAX_CMDBUF_INT_NUMBER)
#define CMDBUF_VCMD_REGISTER_TOTAL_SIZE 9 * 1024 * 1024 - CMDBUF_POOL_TOTAL_SIZE * 2
#define VCMD_REGISTER_SIZE (128 * 4)

#define FAILED_ADDRESS (void *)-1

//#define VCMD_ACCESS_TEST

#ifdef VCMD_ACCESS_TEST
#define MASK_KEY 0xc0000000
#define MASK_LOCK 0xc0000000

#else
#define MASK_KEY 0x00000000
#define MASK_LOCK 0x00000000

#endif

/*module_type support*/
enum vcmd_module_type {
    VCMD_TYPE_ENCODER = 0,
    VCMD_TYPE_CUTREE,
    VCMD_TYPE_DECODER,
    VCMD_TYPE_JPEG_ENCODER,
    VCMD_TYPE_JPEG_DECODER,
    MAX_VCMD_TYPE
};

#define MAX_VCMD_NUMBER (MAX_VCMD_TYPE * MAX_SAME_MODULE_TYPE_CORE_NUMBER)

/** SUBSUYRUN_TYPE is used to judgment the type of running in subsys */
typedef enum {
    VCMD_SUBSYSRUN_TYPE_VCE = 0x0001,         /**< VC8000E */
    VCMD_SUBSYSRUN_TYPE_CUTREE = 0x0002,      /**< CUTREE */
    VCMD_SUBSYSRUN_TYPE_DEC400_VCE = 0x0004,  /**< DEC400_VCE */
    VCMD_SUBSYSRUN_TYPE_L2CACHE_VCE = 0x0008, /**< L2CACHE_VCE */
    VCMD_SUBSYSRUN_TYPE_MMU_VCE = 0x0010,     /**< MMU_VCE */
    VCMD_SUBSYSRUN_TYPE_MMU_CUTREE = 0x0020,  /**< MMU_CUTREE */
    VCMD_SUBSYSRUN_TYPE_VCD = 0x0100,         /**< VC8000D */
    VCMD_SUBSYSRUN_TYPE_DEC400_VCD = 0x0200,  /**< DEC400_VCD */
    VCMD_SUBSYSRUN_TYPE_L2CACHE_VCD = 0x0400, /**< L2CACHE_VCD */
    VCMD_SUBSYSRUN_TYPE_MMU_VCD = 0x0800      /**< MMU_VCD */
} SUBSYSRUN_TYPE;

/* HW Register field names */
typedef enum {
#include "vcmdregisterenum.h"
    VcmdRegisterAmount
} regVcmdName;

/* HW Register field descriptions */
typedef struct {
    u32 name;          /* Register name and index  */
    i32 base;          /* Register base address  */
    u32 mask;          /* Bitmask for this field */
    i32 lsb;           /* LSB for this field [31..0] */
    i32 trace;         /* Enable/disable writing in swreg_params.trc */
    i32 rw;            /* 1=Read-only 2=Write-only 3=Read-Write */
    char *description; /* Field description */
} regVcmdField_s;

/* Flags for read-only, write-only and read-write */
#define RO 1
#define WO 2
#define RW 3

#define VCMD_REGBASE(reg) (asicVcmdRegisterDesc[reg].base)

/* Description field only needed for system model build. */
#ifdef TEST_DATA
#define VCMDREG(name, base, mask, lsb, trace, rw, desc)                                            \
    {                                                                                              \
        name, base, mask, lsb, trace, rw, desc                                                     \
    }
#else
#define VCMDREG(name, base, mask, lsb, trace, rw, desc)                                            \
    {                                                                                              \
        name, base, mask, lsb, trace, rw, ""                                                       \
    }
#endif

#ifdef __cplusplus
}
#endif

#endif //__VCMDSTRUCT_H__
