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
#ifdef FIFO_BUILD_SUPPORT

#include "osal.h"
#include "fifo.h"
#include "ewl.h"
/* macro for assertion, used only when _ASSERT_USED is defined */
/*#ifdef _ASSERT_USED
#ifndef ASSERT
#include <assert.h>
#define ASSERT(expr) assert(expr)
#endif
#else
#define ASSERT(expr)
#endif
*/

/* Container for instance. */
struct Fifo {
    sem_t cs_semaphore;    /* Semaphore for critical section. */
    sem_t read_semaphore;  /* Semaphore for readers. */
    sem_t write_semaphore; /* Semaphore for writers. */
    u32 num_of_slots;
    u32 num_of_objects;
    u32 tail_index;
    FifoObject *nodes;
    u32 abort;
};

enum FifoRet FifoInit(u32 num_of_slots, FifoInst *instance)
{
    struct Fifo *inst = EWLcalloc(1, sizeof(struct Fifo));
    if (inst == NULL)
        return FIFO_ERROR_MEMALLOC;
    inst->num_of_slots = num_of_slots;
    /* Allocate memory for the objects. */
    inst->nodes = EWLcalloc(num_of_slots, sizeof(FifoObject));
    if (inst->nodes == NULL) {
        EWLfree(inst);
        return FIFO_ERROR_MEMALLOC;
    }
    /* Initialize binary critical section semaphore. */
    sem_init(&inst->cs_semaphore, 0, 1);
    /* Then initialize the read and write semaphores. */
    sem_init(&inst->read_semaphore, 0, 0);
    sem_init(&inst->write_semaphore, 0, num_of_slots);
    *instance = inst;
    return FIFO_OK;
}

enum FifoRet FifoPush(FifoInst inst, FifoObject object, enum FifoException e)
{
    struct Fifo *instance = (struct Fifo *)inst;
    int value;

    sem_getvalue(&instance->read_semaphore, &value);
    if ((e == FIFO_EXCEPTION_ENABLE) && ((u32)value == instance->num_of_slots) &&
        (instance->num_of_objects == instance->num_of_slots)) {
        return FIFO_FULL;
    }

    sem_wait(&instance->write_semaphore);
    sem_wait(&instance->cs_semaphore);
    instance->nodes[(instance->tail_index + instance->num_of_objects) % instance->num_of_slots] =
        object;
    instance->num_of_objects++;
    sem_post(&instance->cs_semaphore);
    sem_post(&instance->read_semaphore);
    return FIFO_OK;
}

enum FifoRet FifoPop(FifoInst inst, FifoObject *object, enum FifoException e)
{
    struct Fifo *instance = (struct Fifo *)inst;
    int value;

    sem_getvalue(&instance->write_semaphore, &value);
    if ((e == FIFO_EXCEPTION_ENABLE) && ((u32)value == instance->num_of_slots) &&
        (instance->num_of_objects == 0)) {
        return FIFO_EMPTY;
    }

    sem_wait(&instance->read_semaphore);
    sem_wait(&instance->cs_semaphore);

    if (instance->abort)
        return FIFO_ABORT;

    *object = instance->nodes[instance->tail_index % instance->num_of_slots];
    instance->tail_index++;
    instance->num_of_objects--;
    sem_post(&instance->cs_semaphore);
    sem_post(&instance->write_semaphore);
    return FIFO_OK;
}

u32 FifoCount(FifoInst inst)
{
    u32 count;
    struct Fifo *instance = (struct Fifo *)inst;
    sem_wait(&instance->cs_semaphore);
    count = instance->num_of_objects;
    sem_post(&instance->cs_semaphore);
    return count;
}

u32 FifoHasObject(FifoInst inst, FifoObject object)
{
    u32 i;
    u32 success = 0;
    struct Fifo *instance = (struct Fifo *)inst;
    sem_wait(&instance->cs_semaphore);
    for (i = 0; i < instance->num_of_objects; i++) {
        if (instance->nodes[(instance->tail_index + i) % instance->num_of_slots] == object) {
            success = 1;
            break;
        }
    }
    sem_post(&instance->cs_semaphore);
    return success;
}

void FifoRelease(FifoInst inst)
{
    struct Fifo *instance = (struct Fifo *)inst;
#ifdef HEVC_EXT_BUF_SAFE_RELEASE
    ASSERT(instance->num_of_objects == 0);
#endif
    sem_wait(&instance->cs_semaphore);
    sem_destroy(&instance->cs_semaphore);
    sem_destroy(&instance->read_semaphore);
    sem_destroy(&instance->write_semaphore);
    EWLfree(instance->nodes);
    EWLfree(instance);
}

void FifoSetAbort(FifoInst inst)
{
    struct Fifo *instance = (struct Fifo *)inst;
    if (instance == NULL)
        return;
    instance->abort = 1;
    sem_post(&instance->cs_semaphore);
    sem_post(&instance->read_semaphore);
}

void FifoClearAbort(FifoInst inst)
{
    struct Fifo *instance = (struct Fifo *)inst;
    if (instance == NULL)
        return;
    instance->abort = 0;
}

#endif