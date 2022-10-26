/*------------------------------------------------------------------------------
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                                                                            --
--                            Verisilicon.                                    --
--                                                                            --
--      In the event of publication, the following notice is applicable:      --
--                                                                            --
--                   (C) COPYRIGHT 2014 HANTRO PRODUCTS OY                    --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--         The entire notice above must be reproduced on all copies.          --
--                                                                            --
--------------------------------------------------------------------------------*/
#include <math.h>
#include "osal.h"

#include "vsi_string.h"
#include "base_type.h"
#include "error.h"
#include "hevcencapi.h"
#include "HevcTestBench.h"
#ifdef TEST_DATA
#include "enctrace.h"
#endif
#ifdef INTERNAL_TEST
#include "sw_test_id.h"
#endif
#include "encinputlinebuffer.h"
#include "enccommon.h"
#include "test_bench_utils.h"
#include "get_option.h"
#include "test_bench_pic_config.h"
#include "test_bench_statistic.h"

#define HEVCERR_OUTPUT stdout
#define MAX_GOP_LEN 300

/* The 2 defines below should be reconsidered. */
#ifndef LEAST_MONITOR_FRAME
#define LEAST_MONITOR_FRAME 3
#endif
#ifndef ENCH2_SLICE_READY_INTERRUPT
#define ENCH2_SLICE_READY_INTERRUPT 1
#endif

typedef struct {
    int argc;
    char **argv;
} MainArgs;

#ifdef __FREERTOS__

#ifndef FREERTOS_SIMULATOR
#include "user_freertos.h"
#endif

typedef void *RET_TYPE;
u8 user_freertos_vcmd_en = 1; //used for use_freertos to switch normal_driver or vcmd_driver
static pthread_t tid_task;
#else
typedef int RET_TYPE;
#endif /* __FREERTOS__ */

RET_TYPE MainTask(void *args);
static void *thread_main(void *arg);
static void *process_main(void *arg);
static int run_instance(commandLine_s *cml);
static i32 encode(struct test_bench *tb, VCEncInst encoder, commandLine_s *cml);
static int OpenEncoder(commandLine_s *cml, VCEncInst *pEnc, struct test_bench *tb);
static void CloseEncoder(VCEncInst encoder, struct test_bench *tb);
static int AllocRes(commandLine_s *cmdl, VCEncInst enc, struct test_bench *tb);
static void FreeRes(struct test_bench *tb);
static void HEVCSliceReady(VCEncSliceReady *slice);
static i32 CheckArea(VCEncPictureArea *area, commandLine_s *cml);
static i32 InitInputLineBuffer(inputLineBufferCfg *lineBufCfg, commandLine_s *cml, VCEncIn *encIn,
                               VCEncInst inst, struct test_bench *tb);
static void EncStreamSegmentReady(void *cb_data);
static ReEncodeStrategy ReEncodeCurrentFrame(const VCEncStatisticOut *stat,
                                             NewEncodeParams *new_params);
static void Multimode_Release(commandLine_s *cml);
static i32 allocVariableMem(struct test_bench *tb, commandLine_s *cml);
static void FreeVariableMem(struct test_bench *tb);

/*-----------
  * main
  *-----------*/

commandLine_s *all_pcml = NULL;
commandLine_s *glob_cml = NULL;

int main(i32 argc, char **argv)
{
    osal_thread_init();
    MainArgs args = {argc, argv};
#ifdef __FREERTOS__
#ifndef FREERTOS_SIMULATOR
    Platform_init();
#endif
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&tid_task, &attr, &MainTask, &args);
    vTaskStartScheduler();

    return 0; //should not arrive here if start scheduler successfully in the real env
#else
    return MainTask(&args);
#endif
}

RET_TYPE MainTask(void *args)
{
    MainArgs *args_ = (MainArgs *)args;
    int argc = args_->argc;
    char **argv = args_->argv;

    i32 ret = OK;
    commandLine_s cml, *pcml;
    VCEncApiVersion apiVer;
    VCEncBuild encBuild;
    encBuild.swBuild = encBuild.hwBuild = 0;

    pthread_attr_t attr;
    osal_pid osal_pid_t;
    int pid;
    int status;
    int i;
    u32 client_type;

    apiVer = VCEncGetApiVersion();

    fprintf(stdout, "VC8000 Encoder API version %d.%d.%d", apiVer.major, apiVer.minor,
            apiVer.clnum);
    fprintf(stdout, ", Build at %s %s", __DATE__, __TIME__);
#ifdef P4CL
    fprintf(stdout, ", Based on CL %d", P4CL);
#endif
    fprintf(stdout, ".\n\n");

    /** Parse parameter */
    if (argc < 2) {
        help(argv[0]);
        exit(0);
    }

    default_parameter(&cml);

    if (Parameter_Get(argc, argv, &cml)) {
        Error(2, ERR, "Input parameter error");
        ret = NOK;
        goto return_;
    }
    // printf("log_dir=%d, log_level=%d, trace_map=0x%x, check_map=0x%x",cml.logOutDir, cml.logOutLevel, cml.logTraceMap, cml.logCheckMap);
    if (cml_log(argv[0], &cml) == NOK) {
        Error(2, ERR, "Output CML LOG error !");
        ret = NOK;
        goto return_;
    }

    client_type = IS_H264(cml.codecFormat) ? EWL_CLIENT_TYPE_H264_ENC : EWL_CLIENT_TYPE_HEVC_ENC;
    encBuild = VCEncGetBuild(client_type);
    fprintf(stdout, "HW ID:  0x%08x\t SW Build: %u.%u.%u\n", encBuild.hwBuild,
            encBuild.swBuild / 1000000, (encBuild.swBuild / 1000) % 1000, encBuild.swBuild % 1000);

    /**
   * "multimode" is used to test multiple encoder instances running in multiple processes or threads.
   * User could create multiple encoders getting from different input parameters files
   */

    if (cml.nstream > 0) {
        /* Multi-processes/threads encoder enabled*/
        if (cml.multimode == 1) {
            /* multi thread mode */
            for (i = 0; i < cml.nstream; i++) {
                cml.streamcml[i] = (commandLine_s *)malloc(sizeof(commandLine_s));
                if (parse_stream_cfg(cml.streamcfg[i], cml.streamcml[i])) {
                    fprintf(stderr, "Found invalid input parameter, ignore task --streamcfg%s!\n",
                            cml.streamcfg[i]);
                    cml.pid[i] = 0;
                    cml.tid[i] = NULL;
                    continue;
                }

                cml.streamcml[i]->outReconFrame = 0;

                pthread_t *tid = (pthread_t *)malloc(sizeof(pthread_t));
                pthread_attr_init(&attr);
                pthread_create(tid, &attr, &thread_main, cml.streamcml[i]);
                cml.tid[i] = tid;
            }
        } else if (cml.multimode == 2) {
            /* multi process mode */
            for (i = 0; i < cml.nstream; i++) {
                pcml = cml.streamcml[i] = (commandLine_s *)malloc(sizeof(commandLine_s));
                if (parse_stream_cfg(cml.streamcfg[i], pcml)) {
                    fprintf(stderr, "Found invalid input parameter, ignore task --streamcfg%s!\n",
                            cml.streamcfg[i]);
                    cml.pid[i] = 0;
                    cml.tid[i] = NULL;
                    continue;
                }

                pcml->recon = malloc(255);
                sprintf(pcml->recon, "deblock%d.yuv", i + 1);
                all_pcml = pcml;
                glob_cml = &cml;
                osal_pid_t = osal_fork(&process_main);
                if ((osal_pid_t.tid == NULL) && (osal_pid_t.pid > 0)) {
                    cml.pid[i] = osal_pid_t.pid;
                    cml.tid[i] = NULL;
                } else
                    cml.tid[i] = osal_pid_t.tid;
            }
        } else {
            if (cml.multimode == 0) {
                printf("multi-stream disabled, ignore extra stream configs\n");
            } else {
                printf("Invalid multi stream mode\n");
                exit(-1);
            }
        }
    }
#ifndef VIDEOSTAB_ENABLED
    cml.videoStab = DEFAULT;
#endif

    cml.argc = argc;
    cml.argv = argv;
    ret = run_instance(&cml);
    if (cml.multimode == 1) {
        for (i = 0; i < cml.nstream; i++) {
            if (cml.tid[i] != NULL)
                pthread_join(*cml.tid[i], NULL);
        }
    }

    else if (cml.multimode == 2) {
        for (i = 0; i < cml.nstream; i++) {
            if ((cml.pid[i] > 0) || (cml.tid[i] != NULL)) {
                osal_pid wait_pid_t;
                wait_pid_t.pid = cml.pid[i];
                wait_pid_t.tid = cml.tid[i];
                osal_wait(wait_pid_t, &status);
            }
        }
    }
    if (cml.multimode != 0) {
        Multimode_Release(&cml);
    }
    EWLDetach();
return_:
#ifdef __FREERTOS__
#ifdef FREERTOS_SIMULATOR
    vTaskEndScheduler(); // need to open in simulator, and need to close on real env
#endif
    (void)(ret);

    return (void *)NULL;
#else
    return ret;
#endif
}

void *thread_main(void *arg)
{
    run_instance((commandLine_s *)arg);
    pthread_exit(NULL);
    return NULL;
}

void *process_main(void *arg)
{
    run_instance(all_pcml);
    EWLDetach();
    Multimode_Release(glob_cml);
    return NULL;
}

void SetTbParams(struct test_bench *tb, commandLine_s *cml)
{
    tb->input = cml->input;
    tb->halfDsInput = cml->halfDsInput;
    tb->output = cml->output;
    tb->dec400TableFile = cml->dec400CompTableinput;
    tb->firstPic = cml->firstPic;
    tb->lastPic = cml->lastPic;
    tb->inputRateNumer = cml->inputRateNumer;
    tb->inputRateDenom = cml->inputRateDenom;
    tb->outputRateNumer = cml->outputRateNumer;
    tb->outputRateDenom = cml->outputRateDenom;
    tb->test_data_files = cml->test_data_files;
    tb->width = cml->width;
    tb->height = cml->height;
    tb->input_alignment =
        (cml->exp_of_input_alignment == 0 ? 0 : (1 << cml->exp_of_input_alignment));
    tb->ref_alignment = (cml->exp_of_ref_alignment == 0 ? 0 : (1 << cml->exp_of_ref_alignment));
    tb->ref_ch_alignment =
        (cml->exp_of_ref_ch_alignment == 0 ? 0 : (1 << cml->exp_of_ref_ch_alignment));
    tb->formatCustomizedType = cml->formatCustomizedType;
    tb->idr_interval = cml->intraPicRate;
    tb->byteStream = cml->byteStream;
    tb->interlacedFrame = cml->interlacedFrame;
    tb->tileColumnNum = cml->num_tile_columns;
    tb->tileRowNum = cml->num_tile_rows;
    tb->tileEnable = cml->tiles_enabled_flag;
    tb->parallelCoreNum = cml->parallelCoreNum;
    tb->outbuf_cnt = tb->parallelCoreNum;
    tb->encIn.gopConfig.idr_interval = tb->idr_interval;
    tb->encIn.gopConfig.firstPic = tb->firstPic;
    tb->encIn.gopConfig.lastPic = tb->lastPic;
    tb->encIn.gopConfig.outputRateNumer = tb->outputRateNumer; /* Output frame rate numerator */
    tb->encIn.gopConfig.outputRateDenom = tb->outputRateDenom; /* Output frame rate denominator */
    tb->encIn.gopConfig.inputRateNumer = tb->inputRateNumer;   /* Input frame rate numerator */
    tb->encIn.gopConfig.inputRateDenom = tb->inputRateDenom;   /* Input frame rate denominator */
    tb->encIn.gopConfig.gopLowdelay = cml->gopLowdelay;
    tb->encIn.gopConfig.interlacedFrame = tb->interlacedFrame;
}

