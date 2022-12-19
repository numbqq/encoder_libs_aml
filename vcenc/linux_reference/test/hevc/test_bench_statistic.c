#include "base_type.h"
#ifdef TEST_DATA
#include "enctrace.h"
#endif
#include "hevcencapi.h"
#include "HevcTestBench.h"
#include "test_bench_statistic.h"
#include "rate_control_picture.h"
#include "instance.h"
/*------------------------------------------------------------------------------
    Add new frame bits for moving average bitrate calculation
------------------------------------------------------------------------------*/
void MaAddFrame(ma_s *ma, i32 frameSizeBits)
{
    ma->frame[ma->pos++] = frameSizeBits;

    if (ma->pos == ma->length)
        ma->pos = 0;

    if (ma->count < ma->length)
        ma->count++;
}

/*------------------------------------------------------------------------------
    Calculate average bitrate of moving window
------------------------------------------------------------------------------*/
i32 Ma(ma_s *ma)
{
    i32 i;
    unsigned long long sum = 0; /* Using 64-bits to avoid overflow */

    for (i = 0; i < ma->count; i++)
        sum += ma->frame[i];

    if (!ma->frameRateDenom)
        return 0;

    sum = sum / ma->count;

    return sum * (ma->frameRateNumer + ma->frameRateDenom - 1) / ma->frameRateDenom;
}

/* store the bits consumed by each GOP */
void StatisticFrameBits(VCEncInst inst, struct test_bench *tb, ma_s *ma, u32 frameBits,
                        u64 totalBits)
{
    //#ifdef INTERNAL_TEST
    struct vcenc_instance *vcenc_instance = (struct vcenc_instance *)inst;
    vcencRateControl_s *rc = &vcenc_instance->rateControl;
    asicData_s *asic = &vcenc_instance->asic;
    i32 type =
        asic->regs
            .frameCodingType; // ((type == 1) ? "I-Slice" : ((type == 0)?"P-Slice" : "B-Slice"))
    u32 newGop = type == 0 || rc->predId == 1; //predId == P_SLICE;
    i32 movingBitRate = -1;

    if (!rc->picRc) {
        printf("[RC] Warning: No RC Log when RC is disabled \n");
        return;
    }

    /* initialize */
    if (tb->picture_encoded_cnt == 0) {
        memset(&(tb->bitStat), 0, sizeof(tb->bitStat));

        ma->pos = ma->count = 0;
        ma->frameRateNumer = rc->outRateNum;
        ma->frameRateDenom = rc->outRateDenom;
        ma->length = rc->monitorFrames;
    }

    /* add frame bits into the list */
    MaAddFrame(ma, frameBits);

    if (ma->count >= ma->length) {
        movingBitRate = Ma(ma);
        float ratePercent = movingBitRate * 100.0f / rc->virtualBuffer.bitRate;

        tb->bitStat.nStatFrame++;
        tb->bitStat.maxMovingBitRate = MAX(tb->bitStat.maxMovingBitRate, ratePercent);
        /* assume that real bitRate can not be zero */
        tb->bitStat.minMovingBitRate = tb->bitStat.minMovingBitRate == 0
                                           ? ratePercent
                                           : MIN(tb->bitStat.minMovingBitRate, ratePercent);
        tb->bitStat.sumDeviation += ABS(ratePercent - 100);
    }

    /* do not count I frame bits */
    if (type == 1) //I-Slice
        return;

    if (newGop) {
        /* normalize gopBits of last GOP */
        if (tb->bitStat.nGopFrame) {
            tb->bitStat.gopBits[tb->bitStat.gopPos] /= tb->bitStat.nGopFrame;
            tb->bitStat.nGopFrame = 0;
        }

        if (tb->bitStat.nGop)
            tb->bitStat.gopPos = (tb->bitStat.gopPos + 1) % MAX_BITS_STAT;

        if (tb->bitStat.nGop < MAX_BITS_STAT)
            tb->bitStat.nGop++;
    }

    tb->bitStat.gopBits[tb->bitStat.gopPos] += frameBits;
    tb->bitStat.nGopFrame++;

    u32 targetBitrate = rc->virtualBuffer.bitRate;
    u32 actualBitrate =
        totalBits * rc->outRateNum / ((tb->picture_encoded_cnt + 1) * rc->outRateDenom);
    float bpsDeviation = (float)actualBitrate / targetBitrate - 1;

    printf("[RC] Pic %i ", tb->picture_encoded_cnt);
    if (movingBitRate >= 0)
        printf("movingBitrate: %d ", movingBitRate);

    printf("targetBits=%d actualBits=%d deviation=%.2f%% | targetBitrate=%d "
           "actualBitrate=%d deviation=%.2f%%\n",
           rc->targetPicSize, frameBits, (frameBits * 100.0f) / rc->targetPicSize - 100,
           rc->virtualBuffer.bitRate, actualBitrate, bpsDeviation * 100);
    //#else
    // printf("Do nothing because INTERNAL_TEST=n\n");
    //#endif
}

