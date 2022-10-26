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
--------------------------------------------------------------------------------
--
--  Description : Internal traces
--
------------------------------------------------------------------------------*/

#ifndef __ENCTRACE_H__
#define __ENCTRACE_H__

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/
#include "base_type.h"
#include "hevcencapi.h"
#include "sw_put_bits.h"
#include "sw_picture.h"
//#include "encpreprocess.h"
#include "encasiccontroller.h"
#include "vsi_string.h"

/*------------------------------------------------------------------------------
    2. External compiler flags
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/
struct enc_sw_trace {
    struct container *container;
    struct queue files;               /* Open files */
    struct queue stream_trace;        /* Stream trace buffer store */
    struct queue stream_header_trace; /* Stream header trace buffer store */

    FILE *stream_trace_fp;        /* Stream trace file */
    FILE *stream_header_trace_fp; /* Stream header trace file */
    FILE *deblock_fp;             /* deblock.yuv file */
    FILE *cutree_ctrl_flow_fp;    /* cutree control flow trace file */
    FILE *prof_fp;                /* profile.yuv file, record the bitrate and PSNR */

    int trace_frame_id;
    int cur_frame_idx;
    int cur_frame_enqueue_idx; /* frame idx before enqueue */
    int parallelCoreNum;
    bool bTraceCurFrame;
    bool bTraceCuInfo;
    int trace_pass;
};

/*------------------------------------------------------------------------------
    4. Function prototypes
------------------------------------------------------------------------------*/
i32 Enc_test_data_init(i32 parallelCoreNum);

void Enc_test_data_release(void);

i32 Enc_open_stream_trace(struct buffer *b);

i32 Enc_close_stream_trace(void);

void Enc_add_stream_header_trace(u8 *stream, u32 size);

void Enc_add_comment(struct buffer *b, i32 value, i32 number, char *comment);

// Update frame count and set trace flag accordingly. called when one one frame is done.
void EncTraceUpdateStatus();

// Write reconstruct data.
void EncTraceReconEnd();
int EncTraceRecon(VCEncInst inst, u32 *outIdrFileOffset, char *f_recon, VCEncOut *pEncOut);

// TOP trace file generation routines
void EncTraceReferences(struct container *c, struct sw_picture *pic, int pass);

bool EncTraceCuInfoCheck(void);

void EncTraceCuInformation(VCEncInst inst, VCEncOut *pEncOut, i32 frame, i32 poc);

void EncTraceCtbBits(VCEncInst inst, u16 *ctbBits);

#if 0
void EncTraceAsicParameters(asicData_s *asic);
void EncTracePreProcess(preProcess_s *preProcess);
void EncTraceSegments(u32 *map, i32 bytes, i32 enable, i32 mapModified,
                      i32 *cnt, i32 *qp);
void EncTraceProbs(u16 *ptr, i32 bytes);

void EncDumpMem(u32 *ptr, u32 bytes, char *desc);
void EncDumpRecon(asicData_s *asic);

void EncTraceCloseAll(void);
#endif

#endif
