/* 
 * Copyright (c) 2018, Chips&Media
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#define PLATFORM_LINUX

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <time.h>
#include "enc_regdefine.h"
#include "enc_driver.h"
#include "vpuapifunc.h"
#include "vdi_osal.h"
#if defined(PLATFORM_LINUX) || defined(PLATFORM_QNX)
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#endif

#include "debug.h"
#include "vpuconfig.h"


enum { False, True };

void InitializeDebugEnv(
    Uint32  options
    )
{
    UNREFERENCED_PARAMETER(options);
}

void ReleaseDebugEnv(
    void
    )
{
}

#if 0
Int32 checkLineFeedInHelp(
    struct OptionExt *opt
    )
{
    int i;

    for (i=0;i<MAX_GETOPT_OPTIONS;i++) {
        if (opt[i].name==NULL)
            break;
        if (!strstr(opt[i].help, "\n")) {
            VLOG(INFO, "(%s) doesn't have \\n in options struct in main function. please add \\n\n", opt[i].help);
            return FALSE;
        }
    }
    return TRUE;
}
#endif


#define API_VERSION_MAJOR       5
#define API_VERSION_MINOR       5
#define API_VERSION_PATCH       51
#define API_VERSION             ((API_VERSION_MAJOR<<16) | (API_VERSION_MINOR<<8) | API_VERSION_PATCH)

RetCode PrintVpuProductInfo(
    Uint32      coreIdx,
    ProductInfo *productInfo
    )
{
    BOOL verbose = FALSE;
    RetCode ret = RETCODE_SUCCESS;

    if ((ret = VPU_GetProductInfo(coreIdx, productInfo)) != RETCODE_SUCCESS) {
        //PrintVpuStatus(coreIdx, productInfo->productId);//TODO
        return ret;
    }

    VLOG(INFO, "VPU coreNum : [%d]\n", coreIdx);
    VLOG(INFO, "Firmware : CustomerCode: %04x | version : rev.%d\n", productInfo->customerId, productInfo->fwVersion);
    VLOG(INFO, "Hardware : %04x\n", productInfo->productId);
    VLOG(INFO, "API      : %d.%d.%d\n\n", API_VERSION_MAJOR, API_VERSION_MINOR, API_VERSION_PATCH);	
    if (PRODUCT_ID_VP_SERIES(productInfo->productId))
    {
        VLOG(INFO, "productId       : %08x\n", productInfo->productId);
        VLOG(INFO, "fwVersion       : %08x(r%d)\n", productInfo->fwVersion, productInfo->fwVersion);
        VLOG(INFO, "productName     : %c%c%c%c%04x\n", 
            productInfo->productName>>24, productInfo->productName>>16, productInfo->productName>>8, productInfo->productName, 
            productInfo->productVersion);
        if ( verbose == TRUE )
        {
            Uint32 stdDef0          = productInfo->stdDef0;
            Uint32 stdDef1          = productInfo->stdDef1;
            Uint32 confFeature      = productInfo->confFeature;
            BOOL supportDownScaler  = FALSE;
            BOOL supportAfbce       = FALSE;
            char ch_ox[2]           = {'X', 'O'};

            VLOG(INFO, "==========================\n");
            VLOG(INFO, "stdDef0           : %08x\n", stdDef0);
            /* checking ONE AXI BIT FILE */
            VLOG(INFO, "MAP CONVERTER REG : %d\n", (stdDef0>>31)&1);
            VLOG(INFO, "MAP CONVERTER SIG : %d\n", (stdDef0>>30)&1);
            VLOG(INFO, "BG_DETECT         : %d\n", (stdDef0>>20)&1);
            VLOG(INFO, "3D NR             : %d\n", (stdDef0>>19)&1);
            VLOG(INFO, "ONE-PORT AXI      : %d\n", (stdDef0>>18)&1);
            VLOG(INFO, "2nd AXI           : %d\n", (stdDef0>>17)&1);
            VLOG(INFO, "GDI               : %d\n", !((stdDef0>>16)&1));//no-gdi
            VLOG(INFO, "AFBC              : %d\n", (stdDef0>>15)&1);
            VLOG(INFO, "AFBC VERSION      : %d\n", (stdDef0>>12)&7);
            VLOG(INFO, "FBC               : %d\n", (stdDef0>>11)&1);
            VLOG(INFO, "FBC  VERSION      : %d\n", (stdDef0>>8)&7);
            VLOG(INFO, "SCALER            : %d\n", (stdDef0>>7)&1);
            VLOG(INFO, "SCALER VERSION    : %d\n", (stdDef0>>4)&7);
            VLOG(INFO, "BWB               : %d\n", (stdDef0>>3)&1);
            VLOG(INFO, "==========================\n");
            VLOG(INFO, "stdDef1           : %08x\n", stdDef1);
            VLOG(INFO, "CyclePerTick      : %d\n", (stdDef1>>27)&1); //0:32768, 1:256
            VLOG(INFO, "MULTI CORE EN     : %d\n", (stdDef1>>26)&1);
            VLOG(INFO, "GCU EN            : %d\n", (stdDef1>>25)&1);
            VLOG(INFO, "CU REPORT         : %d\n", (stdDef1>>24)&1);
            VLOG(INFO, "VCORE ID 3        : %d\n", (stdDef1>>19)&1);
            VLOG(INFO, "VCORE ID 2        : %d\n", (stdDef1>>18)&1);
            VLOG(INFO, "VCORE ID 1        : %d\n", (stdDef1>>17)&1);
            VLOG(INFO, "VCORE ID 0        : %d\n", (stdDef1>>16)&1);
            VLOG(INFO, "BW OPT            : %d\n", (stdDef1>>15)&1);
            VLOG(INFO, "==========================\n");
            VLOG(INFO, "confFeature       : %08x\n", confFeature);
            VLOG(INFO, "VP9  ENC Profile2 : %d\n", (confFeature>>7)&1);
            VLOG(INFO, "VP9  ENC Profile0 : %d\n", (confFeature>>6)&1);
            VLOG(INFO, "VP9  DEC Profile2 : %d\n", (confFeature>>5)&1);
            VLOG(INFO, "VP9  DEC Profile0 : %d\n", (confFeature>>4)&1);
            VLOG(INFO, "HEVC ENC MAIN10   : %d\n", (confFeature>>3)&1);
            VLOG(INFO, "HEVC ENC MAIN     : %d\n", (confFeature>>2)&1);
            VLOG(INFO, "HEVC DEC MAIN10   : %d\n", (confFeature>>1)&1);
            VLOG(INFO, "HEVC DEC MAIN     : %d\n", (confFeature>>0)&1);
            VLOG(INFO, "==========================\n");
            VLOG(INFO, "configDate        : %d\n", productInfo->configDate);
            VLOG(INFO, "HW version        : r%d\n", productInfo->configRevision);

            supportDownScaler = (BOOL)((stdDef0>>7)&1);
            supportAfbce      = (BOOL)((stdDef0>>15)&1);

            VLOG (INFO, "------------------------------------\n");
            VLOG (INFO, "VPU CONF| SCALER | AFBCE  |\n");
            VLOG (INFO, "        |   %c    |    %c   |\n", ch_ox[supportDownScaler], ch_ox[supportAfbce]);
            VLOG (INFO, "------------------------------------\n");
            for (coreIdx=0 ; coreIdx<MAX_NUM_VCORE ; coreIdx++) {
                if (productInfo->configVcore[coreIdx]) {
                    Uint32 std_vp9, std_hevc, std_avs2;
                    if ( coreIdx == 0)
                        VLOG (INFO, "        |  HEVC  |   VP9  |  AVS2  |\n");
                    std_vp9  = (productInfo->configVcore[coreIdx] & 0x4) ? 1 : 0;
                    std_hevc = (productInfo->configVcore[coreIdx] & 0x1) ? 1 : 0;
                    std_avs2 = (productInfo->configVcore[coreIdx] & 0x8) ? 1 : 0;
                    VLOG (INFO, " VCore%d |    %c   |    %c   |    %c   |\n",
                        coreIdx, ch_ox[std_hevc], ch_ox[std_vp9], ch_ox[std_avs2]);
                    if ( coreIdx == (MAX_NUM_VCORE-1))
                        VLOG (INFO, "------------------------------------\n");
                }
            }
        }
        else {
            VLOG(INFO, "==========================\n");
            VLOG(INFO, "stdDef0          : %08x\n", productInfo->stdDef0);
            VLOG(INFO, "stdDef1          : %08x\n", productInfo->stdDef1);
            VLOG(INFO, "confFeature      : %08x\n", productInfo->confFeature);
            VLOG(INFO, "configDate       : %08x\n", productInfo->configDate);
            VLOG(INFO, "configRevision   : %08x\n", productInfo->configRevision);
            VLOG(INFO, "configType       : %08x\n", productInfo->configType);
            VLOG(INFO, "==========================\n");
        }
    }
    return ret;
}

