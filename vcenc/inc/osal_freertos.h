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

#ifndef _OSAL_FREERTOS_H_
#define _OSAL_FREERTOS_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _HAVE_PTHREAD_H
#define _HAVE_PTHREAD_H
#endif

#ifdef _HAVE_PTHREAD_H
#ifdef FREERTOS_SIMULATOR
#if defined(_WIN32)
#include "FreeRTOS_POSIX.h"
#include "FreeRTOS_POSIX/pthread.h"
#include "FreeRTOS_POSIX/semaphore.h"
#include "FreeRTOS_POSIX/sched.h"
#include "FreeRTOS_POSIX/unistd.h"
#include "FreeRTOS_POSIX/errno.h"

#include <io.h>
#include <process.h>
#include <windows.h>
#include <corecrt_wtime.h>

#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <time.h>

#elif defined(__linux__) //Linux Simulator
/***************************************************************************************************
Need to include some basic FreeRTOS header files when Freertos simulator in Linux, at the same time,
the related makefile files need to include the FreeRTOS path, like the below:

Exists variale FreeRTOSDir = software/linux/dwl/osal/freertos/FreeRTOS_Kernel in common.mk
ifeq ($(USE_FREERTOS_SIMULATOR), y)
  INCLUDE += -I$(FreeRTOSDir)/Source/include \
             -I$(FreeRTOSDir)/Source/portable/Linux/GCC/Posix \
             -I$(FreeRTOSDir)/Source/portable/Linux
endif
And need to define the macro "__FREERTOS__"
***************************************************************************************************/
/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "event_groups.h"
#include "semphr.h"
#include "task.h"

#include <unistd.h>
#include <stdlib.h>

#include <sys/wait.h>
#include <sys/time.h>
#include <sys/syscall.h>
#include <sys/types.h>

#include <pthread.h>
#include <semaphore.h>
#include <sched.h>

#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#endif /* _WIN32 */

#else //other os, example FREERTOS (Need macro __FREERTOS__)
#include "FreeRTOS_POSIX.h"
#include "FreeRTOS_POSIX/pthread.h"
#include "FreeRTOS_POSIX/semaphore.h"
#include "FreeRTOS_POSIX/sched.h"
#include "FreeRTOS_POSIX/unistd.h"
#include "FreeRTOS_POSIX/errno.h"
#include "FreeRTOS_POSIX/fcntl.h"
#include "FreeRTOS_POSIX/signal.h"

#define pthread_yield() sched_yield()
#ifdef __linux__
#define _STDLIB_H
#endif

#endif /* FREERTOS_SIMULATOR */
#endif /* _HAVE_PTHREAD_H */

#ifdef FREERTOS_SIMULATOR
#ifdef _WIN32
#include <io.h>
#include <process.h>
#include <windows.h>
#include <time.h>
#elif defined(__linux__) //Linux simulator
#include <sys/time.h>
#include <sys/types.h>
#include <sys/mman.h>
#endif /* _WIN32 */

#else //Not FreeRTOS simulator, FreeRTOS and other os
//#include "FreeRTOS_POSIX/sys/types.h"
#include "FreeRTOS_POSIX/time.h"
#undef USE_POSIX_SEMAPHORE
#define USE_POSIX_SEMAPHORE
#undef USE_SYS_V_IPC
#define USE_SYS_V_IPC
#endif /* FREERTOS_SIMULATOR */

#ifndef FREERTOS_SIMULATOR
//malloc for thread security
#define malloc(a) pvPortMalloc(a)
#define free(a) vPortFree(a)
#else
#include <malloc.h>
#endif /* FREERTOS_SIMULATOR */
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#ifdef FREERTOS_SIMULATOR
#if defined(_WIN32) || defined(__linux__)
typedef struct timeval osal_timeval;
#endif /* defined(_WIN32) || defined(__linux__) */

#else //FreeRTOS
typedef struct osal_timeval_ {
    ssize_t tv_sec; /* seconds */
    long tv_usec;   /* and microseconds */
} osal_timeval;
#define timeval osal_timeval_
#endif /* FREERTOS_SIMULATOR */