int SetCodingCtrl(struct test_bench *tb, VCEncInst encoder, commandLine_s *cml)
{
    VCEncCodingCtrl codingCfg;
    i32 i;
    VCEncRet ret;

    /* Encoder setup: coding control */
    if ((ret = VCEncGetCodingCtrl(encoder, &codingCfg)) != VCENC_OK) {
        //PrintErrorValue("VCEncGetCodingCtrl() failed.", ret);
        return -1;
    } else {
        if (cml->sliceSize != DEFAULT)
            codingCfg.sliceSize = cml->sliceSize;
        if (cml->enableCabac != DEFAULT)
            codingCfg.enableCabac = cml->enableCabac;
        if (cml->cabacInitFlag != DEFAULT)
            codingCfg.cabacInitFlag = cml->cabacInitFlag;
        codingCfg.vuiVideoFullRange = 0;
        if (cml->videoRange != DEFAULT)
            codingCfg.vuiVideoFullRange = cml->videoRange;
        if (cml->enableRdoQuant != DEFAULT)
            codingCfg.enableRdoQuant = cml->enableRdoQuant;
        if (cml->layerInRefIdc != DEFAULT)
            codingCfg.layerInRefIdcEnable = cml->layerInRefIdc;

        codingCfg.sramPowerdownDisable = cml->sramPowerdownDisable;
        codingCfg.disableDeblockingFilter = (cml->disableDeblocking != 0);
        codingCfg.tc_Offset = cml->tc_Offset;
        codingCfg.beta_Offset = cml->beta_Offset;
        codingCfg.enableSao = cml->enableSao;
        codingCfg.enableDeblockOverride = cml->enableDeblockOverride;
        codingCfg.deblockOverride = cml->deblockOverride;
        codingCfg.enableDynamicRdo = cml->dynamicRdoEnable;
        codingCfg.dynamicRdoCu16Bias = cml->dynamicRdoCu16Bias;
        codingCfg.dynamicRdoCu16Factor = cml->dynamicRdoCu16Factor;
        codingCfg.dynamicRdoCu32Bias = cml->dynamicRdoCu32Bias;
        codingCfg.dynamicRdoCu32Factor = cml->dynamicRdoCu32Factor;

        if (cml->sei)
            codingCfg.seiMessages = 1;
        else
            codingCfg.seiMessages = 0;

        codingCfg.gdrDuration = cml->gdrDuration;
        codingCfg.fieldOrder = cml->fieldOrder;

        codingCfg.cirStart = cml->cirStart;
        codingCfg.cirInterval = cml->cirInterval;

        codingCfg.intraArea.top = cml->intraAreaTop;
        codingCfg.intraArea.left = cml->intraAreaLeft;
        codingCfg.intraArea.bottom = cml->intraAreaBottom;
        codingCfg.intraArea.right = cml->intraAreaRight;
        codingCfg.intraArea.enable = CheckArea(&codingCfg.intraArea, cml);

        codingCfg.pcm_loop_filter_disabled_flag = cml->pcm_loop_filter_disabled_flag;

        codingCfg.ipcm1Area.top = cml->ipcm1AreaTop;
        codingCfg.ipcm1Area.left = cml->ipcm1AreaLeft;
        codingCfg.ipcm1Area.bottom = cml->ipcm1AreaBottom;
        codingCfg.ipcm1Area.right = cml->ipcm1AreaRight;
        codingCfg.ipcm1Area.enable = CheckArea(&codingCfg.ipcm1Area, cml);

        codingCfg.ipcm2Area.top = cml->ipcm2AreaTop;
        codingCfg.ipcm2Area.left = cml->ipcm2AreaLeft;
        codingCfg.ipcm2Area.bottom = cml->ipcm2AreaBottom;
        codingCfg.ipcm2Area.right = cml->ipcm2AreaRight;
        codingCfg.ipcm2Area.enable = CheckArea(&codingCfg.ipcm2Area, cml);

        codingCfg.ipcm3Area.top = cml->ipcm3AreaTop;
        codingCfg.ipcm3Area.left = cml->ipcm3AreaLeft;
        codingCfg.ipcm3Area.bottom = cml->ipcm3AreaBottom;
        codingCfg.ipcm3Area.right = cml->ipcm3AreaRight;
        codingCfg.ipcm3Area.enable = CheckArea(&codingCfg.ipcm3Area, cml);

        codingCfg.ipcm4Area.top = cml->ipcm4AreaTop;
        codingCfg.ipcm4Area.left = cml->ipcm4AreaLeft;
        codingCfg.ipcm4Area.bottom = cml->ipcm4AreaBottom;
        codingCfg.ipcm4Area.right = cml->ipcm4AreaRight;
        codingCfg.ipcm4Area.enable = CheckArea(&codingCfg.ipcm4Area, cml);

        codingCfg.ipcm5Area.top = cml->ipcm5AreaTop;
        codingCfg.ipcm5Area.left = cml->ipcm5AreaLeft;
        codingCfg.ipcm5Area.bottom = cml->ipcm5AreaBottom;
        codingCfg.ipcm5Area.right = cml->ipcm5AreaRight;
        codingCfg.ipcm5Area.enable = CheckArea(&codingCfg.ipcm5Area, cml);

        codingCfg.ipcm6Area.top = cml->ipcm6AreaTop;
        codingCfg.ipcm6Area.left = cml->ipcm6AreaLeft;
        codingCfg.ipcm6Area.bottom = cml->ipcm6AreaBottom;
        codingCfg.ipcm6Area.right = cml->ipcm6AreaRight;
        codingCfg.ipcm6Area.enable = CheckArea(&codingCfg.ipcm6Area, cml);

        codingCfg.ipcm7Area.top = cml->ipcm7AreaTop;
        codingCfg.ipcm7Area.left = cml->ipcm7AreaLeft;
        codingCfg.ipcm7Area.bottom = cml->ipcm7AreaBottom;
        codingCfg.ipcm7Area.right = cml->ipcm7AreaRight;
        codingCfg.ipcm7Area.enable = CheckArea(&codingCfg.ipcm7Area, cml);

        codingCfg.ipcm8Area.top = cml->ipcm8AreaTop;
        codingCfg.ipcm8Area.left = cml->ipcm8AreaLeft;
        codingCfg.ipcm8Area.bottom = cml->ipcm8AreaBottom;
        codingCfg.ipcm8Area.right = cml->ipcm8AreaRight;
        codingCfg.ipcm8Area.enable = CheckArea(&codingCfg.ipcm8Area, cml);

        codingCfg.ipcmMapEnable = cml->ipcmMapEnable;
        codingCfg.pcm_enabled_flag =
            (codingCfg.ipcm1Area.enable || codingCfg.ipcm2Area.enable ||
             codingCfg.ipcm3Area.enable || codingCfg.ipcm4Area.enable ||
             codingCfg.ipcm5Area.enable || codingCfg.ipcm6Area.enable ||
             codingCfg.ipcm7Area.enable || codingCfg.ipcm8Area.enable || codingCfg.ipcmMapEnable);

        codingCfg.roi1Area.top = cml->roi1AreaTop;
        codingCfg.roi1Area.left = cml->roi1AreaLeft;
        codingCfg.roi1Area.bottom = cml->roi1AreaBottom;
        codingCfg.roi1Area.right = cml->roi1AreaRight;
        if (CheckArea(&codingCfg.roi1Area, cml) && (cml->roi1DeltaQp || (cml->roi1Qp >= 0)))
            codingCfg.roi1Area.enable = 1;
        else
            codingCfg.roi1Area.enable = 0;

        codingCfg.roi2Area.top = cml->roi2AreaTop;
        codingCfg.roi2Area.left = cml->roi2AreaLeft;
        codingCfg.roi2Area.bottom = cml->roi2AreaBottom;
        codingCfg.roi2Area.right = cml->roi2AreaRight;
        if (CheckArea(&codingCfg.roi2Area, cml) && (cml->roi2DeltaQp || (cml->roi2Qp >= 0)))
            codingCfg.roi2Area.enable = 1;
        else
            codingCfg.roi2Area.enable = 0;

        codingCfg.roi3Area.top = cml->roi3AreaTop;
        codingCfg.roi3Area.left = cml->roi3AreaLeft;
        codingCfg.roi3Area.bottom = cml->roi3AreaBottom;
        codingCfg.roi3Area.right = cml->roi3AreaRight;
        if (CheckArea(&codingCfg.roi3Area, cml) && (cml->roi3DeltaQp || (cml->roi3Qp >= 0)))
            codingCfg.roi3Area.enable = 1;
        else
            codingCfg.roi3Area.enable = 0;

        codingCfg.roi4Area.top = cml->roi4AreaTop;
        codingCfg.roi4Area.left = cml->roi4AreaLeft;
        codingCfg.roi4Area.bottom = cml->roi4AreaBottom;
        codingCfg.roi4Area.right = cml->roi4AreaRight;
        if (CheckArea(&codingCfg.roi4Area, cml) && (cml->roi4DeltaQp || (cml->roi4Qp >= 0)))
            codingCfg.roi4Area.enable = 1;
        else
            codingCfg.roi4Area.enable = 0;

        codingCfg.roi5Area.top = cml->roi5AreaTop;
        codingCfg.roi5Area.left = cml->roi5AreaLeft;
        codingCfg.roi5Area.bottom = cml->roi5AreaBottom;
        codingCfg.roi5Area.right = cml->roi5AreaRight;
        if (CheckArea(&codingCfg.roi5Area, cml) && (cml->roi5DeltaQp || (cml->roi5Qp >= 0)))
            codingCfg.roi5Area.enable = 1;
        else
            codingCfg.roi5Area.enable = 0;

        codingCfg.roi6Area.top = cml->roi6AreaTop;
        codingCfg.roi6Area.left = cml->roi6AreaLeft;
        codingCfg.roi6Area.bottom = cml->roi6AreaBottom;
        codingCfg.roi6Area.right = cml->roi6AreaRight;
        if (CheckArea(&codingCfg.roi6Area, cml) && (cml->roi6DeltaQp || (cml->roi6Qp >= 0)))
            codingCfg.roi6Area.enable = 1;
        else
            codingCfg.roi6Area.enable = 0;

        codingCfg.roi7Area.top = cml->roi7AreaTop;
        codingCfg.roi7Area.left = cml->roi7AreaLeft;
        codingCfg.roi7Area.bottom = cml->roi7AreaBottom;
        codingCfg.roi7Area.right = cml->roi7AreaRight;
        if (CheckArea(&codingCfg.roi7Area, cml) && (cml->roi7DeltaQp || (cml->roi7Qp >= 0)))
            codingCfg.roi7Area.enable = 1;
        else
            codingCfg.roi7Area.enable = 0;

        codingCfg.roi8Area.top = cml->roi8AreaTop;
        codingCfg.roi8Area.left = cml->roi8AreaLeft;
        codingCfg.roi8Area.bottom = cml->roi8AreaBottom;
        codingCfg.roi8Area.right = cml->roi8AreaRight;
        if (CheckArea(&codingCfg.roi8Area, cml) && (cml->roi8DeltaQp || (cml->roi8Qp >= 0)))
            codingCfg.roi8Area.enable = 1;
        else
            codingCfg.roi8Area.enable = 0;

        codingCfg.roi1DeltaQp = cml->roi1DeltaQp;
        codingCfg.roi2DeltaQp = cml->roi2DeltaQp;
        codingCfg.roi3DeltaQp = cml->roi3DeltaQp;
        codingCfg.roi4DeltaQp = cml->roi4DeltaQp;
        codingCfg.roi5DeltaQp = cml->roi5DeltaQp;
        codingCfg.roi6DeltaQp = cml->roi6DeltaQp;
        codingCfg.roi7DeltaQp = cml->roi7DeltaQp;
        codingCfg.roi8DeltaQp = cml->roi8DeltaQp;
        codingCfg.roi1Qp = cml->roi1Qp;
        codingCfg.roi2Qp = cml->roi2Qp;
        codingCfg.roi3Qp = cml->roi3Qp;
        codingCfg.roi4Qp = cml->roi4Qp;
        codingCfg.roi5Qp = cml->roi5Qp;
        codingCfg.roi6Qp = cml->roi6Qp;
        codingCfg.roi7Qp = cml->roi7Qp;
        codingCfg.roi8Qp = cml->roi8Qp;

        if (codingCfg.cirInterval)
            printf("  CIR: %d %d\n", codingCfg.cirStart, codingCfg.cirInterval);

        if (codingCfg.intraArea.enable)
            printf("  IntraArea: %dx%d-%dx%d\n", codingCfg.intraArea.left, codingCfg.intraArea.top,
                   codingCfg.intraArea.right, codingCfg.intraArea.bottom);

        if (codingCfg.ipcm1Area.enable)
            printf("  IPCM1Area: %dx%d-%dx%d\n", codingCfg.ipcm1Area.left, codingCfg.ipcm1Area.top,
                   codingCfg.ipcm1Area.right, codingCfg.ipcm1Area.bottom);

        if (codingCfg.ipcm2Area.enable)
            printf("  IPCM2Area: %dx%d-%dx%d\n", codingCfg.ipcm2Area.left, codingCfg.ipcm2Area.top,
                   codingCfg.ipcm2Area.right, codingCfg.ipcm2Area.bottom);

        if (codingCfg.ipcm3Area.enable)
            printf("  IPCM3Area: %dx%d-%dx%d\n", codingCfg.ipcm3Area.left, codingCfg.ipcm3Area.top,
                   codingCfg.ipcm3Area.right, codingCfg.ipcm3Area.bottom);

        if (codingCfg.ipcm4Area.enable)
            printf("  IPCM4Area: %dx%d-%dx%d\n", codingCfg.ipcm4Area.left, codingCfg.ipcm4Area.top,
                   codingCfg.ipcm4Area.right, codingCfg.ipcm4Area.bottom);

        if (codingCfg.ipcm5Area.enable)
            printf("  IPCM5Area: %dx%d-%dx%d\n", codingCfg.ipcm5Area.left, codingCfg.ipcm5Area.top,
                   codingCfg.ipcm5Area.right, codingCfg.ipcm5Area.bottom);

        if (codingCfg.ipcm6Area.enable)
            printf("  IPCM6Area: %dx%d-%dx%d\n", codingCfg.ipcm6Area.left, codingCfg.ipcm6Area.top,
                   codingCfg.ipcm6Area.right, codingCfg.ipcm6Area.bottom);

        if (codingCfg.ipcm7Area.enable)
            printf("  IPCM7Area: %dx%d-%dx%d\n", codingCfg.ipcm7Area.left, codingCfg.ipcm7Area.top,
                   codingCfg.ipcm7Area.right, codingCfg.ipcm7Area.bottom);

        if (codingCfg.ipcm8Area.enable)
            printf("  IPCM8Area: %dx%d-%dx%d\n", codingCfg.ipcm8Area.left, codingCfg.ipcm8Area.top,
                   codingCfg.ipcm8Area.right, codingCfg.ipcm8Area.bottom);

        if (codingCfg.roi1Area.enable)
            printf("  ROI 1: %s %d  %dx%d-%dx%d\n", codingCfg.roi1Qp >= 0 ? "QP" : "QP Delta",
                   codingCfg.roi1Qp >= 0 ? codingCfg.roi1Qp : codingCfg.roi1DeltaQp,
                   codingCfg.roi1Area.left, codingCfg.roi1Area.top, codingCfg.roi1Area.right,
                   codingCfg.roi1Area.bottom);

        if (codingCfg.roi2Area.enable)
            printf("  ROI 2: %s %d  %dx%d-%dx%d\n", codingCfg.roi2Qp >= 0 ? "QP" : "QP Delta",
                   codingCfg.roi2Qp >= 0 ? codingCfg.roi2Qp : codingCfg.roi2DeltaQp,
                   codingCfg.roi2Area.left, codingCfg.roi2Area.top, codingCfg.roi2Area.right,
                   codingCfg.roi2Area.bottom);

        if (codingCfg.roi3Area.enable)
            printf("  ROI 3: %s %d  %dx%d-%dx%d\n", codingCfg.roi3Qp >= 0 ? "QP" : "QP Delta",
                   codingCfg.roi3Qp >= 0 ? codingCfg.roi3Qp : codingCfg.roi3DeltaQp,
                   codingCfg.roi3Area.left, codingCfg.roi3Area.top, codingCfg.roi3Area.right,
                   codingCfg.roi3Area.bottom);

        if (codingCfg.roi4Area.enable)
            printf("  ROI 4: %s %d  %dx%d-%dx%d\n", codingCfg.roi4Qp >= 0 ? "QP" : "QP Delta",
                   codingCfg.roi4Qp >= 0 ? codingCfg.roi4Qp : codingCfg.roi4DeltaQp,
                   codingCfg.roi4Area.left, codingCfg.roi4Area.top, codingCfg.roi4Area.right,
                   codingCfg.roi4Area.bottom);

        if (codingCfg.roi5Area.enable)
            printf("  ROI 5: %s %d  %dx%d-%dx%d\n", codingCfg.roi5Qp >= 0 ? "QP" : "QP Delta",
                   codingCfg.roi5Qp >= 0 ? codingCfg.roi5Qp : codingCfg.roi5DeltaQp,
                   codingCfg.roi5Area.left, codingCfg.roi5Area.top, codingCfg.roi5Area.right,
                   codingCfg.roi5Area.bottom);

        if (codingCfg.roi6Area.enable)
            printf("  ROI 6: %s %d  %dx%d-%dx%d\n", codingCfg.roi6Qp >= 0 ? "QP" : "QP Delta",
                   codingCfg.roi6Qp >= 0 ? codingCfg.roi6Qp : codingCfg.roi6DeltaQp,
                   codingCfg.roi6Area.left, codingCfg.roi6Area.top, codingCfg.roi6Area.right,
                   codingCfg.roi6Area.bottom);

        if (codingCfg.roi7Area.enable)
            printf("  ROI 7: %s %d  %dx%d-%dx%d\n", codingCfg.roi7Qp >= 0 ? "QP" : "QP Delta",
                   codingCfg.roi7Qp >= 0 ? codingCfg.roi7Qp : codingCfg.roi7DeltaQp,
                   codingCfg.roi7Area.left, codingCfg.roi7Area.top, codingCfg.roi7Area.right,
                   codingCfg.roi7Area.bottom);

        if (codingCfg.roi8Area.enable)
            printf("  ROI 8: %s %d  %dx%d-%dx%d\n", codingCfg.roi8Qp >= 0 ? "QP" : "QP Delta",
                   codingCfg.roi8Qp >= 0 ? codingCfg.roi8Qp : codingCfg.roi8DeltaQp,
                   codingCfg.roi8Area.left, codingCfg.roi8Area.top, codingCfg.roi8Area.right,
                   codingCfg.roi8Area.bottom);

        codingCfg.roiMapDeltaQpEnable = cml->roiMapDeltaQpEnable;
        codingCfg.roiMapDeltaQpBlockUnit = cml->roiMapDeltaQpBlockUnit;
        /* enable roiMap if lookaheadDepth for qpDelta mapping */
        if (cml->lookaheadDepth) {
            codingCfg.roiMapDeltaQpEnable = 1;
            codingCfg.roiMapDeltaQpBlockUnit =
                IS_AV1(cml->codecFormat) ? 0 : MAX(1, cml->roiMapDeltaQpBlockUnit);
        }

        codingCfg.RoimapCuCtrl_index_enable = (cml->RoimapCuCtrlIndexBinFile != NULL);
        codingCfg.RoimapCuCtrl_enable =
            ((cml->RoimapCuCtrlInfoBinFile != NULL) || (cml->roiMapConfigFile != NULL)) &&
            (cml->RoiCuCtrlVer != 0);
        codingCfg.RoimapCuCtrl_ver = cml->RoiCuCtrlVer;
        codingCfg.RoiQpDelta_ver = cml->RoiQpDeltaVer;

        /* SKIP map */
        codingCfg.skipMapEnable = cml->skipMapEnable;

        /* RDOQ map */
        codingCfg.rdoqMapEnable = cml->rdoqMapEnable;

        codingCfg.enableScalingList = cml->enableScalingList;
        codingCfg.chroma_qp_offset = cml->chromaQpOffset;

        /* low latency */
        codingCfg.inputLineBufEn = (cml->inputLineBufMode > 0) ? 1 : 0;
        codingCfg.inputLineBufLoopBackEn =
            (cml->inputLineBufMode == 1 || cml->inputLineBufMode == 2) ? 1 : 0;
        if (cml->inputLineBufDepth != DEFAULT)
            codingCfg.inputLineBufDepth = cml->inputLineBufDepth;
        codingCfg.amountPerLoopBack = cml->amountPerLoopBack;
        codingCfg.inputLineBufHwModeEn =
            (cml->inputLineBufMode == 2 || cml->inputLineBufMode == 4) ? 1 : 0;
        codingCfg.inputLineBufCbFunc = VCEncInputLineBufDone;
        codingCfg.inputLineBufCbData = &(tb->inputCtbLineBuf);
        codingCfg.sbi_id_0 = 0;
        codingCfg.sbi_id_1 = 1;
        codingCfg.sbi_id_2 = 2;

        /*stream multi-segment*/
        codingCfg.streamMultiSegmentMode = cml->streamMultiSegmentMode;
        codingCfg.streamMultiSegmentAmount = cml->streamMultiSegmentAmount;
        codingCfg.streamMultiSegCbFunc = &EncStreamSegmentReady;
        codingCfg.streamMultiSegCbData = &(tb->streamSegCtl);

        /* denoise */
        codingCfg.noiseReductionEnable =
            cml->noiseReductionEnable; //0: disable noise reduction; 1: enable noise reduction
        if (cml->noiseLow == 0) {
            codingCfg.noiseLow = 10;
        } else {
            codingCfg.noiseLow =
                CLIP3(1, 30,
                      cml->noiseLow); //0: use default value; valid value range: [1, 30]
        }

        if (cml->firstFrameSigma == 0) {
            codingCfg.firstFrameSigma = 11;
        } else {
            codingCfg.firstFrameSigma = CLIP3(1, 30, cml->firstFrameSigma);
        }

        /* smart */
        codingCfg.smartModeEnable = cml->smartModeEnable;
        codingCfg.smartH264LumDcTh = cml->smartH264LumDcTh;
        codingCfg.smartH264CbDcTh = cml->smartH264CbDcTh;
        codingCfg.smartH264CrDcTh = cml->smartH264CrDcTh;
        for (i = 0; i < 3; i++) {
            codingCfg.smartHevcLumDcTh[i] = cml->smartHevcLumDcTh[i];
            codingCfg.smartHevcChrDcTh[i] = cml->smartHevcChrDcTh[i];
            codingCfg.smartHevcLumAcNumTh[i] = cml->smartHevcLumAcNumTh[i];
            codingCfg.smartHevcChrAcNumTh[i] = cml->smartHevcChrAcNumTh[i];
        }
        codingCfg.smartH264Qp = cml->smartH264Qp;
        codingCfg.smartHevcLumQp = cml->smartHevcLumQp;
        codingCfg.smartHevcChrQp = cml->smartHevcChrQp;
        for (i = 0; i < 4; i++)
            codingCfg.smartMeanTh[i] = cml->smartMeanTh[i];
        codingCfg.smartPixNumCntTh = cml->smartPixNumCntTh;

        /* T35 */
        codingCfg.itu_t_t35.t35_enable = cml->itu_t_t35_enable;
        if (cml->itu_t_t35_enable) {
            codingCfg.itu_t_t35.t35_country_code = cml->itu_t_t35_country_code;
            codingCfg.itu_t_t35.t35_country_code_extension_byte =
                cml->itu_t_t35_country_code_extension_byte;
        }

        /* HDR10 */
        codingCfg.write_once_HDR10 = cml->write_once_HDR10;
        codingCfg.Hdr10Display.hdr10_display_enable = cml->hdr10_display_enable;
        if (cml->hdr10_display_enable) {
            codingCfg.Hdr10Display.hdr10_dx0 = cml->hdr10_dx0;
            codingCfg.Hdr10Display.hdr10_dy0 = cml->hdr10_dy0;
            codingCfg.Hdr10Display.hdr10_dx1 = cml->hdr10_dx1;
            codingCfg.Hdr10Display.hdr10_dy1 = cml->hdr10_dy1;
            codingCfg.Hdr10Display.hdr10_dx2 = cml->hdr10_dx2;
            codingCfg.Hdr10Display.hdr10_dy2 = cml->hdr10_dy2;
            codingCfg.Hdr10Display.hdr10_wx = cml->hdr10_wx;
            codingCfg.Hdr10Display.hdr10_wy = cml->hdr10_wy;
            codingCfg.Hdr10Display.hdr10_maxluma = cml->hdr10_maxluma;
            codingCfg.Hdr10Display.hdr10_minluma = cml->hdr10_minluma;
        }

        codingCfg.Hdr10LightLevel.hdr10_lightlevel_enable = cml->hdr10_lightlevel_enable;
        if (cml->hdr10_lightlevel_enable) {
            codingCfg.Hdr10LightLevel.hdr10_maxlight = cml->hdr10_maxlight;
            codingCfg.Hdr10LightLevel.hdr10_avglight = cml->hdr10_avglight;
        }

        codingCfg.vuiColorDescription.vuiColorDescripPresentFlag = cml->vuiColorDescripPresentFlag;
        if (cml->vuiColorDescripPresentFlag) {
            codingCfg.vuiColorDescription.vuiMatrixCoefficients = cml->vuiMatrixCoefficients;
            codingCfg.vuiColorDescription.vuiColorPrimaries = cml->vuiColorPrimaries;

            /*
      if (cml->hdr10_lightlevel_enable || cml->hdr10_display_enable) {
        if (cml->vui_transfer == 1)
          codingCfg.vuiColor.vui_transfer = VCENC_HDR10_ST2084;
        else if (cml->vui_transfer == 2)
          codingCfg.vuiColor.vui_transfer = VCENC_HDR10_STDB67;
        else
          codingCfg.vuiColor.vui_transfer = VCENC_HDR10_BT2020;
      }
      else
        */
            codingCfg.vuiColorDescription.vuiTransferCharacteristics =
                cml->vuiTransferCharacteristics;
        }

        codingCfg.vuiVideoFormat = cml->vuiVideoFormat;
        codingCfg.vuiVideoSignalTypePresentFlag = cml->vuiVideoSignalTypePresentFlag;
        codingCfg.sampleAspectRatioHeight = cml->vuiAspectRatioHeight;
        codingCfg.sampleAspectRatioWidth = cml->vuiAspectRatioWidth;

        codingCfg.RpsInSliceHeader = cml->RpsInSliceHeader;

        /* Assign ME vertical search range */
        codingCfg.meVertSearchRange = cml->MEVertRange;

        if (cml->psyFactor != DEFAULT)
            codingCfg.psyFactor = cml->psyFactor;
        if (cml->aq_mode != DEFAULT)
            codingCfg.aq_mode = cml->aq_mode;
        if (cml->aq_strength != DEFAULT)
            codingCfg.aq_strength = cml->aq_strength;

        if ((ret = VCEncSetCodingCtrl(encoder, &codingCfg)) != VCENC_OK) {
            //PrintErrorValue("VCEncSetCodingCtrl() failed.", ret);
            return -1;
        }
    }

    return 0;
}