Uint32 ReadRegVCE(
    Uint32 coreIdx,
    Uint32 vce_core_idx,
    Uint32 vce_addr
    )
{//lint !e18
    int     vcpu_reg_addr;
    int     udata;
    int     vce_core_base = 0x8000 + 0x1000*vce_core_idx;

    SetClockGate(coreIdx, 1);
    vdi_fio_write_register(coreIdx, VCORE_DBG_READY(vce_core_idx), 0);

    vcpu_reg_addr = vce_addr >> 2;

    vdi_fio_write_register(coreIdx, VCORE_DBG_ADDR(vce_core_idx),vcpu_reg_addr + vce_core_base);

    if (vdi_fio_read_register(0, VCORE_DBG_READY(vce_core_idx)) == 1)
        udata= vdi_fio_read_register(0, VCORE_DBG_DATA(vce_core_idx));
    else {
        VLOG(ERR, "failed to read VCE register: %d, 0x%04x\n", vce_core_idx, vce_addr);
        udata = -2;//-1 can be a valid value
    }

    SetClockGate(coreIdx, 0);
    return udata;
}

void WriteRegVCE(
    Uint32   coreIdx,
    Uint32   vce_core_idx,
    Uint32   vce_addr,
    Uint32   udata
    )
{
    int vcpu_reg_addr;

    SetClockGate(coreIdx, 1);

    vdi_fio_write_register(coreIdx, VCORE_DBG_READY(vce_core_idx),0);

    vcpu_reg_addr = vce_addr >> 2;

    vdi_fio_write_register(coreIdx, VCORE_DBG_DATA(vce_core_idx),udata);
    vdi_fio_write_register(coreIdx, VCORE_DBG_ADDR(vce_core_idx),(vcpu_reg_addr) & 0x00007FFF);

    while (vdi_fio_read_register(0, VCORE_DBG_READY(vce_core_idx)) == 0xffffffff) {
        VLOG(ERR, "failed to write VCE register: 0x%04x\n", vce_addr);
    }
    SetClockGate(coreIdx, 0);
}

#define VCE_DEC_CHECK_SUM0         0x110
#define VCE_DEC_CHECK_SUM1         0x114
#define VCE_DEC_CHECK_SUM2         0x118
#define VCE_DEC_CHECK_SUM3         0x11C
#define VCE_DEC_CHECK_SUM4         0x120
#define VCE_DEC_CHECK_SUM5         0x124
#define VCE_DEC_CHECK_SUM6         0x128
#define VCE_DEC_CHECK_SUM7         0x12C
#define VCE_DEC_CHECK_SUM8         0x130
#define VCE_DEC_CHECK_SUM9         0x134
#define VCE_DEC_CHECK_SUM10        0x138
#define VCE_DEC_CHECK_SUM11        0x13C

#define READ_BIT(val,high,low) ((((high)==31) && ((low) == 0)) ?  (val) : (((val)>>(low)) & (((1<< ((high)-(low)+1))-1))))


