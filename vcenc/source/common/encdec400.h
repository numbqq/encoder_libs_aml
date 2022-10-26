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

#ifndef ENC_DEC400_H
#define ENC_DEC400_H

#include "base_type.h"
#include "osal.h"

#ifdef __cplusplus
extern "C" {
#endif

enum DEC400_REG {
    gcregAHBDECControl,
    gcregAHBDECControlEx,
    gcregAHBDECReadExConfig0,
    gcregAHBDECReadExConfig1,
    gcregAHBDECReadExConfig2,
    gcregAHBDECReadExConfig3,

    gcregAHBDECReadConfig0,
    gcregAHBDECReadConfig1,
    gcregAHBDECReadConfig2,
    gcregAHBDECReadConfig3,

    gcregAHBDECReadBufferBase0,
    gcregAHBDECReadBufferBase1,
    gcregAHBDECReadBufferBase2,
    gcregAHBDECReadBufferBase3,

    gcregAHBDECReadBufferBase0Ex,
    gcregAHBDECReadBufferBase1Ex,
    gcregAHBDECReadBufferBase2Ex,
    gcregAHBDECReadBufferBase3Ex,

    gcregAHBDECReadBufferEnd0,
    gcregAHBDECReadBufferEnd1,
    gcregAHBDECReadBufferEnd2,
    gcregAHBDECReadBufferEnd3,

    gcregAHBDECReadBufferEnd0Ex,
    gcregAHBDECReadBufferEnd1Ex,
    gcregAHBDECReadBufferEnd2Ex,
    gcregAHBDECReadBufferEnd3Ex,

    gcregAHBDECReadCacheBase0,
    gcregAHBDECReadCacheBase1,
    gcregAHBDECReadCacheBase2,
    gcregAHBDECReadCacheBase3,

    gcregAHBDECReadCacheBase0Ex,
    gcregAHBDECReadCacheBase1Ex,
    gcregAHBDECReadCacheBase2Ex,
    gcregAHBDECReadCacheBase3Ex,

    gcregAHBDECIntrEnblEx2,
    gcregAHBDECIntrAcknowledgeEx2,

    gcregAHBDECWriteConfig0,
    gcregAHBDECWriteConfig1,
    gcregAHBDECWriteConfig2,
    gcregAHBDECWriteConfig3,

    DEC400_REG_MAX
};

enum DEC400_TILE_MODE {
    RASTER256X1,
    RASTER128X1,
    RASTER64X1,
    TILE64X4,
    TILE32X4,
    TILE16X4,
    TILE8x4,
    TILE4x8
};
/*
 * DEC400 Customer id
 */
#define NETINT_DEC400 0x00000525
#define THBM1_DEC400 0x00000518
#define THBM3_DEC400 0x00000520
#define OYBM1_DEC400 0x00000528
#define OYBM3_DEC400 0x00000529
#define GOKE_DEC400 0x00000538
#define FALCON_DEC400 0x00000534
#define XILINX_DEC400 0x00000546
#define THEAD_DEC400 0x00000550
#define XILINX2_DEC400 0x00000555
#define INTELLIF_DEC400 0x00000557

typedef struct {
    u32 format;
    u32 lumWidthSrc;
    u32 lumHeightSrc;
    u32 input_alignment;
    ptr_t dec400LumTableBase;
    ptr_t dec400CbTableBase;
    ptr_t dec400CrTableBase;
    u32 dec400Enable;
    u32 *regVcmdBuf;
    u32 *regVcmdBufSize;
    ptr_t dec400CrDataBase;
    ptr_t dec400CbDataBase;
    ptr_t dec400LumDataBase;
    const void *wlInstance;
    u32 osdInputSuperTile;
} VCDec400data;

typedef struct {
    u32 tile_size;
    u32 bits_tile_in_table;
    u32 planar420_cbcr_arrangement_style; /* 0-separated 1-continuous */
} VCDec4000Attribute;

typedef struct {
    i32 (*WLGetDec400Coreid)(const void *);
    u32 (*WLGetDec400CustomerID)(const void *, u32);
    u32 (*WLGetVCMDSupport)();
    void (*WLCollectWriteDec400RegData)(const void *, u32 *, u32 *, u16, u32, u32 *);
    void (*WLWriteReg)(const void *, u32, u32, u32);
    void (*WLWriteBackReg)(const void *, u32, u32, u32);
    u32 (*WLReadReg)(const void *, u32, u32);
    void (*WLCollectStallDec400)(const void *, u32 *, u32 *);
    void (*WLCollectClrIntReadClearDec400Data)(const void *, u32 *, u32 *, u16);
} DEC400_WL_INTERFACE;

#define EncDec400SetAddrRegisterValue(dec400_data, name, value)                                    \
    do {                                                                                           \
        if (sizeof(ptr_t) == 8) {                                                                  \
            DEC400WriteReg(dec400_data, name, value);                                              \
            DEC400WriteReg(dec400_data, name##Ex, (u32)((value) >> 32));                           \
        } else {                                                                                   \
            DEC400WriteReg(dec400_data, name, (u32)(value));                                       \
        }                                                                                          \
    } while (0)

/*------------------------------------------------------------------------------
      Function prototypes
  ------------------------------------------------------------------------------*/
#ifndef SUPPORT_DEC400
#define GetDec400Attribute(...) (0)
#define VCEncEnableDec400(...) (0)
#define VCEncDisableDec400(...)
#define VCEncSetDec400StreamBypass(...)
#define VCEncDec400RegisiterWL(...)
#define VCEncDec400UnregisiterWL()
#else
i32 GetDec400Attribute(const void *wlInstance, VCDec4000Attribute *dec400_attr);

i32 VCEncEnableDec400(VCDec400data *dec400_data);

void VCEncDisableDec400(VCDec400data *dec400_data);

void VCEncSetDec400StreamBypass(VCDec400data *dec400_data);

void VCEncDec400RegisiterWL(const DEC400_WL_INTERFACE *dec400Wl);

void VCEncDec400UnregisiterWL();
#endif

#ifdef __cplusplus
}
#endif

#endif