int run_instance(commandLine_s *cml)
{
    struct test_bench tb;
    VCEncInst hevc_encoder;
    VCEncGopPicConfig gopPicCfg[MAX_GOP_PIC_CONFIG_NUM];
    VCEncGopPicConfig gopPicCfgPass2[MAX_GOP_PIC_CONFIG_NUM];
    VCEncGopPicSpecialConfig gopPicSpecialCfg[MAX_GOP_SPIC_CONFIG_NUM];
    i32 ret = OK;

    memset(&tb, 0, sizeof(struct test_bench));
    tb.argc = cml->argc;
    tb.argv = cml->argv;

    if (cml->inputFileList) {
        tb.inputFileList = fopen(cml->inputFileList, "rb");
        if (tb.inputFileList == NULL) {
            fprintf(HEVCERR_OUTPUT, "Unable to open inputFileList file: %s\n", cml->inputFileList);
            return -1;
        } else if (getInputFileList(&tb, cml) != 0) {
            fprintf(HEVCERR_OUTPUT, "Unable to parse inputFileList file: %s\n", cml->inputFileList);
            return -1;
        }
    } else {
        /* Check that input file exists */
        tb.yuvFile = fopen(cml->input, "rb");

        if (tb.yuvFile == NULL) {
            fprintf(HEVCERR_OUTPUT, "Unable to open input file: %s\n", cml->input);
            return -1;
        } else {
            fclose(tb.yuvFile);
            tb.yuvFile = NULL;
        }
    }

    /* Check overlay input exist*/
    if (cml->overlayEnables) {
        FILE *tmp_file;
        int i;
        for (i = 0; i < MAX_OVERLAY_NUM; i++) {
            if (((cml->overlayEnables >> i) & 1) && cml->olFormat[i] != 3) {
                tmp_file = fopen(cml->olInput[i], "rb");
                if (tmp_file == NULL) {
                    fprintf(HEVCERR_OUTPUT, "Unable to open overlay input file %d: %s\n", i + 1,
                            cml->olInput[i]);
                    return -1;
                } else {
                    fclose(tmp_file);
                }
            }
        }
    }

    if (cml->replaceMvFile) {
        tb.replaceMvFile = fopen(cml->replaceMvFile, "rb");
        if (tb.replaceMvFile == NULL) {
            fprintf(HEVCERR_OUTPUT, "Unable to open ME1N MV file: %s\n", cml->replaceMvFile);
            return -1;
        }
    }

    if (cml->lookaheadDepth && cml->halfDsInput) {
        FILE *dsFile = fopen(cml->halfDsInput, "rb");
        if (dsFile == NULL) {
            fprintf(HEVCERR_OUTPUT, "Unable to open downsample input file: %s\n", cml->input);
            return -1;
        } else {
            fclose(dsFile);
            dsFile = NULL;
        }
    }

    /* the number of output stream buffers */
    tb.streamBufNum = cml->streamBufChain ? 2 : 1;

    CLIENT_TYPE client_type =
        IS_H264(cml->codecFormat) ? EWL_CLIENT_TYPE_H264_ENC : EWL_CLIENT_TYPE_HEVC_ENC;
    tb.hwid = EncAsicGetAsicHWid(client_type, NULL);
    tb.hwCfg = VCEncGetAsicConfig(cml->codecFormat, NULL);

    /* GOP configuration */
    tb.gopSize = MIN(cml->gopSize, MAX_GOP_SIZE);
    if (tb.gopSize == 0 && cml->gopLowdelay) {
        tb.gopSize = 4;
    }
    memset(gopPicCfg, 0, sizeof(gopPicCfg));
    tb.encIn.gopConfig.pGopPicCfg = gopPicCfg;
    memset(gopPicSpecialCfg, 0, sizeof(gopPicSpecialCfg));
    tb.encIn.gopConfig.pGopPicSpecialCfg = gopPicSpecialCfg;
    if (InitGopConfigs(tb.gopSize, cml, &(tb.encIn.gopConfig), tb.encIn.gopConfig.gopCfgOffset,
                       HANTRO_FALSE, tb.hwid) != 0) {
        return -ret;
    }
    if (cml->lookaheadDepth) {
        memset(gopPicCfgPass2, 0, sizeof(gopPicCfgPass2));
        tb.encIn.gopConfig.pGopPicCfg = gopPicCfgPass2;
        tb.encIn.gopConfig.size = 0;
        memset(gopPicSpecialCfg, 0, sizeof(gopPicSpecialCfg));
        tb.encIn.gopConfig.pGopPicSpecialCfg = gopPicSpecialCfg;
        cml->gopLowdelay = 0; // reset gopLowdelay for pass2
        if (InitGopConfigs(tb.gopSize, cml, &(tb.encIn.gopConfig), tb.encIn.gopConfig.gopCfgOffset,
                           HANTRO_TRUE, tb.hwid) != 0) {
            return -ret;
        }
        tb.encIn.gopConfig.pGopPicCfgPass1 = gopPicCfg;
        tb.encIn.gopConfig.pGopPicCfg = tb.encIn.gopConfig.pGopPicCfgPass2 = gopPicCfgPass2;
    }

    /* Set y and uv stride for overlay region */
    int i;
    for (i = 0; i < MAX_OVERLAY_NUM; i++) {
        //Set default stride
        if (cml->olYStride[i] == 0) {
            switch (cml->olFormat[i]) {
            case 0: //ARGB
                /* For super tile format, 64 rows aligned */
                if (cml->olSuperTile[i] == 0)
                    cml->olYStride[i] = cml->olWidth[i] * 4;
                else
                    cml->olYStride[i] = STRIDE(cml->olWidth[i], 64) * 64 * 4;
                break;
            case 1: //NV12
                cml->olYStride[i] = cml->olWidth[i];
                break;
            case 2: //Bitmap
                cml->olYStride[i] = cml->olWidth[i] / 8;
                break;
            default:
                cml->olYStride[i] = cml->olWidth[i];
            }
        }
        if (cml->olUVStride[i] == 0)
            cml->olUVStride[i] = cml->olYStride[i];

        //Set default cropping info
        if (cml->olCropWidth[i] == 0)
            cml->olCropWidth[i] = cml->olWidth[i];
        if (cml->olCropHeight[i] == 0)
            cml->olCropHeight[i] = cml->olHeight[i];
        if (cml->olScaleWidth[i] == 0)
            cml->olScaleWidth[i] = cml->olCropWidth[i];
        if (cml->olScaleHeight[i] == 0)
            cml->olScaleHeight[i] = cml->olCropHeight[i];

        tb.olEnable[i] = (cml->overlayEnables >> i) & 1;
    }

    if (!allocVariableMem(&tb, cml)) {
        printf("allocVariableNem error!\n");
        return NOK;
    }

    if ((ret = OpenEncoder(cml, &hevc_encoder, &tb)) != 0) {
        FreeVariableMem(&tb);
        return -ret;
    }
    SetTbParams(&tb, cml);
    tb.inputInfoBuf_cnt = VCEncGetEncodeMaxDelay(hevc_encoder) + 1;
    u32 nInputFrame = tb.lastPic - tb.firstPic + 1; //input frames
    if (!tb.inputFileLinkListNum)
        tb.buffer_cnt = MIN(tb.inputInfoBuf_cnt, nInputFrame);
    else
        tb.buffer_cnt = tb.inputInfoBuf_cnt;

    if (cml->replaceMvFile != NULL)
        tb.replaceMvFile = fopen(cml->replaceMvFile, "rb");
    else
        tb.replaceMvFile = NULL;

    /* Set the test ID for internal testing,
   * the SW must be compiled with testing flags */
    VCEncSetTestId(hevc_encoder, cml->testId);

    /* Allocate input and output buffers */
    if (AllocRes(cml, hevc_encoder, &tb) != 0) {
        ret = VCENC_MEMORY_ERROR;
        goto end;
    }

#ifdef TEST_DATA
    Enc_test_data_init(tb.parallelCoreNum);
#endif
    for (int i = 0; i <= tb.inputFileLinkListNum; i++) {
        if (i > 0) {
            VCEncStrmCtrl strmCtrl;
            static char rec_file[512] = "deblock_0.yuv";

            ASSERT(tb.inputFileLinkListP != NULL);

            cml->input = tb.input = tb.inputFileLinkListP->in;
            cml->output = tb.output = tb.inputFileLinkListP->out;

            cml->lumWidthSrc = cml->width = tb.width = strmCtrl.width = tb.inputFileLinkListP->w;
            cml->lumHeightSrc = cml->height = tb.height = strmCtrl.height =
                tb.inputFileLinkListP->h;

            cml->firstPic = tb.firstPic = tb.encIn.gopConfig.firstPic = tb.inputFileLinkListP->a;
            cml->lastPic = tb.lastPic = tb.encIn.gopConfig.lastPic = tb.inputFileLinkListP->b;
            cml->recon = rec_file;
            cml->recon[8] = '0' + i;

            tb.picture_enc_cnt = 0;
            tb.picture_encoded_cnt = 0;
            tb.ivf_frame_cnt = 0;
            tb.ivfSize = 0;

            ret = VCEncSetStrmCtrl(hevc_encoder, &strmCtrl);
            if (ret != VCENC_OK) {
                printf("Set stream control error, ret = %d, width=%d, height=%d.\n", ret,
                       strmCtrl.width, strmCtrl.height);
                tb.inputFileLinkListP = inputFileLinkListDeleteHead(tb.inputFileLinkListP);
                continue;
                //goto end;
            }
            printf("---------------->>>>>>>>>>>>>>>> Change input file to %s\n", tb.input);
        } else {
            if (tb.inputFileLinkListNum > 0) {
                /* skip the first stream if input is specified by input configure file */
                continue;
            }
        }
        ret = encode(&tb, hevc_encoder, cml);
        if (i > 0) {
            tb.inputFileLinkListP = inputFileLinkListDeleteHead(tb.inputFileLinkListP);
        }
        printf("\n");
    }
#ifdef TEST_DATA
    Enc_test_data_release();
#endif

end:
    CloseEncoder(hevc_encoder, &tb);
    FreeVariableMem(&tb);
    FreeRes(&tb);
    return ret;
}

/*------------------------------------------------------------------------------
  processFrame
------------------------------------------------------------------------------*/
void processFrame(struct test_bench *tb, VCEncInst encoder, commandLine_s *cml, VCEncOut *pEncOut,
                  SliceCtl_s *ctl, u64 *streamSize, u32 *maxSliceSize, ma_s *ma, u64 *total_bits,
                  u32 *outIdrFileOffset)
{
    i32 iBuf;
    VCEncIn *pEncIn = &(tb->encIn);
    u64 bitsCnt = 0;

#ifdef TEST_DATA
    //show profile data, only for c-model;
    //write_recon_sw(encoder, encIn.poc);
    EncTraceUpdateStatus();
    if ((cml->outReconFrame == 1) && (cml->writeReconToDDR || cml->intraPicRate != 1))
        EncTraceRecon(encoder, outIdrFileOffset /*encIn.poc*/, cml->recon, pEncOut);
#endif

    /* Write scaled encoder picture to file, packed yuyv 4:2:2 format */
    EWLLinearMem_t scaledPicture = tb->scaledPictureMem;
    EWLSyncMemData(&scaledPicture, 0, scaledPicture.size, DEVICE_TO_HOST);
    WriteScaled((u32 *)pEncOut->scaledPicture, cml);
#ifdef TEST_DATA
    /* write cu encoding information to file <cuInfo.txt> */
    EncTraceCuInformation(encoder, pEncOut, pEncOut->picture_cnt, pEncOut->poc);

    /* write ctb encoded bits to file <ctbBits.txt> */
    EncTraceCtbBits(encoder, pEncOut->ctbBitsData);
#endif
    u32 ivfHeaderSize = 0;
    u32 tileId;
    //multi-core: output bitstream has (tb->parallelCoreNum-1) delay
    //multi-tile: outbuf index == 0
    i32 coreId = FindOutputBufferIdByBusAddr(tb->outbufMemFactory, tb->outbuf_cnt,
                                             pEncOut->consumedAddr.outbufBusAddr);
    for (tileId = 0; tileId < tb->tileColumnNum; tileId++) {
        u32 tilestreamSize =
            (tileId == 0 ? pEncOut->streamSize : pEncOut->tileExtra[tileId - 1].streamSize);
        u32 *pNaluSizeBuf =
            (tileId == 0 ? pEncOut->pNaluSizeBuf : pEncOut->tileExtra[tileId - 1].pNaluSizeBuf);
        u32 numNalus = (tileId == 0 ? pEncOut->numNalus : pEncOut->tileExtra[tileId - 1].numNalus);
        if (tilestreamSize != 0) {
            for (iBuf = 0; iBuf < tb->streamBufNum; iBuf++)
                tb->outbufMem[tileId][iBuf] = &(tb->outbufMemFactory[coreId][tileId][iBuf]);

            if ((cml->streamMultiSegmentMode != 0) || (ctl->multislice_encoding == 0) ||
                (ENCH2_SLICE_READY_INTERRUPT == 0) || (cml->hrdConformance == 1)) {
                VCEncStrmBufs bufs;
                getStreamBufs(&bufs, tb, cml, HANTRO_TRUE, tileId);
                u32 MemSyncDataSize = 0, MemSyncOffset = 0;
                u32 MemSyncDataSize0 = 0, MemSyncOffset0 = 0;
                u32 MemSyncDataSize1 = 0, MemSyncOffset1 = 0;
                if (pEncOut->PreDataSize)
                    MemSyncOffset = pEncOut->PreDataSize;
                else
                    MemSyncOffset = 0;
                MemSyncDataSize = tilestreamSize - pEncOut->PostDataSize - pEncOut->PreDataSize;
#ifdef INTERNAL_TEST
                if ((cml->testId == TID_INT) && cml->streamBufChain &&
                    (cml->parallelCoreNum <= 1)) {
                    if (tilestreamSize > bufs.bufLen[0]) {
                        MemSyncDataSize0 = bufs.bufLen[0] - MemSyncOffset;
                        MemSyncOffset0 = MemSyncOffset + bufs.bufOffset[0];
                        EWLSyncMemData(tb->outbufMem[0][0], MemSyncOffset0, MemSyncDataSize0,
                                       DEVICE_TO_HOST);
                        MemSyncDataSize1 = MemSyncDataSize - MemSyncDataSize0;
                        MemSyncOffset1 = bufs.bufOffset[1];
                        EWLSyncMemData(tb->outbufMem[0][1], MemSyncOffset1, MemSyncDataSize1,
                                       DEVICE_TO_HOST);
                    } else {
                        MemSyncDataSize0 = MemSyncDataSize;
                        MemSyncOffset0 = MemSyncOffset + bufs.bufOffset[0];
                        EWLSyncMemData(tb->outbufMem[0][0], MemSyncOffset0, MemSyncDataSize0,
                                       DEVICE_TO_HOST);
                    }
                } else
                    EWLSyncMemData(tb->outbufMem[0][0], MemSyncOffset, MemSyncDataSize,
                                   DEVICE_TO_HOST);
#else
                EWLSyncMemData(tb->outbufMem[0][0], MemSyncOffset, MemSyncDataSize, DEVICE_TO_HOST);
#endif

                if (tb->streamSegCtl.streamMultiSegEn) {
                    u8 *streamBase =
                        tb->streamSegCtl.streamBase +
                        (tb->streamSegCtl.streamRDCounter % tb->streamSegCtl.segmentAmount) *
                            tb->streamSegCtl.segmentSize;
                    WriteStrm(tb->streamSegCtl.outStreamFile, (u32 *)streamBase,
                              tilestreamSize -
                                  tb->streamSegCtl.streamRDCounter * tb->streamSegCtl.segmentSize,
                              0);
                    tb->streamSegCtl.streamRDCounter = 0;
                } else if (cml->byteStream == 0) {
                    //WriteNalSizesToFile(nalfile, pEncOut->pNaluSizeBuf, pEncOut->numNalus);
                    writeNalsBufs(tb->out, &bufs, pNaluSizeBuf, numNalus, 0,
                                  pEncOut->sliceHeaderSize, 0);
                } else {
                    if (IS_VP9(cml->codecFormat) || (IS_AV1(cml->codecFormat) && cml->ivf)) {
                        u32 offset = 0;
                        for (i32 i = 0; i < numNalus; i++) {
                            u8 frameNotShow = (pEncOut->av1Vp9FrameNotShow >> i) & 1;
                            writeIvfFrame(tb->out, &bufs, pNaluSizeBuf[i], offset,
                                          &tb->ivf_frame_cnt,
                                          IS_AV1(cml->codecFormat) && cml->modifiedTileGroupSize,
                                          frameNotShow, &tb->ivfSize, IS_VP9(cml->codecFormat));
                            offset += pNaluSizeBuf[i];
                            ivfHeaderSize += 12;
                        }
                    } else
                        writeStrmBufs(tb->out, &bufs, 0, tilestreamSize, 0);
                }
            }
            pEncIn->timeIncrement = tb->outputRateDenom;

            *total_bits += tilestreamSize * 8;
            *total_bits += ivfHeaderSize * 8; //IVF frame header size
            *streamSize += tilestreamSize;
            *streamSize += ivfHeaderSize;
            *maxSliceSize = MAX(*maxSliceSize, pEncOut->maxSliceStreamSize);

            printf("=== Encoded %i bits=%d totalBits=%lu averageBitrate=%lu HWCycles=%d "
                   "codingType=%d\n",
                   tb->picture_encoded_cnt, tilestreamSize * 8, (long unsigned int)(*total_bits),
                   (long unsigned int)(*total_bits * tb->outputRateNumer) /
                       ((tb->picture_encoded_cnt + 1) * tb->outputRateDenom),
                   VCEncGetPerformance(encoder), pEncOut->codingType);

#ifdef INTERNAL_TEST
            if (cml->bRcLog)
                StatisticFrameBits(encoder, tb, ma, tilestreamSize * 8, *total_bits);
#endif
            if (pEncOut->codingType != VCENC_NOTCODED_FRAME)
                tb->picture_encoded_cnt++;
            bitsCnt += (*total_bits - tb->totalBits);
            tb->totalBits = *total_bits;
        }
    }
#ifdef INTERNAL_TEST
    EncTraceProfile(encoder, tb, pEncOut, bitsCnt, cml->bRdLog);
#endif
}

