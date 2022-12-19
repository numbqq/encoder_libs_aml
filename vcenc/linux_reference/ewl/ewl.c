/*------------------------------------------------------------------------------
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                                                                            --
--                            Verisilicon.                                    --
--                                                                            --
--                   (C) COPYRIGHT 2019 VERISILICON                           --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
--------------------------------------------------------------------------------

--
--  Abstract : Encoder Wrapper Layer, common parts
--
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/

#include "base_type.h"
#include "ewl.h"
#include "ewl_common.h"
#include "cwl_vc8000_vcmd_common.h"
#include "ewl_local.h"
#include "encswhwregisters.h"
#include "encdec400.h"
#include "enc_log.h"
#include "ewl_memsync.h"
#include "ewl_local.h"

#ifdef __FREERTOS__
#include "user_freertos.h"
#include "dev_common_freertos.h"
#include "memalloc_freertos.h"
#elif defined(__linux__)
//#include "memalloc.h"
#else
#endif

#ifdef __FREERTOS__
//nothing
#elif defined(__linux__)
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <unistd.h>
#endif

#ifdef USE_EFENCE
#include "efence.h"
#endif

/* macro to convert CPU bus address to ASIC bus address */
#ifdef PC_PCI_FPGA_DEMO
//#define BUS_CPU_TO_ASIC(address)    (((address) & (~0xff000000)) | SDRAM_LM_BASE)
#define BUS_CPU_TO_ASIC(address, offset) ((address) - (offset))
#else
#define BUS_CPU_TO_ASIC(address, offset) ((address) | SDRAM_LM_BASE)
#endif

u32 vcmd_supported = 0;
volatile u32 asic_status;

static const char *synthLangName[3] = {"UNKNOWN", "VHDL", "VERILOG"};

/* Function to test input line buffer with hardware handshake, should be set by app. (invalid for system) */
u32 (*pollInputLineBufTestFunc)(void) = NULL;
/*HW core wait*/
static EWLCoreWait_t coreWait;
static pthread_mutex_t ewl_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t ewl_refer_counter_mutex = PTHREAD_MUTEX_INITIALIZER;

#ifdef SUPPORT_MEM_STATISTIC
/*Count heap statistics */
static EWLMemoryStatistics_t memoryInfo = {0};
static pthread_mutex_t linearBufferCountMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t memoryCountMutex = PTHREAD_MUTEX_INITIALIZER;
#endif

/* SW/SW shared memory */
/*------------------------------------------------------------------------------
    Function name   : EWLmalloc
    Description     : Allocate a memory block. Same functionality as
                      the ANSI C malloc()

    Return type     : void pointer to the allocated space, or NULL if there
                      is insufficient memory available

    Argument        : u32 n - Bytes to allocate
------------------------------------------------------------------------------*/
void *EWLmalloc(u32 n)
{
    void *p = malloc((size_t)n);
    PTRACE_I("EWLmalloc\t%8d bytes --> %p\n", n, p);

#ifdef SUPPORT_MEM_STATISTIC
    int memorySize = malloc_usable_size(p);
    pthread_mutex_lock(&memoryCountMutex);
    memoryInfo.maxHeapMallocTimes++;
    memoryInfo.heapMallocTimes++;
    if (memoryInfo.averageHeapMallocTimes < memoryInfo.heapMallocTimes)
        memoryInfo.averageHeapMallocTimes = memoryInfo.heapMallocTimes;
    memoryInfo.memoryValue += memorySize;
    if (memoryInfo.maxHeapMemory < memoryInfo.memoryValue)
        memoryInfo.maxHeapMemory = memoryInfo.memoryValue;
    pthread_mutex_unlock(&memoryCountMutex);
#endif

    return p;
}

/*------------------------------------------------------------------------------
    Function name   : EWLfree
    Description     : Deallocates or frees a memory block. Same functionality as
                      the ANSI C free()

    Return type     : void

    Argument        : void *p - Previously allocated memory block to be freed
------------------------------------------------------------------------------*/
void EWLfree(void *p)
{
    PTRACE_I("EWLfree\t%p\n", p);

    if (p != NULL) {
#ifdef SUPPORT_MEM_STATISTIC
        int memorySize = malloc_usable_size(p);
        pthread_mutex_lock(&memoryCountMutex);
        memoryInfo.heapMallocTimes--;
        memoryInfo.memoryValue -= memorySize;
        pthread_mutex_unlock(&memoryCountMutex);
#endif
        free(p);
    }
}

/*------------------------------------------------------------------------------
    Function name   : EWLcalloc
    Description     : Allocates an array in memory with elements initialized
                      to 0. Same functionality as the ANSI C calloc()

    Return type     : void pointer to the allocated space, or NULL if there
                      is insufficient memory available

    Argument        : u32 n - Number of elements
    Argument        : u32 s - Length in bytes of each element.
------------------------------------------------------------------------------*/
void *EWLcalloc(u32 n, u32 s)
{
    void *p = NULL;
#ifdef __FREERTOS__
    p = malloc((n) * (s));
    if (p)
        memset(p, 0, (n) * (s));
#else
    p = calloc((size_t)n, (size_t)s);
#endif

    PTRACE_I("EWLcalloc\t%8d bytes --> %p\n", n * s, p);

#ifdef SUPPORT_MEM_STATISTIC
    int memorySize = malloc_usable_size(p);
    pthread_mutex_lock(&memoryCountMutex);
    memoryInfo.maxHeapMallocTimes++;
    memoryInfo.heapMallocTimes++;
    if (memoryInfo.averageHeapMallocTimes < memoryInfo.heapMallocTimes)
        memoryInfo.averageHeapMallocTimes = memoryInfo.heapMallocTimes;
    memoryInfo.memoryValue += memorySize;
    if (memoryInfo.maxHeapMemory < memoryInfo.memoryValue)
        memoryInfo.maxHeapMemory = memoryInfo.memoryValue;
    pthread_mutex_unlock(&memoryCountMutex);
#endif

    return p;
}

/*------------------------------------------------------------------------------
    Function name   : EWLmemcpy
    Description     : Copies characters between buffers. Same functionality as
                      the ANSI C memcpy()

    Return type     : The value of destination d

    Argument        : void *d - Destination buffer
    Argument        : const void *s - Buffer to copy from
    Argument        : u32 n - Number of bytes to copy
------------------------------------------------------------------------------*/
mem_ret EWLmemcpy(void *d, const void *s, u32 n)
{
    return memcpy(d, s, (size_t)n);
}

/*------------------------------------------------------------------------------
    Function name   : EWLmemset
    Description     : Sets buffers to a specified character. Same functionality
                      as the ANSI C memset()

    Return type     : The value of destination d

    Argument        : void *d - Pointer to destination
    Argument        : i32 c - Character to set
    Argument        : u32 n - Number of characters
------------------------------------------------------------------------------*/
mem_ret EWLmemset(void *d, i32 c, u32 n)
{
    return memset(d, (int)c, (size_t)n);
}

/*------------------------------------------------------------------------------
    Function name   : EWLmemcmp
    Description     : Compares two buffers. Same functionality
                      as the ANSI C memcmp()

    Return type     : Zero if the first n bytes of s1 match s2

    Argument        : const void *s1 - Buffer to compare
    Argument        : const void *s2 - Buffer to compare
    Argument        : u32 n - Number of characters
------------------------------------------------------------------------------*/
int EWLmemcmp(const void *s1, const void *s2, u32 n)
{
#ifdef SAFESTRING
    int ind = 0;
    int rc = 0;
    if ((rc = memcmp_s(s1, (size_t)n, s2, (size_t)n, &ind)) != EOK) {
        printf("%s %u  Ind=%d  Error rc=%u \n", __FUNCTION__, __LINE__, ind, rc);
        ASSERT(0);
    }

    return ind;
#else
    return memcmp(s1, s2, (size_t)n);
#endif
}

#ifdef POLLING_ISR
static void polling_vcmd_isr(void *inst)
{
    vc8000_cwl_t *enc = (vc8000_cwl_t *)inst;
    long retVal;
    u16 core_id = 0xffff; //0xffff means polling all cores

    while (1) {
        retVal = ioctl(enc->fd_enc, HANTRO_IOCH_POLLING_CMDBUF, &core_id);
        usleep(10000); //10ms
        if (enc->wait_polling_break)
            break;
    }
}
#endif

int MapAsicRegisters(void *dev)
{
    unsigned long base;
    unsigned int size;
    u32 *pRegs;
    vc8000_cwl_t *enc = (vc8000_cwl_t *)dev;
    u32 i;
    subsysReg *reg;
    SUBSYS_CORE_INFO info;
    u32 core_id;

    for (i = 0; i < EWLGetCoreNum(NULL); i++) {
        reg = &enc->reg_all_cores[i];
        base = size = i;
        ioctl(enc->fd_enc, HANTRO_IOCG_HWOFFSET, &base);
        ioctl(enc->fd_enc, HANTRO_IOCG_HWIOSIZE, &size);

        /* map hw registers to user space */
        pRegs = (u32 *)mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, enc->fd_mem, base);
        if (pRegs == MAP_FAILED) {
            PTRACE_E("EWLInit: Failed to mmap regs\n");
            return -1;
        }
        reg->pRegBase = pRegs;
        reg->regSize = size;
        reg->subsys_id = i;

        info.type_info = i;
        ioctl(enc->fd_enc, HANTRO_IOCG_CORE_INFO, &info);
        for (core_id = 0; core_id < CORE_MAX; core_id++) {
            if (info.type_info & (1 << core_id)) {
                u32 idx = ((core_id == CORE_VC8000EJ) ? CORE_VC8000E : core_id);
                reg->core[idx].core_id = idx;
                reg->core[idx].regSize = info.regSize[idx];
                reg->core[idx].regBase = base + info.offset[idx];
                reg->core[idx].pRegBase = (u32 *)((u8 *)pRegs + info.offset[idx]);
            } else
                reg->core[core_id].core_id = -1;
        }
        PTRACE_I("EWLInit: mmap regs %d bytes --> %p\n", size, pRegs);
    }
    return 0;
}

/* get the address and size of on-chip SRAM used for loopback linebuffer */
static i32 EWLInitLineBufSram(vc8000_cwl_t *enc)
{
    if (!enc)
        return EWL_ERROR;

    /* By default, VC8000E doesn't contain such on-chip SRAM.
           In the special case of FPGA verification for line buffer,
           there is a SRAM with some registers,
           used for loopback line buffer and emulation of HW handshake*/
    enc->lineBufSramBase = 0;
    enc->lineBufSramSize = 0;
    enc->pLineBufSram = MAP_FAILED;

#ifdef PCIE_FPGA_VERI_LINEBUF
    if (ioctl(enc->fd_enc, HANTRO_IOCG_SRAMOFFSET, &enc->lineBufSramBase) == -1) {
        PTRACE_E("ioctl HANTRO_IOCG_SRAMOFFSET failed\n");
        return EWL_ERROR;
    }
    if (ioctl(enc->fd_enc, HANTRO_IOCG_SRAMEIOSIZE, &enc->lineBufSramSize) == -1) {
        PTRACE_E("ioctl HANTRO_IOCG_SRAMEIOSIZE failed\n");
        return EWL_ERROR;
    }

    /* map srame address to user space */
    enc->pLineBufSram = (u32 *)mmap(0, enc->lineBufSramSize, PROT_READ | PROT_WRITE, MAP_SHARED,
                                    enc->fd_mem, enc->lineBufSramBase);
    if (enc->pLineBufSram == MAP_FAILED) {
        PTRACE_E("EWLInit: Failed to mmap SRAM Address!\n");
        return EWL_ERROR;
    }
    enc->lineBufSramBase = 0x3FE00000;
#endif

    return EWL_OK;
}

/*------------------------------------------------------------------------------
    Function name   : EWLGetLineBufSram
    Description        : Get the base address of on-chip sram used for input line buffer.

    Return type     : i32 - 0 for success or a negative error code

    Argument        : const void * instance - EWL instance
    Argument        : EWLLinearMem_t *info - place where the sram parameters are returned
------------------------------------------------------------------------------*/
i32 EWLGetLineBufSram(const void *inst, EWLLinearMem_t *info)
{
    vc8000_cwl_t *enc = (vc8000_cwl_t *)inst;

    assert(enc != NULL);
    assert(info != NULL);

    if (enc->pLineBufSram != MAP_FAILED) {
        info->virtualAddress = (u32 *)enc->pLineBufSram;
        info->busAddress = enc->lineBufSramBase;
        info->size = enc->lineBufSramSize;
    } else {
        info->virtualAddress = NULL;
        info->busAddress = 0;
        info->size = 0;
    }

    PTRACE_I("EWLMallocLinear %p (ASIC) --> %p\n", (void *)info->busAddress, info->virtualAddress);
    return EWL_OK;
}

/*------------------------------------------------------------------------------
    Function name   : EWLMallocLoopbackLineBuf
    Description        : allocate loopback line buffer in memory, mainly used when there is no on-chip sram

    Return type     : i32 - 0 for success or a negative error code

    Argument        : const void * instance - EWL instance
    Argument        : EWLLinearMem_t *info - place where the mem parameters are returned
------------------------------------------------------------------------------*/
i32 EWLMallocLoopbackLineBuf(const void *inst, u32 size, EWLLinearMem_t *info)
{
    vc8000_cwl_t *enc = (vc8000_cwl_t *)inst;
    EWLLinearMem_t *buff = (EWLLinearMem_t *)info;
    i32 ret;

    assert(enc != NULL);
    assert(buff != NULL);

    PTRACE_I("EWLMallocLoopbackLineBuf\t%8d bytes\n", size);

    ret = EWLMallocLinear(enc, size, 0, buff);

    PTRACE_I("EWLMallocLoopbackLineBuf %p --> %p\n", (void *)buff->busAddress,
             buff->virtualAddress);

    return ret;
}

