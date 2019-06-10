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

#include <errno.h>
#include "main_helper.h"

typedef struct {
    osal_file_t     fp;
    Uint32          md5Size;
    Uint32          prevMd5[12];
    Uint32          loopCount;
} md5CompContext;


BOOL MD5Comparator_Create(
    ComparatorImpl* impl,
    char*           path
    )
{
    md5CompContext*        ctx;
    osal_file_t     fp;
    Uint32        temp;

    if ((fp=osal_fopen(path, "r")) == NULL) {
        VLOG(ERR, "%s:%d failed to open md5 file: %s, errno(%d)\n", __FUNCTION__, __LINE__, path, errno);
        return FALSE;
    }

    if ((ctx=(md5CompContext*)osal_malloc(sizeof(md5CompContext))) == NULL) {
        osal_fclose(fp);
        return FALSE;
    }

    while (!osal_feof(fp)) {
        if (osal_fscanf((FILE*)fp, "%08x", &temp) < 1) break;
        impl->numOfFrames++;
    }

    osal_fseek(fp, 0, SEEK_SET);

    ctx->fp       = fp;
    ctx->md5Size  = 12;
    ctx->loopCount = 1;
    impl->context = ctx;
    impl->eof     = FALSE;

    return TRUE;
}

BOOL MD5Comparator_Destroy(
    ComparatorImpl*  impl
    )
{
    md5CompContext*    ctx = (md5CompContext*)impl->context;

    osal_fclose(ctx->fp);
    osal_free(ctx);

    return TRUE;
}

BOOL MD5Comparator_Compare(
    ComparatorImpl* impl,
    void*           data,
    Uint32          size
    )
{
    md5CompContext*    ctx = (md5CompContext*)impl->context;
    BOOL        match = TRUE;
    BOOL        lineMatch[12]={0,};
    Uint32      md5[12];
    Uint32      idx;
    Uint32*     decodedMD5 = (Uint32*)data;

    if ( data == (void *)COMPARATOR_SKIP ) {
        for (idx=0; idx<ctx->md5Size; idx++) {
            osal_fscanf((FILE*)ctx->fp, "%08x", &md5[idx]);
            if (IsEndOfFile(ctx->fp) == TRUE) {
                impl->eof = TRUE;
                break;
            }
        };

        return TRUE;
    }

    do {
        osal_memset((void*)md5, 0x00, sizeof(md5));
        if (impl->usePrevDataOneTime == TRUE) {
            impl->usePrevDataOneTime = FALSE;
            osal_memcpy(md5, ctx->prevMd5, ctx->md5Size*sizeof(md5[0]));
        }
        else {
            for (idx=0; idx<ctx->md5Size; idx++) {
                /* FIXME: osal_osal_fscanf has problem on Windows.
                */
                osal_fscanf((FILE*)ctx->fp, "%08x", &md5[idx]);

                if (IsEndOfFile(ctx->fp) == TRUE) {
                    impl->eof = TRUE;
                    break;
                }
            }
        }

        if (data == NULL)
            return FALSE;

        match = TRUE;
        for (idx=0; idx<ctx->md5Size; idx++) {
            if (md5[idx] != decodedMD5[idx]) {
                match = FALSE;
                lineMatch[idx]=FALSE;
            }
            else
                lineMatch[idx]=TRUE;
        }
    } while (impl->enableScanMode == TRUE && match == FALSE && impl->eof == FALSE);

    osal_memcpy(ctx->prevMd5, md5, ctx->md5Size*sizeof(md5[0]));

    if (match == FALSE ) {
        VLOG(ERR, "MISMATCH WITH GOLDEN MD5 at %d frame\n", impl->curIndex);
        VLOG(ERR, "GOLDEN         DECODED      RESULT\n"
                  "------------------------------------\n");
        for (idx=0; idx<ctx->md5Size; idx++)  {
            VLOG(ERR, "%08x       %08x       %s\n", md5[idx], decodedMD5[idx], lineMatch[idx]==TRUE ? " O " : "XXX");
        }
    }

    return match;
}

BOOL MD5Comparator_Configure(
    ComparatorImpl*     impl,
    ComparatorConfType  type,
    void*               val
    )
{
    md5CompContext*    ctx = (md5CompContext*)impl->context;
    BOOL        ret = TRUE;

    switch (type) {
    case COMPARATOR_CONF_SET_GOLDEN_DATA_SIZE:
        ctx->md5Size = *(Uint32*)val;
        impl->numOfFrames /= ctx->md5Size;
        break;
    default:
        ret = FALSE;
        break;
    }

    return ret;
}

BOOL MD5Comparator_Rewind(
    ComparatorImpl*     impl
    )
{
    md5CompContext*    ctx = (md5CompContext*)impl->context;
    Int32       ret;

    if ((ret=osal_fseek(ctx->fp, 0, SEEK_SET)) != 0) {
        VLOG(ERR, "%s:%d Failed to osal_fseek(ret: %d)\n", __FUNCTION__, __LINE__, ret);
        return FALSE;
    }

    impl->eof = FALSE;

    return TRUE;
}

ComparatorImpl md5ComparatorImpl = {
    NULL,
    NULL,
    0,
    0,
    MD5Comparator_Create,
    MD5Comparator_Destroy,
    MD5Comparator_Compare,
    MD5Comparator_Configure,
    MD5Comparator_Rewind,
};
 