/*------------------------------------------------------------------------------
  encode
------------------------------------------------------------------------------*/
i32 encode(struct test_bench *tb, VCEncInst encoder, commandLine_s *cml)
{
    VCEncIn *pEncIn = &(tb->encIn);
    VCEncOut encOut;
    VCEncRet ret;
    i32 encRet = NOK;
    u32 frameCntTotal = 0;
    u64 streamSize = 0;
    u32 maxSliceSize = 0;
    ma_s ma;
    i32 cnt = 1;
    u32 i, tmp;
    i32 p;
    u64 total_bits = 0;
    u8 *pUserData;
    u32 src_img_size;
    u32 pictureSize;
    VCEncRateCtrl rc;
    u32 outIdrFileOffset = 0;
    u32 gopSize = tb->gopSize;
    VCEncPictureCodingType nextCodingType = VCENC_INTRA_FRAME;
    i32 seiFrameId = DEFAULT;
    i32 t35FrameId = DEFAULT;
    i32 fret;
    u32 frameWriteTime = 0;
    u64 totalTime = 0, ioTime = 0;
    struct timeval timeStart, timeEnd, readStart, readEnd, writeStart, writeEnd;
    bool forceEnd = false;
    i32 last_picture_num = -1, cur_picture_num = -1;
    ltr_struct ltr_curr, ltr_next;
    u32 tileId;

    /* lastPic should be greater or equal to firstPic */
    if (tb->firstPic > tb->lastPic) {
        Error(2, ERR, "Last picture to encode is less than first picture to encode");
        goto error;
    }
    pEncIn->tileExtra = tb->vcencInTileExtra;
    encOut.tileExtra = NULL; /* allocate inside sw API */

    /* Output/input streams */
    if (!(tb->in = open_file(tb->input, "rb")))
        goto error;
    if (!(tb->out = open_file(tb->output, "wb")))
        goto error;
    if (tb->halfDsInput && !(tb->inDS = open_file(tb->halfDsInput, "rb")))
        goto error;
    tb->dec400Table = fopen(tb->dec400TableFile, "rb");

    /* Read flexible reference description */
    if (OpenFlexRefsFile(tb, cml))
        goto error;

    //get output buffer for header.
    if (NOK == GetFreeOutputBuffer(tb)) {
        Error(2, ERR, "GetFreeOutputBuffer() fails");
        goto error;
    }
    SetupOutputBuffer(tb, pEncIn);

    InitSliceCtl(tb, cml);
    InitStreamSegmentCrl(tb, cml);

    if (cml->inputLineBufMode) {
        if (InitInputLineBuffer(&(tb->inputCtbLineBuf), cml, pEncIn, encoder, tb)) {
            fprintf(HEVCERR_OUTPUT, "Fail to Init Input Line Buffer: virt_addr=%p, bus_addr=%08x\n",
                    tb->inputCtbLineBuf.sram, (u32)(tb->inputCtbLineBuf.sramBusAddr));
            goto error;
        }
    }

    /* before VCEncStrmStart called */
    InitPicConfig(pEncIn, tb, cml);

    if (tb->flexRefsFile != NULL) {
        pEncIn->flexRefsEnable = 1;
        pEncIn->gopConfig.ltrcnt = 2; //max 2
    }

    if (cml->sendAUD)
        pEncIn->sendAUD = 1;

    /* Video, sequence and picture parameter sets */
    for (p = 0; p < cnt; p++) {
        if (VCEncStrmStart(encoder, pEncIn, &encOut)) {
            Error(2, ERR, "hevc_set_parameter() fails");
            goto error;
        }

        if (IS_VP9(cml->codecFormat) || (IS_AV1(cml->codecFormat) && cml->ivf)) {
            i32 width = ((cml->rotation == 1) || (cml->rotation == 2)) ? cml->height : cml->width;
            i32 height = ((cml->rotation == 1) || (cml->rotation == 2)) ? cml->width : cml->height;
            writeIvfHeader(tb->out, width, height, cml->outputRateNumer, cml->outputRateDenom,
                           IS_VP9(cml->codecFormat));
            total_bits += 32 * 8; //IVF Stream header size
            streamSize += 32;
        }
        VCEncStrmBufs bufs;
        getStreamBufs(&bufs, tb, cml, HANTRO_FALSE, 0);
        if (cml->byteStream == 0) {
            //WriteNalSizesToFile(nalfile, encOut.pNaluSizeBuf, encOut.numNalus);
            writeNalsBufs(tb->out, &bufs, encOut.pNaluSizeBuf, encOut.numNalus, 0, 0, 0);
        } else
            writeStrmBufs(tb->out, &bufs, 0, encOut.streamSize, 0);

        total_bits += encOut.streamSize * 8;
        streamSize += encOut.streamSize;
    }

    //return output buffer for header after VCEncStrmStart.
    if (NOK == ReturnIOBuffer(tb, cml, &encOut.consumedAddr, 0)) {
        Error(2, ERR, "Manage IO buffer failed");
    }

    VCEncGetRateCtrl(encoder, &rc);

    /* Allocate a buffer for user data and read data from file */
    pUserData = ReadUserData(encoder, cml->userData);

    /* Read configuration files for ROI/CuTree/IPCM/GMV/Overlay ... */
    if (readConfigFiles(tb, cml))
        goto error;

    if (OK != ChangeInputToTransYUV(tb, encoder, cml, pEncIn)) {
        Error(2, ERR, "ChangeInputToTransYUV() fail");
        goto error;
    }
#ifdef VMAF_SUPPORT
    if (cml->tune == 4) {
        memset(&tb->vmafctrl, 0, sizeof(tb->vmafctrl));
        vmaf_alloc_frame_buffer(encoder, &tb->vmafctrl.source_extended, cml->width, cml->height);
        vmaf_alloc_frame_buffer(encoder, &tb->vmafctrl.blurred, cml->width, cml->height);
        vmaf_alloc_frame_buffer(encoder, &tb->vmafctrl.sharpened, cml->width, cml->height);
        if (vmaf_prepare_model() < 0) {
            Error(2, ERR,
                  "vmaf_prepare_model() fails. make sure you have write permission "
                  "on current directory.");
            goto error;
        }
    }
#endif
    if (tb->extSEIFile && seiFrameId == DEFAULT) {
        fret = fread(&seiFrameId, sizeof(u32), 1, tb->extSEIFile);
    }

    if (tb->p_t35_file && t35FrameId == DEFAULT) {
        fret = fread(&t35FrameId, sizeof(u32), 1, tb->p_t35_file);
    }
    gettimeofday(&timeStart, NULL);

    while (HANTRO_TRUE) {
        /* test stop encoding at specified frame num*/
        if (cml->stopFrameNum == tb->picture_enc_cnt)
            goto STOP;

        /* check trigger for new parameters */
        if (cml->trigger_pic_cnt == tb->picture_enc_cnt) {
            if (Parameter_Get(cml->argc, cml->argv, cml)) {
                Error(2, ERR, "Input parameter error");
                ret = NOK;
                goto error;
            }

            /* not all tb params and coding ctrl params can change now. */
            SetTbParams(tb, cml);
            if (0 != SetCodingCtrl(tb, encoder, cml)) {
                Error(2, ERR, "SetCodingCtrl() fails");
                ret = NOK;
                goto error;
            }
        }

        /* get output information buffer */
        if (NOK == GetFreeOutputBuffer(tb)) {
            Error(2, ERR, "GetFreeOutputBuffer() fails");
            goto error;
        }
        SetupOutputBuffer(tb, pEncIn);

        cur_picture_num = next_picture(tb, tb->encIn.picture_cnt) + tb->firstPic;
        //before stoping encoding, need to setup output buffer for first time flush
        if (true == forceEnd || cur_picture_num > tb->lastPic)
            break;
        // when inputRate is smaller than outputRate, reuse last input buffer, instead of read picture again.
        if (last_picture_num == cur_picture_num) {
            if (NOK == ReuseInputBuffer(tb, tb->pictureMem->busAddress)) {
                Error(2, ERR, "ReuseInputBuffer() fail");
                goto error;
            }
        } else {
            //get free input buffer
            if (NOK == GetFreeInputPicBuffer(tb)) {
                Error(2, ERR, "GetFreeInputPicBuffer() fails");
                goto error;
            }
        }

        /* Setup encoder input */
        src_img_size = SetupInputBuffer(tb, cml, pEncIn);

        /*Setup dec400 compress table if needed*/
        if (tb->dec400Table) {
            SetupDec400CompTable(tb, pEncIn);
        }

        /*Setup external SRAM if needed*/
        SetupExtSRAM(tb, pEncIn);

        /* get other input information buffer */
        if (NOK == GetFreeInputInfoBuffer(tb)) {
            Error(2, ERR, "GetFreeInputInfoBuffer() fails");
            goto error;
        }

        pictureSize = tb->lumaSize + tb->chromaSize;

        gettimeofday(&readStart, NULL);

        if ((cml->videoStab != DEFAULT) && (cml->videoStab)) {
            i32 ret;
            /* read next frame for video stabilization*/
            ret = read_stab_picture(tb, cml->inputFormat, src_img_size, cml->lumWidthSrc,
                                    cml->lumHeightSrc);
            if (ret == NOK) {
                // TODO: should stop stab now. use old data without change
            }
            pEncIn->busLumaStab = tb->pictureStabMem.busAddress;
            if (EWLSyncMemData(&tb->pictureStabMem, 0, pictureSize, HOST_TO_DEVICE) != EWL_OK) {
                printf("Error! Sync tb->pictureStabMem Data fail!\n");
                goto error;
            }
        }

        /* Read the flex reference descriptio for the encoding frame */
        if (tb->flexRefsFile != NULL && IS_HEVC(cml->codecFormat)) {
            cur_picture_num = pEncIn->picture_cnt =
                GetNextReferenceControl(tb->flexRefsFile, &pEncIn->gopCurrPicConfig, &ltr_curr);
            if (pEncIn->picture_cnt < 0) {
                if (pEncIn->picture_cnt == -2) {
                    printf("End of Reading flexRefs list. Exit Encoding.\n");
                    break;
                } else if (pEncIn->picture_cnt == -1) {
                    Error(2, ERR, "flexRefs input fails");
                }
                goto error;
            }
            FillInputOptionsForFlexRps(pEncIn, &pEncIn->gopCurrPicConfig);
            pEncIn->flexRefsEnable = 1;
        } else if (tb->flexRefsFile != NULL && IS_H264(cml->codecFormat)) {
            memset(&ltr_curr, 0, sizeof(ltr_curr));
            memset(&ltr_next, 0, sizeof(ltr_curr));
            memset(pEncIn->bLTR_need_update, 0, sizeof(pEncIn->bLTR_need_update));

            cur_picture_num = pEncIn->picture_cnt =
                GetNextReferenceControl(tb->flexRefsFile, &pEncIn->gopCurrPicConfig, &ltr_curr);
            if (pEncIn->picture_cnt < 0) {
                if (pEncIn->picture_cnt == -2) {
                    printf("End of Reading flexRefs list. Exit Encoding.\n");
                    break;
                } else if (pEncIn->picture_cnt == -1) {
                    Error(2, ERR, "flexRefs input fails");
                }
                goto error;
            }

            pEncIn->u8IdxEncodedAsLTR = ltr_curr.encoded_as_ltr;
            pEncIn->bIsPeriodUsingLTR = 1;

            if (ltr_curr.encoded_as_ltr > 0) {
                pEncIn->bLTR_need_update[ltr_curr.encoded_as_ltr - 1] =
                    ltr_curr.encoded_as_ltr ? HANTRO_TRUE : HANTRO_FALSE;
                pEncIn->long_term_ref_pic[ltr_curr.encoded_as_ltr - 1] =
                    pEncIn->gopCurrPicConfig.poc;
            }

            memcpy(pEncIn->bLTR_used_by_cur, ltr_curr.bLTR_used_by_cur,
                   sizeof(ltr_curr.bLTR_used_by_cur));

            FillInputOptionsForFlexRps(pEncIn, &pEncIn->gopCurrPicConfig);

            int pos_save = ftell(tb->flexRefsFile);
            pEncIn->picture_cnt_next =
                GetNextReferenceControl(tb->flexRefsFile, &pEncIn->gopNextPicConfig, &ltr_next);
            fseek(tb->flexRefsFile, pos_save, SEEK_SET);
        }

        // when inputRate is smaller than outputRate, reuse last input buffer, instead of read picture again.
        if (last_picture_num != cur_picture_num) {
            /* it will read in read_dec400Data, so don't need read again */
            if (tb->dec400Table == NULL) {
                /* Read YUV to input buffer */
                if (read_picture(tb, cml->inputFormat, src_img_size, cml->lumWidthSrc,
                                 cml->lumHeightSrc, 0)) {
                    printf("Input data read over, try to find next input stream if any\n");
                    /* Next input stream (if any) */
                    if (change_input(tb))
                        break;
                    fclose(tb->in);
                    if (!(tb->in = open_file(tb->input, "rb")))
                        goto error;
                    tb->encIn.picture_cnt = 0;
                    tb->picture_enc_cnt = 0;
                    continue;
                }
                if (tb->halfDsInput &&
                    read_picture(tb, cml->inputFormat, tb->src_img_size_ds, cml->lumWidthSrc / 2,
                                 cml->lumHeightSrc / 2, 1)) {
                    Error(2, ERR, "read down-sampled input fails");
                    goto error;
                }

                if (EWLSyncMemData(tb->pictureMem, 0, pictureSize, HOST_TO_DEVICE) != EWL_OK) {
                    printf("Error! Sync tb->pictureMem Data fail!\n");
                    goto error;
                }
                if (tb->halfDsInput) {
                    if (EWLSyncMemData(tb->pictureDSMem, 0, tb->pictureDSSize, HOST_TO_DEVICE) !=
                        EWL_OK) {
                        printf("Error! Sync tb->pictureDSMem Data fail!\n");
                        goto error;
                    }
                }
                pEncIn->dec400Enable = 1;
            }
            /*if dec400 is used, read dec40 from file */
            else {
                if (read_dec400Data(tb, cml->inputFormat, cml->lumWidthSrc, cml->lumHeightSrc) ==
                    NOK) {
                    printf("DEC400 data read over, please check the yuv real frames and "
                           "your cmdline about lastPic!!\n");
                    break;
                }
                pEncIn->dec400Enable = 2;
                EWLSyncMemData(tb->Dec400CmpTableMem, 0, tb->dec400FrameTableSize, HOST_TO_DEVICE);
            }
            /* Format to some specific customized format*/
            FormatCustomizedYUV(tb, cml);
        }

        frameCntTotal++;

        //for VC8000E test bench, disable axiFE apbfilter by default
        pEncIn->apbFTEnable = 0;

#ifndef SUPPORT_AXIFE
        pEncIn->axiFEEnable = 0;
#else
#ifdef CMODEL_AXIFE
        pEncIn->axiFEEnable = cml->useAXIFE;
#else
        pEncIn->axiFEEnable = 1;
#endif
#endif
        /*
      * per-frame test functions
      */

        /* 1. scene changed frames from usr*/
        pEncIn->sceneChange = 0;
        tmp = next_picture(tb, tb->encIn.picture_cnt) + tb->firstPic;
        for (i = 0; i < MAX_SCENE_CHANGE; i++) {
            if (cml->sceneChange[i] == 0) {
                break;
            }
            if (cml->sceneChange[i] == tmp) {
                pEncIn->sceneChange = 1;
                printf("    Input a Scene Changed Frame! \n");
                break;
            }
        }

        /* 2. GMV setting from user*/
        readGmv(tb, pEncIn, cml);

        if (pEncIn->flexRefsEnable) {
            pEncIn->codingType = pEncIn->gopCurrPicConfig.codingType;
        }

        /* 3. On-fly bitrate setting */
        for (i = 0; i < MAX_BPS_ADJUST; i++)
            if (cml->bpsAdjustFrame[i] && (tb->encIn.picture_cnt == cml->bpsAdjustFrame[i])) {
                rc.bitPerSecond = cml->bpsAdjustBitrate[i];
                printf("Adjusting bitrate target: %d\n", rc.bitPerSecond);
                if ((ret = VCEncSetRateCtrl(encoder, &rc)) != VCENC_OK) {
                    //PrintErrorValue("VCEncSetRateCtrl() failed.", ret);
                }
            }

        /* 4. SetupROI-Map */
        SetupROIMapBuffer(tb, cml, pEncIn, encoder);

        /* 5. encoding specific frame from user */
        /* insert SKIP Frame at specified frame, where all CU/MB are encoded as SKIP MODE. */
        /* FIXME: the skip frame location is not decided according to POC. use skip_frame_offset
     *  or sepecified with input frame count directly. */
        if (0 != tb->idr_interval)
            pEncIn->bSkipFrame = cml->skip_frame_enabled_flag &&
                                 ((tb->picture_enc_cnt % tb->idr_interval) == cml->skip_frame_poc);
        else
            pEncIn->bSkipFrame =
                cml->skip_frame_enabled_flag && (tb->picture_enc_cnt == cml->skip_frame_poc);

        /* Forcely encoded specified frame as IDR frame. */
        pEncIn->bIsIDR = (cml->insertIDR == pEncIn->picture_cnt) ? HANTRO_TRUE : HANTRO_FALSE;

        /* 6. low latency */
        if (cml->inputLineBufMode) {
            if (tb->inputCtbLineBuf.loopBackEn && tb->inputCtbLineBuf.lumBuf.buf) {
                VCEncPreProcessingCfg preProcCfg;
                //[fpga] when loop-back enable, inputCtbLineBuf is used by api, original frame buffer is not used inside api.
                i32 inputPicBufIndex = FindInputPicBufIdByBusAddr(tb, pEncIn->busLuma,
                                                                  cml->formatCustomizedType != -1);
                if (NOK == ReturnBufferById(tb->inputMemFlags, tb->buffer_cnt, inputPicBufIndex))
                    return NOK;

                pEncIn->busLuma = tb->inputCtbLineBuf.lumBuf.busAddress;
                pEncIn->busChromaU = tb->inputCtbLineBuf.cbBuf.busAddress;
                pEncIn->busChromaV = tb->inputCtbLineBuf.crBuf.busAddress;
                for (tileId = 1; tileId < tb->tileColumnNum; tileId++) {
                    pEncIn->tileExtra[tileId - 1].busLuma = tb->inputCtbLineBuf.lumBuf.busAddress;
                    pEncIn->tileExtra[tileId - 1].busChromaU = tb->inputCtbLineBuf.cbBuf.busAddress;
                    pEncIn->tileExtra[tileId - 1].busChromaV = tb->inputCtbLineBuf.crBuf.busAddress;
                }

                /* In loop back mode, data in line buffer start from the line to be encoded*/
                VCEncGetPreProcessing(encoder, &preProcCfg);
                for (tileId = 0; tileId < tb->tileColumnNum; tileId++) {
                    u32 *yOffset = (tileId == 0) ? (&preProcCfg.yOffset)
                                                 : (&preProcCfg.tileExtra[tileId - 1].yOffset);
                    *yOffset = 0;
                }
                VCEncSetPreProcessing(encoder, &preProcCfg);
            }

            tb->inputCtbLineBuf.lumSrc = tb->lum;
            tb->inputCtbLineBuf.cbSrc = tb->cb;
            tb->inputCtbLineBuf.crSrc = tb->cr;
            tb->inputCtbLineBuf.wrCnt = 0;
            pEncIn->lineBufWrCnt = VCEncStartInputLineBuffer(&(tb->inputCtbLineBuf), HANTRO_TRUE);
            pEncIn->initSegNum = tb->inputCtbLineBuf.initSegNum;
        }

        /* 7. Setup Overlay input buffer */
        SetupOverlayBuffer(tb, cml, pEncIn);

        /* 8. Set resend Parameter Set flag for IDR frame */
        if (cml->resendParamSet && tb->picture_enc_cnt) {
            pEncIn->resendPPS = pEncIn->resendSPS = pEncIn->resendVPS = 0;
            /* 2 types of IDR */
            //Force IDR
            if (pEncIn->bIsIDR) {
                pEncIn->resendPPS = pEncIn->resendSPS = pEncIn->resendVPS = 1;
                tb->lastForceIntra = tb->picture_enc_cnt;
            } else if (tb->idr_interval) {
                //intraPicRate
                u8 resend = ((tb->picture_enc_cnt - tb->lastForceIntra) % tb->idr_interval) == 0;
                pEncIn->resendPPS = pEncIn->resendSPS = pEncIn->resendVPS = resend;
            }
        }

        /* Just for Intel test use */
        if (tb->replaceMvFile != NULL)
            SetupMvReplaceBuffer(tb, cml, pEncIn);

        gettimeofday(&readEnd, NULL);
        ioTime += uTimeDiff(readEnd, readStart);

        gettimeofday(&tb->timeFrameStart, NULL);

        printf("=== Encoding %i %s ...\n", tb->picture_enc_cnt, tb->input);
        if (tb->encIn.picture_cnt == seiFrameId) {
            pEncIn->externalSEICount = ReadExternalSeiData(&pEncIn->pExternalSEI, tb->extSEIFile);
            if (!feof(tb->extSEIFile)) {
                fret = fread(&seiFrameId, sizeof(u32), 1, tb->extSEIFile);
            }
        } else {
            pEncIn->externalSEICount = 0;
            pEncIn->pExternalSEI = NULL;
        }

        /* AV1: ITU-T-T35 will use the pExternalSEI as buffer*/
        if (IS_AV1(cml->codecFormat)) {
            if (tb->encIn.picture_cnt == t35FrameId) {
                pEncIn->externalSEICount =
                    ReadT35PayloadData(&pEncIn->pExternalSEI, tb->p_t35_file);
                if (!feof(tb->p_t35_file)) {
                    fret = fread(&t35FrameId, sizeof(u32), 1, tb->p_t35_file);
                }
            } else {
                pEncIn->externalSEICount = 0;
                pEncIn->pExternalSEI = NULL;
            }
        }
#ifdef VMAF_SUPPORT
        if (cml->tune == 4) {
            vmaf_frame_preprocessing(encoder, cml->bitDepthLuma != DEFAULT ? cml->bitDepthLuma : 8,
                                     tb->lum, tb->cb, tb->cr, cml->width, cml->height,
                                     &tb->vmafctrl);
        }
#endif

        ret = VCEncStrmEncode(encoder, pEncIn, &encOut, &HEVCSliceReady, &tb->sliceCtl);
        if (pEncIn->externalSEICount != 0) {
            for (int k = 0; k < pEncIn->externalSEICount; k++) {
                free(pEncIn->pExternalSEI[k].pPayloadData);
            }
            free(pEncIn->pExternalSEI);
            pEncIn->externalSEICount = 0;
            pEncIn->pExternalSEI = NULL;
        }

        switch (ret) {
        case VCENC_FRAME_ENQUEUE:
            tb->picture_enc_cnt++;

            if (tb->flexRefsFile == NULL) {
                pEncIn->picture_cnt++;
            }
            pEncIn->timeIncrement = tb->outputRateDenom;
            break;
        case VCENC_FRAME_READY:
            if (encOut.codingType != VCENC_NOTCODED_FRAME) {
                tb->picture_enc_cnt++;
                if (encOut.streamSize == 0) {
                    tb->picture_encoded_cnt++;
                }
            }

            if (encOut.streamSize == 0) {
                tb->encIn.picture_cnt++;
                break;
            }

            if (cml->inputLineBufMode) {
                VCEncUpdateInitSegNum(&(tb->inputCtbLineBuf));
            }

            gettimeofday(&writeStart, NULL);
            processFrame(tb, encoder, cml, &encOut, &tb->sliceCtl, &streamSize, &maxSliceSize, &ma,
                         &total_bits, &outIdrFileOffset);
            gettimeofday(&writeEnd, NULL);
            frameWriteTime = uTimeDiff(writeEnd, writeStart);
            ioTime += frameWriteTime;

            if (tb->flexRefsFile == NULL) {
                pEncIn->picture_cnt++;
            }

            gettimeofday(&tb->timeFrameEnd, NULL);
            printf("=== Time(us %u HW+SW) ===\n",
                   uTimeDiff(tb->timeFrameEnd, tb->timeFrameStart) - frameWriteTime);

            if (pUserData) {
                /* We want the user data to be written only once so
           * we disable the user data and free the memory after
           * first frame has been encoded. */
                VCEncSetSeiUserData(encoder, NULL, 0);
                free(pUserData);
                pUserData = NULL;
            }
            break;
        case VCENC_OUTPUT_BUFFER_OVERFLOW:
            tb->encIn.picture_cnt++;
            break;
        default:
            Error(2, ERR, "VCEncStrmEncode() fails");
            goto error;
            break;
        }

        if (NOK == ReturnIOBuffer(tb, cml, &encOut.consumedAddr, encOut.picture_cnt)) {
            Error(2, ERR, "Manage IO buffer failed");
        }

        if (cml->profile == VCENC_HEVC_MAIN_STILL_PICTURE_PROFILE)
            break;
        //update last picture num
        last_picture_num = cur_picture_num;
    }

    /* Flush encoder internal buffered frames
    * In Multicore enabled or 2-pass enabled, encoder internal cached frames
    * must be flushed
    */
    while ((ret = VCEncFlush(encoder, pEncIn, &encOut, &HEVCSliceReady, &tb->sliceCtl)) !=
           VCENC_OK) {
        switch (ret) {
        case VCENC_FRAME_READY:
            if (encOut.streamSize == 0) {
                if (encOut.codingType != VCENC_NOTCODED_FRAME)
                    tb->picture_encoded_cnt++;
                break;
            }
            gettimeofday(&writeStart, NULL);
            processFrame(tb, encoder, cml, &encOut, &tb->sliceCtl, &streamSize, &maxSliceSize, &ma,
                         &total_bits, &outIdrFileOffset);
            gettimeofday(&writeEnd, NULL);
            frameWriteTime = uTimeDiff(writeEnd, writeStart);
            ioTime += frameWriteTime;

            break;
        case VCENC_FRAME_ENQUEUE:
            //when 2pass + multicore and frameNum < lookaheadDelay,
            //API will return status enqueue when first frame is encoded in pass-2 in flush flow
            break;
        case VCENC_OUTPUT_BUFFER_OVERFLOW:
            break;
        default:
            Error(2, ERR, "VCEncFlush() fails");
            goto error;
            break;
        }
        if (NOK == ReturnIOBuffer(tb, cml, &encOut.consumedAddr, encOut.picture_cnt)) {
            Error(2, ERR, "Manage IO buffer failed");
        }
        if (NOK == GetFreeOutputBuffer(tb)) {
            Error(2, ERR, "GetFreeOutputBuffer() fails");
            goto error;
        }
        SetupOutputBuffer(tb, pEncIn);
    }

    //return buffer to buffer pull after flush
    if (NOK == ReturnIOBuffer(tb, cml, &encOut.consumedAddr, encOut.picture_cnt)) {
        Error(2, ERR, "Manage IO buffer failed");
    }

    gettimeofday(&timeEnd, NULL);

    totalTime = uTimeDiffLL(timeEnd, timeStart);
    printf("[Total time: %llu us], [IO time: %llu us]\n", totalTime, ioTime);
    printf("[Encode Total time: %llu us], [avgTime: %llu us]\n", totalTime - ioTime,
           (totalTime - ioTime) / frameCntTotal);

STOP:
    /* End stream */
    if (NOK == GetFreeOutputBuffer(tb)) {
        Error(2, ERR, "GetFreeOutputBuffer() fails");
        goto error;
    }
    SetupOutputBuffer(tb, pEncIn);

    ret = VCEncStrmEnd(encoder, pEncIn, &encOut);
    if (ret == VCENC_OK) {
        VCEncStrmBufs bufs;
        getStreamBufs(&bufs, tb, cml, HANTRO_FALSE, 0);
        if (cml->byteStream == 0) {
            //WriteNalSizesToFile(nalfile, encOut.pNaluSizeBuf,encOut.numNalus);
            if (!cml->disableEOS)
                writeNalsBufs(tb->out, &bufs, encOut.pNaluSizeBuf, encOut.numNalus, 0, 0, 0);
        } else {
            if (IS_VP9(cml->codecFormat) || (IS_AV1(cml->codecFormat) && cml->ivf)) {
                u32 offset = 0;
                for (i32 i = 0; i < encOut.numNalus; i++) {
                    u8 frameNotShow = (encOut.av1Vp9FrameNotShow >> i) & 1;
                    writeIvfFrame(tb->out, &bufs, encOut.pNaluSizeBuf[i], offset,
                                  &tb->ivf_frame_cnt,
                                  IS_AV1(cml->codecFormat) && cml->modifiedTileGroupSize,
                                  frameNotShow, &tb->ivfSize, IS_VP9(cml->codecFormat));
                    offset += encOut.pNaluSizeBuf[i];
                    streamSize += 12;
                }
            } else {
                if (!cml->disableEOS)
                    writeStrmBufs(tb->out, &bufs, 0, encOut.streamSize, 0);
            }
        }
        streamSize += encOut.streamSize;
    }
    //return buffer after finish encoding a stream
    if (NOK == ReturnIOBuffer(tb, cml, &encOut.consumedAddr, encOut.picture_cnt)) {
        Error(2, ERR, "Manage IO buffer failed");
    }

    encRet = OK;

    printf("Total of %d frames processed, %d frames encoded, %lu bytes, "
           "maxSliceBytes=%d\n",
           frameCntTotal, tb->picture_encoded_cnt, (long unsigned int)streamSize, maxSliceSize);

#ifdef INTERNAL_TEST
    if (cml->bRcLog) {
        printf("[RC] MovingBitrate: Max=%.2f%% Min=%.2f%% avgDeviation =%.2f%%; "
               "frameBitsAvgDeviation=%.2f%%\n",
               tb->bitStat.maxMovingBitRate, tb->bitStat.minMovingBitRate,
               tb->bitStat.sumDeviation / tb->bitStat.nStatFrame, GetBitsDeviation(tb));
    }
#endif

#ifdef STATS_RDO_COUNT
    extern u32 rdo_refine_cnt[4];
    printf("rdo64=%d, rdo32=%d, rdo16=%d, rdo8=%d\n", rdo_refine_cnt[0], rdo_refine_cnt[1],
           rdo_refine_cnt[2], rdo_refine_cnt[3]);

    extern u32 interintra_skipintra_cnt[6 + 1];
    for (int i = 3; i < 7; i++) {
        printf("InterIntra pk cu%d skipintra_cnt %d total %d\n", 1 << i,
               interintra_skipintra_cnt[i], interintra_cnt[i]);
    }

#endif
error:

#ifdef TEST_DATA
    EncTraceReconEnd();
#endif

#ifdef VMAF_SUPPORT
    if (cml->tune == 4) {
        vmaf_free_frame_buffer(encoder, &tb->vmafctrl.source_extended);
        vmaf_free_frame_buffer(encoder, &tb->vmafctrl.blurred);
        vmaf_free_frame_buffer(encoder, &tb->vmafctrl.sharpened);
    }
#endif

    VC_FCLOSE(tb->in);
    VC_FCLOSE(tb->inDS);
    VC_FCLOSE(tb->out);
    VC_FCLOSE(tb->flexRefsFile);
    VC_FCLOSE(tb->roiMapFile);
    VC_FCLOSE(tb->roiMapBinFile);
    VC_FCLOSE(tb->ipcmMapFile);
    VC_FCLOSE(tb->fmv);
    VC_FCLOSE(tb->gmvFile[0]);
    VC_FCLOSE(tb->gmvFile[1]);
    VC_FCLOSE(tb->skipMapFile);
    VC_FCLOSE(tb->roiMapInfoBinFile);
    VC_FCLOSE(tb->RoimapCuCtrlInfoBinFile);
    VC_FCLOSE(tb->RoimapCuCtrlIndexBinFile);
    VC_FCLOSE(tb->dec400Table);
    VC_FCLOSE(tb->osdDec400TableFile);
    for (i = 0; i < MAX_OVERLAY_NUM; i++) {
        VC_FCLOSE(tb->overlayFile[i]);
    }
    VC_FCLOSE(tb->extSEIFile);
    VC_FCLOSE(tb->rdLogFile);
    VC_FCLOSE(tb->p_t35_file);

    if (encRet != OK) {
        Error(2, ERR, "encode() fails");
        VCEncSetError(encoder);
    }

    return encRet;
}

