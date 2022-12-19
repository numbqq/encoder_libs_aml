/*------------------------------------------------------------------------------
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                                                                            --
--                            Verisilicon.                                    --
--                                                                            --
--      In the event of publication, the following notice is applicable:      --
--                                                                            --
--                   (C) COPYRIGHT 2020 HANTRO PRODUCTS OY                    --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--         The entire notice above must be reproduced on all copies.          --
--                                                                            --
--------------------------------------------------------------------------------
--
--  Description : printf log info to ***.log
--
------------------------------------------------------------------------------*/

/**********************************************************
* INCLUDES
**********************************************************/
#include <stdarg.h>

#ifdef VCE_LOGMSG
#include "enc_log.h"
#include "ewl.h"

/**********************************************************
* TYPES
**********************************************************/

/**********************************************************
* MACROS
**********************************************************/
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#ifdef _MSC_VER
#define snprintf _snprintf
#endif

#define VPU_MAX_DEBUG_BUFFER_LEN 1024 * 10
#define FILE_MAX_SIZE (1024 * 1024)
#define BUF_SIZE 1024
#define HSWREG(n) ((n)*4)

static FILE *log_output[STREAM_COUNT] = {0};
char log_out_filename[256] = {0};
static log_env_setting env_log = {LOG_STDOUT, VCENC_LOG_ERROR, 0x003F, 0x0001};

static unsigned int vcenc_log_trace_bitmap[] = {1 << VCENC_LOG_TRACE_API, 1 << VCENC_LOG_TRACE_REGS,
                                                1 << VCENC_LOG_TRACE_EWL, 1 << VCENC_LOG_TRACE_MEM,
                                                1 << VCENC_LOG_TRACE_RC,  1 << VCENC_LOG_TRACE_CML,
                                                1 << VCENC_LOG_TRACE_PERF};
static char *log_trace_str[] = {"api", "regs", "ewl", "mem", "rc", "cml", "perf"};

static unsigned int vcenc_log_check_bitmap[] = {
    1 << VCENC_LOG_CHECK_RECON, 1 << VCENC_LOG_CHECK_QUALITY, 1 << VCENC_LOG_CHECK_VBV,
    1 << VCENC_LOG_CHECK_RC, 1 << VCENC_LOG_CHECK_FEATURE};
static char *log_check_str[] = {"recon", "quality", "vbv", "rc", "feature"};
pthread_mutex_t log_mutex;

void getNameByPid(pid_t pid, char *task_name)
{
    char proc_pid_path[BUF_SIZE];
    char buf[BUF_SIZE];

    sprintf(proc_pid_path, "/proc/%d/status", pid);
    FILE *fp = fopen(proc_pid_path, "r");
    if (NULL != fp) {
        if (fgets(buf, BUF_SIZE - 1, fp) == NULL) {
            fclose(fp);
        }
        fclose(fp);
        sscanf(buf, "%*s %s", task_name);
    }
}

// rainbow fprint
static void _rainbow_fprint(FILE *stream, vcenc_log_level log_level, char *str_content)
{
    switch (log_level) {
    case VCENC_LOG_FATAL:
        fprintf(stream, "%s", str_content);
        break;
    case VCENC_LOG_ERROR:
        fprintf(stream, "%s", str_content);
        break;
    case VCENC_LOG_WARN:
        fprintf(stream, "%s", str_content);
        break;
    case VCENC_LOG_INFO:
        fprintf(stream, "%s", str_content);
        break;
    default:
        fprintf(stream, "%s", str_content);
        break;
    }
}
static char *_get_time_stamp(char *timestamp)
{
    struct timeval tv;
    if (gettimeofday(&tv, NULL) == 0) {
        sprintf(timestamp, "[%04d.%06d]", (unsigned int)tv.tv_sec & 0xffff,
                (unsigned int)tv.tv_usec);
    }
    return timestamp;
}