u32 EWLGetClientType(const void *inst)
{
    vc8000_cwl_t *enc = (vc8000_cwl_t *)inst;

    return enc->clientType;
}

u32 EWLGetCoreTypeByClientType(u32 client_type)
{
    u32 core_type;
    switch (client_type) {
    case EWL_CLIENT_TYPE_H264_ENC:
    case EWL_CLIENT_TYPE_HEVC_ENC:
    case EWL_CLIENT_TYPE_AV1_ENC:
        core_type = CORE_VC8000E;
        break;
    case EWL_CLIENT_TYPE_JPEG_ENC:
        core_type = CORE_VC8000E;
        break;
    case EWL_CLIENT_TYPE_CUTREE:
        core_type = CORE_CUTREE;
        break;
    case EWL_CLIENT_TYPE_DEC400:
        core_type = CORE_DEC400;
        break;
    case EWL_CLIENT_TYPE_L2CACHE:
        core_type = CORE_L2CACHE;
        break;
    case EWL_CLIENT_TYPE_AXIFE:
        core_type = CORE_AXIFE;
        break;
    case EWL_CLIENT_TYPE_AXIFE_1:
        core_type = CORE_AXIFE_1;
        break;
    case EWL_CLIENT_TYPE_APBFT:
        core_type = CORE_APBFT;
        break;
    default:
        core_type = CORE_VC8000E;
        break;
    }
    return core_type;
}

/*******************************************************************************
 Function name   : EWLCheckCutreeValid
 Description     : check it's cutree or not
*******************************************************************************/
i32 EWLCheckCutreeValid(const void *inst)
{
    vc8000_cwl_t *enc = (vc8000_cwl_t *)inst;
    u32 i = 0, val = 0;

    /* Check invalid parameters */
    if (enc == NULL)
        return EWL_ERROR;
    if (enc->vcmd_mode == VCMD_MODE_DISABLED) {
        u32 core_id = LAST_CORE(enc);
        u32 core_type = EWLGetCoreTypeByClientType(enc->clientType);
        regMapping *reg = &(enc->reg_all_cores[core_id].core[core_type]);

        val = *(reg->pRegBase + 287);
        if (val & 0x10000000)
            return EWL_OK;
        else
            return EWL_ERROR;
    } else {
        return EWL_OK;
    }
}

