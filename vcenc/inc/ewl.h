/*------------------------------------------------------------------------------
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                                                                            --
--                            Verisilicon.                                    --
--                                                                            --
--                   (C) COPYRIGHT 2014 VERISILICON                           --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
--------------------------------------------------------------------------------
--
--  Abstract : H2 Encoder Wrapper Layer for OS services
--
------------------------------------------------------------------------------*/

#ifndef __EWL_H__
#define __EWL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "base_type.h"
#include "vsi_queue.h"
#include "osal.h"
#include "encswhwregisters.h"
#include "vsi_string.h"

#include "ewl_hwcfg.h"

/**
 * \mainpage Overview
 *
 * This document describes the Application Programming Interface (API) of the
 * Hantro VCCORE Encoder Wrapper Layer for VCCORE Video Encoder hardware.
 * Encoder software is written in ANSI C and a compliant compiler is the
 * fundamental requirement for porting the software. Encoder functionality is
 * exposed to an application through an API on top of all the format specific
 * algorithms and control software. Control software relies on an operating
 * system abstraction called the Encoder Wrapper Layer (EWL), which provides
 * all the needed system dependent resources. This structure was chosen to
 * remove the need of modifications in the control software when ported to a
 * new target system.
 *
 * Details of the EWL Encoder Wrapper Layer API are described in this document.
 *  - First components such as data types, return codes, enumerations and relevant structures are described
 *  - Then function syntax and description is presented.
 *  - Finally a software integration guide is presented.
 *
 * \section intro_s1 Supported Standards
 * The API discussed in this version of the document is compatible with the following
 * video encoder standards and profiles:
 *  - HEVC (H.265) - ITU-T Rec. H.265 (04/2013), ISO/IEC 23008-2
 *
 * \section intro_s2 Compatible Hardware
 * Hantro VC8000E Video Encoder IP v6.2.xx or earlier.
 *
 * \section intro_s3 Deprecated Functions
 * This API includes some deprecated functions which may have been supported for earlier
 * Hantro encoders but which are no longer supported for use with VCCORE hardware. These
 * are listed in a separate section near the end of the document.
 */

/**
\page Document Revision History

This section describes top level differences in the versions of this document.

Note: This document is not necessarily updated for each patch or minor revision. The information
in this document tends to be stable across a revision (nnn) series.

| Revision | Date       | Compatible cores                       | Comments |
| -------- | ---------- | -------------------------------------- | -------- |
| 1.00     | 2017-03-28 | VC8000E Series (as of VC8000E v6.0.00) | Initial  |

*/

/** integration.md */

/* HW ID check. */
#define HW_ID_PRODUCT_MASK 0xFFFF0000
#define HW_ID_MAJOR_NUMBER_MASK 0x0000FF00
#define HW_ID_MINOR_NUMBER_MASK 0x000000FF
#define HW_ID_PRODUCT(x) (((x & HW_ID_PRODUCT_MASK) >> 16))
#define HW_ID_MAJOR_NUMBER(x) (((x & HW_ID_MAJOR_NUMBER_MASK) >> 8))
#define HW_ID_MINOR_NUMBER(x) ((x & HW_ID_MINOR_NUMBER_MASK))

#define HW_ID_PRODUCT_H2 0x4832
#define HW_ID_PRODUCT_VC8000E 0x8000
#define HW_ID_PRODUCT_VC9000E 0x9000
#define HW_ID_PRODUCT_VC9000LE 0x9010

#define HW_PRODUCT_H2(x) (HW_ID_PRODUCT_H2 == HW_ID_PRODUCT(x))
#define HW_PRODUCT_VC8000E(x) (HW_ID_PRODUCT_VC8000E == HW_ID_PRODUCT(x))
//0x80006000
#define HW_PRODUCT_SYSTEM60(x)                                                                     \
    ((HW_ID_PRODUCT_VC8000E == HW_ID_PRODUCT(x)) && (HW_ID_MAJOR_NUMBER(x) == 0x60))
//0x80006010
#define HW_PRODUCT_SYSTEM6010(x) (HW_PRODUCT_SYSTEM60(x) && (HW_ID_MINOR_NUMBER(x) == 0x10))
//0x90001000
#define HW_PRODUCT_VC9000(x) (HW_ID_PRODUCT_VC9000E == HW_ID_PRODUCT(x))
//0x90101000
#define HW_PRODUCT_VC9000LE(x) (HW_ID_PRODUCT_VC9000LE == HW_ID_PRODUCT(x))

/** from this ASIC_ID, BUILD_ID is supported */
#define MIN_ASIC_ID_WITH_BUILD_ID 0x80009100

#define HWIF_REG_BUILD_ID (509)
#define HWIF_REG_BUILD_REV (510)
#define HWIF_REG_BUILD_DATE (511)

#define HWIF_REG_CFG1 (80)
#define HWIF_REG_CFG2 (214)
#define HWIF_REG_CFG3 (226)
#define HWIF_REG_CFG4 (287)
#define HWIF_REG_CFGAXI (319)
#define HWIF_REG_CFG5 (430)
#define HWIF_CFG_NUM 7

#define CORE_INFO_MODE_OFFSET 31
#define CORE_INFO_AMOUNT_OFFSET 28

#define MAX_SUPPORT_CORE_NUM 4
/* HW status register bits */
#define ASIC_STATUS_SEGMENT_READY 0x1000
#define ASIC_STATUS_FUSE_ERROR 0x200
#define ASIC_STATUS_SLICE_READY 0x100
#define ASIC_STATUS_LINE_BUFFER_DONE 0x080 /* low latency */
#define ASIC_STATUS_HW_TIMEOUT 0x040
#define ASIC_STATUS_BUFF_FULL 0x020
#define ASIC_STATUS_HW_RESET 0x010
#define ASIC_STATUS_ERROR 0x008
#define ASIC_STATUS_FRAME_READY 0x004
#define ASIC_IRQ_LINE 0x001

