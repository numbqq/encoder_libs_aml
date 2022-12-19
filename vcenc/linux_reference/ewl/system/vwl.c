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

#include "vsi_string.h"
#include "vwl.h"
#include "osal.h"
#include "enc_core.h"
#include "ewl_common.h"
#include "enc_vcmd.h"
#include "fifo.h"
#include "vcmdstruct.h"

#define VWL_CLIENT_TYPE_ALL 0xFFFFFFFF

#define OPCODE_WREG (0x01 << 27)
#define OPCODE_END (0x02 << 27)
#define OPCODE_NOP (0x03 << 27)
#define OPCODE_RREG (0x16 << 27)
#define OPCODE_INT (0x18 << 27)
#define OPCODE_JMP (0x19 << 27)
#define OPCODE_STALL (0x09 << 27)
#define OPCODE_CLRINT (0x1a << 27)
#define OPCODE_JMP_RDY0 (0x19 << 27)
#define OPCODE_JMP_RDY1 ((0x19 << 27) | (1 << 26))
#define JMP_IE_1 (1 << 25)
#define JMP_RDY_1 (1 << 26)

#define CMDBUF_PRIORITY_NORMAL 0
#define CMDBUF_PRIORITY_HIGH 1

#define EXECUTING_CMDBUF_ID_ADDR 26
#define VCMD_EXE_CMDBUF_COUNT 3

#define WORKING_STATE_IDLE 0
#define WORKING_STATE_WORKING 1
#define CMDBUF_EXE_STATUS_OK 0
#define CMDBUF_EXE_STATUS_CMDERR 1
#define CMDBUF_EXE_STATUS_BUSERR 2

#define IRQ_HANDLED 1

static void vcmd_reset_asic(struct hantrovcmd_dev *dev);
static void vcmd_reset_current_asic(struct hantrovcmd_dev *dev);

static int vcmd_isr_cb(int irq, void *dev_id);

static u32 vcmd_read_buf_cb(u32 *addr, void *dev_id);
static void vcmd_write_buf_cb(u32 *addr, u32 value, void *dev_id);

static u32 vcmd_read_reg_cb(void *core, u32 reg_idx, u32 core_type, void *dev_id);
static void vcmd_write_reg_cb(void *core, u32 reg_idx, u32 value, u32 core_type, void *dev_id);

static void vcmd_jmp_cb(void *core, u32 core_type, void *dev_id);
static void vcmd_stall_cb(void *core, u32 core_type, void *dev_id);

//#define VCMD_DEBUG_INTERNAL

//#define CMODEL_VCMD_DRIVER_DEBUG
#undef PDEBUG /* undef it, just in case */
#if defined(_MSC_VER)
#ifdef CMODEL_VCMD_DRIVER_DEBUG
/* This one for user space */
#define PDEBUG(fmt, args) fprintf(stderr, fmt, ##args)
#else
#define PDEBUG(fmt, args) fprintf(stderr, "\n") /* not debugging: nothing */
#endif

#else
#ifdef CMODEL_VCMD_DRIVER_DEBUG
/* This one for user space */
#define PDEBUG(fmt, args...) fprintf(stderr, fmt, ##args)
#else
#define PDEBUG(fmt, args...) /* not debugging: nothing */
#endif
#endif

/*for all vcmds, the core info should be listed here for subsequent use*/
struct vcmd_config vcmd_core_all_array[] = {
#include "vcmd_coreconfig.h"
};

struct vcmd_config vcmd_core_array[MAX_VCMD_NUMBER];

/*these size need to be modified according to hw config.*/
#define VCMD_ENCODER_REGISTER_SIZE (512 * 4)
#define VCMD_DECODER_REGISTER_SIZE (512 * 4)
#define VCMD_IM_REGISTER_SIZE (479 * 4)
#define VCMD_JPEG_ENCODER_REGISTER_SIZE (479 * 4)
#define VCMD_JPEG_DECODER_REGISTER_SIZE (512 * 4)

static struct VwlMem vcmd_buf_mem_pool;
static struct VwlMem vcmd_status_buf_mem_pool;
static struct VwlMem vcmd_registers_mem_pool;

static u16 cmdbuf_used[TOTAL_DISCRETE_CMDBUF_NUM];

/* use this sem to judge if there is available cmdbuf. */
static sem_t reserved_sem;
//for reserve
sem_t vcmd_reserve_cmdbuf_sem[MAX_VCMD_TYPE];

/* pthread cond wait. */
static pthread_cond_t global_vcmd_cond_wait;
static pthread_mutex_t global_vcmd_mutex;
static u32 global_vcmd_release = 0;

static u16 cmdbuf_used_pos;
static u16 cmdbuf_used_residual;

static struct hantrovcmd_dev *vcmd_manager[MAX_VCMD_TYPE][MAX_SAME_MODULE_TYPE_CORE_NUMBER];
bi_list_node *global_cmdbuf_node[TOTAL_DISCRETE_CMDBUF_NUM];

static u16 vcmd_position[MAX_VCMD_TYPE];
static int vcmd_core_num[MAX_VCMD_TYPE];
//this variable used for mc model. when get 0xffffffff in CmodelIoctlWaitCmdbuf(),
//we return the cmdbuf which finished firstly.
pthread_t vcmd_thread[MAX_VCMD_NUMBER];
static int vcmd_run_once[MAX_VCMD_NUMBER] = {0};

/* store cores info */
static struct hantrovcmd_dev *hantrovcmd_data = NULL;
static int total_vcmd_core_num = 0;
static u32 *basemem_reg[MAX_VCMD_TYPE][MAX_SAME_MODULE_TYPE_CORE_NUMBER] = {{NULL}};

/* NOTE: Don't use ',' in descriptions, because it is used as separator in csv
 * parsing. */
const regVcmdField_s asicVcmdRegisterDesc[] = {
#include "vcmdregistertable.h"
};

void init_bi_list(bi_list *list);
bi_list_node *bi_list_create_node(void);
void bi_list_free_node(bi_list_node *node);
void bi_list_insert_node_tail(bi_list *list, bi_list_node *current_node);
void bi_list_insert_node_before(bi_list *list, bi_list_node *base_node, bi_list_node *new_node);
void bi_list_remove_node(bi_list *list, bi_list_node *current_node);

static inline void vwl_set_register_mirror_value(u32 *reg_mirror, regVcmdName name, u32 value)
{
    const regVcmdField_s *field;
    u32 regVal;

    field = &asicVcmdRegisterDesc[name];

    /* Check that value fits in field */
    PDEBUG("field->name == name=%d/n", field->name == name);
    PDEBUG("((field->mask >> field->lsb) << field->lsb) == field->mask=%d/n",
           ((field->mask >> field->lsb) << field->lsb) == field->mask);
    PDEBUG("(field->mask >> field->lsb) >= value=%d/n", (field->mask >> field->lsb) >= value);
    PDEBUG("field->base < ASIC_VCMD_SWREG_AMOUNT * 4=%d/n",
           field->base < ASIC_VCMD_SWREG_AMOUNT * 4);

    /* Clear previous value of field in register */
    regVal = reg_mirror[field->base / 4] & ~(field->mask);

    /* Put new value of field in register */
    reg_mirror[field->base / 4] = regVal | ((value << field->lsb) & field->mask);
}

static inline u32 vwl_get_register_mirror_value(u32 *reg_mirror, regVcmdName name)
{
    const regVcmdField_s *field;
    u32 regVal;

    field = &asicVcmdRegisterDesc[name];

    /* Check that value fits in field */
    PDEBUG("field->name == name=%d/n", field->name == name);
    PDEBUG("((field->mask >> field->lsb) << field->lsb) == field->mask=%d/n",
           ((field->mask >> field->lsb) << field->lsb) == field->mask);
    PDEBUG("field->base < ASIC_VCMD_SWREG_AMOUNT * 4=%d/n",
           field->base < ASIC_VCMD_SWREG_AMOUNT * 4);

    regVal = reg_mirror[field->base / 4];
    regVal = (regVal & field->mask) >> field->lsb;

    return regVal;
}

void vwl_write_register_value(const void *vcmd_core, u32 *reg_mirror, regVcmdName name, u32 value)
{
    const regVcmdField_s *field;
    u32 regVal;
    field = &asicVcmdRegisterDesc[name];
    /* Check that value fits in field */
    PDEBUG("field->name == name=%d\n", field->name == name);
    PDEBUG("((field->mask >> field->lsb) << field->lsb) == field->mask=%d\n",
           ((field->mask >> field->lsb) << field->lsb) == field->mask);
    PDEBUG("(field->mask >> field->lsb) >= value=%d\n", (field->mask >> field->lsb) >= value);
    PDEBUG("field->base < ASIC_VCMD_SWREG_AMOUNT*4=%d\n", field->base < ASIC_VCMD_SWREG_AMOUNT * 4);

    /* Clear previous value of field in register */
    regVal = reg_mirror[field->base / 4] & ~(field->mask);

    /* Put new value of field in register */
    reg_mirror[field->base / 4] = regVal | ((value << field->lsb) & field->mask);

    /* write it into HW registers */
    AsicHwVcmdCoreWriteRegister(vcmd_core, field->base, reg_mirror[field->base / 4]);
}

u32 vwl_get_register_value(const void *vcmd_core, u32 *reg_mirror, regVcmdName name)
{
    const regVcmdField_s *field;
    u32 value;

    field = &asicVcmdRegisterDesc[name];

    PDEBUG("field->base < ASIC_VCMD_SWREG_AMOUNT * 4=%d\n",
           field->base < ASIC_VCMD_SWREG_AMOUNT * 4);

    value = reg_mirror[field->base / 4] = AsicHwVcmdCoreReadRegister(vcmd_core, field->base);
    value = (value & field->mask) >> field->lsb;

    return value;
}

#define vwl_set_addr_register_value(vcmd_core, reg_mirror, name, value)                            \
    do {                                                                                           \
        if (sizeof(ptr_t) == 8) {                                                                  \
            vwl_write_register_value((vcmd_core), (reg_mirror), name, (u32)((ptr_t)value));        \
            vwl_write_register_value((vcmd_core), (reg_mirror), name##_MSB,                        \
                                     (u32)(((ptr_t)value) >> 32));                                 \
        } else {                                                                                   \
            vwl_write_register_value((vcmd_core), (reg_mirror), name, (u32)((ptr_t)value));        \
        }                                                                                          \
    } while (0)

