/*------------------------------------------------------------------------------
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                                                                            --
--                            Verisilicon.                                    --
--                                                                            --
--                   (C) COPYRIGHT 2020 VERISILICON                           --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
------------------------------------------------------------------------------*/

#ifndef __VWL_H__
#define __VWL_H__

#include "base_type.h"
#include "osal.h"
#include "vcmdstruct.h"
#include "enc_vcmd.h"

#define ANY_CMDBUF_ID 0xFFFF

/*need to consider how many memory should be allocated for status.*/
struct qualityProfile {
    i32 frame_number;
    float total_bits;
    float total_Y_PSNR;
    float total_U_PSNR;
    float total_V_PSNR;
    float Y_PSNR_Max;
    float Y_PSNR_Min;
    float U_PSNR_Max;
    float U_PSNR_Min;
    float V_PSNR_Max;
    float V_PSNR_Min;
    double total_ssim;
    double total_ssim_y;
    double total_ssim_u;
    double total_ssim_v;
    i32 QP_Max;
    i32 QP_Min;
};

struct exchange_parameter {
    u32 executing_time; //input ;executing_time=encoded_image_size*(rdoLevel+1)*(rdoq+1);
    u16 module_type;    //input input vc8000e=0,IM=1,vc8000d=2.jpege=3, jpegd=4
    u16 cmdbuf_size;    //input, reserve is not used; link and run is input.
    u16 priority;       //input,normal=0, high/live=1
    u16 cmdbuf_id;      //output ,it is unique in driver.
    u16 core_id;        //just used for polling.
    u8 cmdbuf_reserve;
};

struct cmdbuf_mem_parameter {
    u32 *virt_cmdbuf_addr;
    ptr_t phy_cmdbuf_addr; //cmdbuf pool base physical address
    u32 cmdbuf_total_size; //cmdbuf pool total size in bytes.
    u16 cmdbuf_unit_size;  //one cmdbuf size in bytes. all cmdbuf have same size.
    u32 *virt_status_cmdbuf_addr;
    ptr_t phy_status_cmdbuf_addr; //status cmdbuf pool base physical address
    u32 status_cmdbuf_total_size; //status cmdbuf pool total size in bytes.
    u16 status_cmdbuf_unit_size; //one status cmdbuf size in bytes. all status cmdbuf have same size.
    ptr_t base_ddr_addr; //for pcie interface, hw can only access phy_cmdbuf_addr-pcie_base_ddr_addr.
    u32 *core_register_addr;
    u32 *core_register_addr_IM;
    u32 *core_register_addr_JPEG;
};

struct config_parameter {
    u16 module_type;         //input vc8000e=0,cutree=1,vc8000d=2，jpege=3, jpegd=4
    u16 vcmd_core_num;       //output, how many vcmd cores are there with corresponding module_type.
    u16 submodule_main_addr; //output,if submodule addr == 0xffff, this submodule does not exist.
    u16 submodule_dec400_addr; //output ,if submodule addr == 0xffff, this submodule does not exist.
    u16 submodule_L2Cache_addr; //output,if submodule addr == 0xffff, this submodule does not exist.
    u16 submodule_MMU_addr[2];  //output,if submodule addr == 0xffff, this submodule does not exist.
    u16 submodule_axife_addr[2]; //output,if submodule addr == 0xffff, this submodule does not exist.
    u16 config_status_cmdbuf_id; // output , this status comdbuf save the all register values read in driver init.//used for analyse configuration in cwl.
    u32 vcmd_hw_version_id;
    //u32* core_register_addr[MAX_VCMD_TYPE];
};

struct VwlMem {
    u32 *virtual_address;
    ptr_t bus_address;
    u32 size;
    u16 cmdbuf_id;
};

struct cmdbuf_obj {
    u32 module_type;    //current CMDBUF type: input vc8000e=0,IM=1,vc8000d=2，jpege=3, jpegd=4
    u32 priority;       //current CMDBUFpriority: normal=0, high=1
    u32 executing_time; //current CMDBUFexecuting_time=encoded_image_size*(rdoLevel+1)*(rdoq+1);
    u32 cmdbuf_size;    //current CMDBUF size
    u32 *cmdbuf_virtualAddress; //current CMDBUF start virtual address.
    ptr_t cmdbuf_busAddress;    //current CMDBUF start physical address.
    u32 *status_virtualAddress; //current status CMDBUF start virtual address.
    ptr_t status_busAddress;    //current status CMDBUF start physical address.
    u32 status_size;            //current status CMDBUF size
    u32 executing_status;       //current CMDBUF executing status.
    struct file *filp;          //file pointer in the same process.
    u16 core_id;                //which vcmd core is used.
    u16 cmdbuf_id; //used to manage CMDBUF in driver.It is a handle to identify cmdbuf.also is an interrupt vector.position in pool,same as status position.
    u8 cmdbuf_data_loaded; //0 means sw has not copied data into this CMDBUF; 1 means sw has copied data into this CMDBUF
    u8 cmdbuf_data_linked; //0 :not linked, 1:linked.
    u8 cmdbuf_run_done; //if 0,waiting for CMDBUF finish; if 1, op code in CMDBUF has finished one by one. HANTRO_VCMD_IOCH_WAIT_CMDBUF will check this variable.
    u8 cmdbuf_need_remove; // if 0, not need to remove CMDBUF; 1 CMDBUF can be removed if it is not the last CMDBUF;
    u8 cmdbuf_posted;        //if 1, the cmdbuf sem already posted to listener
    u8 has_end_cmdbuf;       //if 1, the last opcode is end opCode.
    u8 no_normal_int_cmdbuf; //if 1, JMP will not send normal interrupt.
    u8 has_nop_cmdbuf;       //if 1, the opcode before JMP is nop opCode.
    u8 cmdbuf_reserve;       //if 0, no reserve; if 1 reserve ;
    u32 waited;              // if 0, the cmd buf hasn't been waited, otherwise, has been waited.
};