/*------------------------------------------------------------------------------

    OpenEncoder
        Create and configure an encoder instance.

    Params:
        cml     - processed comand line options
        pEnc    - place where to save the new encoder instance
    Return:
        0   - for success
        -1  - error

------------------------------------------------------------------------------*/
int OpenEncoder(commandLine_s *cml, VCEncInst *pEnc, struct test_bench *tb)
{
    VCEncRet ret;
    VCEncConfig cfg;
    VCEncCodingCtrl codingCfg;
    VCEncRateCtrl rcCfg;
    VCEncPreProcessingCfg preProcCfg;
    VCEncInst encoder;
    i32 i;
    const void *ewl_inst = NULL;

    //init parameters.
    memset(&cfg, 0, sizeof(VCEncConfig));
    memset(&codingCfg, 0, sizeof(VCEncCodingCtrl));
    memset(&rcCfg, 0, sizeof(VCEncRateCtrl));
    memset(&preProcCfg, 0, sizeof(VCEncPreProcessingCfg));

    cfg.tile_width = tb->tile_width;
    cfg.tile_height = tb->tile_height;

    preProcCfg.tileExtra = tb->vcencPrpTileExtra;

    /* Default resolution, try parsing input file name */
    if (cml->lumWidthSrc == DEFAULT || cml->lumHeightSrc == DEFAULT) {
        if (GetResolution(cml->input, &cml->lumWidthSrc, &cml->lumHeightSrc)) {
            /* No dimensions found in filename, using default QCIF */
            cml->lumWidthSrc = 176;
            cml->lumHeightSrc = 144;
        }
    }

    /* Encoder initialization */
    if (cml->width == DEFAULT)
        cml->width = cml->lumWidthSrc;

    if (cml->height == DEFAULT)
        cml->height = cml->lumHeightSrc;

    ChangeCmlCustomizedFormat(cml);

    /* outputRateNumer */
    if (cml->outputRateNumer == DEFAULT)
        cml->outputRateNumer = cml->inputRateNumer;

    /* outputRateDenom */
    if (cml->outputRateDenom == DEFAULT)
        cml->outputRateDenom = cml->inputRateDenom;

    /*cfg.ctb_size = cml->max_cu_size;*/
    if (cml->rotation && cml->rotation != 3) {
        cfg.width = cml->height;
        cfg.height = cml->width;
    } else {
        cfg.width = cml->width;
        cfg.height = cml->height;
    }

    cfg.frameRateDenom = cml->outputRateDenom;
    cfg.frameRateNum = cml->outputRateNumer;

    /* intra tools in sps and pps */
    cfg.strongIntraSmoothing = cml->strong_intra_smoothing_enabled_flag;

    cfg.streamType = (cml->byteStream) ? VCENC_BYTE_STREAM : VCENC_NAL_UNIT_STREAM;

    cfg.level = (IS_H264(cml->codecFormat) ? VCENC_H264_LEVEL_5_1 : VCENC_HEVC_LEVEL_6);

    if (cml->level != DEFAULT)
        cfg.level = (VCEncLevel)cml->level;
    else
        cfg.level = VCENC_AUTO_LEVEL; /*api will calculate level.*/

    /* adpate to our native test case with level 0, which is used to be as auto level, customer don't need to  refer to it */
    if (cml->level == 0)
        cfg.level = VCENC_AUTO_LEVEL;

    cfg.tier = VCENC_HEVC_MAIN_TIER;
    if (cml->tier != DEFAULT)
        cfg.tier = (VCEncTier)cml->tier;

    cfg.profile = (IS_H264(cml->codecFormat) ? VCENC_H264_HIGH_PROFILE : VCENC_HEVC_MAIN_PROFILE);

    cfg.codecFormat = cml->codecFormat;

    if (cml->profile != DEFAULT && cml->profile != 0)
        cfg.profile = (VCEncProfile)cml->profile;

    /* convert between H.264/HEVC profiles for testing purpose */
    if (IS_H264(cml->codecFormat)) {
        if (((int)cfg.profile >= VCENC_HEVC_MAIN_PROFILE &&
             cfg.profile < VCENC_HEVC_MAIN_10_PROFILE) ||
            cfg.profile == VCENC_HEVC_MAINREXT)
            cfg.profile = VCENC_H264_HIGH_PROFILE;
    } else {
        if (cfg.profile >= VCENC_H264_BASE_PROFILE && cfg.profile <= VCENC_H264_HIGH_PROFILE)
            cfg.profile = VCENC_HEVC_MAIN_PROFILE;
    }

    cfg.bitDepthLuma = 8;
    if (cml->bitDepthLuma != DEFAULT)
        cfg.bitDepthLuma = cml->bitDepthLuma;

    cfg.bitDepthChroma = 8;
    if (cml->bitDepthChroma != DEFAULT)
        cfg.bitDepthChroma = cml->bitDepthChroma;

    if (cml->codecFormat == VCENC_VIDEO_CODEC_VP9) {
        if (cfg.bitDepthLuma != cfg.bitDepthChroma) {
            printf("Different bit depth for luma and chroma is not supported for VP9\n");
            cfg.bitDepthChroma = cfg.bitDepthLuma;
        }
    }

    if ((cml->interlacedFrame && cml->gopSize != 1) || IS_H264(cml->codecFormat)) {
        //printf("OpenEncoder: treat interlace to progressive for gopSize!=1 case\n");
        cml->interlacedFrame = 0;
    }

    //default maxTLayer
    cfg.maxTLayers = 1;

    /* Find the max number of reference frame */
    if (cml->intraPicRate == 1) {
        cfg.refFrameAmount = 0;
    } else {
        u32 maxRefPics = 0;
        u32 maxTemporalId = 0;
        int idx;
        for (idx = 0; idx < tb->encIn.gopConfig.size; idx++) {
            VCEncGopPicConfig *cfg = &(tb->encIn.gopConfig.pGopPicCfg[idx]);
            if (cfg->codingType != VCENC_INTRA_FRAME) {
                if (maxRefPics < cfg->numRefPics)
                    maxRefPics = cfg->numRefPics;

                if (maxTemporalId < cfg->temporalId)
                    maxTemporalId = cfg->temporalId;
            }
        }
        for (idx = 0; idx < tb->encIn.gopConfig.special_size; idx++) {
            VCEncGopPicSpecialConfig *cfg = &(tb->encIn.gopConfig.pGopPicSpecialCfg[idx]);

            if ((cfg->temporalId != TEMPORALID_RESERVED) && (maxTemporalId < cfg->temporalId))
                maxTemporalId = cfg->temporalId;
        }
        cfg.refFrameAmount = maxRefPics + cml->interlacedFrame + tb->encIn.gopConfig.ltrcnt;
        cfg.maxTLayers = maxTemporalId + 1;
        if (cml->flexRefs != NULL) {
            cfg.refFrameAmount = 4;
            cfg.maxTLayers = 4;
        }
    }
    cfg.compressor = cml->compressor;
    cfg.interlacedFrame = cml->interlacedFrame;
    cfg.enableOutputCuInfo = (cml->enableOutputCuInfo > 0) ? 1 : 0;
    cfg.enableOutputCtbBits = (cml->enableOutputCtbBits > 0) ? 1 : 0;
    cfg.rdoLevel = CLIP3(1, 3, cml->rdoLevel) - 1;
    cfg.verbose = cml->verbose;
    cfg.exp_of_input_alignment = cml->exp_of_input_alignment;
    cfg.exp_of_ref_alignment = cml->exp_of_ref_alignment;
    cfg.exp_of_ref_ch_alignment = cml->exp_of_ref_ch_alignment;
    cfg.exp_of_aqinfo_alignment = cml->exp_of_aqinfo_alignment;
    cfg.exp_of_tile_stream_alignment = cml->exp_of_tile_stream_alignment;
    cfg.exteralReconAlloc = 0;
    cfg.P010RefEnable = cml->P010RefEnable;
    cfg.enableSsim = cml->ssim;
    cfg.enablePsnr = cml->psnr;
    cfg.ctbRcMode = (cml->ctbRc != DEFAULT) ? cml->ctbRc : 0;
    cfg.parallelCoreNum = cml->parallelCoreNum;
    cfg.pass = (cml->lookaheadDepth ? 2 : 0);
    cfg.bPass1AdaptiveGop = (cml->gopSize == 0);
    cfg.picOrderCntType = cml->picOrderCntType;
    cfg.dumpRegister = cml->dumpRegister;
    cfg.dumpCuInfo = (cml->enableOutputCuInfo == 2) ? 1 : 0;
    cfg.dumpCtbBits = (cml->enableOutputCtbBits == 2) ? 1 : 0;
    cfg.rasterscan = cml->rasterscan;
    cfg.log2MaxPicOrderCntLsb = cml->log2MaxPicOrderCntLsb;
    cfg.log2MaxFrameNum = cml->log2MaxFrameNum;
    cfg.lookaheadDepth = cml->lookaheadDepth;
    cfg.extDSRatio = (cml->lookaheadDepth && cml->halfDsInput ? 1 : 0);
    cfg.inLoopDSRatio = cml->inLoopDSRatio;
#ifdef INTERNAL_TEST
    // inLoopDSRatio change must apply before VCEncInit for buffer allocation based on resolution.
    if (cml->testId == TID_IMOUTBLOCKUNIT1 || cml->testId == TID_IMOUTBLOCKUNIT2 ||
        cml->testId == TID_IMOUTBLOCKUNIT3)
        cfg.inLoopDSRatio = 0;
    else if (cml->testId == TID_IMOUTBLOCKUNIT4 || cml->testId == TID_IMOUTBLOCKUNIT5)
        cfg.inLoopDSRatio = 1;
#endif
    cfg.cuInfoVersion = cml->cuInfoVersion;
    cfg.extSramLumHeightBwd = cml->extSramLumHeightBwd;
    cfg.extSramChrHeightBwd = cml->extSramChrHeightBwd;
    cfg.extSramLumHeightFwd = cml->extSramLumHeightFwd;
    cfg.extSramChrHeightFwd = cml->extSramChrHeightFwd;
    cfg.AXIAlignment = cml->AXIAlignment;
    cfg.irqTypeMask = cml->irqTypeMask;
    cfg.irqTypeCutreeMask = cml->irqTypeCutreeMask;
    cfg.tiles_enabled_flag = cml->tiles_enabled_flag;
    cfg.num_tile_columns = cml->num_tile_columns;
    cfg.num_tile_rows = cml->num_tile_rows;
    cfg.loop_filter_across_tiles_enabled_flag = cml->loop_filter_across_tiles_enabled_flag;
    cfg.tileMvConstraint = cml->tileMvConstraint;
    cfg.refRingBufEnable = cml->refRingBufEnable;

    /* temporal cal */
    u32 log2_ctb_size = IS_H264(cml->codecFormat) ? 4 : 6;
    i32 width = ((cml->rotation == 1) || (cml->rotation == 2)) ? cml->height : cml->width;
    i32 height = ((cml->rotation == 1) || (cml->rotation == 2)) ? cml->width : cml->height;
    u32 ctbPerRow = ((width + (1 << log2_ctb_size) - 1) >> log2_ctb_size);
    u32 ctbPerColumn = ((height + (1 << log2_ctb_size) - 1) >> log2_ctb_size);
    u32 tileId;
    for (tileId = 0; tileId < cml->num_tile_columns; tileId++) {
        cfg.tile_width[tileId] = cml->tile_width[tileId] =
            (tileId + 1) * ctbPerRow / cml->num_tile_columns -
            (tileId * ctbPerRow) / cml->num_tile_columns;
    }
    for (tileId = 0; tileId < cml->num_tile_rows; tileId++) {
        cfg.tile_height[tileId] = cml->tile_height[tileId] =
            (tileId + 1) * ctbPerColumn / cml->num_tile_rows -
            (tileId * ctbPerColumn) / cml->num_tile_rows;
    }

    cfg.slice_idx = cml->sliceNode;
    cfg.gopSize = tb->gopSize;

    if (cml->parallelCoreNum > 1 && cfg.width * cfg.height < 256 * 256) {
        printf("Disable multicore for small resolution (< 255*255)\n");
        cfg.parallelCoreNum = cml->parallelCoreNum = 1;
    }
    cfg.codedChromaIdc = cml->codedChromaIdc;
    cfg.writeReconToDDR = (cml->writeReconToDDR || cml->intraPicRate != 1);
    cfg.TxTypeSearchEnable = cml->TxTypeSearchEnable;
    cfg.av1InterFiltSwitch = cml->av1InterFiltSwitch;
    cfg.tune = cml->tune;
    cfg.burstMaxLength = cml->burstMaxLength;

    cfg.fileListExist = (tb->inputFileLinkListNum) ? 1 : 0;

    /* VP9 support TMVP; HEVC/AV1 support TMVP if (tb->hwCfg.temporalMvpSupport),
     if TMVP is not set by user, it's default setting depends on HW capability */
    if (cml->enableTMVP == DEFAULT) {
        i32 tmvpSupport = IS_VP9(cml->codecFormat) ||
                          (tb->hwCfg.temporalMvpSupport && !IS_H264(cml->codecFormat));
        cfg.enableTMVP = tmvpSupport;
    } else
        cfg.enableTMVP = cml->enableTMVP;

    //test bench not supprot binding IO buffer, init bIOBufferBinding as 0
    cfg.bIOBufferBinding = 0;

    if (cml->reEncode == 1) {
        cfg.cb_try_new_params = ReEncodeCurrentFrame;
    }

    if ((ret = VCEncInit(&cfg, pEnc, NULL)) != VCENC_OK) {
        //PrintErrorValue("VCEncInit() failed.", ret);
        return (int)ret;
    }

    encoder = *pEnc;
    ewl_inst = InitEwlInst(cml, tb);

    /* Encoder setup: coding control */
    if (0 != SetCodingCtrl(tb, encoder, cml)) {
        CloseEncoder(encoder, tb);
        return -1;
    }

    /* Encoder setup: rate control */
    if ((ret = VCEncGetRateCtrl(encoder, &rcCfg)) != VCENC_OK) {
        //PrintErrorValue("VCEncGetRateCtrl() failed.", ret);
        CloseEncoder(encoder, tb);
        return -1;
    } else {
        printf("Get rate control: qp %2d qpRange I[%2d, %2d] PB[%2d, %2d] %8d bps  "
               "pic %d skip %d  hrd %d  cpbSize %d cpbMaxRate %d bitrateWindow %d "
               "intraQpDelta %2d\n",
               rcCfg.qpHdr, rcCfg.qpMinI, rcCfg.qpMaxI, rcCfg.qpMinPB, rcCfg.qpMaxPB,
               rcCfg.bitPerSecond, rcCfg.pictureRc, rcCfg.pictureSkip, rcCfg.hrd, rcCfg.hrdCpbSize,
               rcCfg.cpbMaxRate, rcCfg.bitrateWindow, rcCfg.intraQpDelta);

        if (cml->qpHdr != DEFAULT)
            rcCfg.qpHdr = cml->qpHdr;
        else
            rcCfg.qpHdr = -1;
        if (cml->qpMin != DEFAULT)
            rcCfg.qpMinI = rcCfg.qpMinPB = cml->qpMin;
        if (cml->qpMax != DEFAULT)
            rcCfg.qpMaxI = rcCfg.qpMaxPB = cml->qpMax;
        if (cml->qpMinI != DEFAULT)
            rcCfg.qpMinI = cml->qpMinI;
        if (cml->qpMaxI != DEFAULT)
            rcCfg.qpMaxI = cml->qpMaxI;
        if (cml->picSkip != DEFAULT)
            rcCfg.pictureSkip = cml->picSkip;
        if (cml->picRc != DEFAULT)
            rcCfg.pictureRc = cml->picRc;
        if (cml->ctbRc != DEFAULT) {
            if (cml->ctbRc == 4 || cml->ctbRc == 6) {
                rcCfg.ctbRc = cml->ctbRc - 3;
                rcCfg.ctbRcQpDeltaReverse = 1;
            } else {
                rcCfg.ctbRc = cml->ctbRc;
                rcCfg.ctbRcQpDeltaReverse = 0;
            }
        }

        rcCfg.blockRCSize = 0;
        if (cml->blockRCSize != DEFAULT)
            rcCfg.blockRCSize = cml->blockRCSize;

        rcCfg.rcQpDeltaRange = 10;
        if (cml->rcQpDeltaRange != DEFAULT)
            rcCfg.rcQpDeltaRange = cml->rcQpDeltaRange;

        rcCfg.rcBaseMBComplexity = 15;
        if (cml->rcBaseMBComplexity != DEFAULT)
            rcCfg.rcBaseMBComplexity = cml->rcBaseMBComplexity;

        if (cml->picQpDeltaMax != DEFAULT)
            rcCfg.picQpDeltaMax = cml->picQpDeltaMax;
        if (cml->picQpDeltaMin != DEFAULT)
            rcCfg.picQpDeltaMin = cml->picQpDeltaMin;
        if (cml->bitPerSecond != DEFAULT)
            rcCfg.bitPerSecond = cml->bitPerSecond;
        if (cml->cpbMaxRate != DEFAULT)
            rcCfg.cpbMaxRate = cml->cpbMaxRate;
        if (cml->bitVarRangeI != DEFAULT)
            rcCfg.bitVarRangeI = cml->bitVarRangeI;
        if (cml->bitVarRangeP != DEFAULT)
            rcCfg.bitVarRangeP = cml->bitVarRangeP;
        if (cml->bitVarRangeB != DEFAULT)
            rcCfg.bitVarRangeB = cml->bitVarRangeB;

        if (cml->tolMovingBitRate != DEFAULT)
            rcCfg.tolMovingBitRate = cml->tolMovingBitRate;

        if (cml->tolCtbRcInter != DEFAULT)
            rcCfg.tolCtbRcInter = cml->tolCtbRcInter;

        if (cml->tolCtbRcIntra != DEFAULT)
            rcCfg.tolCtbRcIntra = cml->tolCtbRcIntra;

        if (cml->ctbRcRowQpStep != DEFAULT)
            rcCfg.ctbRcRowQpStep = cml->ctbRcRowQpStep;

        rcCfg.longTermQpDelta = cml->longTermQpDelta;

        if (cml->monitorFrames != DEFAULT)
            rcCfg.monitorFrames = cml->monitorFrames;
        else {
            rcCfg.monitorFrames =
                (cml->outputRateNumer + cml->outputRateDenom - 1) / cml->outputRateDenom;
            cml->monitorFrames =
                (cml->outputRateNumer + cml->outputRateDenom - 1) / cml->outputRateDenom;
        }

        if (rcCfg.monitorFrames > MOVING_AVERAGE_FRAMES)
            rcCfg.monitorFrames = MOVING_AVERAGE_FRAMES;

        if (rcCfg.monitorFrames < 10) {
            rcCfg.monitorFrames =
                (cml->outputRateNumer > cml->outputRateDenom) ? 10 : LEAST_MONITOR_FRAME;
        }

        if (cml->fillerData != HANTRO_FALSE)
            rcCfg.fillerData = cml->fillerData;

        if (cml->hrdConformance != DEFAULT)
            rcCfg.hrd = cml->hrdConformance;

        if (cml->cpbSize != DEFAULT)
            rcCfg.hrdCpbSize = cml->cpbSize;

        if (cml->intraPicRate != 0)
            rcCfg.bitrateWindow = MIN(cml->intraPicRate, MAX_GOP_LEN);

        if (cml->bitrateWindow != DEFAULT)
            rcCfg.bitrateWindow = cml->bitrateWindow;

        if (cml->intraQpDelta != DEFAULT)
            rcCfg.intraQpDelta = cml->intraQpDelta;

        if (cml->vbr != DEFAULT)
            rcCfg.vbr = cml->vbr;

        if (cml->crf != DEFAULT)
            rcCfg.crf = cml->crf;

        rcCfg.rcMode = cml->rcMode;
        rcCfg.fixedIntraQp = cml->fixedIntraQp;
        rcCfg.smoothPsnrInGOP = cml->smoothPsnrInGOP;
        rcCfg.u32StaticSceneIbitPercent = cml->u32StaticSceneIbitPercent;

        rcCfg.frameRateNum = cml->outputRateNumer;
        rcCfg.frameRateDenom = cml->outputRateDenom;

        printf("Set rate control: qp %2d qpRange I[%2d, %2d] PB[%2d, %2d] %9d bps  "
               "pic %d skip %d  hrd %d"
               "  cpbSize %d cpbMaxRate %d bitrateWindow %d intraQpDelta %2d "
               "fixedIntraQp %2d\n",
               rcCfg.qpHdr, rcCfg.qpMinI, rcCfg.qpMaxI, rcCfg.qpMinPB, rcCfg.qpMaxPB,
               rcCfg.bitPerSecond, rcCfg.pictureRc, rcCfg.pictureSkip, rcCfg.hrd, rcCfg.hrdCpbSize,
               rcCfg.cpbMaxRate, rcCfg.bitrateWindow, rcCfg.intraQpDelta, rcCfg.fixedIntraQp);

        if ((ret = VCEncSetRateCtrl(encoder, &rcCfg)) != VCENC_OK) {
            //PrintErrorValue("VCEncSetRateCtrl() failed.", ret);
            CloseEncoder(encoder, tb);
            return -1;
        }
    }

    /* Optional scaled image output */
    if (cml->scaledWidth * cml->scaledHeight > 0) {
        i32 dsFrmSize = cml->scaledWidth * cml->scaledHeight * 2;
        /* the scaled image size changes frame by frame when testing down-scaling */
        if (cml->testId == 34) {
            dsFrmSize = cml->width * cml->height * 2;
        }
        if ((cml->bitDepthLuma != 8) || (cml->bitDepthChroma != 8))
            dsFrmSize <<= 1;

        if (cml->scaledOutputFormat == 1)
            dsFrmSize = dsFrmSize * 3 / 4;

        tb->scaledPictureMem.mem_type = VPU_WR | EWL_MEM_TYPE_DPB;
        if (EWLMallocRefFrm(ewl_inst, dsFrmSize, 0, &tb->scaledPictureMem) != EWL_OK) {
            tb->scaledPictureMem.virtualAddress = NULL;
            tb->scaledPictureMem.busAddress = 0;
        }
    }

    /* PreP setup */
    if ((ret = VCEncGetPreProcessing(encoder, &preProcCfg)) != VCENC_OK) {
        //PrintErrorValue("VCEncGetPreProcessing() failed.", ret);
        CloseEncoder(encoder, tb);
        return -1;
    }

    printf("Get PreP: format %d : rotation %d cc %d : scaling %d\n", preProcCfg.inputType,
           preProcCfg.rotation, preProcCfg.colorConversion.type, preProcCfg.scaledOutput);

    u32 origWidth;
    u32 origHeight;
    u32 xOffset;
    u32 yOffset;
    for (tileId = 0; tileId < cml->num_tile_columns; tileId++) {
        origWidth =
            (tileId == 0) ? preProcCfg.origWidth : preProcCfg.tileExtra[tileId - 1].origWidth;
        origHeight =
            (tileId == 0) ? preProcCfg.origHeight : preProcCfg.tileExtra[tileId - 1].origHeight;
        xOffset = (tileId == 0) ? preProcCfg.xOffset : preProcCfg.tileExtra[tileId - 1].xOffset;
        yOffset = (tileId == 0) ? preProcCfg.yOffset : preProcCfg.tileExtra[tileId - 1].yOffset;
        printf("input%u %4dx%d : offset%u %4dx%d\n", tileId, origWidth, origHeight, tileId, xOffset,
               yOffset);
    }

    preProcCfg.inputType = (VCEncPictureType)cml->inputFormat;
    preProcCfg.rotation = (VCEncPictureRotation)cml->rotation;
    preProcCfg.mirror = (VCEncPictureMirror)cml->mirror;

    i32 horOff = MAX(cml->horOffsetSrc, 0);
    i32 verOff = MAX(cml->verOffsetSrc, 0);
    for (tileId = 0; tileId < cml->num_tile_columns; tileId++) {
        if (tileId == 0) {
            preProcCfg.origWidth = cml->lumWidthSrc;
            preProcCfg.origHeight = cml->lumHeightSrc;
            if (cml->horOffsetSrc != DEFAULT || cml->tiles_enabled_flag)
                preProcCfg.xOffset = horOff;
            if (cml->verOffsetSrc != DEFAULT || cml->tiles_enabled_flag)
                preProcCfg.yOffset = verOff;
            horOff += cml->tile_width[tileId] * 64;
        } else {
            preProcCfg.tileExtra[tileId - 1].origWidth = cml->lumWidthSrc;
            preProcCfg.tileExtra[tileId - 1].origHeight = cml->lumHeightSrc;
            if (cml->horOffsetSrc != DEFAULT || cml->tiles_enabled_flag)
                preProcCfg.tileExtra[tileId - 1].xOffset = horOff;
            if (cml->verOffsetSrc != DEFAULT || cml->tiles_enabled_flag)
                preProcCfg.tileExtra[tileId - 1].yOffset = verOff;
            horOff += cml->tile_width[tileId] * 64;
        }
    }

    /* assume input is a frame and we'll separate it as two field for interlace encoding */
    if (cml->interlacedFrame) {
        preProcCfg.origWidth *= 2;
        preProcCfg.origHeight /= 2;
        preProcCfg.yOffset /= 2;
    }

    /* stab setup */
    preProcCfg.videoStabilization = 0;
    if (cml->videoStab != DEFAULT) {
        preProcCfg.videoStabilization = cml->videoStab;
    }

    if (cml->colorConversion != DEFAULT)
        preProcCfg.colorConversion.type = (VCEncColorConversionType)cml->colorConversion;
    if (preProcCfg.colorConversion.type == VCENC_RGBTOYUV_USER_DEFINED) {
        preProcCfg.colorConversion.coeffA = 20000;
        preProcCfg.colorConversion.coeffB = 44000;
        preProcCfg.colorConversion.coeffC = 5000;
        preProcCfg.colorConversion.coeffE = 35000;
        preProcCfg.colorConversion.coeffF = 38000;
        preProcCfg.colorConversion.coeffG = 35000;
        preProcCfg.colorConversion.coeffH = 38000;
        preProcCfg.colorConversion.LumaOffset = 0;
    }

    if (cml->rotation && cml->rotation != 3) {
        preProcCfg.scaledWidth = cml->scaledHeight;
        preProcCfg.scaledHeight = cml->scaledWidth;
    } else {
        preProcCfg.scaledWidth = cml->scaledWidth;
        preProcCfg.scaledHeight = cml->scaledHeight;
    }
    preProcCfg.busAddressScaledBuff = tb->scaledPictureMem.busAddress;
    preProcCfg.virtualAddressScaledBuff = tb->scaledPictureMem.virtualAddress;
    preProcCfg.sizeScaledBuff = tb->scaledPictureMem.size;
    preProcCfg.input_alignment = 1 << cml->exp_of_input_alignment;
    preProcCfg.scaledOutputFormat = cml->scaledOutputFormat;

    printf("Set PreP: format %d : rotation %d cc %d : scaling %d : scaling format "
           "%d\n",
           preProcCfg.inputType, preProcCfg.rotation, preProcCfg.colorConversion.type,
           preProcCfg.scaledOutput, preProcCfg.scaledOutputFormat);

    for (tileId = 0; tileId < cml->num_tile_columns; tileId++) {
        origWidth =
            (tileId == 0) ? preProcCfg.origWidth : preProcCfg.tileExtra[tileId - 1].origWidth;
        origHeight =
            (tileId == 0) ? preProcCfg.origHeight : preProcCfg.tileExtra[tileId - 1].origHeight;
        xOffset = (tileId == 0) ? preProcCfg.xOffset : preProcCfg.tileExtra[tileId - 1].xOffset;
        yOffset = (tileId == 0) ? preProcCfg.yOffset : preProcCfg.tileExtra[tileId - 1].yOffset;
        printf("input%u %4dx%d : offset%u %4dx%d\n", tileId, origWidth, origHeight, tileId, xOffset,
               yOffset);
    }

    if (cml->scaledWidth * cml->scaledHeight > 0)
        preProcCfg.scaledOutput = 1;

    /* constant chroma control */
    preProcCfg.constChromaEn = cml->constChromaEn;
    if (cml->constCb != DEFAULT)
        preProcCfg.constCb = cml->constCb;
    if (cml->constCr != DEFAULT)
        preProcCfg.constCr = cml->constCr;

    ChangeToCustomizedFormat(cml, &preProcCfg);

    /* Set overlay area*/
    for (i = 0; i < MAX_OVERLAY_NUM; i++) {
        preProcCfg.overlayArea[i].xoffset = cml->olXoffset[i];
        preProcCfg.overlayArea[i].cropXoffset = cml->olCropXoffset[i];
        preProcCfg.overlayArea[i].yoffset = cml->olYoffset[i];
        preProcCfg.overlayArea[i].cropYoffset = cml->olCropYoffset[i];
        preProcCfg.overlayArea[i].width = cml->olWidth[i];
        preProcCfg.overlayArea[i].cropWidth = cml->olCropWidth[i];
        preProcCfg.overlayArea[i].height = cml->olHeight[i];
        preProcCfg.overlayArea[i].cropHeight = cml->olCropHeight[i];
        preProcCfg.overlayArea[i].format = cml->olFormat[i];
        preProcCfg.overlayArea[i].alpha = cml->olAlpha[i];
        preProcCfg.overlayArea[i].enable = (cml->overlayEnables >> i) & 1;
        preProcCfg.overlayArea[i].Ystride = cml->olYStride[i];
        preProcCfg.overlayArea[i].UVstride = cml->olUVStride[i];
        preProcCfg.overlayArea[i].bitmapY = cml->olBitmapY[i];
        preProcCfg.overlayArea[i].bitmapU = cml->olBitmapU[i];
        preProcCfg.overlayArea[i].bitmapV = cml->olBitmapV[i];
        preProcCfg.overlayArea[i].superTile = cml->olSuperTile[i];
        preProcCfg.overlayArea[i].scaleWidth = cml->olScaleWidth[i];
        preProcCfg.overlayArea[i].scaleHeight = cml->olScaleHeight[i];
    }

    /* Set mosaic region parameters */
    for (i = 0; i < MAX_MOSAIC_NUM; i++) {
        preProcCfg.mosEnable[i] = (cml->mosaicEnables >> i) & 1;
        preProcCfg.mosXoffset[i] = cml->mosXoffset[i];
        preProcCfg.mosYoffset[i] = cml->mosYoffset[i];
        preProcCfg.mosWidth[i] = cml->mosWidth[i];
        preProcCfg.mosHeight[i] = cml->mosHeight[i];
    }

    if ((ret = VCEncSetPreProcessing(encoder, &preProcCfg)) != VCENC_OK) {
        //PrintErrorValue("VCEncSetPreProcessing() failed.", ret);
        CloseEncoder(encoder, tb);
        return (int)ret;
    }
    return 0;
}
/*------------------------------------------------------------------------------

    CloseEncoder
       Release an encoder insatnce.

   Params:
        encoder - the instance to be released
------------------------------------------------------------------------------*/
void CloseEncoder(VCEncInst encoder, struct test_bench *tb)
{
    VCEncRet ret;
    const void *ewl_inst = tb->ewlInst;

    EWLFreeLinear(ewl_inst, &tb->scaledPictureMem);
    if ((ret = VCEncRelease(encoder)) != VCENC_OK) {
        //PrintErrorValue("VCEncRelease() failed.", ret);
    }
}
/*------------------------------------------------------------------------------

    AllocRes

    Allocation of the physical memories used by both SW and HW:
    the input pictures and the output stream buffer.

    NOTE! The implementation uses the EWL instance from the encoder
          for OS independence. This is not recommended in final environment
          because the encoder will release the EWL instance in case of error.
          Instead, the memories should be allocated from the OS the same way
          as inside EWLMallocLinear().

------------------------------------------------------------------------------*/
int AllocRes(commandLine_s *cmdl, VCEncInst enc, struct test_bench *tb)
{
    i32 ret;
    u32 pictureSize = 0;
    u32 pictureDSSize = 0;
    u32 streamBufRefSize;
    u32 outbufSize[2] = {0, 0};
    u32 block_size;
    u32 transform_size = 0;
    u32 lumaSize = 0, chromaSize = 0;
    u32 coreIdx = 0;
    i32 iBuf;
    u32 tileId;
    u32 coreNum;
    u32 alignment = 0;
    u32 extSramWidthAlignment = 0;
    const void *ewl_inst = tb->ewlInst;
    u32 bitDepthLuma = 8, bitDepthChroma = 8, extSramTotal = 0;
    u32 roiMapDeltaQpMemSize = 0;

    alignment = (cmdl->formatCustomizedType != -1 ? 0 : tb->input_alignment);
    getAlignedPicSizebyFormat(cmdl->inputFormat, cmdl->lumWidthSrc, cmdl->lumHeightSrc, alignment,
                              &lumaSize, &chromaSize, &pictureSize);

    tb->lumaSize = lumaSize;
    tb->chromaSize = chromaSize;

    if (cmdl->formatCustomizedType != -1) {
        u32 trans_format = VCENC_FORMAT_MAX;
        u32 input_alignment = 0;
        switch (cmdl->formatCustomizedType) {
        case 0:
            if (cmdl->inputFormat == VCENC_YUV420_PLANAR) {
                if (IS_HEVC(cmdl->codecFormat))
                    trans_format = VCENC_YUV420_PLANAR_8BIT_TILE_32_32;
                else
                    trans_format = VCENC_YUV420_PLANAR_8BIT_TILE_16_16_PACKED_4;
            }
            break;
        case 1: {
            if (cmdl->inputFormat == VCENC_YUV420_SEMIPLANAR ||
                cmdl->inputFormat == VCENC_YUV420_SEMIPLANAR_VU)
                trans_format = VCENC_YUV420_SEMIPLANAR_8BIT_TILE_4_4;
            else if (cmdl->inputFormat == VCENC_YUV420_PLANAR_10BIT_P010)
                trans_format = VCENC_YUV420_PLANAR_10BIT_P010_TILE_4_4;
            input_alignment = tb->input_alignment;
            break;
        }
        case 2:
            trans_format = VCENC_YUV420_SEMIPLANAR_101010;
            break;
        case 3:
        case 4:
            trans_format = VCENC_YUV420_8BIT_TILE_64_4;
            break;
        case 5:
            trans_format = VCENC_YUV420_10BIT_TILE_32_4;
            break;
        case 6:
        case 7:
            trans_format = VCENC_YUV420_10BIT_TILE_48_4;
            break;
        case 8:
        case 9:
            trans_format = VCENC_YUV420_8BIT_TILE_128_2;
            break;
        case 10:
        case 11:
            trans_format = VCENC_YUV420_10BIT_TILE_96_2;
            break;
        case 12:
            trans_format = VCENC_YUV420_8BIT_TILE_8_8;
            break;
        case 13:
            trans_format = VCENC_YUV420_10BIT_TILE_8_8;
            break;
        default:
            break;
        }

        getAlignedPicSizebyFormat(trans_format, cmdl->lumWidthSrc, cmdl->lumHeightSrc,
                                  input_alignment, NULL, NULL, &transform_size);
        tb->transformedSize = transform_size;
    }

    /* Here we use the EWL instance directly from the encoder
   * because it is the easiest way to allocate the linear memories */
    for (coreIdx = 0; coreIdx < tb->buffer_cnt; coreIdx++) {
        tb->pictureMemFactory[coreIdx].mem_type = EXT_WR | VPU_RD | EWL_MEM_TYPE_DPB;
        ret = EWLMallocLinear(ewl_inst, pictureSize, alignment, &tb->pictureMemFactory[coreIdx]);
        if (ret != EWL_OK) {
            tb->pictureMemFactory[coreIdx].virtualAddress = NULL;
            return 1;
        }

        if (cmdl->formatCustomizedType != -1 && transform_size != 0) {
            tb->transformMemFactory[coreIdx].mem_type = EXT_WR | VPU_RD | EWL_MEM_TYPE_DPB;
            ret = EWLMallocLinear(ewl_inst, transform_size, tb->input_alignment,
                                  &tb->transformMemFactory[coreIdx]);
            if (ret != EWL_OK) {
                tb->transformMemFactory[coreIdx].virtualAddress = NULL;
                return 1;
            }
        }

        tb->pictureDSMemFactory[coreIdx].virtualAddress = NULL;
        if (cmdl->halfDsInput) {
            getAlignedPicSizebyFormat(cmdl->inputFormat, cmdl->lumWidthSrc / 2,
                                      cmdl->lumHeightSrc / 2, alignment, NULL, NULL,
                                      &pictureDSSize);
            tb->pictureDSSize = pictureDSSize;
            tb->pictureDSMemFactory[coreIdx].mem_type = EXT_WR | VPU_RD | EWL_MEM_TYPE_DPB;
            ret = EWLMallocLinear(ewl_inst, pictureDSSize, alignment,
                                  &tb->pictureDSMemFactory[coreIdx]);
            if (ret != EWL_OK) {
                tb->pictureDSMemFactory[coreIdx].virtualAddress = NULL;
                return 1;
            }
        }

        GetDec400CompTablebyFormat(ewl_inst, cmdl->inputFormat, cmdl->lumWidthSrc,
                                   cmdl->lumHeightSrc, alignment, &tb->dec400LumaTableSize,
                                   &tb->dec400ChrTableSize, &tb->dec400FrameTableSize);

        if ((tb->dec400LumaTableSize + tb->dec400ChrTableSize) != 0) {
            tb->Dec400CmpTableMemFactory[coreIdx].mem_type =
                EXT_WR | VPU_RD | EWL_MEM_TYPE_VPU_WORKING;
            ret = EWLMallocLinear(ewl_inst, (tb->dec400LumaTableSize + tb->dec400ChrTableSize), 16,
                                  &tb->Dec400CmpTableMemFactory[coreIdx]);
            if (ret != EWL_OK) {
                tb->Dec400CmpTableMemFactory[coreIdx].virtualAddress = NULL;
                return 1;
            }
        }
    }

    if ((cmdl->videoStab != DEFAULT) && (cmdl->videoStab)) {
        tb->pictureStabMem.mem_type = EXT_WR | VPU_RD | EWL_MEM_TYPE_DPB;
        ret = EWLMallocLinear(ewl_inst, pictureSize, alignment, &tb->pictureStabMem);
        if (ret != EWL_OK) {
            printf("ERROR: Cannot allocate enough memory for stabilization!\n");
            tb->pictureStabMem.virtualAddress = NULL;
            return 1;
        }
    }

    /* Limited amount of memory on some test environment */
    if (cmdl->outBufSizeMax == 0)
        cmdl->outBufSizeMax = 12;
    streamBufRefSize = 4 * tb->pictureMemFactory[0].size;
    streamBufRefSize =
        CLIP3(VCENC_STREAM_MIN_BUF0_SIZE, (u32)cmdl->outBufSizeMax * 1024 * 1024, streamBufRefSize);
    streamBufRefSize =
        (cmdl->streamMultiSegmentMode != 0 ? tb->pictureMemFactory[0].size / 16 : streamBufRefSize);
    outbufSize[0] = streamBufRefSize;
    if (tb->streamBufNum > 1) {
        /* set small stream buffer0 to test two stream buffers */
        outbufSize[0] = tb->width * tb->height * 3 / 2 * 2;
        outbufSize[1] = (streamBufRefSize > outbufSize[0]) ? (streamBufRefSize - outbufSize[0])
                                                           : (outbufSize[0] - streamBufRefSize);
        outbufSize[1] /= (tb->streamBufNum - 1);
    }

    /* output buffer */
    coreNum = cmdl->num_tile_columns == 1 ? tb->outbuf_cnt : 1;
    for (coreIdx = 0; coreIdx < coreNum; coreIdx++) //multi-core based on frame
    {
        for (tileId = 0; tileId < cmdl->num_tile_columns; tileId++) {
            for (iBuf = 0; iBuf < tb->streamBufNum; iBuf++) {
                i32 size =
                    (cmdl->num_tile_columns > 1)
                        ? (streamBufRefSize * cmdl->tile_width[tileId] / cmdl->lumWidthSrc * 64)
                        : outbufSize[iBuf ? 1 : 0];

                /* output buffer use the same alignment setting as input by default */
                i32 outAlignment = tb->input_alignment;
                /* tile stream addr and size control */
                if (cmdl->exp_of_tile_stream_alignment) {
                    outAlignment = 1 << cmdl->exp_of_tile_stream_alignment;
                    size = ((size + outAlignment - 1) >> cmdl->exp_of_tile_stream_alignment)
                           << cmdl->exp_of_tile_stream_alignment;
                }
                tb->outbufMemFactory[coreIdx][tileId][iBuf].mem_type =
                    VPU_WR | CPU_WR | CPU_RD | EWL_MEM_TYPE_SLICE;
                ret = EWLMallocLinear(ewl_inst, size, outAlignment,
                                      &tb->outbufMemFactory[coreIdx][tileId][iBuf]);
                if (ret != EWL_OK) {
                    tb->outbufMemFactory[coreIdx][tileId][iBuf].virtualAddress = NULL;
                    return 1;
                }
            }
        }
    }

    // allocate first delta qp map memory when need.
    // 4 bits per block.
    block_size = ((cmdl->width + cmdl->max_cu_size - 1) & (~(cmdl->max_cu_size - 1))) *
                 ((cmdl->height + cmdl->max_cu_size - 1) & (~(cmdl->max_cu_size - 1))) /
                 (8 * 8 * 2);
    // 8 bits per block if ipcm map/absolute roi qp is supported
    if (tb->hwCfg.roiMapVersion >= 1)
        block_size *= 2;
    block_size = ((block_size + 63) & (~63));

    /* 1, roiMapDeltaQpFile is a text fil cmdl->roiMapDeltaQpBinFile)
  *  2, roiMapInfoBinFile is a binary file, and it's used in 1 pass too;
  *     the difference between roiMapDeltaQpFile and roiMapInfoBinFile is the data's format in the file, one is text data and the other is binary data;
  *     when tb->hwCfg.roiMapVersion < 3, using roiMapDeltaQpFile as input file;
  *     when tb->hwCfg.roiMapVersion == 3, and RoiQpDeltaVer < 3, both roiMapDeltaQpFile and roiMapInfoBinFile can be used;
  *     when tb->hwCfg.roiMapVersion == 3 or 4, and RoiQpDeltaVer >= 3, only roiMapInfoBinFile can be used;
  *  3, roiMapDeltaQpBinFile is a binary file, and it it used in 2 pass, user can use it to input roi data into IM , the roi data merged with pass1 and  used in pass2;
  */
    true_e QpMapValidWhen1pass =
        cmdl->roiMapDeltaQpEnable && (0 == cmdl->lookaheadDepth) &&
        ((cmdl->roiMapDeltaQpFile != NULL) ||
         ((tb->hwCfg.roiMapVersion >= 3) &&
          ((cmdl->roiMapInfoBinFile != NULL) || (cmdl->roiMapConfigFile != NULL))));
    true_e QpMapValidWhen2pass = cmdl->roiMapDeltaQpEnable && (0 != cmdl->lookaheadDepth) &&
                                 (cmdl->roiMapDeltaQpBinFile != NULL);
    true_e ipcmMapInFirstFlag = cmdl->ipcmMapEnable && (cmdl->ipcmMapFile != NULL);
    true_e skipMapInFirstFlag = cmdl->skipMapEnable && (cmdl->skipMapFile != NULL);
    true_e roiMapInFirstValid =
        QpMapValidWhen1pass || QpMapValidWhen2pass || ipcmMapInFirstFlag || skipMapInFirstFlag;
    if (roiMapInFirstValid) {
        tb->roiMapDeltaQpMemFactory[0].mem_type = EXT_WR | VPU_RD | EWL_MEM_TYPE_VPU_WORKING;
        roiMapDeltaQpMemSize = block_size * tb->inputInfoBuf_cnt + ROIMAP_PREFETCH_EXT_SIZE;
        if (EWLMallocLinear(ewl_inst, roiMapDeltaQpMemSize, 0, &tb->roiMapDeltaQpMemFactory[0]) !=
            EWL_OK) {
            tb->roiMapDeltaQpMemFactory[0].virtualAddress = NULL;
            return 1;
        }

        i32 total_size = tb->roiMapDeltaQpMemFactory[0].size;
        for (coreIdx = 0; coreIdx < tb->inputInfoBuf_cnt; coreIdx++) {
            tb->roiMapDeltaQpMemFactory[coreIdx].virtualAddress =
                (u32 *)((ptr_t)tb->roiMapDeltaQpMemFactory[0].virtualAddress +
                        coreIdx * block_size);
            tb->roiMapDeltaQpMemFactory[coreIdx].busAddress =
                tb->roiMapDeltaQpMemFactory[0].busAddress + coreIdx * block_size;
            tb->roiMapDeltaQpMemFactory[coreIdx].size =
                (coreIdx < tb->buffer_cnt - 1 ? block_size
                                              : total_size - (tb->buffer_cnt - 1) * block_size);
            memset(tb->roiMapDeltaQpMemFactory[coreIdx].virtualAddress, 0, block_size);
        }
    }

    // allocate the second roi map memory when need.
    if (tb->hwCfg.roiMapVersion == 3 || tb->hwCfg.roiMapVersion == 4) {
        /* 1, RoimapCuCtrlInfoBinFile is a binary file, and it's used in 1 pass;
    *  2, RoimapCuCtrlIndexBinFile is a binary file, and no using currently;
    *  these two files are used only when tb->hwCfg.roiMapVersion == 3 and the encoder supports two roi map inputing at the same time;
    */
        for (coreIdx = 0; coreIdx < tb->inputInfoBuf_cnt; coreIdx++) {
            if (cmdl->RoimapCuCtrlInfoBinFile != NULL ||
                ((cmdl->roiMapConfigFile != NULL) && (cmdl->RoiCuCtrlVer != 0))) {
                u8 u8CuInfoSize;
                if (cmdl->RoiCuCtrlVer == 3)
                    u8CuInfoSize = 1;
                else if (cmdl->RoiCuCtrlVer == 4)
                    u8CuInfoSize = 2;
                else if (cmdl->RoiCuCtrlVer == 5)
                    u8CuInfoSize = 6;
                else if (cmdl->RoiCuCtrlVer == 6)
                    u8CuInfoSize = 12;
                else // if((cmdl->RoiCuCtrlVer == 7)
                    u8CuInfoSize = 14;

                tb->RoimapCuCtrlInfoMemFactory[coreIdx].mem_type =
                    EXT_WR | VPU_RD | EWL_MEM_TYPE_VPU_WORKING;
                if (EWLMallocLinear(ewl_inst, (block_size * u8CuInfoSize), 0,
                                    &tb->RoimapCuCtrlInfoMemFactory[coreIdx]) != EWL_OK) {
                    tb->RoimapCuCtrlInfoMemFactory[coreIdx].virtualAddress = NULL;
                    return 1;
                }
                memset(tb->RoimapCuCtrlInfoMemFactory[coreIdx].virtualAddress, 0,
                       (block_size * u8CuInfoSize));
            }

            if (cmdl->RoimapCuCtrlIndexBinFile != NULL) {
                block_size = 1 << (IS_H264(cmdl->codecFormat) ? 4 : 6);
                block_size = ((cmdl->width + block_size - 1) & (~(block_size - 1))) *
                             ((cmdl->height + block_size - 1) & (~(block_size - 1))) /
                             (block_size * block_size);
                tb->RoimapCuCtrlIndexMemFactory[coreIdx].mem_type =
                    EXT_WR | VPU_RD | EWL_MEM_TYPE_VPU_WORKING;
                if (EWLMallocLinear(ewl_inst, block_size, 0,
                                    &tb->RoimapCuCtrlIndexMemFactory[coreIdx]) != EWL_OK) {
                    tb->RoimapCuCtrlIndexMemFactory[coreIdx].virtualAddress = NULL;
                    return 1;
                }
                memset(tb->RoimapCuCtrlIndexMemFactory[coreIdx].virtualAddress, 0, block_size);
            }
        }
    }

    if (cmdl->bitDepthLuma != DEFAULT)
        bitDepthLuma = cmdl->bitDepthLuma;

    if (cmdl->bitDepthChroma != DEFAULT)
        bitDepthChroma = cmdl->bitDepthChroma;

    if (IS_HEVC(cmdl->codecFormat) && bitDepthLuma == 8) //hevc main8
        extSramWidthAlignment = 8;
    else if (IS_HEVC(cmdl->codecFormat) && bitDepthLuma == 10) //hevc main10
        extSramWidthAlignment = 16;
    else if (IS_H264(cmdl->codecFormat)) //h264
        extSramWidthAlignment = 16;

    u32 stride = STRIDE((cmdl->rotation && cmdl->rotation != 3 ? cmdl->height : cmdl->width),
                        extSramWidthAlignment);
    tb->extSramLumBwdSize =
        cmdl->extSramLumHeightBwd * 4 * stride * 10 / (bitDepthLuma == 10 ? 8 : 10);
    tb->extSramLumFwdSize =
        cmdl->extSramLumHeightFwd * 4 * stride * 10 / (bitDepthLuma == 10 ? 8 : 10);
    tb->extSramChrBwdSize =
        cmdl->extSramChrHeightBwd * 4 * stride * 10 / (bitDepthChroma == 10 ? 8 : 10);
    tb->extSramChrFwdSize =
        cmdl->extSramChrHeightFwd * 4 * stride * 10 / (bitDepthChroma == 10 ? 8 : 10);

    if (tb->hwCfg.meExternSramSupport)
        extSramTotal = tb->extSramLumBwdSize + tb->extSramLumFwdSize + tb->extSramChrBwdSize +
                       tb->extSramChrFwdSize;

    for (coreIdx = 0; coreIdx < tb->parallelCoreNum; coreIdx++) {
        if (extSramTotal != 0) {
            tb->extSRAMMemFactory[coreIdx].mem_type = VPU_WR | VPU_RD | EWL_MEM_TYPE_VPU_WORKING;
            ret = EWLMallocLinear(ewl_inst, extSramTotal, 16, &tb->extSRAMMemFactory[coreIdx]);
            if (ret != EWL_OK) {
                tb->extSRAMMemFactory[coreIdx].virtualAddress = NULL;
                return 1;
            }
        }
    }
    /*Overlay input buffer*/
    for (coreIdx = 0; coreIdx < tb->inputInfoBuf_cnt; coreIdx++) {
        int i = 0;
        for (i = 0; i < MAX_OVERLAY_NUM; i++) {
            if ((cmdl->overlayEnables >> i) & 1) {
                u32 dec400TableSize = 0;
                switch (cmdl->olFormat[i]) {
                case 0: //ARGB
                    if (cmdl->olSuperTile[i] == 0)
                        block_size = cmdl->olYStride[i] * cmdl->olHeight[i];
                    else
                        block_size = cmdl->olYStride[i] * ((cmdl->olHeight[i] + 63) / 64);
                    break;
                case 1: //NV12
                    block_size = cmdl->olYStride[i] * cmdl->olHeight[i] +
                                 cmdl->olUVStride[i] * cmdl->olHeight[i] / 2;
                    break;
                case 2: //Bitmap
                    block_size = cmdl->olYStride[i] * cmdl->olHeight[i];
                    break;
                default: //3
                    block_size = 0;
                }

                tb->overlayInputMemFactory[coreIdx][i].mem_type =
                    EXT_WR | VPU_RD | EWL_MEM_TYPE_VPU_WORKING;
                if (EWLMallocLinear(ewl_inst, block_size, tb->input_alignment,
                                    &tb->overlayInputMemFactory[coreIdx][i]) != EWL_OK) {
                    tb->overlayInputMemFactory[coreIdx][i].virtualAddress = NULL;
                    return 1;
                }
                memset(tb->overlayInputMemFactory[coreIdx][i].virtualAddress, 0, block_size);

                /* Dec400 buffer */
                if (cmdl->osdDec400CompTableInput && i == 0)
                    GetOsdDec400Size(ewl_inst, cmdl, i, &dec400TableSize);

                if (dec400TableSize) {
                    if (EWLMallocLinear(ewl_inst, dec400TableSize, tb->input_alignment,
                                        &tb->osdDec400CmpTableMemFactory[coreIdx]) != EWL_OK) {
                        tb->osdDec400CmpTableMemFactory[coreIdx].virtualAddress = NULL;
                        return 1;
                    }
                    memset(tb->osdDec400CmpTableMemFactory[coreIdx].virtualAddress, 0,
                           dec400TableSize);
                    tb->osdDec400TableSize = dec400TableSize;
                } else if (i == 0)
                    tb->osdDec400CmpTableMemFactory[coreIdx].virtualAddress = NULL;
            } else {
                tb->overlayInputMemFactory[coreIdx][i].virtualAddress = NULL;
            }
        }
    }

    /* User provided ME1N 21 MVs */
    if (cmdl->replaceMvFile) {
        /* 32x32 aligned, 21 MV for each 32x32, 2byte *(hor + ver) * (L0 + L1) for each MV */
        block_size = 2 * 2 * 2 * 21;
        block_size *= (STRIDE(cmdl->lumWidthSrc, 32) >> 5) * (STRIDE(cmdl->lumHeightSrc, 32) >> 5);
        tb->mvReplaceBuffer.mem_type = EXT_WR | VPU_RD | EWL_MEM_TYPE_VPU_WORKING;
        if (EWLMallocLinear(ewl_inst, block_size, tb->input_alignment, &tb->mvReplaceBuffer) !=
            EWL_OK) {
            tb->mvReplaceBuffer.virtualAddress = NULL;
            return 1;
        }
        memset(tb->mvReplaceBuffer.virtualAddress, 0, block_size);
    } else
        tb->mvReplaceBuffer.virtualAddress = NULL;

    for (coreIdx = 0; coreIdx < tb->buffer_cnt; coreIdx++) {
        printf("Input buffer[%d] size:          %d bytes\n", coreIdx,
               tb->pictureMemFactory[coreIdx].size);
        printf("Input buffer[%d] bus address:   %p\n", coreIdx,
               (void *)tb->pictureMemFactory[coreIdx].busAddress);
        printf("Input buffer[%d] user address:  %p\n", coreIdx,
               tb->pictureMemFactory[coreIdx].virtualAddress);
    }

    for (coreIdx = 0; coreIdx < coreNum; coreIdx++) {
        for (tileId = 0; tileId < tb->tileColumnNum; tileId++) {
            for (iBuf = 0; iBuf < tb->streamBufNum; iBuf++) {
                printf("Output buffer[%d] size:         %d bytes\n", MAX(coreIdx, tileId),
                       tb->outbufMemFactory[coreIdx][tileId][iBuf].size);
                printf("Output buffer[%d] bus address:  %p\n", MAX(coreIdx, tileId),
                       (void *)tb->outbufMemFactory[coreIdx][tileId][iBuf].busAddress);
                printf("Output buffer[%d] user address: %p\n", MAX(coreIdx, tileId),
                       tb->outbufMemFactory[coreIdx][tileId][iBuf].virtualAddress);
            }
        }
    }
    return 0;
}

