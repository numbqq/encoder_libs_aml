#include <unistd.h>/*
* Copyright (c) 2019 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in below
* which is part of this source code package.
*
* Description:
*/

// Copyright (C) 2019 Amlogic, Inc. All rights reserved.
//
// All information contained herein is Amlogic confidential.
//
// This software is provided to you pursuant to Software License
// Agreement (SLA) with Amlogic Inc ("Amlogic"). This software may be
// used only in accordance with the terms of this agreement.
//
// Redistribution and use in source and binary forms, with or without
// modification is strictly prohibited without prior written permission
// from Amlogic.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <unistd.h>

#include "vdi.h"
#include "tahoe.h"

extern venc_info_t s_venc_info;
/*extern */int venc_init_flag = 0;

debug_log_level_t g_tahoe_log_level = ERR;

#define TAHOE_DEVICE_NAME "/dev/tahoe"

void tahoe_set_log_level(debug_log_level_t level)
{
    char *log_level = getenv("TAHOE_LOG_LEVEL");
    if (log_level) {
        g_tahoe_log_level = (debug_log_level_t)atoi(log_level);
        #ifdef __ANDROID__
        ALOGE("Set log level by environment to %d\n", g_tahoe_log_level);
        #else
        printf("Set log level by environment to %d\n", g_tahoe_log_level);
        #endif
    } else {
        g_tahoe_log_level = level;
        #ifdef __ANDROID__
        ALOGE("Set log level to %d\n", g_tahoe_log_level);
        #else
        printf("Set log level to %d\n", g_tahoe_log_level);
        #endif
    }
    return;
}


int venc_lock(void)
{
    venc_info_t *venc;
    venc = &s_venc_info;

    if (!venc || venc->venc_fd == -1 ||
        venc_init_flag == INIT_VDI_STAT_NULL)
        return -1;
    if (pthread_mutex_lock(&venc->vmem_mutex) != 0) {
        VLOG(ERR,"%s:%d failed to pthread_mutex_locK\n", __FUNCTION__, __LINE__);
        return -1;
    }
    return 0;
}

void venc_unlock(void)
{
    venc_info_t *venc;

    venc = &s_venc_info;

    if (!venc || venc->venc_fd == -1 ||
        venc_init_flag == INIT_VDI_STAT_NULL)
        return;

    pthread_mutex_unlock(&venc->vmem_mutex);//lint !e455
}

int vdi_debug(unsigned int data)
{
    venc_info_t *venc = &s_venc_info;
    int ret = -1;
    ret = ioctl(venc->venc_fd, IOC_NR_FOR_DEBUG, (void*)&data);
    return ret;
}

int vdi_flush_memory(venc_buffer_t* vb)
{
    venc_info_t *venc = &s_venc_info;
    int ret = -1;
    venc_buffer_t vdb;

    if (!vb)
        return -1;
    memset(&vdb, 0x00, sizeof(venc_buffer_t));

    //__need_modify__
    //待加范围判断
    vdb = *vb;
    if (vdb.size > 0)
        ret = ioctl(venc->venc_fd, IOC_FLUSH_BUFFER, (void*)&vdb);
    return ret;
}

int vdi_invalidate_memory(venc_buffer_t *vb)
{
    venc_info_t *venc = &s_venc_info;
    int ret = -1;
    venc_buffer_t vdb;

    if (!vb)
        return -1;
    memset(&vdb, 0x00, sizeof(venc_buffer_t));

    //__need_modify__
    //待加范围判断
    vdb = *vb;
    if (vdb .size > 0)
        ret = ioctl(venc->venc_fd, IOC_CACHE_INV_BUFFER, (void*)&vdb);
    return ret;
}

void safe_memcpy(void *dst, const void *src, size_t n) {
    volatile char *d = dst;
    volatile const char *s = src;
    for (size_t i = 0; i < n; i++) {
        d[i] = s[i];
    }
}

int vdi_read_memory(ulong addr, unsigned char *data, int len)
{
    venc_info_t *venc;
    venc_buffer_t vdb;
    ulong offset;
    int i;

    venc = &s_venc_info;
    int ret = -1;

    if (!venc || venc->venc_fd == -1)
        return -1;

    memset(&vdb, 0x00, sizeof(venc_buffer_t));

    for (i=0; i<MAX_VPU_BUFFER_POOL; i++)
    {
        if (venc->vpu_buffer_pool[i].inuse == 1)
        {
            vdb = venc->vpu_buffer_pool[i].vdb;
            if (addr >= vdb.phys_addr && addr < (vdb.phys_addr + vdb.size))
                break;
        }
    }

    if (!vdb.size)
        return -1;

    offset = addr - (ulong)vdb.phys_addr;

    vdb.phys_addr = addr;
    vdb.size = len;

    len = ((len + 15) >> 4) << 4;
    memcpy(data, (const void *)((ulong)vdb.virt_addr+offset), len);

    return len;
}

