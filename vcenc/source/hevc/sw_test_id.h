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

//  Abstract : Encoder setup according to a test vector

#ifndef __HEVC_TESTID_H__
#define __HEVC_TESTID_H__

// header
#include "base_type.h"
#include "error.h"
#include "instance.h"
#include "sw_parameter_set.h"
#include "rate_control_picture.h"
#include "sw_slice.h"
#include "tools.h"
#include "enccommon.h"

/*------------------------------------------------------------------------------
    2. External compiler flags
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    4. Function prototypes
------------------------------------------------------------------------------*/

void HevcConfigureTestBeforeFrame(struct vcenc_instance *inst);
void HevcConfigureTestBeforeStart(struct vcenc_instance *inst);
void HevcRFDTest(struct vcenc_instance *inst, struct sw_picture *pic);
void HevcStreamBufferLimitTest(struct vcenc_instance *inst, VCEncStrmBufs *bufs);
u32 VCEncReEncodeTest(struct vcenc_instance *inst, struct sw_picture *pic, u32 status);
void EncConfigureTestId(struct vcenc_instance *inst);

typedef enum {
    //TestID          Description
    TID_NONE = 0, // No action, normal encoder operation
    TID_QP = 1,   // Frame quantization test, adjust qp for every frame, qp = 0..51
    TID_SLC = 2,  // Slice test, adjust slice amount for each frame
    TID_INT = 4,  // Stream buffer limit test, limit=500 (4kB) for first frame
    /*
      4
      6       Quantization test, min and max QP values.
  */
    TID_DBF = 7, // Filter test, set disableDeblocking and filterOffsets A and B
    /*
      8       Segment test, set segment map and segment qps
      9       Reference frame test, all combinations of reference and refresh.
      10      Segment map test
      11      Temporal layer test, reference and refresh as with 3 layers
      12      User data test
  */
    TID_F32 = 14, // Intra32Favor test, set to maximum value
    TID_F16 = 15, // Intra16Favor test, set to maximum value
    /*    16      Cropping test, set cropping values for every frame
      19      RGB input mask test, set all values
      20      MAD test, test all MAD QP change values
      21      InterFavor test, set to maximum value
      22      MV test, set cropping offsets so that max MVs are tested
      23      DMV penalty test, set to minimum/maximum values
      24      Max overfill MV
  */
    TID_ROI = 26, // ROI test
    TID_IA = 27,  // Intra area test
    TID_CIR = 28, // CIR test
    /*
      29      Intra slice map test
      31      Non-zero penalty test, don't use zero penalties
      34      Downscaling test
  */
    TID_LTR = 35,        // Long term reference test
    TID_POC = 36,        //test delta_poc
    TID_RFD = 37,        //test rfd with error compress table
    TID_SMT = 38,        //smart alg test
    TID_IPCM = 39,       // IPCM test
    TID_CCH = 40,        //const chroma test
    TID_RC = 41,         //test ctb rc
    TID_GMV = 42,        //test global mv
    TID_EXTLINEBUF = 43, // test external line buffer
    TID_MEVR = 44,       //test ME vertical search range
    TID_INPUTASREF = 45, //test RefFrameUsingInputFrameEnable=1
    TID_NON_I4X4 = 46,
    TID_NON_I8X8 = 47,
    TID_ME_QP = 48,
    TID_RDOQ_LAMBDA_ADJ = 49,
    TID_DISABLE_BI_IN_LDB = 50,
    TID_BILINEAR_DS = 51,
    TID_SIMPLE_RDO = 52,
    TID_INTRA_BY_SATD = 53,
    TID_RE_ENCODE = 54,
    TID_MOTION_SCORE = 55,
    TID_IMOUTBLOCKUNIT1 =
        56, // test IM original/output block unit combination. dsRatio 1/1, outRoiMapDeltaQpBlockUnit 64x64
    TID_IMOUTBLOCKUNIT2 =
        57, // test IM original/output block unit combination. dsRatio 1/1, outRoiMapDeltaQpBlockUnit 32x32
    TID_IMOUTBLOCKUNIT3 =
        58, // test IM original/output block unit combination. dsRatio 1/1, outRoiMapDeltaQpBlockUnit 16x16
    TID_IMOUTBLOCKUNIT4 =
        59, // test IM original/output block unit combination. dsRatio 1/2, outRoiMapDeltaQpBlockUnit 64x64
    TID_IMOUTBLOCKUNIT5 =
        60, // test IM original/output block unit combination. dsRatio 1/2, outRoiMapDeltaQpBlockUnit 32x32
    TID_BYPASS_RDO = 61, // intra_by_satd + input_as_ref to test bypass rdo
    TID_MAX
} TID;

#endif
