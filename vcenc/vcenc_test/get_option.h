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

#ifndef GET_OPTION_H
#define GET_OPTION_H

#include "base_type.h"
#define LOGSIZEBUFFER 1024 * 10
char *cml_all_log;
//--%s=%d         one option matches the one cml-parameter
//--%s=%d:%d:%d   one option matches the multi-cml
#define MAX_NUM_ALINE 1024
#define CML_OUT(type)                                                                              \
    do {                                                                                           \
        logstr(cml_all_log, cml_log_buffer, cml_all_log_size, "--%s=", option[i].long_opt);        \
        for (int j = 0; j < option[i].multiple_num; j++) {                                         \
            if (j == option[i].multiple_num - 1) {                                                 \
                logstr(cml_all_log, cml_log_buffer, cml_all_log_size, "%d \\\n",                   \
                       *(type *)(((i8 *)cml) + option[i].offset[j]));                              \
                break;                                                                             \
            }                                                                                      \
            logstr(cml_all_log, cml_log_buffer, cml_all_log_size,                                  \
                   "%d:", *(type *)(((i8 *)cml) + option[i].offset[j]));                           \
        }                                                                                          \
    } while (0)

#define MULTIPLE_MAX_NUM 30

typedef enum {
    I32_NUM_TYPE = 0,
    U32_NUM_TYPE,
    U64_NUM_TYPE,
    I16_NUM_TYPE,
    STR_TYPE,
    ENUM_STR_TYPE, // Independent deal with ENUM_STR_TYPE.
    FLOAT_TYPE,
    MIX_TYPE, // one option matches not only a kind of type but also other kinds. Independent deal with MIX_TYPE.
    TODO_TYPE,
} cml_type;

struct option {
    char *long_opt;
    char short_opt;
    i32 enable;
    cml_type data_type;
    i32 multiple_num;
    i32 offset[MULTIPLE_MAX_NUM];
};

struct enum_str_option {
    char *long_opt;
    char *enum_str[MULTIPLE_MAX_NUM];
};

struct parameter {
    i32 cnt;
    char *argument;
    char short_opt;
    char *longOpt;
    i32 enable;
    u32 match_index;
};

i32 get_option(i32 argc, char **argv, struct option *, struct parameter *);
int ParseDelim(char *optArg, char delim);
int HasDelim(char *optArg, char delim);
void help(char *test_app);
i32 Parameter_Get(i32 argc, char **argv, commandLine_s *cml);
i8 cml_log(char *p_argv, commandLine_s *cml);
i32 change_input(struct test_bench *tb);
void default_parameter(commandLine_s *cml);
int parse_stream_cfg(const char *streamcfg, commandLine_s *pcml);
i32 Parameter_Check(commandLine_s *cml);

#endif
