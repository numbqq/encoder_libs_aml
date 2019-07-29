//--=========================================================================--
//  This file is a part of VPU Reference API project
//-----------------------------------------------------------------------------
//
//       This confidential and proprietary software may be used only
//     as authorized by a licensing agreement from Chips&Media Inc.
//     In the event of publication, the following notice is applicable:
//
//            (C) COPYRIGHT 2006 - 2011  CHIPS&MEDIA INC.
//                      ALL RIGHTS RESERVED
//
//       The entire notice above must be reproduced on all authorized
//       copies.
//		This file should be modified by some customers according to their SOC configuration.
//--=========================================================================--

#ifndef _VPU_CONFIG_H_
#define _VPU_CONFIG_H_

#define ENC_STREAM_BUF_SIZE  0xF00000
#define ENC_STREAM_BUF_COUNT 4


#define BODA950_CODE                    0x9500
#define CODA960_CODE                    0x9600
#define CODA980_CODE                    0x9800

#define VP512_CODE                    0x5120
#define VP520_CODE                    0x5200
#define VP515_CODE                    0x5150
#define VP525_CODE                    0x5250

#define VP511_CODE                    0x5110
#define VP521_CODE                    0x5210
#define VP521C_CODE                   0x521c

#define PRODUCT_CODE_W_SERIES(x) (x == VP512_CODE || x == VP520_CODE || x == VP515_CODE || x == VP525_CODE || x == VP511_CODE || x == VP521_CODE || x == VP521C_CODE)
#define PRODUCT_CODE_NOT_W_SERIES(x) (x == BODA950_CODE || x == CODA960_CODE || x == CODA980_CODE)

#define VP5_MAX_CODE_BUF_SIZE             (1024*1024)
#define VP520ENC_WORKBUF_SIZE             (128*1024)
#define VP525ENC_WORKBUF_SIZE             (2*1024*1024)	// svac encoder needs 2MB for workbuffer
#define VP521ENC_WORKBUF_SIZE             (128*1024)      //HEVC 128K, AVC 40K

#define VP512DEC_WORKBUF_SIZE             (2*1024*1024)
#define VP515DEC_WORKBUF_SIZE             (2*1024*1024)
#define VP525DEC_WORKBUF_SIZE             (1.5*1024*1024)
#define VP521DEC_WORKBUF_SIZE             (1.5*1024*1024)
#define VP525_SVAC_DEC_WORKBUF_SIZE       (7*1024*1024) // max mvcol buffer included in workbuffer due to sequence change.


#define MAX_INST_HANDLE_SIZE            48              /* DO NOT CHANGE THIS VALUE */
#define MAX_NUM_INSTANCE                4
#define MAX_NUM_VPU_CORE                1
#define MAX_NUM_VCORE                   1

    #define MAX_ENC_AVC_PIC_WIDTH           4096
    #define MAX_ENC_AVC_PIC_HEIGHT          2304  
#define MAX_ENC_PIC_WIDTH               4096
#define MAX_ENC_PIC_HEIGHT              2304
#define MIN_ENC_PIC_WIDTH               96
#define MIN_ENC_PIC_HEIGHT              16

// for VPU420
#define W4_MIN_ENC_PIC_WIDTH            256
#define W4_MIN_ENC_PIC_HEIGHT           128
#define W4_MAX_ENC_PIC_WIDTH            8192
#define W4_MAX_ENC_PIC_HEIGHT           8192

#define MAX_DEC_PIC_WIDTH               4096
#define MAX_DEC_PIC_HEIGHT              2304

#define MAX_CTU_NUM                     0x4000      // CTU num for max resolution = 8192x8192/(64x64)
#define MAX_SUB_CTU_NUM	                (MAX_CTU_NUM*4)
#define MAX_MB_NUM                      0x40000     // MB num for max resolution = 8192x8192/(16x16)

//  Application specific configuration
#define VPU_ENC_TIMEOUT                 60000
#define VPU_DEC_TIMEOUT                 20000
#define VPU_BUSY_CHECK_TIMEOUT          5000

