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

#ifndef TEST_BENCH_UTILS_H
#define TEST_BENCH_UTILS_H

#include "base_type.h"
FILE *open_file(char *name, char *mode);

#define VC_FCLOSE(fp)                                                                              \
    {                                                                                              \
        if (fp) {                                                                                  \
            fclose(fp);                                                                            \
            fp = NULL;                                                                             \
        }                                                                                          \
    }

#if 0
static void WriteNalSizesToFile(FILE *file, const u32 *pNaluSizeBuf,
                                u32 numNalus);
#endif
/* Test bench utils is to handle files<->memory operation*/

//Encoder output: memory -> file
void WriteStrm(FILE *fout, u32 *strmbuf, u32 size, u32 endian);
void WriteScaled(u32 *strmbuf, struct commandLine_s *cml);

//input YUV format convertion for specific format
FILE *FormatCustomizedYUV(struct test_bench *tb, commandLine_s *cml);
void ChangeCmlCustomizedFormat(commandLine_s *cml);
void ChangeToCustomizedFormat(commandLine_s *cml, VCEncPreProcessingCfg *preProcCfg);
i32 ChangeInputToTransYUV(struct test_bench *tb, VCEncInst encoder, commandLine_s *cml,
                          VCEncIn *pEncIn);

//read&parse input cfg files for different features
char *nextToken(char *str);
i32 readConfigFiles(struct test_bench *tb, commandLine_s *cml);
i32 ParsingSmartConfig(char *fname, commandLine_s *cml);

//input YUV read
i32 read_picture(struct test_bench *tb, u32 inputFormat, u32 src_img_size, u32 src_width,
                 u32 src_height, int bDS);
u64 next_picture(struct test_bench *tb, int picture_cnt);
void getAlignedPicSizebyFormat(VCEncPictureType type, u32 width, u32 height, u32 alignment,
                               u32 *luma_Size, u32 *chroma_Size, u32 *picture_Size);

//GOP pattern file parse
int InitGopConfigs(int gopSize, commandLine_s *cml, VCEncGopConfig *gopCfg, u8 *gopCfgOffset,
                   bool bPass2, u32 hwId);

u32 SetupInputBuffer(struct test_bench *tb, commandLine_s *cml, VCEncIn *pEncIn);

void SetupOutputBuffer(struct test_bench *tb, VCEncIn *pEncIn);

/* multi-core buffet selection */
i32 FindOutputBufferIdByBusAddr(EWLLinearMem_t memFactory[][HEVC_MAX_TILE_COLS][MAX_STRM_BUF_NUM],
                                const u32 MemNum, const ptr_t busAddr);
i32 FindInputPicBufIdByBusAddr(struct test_bench *tb, const ptr_t busAddr, i8 bTrans);
i32 ReturnBufferById(u32 *memsRefFlag, const u32 MemNum, i32 bufferIndex);
i32 ReturnIOBuffer(struct test_bench *tb, commandLine_s *cml, VCEncConsumedAddr *consumedAddrs,
                   i32 picture_cnt);
i32 GetFreeOutputBuffer(struct test_bench *tb);
i32 GetFreeInputPicBuffer(struct test_bench *tb);
i32 GetFreeInputInfoBuffer(struct test_bench *tb);
void writeStrmBufs(FILE *fout, VCEncStrmBufs *bufs, u32 offset, u32 size, u32 endian);
void writeNalsBufs(FILE *fout, VCEncStrmBufs *bufs, const u32 *pNaluSizeBuf, u32 numNalus,
                   u32 offset, u32 hdrSize, u32 endian);
void getStreamBufs(VCEncStrmBufs *bufs, struct test_bench *tb, bool encoding, u32 index);
void readStrmBufs(unsigned char *data, VCEncStrmBufs *bufs, u32 offset, u32 size, u32 endian);

/* timer help*/
unsigned int uTimeDiff(struct timeval end, struct timeval start);
u64 uTimeDiffLL(struct timeval end, struct timeval start);

/*DEC400 Config*/
void GetDec400CompTablebyFormat(const void *wl_inst, VCEncPictureType type, u32 width, u32 height,
                                u32 alignment, u32 *luma_Size, u32 *chroma_Size, u32 *picture_Size);

void InitPicConfig(VCEncIn *pEncIn, commandLine_s *cml);

/*get tb's ewl instance to malloc/free buffer in test_bench*/
const void *InitEwlInst(const commandLine_s *cml, struct test_bench *tb);
/*after all buffers allocated in test_bench had been freed, release tb's ewl instance*/
void ReleaseEwlInst(struct test_bench *tb);

#endif
