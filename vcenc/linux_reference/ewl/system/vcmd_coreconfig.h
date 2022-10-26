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

#ifndef __VCMD_CORECONFIG_H__
#define __VCMD_CORECONFIG_H__

//video encoder vcmd configuration

#define VCMD_ENC_IO_ADDR_0 0x90000 /*customer specify according to own platform*/
#define VCMD_ENC_IO_SIZE_0 (ASIC_VCMD_SWREG_AMOUNT * 4) /* bytes */
#define VCMD_ENC_INT_PIN_0 -1
#define VCMD_ENC_MODULE_TYPE_0 0
#define VCMD_ENC_MODULE_MAIN_ADDR_0 0x1000   /*customer specify according to own platform*/
#define VCMD_ENC_MODULE_DEC400_ADDR_0 0XFFFF //0X6000    /*0xffff means no such kind of submodule*/
#define VCMD_ENC_MODULE_L2CACHE_ADDR_0 0XFFFF
#define VCMD_ENC_MODULE_MMU0_ADDR_0 0XFFFF   //0X2000
#define VCMD_ENC_MODULE_MMU1_ADDR_0 0XFFFF   //0X4000
#define VCMD_ENC_MODULE_AXIFE0_ADDR_0 0XFFFF //0X3000
#define VCMD_ENC_MODULE_AXIFE1_ADDR_0 0XFFFF //0X5000

#define VCMD_ENC_IO_ADDR_1 0x91000 /*customer specify according to own platform*/
#define VCMD_ENC_IO_SIZE_1 (ASIC_VCMD_SWREG_AMOUNT * 4) /* bytes */
#define VCMD_ENC_INT_PIN_1 -1
#define VCMD_ENC_MODULE_TYPE_1 0
#define VCMD_ENC_MODULE_MAIN_ADDR_1 0x1000   /*customer specify according to own platform*/
#define VCMD_ENC_MODULE_DEC400_ADDR_1 0XFFFF /*0xffff means no such kind of submodule*/
#define VCMD_ENC_MODULE_L2CACHE_ADDR_1 0XFFFF
#define VCMD_ENC_MODULE_MMU0_ADDR_1 0XFFFF
#define VCMD_ENC_MODULE_MMU1_ADDR_1 0XFFFF
#define VCMD_ENC_MODULE_AXIFE0_ADDR_1 0XFFFF
#define VCMD_ENC_MODULE_AXIFE1_ADDR_1 0XFFFF

#define VCMD_ENC_IO_ADDR_2 0x92000 /*customer specify according to own platform*/
#define VCMD_ENC_IO_SIZE_2 (ASIC_VCMD_SWREG_AMOUNT * 4) /* bytes */
#define VCMD_ENC_INT_PIN_2 -1
#define VCMD_ENC_MODULE_TYPE_2 0
#define VCMD_ENC_MODULE_MAIN_ADDR_2 0x1000   /*customer specify according to own platform*/
#define VCMD_ENC_MODULE_DEC400_ADDR_2 0XFFFF /*0xffff means no such kind of submodule*/
#define VCMD_ENC_MODULE_L2CACHE_ADDR_2 0XFFFF
#define VCMD_ENC_MODULE_MMU0_ADDR_2 0XFFFF
#define VCMD_ENC_MODULE_MMU1_ADDR_2 0XFFFF
#define VCMD_ENC_MODULE_AXIFE0_ADDR_2 0XFFFF
#define VCMD_ENC_MODULE_AXIFE1_ADDR_2 0XFFFF

#define VCMD_ENC_IO_ADDR_3 0x93000 /*customer specify according to own platform*/
#define VCMD_ENC_IO_SIZE_3 (ASIC_VCMD_SWREG_AMOUNT * 4) /* bytes */
#define VCMD_ENC_INT_PIN_3 -1
#define VCMD_ENC_MODULE_TYPE_3 0
#define VCMD_ENC_MODULE_MAIN_ADDR_3 0x1000   /*customer specify according to own platform*/
#define VCMD_ENC_MODULE_DEC400_ADDR_3 0XFFFF /*0xffff means no such kind of submodule*/
#define VCMD_ENC_MODULE_L2CACHE_ADDR_3 0XFFFF
#define VCMD_ENC_MODULE_MMU0_ADDR_3 0XFFFF
#define VCMD_ENC_MODULE_MMU1_ADDR_3 0XFFFF
#define VCMD_ENC_MODULE_AXIFE0_ADDR_3 0XFFFF
#define VCMD_ENC_MODULE_AXIFE1_ADDR_3 0XFFFF

