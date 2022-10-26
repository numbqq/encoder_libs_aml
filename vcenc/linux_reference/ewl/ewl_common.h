/*------------------------------------------------------------------------------
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                                                                            --
--                            Verisilicon.                                    --
--                                                                            --
--                   (C) COPYRIGHT 2021 VERISILICON                           --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
--------------------------------------------------------------------------------

--
--  Abstract : header file of Encoder Wrapper Layer Common Part
--
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/

#ifndef __EWL_COMMON_H__
#define __EWL_COMMON_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "base_type.h"
#include "ewl.h"

#define VCE_BUILD_ID_PBASE 0x800  /* BUILD ID Base for on-the-fly parsing cores */
#define VCE_BUILD_ID_LBASE 0x1000 /* BUILD ID Base for legacy cores, for c-model testing. */
#define VCE_BUILD_ID_START 0x2000 /* BUILD ID Start value for valid ID */

#ifdef SYSTEMSHARED
/* rename EWL functions for access in c-model library. */
#define EWLmemcmp memcmp
#define EWLGetCoreSignature CoreEncGetCoreSignature
#define EWLGetCoreConfig CoreEncGetCoreConfig
#endif

/** The Signature is the Register Value set which can identify the core. */
typedef struct EWLCoreSignature {
    u32 hw_asic_id;  /**< HW ASIC ID read from swreg0 */
    u32 hw_build_id; /**< HW BUILD ID read from swreg509. valid from VC9000. */
    /** Read Only Registers values which records HW features before VC9000 */
    u32 fuse[HWIF_CFG_NUM];
} EWLCoreSignature_t;

i32 EWLGetCoreSignature(u32 *regs, EWLCoreSignature_t *signature);
i32 EWLGetCoreConfig(EWLCoreSignature_t *signature, EWLHwConfig_t *cfg_info);

#ifdef __cplusplus
}
#endif

#endif /* __EWL_COMMON_H__ */
