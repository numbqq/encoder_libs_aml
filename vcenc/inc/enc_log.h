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
#ifndef _ENC_LOG_H
#define _ENC_LOG_H

/**********************************************************
* INCLUDES
**********************************************************/
#if defined(__cplusplus)
extern "C" {
#endif

/**
 * \defgroup api_logmsg Log and Message Utility API
 *
 * @{
 */

/** Types of control for log */
enum {
    VCENC_LOG_OUTPUT = 0, /**< For ENV to control output device */
    VCENC_LOG_LEVEL = 1,  /**< For ENV to control output level */
    VCENC_LOG_TRACE = 2,  /**< For ENV to control trace information for prompt and debug */
    VCENC_LOG_CHECK = 3   /**< For ENV to control check information for test only. */
};

/** Values of ENV "VCENC_LOG_OUTPUT" */
typedef enum _vcenc_log_output {
    LOG_STDOUT = 0,    /**< All log output to stdout. */
    LOG_ONE_FILE = 1,  /**< All log use one log file.
                             * \n Trace file name is "vcenc_trace_p${pid}.log".
                             * \n Check file name is "vcenc_check_p${pid}.log". */
    LOG_BY_THREAD = 2, /**< Each thread of each instance has its own log file.
                             * \n Log file name is "vcenc_trace_p${pid}t${tid}.log". */
    LOG_STDERR = 3,    /**< All log output to stderr. */
    LOG_COUNT = 4      /**< Number of output device option. */
} vcenc_log_output;

/** Types of log message */
enum {
    STREAM_TRACE_FILE = 0, /**< Trace output is used for prompt and debug */
    STREAM_CHECK_FILE = 1, /**< Check output is used for testing */
    STREAM_COUNT = 2
};

/** Category for VCENC_LOG_TRACE ENV
 * The correspoing bit defined in ENV VCENC_LOG_TRACE to indicate what kinds of log should be output for debug. */
enum {
    VCENC_LOG_TRACE_API = 0,  /**< Dump API call, replace "-DHEVCENC_TRACE" */
    VCENC_LOG_TRACE_REGS = 1, /**< Dump registers, replace "-DTRACE_REGS" */
    VCENC_LOG_TRACE_EWL = 2,  /**< Dump EWL, replace "-DTRACE_EWL" */
    VCENC_LOG_TRACE_MEM = 3,  /**< Dump memory usage, replace "-DTRACE_MEM_USAGE" */
    VCENC_LOG_TRACE_RC = 4,   /**< Dump Rate Control Status; */
    VCENC_LOG_TRACE_CML = 5,  /**< Output full command line; */
    VCENC_LOG_TRACE_PERF = 6, /**< Output performance information; */
    VCENC_LOG_TRACE_COUNT = 7
};

/** Category for VCENC_LOG_CHECK ENV
 * The correspoing bit defined in ENV VCENC_LOG_CHECK to indicate what kinds of log should be output for test. */
enum {
    VCENC_LOG_CHECK_RECON = 0,   /**< Output recon YUV data. */
    VCENC_LOG_CHECK_QUALITY = 1, /**< Output quality PSNR/SSIM for each frame; */
    VCENC_LOG_CHECK_VBV = 2,     /**< Output VBV information for checking RC; */
    VCENC_LOG_CHECK_RC = 3,      /**< Output RC information for RC profiling; */
    VCENC_LOG_CHECK_FEATURE = 4, /**< Output features for coverage; */
    VCENC_LOG_CHECK_COUNT = 5
};

/** Define the level defined in VCENC_LOG_LEVEL. The message defined smaller than the level will
 * be output */
typedef enum _vcenc_log_level {
    VCENC_LOG_QUIET = 0, /**< "quiet": no output; */
    VCENC_LOG_FATAL = 1, /**< "fatal": serious errors; */
    VCENC_LOG_ERROR = 2, /**< "error": error happens; */
    VCENC_LOG_WARN = 3,  /**< "warn": may not well used; */
    VCENC_LOG_INFO = 4,  /**< "info": provide some information; */
    VCENC_LOG_DEBUG = 5, /**< "debug": provide debug information; */
    VCENC_LOG_ALL = 6,   /**< "": every thing; */
    VCENC_LOG_COUNT = 7
} vcenc_log_level;

/** \brief Record the setting read from ENV */
typedef struct _log_env_setting {
    vcenc_log_output out_dir;
    vcenc_log_level out_level;
    unsigned int k_trace_map;
    unsigned int k_check_map;
} log_env_setting;

#ifdef VCE_LOGMSG
/** Initialize the log message system
 *
 * all parameter get as this order:
 *              1. command line
 *              2. environment setting
 *              3. default value.
 * \param [in] to control log message output device.
 *              follow type definition of "vcenc_log_output"
 * \param [in] to control log message output level, from "QUIET(0)" to "ALL(6)"
 *              follow type definition of "vcenc_log_level"
 * \param [in] to control log message output trace information for prompt and debug. [63]=b`0111111
 *              0 = dump API call.
 *              1 = dump registers.
 *              2 = dump EWL.
 *              3 = dump memory usage.
 *              4 = dump Rate Control Status
 *              5 = output full command line
 *              6 = output performance information
 *
 * \param [in] to control log message output check information for test only. [1]=b`00001
 *              0 = output recon YUV data.
 *              1 = output quality PSNR/SSIM for each frame.
 *              2 = output VBV information for checking RC.
 *              3 = output RC information for RC profiling.
 *              4 = output features for coverage
 */
int VCEncLogInit(unsigned int out_dir, unsigned int out_level, unsigned int trace_map,
                 unsigned int check_map);

/** Print the message according to log level and trace category. trace log is used
 * for information prompt and debug.
 *
 * \param [in] inst The instance the calling belong to.
 * \param [in] level Show the important of the message, from "FATAL(1)" to "DEBUG(5)"
 * \param [in] log_trace_level The bit mask to indicate which category of the information
 *             shall be printed when corresponding bit is 1.
 * \param [in] fmt The format same as "printf"
 * \retunr None.
 */
void VCEncTraceMsg(void *inst, vcenc_log_level level, unsigned int log_trace_level, const char *fmt,
                   ...);

/** Print the message according to log level and check category. check log is used for testing
 *  only.
 *
 * \param [in] inst  The instance the calling belong to.
 * \param [in] level  Show the important of the message, from "FATAL(1)" to "DEBUG(5)"
 * \param [in] log_check_level The bit mask to indicate which category of the information
 *             shall be output to check file when corresponding bit is 1.
 * \param [in] fmt The format same as "printf"
 * \retunr None.
 */
void VCEncCheckMsg(void *inst, vcenc_log_level level, unsigned int log_check_level, const char *fmt,
                   ...);

/** Release related resource used in logging messges */
int VCEncLogDestroy(void);

/** Output registers information, used internally. */
void EncTraceRegs(const void *ewl, unsigned int readWriteFlag, unsigned int mbNum,
                  unsigned int *regs);
#else
#define VCEncLogInit(...)
#define VCEncTraceMsg(...)
#define VCEncCheckMsg(...)
#define VCEncLogDestroy()
#define EncTraceRegs(...)
#endif

/** @} */

#if defined(__cplusplus)
}
#endif

#endif /* _ENC_LOG_H */
