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
--                 The entire notice above must be reproduced                                                  --
--                  on all copies and should not be removed.                                                     --
--                                                                                                                                --
--------------------------------------------------------------------------------
--
--  Abstract : VC Encoder DEC400 Interface Implementation
--
------------------------------------------------------------------------------*/
#ifdef SUPPORT_DEC400
#include "vsi_string.h"
#include "instance.h"
#include "encdec400.h"
#include "osal.h"

#ifdef SUPPORT_DEC400
#define FULL_ZERO (0x0)
#define FULL_FF (0xFFFFFFFF)
#define BYPASS_VALUE FULL_ZERO

static VCDec4000Attribute dec400_attribute;
static i32 dec400_customer_idx = -1;
static u32 dec400_offset_idx = 0;
/* 0-64 byte alignment(default) 1-32*/
static u32 dec400_data_algin_offset_idx = 0;

u32 reg_offset[2][DEC400_REG_MAX] = {

    {0xB00,  0xB70,  0xC80,  0xC84,  0xC88,  0xC8C,  0x800, 0x804, 0x808, 0x80C,
     0x900,  0x904,  0x908,  0x90C,  0xF80,  0xF84,  0xF88, 0xF8C, 0xE80, 0xE84,
     0xE88,  0xE8C,  0x1180, 0x1184, 0x1188, 0x118C, 0x980, 0x984, 0x988, 0x98C,
     0x1000, 0x1004, 0x1008, 0x100C, 0x0,    0x0,    0x880, 0x884, 0x888, 0x88C},

    {0x800,  0x804,  0x900,  0x904,  0x908, 0x90C, 0x880,  0x884,  0x888,  0x88C,
     0xA80,  0xA84,  0xA88,  0xA8C,  0xB00, 0xB04, 0xB08,  0xB0C,  0xB80,  0xB84,
     0xB88,  0XB8C,  0xC00,  0xC04,  0xC08, 0xC0C, 0x1080, 0x1084, 0x1088, 0x108C,
     0x1100, 0x1104, 0x1108, 0x110C, 0x814, 0x820, 0x980,  0x984,  0x988,  0x98C}};

static DEC400_WL_INTERFACE dec400WlInterface;

static void EncDec400getAlignedPicSizebyFormat(u32 type, u32 width, u32 height, u32 alignment,
                                               u32 *luma_Size, u32 *chroma_Size, u32 *picture_Size);
static void VCEncSetReadChannel(u32 ts_size_pix, u32 offset, VCDec400data *dec400_data);
static void VCEncSetReadChannel_2(u32 ts_size_pix, u32 offset, VCDec400data *dec400_data);
static void VCEncSetReadChannel_3(u32 ts_size_pix, u32 offset, VCDec400data *dec400_data);
static void VCEncSetReadChannel_4(u32 ts_size_pix, u32 offset, VCDec400data *dec400_data);
static void DEC400WriteReg(VCDec400data *dec400_data, u32 offset, u32 val);
static void DEC400WriteRegBack(VCDec400data *dec400_data, u32 offset, u32 val);
static u32 DEC400ReadReg(const void *pwl, u32 offset);

