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
--  Abstract : Encoder Wrapper Layer, common parts
--
------------------------------------------------------------------------------*/
#ifdef SUPPORT_MEM_SYNC
/*------------------------------------------------------------------------------
1. Include headers
------------------------------------------------------------------------------*/
#include "ewl.h"
#include "ewl_local.h"
#include "ewl_memsync.h"
#include "enc_log.h"
/*------------------------------------------------------------------------------
Function name   : EWLMemSyncAllocHostBuffer
Description     : Allocate a contiguous, linear RAM  memory buffer in host
                  when need independent memory instead of shared memory

Return type     : i32 - 0 for success or a negative error code

Argument        : const void * instance - EWL instance
Argument        : u32 alignment - alignment in bytes of the requested memory
Argument        : EWLLinearMem_t *buff - place where the allocated memory
                  buffer parameters are returned
Argument        : MemallocParams params - the parameters of alloc buffer
------------------------------------------------------------------------------*/
i32 EWLMemSyncAllocHostBuffer(const void *inst, u32 size, u32 alignment, EWLLinearMem_t *buff)
{
    vc8000_cwl_t *enc;

    enc = (vc8000_cwl_t *)inst;
    struct sync_mem *priv = (struct sync_mem *)buff->priv;
    /*host buffer Need client to adjust by their platform*/
    if (buff->mem_type & CPU_RD || buff->mem_type & CPU_WR || buff->mem_type & EXT_WR ||
        buff->mem_type & EXT_RD) {
        buff->allocVirtualAddr = (u32 *)EWLcalloc(1, size + LINMEM_ALIGN);
        buff->virtualAddress = (u32 *)NEXT_ALIGNED(buff->allocVirtualAddr);
    } else {
        buff->allocVirtualAddr = NULL;
        buff->virtualAddress = NULL;
    }
    return EWL_OK;
}

/*------------------------------------------------------------------------------
Function name   : EWLMemSyncFreeHostBuffer
Description     : Release a linera memory buffer in host side,
                  when need independent memory instead of shared memory.

Return type     : i32 - 0 for success or a negative error code


Argument        : const void * instance - EWL instance
Argument        : EWLLinearMem_t *buff - linear buffer memory information
------------------------------------------------------------------------------*/
i32 EWLMemSyncFreeHostBuffer(const void *inst, EWLLinearMem_t *buff)
{
    /*free host buffer: Need client to adjust by their platform*/
    if (buff->virtualAddress != NULL) {
        EWLfree(buff->allocVirtualAddr);
        buff->allocVirtualAddr = NULL;
        buff->virtualAddress = NULL;
    }
    return EWL_OK;
}

/*------------------------------------------------------------------------------
Function name   : EWLSyncMemData
Description     : Synchronize the data of the host side
                  and device side Memory

Return value     : EWL_OK means successful data synchronization
                   EWL_PAR_ERROR means parameter is wrong
                   EWL_HW_ERROR means failed to synchronize data with HW

Argument        : EWLLinearMem_t *mem - memory information needs to be synchronized
Argument        : u32 offset - the offset of memsync in bytes; implemented alignment in API
Argument        : u32 length - the length of the memory data synchronization.
Argument        : enum EWLMemSyncDirection dir - the direction of memory data synchronized
------------------------------------------------------------------------------*/
i32 EWLSyncMemData(EWLLinearMem_t *mem, u32 offset, u32 length, enum EWLMemSyncDirection dir)
{
    if (mem == NULL || mem->virtualAddress == NULL || mem->busAddress == 0)
        return EWL_PAR_ERROR;
    if (length > mem->size)
        return EWL_PAR_ERROR;
    struct sync_mem *priv = (struct sync_mem *)mem->priv;
    u32 *MemSyncVirtualAddress = mem->virtualAddress;
    ptr_t MemSyncBusAddress = mem->busAddress;

#ifdef MEM_SYNC_TEST //Just for test; Customer should make modification based on the actual DMA platform
    MemSyncBusAddress = (ptr_t)priv->dev_va;
    if (offset) {
        MemSyncVirtualAddress = (u32 *)((u8 *)MemSyncVirtualAddress + offset);
        MemSyncBusAddress += offset;
    }
    if (dir == HOST_TO_DEVICE) {
        EWLmemcpy((u32 *)MemSyncBusAddress, MemSyncVirtualAddress, length);
    } else if (dir == DEVICE_TO_HOST) {
        EWLmemcpy(MemSyncVirtualAddress, (u32 *)MemSyncBusAddress, length);
    }
#endif

    return EWL_OK;
}

#endif