/*------------------------------------------------------------------------------

    FreeRes

    Release all resources allcoated byt AllocRes()

------------------------------------------------------------------------------*/
void FreeRes(struct test_bench *tb)
{
    u32 coreIdx = 0;
    i32 iBuf;
    const void *ewl_inst = tb->ewlInst;

    EWLFreeLinear(ewl_inst, &tb->pictureStabMem);

    EWLFreeLinear(ewl_inst, &tb->roiMapDeltaQpMemFactory[0]);

    for (coreIdx = 0; coreIdx < tb->inputInfoBuf_cnt; coreIdx++) {
        EWLFreeLinear(ewl_inst, &tb->RoimapCuCtrlInfoMemFactory[coreIdx]);
        EWLFreeLinear(ewl_inst, &tb->RoimapCuCtrlIndexMemFactory[coreIdx]);
    }

    for (coreIdx = 0; coreIdx < tb->buffer_cnt; coreIdx++) {
        EWLFreeLinear(ewl_inst, &tb->Dec400CmpTableMemFactory[coreIdx]);
        EWLFreeLinear(ewl_inst, &tb->pictureMemFactory[coreIdx]);
        EWLFreeLinear(ewl_inst, &tb->pictureDSMemFactory[coreIdx]);
        EWLFreeLinear(ewl_inst, &tb->transformMemFactory[coreIdx]);
    }

    u32 tileId;
    for (coreIdx = 0; coreIdx < MAX_CORE_NUM; coreIdx++) {
        for (tileId = 0; tileId < HEVC_MAX_TILE_COLS; tileId++) {
            for (iBuf = 0; iBuf < MAX_STRM_BUF_NUM; iBuf++) {
                EWLFreeLinear(ewl_inst, &tb->outbufMemFactory[coreIdx][tileId][iBuf]);
            }
        }
    }

    for (coreIdx = 0; coreIdx < tb->parallelCoreNum; coreIdx++) {
        if (tb->extSRAMMemFactory[coreIdx].size) {
            EWLFreeLinear(ewl_inst, &tb->extSRAMMemFactory[coreIdx]);
        }
    }

    for (coreIdx = 0; coreIdx < tb->inputInfoBuf_cnt; coreIdx++) {
        for (iBuf = 0; iBuf < MAX_OVERLAY_NUM; iBuf++) {
            EWLFreeLinear(ewl_inst, &tb->overlayInputMemFactory[coreIdx][iBuf]);
        }
        EWLFreeLinear(ewl_inst, &tb->osdDec400CmpTableMemFactory[coreIdx]);
    }
    EWLFreeLinear(ewl_inst, &tb->mvReplaceBuffer);

    ReleaseEwlInst(tb);
}