// codec specific configuration
#define VPU_REORDER_ENABLE              1   // it can be set to 1 to handle reordering DPB in host side.
#define CBCR_INTERLEAVE			        1 //[default 1 for BW checking with CnMViedo Conformance] 0 (chroma separate mode), 1 (chroma interleave mode) // if the type of tiledmap uses the kind of MB_RASTER_MAP. must set to enable CBCR_INTERLEAVE
#define VPU_ENABLE_BWB			        1 

#define HOST_ENDIAN                     VDI_128BIT_LITTLE_ENDIAN
#define VPU_FRAME_ENDIAN                HOST_ENDIAN
#define VPU_STREAM_ENDIAN               HOST_ENDIAN
#define VPU_USER_DATA_ENDIAN            HOST_ENDIAN
#define VPU_SOURCE_ENDIAN               HOST_ENDIAN
#define DRAM_BUS_WIDTH                  16


// for VP520
#define USE_SRC_PRP_AXI         0
#define USE_SRC_PRI_AXI         1
#define DEFAULT_SRC_AXI         USE_SRC_PRP_AXI

/************************************************************************/
/* VPU COMMON MEMORY                                                    */
/************************************************************************/
#define COMMAND_QUEUE_DEPTH             4

#define ENC_SRC_BUF_NUM             (12+COMMAND_QUEUE_DEPTH)          //!< case of GOPsize = 8 (IBBBBBBBP), max src buffer num  = 12

#define ONE_TASKBUF_SIZE_FOR_VP5DEC_CQ         (8*1024*1024)   /* upto 8Kx4K, need 8Mbyte per task*/
#define ONE_TASKBUF_SIZE_FOR_VP5ENC_CQ         (8*1024*1024)  /* upto 8Kx8K, need 8Mbyte per task.*/
#define ONE_TASKBUF_SIZE_FOR_VP511DEC_CQ       (8*1024*1024)  /* upto 8Kx8K, need 8Mbyte per task.*/

#define ONE_TASKBUF_SIZE_FOR_CQ     ONE_TASKBUF_SIZE_FOR_VP5ENC_CQ    
#define SIZE_COMMON                 ((2*1024*1024) + (COMMAND_QUEUE_DEPTH*ONE_TASKBUF_SIZE_FOR_CQ))

//=====4. VPU REPORT MEMORY  ======================//
#define SIZE_REPORT_BUF                 (0x10000)

#define STREAM_END_SIZE                 0
#define STREAM_END_SET_FLAG             0
#define STREAM_END_CLEAR_FLAG           -1
#define EXPLICIT_END_SET_FLAG           -2

#define USE_BIT_INTERNAL_BUF            1
#define USE_IP_INTERNAL_BUF             1
#define USE_DBKY_INTERNAL_BUF           1
#define USE_DBKC_INTERNAL_BUF           1
#define USE_OVL_INTERNAL_BUF            1
#define USE_BTP_INTERNAL_BUF            1
#define USE_ME_INTERNAL_BUF             1

/* VPU410 only */
#define USE_BPU_INTERNAL_BUF            1
#define USE_VCE_IP_INTERNAL_BUF         1
#define USE_VCE_LF_ROW_INTERNAL_BUF     1

/* VPU420 only */
#define USE_IMD_INTERNAL_BUF            1
#define USE_RDO_INTERNAL_BUF            1
#define USE_LF_INTERNAL_BUF             1


#define VP5_UPPER_PROC_AXI_ID     0x0

#define VP5_PROC_AXI_ID           0x0
#define VP5_PRP_AXI_ID            0x0
#define VP5_FBD_Y_AXI_ID          0x0
#define VP5_FBC_Y_AXI_ID          0x0
#define VP5_FBD_C_AXI_ID          0x0
#define VP5_FBC_C_AXI_ID          0x0
#define VP5_SEC_AXI_ID            0x0
#define VP5_PRI_AXI_ID            0x0

#endif  /* _VPU_CONFIG_H_ */