typedef struct {
    u32 refFrmAlloc[MEM_CHUNKS];
    u32 linMemAlloc[MEM_CHUNKS];
    u32 refFrmChunks;
    u32 linMemChunks;
    u32 totalChunks;                /* Total malloc's */
    u32 *chunks[MEM_CHUNKS];        /* Returned by malloc */
    u32 *alignedChunks[MEM_CHUNKS]; /* Aligned */
    struct queue freelist;
    struct queue workers;
    u8 *streamBase;
    u8 *streamTempBuffer;
    u32 streamLength;
    u8 segmentAmount;
    i8 frameRdy;
    u32 clientType;
    struct qualityProfile prof;

    WorkInst winst;
    /*vcmd*/
    /* for access some cmdbuf */
    pthread_mutex_t lock;

    struct config_parameter vcmd_enc_core_info;
    struct cmdbuf_mem_parameter vcmd_cmdbuf_info;

    struct exchange_parameter reserve_cmdbuf_info;

    u32 *main_module_init_status_addr[MAX_VCMD_TYPE];
    u32 performance;
    u32 core_last_wait;

    u8 cmdbuf_used[TOTAL_DISCRETE_CMDBUF_NUM];
    u32 cmdbuf_used_pos;
    u32 global_cmdbuf_core_map[TOTAL_DISCRETE_CMDBUF_NUM];
    u16 vcmd_position[MAX_VCMD_TYPE];
    int vcmd_type_core_num[MAX_VCMD_TYPE];
    struct cmdbuf_obj cmdbuf_data[TOTAL_DISCRETE_CMDBUF_NUM];
    SysCore *core[MAX_HWCORE_NUM];
    int vcmd_core_num_total;
    u32 jpeg_shared;
} ewlSysInstance;

#define VCMD_HW_ID_1_0_C 0x43421001
#define VCMD_HW_ID_1_1_1 0x43421101
#define VCMD_HW_ID_1_1_2 0x43421102

typedef struct bi_list_node {
    void *data;
    struct bi_list_node *next;
    struct bi_list_node *previous;
} bi_list_node;

typedef struct bi_list {
    bi_list_node *head;
    bi_list_node *tail;
} bi_list;

struct hantrovcmd_dev {
    const void *vcmd_core;
    struct HwVcmdCoreConfig vcmd_core_cfg; //config of each core,such as base addr, irq,etc
    u32 formats;                           //formats supported in this core.
    u32 core_id;                           //vcmd core id for driver and sw internal use
    u32 id_in_type;
    u32 sw_cmdbuf_rdy_num;
    bi_list list_manager;
    volatile u8 *hwregs; /* IO mem base */
    u32 reg_mirror[ASIC_VCMD_SWREG_AMOUNT];
    u32 duration_without_int; //number of cmdbufs without interrupt.
    u8 working_state;
    u32 total_exe_time;
    u16 status_cmdbuf_id;              //used for analyse configuration in cwl.
    u32 hw_version_id;                 /*megvii 0x43421001, later 0x43421102*/
    u32 *vcmd_reg_mem_virtual_address; //start virtual address of vcmd registers memory of  CMDBUF.
    ptr_t vcmd_reg_mem_bus_address;    //start physical address of vcmd registers memory of  CMDBUF.
    u32 vcmd_reg_mem_size;             // size of vcmd registers memory of CMDBUF.
    u32 b_vcmd_rdy;                    //1: vcmd poll an interrupt
    sem_t *multi_vcmd_rdy;             //
};

i32 CmodelIoctlReserveCmdbuf(struct exchange_parameter *param);

i32 CmodelIoctlEnableCmdbuf(struct exchange_parameter *param);

i32 CmodelIoctlWaitCmdbuf(u16 *cmd_buf_id);

i32 CmodelIoctlReleaseCmdbuf(u32 cmd_buf_id);

i32 CmodelIoctlGetCmdbufParameter(struct cmdbuf_mem_parameter *vcmd_mem_params);

i32 CmodelIoctlGetVcmdParameter(struct config_parameter *vcmd_params);

i32 CmodelVcmdInit();

void CmodelVcmdRelease();
i32 CmodelVcmdCheck();

#endif //__VWL_H__