/*------------------------------------------------------------------------------

    CheckArea

------------------------------------------------------------------------------*/
i32 CheckArea(VCEncPictureArea *area, commandLine_s *cml)
{
    i32 w = (cml->width + cml->max_cu_size - 1) / cml->max_cu_size;
    i32 h = (cml->height + cml->max_cu_size - 1) / cml->max_cu_size;

    if ((area->left < (u32)w) && (area->right < (u32)w) && (area->top < (u32)h) &&
        (area->bottom < (u32)h))
        return 1;

    return 0;
}

/*    Callback function called by the encoder SW after "slice ready"
    interrupt from HW. Note that this function is not necessarily called
    after every slice i.e. it is possible that two or more slices are
    completed between callbacks.
------------------------------------------------------------------------------*/
void HEVCSliceReady(VCEncSliceReady *slice)
{
    u32 i;
    u32 streamSize;
    u32 pos;
    SliceCtl_s *ctl = (SliceCtl_s *)slice->pAppData;
    /* Here is possible to implement low-latency streaming by
   * sending the complete slices before the whole frame is completed. */
    if (ctl->multislice_encoding && (ENCH2_SLICE_READY_INTERRUPT)) {
        pos = slice->slicesReadyPrev ? ctl->streamPos : /* Here we store the slice pointer */
                  0;                                    /* Pointer to beginning of frame */
        streamSize = 0;
        for (i = slice->nalUnitInfoNumPrev; i < slice->nalUnitInfoNum; i++) {
            streamSize += *(slice->sliceSizes + i);
        }

        u32 MemSyncOffset = pos + slice->PreDataSize;
        u32 MemSyncDataSize = streamSize - MemSyncOffset;
        u32 MemSyncDataSize0 = 0, MemSyncOffset0 = 0;
        u32 MemSyncDataSize1 = 0, MemSyncOffset1 = 0;
        if (streamSize > slice->streamBufs.bufLen[0]) //streamBufChain
        {
            MemSyncDataSize0 = slice->streamBufs.bufLen[0] - MemSyncOffset;
            MemSyncOffset0 = MemSyncOffset +
                             (slice->streamBufs.buf[0] - (u8 *)slice->outbufMem[0]->virtualAddress);
            EWLSyncMemData(slice->outbufMem[0], MemSyncOffset0, MemSyncDataSize0, DEVICE_TO_HOST);
            MemSyncDataSize1 = MemSyncDataSize - MemSyncDataSize0;
            MemSyncOffset1 = slice->streamBufs.buf[1] - (u8 *)slice->outbufMem[1]->virtualAddress;
            EWLSyncMemData(slice->outbufMem[1], MemSyncOffset1, MemSyncDataSize1, DEVICE_TO_HOST);
        } else {
            MemSyncOffset += (slice->streamBufs.buf[0] - (u8 *)slice->outbufMem[0]->virtualAddress);
            EWLSyncMemData(slice->outbufMem[0], MemSyncOffset, MemSyncDataSize, DEVICE_TO_HOST);
        }
        slice->PreDataSize = 0;

        if (ctl->output_byte_stream == 0) {
            writeNalsBufs(ctl->outStreamFile, &slice->streamBufs,
                          slice->sliceSizes + slice->nalUnitInfoNumPrev,
                          slice->nalUnitInfoNum - slice->nalUnitInfoNumPrev, pos, 0, 0);
        } else {
            writeStrmBufs(ctl->outStreamFile, &slice->streamBufs, pos, streamSize, 0);
        }

        printf("%s: streamSize %d\n", __FUNCTION__, streamSize);
        pos += streamSize;
        /* Store the slice pointer for next callback */
        ctl->streamPos = pos;
    }
}

