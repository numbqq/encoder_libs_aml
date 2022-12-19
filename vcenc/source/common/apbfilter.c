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
#include "apbfilter.h"

void ReadApbFilterHwCfg(struct ApbFilterHwCfg *apb_filter_hw_cfg)
{
    //should get from apb filter fuse in future
    apb_filter_hw_cfg->support_page_max = 16;
    apb_filter_hw_cfg->rw_protest = 0;
    apb_filter_hw_cfg->page_sel_offset = 4;

    return;
}

i32 ConfigApbFilter(ApbFilterREG *regs, struct ApbFilterCfg *cfg)
{
    u32 i;
    struct ApbFilterHwCfg apb_hw_cfg;
    struct ApbFilterMask *reg_mask;
    ApbFilterREG *reg;

    ReadApbFilterHwCfg(&apb_hw_cfg);

    for (i = 0; i < cfg->num_regs; i++) {
        reg_mask = &(cfg->reg_mask[i]);
        if (reg_mask->page_idx > apb_hw_cfg.support_page_max)
            return -1;

        reg = &(regs[reg_mask->reg_idx]);

        reg->value &= ~(1 << reg_mask->page_idx);
        reg->value |= reg_mask->value << reg_mask->page_idx;
        reg->offset = reg_mask->reg_idx * 4;
        reg->flag = 1;
    }

    return 0;
}

i32 SelApbFilterPage(ApbFilterREG *reg, u32 page_sel, u32 page_sel_offset)
{
    u32 i;
    struct ApbFilterHwCfg apb_hw_cfg;

    ReadApbFilterHwCfg(&apb_hw_cfg);

    if (page_sel > apb_hw_cfg.support_page_max)
        return -1;

    reg->value = page_sel;
    reg->offset = page_sel_offset;
    reg->flag = 1;

    return 0;
}