static void	DisplayVceEncDebugCommon521(int coreIdx, int vcore_idx, int set_mode, int debug0, int debug1, int debug2)
{
    int reg_val;
    VLOG(ERR, "---------------Common Debug INFO-----------------\n");

    WriteRegVCE(coreIdx, vcore_idx, set_mode,0 );

    reg_val = ReadRegVCE(coreIdx, vcore_idx, debug0);
    VLOG(ERR,"\t- subblok_done    :  0x%x\n", READ_BIT(reg_val,30,23));
    VLOG(ERR,"\t- pipe_on[4]      :  0x%x\n", READ_BIT(reg_val,20,20));
    VLOG(ERR,"\t- cur_s2ime       :  0x%x\n", READ_BIT(reg_val,19,16));
    VLOG(ERR,"\t- cur_pipe        :  0x%x\n", READ_BIT(reg_val,15,12));
    VLOG(ERR,"\t- pipe_on[3:0]    :  0x%x\n", READ_BIT(reg_val,11, 8));

    reg_val = ReadRegVCE(coreIdx, vcore_idx, debug1);
    VLOG(ERR,"\t- i_avc_rdo_debug :  0x%x\n", READ_BIT(reg_val,31,31));
    VLOG(ERR,"\t- curbuf_prp      :  0x%x\n", READ_BIT(reg_val,28,25));
    VLOG(ERR,"\t- curbuf_s2       :  0x%x\n", READ_BIT(reg_val,24,21));
    VLOG(ERR,"\t- curbuf_s0       :  0x%x\n", READ_BIT(reg_val,20,17));
    VLOG(ERR,"\t- cur_s2ime_sel   :  0x%x\n", READ_BIT(reg_val,16,16));
    VLOG(ERR,"\t- cur_mvp         :  0x%x\n", READ_BIT(reg_val,15,14));
    VLOG(ERR,"\t- cmd_ready       :  0x%x\n", READ_BIT(reg_val,13,13));
    VLOG(ERR,"\t- rc_ready        :  0x%x\n", READ_BIT(reg_val,12,12));
    VLOG(ERR,"\t- pipe_cmd_cnt    :  0x%x\n", READ_BIT(reg_val,11, 9));
    VLOG(ERR,"\t- subblok_done    :  LF_PARAM 0x%x SFU 0x%x LF 0x%x RDO 0x%x IMD 0x%x FME 0x%x IME 0x%x\n", 
        READ_BIT(reg_val, 6, 6), READ_BIT(reg_val, 5, 5), READ_BIT(reg_val, 4, 4), READ_BIT(reg_val, 3, 3), 
        READ_BIT(reg_val, 2, 2), READ_BIT(reg_val, 1, 1), READ_BIT(reg_val, 0, 0));

    reg_val = ReadRegVCE(coreIdx, vcore_idx, debug2);
    //VLOG(ERR,"\t- reserved          :  0x%x\n", READ_BIT(reg_val,31, 23));
    VLOG(ERR,"\t- cur_prp_dma_state :  0x%x\n", READ_BIT(reg_val,22, 20));
    VLOG(ERR,"\t- cur_prp_state     :  0x%x\n", READ_BIT(reg_val,19, 18));
    VLOG(ERR,"\t- main_ctu_xpos     :  0x%x\n", READ_BIT(reg_val,17,  9));
    VLOG(ERR,"\t- main_ctu_ypos     :  0x%x\n", READ_BIT(reg_val, 8,  0));
}

static void	DisplayVceEncDebugMode2(int core_idx, int vcore_idx, int set_mode, int* debug)
{
    int reg_val;
    VLOG(ERR,"----------- MODE 2 : ----------\n");

    WriteRegVCE(core_idx,vcore_idx, set_mode, 2);

    reg_val = ReadRegVCE(core_idx, vcore_idx, debug[7]);
    VLOG(ERR,"\t- s2fme_info_full    :  0x%x\n", READ_BIT(reg_val,26,26));
    VLOG(ERR,"\t- ime_cmd_ref_full   :  0x%x\n", READ_BIT(reg_val,25,25));
    VLOG(ERR,"\t- ime_cmd_ctb_full   :  0x%x\n", READ_BIT(reg_val,24,24));
    VLOG(ERR,"\t- ime_load_info_full :  0x%x\n", READ_BIT(reg_val,23,23));
    VLOG(ERR,"\t- mvp_nb_info_full   :  0x%x\n", READ_BIT(reg_val,22,22));
    VLOG(ERR,"\t- ime_final_mv_full  :  0x%x\n", READ_BIT(reg_val,21,21));
    VLOG(ERR,"\t- ime_mv_full        :  0x%x\n", READ_BIT(reg_val,20,20));
    VLOG(ERR,"\t- cur_fme_fsm[3:0]   :  0x%x\n", READ_BIT(reg_val,19,16));
    VLOG(ERR,"\t- cur_s2me_fsm[3:0]  :  0x%x\n", READ_BIT(reg_val,15,12));
    VLOG(ERR,"\t- cur_s2mvp_fsm[3:0] :  0x%x\n", READ_BIT(reg_val,11, 8));
    VLOG(ERR,"\t- cur_ime_fsm[3:0]   :  0x%x\n", READ_BIT(reg_val, 7, 4));
    VLOG(ERR,"\t- cur_sam_fsm[3:0]   :  0x%x\n", READ_BIT(reg_val, 3, 0));
}

#define VCE_LF_PARAM               0xA6c 
#define VCE_BIN_WDMA_CUR_ADDR      0xB1C 
#define VCE_BIN_PIC_PARAM          0xB20 
#define VCE_BIN_WDMA_BASE          0xB24 
#define VCE_BIN_WDMA_END           0xB28 
static void	DisplayVceEncReadVCE(int coreIdx, int vcore_idx)
{
    int reg_val;

    VLOG(ERR, "---------------DisplayVceEncReadVCE-----------------\n");
    reg_val = ReadRegVCE(coreIdx, vcore_idx, VCE_LF_PARAM);
    VLOG(ERR,"\t- VCE_LF_PARAM             :  0x%x\n", reg_val);

    reg_val = ReadRegVCE(coreIdx, vcore_idx, VCE_BIN_WDMA_CUR_ADDR);
    VLOG(ERR,"\t- VCE_BIN_WDMA_CUR_ADDR    :  0x%x\n", reg_val);
    reg_val = ReadRegVCE(coreIdx, vcore_idx, VCE_BIN_PIC_PARAM);
    VLOG(ERR,"\t- VCE_BIN_PIC_PARAM        :  0x%x\n", reg_val);
    reg_val = ReadRegVCE(coreIdx, vcore_idx, VCE_BIN_WDMA_BASE);
    VLOG(ERR,"\t- VCE_BIN_WDMA_BASE        :  0x%x\n", reg_val);
    reg_val = ReadRegVCE(coreIdx, vcore_idx, VCE_BIN_WDMA_END);
    VLOG(ERR,"\t- VCE_BIN_WDMA_END         :  0x%x\n", reg_val);
}

void PrintVpuStatus(
    Uint32 coreIdx, 
    Uint32 productId
    )
{
    VLOG(ERR,"\t- VPU idx %x, productID %x\n", coreIdx, productId);
}