//video encoder cutree/IM  vcmd configuration

#define VCMD_IM_IO_ADDR_0 0x94000 //0xA0000    /*customer specify according to own platform*/
#define VCMD_IM_IO_SIZE_0 (ASIC_VCMD_SWREG_AMOUNT * 4) /* bytes */
#define VCMD_IM_INT_PIN_0 -1
#define VCMD_IM_MODULE_TYPE_0 1
#define VCMD_IM_MODULE_MAIN_ADDR_0 0x1000   /*customer specify according to own platform*/
#define VCMD_IM_MODULE_DEC400_ADDR_0 0XFFFF /*0xffff means no such kind of submodule*/
#define VCMD_IM_MODULE_L2CACHE_ADDR_0 0XFFFF
#define VCMD_IM_MODULE_MMU0_ADDR_0 0XFFFF //0X2000
#define VCMD_IM_MODULE_MMU1_ADDR_0 0XFFFF
#define VCMD_IM_MODULE_AXIFE0_ADDR_0 0XFFFF //0X3000
#define VCMD_IM_MODULE_AXIFE1_ADDR_0 0XFFFF // 0XFFFF

#define VCMD_IM_IO_ADDR_1 0xa1000 /*customer specify according to own platform*/
#define VCMD_IM_IO_SIZE_1 (ASIC_VCMD_SWREG_AMOUNT * 4) /* bytes */
#define VCMD_IM_INT_PIN_1 -1
#define VCMD_IM_MODULE_TYPE_1 1
#define VCMD_IM_MODULE_MAIN_ADDR_1 0x1000   /*customer specify according to own platform*/
#define VCMD_IM_MODULE_DEC400_ADDR_1 0XFFFF /*0xffff means no such kind of submodule*/
#define VCMD_IM_MODULE_L2CACHE_ADDR_1 0XFFFF
#define VCMD_IM_MODULE_MMU0_ADDR_1 0XFFFF
#define VCMD_IM_MODULE_MMU1_ADDR_1 0XFFFF
#define VCMD_IM_MODULE_AXIFE0_ADDR_1 0XFFFF
#define VCMD_IM_MODULE_AXIFE1_ADDR_1 0XFFFF

#define VCMD_IM_IO_ADDR_2 0xa2000 /*customer specify according to own platform*/
#define VCMD_IM_IO_SIZE_2 (ASIC_VCMD_SWREG_AMOUNT * 4) /* bytes */
#define VCMD_IM_INT_PIN_2 -1
#define VCMD_IM_MODULE_TYPE_2 1
#define VCMD_IM_MODULE_MAIN_ADDR_2 0x1000   /*customer specify according to own platform*/
#define VCMD_IM_MODULE_DEC400_ADDR_2 0XFFFF /*0xffff means no such kind of submodule*/
#define VCMD_IM_MODULE_L2CACHE_ADDR_2 0XFFFF
#define VCMD_IM_MODULE_MMU0_ADDR_2 0XFFFF
#define VCMD_IM_MODULE_MMU1_ADDR_2 0XFFFF
#define VCMD_IM_MODULE_AXIFE0_ADDR_2 0XFFFF
#define VCMD_IM_MODULE_AXIFE1_ADDR_2 0XFFFF

#define VCMD_IM_IO_ADDR_3 0xa3000 /*customer specify according to own platform*/
#define VCMD_IM_IO_SIZE_3 (ASIC_VCMD_SWREG_AMOUNT * 4) /* bytes */
#define VCMD_IM_INT_PIN_3 -1
#define VCMD_IM_MODULE_TYPE_3 1
#define VCMD_IM_MODULE_MAIN_ADDR_3 0x1000   /*customer specify according to own platform*/
#define VCMD_IM_MODULE_DEC400_ADDR_3 0XFFFF /*0xffff means no such kind of submodule*/
#define VCMD_IM_MODULE_L2CACHE_ADDR_3 0XFFFF
#define VCMD_IM_MODULE_MMU0_ADDR_3 0XFFFF
#define VCMD_IM_MODULE_MMU1_ADDR_3 0XFFFF
#define VCMD_IM_MODULE_AXIFE0_ADDR_3 0XFFFF
#define VCMD_IM_MODULE_AXIFE1_ADDR_3 0XFFFF

//JPEG encoder vcmd configuration

