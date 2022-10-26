/*------------------------------------------------------------------------------
--                                                                                                                               --
--       This software is confidential and proprietary and may be used                                   --
--        only as expressly authorized by a licensing agreement from                                     --
--                                                                                                                               --
--                            Verisilicon.                                                                                    --
--                                                                                                                               --
--                   (C) COPYRIGHT 2014 VERISILICON                                                            --
--                            ALL RIGHTS RESERVED                                                                    --
--                                                                                                                               --
--                 The entire notice above must be reproduced                                                 --
--                  on all copies and should not be removed.                                                    --
--                                                                                                                               --
--------------------------------------------------------------------------------*/
#ifndef POOL_H
#define POOL_H
#include <pthread.h>

#include "base_type.h"
#include "vsi_queue.h"
#include "hevcencapi.h"

//init a software buffer pool
VCEncRet InitBufferPool(void **pool, u32 nodeSize, u32 nodeNum);
//get a buffer from the software buffer pool
VCEncRet GetBufferFromPool(void *pool, void **buffer);
//return a buffer to the software buffer pool
VCEncRet PutBufferToPool(void *pool, void **buffer);
//relase the software buffer pool
VCEncRet ReleaseBufferPool(void **pool);

//dynamic get free buffer from pool (if no valid buffer, malloc new buffer);
void *DynamicGetBufferFromPool(struct queue *pool, u32 bufSize);
//return buffer to dynamic buffer pool
void DynamicPutBufferToPool(struct queue *pool, void *pBuf);
//release dynamic buffer pool
void DynamicReleasePool(struct queue *pool, struct queue *queue);

#endif
