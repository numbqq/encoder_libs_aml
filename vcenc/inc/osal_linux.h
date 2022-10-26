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

#ifndef _OSAL_LINUX_H_
#define _OSAL_LINUX_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _HAVE_PTHREAD_H
#define _HAVE_PTHREAD_H
#endif

#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/syscall.h>
#include <sys/types.h>

#ifdef _HAVE_PTHREAD_H
#include <pthread.h>
#include <semaphore.h>
#include <sched.h>
#include <unistd.h>
#endif /* _HAVE_PTHREAD_H */

#include <fcntl.h>
#include <errno.h>
#include <malloc.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <assert.h>

typedef struct timeval osal_timeval;
typedef struct timezone osal_timezone;

typedef void *(*process_main_ptr)(void *arg);

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

#define _FILE_OFFSET_BITS 64 // for 64 bit fseeko
#define fseek fseeko
#define int16 int16_t
#define int64 int64_t
#define __int64 long long

#ifndef CMODEL_DLL_API
#define CMODEL_DLL_API
#endif

static av_unused int osal_gettimeofday(osal_timeval *tp, void *tzp)
{
    return gettimeofday(tp, (osal_timezone *)tzp);
}

static av_unused void osal_usleep(unsigned long usec)
{
    if (usec == 0)
        sched_yield();
    else
        usleep(usec);
}

static av_unused void osal_open_memstream(struct stream_trace *stream_trace)
{
    stream_trace->fp = open_memstream(&stream_trace->buffer, &stream_trace->size);
}

static av_unused void *osal_aligned_malloc(unsigned int boundary, unsigned int memory_size)
{
    return memalign(boundary, memory_size);
}

static av_unused void osal_aligned_free(void *aligned_momory)
{
    free(aligned_momory);
}

#ifdef _HAVE_PTHREAD_H
static av_unused osal_pid osal_fork(process_main_ptr process_main)
{
    int pid;
    osal_pid osal_pid_t;
    osal_pid_t.tid = NULL;
    if (0 == (pid = fork())) {
        process_main(NULL);
        exit(0);
    } else if (pid > 0) {
        osal_pid_t.pid = pid;
    } else {
        perror("failed to fork new process to process streams");
        exit(pid);
    }
    printf("osal_pid_t.osal_pid is %d\n", osal_pid_t.pid);
    return osal_pid_t;
}

static av_unused void osal_wait(osal_pid wait_pid, int *status)
{
    waitpid(wait_pid.pid, status, 0);
}
#endif

#ifdef __cplusplus
}
#endif

#endif /* _OSAL_LINUX_H_ */