#define VCMD_JPEGE_IO_ADDR_0 0x90000 /*customer specify according to own platform*/
#define VCMD_JPEGE_IO_SIZE_0 (ASIC_VCMD_SWREG_AMOUNT * 4) /* bytes */
#define VCMD_JPEGE_INT_PIN_0 -1
#define VCMD_JPEGE_MODULE_TYPE_0 3
#define VCMD_JPEGE_MODULE_MAIN_ADDR_0 0x1000 /*customer specify according to own platform*/
#define VCMD_JPEGE_MODULE_DEC400_ADDR_0                                                            \
    0XFFFF //0X4000    /*0xffff means no such kind of submodule*/
#define VCMD_JPEGE_MODULE_L2CACHE_ADDR_0 0XFFFF
#define VCMD_JPEGE_MODULE_MMU0_ADDR_0 0XFFFF //0X2000
#define VCMD_JPEGE_MODULE_MMU1_ADDR_0 0XFFFF
#define VCMD_JPEGE_MODULE_AXIFE0_ADDR_0 0XFFFF //0X3000
#define VCMD_JPEGE_MODULE_AXIFE1_ADDR_0 0XFFFF

#define VCMD_JPEGE_IO_ADDR_1 0xC1000 /*customer specify according to own platform*/
#define VCMD_JPEGE_IO_SIZE_1 (ASIC_VCMD_SWREG_AMOUNT * 4) /* bytes */
#define VCMD_JPEGE_INT_PIN_1 -1
#define VCMD_JPEGE_MODULE_TYPE_1 3
#define VCMD_JPEGE_MODULE_MAIN_ADDR_1 0x1000   /*customer specify according to own platform*/
#define VCMD_JPEGE_MODULE_DEC400_ADDR_1 0XFFFF /*0xffff means no such kind of submodule*/
#define VCMD_JPEGE_MODULE_L2CACHE_ADDR_1 0XFFFF
#define VCMD_JPEGE_MODULE_MMU0_ADDR_1 0XFFFF
#define VCMD_JPEGE_MODULE_MMU1_ADDR_1 0XFFFF
#define VCMD_JPEGE_MODULE_AXIFE0_ADDR_1 0XFFFF
#define VCMD_JPEGE_MODULE_AXIFE1_ADDR_1 0XFFFF

#define VCMD_JPEGE_IO_ADDR_2 0xC2000 /*customer specify according to own platform*/
#define VCMD_JPEGE_IO_SIZE_2 (ASIC_VCMD_SWREG_AMOUNT * 4) /* bytes */
#define VCMD_JPEGE_INT_PIN_2 -1
#define VCMD_JPEGE_MODULE_TYPE_2 3
#define VCMD_JPEGE_MODULE_MAIN_ADDR_2 0x1000   /*customer specify according to own platform*/
#define VCMD_JPEGE_MODULE_DEC400_ADDR_2 0XFFFF /*0xffff means no such kind of submodule*/
#define VCMD_JPEGE_MODULE_L2CACHE_ADDR_2 0XFFFF
#define VCMD_JPEGE_MODULE_MMU0_ADDR_2 0XFFFF
#define VCMD_JPEGE_MODULE_MMU1_ADDR_2 0XFFFF
#define VCMD_JPEGE_MODULE_AXIFE0_ADDR_2 0XFFFF
#define VCMD_JPEGE_MODULE_AXIFE1_ADDR_2 0XFFFF

#define VCMD_JPEGE_IO_ADDR_3 0xC3000 /*customer specify according to own platform*/
#define VCMD_JPEGE_IO_SIZE_3 (ASIC_VCMD_SWREG_AMOUNT * 4) /* bytes */
#define VCMD_JPEGE_INT_PIN_3 -1
#define VCMD_JPEGE_MODULE_TYPE_3 3
#define VCMD_JPEGE_MODULE_MAIN_ADDR_3 0x1000   /*customer specify according to own platform*/
#define VCMD_JPEGE_MODULE_DEC400_ADDR_3 0XFFFF /*0xffff means no such kind of submodule*/
#define VCMD_JPEGE_MODULE_L2CACHE_ADDR_3 0XFFFF
#define VCMD_JPEGE_MODULE_MMU0_ADDR_3 0XFFFF
#define VCMD_JPEGE_MODULE_MMU1_ADDR_3 0XFFFF
#define VCMD_JPEGE_MODULE_AXIFE0_ADDR_3 0XFFFF
#define VCMD_JPEGE_MODULE_AXIFE1_ADDR_3 0XFFFF