#define ASIC_STATUS_ALL                                                                            \
    (ASIC_STATUS_SEGMENT_READY | ASIC_STATUS_FUSE_ERROR | ASIC_STATUS_SLICE_READY |                \
     ASIC_STATUS_LINE_BUFFER_DONE | ASIC_STATUS_HW_TIMEOUT | ASIC_STATUS_BUFF_FULL |               \
     ASIC_STATUS_HW_RESET | ASIC_STATUS_ERROR | ASIC_STATUS_FRAME_READY)

/* Return values */
#define EWL_OK 0
#define EWL_ERROR -1
#define EWL_NOT_SUPPORT -2

#define EWL_HW_WAIT_OK EWL_OK
#define EWL_HW_WAIT_ERROR EWL_ERROR
#define EWL_HW_WAIT_TIMEOUT 1

/* HW configuration values */
#define EWL_HW_BUS_TYPE_UNKNOWN 0
#define EWL_HW_BUS_TYPE_AHB 1
#define EWL_HW_BUS_TYPE_OCP 2
#define EWL_HW_BUS_TYPE_AXI 3
#define EWL_HW_BUS_TYPE_PCI 4

#define EWL_HW_BUS_WIDTH_UNKNOWN 0
#define EWL_HW_BUS_WIDTH_32BITS 1
#define EWL_HW_BUS_WIDTH_64BITS 2
#define EWL_HW_BUS_WIDTH_128BITS 3

#define EWL_HW_SYNTHESIS_LANGUAGE_UNKNOWN 0
#define EWL_HW_SYNTHESIS_LANGUAGE_VHDL 1
#define EWL_HW_SYNTHESIS_LANGUAGE_VERILOG 2

#define EWL_HW_CONFIG_NOT_SUPPORTED 0
#define EWL_HW_CONFIG_ENABLED 1

#define EWL_MAX_CORES 4

/** core_id : bit0~7 is core index, bit 16~23 is slice node index. */
/* get core slice index from core ID */
#define CORE(id) ((u32)(id)&0xff)
#define NODE(id) ((u32)(id) >> 16)
/* combine slice node and core index as COREID */
#define COREID(node, core) (((u32)(node) << 16) | ((u32)(core)&0xff))

#ifdef WIN32
#define GETPID() _getpid()
#else
#define GETPID() getpid()
#endif

#define MEM_CHUNKS 720

/* Page size and align for linear memory chunks. */
#define LINMEM_ALIGN 4096

#define NEXT_ALIGNED(x) ((((ptr_t)(x)) + LINMEM_ALIGN - 1) / LINMEM_ALIGN * LINMEM_ALIGN)
#define NEXT_ALIGNED_SYS(x, align) ((((ptr_t)(x)) + align - 1) / align * align)

#define EWL_MEM_TYPE_CPU 0x0000U         /**< CPU RW. non-secure CMA memory */
#define EWL_MEM_TYPE_SLICE 0x0001U       /**< As output buffer for Encoder */
#define EWL_MEM_TYPE_DPB 0x0002U         /**< As Input/Recon buffer for Encoder*/
#define EWL_MEM_TYPE_VPU_WORKING 0x0003U /**< Non-secure memory. As working buffer for Encoder */
#define EWL_MEM_TYPE_VPU_WORKING_SPECIAL 0x0004U /**< VPU R, CPU RW. */
#define EWL_MEM_TYPE_VPU_ONLY 0x0005U            /**< VPU RW only. */

#define CPU_RD 0x0100U /**< CPU Read  memory*/
#define CPU_WR 0x0200U /**< CPU Write memory*/
#define VPU_RD 0x0400U /**< VPU Read  memory*/
#define VPU_WR 0x0800U /**< VPU Write memory*/
#define EXT_RD 0x1000U /**< EXT Read  memory*/
#define EXT_WR 0x2000U /**< EXT Write memory*/

#define EWL_PAR_ERROR EWL_ERROR
#define EWL_HW_ERROR -2

/**
 * @enum EWLMemSyncDirection
 * @brief   EWL transfer direction
 */
enum EWLMemSyncDirection {
    HOST_TO_DEVICE = 0, /**< copy the data from host memory to device memory */
    DEVICE_TO_HOST = 1  /**< copy the data from device memory to host memory */
};

#ifdef MEM_ONLY_DEV_CHECK
#define EWL_DEVMEM_VAILD(mem) ((mem).busAddress != 0)
#else
#define EWL_DEVMEM_VAILD(mem) ((mem).virtualAddress != NULL)
#endif
/**
 * \defgroup api_ewl Encoder Wrapper Layer(EWL) API
 *
 * @{
 */
/* Allocated memory information for counting*/
typedef struct EWLMemoryStatistics {
    u32 maxHeapMemory;          /**< count Max Memory Value. */
    u32 memoryValue;            /**< count Memory Value. */
    u32 maxHeapMallocTimes;     /**< count EWLmalloc()/calloc() function called times. */
    u32 averageHeapMallocTimes; /**< count max [EWLmalloc() - EWLfree()] function called times. */
    u32 heapMallocTimes;        /**< count [EWLmalloc() - EWLfree()] function called times. */

    u32 maxLinearMemory;      /**< count Max LinearBuffer Memory Value. */
    u32 linearMemoryValue;    /**< count LinearBuffer Memory Value. */
    u32 maxLinearMallocTimes; /**< count EWLMallocLinear()/EWLMallocRefFrm() function called times. */
    u32 averageLinearMallocTimes; /**< count max [EWLMallocLinear() - EWLFreeLinear()] function called times. */
    u32 linearMallocTimes; /**< count [EWLMallocLinear() - EWLFreeLinear()] function called times. */
} EWLMemoryStatistics_t;