void EncDec400getAlignedPicSizebyFormat(u32 type, u32 width, u32 height, u32 alignment,
                                        u32 *luma_Size, u32 *chroma_Size, u32 *picture_Size)
{
    u32 luma_stride = 0, chroma_stride = 0;
    u32 lumaSize = 0, chromaSize = 0, pictureSize = 0;

    EncGetAlignedByteStride(width, type, &luma_stride, &chroma_stride, alignment);
    switch (type) {
    case VCENC_YUV420_PLANAR:
        lumaSize = luma_stride * height;
        chromaSize = chroma_stride * height / 2 * 2;
        break;
    case VCENC_YUV420_SEMIPLANAR:
    case VCENC_YUV420_SEMIPLANAR_VU:
        lumaSize = luma_stride * height;
        chromaSize = chroma_stride * height / 2;
        break;
    case VCENC_YUV422_INTERLEAVED_YUYV:
    case VCENC_YUV422_INTERLEAVED_UYVY:
    case VCENC_RGB565:
    case VCENC_BGR565:
    case VCENC_RGB555:
    case VCENC_BGR555:
    case VCENC_RGB444:
    case VCENC_BGR444:
    case VCENC_RGB888:
    case VCENC_BGR888:
    case VCENC_RGB101010:
    case VCENC_BGR101010:
        lumaSize = luma_stride * height;
        chromaSize = 0;
        break;
    case VCENC_YUV420_PLANAR_10BIT_I010:
        lumaSize = luma_stride * height;
        chromaSize = chroma_stride * height / 2 * 2;
        break;
    case VCENC_YUV420_PLANAR_10BIT_P010:
        lumaSize = luma_stride * height;
        chromaSize = chroma_stride * height / 2;
        break;
    case VCENC_YUV420_PLANAR_10BIT_PACKED_PLANAR:
        lumaSize = luma_stride * 10 / 8 * height;
        chromaSize = chroma_stride * 10 / 8 * height / 2 * 2;
        break;
    case VCENC_YUV420_10BIT_PACKED_Y0L2:
        lumaSize = luma_stride * 2 * 2 * height / 2;
        chromaSize = 0;
        break;
    case VCENC_YUV420_PLANAR_8BIT_TILE_32_32:
        lumaSize = luma_stride * ((height + 32 - 1) & (~(32 - 1)));
        chromaSize = lumaSize / 2;
        break;
    case VCENC_YUV420_PLANAR_8BIT_TILE_16_16_PACKED_4:
        lumaSize = luma_stride * height * 2 * 12 / 8;
        chromaSize = 0;
        break;
    case VCENC_YUV420_SEMIPLANAR_8BIT_TILE_4_4:
    case VCENC_YUV420_SEMIPLANAR_VU_8BIT_TILE_4_4:
        lumaSize = luma_stride * ((height + 3) / 4);
        chromaSize = chroma_stride * (((height / 2) + 3) / 4);
        break;
    case VCENC_YUV420_PLANAR_10BIT_P010_TILE_4_4:
        lumaSize = luma_stride * ((height + 3) / 4);
        chromaSize = chroma_stride * (((height / 2) + 3) / 4);
        break;
    case VCENC_YUV420_SEMIPLANAR_101010:
        lumaSize = luma_stride * height;
        chromaSize = chroma_stride * height / 2;
        break;
    case VCENC_YUV420_8BIT_TILE_64_4:
    case VCENC_YUV420_UV_8BIT_TILE_64_4:
        lumaSize = luma_stride * ((height + 3) / 4);
        chromaSize = chroma_stride * (((height / 2) + 3) / 4);
        break;
    case VCENC_YUV420_10BIT_TILE_32_4:
        lumaSize = luma_stride * ((height + 3) / 4);
        chromaSize = chroma_stride * (((height / 2) + 3) / 4);
        break;
    case VCENC_YUV420_10BIT_TILE_48_4:
    case VCENC_YUV420_VU_10BIT_TILE_48_4:
        lumaSize = luma_stride * ((height + 3) / 4);
        chromaSize = chroma_stride * (((height / 2) + 3) / 4);
        break;
    case VCENC_YUV420_8BIT_TILE_128_2:
    case VCENC_YUV420_UV_8BIT_TILE_128_2:
        lumaSize = luma_stride * ((height + 1) / 2);
        chromaSize = chroma_stride * (((height / 2) + 1) / 2);
        break;
    case VCENC_YUV420_10BIT_TILE_96_2:
    case VCENC_YUV420_VU_10BIT_TILE_96_2:
        lumaSize = luma_stride * ((height + 1) / 2);
        chromaSize = chroma_stride * (((height / 2) + 1) / 2);
        break;
    case VCENC_YUV420_8BIT_TILE_8_8:
        lumaSize = luma_stride * ((height + 7) / 8);
        chromaSize = chroma_stride * (((height / 2) + 3) / 4);
        break;
    case VCENC_YUV420_10BIT_TILE_8_8:
        lumaSize = luma_stride * ((height + 7) / 8);
        chromaSize = chroma_stride * (((height / 2) + 3) / 4);
        break;
    default:
        printf("not support this format\n");
        chromaSize = lumaSize = 0;
        break;
    }

    pictureSize = lumaSize + chromaSize;
    if (luma_Size != NULL)
        *luma_Size = lumaSize;
    if (chroma_Size != NULL)
        *chroma_Size = chromaSize;
    if (picture_Size != NULL)
        *picture_Size = pictureSize;
}

