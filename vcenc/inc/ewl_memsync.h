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

#ifndef __EWL_MEMSYNC_H__
#define __EWL_MEMSYNC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "base_type.h"
#include "ewl.h"

/*If want to define MEM_SYNC_TEST, the premise is open SUPPORT_MEM_SYNC, otherwise, it doesn't make sense*/
#ifdef SUPPORT_MEM_SYNC
//#define MEM_SYNC_TEST
#else
#ifdef MEM_SYNC_TEST
#undef MEM_SYNC_TEST
#endif
#endif

/** struct sync_mem is used to get some necessary buffer address when doing test verification
    * of memory synchronization, user can overload their own structure */
struct sync_mem {
    u32 *dev_va;        /**< virtual address for mem in device from Host view */
    ptr_t dev_pa_alloc; /**< physical address for alloc mem in device from Host view */
    u32 *dev_va_alloc;  /**< virtual address for alloc mem in device from Host view */
};

/**
 * \defgroup api_ewl Encoder Wrapper Layer(EWL) API
 *
 * @{
 */
/** If support memory sync, can use the function to trans data between host and device*/
i32 EWLSyncMemData(EWLLinearMem_t *mem, u32 offset, u32 length, enum EWLMemSyncDirection dir);

/** If support memory sync, can use the function to alloc buffer in host side*/
i32 EWLMemSyncAllocHostBuffer(const void *inst, u32 size, u32 alignment, EWLLinearMem_t *buff);

/** If support memory sync, can use the function to free buffer in host side*/
i32 EWLMemSyncFreeHostBuffer(const void *inst, EWLLinearMem_t *buff);

/**@}*/

#ifndef SUPPORT_MEM_SYNC
#define EWLSyncMemData(mem, offset, length, dir) (EWL_OK)
#define EWLMemSyncAllocHostBuffer(inst, size, alignment, buff) (EWL_NOT_SUPPORT)
#define EWLMemSyncFreeHostBuffer(inst, buff) (EWL_NOT_SUPPORT)
#endif

#ifdef __cplusplus
}
#endif

#endif /* __EWL_MEMSYNC_H__ */