/* calculate bits deviation from the stored GOP bits */
float GetBitsDeviation(struct test_bench *tb)
{
    float theta = 0.0;
    if (tb->bitStat.nGop) {
        i32 i;
        /* calculate gop bits standard deviation */
        u32 gopAvgBits = 0, gopSquareDiff = 0;
        for (i = 0; i < tb->bitStat.nGop; i++)
            gopAvgBits += tb->bitStat.gopBits[i];

        if (gopAvgBits) {
            gopAvgBits /= tb->bitStat.nGop;

            for (i = 0; i < tb->bitStat.nGop; i++) {
                i32 diff = tb->bitStat.gopBits[i] - gopAvgBits;
                gopSquareDiff += diff * diff;
            }
            theta = sqrt((float)gopSquareDiff / tb->bitStat.nGop);
            theta = theta * 100 / gopAvgBits;
        }
    }
    return theta;
}

/*------------------------------------------------------------------------------
    Function name   : EncTraceProfile
    Description     : print the distortion & bits data per frame, only valid for c-model.
    Return type     : void
------------------------------------------------------------------------------*/
void EncTraceProfile(VCEncInst inst, struct test_bench *tb, VCEncOut *pEncOut, u64 bits, u8 force)
{
    struct vcenc_instance *vcenc_instance = (struct vcenc_instance *)inst;
    asicData_s *asic = &vcenc_instance->asic;
    struct container *c;
    struct sw_picture *pic;
    i32 poc = vcenc_instance->poc;

    /* two ways to output profile.log considering TRACE=y/n:
     --rdLog=1 or tb.cfg:profile.log, since tb.cfg can't work with TRACE=n
  */
    FILE *fp = NULL;
#ifdef TEST_DATA
    extern struct enc_sw_trace ctrl_sw_trace;
    fp = ctrl_sw_trace.prof_fp;
#endif

    if (!fp && force) {
        if (!tb->rdLogFile) {
            tb->rdLogFile = fopen("profile.log", "wb");
        }
        fp = tb->rdLogFile;
    }
    if (!fp)
        return;

    c = get_container(vcenc_instance);
    pic = get_picture(c, poc);
    if (!pic)
        return;

    i32 type = asic->regs.frameCodingType;

    // MUST be the same as the struct in "pictur".
    struct {
        i32 bitnum;
        float psnr_y, psnr_u, psnr_v;
        double ssim;
        double ssim_y, ssim_u, ssim_v;
    } prof;

    if (vcenc_instance->rateControl.frameCoded == ENCHW_NO)
        return;

    if (vcenc_instance->num_tile_columns > 1) {
        prof.psnr_y = pEncOut->psnr[0];
        prof.psnr_u = pEncOut->psnr[1];
        prof.psnr_v = pEncOut->psnr[2];
        prof.ssim = (asic->regs.codedChromaIdc == 0)
                        ? pEncOut->ssim[0]
                        : (pEncOut->ssim[0] * 0.8 + 0.1 * (pEncOut->ssim[1] + pEncOut->ssim[2]));
        prof.ssim_y = pEncOut->ssim[0];
        prof.ssim_u = pEncOut->ssim[1];
        prof.ssim_v = pEncOut->ssim[2];
    } else {
        EWLLinearMem_t *compress_coeff_SCAN =
            &asic->compress_coeff_SCAN[(vcenc_instance->jobCnt + 1) %
                                       vcenc_instance->parallelCoreNum];
        EWLSyncMemData(compress_coeff_SCAN, 0, sizeof(prof), DEVICE_TO_HOST);

        void *prof_data = (u8 *)(asic->compress_coeff_SCAN[(vcenc_instance->jobCnt + 1) %
                                                           vcenc_instance->parallelCoreNum]
                                     .virtualAddress);

        if (!prof_data)
            return;
        memcpy(&prof, prof_data, sizeof(prof));
    }

    fprintf(fp,
            "POC %d QP %2.2f  %ju bits [Y %.4f dB  U %.4f dB  V %.4f dB] [SSIM "
            "%.4f]  [SSIM Y %.4f  U %.4f  V %.4f] [%s]\n",
            poc, asic->regs.sumOfQP * 1.0 / asic->regs.sumOfQPNumber, bits, prof.psnr_y,
            prof.psnr_u, prof.psnr_v, prof.ssim, prof.ssim_y, prof.ssim_u, prof.ssim_v,
            ((type == 1) ? "I-Slice" : ((type == 0) ? "P-Slice" : "B-Slice")));
}
