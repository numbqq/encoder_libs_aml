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
--------------------------------------------------------------------------------
--
--  Description :  Encoder SW/HW interface register definitions
--
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------

    Table of contents

    1. Include headers
    2. External compiler flags
    3. Module defines

------------------------------------------------------------------------------*/
#ifndef ENC_SWHWREGISTERS_H
#define ENC_SWHWREGISTERS_H

#ifdef __cplusplus
extern "C" {
#endif
/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/

#include "base_type.h"

/*------------------------------------------------------------------------------
    2. External compiler flags
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/

#define ASIC_SWREG_AMOUNT 512

#define ASIC_ID_BLUEBERRY 0x48321220
#define ASIC_ID_CLOUDBERRY 0x48321480
#define ASIC_ID_DRAGONFLY 0x48322170
#define ASIC_ID_EVERGREEN 0x48323590
#define ASIC_ID_EVERGREEN_PLUS 0x48324470
#define ASIC_ID_FOXTAIL 0x48324780

#define ASIC_INPUT_YUV420PLANAR 0x00
#define ASIC_INPUT_YUV420SEMIPLANAR 0x01
#define ASIC_INPUT_YUYV422INTERLEAVED 0x02
#define ASIC_INPUT_UYVY422INTERLEAVED 0x03
#define ASIC_INPUT_RGB565 0x04
#define ASIC_INPUT_RGB555 0x05
#define ASIC_INPUT_RGB444 0x06
#define ASIC_INPUT_RGB888 0x07
#define ASIC_INPUT_RGB101010 0x08
#define ASIC_INPUT_I010                                                                            \
    0x09 //yuv420 10 bit, in 16 bit pixel,  low 10bits valid, high 6 bits invalid.
#define ASIC_INPUT_P010                                                                            \
    0x0a //yuv420 10 bit, in 16 bit pixel,  high 10bits valid, low 6 bits invalid.
#define ASIC_INPUT_PACKED_10BIT_PLANAR 0x0b       //yuv420 10 bit, packed planar.
#define ASIC_INPUT_PACKED_10BIT_Y0L2 0x0c         //yuv420 10 bit, packed Y0L2.
#define ASIC_INPUT_YUV420_TILE32 0x0d             //yuv420 8 bit, tile 32_32 format.
#define ASIC_INPUT_YUV420_TILE16_PCK4 0x0e        //yuv420 8 bit, tile 16_16 packed 4 format.
#define ASIC_INPUT_YUV420_TILE4 0x0f              //yuv420 8 bit, 4x4 tile format.
#define ASIC_INPUT_P010_TILE4 0x10                //yuv420 10bit, 4x4 tile format.
#define ASIC_INPUT_SP_101010 0x11                 //yuv420 10bit, semi-planar
#define ASIC_INPUT_YUV422_888 0x12                //yuv422 888
#define ASIC_INPUT_YUV420_TILE64 0x13             //yuv420uv tile 64_4
#define ASIC_INPUT_YUV420PCK16_TILE 0x14          //yuv420 packed16 tile 32_4
#define ASIC_INPUT_YUV420PCK10_TILE 0x15          //yuv420 packed10 tile 48_4
#define ASIC_INPUT_YUV420_TILE128 0x16            //yuv420uv tile 128_2
#define ASIC_INPUT_YUV420PCK10_TILE96 0x17        //yuv420 packed10 tile 96_2
#define ASIC_INPUT_YUV420SP_TILE8 0x18            //yuv420 packed8 tile 8_8
#define ASIC_INPUT_P010_TILE8 0x19                //yuv420 packed10 tile 8_8
#define ASIC_INPUT_YUV420_10BIT_UV_TILE128x2 0x1a //yuv420 semiplanar 10 bit(NV12), 128x2 tile
#define ASIC_INPUT_YUV420_UV_TILE64x2 0x20        //yuv420 semiplanar 8 bit(NV12), 64x2 tile
#define ASIC_INPUT_RGB888_24BIT 0x21              //RGB888 24bpp

/* Bytes of external memory for VP9 counters for probability updates,
 * 252 counters for dct coeff probs, 1 for skipped, 1 for intra type and
 * 2 * 11 for mv probs, each counter 2 bytes */
#define ASIC_VP9_PROB_COUNT_SIZE 244 * 2
#define ASIC_VP9_PROB_COUNT_MODE_OFFSET 220
#define ASIC_VP9_PROB_COUNT_MV_OFFSET 222
#define ASIC_REG_INDEX_STATUS (20 / 4)

#define MAX_SLICE_NUM 256

/* register flag bit field to indicate its attribute */
typedef enum {

    REGFLAGS_HEVC = (1 << 0),
    REGFLAGS_H264 = (1 << 1),
    REGFLAGS_JPEG = (1 << 2),
    REGFLAGS_AV1 = (1 << 3),
    REGFLAGS_VP9 = (1 << 4),
    REGFLAGS_PRP = (1 << 12),
    REGFLAGS_ADDR_LSB = (1 << 16),
    REGFLAGS_ADDR_MSB = (1 << 17),
    REGFLAGS_ADDR_IN = (1 << 18),
    REGFLAGS_ADDR_OUT = (1 << 19),

} regAttrib;

/* HW Register field names */
typedef enum {

#include "registerenum.h"

    HEncRegisterAmount

} regName;

/* HW Register field descriptions */
typedef struct {
    u32 name;  /* Register name and index  */
    i32 base;  /* Register base address  */
    u32 mask;  /* Bitmask for this field */
    i32 lsb;   /* LSB for this field [31..0] */
    i32 trace; /* Enable/disable writing in trace_Top_swreg_params.trc */
    i32 rw;    /* 1=Read-only 2=Write-only 3=Read-Write */
    u32 attrib; /* Attribute flags for this register, defined with or result of fields REGFLAGS_XXX */
    char *description; /* Field description */
} regField_s;

/* Flags for read-only, write-only and read-write */
#define RO 1
#define WO 2
#define RW 3

#define NONE 0
#define ENC_IN 1
#define ENC_OUT 2
#define ENC_INOUT 3

#define REGBASE(reg) (asicRegisterDesc[reg].base)

/* Description field only needed for system model build. */
#ifdef TEST_DATA
#define H2REG(name, base, mask, lsb, trace, rw, attrib, desc)                                      \
    {                                                                                              \
        name, base, mask, lsb, trace, rw, attrib, desc                                             \
    }
#else
#define H2REG(name, base, mask, lsb, trace, rw, attrib, desc)                                      \
    {                                                                                              \
        name, base, mask, lsb, trace, rw, attrib, ""                                               \
    }
#endif

/*------------------------------------------------------------------------------
    4. Function prototypes
------------------------------------------------------------------------------*/
extern const regField_s asicRegisterDesc[];

/*------------------------------------------------------------------------------

    EncAsicSetRegisterValue

    Set a value into a defined register field

------------------------------------------------------------------------------*/
static inline void EncAsicSetRegisterValue(u32 *regMirror, regName name, u32 value)
{
    const regField_s *field;
    u32 regVal;

    field = &asicRegisterDesc[name];

#ifdef DEBUG_PRINT_REGS
    printf("EncAsicSetRegister 0x%2x  0x%08x  Value: %10d  %s\n", field->base, field->mask, value,
           field->description);
#endif

    /* Check that value fits in field */
    ASSERT(field->name == name);
    ASSERT(((field->mask >> field->lsb) << field->lsb) == field->mask);
    ASSERT((field->mask >> field->lsb) >= value);
    ASSERT(field->base < ASIC_SWREG_AMOUNT * 4);

    /* Clear previous value of field in register */
    regVal = regMirror[field->base / 4] & ~(field->mask);

    /* Put new value of field in register */
    regMirror[field->base / 4] = regVal | ((value << field->lsb) & field->mask);
}

u32 EncAsicGetRegisterValue(const void *ewl, u32 *regMirror, regName name);
void EncAsicWriteRegisterValue(const void *ewl, u32 *regMirror, regName name, u32 value);

#define EncAsicSetAddrRegisterValue(reg_base, name, value)                                         \
    do {                                                                                           \
        if (sizeof(ptr_t) == 8) {                                                                  \
            EncAsicSetRegisterValue((reg_base), name, (u32)(value));                               \
            EncAsicSetRegisterValue((reg_base), name##_MSB, (u32)(((u64)value) >> 32));            \
        } else {                                                                                   \
            EncAsicSetRegisterValue((reg_base), name, (u32)(value));                               \
        }                                                                                          \
    } while (0)

#define EncAsicGetAddrRegisterValue(ewl, reg_base, name)                                           \
    ((sizeof(ptr_t) == 8)                                                                          \
         ? ((((ptr_t)EncAsicGetRegisterValue((ewl), (reg_base), name)) |                           \
             (((ptr_t)EncAsicGetRegisterValue((ewl), (reg_base), name##_MSB)) << 32)))             \
         : ((ptr_t)EncAsicGetRegisterValue((ewl), (reg_base), (name))))

#ifdef __cplusplus
}
#endif

#endif