void PrintEncVpuStatus(
    EncHandle   handle
    )
{
    Int32       coreIdx;
    Int32       productId;

    coreIdx   = handle->coreIdx;
    productId = handle->productId;

    SetClockGate(coreIdx, 1);
    vdi_print_vpu_status(coreIdx);

    if ( productId == PRODUCT_ID_520 || productId == PRODUCT_ID_525 || productId == PRODUCT_ID_521)
    {
        Uint32    reg_val, num;
        int       vce_enc_debug[12] = {0, };
        int       set_mode;
        int       vcore_num, vcore_idx;
        int i;
        Uint32    vcpu_reg[31]= {0,};

        SetClockGate(coreIdx, 1);
        VLOG(ERR,"-------------------------------------------------------------------------------\n");
        VLOG(ERR,"------                            VCPU STATUS(ENC)                        -----\n");
        VLOG(ERR,"-------------------------------------------------------------------------------\n");
        VLOG(ERR,"BS_OPT: 0x%08x\n", VpuReadReg(coreIdx, VP5_BS_OPTION));

        // --------- VCPU register Dump 
        VLOG(ERR,"[+] VCPU REG Dump\n");
        for (i = 0; i < 25; i++) {
            VpuWriteReg (coreIdx, VP5_VPU_PDBG_IDX_REG, (1<<9) | (i & 0xff));
            vcpu_reg[i] = VpuReadReg (coreIdx, VP5_VPU_PDBG_RDATA_REG);

            if (i < 16) {
                VLOG(ERR,"0x%08x\t",  vcpu_reg[i]);
                if ((i % 4) == 3) VLOG(ERR,"\n");
            }
            else {
                switch (i) {
                case 16: VLOG(ERR,"CR0: 0x%08x\t", vcpu_reg[i]); break;
                case 17: VLOG(ERR,"CR1: 0x%08x\n", vcpu_reg[i]); break;
                case 18: VLOG(ERR,"ML:  0x%08x\t", vcpu_reg[i]); break;
                case 19: VLOG(ERR,"MH:  0x%08x\n", vcpu_reg[i]); break;
                case 21: VLOG(ERR,"LR:  0x%08x\n", vcpu_reg[i]); break;
                case 22: VLOG(ERR,"PC:  0x%08x\n", vcpu_reg[i]);break;
                case 23: VLOG(ERR,"SR:  0x%08x\n", vcpu_reg[i]);break;
                case 24: VLOG(ERR,"SSP: 0x%08x\n", vcpu_reg[i]);break;
                }
            }
        }
        {
//#define VP5_RET_ENC_DEBUG_START                  (VP5_REG_BASE + 0x180)
//#define VP5_RET_ENC_DEBUG_END                    (VP5_REG_BASE + 0x1B4)
            for(i=0; i < 14; i++)
                VLOG(ERR, "debug[%d] [%x]\r\n", i, VpuReadReg(coreIdx, (VP5_REG_BASE + 0x180)+ (i*4)));
        }

        VLOG(ERR,"[+] VCPU DMA Dump\n");
        for(i = 0x2000; i < 0x2018; i += 16) {
            VLOG(INFO,"0x%04xh: 0x%08x 0x%08x 0x%08x 0x%08x\n", (VP5_REG_BASE + i),
                vdi_fio_read_register(coreIdx, (VP5_REG_BASE + i)),
                vdi_fio_read_register(coreIdx, (VP5_REG_BASE + i + 4 )),
                vdi_fio_read_register(coreIdx, (VP5_REG_BASE + i + 8 )),
                vdi_fio_read_register(coreIdx, (VP5_REG_BASE + i + 12)));
        }
        VLOG(ERR,"[-] VCPU DMA Dump\n");

        VLOG(ERR,"[+] VCPU HOST REG Dump\n");
        for(i = 0x3000; i < 0x30fc; i += 16) {
            VLOG(INFO,"0x%04xh: 0x%08x 0x%08x 0x%08x 0x%08x\n", (VP5_REG_BASE + i),
                vdi_fio_read_register(coreIdx, (VP5_REG_BASE + i)),
                vdi_fio_read_register(coreIdx, (VP5_REG_BASE + i + 4 )),
                vdi_fio_read_register(coreIdx, (VP5_REG_BASE + i + 8 )),
                vdi_fio_read_register(coreIdx, (VP5_REG_BASE + i + 12)));
        }
        VLOG(ERR,"[-] VCPU HOST REG Dump\n");

        VLOG(ERR,"[+] VCPU ENT ENC REG Dump\n");
        for(i = 0x6800; i < 0x7000; i += 16) {
            VLOG(INFO,"0x%04xh: 0x%08x 0x%08x 0x%08x 0x%08x\n", (VP5_REG_BASE + i),
                vdi_fio_read_register(coreIdx, (VP5_REG_BASE + i)),
                vdi_fio_read_register(coreIdx, (VP5_REG_BASE + i + 4 )),
                vdi_fio_read_register(coreIdx, (VP5_REG_BASE + i + 8 )),
                vdi_fio_read_register(coreIdx, (VP5_REG_BASE + i + 12)));
        }
        VLOG(ERR,"[-] VCPU ENT ENC REG Dump\n");

        VLOG(ERR,"[+] VCPU HOST MEM Dump\n");
        for(i = 0x7000; i < 0x70fc; i += 16) {
            VLOG(INFO,"0x%04xh: 0x%08x 0x%08x 0x%08x 0x%08x\n", (VP5_REG_BASE + i),
                vdi_fio_read_register(coreIdx, (VP5_REG_BASE + i)),
                vdi_fio_read_register(coreIdx, (VP5_REG_BASE + i + 4 )),
                vdi_fio_read_register(coreIdx, (VP5_REG_BASE + i + 8 )),
                vdi_fio_read_register(coreIdx, (VP5_REG_BASE + i + 12)));
        }
        VLOG(ERR,"[-] VCPU SPP Dump\n");

        VLOG(ERR,"vce run flag = %d\n", VpuReadReg(coreIdx, 0x1E8));
        // --------- BIT register Dump 
        VLOG(ERR,"[+] BPU REG Dump\n");
        for (i=0;i < 30; i++)
        {
            Uint32 temp;
            temp = vdi_fio_read_register(coreIdx, (VP5_REG_BASE + 0x8000 + 0x18));
            VLOG(ERR,"BITPC = 0x%08x\n", temp);
            if ( temp == 0xffffffff)
                return;
        }

        // --------- BIT HEVC Status Dump 
        VLOG(ERR,"==================================\n");
        VLOG(ERR,"[-] BPU REG Dump\n");
        VLOG(ERR,"==================================\n");
        {
            Uint32 temp;
            temp = vdi_fio_read_register(coreIdx, (VP5_REG_BASE + 0x8000 + 0x1E8));
            VLOG(ERR,"DBG_INFO_1= 0x%08x\n", temp);
            temp = vdi_fio_read_register(coreIdx, (VP5_REG_BASE + 0x8000 + 0x1EC));
            VLOG(ERR,"DBG_INFO_2= 0x%08x\n", temp);
        }

        VLOG(ERR,"DBG_FIFO_VALID		[%08x]\n",vdi_fio_read_register(coreIdx, (VP5_REG_BASE + 0x8000 + 0x6C)));

        //SDMA debug information
        vdi_fio_write_register(coreIdx,(VP5_REG_BASE + 0x8000 + 0x74),(1<<20) | (1<<16)| 0x13c);
        while((vdi_fio_read_register(coreIdx,(VP5_REG_BASE + 0x8000 + 0x7c))& 0x1) ==0);
        reg_val = vdi_fio_read_register(coreIdx,(VP5_REG_BASE + 0x8000 + 0x78));
        VLOG(ERR,"SDMA_DBG_INFO		[%08x]\n", reg_val);
        VLOG(ERR,"\t- [   28] need_more_update  : 0x%x \n", (reg_val>>28)&0x1 );
        VLOG(ERR,"\t- [27:25] tr_init_fsm       : 0x%x \n", (reg_val>>25)&0x7 );
        VLOG(ERR,"\t- [24:18] remain_trans_size : 0x%x \n", (reg_val>>18)&0x7F);
        VLOG(ERR,"\t- [17:13] wdata_out_cnt     : 0x%x \n", (reg_val>>13)&0x1F);
        VLOG(ERR,"\t- [12:10] wdma_wd_fsm       : 0x%x \n", (reg_val>>10)&0x1F);
        VLOG(ERR,"\t- [ 9: 7] wdma_wa_fsm       : 0x%x ", (reg_val>> 7)&0x7 );
        if (((reg_val>>7) &0x7) == 3){
            VLOG(ERR,"-->WDMA_WAIT_ADDR  \n");
        } else {
            VLOG(ERR,"\n");
        }
        VLOG(ERR,"\t- [ 6: 5] sdma_init_fsm     : 0x%x \n", (reg_val>> 5)&0x3 );
        VLOG(ERR,"\t- [ 4: 1] save_fsm          : 0x%x \n", (reg_val>> 1)&0xF );
        VLOG(ERR,"\t- [    0] unalign_written   : 0x%x \n", (reg_val>> 0)&0x1 );

        vdi_fio_write_register(coreIdx,(VP5_REG_BASE + 0x8000 + 0x74),(1<<20)| (1<<16)| 0x13b);
        while((vdi_fio_read_register(coreIdx,(VP5_REG_BASE + 0x8000 + 0x7c))& 0x1) ==0);
        reg_val = vdi_fio_read_register(coreIdx,(VP5_REG_BASE + 0x8000 + 0x78));
        VLOG(ERR,"SDMA_NAL_MEM_INF	[%08x]\n", reg_val);
        VLOG(ERR,"\t- [ 7: 1] nal_mem_empty_room : 0x%x \n", (reg_val>> 1)&0x3F);
        VLOG(ERR,"\t- [    0] ge_wnbuf_size      : 0x%x \n", (reg_val>> 0)&0x1 );

        vdi_fio_write_register(coreIdx,(VP5_REG_BASE + 0x8000 + 0x74),(1<<20)| (1<<16) | 0x131);
        while((vdi_fio_read_register(coreIdx,(VP5_REG_BASE + 0x8000 + 0x7c))& 0x1) ==0);
        reg_val = vdi_fio_read_register(coreIdx,(VP5_REG_BASE + 0x8000 + 0x78));
        VLOG(ERR,"SDMA_IRQ		[%08x]: [1]sdma_irq : 0x%x, [2]enable_sdma_irq : 0x%x\n",reg_val, (reg_val >> 1)&0x1,(reg_val &0x1));

        vdi_fio_write_register(coreIdx,(VP5_REG_BASE + 0x8000 + 0x74),(1<<20)| (1<<16) | 0x134);
        while((vdi_fio_read_register(coreIdx,(VP5_REG_BASE + 0x8000 + 0x7c))& 0x1) ==0);
        VLOG(ERR,"SDMA_BS_BASE_ADDR [%08x]\n",vdi_fio_read_register(coreIdx,(VP5_REG_BASE + 0x8000 + 0x78)));

        vdi_fio_write_register(coreIdx,(VP5_REG_BASE + 0x8000 + 0x74),(1<<20)| (1<<16) | 0x135);
        while((vdi_fio_read_register(coreIdx,(VP5_REG_BASE + 0x8000 + 0x7c))& 0x1) ==0);
        VLOG(ERR,"SDMA_NAL_STR_ADDR [%08x]\n",vdi_fio_read_register(coreIdx,(VP5_REG_BASE + 0x8000 + 0x78)));

        vdi_fio_write_register(coreIdx,(VP5_REG_BASE + 0x8000 + 0x74),(1<<20)| (1<<16) | 0x136);
        while((vdi_fio_read_register(coreIdx,(VP5_REG_BASE + 0x8000 + 0x7c))& 0x1) ==0 );
        VLOG(ERR,"SDMA_IRQ_ADDR     [%08x]\n",vdi_fio_read_register(coreIdx,(VP5_REG_BASE + 0x8000 + 0x78)));

        vdi_fio_write_register(coreIdx,(VP5_REG_BASE + 0x8000 + 0x74),(1<<20)| (1<<16) | 0x137);
        while((vdi_fio_read_register(coreIdx,(VP5_REG_BASE + 0x8000 + 0x7c))& 0x1) ==0 );
        VLOG(ERR,"SDMA_BS_END_ADDR	[%08x]\n",vdi_fio_read_register(coreIdx,(VP5_REG_BASE + 0x8000 + 0x78)));

        vdi_fio_write_register(coreIdx,(VP5_REG_BASE + 0x8000 + 0x74),(1<<20)| (1<<16) | 0x13A);
        while((vdi_fio_read_register(coreIdx,(VP5_REG_BASE + 0x8000 + 0x7c))& 0x1) ==0 );
        VLOG(ERR,"SDMA_CUR_ADDR		[%08x]\n",vdi_fio_read_register(coreIdx,(VP5_REG_BASE + 0x8000 + 0x78)));

        vdi_fio_write_register(coreIdx,(VP5_REG_BASE + 0x8000 + 0x74),(1<<20)| (1<<16) | 0x139);
        while((vdi_fio_read_register(coreIdx,(VP5_REG_BASE + 0x8000 + 0x7c))& 0x1) ==0 );
        reg_val = vdi_fio_read_register(coreIdx,(VP5_REG_BASE + 0x8000 + 0x78));
        VLOG(ERR,"SDMA_STATUS			[%08x]\n",reg_val);
        VLOG(ERR,"\t- [2] all_wresp_done : 0x%x \n", (reg_val>> 2)&0x1);
        VLOG(ERR,"\t- [1] sdma_init_busy : 0x%x \n", (reg_val>> 1)&0x1 );
        VLOG(ERR,"\t- [0] save_busy      : 0x%x \n", (reg_val>> 0)&0x1 );

        vdi_fio_write_register(coreIdx,(VP5_REG_BASE + 0x8000 + 0x74),(1<<20)| (1<<16) | 0x164);
        while((vdi_fio_read_register(coreIdx,(VP5_REG_BASE + 0x8000 + 0x7c))& 0x1) ==0 );
        reg_val = vdi_fio_read_register(coreIdx,(VP5_REG_BASE + 0x8000 + 0x78));
        VLOG(ERR,"SHU_DBG				[%08x] : shu_unaligned_num (0x%x)\n",reg_val, reg_val);

        vdi_fio_write_register(coreIdx,(VP5_REG_BASE + 0x8000 + 0x74),(1<<20)| (1<<16) | 0x169);
        while((vdi_fio_read_register(coreIdx,(VP5_REG_BASE + 0x8000 + 0x7c))& 0x1) ==0 );
        VLOG(ERR,"SHU_NBUF_WPTR		[%08x]\n",vdi_fio_read_register(coreIdx,(VP5_REG_BASE + 0x8000 + 0x78)));

        vdi_fio_write_register(coreIdx,(VP5_REG_BASE + 0x8000 + 0x74),(1<<20)| (1<<16) | 0x16A);
        while((vdi_fio_read_register(coreIdx,(VP5_REG_BASE + 0x8000 + 0x7c))& 0x1) ==0 );
        VLOG(ERR,"SHU_NBUF_RPTR		[%08x]\n",vdi_fio_read_register(coreIdx,(VP5_REG_BASE + 0x8000 + 0x78)));

        vdi_fio_write_register(coreIdx,(VP5_REG_BASE + 0x8000 + 0x74),(1<<20)| (1<<16) | 0x16C);
        while((vdi_fio_read_register(coreIdx,(VP5_REG_BASE + 0x8000 + 0x7c))& 0x1) ==0 );
        reg_val = vdi_fio_read_register(coreIdx,(VP5_REG_BASE + 0x8000 + 0x78));
        VLOG(ERR,"SHU_NBUF_INFO		[%08x]\n",reg_val);
        VLOG(ERR,"\t- [5:1] nbuf_remain_byte : 0x%x \n", (reg_val>> 1)&0x1F);
        VLOG(ERR,"\t- [  0] nbuf_wptr_wrap   : 0x%x \n", (reg_val>> 0)&0x1 );

        vdi_fio_write_register(coreIdx,(VP5_REG_BASE + 0x8000 + 0x74),(1<<20)| (1<<16) | 0x184);
        while((vdi_fio_read_register(coreIdx,(VP5_REG_BASE + 0x8000 + 0x7c))& 0x1) ==0 );
        VLOG(ERR,"CTU_LAST_ENC_POS	[%08x]\n",vdi_fio_read_register(coreIdx,(VP5_REG_BASE + 0x8000 + 0x78)));

        vdi_fio_write_register(coreIdx,(VP5_REG_BASE + 0x8000 + 0x74),(1<<20)| (1<<16) | 0x187);
        while((vdi_fio_read_register(coreIdx,(VP5_REG_BASE + 0x8000 + 0x7c))& 0x1) ==0 );
        VLOG(ERR,"CTU_POS_IN_PIC		[%08x]\n",vdi_fio_read_register(coreIdx,(VP5_REG_BASE + 0x8000 + 0x78)));

        vdi_fio_write_register(coreIdx,(VP5_REG_BASE + 0x8000 + 0x74),(1<<20)| (1<<16) | 0x110);
        while((vdi_fio_read_register(coreIdx,(VP5_REG_BASE + 0x8000 + 0x7c))& 0x1) ==0 );
        VLOG(ERR,"MIB_EXTADDR			[%08x]\n",vdi_fio_read_register(coreIdx,(VP5_REG_BASE + 0x8000 + 0x78)));

        vdi_fio_write_register(coreIdx,(VP5_REG_BASE + 0x8000 + 0x74),(1<<20)| (1<<16) | 0x111);
        while((vdi_fio_read_register(coreIdx,(VP5_REG_BASE + 0x8000 + 0x7c))& 0x1) ==0 );
        VLOG(ERR,"MIB_INTADDR			[%08x]\n",vdi_fio_read_register(coreIdx,(VP5_REG_BASE + 0x8000 + 0x78)));

        vdi_fio_write_register(coreIdx,(VP5_REG_BASE + 0x8000 + 0x74),(1<<20)| (1<<16) | 0x113);
        while((vdi_fio_read_register(coreIdx,(VP5_REG_BASE + 0x8000 + 0x7c))& 0x1) ==0 );
        VLOG(ERR,"MIB_CMD				[%08x]\n",vdi_fio_read_register(coreIdx,(VP5_REG_BASE + 0x8000 + 0x78)));

        vdi_fio_write_register(coreIdx,(VP5_REG_BASE + 0x8000 + 0x74),(1<<20)| (1<<16) | 0x114);
        while((vdi_fio_read_register(coreIdx,(VP5_REG_BASE + 0x8000 + 0x7c))& 0x1) ==0 );
        VLOG(ERR,"MIB_BUSY			[%08x]\n",vdi_fio_read_register(coreIdx,(VP5_REG_BASE + 0x8000 + 0x78)));

        VLOG(ERR,"DBG_BPU_ENC_NB0		[%08x]\n",vdi_fio_read_register(coreIdx,(VP5_REG_BASE + 0x8000 + 0x40)));
        VLOG(ERR,"DBG_BPU_CTU_CTRL0	[%08x]\n",vdi_fio_read_register(coreIdx,(VP5_REG_BASE + 0x8000 + 0x44)));
        VLOG(ERR,"DBG_BPU_CAB_FSM0	[%08x]\n",vdi_fio_read_register(coreIdx,(VP5_REG_BASE + 0x8000 + 0x48)));
        VLOG(ERR,"DBG_BPU_BIN_GEN0	[%08x]\n",vdi_fio_read_register(coreIdx,(VP5_REG_BASE + 0x8000 + 0x4C)));
        VLOG(ERR,"DBG_BPU_CAB_MBAE0	[%08x]\n",vdi_fio_read_register(coreIdx,(VP5_REG_BASE + 0x8000 + 0x50)));
        VLOG(ERR,"DBG_BPU_BUS_BUSY	[%08x]\n",vdi_fio_read_register(coreIdx,(VP5_REG_BASE + 0x8000 + 0x68)));
        VLOG(ERR,"DBG_FIFO_VALID		[%08x]\n",vdi_fio_read_register(coreIdx,(VP5_REG_BASE + 0x8000 + 0x6C)));
        VLOG(ERR,"DBG_BPU_CTU_CTRL1	[%08x]\n",vdi_fio_read_register(coreIdx,(VP5_REG_BASE + 0x8000 + 0x54)));
        VLOG(ERR,"DBG_BPU_CTU_CTRL2	[%08x]\n",vdi_fio_read_register(coreIdx,(VP5_REG_BASE + 0x8000 + 0x58)));
        VLOG(ERR,"DBG_BPU_CTU_CTRL3	[%08x]\n",vdi_fio_read_register(coreIdx,(VP5_REG_BASE + 0x8000 + 0x5C)));

        for (i=0x80; i<0xA0; i+=4) {
            reg_val = vdi_fio_read_register(coreIdx,(VP5_REG_BASE + 0x8000 + i));
            num     = (i - 0x80)/2;
            VLOG(ERR,"DBG_BIT_STACK		[%08x] : Stack%02d (0x%04x), Stack%02d(0x%04x) \n",reg_val, num, reg_val>>16, num+1, reg_val & 0xffff);
        }

        reg_val = vdi_fio_read_register(coreIdx,(VP5_REG_BASE + 0x8000 + 0xA0));
        VLOG(ERR,"DGB_BIT_CORE_INFO	[%08x] : pc_ctrl_id (0x%04x), pfu_reg_pc(0x%04x)\n",reg_val,reg_val>>16, reg_val & 0xffff);
        reg_val = vdi_fio_read_register(coreIdx,(VP5_REG_BASE + 0x8000 + 0xA4));
        VLOG(ERR,"DGB_BIT_CORE_INFO	[%08x] : ACC0 (0x%08x)\n",reg_val, reg_val);
        reg_val = vdi_fio_read_register(coreIdx,(VP5_REG_BASE + 0x8000 + 0xA8));
        VLOG(ERR,"DGB_BIT_CORE_INFO	[%08x] : ACC1 (0x%08x)\n",reg_val, reg_val);

        reg_val = vdi_fio_read_register(coreIdx,(VP5_REG_BASE + 0x8000 + 0xAC));
        VLOG(ERR,"DGB_BIT_CORE_INFO	[%08x] : pfu_ibuff_id(0x%04x), pfu_ibuff_op(0x%04x)\n",reg_val,reg_val>>16, reg_val & 0xffff);

        for (num=0; num<5; num+=1) {
            reg_val = vdi_fio_read_register(coreIdx,(VP5_REG_BASE + 0x8000 + 0xB0));
            VLOG(ERR,"DGB_BIT_CORE_INFO	[%08x] : core_pram_rd_en(0x%04x), core_pram_rd_addr(0x%04x)\n",reg_val,reg_val>>16, reg_val & 0xffff);
        }

        VLOG(ERR,"SAO_LUMA_OFFSET	[%08x]\n",vdi_fio_read_register(coreIdx,(VP5_REG_BASE + 0x8000 + 0xB4)));
        VLOG(ERR,"SAO_CB_OFFSET	[%08x]\n",vdi_fio_read_register(coreIdx,(VP5_REG_BASE + 0x8000 + 0xB8)));
        VLOG(ERR,"SAO_CR_OFFSET	[%08x]\n",vdi_fio_read_register(coreIdx,(VP5_REG_BASE + 0x8000 + 0xBC)));

        VLOG(ERR,"GDI_NO_MORE_REQ		[%08x]\n",vdi_fio_read_register(coreIdx,(VP5_REG_BASE + 0x8000 + 0x8f0)));
        VLOG(ERR,"GDI_EMPTY_FLAG		[%08x]\n",vdi_fio_read_register(coreIdx,(VP5_REG_BASE + 0x8000 + 0x8f4)));

        VLOG(ERR,"VP5_CODE VCE DUMP\n");

        vce_enc_debug[0] = 0x0ba0;//MODE SEL //parameter VCE_ENC_DEBUG0            = 9'h1A0; 
        vce_enc_debug[1] = 0x0ba4;
        vce_enc_debug[2] = 0x0ba8;
        vce_enc_debug[3] = 0x0bac;
        vce_enc_debug[4] = 0x0bb0;
        vce_enc_debug[5] = 0x0bb4;
        vce_enc_debug[6] = 0x0bb8;
        vce_enc_debug[7] = 0x0bbc;
        vce_enc_debug[8] = 0x0bc0;
        vce_enc_debug[9] = 0x0bc4;
        set_mode         = 0x0ba0;
        vcore_num        = 1;

        for (vcore_idx = 0; vcore_idx < vcore_num ; vcore_idx++) {
            VLOG(ERR,"==========================================\n");
            VLOG(ERR,"[+] VCE REG Dump VCORE_IDX : %d\n",vcore_idx);
            VLOG(ERR,"==========================================\n");
            DisplayVceEncReadVCE             (coreIdx, vcore_idx);
            DisplayVceEncDebugCommon521      (coreIdx, vcore_idx, set_mode, vce_enc_debug[0], vce_enc_debug[1], vce_enc_debug[2]);
            DisplayVceEncDebugMode2          (coreIdx, vcore_idx, set_mode, vce_enc_debug);
        }
    }
    SetClockGate(coreIdx, 0);
}


