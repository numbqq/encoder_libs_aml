/*------------------------------------------------------------------------------
--       Copyright (c) 2015-2017, VeriSilicon Inc. All rights reserved        --
--         Copyright (c) 2011-2014, Google Inc. All rights reserved.          --
--         Copyright (c) 2007-2010, Hantro OY. All rights reserved.           --
--                                                                            --
-- This software is confidential and proprietary and may be used only as      --
--   expressly authorized by VeriSilicon in a written licensing agreement.    --
--                                                                            --
--         This entire notice must be reproduced on all copies                --
--                       and may not be removed.                              --
--                                                                            --
--------------------------------------------------------------------------------
-- Redistribution and use in source and binary forms, with or without         --
-- modification, are permitted provided that the following conditions are met:--
--   * Redistributions of source code must retain the above copyright notice, --
--       this list of conditions and the following disclaimer.                --
--   * Redistributions in binary form must reproduce the above copyright      --
--       notice, this list of conditions and the following disclaimer in the  --
--       documentation and/or other materials provided with the distribution. --
--   * Neither the names of Google nor the names of its contributors may be   --
--       used to endorse or promote products derived from this software       --
--       without specific prior written permission.                           --
--------------------------------------------------------------------------------
-- THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"--
-- AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE  --
-- IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE --
-- ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE  --
-- LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR        --
-- CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF       --
-- SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS   --
-- INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN    --
-- CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)    --
-- ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE --
-- POSSIBILITY OF SUCH DAMAGE.                                                --
--------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
#ifndef AXIFE_H
#define AXIFE_H
#include "base_type.h"
#include "vsi_string.h"
#include "osal.h"

#define AXIFE_MODE_ADDRESS 0
#define AXIFE_MODE_ID 1
#define AXIFE_MAX_CHAN_NUM 64
#define AXIFE_REG_NUM (16 + AXIFE_MAX_CHAN_NUM * 8)

enum AxiRegName {
#include "axife_enum.h"
    AXI_LAST_REG
};

typedef struct {
    u32 value;
    u32 offset;
    u32 flag;
} REG;

typedef struct {
    REG registers[AXIFE_REG_NUM];
} AXIFE_REG_OUT;

/* AXI FE configure */
struct AxiFeHwCfg {
    /* Read channel number by hardware configured named AXI_CHN_NUMR */
    u8 axi_rd_chn_num;
    /* Write channel number by hardware configured named AXI_CHN_NUMW */
    u8 axi_wr_chn_num;
    /* Read BurstLength by hardware configured MASTER_BLR(unit:SYS_DATA_WIDTH,default:16bytes)*/
    u8 axi_rd_burst_length;
    /* Write BurstLength by hardware configured MASTER_BLW(unit:SYS_DATA_WIDTH,default:16bytes)*/
    u8 axi_wr_burst_length;
    /* work_mode(ID or addr) configured by hardware*/
    u8 fe_mode;
};

struct AxiFeCommonCfg {
    /* secure mode flag to set APORT_axim[1]*/
    u32 sw_secure_mode;
    u32 sw_axi_user_mode;
    u32 sw_axi_addr_mode;
    u8 sw_axi_prot_mode;
    u8 sw_work_mode;
    /* this control is only valid when workMode == "pass through" */
    u8 sw_single_is_enable;
    /* when hot reset, whether reset software registers start from reg11*/
    u8 sw_reset_reg_enable;
    /* base ID for master read bursts transaction*/
    u8 sw_axi_rd_id;
    /* base ID for master write bursts transcation */
    u8 sw_axi_wr_id;
};

/*Description of a read/write channel */
struct ChnDesc {
    /*Address mode:high 32-bit part of the start and end address
    ID mode: the AXI ID of the address segment*/
    u32 sw_axi_base_addr_id;
    u32 sw_axi_start_addr;
    u32 sw_axi_end_addr;
    u32 sw_axi_user; /*software programed AWUSER axim*/
    u32 sw_axi_ns;   /*software programmed AWPROT_axim*/
    u32 sw_axi_qos;  /*software programmed AWQOS_axim*/
};

/* All the channels configured by sw, adding a read/write configuration
   for channels that are not specified explicitly*/
struct AxiFeChns {
    u32 nbr_rd_chns; /*nbr of configured read channels */
    u32 nbr_wr_chns; /*nbr of configured write channels*/
    struct ChnDesc *rd_channels;
    struct ChnDesc *wr_channels;
    /* The remained address segments that are not specified by the channels
     address seqment groups above, for read and write respectively*/
    u8 sw_axi_wuser; /*software programmed AWUSER_axim*/
    u8 sw_axi_wns;   /*software programmed AWPROT_axim[1]*/
    u8 sw_axi_wqos;  /*software programmed AWQOS_axim*/
    /*read*/
    u8 sw_axi_ruser;
    u8 sw_axi_rns;
    u8 sw_axi_rqos;
};

/*Statistics from AXI FE*/
struct AxiFeStat {
    /*AXI read burst length accumulator for current frame (ARLEN+1)*/
    u32 sw_axi_r_len_cnt;
    /*AXI data read cycle accumulator for current frame*/
    u32 sw_axi_r_dat_cnt;
    /*AXI read requests accumulator for current frame*/
    u32 sw_axi_r_req_cnt;
    /* Accumulator for AXI read last data of each burst for current frame*/
    u32 sw_axi_rlast_cnt;
    /*AXI write burst length accumulator for current frame*/
    u32 sw_axi_w_len_cnt;
    /*AXI data write cycle accumulator for current frame*/
    u32 sw_axi_w_dat_cnt;
    /*AXI read requests accumulator for current frame*/
    u32 sw_axi_w_req_cnt;
    /*Accumulator for AXI write last data of each burst for current frame*/
    u32 sw_axi_wlast_cnt;
    /*AXI write responses accumulators for current frame*/
    u32 sw_axi_w_ack_cnt;
};

void ConfigAxiFeChns(REG *axife_shadow_regs, u32 axiFeFuse, struct AxiFeChns *fe_chns);
void ConfigAxiFe(REG *axife_shadow_regs, struct AxiFeCommonCfg *fe_common_cfg);
void EnableAxiFe(REG *axife_shadow_regs, u32 mode);
void DisableAxiFe(REG *axife_shadow_regs);

#endif