static void DEC400WriteReg(VCDec400data *dec400_data, u32 offset, u32 val)
{
    u32 addr_offset = reg_offset[dec400_offset_idx][offset];
    u32 vcmd_en;
    vcmd_en = dec400WlInterface.WLGetVCMDSupport();
    if (vcmd_en == 1) {
        u32 current_length = 0;
        dec400WlInterface.WLCollectWriteDec400RegData(
            dec400_data->wlInstance, &val, dec400_data->regVcmdBuf + *dec400_data->regVcmdBufSize,
            addr_offset / 4, 1, &current_length);
        *dec400_data->regVcmdBufSize += current_length;
    } else {
        dec400WlInterface.WLWriteReg(dec400_data->wlInstance, addr_offset, val,
                                     EWL_CLIENT_TYPE_DEC400);
    }
}

static void DEC400WriteRegBack(VCDec400data *dec400_data, u32 offset, u32 val)
{
    u32 addr_offset = reg_offset[dec400_offset_idx][offset];
    u32 vcmd_en;
    vcmd_en = dec400WlInterface.WLGetVCMDSupport();
    if (vcmd_en == 1) {
        u32 current_length = 0;
        dec400WlInterface.WLCollectWriteDec400RegData(
            dec400_data->wlInstance, &val, dec400_data->regVcmdBuf + *dec400_data->regVcmdBufSize,
            addr_offset / 4, 1, &current_length);
        *dec400_data->regVcmdBufSize += current_length;
    } else {
        dec400WlInterface.WLWriteBackReg(dec400_data->wlInstance, addr_offset, val,
                                         EWL_CLIENT_TYPE_DEC400);
    }
}

static u32 DEC400ReadReg(const void *wlInstance, u32 offset)
{
    u32 addr_offset = reg_offset[dec400_offset_idx][offset];
    return dec400WlInterface.WLReadReg(wlInstance, addr_offset, EWL_CLIENT_TYPE_DEC400);
}

void VCEncSetReadChannel(u32 ts_size_pix, u32 offset, VCDec400data *dec400_data)
{
    if (ts_size_pix == RASTER256X1)
        DEC400WriteReg(dec400_data, offset, 0x12030029 & (~(dec400_data_algin_offset_idx << 16)));
    else if (ts_size_pix == RASTER128X1)
        DEC400WriteReg(dec400_data, offset, 0x14030029 & (~(dec400_data_algin_offset_idx << 16)));
    else if (ts_size_pix == RASTER64X1)
        DEC400WriteReg(dec400_data, offset, 0x1E030029 & (~(dec400_data_algin_offset_idx << 16)));
    else if (ts_size_pix == TILE64X4)
        DEC400WriteReg(dec400_data, offset, 0x0E030029 & (~(dec400_data_algin_offset_idx << 16)));
    else if (ts_size_pix == TILE32X4)
        DEC400WriteReg(dec400_data, offset, 0x10030029 & (~(dec400_data_algin_offset_idx << 16)));
}

void VCEncSetReadChannel_2(u32 ts_size_pix, u32 offset, VCDec400data *dec400_data)
{
    if (ts_size_pix == RASTER256X1)
        DEC400WriteReg(dec400_data, offset, 0x14030031 & (~(dec400_data_algin_offset_idx << 16)));
    else if (ts_size_pix == RASTER128X1)
        DEC400WriteReg(dec400_data, offset, 0x1E030031 & (~(dec400_data_algin_offset_idx << 16)));
    else if (ts_size_pix == RASTER64X1)
        DEC400WriteReg(dec400_data, offset, 0x2C030031 & (~(dec400_data_algin_offset_idx << 16)));
    else if (ts_size_pix == TILE64X4)
        DEC400WriteReg(dec400_data, offset, 0x10030031 & (~(dec400_data_algin_offset_idx << 16)));
    else if (ts_size_pix == TILE32X4)
        DEC400WriteReg(dec400_data, offset, 0x04030031 & (~(dec400_data_algin_offset_idx << 16)));
}

void VCEncSetReadChannel_3(u32 ts_size_pix, u32 offset, VCDec400data *dec400_data)
{
    if (ts_size_pix == RASTER64X1)
        DEC400WriteReg(dec400_data, offset, 0x1E030009 & (~(dec400_data_algin_offset_idx << 16)));
    else if (ts_size_pix == 32)
        DEC400WriteReg(dec400_data, offset, 0x2C030009 & (~(dec400_data_algin_offset_idx << 16)));
}

void VCEncSetReadChannel_4(u32 ts_size_pix, u32 offset, VCDec400data *dec400_data)
{
    if (ts_size_pix == RASTER64X1)
        DEC400WriteReg(dec400_data, offset, 0x1E030079 & (~(dec400_data_algin_offset_idx << 16)));
    else if (ts_size_pix == 32)
        DEC400WriteReg(dec400_data, offset, 0x2C030079 & (~(dec400_data_algin_offset_idx << 16)));
}

