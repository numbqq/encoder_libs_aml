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
void WriteNals(FILE *fout, u32 *strmbuf, const u32 *pNaluSizeBuf, u32 numNalus, u32 hdrSize,
               u32 endian);
void WriteStrm(FILE *fout, u32 *strmbuf, u32 size, u32 endian);
void WriteScaled(u32 *strmbuf, struct commandLine_s *cml);

//roi-map feature: input cfg file -> memory
//void writeFlags2Memory(char ipcm_flag, u8* memory,u16 column,u16 row,u16 blockunit,u16 width, u16 ctb_size, u32 ctb_per_row, u32 ctb_per_column);
//void writeFlagsRowData2Memory(char* ipcmRowStartAddr,u8* memory,u16 width,u16 rowNumber,u16 blockunit, u16 ctb_size, u32 ctb_per_row, u32 ctb_per_column);
//void writeQpValue2Memory(char qpDelta, u8* memory,u16 column,u16 row,u16 blockunit,u16 width, u16 ctb_size, u32 ctb_per_row, u32 ctb_per_column);
//void writeQpDeltaData2Memory(char qpDelta,u8* memory,u16 column,u16 row,u16 blockunit,u16 width, u16 ctb_size, u32 ctb_per_row, u32 ctb_per_column);
//void writeQpDeltaRowData2Memory(char* qpDeltaRowStartAddr,u8* memory,u16 width,u16 rowNumber,u16 blockunit, u16 ctb_size, u32 ctb_per_row, u32 ctb_per_column, i32 roiMapVersion);
//i32 copyQPDelta2Memory (commandLine_s *cml, VCEncInst enc, struct test_bench *tb);
//i32 copyCuTreeQPDelta2Memory (commandLine_s *cml, VCEncInst enc, struct test_bench *tb, int cur_poc);
//int copyFlagsMap2Memory(commandLine_s *cml, VCEncInst enc, struct test_bench *tb);
void SetupROIMapBuffer(struct test_bench *tb, commandLine_s *cml, VCEncIn *pEncIn,
                       VCEncInst encoder);

/* Setup overlay input buffer and read 1 frame data into buffer*/
void SetupOverlayBuffer(struct test_bench *tb, commandLine_s *cml, VCEncIn *pEncIn);
/* Get osd DEV400 input size */
void GetOsdDec400Size(const void *wl_instance, commandLine_s *cmdl, u8 idx, u32 *dec400TableSize);

/* Setup user provided ME1N mv buffer */
void SetupMvReplaceBuffer(struct test_bench *tb, commandLine_s *cml, VCEncIn *pEncIn);

//CUInfor dump: memory -> files
void WriteCuInformation(struct test_bench *tb, VCEncInst encoder, VCEncOut *pEncOut,
                        commandLine_s *cml, i32 frame, i32 poc);

//input YUV format convertion for specific format
float getPixelWidthInByte(VCEncPictureType type);
FILE *FormatCustomizedYUV(struct test_bench *tb, commandLine_s *cml);
void ChangeCmlCustomizedFormat(commandLine_s *cml);
void ChangeToCustomizedFormat(commandLine_s *cml, VCEncPreProcessingCfg *preProcCfg);
i32 ChangeInputToTransYUV(struct test_bench *tb, VCEncInst encoder, commandLine_s *cml,
                          VCEncIn *pEncIn);

//read&parse input cfg files for different features
char *nextToken(char *str);
i32 readConfigFiles(struct test_bench *tb, commandLine_s *cml);
i32 readGmv(struct test_bench *tb, VCEncIn *pEncIn, commandLine_s *cml);
i32 ParsingSmartConfig(char *fname, commandLine_s *cml);

//input YUV read
i32 read_picture(struct test_bench *tb, u32 inputFormat, u32 src_img_size, u32 src_width,
                 u32 src_height, int bDS);
i32 read_stab_picture(struct test_bench *tb, u32 inputFormat, u32 src_img_size, u32 src_width,
                      u32 src_height);
u64 next_picture(struct test_bench *tb, int picture_cnt);
void getAlignedPicSizebyFormat(VCEncPictureType type, u32 width, u32 height, u32 alignment,
                               u32 *luma_Size, u32 *chroma_Size, u32 *picture_Size);