/** Allocated linear memory area information */
typedef struct EWLLinearMem {
    u32 *virtualAddress;   /**< aligned virtual address access by CPU */
    ptr_t busAddress;      /**< aligned address access by VPU */
    u32 size;              /**< size in byte */
    u32 *allocVirtualAddr; /**< allocated virtual address access by CPU */
    ptr_t allocBusAddr;    /**< allocated address access by VPU */
    unsigned long id;      /**< ID to identify buffer */
    u32 mem_type;          /**< Indicate the memory owners and attributes,
                              * The bit[31:16] means access memory owners,
                              * The bit[15:0] indicate the attributes of the memory.*/
    u32 total_size;        /**< total size of the allocated buffer in byte */
    void *priv;            /**< for application use */
} EWLLinearMem_t;

/** EWLInitParam is used to pass parameters when initializing the EWL */
typedef struct EWLInitParam {
    u32 clientType; /**< encoding format type,0: */
    void *context;  /**< initialized in application and it will be resent to application by EWL */
    u32 slice_idx;  /**< \brief slice node for multi-node VPU */
} EWLInitParam_t;

/** L2-cache data */
typedef struct CacheData {
    void **cache; /**< L2-cache */
} CacheData_t;

typedef void (*CoreWaitCallBackFunc)(const void *ewl, void *data);

/** The detailed parameter of a job  */
typedef struct EWLCoreWaitJob {
    struct node *next;                  /**< point to the next job */
    u32 id;                             /**< ID to identify job */
    u32 core_id;                        /**< HW subsystem id */
    const void *inst;                   /**< point to struct vcenc_instance */
    u32 VC8000E_reg[ASIC_SWREG_AMOUNT]; /**< hadware registers, the maxsize is ASIC_SWREG_AMOUNT */
    i32 out_status; /**< return interrupt status of the job, the value (ASIC_STATUS_SEGMENT_READY |\
                               ASIC_STATUS_FUSE_ERROR |\
                               ASIC_STATUS_SLICE_READY |\
                               ASIC_STATUS_LINE_BUFFER_DONE |\
                               ASIC_STATUS_HW_TIMEOUT |\
                               ASIC_STATUS_BUFF_FULL |\
                               ASIC_STATUS_HW_RESET |\
                               ASIC_STATUS_ERROR |\
                               ASIC_STATUS_FRAME_READY) */
    u32 slices_rdy; /**< the amount of completed slices */
    u32 low_latency_rd; /**< the number of CTB rows that has been fetched from Input buffer by encoder */
    u32 dec400_enable; /**< 1: bypass 2: enable */
    //VCDec400data dec400_data;
    CoreWaitCallBackFunc dec400_callback; /**< dec400 callback resvered for future */
    u32 axife_enable; /**< 0: axiFE disable   1:axiFE normal mode enable   2:axiFE bypass  3.security mode*/
    CoreWaitCallBackFunc axife_callback; /**< a callback about axife, see also VCEncAxiFeDisable */
    u32 l2cache_enable;                  /**< SUPPORT_CACHE=y:1   SUPPORT_CACHE=n:0  */
    CacheData_t l2cache_data;            /**< L2-cache */
    CoreWaitCallBackFunc l2cache_callback; /**< a callback about axife, see also DisableCache */
} EWLCoreWaitJob_t;

/** The related jobs queue parameter */
typedef struct EWLCoreWait {
    struct queue jobs;         /**< jobs queue */
    pthread_mutex_t job_mutex; /**< mutex for protect job_pool */
    pthread_cond_t job_cond;   /**< condition variables for job_mutex */
    struct queue out;          /**< the completed jobs, include the abnormal and normal jobs */
    pthread_mutex_t out_mutex; /**< protect out queue */
    pthread_cond_t out_cond;   /**< condition variables for out_mutex */
    pthread_t *tid_CoreWait;   /**< point to thread object */
    bool bFlush;               /**< 1: encode complete   0:encode incomplete */
    u32 refer_counter; /**< counter for HEVC, H264, AV1, VP9 encoder type, equal to 0 when release the EWL instance */
    struct queue job_pool; /**< the pool storing un-used job for other usage */
} EWLCoreWait_t;

/** The node of a job queue */
typedef struct EWLWaitJobCfg {
    u32 waitCoreJobid; /**< driver maintain */
    u32 dec400_enable; /**< 1: bypass 2: enable */
    void *dec400_data; /* for dec400_data storage */
    u32 axife_enable; /**< 0: axiFE disable   1:axiFE normal mode enable   2:axiFE bypass   3.security mode*/
    CoreWaitCallBackFunc axife_callback; /**< a callback about axife, see also VCEncAxiFeDisable */
    u32 l2cache_enable;                  /**< SUPPORT_CACHE=y:1 SUPPORT_CACHE=n:0  */
    void *l2cache_data;                  /**< L2-cache */
    CoreWaitCallBackFunc l2cache_callback; /**< a callback about L2 cache, see also DisableCache */
} EWLWaitJobCfg_t;

#if 0
#define EWL_CLIENT_TYPE_H264_ENC 0U
#define EWL_CLIENT_TYPE_HEVC_ENC 1U
#define EWL_CLIENT_TYPE_VP9_ENC 2U
#define EWL_CLIENT_TYPE_JPEG_ENC 3U
#define EWL_CLIENT_TYPE_CUTREE 4U
#define EWL_CLIENT_TYPE_VIDEOSTAB 5U
#define EWL_CLIENT_TYPE_DEC400 6U
#define EWL_CLIENT_TYPE_AV1_ENC 7U
#endif