void VCEncSetReadChannel_OSD(u32 ts_size_pix, u32 offset, VCDec400data *dec400_data)
{
    if (ts_size_pix == TILE8x4)
        DEC400WriteReg(dec400_data, offset, 0x6030001);
    else if (ts_size_pix == TILE4x8)
        DEC400WriteReg(dec400_data, offset, 0x8030001);
}

#endif

/*******************************************************************************
 Function name   : VCEncDec400RegisiterWL
 Description     : Regisiter to corresponding interface of Wrapper Layer before calling Dec400 interface.
 Return type     : void
 Argument        : const DEC400_WL_INTERFACE* dec400Wl - Wrapper Layer interface set.
*******************************************************************************/
void VCEncDec400RegisiterWL(const DEC400_WL_INTERFACE *dec400Wl)
{
#ifdef SUPPORT_DEC400
    dec400WlInterface.WLGetDec400Coreid = dec400Wl->WLGetDec400Coreid;
    dec400WlInterface.WLGetDec400CustomerID = dec400Wl->WLGetDec400CustomerID;
    dec400WlInterface.WLGetVCMDSupport = dec400Wl->WLGetVCMDSupport;
    dec400WlInterface.WLCollectWriteDec400RegData = dec400Wl->WLCollectWriteDec400RegData;
    dec400WlInterface.WLWriteReg = dec400Wl->WLWriteReg;
    dec400WlInterface.WLWriteBackReg = dec400Wl->WLWriteBackReg;
    dec400WlInterface.WLReadReg = dec400Wl->WLReadReg;
    dec400WlInterface.WLCollectClrIntReadClearDec400Data =
        dec400Wl->WLCollectClrIntReadClearDec400Data;
    dec400WlInterface.WLCollectStallDec400 = dec400Wl->WLCollectStallDec400;
#endif
    return;
}

/*******************************************************************************
 Function name   : VCEncDec400UnregisiterWl
 Description     : unRegisiter Wrapper Layer
 Return type     : void
*******************************************************************************/
void VCEncDec400UnregisiterWl()
{
#ifdef SUPPORT_DEC400
    dec400WlInterface.WLGetDec400Coreid = NULL;
    dec400WlInterface.WLGetDec400CustomerID = NULL;
    dec400WlInterface.WLGetVCMDSupport = NULL;
    dec400WlInterface.WLCollectWriteDec400RegData = NULL;
    dec400WlInterface.WLWriteReg = NULL;
    dec400WlInterface.WLWriteBackReg = NULL;
    dec400WlInterface.WLReadReg = NULL;
    dec400WlInterface.WLCollectClrIntReadClearDec400Data = NULL;
    dec400WlInterface.WLCollectStallDec400 = NULL;
#endif
    return;
}