int venc_init(void)
{
    venc_info_t *venc = &s_venc_info;

    if (venc_init_flag == INIT_VDI_STAT_NULL) {
        venc_init_flag = INIT_VDI_STAT_INIT;
        memset(&s_venc_info, 0x00, sizeof(s_venc_info));
        s_venc_info.venc_fd = -1;
    }

    if (venc->venc_fd != -1 && venc_init_flag == INIT_VDI_STAT_DONE)
    {
        VLOG(INFO,"[VDI] opened already.\n");
//        vdi->task_num++;
        return 0;
    }
    venc_init_flag = INIT_VDI_STAT_OPENING;

    venc->venc_fd = open(TAHOE_DEVICE_NAME, O_RDWR);
    if (venc->venc_fd < 0) {
        VLOG(INFO,"[VENC] Can't open venc driver. [error=%s]. \n", strerror(errno));
        return -1;
    }
    VLOG(INFO,"open venc fd:%d,&venc:%lx!",venc->venc_fd,(unsigned long)venc);

    memset(venc->vpu_buffer_pool, 0x00, sizeof(vpudrv_buffer_pool_t)*MAX_VPU_BUFFER_POOL);
    venc_init_flag = INIT_VDI_STAT_ALLOC;
//    if (!venc_get_instance_pool())
//    {
//        printf("[VDI] fail to create shared info for saving context \n");
//        return -1;
//    }
    if (ioctl(venc->venc_fd, IOC_GET_REGISTER_INFO, &venc->venc_register) < 0)
    {
        VLOG(ERR,"[VENC] fail to get host interface register\n");
    }
    venc->venc_register.virt_addr = (unsigned long)mmap(NULL, venc->venc_register.size, PROT_READ | PROT_WRITE, MAP_SHARED, venc->venc_fd, venc->venc_register.phys_addr);
    if ((void *)venc->venc_register.virt_addr == MAP_FAILED)
    {
        VLOG(ERR,"[VENC] fail to map venc registers \n");
    }
    VLOG(INFO,"[VENC] map venc_register paddr=0x%lx, virtaddr=0x%lx, size=0x%x\n", venc->venc_register.phys_addr,venc->venc_register.virt_addr, venc->venc_register.size);

    if (ioctl(venc->venc_fd, IOC_GET_COMMON_BUF_INFO, &venc->common_buffer) < 0)
    {
        VLOG(ERR,"[VENC] fail to get common buffer info\n");
    }
    venc->common_buffer.virt_addr = (unsigned long)mmap(NULL, venc->common_buffer.size, PROT_READ | PROT_WRITE, MAP_SHARED, venc->venc_fd, venc->common_buffer.phys_addr);
    if ((void *)venc->common_buffer.virt_addr == MAP_FAILED)
    {
        VLOG(ERR,"[VENC] fail to map common buffer\n");
    }
    VLOG(INFO,"[VENC] map common buffer paddr=0x%lx, virtaddr=0x%lx, size=0x%x\n", venc->common_buffer.phys_addr,venc->common_buffer.virt_addr, venc->common_buffer.size);

    if (ioctl(venc->venc_fd, IOC_GET_FW_BUF_INFO, &venc->fw_buffer) < 0)
    {
        VLOG(ERR,"[VENC] fail to get common buffer info\n");
    }
    venc->fw_buffer.virt_addr = (unsigned long)mmap(NULL, venc->fw_buffer.size, PROT_READ | PROT_WRITE, MAP_SHARED, venc->venc_fd, venc->fw_buffer.phys_addr);
    if ((void *)venc->fw_buffer.virt_addr == MAP_FAILED)
    {
        VLOG(ERR,"[VENC] fail to map common buffer\n");
    }
    VLOG(INFO,"[VENC] map fw buffer paddr=0x%lx, virtaddr=0x%lx, size=0x%x\n", venc->fw_buffer.phys_addr,venc->fw_buffer.virt_addr, venc->fw_buffer.size);

    VLOG(INFO,"[VENC] success to init driver \n");

    venc_init_flag = INIT_VDI_STAT_DONE;
    return 0;
}