struct stream_trace {
    struct node *next;
    char *buffer;
    char comment[256];
    size_t size;
    FILE *fp;
    unsigned int cnt;
};

typedef void *(*process_main_ptr)(void *arg);
typedef struct {
    int pid;
    pthread_t *tid;
} osal_pid;

#ifdef FREERTOS_SIMULATOR
//file operations
#if defined(_MSC_VER)
/* MSVS doesn't define off_t, and uses _f{seek,tell}i64. */
//typedef __int64 off_t;
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

#elif defined(__linux__)
//Nothing to do
#endif /* _MSC_VER */

#else //For FreeRTOS, close all the file operations, maybe can use FreeRTOS+FAT
//typedef long long off_t;
#define off64_t off_t
#endif /* FREERTOS_SIMULATOR */

#ifdef FREERTOS_SIMULATOR
#ifdef _WIN32
#define putw _putw

typedef unsigned long(__stdcall *THREAD_ROUTINE)(void *Argument);
#define snprintf _snprintf
#define open _open
#define close _close
#define read _read
#define write _write
//#define lseek _lseeki64
//#define fsync _commit
//#define tell _telli64
#define NAME_MAX 255

//#undef fseek
#define fseek _fseeki64
#define int16 __int16
#define int64 __int64
#ifndef CMODEL_DLL_API
#ifdef CMODEL_DLL_SUPPORT /* cmodel in dll */
#ifdef _CMODEL_           /* This macro will be enabled when compiling cmodel libs. */
#define CMODEL_DLL_API __declspec(dllexport)
#else
#define CMODEL_DLL_API __declspec(dllimport)
#endif /* _CMODEL_ */
#else
/* win non-dll. */
#define CMODEL_DLL_API
#endif /* CMODEL_DLL_SUPPORT */
#endif /* CMODEL_DLL_API */

#undef PTHREAD_MUTEX_INITIALIZER
#define PTHREAD_MUTEX_INITIALIZER                                                                  \
    {                                                                                              \
        0, 0, 0, 0                                                                                 \
    }
/*
 * pthread_attr_{set}inheritsched
 */
#define PTHREAD_INHERIT_SCHED 0
#define PTHREAD_EXPLICIT_SCHED 1 /* Default */

enum {
    //SCHED_OTHER = 0,
    SCHED_FIFO = 1,
    SCHED_RR,
    SCHED_MIN = SCHED_OTHER,
    SCHED_MAX = SCHED_RR
};

#elif defined(__linux__)
#define _FILE_OFFSET_BITS 64 // for 64 bit fseeko
#define fseek fseeko
#define int16 int16_t
#define int64 int64_t
#define __int64 long long

#ifndef CMODEL_DLL_API
#define CMODEL_DLL_API
#endif
#endif /* _WIN32 */

#else //For FreeRTOS, close all the file operations, maybe can use FreeRTOS+FAT
//#define fseek(a, b, c)
#define int16 short
#define int64 long long
#define __int64 long long

#ifndef CMODEL_DLL_API
#define CMODEL_DLL_API
#endif /* CMODEL_DLL_API */
/*
* pthread_attr_{set}inheritsched
*/
#define PTHREAD_INHERIT_SCHED 0
#define PTHREAD_EXPLICIT_SCHED 1 /* Default */
enum {
    //SCHED_OTHER = 0,
    SCHED_FIFO = 1,
    SCHED_RR,
    SCHED_MIN = SCHED_OTHER,
    SCHED_MAX = SCHED_RR
};

#endif /* FREERTOS_SIMULATOR */

#ifndef FREERTOS_SIMULATOR
#ifdef _HAVE_PTHREAD_H
typedef struct pthread_once {
    unsigned short done;
    pthread_mutex_t once_mutex;
} pthread_once_t;
#define PTHREAD_ONCE_INIT (((pthread_once_t){.done = 0, .once_mutex = PTHREAD_MUTEX_INITIALIZER}))

static av_unused int pthread_condattr_init(pthread_condattr_t *attr)
{
    //TODO...
    (void)attr;

    return 0;
}

static av_unused int pthread_condattr_destroy(pthread_condattr_t *attr)
{
    //TODO...
    (void)attr;

    return 0;
}