/*******************************************************************************
 Function name   : Dec400GetAttribute
 Description     : get dec400's attribute
 Return type     : i32 - error code
 Argument        : const void *wlInstance - void pointer to wl instance
                   VCDec4000Attribute *dec400_attribute - dec400 attribute
*******************************************************************************/
i32 GetDec400Attribute(const void *wlInstance, VCDec4000Attribute *dec400_attr)
{
#ifdef SUPPORT_DEC400
    i32 core_id;
    u32 dec400_customer_reg;

    ASSERT(dec400_attr);

    if (-1 != dec400_customer_idx) {
        *dec400_attr = dec400_attribute;
        return VCENC_OK;
    }

    //get version id
    if ((core_id = dec400WlInterface.WLGetDec400Coreid(wlInstance)) == -1)
        return VCENC_INVALID_ARGUMENT;

    if ((dec400_customer_reg = dec400WlInterface.WLGetDec400CustomerID(wlInstance, core_id)) == 0)
        return VCENC_INVALID_ARGUMENT;

    printf("-->dec400_customer_reg is 0x%x\n", dec400_customer_reg);
    /* Tile size is actually decided by tile_mode and byte_per_pixel
     tileSize = tile_mode_x * tile_mode_y * byte_per_pixel
     tableSize = STRIDE(stride, tile_mode_x) * STRIDE(height, tile_mode_y) / tileSize
     Ex: tile mode 8x4, format ARGB8888
     tileSize = 8*4*4 = 128 */

    if (dec400_customer_reg == THBM1_DEC400 || dec400_customer_reg == THBM3_DEC400) {
        dec400_customer_idx = 0;
        dec400_data_algin_offset_idx = 0;
        dec400_attribute.tile_size = 256;
        dec400_attribute.bits_tile_in_table = 4;
        dec400_attribute.planar420_cbcr_arrangement_style = 0;
    } else if (dec400_customer_reg == NETINT_DEC400 || dec400_customer_reg == FALCON_DEC400 ||
               dec400_customer_reg == THEAD_DEC400 || dec400_customer_reg == INTELLIF_DEC400) {
        dec400_customer_idx = 1;
        dec400_data_algin_offset_idx = 1;
        dec400_attribute.tile_size = 256;
        dec400_attribute.bits_tile_in_table = 4;
        dec400_attribute.planar420_cbcr_arrangement_style = 1;
    } else if (dec400_customer_reg == GOKE_DEC400) {
        dec400_customer_idx = 2;
        dec400_data_algin_offset_idx = 1;
        dec400_attribute.tile_size = 128;
        dec400_attribute.bits_tile_in_table = 4;
        dec400_attribute.planar420_cbcr_arrangement_style = 0;
    } else if (dec400_customer_reg ==
               OYBM1_DEC400) { // OYB M1 planner format: padding cb cr respectively
        dec400_customer_idx = 3;
        dec400_data_algin_offset_idx = 0;
        dec400_attribute.tile_size = 256;
        dec400_attribute.bits_tile_in_table = 4;
        dec400_attribute.planar420_cbcr_arrangement_style = 0;
    } else if (dec400_customer_reg == OYBM3_DEC400) {
        dec400_customer_idx = 4;
        dec400_data_algin_offset_idx = 0;
        dec400_attribute.tile_size = 256;
        dec400_attribute.bits_tile_in_table = 4;
        dec400_attribute.planar420_cbcr_arrangement_style = 0;
    } else if (dec400_customer_reg == XILINX2_DEC400) {
        dec400_customer_idx = 5;
        dec400_data_algin_offset_idx = 1;
        dec400_attribute.tile_size = 256;
        dec400_attribute.bits_tile_in_table = 4;
        dec400_attribute.planar420_cbcr_arrangement_style = 0;
    } else {
        printf("current doesn't support this dec400_customer_reg, please check\n");
        ASSERT(0);
    }

    if (0 != dec400_customer_idx) {
        dec400_offset_idx = 1;
    }
    *dec400_attr = dec400_attribute;
#endif

    return VCENC_OK;
}