void HandleEncoderError(
    EncHandle       handle,
    Uint32          encPicCnt,
    EncOutputInfo*  outputInfo
    )
{
    UNREFERENCED_PARAMETER(handle);
    VLOG(ERR,"HandleEncoderError handle %p  encPicCnt %d Output %p\n",
        handle, encPicCnt, outputInfo);
}

void DumpCodeBuffer(
    const char* path
    )
{
    Uint8*          buffer;
    vpu_buffer_t    vb;
    PhysicalAddress addr;
    osal_file_t     ofp;

    VLOG(ERR,"DUMP CODE AREA into %s ", path);

    buffer = (Uint8*)osal_malloc(1024*1024);
    if ((ofp=osal_fopen(path, "wb")) == NULL) {
        VLOG(ERR,"[FAIL]\n");
    } 
    else {
        vdi_get_common_memory(0, &vb);

        addr   = vb.phys_addr;
        VpuReadMem(0, addr, buffer, VP5_MAX_CODE_BUF_SIZE, VDI_128BIT_LITTLE_ENDIAN);
        osal_fwrite(buffer, 1, VP5_MAX_CODE_BUF_SIZE, ofp);
        osal_fclose(ofp);
        VLOG(ERR,"[OK]\n");
    }
    osal_free(buffer);
}

