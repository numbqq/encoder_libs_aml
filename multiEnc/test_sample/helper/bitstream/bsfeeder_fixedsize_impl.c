//--=========================================================================--
//  This file is a part of VPU Reference API project
//-----------------------------------------------------------------------------
//
//       This confidential and proprietary software may be used only
//     as authorized by a licensing agreement from Chips&Media Inc.
//     In the event of publication, the following notice is applicable:
//
//            (C) COPYRIGHT CHIPS&MEDIA INC.
//                      ALL RIGHTS RESERVED
//
//       The entire notice above must be reproduced on all authorized
//       copies.
//
//--=========================================================================--
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>

#include "vpuapifunc.h"
#include "main_helper.h"

#define MAX_FEEDING_SIZE        0x400000        /* 4MBytes */
#define DEFAULT_FEEDING_SIZE    0x20000         /* 128KBytes */

typedef struct FeederFixedContext {
    osal_file_t*    fp;
    Uint32          feedingSize;
    BOOL            eos;
} FeederFixedContext;

void* BSFeederFixedSize_Create(
    const char* path,
    CodStd      codecId
    )
{
    osal_file_t*    fp = NULL;
    FeederFixedContext*  context=NULL;

    UNREFERENCED_PARAMETER(codecId);

    if ((fp=osal_fopen(path, "rb")) == NULL) {
        VLOG(ERR, "%s:%d failed to open %s\n", __FUNCTION__, __LINE__, path);
        return NULL;
    }

    context = (FeederFixedContext*)osal_malloc(sizeof(FeederFixedContext));
    if (context == NULL) {
        VLOG(ERR, "%s:%d failed to allocate memory\n", __FUNCTION__, __LINE__);
        osal_fclose(fp);
        return NULL;
    }

    context->fp          = fp;
    context->feedingSize = DEFAULT_FEEDING_SIZE;
    context->eos         = FALSE;

    return (void*)context;
}

BOOL BSFeederFixedSize_Destroy(
    void* feeder
    )
{
    FeederFixedContext* context = (FeederFixedContext*)feeder;

    if (context == NULL) {
        VLOG(ERR, "%s:%d Null handle\n", __FUNCTION__, __LINE__);
        return FALSE;
    }

    if (context->fp) 
        osal_fclose(context->fp);

    osal_free(context);

    return TRUE;
}

Int32 BSFeederFixedSize_Act(
    void*       feeder,
    BSChunk*    chunk
    )
{
    FeederFixedContext*  context = (FeederFixedContext*)feeder;
    size_t          nRead;
    Uint32          size;
    Uint32          feedingSize;

    if (context == NULL) {
        VLOG(ERR, "%s:%d Null handle\n", __FUNCTION__, __LINE__);
        return 0;
    }

    if (context->eos == TRUE) {
        chunk->eos = TRUE;
        return 0;
    }

    feedingSize = context->feedingSize;
    if (feedingSize == 0) {
        Uint32  KB = 1024;
        BOOL    probability10;

        srand((Uint32)time(NULL));
        feedingSize   = rand() % MAX_FEEDING_SIZE;
        probability10 = (BOOL)((feedingSize%100) < 10);
        if (feedingSize < KB) {
            if (probability10 == FALSE) 
                feedingSize *= 100;
        }
    }

    size = (chunk->size < feedingSize) ? chunk->size : feedingSize;

    do {
        nRead = osal_fread(chunk->data, 1, size, context->fp);
        if ((Int32)nRead < 0) {
            VLOG(ERR, "%s:%d failed to read bitstream(errno: %d)\n", __FUNCTION__, __LINE__, errno);
            return 0;
        } 
        else if (nRead < size) {
            context->eos = TRUE;
            chunk->eos   = TRUE;
        }
    } while (FALSE);

    return nRead;
}

BOOL BSFeederFixedSize_Rewind(
    void*       feeder
    )
{
    FeederFixedContext*  context = (FeederFixedContext*)feeder;
    Int32           ret;

    if ((ret=osal_fseek(context->fp, 0, SEEK_SET)) != 0) {
        VLOG(ERR, "%s:%d Failed to fseek(ret: %d)\n", __FUNCTION__, __LINE__, ret);
        return FALSE;
    }
    context->eos = FALSE;

    return TRUE;
}

void BSFeederFixedSize_SetFeedingSize(
    void*   feeder,
    Uint32  feedingSize
    )
{
    FeederFixedContext*  context = (FeederFixedContext*)feeder;
    if (feedingSize > 0) {
        context->feedingSize = feedingSize;
    }
}

