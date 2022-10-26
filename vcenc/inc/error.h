/*------------------------------------------------------------------------------
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                                                                            --
--                            Verisilicon.                                    --
--                                                                            --
--                   (C) COPYRIGHT 2014 VERISILICON                           --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
--------------------------------------------------------------------------------*/

#ifndef ERROR_H
#define ERROR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "base_type.h"
#include "osal.h"

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define ERR "Error: " __FILE__ ", line " TOSTRING(__LINE__) ": "
#define SYSERR "System error message"

/* rename functions with prefix. */
#define Error VSIAPI(Error)

#ifdef ERROR_BUILD_SUPPORT
void Error(i32 numArgs, ...);
#else
#ifndef SYSTEMSHARED
#define VSIAPIError(...)
#else
#define VSISYSError(...)
#endif
#endif

#ifdef __cplusplus
}
#endif

#endif