/*******************************************************************************
 Function name   : VCEncEnableDec400
 Description     :
 Return type     : i32 - error code
 Argument        : VCDec400data *dec400_data - pointer to struct VCDec400data.
*******************************************************************************/
i32 VCEncEnableDec400(VCDec400data *dec400_data)
{
#ifdef SUPPORT_DEC400
    //regValues_s* regs = &asic->regs;
    u32 ts_size_pix = 0;
    u32 format = dec400_data->format;
    u32 lumWidthSrc = dec400_data->lumWidthSrc;
    u32 lumHeightSrc = dec400_data->lumHeightSrc;
    u32 input_alignment = dec400_data->input_alignment;
    ptr_t dec400LumTableBase = dec400_data->dec400LumTableBase;
    ptr_t dec400CbTableBase = dec400_data->dec400CbTableBase;
    ptr_t dec400CrTableBase = dec400_data->dec400CrTableBase;
    const void *wlInstance = dec400_data->wlInstance;
    u32 luma_Size = 0, chroma_Size = 0, U_size = 0, V_size = 0;
    VCDec4000Attribute dec400_attr;

    if (VCENC_INVALID_ARGUMENT == GetDec400Attribute(wlInstance, &dec400_attr)) {
        return VCENC_INVALID_ARGUMENT;
    }

    EncDec400getAlignedPicSizebyFormat(format, lumWidthSrc, lumHeightSrc, input_alignment,
                                       &luma_Size, &chroma_Size, NULL);

    /* OSD input is SuperTile format */
    if (dec400_data->osdInputSuperTile != 0) {
        chroma_Size = 0;
        luma_Size = lumWidthSrc * ((lumHeightSrc + 63) / 64);
    }

    switch (format) {
    case VCENC_YUV420_PLANAR:
    case VCENC_YUV420_SEMIPLANAR:
    case VCENC_YUV420_SEMIPLANAR_VU:
        if (2 == dec400_customer_idx)
            ts_size_pix = RASTER128X1;
        else if (5 == dec400_customer_idx)
            ts_size_pix = TILE64X4;
        else
            ts_size_pix = RASTER256X1;
        break;
    case VCENC_YUV420_PLANAR_10BIT_I010:
    case VCENC_YUV420_PLANAR_10BIT_P010:
        ts_size_pix = RASTER128X1;
        break;
    case VCENC_RGB888:
    case VCENC_BGR888:
    case VCENC_RGB101010:
    case VCENC_BGR101010:
        ts_size_pix = RASTER64X1;
        if (dec400_data->osdInputSuperTile == 1)
            ts_size_pix = TILE8x4;
        else if (dec400_data->osdInputSuperTile == 2)
            ts_size_pix = TILE4x8;
        break;
    case VCENC_YUV420_SEMIPLANAR_8BIT_TILE_4_4:
    case VCENC_YUV420_SEMIPLANAR_VU_8BIT_TILE_4_4:
        ts_size_pix = TILE64X4;
        break;
    case VCENC_YUV420_PLANAR_10BIT_P010_TILE_4_4:
        ts_size_pix = TILE32X4;
        break;
    default:
        return VCENC_INVALID_ARGUMENT;
    }

    DEC400WriteReg(dec400_data, gcregAHBDECControl, 0x00000000);
    DEC400WriteReg(dec400_data, gcregAHBDECControlEx, 0x00000000);

    if (dec400_customer_idx != 0)
        DEC400WriteReg(dec400_data, gcregAHBDECIntrEnblEx2, 0x00000001);

    switch (format) {
    //V component
    case VCENC_YUV420_PLANAR:
    case VCENC_YUV420_PLANAR_10BIT_I010:
        V_size = chroma_Size / 2;

        if (format == VCENC_YUV420_PLANAR) {
            DEC400WriteReg(dec400_data, gcregAHBDECReadExConfig2, 0x00000000);
        } else {
            DEC400WriteReg(dec400_data, gcregAHBDECReadExConfig2, 0x00030000);
        }

        VCEncSetReadChannel(ts_size_pix, gcregAHBDECReadConfig2, dec400_data);
        if (dec400_customer_idx == 3)
            VCEncSetReadChannel(ts_size_pix, gcregAHBDECWriteConfig2, dec400_data);

        EncDec400SetAddrRegisterValue(dec400_data, gcregAHBDECReadBufferBase2,
                                      dec400_data->dec400CrDataBase);
        ptr_t end2_addr = (dec400_data->dec400CrDataBase == BYPASS_VALUE)
                              ? BYPASS_VALUE
                              : (dec400_data->dec400CrDataBase + V_size - 1);
        EncDec400SetAddrRegisterValue(dec400_data, gcregAHBDECReadBufferEnd2, end2_addr);

        EncDec400SetAddrRegisterValue(dec400_data, gcregAHBDECReadCacheBase2, dec400CrTableBase);

    //U component
    case VCENC_YUV420_SEMIPLANAR:
    case VCENC_YUV420_SEMIPLANAR_VU:
    case VCENC_YUV420_PLANAR_10BIT_P010:
    case VCENC_YUV420_SEMIPLANAR_8BIT_TILE_4_4:
    case VCENC_YUV420_SEMIPLANAR_VU_8BIT_TILE_4_4:
    case VCENC_YUV420_PLANAR_10BIT_P010_TILE_4_4:
        if (format == VCENC_YUV420_PLANAR) {
            DEC400WriteReg(dec400_data, gcregAHBDECReadExConfig1, 0x00000000);
            VCEncSetReadChannel(ts_size_pix, gcregAHBDECReadConfig1, dec400_data);
            if (dec400_customer_idx == 3)
                VCEncSetReadChannel(ts_size_pix, gcregAHBDECWriteConfig1, dec400_data);
            U_size = chroma_Size / 2;
        } else if (format == VCENC_YUV420_PLANAR_10BIT_I010) {
            DEC400WriteReg(dec400_data, gcregAHBDECReadExConfig1, 0x00030000);
            VCEncSetReadChannel(ts_size_pix, gcregAHBDECReadConfig1, dec400_data);
            U_size = chroma_Size / 2;
        } else if (format == VCENC_YUV420_PLANAR_10BIT_P010 ||
                   format == VCENC_YUV420_PLANAR_10BIT_P010_TILE_4_4) {
            DEC400WriteReg(dec400_data, gcregAHBDECReadExConfig1, 0x00010000);
            VCEncSetReadChannel_2(ts_size_pix, gcregAHBDECReadConfig1, dec400_data);
            U_size = chroma_Size;
        } else //NV12,NV21
        {
            DEC400WriteReg(dec400_data, gcregAHBDECReadExConfig1, 0x00000000);
            VCEncSetReadChannel_2(ts_size_pix, gcregAHBDECReadConfig1, dec400_data);
            if (dec400_customer_idx == 3)
                VCEncSetReadChannel_2(ts_size_pix, gcregAHBDECWriteConfig1, dec400_data);
            U_size = chroma_Size;
        }

        EncDec400SetAddrRegisterValue(dec400_data, gcregAHBDECReadBufferBase1,
                                      dec400_data->dec400CbDataBase);
        ptr_t end1_addr = (dec400_data->dec400CbDataBase == BYPASS_VALUE)
                              ? BYPASS_VALUE
                              : (dec400_data->dec400CbDataBase + U_size - 1);
        EncDec400SetAddrRegisterValue(dec400_data, gcregAHBDECReadBufferEnd1, end1_addr);

        EncDec400SetAddrRegisterValue(dec400_data, gcregAHBDECReadCacheBase1, dec400CbTableBase);

    //Y component
    case VCENC_RGB888:
    case VCENC_BGR888:
    case VCENC_RGB101010:
    case VCENC_BGR101010:
        if (dec400_data->osdInputSuperTile) {
            DEC400WriteReg(dec400_data, gcregAHBDECReadExConfig3, 0x00000000);
            VCEncSetReadChannel_OSD(ts_size_pix, gcregAHBDECReadConfig3, dec400_data);
            /* Because of DEC400 HW bug, have to set writeConfig too for align 64 byte */
            VCEncSetReadChannel_OSD(ts_size_pix, gcregAHBDECWriteConfig3, dec400_data);

            EncDec400SetAddrRegisterValue(dec400_data, gcregAHBDECReadBufferBase3,
                                          dec400_data->dec400LumDataBase);
            ptr_t end3_addr = (dec400_data->dec400LumDataBase == BYPASS_VALUE)
                                  ? BYPASS_VALUE
                                  : (dec400_data->dec400LumDataBase + luma_Size - 1);
            EncDec400SetAddrRegisterValue(dec400_data, gcregAHBDECReadBufferEnd3, end3_addr);

            EncDec400SetAddrRegisterValue(dec400_data, gcregAHBDECReadCacheBase3,
                                          dec400LumTableBase);
            break;
        }
        if (format == VCENC_YUV420_PLANAR) {
            DEC400WriteReg(dec400_data, gcregAHBDECReadExConfig0, 0x00000000);
        } else if (format == VCENC_YUV420_PLANAR_10BIT_I010) {
            DEC400WriteReg(dec400_data, gcregAHBDECReadExConfig0, 0x00030000);
        } else if (format == VCENC_YUV420_PLANAR_10BIT_P010 ||
                   format == VCENC_YUV420_PLANAR_10BIT_P010_TILE_4_4) {
            DEC400WriteReg(dec400_data, gcregAHBDECReadExConfig0, 0x00010000);
        } else if (format == VCENC_YUV420_SEMIPLANAR || format == VCENC_YUV420_SEMIPLANAR_VU ||
                   format == VCENC_YUV420_SEMIPLANAR_8BIT_TILE_4_4 ||
                   format == VCENC_YUV420_SEMIPLANAR_VU_8BIT_TILE_4_4) {
            DEC400WriteReg(dec400_data, gcregAHBDECReadExConfig0, 0x00000000);
        }

        if (format == VCENC_RGB888 || format == VCENC_BGR888) {
            VCEncSetReadChannel_3(ts_size_pix, gcregAHBDECReadConfig0, dec400_data);
            if (dec400_customer_idx == 3)
                VCEncSetReadChannel_3(ts_size_pix, gcregAHBDECWriteConfig0, dec400_data);
        } else if (format == VCENC_RGB101010 || format == VCENC_BGR101010) {
            VCEncSetReadChannel_4(ts_size_pix, gcregAHBDECReadConfig0, dec400_data);
            if (dec400_customer_idx == 3)
                VCEncSetReadChannel_4(ts_size_pix, gcregAHBDECWriteConfig0, dec400_data);
        } else {
            VCEncSetReadChannel(ts_size_pix, gcregAHBDECReadConfig0, dec400_data);
            if (dec400_customer_idx == 3)
                VCEncSetReadChannel(ts_size_pix, gcregAHBDECWriteConfig0, dec400_data);
        }

        EncDec400SetAddrRegisterValue(dec400_data, gcregAHBDECReadBufferBase0,
                                      dec400_data->dec400LumDataBase);
        ptr_t end0_addr = (dec400_data->dec400LumDataBase == BYPASS_VALUE)
                              ? BYPASS_VALUE
                              : (dec400_data->dec400LumDataBase + luma_Size - 1);
        EncDec400SetAddrRegisterValue(dec400_data, gcregAHBDECReadBufferEnd0, end0_addr);

        EncDec400SetAddrRegisterValue(dec400_data, gcregAHBDECReadCacheBase0, dec400LumTableBase);
        break;
    default:
        return VCENC_INVALID_ARGUMENT;
    }

#endif
    return VCENC_OK;
}

