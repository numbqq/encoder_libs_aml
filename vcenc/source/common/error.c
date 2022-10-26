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
#ifdef ERROR_BUILD_SUPPORT

#include <stdarg.h>
#include <stdio.h>
#include "vsi_string.h"
#include "error.h"

void Error(i32 numArgs, ...)
{
    va_list ap;
    char *s;
    i32 i;

    va_start(ap, numArgs);
    for (i = 0; i < numArgs; i++) {
        if (!(s = va_arg(ap, char *)))
            continue;
        if (vsi_strcmp(SYSERR, s) == 0) {
            perror(NULL);
            return;
        } else {
            fprintf(stderr, "%s", s);
        }
    }
    fprintf(stderr, "\n");
    va_end(ap);
}

#endif