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
--  Description : Resolution Change Organize
--
------------------------------------------------------------------------------*/

#include <math.h>
#include "osal.h"

#include "vsi_string.h"
#include "base_type.h"
#include "error.h"
#include "hevcencapi.h"
#include "HevcTestBench.h"
#include "enccommon.h"
#include "test_bench_utils.h"
#include "test_bench_pic_config.h"
#ifdef INTERNAL_TEST
#include "sw_test_id.h"
#endif
#ifdef TEST_DATA
#include "enctrace.h"
#endif

/**
\page Resolution Change Encoding

\section input file list Example

Here's an example for input file list file. e.g. as file "refs.lst"

```
-i /workspace/vpusw01/home/cn9057/yuv/soccer_30fps_4cif_704x576.yuv -w704 -h576 -a0 -b30 -o city.264
-i /workspace/vpusw01/home/cn9057/yuv/forman_cif.yuv -w352 -h288 -a0 -b30 -o foreman_cif.264

```

Following command will encode 8 frames according to above reference list.
```
./h264_testencDEV -w704 -h576 -i rush_hour_25fps_w704h576.yuv -b2 -o stream.264 --lookaheadDepth=4 -U1
 --bitPerSecond=2000000 --gopSize=4 --inputFileList=../cfg/input_file_list.cfg
```
Note:
 - The first yuv file to be encoded is rush_hour_25fps_w704h576.yuv, others are list in the input_file_list.cfg file.
 - The max width and width is defined by the first stream, for this command line is 704x576, any yuv file listed in
   the input_file_list.cfg exceed this side is considered to be illegal.
*/

static char *nextTokenMinus(char *str)
{
    char *p = strchr(str, '-');
    if (p) {
        if (*p == '\0')
            p = NULL;
    }
    return p;
}

static void removeSpaceAndReturn(char *str)
{
    int i = 0, j = 0;
    while (1) {
        if (str[i] != ' ' && str[i] != '\n' && str[i] != '\r' && str[i] != '\0')
            str[j++] = str[i];
        else if (str[i] == '\0') {
            str[j] = '\0';
            break;
        }
        i++;
    }
}

/*A link list is invited for the input file list.*/
/* create a link list.*/
static inputFileLinkList *inputFileLinkListCreat(char *in, char *out, u32 a, u32 b, u32 w, u32 h)
{
    inputFileLinkList *head;
    head = (inputFileLinkList *)malloc(sizeof(inputFileLinkList));
    head->in = (char *)malloc(strlen(in) + 1);
    head->out = (char *)malloc(strlen(out) + 1);
    strcpy(head->in, in);
    strcpy(head->out, out);
    head->a = a;
    head->b = b;
    head->w = w;
    head->h = h;
    head->next = NULL;
    return head;
}

/* Insert to tail of link list.*/
static void inputFileLinkListInsertTail(inputFileLinkList *list, char *in, char *out, u32 a, u32 b,
                                        u32 w, u32 h)
{
    inputFileLinkList *newNode;
    while (list != NULL && list->next != NULL) {
        list = list->next;
    }
    if (list != NULL) {
        newNode = (inputFileLinkList *)malloc(sizeof(inputFileLinkList));
        newNode->in = (char *)malloc(strlen(in) + 1);
        newNode->out = (char *)malloc(strlen(out) + 1);
        strcpy(newNode->in, in);
        strcpy(newNode->out, out);
        newNode->a = a;
        newNode->b = b;
        newNode->w = w;
        newNode->h = h;
        newNode->next = NULL;
        list->next = newNode;
    }
}

/* Delete head of link list.*/
inputFileLinkList *inputFileLinkListDeleteHead(inputFileLinkList *list)
{
    if (list == NULL)
        return NULL;
    inputFileLinkList *newHead = list->next; //maybe null
    free(list->in);
    free(list->out);
    free(list);
    return newHead;
}

static int ParseInputFileList(struct test_bench *tb, commandLine_s *cmdl)
{
#define MAX_LINE_LENGTH 1024
    char achParserBuffer[MAX_LINE_LENGTH] = {0};
    char option = 0;
    char tmp[10] = {0};
    char input_dir[MAX_LINE_LENGTH] = {0};
    char output_dir[MAX_LINE_LENGTH] = {0};
    u32 a = 0, b = 0, w = 0, h = 0;

    while (1) {
        if (feof(tb->inputFileList))
            break;

        achParserBuffer[0] = '\0';
        char *line = fgets((char *)achParserBuffer, MAX_LINE_LENGTH, tb->inputFileList);
        if (!line)
            break;

        removeSpaceAndReturn(line);

        while (1) {
            if (line == NULL)
                break;
            line = nextTokenMinus(line);
            if (line != NULL) {
                option = line[1];
                line = line + 2;
                switch (option) {
                case 'i':
                    sscanf(line, "%[^-]", input_dir);
                    break;
                case 'o':
                    sscanf(line, "%[^-]", output_dir);
                    break;
                case 'a':
                    sscanf(line, "%[^-]", tmp);
                    a = atoi((char *)tmp);
                    break;
                case 'b':
                    sscanf(line, "%[^-]", tmp);
                    b = atoi((char *)tmp);
                    break;
                case 'w':
                    sscanf(line, "%[^-]", tmp);
                    w = atoi((char *)tmp);
                    break;
                case 'h':
                    sscanf(line, "%[^-]", tmp);
                    h = atoi((char *)tmp);
                    break;
                default:
                    break;
                }
            }
        }
        if ((char *)input_dir == NULL || (char *)output_dir == NULL || a > b || w <= 0 || h <= 0) {
            printf("Parse Input FileList ERROR.\n");
            return -1;
        }
        if (tb->inputFileLinkListNum == 0) {
            tb->inputFileLinkListP = inputFileLinkListCreat(input_dir, output_dir, a, b, w, h);
        } else {
            inputFileLinkListInsertTail(tb->inputFileLinkListP, input_dir, output_dir, a, b, w, h);
        }
        tb->inputFileLinkListNum++;
    }

    fclose(tb->inputFileList);
    return 0;
}

int getInputFileList(struct test_bench *tb, commandLine_s *cmdl)
{
    int ret = 0;
    tb->inputFileLinkListNum = 0;
    ret = ParseInputFileList(tb, cmdl);
    return ret;
}
