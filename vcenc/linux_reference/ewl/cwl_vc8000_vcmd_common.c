/*------------------------------------------------------------------------------
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                                                                            --
--                            Verisilicon.                                    --
--                                                                            --
--                   (C) COPYRIGHT 2019 VERISILICON                           --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
------------------------------------------------------------------------------*/
#ifdef VCMD_BUILD_SUPPORT

#include "cwl_vc8000_vcmd_common.h"
void *EWLmemcpy(void *d, const void *s, u32 n);

void CWLCollectWriteRegData(u32 *src, u32 *dst, u16 reg_start, u32 reg_length, u32 *total_length)
{
    u32 i;
    u32 data_length = 0;

    {
        //opcode
        *dst++ = OPCODE_WREG | (reg_length << 16) | (reg_start * 4);
        data_length++;
        //data
        EWLmemcpy(dst, src, reg_length * sizeof(u32));
        data_length += reg_length;
        dst += reg_length;

        //alignment
        if (data_length % 2) {
            *dst = 0;
            data_length++;
        }
        *total_length = data_length;
    }
}

void CWLCollectStallData(u32 *dst, u32 *total_length, u32 interruput_mask)
{
    u32 data_length = 0;
    {
        //opcode
        *dst++ = OPCODE_STALL | (0 << 16) | (interruput_mask);
        data_length++;
        //alignment
        *dst = 0;
        data_length++;
        *total_length = data_length;
    }
}

void CWLCollectReadRegData(u32 *dst, u16 reg_start, u32 reg_length, u32 *total_length,
                           ptr_t status_data_base_addr)
{
    u32 data_length = 0;
    //status_data_base_addr += reg_start*4;
    {
        //opcode
        *dst++ = OPCODE_RREG | (reg_length << 16) | (reg_start * 4);
        data_length++;
        //data
        *dst++ = (u32)status_data_base_addr;
        data_length++;
        *dst++ = (u32)((u64)status_data_base_addr >> 32);
        data_length++;

        //alignment

        *dst = 0;
        data_length++;

        *total_length = data_length;
    }
}

void CWLCollectNopData(u32 *dst, u32 *total_length)
{
    u32 i;
    u32 data_length = 0;
    {
        //opcode
        *dst++ = OPCODE_NOP | (0);
        data_length++;
        //alignment
        *dst = 0;
        data_length++;
        *total_length = data_length;
    }
}

void CWLCollectIntData(u32 *dst, u16 int_vecter, u32 *total_length)
{
    u32 data_length = 0;
    {
        //opcode
        *dst++ = OPCODE_INT | (int_vecter);
        data_length++;
        //alignment
        *dst = 0;
        data_length++;
        *total_length = data_length;
    }
}

void CWLCollectJmpData(u32 *dst, u32 *total_length, u16 cmdbuf_id)
{
    u32 data_length = 0;
    {
        //opcode
        *dst++ = OPCODE_JMP_RDY0 | (0);
        data_length++;
        //addr
        *dst++ = (0);
        data_length++;
        *dst++ = (0);
        data_length++;
        *dst++ = (cmdbuf_id);
        data_length++;
        //alignment, do not do this step for driver will use this data to identify JMP and End opcode.
        *total_length = data_length;
    }
}

void CWLCollectClrIntData(u32 *dst, u32 clear_type, u16 interrupt_reg_addr, u32 bitmask,
                          u32 *total_length)
{
    u32 data_length = 0;
    {
        //opcode
        *dst++ = OPCODE_CLRINT | (clear_type << 25) | interrupt_reg_addr * 4;
        data_length++;
        //bitmask
        *dst = bitmask;
        data_length++;
        *total_length = data_length;
    }
}

#endif
