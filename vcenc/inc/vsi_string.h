/*------------------------------------------------------------------------------
-- Copyright (c) 2019, VeriSilicon Inc. or its affiliates. All rights reserved--
--                                                                            --
-- Permission is hereby granted, free of charge, to any person obtaining a    --
-- copy of this software and associated documentation files (the "Software"), --
-- to deal in the Software without restriction, including without limitation  --
-- the rights to use copy, modify, merge, publish, distribute, sublicense,    --
-- and/or sell copies of the Software, and to permit persons to whom the      --
-- Software is furnished to do so, subject to the following conditions:       --
--                                                                            --
-- The above copyright notice and this permission notice shall be included in --
-- all copies or substantial portions of the Software.                        --
--                                                                            --
-- THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR --
-- IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   --
-- FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE--
-- AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER     --
-- LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING    --
-- FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        --
-- DEALINGS IN THE SOFTWARE.                                                  --
--------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
#ifndef _VSI_STRING_H_
#define _VSI_STRING_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <assert.h>

#if defined(__GNUC__) || defined(__clang__)
#define av_unused __attribute__((unused))
#else
#define av_unused
#endif

#ifdef SAFESTRING
//safestring imp for Intel
#include "safe_lib.h"
#include "snprintf_s.h"

#define strlen(dest) strnlen_s(dest, 512)
//#define wcslen(dest)              wcsnlen_s(dest, 512)

#define strcat(dest, src) strcat_s(dest, 512, src)
//#define strncat(dest,src,n)       strncat_s(dest, 512, src, n)

#define strcpy(dest, src) strcpy_s(dest, 512, src)
#define strncpy(dest, src, n) strncpy_s(dest, 512, src, n)

#define memcpy(dest, src, n) memcpy_s(dest, n, src, n)
#define memmove(dest, src, n) memmove_s(dest, n, src, n)
/* #define memcmp(s1,s2,n)           int ind = 0;\
                                  int rc = 0;\
                                  if (rc = memcmp_s(s1, 512, s2, n, &ind) != EOK ) {\
                                    printf("%s %u  Ind=%d  Error rc=%u \n", __FUNCTION__, __LINE__, ind, rc);\
                                    assert(0);\
                                  }\
                                    return ind; */

#define memset(dest, value, n) memset_s(dest, n, value)

typedef errno_t mem_ret;
av_unused static mem_ret vsi_strcmp(const char *s1, const char *s2)
{
    int ind = 0;
    int rc = 0;
    if ((rc = strcmp_s(s1, RSIZE_MAX_STR, s2, &ind)) != EOK) {
        printf("%s %u  Ind=%d  Error rc=%u \n", __FUNCTION__, __LINE__, ind, rc);
        assert(0);
    }

    return ind;
}

#define strcmp vsi_strcmp
//...
#else
/* For memset, strcpy and strlen */
#include <string.h>

#define snprintf_s_i(dest, n, format, a) snprintf(dest, n, format, a)
#define snprintf_s_si(dest, dmax, format, s, a) snprintf(dest, dmax, format, s, a)
#define snprintf_s_l(dest, dmax, format, a) snprintf(dest, dmax, format, a)
#define snprintf_s_sl(dest, dmax, format, s, a) snprintf(dest, dmax, format, s, a)
#define snprintf_s_s(dest, dmax, format, s) snprintf(dest, dmax, format, s)
#define snprintf_s_sssiii(dest, dmax, format, s1, s2, s3, inta1, inta2, inta3)                     \
    snprintf(dest, dmax, format, s1, s2, s3, inta1, inta2, inta3)

typedef void *mem_ret;

av_unused static int vsi_strcmp(const char *s1, const char *s2)
{
    return strcmp(s1, s2);
}

#endif /* SAFESTRING */

#ifdef __cplusplus
}
#endif

#endif /* _VSI_STRING_H_ */