ReEncodeStrategy ReEncodeCurrentFrame(const VCEncStatisticOut *stat, NewEncodeParams *new_params)
{
    ReEncodeStrategy use_new_params = 0;
    double thres0 = 2.0;
    double thres1 = 0.5;
    double thres2 = 35.0;
    int min_qp = 20;
    int max_qp = 51;
    int re_encode_qp = 40;
    EWLLinearMem_t *cur_output_buffer = (EWLLinearMem_t *)stat->kOutputbufferMem[0];
    EWLLinearMem_t *new_output_buffer = &new_params->output_buffer_mem[0];

    new_params->strategy = 0;
#if 0
  if (( stat->frame_real_bits < thres0 * stat->frame_target_bits) &&(stat->frame_real_bits > thres1 * stat->frame_target_bits)) {
     return -1;
  }

  if ((stat->frame_real_bits < thres0 * stat->frame_target_bits) &&(stat->psnr_y > thres2)) {
    return -1;
  }

  if (stat->frame_real_bits > stat->frame_target_bits * 20) {
    re_encode_qp =  CLIP3(stat->avg_qp_y + 24, min_qp, max_qp);
  } else if (stat->frame_real_bits > stat->frame_target_bits * 16) {
    re_encode_qp = 45;
  } else if (stat->frame_real_bits > stat->frame_target_bits * 12) {
    re_encode_qp = 42
  } else if (stat->frame_real_bits > stat->frame_target_bits * 2) {
    re_encode_qp = 40;
  } else if ( stat->frame_real_bits < stat->frame_target_bits/12) {
    re_encode_qp = 38;
  } else if (stat->frame_real_bits < stat->frame_target_bits/8){
    re_encode_qp = 36;
  } else {
    re_encode_qp = 34;
  }
  new_params->qp = reEncodeQp;
  new_params->strategy = NEW_QP;
#else
    if (stat->output_buffer_over_flow == 1) {
        u32 new_size = cur_output_buffer->size +
                       1024; /* customer can set the size that need to increase by oneself */
        new_params->output_buffer_mem[0].mem_type = VPU_WR | CPU_WR | CPU_RD | EWL_MEM_TYPE_SLICE;
        if (EWLMallocLinear(stat->kEwl, new_size, 0, new_output_buffer) != EWL_OK) {
            use_new_params = 0;
            printf("Faild, try to reallocate a new output buffer...\n");
        } else {
            // reallocate the buffer
            printf("adjusted output buffer when buffer overflow\n");
            printf("Output buffer bus address: from %p to %p\n",
                   (void *)cur_output_buffer->busAddress, (void *)new_output_buffer->busAddress);
            printf("Output buffer user address: from %p to %p\n",
                   (void *)cur_output_buffer->virtualAddress,
                   (void *)new_output_buffer->virtualAddress);
            printf("Output buffer size: from %d to %d bytes\n", cur_output_buffer->size,
                   new_output_buffer->size);
            /* copy the previous vps, sps, pps to new output buffer */
            if (stat->header_stream_byte)
                EWLmemcpy(new_output_buffer->virtualAddress, cur_output_buffer->virtualAddress,
                          stat->header_stream_byte);
            /* a new output buffer is used, maybe need to release the cur_output_buffer */
            EWLFreeLinear(stat->kEwl, cur_output_buffer);
            *cur_output_buffer = *new_output_buffer;
            new_params->strategy = NEW_OUTPUT_BUFFER;
        }
    }
#endif

    if (new_params->try_counter >= 10) /* don't re-encode after 10 times */
        use_new_params = 0;
    else
        use_new_params = new_params->strategy;
    new_params->try_counter++;

    return use_new_params;
}

/*------------------------------------------------------------------------------

    InitInputLineBuffer
    -get line buffer params for IRQ handle
    -get address of input line buffer
------------------------------------------------------------------------------*/
i32 InitInputLineBuffer(inputLineBufferCfg *lineBufCfg, commandLine_s *cml, VCEncIn *encIn,
                        VCEncInst inst, struct test_bench *tb)
{
    VCEncCodingCtrl codingCfg;
    u32 stride, chroma_stride, client_type;
    VCEncGetAlignedStride(cml->lumWidthSrc, cml->inputFormat, &stride, &chroma_stride,
                          tb->input_alignment);
    VCEncGetCodingCtrl(inst, &codingCfg);
    client_type = IS_H264(cml->codecFormat) ? EWL_CLIENT_TYPE_H264_ENC : EWL_CLIENT_TYPE_HEVC_ENC;
    u32 tileId;

    memset(lineBufCfg, 0, sizeof(inputLineBufferCfg));
    lineBufCfg->depth = codingCfg.inputLineBufDepth;
    lineBufCfg->hwHandShake = codingCfg.inputLineBufHwModeEn;
    lineBufCfg->loopBackEn = codingCfg.inputLineBufLoopBackEn;
    lineBufCfg->amountPerLoopBack = codingCfg.amountPerLoopBack;
    lineBufCfg->initSegNum = 0;
    lineBufCfg->inst = (void *)inst;
    //lineBufCfg->asic   = &(((struct vcenc_instance *)inst)->asic);
    lineBufCfg->wrCnt = 0;
    lineBufCfg->inputFormat = cml->inputFormat;
    lineBufCfg->lumaStride = stride;
    lineBufCfg->chromaStride = chroma_stride;
    lineBufCfg->encWidth = cml->width;
    lineBufCfg->encHeight = cml->height;
    lineBufCfg->srcHeight = cml->lumHeightSrc;
    lineBufCfg->srcVerOffset = cml->verOffsetSrc;
    lineBufCfg->getMbLines = &VCEncGetEncodedMbLines;
    lineBufCfg->setMbLines = &VCEncSetInputMBLines;
    lineBufCfg->ctbSize = (IS_H264(cml->codecFormat) || tb->hwCfg.prpSbiSupport)
                              ? 16
                              : 64; //flexa currently support 16 lines
    lineBufCfg->lumSrc = tb->lum;
    lineBufCfg->cbSrc = tb->cb;
    lineBufCfg->crSrc = tb->cr;
    lineBufCfg->client_type = client_type;

    if (VCEncInitInputLineBuffer(lineBufCfg))
        return -1;

    /* loopback mode */
    if (lineBufCfg->loopBackEn && lineBufCfg->lumBuf.buf) {
        VCEncPreProcessingCfg preProcCfg;
        encIn->busLuma = lineBufCfg->lumBuf.busAddress;
        encIn->busChromaU = lineBufCfg->cbBuf.busAddress;
        encIn->busChromaV = lineBufCfg->crBuf.busAddress;
        for (tileId = 1; tileId < cml->num_tile_columns; tileId++) {
            encIn->tileExtra[tileId - 1].busLuma = lineBufCfg->lumBuf.busAddress;
            encIn->tileExtra[tileId - 1].busChromaU = lineBufCfg->cbBuf.busAddress;
            encIn->tileExtra[tileId - 1].busChromaV = lineBufCfg->crBuf.busAddress;
        }

        /* In loop back mode, data in line buffer start from the line to be encoded*/
        VCEncGetPreProcessing(inst, &preProcCfg);
        for (tileId = 0; tileId < cml->num_tile_columns; tileId++) {
            u32 *yOffset =
                (tileId == 0) ? (&preProcCfg.yOffset) : (&preProcCfg.tileExtra[tileId - 1].yOffset);
            *yOffset = 0;
        }
        VCEncSetPreProcessing(inst, &preProcCfg);
    }

    return 0;
}

/* Callback function called by the encoder SW after "segment ready"
    interrupt from HW. Note that this function is called after every segment is ready.
------------------------------------------------------------------------------*/
void EncStreamSegmentReady(void *cb_data)
{
    u8 *streamBase;
    SegmentCtl_s *ctl = (SegmentCtl_s *)cb_data;

    if (ctl->streamMultiSegEn) {
        streamBase =
            ctl->streamBase + (ctl->streamRDCounter % ctl->segmentAmount) * ctl->segmentSize;

        if (ctl->output_byte_stream == 0 && ctl->startCodeDone == 0) {
            const u8 start_code_prefix[4] = {0x0, 0x0, 0x0, 0x1};
            fwrite(start_code_prefix, 1, 4, ctl->outStreamFile);
            ctl->startCodeDone = 1;
        }
        printf("<----receive segment irq %d\n", ctl->streamRDCounter);
        WriteStrm(ctl->outStreamFile, (u32 *)streamBase, ctl->segmentSize, 0);

        ctl->streamRDCounter++;
    }
}

void Multimode_Release(commandLine_s *cml)
{
    int i;
    for (i = 0; i < cml->nstream; i++) {
        if (cml->streamcml[i] != NULL) {
            if (cml->multimode == 2) {
                if (cml->streamcml[i]->recon) {
                    free(cml->streamcml[i]->recon);
                    cml->streamcml[i]->recon = NULL;
                }
            }
            if (cml->streamcml[i]->argv) {
                if (cml->streamcml[i]->argv[0]) {
                    free(cml->streamcml[i]->argv[0]);
                    cml->streamcml[i]->argv[0] = NULL;
                }
                if (cml->streamcml[i]->argv) {
                    free(cml->streamcml[i]->argv);
                    cml->streamcml[i]->argv = NULL;
                }
            }
            free(cml->streamcml[i]);
            cml->streamcml[i] = NULL;
        }
        if (cml->tid[i] != NULL) {
            free(cml->tid[i]);
            cml->tid[i] = NULL;
        }
    }
}

i32 allocVariableMem(struct test_bench *tb, commandLine_s *cml)
{
    tb->tile_width = (i32 *)calloc(cml->num_tile_columns, sizeof(i32));
    if (tb->tile_width == NULL)
        return 0;
    tb->tile_height = (i32 *)calloc(cml->num_tile_rows, sizeof(i32));
    if (tb->tile_height == NULL)
        return 0;

    if (cml->num_tile_columns > 1) {
        tb->vcencInTileExtra =
            (VCEncInTileExtra *)calloc(cml->num_tile_columns - 1, sizeof(VCEncInTileExtra));
        if (tb->vcencInTileExtra == NULL)
            return 0;
        tb->vcencPrpTileExtra =
            (VCEncPrpTileExtra *)calloc(cml->num_tile_columns - 1, sizeof(VCEncPrpTileExtra));
        if (tb->vcencPrpTileExtra == NULL)
            return 0;
    }

    return 1;
}
void FreeVariableMem(struct test_bench *tb)
{
    free(tb->tile_width);
    free(tb->tile_height);

    if (tb->tileColumnNum > 1) {
        free(tb->vcencInTileExtra);
        free(tb->vcencPrpTileExtra);
    }
}