/** CLIENT_TYPE is used to select available HW engine when initializing the EWL */
typedef enum {
    EWL_CLIENT_TYPE_H264_ENC = 0U,  /**< H264 Encoder */
    EWL_CLIENT_TYPE_HEVC_ENC = 1U,  /**< HEVC Encoder */
    EWL_CLIENT_TYPE_VP9_ENC = 2U,   /**< VP9 Encoder */
    EWL_CLIENT_TYPE_JPEG_ENC = 3U,  /**< JPEG Encoder */
    EWL_CLIENT_TYPE_CUTREE = 4U,    /**< CU Tree Analyzer Engine */
    EWL_CLIENT_TYPE_VIDEOSTAB = 5U, /**< Video Stabilization Engine (invalid now) */
    EWL_CLIENT_TYPE_DEC400 = 6U,    /**< DEC400 engine */
    EWL_CLIENT_TYPE_AV1_ENC = 7U,   /**< AV1 Encoder */
    EWL_CLIENT_TYPE_L2CACHE = 8U,   /**< L2CACHE engine */
    EWL_CLIENT_TYPE_AXIFE = 9U,     /**< AXIfe engine */
    EWL_CLIENT_TYPE_APBFT = 10U,    /**< APBfilter engine */
    EWL_CLIENT_TYPE_AXIFE_1 = 11U,  /**< AXIfe_1 engine */
    EWL_CLIENT_TYPE_MEM = 12U,      /**< alloc/free memory */
    EWL_CLIENT_TYPE_MAX
} CLIENT_TYPE;

/* Use 'k' as magic number */
#define HANTRO_IOC_MAGIC 'k'

/*
 * S means "Set" through a ptr,
 * T means "Tell" directly with the argument value
 * G means "Get": reply by setting through a pointer
 * Q means "Query": response is on the return value
 * X means "eXchange": G and S atomically
 * H means "sHift": T and Q atomically
 */

#define HANTRO_IOCG_HWOFFSET _IOR(HANTRO_IOC_MAGIC, 3, unsigned long *)
#define HANTRO_IOCG_HWIOSIZE _IOR(HANTRO_IOC_MAGIC, 4, unsigned int *)
#define HANTRO_IOC_CLI _IO(HANTRO_IOC_MAGIC, 5)
#define HANTRO_IOC_STI _IO(HANTRO_IOC_MAGIC, 6)
#define HANTRO_IOCX_VIRT2BUS _IOWR(HANTRO_IOC_MAGIC, 7, unsigned long *)
#define HANTRO_IOCH_ARDRESET _IO(HANTRO_IOC_MAGIC, 8) /* debugging tool */
#define HANTRO_IOCG_SRAMOFFSET _IOR(HANTRO_IOC_MAGIC, 9, unsigned long *)
#define HANTRO_IOCG_SRAMEIOSIZE _IOR(HANTRO_IOC_MAGIC, 10, unsigned int *)
#define HANTRO_IOCH_ENC_RESERVE _IOR(HANTRO_IOC_MAGIC, 11, unsigned int *)
#define HANTRO_IOCH_ENC_RELEASE _IOR(HANTRO_IOC_MAGIC, 12, unsigned int *)
#define HANTRO_IOCG_CORE_NUM _IOR(HANTRO_IOC_MAGIC, 13, unsigned int *)
#define HANTRO_IOCG_CORE_INFO _IOR(HANTRO_IOC_MAGIC, 14, SUBSYS_CORE_INFO *)
#define HANTRO_IOCG_CORE_WAIT _IOR(HANTRO_IOC_MAGIC, 15, unsigned int *)
#define HANTRO_IOCG_ANYCORE_WAIT _IOR(HANTRO_IOC_MAGIC, 16, CORE_WAIT_OUT *)
#define HANTRO_IOCG_ANYCORE_WAIT_POLLING _IOR(HANTRO_IOC_MAGIC, 17, CORE_WAIT_OUT *)

#define HANTRO_IOCH_GET_CMDBUF_PARAMETER _IOWR(HANTRO_IOC_MAGIC, 25, struct cmdbuf_mem_parameter)
#define HANTRO_IOCH_GET_CMDBUF_POOL_SIZE _IOWR(HANTRO_IOC_MAGIC, 26, unsigned long)
#define HANTRO_IOCH_SET_CMDBUF_POOL_BASE _IOWR(HANTRO_IOC_MAGIC, 27, unsigned long)
#define HANTRO_IOCH_GET_VCMD_PARAMETER _IOWR(HANTRO_IOC_MAGIC, 28, struct config_parameter)
#define HANTRO_IOCH_RESERVE_CMDBUF _IOWR(HANTRO_IOC_MAGIC, 29, struct exchange_parameter)
#define HANTRO_IOCH_LINK_RUN_CMDBUF _IOWR(HANTRO_IOC_MAGIC, 30, struct exchange_parameter)
#define HANTRO_IOCH_WAIT_CMDBUF _IOR(HANTRO_IOC_MAGIC, 31, u16)
#define HANTRO_IOCH_RELEASE_CMDBUF _IOR(HANTRO_IOC_MAGIC, 32, u16)
#define HANTRO_IOCH_POLLING_CMDBUF _IOR(HANTRO_IOC_MAGIC, 33, u16)

#define HANTRO_IOCH_GET_VCMD_ENABLE _IOWR(HANTRO_IOC_MAGIC, 50, u64)
#define HANTRO_IOCH_GET_MMU_ENABLE _IOWR(HANTRO_IOC_MAGIC, 51, u32)