void PrintMemoryAccessViolationReason(
    Uint32          coreIdx, 
    void            *outp
    )
{
    VLOG(ERR, "PrintMemoryAccessViolationReaso coreIdx %d, outp %p\n",
        coreIdx, outp);
}

void vdi_make_log(unsigned long coreIdx, const char *str, int step)
{
    Uint32 val;

    val = VpuReadReg(coreIdx, VP5_CMD_INSTANCE_INFO);
    val &= 0xffff;
    if (step == 1) {
        VLOG(INFO, "\n**%s start(%d)\n", str, val);
    } else if (step == 2) {
        VLOG(INFO, "\n**%s timeout(%d)\n", str, val);
    } else {
        VLOG(INFO, "\n**%s end(%d)\n", str, val);
    }
}

void vdi_log(unsigned long coreIdx, int cmd, int step)
{
    int i;
    int productId;

    if (coreIdx >= MAX_NUM_VPU_CORE)
        return ;

    productId = VPU_GetProductId(coreIdx);

    if (PRODUCT_ID_VP_SERIES(productId))
    {
        switch(cmd)
        {
        case VP5_INIT_VPU:
            vdi_make_log(coreIdx, "INIT_VPU", step);
            break;
        case VP5_ENC_SET_PARAM:
            vdi_make_log(coreIdx, "ENC_SET_PARAM", step);
            break;
        case VP5_INIT_SEQ:
            vdi_make_log(coreIdx, "DEC INIT_SEQ", step);
            break;
        case VP5_DESTROY_INSTANCE:
            vdi_make_log(coreIdx, "DESTROY_INSTANCE", step);
            break;
        case VP5_DEC_PIC://ENC_PIC for ENC
            vdi_make_log(coreIdx, "DEC_PIC(ENC_PIC)", step);
            break;
        case VP5_SET_FB:
            vdi_make_log(coreIdx, "SET_FRAMEBUF", step);
            break;
        case VP5_FLUSH_INSTANCE:
            vdi_make_log(coreIdx, "FLUSH INSTANCE", step);
            break;
        case VP5_QUERY:
            vdi_make_log(coreIdx, "QUERY", step);
            break;
        case VP5_SLEEP_VPU:
            vdi_make_log(coreIdx, "SLEEP_VPU", step);
            break;
        case VP5_WAKEUP_VPU:
            vdi_make_log(coreIdx, "WAKEUP_VPU", step);
            break;
        case VP5_UPDATE_BS:
            vdi_make_log(coreIdx, "UPDATE_BS", step);
            break;           
        case VP5_CREATE_INSTANCE:
            vdi_make_log(coreIdx, "CREATE_INSTANCE", step);
            break;
        default:
            vdi_make_log(coreIdx, "ANY_CMD", step);
            break;
        }
    }
    else {
        VLOG(ERR, "Unknown product id : %08x\n", productId);
        return;
    }

    for (i=0x0; i<0x200; i=i+16) { // host IF register 0x100 ~ 0x200
        VLOG(INFO, "0x%04xh: 0x%08x 0x%08x 0x%08x 0x%08x\n", i,
            vdi_read_register(coreIdx, i), vdi_read_register(coreIdx, i+4),
            vdi_read_register(coreIdx, i+8), vdi_read_register(coreIdx, i+0xc));
    }

    if (PRODUCT_ID_VP_SERIES(productId))
    {
        if (cmd == VP5_INIT_VPU || cmd == VP5_RESET_VPU || cmd == VP5_CREATE_INSTANCE)
        {
            vdi_print_vpu_status(coreIdx);
        }
    }
    else {
        VLOG(ERR, "Unknown product id : %08x\n", productId);
        return;
    }
}