/*******************************************************************************
 Function name   : VCEncDisableDec400
 Description     :
 Return type     : void
 Argument        : VCDec400data *dec400_data - pointer to struct VCDec400data.
*******************************************************************************/
void VCEncDisableDec400(VCDec400data *dec400_data)
{
#ifdef SUPPORT_DEC400
    const void *wlInstance = dec400_data->wlInstance;
    u32 loop = 1000;
    u32 vcmd_en;

    vcmd_en = dec400WlInterface.WLGetVCMDSupport();

    if (dec400_customer_idx == 0) {
        //do SW reset after one frame and wait it done by HW if needed
        DEC400WriteRegBack(dec400_data, gcregAHBDECControl, 0x00000010);
#ifdef PC_PCI_FPGA_DEMO
        usleep(80000);
#endif
    } else {
        //flush tile status and wait for IRQ
        DEC400WriteRegBack(dec400_data, gcregAHBDECControl, 0x00000001);
        if (vcmd_en == 0) {
            do {
                if (DEC400ReadReg(wlInstance, gcregAHBDECIntrAcknowledgeEx2) & 0x00000001)
                    break;
                usleep(80);
            } while (loop--);
        } else {
            //insert stall cmd.
            {
                u32 current_length = 0;
                dec400WlInterface.WLCollectStallDec400(
                    dec400_data->wlInstance, dec400_data->regVcmdBuf + *dec400_data->regVcmdBufSize,
                    &current_length);
                *dec400_data->regVcmdBufSize += current_length;
            }
            //clear int status register
            {
                u32 current_length = 0;
                u32 addr_offset = reg_offset[dec400_offset_idx][gcregAHBDECIntrAcknowledgeEx2];
                dec400WlInterface.WLCollectClrIntReadClearDec400Data(
                    dec400_data->wlInstance, dec400_data->regVcmdBuf + *dec400_data->regVcmdBufSize,
                    &current_length, addr_offset / 4);
                *dec400_data->regVcmdBufSize += current_length;
            }
        }
    }

#endif
}

