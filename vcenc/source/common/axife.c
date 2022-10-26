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
#ifdef SUPPORT_AXIFE

#include "axife.h"

#define AXIFE_RD 0
#define AXIFE_WR 1

static const u32 reg_mask[33] = {
    0x00000000, 0x00000001, 0x00000003, 0x00000007, 0x0000000F, 0x0000001F, 0x0000003F,
    0x0000007F, 0x000000FF, 0x000001FF, 0x000003FF, 0x000007FF, 0x00000FFF, 0x00001FFF,
    0x00003FFF, 0x00007FFF, 0x0000FFFF, 0x0001FFFF, 0x0003FFFF, 0x0007FFFF, 0x000FFFFF,
    0x001FFFFF, 0x003FFFFF, 0x007FFFFF, 0x00FFFFFF, 0x01FFFFFF, 0x03FFFFFF, 0x07FFFFFF,
    0x0FFFFFFF, 0x1FFFFFFF, 0x3FFFFFFF, 0x7FFFFFFF, 0xFFFFFFFF};

static const u32 axife_reg_spec[AXI_LAST_REG][4] = {
#include "axife_register.h"
};

#define AXI_CHAN_REG_NUM 16
static void AxiFeSetRegister(REG *reg_base, u32 id, u32 value)
{
    u32 tmp;
    if (axife_reg_spec[id][0] >= AXI_CHAN_REG_NUM) {
        printf("channel registers not use this function\n");
        return;
    }
    tmp = reg_base[axife_reg_spec[id][0]].value;
    tmp &= ~(reg_mask[axife_reg_spec[id][1]] << (axife_reg_spec[id][2]));
    tmp |= (value & reg_mask[axife_reg_spec[id][1]]) << (axife_reg_spec[id][2]);
    reg_base[axife_reg_spec[id][0]].value = tmp;
    reg_base[axife_reg_spec[id][0]].offset = axife_reg_spec[id][0] * 4;
    reg_base[axife_reg_spec[id][0]].flag = 1;
}

static u32 AxiFeGetRegister(REG *reg_base, u32 id)
{
    u32 tmp;
    if (axife_reg_spec[id][0] >= AXI_CHAN_REG_NUM) {
        printf("channel registers not use this function\n");
        return 0;
    }
    tmp = reg_base[axife_reg_spec[id][0]].value;
    tmp = tmp >> (axife_reg_spec[id][2]);
    tmp &= reg_mask[axife_reg_spec[id][1]];
    return tmp;
}

static void AxiSetChns(REG *reg_base, u32 id, u32 dir, u32 mode, struct AxiFeHwCfg *fe_hw_cfg,
                       struct ChnDesc *chan_dec)
{
    u32 offset = 0;
    /* as the spec, missing read/write channels when AXI_CHN_NUMW is different AXI_CHN_NUMR, space is saved.*/
    if (dir == AXIFE_RD) {
        if (id < fe_hw_cfg->axi_wr_chn_num)
            offset = id * 8;
        else
            offset = fe_hw_cfg->axi_wr_chn_num * 8 + (id - fe_hw_cfg->axi_wr_chn_num) * 4;
    } else {
        if (id < fe_hw_cfg->axi_rd_chn_num)
            offset = id * 8 + 4;
        else
            offset = fe_hw_cfg->axi_rd_chn_num * 8 + (id - fe_hw_cfg->axi_rd_chn_num) * 4;
    }
    /* configure the channel parameters*/
    reg_base[offset].value = chan_dec->sw_axi_base_addr_id;
    reg_base[offset].offset = offset * 4;
    reg_base[offset].flag = 1;
    /*only address mode need to configure start/end address*/
    if (mode == AXIFE_MODE_ADDRESS) {
        reg_base[offset + 1].value = chan_dec->sw_axi_start_addr << 4;
        reg_base[offset + 1].offset = (offset + 1) * 4;
        reg_base[offset + 1].flag = 1;
        reg_base[offset + 2].value = chan_dec->sw_axi_end_addr << 4;
        reg_base[offset + 2].offset = (offset + 2) * 4;
        reg_base[offset + 2].flag = 1;
    }
    reg_base[offset + 3].value |= chan_dec->sw_axi_user;
    reg_base[offset + 3].value |= (chan_dec->sw_axi_ns << 8);
    reg_base[offset + 3].value |= (chan_dec->sw_axi_qos << 9);
    reg_base[offset + 3].offset = (offset + 3) * 4;
    reg_base[offset + 3].flag = 1;
    return;
}

void ReadAxiFeHwCfg(u32 axiFeFuse, struct AxiFeHwCfg *fe_hw_cfg)
{
    fe_hw_cfg->axi_rd_chn_num = axiFeFuse & 0x7F;
    fe_hw_cfg->axi_wr_chn_num = (axiFeFuse >> 7) & 0x7F;
    fe_hw_cfg->axi_rd_burst_length = (axiFeFuse >> 14) & 0x1F;
    fe_hw_cfg->axi_wr_burst_length = (axiFeFuse >> 22) & 0x1F;
    fe_hw_cfg->fe_mode = 0; //addr mode

    return;
}

