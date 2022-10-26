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
#include <stdlib.h>

#include "pool.h"
#include "tools.h"

typedef struct {
    void *buffer;
    u32 *flags;
    pthread_mutex_t poolMutex; //mutex of buffer pool
    u32 nodeSize;              //size of a single buffer
    u32 totalNum;              //alloc buffer numbers
} BufferPool_t;

/*------------------------------------------------------------------------------
    Function name   : InitBufferPool
    Description     : alloc a  continuous buffer for buffer pool
    Return type     : VCEncRet
    Argument        : void* pool[out]    bufferpool address
    Argument        : u32 nodeSize[in]   buffer size
    Argument        : u32 nodeNum[in]    buffer number (enough to use)
------------------------------------------------------------------------------*/
VCEncRet InitBufferPool(void **pool, u32 nodeSize, u32 nodeNum)
{
    BufferPool_t *bufferPool = NULL;
    pthread_mutexattr_t mutexattr;

    bufferPool = (BufferPool_t *)EWLcalloc(1, sizeof(BufferPool_t));
    if (NULL == bufferPool) {
        return VCENC_MEMORY_ERROR;
    }
    *pool = (void *)bufferPool;

    //alloc buffer
    bufferPool->buffer = EWLcalloc(nodeNum, nodeSize);
    if (NULL == bufferPool->buffer) {
        return VCENC_MEMORY_ERROR;
    }

    //alloc buffer's usable flag
    bufferPool->flags = (u32 *)EWLcalloc(nodeNum, sizeof(u32));
    if (NULL == bufferPool->flags) {
        return VCENC_MEMORY_ERROR;
    }

    pthread_mutexattr_init(&mutexattr);
    pthread_mutex_init(&bufferPool->poolMutex, &mutexattr);
    pthread_mutexattr_destroy(&mutexattr);

    bufferPool->nodeSize = nodeSize;
    bufferPool->totalNum = nodeNum;

    return VCENC_OK;
}

/*------------------------------------------------------------------------------
    Function name   : GetBufferfromPool
    Description     : get a buffer from buffer pool
    Return type     : VCEncRet
    Argument        : void* pool[in]       bufferpool
    Argument        : void** buffer[out]   buffer address
------------------------------------------------------------------------------*/
VCEncRet GetBufferFromPool(void *pool, void **buffer)
{
    char *buf = NULL;
    BufferPool_t *bufferPool = NULL;
    VCEncRet ret = VCENC_ERROR;
    i8 bFind = HANTRO_FALSE;

    do {
        if (NULL == pool || NULL == buffer) {
            ret = VCENC_INVALID_ARGUMENT;
            break;
        }
        bufferPool = (BufferPool_t *)pool;

        pthread_mutex_lock(&bufferPool->poolMutex);
        for (u32 i = 0; i < bufferPool->totalNum; i++) {
            if (0 == bufferPool->flags[i]) {
                //found usable buffer
                bufferPool->flags[i] = 1;
                buf = (char *)bufferPool->buffer + i * bufferPool->nodeSize;
                bFind = HANTRO_TRUE;
                break;
            }
        }
        pthread_mutex_unlock(&bufferPool->poolMutex);

        if (HANTRO_FALSE == bFind)
            break;

        *buffer = (void *)buf;
        ret = VCENC_OK;
    } while (0);

    return ret;
}

VCEncRet PutBufferToPool(void *pool, void **buffer)
{
    BufferPool_t *bufferPool = NULL;
    VCEncRet ret = VCENC_ERROR;
    i8 bFind = HANTRO_FALSE;

    do {
        if (NULL == pool || NULL == buffer) {
            ret = VCENC_INVALID_ARGUMENT;
            break;
        }
        bufferPool = (BufferPool_t *)pool;

        pthread_mutex_lock(&bufferPool->poolMutex);
        for (u32 i = 0; i < bufferPool->totalNum; i++) {
            if (*buffer == (char *)bufferPool->buffer + i * bufferPool->nodeSize) {
                // found buffer id to return to pool
                bufferPool->flags[i] = 0;
                *buffer = NULL;
                bFind = HANTRO_TRUE;
                break;
            }
        }
        pthread_mutex_unlock(&bufferPool->poolMutex);
        if (HANTRO_FALSE == bFind)
            break;

        ret = VCENC_OK;
    } while (0);

    return ret;
}

/*------------------------------------------------------------------------------
    Function name   : ReleaseBufferPool
    Description     : free buffers in buffer pool
    Return type     : VCEncRet
    Argument        : void** pool
------------------------------------------------------------------------------*/
VCEncRet ReleaseBufferPool(void **pool)
{
    struct node *node, *tail;

    if (NULL == pool || NULL == *pool) {
        return VCENC_OK;
    }
    BufferPool_t *bufferPool = (BufferPool_t *)*pool;

    pthread_mutex_lock(&bufferPool->poolMutex);
    FreeBuffer(&bufferPool->buffer);         //release buffer
    FreeBuffer((void **)&bufferPool->flags); //release flags
    pthread_mutex_unlock(&bufferPool->poolMutex);

    pthread_mutex_destroy(&bufferPool->poolMutex);
    FreeBuffer(pool);

    return VCENC_OK;
}

/*------------------------------------------------------------------------------
    Function name   : DynamicGetBufferFromPool
    Description     : dynamic get free buffer from pool (if no valid buffer, malloc new buffer)
    Return type     : void*
    Argument        : struct queue *pool  --buffer pool
    Argument        : u32 bufSize  --buffer size
------------------------------------------------------------------------------*/
void *DynamicGetBufferFromPool(struct queue *pool, u32 bufSize)
{
    void *pBuf = (void *)queue_get(pool);
    if (!pBuf)
        pBuf = EWLcalloc(1, bufSize);
    else
        memset(pBuf, 0, bufSize);
    return pBuf;
}

/*------------------------------------------------------------------------------
    Function name   : DynamicPutBufferToPool
    Description     : return buffer to dynamic buffer pool
    Return type     : void
    Argument        : struct queue *pool  --buffer pool
    Argument        : void*pBuf   --buffer to be returned to pool
------------------------------------------------------------------------------*/
void DynamicPutBufferToPool(struct queue *pool, void *pBuf)
{
    queue_put(pool, (struct node *)pBuf);
    return;
}

/*------------------------------------------------------------------------------
    Function name   : DynamicReleasePool
    Description     : release dynamic buffer pool
    Return type     : void
    Argument        : struct queue *pool  --buffer pool
    Argument        : struct queue *queue  --buffers got from the pool is in the queue
------------------------------------------------------------------------------*/
void DynamicReleasePool(struct queue *pool, struct queue *queue)
{
    struct node *pBuf = NULL;
    while ((pBuf = queue_get(pool)) != NULL)
        FreeBuffer((void **)&pBuf);
    while ((pBuf = queue_get(queue)) != NULL)
        FreeBuffer((void **)&pBuf);

    return;
}