#define GET_ENCODER_IDX(type_info) (CORE_VC8000E)

#define JMP_IE_1 BIT(25)
#define JMP_RDY_1 BIT(26)

#define CLRINT_OPTYPE_READ_WRITE_1_CLEAR 0
#define CLRINT_OPTYPE_READ_WRITE_0_CLEAR 1
#define CLRINT_OPTYPE_READ_CLEAR 2

#define VC8000E_FRAME_RDY_INT_MASK 0x0001
#define VC8000E_CUTREE_RDY_INT_MASK 0x0002
#define VC8000E_DEC400_INT_MASK 0x0004

#define HW_ID_1_0_C 0x43421001

enum {
    CORE_VC8000E = 0,
    CORE_VC8000EJ = 1,
    CORE_CUTREE = 2,
    CORE_DEC400 = 3,
    CORE_MMU = 4,
    CORE_L2CACHE = 5,
    CORE_AXIFE = 6,
    CORE_APBFT = 7,
    CORE_MMU_1 = 8,
    CORE_AXIFE_1 = 9,
    CORE_MAX
};

//#define CORE_MAX  (CORE_MMU)

/*module_type support*/

enum vcmd_module_type {
    VCMD_TYPE_ENCODER = 0,
    VCMD_TYPE_CUTREE,
    VCMD_TYPE_DECODER,
    VCMD_TYPE_JPEG_ENCODER,
    VCMD_TYPE_JPEG_DECODER,
    MAX_VCMD_TYPE
};

#ifdef CONFIG_COMPAT
struct cmdbuf_mem_parameter {
    u32 cmd_virt_addr; //cmdbuf pool base virtual address
    u32 status_virt_addr;
    u32 cmd_phy_addr;      //cmdbuf pool base physical address, it's for cpu
    u32 cmd_hw_addr;       //cmdbuf pool base hardware address, it's for hardware ip
    u32 cmd_total_size;    //cmdbuf pool total size in bytes.
    u32 status_phy_addr;   //status cmdbuf pool base physical address, it's for cpu
    u32 status_hw_addr;    //status cmdbuf pool base hardware address, it's for hardware ip
    u32 status_total_size; //status cmdbuf pool total size in bytes.
    u32 base_ddr_addr; //for pcie interface, hw can only access phy_cmdbuf_addr-pcie_base_ddr_addr.
                       //for other interface, this value should be 0?
    u16 status_unit_size; //one status cmdbuf size in bytes. all status cmdbuf have same size.
    u16 cmd_unit_size;    //one cmdbuf size in bytes. all cmdbuf have same size.
};
#else
struct cmdbuf_mem_parameter {
};
#endif
struct config_parameter {
    u16 module_type;         //input vc8000e=0,cutree=1,vc8000d=2，jpege=3, jpegd=4
    u16 vcmd_core_num;       //output, how many vcmd cores are there with corresponding module_type.
    u16 submodule_main_addr; //output,if submodule addr == 0xffff, this submodule does not exist.
    u16 submodule_dec400_addr; //output ,if submodule addr == 0xffff, this submodule does not exist.
    u16 submodule_L2Cache_addr; //output,if submodule addr == 0xffff, this submodule does not exist.

    u16 submodule_MMU_addr[2]; //output,if submodule addr == 0xffff, this submodule does not exist.
    u16 submodule_axife_addr
        [2]; //output,if submodule addr == 0xffff, this submodule does not exist.
    u16 config_status_cmdbuf_id; // output , this status comdbuf save the all register values read in driver init.//used for analyse configuration in cwl.
    u32 vcmd_hw_version_id;
};

/*need to consider how many memory should be allocated for status.*/
struct exchange_parameter {
    u32 executing_time; //input ;executing_time=encoded_image_size*(rdoLevel+1)*(rdoq+1);
    u16 module_type;    //input input vc8000e=0,IM=1,vc8000d=2，jpege=3, jpegd=4
    u16 cmdbuf_size;    //input, reserve is not used; link and run is input.
    u16 priority;       //input,normal=0, high/live=1
    u16 cmdbuf_id;      //output ,it is unique in driver.
    u16 core_id;        //just used for polling.
};

typedef struct CoreWaitOut {
    u32 job_id[4];
    u32 irq_status[4];
    u32 irq_num;
} CORE_WAIT_OUT;

typedef struct {
    u32 type_info; //indicate which IP is contained in this subsystem and each uses one bit of this variable
    unsigned long offset[CORE_MAX];
    unsigned long regSize[CORE_MAX];
    int irq[CORE_MAX];
} SUBSYS_CORE_INFO;

#define HANTRO_IOC_MMU 'm'

#define HANTRO_IOCS_MMU_MEM_MAP _IOWR(HANTRO_IOC_MMU, 1, struct addr_desc *)
#define HANTRO_IOCS_MMU_MEM_UNMAP _IOWR(HANTRO_IOC_MMU, 2, struct addr_desc *)
#define HANTRO_IOCS_MMU_FLUSH _IOWR(HANTRO_IOC_MMU, 3, unsigned int *)

struct addr_desc {
    void *virtual_address; /* buffer virtual address */
    size_t bus_address;    /* buffer physical address */
    unsigned int size;     /* physical size */
};

//memalloc
/*
 * Ioctl definitions
 */
/* Use 'k' as magic number */
#define MEMALLOC_IOC_MAGIC 'k'
/*
 * S means "Set" through a ptr,
 * T means "Tell" directly with the argument value
 * G means "Get": reply by setting through a pointer
 * Q means "Query": response is on the return value
 * X means "eXchange": G and S atomically
 * H means "sHift": T and Q atomically
 */