/*******************************************************************************
 Function name   : VCEncSetDec400StreamBypass
 Description     : Make dec400 bypass and nothing to do by configing the start
                   and end address as 0 for dec400 table and data that will make
                   dec400 cann't hold the frame buffer data.
 Return type     : void
 Argument        : VCDec400data *dec400_data - pointer to struct VCDec400data.
*******************************************************************************/
void VCEncSetDec400StreamBypass(VCDec400data *dec400_data)
{
#ifdef SUPPORT_DEC400
    i32 core_id = dec400WlInterface.WLGetDec400Coreid(dec400_data->wlInstance);
    if (core_id == -1)
        return;

    if (dec400_customer_idx == 0) {
        //do SW reset after one frame and wait it done by HW if needed
        DEC400WriteReg(dec400_data, gcregAHBDECControl, 0x00000010);
#ifdef PC_PCI_FPGA_DEMO
        usleep(80000);
#endif
        //disable global bypass to enable stream bypass
        DEC400WriteReg(dec400_data, gcregAHBDECControl, 0x02010088);
    } else if (dec400_customer_idx == 1) {
        dec400_data->dec400LumTableBase = BYPASS_VALUE;
        dec400_data->dec400CbTableBase = BYPASS_VALUE;
        dec400_data->dec400CrTableBase = BYPASS_VALUE;
        dec400_data->dec400CrDataBase = BYPASS_VALUE;
        dec400_data->dec400CbDataBase = BYPASS_VALUE;
        dec400_data->dec400LumDataBase = BYPASS_VALUE;
        VCEncEnableDec400(dec400_data);
    } else {
        /* didn't support bypass dec400? */
    }
#endif
}

#endif