int venc_destroy(void)
{
    int i = 0;
    venc_info_t *venc = &s_venc_info;
    venc_buffer_t vdb;

    if (NULL != venc && venc->venc_fd != -1)
    {
        if (venc->venc_register.virt_addr)
            munmap((void *)venc->venc_register.virt_addr, venc->venc_register.size);
        if (venc->fw_buffer.virt_addr)
            munmap((void *)venc->fw_buffer.virt_addr, venc->fw_buffer.size);
        if (venc->common_buffer.virt_addr)
            munmap((void *)venc->common_buffer.virt_addr, venc->common_buffer.size);

        for (i=0; i<MAX_VPU_BUFFER_POOL; i++)
        {
            venc->vpu_buffer_pool[i].inuse = 0;
            venc->vpu_buffer_pool_count--;
            vdb = venc->vpu_buffer_pool[i].vdb;
            if (vdb.size > 0)
            {
                munmap((void *)vdb.virt_addr, vdb.size);
            }
            if  (venc->vpu_buffer_pool_count == 0)
                break;
        }
        close(venc->venc_fd);
        venc->venc_fd = -1;
    }
    if (venc_init_flag != INIT_VDI_STAT_NULL) {
        venc_init_flag = INIT_VDI_STAT_NULL;
    }
    return 0;
}

int vdi_allocate_dma_memory(venc_buffer_t *vb)
{
    venc_info_t *venc = &s_venc_info;
    int i;
    venc_buffer_t vdb;

    if (!venc || venc->venc_fd == -1)
        return -1;

    memset(&vdb, 0x00, sizeof(venc_buffer_t));

    vdb.size = vb->size;
    vdb.cached = 1;
    if (ioctl(venc->venc_fd, IOC_ALLOCATE_PHYSICAL_MEMORY, &vdb) < 0)
    {
        VLOG(ERR,"fail to vdi_allocate_dma_memory size=%d\n", vb->size);
        return -1;
    }
    vb->phys_addr = (ulong)vdb.phys_addr;
    vb->base = (ulong)vdb.base;
    //map to virtual address
    vdb.virt_addr = (ulong)mmap(NULL, vdb.size, PROT_READ | PROT_WRITE,
        MAP_SHARED, venc->venc_fd, vdb.phys_addr);
    if ((void *)vdb.virt_addr == MAP_FAILED)
    {
        memset(vb, 0x00, sizeof(venc_buffer_t));
        return -1;
    }
    vb->virt_addr = vdb.virt_addr;
    //vmem_lock(vdi);
    venc_lock();
    for (i=0; i<MAX_VPU_BUFFER_POOL; i++)
    {
        if (venc->vpu_buffer_pool[i].inuse == 0)
        {
            venc->vpu_buffer_pool[i].vdb = vdb;
            venc->vpu_buffer_pool_count++;
            venc->vpu_buffer_pool[i].inuse = 1;
            break;
        }
    }
    venc_unlock();
    //vmem_unlock(vdi);

    VLOG(INFO,"vdi_allocate_dma_memory, physaddr=%lx, virtaddr=%p~%p, size=%d\n",
        vb->phys_addr, (void *)vb->virt_addr, (void *)(vb->virt_addr + vb->size), vb->size);
    return 0;
}

int vdi_config_dma(tahoe_dma_buf_info_t *info)
{
    int ret;
    venc_info_t *venc = &s_venc_info;

    if (!venc || venc->venc_fd == -1 ||
        venc_init_flag == INIT_VDI_STAT_NULL)
        return -1;

    VLOG(INFO,"[VDI] CONFIG_DMA plane %d fd(%d, %d, %d)\n",
         info->num_planes, info->fd[0], info->fd[1], info->fd[2]);
    ret = ioctl(venc->venc_fd, IOC_CONFIG_DMA, (void*)info);
    VLOG(INFO,"[VDI] DMA physic %d phy(%lx, %lx, %lx)\n",
     info->num_planes, info->phys_addr[0], info->phys_addr[1], info->phys_addr[2]);
    return ret;
}

int vdi_unmap_dma(tahoe_dma_buf_info_t *info)
{
    int ret;
    venc_info_t *venc = &s_venc_info;

    if (!venc || venc->venc_fd == -1 ||
        venc_init_flag == INIT_VDI_STAT_NULL)
        return -1;

    ret = ioctl(venc->venc_fd, IOC_UNMAP_DMA, (void*)info);

    return ret;
}