#ifdef CONFIG_COMPAT
typedef struct MemallocParams_t {
    unsigned int bus_address;
    unsigned int size;
    unsigned int translation_offset;
    unsigned int mem_type;
} MemallocParams;
#else
typedef struct {
    unsigned long bus_address;
    unsigned int size;
    unsigned long translation_offset;
    unsigned int mem_type;
} MemallocParams;
#endif

#ifdef CONFIG_COMPAT
#define MEMALLOC_IOCXGETBUFFER _IOWR(MEMALLOC_IOC_MAGIC, 1, struct MemallocParams_t)
#define MEMALLOC_IOCSFREEBUFFER _IOW(MEMALLOC_IOC_MAGIC, 2, u32)
#define MEMALLOC_IOCGMEMBASE _IOR(MEMALLOC_IOC_MAGIC, 3, u32)

/* ... more to come */
#define MEMALLOC_IOCHARDRESET _IO(MEMALLOC_IOC_MAGIC, 15) /* debugging tool */
#else
#define MEMALLOC_IOCXGETBUFFER _IOWR(MEMALLOC_IOC_MAGIC, 1, MemallocParams *)
#define MEMALLOC_IOCSFREEBUFFER _IOW(MEMALLOC_IOC_MAGIC, 2, unsigned long *)
#define MEMALLOC_IOCGMEMBASE _IOR(MEMALLOC_IOC_MAGIC, 3, unsigned long *)

/* ... more to come */
#define MEMALLOC_IOCHARDRESET _IO(MEMALLOC_IOC_MAGIC, 15) /* debugging tool */
#endif



typedef struct {
    i32 (*EWLGetLineBufSramP)(const void *, EWLLinearMem_t *);
    i32 (*EWLMallocLoopbackLineBufP)(const void *, u32, EWLLinearMem_t *);
    u32 (*EWLGetClientTypeP)(const void *);
    u32 (*EWLGetCoreTypeByClientTypeP)(u32);
    i32 (*EWLCheckCutreeValidP)(const void *);
    u32 (*EWLReadAsicIDP)(u32, void *);
    EWLHwConfig_t (*EWLReadAsicConfigP)(u32, void *);
    const void *(*EWLInitP)(EWLInitParam_t *);
    i32 (*EWLReleaseP)(const void *);
    void (*EwlReleaseCoreWaitP)(void *);
    EWLCoreWaitJob_t *(*EWLDequeueCoreOutJobP)(const void *, u32);
    void (*EWLEnqueueOutToWaitP)(const void *, EWLCoreWaitJob_t *);
    void (*EWLEnqueueWaitjobP)(const void *, u32);
    void (*EWLPutJobtoPoolP)(const void *, struct node *);
    void (*EWLPutJobtoPool)(const void *, struct node *);
    void (*EWLWriteCoreRegP)(const void *, u32, u32, u32);
    void (*EWLWriteRegP)(const void *, u32, u32);
    void (*EWLSetReserveBaseDataP)(const void *, u32, u32, u32, u32, u32, u16);
    void (*EWLCollectWriteRegDataP)(const void *, u32 *, u32 *, u16, u32, u32 *);
    void (*EWLCollectNopDataP)(const void *, u32 *, u32 *);
    void (*EWLCollectStallDataEncVideoP)(const void *, u32 *, u32 *);
    void (*EWLCollectStallDataCuTreeP)(const void *, u32 *, u32 *);
    void (*EWLCollectReadRegDataP)(const void *, u32 *, u16, u32, u32 *, u16);
    void (*EWLCollectIntDataP)(const void *, u32 *, u32 *, u16);
    void (*EWLCollectJmpDataP)(const void *, u32 *, u32 *, u16);
    void (*EWLCollectClrIntDataP)(const void *, u32 *, u32 *);
    void (*EWLCollectClrIntReadClearDec400DataP)(const void *, u32 *, u32 *, u16);
    void (*EWLCollectStopHwDataP)(const void *, u32 *, u32 *);
    void (*EWLCollectReadVcmdRegDataP)(const void *, u32 *, u16, u32, u32 *, u16);
    void (*EWLCollectWriteDec400RegDataP)(const void *, u32 *, u32 *, u16, u32, u32 *);
    void (*EWLCollectStallDec400P)(const void *, u32 *, u32 *);
    void (*EWLWriteBackRegP)(const void *, u32, u32);
    void (*EWLEnableHWP)(const void *, u32, u32);
    u32 (*EWLGetPerformanceP)(const void *);
    void (*EWLDisableHWP)(const void *, u32, u32);
    u32 (*EWLReadRegP)(const void *, u32);
    u32 (*EWLReadRegInitP)(const void *, u32);
    i32 (*EWLMallocRefFrmP)(const void *, u32, u32, EWLLinearMem_t *);
    void (*EWLFreeRefFrmP)(const void *, EWLLinearMem_t *);
    i32 (*EWLMallocLinearP)(const void *, u32, u32, EWLLinearMem_t *);
    void (*EWLFreeLinearP)(const void *, EWLLinearMem_t *);
    i32 (*EWLSyncMemDataP)(EWLLinearMem_t *, u32, u32, enum EWLMemSyncDirection);
    i32 (*EWLMemSyncAllocHostBufferP)(const void *, u32, u32, EWLLinearMem_t *);
    i32 (*EWLMemSyncFreeHostBufferP)(const void *, EWLLinearMem_t *);
    void (*EWLDCacheRangeFlushP)(const void *, EWLLinearMem_t *);
    i32 (*EWLWaitHwRdyP)(const void *, u32 *, void *, u32 *);
    void (*EWLDCacheRangeRefreshP)(const void *, EWLLinearMem_t *);
    void (*EWLReleaseHwP)(const void *);
    i32 (*EWLReserveHwP)(const void *, u32 *, u32 *);
    u32 (*EWLGetCoreNumP)(void *);
    i32 (*EWLGetDec400CoreidP)(const void *);
    u32 (*EWLGetDec400CustomerIDP)(const void *, u32);
    void (*EWLGetDec400AttributeP)(const void *, u32 *, u32 *, u32 *);
    i32 (*EWLReserveCmdbufP)(const void *, u16, u16 *);
    void (*EWLCopyDataToCmdbufP)(const void *, u32 *, u32, u16);
    i32 (*EWLLinkRunCmdbufP)(const void *, u16, u16);
    i32 (*EWLWaitCmdbufP)(const void *, u16, u32 *);
    void (*EWLGetRegsByCmdbufP)(const void *, u16, u32 *);
    i32 (*EWLReleaseCmdbufP)(const void *, u16);
    void (*EWLTraceProfileP)(const void *, void *, i32, i32);
    u32 (*EWLGetVCMDSupportP)();
    void (*EWLSetVCMDModeP)(const void *inst, u32 mode);
    u32 (*EWLGetVCMDModeP)(const void *inst);

    u32 (*EWLReleaseEwlWorkerInstP)(const void *inst);
    void (*EWLClearTraceProfileP)(const void *);
} EWLFun;

