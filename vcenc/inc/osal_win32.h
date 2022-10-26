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

#ifndef _OSAL_WIN32_H_
#define _OSAL_WIN32_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _HAVE_PTHREAD_H
#define _HAVE_PTHREAD_H
#endif

#ifdef _HAVE_PTHREAD_H
#include "pthread.h"
#include "semaphore.h"
#include "sched.h"
#endif /* _HAVE_PTHREAD_H */

#include <io.h>
#include <process.h>
#include <windows.h>
#include <time.h>

#include <fcntl.h>
#include <errno.h>
#include <malloc.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <assert.h>

#if defined(_MSC_VER)
/* MSVS doesn't define off_t, and uses _f{seek,tell}i64. */
typedef __int64 off_t;
typedef __int64 off64_t;
#define fseeko _fseeki64
#define ftello _ftelli64
#define fseeko64 _fseeki64
#define ftello64 _ftelli64
#elif defined(_WIN32)
/* MinGW defines off_t as long and uses f{seek,tell}o64/off64_t for large files. */
#define fseeko fseeko64
#define ftello ftello64
#define off_t off64_t
#endif
#define snprintf _snprintf
#define open _open
#define close _close
#define read _read
#define write _write
//#define lseek _lseeki64
//#define fsync _commit
//#define tell _telli64

#define NAME_MAX 255
#undef fseek
#define fseek _fseeki64
#define int16 __int16
#define int64 __int64

#define putw _putw

#ifndef CMODEL_DLL_API
#ifdef CMODEL_DLL_SUPPORT /* cmodel in dll */
#ifdef _CMODEL_           /* This macro will be enabled when compiling cmodel libs. */
#define CMODEL_DLL_API __declspec(dllexport)
#else
#define CMODEL_DLL_API __declspec(dllimport)
#endif
#else
/* win non-dll. */
#define CMODEL_DLL_API
#endif
#endif

typedef unsigned long(__stdcall *THREAD_ROUTINE)(void *Argument);
typedef void *(*process_main_ptr)(void *arg);

typedef struct timeval osal_timeval;

struct stream_trace {
    struct node *next;
    char *buffer;
    char comment[256];
    size_t size;
    FILE *fp;
    unsigned int cnt;
};

typedef struct {
    int pid;
    pthread_t *tid;
} osal_pid;

static av_unused int osal_gettimeofday(osal_timeval *tp, void *tzp)
{
    time_t clock;
    struct tm tm;
    SYSTEMTIME wtm;

    GetLocalTime(&wtm);
    tm.tm_year = wtm.wYear - 1900;
    tm.tm_mon = wtm.wMonth - 1;
    tm.tm_mday = wtm.wDay;
    tm.tm_hour = wtm.wHour;
    tm.tm_min = wtm.wMinute;
    tm.tm_sec = wtm.wSecond;
    tm.tm_isdst = -1;
    clock = mktime(&tm);
    tp->tv_sec = (long)clock;
    tp->tv_usec = wtm.wMilliseconds * 1000;

    return 0;
}

static av_unused void osal_usleep(unsigned long usec)
{
    if (usec < 1000)
        Sleep(10);
    else
        Sleep(usec / 1000);
}

static av_unused void osal_open_memstream(struct stream_trace *stream_trace)
{
    int fd;
    HANDLE fm, h;
    fd = _fileno(stream_trace->fp);
    h = (HANDLE)_get_osfhandle(fd);

    fm = CreateFileMapping(h, NULL, PAGE_READWRITE | SEC_RESERVE, 0, 16 * 1024 * 1024, NULL);
    if (fm == NULL) {
        fprintf(stderr, "Could not access memory space! %sn", strerror(GetLastError()));
        exit(GetLastError());
    }

    GetFileSize(h, stream_trace->size);

    stream_trace->buffer = MapViewOfFile(fm, FILE_MAP_ALL_ACCESS, 0, 0, 0);
    if (stream_trace->buffer = NULL) {
        fprintf(stderr, "Could not fill memory space! %sn", strerror(GetLastError()));
        exit(GetLastError());
    }
}

static av_unused void *osal_aligned_malloc(unsigned int boundary, unsigned int memory_size)
{
    return _aligned_malloc(memory_size, boundary);
}

static av_unused void osal_aligned_free(void *aligned_momory)
{
    _aligned_free(aligned_momory);
}

#ifdef _HAVE_PTHREAD_H
static av_unused osal_pid osal_fork(process_main_ptr process_main)
{
    osal_pid osal_pid_t;
    osal_pid_t.pid = 0;
    pthread_t *ptid = (pthread_t *)malloc(sizeof(pthread_t));
    pthread_create(ptid, NULL, process_main, NULL);
    osal_pid_t.tid = ptid;
    return osal_pid_t;
}

static av_unused void osal_wait(osal_pid wait_pid, int *status)
{
    pthread_join(*wait_pid.tid, NULL);
}
#endif

#ifdef __cplusplus
}
#endif

#endif /* _OSAL_WIN32_H_ */