#define VWLGetAddrRegisterValue(vcmd_core, reg_mirror, name)                                       \
    ((sizeof(ptr_t) == 8)                                                                          \
         ? ((((ptr_t)vwl_get_register_value((vcmd_core), (reg_mirror), name)) |                    \
             (((ptr_t)vwl_get_register_value((vcmd_core), (reg_mirror), name##_MSB)) << 32)))      \
         : ((ptr_t)vwl_get_register_value((vcmd_core), (reg_mirror), (name))))

void init_bi_list(bi_list *list)
{
    list->head = NULL;
    list->tail = NULL;
}

bi_list_node *bi_list_create_node(void)
{
    bi_list_node *node = NULL;
    node = malloc(sizeof(bi_list_node));
    if (node == NULL) {
        PDEBUG("%s\n", "vmalloc for node fail!");
        return node;
    }
    memset(node, 0, sizeof(bi_list_node));
    return node;
}
void bi_list_free_node(bi_list_node *node)
{
    //free current node
    free(node);
    return;
}

void bi_list_insert_node_tail(bi_list *list, bi_list_node *current_node)
{
    if (current_node == NULL) {
        PDEBUG("%s\n", "insert node tail  NULL");
        return;
    }
    if (list->tail) {
        current_node->previous = list->tail;
        list->tail->next = current_node;
        list->tail = current_node;
        list->tail->next = NULL;
    } else {
        list->head = current_node;
        list->tail = current_node;
        current_node->next = NULL;
        current_node->previous = NULL;
    }
    return;
}

void bi_list_insert_node_before(bi_list *list, bi_list_node *base_node, bi_list_node *new_node)
{
    bi_list_node *temp_node_previous = NULL;
    if (new_node == NULL) {
        PDEBUG("%s\n", "insert node before new node NULL");
        return;
    }
    if (base_node) {
        if (base_node->previous) {
            //at middle position
            temp_node_previous = base_node->previous;
            temp_node_previous->next = new_node;
            new_node->next = base_node;
            base_node->previous = new_node;
            new_node->previous = temp_node_previous;
        } else {
            //at head
            base_node->previous = new_node;
            new_node->next = base_node;
            list->head = new_node;
            new_node->previous = NULL;
        }
    } else {
        //at tail
        bi_list_insert_node_tail(list, new_node);
    }
    return;
}

void bi_list_remove_node(bi_list *list, bi_list_node *current_node)
{
    bi_list_node *temp_node_previous = NULL;
    bi_list_node *temp_node_next = NULL;
    if (current_node == NULL) {
        PDEBUG("%s\n", "remove node NULL");
        return;
    }
    temp_node_next = current_node->next;
    temp_node_previous = current_node->previous;

    if (temp_node_next == NULL && temp_node_previous == NULL) {
        //there is only one node.
        list->head = NULL;
        list->tail = NULL;
    } else if (temp_node_next == NULL) {
        //at tail
        list->tail = temp_node_previous;
        temp_node_previous->next = NULL;
    } else if (temp_node_previous == NULL) {
        //at head
        list->head = temp_node_next;
        temp_node_next->previous = NULL;
    } else {
        //at middle position
        temp_node_previous->next = temp_node_next;
        temp_node_next->previous = temp_node_previous;
    }
    return;
}

/**********************************************************************************************************\
*cmdbuf object management
\***********************************************************************************************************/
static struct cmdbuf_obj *create_cmdbuf_obj(void)
{
    struct cmdbuf_obj *cmdbuf_obj = NULL;
    cmdbuf_obj = malloc(sizeof(struct cmdbuf_obj));
    if (cmdbuf_obj == NULL) {
        PDEBUG("%s\n", "malloc for cmdbuf_obj fail!");
        return cmdbuf_obj;
    }
    memset(cmdbuf_obj, 0, sizeof(struct cmdbuf_obj));
    return cmdbuf_obj;
}

static void free_cmdbuf_obj(struct cmdbuf_obj *cmdbuf_obj)
{
    if (cmdbuf_obj == NULL) {
        PDEBUG("%s\n", "remove_cmdbuf_obj NULL");
        return;
    }
    //free current cmdbuf_obj
    free(cmdbuf_obj);
    return;
}

//calculate executing_time of each vcmd
static u32 calculate_executing_time_after_node(bi_list_node *exe_cmdbuf_node)
{
    u32 time_run_all = 0;
    struct cmdbuf_obj *cmdbuf_obj_temp = NULL;
    while (1) {
        if (exe_cmdbuf_node == NULL)
            break;
        cmdbuf_obj_temp = (struct cmdbuf_obj *)exe_cmdbuf_node->data;
        time_run_all += cmdbuf_obj_temp->executing_time;
        exe_cmdbuf_node = exe_cmdbuf_node->next;
    }
    return time_run_all;
}
static u32 calculate_executing_time_after_node_high_priority(bi_list_node *exe_cmdbuf_node)
{
    u32 time_run_all = 0;
    struct cmdbuf_obj *cmdbuf_obj_temp = NULL;
    if (exe_cmdbuf_node == NULL)
        return time_run_all;
    cmdbuf_obj_temp = (struct cmdbuf_obj *)exe_cmdbuf_node->data;
    time_run_all += cmdbuf_obj_temp->executing_time;
    exe_cmdbuf_node = exe_cmdbuf_node->next;
    while (1) {
        if (exe_cmdbuf_node == NULL)
            break;
        cmdbuf_obj_temp = (struct cmdbuf_obj *)exe_cmdbuf_node->data;
        if (cmdbuf_obj_temp->priority == CMDBUF_PRIORITY_NORMAL)
            break;
        time_run_all += cmdbuf_obj_temp->executing_time;
        exe_cmdbuf_node = exe_cmdbuf_node->next;
    }
    return time_run_all;
}

static i32 allocate_cmdbuf(struct VwlMem *new_cmdbuf_addr, struct VwlMem *new_status_cmdbuf_addr)
{
    new_cmdbuf_addr->size = CMDBUF_MAX_SIZE;

    /* if there is not available cmdbuf, lock the function. */
    sem_wait(&reserved_sem);

    while (1) {
        if (cmdbuf_used[cmdbuf_used_pos] == 0 && (global_cmdbuf_node[cmdbuf_used_pos] == NULL)) {
            cmdbuf_used[cmdbuf_used_pos] = 1;
            cmdbuf_used_residual -= 1;
            new_cmdbuf_addr->cmdbuf_id = cmdbuf_used_pos;
            new_cmdbuf_addr->virtual_address =
                vcmd_buf_mem_pool.virtual_address + cmdbuf_used_pos * CMDBUF_MAX_SIZE / 4;
            new_cmdbuf_addr->bus_address =
                (ptr_t)new_cmdbuf_addr->virtual_address + cmdbuf_used_pos * CMDBUF_MAX_SIZE;

            new_status_cmdbuf_addr->virtual_address =
                vcmd_status_buf_mem_pool.virtual_address + cmdbuf_used_pos * CMDBUF_MAX_SIZE / 4;
            new_status_cmdbuf_addr->bus_address =
                (ptr_t)new_status_cmdbuf_addr->virtual_address + cmdbuf_used_pos * CMDBUF_MAX_SIZE;
            new_status_cmdbuf_addr->size = CMDBUF_MAX_SIZE;
            new_status_cmdbuf_addr->cmdbuf_id = cmdbuf_used_pos;
            cmdbuf_used_pos++;
            if (cmdbuf_used_pos >= TOTAL_DISCRETE_CMDBUF_NUM)
                cmdbuf_used_pos = 0;
            return 0;
        } else {
            cmdbuf_used_pos++;
            if (cmdbuf_used_pos >= TOTAL_DISCRETE_CMDBUF_NUM)
                cmdbuf_used_pos = 0;
        }
    }

    return -1;
}

static void free_cmdbuf_mem(u16 cmdbuf_id)
{
    cmdbuf_used[cmdbuf_id] = 0;
    cmdbuf_used_residual += 1;
    sem_post(&reserved_sem);
}

static bi_list_node *create_cmdbuf_node(void)
{
    bi_list_node *current_node = NULL;
    struct cmdbuf_obj *cmdbuf_obj = NULL;
    struct VwlMem new_cmdbuf_addr;
    struct VwlMem new_status_cmdbuf_addr;

    allocate_cmdbuf(&new_cmdbuf_addr, &new_status_cmdbuf_addr);

    cmdbuf_obj = create_cmdbuf_obj();
    if (cmdbuf_obj == NULL) {
        PDEBUG("%s\n", "create_cmdbuf_obj fail!");
        free_cmdbuf_mem(new_cmdbuf_addr.cmdbuf_id);
        return NULL;
    }
    cmdbuf_obj->cmdbuf_busAddress = new_cmdbuf_addr.bus_address;
    cmdbuf_obj->cmdbuf_virtualAddress = new_cmdbuf_addr.virtual_address;
    cmdbuf_obj->cmdbuf_size = new_cmdbuf_addr.size;
    cmdbuf_obj->cmdbuf_id = new_cmdbuf_addr.cmdbuf_id;
    cmdbuf_obj->status_busAddress = new_status_cmdbuf_addr.bus_address;
    cmdbuf_obj->status_virtualAddress = new_status_cmdbuf_addr.virtual_address;
    cmdbuf_obj->status_size = new_status_cmdbuf_addr.size;
    current_node = bi_list_create_node();
    if (current_node == NULL) {
        PDEBUG("%s\n", "bi_list_create_node fail!");
        free_cmdbuf_mem(new_cmdbuf_addr.cmdbuf_id);
        free_cmdbuf_obj(cmdbuf_obj);
        return NULL;
    }
    current_node->data = (void *)cmdbuf_obj;
    current_node->next = NULL;
    current_node->previous = NULL;
    return current_node;
}

static void free_cmdbuf_node(bi_list_node *cmdbuf_node)
{
    struct cmdbuf_obj *cmdbuf_obj = NULL;
    if (cmdbuf_node == NULL) {
        PDEBUG("%s\n", "remove_cmdbuf_node NULL");
        return;
    }
    cmdbuf_obj = (struct cmdbuf_obj *)cmdbuf_node->data;
    //free cmdbuf mem in pool
    free_cmdbuf_mem(cmdbuf_obj->cmdbuf_id);

    //free struct cmdbuf_obj
    free_cmdbuf_obj(cmdbuf_obj);
    //free current cmdbuf_node entity.
    bi_list_free_node(cmdbuf_node);
    return;
}

//just remove, not free the node.
static bi_list_node *remove_cmdbuf_node_from_list(bi_list *list, bi_list_node *cmdbuf_node)
{
    if (cmdbuf_node == NULL) {
        PDEBUG("%s\n", "remove_cmdbuf_node_from_list  NULL");
        return NULL;
    }

    if (cmdbuf_node->next) {
        bi_list_remove_node(list, cmdbuf_node);
        return cmdbuf_node;
    } else {
        //the last one, should not be removed.
        return NULL;
    }
}

static bi_list_node *get_cmdbuf_node_in_list_by_addr(ptr_t cmdbuf_addr, bi_list *list)
{
    bi_list_node *new_cmdbuf_node = NULL;
    struct cmdbuf_obj *cmdbuf_obj = NULL;
    new_cmdbuf_node = list->head;
    while (1) {
        if (new_cmdbuf_node == NULL)
            return NULL;
        cmdbuf_obj = (struct cmdbuf_obj *)new_cmdbuf_node->data;
        if (((cmdbuf_obj->cmdbuf_busAddress) <= cmdbuf_addr) &&
            (((cmdbuf_obj->cmdbuf_busAddress + cmdbuf_obj->cmdbuf_size) > cmdbuf_addr))) {
            return new_cmdbuf_node;
        }
        new_cmdbuf_node = new_cmdbuf_node->next;
    }
    return NULL;
}

static i32 select_vcmd(bi_list_node *new_cmdbuf_node)
{
    struct cmdbuf_obj *cmdbuf_obj = NULL;
    struct hantrovcmd_dev *dev = NULL;
    struct hantrovcmd_dev *smallest_dev = NULL;
    bi_list *list = NULL;
    bi_list_node *curr_cmdbuf_node = NULL;
    struct cmdbuf_obj *cmdbuf_obj_temp = NULL;
    int counter = 0;
    u32 executing_time = 0xffffffff;
    u32 hw_rdy_cmdbuf_num = 0;
    ptr_t exe_cmdbuf_addr = 0;
    u32 cmdbuf_id = 0;

    cmdbuf_obj = (struct cmdbuf_obj *)new_cmdbuf_node->data;

    //there is an empty vcmd to be used
    while (1) {
        dev = vcmd_manager[cmdbuf_obj->module_type][vcmd_position[cmdbuf_obj->module_type]];
        list = &dev->list_manager;
        //FIXME:...
        if (/*(dev->formats & vwl_client_type) && */ list->tail == NULL) {
            bi_list_insert_node_tail(list, new_cmdbuf_node);
            vcmd_position[cmdbuf_obj->module_type]++;
            if (vcmd_position[cmdbuf_obj->module_type] >= vcmd_core_num[cmdbuf_obj->module_type])
                vcmd_position[cmdbuf_obj->module_type] = 0;
            cmdbuf_obj->core_id = dev->core_id;
            return 0;
        } else {
            vcmd_position[cmdbuf_obj->module_type]++;
            if (vcmd_position[cmdbuf_obj->module_type] >= vcmd_core_num[cmdbuf_obj->module_type])
                vcmd_position[cmdbuf_obj->module_type] = 0;
            counter++;
        }
        if (counter >= vcmd_core_num[cmdbuf_obj->module_type])
            break;
    }

    //there is a vcmd which tail node -> cmdbuf_run_done == 1. It means this vcmd has nothing to do, so we select it.
    counter = 0;
    while (1) {
        dev = vcmd_manager[cmdbuf_obj->module_type][vcmd_position[cmdbuf_obj->module_type]];
        list = &dev->list_manager;
        curr_cmdbuf_node = list->tail;
        if (curr_cmdbuf_node == NULL) {
            bi_list_insert_node_tail(list, new_cmdbuf_node);
            vcmd_position[cmdbuf_obj->module_type]++;
            if (vcmd_position[cmdbuf_obj->module_type] >= vcmd_core_num[cmdbuf_obj->module_type])
                vcmd_position[cmdbuf_obj->module_type] = 0;
            cmdbuf_obj->core_id = dev->core_id;
            return 0;
        }
        cmdbuf_obj_temp = (struct cmdbuf_obj *)curr_cmdbuf_node->data;
        if (cmdbuf_obj_temp->cmdbuf_run_done == 1) {
            bi_list_insert_node_tail(list, new_cmdbuf_node);
            vcmd_position[cmdbuf_obj->module_type]++;
            if (vcmd_position[cmdbuf_obj->module_type] >= vcmd_core_num[cmdbuf_obj->module_type])
                vcmd_position[cmdbuf_obj->module_type] = 0;
            cmdbuf_obj->core_id = dev->core_id;
            return 0;
        } else {
            vcmd_position[cmdbuf_obj->module_type]++;
            if (vcmd_position[cmdbuf_obj->module_type] >= vcmd_core_num[cmdbuf_obj->module_type])
                vcmd_position[cmdbuf_obj->module_type] = 0;
            counter++;
        }
        if (counter >= vcmd_core_num[cmdbuf_obj->module_type])
            break;
    }

    //another case, tail = executing node, and vcmd=pend state (finish but not generate interrupt)
    counter = 0;
    while (1) {
        dev = vcmd_manager[cmdbuf_obj->module_type][vcmd_position[cmdbuf_obj->module_type]];
        list = &dev->list_manager;
        //read executing cmdbuf address
        if (dev->hw_version_id <= VCMD_HW_ID_1_0_C)
            hw_rdy_cmdbuf_num =
                vwl_get_register_value(dev->vcmd_core, dev->reg_mirror, HWIF_VCMD_EXE_CMDBUF_COUNT);
        else {
            hw_rdy_cmdbuf_num = *(dev->vcmd_reg_mem_virtual_address + VCMD_EXE_CMDBUF_COUNT);
            if (hw_rdy_cmdbuf_num != dev->sw_cmdbuf_rdy_num)
                hw_rdy_cmdbuf_num += 1;
        }
        curr_cmdbuf_node = list->tail;
        if (curr_cmdbuf_node == NULL) {
            bi_list_insert_node_tail(list, new_cmdbuf_node);
            vcmd_position[cmdbuf_obj->module_type]++;
            if (vcmd_position[cmdbuf_obj->module_type] >= vcmd_core_num[cmdbuf_obj->module_type])
                vcmd_position[cmdbuf_obj->module_type] = 0;
            cmdbuf_obj->core_id = dev->core_id;
            return 0;
        }

        if ((dev->sw_cmdbuf_rdy_num == hw_rdy_cmdbuf_num)) {
            bi_list_insert_node_tail(list, new_cmdbuf_node);
            vcmd_position[cmdbuf_obj->module_type]++;
            if (vcmd_position[cmdbuf_obj->module_type] >= vcmd_core_num[cmdbuf_obj->module_type])
                vcmd_position[cmdbuf_obj->module_type] = 0;
            cmdbuf_obj->core_id = dev->core_id;
            return 0;
        } else {
            vcmd_position[cmdbuf_obj->module_type]++;
            if (vcmd_position[cmdbuf_obj->module_type] >= vcmd_core_num[cmdbuf_obj->module_type])
                vcmd_position[cmdbuf_obj->module_type] = 0;
            counter++;
        }
        if (counter >= vcmd_core_num[cmdbuf_obj->module_type])
            break;
    }

    //there is no idle vcmd,if low priority,calculate exe time, select the least one.
    // or if high priority, calculate the exe time, select the least one and abort it.
    if (cmdbuf_obj->priority == CMDBUF_PRIORITY_NORMAL) {
        counter = 0;
        //calculate total execute time of all devices
        while (1) {
            dev = vcmd_manager[cmdbuf_obj->module_type][vcmd_position[cmdbuf_obj->module_type]];
            //read executing cmdbuf address
            if (dev->hw_version_id <= VCMD_HW_ID_1_0_C) {
                exe_cmdbuf_addr = VWLGetAddrRegisterValue(dev->vcmd_core, dev->reg_mirror,
                                                          HWIF_VCMD_EXECUTING_CMD_ADDR);
                list = &dev->list_manager;
                //get the executing cmdbuf node.
                curr_cmdbuf_node = get_cmdbuf_node_in_list_by_addr(exe_cmdbuf_addr, list);

                //calculate total execute time of this device
                dev->total_exe_time = calculate_executing_time_after_node(curr_cmdbuf_node);
            } else {
                //cmdbuf_id = vcmd_get_register_value((const void *)dev->hwregs,dev->reg_mirror,HWIF_VCMD_CMDBUF_EXECUTING_ID);
                cmdbuf_id = *(dev->vcmd_reg_mem_virtual_address + EXECUTING_CMDBUF_ID_ADDR + 1);
                if (cmdbuf_id >= TOTAL_DISCRETE_CMDBUF_NUM ||
                    (cmdbuf_id == 0 && vcmd_run_once[dev->core_id])) {
                    printf("cmdbuf_id greater than the ceiling !!\n");
                    return -1;
                }
                //get the executing cmdbuf node.
                curr_cmdbuf_node = global_cmdbuf_node[cmdbuf_id];
                if (curr_cmdbuf_node == NULL) {
                    list = &dev->list_manager;
                    curr_cmdbuf_node = list->head;
                    while (1) {
                        if (curr_cmdbuf_node == NULL)
                            break;
                        cmdbuf_obj_temp = (struct cmdbuf_obj *)curr_cmdbuf_node->data;
                        if (cmdbuf_obj_temp->cmdbuf_data_linked &&
                            cmdbuf_obj_temp->cmdbuf_run_done == 0)
                            break;
                        curr_cmdbuf_node = curr_cmdbuf_node->next;
                    }
                }

                //calculate total execute time of this device
                dev->total_exe_time = calculate_executing_time_after_node(curr_cmdbuf_node);
            }
            vcmd_position[cmdbuf_obj->module_type]++;
            if (vcmd_position[cmdbuf_obj->module_type] >= vcmd_core_num[cmdbuf_obj->module_type])
                vcmd_position[cmdbuf_obj->module_type] = 0;
            counter++;
            if (counter >= vcmd_core_num[cmdbuf_obj->module_type])
                break;
        }
        //find the device with the least total_exe_time.
        counter = 0;
        executing_time = 0xffffffff;
        while (1) {
            dev = vcmd_manager[cmdbuf_obj->module_type][vcmd_position[cmdbuf_obj->module_type]];
            if (dev->total_exe_time <= executing_time) {
                executing_time = dev->total_exe_time;
                smallest_dev = dev;
            }
            vcmd_position[cmdbuf_obj->module_type]++;
            if (vcmd_position[cmdbuf_obj->module_type] >= vcmd_core_num[cmdbuf_obj->module_type])
                vcmd_position[cmdbuf_obj->module_type] = 0;
            counter++;
            if (counter >= vcmd_core_num[cmdbuf_obj->module_type])
                break;
        }
        //insert list
        list = &smallest_dev->list_manager;
        bi_list_insert_node_tail(list, new_cmdbuf_node);
        cmdbuf_obj->core_id = smallest_dev->core_id;
        return 0;
    } else {
        //CMDBUF_PRIORITY_HIGH
        counter = 0;
        //calculate total execute time of all devices
        while (1) {
            dev = vcmd_manager[cmdbuf_obj->module_type][vcmd_position[cmdbuf_obj->module_type]];
            if (dev->hw_version_id <= VCMD_HW_ID_1_0_C) {
                //read executing cmdbuf address
                exe_cmdbuf_addr = VWLGetAddrRegisterValue(dev->vcmd_core, dev->reg_mirror,
                                                          HWIF_VCMD_EXECUTING_CMD_ADDR);
                list = &dev->list_manager;
                //get the executing cmdbuf node.
                curr_cmdbuf_node = get_cmdbuf_node_in_list_by_addr(exe_cmdbuf_addr, list);

                //calculate total execute time of this device
                dev->total_exe_time =
                    calculate_executing_time_after_node_high_priority(curr_cmdbuf_node);
            } else {
                //cmdbuf_id = vcmd_get_register_value((const void *)dev->hwregs,dev->reg_mirror,HWIF_VCMD_CMDBUF_EXECUTING_ID);
                cmdbuf_id = *(dev->vcmd_reg_mem_virtual_address + EXECUTING_CMDBUF_ID_ADDR);
                if (cmdbuf_id >= TOTAL_DISCRETE_CMDBUF_NUM ||
                    (cmdbuf_id == 0 && vcmd_run_once[dev->core_id])) {
                    printf("cmdbuf_id greater than the ceiling !!\n");
                    return -1;
                }
                //get the executing cmdbuf node.
                curr_cmdbuf_node = global_cmdbuf_node[cmdbuf_id];
                if (curr_cmdbuf_node == NULL) {
                    list = &dev->list_manager;
                    curr_cmdbuf_node = list->head;
                    while (1) {
                        if (curr_cmdbuf_node == NULL)
                            break;
                        cmdbuf_obj_temp = (struct cmdbuf_obj *)curr_cmdbuf_node->data;
                        if (cmdbuf_obj_temp->cmdbuf_data_linked &&
                            cmdbuf_obj_temp->cmdbuf_run_done == 0)
                            break;
                        curr_cmdbuf_node = curr_cmdbuf_node->next;
                    }
                }

                //calculate total execute time of this device
                dev->total_exe_time = calculate_executing_time_after_node(curr_cmdbuf_node);
            }
            vcmd_position[cmdbuf_obj->module_type]++;
            if (vcmd_position[cmdbuf_obj->module_type] >= vcmd_core_num[cmdbuf_obj->module_type])
                vcmd_position[cmdbuf_obj->module_type] = 0;
            counter++;
            if (counter >= vcmd_core_num[cmdbuf_obj->module_type])
                break;
        }
        //find the smallest device.
        counter = 0;
        executing_time = 0xffffffff;
        while (1) {
            dev = vcmd_manager[cmdbuf_obj->module_type][vcmd_position[cmdbuf_obj->module_type]];
            if (dev->total_exe_time <= executing_time) {
                executing_time = dev->total_exe_time;
                smallest_dev = dev;
            }
            vcmd_position[cmdbuf_obj->module_type]++;
            if (vcmd_position[cmdbuf_obj->module_type] >= vcmd_core_num[cmdbuf_obj->module_type])
                vcmd_position[cmdbuf_obj->module_type] = 0;
            counter++;
            if (counter >= vcmd_core_num[cmdbuf_obj->module_type])
                break;
        }
        //abort the vcmd and wait
        vwl_write_register_value(smallest_dev->vcmd_core, smallest_dev->reg_mirror,
                                 HWIF_VCMD_START_TRIGGER, 0);
        //FIXME
        //if(wait_event_interruptible(*smallest_dev->wait_abort_queue, wait_abort_rdy(dev)) )
        //  return -ERESTARTSYS;

        //need to select inserting position again because hw maybe have run to the next node.
        //CMDBUF_PRIORITY_HIGH
        curr_cmdbuf_node = smallest_dev->list_manager.head;
        while (1) {
            //if list is empty or tail,insert to tail
            if (curr_cmdbuf_node == NULL)
                break;
            cmdbuf_obj_temp = (struct cmdbuf_obj *)curr_cmdbuf_node->data;
            //if find the first node which priority is normal, insert node prior to  the node
            if ((cmdbuf_obj_temp->priority == CMDBUF_PRIORITY_NORMAL) &&
                (cmdbuf_obj_temp->cmdbuf_run_done == 0))
                break;
            curr_cmdbuf_node = curr_cmdbuf_node->next;
        }
        bi_list_insert_node_before(list, curr_cmdbuf_node, new_cmdbuf_node);
        cmdbuf_obj->core_id = smallest_dev->core_id;

        return 0;
    }

    return 0;
}

static i32 reserve_cmdbuf(struct exchange_parameter *input_para)
{
    bi_list_node *new_cmdbuf_node = NULL;
    struct cmdbuf_obj *cmdbuf_obj = NULL;

    input_para->cmdbuf_id = 0;
    if (input_para->cmdbuf_size > CMDBUF_MAX_SIZE) {
        return -1;
    }

    new_cmdbuf_node = create_cmdbuf_node();
    if (new_cmdbuf_node == NULL)
        return -1;

    //set info from ewl
    cmdbuf_obj = (struct cmdbuf_obj *)new_cmdbuf_node->data;
    cmdbuf_obj->module_type = input_para->module_type;
    cmdbuf_obj->priority = input_para->priority;
    cmdbuf_obj->executing_time = input_para->executing_time;
    cmdbuf_obj->cmdbuf_size = CMDBUF_MAX_SIZE;
    cmdbuf_obj->cmdbuf_posted = 0;
    cmdbuf_obj->cmdbuf_reserve = input_para->cmdbuf_reserve;
    input_para->cmdbuf_size = CMDBUF_MAX_SIZE;
    //return cmdbuf_id to ewl
    input_para->cmdbuf_id = cmdbuf_obj->cmdbuf_id;
    global_cmdbuf_node[input_para->cmdbuf_id] = new_cmdbuf_node;
    PDEBUG("reserve cmdbuf %d\n", input_para->cmdbuf_id);
    return 0;
}

static long release_cmdbuf_node_cleanup(bi_list *list)
{
    bi_list_node *new_cmdbuf_node = NULL;
    struct cmdbuf_obj *cmdbuf_obj = NULL;
    while (1) {
        new_cmdbuf_node = list->head;
        if (new_cmdbuf_node == NULL)
            return 0;
        //remove node from list
        bi_list_remove_node(list, new_cmdbuf_node);
        //free node
        cmdbuf_obj = (struct cmdbuf_obj *)new_cmdbuf_node->data;
        global_cmdbuf_node[cmdbuf_obj->cmdbuf_id] = NULL;
        free_cmdbuf_node(new_cmdbuf_node);
    }
    return 0;
}

static bi_list_node *find_last_linked_cmdbuf(bi_list_node *current_node)
{
    bi_list_node *new_cmdbuf_node = current_node;
    bi_list_node *last_cmdbuf_node;
    struct cmdbuf_obj *cmdbuf_obj = NULL;
    if (current_node == NULL)
        return NULL;
    last_cmdbuf_node = new_cmdbuf_node;
    new_cmdbuf_node = new_cmdbuf_node->previous;
    while (1) {
        if (new_cmdbuf_node == NULL)
            return last_cmdbuf_node;
        cmdbuf_obj = (struct cmdbuf_obj *)new_cmdbuf_node->data;
        if (cmdbuf_obj->cmdbuf_data_linked) {
            return new_cmdbuf_node;
        }
        last_cmdbuf_node = new_cmdbuf_node;
        new_cmdbuf_node = new_cmdbuf_node->previous;
    }
    return NULL;
}

static i32 release_cmdbuf(u16 cmdbuf_id)
{
    struct cmdbuf_obj *cmdbuf_obj = NULL;
    bi_list_node *last_cmdbuf_node = NULL;
    bi_list_node *new_cmdbuf_node = NULL;
    bi_list *list = NULL;
    u32 module_type;

    struct hantrovcmd_dev *dev = NULL;
    /*get cmdbuf object according to cmdbuf_id*/
    new_cmdbuf_node = global_cmdbuf_node[cmdbuf_id];
    if (new_cmdbuf_node == NULL) {
        //should not happen
        PDEBUG("%s\n", "ERROR cmdbuf_id !!\n");
        return -1;
    }
    cmdbuf_obj = (struct cmdbuf_obj *)new_cmdbuf_node->data;
    module_type = cmdbuf_obj->module_type;
    sem_wait(&vcmd_reserve_cmdbuf_sem[module_type]);
    //TODO
    dev = &hantrovcmd_data[cmdbuf_obj->core_id];

    list = &dev->list_manager;
    cmdbuf_obj->cmdbuf_need_remove = 1;
    last_cmdbuf_node = new_cmdbuf_node->previous;
    while (1) {
        //remove current node
        cmdbuf_obj = (struct cmdbuf_obj *)new_cmdbuf_node->data;
        if (cmdbuf_obj->cmdbuf_need_remove == 1) {
            new_cmdbuf_node = remove_cmdbuf_node_from_list(list, new_cmdbuf_node);
            if (new_cmdbuf_node) {
                //free node
                global_cmdbuf_node[cmdbuf_obj->cmdbuf_id] = NULL;
                dev->total_exe_time -= cmdbuf_obj->executing_time;
                free_cmdbuf_node(new_cmdbuf_node);
            }
        }
        if (last_cmdbuf_node == NULL)
            break;
        new_cmdbuf_node = last_cmdbuf_node;
        last_cmdbuf_node = new_cmdbuf_node->previous;
    }
    sem_post(&vcmd_reserve_cmdbuf_sem[module_type]);
    return 0;
}

static void vcmd_link_cmdbuf(struct hantrovcmd_dev *dev, bi_list_node *last_linked_cmdbuf_node)
{
    bi_list_node *new_cmdbuf_node = NULL;
    bi_list_node *next_cmdbuf_node = NULL;
    struct cmdbuf_obj *cmdbuf_obj = NULL;
    struct cmdbuf_obj *next_cmdbuf_obj = NULL;
    u32 *jmp_addr = NULL;
    u32 operation_code;
    new_cmdbuf_node = last_linked_cmdbuf_node;

    //for the first cmdbuf.
    if (new_cmdbuf_node != NULL) {
        cmdbuf_obj = (struct cmdbuf_obj *)new_cmdbuf_node->data;
        if (cmdbuf_obj->cmdbuf_data_linked == 0) {
            dev->sw_cmdbuf_rdy_num++;
            cmdbuf_obj->cmdbuf_data_linked = 1;
            dev->duration_without_int = 0;
            if (cmdbuf_obj->has_end_cmdbuf == 0) {
                if (cmdbuf_obj->has_nop_cmdbuf == 1) {
                    dev->duration_without_int = cmdbuf_obj->executing_time;
                    //maybe nop is modified, so write back.
                    if (dev->duration_without_int >= INT_MIN_SUM_OF_IMAGE_SIZE) {
                        jmp_addr =
                            cmdbuf_obj->cmdbuf_virtualAddress + (cmdbuf_obj->cmdbuf_size / 4);
                        operation_code = *(jmp_addr - 4);
                        operation_code = JMP_IE_1 | operation_code;
                        *(jmp_addr - 4) = operation_code;
                        dev->duration_without_int = 0;
                    }
                }
            }
        }
    }

    while (1) {
        if (new_cmdbuf_node == NULL)
            break;
        /* first cmdbuf has been added.*/
        if (new_cmdbuf_node->next == NULL)
            break;
        next_cmdbuf_node = new_cmdbuf_node->next;
        cmdbuf_obj = (struct cmdbuf_obj *)new_cmdbuf_node->data;
        next_cmdbuf_obj = (struct cmdbuf_obj *)next_cmdbuf_node->data;
        if (cmdbuf_obj->has_end_cmdbuf == 0) {
            //need to link， current cmdbuf link to next cmdbuf
            jmp_addr = cmdbuf_obj->cmdbuf_virtualAddress + (cmdbuf_obj->cmdbuf_size / 4);
            if (dev->hw_version_id > VCMD_HW_ID_1_0_C) {
                //set next cmdbuf id
                *(jmp_addr - 1) = next_cmdbuf_obj->cmdbuf_id;
            }
            if (sizeof(ptr_t) == 8) {
                *(jmp_addr - 2) = (u32)(((u64)(ptr_t)next_cmdbuf_obj->cmdbuf_busAddress >> 32));
            } else {
                *(jmp_addr - 2) = 0;
            }
            *(jmp_addr - 3) = (u32)((ptr_t)next_cmdbuf_obj->cmdbuf_virtualAddress);
            operation_code = *(jmp_addr - 4);
            operation_code >>= 16;
            operation_code <<= 16;
            *(jmp_addr - 4) =
                (u32)(operation_code | JMP_RDY_1 | ((next_cmdbuf_obj->cmdbuf_size + 7) / 8));
            next_cmdbuf_obj->cmdbuf_data_linked = 1;
            dev->sw_cmdbuf_rdy_num++;
            //modify nop code of next cmdbuf
            if (next_cmdbuf_obj->has_end_cmdbuf == 0) {
                if (next_cmdbuf_obj->has_nop_cmdbuf == 1) {
                    dev->duration_without_int += next_cmdbuf_obj->executing_time;

                    //maybe we see the modified nop before abort, so need to write back.
                    if (dev->duration_without_int >= INT_MIN_SUM_OF_IMAGE_SIZE) {
                        jmp_addr = next_cmdbuf_obj->cmdbuf_virtualAddress +
                                   (next_cmdbuf_obj->cmdbuf_size / 4);
                        operation_code = *(jmp_addr - 4);
                        operation_code = JMP_IE_1 | operation_code;
                        *(jmp_addr - 4) = operation_code;
                        dev->duration_without_int = 0;
                    }
                }
            } else {
                dev->duration_without_int = 0;
            }

#ifdef VCMD_DEBUG_INTERNAL
            {
                u32 i;
                printf("vcmd link, last cmdbuf content\n");
                for (i = cmdbuf_obj->cmdbuf_size / 4 - 8; i < cmdbuf_obj->cmdbuf_size / 4; i++) {
                    printf("current linked cmdbuf data %d =0x%x\n", i,
                           *(cmdbuf_obj->cmdbuf_virtualAddress + i));
                }
            }
#endif
        } else {
            break;
        }

        new_cmdbuf_node = new_cmdbuf_node->next;
    }
    return;
}

static void vcmd_delink_cmdbuf(struct hantrovcmd_dev *dev, bi_list_node *last_linked_cmdbuf_node)
{
    bi_list_node *new_cmdbuf_node = NULL;
    struct cmdbuf_obj *cmdbuf_obj = NULL;

    new_cmdbuf_node = last_linked_cmdbuf_node;
    while (1) {
        if (new_cmdbuf_node == NULL)
            break;
        cmdbuf_obj = (struct cmdbuf_obj *)new_cmdbuf_node->data;
        if (cmdbuf_obj->cmdbuf_data_linked) {
            cmdbuf_obj->cmdbuf_data_linked = 0;
        } else
            break;
        new_cmdbuf_node = new_cmdbuf_node->next;
    }
    dev->sw_cmdbuf_rdy_num = 0;
}

static void vcmd_start(struct hantrovcmd_dev *dev, bi_list_node *first_linked_cmdbuf_node)
{
    struct cmdbuf_obj *cmdbuf_obj = NULL;
    int i;
    if (dev->working_state == WORKING_STATE_IDLE) {
        if ((first_linked_cmdbuf_node != NULL) && dev->sw_cmdbuf_rdy_num) {
            cmdbuf_obj = (struct cmdbuf_obj *)first_linked_cmdbuf_node->data;
            //0x40
            vwl_set_register_mirror_value(dev->reg_mirror, HWIF_VCMD_AXI_CLK_GATE_DISABLE, 0);
            vwl_set_register_mirror_value(
                dev->reg_mirror, HWIF_VCMD_MASTER_OUT_CLK_GATE_DISABLE,
                1); //this bit should be set 1 only when need to reset dec400
            vwl_set_register_mirror_value(dev->reg_mirror, HWIF_VCMD_CORE_CLK_GATE_DISABLE, 1);
            vwl_set_register_mirror_value(dev->reg_mirror, HWIF_VCMD_ABORT_MODE, 0);
            vwl_set_register_mirror_value(dev->reg_mirror, HWIF_VCMD_RESET_CORE, 0);
            vwl_set_register_mirror_value(dev->reg_mirror, HWIF_VCMD_RESET_ALL, 0);
            vwl_set_register_mirror_value(dev->reg_mirror, HWIF_VCMD_START_TRIGGER, 1);
            //0x48
            if (dev->hw_version_id <= VCMD_HW_ID_1_0_C)
                vwl_set_register_mirror_value(dev->reg_mirror, HWIF_VCMD_IRQ_INTCMD_EN, 0xffff);
            else {
                vwl_set_register_mirror_value(dev->reg_mirror, HWIF_VCMD_IRQ_JMPP_EN, 1);
                vwl_set_register_mirror_value(dev->reg_mirror, HWIF_VCMD_IRQ_JMPD_EN, 1);
            }
            vwl_set_register_mirror_value(dev->reg_mirror, HWIF_VCMD_IRQ_RESET_EN, 1);
            vwl_set_register_mirror_value(dev->reg_mirror, HWIF_VCMD_IRQ_ABORT_EN, 1);
            vwl_set_register_mirror_value(dev->reg_mirror, HWIF_VCMD_IRQ_CMDERR_EN, 1);
            vwl_set_register_mirror_value(dev->reg_mirror, HWIF_VCMD_IRQ_TIMEOUT_EN, 1);
            vwl_set_register_mirror_value(dev->reg_mirror, HWIF_VCMD_IRQ_BUSERR_EN, 1);
            vwl_set_register_mirror_value(dev->reg_mirror, HWIF_VCMD_IRQ_ENDCMD_EN, 1);
            //0x4c
            vwl_set_register_mirror_value(dev->reg_mirror, HWIF_VCMD_TIMEOUT_EN, 1);
            vwl_set_register_mirror_value(dev->reg_mirror, HWIF_VCMD_TIMEOUT_CYCLES, 0x1dcd6500);
            vwl_set_register_mirror_value(dev->reg_mirror, HWIF_VCMD_EXECUTING_CMD_ADDR,
                                          (u32)((ptr_t)cmdbuf_obj->cmdbuf_virtualAddress));
            if (sizeof(ptr_t) == 8) {
                vwl_set_register_mirror_value(
                    dev->reg_mirror, HWIF_VCMD_EXECUTING_CMD_ADDR_MSB,
                    (u32)(((u64)(ptr_t)cmdbuf_obj->cmdbuf_virtualAddress) >> 32));
            } else {
                vwl_set_register_mirror_value(dev->reg_mirror, HWIF_VCMD_EXECUTING_CMD_ADDR_MSB, 0);
            }
            vwl_set_register_mirror_value(dev->reg_mirror, HWIF_VCMD_EXE_CMDBUF_LENGTH,
                                          (u32)((cmdbuf_obj->cmdbuf_size + 7) / 8));
            vwl_set_register_mirror_value(dev->reg_mirror, HWIF_VCMD_RDY_CMDBUF_COUNT,
                                          dev->sw_cmdbuf_rdy_num);
            vwl_set_register_mirror_value(dev->reg_mirror, HWIF_VCMD_MAX_BURST_LEN, 0x10);

            if (dev->hw_version_id > VCMD_HW_ID_1_0_C) {
                vwl_write_register_value(dev->vcmd_core, dev->reg_mirror,
                                         HWIF_VCMD_CMDBUF_EXECUTING_ID, (u32)cmdbuf_obj->cmdbuf_id);
            }

            AsicHwVcmdCoreWriteRegister(dev->vcmd_core, 0x44,
                                        AsicHwVcmdCoreReadRegister(dev->vcmd_core, 0x44));
            for (i = 0; i < 7; i++) {
                AsicHwVcmdCoreWriteRegister(dev->vcmd_core, 0x48 + i * 4,
                                            dev->reg_mirror[(0x48 + i * 4) / 4]);
            }
            dev->working_state = WORKING_STATE_WORKING;
            //start
            vwl_set_register_mirror_value(
                dev->reg_mirror, HWIF_VCMD_MASTER_OUT_CLK_GATE_DISABLE,
                0); //this bit should be set 1 only when need to reset dec400
            vwl_set_register_mirror_value(dev->reg_mirror, HWIF_VCMD_START_TRIGGER, 1);
            AsicHwVcmdCoreWriteRegister(dev->vcmd_core, 0x40, dev->reg_mirror[0x40 / 4]);
        }
    }
}

static i32 link_and_run_cmdbuf(struct exchange_parameter *input_para)
{
    struct cmdbuf_obj *cmdbuf_obj = NULL;
    bi_list_node *new_cmdbuf_node = NULL;
    u16 cmdbuf_id = input_para->cmdbuf_id;
    int ret;
    u32 *jmp_addr = NULL;
    u32 opCode;
    u32 tempOpcode;
    struct hantrovcmd_dev *dev = NULL;
    bi_list_node *last_cmdbuf_node;
    u32 record_last_cmdbuf_rdy_num;
    new_cmdbuf_node = global_cmdbuf_node[cmdbuf_id];
    if (new_cmdbuf_node == NULL) {
        //should not happen
        PDEBUG("%s\n", "vcmd: ERROR cmdbuf_id !!\n");
        return -1;
    }

    cmdbuf_obj = (struct cmdbuf_obj *)new_cmdbuf_node->data;
    cmdbuf_obj->cmdbuf_data_loaded = 1;
    cmdbuf_obj->cmdbuf_size = input_para->cmdbuf_size;
    cmdbuf_obj->waited = 0;

#ifdef VCMD_DEBUG_INTERNAL
    {
        u32 i;
        printf("vcmd link, current cmdbuf content\n");
        for (i = 0; i < cmdbuf_obj->cmdbuf_size / 4; i++) {
            printf("current cmdbuf data %d =0x%x\n", i, *(cmdbuf_obj->cmdbuf_virtualAddress + i));
        }
    }
#endif

    //test nop and end opcode, then assign value.
    cmdbuf_obj->has_end_cmdbuf = 0;       //0: has jmp opcode,1 has end code
    cmdbuf_obj->no_normal_int_cmdbuf = 0; //0: interrupt when JMP,1 not interrupt when JMP
    jmp_addr = cmdbuf_obj->cmdbuf_virtualAddress + (cmdbuf_obj->cmdbuf_size / 4);
    opCode = tempOpcode = *(jmp_addr - 4);
    opCode >>= 27;
    opCode <<= 27;
    //we can't identify END opcode or JMP opcode, so we don't support END opcode in control sw and driver.
    if (opCode == OPCODE_JMP) {
        //jmp
        opCode = tempOpcode;
        opCode &= 0x02000000;
        if (opCode == JMP_IE_1) {
            cmdbuf_obj->no_normal_int_cmdbuf = 0;
        } else {
            cmdbuf_obj->no_normal_int_cmdbuf = 1;
        }
    } else {
        //not support other opcode
        return -1;
    }

    sem_wait(&vcmd_reserve_cmdbuf_sem[cmdbuf_obj->module_type]);

    ret = select_vcmd(new_cmdbuf_node);
    if (ret)
        return ret;

    dev = &hantrovcmd_data[cmdbuf_obj->core_id];
    input_para->core_id = cmdbuf_obj->core_id;
    dev->total_exe_time += cmdbuf_obj->executing_time;
    PDEBUG("Allocate cmd buffer [%d] to core [%d]\n", cmdbuf_id, input_para->core_id);
    if (dev->hw_version_id > VCMD_HW_ID_1_0_C) {
        //read vcmd executing register into ddr memory.
        //now core id is got and output ddr address of vcmd register can be filled in.
        //each core has its own fixed output ddr address of vcmd registers.
        jmp_addr = cmdbuf_obj->cmdbuf_virtualAddress;
        if (sizeof(ptr_t) == 8) {
            *(jmp_addr + 2) = (u32)(
                (u64)(dev->vcmd_reg_mem_bus_address + (EXECUTING_CMDBUF_ID_ADDR + 1) * 4) >> 32);
        } else {
            *(jmp_addr + 2) = 0;
        }
        *(jmp_addr + 1) =
            (u32)((dev->vcmd_reg_mem_bus_address + (EXECUTING_CMDBUF_ID_ADDR + 1) * 4));

        jmp_addr = cmdbuf_obj->cmdbuf_virtualAddress + (cmdbuf_obj->cmdbuf_size / 4);
        //read vcmd all registers into ddr memory.
        //now core id is got and output ddr address of vcmd registers can be filled in.
        //each core has its own fixed output ddr address of vcmd registers.
        if (sizeof(ptr_t) == 8) {
            *(jmp_addr - 6) = (u32)((u64)dev->vcmd_reg_mem_bus_address >> 32);
        } else {
            *(jmp_addr - 6) = 0;
        }

        *(jmp_addr - 7) = (u32)(dev->vcmd_reg_mem_bus_address);
    }

    last_cmdbuf_node = find_last_linked_cmdbuf(new_cmdbuf_node);
    record_last_cmdbuf_rdy_num = dev->sw_cmdbuf_rdy_num;
    vcmd_link_cmdbuf(dev, last_cmdbuf_node);
    if (dev->working_state == WORKING_STATE_IDLE) {
        //run
        vcmd_start(dev, last_cmdbuf_node);
    } else {
        //just update cmdbuf ready number
        if (record_last_cmdbuf_rdy_num != dev->sw_cmdbuf_rdy_num)
            vwl_write_register_value(dev->vcmd_core, dev->reg_mirror, HWIF_VCMD_RDY_CMDBUF_COUNT,
                                     dev->sw_cmdbuf_rdy_num);
    }

    sem_post(&vcmd_reserve_cmdbuf_sem[cmdbuf_obj->module_type]);

    return 0;
}

/******************************************************************************/
static int check_cmdbuf_irq(struct hantrovcmd_dev *dev, struct cmdbuf_obj *new_cmdbuf_obj,
                            u32 *irq_status_ret)
{
    int rdy = 0;
    if (new_cmdbuf_obj->cmdbuf_run_done) {
        rdy = 1;
        *irq_status_ret =
            new_cmdbuf_obj->executing_status; //need to decide how to assign this variable
    }
    return rdy;
}

/******************************************************************************/
static int check_mc_cmdbuf_irq(struct cmdbuf_obj *new_cmdbuf_obj, u32 *irq_status_ret)
{
    int k;
    struct hantrovcmd_dev *dev = NULL;
    struct bi_list_node *new_cmdbuf_node;

    for (k = 0; k < TOTAL_DISCRETE_CMDBUF_NUM; k++) {
        new_cmdbuf_node = global_cmdbuf_node[k];
        if (new_cmdbuf_node == NULL)
            continue;
        new_cmdbuf_obj = new_cmdbuf_node->data;
        if ((new_cmdbuf_obj->cmdbuf_reserve == 1) || (new_cmdbuf_obj->waited == 1))
            continue;
        dev = &hantrovcmd_data[new_cmdbuf_obj->core_id];
        if (check_cmdbuf_irq(dev, new_cmdbuf_obj, irq_status_ret) == 1) {
            /* Return cmdbuf_id when ANY_CMDBUF_ID is used. */
            if (!new_cmdbuf_obj->waited) {
                *irq_status_ret = new_cmdbuf_obj->cmdbuf_id;
                new_cmdbuf_obj->waited = 1;
                return 1;
            }
        }
    }
    return 0;
}

static unsigned int wait_cmdbuf_ready(u16 *cmd_buf_id)
{
    struct cmdbuf_obj *new_cmdbuf_obj = NULL;
    struct hantrovcmd_dev *dev = NULL;
    u32 irq_status = 0;
    struct timeval now_timer;
    struct timespec wait_timer;

    if (*cmd_buf_id != ANY_CMDBUF_ID) {
        new_cmdbuf_obj = (struct cmdbuf_obj *)global_cmdbuf_node[*cmd_buf_id]->data;

        dev = &hantrovcmd_data[new_cmdbuf_obj->core_id];

        pthread_mutex_lock(&global_vcmd_mutex);
        while (check_cmdbuf_irq(dev, new_cmdbuf_obj, &irq_status) == 0) {
            gettimeofday(&now_timer, NULL);
            wait_timer.tv_sec = now_timer.tv_sec + 1;
            wait_timer.tv_nsec = now_timer.tv_usec * 1000;
            pthread_cond_timedwait(&global_vcmd_cond_wait, &global_vcmd_mutex, &wait_timer);
            //            pthread_cond_wait(&global_vcmd_cond_wait,&global_vcmd_mutex);
            if (global_vcmd_release == 1)
                break;
        }
        pthread_mutex_unlock(&global_vcmd_mutex);
    } else {
        pthread_mutex_lock(&global_vcmd_mutex);
        while (check_mc_cmdbuf_irq(new_cmdbuf_obj, &irq_status) == 0) {
            gettimeofday(&now_timer, NULL);
            wait_timer.tv_sec = now_timer.tv_sec + 1;
            wait_timer.tv_nsec = now_timer.tv_usec * 1000;
            pthread_cond_timedwait(&global_vcmd_cond_wait, &global_vcmd_mutex, &wait_timer);
            //            pthread_cond_wait(&global_vcmd_cond_wait,&global_vcmd_mutex);
            if (global_vcmd_release == 1)
                break;
        }
        pthread_mutex_unlock(&global_vcmd_mutex);
        *cmd_buf_id = (u16)irq_status;
    }

    return 0;
}

static void create_read_all_registers_cmdbuf(struct exchange_parameter *input_para)
{
    u32 register_range[] = {VCMD_ENCODER_REGISTER_SIZE, VCMD_IM_REGISTER_SIZE,
                            VCMD_DECODER_REGISTER_SIZE, VCMD_JPEG_ENCODER_REGISTER_SIZE,
                            VCMD_JPEG_DECODER_REGISTER_SIZE};
    u32 counter_cmdbuf_size = 0;
    u32 *set_base_addr =
        vcmd_buf_mem_pool.virtual_address + input_para->cmdbuf_id * CMDBUF_MAX_SIZE / 4;
    //u32 *status_base_virt_addr=vcmd_status_buf_mem_pool.virtualAddress + input_para->cmdbuf_id*CMDBUF_MAX_SIZE/4+(vcmd_manager[input_para->module_type][0]->vcmd_core_cfg.submodule_main_addr/4+0);
    ptr_t status_base_phy_addr = vcmd_status_buf_mem_pool.bus_address +
                                 input_para->cmdbuf_id * CMDBUF_MAX_SIZE +
                                 (vcmd_manager[input_para->module_type][input_para->core_id]
                                      ->vcmd_core_cfg.vcmd_core_config_ptr->submodule_main_addr +
                                  0);
    u32 offset_inc = 0;
    u32 offset_inc_dec400 = 0;
    if (vcmd_manager[input_para->module_type][input_para->core_id]->hw_version_id >
        VCMD_HW_ID_1_0_C) {
        PDEBUG("vc8000_vcmd_driver:create cmdbuf data when hw_version_id = 0x%x\n",
               vcmd_manager[input_para->module_type][input_para->core_id]->hw_version_id);

        //read vcmd executing cmdbuf id registers to ddr for balancing core load.
        *(set_base_addr + 0) = (OPCODE_RREG) | (1 << 16) | (EXECUTING_CMDBUF_ID_ADDR * 4);
        counter_cmdbuf_size += 4;
        *(set_base_addr + 1) = (u32)0; //will be changed in link stage
        counter_cmdbuf_size += 4;
        *(set_base_addr + 2) = (u32)0; //will be changed in link stage
        counter_cmdbuf_size += 4;
        //alignment
        *(set_base_addr + 3) = 0;
        counter_cmdbuf_size += 4;

        //read all registers
        *(set_base_addr + 4) = (OPCODE_RREG) |
                               ((register_range[input_para->module_type] / 4) << 16) |
                               (vcmd_manager[input_para->module_type][input_para->core_id]
                                    ->vcmd_core_cfg.vcmd_core_config_ptr->submodule_main_addr +
                                0);
        counter_cmdbuf_size += 4;
        *(set_base_addr + 5) = (u32)(status_base_phy_addr);
        counter_cmdbuf_size += 4;
        if (sizeof(ptr_t) == 8) {
            *(set_base_addr + 6) = (u32)((u64)(status_base_phy_addr) >> 32);
        } else {
            *(set_base_addr + 6) = 0;
        }
        counter_cmdbuf_size += 4;
        //alignment
        *(set_base_addr + 7) = 0;
        counter_cmdbuf_size += 4;
        if (vcmd_manager[input_para->module_type][0]
                ->vcmd_core_cfg.vcmd_core_config_ptr->submodule_L2Cache_addr != 0xffff) {
            //read L2 cache register
            offset_inc = 4;
            status_base_phy_addr =
                vcmd_status_buf_mem_pool.bus_address + input_para->cmdbuf_id * CMDBUF_MAX_SIZE +
                (vcmd_manager[input_para->module_type][0]
                         ->vcmd_core_cfg.vcmd_core_config_ptr->submodule_L2Cache_addr /
                     2 +
                 0);
            //map_status_base_phy_addr=vcmd_status_buf_mem_pool.mmu_bus_address + input_para->cmdbuf_id*CMDBUF_MAX_SIZE+(vcmd_manager[input_para->module_type][0]->vcmd_core_cfg.submodule_L2Cache_addr/2+0);
            //read L2cache IP first register
            *(set_base_addr + 8) =
                (OPCODE_RREG) | (1 << 16) |
                (vcmd_manager[input_para->module_type][0]
                     ->vcmd_core_cfg.vcmd_core_config_ptr->submodule_L2Cache_addr +
                 0);
            counter_cmdbuf_size += 4;
            *(set_base_addr + 9) = (u32)(status_base_phy_addr);
            counter_cmdbuf_size += 4;
            if (sizeof(ptr_t) == 8) {
                *(set_base_addr + 10) = (u32)((u64)(status_base_phy_addr) >> 32);
            } else {
                *(set_base_addr + 10) = 0;
            }
            counter_cmdbuf_size += 4;
            //alignment
            *(set_base_addr + 11) = 0;
            counter_cmdbuf_size += 4;
        }
        if (vcmd_manager[input_para->module_type][0]
                ->vcmd_core_cfg.vcmd_core_config_ptr->submodule_dec400_addr != 0xffff) {
            //read dec400 register
            offset_inc_dec400 = 4;
            status_base_phy_addr =
                vcmd_status_buf_mem_pool.bus_address + input_para->cmdbuf_id * CMDBUF_MAX_SIZE +
                (vcmd_manager[input_para->module_type][0]
                         ->vcmd_core_cfg.vcmd_core_config_ptr->submodule_dec400_addr /
                     2 +
                 0);
            //map_status_base_phy_addr=vcmd_status_buf_mem_pool.mmu_bus_address + input_para->cmdbuf_id*CMDBUF_MAX_SIZE+(vcmd_manager[input_para->module_type][0]->vcmd_core_cfg.vcmd_core_config_ptr->submodule_dec400_addr/2+0);
            //read DEC400 IP first register
            *(set_base_addr + 8 + offset_inc) =
                (OPCODE_RREG) | (0x2b << 16) |
                (vcmd_manager[input_para->module_type][0]
                     ->vcmd_core_cfg.vcmd_core_config_ptr->submodule_dec400_addr +
                 0);
            counter_cmdbuf_size += 4;
            *(set_base_addr + 9 + offset_inc) = (u32)(status_base_phy_addr);
            counter_cmdbuf_size += 4;
            if (sizeof(ptr_t) == 8) {
                *(set_base_addr + 10 + offset_inc) = (u32)((u64)(status_base_phy_addr) >> 32);
            } else {
                *(set_base_addr + 10 + offset_inc) = 0;
            }
            counter_cmdbuf_size += 4;
            //alignment
            *(set_base_addr + 11 + offset_inc) = 0;
            counter_cmdbuf_size += 4;
        }

#if 0
        //INT code, interrupt immediately
        *(set_base_addr + 4) = (OPCODE_INT) | 0 | input_para->cmdbuf_id;
        counter_cmdbuf_size += 4;
        //alignment
        *(set_base_addr + 5) = 0;
        counter_cmdbuf_size += 4;
#endif
        //read vcmd registers to ddr
        *(set_base_addr + 8 + offset_inc + offset_inc_dec400) = (OPCODE_RREG) | (27 << 16) | (0);
        counter_cmdbuf_size += 4;
        *(set_base_addr + 9 + offset_inc + offset_inc_dec400) =
            (u32)0; //will be changed in link stage
        counter_cmdbuf_size += 4;
        *(set_base_addr + 10 + offset_inc + offset_inc_dec400) =
            (u32)0; //will be changed in link stage
        counter_cmdbuf_size += 4;
        //alignment
        *(set_base_addr + 11 + offset_inc + offset_inc_dec400) = 0;
        counter_cmdbuf_size += 4;
        //JMP RDY = 0
        *(set_base_addr + 12 + offset_inc + offset_inc_dec400) =
            (OPCODE_JMP_RDY0) | 0 | JMP_IE_1 | 0;
        counter_cmdbuf_size += 4;
        *(set_base_addr + 13 + offset_inc + offset_inc_dec400) = 0;
        counter_cmdbuf_size += 4;
        *(set_base_addr + 14 + offset_inc + offset_inc_dec400) = 0;
        counter_cmdbuf_size += 4;
        *(set_base_addr + 15 + offset_inc + offset_inc_dec400) = input_para->cmdbuf_id;
        //don't add the last alignment DWORD in order to  identify END command or JMP command.
        //counter_cmdbuf_size += 4;
        input_para->cmdbuf_size = (16 + offset_inc + offset_inc_dec400) * 4;
    } else {
        PDEBUG("vc8000_vcmd_driver:create cmdbuf data when hw_version_id = 0x%x\n",
               vcmd_manager[input_para->module_type][input_para->core_id]->hw_version_id);
        //read all registers
        *(set_base_addr + 0) = (OPCODE_RREG) |
                               ((register_range[input_para->module_type] / 4) << 16) |
                               (vcmd_manager[input_para->module_type][input_para->core_id]
                                    ->vcmd_core_cfg.vcmd_core_config_ptr->submodule_main_addr +
                                0);
        counter_cmdbuf_size += 4;
        *(set_base_addr + 1) = (u32)(status_base_phy_addr);
        counter_cmdbuf_size += 4;
        *(set_base_addr + 2) = (u32)(status_base_phy_addr >> 32);
        counter_cmdbuf_size += 4;
        //alignment
        *(set_base_addr + 3) = 0;
        counter_cmdbuf_size += 4;
#if 0
        //INT code, interrupt immediately
        *(set_base_addr + 4) = (OPCODE_INT) | 0 | input_para->cmdbuf_id;
        counter_cmdbuf_size += 4;
        //alignment
        *(set_base_addr + 5) = 0;
        counter_cmdbuf_size += 4;
#endif
        //JMP RDY = 0
        *(set_base_addr + 4) = (OPCODE_JMP_RDY0) | 0 | JMP_IE_1 | 0;
        counter_cmdbuf_size += 4;
        *(set_base_addr + 5) = 0;
        counter_cmdbuf_size += 4;
        *(set_base_addr + 6) = 0;
        counter_cmdbuf_size += 4;
        *(set_base_addr + 7) = input_para->cmdbuf_id;
        //don't add the last alignment DWORD in order to  identify END command or JMP command.
        //counter_cmdbuf_size += 4;
        input_para->cmdbuf_size = 8 * 4;
    }
}

static void read_main_module_all_registers(u32 core_id)
{
    int ret;
    struct exchange_parameter input_para;
    u32 *status_base_virt_addr;
    u16 cmd_buf_id;
    u32 main_module_type;
    u32 manager_id;

    main_module_type = vcmd_core_array[core_id].sub_module_type;
    manager_id = hantrovcmd_data[core_id].id_in_type;
    input_para.executing_time = 0;
    input_para.priority = CMDBUF_PRIORITY_NORMAL;
    input_para.module_type = main_module_type;
    input_para.cmdbuf_size = 0;
    input_para.cmdbuf_reserve = 1;
    input_para.core_id = manager_id;
    ret = reserve_cmdbuf(&input_para);
    (void)(ret);
    vcmd_manager[main_module_type][manager_id]->status_cmdbuf_id = input_para.cmdbuf_id;
    create_read_all_registers_cmdbuf(&input_para);
    link_and_run_cmdbuf(&input_para);
    //sleep(1);
    //vcmd_isr_cb(input_para.core_id, &hantrovcmd_data[input_para.core_id]);
    cmd_buf_id = input_para.cmdbuf_id;
    //printf()
    wait_cmdbuf_ready(&cmd_buf_id);
    status_base_virt_addr = vcmd_status_buf_mem_pool.virtual_address +
                            input_para.cmdbuf_id * CMDBUF_MAX_SIZE / 4 +
                            (vcmd_manager[main_module_type][manager_id]
                                     ->vcmd_core_cfg.vcmd_core_config_ptr->submodule_main_addr /
                                 4 +
                             0);
    printf("vc8000_vcmd_driver: main module register 0:0x%x\n", *status_base_virt_addr);
    printf("vc8000_vcmd_driver: main module register 80:0x%x\n", *(status_base_virt_addr + 80));
    printf("vc8000_vcmd_driver: main module register 214:0x%x\n", *(status_base_virt_addr + 214));
    printf("vc8000_vcmd_driver: main module register 226:0x%x\n", *(status_base_virt_addr + 226));
    printf("vc8000_vcmd_driver: main module register 287:0x%x\n", *(status_base_virt_addr + 287));
    printf("vc8000_vcmd_driver: main module register 509:0x%x\n", *(status_base_virt_addr + 509));

    //don't release cmdbuf because it can be used repeatedly
    //release_cmdbuf(input_para.cmdbuf_id);
}

/* simulate ioctl reserve cmdbuf */
i32 CmodelIoctlReserveCmdbuf(struct exchange_parameter *param)
{
    return reserve_cmdbuf(param);
}

i32 CmodelIoctlEnableCmdbuf(struct exchange_parameter *param)
{
    return link_and_run_cmdbuf(param);
}

i32 CmodelIoctlWaitCmdbuf(u16 *cmd_buf_id)
{
    return wait_cmdbuf_ready(cmd_buf_id);
}

i32 CmodelIoctlReleaseCmdbuf(u32 cmd_buf_id)
{
    PDEBUG("release cmdbuf %d\n", cmd_buf_id);
    return release_cmdbuf(cmd_buf_id);
}

i32 CmodelIoctlGetCmdbufParameter(struct cmdbuf_mem_parameter *vcmd_mem_params)
{
    //get cmd buffer info
    vcmd_mem_params->cmdbuf_unit_size = CMDBUF_MAX_SIZE;
    vcmd_mem_params->status_cmdbuf_unit_size = CMDBUF_MAX_SIZE;
    vcmd_mem_params->cmdbuf_total_size = CMDBUF_POOL_TOTAL_SIZE;
    vcmd_mem_params->status_cmdbuf_total_size = CMDBUF_POOL_TOTAL_SIZE;
    vcmd_mem_params->virt_status_cmdbuf_addr = vcmd_status_buf_mem_pool.virtual_address;
    vcmd_mem_params->phy_status_cmdbuf_addr = vcmd_status_buf_mem_pool.bus_address;
    vcmd_mem_params->virt_cmdbuf_addr = vcmd_buf_mem_pool.virtual_address;
    vcmd_mem_params->phy_cmdbuf_addr = vcmd_buf_mem_pool.bus_address;
    vcmd_mem_params->core_register_addr = basemem_reg[VCMD_TYPE_ENCODER][0];
    vcmd_mem_params->core_register_addr_IM = basemem_reg[VCMD_TYPE_CUTREE][0];
    vcmd_mem_params->core_register_addr_JPEG = basemem_reg[VCMD_TYPE_ENCODER][0];
    return 0;
}

i32 CmodelIoctlGetVcmdParameter(struct config_parameter *vcmd_params)
{
    //get vcmd info
    if (vcmd_core_num[vcmd_params->module_type]) {
        vcmd_params->submodule_main_addr =
            vcmd_manager[vcmd_params->module_type][0]
                ->vcmd_core_cfg.vcmd_core_config_ptr->submodule_main_addr;
        vcmd_params->submodule_dec400_addr =
            vcmd_manager[vcmd_params->module_type][0]
                ->vcmd_core_cfg.vcmd_core_config_ptr->submodule_dec400_addr;
        vcmd_params->submodule_L2Cache_addr =
            vcmd_manager[vcmd_params->module_type][0]
                ->vcmd_core_cfg.vcmd_core_config_ptr->submodule_L2Cache_addr;
        vcmd_params->submodule_MMU_addr[0] =
            vcmd_manager[vcmd_params->module_type][0]
                ->vcmd_core_cfg.vcmd_core_config_ptr->submodule_MMU_addr[0];
        vcmd_params->submodule_MMU_addr[1] =
            vcmd_manager[vcmd_params->module_type][0]
                ->vcmd_core_cfg.vcmd_core_config_ptr->submodule_MMU_addr[1];
        vcmd_params->submodule_axife_addr[0] =
            vcmd_manager[vcmd_params->module_type][0]
                ->vcmd_core_cfg.vcmd_core_config_ptr->submodule_axife_addr[0];
        vcmd_params->submodule_axife_addr[1] =
            vcmd_manager[vcmd_params->module_type][0]
                ->vcmd_core_cfg.vcmd_core_config_ptr->submodule_axife_addr[1];
        vcmd_params->config_status_cmdbuf_id =
            vcmd_manager[vcmd_params->module_type][0]->status_cmdbuf_id;
        vcmd_params->vcmd_core_num = vcmd_core_num[vcmd_params->module_type];
        vcmd_params->vcmd_hw_version_id =
            vcmd_manager[vcmd_params->module_type][0]->vcmd_core_cfg.vcmd_hw_version_id;
    } else {
        vcmd_params->submodule_main_addr = 0xffff;
        vcmd_params->submodule_dec400_addr = 0xffff;
        vcmd_params->submodule_L2Cache_addr = 0xffff;
        vcmd_params->submodule_MMU_addr[0] = 0xffff;
        vcmd_params->submodule_MMU_addr[1] = 0xffff;
        vcmd_params->submodule_axife_addr[0] = 0xffff;
        vcmd_params->submodule_axife_addr[1] = 0xffff;
        vcmd_params->config_status_cmdbuf_id = 0;
        vcmd_params->vcmd_core_num = 0;
    }

    return 0;
}
i32 CmodelVcmdCheck()
{
    i32 ret;
    if (hantrovcmd_data == NULL)
        ret = 0;
    else
        ret = 1;
    return ret;
}

i32 CmodelVcmdGetCoreConfig()
{
    int i, vcmd_core_cont = 0;
    int VCEcont = 0;  //vce core0~3
    int IMcont = 4;   //im core4~7
    int JPEGcont = 8; //jpeg core8~11
    SysCoreInfo CoreInfo;
    CoreInfo = CoreEncGetHwInfo(0, 0);

    for (i = 0; i < CoreInfo.cores_per_slice; i++) {
        if (CoreInfo.Cfg[i].has_vcmd == HasExtHw) {
            EWLCoreSignature_t signature;
            EWLHwConfig_t cfg;

            signature.hw_asic_id = CoreInfo.Cfg[i].asic_id;
            signature.hw_build_id = CoreInfo.Cfg[i].build_id;
            signature.fuse[1] = CoreInfo.Cfg[i].fuse1;
            signature.fuse[2] = CoreInfo.Cfg[i].fuse2;
            signature.fuse[3] = CoreInfo.Cfg[i].fuse3;
            signature.fuse[4] = CoreInfo.Cfg[i].fuse4;
            signature.fuse[5] = CoreInfo.Cfg[i].fuse5;
            signature.fuse[6] = CoreInfo.Cfg[i].fuseAXI;
            EWLGetCoreConfig(&signature, &cfg);

            //DEV
            if (cfg.h264Enabled && cfg.hevcEnabled && cfg.jpegEnabled && cfg.cuTreeSupport) {
                //h264/hevc/jpeg/cutree
                if (i < 2) {
                    vcmd_core_array[vcmd_core_cont] = vcmd_core_all_array[VCEcont];
                    VCEcont++;
                } else if (i == 2) {
                    vcmd_core_array[vcmd_core_cont] = vcmd_core_all_array[IMcont];
                    IMcont++;
                } else if (i == 3) {
                    vcmd_core_array[vcmd_core_cont] = vcmd_core_all_array[JPEGcont];
                    JPEGcont++;
                }
                vcmd_core_cont++;
            } else {
                //target - configure cutree sepeartedly
                if (cfg.h264Enabled || cfg.hevcEnabled || cfg.av1Enabled) {
                    vcmd_core_array[vcmd_core_cont] = vcmd_core_all_array[VCEcont];
                    vcmd_core_cont++;
                    VCEcont++;
                    if (cfg.cuTreeSupport) {
                        vcmd_core_array[vcmd_core_cont] = vcmd_core_all_array[IMcont];
                        vcmd_core_cont++;
                        IMcont++;
                    }
                } else if (cfg.cuTreeSupport) {
                    vcmd_core_array[vcmd_core_cont] = vcmd_core_all_array[IMcont];
                    vcmd_core_cont++;
                    IMcont++;
                } else if (cfg.jpegEnabled) {
                    vcmd_core_array[vcmd_core_cont] = vcmd_core_all_array[JPEGcont];
                    vcmd_core_cont++;
                    JPEGcont++;
                }
            }
        }
    }
    total_vcmd_core_num = (vcmd_core_cont > MAX_HWCORE_NUM) ? MAX_HWCORE_NUM : vcmd_core_cont;
    return 0;
}

i32 CmodelVcmdInit()
{
    int i, k, ret;
    ret = CmodelVcmdGetCoreConfig();
    (void)ret;
    static const struct VcmdCallbacks vcmdcallbacks = {
        .vcmd_nor_isr_callback = vcmd_isr_cb,
        .vcmd_abn_isr_callback = vcmd_isr_cb,
        .vcmd_read_buf_callback = vcmd_read_buf_cb,
        .vcmd_write_buf_callback = vcmd_write_buf_cb,
        .vcmd_read_reg_callback = vcmd_read_reg_cb,
        .vcmd_write_reg_callback = vcmd_write_reg_cb,
        .vcmd_jmp_callback = vcmd_jmp_cb,
        .vcmd_stall_callback = vcmd_stall_cb,
    };
    hantrovcmd_data =
        (struct hantrovcmd_dev *)malloc(sizeof(struct hantrovcmd_dev) * total_vcmd_core_num);
    if (hantrovcmd_data == NULL) {
        fprintf(stderr, "error allocating memory for hantrovcmd data!\n");
        return -1;
    }

    memset(hantrovcmd_data, 0, sizeof(struct hantrovcmd_dev) * total_vcmd_core_num);

    vcmd_buf_mem_pool.virtual_address = (u32 *)malloc(CMDBUF_POOL_TOTAL_SIZE);
    if (!vcmd_buf_mem_pool.virtual_address) {
        fprintf(stderr, "error allocating memory for vcmdbuf!\n");
        free(hantrovcmd_data);
        return -1;
    }
    vcmd_buf_mem_pool.size = CMDBUF_POOL_TOTAL_SIZE;
    vcmd_buf_mem_pool.bus_address = (ptr_t)vcmd_buf_mem_pool.virtual_address;
    vcmd_buf_mem_pool.cmdbuf_id = 0xFFFF;

    vcmd_status_buf_mem_pool.virtual_address = (u32 *)malloc(CMDBUF_POOL_TOTAL_SIZE);
    if (!vcmd_status_buf_mem_pool.virtual_address) {
        fprintf(stderr, "error allocating memory for vcmd status buf!\n");
        free(hantrovcmd_data);
        free(vcmd_buf_mem_pool.virtual_address);
        return -1;
    }

    vcmd_status_buf_mem_pool.size = CMDBUF_POOL_TOTAL_SIZE;
    vcmd_status_buf_mem_pool.bus_address = (ptr_t)vcmd_status_buf_mem_pool.virtual_address;
    vcmd_status_buf_mem_pool.cmdbuf_id = 0xFFFF;

    vcmd_registers_mem_pool.virtual_address = (u32 *)malloc(CMDBUF_VCMD_REGISTER_TOTAL_SIZE);
    if (!vcmd_registers_mem_pool.virtual_address) {
        fprintf(stderr, "error allocating memory for vcmd status buf!\n");
        free(hantrovcmd_data);
        free(vcmd_buf_mem_pool.virtual_address);
        free(vcmd_status_buf_mem_pool.virtual_address);
        return -1;
    }
    vcmd_registers_mem_pool.size = CMDBUF_VCMD_REGISTER_TOTAL_SIZE;
    vcmd_registers_mem_pool.bus_address = (ptr_t)vcmd_registers_mem_pool.virtual_address;
    vcmd_registers_mem_pool.cmdbuf_id = 0xFFFF;

    for (k = 0; k < MAX_VCMD_TYPE; k++) {
        vcmd_core_num[k] = 0;
        vcmd_position[k] = 0;
        for (i = 0; i < MAX_SAME_MODULE_TYPE_CORE_NUMBER; i++) {
            vcmd_manager[k][i] = NULL;
        }
    }

    for (i = 0; i < total_vcmd_core_num; i++) {
        hantrovcmd_data[i].vcmd_core_cfg.vcmd_core_config_ptr = &vcmd_core_array[i];
        hantrovcmd_data[i].vcmd_core_cfg.core_id = i;
        hantrovcmd_data[i].vcmd_core_cfg.vcmd_hw_version_id = VCMD_HW_ID_1_1_1;
        hantrovcmd_data[i].vcmd_core_cfg.dev_id = &hantrovcmd_data[i];
        hantrovcmd_data[i].hwregs = NULL;
        hantrovcmd_data[i].core_id = i;
        hantrovcmd_data[i].working_state = WORKING_STATE_IDLE;
        hantrovcmd_data[i].sw_cmdbuf_rdy_num = 0;
        hantrovcmd_data[i].total_exe_time = 0;
        //FIXME: support all formats
        hantrovcmd_data[i].formats = VWL_CLIENT_TYPE_ALL;
        init_bi_list(&hantrovcmd_data[i].list_manager);
        hantrovcmd_data[i].duration_without_int = 0;
        hantrovcmd_data[i].vcmd_reg_mem_bus_address =
            vcmd_registers_mem_pool.bus_address + i * VCMD_REGISTER_SIZE;
        hantrovcmd_data[i].vcmd_reg_mem_virtual_address =
            vcmd_registers_mem_pool.virtual_address + i * VCMD_REGISTER_SIZE / 4;
        hantrovcmd_data[i].vcmd_reg_mem_size = VCMD_REGISTER_SIZE;
        vcmd_run_once[i] = 0;
        memset(hantrovcmd_data[i].vcmd_reg_mem_virtual_address, 0, VCMD_REGISTER_SIZE);
    }

    for (i = 0; i < total_vcmd_core_num; i++) {
        int type = vcmd_core_array[i].sub_module_type;
        int core = vcmd_core_num[type];
        struct hantrovcmd_dev *dev = &hantrovcmd_data[i];
        dev->vcmd_core_cfg.vcmdcallback_ptr = vcmdcallbacks;
        dev->vcmd_core = AsicHwVcmdCoreInit(&dev->vcmd_core_cfg);
        dev->hwregs = (u8 *)AsicHwVcmdCoreGetBase(dev->vcmd_core);
        vcmd_manager[type][core] = dev;
        dev->id_in_type = core;
        hantrovcmd_data[i].hw_version_id = *(u32 *)dev->hwregs;
        basemem_reg[type][core] = dev->vcmd_core_cfg.baseMem;
        vcmd_core_num[type]++;
        vcmd_run_once[i] = 0;
        memset(hantrovcmd_data[i].vcmd_reg_mem_virtual_address, 0, VCMD_REGISTER_SIZE);
    }

    global_vcmd_release = 0;
    pthread_mutex_init(&global_vcmd_mutex, NULL);
    pthread_cond_init(&global_vcmd_cond_wait, NULL);

    vcmd_reset_asic(hantrovcmd_data);
    //for cmdbuf management
    cmdbuf_used_pos = 0;
    for (k = 0; k < TOTAL_DISCRETE_CMDBUF_NUM; k++) {
        cmdbuf_used[k] = 0;
        global_cmdbuf_node[k] = NULL;
    }

    cmdbuf_used_residual = TOTAL_DISCRETE_CMDBUF_NUM;
    cmdbuf_used_pos = 1;
    cmdbuf_used[0] = 1;
    cmdbuf_used_residual -= 1;

    //donot use cmdbuf 0 in default.
    sem_init(&reserved_sem, 0, TOTAL_DISCRETE_CMDBUF_NUM - 1);

    for (i = 0; i < MAX_VCMD_TYPE; i++) {
        if (vcmd_core_num[i] == 0)
            continue;
        sem_init(&vcmd_reserve_cmdbuf_sem[i], 0, 1);
    }

    /*read all registers for analyzing configuration in cwl*/
    for (i = 0; i < total_vcmd_core_num; i++) {
        read_main_module_all_registers(i);
    }

    return 0;
}

void CmodelVcmdRelease()
{
    int i;
    global_vcmd_release = 1;
    pthread_cond_destroy(&global_vcmd_cond_wait);
    pthread_mutex_destroy(&global_vcmd_mutex);

    for (i = 0; i < total_vcmd_core_num; i++) {
        release_cmdbuf_node_cleanup(&hantrovcmd_data[i].list_manager);
    }

    for (i = 0; i < MAX_VCMD_TYPE; i++) {
        vcmd_core_num[i] = 0;
    }

    for (i = 0; i < total_vcmd_core_num; i++) {
        AsicHwVcmdCoreRelease(vcmd_manager[vcmd_core_array[i].sub_module_type]
                                          [vcmd_core_num[vcmd_core_array[i].sub_module_type]]
                                              ->vcmd_core);
        vcmd_core_num[vcmd_core_array[i].sub_module_type]++;
    }

    for (i = 0; i < MAX_VCMD_TYPE; i++) {
        if (vcmd_core_num[i] == 0)
            continue;
        sem_destroy(&vcmd_reserve_cmdbuf_sem[i]);
    }

    free(vcmd_status_buf_mem_pool.virtual_address);
    free(vcmd_registers_mem_pool.virtual_address);
    free(vcmd_buf_mem_pool.virtual_address);
    free(hantrovcmd_data);

    sem_destroy(&reserved_sem);

    hantrovcmd_data = NULL;
    return;
}

static int vcmd_isr_cb(int irq, void *dev_id)
{
    unsigned int handled = 0;
    struct hantrovcmd_dev *dev = (struct hantrovcmd_dev *)dev_id;
    u32 irq_status = 0;
    bi_list_node *new_cmdbuf_node = NULL;
    bi_list_node *base_cmdbuf_node = NULL;
    struct cmdbuf_obj *cmdbuf_obj = NULL;
    ptr_t exe_cmdbuf_bus_address;
    u32 cmdbuf_processed_num = 0;
    u32 cmdbuf_id = 0;
    /*If core is not reserved by any user, but irq is received, just clean it*/
    if (dev->list_manager.head == NULL) {
        PDEBUG("%s\n", "vcmd_isr_cb:received IRQ but core has nothing to do.\n");
        irq_status = AsicHwVcmdCoreReadRegister(dev->vcmd_core, VCMD_REGISTER_INT_STATUS_OFFSET);
        AsicHwVcmdCoreWriteRegister(dev->vcmd_core, VCMD_REGISTER_INT_STATUS_OFFSET, irq_status);
        return IRQ_HANDLED;
    }

    PDEBUG("%s\n", "vcmd_isr_cb:received IRQ!\n");
    irq_status = AsicHwVcmdCoreReadRegister(dev->vcmd_core, VCMD_REGISTER_INT_STATUS_OFFSET);
#ifdef VCMD_DEBUG_INTERNAL
    {
        u32 i, fordebug;
        for (i = 0; i < ASIC_VCMD_SWREG_AMOUNT; i++) {
            fordebug = AsicHwVcmdCoreReadRegister(dev->vcmd_core, i * 4);
            printf("vcmd register %d:0x%x\n", i, fordebug);
        }
    }
#endif

    if (!irq_status) {
        //printk(KERN_INFO"vcmd_isr_cb error,irq_status :0x%x",irq_status);
        return IRQ_HANDLED;
    }

    PDEBUG("irq_status of %d is:%x\n", dev->core_id, irq_status);
    AsicHwVcmdCoreWriteRegister(dev->vcmd_core, VCMD_REGISTER_INT_STATUS_OFFSET, irq_status);
    dev->reg_mirror[VCMD_REGISTER_INT_STATUS_OFFSET / 4] = irq_status;

    if ((dev->hw_version_id > VCMD_HW_ID_1_0_C) && (irq_status & 0x3f)) {
        //if error,read from register directly.
        cmdbuf_id =
            vwl_get_register_value(dev->vcmd_core, dev->reg_mirror, HWIF_VCMD_CMDBUF_EXECUTING_ID);
        if (cmdbuf_id >= TOTAL_DISCRETE_CMDBUF_NUM) {
            printf("vcmd_isr_cb error cmdbuf_id greater than the ceiling !!\n");
            return IRQ_HANDLED;
        }
    } else if ((dev->hw_version_id > VCMD_HW_ID_1_0_C)) {
        //read cmdbuf id from ddr
#ifdef VCMD_DEBUG_INTERNAL
        {
            u32 i, fordebug;
            printf("ddr vcmd register phy_addr=0x%lx\n", dev->vcmd_reg_mem_bus_address);
            printf("ddr vcmd register virt_addr=0x%lx\n",
                   (long unsigned int)dev->vcmd_reg_mem_virtual_address);
            for (i = 0; i < ASIC_VCMD_SWREG_AMOUNT; i++) {
                fordebug = *(dev->vcmd_reg_mem_virtual_address + i);
                printf("ddr vcmd register %d:0x%x\n", i, fordebug);
            }
        }
#endif

        cmdbuf_id = *(dev->vcmd_reg_mem_virtual_address + EXECUTING_CMDBUF_ID_ADDR);
        if (cmdbuf_id >= TOTAL_DISCRETE_CMDBUF_NUM) {
            printf("vcmd_isr_cb error cmdbuf_id greater than the ceiling !!\n");
            return IRQ_HANDLED;
        }
    }

    if (vwl_get_register_mirror_value(dev->reg_mirror, HWIF_VCMD_IRQ_RESET)) {
        //reset error,all cmdbuf that is not  done will be run again.
        new_cmdbuf_node = dev->list_manager.head;
        dev->working_state = WORKING_STATE_IDLE;
        //find the first run_done=0
        while (1) {
            if (new_cmdbuf_node == NULL)
                break;
            cmdbuf_obj = (struct cmdbuf_obj *)new_cmdbuf_node->data;
            if ((cmdbuf_obj->cmdbuf_run_done == 0))
                break;
            new_cmdbuf_node = new_cmdbuf_node->next;
        }
        base_cmdbuf_node = new_cmdbuf_node;
        vcmd_delink_cmdbuf(dev, base_cmdbuf_node);
        vcmd_link_cmdbuf(dev, base_cmdbuf_node);
        if (dev->sw_cmdbuf_rdy_num != 0) {
            //restart new command
            vcmd_start(dev, base_cmdbuf_node);
        }
        handled++;
        return IRQ_HANDLED;
    }

    if (vwl_get_register_mirror_value(dev->reg_mirror, HWIF_VCMD_IRQ_ABORT)) {
        //abort error,don't need to reset
        new_cmdbuf_node = dev->list_manager.head;
        dev->working_state = WORKING_STATE_IDLE;
        if (dev->hw_version_id > VCMD_HW_ID_1_0_C) {
            new_cmdbuf_node = global_cmdbuf_node[cmdbuf_id];
            if (new_cmdbuf_node == NULL) {
                printf("vcmd_isr_cb error cmdbuf_id !!\n");
                return IRQ_HANDLED;
            }
        } else {
            exe_cmdbuf_bus_address = VWLGetAddrRegisterValue(dev->vcmd_core, dev->reg_mirror,
                                                             HWIF_VCMD_EXECUTING_CMD_ADDR);
            //find the cmderror cmdbuf
            while (1) {
                if (new_cmdbuf_node == NULL) {
                    return IRQ_HANDLED;
                }
                cmdbuf_obj = (struct cmdbuf_obj *)new_cmdbuf_node->data;
                if ((((cmdbuf_obj->cmdbuf_busAddress) <= exe_cmdbuf_bus_address) &&
                     (((cmdbuf_obj->cmdbuf_busAddress + cmdbuf_obj->cmdbuf_size) >
                       exe_cmdbuf_bus_address))) &&
                    (cmdbuf_obj->cmdbuf_run_done == 0))
                    break;
                new_cmdbuf_node = new_cmdbuf_node->next;
            }
        }

        base_cmdbuf_node = new_cmdbuf_node;
        // this cmdbuf and cmdbufs prior to itself, run_done = 1
        pthread_mutex_lock(&global_vcmd_mutex);
        while (1) {
            if (new_cmdbuf_node == NULL)
                break;
            cmdbuf_obj = (struct cmdbuf_obj *)new_cmdbuf_node->data;
            if ((cmdbuf_obj->cmdbuf_run_done == 0)) {
                cmdbuf_obj->cmdbuf_run_done = 1;
                cmdbuf_obj->executing_status = CMDBUF_EXE_STATUS_OK;
                cmdbuf_processed_num++;
                vcmd_run_once[((struct cmdbuf_obj *)(global_cmdbuf_node[cmdbuf_id]->data))
                                  ->core_id] = 1;
            } else
                break;
            new_cmdbuf_node = new_cmdbuf_node->previous;
        }
        pthread_cond_broadcast(&global_vcmd_cond_wait);
        pthread_mutex_unlock(&global_vcmd_mutex);
        base_cmdbuf_node = base_cmdbuf_node->next;
        /*if(cmdbuf_processed_num)
          wake_up_interruptible_all(dev->wait_queue);
        //to let high priority cmdbuf be inserted
        wake_up_interruptible_all(dev->wait_abort_queue);*/
        handled++;
        return IRQ_HANDLED;
    }

    if (vwl_get_register_mirror_value(dev->reg_mirror, HWIF_VCMD_IRQ_BUSERR)) {
        //bus error ,don't need to reset ， where to record status?
        new_cmdbuf_node = dev->list_manager.head;
        dev->working_state = WORKING_STATE_IDLE;
        if (dev->hw_version_id > VCMD_HW_ID_1_0_C) {
            new_cmdbuf_node = global_cmdbuf_node[cmdbuf_id];
            if (new_cmdbuf_node == NULL) {
                printf("vcmd_isr_cb error cmdbuf_id !!\n");
                return IRQ_HANDLED;
            }
        } else {
            exe_cmdbuf_bus_address = VWLGetAddrRegisterValue(dev->vcmd_core, dev->reg_mirror,
                                                             HWIF_VCMD_EXECUTING_CMD_ADDR);
            //find the buserr cmdbuf
            while (1) {
                if (new_cmdbuf_node == NULL) {
                    return IRQ_HANDLED;
                }
                cmdbuf_obj = (struct cmdbuf_obj *)new_cmdbuf_node->data;
                if ((((cmdbuf_obj->cmdbuf_busAddress) <= exe_cmdbuf_bus_address) &&
                     (((cmdbuf_obj->cmdbuf_busAddress + cmdbuf_obj->cmdbuf_size) >
                       exe_cmdbuf_bus_address))) &&
                    (cmdbuf_obj->cmdbuf_run_done == 0))
                    break;
                new_cmdbuf_node = new_cmdbuf_node->next;
            }
        }
        base_cmdbuf_node = new_cmdbuf_node;
        // this cmdbuf and cmdbufs prior to itself, run_done = 1
        pthread_mutex_lock(&global_vcmd_mutex);
        while (1) {
            if (new_cmdbuf_node == NULL)
                break;
            cmdbuf_obj = (struct cmdbuf_obj *)new_cmdbuf_node->data;
            if ((cmdbuf_obj->cmdbuf_run_done == 0)) {
                cmdbuf_obj->cmdbuf_run_done = 1;
                cmdbuf_obj->executing_status = CMDBUF_EXE_STATUS_OK;
                cmdbuf_processed_num++;
                vcmd_run_once[((struct cmdbuf_obj *)(global_cmdbuf_node[cmdbuf_id]->data))
                                  ->core_id] = 1;
            } else
                break;
            new_cmdbuf_node = new_cmdbuf_node->previous;
        }
        pthread_cond_broadcast(&global_vcmd_cond_wait);
        pthread_mutex_unlock(&global_vcmd_mutex);
        new_cmdbuf_node = base_cmdbuf_node;
        if (new_cmdbuf_node != NULL) {
            cmdbuf_obj = (struct cmdbuf_obj *)new_cmdbuf_node->data;
            cmdbuf_obj->executing_status = CMDBUF_EXE_STATUS_BUSERR;
        }
        base_cmdbuf_node = base_cmdbuf_node->next;
        vcmd_link_cmdbuf(dev, base_cmdbuf_node);
        if (dev->sw_cmdbuf_rdy_num != 0) {
            //restart new command
            vcmd_start(dev, base_cmdbuf_node);
        }
        //if(cmdbuf_processed_num)
        //wake_up_interruptible_all(dev->wait_queue);
        handled++;
        return IRQ_HANDLED;
    }

    if (vwl_get_register_mirror_value(dev->reg_mirror, HWIF_VCMD_IRQ_TIMEOUT)) {
        //time out,need to reset
        new_cmdbuf_node = dev->list_manager.head;
        dev->working_state = WORKING_STATE_IDLE;
        if (dev->hw_version_id > VCMD_HW_ID_1_0_C) {
            new_cmdbuf_node = global_cmdbuf_node[cmdbuf_id];
            if (new_cmdbuf_node == NULL) {
                printf("vcmd_isr_cb error cmdbuf_id !!\n");
                return IRQ_HANDLED;
            }
        } else {
            exe_cmdbuf_bus_address = VWLGetAddrRegisterValue(dev->vcmd_core, dev->reg_mirror,
                                                             HWIF_VCMD_EXECUTING_CMD_ADDR);
            //find the timeout cmdbuf
            while (1) {
                if (new_cmdbuf_node == NULL) {
                    return IRQ_HANDLED;
                }
                cmdbuf_obj = (struct cmdbuf_obj *)new_cmdbuf_node->data;
                if ((((cmdbuf_obj->cmdbuf_busAddress) <= exe_cmdbuf_bus_address) &&
                     (((cmdbuf_obj->cmdbuf_busAddress + cmdbuf_obj->cmdbuf_size) >
                       exe_cmdbuf_bus_address))) &&
                    (cmdbuf_obj->cmdbuf_run_done == 0))
                    break;
                new_cmdbuf_node = new_cmdbuf_node->next;
            }
        }
        base_cmdbuf_node = new_cmdbuf_node;
        new_cmdbuf_node = new_cmdbuf_node->previous;
        // this cmdbuf and cmdbufs prior to itself, run_done = 1
        pthread_mutex_lock(&global_vcmd_mutex);
        while (1) {
            if (new_cmdbuf_node == NULL)
                break;
            cmdbuf_obj = (struct cmdbuf_obj *)new_cmdbuf_node->data;
            if ((cmdbuf_obj->cmdbuf_run_done == 0)) {
                cmdbuf_obj->cmdbuf_run_done = 1;
                cmdbuf_obj->executing_status = CMDBUF_EXE_STATUS_OK;
                cmdbuf_processed_num++;
                vcmd_run_once[((struct cmdbuf_obj *)(global_cmdbuf_node[cmdbuf_id]->data))
                                  ->core_id] = 1;
            } else
                break;
            new_cmdbuf_node = new_cmdbuf_node->previous;
        }
        pthread_cond_broadcast(&global_vcmd_cond_wait);
        pthread_mutex_unlock(&global_vcmd_mutex);
        vcmd_link_cmdbuf(dev, base_cmdbuf_node);
        if (dev->sw_cmdbuf_rdy_num != 0) {
            //reset
            vcmd_reset_current_asic(dev);
            //restart new command
            vcmd_start(dev, base_cmdbuf_node);
        }
        //if(cmdbuf_processed_num)
        //wake_up_interruptible_all(dev->wait_queue);
        handled++;
        return IRQ_HANDLED;
    }

    if (vwl_get_register_mirror_value(dev->reg_mirror, HWIF_VCMD_IRQ_CMDERR)) {
        //command error,don't need to reset
        new_cmdbuf_node = dev->list_manager.head;
        dev->working_state = WORKING_STATE_IDLE;
        if (dev->hw_version_id > VCMD_HW_ID_1_0_C) {
            new_cmdbuf_node = global_cmdbuf_node[cmdbuf_id];
            if (new_cmdbuf_node == NULL) {
                printf("vcmd_isr_cb error cmdbuf_id !!\n");
                return IRQ_HANDLED;
            }
        } else {
            exe_cmdbuf_bus_address = VWLGetAddrRegisterValue(dev->vcmd_core, dev->reg_mirror,
                                                             HWIF_VCMD_EXECUTING_CMD_ADDR);
            //find the cmderror cmdbuf
            while (1) {
                if (new_cmdbuf_node == NULL) {
                    return IRQ_HANDLED;
                }
                cmdbuf_obj = (struct cmdbuf_obj *)new_cmdbuf_node->data;
                if ((((cmdbuf_obj->cmdbuf_busAddress) <= exe_cmdbuf_bus_address) &&
                     (((cmdbuf_obj->cmdbuf_busAddress + cmdbuf_obj->cmdbuf_size) >
                       exe_cmdbuf_bus_address))) &&
                    (cmdbuf_obj->cmdbuf_run_done == 0))
                    break;
                new_cmdbuf_node = new_cmdbuf_node->next;
            }
        }
        base_cmdbuf_node = new_cmdbuf_node;
        // this cmdbuf and cmdbufs prior to itself, run_done = 1
        pthread_mutex_lock(&global_vcmd_mutex);
        while (1) {
            if (new_cmdbuf_node == NULL)
                break;
            cmdbuf_obj = (struct cmdbuf_obj *)new_cmdbuf_node->data;
            if ((cmdbuf_obj->cmdbuf_run_done == 0)) {
                cmdbuf_obj->cmdbuf_run_done = 1;
                cmdbuf_obj->executing_status = CMDBUF_EXE_STATUS_OK;
                cmdbuf_processed_num++;
                vcmd_run_once[((struct cmdbuf_obj *)(global_cmdbuf_node[cmdbuf_id]->data))
                                  ->core_id] = 1;
            } else
                break;
            new_cmdbuf_node = new_cmdbuf_node->previous;
        }
        pthread_cond_broadcast(&global_vcmd_cond_wait);
        pthread_mutex_unlock(&global_vcmd_mutex);

        new_cmdbuf_node = base_cmdbuf_node;
        if (new_cmdbuf_node != NULL) {
            cmdbuf_obj = (struct cmdbuf_obj *)new_cmdbuf_node->data;
            cmdbuf_obj->executing_status = CMDBUF_EXE_STATUS_CMDERR; //cmderr
        }
        base_cmdbuf_node = base_cmdbuf_node->next;
        vcmd_link_cmdbuf(dev, base_cmdbuf_node);
        if (dev->sw_cmdbuf_rdy_num != 0) {
            //restart new command
            vcmd_start(dev, base_cmdbuf_node);
        }
        //if(cmdbuf_processed_num)
        //wake_up_interruptible_all(dev->wait_queue);
        handled++;
        return IRQ_HANDLED;
    }

    if (vwl_get_register_mirror_value(dev->reg_mirror, HWIF_VCMD_IRQ_ENDCMD)) {
        //end command interrupt
        new_cmdbuf_node = dev->list_manager.head;
        dev->working_state = WORKING_STATE_IDLE;
        if (dev->hw_version_id > VCMD_HW_ID_1_0_C) {
            new_cmdbuf_node = global_cmdbuf_node[cmdbuf_id];
            if (new_cmdbuf_node == NULL) {
                printf("vcmd_isr_cb error cmdbuf_id !!\n");
                return IRQ_HANDLED;
            }
        } else {
            //find the end cmdbuf
            while (1) {
                if (new_cmdbuf_node == NULL) {
                    return IRQ_HANDLED;
                }
                cmdbuf_obj = (struct cmdbuf_obj *)new_cmdbuf_node->data;
                if ((cmdbuf_obj->has_end_cmdbuf == 1) && (cmdbuf_obj->cmdbuf_run_done == 0))
                    break;
                new_cmdbuf_node = new_cmdbuf_node->next;
            }
        }
        base_cmdbuf_node = new_cmdbuf_node;
        // this cmdbuf and cmdbufs prior to itself, run_done = 1
        pthread_mutex_lock(&global_vcmd_mutex);
        while (1) {
            if (new_cmdbuf_node == NULL)
                break;
            cmdbuf_obj = (struct cmdbuf_obj *)new_cmdbuf_node->data;
            if ((cmdbuf_obj->cmdbuf_run_done == 0)) {
                cmdbuf_obj->cmdbuf_run_done = 1;
                cmdbuf_obj->executing_status = CMDBUF_EXE_STATUS_OK;
                cmdbuf_processed_num++;
                vcmd_run_once[((struct cmdbuf_obj *)(global_cmdbuf_node[cmdbuf_id]->data))
                                  ->core_id] = 1;
            } else
                break;
            new_cmdbuf_node = new_cmdbuf_node->previous;
        }
        pthread_cond_broadcast(&global_vcmd_cond_wait);
        pthread_mutex_unlock(&global_vcmd_mutex);

        base_cmdbuf_node = base_cmdbuf_node->next;
        vcmd_link_cmdbuf(dev, base_cmdbuf_node);
        if (dev->sw_cmdbuf_rdy_num != 0) {
            //restart new command
            vcmd_start(dev, base_cmdbuf_node);
        }
        handled++;
        return IRQ_HANDLED;
    }
    if (dev->hw_version_id <= VCMD_HW_ID_1_0_C)
        cmdbuf_id = vwl_get_register_mirror_value(dev->reg_mirror, HWIF_VCMD_IRQ_INTCMD);
    if (cmdbuf_id) {
        if (dev->hw_version_id <= VCMD_HW_ID_1_0_C) {
            if (cmdbuf_id >= TOTAL_DISCRETE_CMDBUF_NUM) {
                printf("vcmd_isr_cb error cmdbuf_id greater than the ceiling !!\n");
                return IRQ_HANDLED;
            }
        }
        new_cmdbuf_node = global_cmdbuf_node[cmdbuf_id];
        if (new_cmdbuf_node == NULL) {
            printf("vcmd_isr_cb error cmdbuf_id !!\n");
            return IRQ_HANDLED;
        }
        // interrupt cmdbuf and cmdbufs prior to itself, run_done = 1
        pthread_mutex_lock(&global_vcmd_mutex);
        while (1) {
            if (new_cmdbuf_node == NULL)
                break;
            cmdbuf_obj = (struct cmdbuf_obj *)new_cmdbuf_node->data;
            if ((cmdbuf_obj->cmdbuf_run_done == 0)) {
                cmdbuf_obj->cmdbuf_run_done = 1;
                cmdbuf_obj->executing_status = CMDBUF_EXE_STATUS_OK;
                cmdbuf_processed_num++;
                vcmd_run_once[((struct cmdbuf_obj *)(global_cmdbuf_node[cmdbuf_id]->data))
                                  ->core_id] = 1;
            } else {
                break;
            }
            new_cmdbuf_node = new_cmdbuf_node->previous;
        }

        pthread_cond_broadcast(&global_vcmd_cond_wait);
        pthread_mutex_unlock(&global_vcmd_mutex);
        handled++;
    }
#if 0
    if (cmdbuf_processed_num)
        sem_post(&vcmd_rdy[((struct cmdbuf_obj*)(global_cmdbuf_node[cmdbuf_id]->data))->core_id]);
#endif
    //wake_up_interruptible_all(dev->wait_queue);
    if (!handled) {
        PDEBUG("%s\n", "IRQ received, but not hantro's!");
    }
    return IRQ_HANDLED;
}

static u32 vcmd_read_buf_cb(u32 *addr, void *dev_id)
{
    u32 ret;
    ptr_t addr_tmp;
    addr_tmp = ((ptr_t)addr) ^ MASK_KEY;
    ret = *(u32 *)addr_tmp;
    return ret;
}

static void vcmd_write_buf_cb(u32 *addr, u32 value, void *dev_id)
{
    ptr_t addr_tmp;
    addr_tmp = ((ptr_t)addr) ^ MASK_KEY;
    *(u32 *)addr_tmp = value;
}

static u32 vcmd_read_reg_cb(void *core, u32 reg_idx, u32 core_type, void *dev_id)
{
    u32 res = 0;
    u32 offset;
    offset = reg_idx * 4;
    if (core_type == MODULE_MAIN_CORE || core_type == MODULE_VCMD) {
        res = AsicHwSubsysGetRegister(core, offset);
    } else {
        /*DEC400, CACHE ...*/
    }
    return res;
}

static void vcmd_write_reg_cb(void *core, u32 reg_idx, u32 value, u32 core_type, void *dev_id)
{
    u32 offset;
    offset = reg_idx * 4;
    if (core_type == MODULE_MAIN_CORE) {
        AsicHwSubsysSetRegister(core, offset, value);
        if ((offset == ASIC_REG_INDEX_STATUS * 4) && (value & 0x01)) {
            AsicHwSubsysCoreRun(core);
        }
    } else {
        /*DEC400, CACHE ...*/
    }
}

static void vcmd_jmp_cb(void *core, u32 core_type, void *dev_id)
{
    printf("Finished Processing One VcmdBuf!\n");
}

static void vcmd_stall_cb(void *core, u32 core_type, void *dev_id)
{
    if (core_type == MODULE_MAIN_CORE) {
        AsicHwSubsysCoreWait(core);
    } else {
        /*DEC400, CACHE ...*/
    }
}

static void vcmd_reset_asic(struct hantrovcmd_dev *dev)
{
    int i, n;
    u32 result;
    for (n = 0; n < total_vcmd_core_num; n++) {
        if (dev[n].hwregs != NULL) {
            //disable interrupt at first
            AsicHwVcmdCoreWriteRegister(dev[n].vcmd_core, VCMD_REGISTER_INT_CTL_OFFSET, 0x0000);
            //reset all
            AsicHwVcmdCoreWriteRegister(dev[n].vcmd_core, VCMD_REGISTER_CONTROL_OFFSET, 0x0002);
            //read status register
            result = AsicHwVcmdCoreReadRegister(dev[n].vcmd_core, VCMD_REGISTER_INT_STATUS_OFFSET);
            //clean status register
            AsicHwVcmdCoreWriteRegister(dev[n].vcmd_core, VCMD_REGISTER_INT_STATUS_OFFSET, result);
            for (i = VCMD_REGISTER_CONTROL_OFFSET;
                 i < dev[n].vcmd_core_cfg.vcmd_core_config_ptr->vcmd_iosize; i += 4) {
                //set all register 0
                AsicHwVcmdCoreWriteRegister(dev[n].vcmd_core, i, 0x0000);
            }
            //enable all interrupt
            AsicHwVcmdCoreWriteRegister(dev[n].vcmd_core, VCMD_REGISTER_INT_CTL_OFFSET, 0xffffffff);
        }
    }
}

static void vcmd_reset_current_asic(struct hantrovcmd_dev *dev)
{
    u32 result;

    if (dev->hwregs != NULL) {
        //disable interrupt at first
        AsicHwVcmdCoreWriteRegister(dev->vcmd_core, VCMD_REGISTER_INT_CTL_OFFSET, 0x0000);
        //reset all
        AsicHwVcmdCoreWriteRegister(dev->vcmd_core, VCMD_REGISTER_CONTROL_OFFSET, 0x0002);
        //read status register
        result = AsicHwVcmdCoreReadRegister(dev->vcmd_core, VCMD_REGISTER_INT_STATUS_OFFSET);
        //clean status register
        AsicHwVcmdCoreWriteRegister(dev->vcmd_core, VCMD_REGISTER_INT_STATUS_OFFSET, result);
    }
}