extern u32 (*pollInputLineBufTestFunc)(void);

/*------------------------------------------------------------------------------
      4.  Function prototypes
  ------------------------------------------------------------------------------*/

/**
 * Reads and returns the hardware ID register value, static implementation.
 * \param [in] core_id Describes the hardware core to read.
 * \param [in] ctx context to access this hardware if needed, e.g the device descriptions for this instance.
 * \return The hardware ID register value.
 */
u32 EWLReadAsicID(u32 core_id, void *ctx);

u32 EWLGetCoreNum(void *ctx);

i32 EWLGetDec400Coreid(const void *inst);

void EWLDisableDec400(const void *inst);

int MapAsicRegisters(void *ewl);

u32 EWLGetClientType(const void *inst);

u32 EWLGetCoreTypeByClientType(u32 client_type);

i32 EWLCheckCutreeValid(const void *inst);

u32 EWLGetDec400CustomerID(const void *inst, u32 core_id);

void EWLGetDec400Attribute(const void *inst, u32 *tile_size, u32 *bits_tile_in_table,
                           u32 *planar420_cbcr_arrangement_style);

/** Read and return HW configuration info, static implementation */
EWLHwConfig_t EWLReadAsicConfig(u32 core_id, void *ctx);

/**
   * \brief Initialize the EWL instance
   *
   * EWLInit is called when the encoder instance is initialized.
   *
   * \param [in] param the configure for this instance
   * \return a wrapper instance or NULL for error
   *
   */
const void *EWLInit(EWLInitParam_t *param);

/**
   * Release the EWL instance. It is called when the encoder instance is released
   * \return EWL_OK or EWL_ERROR
   */
i32 EWLRelease(const void *inst);

/**
   * Reserve the HW resource for one codec instance. EWLReserveHw is called when beginning a frame encoding
   * The function may block until the resource is available.
   * \return EWL_OK if the resource was successfully reserved for this instance
   * \return EWL_ERROR if unable to reserve the resource.
   */
i32 EWLReserveHw(const void *inst, u32 *core_info, u32 *job_id);

/**
   * \brief Release the HW resource
   *
   * EWLReleaseHw is called when the HW has finished the frame encoding. Software should save the hardware
   *   status locally and continue the further processing according to such information. Hardware resource
   *   will not available then. The HW can be used by another instance then.
   */
void EWLReleaseHw(const void *inst);

void EwlReleaseCoreWait(void *inst);

EWLCoreWaitJob_t *EWLDequeueCoreOutJob(const void *inst, u32 waitCoreJobid);

void EWLEnqueueOutToWait(const void *inst, EWLCoreWaitJob_t *job);

void EWLEnqueueWaitjob(const void *inst, EWLWaitJobCfg_t *cfg);

void EWLPutJobtoPool(const void *inst, struct node *job);

u32 EWLGetPerformance(const void *inst);

/** Frame buffers memory */
i32 EWLMallocRefFrm(const void *instance, u32 size, u32 alignment, EWLLinearMem_t *info);
void EWLFreeRefFrm(const void *inst, EWLLinearMem_t *info);

/** SW/HW shared memory without Memory Synchronize*/
i32 EWLMallocLinear(const void *instance, u32 size, u32 alignment, EWLLinearMem_t *info);
void EWLFreeLinear(const void *inst, EWLLinearMem_t *info);

/* D-Cache coherence */ /* Not in use currently */
void EWLDCacheRangeFlush(const void *instance, EWLLinearMem_t *info);
void EWLDCacheRangeRefresh(const void *instance, EWLLinearMem_t *info);

/**
   * \brief Write value to a HW register
   *
   * All registers are written at once at the beginning of frame encoding
   * Offset is relative to the the HW ID register (#0) in bytes
   * Enable indicates when the HW is enabled. If shadow registers are used then
   * they must be flushed to the HW registers when enable is '1' before
   * writing the register that enables the HW
   */
void EWLWriteReg(const void *inst, u32 offset, u32 val);
/** Write back value to a HW register on callback/frame done (for multicore) */
void EWLWriteBackReg(const void *inst, u32 offset, u32 val);
/** Write value to HW register by specified core id*/
void EWLWriteCoreReg(const void *inst, u32 offset, u32 val, u32 core_id);

void EWLWriteRegbyClientType(const void *inst, u32 offset, u32 val, u32 client_type);