//encoder configuration

{VCMD_ENC_IO_ADDR_0,
 VCMD_ENC_IO_SIZE_0,
 VCMD_ENC_INT_PIN_0,
 VCMD_ENC_MODULE_TYPE_0,
 VCMD_ENC_MODULE_MAIN_ADDR_0,
 VCMD_ENC_MODULE_DEC400_ADDR_0,
 VCMD_ENC_MODULE_L2CACHE_ADDR_0,
 {VCMD_ENC_MODULE_MMU0_ADDR_0, VCMD_ENC_MODULE_MMU1_ADDR_0},
 {VCMD_ENC_MODULE_AXIFE0_ADDR_0, VCMD_ENC_MODULE_AXIFE1_ADDR_0}},

    {VCMD_ENC_IO_ADDR_1,
     VCMD_ENC_IO_SIZE_1,
     VCMD_ENC_INT_PIN_1,
     VCMD_ENC_MODULE_TYPE_1,
     VCMD_ENC_MODULE_MAIN_ADDR_1,
     VCMD_ENC_MODULE_DEC400_ADDR_1,
     VCMD_ENC_MODULE_L2CACHE_ADDR_1,
     {VCMD_ENC_MODULE_MMU0_ADDR_1, VCMD_ENC_MODULE_MMU1_ADDR_1},
     {VCMD_ENC_MODULE_AXIFE0_ADDR_1, VCMD_ENC_MODULE_AXIFE1_ADDR_1}},

    {VCMD_ENC_IO_ADDR_2,
     VCMD_ENC_IO_SIZE_2,
     VCMD_ENC_INT_PIN_2,
     VCMD_ENC_MODULE_TYPE_2,
     VCMD_ENC_MODULE_MAIN_ADDR_2,
     VCMD_ENC_MODULE_DEC400_ADDR_2,
     VCMD_ENC_MODULE_L2CACHE_ADDR_2,
     {VCMD_ENC_MODULE_MMU0_ADDR_2, VCMD_ENC_MODULE_MMU1_ADDR_2},
     {VCMD_ENC_MODULE_AXIFE0_ADDR_2, VCMD_ENC_MODULE_AXIFE1_ADDR_2}},

    {VCMD_ENC_IO_ADDR_3,
     VCMD_ENC_IO_SIZE_3,
     VCMD_ENC_INT_PIN_3,
     VCMD_ENC_MODULE_TYPE_3,
     VCMD_ENC_MODULE_MAIN_ADDR_3,
     VCMD_ENC_MODULE_DEC400_ADDR_3,
     VCMD_ENC_MODULE_L2CACHE_ADDR_3,
     {VCMD_ENC_MODULE_MMU0_ADDR_3, VCMD_ENC_MODULE_MMU1_ADDR_3},
     {VCMD_ENC_MODULE_AXIFE0_ADDR_3, VCMD_ENC_MODULE_AXIFE1_ADDR_3}},

    //cutree/IM configuration

    {VCMD_IM_IO_ADDR_0,
     VCMD_IM_IO_SIZE_0,
     VCMD_IM_INT_PIN_0,
     VCMD_IM_MODULE_TYPE_0,
     VCMD_IM_MODULE_MAIN_ADDR_0,
     VCMD_IM_MODULE_DEC400_ADDR_0,
     VCMD_IM_MODULE_L2CACHE_ADDR_0,
     {VCMD_IM_MODULE_MMU0_ADDR_0, VCMD_IM_MODULE_MMU1_ADDR_0},
     {VCMD_IM_MODULE_AXIFE0_ADDR_0, VCMD_IM_MODULE_AXIFE1_ADDR_0}},

    {VCMD_IM_IO_ADDR_1,
     VCMD_IM_IO_SIZE_1,
     VCMD_IM_INT_PIN_1,
     VCMD_IM_MODULE_TYPE_1,
     VCMD_IM_MODULE_MAIN_ADDR_1,
     VCMD_IM_MODULE_DEC400_ADDR_1,
     VCMD_IM_MODULE_L2CACHE_ADDR_1,
     {VCMD_IM_MODULE_MMU0_ADDR_1, VCMD_IM_MODULE_MMU1_ADDR_1},
     {VCMD_IM_MODULE_AXIFE0_ADDR_1, VCMD_IM_MODULE_AXIFE1_ADDR_1}},

    {VCMD_IM_IO_ADDR_2,
     VCMD_IM_IO_SIZE_2,
     VCMD_IM_INT_PIN_2,
     VCMD_IM_MODULE_TYPE_2,
     VCMD_IM_MODULE_MAIN_ADDR_2,
     VCMD_IM_MODULE_DEC400_ADDR_2,
     VCMD_IM_MODULE_L2CACHE_ADDR_2,
     {VCMD_IM_MODULE_MMU0_ADDR_2, VCMD_IM_MODULE_MMU1_ADDR_2},
     {VCMD_IM_MODULE_AXIFE0_ADDR_2, VCMD_IM_MODULE_AXIFE1_ADDR_2}},

    {VCMD_IM_IO_ADDR_3,
     VCMD_IM_IO_SIZE_3,
     VCMD_IM_INT_PIN_3,
     VCMD_IM_MODULE_TYPE_3,
     VCMD_IM_MODULE_MAIN_ADDR_3,
     VCMD_IM_MODULE_DEC400_ADDR_3,
     VCMD_IM_MODULE_L2CACHE_ADDR_3,
     {VCMD_IM_MODULE_MMU0_ADDR_3, VCMD_IM_MODULE_MMU1_ADDR_3},
     {VCMD_IM_MODULE_AXIFE0_ADDR_3, VCMD_IM_MODULE_AXIFE1_ADDR_3}},

    //JPEG encoder configuration

    {VCMD_JPEGE_IO_ADDR_0,
     VCMD_JPEGE_IO_SIZE_0,
     VCMD_JPEGE_INT_PIN_0,
     VCMD_JPEGE_MODULE_TYPE_0,
     VCMD_JPEGE_MODULE_MAIN_ADDR_0,
     VCMD_JPEGE_MODULE_DEC400_ADDR_0,
     VCMD_JPEGE_MODULE_L2CACHE_ADDR_0,
     {VCMD_JPEGE_MODULE_MMU0_ADDR_0, VCMD_JPEGE_MODULE_MMU1_ADDR_0},
     {VCMD_JPEGE_MODULE_AXIFE0_ADDR_0, VCMD_JPEGE_MODULE_AXIFE1_ADDR_0}},

    {VCMD_JPEGE_IO_ADDR_1,
     VCMD_JPEGE_IO_SIZE_1,
     VCMD_JPEGE_INT_PIN_1,
     VCMD_JPEGE_MODULE_TYPE_1,
     VCMD_JPEGE_MODULE_MAIN_ADDR_1,
     VCMD_JPEGE_MODULE_DEC400_ADDR_1,
     VCMD_JPEGE_MODULE_L2CACHE_ADDR_1,
     {VCMD_JPEGE_MODULE_MMU0_ADDR_1, VCMD_JPEGE_MODULE_MMU1_ADDR_1},
     {VCMD_JPEGE_MODULE_AXIFE0_ADDR_1, VCMD_JPEGE_MODULE_AXIFE1_ADDR_1}},

    {VCMD_JPEGE_IO_ADDR_2,
     VCMD_JPEGE_IO_SIZE_2,
     VCMD_JPEGE_INT_PIN_2,
     VCMD_JPEGE_MODULE_TYPE_2,
     VCMD_JPEGE_MODULE_MAIN_ADDR_2,
     VCMD_JPEGE_MODULE_DEC400_ADDR_2,
     VCMD_JPEGE_MODULE_L2CACHE_ADDR_2,
     {VCMD_JPEGE_MODULE_MMU0_ADDR_2, VCMD_JPEGE_MODULE_MMU1_ADDR_2},
     {VCMD_JPEGE_MODULE_AXIFE0_ADDR_2, VCMD_JPEGE_MODULE_AXIFE1_ADDR_2}},

    {VCMD_JPEGE_IO_ADDR_3,
     VCMD_JPEGE_IO_SIZE_3,
     VCMD_JPEGE_INT_PIN_3,
     VCMD_JPEGE_MODULE_TYPE_3,
     VCMD_JPEGE_MODULE_MAIN_ADDR_3,
     VCMD_JPEGE_MODULE_DEC400_ADDR_3,
     VCMD_JPEGE_MODULE_L2CACHE_ADDR_3,
     {VCMD_JPEGE_MODULE_MMU0_ADDR_3, VCMD_JPEGE_MODULE_MMU1_ADDR_3},
     {VCMD_JPEGE_MODULE_AXIFE0_ADDR_3, VCMD_JPEGE_MODULE_AXIFE1_ADDR_3}},

#endif //__VCMD_CORECONFIG_H__