static void vp5xx_vcore_status(
    Uint32 coreIdx
    )
{
    Uint32 i;

    VLOG(INFO,"[+] BPU REG Dump\n");
    for(i = 0x8000; i < 0x80FC; i += 16) {
        VLOG(INFO,"0x%04xh: 0x%08x 0x%08x 0x%08x 0x%08x\n", (VP5_REG_BASE + i), 
            vdi_fio_read_register(coreIdx, (VP5_REG_BASE + i)),
            vdi_fio_read_register(coreIdx, (VP5_REG_BASE + i + 4 )),
            vdi_fio_read_register(coreIdx, (VP5_REG_BASE + i + 8 )),
            vdi_fio_read_register(coreIdx, (VP5_REG_BASE + i + 12)));
    }
    VLOG(INFO,"[-] BPU REG Dump\n");

    // --------- VCE register Dump 
    VLOG(INFO,"[+] VCE REG Dump\n");
    for (i=0x000; i<0x1fc; i+=16) {
        VLOG(INFO,"0x%04xh: 0x%08x 0x%08x 0x%08x 0x%08x\n", i, 
            ReadRegVCE(coreIdx, 0, (i+0x00)),
            ReadRegVCE(coreIdx, 0, (i+0x04)),
            ReadRegVCE(coreIdx, 0, (i+0x08)),
            ReadRegVCE(coreIdx, 0, (i+0x0c)));
    }
    VLOG(INFO,"[-] VCE REG Dump\n");
}