void EWLWriteBackRegbyClientType(const void *inst, u32 offset, u32 val, u32 client_type);

/** Read and return the value of a HW register
   * The status register is read after every macroblock encoding by SW
   * The other registers which may be updated by the HW are read after
   * BUFFER_FULL or FRAME_READY interrupt
   * Offset is relative to the the HW ID register (#0) in bytes */
u32 EWLReadReg(const void *inst, u32 offset);

u32 EWLReadRegbyClientType(const void *inst, u32 offset, u32 client_type);

/** Writing all registers in one call */ /*Not in use currently */
void EWLWriteRegAll(const void *inst, const u32 *table, u32 size);
/** Reading all registers in one call */ /*Not in use currently */
void EWLReadRegAll(const void *inst, u32 *table, u32 size);

/** HW enable/disable. This will write <val> to register <offset> and by */
/** this enablig/disabling the hardware. */
void EWLEnableHW(const void *inst, u32 offset, u32 val);
void EWLDisableHW(const void *inst, u32 offset, u32 val);

/**
   * \brief Synchronize SW with HW
   *
   * EWLWaitHwRdy is called after enabling the HW to wait for IRQ from HW.
   * If slicesReady pointer is given, at input it should contain the number
   * of slicesReady received. The function will return when the HW has finished
   * encoding next slice. Upon return the slicesReady pointer will contain
   * the number of slices that are ready and available in the HW output buffer.
   *
   * \return EWL_HW_WAIT_OK when hardware can work normally even if error is detected
   * \return EWL_HW_WAIT_ERROR error is detected by software
   * \return EWL_HW_WAIT_TIMEOUT hardware timeout happens.
   */
i32 EWLWaitHwRdy(const void *instance, u32 *slicesReady, void *waitOut, u32 *status_register);

/** SW/SW shared memory handling */
void *EWLmalloc(u32 n);
void *EWLcalloc(u32 n, u32 s);
void EWLfree(void *p);
mem_ret EWLmemcpy(void *d, const void *s, u32 n);
mem_ret EWLmemset(void *d, i32 c, u32 n);
int EWLmemcmp(const void *s1, const void *s2, u32 n);

/** Get the address/size of on-chip sram used for input line buffer. */
i32 EWLGetLineBufSram(const void *instance, EWLLinearMem_t *info);

/** allocate loopback line buffer in memory, mainly used when there is no on-chip sram */
i32 EWLMallocLoopbackLineBuf(const void *instance, u32 size, EWLLinearMem_t *info);

/**
   * Get the PSNR/SSIM result, valid only in the c-model.
   */
void EWLTraceProfile(const void *inst, void *prof_data, i32 qp, i32 poc);

void EWLCollectWriteRegData(const void *inst, u32 *src, u32 *dst, u16 reg_start, u32 reg_length,
                            u32 *total_length);
void EWLCollectStallDataEncVideo(const void *inst, u32 *dst, u32 *total_length);
void EWLCollectStallDataCuTree(const void *inst, u32 *dst, u32 *total_length);
void EWLCollectStallDec400(const void *inst, u32 *dst, u32 *total_length);
void EWLCollectReadRegData(const void *inst, u32 *dst, u16 reg_start, u32 reg_length,
                           u32 *total_length, u16 cmdbuf_id);
void EWLCollectWriteMMURegData(const void *inst, u32 *dst, u32 *total_length);
void EWLCollectReadVcmdRegData(const void *inst, u32 *dst, u16 reg_start, u32 reg_length,
                               u32 *total_length, u16 cmdbuf_id);
void EWLCollectWriteDec400RegData(const void *inst, u32 *src, u32 *dst, u16 reg_start,
                                  u32 reg_length, u32 *total_length);
void EWLCollectClrIntReadClearDec400Data(const void *inst, u32 *dst, u32 *total_length,
                                         u16 addr_offset);
void EWLCollectIntData(const void *inst, u32 *dst, u32 *total_length, u16 cmdbuf_id);
void EWLCollectJmpData(const void *inst, u32 *dst, u32 *total_length, u16 cmdbuf_id);
void EWLCollectClrIntData(const void *inst, u32 *dst, u32 *total_length);
void EWLCollectStopHwData(const void *inst, u32 *dst, u32 *total_length);
void EWLCollectNopData(const void *inst, u32 *dst, u32 *total_length);
u32 EWLReadRegInit(const void *inst, u32 offset);
i32 EWLReserveCmdbuf(const void *inst, u16 size, u16 *cmdbufid);
void EWLCopyDataToCmdbuf(const void *inst, u32 *src, u32 size, u16 cmdbuf_id);
i32 EWLLinkRunCmdbuf(const void *inst, u16 cmdbufid, u16 cmdbuf_size);
i32 EWLWaitCmdbuf(const void *inst, u16 cmdbufid, u32 *status);
void EWLGetRegsByCmdbuf(const void *ewl, u16 cmdbufid, u32 *regMirror);
i32 EWLReleaseCmdbuf(const void *inst, u16 cmdbufid);
void EWLSetReserveBaseData(const void *inst, u32 width, u32 height, u32 rdoLevel, u32 bRDOQEnable,
                           u32 module_type, u16 priority);

void EWLSetVCMDMode(const void *inst, u32 mode);
u32 EWLGetVCMDMode(const void *inst);

u32 EWLGetVCMDSupport();

void EWLAttach(void *ctx, int slice_idx, i32 vcmd_support);
void EWLDetach();

u32 EWLReleaseEwlWorkerInst(const void *inst);
void EWLClearTraceProfile(const void *inst);
/**@}*/

#ifdef __cplusplus
}
#endif
#endif /*__EWL_H__*/