u32 EWLReadAsicID(u32 id, void *ctx)
{
    u32 hw_id = ~0;
    int fd_mem = -1, fd_enc = -1;
    unsigned long base = ~0;
    unsigned int size;
    u32 *pRegs = MAP_FAILED;
    u32 core_num = 0;
    u32 *mapped_addr = MAP_FAILED;
    u32 mapped_size = 0;

    fd_mem = open(MEM_MODULE_PATH, O_RDONLY);
    if (fd_mem == -1) {
        PTRACE_E("EWLReadAsicID: failed to open: %s\n", MEM_MODULE_PATH);
        goto end;
    }

    fd_enc = open(ENC_MODULE_PATH, O_RDONLY);
    if (fd_enc == -1) {
        PTRACE_E("EWLReadAsicID: failed to open: %s\n", ENC_MODULE_PATH);
        goto end;
    }

    if (vcmd_supported == 0) {
        SUBSYS_CORE_INFO info;
        u32 core_id = id;

        core_num = EWLGetCoreNum(ctx);
        if (core_id > core_num - 1)
            goto end;
        /* ask module for base */
        base = core_id;
        if (ioctl(fd_enc, HANTRO_IOCG_HWOFFSET, &base) == -1) {
            PTRACE_E("ioctl failed\n");
            goto end;
        }
        size = core_id;
        if (ioctl(fd_enc, HANTRO_IOCG_HWIOSIZE, &size) == -1) {
            PTRACE_E("ioctl failed\n");
            goto end;
        }
        /* map hw registers to user space */
        pRegs = (u32 *)mmap(0, size, PROT_READ, MAP_SHARED, fd_mem, base);
        if (pRegs == MAP_FAILED) {
            PTRACE_E("EWLReadAsicID: Failed to mmap regs\n");
            goto end;
        }
        mapped_addr = pRegs;
        mapped_size = size;
        info.type_info = core_id;
        if (ioctl(fd_enc, HANTRO_IOCG_CORE_INFO, &info) == -1) {
            PTRACE_E("ioctl failed\n");
            goto end;
        }
        core_id = GET_ENCODER_IDX(info.type_info);
        hw_id = *((u32 *)((u8 *)pRegs + info.offset[core_id]));
    } else {
        struct config_parameter vcmd_core_info;
        struct cmdbuf_mem_parameter vcmd_cmdbuf_info;
        u32 module_type = 0;
        u32 client_type;
        vcmd_cmdbuf_info.status_virt_addr = (u32)MAP_FAILED;

        client_type = id;
        switch (client_type) {
        case EWL_CLIENT_TYPE_H264_ENC:
        case EWL_CLIENT_TYPE_HEVC_ENC:
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

        if (module_type > MAX_VCMD_TYPE - 1)
            goto end;

        if (ioctl(fd_enc, HANTRO_IOCH_GET_CMDBUF_PARAMETER, &vcmd_cmdbuf_info)) {
            PTRACE_E("ioctl HANTRO_IOCH_GET_CMDBUF_PARAMETER failed \n");
            goto end;
        }

        //remap status cmdbuf virtual address.
        /* Map the bus address to virtual address */
        vcmd_cmdbuf_info.status_virt_addr =
            (u32)mmap(0, vcmd_cmdbuf_info.status_total_size, PROT_READ, MAP_SHARED, fd_mem,
                      vcmd_cmdbuf_info.status_phy_addr);

        if ((u32 *)vcmd_cmdbuf_info.status_virt_addr == MAP_FAILED) {
            PTRACE_E("ioctl mmap  failed \n");
            goto end;
        }
        mapped_addr = (u32 *)vcmd_cmdbuf_info.status_virt_addr;
        mapped_size = vcmd_cmdbuf_info.status_total_size;
        vcmd_core_info.module_type = module_type;
        if (ioctl(fd_enc, HANTRO_IOCH_GET_VCMD_PARAMETER, &vcmd_core_info)) {
            printf("ioctl HANTRO_IOCH_GET_CMDBUF_BASE_ADDR failed\n");
            ASSERT(0);
        }
        if (vcmd_core_info.vcmd_core_num == 0) {
            if (module_type == VCMD_TYPE_JPEG_ENCODER) {
                vcmd_core_info.module_type = VCMD_TYPE_ENCODER;
                if (ioctl(fd_enc, HANTRO_IOCH_GET_VCMD_PARAMETER, &vcmd_core_info)) {
                    printf("ioctl HANTRO_IOCH_GET_CMDBUF_BASE_ADDR failed\n");
                    ASSERT(0);
                }
            }

            if (vcmd_core_info.vcmd_core_num == 0) {
                PTRACE_I("there is no proper vcmd  for encoder \n");
                goto end;
            }
        }
        pRegs = (u32 *)vcmd_cmdbuf_info.status_virt_addr +
                vcmd_core_info.config_status_cmdbuf_id * vcmd_cmdbuf_info.cmd_unit_size / 4 +
                vcmd_core_info.submodule_main_addr / 2 / 4;
        hw_id = pRegs[0];
    }

    PTRACE_I("EWLReadAsicID: 0x%08x at 0x%08lx\n", hw_id, base);
end:
    if (mapped_addr != MAP_FAILED) {
        munmap(mapped_addr, mapped_size);
    }

    if (fd_mem != -1)
        close(fd_mem);
    if (fd_enc != -1)
        close(fd_enc);

    return hw_id;
}

EWLHwConfig_t EWLReadAsicConfig(u32 id, void *ctx)
{
    int fd_mem = -1, fd_enc = -1;
    unsigned long base;
    unsigned int size;
    u32 *pRegs = MAP_FAILED, cfgval;
    u32 hw_id = ~0;
    EWLHwConfig_t cfg_info;
    SUBSYS_CORE_INFO info;
    u32 *mapped_addr = MAP_FAILED;
    u32 mapped_size;

    memset(&cfg_info, 0, sizeof(cfg_info));

    fd_mem = open(MEM_MODULE_PATH, O_RDONLY);
    if (fd_mem == -1) {
        PTRACE_E("EWLReadAsicConfig: failed to open: %s\n", MEM_MODULE_PATH);
        goto end;
    }

    fd_enc = open(ENC_MODULE_PATH, O_RDONLY);
    if (fd_enc == -1) {
        PTRACE_E("EWLReadAsicConfig: failed to open: %s\n", ENC_MODULE_PATH);
        goto end;
    }

    if (vcmd_supported == 0) {
        u32 core_id = id;
        if (core_id > EWLGetCoreNum(ctx) - 1)
            goto end;

        base = core_id;
        size = core_id;
        ioctl(fd_enc, HANTRO_IOCG_HWOFFSET, &base);
        ioctl(fd_enc, HANTRO_IOCG_HWIOSIZE, &size);

        /* map hw registers to user space */
        pRegs = (u32 *)mmap(0, size, PROT_READ, MAP_SHARED, fd_mem, base);

        if (pRegs == MAP_FAILED) {
            PTRACE_E("EWLReadAsicConfig: Failed to mmap regs\n");
            goto end;
        }

        mapped_addr = pRegs;
        mapped_size = size;

        info.type_info = core_id;
        if (ioctl(fd_enc, HANTRO_IOCG_CORE_INFO, &info) == -1) {
            PTRACE_E("ioctl failed\n");
            goto end;
        }

        core_id = GET_ENCODER_IDX(info.type_info);
        pRegs = (u32 *)((u8 *)pRegs + info.offset[core_id]);
        hw_id = pRegs[0];
        cfgval = pRegs[80];
    } else {
        struct config_parameter vcmd_core_info;
        struct cmdbuf_mem_parameter vcmd_cmdbuf_info;
        vcmd_cmdbuf_info.status_virt_addr = (u32)MAP_FAILED;
        u32 client_type = id;
        u32 module_type = 0;

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
        if (module_type > MAX_VCMD_TYPE - 1)
            goto end;

        if (ioctl(fd_enc, HANTRO_IOCH_GET_CMDBUF_PARAMETER, &vcmd_cmdbuf_info)) {
            PTRACE_E("ioctl HANTRO_IOCH_GET_CMDBUF_PARAMETER failed \n");
            goto end;
        }

        //remap status cmdbuf virtual address.
        /* Map the bus address to virtual address */

        vcmd_cmdbuf_info.status_virt_addr =
            (u32)mmap(0, vcmd_cmdbuf_info.status_total_size, PROT_READ, MAP_SHARED, fd_mem,
                      vcmd_cmdbuf_info.status_phy_addr);

        if ((u32 *)vcmd_cmdbuf_info.status_virt_addr == MAP_FAILED) {
            PTRACE_E("ioctl mmap  failed \n");
            goto end;
        }

        mapped_addr = (u32 *)vcmd_cmdbuf_info.status_virt_addr;
        mapped_size = vcmd_cmdbuf_info.status_total_size;

        vcmd_core_info.module_type = module_type;
        if (ioctl(fd_enc, HANTRO_IOCH_GET_VCMD_PARAMETER, &vcmd_core_info)) {
            printf("ioctl HANTRO_IOCH_GET_CMDBUF_BASE_ADDR failed\n");
            ASSERT(0);
        }

        if (vcmd_core_info.vcmd_core_num == 0) {
            if (module_type == VCMD_TYPE_JPEG_ENCODER) {
                vcmd_core_info.module_type = VCMD_TYPE_ENCODER;
                if (ioctl(fd_enc, HANTRO_IOCH_GET_VCMD_PARAMETER, &vcmd_core_info)) {
                    printf("ioctl HANTRO_IOCH_GET_CMDBUF_BASE_ADDR failed\n");
                    ASSERT(0);
                }
            }

            if (vcmd_core_info.vcmd_core_num == 0) {
                PTRACE_E("there is no proper vcmd  for encoder \n");
                goto end;
            }
        }
        pRegs = (u32 *)vcmd_cmdbuf_info.status_virt_addr +
                vcmd_core_info.config_status_cmdbuf_id * vcmd_cmdbuf_info.cmd_unit_size / 4 +
                vcmd_core_info.submodule_main_addr / 2 / 4;
        hw_id = pRegs[0];
        cfgval = pRegs[80];
    }

    EWLCoreSignature_t signature;
    if (EWL_OK != EWLGetCoreSignature(pRegs, &signature)) {
        PTRACE_E("ERROR when get the core signature.");
    }

    assert(signature.hw_asic_id == hw_id);
    if (EWL_OK != EWLGetCoreConfig(&signature, &cfg_info)) {
        PTRACE_E("ERROR when get the core feature list.");
    }

end:
    if (mapped_addr != MAP_FAILED) {
        munmap(mapped_addr, mapped_size);
    }

    if (fd_mem != -1)
        close(fd_mem);
    if (fd_enc != -1)
        close(fd_enc);

    return cfg_info;
}

/* Get one job from queue */
EWLCoreWaitJob_t *EWLDequeueCoreOutJob(const void *inst, u32 waitCoreJobid)
{
    EWLCoreWait_t *pCoreWait = (EWLCoreWait_t *)&coreWait;
    EWLCoreWaitJob_t *job, *out;

    while (!pCoreWait->bFlush) {
        out = NULL;
        pthread_mutex_lock(&pCoreWait->out_mutex);

        job = (EWLCoreWaitJob_t *)queue_tail(&pCoreWait->out);

        while (job != NULL) {
            if (job->id == waitCoreJobid) {
                out = job;
                queue_remove(&pCoreWait->out, (struct node *)job);
                break;
            }

            job = (EWLCoreWaitJob_t *)((struct node *)job->next);
        }

        while (job == NULL && !pCoreWait->bFlush) {
            pthread_cond_wait(&pCoreWait->out_cond, &pCoreWait->out_mutex);
            job = (EWLCoreWaitJob_t *)queue_tail(&pCoreWait->out);
        }

        pthread_mutex_unlock(&pCoreWait->out_mutex);
        if (out != NULL)
            return out;
    }

    return NULL;
}

/* Get one job from queue */
static EWLCoreWaitJob_t *EWLDequeueCoreWaitJob(EWLCoreWait_t *coreWait)
{
    pthread_mutex_lock(&coreWait->job_mutex);
    EWLCoreWaitJob_t *job = (EWLCoreWaitJob_t *)queue_tail(&coreWait->jobs);
    while (job == NULL && !coreWait->bFlush) {
        pthread_cond_wait(&coreWait->job_cond, &coreWait->job_mutex);
        job = (EWLCoreWaitJob_t *)queue_tail(&coreWait->jobs);
    }
    pthread_mutex_unlock(&coreWait->job_mutex);
    return job;
}

/* Get one job from job pool and if no available allocate a new one */
static EWLCoreWaitJob_t *EWLGetJobfromPool(EWLCoreWait_t *coreWait)
{
    EWLCoreWaitJob_t *job = (EWLCoreWaitJob_t *)queue_get(&coreWait->job_pool);
    if (job == NULL)
        job = (EWLCoreWaitJob_t *)EWLmalloc(sizeof(EWLCoreWaitJob_t));

    return job;
}

/*Put un-used job to pool for other usage*/
void EWLPutJobtoPool(const void *inst, struct node *job)
{
    pthread_mutex_lock(&coreWait.job_mutex);
    queue_put(&coreWait.job_pool, job);
    pthread_mutex_unlock(&coreWait.job_mutex);
}

void EWLGetRegsAfterFrameDone(const void *inst, EWLCoreWaitJob_t *job, u32 irq_status)
{
    vc8000_cwl_t *enc = (vc8000_cwl_t *)inst;
    int i;

    if (irq_status == ASIC_STATUS_FRAME_READY) {
        for (i = 0; i < ASIC_SWREG_AMOUNT; i++) {
            job->VC8000E_reg[i] = EWLReadReg(enc, i * 4);
        }
    }

    if (job->dec400_enable)
        EWLDisableDec400(inst);

    if (job->axife_enable)
        job->axife_callback(job->inst, NULL);

    if (job->l2cache_enable) {
        job->l2cache_callback(inst, &job->l2cache_data);
    }

    EWLReleaseHw(inst);
}

void EWLGetCoreOutRel(const void *inst, i32 ewl_ret, EWLCoreWaitJob_t *job)
{
    vc8000_cwl_t *enc = (vc8000_cwl_t *)inst;
    EWLWorker *worker;
    u32 i = 0;
    u32 status = job->out_status;
    u32 core_id = FIRST_CORE(enc);
    enc->unCheckPid = 1;

    /*move the core received irq to queue tail for subsequent register operation is for core on the tail*/
    if (core_id != job->core_id) {
        pthread_mutex_lock(&ewl_mutex);
        EWLWorker *worker = (EWLWorker *)queue_tail(&enc->workers);
        while (worker && worker->core_id != job->core_id) {
            worker = (EWLWorker *)worker->next;
        }
        queue_remove(&enc->workers, (struct node *)worker);
        queue_put_tail(&enc->workers, (struct node *)worker);
        pthread_mutex_unlock(&ewl_mutex);
    }

    if (ewl_ret != EWL_OK) {
        job->out_status = ASIC_STATUS_ERROR;

        PTRACE_E("EWLGetCoreOutRel: ERROR Core return != EWL_OK.");
        EWLDisableHW(inst, ASIC_REG_INDEX_STATUS * 4, 0);

        /* Release Core so that it can be used by other codecs */
        EWLGetRegsAfterFrameDone(inst, job, job->out_status);
    } else {
        /* Check ASIC status bits and release HW */
        status &= ASIC_STATUS_ALL;

        if (status & ASIC_STATUS_ERROR) {
            /* Get registers for debugging */
            status = ASIC_STATUS_ERROR;
            EWLGetRegsAfterFrameDone(inst, job, status);
        } else if (status & ASIC_STATUS_FUSE_ERROR) {
            /* Get registers for debugging */
            status = ASIC_STATUS_ERROR;
            EWLGetRegsAfterFrameDone(inst, job, status);
        } else if (status & ASIC_STATUS_HW_TIMEOUT) {
            /* Get registers for debugging */
            status = ASIC_STATUS_HW_TIMEOUT;
            EWLGetRegsAfterFrameDone(inst, job, status);
        } else if (status & ASIC_STATUS_FRAME_READY) {
            /* read out all register */
            status = ASIC_STATUS_FRAME_READY;
            EWLGetRegsAfterFrameDone(inst, job, status);
        } else if (status & ASIC_STATUS_BUFF_FULL) {
            /* ASIC doesn't support recovery from buffer full situation,
             * at the same time with buff full ASIC also resets itself. */
            status = ASIC_STATUS_BUFF_FULL;
            EWLGetRegsAfterFrameDone(inst, job, status);
        } else if (status & ASIC_STATUS_HW_RESET) {
            status = ASIC_STATUS_HW_RESET;
            EWLGetRegsAfterFrameDone(inst, job, status);
        } else if (status & ASIC_STATUS_SLICE_READY) {
            status = ASIC_STATUS_SLICE_READY;
            job->slices_rdy = ((EWLReadReg(enc, 7 * 4) >> 17) & 0xFF);
        } else if (status & ASIC_STATUS_LINE_BUFFER_DONE) {
            status = ASIC_STATUS_LINE_BUFFER_DONE;
            job->VC8000E_reg[196] = EWLReadReg(enc, 196 * 4);

            //save sw handshake rd pointer if sw handshake is enabled
            if ((!(job->VC8000E_reg[196] >> 31)) &&
                (job->low_latency_rd < ((job->VC8000E_reg[196] & 0x000ffc00) >> 10)))
                job->low_latency_rd = ((job->VC8000E_reg[196] & 0x000ffc00) >> 10);
            else
                status = 0;
        } else if (status & ASIC_STATUS_SEGMENT_READY) {
            status = ASIC_STATUS_SEGMENT_READY;
            for (i = 1; i < ASIC_SWREG_AMOUNT; i++) {
                job->VC8000E_reg[i] = EWLReadReg(enc, i * 4);
            }
        }

        job->out_status = status;
    }
}

void EWLDisableDec400(const void *inst)
{
#ifdef SUPPORT_DEC400
    vc8000_cwl_t *enc = (vc8000_cwl_t *)inst;
    u32 loop = 1000;
    u32 dec400_customer_regs = 0;

    i32 core_id = FIRST_CORE(enc);

    if ((dec400_customer_regs = EWLGetDec400CustomerID(inst, core_id)) == -1)
        return;

    if (dec400_customer_regs == THBM1_DEC400 || dec400_customer_regs == THBM3_DEC400) {
        //do SW reset after one frame and wait it done by HW if needed
        EWLWriteBackRegbyClientType(inst, 0xB00, 0x00000010, EWL_CLIENT_TYPE_DEC400);

#ifdef PC_PCI_FPGA_DEMO
        usleep(80000);
#endif
    } else {
        //flush tile status and wait for IRQ
        EWLWriteBackRegbyClientType(inst, 0x800, 0x00000001, EWL_CLIENT_TYPE_DEC400);
        do {
            if (EWLReadRegbyClientType(inst, 0x820, EWL_CLIENT_TYPE_DEC400) & 0x00000001)
                break;

            usleep(80);
        } while (loop--);
    }

#endif
}

/* get register setting from api, reserve a core and set registers*/
static void *EWLCoreWaitThread(void *pCoreWait)
{
    EWLCoreWait_t *coreWait = (EWLCoreWait_t *)pCoreWait;
    struct node *job;
    u32 reserve_core_info = 0;
    u32 i, client_type_previous;
    vc8000_cwl_t ewl;
    CORE_WAIT_OUT waitOut;
    i32 ewl_ret = EWL_OK;
    u32 has_output = 0;
    u32 wait_error = 0;

    while ((job = (struct node *)EWLDequeueCoreWaitJob(coreWait)) != NULL) {
        ewl.fd_enc = ((vc8000_cwl_t *)(((EWLCoreWaitJob_t *)job)->inst))->fd_enc;

        memset(&waitOut, 0, sizeof(CORE_WAIT_OUT));

        /* wait for VC8000E Core done */
        if (wait_error == 0 && ((ewl_ret = EWLWaitHwRdy(&ewl, NULL, &waitOut, NULL)) != EWL_OK)) {
            wait_error = 1;
        }

        pthread_mutex_lock(&coreWait->job_mutex);
        job = queue_tail(&coreWait->jobs);

        while (job != NULL) {
            struct node *next = job->next;

            for (i = 0; i < waitOut.irq_num; i++) {
                if (waitOut.job_id[i] == ((EWLCoreWaitJob_t *)job)->id) {
                    ((EWLCoreWaitJob_t *)job)->out_status = waitOut.irq_status[i];
                    EWLGetCoreOutRel(((EWLCoreWaitJob_t *)job)->inst, ewl_ret,
                                     (EWLCoreWaitJob_t *)job);

                    if (((EWLCoreWaitJob_t *)job)->out_status &
                        (ASIC_STATUS_FUSE_ERROR | ASIC_STATUS_HW_TIMEOUT | ASIC_STATUS_BUFF_FULL |
                         ASIC_STATUS_HW_RESET | ASIC_STATUS_ERROR | ASIC_STATUS_FRAME_READY)) {
                        queue_remove(&coreWait->jobs, (struct node *)job);
                        pthread_mutex_lock(&coreWait->out_mutex);
                        queue_put(&coreWait->out, job);
                        pthread_mutex_unlock(&coreWait->out_mutex);
                        has_output = 1;
                    } else if (((EWLCoreWaitJob_t *)job)->out_status) {
                        struct node *tmp_out = (struct node *)EWLGetJobfromPool(coreWait);
                        memcpy(tmp_out, job, sizeof(EWLCoreWaitJob_t));
                        pthread_mutex_lock(&coreWait->out_mutex);
                        queue_put(&coreWait->out, tmp_out);
                        pthread_mutex_unlock(&coreWait->out_mutex);
                        has_output = 1;
                    }

                    break;
                }
            }

            if (wait_error == 1) {
                EWLGetCoreOutRel(((EWLCoreWaitJob_t *)job)->inst, ewl_ret, (EWLCoreWaitJob_t *)job);
                queue_remove(&coreWait->jobs, (struct node *)job);
                pthread_mutex_lock(&coreWait->out_mutex);
                queue_put(&coreWait->out, job);
                pthread_mutex_unlock(&coreWait->out_mutex);
                has_output = 1;
            }

            job = next;
        }

        pthread_mutex_unlock(&coreWait->job_mutex);

        if (has_output == 1) {
            pthread_cond_broadcast(&coreWait->out_cond);
            has_output = 0;
        }
    }

    return NULL;
}

void EWLEnqueueWaitjob(const void *inst, EWLWaitJobCfg_t *cfg)
{
    vc8000_cwl_t *enc = (vc8000_cwl_t *)inst;
    if (enc == NULL)
        return;

    if (enc->vcmd_mode == VCMD_MODE_ENABLED)
        return;

    //enqueue the job for core wait thread
    pthread_mutex_lock(&coreWait.job_mutex);

    EWLCoreWaitJob_t *job = EWLGetJobfromPool(&coreWait);
    memset(job, 0, sizeof(EWLCoreWaitJob_t));
    job->id = cfg->waitCoreJobid;
    job->core_id = LAST_CORE(((vc8000_cwl_t *)inst));
    job->inst = inst;
    job->dec400_enable = cfg->dec400_enable;
    job->axife_enable = cfg->axife_enable;
    job->axife_callback = cfg->axife_callback;
    job->l2cache_enable = cfg->l2cache_enable;
    memcpy(&job->l2cache_data, cfg->l2cache_data, sizeof(CacheData_t));
    job->l2cache_callback = cfg->l2cache_callback;

    queue_put(&coreWait.jobs, (struct node *)job);
    pthread_cond_signal(&coreWait.job_cond);
    pthread_mutex_unlock(&coreWait.job_mutex);
}

/* create ewl core wait thread and now just support VC8000E*/
static void EwlCreateCoreWait(void)
{
    EWLCoreWait_t *pCoreWait = &coreWait;

    pthread_attr_t attr;
    pthread_t *tid_CoreWait = (pthread_t *)EWLmalloc(sizeof(pthread_t));
    pthread_mutexattr_t mutexattr;
    pthread_condattr_t condattr;

    queue_init(&coreWait.jobs);
    queue_init(&coreWait.out);

    pthread_mutexattr_init(&mutexattr);
    pthread_mutex_init(&coreWait.job_mutex, &mutexattr);
    pthread_mutex_init(&coreWait.out_mutex, &mutexattr);
    pthread_mutexattr_destroy(&mutexattr);
    pthread_condattr_init(&condattr);
    pthread_cond_init(&coreWait.job_cond, &condattr);
    pthread_cond_init(&coreWait.out_cond, &condattr);
    pthread_condattr_destroy(&condattr);

    pthread_attr_init(&attr);
    pthread_create(tid_CoreWait, &attr, &EWLCoreWaitThread, pCoreWait);
    pthread_attr_destroy(&attr);

    coreWait.tid_CoreWait = tid_CoreWait;

    return;
}

void EwlReleaseCoreWait(void *inst)
{
    /* Wait for Core Wait Thread finish */
    pthread_mutex_lock(&ewl_refer_counter_mutex);
    if (coreWait.tid_CoreWait && coreWait.refer_counter == 0) {
        pthread_join(*coreWait.tid_CoreWait, NULL);

        pthread_mutex_destroy(&coreWait.job_mutex);
        pthread_mutex_destroy(&coreWait.out_mutex);
        pthread_cond_destroy(&coreWait.job_cond);
        pthread_cond_destroy(&coreWait.out_cond);

        EWLfree(coreWait.tid_CoreWait);
        coreWait.tid_CoreWait = NULL;
        EWLCoreWaitJob_t *job = NULL;
        while ((job = (EWLCoreWaitJob_t *)queue_get(&coreWait.jobs)) != NULL) {
            EWLfree(job);
            job = NULL;
        }

        while ((job = (EWLCoreWaitJob_t *)queue_get(&coreWait.out)) != NULL) {
            EWLfree(job);
            job = NULL;
        }

        while ((job = (EWLCoreWaitJob_t *)queue_get(&coreWait.job_pool)) != NULL) {
            EWLfree(job);
            job = NULL;
        }

#ifdef SUPPORT_MEM_STATISTIC
        MEM_LOG_I("Max Heap Memory:           %8d bytes\n", memoryInfo.maxHeapMemory);
        MEM_LOG_I("Max Heap Malloc Times:     %8d times\n", memoryInfo.maxHeapMallocTimes);
        MEM_LOG_I("Average Heap Malloc Times: %8d times\n", memoryInfo.averageHeapMallocTimes);

        MEM_LOG_I("Heap Max Linear Memory:    %8d bytes\n", memoryInfo.maxLinearMemory);
        MEM_LOG_I("Heap Mallc Linear Times:   %8d times\n", memoryInfo.maxLinearMallocTimes);
        MEM_LOG_I("Average Heap Malloc Times: %8d times\n", memoryInfo.averageLinearMallocTimes);
#endif
    }
    pthread_mutex_unlock(&ewl_refer_counter_mutex);
}

const void *EWLInit(EWLInitParam_t *param)
{
    vc8000_cwl_t *enc = NULL;
    int i;

    PTRACE_I("EWLInit: Start\n");

    /* Check for NULL pointer */
    if (param == NULL || param->clientType >= EWL_CLIENT_TYPE_MAX) {
        PTRACE_E(("EWLInit: Bad calling parameters!\n"));
        return NULL;
    }

    /* Allocate instance */
    if ((enc = (vc8000_cwl_t *)EWLmalloc(sizeof(vc8000_cwl_t))) == NULL) {
        PTRACE_E("EWLInit: failed to alloc vc8000_cwl_t struct\n");
        return NULL;
    }
    memset(enc, 0, sizeof(vc8000_cwl_t));

    enc->clientType = param->clientType;
    enc->fd_mem = enc->fd_enc = enc->fd_memalloc = -1;
    enc->reg_all_cores = NULL;
    enc->vcmd_cmdbuf_info.cmd_virt_addr = (u32)MAP_FAILED;
    enc->vcmd_cmdbuf_info.status_virt_addr = (u32)MAP_FAILED;
    /* New instance allocated */
    enc->fd_mem = open(MEM_MODULE_PATH, O_RDWR | O_SYNC);
    if (enc->fd_mem == -1) {
        PTRACE_E("EWLInit: failed to open: %s\n", MEM_MODULE_PATH);
        goto err;
    }

    enc->fd_enc = open(ENC_MODULE_PATH, O_RDWR);
    if (enc->fd_enc == -1) {
        PTRACE_E("EWLInit: failed to open: %s\n", ENC_MODULE_PATH);
        goto err;
    }

    enc->fd_memalloc = open(MEMALLOC_MODULE_PATH, O_RDWR);
    if (enc->fd_memalloc == -1) {
        PTRACE_E("EWLInit: failed to open: %s\n", MEMALLOC_MODULE_PATH);
        goto err;
    }

    if (ioctl(enc->fd_enc, HANTRO_IOCH_GET_MMU_ENABLE, &enc->mmuEnable)) {
        PTRACE_E("ioctl HANTRO_IOCH_GET_MMU_ENABLE failed \n");
        goto err;
    }

    if (vcmd_supported == 0) {
        enc->vcmd_mode = VCMD_MODE_DISABLED;
        enc->reg_all_cores = EWLmalloc(EWLGetCoreNum(NULL) * sizeof(subsysReg));
        enc->coreAmout = EWLGetCoreNum(NULL);

        /* map hw registers to user space.*/
        if (MapAsicRegisters((void *)enc) != 0) {
            PTRACE_E("EWLReserveHw map register failed\n");
            goto err;
        }

        if (EWLInitLineBufSram(enc) != EWL_OK) {
            PTRACE_E("EWLInit: PCIE FPGA Verification Fail!\n");
            goto err;
        }

        queue_init(&enc->freelist);
        queue_init(&enc->workers);
        for (i = 0; i < (int)EWLGetCoreNum(NULL); i++) {
            EWLWorker *worker = EWLmalloc(sizeof(EWLWorker));
            worker->core_id = i;
            worker->next = NULL;
            queue_put(&enc->freelist, (struct node *)worker);
        }

        if (enc->clientType == EWL_CLIENT_TYPE_HEVC_ENC ||
            enc->clientType == EWL_CLIENT_TYPE_H264_ENC ||
            enc->clientType == EWL_CLIENT_TYPE_AV1_ENC ||
            enc->clientType == EWL_CLIENT_TYPE_VP9_ENC) {
            pthread_mutex_lock(&ewl_refer_counter_mutex);
            if (coreWait.refer_counter == 0)
                EwlCreateCoreWait();

            coreWait.refer_counter++;
            pthread_mutex_unlock(&ewl_refer_counter_mutex);
        }

    } else {
        assert(enc->reg_all_cores == NULL);

        u16 module_type = VCMD_TYPE_ENCODER;
        enc->vcmd_mode = VCMD_MODE_ENABLED;
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

        if (ioctl(enc->fd_enc, HANTRO_IOCH_GET_CMDBUF_PARAMETER, &enc->vcmd_cmdbuf_info)) {
            PTRACE_E("ioctl HANTRO_IOCH_GET_CMDBUF_PARAMETER failed \n");
            goto err;
        }
        enc->vcmd_enc_core_info.module_type = module_type;
        if (ioctl(enc->fd_enc, HANTRO_IOCH_GET_VCMD_PARAMETER, &enc->vcmd_enc_core_info)) {
            printf("ioctl HANTRO_IOCH_GET_CMDBUF_BASE_ADDR failed\n");
            ASSERT(0);
        }
        if (enc->vcmd_enc_core_info.vcmd_core_num == 0) {
            if (module_type == VCMD_TYPE_JPEG_ENCODER) {
                enc->vcmd_enc_core_info.module_type = VCMD_TYPE_ENCODER;
                if (ioctl(enc->fd_enc, HANTRO_IOCH_GET_VCMD_PARAMETER, &enc->vcmd_enc_core_info)) {
                    printf("ioctl HANTRO_IOCH_GET_CMDBUF_BASE_ADDR failed\n");
                    ASSERT(0);
                }
            }

            if (enc->vcmd_enc_core_info.vcmd_core_num == 0) {
                PTRACE_I("there is no proper vcmd  for encoder \n");
                goto err;
            }
        }

        //remap status cmdbuf virtual address.
        /* Map the bus address to virtual address */
        enc->vcmd_cmdbuf_info.cmd_virt_addr =
            (u32)mmap(0, enc->vcmd_cmdbuf_info.cmd_total_size, PROT_READ | PROT_WRITE, MAP_SHARED,
                      enc->fd_mem, enc->vcmd_cmdbuf_info.cmd_phy_addr);

        if ((u32 *)enc->vcmd_cmdbuf_info.cmd_virt_addr == MAP_FAILED) {
            PTRACE_E("EWLInit: Failed to mmap busAddress: %p\n",
                     (void *)enc->vcmd_cmdbuf_info.cmd_phy_addr);
            goto err;
        }

        enc->vcmd_cmdbuf_info.status_virt_addr =
            (u32)mmap(0, enc->vcmd_cmdbuf_info.status_total_size, PROT_READ | PROT_WRITE,
                      MAP_SHARED, enc->fd_mem, enc->vcmd_cmdbuf_info.status_phy_addr);

        if ((u32 *)enc->vcmd_cmdbuf_info.status_virt_addr == MAP_FAILED) {
            PTRACE_E("EWLInit: Failed to mmap busAddress: %p\n",
                     (void *)enc->vcmd_cmdbuf_info.status_phy_addr);
            goto err;
        }
        queue_init(&enc->workers);
        enc->main_module_init_status_addr = (u32 *)enc->vcmd_cmdbuf_info.status_virt_addr +
                                            enc->vcmd_cmdbuf_info.status_unit_size / 4 *
                                                enc->vcmd_enc_core_info.config_status_cmdbuf_id;
        enc->main_module_init_status_addr += enc->vcmd_enc_core_info.submodule_main_addr / 2 / 4;
    }

    //register Wrapper layer interface called by dec400
    DEC400_WL_INTERFACE dec400Wl;
    dec400Wl.WLGetDec400Coreid = EWLGetDec400Coreid;
    dec400Wl.WLGetDec400CustomerID = EWLGetDec400CustomerID;
    dec400Wl.WLGetVCMDSupport = EWLGetVCMDSupport;
    dec400Wl.WLCollectWriteDec400RegData = EWLCollectWriteDec400RegData;
    dec400Wl.WLWriteReg = EWLWriteRegbyClientType;
    dec400Wl.WLWriteBackReg = EWLWriteBackRegbyClientType;
    dec400Wl.WLReadReg = EWLReadRegbyClientType;
    dec400Wl.WLCollectClrIntReadClearDec400Data = EWLCollectClrIntReadClearDec400Data;
    dec400Wl.WLCollectStallDec400 = EWLCollectStallDec400;
    VCEncDec400RegisiterWL(&dec400Wl);

    PTRACE_I("EWLInit: Return %p\n", enc);
    return enc;

err:
    EWLRelease(enc);
    PTRACE_I("EWLInit: Return NULL\n");
    return NULL;
}

i32 EWLRelease(const void *inst)
{
    vc8000_cwl_t *enc = (vc8000_cwl_t *)inst;
    u32 i;

    assert(enc != NULL);

    if (enc == NULL)
        return EWL_OK;

    if (enc->vcmd_mode == VCMD_MODE_DISABLED) {
        if (enc->clientType == EWL_CLIENT_TYPE_HEVC_ENC ||
            enc->clientType == EWL_CLIENT_TYPE_H264_ENC ||
            enc->clientType == EWL_CLIENT_TYPE_AV1_ENC ||
            enc->clientType == EWL_CLIENT_TYPE_VP9_ENC) {
            pthread_mutex_lock(&ewl_refer_counter_mutex);
            if (coreWait.refer_counter > 0)
                coreWait.refer_counter--;

            if (coreWait.refer_counter == 0) {
                pthread_mutex_lock(&coreWait.job_mutex);
                coreWait.bFlush = HANTRO_TRUE;
                pthread_cond_signal(&coreWait.job_cond);
                pthread_mutex_unlock(&coreWait.job_mutex);
            }
            pthread_mutex_unlock(&ewl_refer_counter_mutex);
        }
        /* Release the register mapping info of each core */
        for (i = 0; i < EWLGetCoreNum(NULL); i++) {
            if (enc->reg_all_cores != NULL && enc->reg_all_cores[i].pRegBase != MAP_FAILED) {
                munmap((void *)enc->reg_all_cores[i].pRegBase, enc->reg_all_cores[i].regSize);
            }
        }

        EWLfree(enc->reg_all_cores);
        enc->reg_all_cores = NULL;

        /* Release the sram */
        if (enc->pLineBufSram != MAP_FAILED) {
            munmap((void *)enc->pLineBufSram, enc->lineBufSramSize);
        }

        free_nodes(enc->freelist.tail);
    } else {
        if ((u32 *)enc->vcmd_cmdbuf_info.cmd_virt_addr != MAP_FAILED) {
            munmap((u32 *)enc->vcmd_cmdbuf_info.cmd_virt_addr,
                   enc->vcmd_cmdbuf_info.cmd_total_size);
        }
        if ((u32 *)enc->vcmd_cmdbuf_info.status_virt_addr != MAP_FAILED) {
            munmap((u32 *)enc->vcmd_cmdbuf_info.status_virt_addr,
                   enc->vcmd_cmdbuf_info.status_total_size);
        }
    }

    free_nodes(enc->workers.tail);
    if (enc->fd_mem != -1)
        close(enc->fd_mem);
    if (enc->fd_enc != -1)
        close(enc->fd_enc);
    if (enc->fd_memalloc != -1)
        close(enc->fd_memalloc);
    EWLfree(enc);

    PTRACE_I("EWLRelease: instance freed\n");

    return EWL_OK;
}

void EWLWriteRegbyClientType(const void *inst, u32 offset, u32 val, u32 client_type)
{
    vc8000_cwl_t *enc;
    regMapping *reg;
    u32 core_id, core_type;

    enc = (vc8000_cwl_t *)inst;
    if (enc->vcmd_mode == VCMD_MODE_ENABLED) {
        return;
    }

    core_id = LAST_CORE(enc);

    core_type = EWLGetCoreTypeByClientType(client_type);
    reg = &(enc->reg_all_cores[core_id].core[core_type]);

    if (reg->core_id == -1)
        return;

    assert(reg != NULL && offset < reg->regSize);

    if (offset == 0x04) {
        //asic_status = val;
    }

    offset = offset / 4;
    *(reg->pRegBase + offset) = val;

    PTRACE_I("EWLWriteReg 0x%02x with value %08x\n", offset * 4, val);
}

/*******************************************************************************
 Function name   : EWLWriteReg
 Description     : Set the content of a hadware register
 Return type     : void
 Argument        : u32 offset
 Argument        : u32 val
*******************************************************************************/
void EWLWriteCoreReg(const void *inst, u32 offset, u32 val, u32 core_id)
{
    vc8000_cwl_t *enc;
    u32 core_type;
    regMapping *reg;

    enc = (vc8000_cwl_t *)inst;
    if (enc->vcmd_mode == VCMD_MODE_ENABLED) {
        return;
    }

    core_type = EWLGetCoreTypeByClientType(enc->clientType);
    reg = &(enc->reg_all_cores[core_id].core[core_type]);

    assert(reg != NULL && offset < reg->regSize);

    if (offset == 0x04) {
        //asic_status = val;
    }

    offset = offset / 4;
    *(reg->pRegBase + offset) = val;

    PTRACE_I("EWLWriteReg 0x%02x with value %08x\n", offset * 4, val);
}

void EWLWriteReg(const void *inst, u32 offset, u32 val)
{
    u32 core_id = 0;
    vc8000_cwl_t *enc;

    enc = (vc8000_cwl_t *)inst;
    if (enc->vcmd_mode == VCMD_MODE_DISABLED) {
        core_id = LAST_CORE(enc);
    }
    EWLWriteCoreReg(inst, offset, val, core_id);
}

void EWLWriteBackRegbyClientType(const void *inst, u32 offset, u32 val, u32 client_type)
{
    vc8000_cwl_t *enc;
    regMapping *reg;
    u32 core_id, core_type;

    enc = (vc8000_cwl_t *)inst;
    if (enc->vcmd_mode == VCMD_MODE_ENABLED) {
        return;
    }

    core_id = FIRST_CORE(enc);

    core_type = EWLGetCoreTypeByClientType(client_type);
    reg = &(enc->reg_all_cores[core_id].core[core_type]);

    if (reg->core_id == -1)
        return;

    assert(reg != NULL && offset < reg->regSize);

    if (offset == 0x04) {
        //asic_status = val;
    }

    offset = offset / 4;
    *(reg->pRegBase + offset) = val;

    PTRACE_I("EWLWriteReg 0x%02x with value %08x\n", offset * 4, val);
}

void EWLWriteBackReg(const void *inst, u32 offset, u32 val)
{
    u32 core_id = 0;
    vc8000_cwl_t *enc;

    enc = (vc8000_cwl_t *)inst;
    if (enc->vcmd_mode == VCMD_MODE_DISABLED) {
        core_id = FIRST_CORE(enc);
    }
    EWLWriteCoreReg(inst, offset, val, core_id);
}

/*------------------------------------------------------------------------------
    Function name   : EWLEnableHW
    Description     :
    Return type     : void
    Argument        : const void *inst
    Argument        : u32 offset
    Argument        : u32 val
------------------------------------------------------------------------------*/
void EWLEnableHW(const void *inst, u32 offset, u32 val)
{
    u32 core_id = 0;
    vc8000_cwl_t *enc;
    u32 core_type;
    regMapping *reg;

    enc = (vc8000_cwl_t *)inst;
    if (enc->vcmd_mode == VCMD_MODE_ENABLED) {
        return;
    }

    core_id = LAST_CORE(enc);
    core_type = EWLGetCoreTypeByClientType(enc->clientType);

#ifndef SUPPORT_SHARED_MMU
    if (enc->mmuEnable == 1) {
        ioctl(enc->fd_enc, HANTRO_IOCS_MMU_FLUSH, &core_id);
    }
#endif

    reg = &(enc->reg_all_cores[core_id].core[core_type]);

    assert(reg != NULL && offset < reg->regSize);

    offset = offset / 4;
    *(reg->pRegBase + offset) = val;

    PTRACE_I("EWLEnableHW 0x%02x with value %08x\n", offset * 4, val);
}

/*------------------------------------------------------------------------------
    Function name   : EWLDisableHW
    Description     :
    Return type     : void
    Argument        : const void *inst
    Argument        : u32 offset
    Argument        : u32 val
------------------------------------------------------------------------------*/
void EWLDisableHW(const void *inst, u32 offset, u32 val)
{
    u32 core_id = 0;
    vc8000_cwl_t *enc;
    u32 core_type;
    regMapping *reg;

    enc = (vc8000_cwl_t *)inst;
    if (enc->vcmd_mode == VCMD_MODE_ENABLED) {
        return;
    }

    core_id = FIRST_CORE(enc);
    core_type = EWLGetCoreTypeByClientType(enc->clientType);
    reg = &(enc->reg_all_cores[core_id].core[core_type]);

    assert(reg != NULL && offset < reg->regSize);

    offset = offset / 4;
    *(reg->pRegBase + offset) = val;

    PTRACE_I("EWLDisableHW 0x%02x with value %08x\n", offset * 4, val);
}

/*------------------------------------------------------------------------------
    Function name   : EWLGetPerformance
    Description     :
    Return type     : void
    Argument        : const void *inst
    Argument        : u32 offset
    Argument        : u32 val
------------------------------------------------------------------------------*/
u32 EWLGetPerformance(const void *inst)
{
    vc8000_cwl_t *enc;

    enc = (vc8000_cwl_t *)inst;
    return enc->performance;
}

u32 EWLReadRegbyClientType(const void *inst, u32 offset, u32 client_type)
{
    vc8000_cwl_t *enc;
    u32 val;
    u32 core_id;
    u32 core_type;
    regMapping *reg;

    enc = (vc8000_cwl_t *)inst;
    if (enc->vcmd_mode == VCMD_MODE_ENABLED) {
        return -1;
    }

    core_id = FIRST_CORE(enc);
    core_type = EWLGetCoreTypeByClientType(client_type);
    reg = &(enc->reg_all_cores[core_id].core[core_type]);
    assert(offset < reg->regSize);

    offset = offset / 4;
    val = *(reg->pRegBase + offset);
    PTRACE_I("EWLReadReg 0x%02x --> %08x\n", offset * 4, val);
    return val;
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
u32 EWLReadReg(const void *inst, u32 offset)
{
    vc8000_cwl_t *enc;
    u32 val;
    volatile u32 *status_addr;

    enc = (vc8000_cwl_t *)inst;

    if (enc->vcmd_mode == VCMD_MODE_DISABLED) {
        u32 core_id;
        u32 core_type;
        regMapping *reg;
        core_id = FIRST_CORE(enc);
        core_type = EWLGetCoreTypeByClientType(enc->clientType);
        reg = &(enc->reg_all_cores[core_id].core[core_type]);
        assert(offset < reg->regSize);
        status_addr = reg->pRegBase;

    } else {
        u16 cmdbufid;
        cmdbufid = FIRST_CMDBUF_ID(enc);
        status_addr = (u32 *)enc->vcmd_cmdbuf_info.status_virt_addr +
                      enc->vcmd_cmdbuf_info.status_unit_size / 4 * cmdbufid;
        status_addr += enc->vcmd_enc_core_info.submodule_main_addr / 2 / 4;
    }
    offset = offset / 4;
    val = *(status_addr + offset);
    PTRACE_I("EWLReadReg 0x%02x --> %08x\n", offset * 4, val);
    return val;
}

u32 EWLReadRegInit(const void *inst, u32 offset)
{
    vc8000_cwl_t *enc;
    u32 val;
    u32 *status_addr;

    enc = (vc8000_cwl_t *)inst;
    if (enc->vcmd_mode == VCMD_MODE_DISABLED) {
        return EWL_OK;
    }
    status_addr = enc->main_module_init_status_addr;
    offset = offset / 4;
    val = *(status_addr + offset);

    PTRACE_I("EWLReadReg 0x%02x --> %08x\n", offset * 4, val);

    return val;
}

/*------------------------------------------------------------------------------
Function name   : EWLmmapBuffer
Description     : mmap the bus address of VPU buffer to virtual address

Return type     : u32* - virtual address after mmap

Argument        : const void * instance - EWL instance
Argument        : MemallocParams params - parameters of memory alloc
Argument        : EWLLinearMem_t *buff - place where the allocated memory
                  buffer parameters are returned
------------------------------------------------------------------------------*/
static u32 *EWLmmapBuffer(const void *inst, MemallocParams params, EWLLinearMem_t *buff)
{
    vc8000_cwl_t *enc;
    enc = (vc8000_cwl_t *)inst;
    assert(enc != NULL);
    assert(buff != NULL);
    u32 *mmapAddr = MAP_FAILED;

    /* Map the bus address to virtual address */
    mmapAddr = (u32 *)mmap(0, buff->size, PROT_READ | PROT_WRITE, MAP_SHARED, enc->fd_mem,
                           params.bus_address);
    if ((buff->mem_type != EWL_MEM_TYPE_DPB) && (buff->mem_type != EWL_MEM_TYPE_VPU_ONLY)) {
        if (mmapAddr == MAP_FAILED) {
            PTRACE_E("EWLmmapBuffer: Failed to mmap busAddress: %p size %x\n",
                     (void *)params.bus_address, sizeof(MemallocParams));
            return mmapAddr;
        }
    }
    return mmapAddr;
}

/*------------------------------------------------------------------------------
Function name   : EWLunmmapBuffer
Description     : unmmap the virtual address mmapped from bus address of VPU

Return type     : void

Argument        : u32 *virtualAddress - alignment virtual address
Argument        : u32 *allocVirtualAddr - allocated virtual address
Argument        : u32 size - size in byte of buffer
------------------------------------------------------------------------------*/
static void EWLunmmapBuffer(u32 *virtualAddress, u32 *allocVirtualAddr, u32 size)
{
    if (allocVirtualAddr != MAP_FAILED)
        munmap(allocVirtualAddr, size);
}

/*------------------------------------------------------------------------------
Function name   : EWLMMUMemoryMap
Description     : MMU mmap the VPU buffer

Return type     : void

Argument        : const void * instance - EWL instance
Argument        : MemallocParams params - parameters of memory alloc
Argument        : The parameter of alignment
Argument        : EWLLinearMem_t *buff - place where the allocated memory
                  buffer parameters are returned
------------------------------------------------------------------------------*/
static void EWLMMUMemoryMap(const void *inst, MemallocParams params, u32 alignment,
                            EWLLinearMem_t *buff)
{
    vc8000_cwl_t *enc;
    int ioctl_req;
    struct addr_desc addr;
    u32 *MMUMapAddr = NULL;
    u32 core_id;
    assert(buff != NULL);

#ifndef SUPPORT_MEM_SYNC
    MMUMapAddr = buff->allocVirtualAddr;
#else
    struct sync_mem *priv = (struct sync_mem *)buff->priv;
    if (priv->dev_va_alloc != MAP_FAILED)
        MMUMapAddr = priv->dev_va_alloc;
    else {
        MMUMapAddr = (u32 *)buff->allocBusAddr;
        PTRACE_I("EWLMMUMemoryMap map device memory not supported yet!\n");
        return;
    }
#endif

    enc = (vc8000_cwl_t *)inst;
    if (enc->mmuEnable == 1 && MMUMapAddr != MAP_FAILED) {
        addr.virtual_address = MMUMapAddr;
        addr.size = params.size;

        mlock(addr.virtual_address, addr.size);
        ioctl_req = (int)HANTRO_IOCS_MMU_MEM_MAP;
        ioctl(enc->fd_enc, ioctl_req, &addr);
        buff->busAddress = addr.bus_address;
        buff->busAddress = (buff->busAddress + (alignment - 1)) & (~(((u64)alignment) - 1));

#ifdef SUPPORT_SHARED_MMU
        core_id = 0;
        ioctl(enc->fd_enc, HANTRO_IOCS_MMU_FLUSH, &core_id);
#endif
    }
}

/*------------------------------------------------------------------------------
Function name   : EWLMMUMemoryUnMap
Description     : MMU unmmap the VPU buffer

Return type     : void

Argument        : const void * instance - EWL instance
Argument        : EWLLinearMem_t *buff - place where the allocated memory
                  buffer parameters are returned
------------------------------------------------------------------------------*/
static void EWLMMUMemoryUnMap(const void *inst, EWLLinearMem_t *buff)
{
    int ioctl_req;
    struct addr_desc addr;
    vc8000_cwl_t *enc;
    u32 *MMUunmapAddr = MAP_FAILED;
    assert(buff != NULL);

#ifndef SUPPORT_MEM_SYNC
    MMUunmapAddr = buff->allocVirtualAddr;
#else
    struct sync_mem *priv = (struct sync_mem *)buff->priv;
    if (priv->dev_va_alloc != MAP_FAILED)
        MMUunmapAddr = priv->dev_va_alloc;
    else {
        PTRACE_I("EWLMMUMemoryUnMap map device memory not supported yet!\n");
        return;
    }
#endif

    enc = (vc8000_cwl_t *)inst;
    if (enc->mmuEnable == 1 && MMUunmapAddr != MAP_FAILED) {
        addr.virtual_address = MMUunmapAddr;
        ioctl_req = (int)HANTRO_IOCS_MMU_MEM_UNMAP;
        ioctl(enc->fd_enc, ioctl_req, &addr);
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
i32 EWLMallocRefFrm(const void *inst, u32 size, u32 alignment, EWLLinearMem_t *info)
{
    vc8000_cwl_t *enc;
    i32 ret;
    EWLLinearMem_t *buff;

    enc = (vc8000_cwl_t *)inst;
    buff = (EWLLinearMem_t *)info;
    assert(enc != NULL);
    assert(buff != NULL);

    PTRACE_I("EWLMallocRefFrm\t%8d bytes\n", size);

    ret = EWLMallocLinear(enc, size, alignment, buff);

    PTRACE_I("EWLMallocRefFrm %p --> %p\n", (void *)buff->busAddress, buff->virtualAddress);
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
void EWLFreeRefFrm(const void *inst, EWLLinearMem_t *info)
{
    vc8000_cwl_t *enc;
    EWLLinearMem_t *buff;

    enc = (vc8000_cwl_t *)inst;
    buff = (EWLLinearMem_t *)info;
    assert(enc != NULL);
    assert(buff != NULL);

    EWLFreeLinear(enc, buff);
    PTRACE_I("EWLFreeRefFrm\t%p\n", buff->virtualAddress);
}

/*------------------------------------------------------------------------------
Function name   : EWLMallocLinear
Description     : Allocate a contiguous, linear RAM  memory buffer

Return type     : i32 - 0 for success or a negative error code

Argument        : const void * instance - EWL instance
Argument        : u32 size - size in bytes of the requested memory
Argument        : EWLLinearMem_t *info - place where the allocated memory
                  buffer parameters are returned
------------------------------------------------------------------------------*/
i32 EWLMallocLinear(const void *inst, u32 size, u32 alignment, EWLLinearMem_t *info)
{
    vc8000_cwl_t *enc;
    EWLLinearMem_t *buff;
    MemallocParams params;
    u32 pgsize;
    i32 ret;
    enc = (vc8000_cwl_t *)inst;
    buff = (EWLLinearMem_t *)info;
    assert(enc != NULL);
    assert(buff != NULL);
    pgsize = getpagesize();

    PTRACE_I("EWLMallocLinear\t%8d bytes\n", size);

    if (alignment == 0)
        alignment = 1;

    buff->size = buff->total_size =
        (((size + (alignment - 1)) & (~(alignment - 1))) + (pgsize - 1)) & (~(pgsize - 1));
    params.size = (size + (alignment - 1)) & (~(alignment - 1));

    buff->virtualAddress = 0;
    buff->busAddress = 0;
    buff->allocVirtualAddr = 0;
    buff->allocBusAddr = 0;
    params.mem_type = buff->mem_type;
    /* get memory linear memory buffers */
    ioctl(enc->fd_memalloc, MEMALLOC_IOCXGETBUFFER, &params);
    if (params.bus_address == 0) {
        PTRACE_E("EWLMallocLinear: Linear buffer not allocated\n");
        return EWL_ERROR;
    }
    /* ASIC might be in different address space */
    buff->allocBusAddr = BUS_CPU_TO_ASIC(params.bus_address, params.translation_offset);
    buff->busAddress = (buff->allocBusAddr + (alignment - 1)) & (~(((u64)alignment) - 1));
    if (sizeof(buff->busAddress) == 8 && (buff->busAddress >> 32) != 0) {
        PTRACE_I("EWLInit: allocated busAddress overflow 32 bit: (%p), please ensure HW "
                 "support 64bits address space\n",
                 (void *)params.bus_address);
    }

    buff->allocVirtualAddr = MAP_FAILED;
    ret = EWLMemSyncAllocHostBuffer((const void *)enc, buff->size, alignment, buff);
    if (ret == EWL_NOT_SUPPORT) {
        buff->allocVirtualAddr =
            EWLmmapBuffer(inst, params, buff); /* Map the bus address to virtual address */
        if (buff->allocVirtualAddr == MAP_FAILED)
            return EWL_ERROR;
        buff->virtualAddress = buff->allocVirtualAddr + (buff->busAddress - buff->allocBusAddr);
    }
#ifdef MEM_SYNC_TEST //The premise is open SUPPORT_MEM_SYNC, otherwise, it doesn't make sense
    struct sync_mem *priv = (struct sync_mem *)buff->priv;
    priv->dev_va_alloc = MAP_FAILED;
    if (buff->virtualAddress) { //When open MEM_SYNC,It indicates that there must be a buffer on the host side,
        //If necessary test Memory sync, can map the buffer on the device side to priv
        priv->dev_pa_alloc = params.bus_address;
        priv->dev_va_alloc = EWLmmapBuffer(inst, params, buff);
        priv->dev_va = priv->dev_va_alloc + (buff->busAddress - buff->allocBusAddr);
    }
#endif

    EWLMMUMemoryMap(inst, params, alignment, buff);

#ifdef SUPPORT_MEM_STATISTIC
    pthread_mutex_lock(&linearBufferCountMutex);
    memoryInfo.maxLinearMallocTimes++;
    memoryInfo.linearMallocTimes++;
    if (memoryInfo.averageLinearMallocTimes < memoryInfo.linearMallocTimes)
        memoryInfo.averageLinearMallocTimes = memoryInfo.linearMallocTimes;
    memoryInfo.linearMemoryValue += buff->total_size;
    if (memoryInfo.maxLinearMemory < memoryInfo.linearMemoryValue)
        memoryInfo.maxLinearMemory = memoryInfo.linearMemoryValue;
    pthread_mutex_unlock(&linearBufferCountMutex);
#endif

    PTRACE_I("EWLMallocLinear %p (CPU) %p (ASIC) --> %p\n", (void *)params.bus_address,
             (void *)buff->busAddress, buff->virtualAddress);

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
void EWLFreeLinear(const void *inst, EWLLinearMem_t *info)
{
    vc8000_cwl_t *enc;
    EWLLinearMem_t *buff;
    i32 ret;

    enc = (vc8000_cwl_t *)inst;
    buff = (EWLLinearMem_t *)info;
    assert(enc != NULL);
    assert(buff != NULL);

    EWLMMUMemoryUnMap(inst, buff);

#ifdef MEM_SYNC_TEST
    struct sync_mem *priv = (struct sync_mem *)buff->priv;
    EWLunmmapBuffer(priv->dev_va, priv->dev_va_alloc, buff->size);
    priv->dev_va = NULL;
    priv->dev_va_alloc = NULL;
    PTRACE_I("EWLFreeLinear VPU buffer for Memory Sync Test\t%p\n", priv->dev_va_alloc);
#endif

    ret = EWLMemSyncFreeHostBuffer((const void *)enc, buff);
    if (ret == EWL_NOT_SUPPORT) {
        EWLunmmapBuffer(buff->virtualAddress, buff->allocVirtualAddr, buff->size);
        buff->virtualAddress = NULL;
        buff->allocVirtualAddr = NULL;
    }

    if (buff->allocBusAddr != 0) {
        ioctl(enc->fd_memalloc, MEMALLOC_IOCSFREEBUFFER, &buff->allocBusAddr);
        buff->allocBusAddr = 0;
        buff->busAddress = 0;
    }

#ifdef SUPPORT_MEM_STATISTIC
    if (buff->total_size != 0) {
        pthread_mutex_lock(&linearBufferCountMutex);
        memoryInfo.linearMallocTimes--;
        memoryInfo.linearMemoryValue -= buff->total_size;
        pthread_mutex_unlock(&linearBufferCountMutex);
    }
#endif

    PTRACE_I("EWLFreeLinear\t%p\n", buff->allocVirtualAddr);
    buff->size = 0;
    buff->total_size = 0;
}

/*******************************************************************************
 Function name   : EWLGetDec400Coreid
 Description     : get one dec400 core id
*******************************************************************************/
i32 EWLGetDec400Coreid(const void *inst)
{
    vc8000_cwl_t *enc;
    u32 val;
    u32 *status_addr;
    u32 core_id = -1;
    u32 i;

    enc = (vc8000_cwl_t *)inst;

    if (enc->vcmd_mode == VCMD_MODE_DISABLED) {
        for (i = 0; i < EWLGetCoreNum(NULL); i++) {
            if (enc->reg_all_cores[i].core[CORE_DEC400].core_id != -1) {
                core_id = i;
                return core_id;
            }
        }
    } else {
        status_addr = (u32 *)enc->vcmd_cmdbuf_info.status_virt_addr +
                      enc->vcmd_cmdbuf_info.status_unit_size / 4 *
                          enc->vcmd_enc_core_info.config_status_cmdbuf_id;
        status_addr += enc->vcmd_enc_core_info.submodule_dec400_addr / 2 / 4;
        val = *(status_addr + 0x2a);
        if (val == 0x01004000 || val == 0x01004002)
            return 0;
        else
            return -1;
    }
    return core_id;
}

/*******************************************************************************
 Function name   : EWLGetDec400CustomerID
 Description     : get dec400 customer id
*******************************************************************************/
u32 EWLGetDec400CustomerID(const void *inst, u32 core_id)
{
    vc8000_cwl_t *enc;
    u32 dec400_customer_reg = 0;
    u32 *status_addr;

    enc = (vc8000_cwl_t *)inst;

    if (enc->vcmd_mode == VCMD_MODE_DISABLED) {
        regMapping *reg;
        reg = &(enc->reg_all_cores[core_id].core[CORE_DEC400]);
        dec400_customer_reg = *((u32 *)((u8 *)reg->pRegBase + 0x30));
    } else {
        status_addr = (u32 *)enc->vcmd_cmdbuf_info.status_virt_addr +
                      enc->vcmd_cmdbuf_info.status_unit_size / 4 *
                          enc->vcmd_enc_core_info.config_status_cmdbuf_id;
        status_addr += enc->vcmd_enc_core_info.submodule_dec400_addr / 2 / 4;
        dec400_customer_reg = *(status_addr + 0x0c);
    }

    return dec400_customer_reg;
}
/*******************************************************************************
 Function name   : EWLGetDec400Attribte
 Description     : get dec400 attribute
*******************************************************************************/
void EWLGetDec400Attribute(const void *inst, u32 *tile_size, u32 *bits_tile_in_table,
                           u32 *planar420_cbcr_arrangement_style)
{
#ifdef SUPPORT_DEC400
    VCDec4000Attribute dec400_attr;

    GetDec400Attribute(inst, &dec400_attr);
    *tile_size = dec400_attr.tile_size;
    *bits_tile_in_table = dec400_attr.bits_tile_in_table;
    *planar420_cbcr_arrangement_style = dec400_attr.planar420_cbcr_arrangement_style;
#endif
}

void EWLSetReserveBaseData(const void *inst, u32 width, u32 height, u32 rdoLevel, u32 bRDOQEnable,
                           u32 client_type, u16 priority)
{
    vc8000_cwl_t *enc;

    enc = (vc8000_cwl_t *)inst;
    if (enc->vcmd_mode == VCMD_MODE_DISABLED) {
        return;
    }
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
        enc->reserve_cmdbuf_info.module_type = enc->vcmd_enc_core_info.module_type;
        break;
    default:
        break;
    }
}

void EWLCollectWriteRegData(const void *inst, u32 *src, u32 *dst, u16 reg_start, u32 reg_length,
                            u32 *total_length)
{
    vc8000_cwl_t *enc;
    u16 reg_base;

    enc = (vc8000_cwl_t *)inst;
    if (enc->vcmd_mode == VCMD_MODE_DISABLED) {
        return;
    }
    reg_base = enc->vcmd_enc_core_info.submodule_main_addr / 4;
    CWLCollectWriteRegData(src, dst, reg_base + reg_start, reg_length, total_length);
    return;
}

void EWLCollectNopData(const void *inst, u32 *dst, u32 *total_length)
{
    vc8000_cwl_t *enc;

    enc = (vc8000_cwl_t *)inst;
    if (enc->vcmd_mode == VCMD_MODE_DISABLED) {
        return;
    }

    CWLCollectNopData(dst, total_length);
    return;
}

void EWLCollectStallDataEncVideo(const void *inst, u32 *dst, u32 *total_length)
{
    vc8000_cwl_t *enc;
    u32 interruput_mask;

    enc = (vc8000_cwl_t *)inst;
    if (enc->vcmd_mode == VCMD_MODE_DISABLED) {
        return;
    }

    interruput_mask = VC8000E_FRAME_RDY_INT_MASK;
    CWLCollectStallData(dst, total_length, interruput_mask);
    return;
}

void EWLCollectStallDataCuTree(const void *inst, u32 *dst, u32 *total_length)
{
    vc8000_cwl_t *enc;
    u32 interruput_mask;

    enc = (vc8000_cwl_t *)inst;
    if (enc->vcmd_mode == VCMD_MODE_DISABLED) {
        return;
    }

    interruput_mask = VC8000E_CUTREE_RDY_INT_MASK;
    CWLCollectStallData(dst, total_length, interruput_mask);
    return;
}

void EWLCollectReadRegData(const void *inst, u32 *dst, u16 reg_start, u32 reg_length,
                           u32 *total_length, u16 cmdbuf_id)
{
    vc8000_cwl_t *enc;
    u16 reg_base;
    ptr_t status_data_base_addr;

    enc = (vc8000_cwl_t *)inst;
    if (enc->vcmd_mode == VCMD_MODE_DISABLED) {
        return;
    }
    reg_base = enc->vcmd_enc_core_info.submodule_main_addr / 4;
    status_data_base_addr =
        enc->vcmd_cmdbuf_info.status_hw_addr + enc->vcmd_cmdbuf_info.status_unit_size * cmdbuf_id;

    status_data_base_addr += enc->vcmd_enc_core_info.submodule_main_addr / 2;
    CWLCollectReadRegData(dst, reg_base + reg_start, reg_length, total_length,
                          status_data_base_addr + reg_start * 4);
    return;
}

void EWLCollectIntData(const void *inst, u32 *dst, u32 *total_length, u16 cmdbuf_id)
{
    vc8000_cwl_t *enc;

    enc = (vc8000_cwl_t *)inst;
    if (enc->vcmd_mode == VCMD_MODE_DISABLED) {
        return;
    }

    CWLCollectIntData(dst, cmdbuf_id, total_length);
    return;
}

void EWLCollectJmpData(const void *inst, u32 *dst, u32 *total_length, u16 cmdbuf_id)
{
    vc8000_cwl_t *enc;

    enc = (vc8000_cwl_t *)inst;
    if (enc->vcmd_mode == VCMD_MODE_DISABLED) {
        return;
    }

    CWLCollectJmpData(dst, total_length, cmdbuf_id);
    return;
}

void EWLCollectClrIntData(const void *inst, u32 *dst, u32 *total_length)
{
    vc8000_cwl_t *enc;
    u16 reg_base;
    u16 clear_type;
    u32 bitmask;

    enc = (vc8000_cwl_t *)inst;
    if (enc->vcmd_mode == VCMD_MODE_DISABLED) {
        return;
    }

    clear_type = CLRINT_OPTYPE_READ_WRITE_1_CLEAR;
    reg_base = enc->vcmd_enc_core_info.submodule_main_addr / 4;
    bitmask = 0xffff;
    CWLCollectClrIntData(dst, clear_type, reg_base + 1, bitmask, total_length);
    return;
}

void EWLCollectClrIntReadClearDec400Data(const void *inst, u32 *dst, u32 *total_length,
                                         u16 addr_offset)
{
    vc8000_cwl_t *enc;
    u16 reg_base;
    u16 clear_type;
    u32 bitmask;

    enc = (vc8000_cwl_t *)inst;
    if (enc->vcmd_mode == VCMD_MODE_DISABLED) {
        return;
    }

    clear_type = CLRINT_OPTYPE_READ_CLEAR;
    reg_base = enc->vcmd_enc_core_info.submodule_dec400_addr / 4;
    bitmask = 0xffff;
    CWLCollectClrIntData(dst, clear_type, reg_base + addr_offset, bitmask, total_length);
    return;
}

void EWLCollectStopHwData(const void *inst, u32 *dst, u32 *total_length)
{
    vc8000_cwl_t *enc;
    u16 reg_base;
    u16 clear_type;
    u32 bitmask;

    enc = (vc8000_cwl_t *)inst;
    if (enc->vcmd_mode == VCMD_MODE_DISABLED) {
        return;
    }
    clear_type = CLRINT_OPTYPE_READ_WRITE_1_CLEAR;
    reg_base = enc->vcmd_enc_core_info.submodule_main_addr / 4;
    bitmask = 0xfffe;
    CWLCollectClrIntData(dst, clear_type, reg_base + ASIC_REG_INDEX_STATUS, bitmask, total_length);
    return;
}

// this function is for vcmd.
void EWLCollectReadVcmdRegData(const void *inst, u32 *dst, u16 reg_start, u32 reg_length,
                               u32 *total_length, u16 cmdbuf_id)
{
    vc8000_cwl_t *enc;
    u16 reg_base;
    ptr_t status_data_base_addr = 0;

    enc = (vc8000_cwl_t *)inst;
    if (enc->vcmd_mode == VCMD_MODE_DISABLED) {
        return;
    }

    reg_base = 0;
    if (enc->vcmd_enc_core_info.vcmd_hw_version_id > HW_ID_1_0_C) {
        CWLCollectReadRegData(dst, reg_base + reg_start, reg_length, total_length,
                              status_data_base_addr + reg_start * 4);
    } else {
        *total_length = 0;
    }
    return;
}

//this function is for dec400
void EWLCollectWriteDec400RegData(const void *inst, u32 *src, u32 *dst, u16 reg_start,
                                  u32 reg_length, u32 *total_length)
{
    vc8000_cwl_t *enc;
    u16 reg_base;

    enc = (vc8000_cwl_t *)inst;
    if (enc->vcmd_mode == VCMD_MODE_DISABLED) {
        return;
    }
    reg_base = enc->vcmd_enc_core_info.submodule_dec400_addr / 4;
    CWLCollectWriteRegData(src, dst, reg_base + reg_start, reg_length, total_length);
    return;
}

//this function is for dec400
void EWLCollectStallDec400(const void *inst, u32 *dst, u32 *total_length)
{
    vc8000_cwl_t *enc;
    u32 interruput_mask;

    enc = (vc8000_cwl_t *)inst;
    if (enc->vcmd_mode == VCMD_MODE_DISABLED) {
        return;
    }

    interruput_mask = VC8000E_DEC400_INT_MASK;
    CWLCollectStallData(dst, total_length, interruput_mask);
    return;
}

void EWLCollectWriteMMURegData(const void *inst, u32 *dst, u32 *total_length)
{
    vc8000_cwl_t *enc;
    u16 reg_base;
    u32 current_length = 0;
    u16 reg_start = 97;
    u32 reg_length = 2;
    u32 mmu_ctrl[2] = {0x10, 0x00};
    u32 *src;

#ifdef SUPPORT_SHARED_MMU
    return;
#endif

    enc = (vc8000_cwl_t *)inst;
    if (enc->mmuEnable != 1)
        return;

    src = mmu_ctrl;

    if (enc->vcmd_mode == VCMD_MODE_DISABLED) {
        return;
    }
    reg_base = enc->vcmd_enc_core_info.submodule_MMU_addr[0] / 4;
    CWLCollectWriteRegData(src, dst + *total_length, reg_base + reg_start, 1, &current_length);
    *total_length += current_length;
    //CWLCollectNopData(dst+*total_length, &current_length);
    //*total_length += current_length;
    CWLCollectWriteRegData(src + 1, dst + *total_length, reg_base + reg_start, 1, &current_length);
    *total_length += current_length;

    if (enc->vcmd_enc_core_info.submodule_MMU_addr[1] != 0xffff) {
        reg_base = enc->vcmd_enc_core_info.submodule_MMU_addr[1] / 4;
        CWLCollectWriteRegData(src, dst + *total_length, reg_base + reg_start, 1, &current_length);
        *total_length += current_length;
        //CWLCollectNopData(dst+*total_length, &current_length);
        //*total_length += current_length;
        CWLCollectWriteRegData(src + 1, dst + *total_length, reg_base + reg_start, 1,
                               &current_length);
        *total_length += current_length;
    }
}

/*******************************************************************************
 Function name   : EWLReserveHw
 Description     : Reserve HW resource for currently running codec
*******************************************************************************/
i32 EWLReserveCmdbuf(const void *inst, u16 size, u16 *cmdbufid)
{
    vc8000_cwl_t *enc;
    i32 ret;
    struct exchange_parameter *exchange_data;

    enc = (vc8000_cwl_t *)inst;
    /* Check invalid parameters */
    if (enc == NULL)
        return EWL_ERROR;

    if (enc->vcmd_mode == VCMD_MODE_DISABLED) {
        return EWL_OK;
    }

    exchange_data = &enc->reserve_cmdbuf_info;
    exchange_data->cmdbuf_size = size * 4;

    PTRACE_I("EWLReserveCmdbufHw: PID %d trying to reserve ...\n", GETPID());

    ret = ioctl(enc->fd_enc, HANTRO_IOCH_RESERVE_CMDBUF, exchange_data);

    if (ret < 0) {
        PTRACE_E("EWLReserveCmdbuf failed\n");
        return EWL_ERROR;
    } else {
        EWLWorker *worker = EWLmalloc(sizeof(EWLWorker));
        worker->cmdbuf_id = exchange_data->cmdbuf_id;
        worker->next = NULL;
        queue_put(&enc->workers, (struct node *)worker);
        *cmdbufid = exchange_data->cmdbuf_id;
        PTRACE_I("EWLReserveCmdbuf successed\n");
    }

    PTRACE_I("EWLReserveCmdbuf: ENC cmdbuf locked by PID %d\n", GETPID());
    return EWL_OK;
}

void EWLCopyDataToCmdbuf(const void *inst, u32 *src, u32 size, u16 cmdbuf_id)
{
    vc8000_cwl_t *enc;

    enc = (vc8000_cwl_t *)inst;
    if (enc->vcmd_mode == VCMD_MODE_DISABLED) {
        return;
    }

    EWLmemcpy((u32 *)enc->vcmd_cmdbuf_info.cmd_virt_addr +
                  enc->vcmd_cmdbuf_info.cmd_unit_size / 4 * cmdbuf_id,
              src, size * 4);
}

/*******************************************************************************
 Function name   : EWLLinkRunCmdbuf
 Description     : link and run current cmdbuf
*******************************************************************************/
i32 EWLLinkRunCmdbuf(const void *inst, u16 cmdbufid, u16 cmdbuf_size)
{
    vc8000_cwl_t *enc;
    i32 ret;
    u16 core_info_hw;
    struct exchange_parameter *exchange_data;

    enc = (vc8000_cwl_t *)inst;
    /* Check invalid parameters */
    if (enc == NULL)
        return EWL_ERROR;

    if (enc->vcmd_mode == VCMD_MODE_DISABLED) {
        return EWL_OK;
    }
    core_info_hw = cmdbufid;
    exchange_data = &enc->reserve_cmdbuf_info;
    if (cmdbufid != exchange_data->cmdbuf_id)
        return EWL_ERROR;

    PTRACE_I("EWLLinkRunCmdbuf: PID %d trying to link and  run cmdbuf ...\n", GETPID());
    exchange_data->cmdbuf_size = cmdbuf_size * 4;
    ret = ioctl(enc->fd_enc, HANTRO_IOCH_LINK_RUN_CMDBUF, exchange_data);

    if (ret < 0) {
        PTRACE_E("EWLLinkRunCmdbuf failed\n");
        return EWL_ERROR;
    } else {
        PTRACE_I("EWLLinkRunCmdbuf successed\n");
    }

    PTRACE_I("EWLLinkRunCmdbuf:  cmdbuf locked by PID %d\n", GETPID());

    return EWL_OK;
}

/*******************************************************************************
 Function name   : EWLWaitCmdbuf
 Description     : wait cmdbuf run done
*******************************************************************************/
i32 EWLWaitCmdbuf(const void *inst, u16 cmdbufid, u32 *status)
{
    vc8000_cwl_t *enc;
    i32 ret = 0;
    u16 core_info_hw;
    u32 *ddr_addr = NULL;
#ifdef POLLING_ISR
    pthread_attr_t attr;
    pthread_t tid;
#endif

    enc = (vc8000_cwl_t *)inst;
    core_info_hw = cmdbufid;

    /* Check invalid parameters */
    if (enc == NULL)
        return EWL_ERROR;

    if (enc->vcmd_mode == VCMD_MODE_DISABLED) {
        return EWL_OK;
    }

#ifdef POLLING_ISR
    {
        enc->wait_core_id_polling = enc->reserve_cmdbuf_info.core_id;
        enc->wait_polling_break = 0;
        pthread_attr_init(&attr);
        pthread_create(&tid, &attr, (void *)polling_vcmd_isr, (void *)enc);
        pthread_attr_destroy(&attr);
    }
#endif
    PTRACE_I("EWLWaitCmdbuf: PID %d wait cmdbuf ...\n", GETPID());
    ret = ioctl(enc->fd_enc, HANTRO_IOCH_WAIT_CMDBUF, &core_info_hw);

    if (ret < 0) {
        PTRACE_E("EWLWaitCmdbuf failed\n");
        *status = 0;
#ifdef POLLING_ISR
        {
            enc->wait_polling_break = 1;
            pthread_join(tid, NULL);
        }
#endif

        return EWL_HW_WAIT_ERROR;
    } else {
        PTRACE_I("EWLWaitCmdbuf successed\n");
        ddr_addr = (u32 *)enc->vcmd_cmdbuf_info.status_virt_addr +
                   enc->vcmd_cmdbuf_info.status_unit_size / 4 * cmdbufid;
        ddr_addr += enc->vcmd_enc_core_info.submodule_main_addr / 2 / 4;
        *status = *(ddr_addr + 1);
    }
#ifdef POLLING_ISR
    {
        enc->wait_polling_break = 1;
        pthread_join(tid, NULL);
    }
#endif
    PTRACE_I("EWLWaitCmdbuf:  cmdbuf locked by PID %d\n", GETPID());

    return EWL_OK;
}

void EWLGetRegsByCmdbuf(const void *inst, u16 cmdbufid, u32 *regMirror)
{
    vc8000_cwl_t *enc;
    enc = (vc8000_cwl_t *)inst;
    i32 ret;
    u32 *ddr_addr = NULL;
    PTRACE_I("EncGetRegsByCmdbuf: PID %d wait cmdbuf ...\n", GETPID());

    ddr_addr = (u32 *)enc->vcmd_cmdbuf_info.status_virt_addr +
               enc->vcmd_cmdbuf_info.status_unit_size / 4 * cmdbufid;
    ddr_addr += enc->vcmd_enc_core_info.submodule_main_addr / 2 / 4;
    EWLmemcpy(regMirror, ddr_addr, ASIC_SWREG_AMOUNT * sizeof(u32));

    PTRACE_I("EncGetRegsByCmdbuf:  cmdbuf locked by PID %d\n", GETPID());
}

/*******************************************************************************
 Function name   : EWLReleaseCmdbuf
 Description     : wait cmdbuf run done
*******************************************************************************/
i32 EWLReleaseCmdbuf(const void *inst, u16 cmdbufid)
{
    vc8000_cwl_t *enc;
    i32 ret;
    u16 core_info_hw;

    enc = (vc8000_cwl_t *)inst;
    core_info_hw = cmdbufid;

    /* Check invalid parameters */
    if (enc == NULL)
        return EWL_ERROR;

    if (enc->vcmd_mode == VCMD_MODE_DISABLED) {
        return EWL_OK;
    }

    PTRACE_I("EWLReleaseCmdbuf: PID %d wait cmdbuf ...\n", GETPID());
    enc->performance = EWLReadReg(inst, 82 * 4); //save the performance before release hw

    ret = ioctl(enc->fd_enc, HANTRO_IOCH_RELEASE_CMDBUF, &core_info_hw);

    if (ret < 0) {
        PTRACE_E("EWLReleaseCmdbuf failed\n");
        return EWL_ERROR;
    } else {
        EWLWorker *worker = (EWLWorker *)queue_get(&enc->workers);
        EWLfree(worker);
        PTRACE_I("EWLReleaseCmdbuf successed\n");
    }

    PTRACE_I("EWLReleaseCmdbuf:  cmdbuf locked by PID %d\n", GETPID());

    return EWL_OK;
}

/*******************************************************************************
 Function name   : EWLWaitHwRdy
 Description     : Poll the encoder interrupt register to notice IRQ
 Return type     : i32
 Argument        : void
*******************************************************************************/
#ifdef POLLING_ISR
i32 EWLWaitHwRdy(const void *inst, u32 *slicesReady, void *waitOut, u32 *status_register)
{
    vc8000_cwl_t *enc;
    regMapping *reg = NULL;
    volatile u32 irq_stats;
    u32 prevSlicesReady = 0;
    i32 ret = EWL_HW_WAIT_TIMEOUT;
    struct timespec t;
    u32 timeout = 1000; /* Polling interval in microseconds */
    int loop = 500;     /* How many times to poll before timeout */
    u32 wClr;
    u32 hwId = 0;
    u32 core_id;
    u32 core_type;

    enc = (vc8000_cwl_t *)inst;
    assert(enc != NULL);

    if (enc->vcmd_mode == VCMD_MODE_ENABLED) {
        return EWL_OK;
    }

    PTRACE_I("EWLWaitHwRdy\n");
    if (waitOut != NULL) {
        u32 i, loop_num = 20;
        for (i = 0; i < loop_num; i++) {
            if (-1 ==
                ioctl(enc->fd_enc, HANTRO_IOCG_ANYCORE_WAIT_POLLING, (CORE_WAIT_OUT *)waitOut)) {
                PTRACE_E("ioctl HANTRO_IOCG_ANYCORE_WAIT_POLLING failed\n");
                ret = EWL_HW_WAIT_ERROR;
                break;
            } else if (((CORE_WAIT_OUT *)waitOut)->irq_num != 0) {
                ret = EWL_OK;
                break;
            }
        }

        return ret;
    }

    core_id = FIRST_CORE(enc);
    core_type = EWLGetCoreTypeByClientType(enc->clientType);
    reg = &enc->reg_all_cores[core_id].core[core_type];
    hwId = reg->pRegBase[0];

    /* The function should return when a slice is ready */
    if (slicesReady)
        prevSlicesReady = *slicesReady;

    if (timeout == (u32)(-1)) {
        loop = -1;      /* wait forever (almost) */
        timeout = 1000; /* 1ms polling interval */
    }

    t.tv_sec = 0;
    t.tv_nsec = timeout - t.tv_sec * 1000;
    t.tv_nsec = 100 * 1000 * 1000;

    do {
        /* Get the number of completed slices from ASIC registers. */
        if (slicesReady)
            *slicesReady = (reg->pRegBase[7] >> 17) & 0xFF;

#ifdef PCIE_FPGA_VERI_LINEBUF
        /* Only for verification purpose, to test input line buffer with hardware handshake mode. */
        if (pollInputLineBufTestFunc)
            pollInputLineBufTestFunc();
#endif

        irq_stats = reg->pRegBase[1];
        PTRACE_I("EWLWaitHw: IRQ stat = %08x\n", irq_stats);

        if ((irq_stats & ASIC_STATUS_ALL)) {
            /* clear all IRQ bits. */
            wClr = (HW_ID_MAJOR_NUMBER(hwId) >= 0x61 || HW_PRODUCT_VC9000(hwId) ||
                    HW_PRODUCT_VC9000LE(hwId) ||
                    (HW_PRODUCT_SYSTEM60(hwId) && HW_ID_MINOR_NUMBER(hwId) >= 1))
                       ? irq_stats
                       : (irq_stats & (~(ASIC_STATUS_ALL | ASIC_IRQ_LINE)));
            EWLWriteBackReg(inst, 0x04, wClr);
            ret = EWL_OK;
            loop = 0;
        }

        if (slicesReady) {
            if (*slicesReady > prevSlicesReady) {
                ret = EWL_OK;
                /*loop = 0; */
            }
        }

        if (loop) {
            if (nanosleep(&t, NULL) != 0) {
                PTRACE_I("EWLWaitHw: Sleep interrupted!\n");
            }
        }
    } while (loop--);

    *status_register = irq_stats;

    asic_status = irq_stats; /* update the buffered asic status */

    if (slicesReady) {
        PTRACE_I("EWLWaitHw: slicesReady = %d\n", *slicesReady);
    }
    PTRACE_I("EWLWaitHw: asic_status = %x\n", asic_status);
    PTRACE_I("EWLWaitHw: OK!\n");

    return ret;
}
#else
i32 EWLWaitHwRdy(const void *inst, u32 *slicesReady, void *waitOut, u32 *status_register)
{
    vc8000_cwl_t *enc;
    i32 ret = EWL_HW_WAIT_OK;
    u32 prevSlicesReady = 0;
    u32 core_info = 0;
    u32 core_id = 0;
    u32 core_type;

    PTRACE_I("EWLWaitHw: Start\n");

    enc = (vc8000_cwl_t *)inst;
    assert(enc != NULL);
    core_type = EWLGetCoreTypeByClientType(enc->clientType);

    if (slicesReady)
        prevSlicesReady = *slicesReady;

    /* Check invalid parameters */
    if (enc == NULL) {
        assert(0);
        return EWL_HW_WAIT_ERROR;
    }

    if (enc->vcmd_mode == VCMD_MODE_ENABLED) {
        return EWL_OK;
    }

    if (waitOut != NULL) {
        if (-1 == ioctl(enc->fd_enc, HANTRO_IOCG_ANYCORE_WAIT, (CORE_WAIT_OUT *)waitOut)) {
            PTRACE_E("ioctl HANTRO_IOCG_ANYCORE_WAIT failed\n");
            ret = EWL_HW_WAIT_ERROR;
        } else
            ret = EWL_OK;

        return ret;
    }

    core_info |= (FIRST_CORE(enc) << 4);
    core_info |= core_type;

    ret = core_info;
    if ((core_id = ioctl(enc->fd_enc, HANTRO_IOCG_CORE_WAIT, &ret)) == -1) {
        PTRACE_E("ioctl HANTRO_IOCG_CORE_WAIT failed\n");
        ret = EWL_HW_WAIT_ERROR;
        goto out;
    }

    if (slicesReady)
        *slicesReady =
            (enc->reg_all_cores[FIRST_CORE(enc)].core[core_type].pRegBase[7] >> 17) & 0xFF;

    if (FIRST_CORE(enc) != core_id)
        ret = EWL_HW_WAIT_ERROR;
out:
    *status_register = ret;
    PTRACE_I("EWLWaitHw: OK!\n");

    return EWL_OK;
}
#endif

i32 EWLReserveHw(const void *inst, u32 *core_info, u32 *job_id)
{
    vc8000_cwl_t *enc;
    u32 c = 0;
    u32 i = 0, valid_num = 0;
    i32 ret;
    u8 subsys_mapping = 0;
    u32 core_info_hw;
    u32 core_type, job_idx;

    PTRACE_I("EWLReserveHw: PID %d trying to reserve ...\n", GETPID());

    enc = (vc8000_cwl_t *)inst;
    /* Check invalid parameters */
    if (enc == NULL)
        return EWL_ERROR;

    if (enc->vcmd_mode == VCMD_MODE_ENABLED) {
        return EWL_OK;
    }

    core_info_hw = *core_info;
    core_type = EWLGetCoreTypeByClientType(enc->clientType);
    if (enc->clientType == EWL_CLIENT_TYPE_JPEG_ENC)
        core_type = CORE_VC8000EJ;

    core_info_hw |= (core_type & 0xFF);
    ret = ioctl(enc->fd_enc, HANTRO_IOCH_ENC_RESERVE, &core_info_hw);

    if (ret < 0) {
        PTRACE_E("EWLReserveHw failed\n");
        return EWL_ERROR;
    } else {
        PTRACE_I("EWLReserveHw successed\n");
    }

    core_type = EWLGetCoreTypeByClientType(enc->clientType);
    subsys_mapping = (u8)(core_info_hw & 0xFF);

    if (job_id != NULL)
        *job_id = (core_info_hw >> 16);

    i = 0;
    while (subsys_mapping) {
        if (subsys_mapping & 0x1) {
            enc->reg.core_id = i;
            enc->reg.regSize = enc->reg_all_cores[i].core[core_type].regSize;
            enc->reg.regBase = enc->reg_all_cores[i].core[core_type].regBase;
            enc->reg.pRegBase = enc->reg_all_cores[i].core[core_type].pRegBase;
            PTRACE_I("core %d is reserved\n", i);
            break;
        }
        subsys_mapping = subsys_mapping >> 1;
        i++;
    }

    pthread_mutex_lock(&ewl_mutex);
    EWLWorker *worker = (EWLWorker *)queue_tail(&enc->freelist);
    while (worker && worker->core_id != enc->reg.core_id) {
        worker = (EWLWorker *)worker->next;
    }
    queue_remove(&enc->freelist, (struct node *)worker);
    queue_put(&enc->workers, (struct node *)worker);
    pthread_mutex_unlock(&ewl_mutex);

    EWLWriteReg(enc, 0x14, 0); //disable encoder

    PTRACE_I("EWLReserveHw: ENC HW locked by PID %d\n", GETPID());

    return EWL_OK;
}

/*******************************************************************************
 Function name   : EWLReleaseHw
 Description     : Release HW resource when frame is ready
*******************************************************************************/
void EWLReleaseHw(const void *inst)
{
    vc8000_cwl_t *enc;
    u32 val;
    u32 core_info = 0;
    u32 core_id;
    u32 core_type;

    enc = (vc8000_cwl_t *)inst;
    assert(enc != NULL);

    if (enc->vcmd_mode == VCMD_MODE_ENABLED) {
        return;
    }

    core_id = FIRST_CORE(enc);
    core_type = EWLGetCoreTypeByClientType(enc->clientType);

    if (enc->clientType == EWL_CLIENT_TYPE_JPEG_ENC)
        core_type = CORE_VC8000EJ;

    enc->performance = EWLReadReg(inst, 82 * 4); //save the performance before release hw

    core_info |= enc->unCheckPid << 31;
    core_info |= core_id << 4;
    core_info |= core_type;

    val = EWLReadReg(inst, 0x14);
    EWLWriteBackReg(inst, 0x14, val & (~0x01)); /* reset ASIC */
    enc->reg.core_id = -1;
    enc->reg.regSize = 0;
    enc->reg.regBase = 0;
    enc->reg.pRegBase = NULL;

    PTRACE_I("EWLReleaseHw: PID %d trying to release ...\n", GETPID());
    pthread_mutex_lock(&ewl_mutex);
    EWLWorker *worker = (EWLWorker *)queue_get(&enc->workers);
    queue_remove(&enc->workers, (struct node *)worker);
    queue_put(&enc->freelist, (struct node *)worker);
    pthread_mutex_unlock(&ewl_mutex);

    ioctl(enc->fd_enc, HANTRO_IOCH_ENC_RELEASE, &core_info);

    PTRACE_I("EWLReleaseHw: HW released by PID %d\n", GETPID());
    return;
}

/*******************************************************************************
 Function name   : EWLGetCoreNum
 Description     : Get the total num of cores
 Return type     : u32 ID
 Argument        : void
*******************************************************************************/
u32 EWLGetCoreNum(void *ctx)
{
    int fd_mem = -1, fd_enc = -1;
    static u32 core_num = 0;

    if (vcmd_supported == 1)
        return core_num;

    if (core_num == 0) {
        fd_mem = open(MEM_MODULE_PATH, O_RDONLY);
        if (fd_mem == -1) {
            PTRACE_E("EWLGetCoreNum: failed to open: %s\n", MEM_MODULE_PATH);
            goto end;
        }

        fd_enc = open(ENC_MODULE_PATH, O_RDONLY);
        if (fd_enc == -1) {
            PTRACE_E("EWLGetCoreNum: failed to open: %s\n", ENC_MODULE_PATH);
            goto end;
        }

        ioctl(fd_enc, HANTRO_IOCG_CORE_NUM, &core_num);
    end:
        if (fd_mem != -1)
            close(fd_mem);
        if (fd_enc != -1)
            close(fd_enc);
    }

    PTRACE_I("EWLGetCoreNum: %d\n", core_num);
    return core_num;
}

void EWLTraceProfile(const void *inst, void *prof_data, i32 qp, i32 poc)
{
}

void EWLDCacheRangeFlush(const void *inst, EWLLinearMem_t *info)
{
}

void EWLDCacheRangeRefresh(const void *inst, EWLLinearMem_t *info)
{
}

/*******************************************************************************
 Function name   : EWLSetVCMDMode
 Description     : Set VCMD work mode to bypass(0) or enable(1)
 Return type     : void
 Argument        : mode: 0-disable, 1-enable
*******************************************************************************/
void EWLSetVCMDMode(const void *inst, u32 mode)
{
    vc8000_cwl_t *enc;

    enc = (vc8000_cwl_t *)inst;
    assert(enc != NULL);

    if (vcmd_supported) {
        /* TBD, need to communicate with kernel driver to bypass/enable vcmd,
           then set enc->vcmd_mode correspondingly.
           For now, just disable this feature here */
        //enc->vcmd_mode = mode ? VCMD_MODE_ENABLED : VCMD_MODE_DISABLED;
        (void)mode;
    }
}

/*******************************************************************************
 Function name   : EWLGetVCMDMode
 Description     : Get VCMD work mode to bypass(0) or enable(1)
 Return type     : mode: 0-disable, 1-enable
 Argument        : -
*******************************************************************************/
u32 EWLGetVCMDMode(const void *inst)
{
    vc8000_cwl_t *enc;

    enc = (vc8000_cwl_t *)inst;
    assert(enc != NULL);

    return (enc->vcmd_mode == VCMD_MODE_DISABLED) ? 0 : 1;
}

u32 EWLGetVCMDSupport()
{
    return vcmd_supported;
}

void EWLAttach(void *ctx, int slice_idx, i32 vcmd_support)
{
    int fd_enc = -1;
    fd_enc = open(ENC_MODULE_PATH, O_RDONLY);
    if (fd_enc == -1) {
        PTRACE_E("EWLAttach: failed to open: %s\n", ENC_MODULE_PATH);
        return;
    }

    if (ioctl(fd_enc, HANTRO_IOCH_GET_VCMD_ENABLE, &vcmd_supported) == -1) {
        PTRACE_E("EWLAttach:ioctl failed\n");
    }
    close(fd_enc);
}

void EWLDetach()
{
    return;
}

u32 EWLReleaseEwlWorkerInst(const void *inst)
{
    return 0;
}

void EWLClearTraceProfile(const void *inst)
{
}
