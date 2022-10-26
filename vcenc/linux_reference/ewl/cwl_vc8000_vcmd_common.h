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
--------------------------------------------------------------------------------
--
--  Abstract : VC8000 CODEC Wrapper Layer
--
------------------------------------------------------------------------------*/
#ifndef __CWL_VC8000_VCMD_COMMON_H__
#define __CWL_VC8000_VCMD_COMMON_H__

#include "base_type.h"
#include "osal.h"

#define OPCODE_WREG (0x01 << 27)
#define OPCODE_END (0x02 << 27)
#define OPCODE_NOP (0x03 << 27)
#define OPCODE_RREG (0x16 << 27)
#define OPCODE_INT (0x18 << 27)
#define OPCODE_JMP (0x19 << 27)
#define OPCODE_STALL (0x09 << 27)
#define OPCODE_CLRINT (0x1a << 27)
#define OPCODE_JMP_RDY0 (0x19 << 27)
#define OPCODE_JMP_RDY1 ((0x19 << 27) | (1 << 26))

#ifndef VCMD_BUILD_SUPPORT
#define CWLCollectWriteRegData(src, dst, reg_start, reg_length, total_length)
#define CWLCollectStallData(dst, total_length, interruput_mask)
#define CWLCollectReadRegData(dst, reg_start, reg_length, total_length, status_data_base_addr)
#define CWLCollectNopData(dst, total_length)
#define CWLCollectIntData(dst, int_vecter, total_length)
#define CWLCollectJmpData(dst, total_length, cmdbuf_id)
#define CWLCollectClrIntData(dst, clear_type, interrupt_reg_addr, bitmask, total_length)
#else
void CWLCollectWriteRegData(u32 *src, u32 *dst, u16 reg_start, u32 reg_length, u32 *total_length);
void CWLCollectStallData(u32 *dst, u32 *total_length, u32 interruput_mask);
void CWLCollectReadRegData(u32 *dst, u16 reg_start, u32 reg_length, u32 *total_length,
                           ptr_t status_data_base_addr);
void CWLCollectNopData(u32 *dst, u32 *total_length);
void CWLCollectIntData(u32 *dst, u16 int_vecter, u32 *total_length);
void CWLCollectJmpData(u32 *dst, u32 *total_length, u16 cmdbuf_id);
void CWLCollectClrIntData(u32 *dst, u32 clear_type, u16 interrupt_reg_addr, u32 bitmask,
                          u32 *total_length);
#endif

#endif /* __CWL_VC8000_VCMD_COMMON_H__ */
