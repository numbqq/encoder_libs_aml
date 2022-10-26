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

/*------------------------------------------------------------------------------
1. Include headers
------------------------------------------------------------------------------*/
#include "ewl.h"
#include "ewl_sys_local.h"
#include "ewl_memsync.h"
#include "enc_log.h"

/*------------------------------------------------------------------------------
Function name   : EWLMemSyncAllocHostBufferSys
Description     : Allocate a contiguous, linear RAM  memory buffer in host
                when need independent memory instead of shared memory

Return type     : i32 - 0 for success or a negative error code

Argument        : const void * instance - EWL instance
Argument        : u32 alignment - alignment in bytes of the requested memory
Argument        : EWLLinearMem_t *buff - place where the allocated memory
                  buffer parameters are returned
Argument        : MemallocParams params - the parameters of alloc buffer
------------------------------------------------------------------------------*/
i32 EWLMemSyncAllocHostBufferSys(const void *instance, u32 size, u32 alignment,
                                 EWLLinearMem_t *buff)
{
    ewlSysInstance *inst = (ewlSysInstance *)instance;

    if (buff->mem_type & CPU_WR || buff->mem_type & CPU_RD || buff->mem_type & EXT_WR ||
        buff->mem_type & EXT_RD) {
        inst->chunks[inst->totalChunks] = (u32 *)calloc(1, size + LINMEM_ALIGN);
        if (inst->chunks[inst->totalChunks] == NULL) {
            buff->virtualAddress = NULL;
            return EWL_ERROR;
        }
        inst->alignedChunks[inst->totalChunks] =
            (u32 *)NEXT_ALIGNED(inst->chunks[inst->totalChunks]);
        PTRACE_I((void *)instance, ("EWLMemSyncAllocHostBufferSys: %p, aligned %p\n",
                                    (void *)inst->chunks[inst->totalChunks],
                                    (void *)inst->alignedChunks[inst->totalChunks]));
        buff->virtualAddress = inst->alignedChunks[inst->totalChunks++];
    } else {
        buff->virtualAddress = NULL;
    }

    return EWL_OK;
}

/*------------------------------------------------------------------------------
Function name   : EWLMemSyncFreeHostBufferSys
Description     : Release a linera memory buffer in host side,
                when need independent memory instead of shared memory.

Return type     : i32 - 0 for success or a negative error code

Argument        : const void * instance - EWL instance
Argument        : EWLLinearMem_t *buff - linear buffer memory information
------------------------------------------------------------------------------*/
i32 EWLMemSyncFreeHostBufferSys(const void *instance, EWLLinearMem_t *buff)
{
    ewlSysInstance *inst = (ewlSysInstance *)instance;
    u32 i;
    if (buff->virtualAddress != NULL) {
        for (i = 0; i < inst->totalChunks; i++) {
            if (inst->alignedChunks[i] == buff->virtualAddress) {
                PTRACE_I((void *)instance, "EWLMemSyncFreeHostBufferSys virtualAddress \t%p\n",
                         (void *)buff->virtualAddress);

                free(inst->chunks[i]);
                inst->chunks[i] = NULL;
                inst->alignedChunks[i] = NULL;
                buff->virtualAddress = NULL;
                buff->total_size = 0;
                buff->size = 0;
                return EWL_OK;
            }
        }
    }
    return EWL_OK;
}

/*------------------------------------------------------------------------------
Function name   : EWLSyncMemDataSys
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
i32 EWLSyncMemDataSys(EWLLinearMem_t *mem, u32 offset, u32 length, enum EWLMemSyncDirection dir)
{
    assert(mem != NULL);
    assert(length <= mem->size);
    if (mem->virtualAddress == NULL || mem->busAddress == 0)
        return EWL_PAR_ERROR;

    u32 *MemSyncVirtualAddress = mem->virtualAddress;
    ptr_t MemSyncBusAddress = mem->busAddress;
    if (offset) {
        MemSyncVirtualAddress = (u32 *)((u8 *)MemSyncVirtualAddress + offset);
        MemSyncBusAddress += offset;
    }

    if (dir == HOST_TO_DEVICE) {
        EWLmemcpy((u32 *)MemSyncBusAddress, MemSyncVirtualAddress, length);
    } else if (dir == DEVICE_TO_HOST) {
        EWLmemcpy(MemSyncVirtualAddress, (u32 *)MemSyncBusAddress, length);
    }
    return EWL_OK;
}