void ConfigAxiFe(REG *axife_shadow_regs, struct AxiFeCommonCfg *fe_common_cfg)
{
    AxiFeSetRegister(axife_shadow_regs, AXI_SW_SECURE_MODE, fe_common_cfg->sw_secure_mode);
    AxiFeSetRegister(axife_shadow_regs, AXI_SW_AXI_USER_MODE, fe_common_cfg->sw_axi_user_mode);
    AxiFeSetRegister(axife_shadow_regs, AXI_SW_AXI_ADDR_MODE, fe_common_cfg->sw_axi_addr_mode);
    AxiFeSetRegister(axife_shadow_regs, AXI_SW_AXI_PROT_MODE, fe_common_cfg->sw_axi_prot_mode);
    AxiFeSetRegister(axife_shadow_regs, AXI_SW_WORK_MODE, fe_common_cfg->sw_work_mode);
    AxiFeSetRegister(axife_shadow_regs, AXI_SW_SINGLE_ID_EN, fe_common_cfg->sw_single_is_enable);
    AxiFeSetRegister(axife_shadow_regs, AXI_SW_RESET_REG_EN, fe_common_cfg->sw_reset_reg_enable);
    AxiFeSetRegister(axife_shadow_regs, AXI_SW_AXI_RD_ID, fe_common_cfg->sw_axi_rd_id);
    AxiFeSetRegister(axife_shadow_regs, AXI_SW_AXI_WR_ID, fe_common_cfg->sw_axi_wr_id);
}

void ConfigAxiFeChns(REG *axife_shadow_regs, u32 axiFeFuse, struct AxiFeChns *fe_chns)
{
    u32 i;
    u32 mode = 0;
    struct AxiFeHwCfg fe_hw_cfg;

    ReadAxiFeHwCfg(axiFeFuse, &fe_hw_cfg);
    if (fe_chns->nbr_rd_chns > fe_hw_cfg.axi_rd_chn_num ||
        fe_chns->nbr_wr_chns > fe_hw_cfg.axi_wr_chn_num) {
        printf("the configured channel number is not enough\n");
        return;
    }
    mode = fe_hw_cfg.fe_mode;
    for (i = 0; i < fe_chns->nbr_rd_chns; i++)
        AxiSetChns(axife_shadow_regs, i, AXIFE_RD, (mode ? AXIFE_MODE_ID : AXIFE_MODE_ADDRESS),
                   &fe_hw_cfg, &(fe_chns->rd_channels[i]));

    for (i = 0; i < fe_chns->nbr_wr_chns; i++)
        AxiSetChns(axife_shadow_regs, i, AXIFE_WR, (mode ? AXIFE_MODE_ID : AXIFE_MODE_ADDRESS),
                   &fe_hw_cfg, &(fe_chns->wr_channels[i]));

    AxiFeSetRegister(axife_shadow_regs, AXI_SW_AXI_REM_RUSER, fe_chns->sw_axi_ruser);
    AxiFeSetRegister(axife_shadow_regs, AXI_SW_AXI_REM_RNS, fe_chns->sw_axi_rns);
    AxiFeSetRegister(axife_shadow_regs, AXI_SW_AXI_REM_RQOS, fe_chns->sw_axi_rqos);

    AxiFeSetRegister(axife_shadow_regs, AXI_SW_AXI_REM_WUSER, fe_chns->sw_axi_wuser);
    AxiFeSetRegister(axife_shadow_regs, AXI_SW_AXI_REM_WNS, fe_chns->sw_axi_wns);
    AxiFeSetRegister(axife_shadow_regs, AXI_SW_AXI_REM_WQOS, fe_chns->sw_axi_wqos);
}

void EnableAxiFe(REG *axife_shadow_regs, u32 mode)
{
    if (mode) {
        AxiFeSetRegister(axife_shadow_regs, AXI_SW_WORK_MODE, 1);
        AxiFeSetRegister(axife_shadow_regs, AXI_SW_FRONTEND_EN, 1);
    } else {
        AxiFeSetRegister(axife_shadow_regs, AXI_SW_WORK_MODE, 0);
        AxiFeSetRegister(axife_shadow_regs, AXI_SW_FRONTEND_EN, 1);
    }
}

void ReadAxiFeStat(REG *axife_shadow_regs, struct AxiFeStat *fe_stat)
{
    fe_stat->sw_axi_r_len_cnt = AxiFeGetRegister(axife_shadow_regs, AXI_SW_AXI_R_LEN_CNT);
    fe_stat->sw_axi_r_dat_cnt = AxiFeGetRegister(axife_shadow_regs, AXI_SW_AXI_R_DAT_CNT);
    fe_stat->sw_axi_r_req_cnt = AxiFeGetRegister(axife_shadow_regs, AXI_SW_AXI_R_REQ_CNT);
    fe_stat->sw_axi_rlast_cnt = AxiFeGetRegister(axife_shadow_regs, AXI_SW_AXI_RLAST_CNT);
    fe_stat->sw_axi_w_len_cnt = AxiFeGetRegister(axife_shadow_regs, AXI_SW_AXI_W_LEN_CNT);
    fe_stat->sw_axi_w_dat_cnt = AxiFeGetRegister(axife_shadow_regs, AXI_SW_AXI_W_DAT_CNT);
    fe_stat->sw_axi_w_req_cnt = AxiFeGetRegister(axife_shadow_regs, AXI_SW_AXI_W_REG_CNT);
    fe_stat->sw_axi_wlast_cnt = AxiFeGetRegister(axife_shadow_regs, AXI_SW_AXI_WLAST_CNT);
    fe_stat->sw_axi_w_ack_cnt = AxiFeGetRegister(axife_shadow_regs, AXI_SW_AXI_W_ACK_CNT);
    return;
}

void DisableAxiFe(REG *axife_shadow_regs)
{
    AxiFeSetRegister(axife_shadow_regs, AXI_SW_FRONTEND_EN, 0);
}

void ResetAxiFe(REG *axife_shadow_regs)
{
    AxiFeSetRegister(axife_shadow_regs, AXI_SW_SOFT_RESET, 1);
}

#endif