int VCEncLogInit(unsigned int out_dir, unsigned int out_level, unsigned int trace_map,
                 unsigned int check_map)
{
    FILE *fp1, *fp2;
    static int init_done_flag = -1;

    if (1 == init_done_flag) {
        return 0;
    }

    // _get_log_env();
    env_log.out_dir = (vcenc_log_output)out_dir;
    env_log.out_level = (vcenc_log_level)out_level;
    env_log.k_trace_map = trace_map;
    env_log.k_check_map = check_map;

    if ((LOG_ONE_FILE == env_log.out_dir) && (VCENC_LOG_QUIET != env_log.out_level)) {
        sprintf(log_out_filename, "vcenc_trace_p%d.log", GETPID());
        fp1 = fopen(log_out_filename, "wt");
        sprintf(log_out_filename, "vcenc_check_p%d.log", GETPID());
        fp2 = fopen(log_out_filename, "wt");
        if ((fp1 == NULL) | (fp2 == NULL)) {
            printf("Fail to open LOG file! [%s:%d] \n", __FILE__, __LINE__);
            return -1;
        }
        log_output[STREAM_TRACE_FILE] = fp1;
        log_output[STREAM_CHECK_FILE] = fp2;
    }
    // printf("<<<<< out_dir: 0x%x >>> out_level: 0x%x <<< k_trace_map: 0x%x >>>> k_check_map: 0x%x\n", env_log.out_dir, env_log.out_level, env_log.k_trace_map, env_log.k_check_map);
    pthread_mutex_init(&log_mutex, NULL);
    init_done_flag = 1;

    return 0;
}

int VCEncLogDestroy(void)
{
    if ((LOG_ONE_FILE == env_log.out_dir) && (VCENC_LOG_QUIET != env_log.out_level)) {
        fclose(log_output[STREAM_TRACE_FILE]);
        fclose(log_output[STREAM_CHECK_FILE]);
    }
    pthread_mutex_destroy(&log_mutex);

    return 1;
}

void VCEncTraceMsg(void *inst, vcenc_log_level level, unsigned int log_trace_level, const char *fmt,
                   ...)
{
    char arg_buffer[VPU_MAX_DEBUG_BUFFER_LEN - 128] = {0};
    char msg_buffer[VPU_MAX_DEBUG_BUFFER_LEN] = {0};
    char time_stamp_buffer[128] = {0};
    va_list arg;
    FILE *fLog;
    // static int init_done_flag = -1;

    // if(-1 == init_done_flag)
    // {
    //     if(-1 == VCEncLogInit())
    //     {
    //         printf("VCEnc log init failed, log message will not output properly!\n");
    //         return;
    //     }
    //     init_done_flag = 1;
    // }

    //do nothong if env "VCENC_LOG_LEVEL" not match
    if (level > env_log.out_level) {
        // printf("___________________trace level: %d, env_log.out_level: 0x%x \n", level, env_log.out_level);
        return;
    }
    // do nothing if env "VCENC_LOG_TRACE" not match
    if ((env_log.k_trace_map & vcenc_log_trace_bitmap[log_trace_level]) == 0) {
        // printf("___________________k_trace_map: 0x%x, input_level: 0x%x \n", env_log.k_trace_map, vcenc_log_trace_bitmap[log_trace_level]);
        return;
    }
    va_start(arg, fmt);
    vsnprintf(arg_buffer, VPU_MAX_DEBUG_BUFFER_LEN - 128, fmt, arg);
    va_end(arg);
    // add time stamp
    _get_time_stamp(time_stamp_buffer);
    // printf("==== trace level: 0x%x====== trace bitmap:0x%x===== trace string:%s=============\n", log_trace_level, vcenc_log_trace_bitmap[log_trace_level], log_trace_str[log_trace_level]);

    sprintf(msg_buffer, "[%s]%s[%p]%s", log_trace_str[log_trace_level], time_stamp_buffer, (inst),
            arg_buffer);
    if ((LOG_ONE_FILE == env_log.out_dir) && (VCENC_LOG_QUIET != env_log.out_level)) {
        pthread_mutex_lock(&log_mutex);
        fprintf(log_output[STREAM_TRACE_FILE], "%s", msg_buffer);
        pthread_mutex_unlock(&log_mutex);
        fflush(log_output[STREAM_TRACE_FILE]);
    } else if (LOG_BY_THREAD == env_log.out_dir) {
        //combine the log filename
        sprintf(log_out_filename, "vcenc_trace_p%d_t%lu.log", GETPID(), pthread_self());
        fLog = fopen(log_out_filename, "at+");
        fprintf(fLog, "%s", msg_buffer);
        fclose(fLog);
        fflush(fLog);
    } else if (LOG_STDOUT == env_log.out_dir) {
        _rainbow_fprint(stdout, level, msg_buffer);
        fflush(stdout);
    } else {
        _rainbow_fprint(stderr, level, msg_buffer);
    }

} /* VCEncTraceMsg() */

