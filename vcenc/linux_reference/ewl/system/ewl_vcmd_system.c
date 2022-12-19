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
--  Abstract : Encoder Wrapper Layer system model adapter
--
------------------------------------------------------------------------------*/

#include "tools.h"

#include "enc_vcmd.h"

/* Common EWL interface */
#include "ewl.h"
#include "ewl_common.h"
#include "encasiccontroller.h"
#include "vcmdstruct.h"
#include "vwl.h"

#define TRACE_CMDBUF

/* Macro for debug printing */
#define PTRACE(...) VCEncTraceMsg(NULL, VCENC_LOG_INFO, VCENC_LOG_TRACE_EWL, ##__VA_ARGS__)
#define PTRACE_I(inst, ...) VCEncTraceMsg(inst, VCENC_LOG_INFO, VCENC_LOG_TRACE_EWL, ##__VA_ARGS__)
#define PTRACE_E(inst, ...)                                                                        \
    VCEncTraceMsg(inst, VCENC_LOG_ERROR, VCENC_LOG_TRACE_EWL, "[%s:%d]", __func__, __LINE__,       \
                  ##__VA_ARGS__)
#define MEM_LOG_I(inst, ...) VCEncTraceMsg(inst, VCENC_LOG_INFO, VCENC_LOG_TRACE_MEM, ##__VA_ARGS__)

#define ASIC_STATUS_IRQ_INTERVAL 0x100

/* Mask fields */
#define mask_1b (u32)0x00000001
#define mask_2b (u32)0x00000003
#define mask_3b (u32)0x00000007
#define mask_4b (u32)0x0000000F
#define mask_5b (u32)0x0000001F
#define mask_6b (u32)0x0000003F
#define mask_8b (u32)0x000000FF
#define mask_9b (u32)0x000001FF
#define mask_10b (u32)0x000003FF
#define mask_18b (u32)0x0003FFFF

/* Page size and align for linear memory chunks. */
#define LINMEM_ALIGN 4096
#define NEXT_ALIGNED(x) ((((ptr_t)(x)) + LINMEM_ALIGN - 1) / LINMEM_ALIGN * LINMEM_ALIGN)
//#define DEBUG_VCMD

typedef struct {
    struct node *next;
    int core_id;
    int cmdbuf_id;
} EWLWorker;

#define FIRST_CORE(inst) (((EWLWorker *)(inst->workers.tail))->core_id)
#define LAST_CORE(inst) (((EWLWorker *)(inst->workers.head))->core_id)
#define LAST_WAIT_CORE(inst) (((EWLWorker *)(inst->freelist.head))->core_id)
#define FIRST_CMDBUF_ID(inst) (((EWLWorker *)(inst->workers.tail))->cmdbuf_id)
#define LAST_CMDBUF_ID(inst) (((EWLWorker *)(inst->workers.head))->cmdbuf_id)

#define BUS_WIDTH_128_BITS 128
#define BUS_WRITE_U32_CNT_MAX (BUS_WIDTH_128_BITS / (8 * sizeof(u32)))
#define BUS_WRITE_U8_CNT_MAX (BUS_WIDTH_128_BITS / (8 * sizeof(u8)))

#define MAX_CMDBUF_PRIORITY_TYPE 2 //0:normal priority,1:high priority

#define CMDBUF_PRIORITY_NORMAL 0
#define CMDBUF_PRIORITY_HIGH 1

#define OPCODE_WREG (0x01 << 27)
#define OPCODE_END (0x02 << 27)
#define OPCODE_NOP (0x03 << 27)
#define OPCODE_RREG (0x16 << 27)
#define OPCODE_INT (0x18 << 27)
#define OPCODE_JMP (0x19 << 27)
#define OPCODE_STALL (0x09 << 27)
#define OPCODE_CLRINT (0x1a << 27)
#define OPCODE_JMP_RDY0 (0x19 << 27)
#define OPCODE_JMP_RDY1 (0x19 << 27) | (1 << 26)

#define CLRINT_OPTYPE_READ_WRITE_1_CLEAR 0
#define CLRINT_OPTYPE_READ_WRITE_0_CLEAR 1
#define CLRINT_OPTYPE_READ_CLEAR 2

#define VC8000E_FRAME_RDY_INT_MASK 0x0001
#define VC8000E_CUTREE_RDY_INT_MASK 0x0002
#define VC8000E_DEC400_INT_MASK 0x0004
#define VC8000E_L2CACHE_INT_MASK 0x0008
#define VC8000E_MMU_INT_MASK 0x0010

#define VC8000D_FRAME_RDY_INT_MASK 0x0100
#define VC8000D_DEC400_INT_MASK 0x0400
#define VC8000D_L2CACHE_INT_MASK 0x0800
#define VC8000D_MMU_INT_MASK 0x1000

static pthread_mutex_t ewl_mutex = PTHREAD_MUTEX_INITIALIZER;
static volatile int StreamNum = 0;
#ifdef TRACE_CMDBUF
#ifdef TEST_DATA
static void trace_print_top_cmdbuf_dataVcmd(u8 *data, i32 length);
#endif
#endif
static i32 EWLReleaseVcmd(const void *instance);

static void CWLCollectWriteRegDataVcmd(u32 *src, u32 *dst, u16 reg_start, u32 reg_length,
                                       u32 *total_length)
{
    u32 i;
    u32 data_length = 0;

    {
        //opcode
        *dst++ = OPCODE_WREG | (reg_length << 16) | (reg_start * 4);
        data_length++;
        //data
#if 0
    EWLmemcmp(dst, src, reg_length*sizeof(u32));
    data_length += reg_length;
    dst += reg_length;
#else
        for (i = 0; i < reg_length; i++) {
            *dst++ = *src++;
            data_length++;
        }
#endif
        //alignment
        if (data_length % 2) {
            *dst = 0;
            data_length++;
        }
        *total_length = data_length;
    }
}

static void CWLCollectStallDataVcmd(u32 *dst, u32 *total_length, u32 interruput_mask)
{
    u32 data_length = 0;
    {
        //opcode
        *dst++ = OPCODE_STALL | (0 << 16) | (interruput_mask);
        data_length++;
        //alignment
        *dst = 0;
        data_length++;
        *total_length = data_length;
    }
}

static void CWLCollectReadRegDataVcmd(u32 *dst, u16 reg_start, u32 reg_length, u32 *total_length,
                                      ptr_t status_data_base_addr)
{
    u32 data_length = 0;
    //status_data_base_addr += reg_start*4;
    {
        //opcode
        *dst++ = OPCODE_RREG | (reg_length << 16) | (reg_start * 4);
        data_length++;
        //data
        *dst++ = (u32)status_data_base_addr;
        data_length++;
        *dst++ = (u32)(status_data_base_addr >> 32);
        data_length++;

        //alignment

        *dst = 0;
        data_length++;

        *total_length = data_length;
    }
}

static void CWLCollectNopDataVcmd(u32 *dst, u32 *total_length)
{
    u32 data_length = 0;
    {
        //opcode
        *dst++ = OPCODE_NOP | (0);
        data_length++;
        //alignment
        *dst = 0;
        data_length++;
        *total_length = data_length;
    }
}

static void CWLCollectIntDataVcmd(u32 *dst, u16 int_vecter, u32 *total_length)
{
    u32 data_length = 0;
    {
        //opcode
        *dst++ = OPCODE_INT | (int_vecter);
        data_length++;
        //alignment
        *dst = 0;
        data_length++;
        *total_length = data_length;
    }
}

static void CWLCollectJmpDataVcmd(u32 *dst, u32 *total_length, u16 cmdbuf_id)
{
    u32 data_length = 0;
    {
        //opcode
        *dst++ = OPCODE_JMP_RDY0 | (0);
        data_length++;
        //addr
        *dst++ = (0);
        data_length++;
        *dst++ = (0);
        data_length++;
        *dst++ = (cmdbuf_id);
        data_length++;
        //alignment, do not do this step for driver will use this data to identify JMP and End opcode.
        *total_length = data_length;
    }
}

static void CWLCollectClrIntDataVcmd(u32 *dst, u32 clear_type, u16 interrupt_reg_addr, u32 bitmask,
                                     u32 *total_length)
{
    u32 data_length = 0;
    {
        //opcode
        *dst++ = OPCODE_CLRINT | (clear_type << 25) | interrupt_reg_addr * 4;
        data_length++;
        //bitmask
        *dst = bitmask;
        data_length++;
        *total_length = data_length;
    }
}

static i32 EWLGetLineBufSramVcmd(const void *instance, EWLLinearMem_t *info)
{
    assert(instance != NULL);
    assert(info != NULL);

    info->virtualAddress = NULL;
    info->busAddress = 0;
    info->size = 0;

    return EWL_OK;
}

/*------------------------------------------------------------------------------
    Function name   : EWLMallocLoopbackLineBuf
    Description        : allocate loopback line buffer in memory, mainly used when there is no on-chip sram

    Return type     : i32 - 0 for success or a negative error code

    Argument        : const void * instance - EWL instance
    Argument        : EWLLinearMem_t *info - place where the mem parameters are returned
------------------------------------------------------------------------------*/
static i32 EWLMallocLoopbackLineBufVcmd(const void *instance, u32 size, EWLLinearMem_t *info)
{
    assert(instance != NULL);
    assert(info != NULL);

    info->virtualAddress = NULL;
    info->busAddress = 0;
    info->size = 0;

    return EWL_OK;
}

static u32 EWLGetClientTypeVcmd(const void *inst)
{
    ewlSysInstance *enc = (ewlSysInstance *)inst;
    u32 client_type = enc->clientType;

    return client_type;
}

static u32 EWLGetCoreTypeByClientTypeVcmd(u32 client_type)
{
    return 0;
}

static u32 EWLChangeClientTypeVcmd(const void *inst, u32 client_type)
{
    return 0;
}

static i32 EWLCheckCutreeValidVcmd(const void *inst)
{
    return EWL_OK;
}

static void EwlReleaseCoreWaitVcmd(void *inst)
{
    return;
}

/*******************************************************************************
 Function name   : EWLReadAsicID
 Description     : Read ASIC ID register, static implementation
 Return type     : u32 ID
 Argument        : enum vcmd_module_type module_type)
*******************************************************************************/
static u32 EWLReadAsicIDVcmd(u32 client_type, void *ctx)
{
    u32 *pRegs = FAILED_ADDRESS;
    u32 id = ~0;
    int ret;
    struct config_parameter vcmd_core_info;
    struct cmdbuf_mem_parameter vcmd_cmdbuf_info;
    u32 module_type = 0;
    vcmd_cmdbuf_info.virt_status_cmdbuf_addr = FAILED_ADDRESS;
    ewlSysInstance inst;
    int CmodelVcmdInitFlag = 0;

    (void)inst;
    switch (client_type) {
    case EWL_CLIENT_TYPE_H264_ENC:
    case EWL_CLIENT_TYPE_HEVC_ENC:
    case EWL_CLIENT_TYPE_AV1_ENC:
        module_type = VCMD_TYPE_ENCODER;
        break;
    case EWL_CLIENT_TYPE_CUTREE:
        module_type = VCMD_TYPE_CUTREE;
        break;
    case EWL_CLIENT_TYPE_JPEG_ENC:
        module_type = VCMD_TYPE_JPEG_ENCODER;
        break;
    default:
        break;
    }
    if (module_type > MAX_VCMD_TYPE - 1) {
        PTRACE_E((void *)ctx, "EWLReadAsicID: The module_type is wrong!\n");
        return -1;
    }

    if (CmodelVcmdCheck() == 0) {
        ret = CmodelVcmdInit();
        if (ret == -1) {
            return -1;
        }
        CmodelVcmdInitFlag = 1;
    }

    if (CmodelIoctlGetCmdbufParameter(&vcmd_cmdbuf_info) == -1) {
        PTRACE_E((void *)ctx, "ioctl HANTRO_VCMD_IOCH_GET_CMDBUF_PARAMETER failed\n");
        if (CmodelVcmdInitFlag == 1) {
            CmodelVcmdRelease();
        }
        return -1;
    }

    vcmd_core_info.module_type = module_type;
    if (CmodelIoctlGetVcmdParameter(&vcmd_core_info) == -1) {
        PTRACE_E((void *)ctx, "ioctl HANTRO_VCMD_IOCH_GET_VCMD_PARAMETER failed\n");

        ASSERT(0);
    }
    if (vcmd_core_info.vcmd_core_num == 0) {
        if (module_type == VCMD_TYPE_JPEG_ENCODER) {
            vcmd_core_info.module_type = VCMD_TYPE_ENCODER;
            if (CmodelIoctlGetVcmdParameter(&vcmd_core_info) == -1) {
                PTRACE_E((void *)ctx, "ioctl HANTRO_VCMD_IOCH_GET_VCMD_PARAMETER failed\n");
                ASSERT(0);
            }
        }

        if (vcmd_core_info.vcmd_core_num == 0) {
            PTRACE_E((void *)ctx, "there is no proper vcmd  for encoder \n");
            if (CmodelVcmdInitFlag == 1) {
                CmodelVcmdRelease();
            }
            return id;
        }
    }

    pRegs = vcmd_cmdbuf_info.virt_status_cmdbuf_addr +
            vcmd_core_info.config_status_cmdbuf_id * vcmd_cmdbuf_info.cmdbuf_unit_size / 4 +
            vcmd_core_info.submodule_main_addr / 4;
    id = pRegs[0];

    PTRACE_I((void *)ctx, "EWLReadAsicID: 0x%08x\n", id);

    return id;
}

/*******************************************************************************
 Function name   : EWLReadAsicConfig
 Description     : Reads ASIC capability register, static implementation
 Return type     : EWLHwConfig_t
 Argument        : void
*******************************************************************************/
static EWLHwConfig_t EWLReadAsicConfigVcmd(u32 client_type, void *ctx)
{
    u32 *pRegs = FAILED_ADDRESS;
    EWLHwConfig_t cfg_info;
    struct config_parameter vcmd_core_info;
    struct cmdbuf_mem_parameter vcmd_cmdbuf_info;
    vcmd_cmdbuf_info.virt_status_cmdbuf_addr = FAILED_ADDRESS;
    u32 module_type = 0;
    //ewlSysInstance inst;
    int ret;
    int CmodelVcmdInitFlag = 0;

    switch (client_type) {
    case EWL_CLIENT_TYPE_H264_ENC:
    case EWL_CLIENT_TYPE_HEVC_ENC:
    case EWL_CLIENT_TYPE_AV1_ENC:
        module_type = VCMD_TYPE_ENCODER;
        break;
    case EWL_CLIENT_TYPE_CUTREE:
        module_type = VCMD_TYPE_CUTREE;
        break;
    case EWL_CLIENT_TYPE_JPEG_ENC:
        module_type = VCMD_TYPE_JPEG_ENCODER;
        break;
    default:
        break;
    }

    memset(&cfg_info, 0, sizeof(cfg_info));

    if (module_type > MAX_VCMD_TYPE - 1)
        return cfg_info;
    if (CmodelVcmdCheck() == 0) {
        ret = CmodelVcmdInit();
        if (ret == -1) {
            return cfg_info;
        }
        CmodelVcmdInitFlag = 1;
    }
    if (CmodelIoctlGetCmdbufParameter(&vcmd_cmdbuf_info) == -1) {
        PTRACE_E((void *)ctx, "ioctl HANTRO_VCMD_IOCH_GET_CMDBUF_PARAMETER failed\n");
        if (CmodelVcmdInitFlag == 1) {
            CmodelVcmdRelease();
        }
        return cfg_info;
    }

    vcmd_core_info.module_type = module_type;
    if (CmodelIoctlGetVcmdParameter(&vcmd_core_info) == -1) {
        PTRACE_E((void *)ctx, "ioctl HANTRO_VCMD_IOCH_GET_VCMD_PARAMETER failed\n");
        ASSERT(0);
    }

    if (vcmd_core_info.vcmd_core_num == 0) {
        if (module_type == VCMD_TYPE_JPEG_ENCODER) {
            vcmd_core_info.module_type = VCMD_TYPE_ENCODER;
            if (CmodelIoctlGetVcmdParameter(&vcmd_core_info) == -1) {
                PTRACE_E((void *)ctx, "ioctl HANTRO_VCMD_IOCH_GET_VCMD_PARAMETER failed\n");
                ASSERT(0);
            }
        }
        if (vcmd_core_info.vcmd_core_num == 0) {
            PTRACE_E((void *)ctx, "there is no proper vcmd  for encoder \n");
            if (CmodelVcmdInitFlag == 1) {
                CmodelVcmdRelease();
            }
            return cfg_info;
        }
    }

    pRegs = vcmd_cmdbuf_info.virt_status_cmdbuf_addr +
            vcmd_core_info.config_status_cmdbuf_id * vcmd_cmdbuf_info.cmdbuf_unit_size / 4 +
            vcmd_core_info.submodule_main_addr / 4;

    EWLCoreSignature_t signature;
    if (EWL_OK != EWLGetCoreSignature(pRegs, &signature)) {
        PTRACE("ERROR when get the core signature.");
    }

    if (EWL_OK != EWLGetCoreConfig(&signature, &cfg_info)) {
        PTRACE("ERROR when get the core feature list.");
    }

    return cfg_info;
}

/*******************************************************************************
 Function name   : EWLInit
 Description     : Allocate resources and setup the wrapper module
 Return type     : ewl_ret
 Argument        : void
*******************************************************************************/
static const void *EWLInitVcmd(EWLInitParam_t *param)
{
    ewlSysInstance *inst;
    int ret, i;

    u32 *baseMem = NULL;
    u32 *baseMem_IM = NULL;
    u32 *baseMem_JPEG = NULL;
    u16 module_type = VCMD_TYPE_ENCODER;
    PTRACE_I(NULL, "EWLInit: Start\n");

    /* Check for NULL pointer */
    if (param == NULL || param->clientType >= EWL_CLIENT_TYPE_MAX) {
        PTRACE_E(NULL, "EWLInit: Bad calling parameters!\n");
        return NULL;
    }

    switch (param->clientType) {
    case EWL_CLIENT_TYPE_H264_ENC:
    case EWL_CLIENT_TYPE_HEVC_ENC:
    case EWL_CLIENT_TYPE_AV1_ENC:
        module_type = VCMD_TYPE_ENCODER;
        break;
    case EWL_CLIENT_TYPE_CUTREE:
        module_type = VCMD_TYPE_CUTREE;
        break;
    case EWL_CLIENT_TYPE_JPEG_ENC:
        module_type = VCMD_TYPE_JPEG_ENCODER;
        break;
    default:
        break;
    }

    inst = (ewlSysInstance *)malloc(sizeof(ewlSysInstance));
    if (inst == NULL)
        return NULL;
    memset(inst, 0, sizeof(ewlSysInstance));
    if (module_type != VCMD_TYPE_CUTREE)
        StreamNum++;
    inst->clientType = param->clientType;
    inst->linMemChunks = 0;
    inst->refFrmChunks = 0;
    inst->totalChunks = 0;
    inst->streamTempBuffer = NULL;
    inst->vcmd_cmdbuf_info.virt_cmdbuf_addr = FAILED_ADDRESS;
    inst->vcmd_cmdbuf_info.virt_status_cmdbuf_addr = FAILED_ADDRESS;

    inst->prof.frame_number = 0;
    inst->prof.total_bits = 0;
    inst->prof.total_Y_PSNR = 0.0;
    inst->prof.total_U_PSNR = 0.0;
    inst->prof.total_V_PSNR = 0.0;
    inst->prof.Y_PSNR_Max = -1;
    inst->prof.Y_PSNR_Min = 1000.0;
    inst->prof.U_PSNR_Max = -1;
    inst->prof.U_PSNR_Min = 1000.0;
    inst->prof.V_PSNR_Max = -1;
    inst->prof.V_PSNR_Min = 1000.0;
    inst->prof.total_ssim = 0.0;
    inst->prof.total_ssim_y = 0.0;
    inst->prof.total_ssim_u = 0.0;
    inst->prof.total_ssim_v = 0.0;
    inst->prof.QP_Max = 0;
    inst->prof.QP_Min = 52;
    inst->jpeg_shared = 0;

    if (CmodelVcmdCheck() == 0) {
        ret = CmodelVcmdInit();
        if (ret == -1) {
            PTRACE_E(NULL, "CmodelVcmdInit() failed\n");
            goto err;
        }
    }

    if (CmodelIoctlGetCmdbufParameter(&inst->vcmd_cmdbuf_info) == -1) {
        PTRACE_E(NULL, "ioctl HANTRO_VCMD_IOCH_GET_CMDBUF_PARAMETER failed\n");
        ASSERT(0);
    }

    inst->vcmd_enc_core_info.module_type = module_type;
    if (CmodelIoctlGetVcmdParameter(&inst->vcmd_enc_core_info) == -1) {
        PTRACE_E(NULL, "ioctl HANTRO_VCMD_IOCH_GET_VCMD_PARAMETER failed\n");
        ASSERT(0);
    }

    if (inst->vcmd_enc_core_info.vcmd_core_num == 0) {
        if (module_type == VCMD_TYPE_JPEG_ENCODER) {
            inst->vcmd_enc_core_info.module_type = VCMD_TYPE_ENCODER;
            if (CmodelIoctlGetVcmdParameter(&inst->vcmd_enc_core_info) == -1) {
                PTRACE_E(NULL, "ioctl HANTRO_VCMD_IOCH_GET_VCMD_PARAMETER failed\n");
                ASSERT(0);
            }
        }
        if (inst->vcmd_enc_core_info.vcmd_core_num == 0) {
            PTRACE_E(NULL, "there is no proper vcmd  for encoder \n");
            goto err;
        }
        inst->jpeg_shared = 1;
    }

    if (inst->vcmd_cmdbuf_info.virt_cmdbuf_addr == FAILED_ADDRESS) {
        PTRACE_E(NULL, "EWLInit: vcmd_cmdbuf_info.virt_cmdbuf_addr is wrong: %p\n",
                 (void *)inst->vcmd_cmdbuf_info.virt_cmdbuf_addr);
        goto err;
    }

    if (inst->vcmd_cmdbuf_info.virt_status_cmdbuf_addr == FAILED_ADDRESS) {
        PTRACE_E(NULL, "EWLInit: vcmd_cmdbuf_info.virt_status_cmdbuf_addr is wrong: %p\n",
                 (void *)inst->vcmd_cmdbuf_info.virt_status_cmdbuf_addr);
        goto err;
    }

    queue_init(&inst->workers);
    for (i = 0; i < MAX_VCMD_TYPE; i++) {
        inst->main_module_init_status_addr[i] = NULL;
    }
    if (module_type == VCMD_TYPE_ENCODER) {
        inst->main_module_init_status_addr[VCMD_TYPE_ENCODER] =
            inst->vcmd_cmdbuf_info.virt_status_cmdbuf_addr +
            inst->vcmd_cmdbuf_info.status_cmdbuf_unit_size / 4 *
                inst->vcmd_enc_core_info.config_status_cmdbuf_id;
        inst->main_module_init_status_addr[VCMD_TYPE_ENCODER] +=
            inst->vcmd_enc_core_info.submodule_main_addr / 2 / 4;

        baseMem = inst->main_module_init_status_addr[VCMD_TYPE_ENCODER];
        for (i = 0; i < ASIC_SWREG_AMOUNT; i++) {
            baseMem[i] = *(inst->vcmd_cmdbuf_info.core_register_addr);
            inst->vcmd_cmdbuf_info.core_register_addr++;
        }
    } else if (module_type == VCMD_TYPE_CUTREE) {
        inst->main_module_init_status_addr[VCMD_TYPE_CUTREE] =
            inst->vcmd_cmdbuf_info.virt_status_cmdbuf_addr +
            inst->vcmd_cmdbuf_info.status_cmdbuf_unit_size / 4 *
                inst->vcmd_enc_core_info.config_status_cmdbuf_id;
        inst->main_module_init_status_addr[VCMD_TYPE_CUTREE] +=
            inst->vcmd_enc_core_info.submodule_main_addr / 2 / 4;

        baseMem_IM = inst->main_module_init_status_addr[VCMD_TYPE_CUTREE];
        for (i = 0; i < ASIC_SWREG_AMOUNT; i++) {
            baseMem_IM[i] = *(inst->vcmd_cmdbuf_info.core_register_addr_IM);
            inst->vcmd_cmdbuf_info.core_register_addr_IM++;
        }
    }

    else if (module_type == VCMD_TYPE_JPEG_ENCODER) {
        inst->main_module_init_status_addr[VCMD_TYPE_JPEG_ENCODER] =
            inst->vcmd_cmdbuf_info.virt_status_cmdbuf_addr +
            inst->vcmd_cmdbuf_info.status_cmdbuf_unit_size / 4 *
                inst->vcmd_enc_core_info.config_status_cmdbuf_id;
        inst->main_module_init_status_addr[VCMD_TYPE_JPEG_ENCODER] +=
            inst->vcmd_enc_core_info.submodule_main_addr / 2 / 4;

        baseMem_JPEG = inst->main_module_init_status_addr[VCMD_TYPE_JPEG_ENCODER];
        for (i = 0; i < ASIC_SWREG_AMOUNT; i++) {
            baseMem_JPEG[i] = *(inst->vcmd_cmdbuf_info.core_register_addr_JPEG);
            inst->vcmd_cmdbuf_info.core_register_addr_JPEG++;
        }
    }

    PTRACE_I(NULL, "EWLInit: Return 0x%p\n", inst);
    return inst;

err:
    EWLReleaseVcmd(inst);
    PTRACE_E(NULL, "EWLInit: Return NULL\n");
    return NULL;
}

/*******************************************************************************
 Function name   : EWLRelease
 Description     : Release the wrapper module by freeing all the resources
 Return type     : ewl_ret
 Argument        : void
*******************************************************************************/
static i32 EWLReleaseVcmd(const void *instance)
{
    u32 i, tothw, totswhw;
    ewlSysInstance *inst = (ewlSysInstance *)instance;

    if (inst->streamTempBuffer)
        free(inst->streamTempBuffer);

    //Release vcmd cmodel
    if (inst->vcmd_enc_core_info.module_type != VCMD_TYPE_CUTREE && CmodelVcmdCheck() == 1) {
        if (StreamNum == 1)
            CmodelVcmdRelease();
        else if (StreamNum > 1) {
            StreamNum--;
            return 0;
        }
    }

    for (i = 0, tothw = 0; i < inst->refFrmChunks; i++) {
        tothw += inst->refFrmAlloc[i];
        MEM_LOG_I((void *)inst, "\n      HW Memory:   %u\n", inst->refFrmAlloc[i]);
    }
    printf("\nTotal HW Memory:   %u", tothw);
    for (i = 0, totswhw = 0; i < inst->linMemChunks; i++) {
        totswhw += inst->linMemAlloc[i];
        MEM_LOG_I((void *)inst, "\n      SWHW Memory: %u\n", inst->linMemAlloc[i]);
    }
    printf("\nTotal SWHW Memory: %u\n", totswhw);

    free_nodes(inst->workers.tail);
    free_nodes(inst->freelist.tail);
    free(inst);
    CoreEncShutdown();
    return EWL_OK;
}

/*******************************************************************************
 Function name   : EWLWriteReg
 Description     : Set the content of a hadware register
 Return type     : void
 Argument        : u32 offset
 Argument        : u32 val
*******************************************************************************/
static void EWLWriteCoreRegVcmd(const void *instance, u32 offset, u32 val, u32 core_id)
{
    ewlSysInstance *inst = (ewlSysInstance *)instance;
    (void)inst;
    PTRACE_I((void *)instance, "EWLWriteReg 0x%02x with value %08x\n", offset * 4, val);
}

static void EWLWriteRegVcmd(const void *instance, u32 offset, u32 val)
{
    ewlSysInstance *inst = (ewlSysInstance *)instance;
    EWLWriteCoreRegVcmd(inst, offset, val, 0);
}

static void EWLSetReserveBaseDataVcmd(const void *inst, u32 width, u32 height, u32 rdoLevel,
                                      u32 bRDOQEnable, u32 client_type, u16 priority)
{
    ewlSysInstance *enc = (ewlSysInstance *)inst;
    enc->reserve_cmdbuf_info.priority = priority;
    enc->reserve_cmdbuf_info.executing_time = width * height * (rdoLevel + 1) * (bRDOQEnable + 1);
    switch (client_type) {
    case EWL_CLIENT_TYPE_H264_ENC:
        enc->reserve_cmdbuf_info.module_type = VCMD_TYPE_ENCODER;
        break;
    case EWL_CLIENT_TYPE_HEVC_ENC:
        enc->reserve_cmdbuf_info.module_type = VCMD_TYPE_ENCODER;
        break;
    case EWL_CLIENT_TYPE_AV1_ENC:
        enc->reserve_cmdbuf_info.module_type = VCMD_TYPE_ENCODER;
        break;
    case EWL_CLIENT_TYPE_CUTREE:
        enc->reserve_cmdbuf_info.module_type = VCMD_TYPE_CUTREE;
        break;
    case EWL_CLIENT_TYPE_JPEG_ENC:
        if (enc->jpeg_shared == 1)
            enc->reserve_cmdbuf_info.module_type = VCMD_TYPE_ENCODER;
        else
            enc->reserve_cmdbuf_info.module_type = VCMD_TYPE_JPEG_ENCODER;
        break;
    default:
        break;
    }
}

//these functions are for vc8000e
static void EWLCollectWriteRegDataVcmd(const void *inst, u32 *src, u32 *dst, u16 reg_start,
                                       u32 reg_length, u32 *total_length)
{
    ewlSysInstance *enc = (ewlSysInstance *)inst;
    u16 reg_base = enc->vcmd_enc_core_info.submodule_main_addr / 4;
    CWLCollectWriteRegDataVcmd(src, dst, reg_base + reg_start, reg_length, total_length);
    return;
}

static void EWLCollectNopDataVcmd(const void *inst, u32 *dst, u32 *total_length)
{
    ewlSysInstance *enc = (ewlSysInstance *)inst;
    (void)enc;
    CWLCollectNopDataVcmd(dst, total_length);
    return;
}

static void EWLCollectStallDataEncVideoVcmd(const void *inst, u32 *dst, u32 *total_length)
{
    ewlSysInstance *enc = (ewlSysInstance *)inst;
    u32 interruput_mask = VC8000E_FRAME_RDY_INT_MASK;
    (void)enc;
    CWLCollectStallDataVcmd(dst, total_length, interruput_mask);
    return;
}
static void EWLCollectStallDataCuTreeVcmd(const void *inst, u32 *dst, u32 *total_length)
{
    ewlSysInstance *enc = (ewlSysInstance *)inst;
    u32 interruput_mask = VC8000E_CUTREE_RDY_INT_MASK;
    (void)enc;
    CWLCollectStallDataVcmd(dst, total_length, interruput_mask);
    return;
}

static void EWLCollectReadRegDataVcmd(const void *inst, u32 *dst, u16 reg_start, u32 reg_length,
                                      u32 *total_length, u16 cmdbuf_id)
{
    ewlSysInstance *enc = (ewlSysInstance *)inst;
    u16 reg_base = enc->vcmd_enc_core_info.submodule_main_addr / 4;
    ptr_t status_data_base_addr;
    status_data_base_addr = enc->vcmd_cmdbuf_info.phy_status_cmdbuf_addr -
                            enc->vcmd_cmdbuf_info.base_ddr_addr +
                            enc->vcmd_cmdbuf_info.status_cmdbuf_unit_size * cmdbuf_id;
    status_data_base_addr += enc->vcmd_enc_core_info.submodule_main_addr / 2;
    CWLCollectReadRegDataVcmd(dst, reg_base + reg_start, reg_length, total_length,
                              status_data_base_addr + reg_start * 4);
    return;
}

static void EWLCollectIntDataVcmd(const void *inst, u32 *dst, u32 *total_length, u16 cmdbuf_id)
{
    ewlSysInstance *enc = (ewlSysInstance *)inst;
    (void)enc;
    CWLCollectIntDataVcmd(dst, cmdbuf_id, total_length);
    return;
}

static void EWLCollectJmpDataVcmd(const void *inst, u32 *dst, u32 *total_length, u16 cmdbuf_id)
{
    ewlSysInstance *enc = (ewlSysInstance *)inst;
    (void)enc;
    CWLCollectJmpDataVcmd(dst, total_length, cmdbuf_id);
    return;
}

static void EWLCollectClrIntDataVcmd(const void *inst, u32 *dst, u32 *total_length)
{
    ewlSysInstance *enc = (ewlSysInstance *)inst;
    u16 clear_type = CLRINT_OPTYPE_READ_WRITE_1_CLEAR;
    u16 reg_base = enc->vcmd_enc_core_info.submodule_main_addr / 4;
    u32 bitmask = 0xffff;
    CWLCollectClrIntDataVcmd(dst, clear_type, reg_base + 1, bitmask, total_length);
    return;
}

static void EWLCollectClrIntReadClearDec400DataVcmd(const void *inst, u32 *dst, u32 *total_length,
                                                    u16 addr_offset)
{
    ewlSysInstance *enc = (ewlSysInstance *)inst;
    u16 clear_type = CLRINT_OPTYPE_READ_CLEAR;
    u16 reg_base = enc->vcmd_enc_core_info.submodule_dec400_addr / 4;
    u32 bitmask = 0xffff;
    CWLCollectClrIntDataVcmd(dst, clear_type, reg_base + addr_offset, bitmask, total_length);
    return;
}

static void EWLCollectStopHwDataVcmd(const void *inst, u32 *dst, u32 *total_length)
{
    ewlSysInstance *enc = (ewlSysInstance *)inst;
    u16 clear_type = CLRINT_OPTYPE_READ_WRITE_1_CLEAR;
    u16 reg_base = enc->vcmd_enc_core_info.submodule_main_addr / 4;
    u32 bitmask = 0xfffe;
    CWLCollectClrIntDataVcmd(dst, clear_type, reg_base + ASIC_REG_INDEX_STATUS, bitmask,
                             total_length);
    return;
}

// this function is for vcmd.
static void EWLCollectReadVcmdRegDataVcmd(const void *inst, u32 *dst, u16 reg_start, u32 reg_length,
                                          u32 *total_length, u16 cmdbuf_id)
{
    ewlSysInstance *enc = (ewlSysInstance *)inst;
    u16 reg_base = 0;
    ptr_t status_data_base_addr = 0; //will be modified in driver.
    if (enc->vcmd_enc_core_info.vcmd_hw_version_id > VCMD_HW_ID_1_0_C)
        CWLCollectReadRegDataVcmd(dst, reg_base + reg_start, reg_length, total_length,
                                  status_data_base_addr + reg_start * 4);
    else {
        *total_length = 0;
    }
    return;
}

//this function is for dec400
static void EWLCollectWriteDec400RegDataVcmd(const void *inst, u32 *src, u32 *dst, u16 reg_start,
                                             u32 reg_length, u32 *total_length)
{
    ewlSysInstance *enc = (ewlSysInstance *)inst;
    u16 reg_base = enc->vcmd_enc_core_info.submodule_dec400_addr / 4;
    CWLCollectWriteRegDataVcmd(src, dst, reg_base + reg_start, reg_length, total_length);
    return;
}

//this function is for dec400
static void EWLCollectStallDec400Vcmd(const void *inst, u32 *dst, u32 *total_length)
{
    ewlSysInstance *enc = (ewlSysInstance *)inst;
    u32 interruput_mask = VC8000E_DEC400_INT_MASK;
    (void)enc;
    CWLCollectStallDataVcmd(dst, total_length, interruput_mask);
    return;
}

static void EWLWriteBackRegVcmd(const void *inst, u32 offset, u32 val)
{
    ewlSysInstance *enc = (ewlSysInstance *)inst;
    (void)enc;
    EWLWriteCoreRegVcmd(inst, offset, val, 0);
}

/*------------------------------------------------------------------------------
    Function name   : EWLEnableHW
    Description     :
    Return type     : void
    Argument        : const void *inst
    Argument        : u32 offset
    Argument        : u32 val
------------------------------------------------------------------------------*/
static void EWLEnableHWVcmd(const void *inst, u32 offset, u32 val)
{
    ewlSysInstance *enc = (ewlSysInstance *)inst;
    (void)enc;
    PTRACE_I((void *)inst, "EWLEnableHW 0x%02x with value %08x\n", offset * 4, val);
}

/*------------------------------------------------------------------------------
    Function name   : EWLDisableHW
    Description     :
    Return type     : void
    Argument        : const void *inst
    Argument        : u32 offset
    Argument        : u32 val
------------------------------------------------------------------------------*/
static void EWLDisableHWVcmd(const void *inst, u32 offset, u32 val)
{
    ewlSysInstance *enc = (ewlSysInstance *)inst;
    (void)enc;
    PTRACE_I((void *)inst, "EWLDisableHW 0x%02x with value %08x\n", offset * 4, val);
}

/*------------------------------------------------------------------------------
    Function name   : EWLGetPerformance
    Description     :
    Return type     : void
    Argument        : const void *inst
    Argument        : u32 offset
    Argument        : u32 val
------------------------------------------------------------------------------*/
static u32 EWLGetPerformanceVcmd(const void *inst)
{
    ewlSysInstance *enc = (ewlSysInstance *)inst;
    return enc->performance;
}

/*******************************************************************************
 Function name   : EWLReadReg
 Description     : Retrive the content of a hadware register
                    Note: The status register will be read after every MB
                    so it may be needed to buffer it's content if reading
                    the HW register is slow.
 Return type     : u32
 Argument        : u32 offset
*******************************************************************************/
static u32 EWLReadRegVcmd(const void *inst, u32 offset)
{
    u32 val;
    ewlSysInstance *enc = (ewlSysInstance *)inst;
    u32 *status_addr;
    u16 cmdbufid = FIRST_CMDBUF_ID(enc);
    status_addr = enc->vcmd_cmdbuf_info.virt_status_cmdbuf_addr +
                  enc->vcmd_cmdbuf_info.status_cmdbuf_unit_size / 4 * cmdbufid;
    status_addr += enc->vcmd_enc_core_info.submodule_main_addr / 2 / 4;

    offset = offset / 4;
    val = *(status_addr + offset);

    PTRACE_I((void *)inst, "EWLReadReg 0x%02x --> %08x\n", offset * 4, val);

    return val;
}

static u32 EWLReadRegInitVcmd(const void *inst, u32 offset)
{
    u32 val;
    ewlSysInstance *enc = (ewlSysInstance *)inst;
    u32 *status_addr = NULL;
    if (enc->main_module_init_status_addr[VCMD_TYPE_ENCODER])
        status_addr = enc->main_module_init_status_addr[VCMD_TYPE_ENCODER];
    else if (enc->main_module_init_status_addr[VCMD_TYPE_CUTREE])
        status_addr = enc->main_module_init_status_addr[VCMD_TYPE_CUTREE];
    else if (enc->main_module_init_status_addr[VCMD_TYPE_JPEG_ENCODER])
        status_addr = enc->main_module_init_status_addr[VCMD_TYPE_JPEG_ENCODER];

    offset = offset / 4;
    val = *(status_addr + offset);

    PTRACE_I((void *)inst, "EWLReadReg 0x%02x --> %08x\n", offset * 4, val);

    return val;
}

/*------------------------------------------------------------------------------
    Function name   : EWLMallocLinear
    Description     : Allocate a contiguous, linear RAM  memory buffer

    Return type     : i32 - 0 for success or a negative error code

    Argument        : const void * instance - EWL instance
    Argument        : u32 size - size in bytes of the requested memory
    Argument        : EWLLinearMem_t *info - place where the allocated
                        memory buffer parameters are returned
------------------------------------------------------------------------------*/
static i32 EWLMallocLinearVcmd(const void *instance, u32 size, u32 alignment, EWLLinearMem_t *info)
{
    ewlSysInstance *inst = (ewlSysInstance *)instance;
    assert(instance != NULL);
    assert(inst->linMemChunks < MEM_CHUNKS);
    assert(inst->refFrmChunks < MEM_CHUNKS);
    assert(inst->totalChunks < MEM_CHUNKS);

    if (((info->mem_type & VPU_RD) || (info->mem_type & VPU_WR)) &&
        ((info->mem_type & CPU_RD) == 0 && (info->mem_type & CPU_WR) == 0))
        inst->refFrmAlloc[inst->refFrmChunks++] = size;
    else
        inst->linMemAlloc[inst->linMemChunks++] = size;
    PTRACE_I((void *)instance, "EWLMallocLinear: %8d bytes (aligned %8ld)\n", size,
             NEXT_ALIGNED(size));

    size = NEXT_ALIGNED(size);

    inst->chunks[inst->totalChunks] = (u32 *)malloc(size + LINMEM_ALIGN);
    if (inst->chunks[inst->totalChunks] == NULL)
        return EWL_ERROR;
    inst->alignedChunks[inst->totalChunks] = (u32 *)NEXT_ALIGNED(inst->chunks[inst->totalChunks]);
    PTRACE_I((void *)instance, "EWLMallocLinear: %p, aligned %p\n",
             (void *)inst->chunks[inst->totalChunks],
             (void *)inst->alignedChunks[inst->totalChunks]);

    info->virtualAddress = inst->alignedChunks[inst->totalChunks++];
    info->busAddress = (ptr_t)info->virtualAddress;
    info->size = info->total_size = size;

    return EWL_OK;
}

/*------------------------------------------------------------------------------
    Function name   : EWLFreeLinear
    Description     : Release a linera memory buffer, previously allocated with
                        EWLMallocLinear.

    Return type     : void

    Argument        : const void * instance - EWL instance
    Argument        : EWLLinearMem_t *info - linear buffer memory information
------------------------------------------------------------------------------*/
static void EWLFreeLinearVcmd(const void *instance, EWLLinearMem_t *info)
{
    ewlSysInstance *inst = (ewlSysInstance *)instance;
    u32 i;
    assert(instance != NULL);
    if (info->virtualAddress == NULL)
        return;

    for (i = 0; i < inst->totalChunks; i++) {
        if (inst->alignedChunks[i] == info->virtualAddress) {
            PTRACE_I((void *)instance, "EWLFreeLinear \t%p\n", (void *)info->virtualAddress);
            free(inst->chunks[i]);
            inst->chunks[i] = NULL;
            inst->alignedChunks[i] = NULL;
            info->virtualAddress = NULL;
            info->busAddress = 0;
            info->total_size = 0;
            info->size = 0;
        }
    }
}

/*------------------------------------------------------------------------------
    Function name   : EWLMallocRefFrm
    Description     : Allocate a frame buffer (contiguous linear RAM memory)

    Return type     : i32 - 0 for success or a negative error code

    Argument        : const void * instance - EWL instance
    Argument        : u32 size - size in bytes of the requested memory
    Argument        : EWLLinearMem_t *info - place where the allocated memory
                        buffer parameters are returned
------------------------------------------------------------------------------*/
static i32 EWLMallocRefFrmVcmd(const void *instance, u32 size, u32 alignment, EWLLinearMem_t *info)
{
    ewlSysInstance *inst = (ewlSysInstance *)instance;
    assert(instance != NULL);
    assert(inst->refFrmChunks < MEM_CHUNKS);
    assert(inst->totalChunks < MEM_CHUNKS);
    i32 ret;

    ret = EWLMallocLinearVcmd(instance, size, alignment, info);

    PTRACE_I((void *)instance, "EWLMallocRefFrm\t%8d bytes\n", size);
    return ret;
}

/*------------------------------------------------------------------------------
    Function name   : EWLFreeRefFrm
    Description     : Release a frame buffer previously allocated with
                        EWLMallocRefFrm.

    Return type     : void

    Argument        : const void * instance - EWL instance
    Argument        : EWLLinearMem_t *info - frame buffer memory information
------------------------------------------------------------------------------*/
static void EWLFreeRefFrmVcmd(const void *instance, EWLLinearMem_t *info)
{
    ewlSysInstance *inst = (ewlSysInstance *)instance;
    u32 i;
    assert(instance != NULL);
    if (NULL == info->virtualAddress)
        return;

    for (i = 0; i < inst->totalChunks; i++) {
        if (inst->alignedChunks[i] == info->virtualAddress) {
            PTRACE_I((void *)instance, "EWLFreeRefFrm\t%p\n", (void *)info->virtualAddress);
            free(inst->chunks[i]);
            inst->chunks[i] = NULL;
            inst->alignedChunks[i] = NULL;
            info->virtualAddress = NULL;
            info->busAddress = 0;
            info->total_size = 0;
            info->size = 0;
            return;
        }
    }
}

i32 EWLSyncMemDataVcmd(EWLLinearMem_t *mem, u32 offset, u32 length, enum EWLMemSyncDirection dir)
{
    (void)mem;
    (void)offset;
    (void)length;
    (void)dir;
    return 0;
}

i32 EWLMemSyncAllocHostBufferVcmd(const void *instance, u32 size, u32 alignment,
                                  EWLLinearMem_t *buff)
{
    (void)size;
    (void)alignment;
    (void)buff;
    return 0;
}

i32 EWLMemSyncFreeHostBufferVcmd(const void *instance, EWLLinearMem_t *buff)
{
    (void)buff;
    return 0;
}

static i32 EWLGetDec400CoreidVcmd(const void *inst)
{
    return 0;
}
static u32 EWLGetDec400CustomerIDVcmd(const void *inst, u32 core_id)
{
    return 0x00005611;
}

static void EWLGetDec400AttributeVcmd(const void *inst, u32 *tile_size, u32 *bits_tile_in_table,
                                      u32 *planar420_cbcr_arrangement_style)
{
    *tile_size = 256;
    *bits_tile_in_table = 4;
    *planar420_cbcr_arrangement_style = 0;
}

/*******************************************************************************
 Function name   : EWLReserveHw
 Description     : Reserve HW resource for currently running codec
*******************************************************************************/
static i32 EWLReserveCmdbufVcmd(const void *inst, u16 size, u16 *cmdbufid)
{
    ewlSysInstance *ewl = (ewlSysInstance *)inst;
    i32 ret;
    struct exchange_parameter *exchange_data = &ewl->reserve_cmdbuf_info;
    exchange_data->cmdbuf_size = size * 4;
    /* Check invalid parameters */
    if (ewl == NULL)
        return EWL_ERROR;
    PTRACE_I((void *)inst, "EWLReserveCmdbufHw: PID %d trying to reserve ...\n", GETPID());

    ret = CmodelIoctlReserveCmdbuf(exchange_data);
    if (ret < 0) {
        PTRACE_I((void *)inst, "EWLReserveCmdbuf failed\n");
        return EWL_ERROR;
    } else {
        EWLWorker *worker = malloc(sizeof(EWLWorker));
        worker->cmdbuf_id = exchange_data->cmdbuf_id;
        worker->next = NULL;
        worker->core_id = exchange_data->core_id;
        pthread_mutex_lock(&ewl_mutex);
        queue_put(&ewl->workers, (struct node *)worker);
        pthread_mutex_unlock(&ewl_mutex);
        *cmdbufid = exchange_data->cmdbuf_id;
        PTRACE_I((void *)inst, "EWLReserveCmdbuf successed\n");
    }
    ewl->cmdbuf_data[*cmdbufid].cmdbuf_size = size;
    ewl->cmdbuf_data[*cmdbufid].module_type = exchange_data->module_type;
    ewl->cmdbuf_data[*cmdbufid].core_id = exchange_data->core_id;
    ewl->cmdbuf_data[*cmdbufid].cmdbuf_virtualAddress =
        ewl->vcmd_cmdbuf_info.virt_cmdbuf_addr +
        ewl->vcmd_cmdbuf_info.cmdbuf_unit_size / 4 * (*cmdbufid);
    ewl->cmdbuf_data[*cmdbufid].status_virtualAddress =
        ewl->vcmd_cmdbuf_info.virt_status_cmdbuf_addr +
        ewl->vcmd_cmdbuf_info.status_cmdbuf_unit_size / 4 * (*cmdbufid);
    PTRACE_I((void *)inst, "EWLReserveCmdbuf: ENC cmdbuf locked by PID %d\n", GETPID());

    return EWL_OK;
}

static void EWLCopyDataToCmdbufVcmd(const void *inst, u32 *src, u32 size, u16 cmdbuf_id)
{
    ewlSysInstance *ewl = (ewlSysInstance *)inst;
    EWLmemcpy(ewl->cmdbuf_data[cmdbuf_id].cmdbuf_virtualAddress, src, size * 4);
#ifdef TRACE_CMDBUF
#ifdef TEST_DATA

    trace_print_top_cmdbuf_dataVcmd((u8 *)ewl->cmdbuf_data[cmdbuf_id].cmdbuf_virtualAddress,
                                    size * 4);

#endif
#endif
}

/*******************************************************************************
 Function name   : EWLLinkRunCmdbuf
 Description     : link and run current cmdbuf
*******************************************************************************/
static i32 EWLLinkRunCmdbufVcmd(const void *inst, u16 cmdbufid, u16 cmdbuf_size)
{
    ewlSysInstance *ewl = (ewlSysInstance *)inst;
    i32 ret;
    struct exchange_parameter *exchange_data = &ewl->reserve_cmdbuf_info;

    /* Check invalid parameters */
    if (ewl == NULL)
        return EWL_ERROR;
    if (cmdbufid != exchange_data->cmdbuf_id)
        return EWL_ERROR;

    PTRACE_I((void *)inst, "EWLLinkRunCmdbuf: PID %d trying to link and  run cmdbuf ...\n",
             GETPID());
    exchange_data->cmdbuf_size = cmdbuf_size * 4;

    ret = CmodelIoctlEnableCmdbuf(exchange_data);

    if (ret < 0) {
        PTRACE_I((void *)inst, "EWLLinkRunCmdbuf failed\n");
        return EWL_ERROR;
    } else {
        PTRACE_I((void *)inst, "EWLLinkRunCmdbuf successed\n");
    }

    PTRACE_I((void *)inst, "EWLLinkRunCmdbuf:  cmdbuf locked by PID %d\n", GETPID());

    return EWL_OK;
}

/*******************************************************************************
 Function name   : EWLWaitCmdbuf
 Description     : wait cmdbuf run done
*******************************************************************************/
static i32 EWLWaitCmdbufVcmd(const void *inst, u16 cmdbufid, u32 *status)
{
    ewlSysInstance *ewl = (ewlSysInstance *)inst;
    i32 ret;
    u32 *ddr_addr = NULL;
    /* Check invalid parameters */
    if (ewl == NULL)
        return EWL_ERROR;
    PTRACE_I((void *)inst, "EWLWaitCmdbuf: PID %d wait cmdbuf ...\n", GETPID());
    ret = CmodelIoctlWaitCmdbuf(&cmdbufid);

    if (ret < 0) {
        PTRACE_I((void *)inst, "EWLWaitCmdbuf failed\n");
        *status = 0;
        return EWL_HW_WAIT_ERROR;
    } else {
        PTRACE_I((void *)inst, "EWLWaitCmdbuf successed\n");
        ddr_addr = ewl->vcmd_cmdbuf_info.virt_status_cmdbuf_addr +
                   ewl->vcmd_cmdbuf_info.status_cmdbuf_unit_size / 4 * cmdbufid;
        ddr_addr += ewl->vcmd_enc_core_info.submodule_main_addr / 2 / 4;
        *status = *(ddr_addr + 1);
    }

    PTRACE_I((void *)inst, "EWLWaitCmdbuf:  cmdbuf locked by PID %d\n", GETPID());

    return EWL_OK;
}

static void EWLGetRegsByCmdbufVcmd(const void *inst, u16 cmdbufid, u32 *regMirror)
{
    ewlSysInstance *ewl = (ewlSysInstance *)inst;
    u32 *ddr_addr = NULL;
    PTRACE_I((void *)inst, "EWLGetRegByCmdbufVcmd: PID %d wait cmdbuf ...\n", GETPID());

    ddr_addr = ewl->vcmd_cmdbuf_info.virt_status_cmdbuf_addr +
               ewl->vcmd_cmdbuf_info.status_cmdbuf_unit_size / 4 * cmdbufid;
    ddr_addr += ewl->vcmd_enc_core_info.submodule_main_addr / 2 / 4;
    EWLmemcpy(regMirror, ddr_addr, ASIC_SWREG_AMOUNT * sizeof(u32));

    PTRACE_I((void *)inst, "EWLGetRegByCmdbufVcmd:  cmdbuf locked by PID %d\n", GETPID());
}

/*******************************************************************************
 Function name   : EWLReleaseCmdbuf
 Description     : wait cmdbuf run done
*******************************************************************************/
static i32 EWLReleaseCmdbufVcmd(const void *inst, u16 cmdbufid)
{
    ewlSysInstance *ewl = (ewlSysInstance *)inst;
    i32 ret;

    /* Check invalid parameters */
    if (ewl == NULL)
        return EWL_ERROR;
    PTRACE_I((void *)inst, "EWLReleaseCmdbuf: PID %d wait cmdbuf ...\n", GETPID());
    ewl->performance = EWLReadRegVcmd(inst, 82 * 4); //save the performance before release hw

    ret = CmodelIoctlReleaseCmdbuf(cmdbufid);
    if (ret < 0) {
        PTRACE_I((void *)inst, "EWLReleaseCmdbuf failed\n");
        return EWL_ERROR;
    } else {
        pthread_mutex_lock(&ewl_mutex);
        EWLWorker *worker = (EWLWorker *)queue_get(&ewl->workers);
        pthread_mutex_unlock(&ewl_mutex);
        free(worker);
        PTRACE_I((void *)inst, "EWLReleaseCmdbuf successed\n");
    }

    PTRACE_I((void *)inst, "EWLReleaseCmdbuf:  cmdbuf locked by PID %d\n", GETPID());

    return EWL_OK;
}

/*------------------------------------------------------------------------------
    Function name   : EWLTraceProfile
    Description     : print the PSNR and SSIM data, only valid for c-model.
    Return type     : void
    Argument        : none
------------------------------------------------------------------------------*/
static void EWLTraceProfileVcmd(const void *instance, void *prof_data, i32 qp, i32 poc)
{
#if 0
    ewlSysInstance *inst = (ewlSysInstance *)instance;
    float bit_rate_avg;
    float Y_PSNR_avg;
    float U_PSNR_avg;
    float V_PSNR_avg;
    u32 core_id = LAST_CORE(inst);
    SysCore *core = inst->core[core_id];

    i32 type;
    i32 qp;

    // MUST be the same as the struct in "pictur".
    struct {
        i32 bitnum;
        float psnr_y, psnr_u, psnr_v;
        double ssim;
        double ssim_y, ssim_u, ssim_v;
    } prof;

    void *prof_data;
    i32 poc;

    prof_data = (void*)CoreEncGetAddrRegisterValue(core, HWIF_ENC_COMPRESSEDCOEFF_BASE);
    qp = CoreEncGetRegisterValue(core, HWIF_ENC_PIC_QP);
    poc = CoreEncGetRegisterValue(core, HWIF_ENC_POC);

    memcpy(&prof, prof_data, sizeof(prof));

    if (inst->prof.QP_Max < qp)
        inst->prof.QP_Max = qp;
    if (inst->prof.QP_Min > qp)
        inst->prof.QP_Min = qp;
    if (inst->prof.Y_PSNR_Max < prof.psnr_y)
        inst->prof.Y_PSNR_Max = prof.psnr_y;
    if (inst->prof.Y_PSNR_Min > prof.psnr_y)
        inst->prof.Y_PSNR_Min = prof.psnr_y;
    if (inst->prof.U_PSNR_Max < prof.psnr_y)
        inst->prof.U_PSNR_Max = prof.psnr_y;
    if (inst->prof.U_PSNR_Min > prof.psnr_y)
        inst->prof.U_PSNR_Min = prof.psnr_y;
    if (inst->prof.V_PSNR_Max < prof.psnr_y)
        inst->prof.V_PSNR_Max = prof.psnr_y;
    if (inst->prof.V_PSNR_Min > prof.psnr_y)
        inst->prof.V_PSNR_Min = prof.psnr_y;
    inst->prof.frame_number++;
    inst->prof.total_bits += prof.bitnum;
    bit_rate_avg = (inst->prof.total_bits / inst->prof.frame_number) * 30;
    inst->prof.total_Y_PSNR += prof.psnr_y;
    Y_PSNR_avg = inst->prof.total_Y_PSNR / inst->prof.frame_number;
    inst->prof.total_U_PSNR += prof.psnr_u;
    U_PSNR_avg = inst->prof.total_U_PSNR / inst->prof.frame_number;
    inst->prof.total_V_PSNR += prof.psnr_v;
    V_PSNR_avg = inst->prof.total_V_PSNR / inst->prof.frame_number;

    inst->prof.total_ssim += prof.ssim;
    inst->prof.total_ssim_y += prof.ssim_y;
    inst->prof.total_ssim_u += prof.ssim_u;
    inst->prof.total_ssim_v += prof.ssim_v;

    printf("    CModel::POC %3d QP %3d %9d bits [Y %.4f dB  U %.4f dB  V %.4f dB] [SSIM %.4f average_SSIM %.4f] [SSIM Y %.4f U %.4f V %.4f average_SSIM Y %.4f U %.4f V %.4f]\n",
           poc, qp, prof.bitnum, prof.psnr_y, prof.psnr_u, prof.psnr_v, prof.ssim, inst->prof.total_ssim / inst->prof.frame_number, prof.ssim_y, prof.ssim_u, prof.ssim_v,
           inst->prof.total_ssim_y / inst->prof.frame_number, inst->prof.total_ssim_u / inst->prof.frame_number, inst->prof.total_ssim_v / inst->prof.frame_number);
    printf("    CModel::POC %3d QPMin/QPMax %d/%d Y_PSNR_Min/Max %.4f/%.4f dB U_PSNR_Min/Max %.4f/%.4f dB V_PSNR_Min/Max %.4f/%.4f \n",
           poc, inst->prof.QP_Min, inst->prof.QP_Max, inst->prof.Y_PSNR_Min, inst->prof.Y_PSNR_Max, inst->prof.U_PSNR_Min, inst->prof.U_PSNR_Max, inst->prof.V_PSNR_Min, inst->prof.V_PSNR_Max);
    printf("    CModel::POC %3d frame %d Y_PSNR_avg %.4f dB  U_PSNR_avg %.4f dB V_PSNR_avg %.4f dB \n",
           poc, inst->prof.frame_number - 1, Y_PSNR_avg, U_PSNR_avg, V_PSNR_avg);
#endif
}
#ifdef TRACE_CMDBUF
#ifdef TEST_DATA
static void trace_print_bus_line_big_endian_vcmd(FILE *file, u8 *data, i32 bus_length)
{
    u32 word;

    ASSERT(bus_length == 16);

    word = data[15] << 24 | data[14] << 16 | data[13] << 8 | data[12];

    fprintf(file, "%08X", word);
    word = data[11] << 24 | data[10] << 16 | data[9] << 8 | data[8];
    fprintf(file, "%08X", word);
    word = data[7] << 24 | data[6] << 16 | data[5] << 8 | data[4];
    fprintf(file, "%08X", word);
    word = data[3] << 24 | data[2] << 16 | data[1] << 8 | data[0];
    fprintf(file, "%08X\n", word);
}

static void trace_print_128bits_bigendian_vcmd(u8 *data, i32 length, FILE *file)
{
    i32 j;
    u8 save_data[16];

    if (!file || !data)
        return;

    /* Write denali data format: one address of 128-bit data on each line */
    for (j = 0; j < length / 16; j++) {
        trace_print_bus_line_big_endian_vcmd(file, data + j * 16, 16);
    }

    //handle tail
    int rest = (length)&0xf;
    if (rest) {
        int i;
        int idx = (length >> 4) << 4;
        for (i = 0; i < rest; i++)
            save_data[i] = data[idx + i];
        for (i = rest; i < 16; i++)
            save_data[i] = 0;
        trace_print_bus_line_big_endian_vcmd(file, save_data, 16);
    }
}

static void trace_print_top_cmdbuf_dataVcmd(u8 *data, i32 length)
{
    static FILE *fileHex = NULL;
    static u32 cmdbuf_counter = 0;
    if (fileHex == NULL)
        fileHex = fopen("Top_swreg_cmdbuf_data.trc", "wt");
    fprintf(fileHex, "# cmdbuf=%i cmdbuf input,valid data length=%d \n#-\n", cmdbuf_counter++,
            length);
    trace_print_128bits_bigendian_vcmd(data, length, fileHex);
}
#endif
#endif

static void EWLDCacheRangeFlushVcmd(const void *instance, EWLLinearMem_t *info)
{
}

static void EWLDCacheRangeRefreshVcmd(const void *instance, EWLLinearMem_t *info)
{
}

static i32 EWLWaitHwRdyVcmd(const void *instance, u32 *slicesReady, void *waitOut,
                            u32 *status_register)
{
    return EWL_OK;
}

static i32 EWLReserveHwVcmd(const void *inst, u32 *core_info, u32 *job_id)
{
    return EWL_OK;
}

static void EWLReleaseHwVcmd(const void *inst)
{
}

static u32 EWLGetCoreNumVcmd(void *ctx)
{
    return EWL_OK;
}

void EWLSetVCMDModeVcmd(const void *inst, u32 mode)
{
    (void)inst;
    (void)mode;
}

u32 EWLGetVCMDModeVcmd(const void *inst)
{
    (void)inst;
    return 1;
}

u32 EWLGetVCMDSupportVcmd()
{
    return 1;
}

u32 EWLReleaseEwlWorkerInstVcmd(const void *inst)
{
    ewlSysInstance *instance = (ewlSysInstance *)inst;
    if (instance->winst.hevc != NULL) {
        free(instance->winst.hevc);
        instance->winst.hevc = NULL;
    }
    if (instance->winst.jpeg != NULL) {
        free(instance->winst.jpeg);
        instance->winst.jpeg = NULL;
    }
    if (instance->winst.cutree != NULL) {
        free(instance->winst.cutree);
        instance->winst.cutree = NULL;
    }
    return 0;
}

static void EWLClearTraceProfileVcmd(const void *instance)
{
}
void EWLAttachVCDM(EWLFun *EWLFunP)
{
    EWLFunP->EWLGetLineBufSramP = EWLGetLineBufSramVcmd;
    EWLFunP->EWLMallocLoopbackLineBufP = EWLMallocLoopbackLineBufVcmd;
    EWLFunP->EWLGetClientTypeP = EWLGetClientTypeVcmd;
    EWLFunP->EWLGetCoreTypeByClientTypeP = EWLGetCoreTypeByClientTypeVcmd;
    EWLFunP->EWLCheckCutreeValidP = EWLCheckCutreeValidVcmd;
    EWLFunP->EwlReleaseCoreWaitP = EwlReleaseCoreWaitVcmd;
    EWLFunP->EWLReadAsicIDP = EWLReadAsicIDVcmd;
    EWLFunP->EWLReadAsicConfigP = EWLReadAsicConfigVcmd;
    EWLFunP->EWLInitP = EWLInitVcmd;
    EWLFunP->EWLReleaseP = EWLReleaseVcmd;
    EWLFunP->EWLWriteCoreRegP = EWLWriteCoreRegVcmd;
    EWLFunP->EWLWriteRegP = EWLWriteRegVcmd;
    EWLFunP->EWLSetReserveBaseDataP = EWLSetReserveBaseDataVcmd;
    EWLFunP->EWLCollectWriteRegDataP = EWLCollectWriteRegDataVcmd;
    EWLFunP->EWLCollectNopDataP = EWLCollectNopDataVcmd;
    EWLFunP->EWLCollectStallDataEncVideoP = EWLCollectStallDataEncVideoVcmd;
    EWLFunP->EWLCollectStallDataCuTreeP = EWLCollectStallDataCuTreeVcmd;
    EWLFunP->EWLCollectReadRegDataP = EWLCollectReadRegDataVcmd;
    EWLFunP->EWLCollectIntDataP = EWLCollectIntDataVcmd;
    EWLFunP->EWLCollectJmpDataP = EWLCollectJmpDataVcmd;
    EWLFunP->EWLCollectClrIntDataP = EWLCollectClrIntDataVcmd;
    EWLFunP->EWLCollectClrIntReadClearDec400DataP = EWLCollectClrIntReadClearDec400DataVcmd;
    EWLFunP->EWLCollectStopHwDataP = EWLCollectStopHwDataVcmd;
    EWLFunP->EWLCollectReadVcmdRegDataP = EWLCollectReadVcmdRegDataVcmd;
    EWLFunP->EWLCollectWriteDec400RegDataP = EWLCollectWriteDec400RegDataVcmd;
    EWLFunP->EWLCollectStallDec400P = EWLCollectStallDec400Vcmd;
    EWLFunP->EWLWriteBackRegP = EWLWriteBackRegVcmd;
    EWLFunP->EWLEnableHWP = EWLEnableHWVcmd;
    EWLFunP->EWLDisableHWP = EWLDisableHWVcmd;
    EWLFunP->EWLGetPerformanceP = EWLGetPerformanceVcmd;
    EWLFunP->EWLReadRegP = EWLReadRegVcmd;
    EWLFunP->EWLReadRegInitP = EWLReadRegInitVcmd;
    EWLFunP->EWLMallocRefFrmP = EWLMallocRefFrmVcmd;
    EWLFunP->EWLFreeRefFrmP = EWLFreeRefFrmVcmd;
    EWLFunP->EWLMallocLinearP = EWLMallocLinearVcmd;
    EWLFunP->EWLFreeLinearP = EWLFreeLinearVcmd;
    EWLFunP->EWLGetDec400CoreidP = EWLGetDec400CoreidVcmd;
    EWLFunP->EWLGetDec400CustomerIDP = EWLGetDec400CustomerIDVcmd;
    EWLFunP->EWLGetDec400AttributeP = EWLGetDec400AttributeVcmd;
    EWLFunP->EWLReserveCmdbufP = EWLReserveCmdbufVcmd;
    EWLFunP->EWLCopyDataToCmdbufP = EWLCopyDataToCmdbufVcmd;
    EWLFunP->EWLLinkRunCmdbufP = EWLLinkRunCmdbufVcmd;
    EWLFunP->EWLWaitCmdbufP = EWLWaitCmdbufVcmd;
    EWLFunP->EWLGetRegsByCmdbufP = EWLGetRegsByCmdbufVcmd;
    EWLFunP->EWLReleaseCmdbufP = EWLReleaseCmdbufVcmd;
    EWLFunP->EWLTraceProfileP = EWLTraceProfileVcmd;

    //empty function
    EWLFunP->EWLDCacheRangeFlushP = EWLDCacheRangeFlushVcmd;
    EWLFunP->EWLDCacheRangeRefreshP = EWLDCacheRangeRefreshVcmd;
    EWLFunP->EWLWaitHwRdyP = EWLWaitHwRdyVcmd;
    EWLFunP->EWLReserveHwP = EWLReserveHwVcmd;
    EWLFunP->EWLReleaseHwP = EWLReleaseHwVcmd;
    EWLFunP->EWLGetCoreNumP = EWLGetCoreNumVcmd;
    EWLFunP->EWLGetVCMDSupportP = EWLGetVCMDSupportVcmd;
    EWLFunP->EWLSetVCMDModeP = EWLSetVCMDModeVcmd;
    EWLFunP->EWLGetVCMDModeP = EWLGetVCMDModeVcmd;
    EWLFunP->EWLReleaseEwlWorkerInstP = EWLReleaseEwlWorkerInstVcmd;
    EWLFunP->EWLClearTraceProfileP = EWLClearTraceProfileVcmd;
    EWLFunP->EWLSyncMemDataP = EWLSyncMemDataVcmd;
    EWLFunP->EWLMemSyncAllocHostBufferP = EWLMemSyncAllocHostBufferVcmd;
}