static av_unused int pthread_attr_setinheritsched(pthread_attr_t *attr, int inheritsched)
{
    //TODO...
    (void)attr;
    (void)inheritsched;

    return 0;
}

static av_unused int pthread_once(pthread_once_t *once_control, void (*init_routine)(void))
{
#ifdef SEM_REPLACE_MUTEX
    sem_wait(&once_control->once_mutex);
#else
    pthread_mutex_lock(&once_control->once_mutex);
#endif
    if (once_control->done == 0) {
        init_routine();
        once_control->done = 1;
    }
#ifdef SEM_REPLACE_MUTEX
    sem_post(&once_control->once_mutex);
#else
    pthread_mutex_unlock(&once_control->once_mutex);
#endif
    return 0;
}
#endif /* _HAVE_PTHREAD_H */
#endif /* FREERTOS_SIMULATOR */

/*------------------------------------------------------------------------------
 Function name   : osal_gettimeofday
 Description     : open method
 Parameters      : osal_timeval *tp - Used to store  time of the current time
                     included the seconds and nanosecnods
                   void *tzp - not used
 Return type     : int
------------------------------------------------------------------------------*/
static av_unused int osal_gettimeofday(osal_timeval *tp, void *tzp)
{
#ifdef FREERTOS_SIMULATOR
#if defined(_WIN32)
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
#elif defined(__linux__)
    return gettimeofday(tp, tzp);
#endif /* _WIN32 */

#else  //FreeRTOS and other os need to implement  by oneself
    //TODO...
    return 0;
#endif /* FREERTOS_SIMULATOR */
}

static av_unused void osal_usleep(unsigned long usec)
{
    usleep(usec);
}

static av_unused void osal_open_memstream(struct stream_trace *stream_trace)
{
#ifdef FREERTOS_SIMULATOR
#if defined(_WIN32)
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
#else
    stream_trace->fp = open_memstream(&stream_trace->buffer, &stream_trace->size);
#endif /* _WIN32 */
#else  // For other os
#endif /* FREERTOS_SIMULATOR */
}

/* For cmodel */
static av_unused void *osal_aligned_malloc(unsigned int boundary, unsigned int memory_size)
{
#ifdef FREERTOS_SIMULATOR
#ifdef _WIN32
    return _aligned_malloc(memory_size, boundary);
#elif defined(__linux__)
    return memalign(boundary, memory_size);
#endif /* _WIN32 */

#else  //FreeRTOS and other os need to implement by oneself
    return NULL;
#endif /* FREERTOS_SIMULATOR */
}

/* For cmodel */
static av_unused void osal_aligned_free(void *aligned_momory)
{
#ifdef FREERTOS_SIMULATOR
#ifdef _WIN32
    _aligned_free(aligned_momory);
#elif defined(__linux__)
    free(aligned_momory);
#endif /* _WIN32 */

#else  //FreeRTOS and other os need to implement by oneself
    ;
#endif /* FREERTOS_SIMULATOR */
}

#ifdef _HAVE_PTHREAD_H
static av_unused osal_pid osal_fork(process_main_ptr process_main)
{
    printf("FreeRTOS NOT Support multi-process!");
    assert(0);
    //resolve the warning
    osal_pid osal_pid_t = {0, NULL};
    return osal_pid_t;
}

static av_unused void osal_wait(osal_pid wait_pid, int *status)
{
    printf("FreeRTOS NOT Support multi-process!");
    assert(0);
}

#ifdef SEM_REPLACE_MUTEX

#include "dev_common_freertos.h"

static inline signed int sem_imp_mutex_init(sem_t *mutex, const pthread_mutexattr_t *attr)
{
    sem_init(mutex, 0, 1);

    return 0;
}

static inline signed int sem_imp_cond_init(sem_t *cond, const pthread_condattr_t *attr)
{
    sem_init(cond, 0, 0);

    return 0;
}

static inline signed int sem_imp_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex)
{
    if (sem_trywait(cond) != 0) {
        sem_post(mutex);
        sem_wait(cond);
        sem_wait(mutex);
    }

    return 0;
}
#endif
#endif

#ifdef __cplusplus
}
#endif

#endif /* _OSAL_FREERTOS_H_ */
