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
#ifndef APBFILTER_H
#define APBFILTER_H
#include "base_type.h"
#include "vsi_string.h"
#include "osal.h"

struct ApbFilterHwCfg {
    /* the max pages supported by HW*/
    u8 support_page_max;
    /* separate r/w protest support*/
    u8 rw_protest;
    /* page_sel register offset to mask register*/
    u32 page_sel_offset;
};

struct ApbFilterMask {
    /* masked registers idx */
    u32 reg_idx;
    /* mask value of each register in security mode. 0:accessible 1:denial*/
    u32 value;
    /* page idx*/
    u8 page_idx;
};

/*the APB filter config*/
struct ApbFilterCfg {
    //u32 security_mode; /* 0: non-security mode 1:security mode*/
    u32 num_regs;                   /*number of registers to config for protection*/
    struct ApbFilterMask *reg_mask; /*pointer to an array of mask registers*/
};

typedef struct {
    u32 value;
    u32 offset;
    u32 flag;
} ApbFilterREG;

void ReadApbFilterHwCfg(struct ApbFilterHwCfg *apb_filter_hw_cfg);
i32 ConfigApbFilter(ApbFilterREG *regs, struct ApbFilterCfg *cfg);
i32 SelApbFilterPage(ApbFilterREG *reg, u32 page_sel, u32 page_sel_offset);

#endif