void VCEncCheckMsg(void *inst, vcenc_log_level level, unsigned int log_check_level, const char *fmt,
                   ...)
{
    char arg_buffer[VPU_MAX_DEBUG_BUFFER_LEN - 128] = {0};
    char msg_buffer[VPU_MAX_DEBUG_BUFFER_LEN] = {0};
    char time_stamp_buffer[128] = {0};
    va_list arg;
    FILE *fLog;
    // static int init_done_flag = -1;

    // if(-1 == init_done_flag)
    // {
    //     VCEncLogInit();
    //     init_done_flag = 1;
    // }

    //do nothong if env "VCENC_LOG_LEVEL" not match
    if (level > env_log.out_level) {
        // printf("KKKKKKKKKK check level: %d, log_check_level: 0x%x KKKKKKKKKKKKKKKKKKKK \n", level, log_check_level);
        return;
    }
    // do nothing if env "VCENC_LOG_CHECK" not match
    if ((env_log.k_check_map & vcenc_log_check_bitmap[log_check_level]) == 0) {
        return;
    }
    va_start(arg, fmt);
    vsnprintf(arg_buffer, VPU_MAX_DEBUG_BUFFER_LEN - 128, fmt, arg);
    va_end(arg);
    // add time stamp
    _get_time_stamp(time_stamp_buffer);
    // printf("========= check level: 0x%x===== check bitmap:0x%x===== check string:%s=============\n", log_check_level, vcenc_log_check_bitmap[log_check_level], log_check_str[log_check_level]);
    sprintf(msg_buffer, "[%s]%s[%p]%s", log_check_str[log_check_level], time_stamp_buffer, (inst),
            arg_buffer);
    if ((LOG_ONE_FILE == env_log.out_dir) && (VCENC_LOG_QUIET != env_log.out_level)) {
        fprintf(log_output[STREAM_CHECK_FILE], "%s", msg_buffer);
        fflush(log_output[STREAM_CHECK_FILE]);
    } else if (LOG_BY_THREAD == env_log.out_dir) {
        //combine the log filename
        sprintf(log_out_filename, "vcenc_check_p%d_t%lu.log", GETPID(), pthread_self());
        fLog = fopen(log_out_filename, "at+");
        fprintf(fLog, "%s", msg_buffer);
        fclose(fLog);
        fflush(fLog);
    } else if (LOG_STDOUT == env_log.out_dir) {
        _rainbow_fprint(stdout, level, msg_buffer);
        fflush(stdout);
    } else {
        _rainbow_fprint(stderr, level, msg_buffer);
    }
} /* VCEncCheckMsg() */

#define REGS_LOGI(inst, fmt, ...)                                                                  \
    VCEncTraceMsg(inst, VCENC_LOG_INFO, VCENC_LOG_TRACE_REGS, "[%s:%d]" fmt, __FUNCTION__,         \
                  __LINE__, ##__VA_ARGS__)
/*------------------------------------------------------------------------------

    EncTraceRegs

------------------------------------------------------------------------------*/
void EncTraceRegs(const void *ewl, unsigned int readWriteFlag, unsigned int mbNum,
                  unsigned int *regs)
{
    int i;
    int lastRegAddr = ASIC_SWREG_AMOUNT * 4; //0x48C;
    char rw = 'W';
    static int frame = 0;

    REGS_LOGI((void *)ewl, "pic=%d\n", frame);
    REGS_LOGI((void *)ewl, "mb=%d\n", mbNum);

    /* After frame is finished, registers are read */
    if (readWriteFlag) {
        rw = 'R';
        frame++;
    }

    /* Dump registers in same denali format as the system model */
    for (i = 0; i < lastRegAddr; i += 4) {
        /* DMV penalty tables not possible to read from ASIC: 0x180-0x27C */
        //if ((i != 0xA0) && (i != 0x38) && (i < 0x180 || i > 0x27C))
        if (i != HSWREG(ASIC_REG_INDEX_STATUS))
            REGS_LOGI((void *)ewl, "%c %08x/%08x\n", rw, i,
                      regs != NULL ? regs[i / 4] : EWLReadReg(ewl, i));
    }

    /* Regs with enable bits last, force encoder enable high for frame start */
    REGS_LOGI((void *)ewl, "%c %08x/%08x\n", rw, HSWREG(ASIC_REG_INDEX_STATUS),
              (regs != NULL ? regs[ASIC_REG_INDEX_STATUS]
                            : EWLReadReg(ewl, HSWREG(ASIC_REG_INDEX_STATUS))) |
                  (readWriteFlag == 0));

    REGS_LOGI((void *)ewl, "\n");

    /*fclose(fRegs);
   * fRegs = NULL; */
}
#endif