void vdi_print_vpu_status(unsigned long coreIdx)
{
    Uint32 productCode;

    productCode = vdi_read_register(coreIdx, VPU_PRODUCT_CODE_REGISTER);

    if (PRODUCT_CODE_VP(productCode))
    {
        Uint32 vcpu_reg[31]= {0,};
        Uint32 i;

        VLOG(INFO,"-------------------------------------------------------------------------------\n");
        VLOG(INFO,"------                            VCPU STATUS                             -----\n");
        VLOG(INFO,"-------------------------------------------------------------------------------\n");

        // --------- VCPU register Dump 
        VLOG(INFO,"[+] VCPU REG Dump\n");
        for (i = 0; i < 25; i++) {
            VpuWriteReg (coreIdx, 0x14, (1<<9) | (i & 0xff));
            vcpu_reg[i] = VpuReadReg (coreIdx, 0x1c);

            if (i < 16) {
                VLOG(INFO,"0x%08x\t",  vcpu_reg[i]);
                if ((i % 4) == 3) VLOG(INFO,"\n");
            }
            else {
                switch (i) {
                case 16: VLOG(INFO,"CR0: 0x%08x\t", vcpu_reg[i]); break;
                case 17: VLOG(INFO,"CR1: 0x%08x\n", vcpu_reg[i]); break;
                case 18: VLOG(INFO,"ML:  0x%08x\t", vcpu_reg[i]); break;
                case 19: VLOG(INFO,"MH:  0x%08x\n", vcpu_reg[i]); break;
                case 21: VLOG(INFO,"LR:  0x%08x\n", vcpu_reg[i]); break;
                case 22: VLOG(INFO,"PC:  0x%08x\n", vcpu_reg[i]); break;
                case 23: VLOG(INFO,"SR:  0x%08x\n", vcpu_reg[i]); break;
                case 24: VLOG(INFO,"SSP: 0x%08x\n", vcpu_reg[i]); break;
                default: break;
                }
            }
        }
        VLOG(INFO,"[-] VCPU REG Dump\n");
        /// -- VCPU ENTROPY PERI DECODE Common 

        VLOG(INFO,"[+] VCPU ENT DEC REG Dump\n");
        for(i = 0x6000; i < 0x6800; i += 16) {
            VLOG(INFO,"0x%04xh: 0x%08x 0x%08x 0x%08x 0x%08x\n", (VP5_REG_BASE + i), 
                vdi_fio_read_register(coreIdx, (VP5_REG_BASE + i)),
                vdi_fio_read_register(coreIdx, (VP5_REG_BASE + i + 4 )),
                vdi_fio_read_register(coreIdx, (VP5_REG_BASE + i + 8 )),
                vdi_fio_read_register(coreIdx, (VP5_REG_BASE + i + 12)));
        }
        VLOG(INFO,"[-] VCPU ENT DEC REG Dump\n");
        vp5xx_vcore_status(coreIdx);
        VLOG(INFO,"-------------------------------------------------------------------------------\n");
    }
    else {
        VLOG(ERR, "Unknown product id : %08x\n", productCode);
    }
}