u32 GetResolution(char *filename, i32 *pWidth, i32 *pHeight);

//GOP pattern file parse
int InitGopConfigs(int gopSize, commandLine_s *cml, VCEncGopConfig *gopCfg, u8 *gopCfgOffset,
                   bool bPass2, u32 hwId);

//SEI information from cfg file
u8 *ReadUserData(VCEncInst encoder, char *name);

//Bitrate statics
void MaAddFrame(ma_s *ma, i32 frameSizeBits);
i32 Ma(ma_s *ma);

u32 SetupInputBuffer(struct test_bench *tb, commandLine_s *cml, VCEncIn *pEncIn);

void SetupOutputBuffer(struct test_bench *tb, VCEncIn *pEncIn);
void SetupMultiTileOutputBuffer(struct test_bench *tb, VCEncIn *pEncIn);

void SetupExtSRAM(struct test_bench *tb, VCEncIn *pEncIn);

//flexible reference
i32 OpenFlexRefsFile(struct test_bench *tb, commandLine_s *cml);
u32 InitReferenceControl(struct test_bench *tb, commandLine_s *cml);
int GetNextReferenceControl(FILE *fp, VCEncGopPicConfig *rps, ltr_struct *ltr_t);
int FillInputOptionsForFlexRps(VCEncIn *in, VCEncGopPicConfig *rps);

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
i32 ReturnMultiTileOutputBuf(struct test_bench *tb);
i32 ReuseInputBuffer(struct test_bench *tb, ptr_t busAddr);
void InitOutputBuffer(struct test_bench *tb, VCEncIn *pEncIn);
void InitSliceCtl(struct test_bench *tb, struct commandLine_s *cml);
void InitStreamSegmentCrl(struct test_bench *tb, struct commandLine_s *cml);
void writeStrmBufs(FILE *fout, VCEncStrmBufs *bufs, u32 offset, u32 size, u32 endian);
void writeNalsBufs(FILE *fout, VCEncStrmBufs *bufs, const u32 *pNaluSizeBuf, u32 numNalus,
                   u32 offset, u32 hdrSize, u32 endian);
void getStreamBufs(VCEncStrmBufs *bufs, struct test_bench *tb, commandLine_s *cml, bool encoding,
                   u32 index);
void writeIvfFrame(FILE *fout, VCEncStrmBufs *bufs, u32 streamSize, u32 offset, i32 *frameCnt,
                   bool bTileGroupSizeModified, u8 frameNotShow, i32 *ivfSize, u32 bVp9);
void writeIvfHeader(FILE *fout, i32 width, i32 height, i32 rateNum, i32 rateDenom, i32 vp9);

/* timer help*/
unsigned int uTimeDiff(struct timeval end, struct timeval start);
u64 uTimeDiffLL(struct timeval end, struct timeval start);

/*DEC400 Config*/
void SetupDec400CompTable(struct test_bench *tb, VCEncIn *pEncIn);
i32 read_dec400Data(struct test_bench *tb, u32 inputFormat, u32 src_width, u32 src_height);
void GetDec400CompTablebyFormat(const void *wl_inst, VCEncPictureType type, u32 width, u32 height,
                                u32 alignment, u32 *luma_Size, u32 *chroma_Size, u32 *picture_Size);

/* External SEI */
u8 ReadExternalSeiData(ExternalSEI **pExternalSEI, FILE *file);

//for resolution chage
int getInputFileList(struct test_bench *tb, commandLine_s *cmdl);
inputFileLinkList *inputFileLinkListDeleteHead(inputFileLinkList *list);

/* store the bits consumed by each GOP */
void StatisticFrameBits(VCEncInst inst, struct test_bench *tb, ma_s *ma, u32 frameBits,
                        u64 totalBits);

/* calculate bits deviation from the stored GOP bits */
float GetBitsDeviation(struct test_bench *tb);

/* ITU-T-T35 SEI */
u32 ReadT35PayloadData(ExternalSEI **p_external_sei, FILE *file);

/*get tb's ewl instance to malloc/free buffer in test_bench*/
const void *InitEwlInst(const commandLine_s *cml, struct test_bench *tb);
/*after all buffers allocated in test_bench had been freed, release tb's ewl instance*/
void ReleaseEwlInst(struct test_bench *tb);

#endif
