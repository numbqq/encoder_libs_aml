/*
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

#include <stdio.h>
#include <pthread.h>
#include <sys/mman.h>
#include "tahoe.h"
#include "vp_codec_tahoe.h"
#include "vdi.h"
#include "enc_driver.h"
#include "tahoe_register.h"
#include <string.h>
#include <sys/poll.h>
#include <linux/videodev2.h>
#include <linux/v4l2-controls.h>
#include <errno.h>

#define MAX_FRAME_WINDOW       (120)
venc_info_t s_venc_info;

#include <fcntl.h>      // open(), O_RDWR, O_RDONLY...
#include <unistd.h>     // close(), read(), write()
#include <stdlib.h>


struct v4l2_buffer *src_buf_info = NULL;
struct v4l2_buffer *dst_buf_info = NULL;
unsigned char **src_buffers = NULL;
unsigned char **dst_buffers = NULL;
int *src_buf_lengths = NULL;
int *dst_buf_lengths = NULL;
int src_buf_count = 0, dst_buf_count = 0;
//int frame_count = 0;
//struct v4l2_buffer src_buf, dst_buf;
struct timespec ts_start, ts_end;
#define CLEAR(x) memset(&(x), 0, sizeof(x))


void query_and_set_control(int fd, unsigned int id, int value) {
    struct v4l2_queryctrl query;
    struct v4l2_control ctrl;
    
    CLEAR(query);
    query.id = id;
    
    if (ioctl(fd, VIDIOC_QUERYCTRL, &query) == 0) {
        printf("[%s] range: %d - %d, default: %d, step: %d\n", 
               query.name, query.minimum, query.maximum, query.default_value, query.step);
        
        if (value >= query.minimum && value <= query.maximum) {
            CLEAR(ctrl);
            ctrl.id = id;
            ctrl.value = value;
            if (ioctl(fd, VIDIOC_S_CTRL, &ctrl) == 0) {
                printf("[%s] set to %d successfully\n", query.name, value);
            } else {
                perror("VIDIOC_S_CTRL failed");
            }
        } else {
            printf("[%s] value %d out of range [%d, %d], skipping\n", 
                   query.name, value, query.minimum, query.maximum);
        }
    } else {
        printf("Control 0x%08x not supported\n", id);
    }
}

void setup_encoder_params(int fd, handle_t *encode_info) {
    printf("\n=== Setting up encoder parameters ===\n");
    
    // GOP
    query_and_set_control(fd, V4L2_CID_MPEG_VIDEO_GOP_SIZE, encode_info->enc_gop);
    
    // Bitrate
    query_and_set_control(fd, V4L2_CID_MPEG_VIDEO_BITRATE, encode_info->enc_bitrate);
    
    // H.264 Profile
    if (encode_info->profile == 3 && encode_info->enc_fmt == CODEC_ID_H264)
        query_and_set_control(fd, V4L2_CID_MPEG_VIDEO_H264_PROFILE, V4L2_MPEG_VIDEO_H264_PROFILE_HIGH);
//    
//    // H.264 Level
    if (encode_info->enc_fmt == CODEC_ID_H264)
        query_and_set_control(fd, V4L2_CID_MPEG_VIDEO_H264_LEVEL, V4L2_MPEG_VIDEO_H264_LEVEL_4_0);
//    
    if (encode_info->profile == 3 && encode_info->enc_fmt == CODEC_ID_H265)
        query_and_set_control(fd, V4L2_CID_MPEG_VIDEO_HEVC_PROFILE, V4L2_MPEG_VIDEO_HEVC_PROFILE_MAIN);

    if (encode_info->enc_fmt == CODEC_ID_H265)
        query_and_set_control(fd, V4L2_CID_MPEG_VIDEO_HEVC_LEVEL, V4L2_MPEG_VIDEO_HEVC_LEVEL_4);
//    query_and_set_control(fd, V4L2_CID_MPEG_VIDEO_HEVC_TIER_FLAG, 
//                          V4L2_MPEG_VIDEO_HEVC_TIER_MAIN);
}

int setup_src_format_mplane(int fd, handle_t *encode_info,int plane_num) {
    struct v4l2_format fmt;
    CLEAR(fmt);
    fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    fmt.fmt.pix_mp.width = encode_info->enc_width;
    fmt.fmt.pix_mp.height = encode_info->enc_height;
    if (encode_info->mfdin_input_fmt == MFDI_INPUT_FMT_NV12)
        fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_NV12;
    else if (encode_info->mfdin_input_fmt == MFDI_INPUT_FMT_NV21)
        fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_NV21;
    else if (encode_info->mfdin_input_fmt == MFDI_INPUT_FMT_YUV420_PLANAR)
        fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_YUV420;
    else if (encode_info->mfdin_input_fmt == MFDI_INPUT_FMT_RGB888)
        fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_RGB24;
    #if 0
    else if (encode_info->mfdin_input_fmt == MFDI_INPUT_FMT_RGBA8888)
        fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_RGBA32;
    #endif
    fmt.fmt.pix_mp.field = V4L2_FIELD_NONE;
    fmt.fmt.pix_mp.colorspace = V4L2_COLORSPACE_REC709;

    printf("setup_src_format_mplane fmt input:%d\n",encode_info->mfdin_input_fmt);
    if (encode_info->current_mode == DMA_TYPE) { //dmabuf
        if (encode_info->mfdin_input_fmt == MFDI_INPUT_FMT_NV12)
            fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_NV12M;
        else if (encode_info->mfdin_input_fmt == MFDI_INPUT_FMT_NV21)
            fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_NV21M;
        else if (encode_info->mfdin_input_fmt == MFDI_INPUT_FMT_YUV420_PLANAR)
            fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_YUV420M;
        fmt.fmt.pix_mp.num_planes = plane_num;
        if (1 == fmt.fmt.pix_mp.num_planes) { //rgb rgba?
            fmt.fmt.pix_mp.plane_fmt[0].bytesperline = encode_info->enc_width;
            fmt.fmt.pix_mp.plane_fmt[0].sizeimage = encode_info->enc_width * encode_info->enc_height * 3 / 2;
        }
        else if (2 == fmt.fmt.pix_mp.num_planes) {
            fmt.fmt.pix_mp.plane_fmt[0].bytesperline = encode_info->enc_width;
            fmt.fmt.pix_mp.plane_fmt[0].sizeimage = encode_info->enc_width * encode_info->enc_height;

            fmt.fmt.pix_mp.plane_fmt[1].bytesperline = encode_info->enc_width;
            fmt.fmt.pix_mp.plane_fmt[1].sizeimage = encode_info->enc_width * encode_info->enc_height / 2;
        }
        else if (3 == fmt.fmt.pix_mp.num_planes){
            fmt.fmt.pix_mp.plane_fmt[0].bytesperline = encode_info->enc_width;
            fmt.fmt.pix_mp.plane_fmt[0].sizeimage = encode_info->enc_width * encode_info->enc_height;

            fmt.fmt.pix_mp.plane_fmt[1].bytesperline = encode_info->enc_width;
            fmt.fmt.pix_mp.plane_fmt[1].sizeimage = encode_info->enc_width * encode_info->enc_height / 4;

            fmt.fmt.pix_mp.plane_fmt[2].bytesperline = encode_info->enc_width;
            fmt.fmt.pix_mp.plane_fmt[2].sizeimage = encode_info->enc_width * encode_info->enc_height / 4;
        }
    }
    else { //mmap
        fmt.fmt.pix_mp.num_planes = 1;
        if (encode_info->mfdin_input_fmt == MFDI_INPUT_FMT_RGB888) {
            fmt.fmt.pix_mp.plane_fmt[0].bytesperline = encode_info->enc_width * 3;
            fmt.fmt.pix_mp.plane_fmt[0].sizeimage = encode_info->enc_width * encode_info->enc_height * 3;
        }
        else if (encode_info->mfdin_input_fmt == MFDI_INPUT_FMT_RGBA8888) {
            fmt.fmt.pix_mp.plane_fmt[0].bytesperline = encode_info->enc_width * 4;
            fmt.fmt.pix_mp.plane_fmt[0].sizeimage = encode_info->enc_width * encode_info->enc_height * 4;
        }
        else {
            fmt.fmt.pix_mp.plane_fmt[0].bytesperline = encode_info->enc_width;
            fmt.fmt.pix_mp.plane_fmt[0].sizeimage = encode_info->enc_width * encode_info->enc_height * 3 / 2;
        }
    }

    printf("sizeimage:%d %d\n",fmt.fmt.pix_mp.plane_fmt[0].sizeimage,fmt.fmt.pix_mp.plane_fmt[1].sizeimage);
    
    if (ioctl(fd, VIDIOC_S_FMT, &fmt) < 0) {
        perror("VIDIOC_S_FMT (src) failed");
        return -1;
    }
    
    printf("Source format (MPLANE): %dx%d, pixelformat: %c%c%c%c, planes: %d\n",
           fmt.fmt.pix_mp.width, fmt.fmt.pix_mp.height,
           fmt.fmt.pix_mp.pixelformat & 0xFF,
           (fmt.fmt.pix_mp.pixelformat >> 8) & 0xFF,
           (fmt.fmt.pix_mp.pixelformat >> 16) & 0xFF,
           (fmt.fmt.pix_mp.pixelformat >> 24) & 0xFF,
           fmt.fmt.pix_mp.num_planes);
    
    return 0;
}

int setup_dst_format_mplane(int fd, handle_t *encode_info) {
    struct v4l2_format fmt;
    CLEAR(fmt);
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    fmt.fmt.pix_mp.width = encode_info->enc_width;
    fmt.fmt.pix_mp.height = encode_info->enc_height;
    if (encode_info->enc_fmt == CODEC_ID_H264)
        fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_H264;
    else if (encode_info->enc_fmt == CODEC_ID_H265)
        fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_HEVC;
    fmt.fmt.pix_mp.field = V4L2_FIELD_NONE;
    fmt.fmt.pix_mp.num_planes = 1;
    fmt.fmt.pix_mp.plane_fmt[0].sizeimage = 5*1024*1024;
    
    if (ioctl(fd, VIDIOC_S_FMT, &fmt) < 0) {
        perror("VIDIOC_S_FMT (dst) failed");
        return -1;
    }
    
    printf("Destination format (MPLANE): %dx%d, pixelformat: %c%c%c%c, planes: %d\n",
           fmt.fmt.pix_mp.width, fmt.fmt.pix_mp.height,
           fmt.fmt.pix_mp.pixelformat & 0xFF,
           (fmt.fmt.pix_mp.pixelformat >> 8) & 0xFF,
           (fmt.fmt.pix_mp.pixelformat >> 16) & 0xFF,
           (fmt.fmt.pix_mp.pixelformat >> 24) & 0xFF,
           fmt.fmt.pix_mp.num_planes);
    
    return 0;
}

int setup_buffers_mplane(int fd, int type, int count, 
                         struct v4l2_buffer **buf_info, 
                         unsigned char ***buffers, 
                         int **buf_lengths,
                         int *num_buffers,enum v4l2_memory memory_type) {
    struct v4l2_requestbuffers req;
    CLEAR(req);
    req.count = count;
    req.type = type;
    //req.memory = V4L2_MEMORY_MMAP;

    if (0 != memory_type) {
        req.memory = memory_type;
    }
    else { //by default
        req.memory = V4L2_MEMORY_MMAP;
    }
    
    if (ioctl(fd, VIDIOC_REQBUFS, &req) < 0) {
        perror("VIDIOC_REQBUFS failed");
        return -1;
    }
    
    *num_buffers = req.count;
    printf("Allocated %d buffers for type %d\n", req.count, type);
    
    if (req.count == 0) {
        printf("No buffers allocated\n");
        return -1;
    }
    
    *buf_info = calloc(req.count, sizeof(struct v4l2_buffer));
    *buffers = calloc(req.count, sizeof(unsigned char *));
    *buf_lengths = calloc(req.count, sizeof(int));
    
    if (!*buf_info || !*buffers || !*buf_lengths) {
        perror("calloc failed");
        return -1;
    }
    
    for (int i = 0; i < req.count; i++) {
        struct v4l2_buffer buf;
        struct v4l2_plane planes[VIDEO_MAX_PLANES];
        
        CLEAR(buf);
        buf.type = type;
        //buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        buf.length = VIDEO_MAX_PLANES;
//        buf.m.planes = planes;
        buf.m.planes = calloc(VIDEO_MAX_PLANES,
                          sizeof(struct v4l2_plane));
        
        if (0 != memory_type) {
            buf.memory = memory_type;
        }
        else { //by default
            buf.memory = V4L2_MEMORY_MMAP;
        }
        
        if (ioctl(fd, VIDIOC_QUERYBUF, &buf) < 0) {
            perror("VIDIOC_QUERYBUF failed");
            return -1;
        }
        
        memcpy(&((*buf_info)[i]), &buf, sizeof(struct v4l2_buffer));
        
        (*buf_lengths)[i] = 0;
        for (int p = 0; p < buf.length; p++) {
            int plane_size = buf.m.planes[p].length;
            (*buf_lengths)[i] += plane_size;
            
            printf("Buffer %d, plane %d: offset=0x%08x, length=%d\n", 
                   i, p, buf.m.planes[p].m.mem_offset, plane_size);
            
            if ((p == 0) && (buf.memory == V4L2_MEMORY_MMAP)) {
                (*buffers)[i] = mmap(NULL, plane_size, 
                                   PROT_READ | PROT_WRITE, 
                                   MAP_SHARED, fd, 
                                   buf.m.planes[p].m.mem_offset);
                if ((*buffers)[i] == MAP_FAILED) {
                    perror("mmap failed");
                    return -1;
                }
                printf("Buffer %d mapped at %p, size %d\n", 
                       i, (*buffers)[i], plane_size);
            }
        }
    }
    
    return 0;
}

void detectV4l2Support(handle_t *encode_info) {
    if ((access("/dev/video90", F_OK ) != -1)) {
        encode_info->v4l2_enable = true;
        VLOG(ERR, "v4l2 component present");
        return;
    }
    return;
}



int queue_buffer_mplane(int fd, struct v4l2_buffer *buf) {
    if (ioctl(fd, VIDIOC_QBUF, buf) < 0) {
        perror("VIDIOC_QBUF failed");
        return -1;
    }
    return 0;
}

int dequeue_buffer_mplane(int fd, int type, struct v4l2_buffer *buf, struct v4l2_plane *planes,enum v4l2_memory memory_type) {
//    struct v4l2_plane planes[VIDEO_MAX_PLANES];
    CLEAR(*buf);
    memset(planes, 0, sizeof(struct v4l2_plane) * VIDEO_MAX_PLANES);
    buf->type = type;
    if (0 != memory_type) {
        buf->memory = memory_type;
    }
    else {
        buf->memory = V4L2_MEMORY_MMAP;
    }
    buf->length = VIDEO_MAX_PLANES;
    buf->m.planes = planes;
    if (ioctl(fd, VIDIOC_DQBUF, buf) < 0) {
        if (errno == EAGAIN) {
            return -EAGAIN;
        }
        perror("VIDIOC_DQBUF failed");
        return -1;
    }

    return 0;
}

int stream_on(int fd, int type) {
    enum v4l2_buf_type buf_type = type;
    if (ioctl(fd, VIDIOC_STREAMON, &buf_type) < 0) {
        perror("VIDIOC_STREAMON failed");
        return -1;
    }
    return 0;
}

int stream_off(int fd, int type) {
    enum v4l2_buf_type buf_type = type;
    if (ioctl(fd, VIDIOC_STREAMOFF, &buf_type) < 0) {
        perror("VIDIOC_STREAMOFF failed");
        return -1;
    }
    return 0;
}

void print_frame_info(struct v4l2_buffer *buf, const char *tag) {
    printf("[%s] Buffer %d, type=%d, bytesused=", tag, buf->index, buf->type);
    if (buf->length > 0 && buf->m.planes) {
        int total = 0;
        for (int p = 0; p < buf->length; p++) {
            total += buf->m.planes[p].bytesused;
        }
        printf("%d", total);
    } else {
        printf("%d", buf->bytesused);
    }
    printf(", flags=0x%x\n", buf->flags);
}


int get_header(handle_t *handle, unsigned char *pHeader, unsigned int *pLength)
{
    venc_info_t *venc = &s_venc_info;
    unsigned long vpsPhyAddr, spsPhyAddr, ppsPhyAddr;
    int headerLength, vpsLength, spsLength, ppsLength;
    unsigned int headeroffset = 0;
    unsigned long headerVirAddr = 0;
    unsigned int des_offset = 0;
    char header_tmp[128];


    venc_buffer_t vdb;

    if (!pHeader)
        return -1;
    memset(&vdb, 0x00, sizeof(venc_buffer_t));

    vpsPhyAddr = handle->enc_header.vpsPhyAddr;
    spsPhyAddr = handle->enc_header.spsPhyAddr;
    ppsPhyAddr = handle->enc_header.ppsPhyAddr;
    headerLength = handle->enc_header.headerLength;
    VLOG(INFO,"[%s][%d]vpsPhyAddr:0x%lx\n", __func__, __LINE__,vpsPhyAddr);
    VLOG(INFO,"[%s][%d]spsPhyAddr:0x%lx\n", __func__, __LINE__,spsPhyAddr);
    VLOG(INFO,"[%s][%d]ppsPhyAddr:0x%lx\n", __func__, __LINE__,ppsPhyAddr);
    VLOG(INFO,"[%s][%d]headerLength:0x%lx\n", __func__, __LINE__,headerLength);

//00 vps sps pps，各占一个字节，最高8位没有
    vpsLength = (headerLength & 0xff0000) >> 16;
    spsLength = (headerLength & 0xff00) >> 8;
    ppsLength = (headerLength & 0xff);

    VLOG(INFO,"header origin addr:%lx %lx %lx\n",handle->header_buf_addr[0],handle->header_buf_addr[1],handle->header_buf_addr[2]);

    if (handle->enc_fmt == CODEC_ID_H265) {
        headeroffset = vpsPhyAddr - handle->header_buf_addr[0];
        vdi_read_memory(/*vpsPhyAddr*/handle->header_buf_addr[0], /*pHeader*/header_tmp, headeroffset + vpsLength);
        memcpy(pHeader,(header_tmp + headeroffset),vpsLength);
        des_offset = vpsLength;
    }
    headeroffset = spsPhyAddr - handle->header_buf_addr[1];
    vdi_read_memory(/*spsPhyAddr*/handle->header_buf_addr[1], /*pHeader+vpsLength*/header_tmp, headeroffset + spsLength);
    memcpy((pHeader + des_offset),(header_tmp  + headeroffset),spsLength);
    des_offset += spsLength;

    headeroffset = ppsPhyAddr - handle->header_buf_addr[2];
    vdi_read_memory(/*ppsPhyAddr*/handle->header_buf_addr[2], /*pHeader+vpsLength+spsLength*/header_tmp, headeroffset + ppsLength);
    memcpy((pHeader + des_offset),(header_tmp + headeroffset),ppsLength);
    *pLength = vpsLength + spsLength + ppsLength;

#if 0
    if (handle->enc_fmt == CODEC_ID_H265) {
        headeroffset = vpsPhyAddr - handle->header_buf_addr[0];
        vdi_read_memory(/*vpsPhyAddr*/handle->header_buf_addr[0], /*pHeader*/header_tmp, headeroffset + vpsLength);
        memcpy(pHeader,header_tmp + headeroffset,vpsLength);
        headeroffset = vpsPhyAddr - handle->header_buf_addr[1];
        vdi_read_memory(/*spsPhyAddr*/handle->header_buf_addr[1], /*pHeader+vpsLength*/header_tmp, headeroffset + spsLength);
        memcpy(pHeader,header_tmp + vpsLength + headeroffset,spsLength);
        headeroffset = vpsPhyAddr - handle->header_buf_addr[2];
        vdi_read_memory(/*ppsPhyAddr*/handle->header_buf_addr[0], /*pHeader+vpsLength+spsLength*/header_tmp, headeroffset + ppsLength);
        memcpy(pHeader,header_tmp + vpsLength + spsLength + headeroffset,ppsLength);
        *pLength = vpsLength + spsLength + ppsLength;
    } else if (handle->enc_fmt == CODEC_ID_H264) {
        vdi_read_memory(spsPhyAddr, pHeader, spsLength);
        vdi_read_memory(ppsPhyAddr, pHeader+spsLength, ppsLength);
        *pLength = spsLength + ppsLength;
    }
#endif
    VLOG(INFO,"[%s][%d]headerLength:%d\n", __func__, __LINE__,*pLength);
    return 0;
}


int get_bsbframe(handle_t *handle, unsigned char *buffer, unsigned int *buf_nal_size, signed char *frame_type)
{
    if (handle->v4l2_enable) {
        venc_info_t *venc = &s_venc_info;
        struct v4l2_buffer dst_buf;
        struct v4l2_plane planes[VIDEO_MAX_PLANES];
        int ret = -1;

        ret = dequeue_buffer_mplane(venc->venc_fd, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE, &dst_buf, planes, 0);
        while (ret != 0) {
            usleep(1000);
            ret = dequeue_buffer_mplane(venc->venc_fd, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE, &dst_buf, planes, 0);
        }

        if (ret == 0) {
            handle->data_len = 0;
            if (dst_buf.length > 0 && dst_buf.m.planes) {
                for (int p = 0; p < dst_buf.length; p++) {
                    handle->data_len += dst_buf.m.planes[p].bytesused;
                }
            }

            safe_memcpy(buffer, dst_buffers[dst_buf.index], handle->data_len);
            if (dst_buf.flags & V4L2_BUF_FLAG_LAST) {
                printf("End of stream reached\n");
            }

            if (dst_buf.flags & V4L2_BUF_FLAG_KEYFRAME) {
                (*frame_type) = 0;
            }
            else {
                (*frame_type) = 1;
            }
            
            if (queue_buffer_mplane(venc->venc_fd, &dst_buf) < 0) {
                printf("queue_buffer_mplane err\n");
            }

            (*buf_nal_size) = handle->data_len;

        } else if (ret != -EAGAIN) {
            perror("dequeue dst failed");
        }
    }
    else {
        venc_info_t *venc = &s_venc_info;
        unsigned int esaddr_offset = 0;
        unsigned long esPhyAddr;
        int eslength;
        unsigned long espVirAddr = 0;
        unsigned long slice_header_length = 0;
        int length_cp = 0;

        slice_header_length = handle->enc_es.slice_header_length;
        esPhyAddr = handle->enc_es.esPhyAddr;
        eslength = handle->enc_es.eslength;
        esaddr_offset = esPhyAddr - venc->common_buffer[handle->chn_id].phys_addr;
        espVirAddr = venc->common_buffer[handle->chn_id].virt_addr + esaddr_offset;
        VLOG(INFO,"[%s][%d]offset:0x%x\n", __func__, __LINE__, slice_header_length);
        VLOG(INFO,"[%s][%d]esPhyAddr:0x%lx\n", __func__, __LINE__, esPhyAddr);
        VLOG(INFO,"[%s][%d]esaddr_offset:0x%lx\n", __func__, __LINE__, esaddr_offset);
        VLOG(INFO,"[%s][%d]espVirAddr:0x%lx\n", __func__, __LINE__, espVirAddr);
        VLOG(INFO,"[%s][%d]eslength:%d\n", __func__, __LINE__, eslength);

        length_cp = ((eslength + 15) >> 4) << 4;
        memcpy(buffer, (const void *)((ulong)espVirAddr), length_cp);
        *buf_nal_size = eslength;
        (*frame_type) = handle->enc_es.frame_type;
        VLOG(INFO,"[%s][%d]buf_nal_size:%d,frame_type:%d\n", __func__, __LINE__,*buf_nal_size,(*frame_type));
    }
    return 0;
}

int get_dsframe(handle_t *handle, unsigned char *buffer, unsigned int *buf_nal_size)
{
    venc_info_t *venc = &s_venc_info;
    unsigned long dsPhyAddr;
    int dslength;
    unsigned long dsVirAddr = 0;
    if (!handle)
        return -1;

    dsPhyAddr = venc->common_buffer[handle->chn_id].phys_addr;
    dsVirAddr = venc->common_buffer[handle->chn_id].virt_addr;
    if ((handle->mfdin_input_fmt == MFDI_INPUT_FMT_NV12) ||
        (handle->mfdin_input_fmt == MFDI_INPUT_FMT_NV21) ||
        (handle->mfdin_input_fmt == MFDI_INPUT_FMT_YUV420_PLANAR)) {
        dslength = handle->ds_w * handle->ds_h * 3 / 2;
    } else if (handle->mfdin_input_fmt == MFDI_INPUT_FMT_RGB888) {
        dslength = handle->ds_w * handle->ds_h * 3;
    } else if (handle->mfdin_input_fmt == MFDI_INPUT_FMT_RGBA8888) {
        dslength = handle->ds_w * handle->ds_h * 4;
    }
    VLOG(INFO,"[%s][%d]dsPhyAddr:0x%lx\n", __func__, __LINE__, dsPhyAddr);
    VLOG(INFO,"[%s][%d]dsVirAddr:0x%lx\n", __func__, __LINE__, dsVirAddr);
    VLOG(INFO,"[%s][%d]dslength:%d\n", __func__, __LINE__, dslength);

    memcpy(buffer, (const void *)((ulong)dsVirAddr), dslength);
    *buf_nal_size = dslength;
    VLOG(INFO,"[%s][%d]buf_nal_size:%d\n", __func__, __LINE__,*buf_nal_size);
    return 0;
}

int check_initparam(handle_t handle)
{
    if (handle.enc_width < MIN_WIDTH || handle.enc_height < MIN_HEIGHT) {
        VLOG(ERR,"[%s][%d]width %d x height %d must be greater than %d x %d.\n",__func__,__LINE__, handle.enc_width, handle.enc_height, MIN_WIDTH, MIN_HEIGHT);
        return -1;
    }

    if (handle.crop_enable && (handle.crop_w < MIN_WIDTH || handle.crop_h < MIN_HEIGHT)) {
        VLOG(ERR,"[%s][%d]crop_w %d x crop_h %d must be greater than %d x %d.\n",__func__,__LINE__, handle.crop_w, handle.crop_h, MIN_WIDTH, MIN_HEIGHT);
        return -1;
    }
    return 0;
}

void encoder_open(char *fwfile) {

//  venc_init();
//  vrisc_reset();
//  firmware_load(fwfile);
//  firmware_boot();

    venc_info_t *venc = &s_venc_info;
    struct v4l2_capability cap;


    memset(&s_venc_info, 0x00, sizeof(s_venc_info));
    s_venc_info.venc_fd = -1;

    venc->venc_fd = open("/dev/video90", O_RDWR);
    if (venc->venc_fd < 0) {
        VLOG(INFO,"[VENC] Can't open venc driver. [error=%s]. \n", strerror(errno));
//        return -1;
    }
    VLOG(INFO,"open venc fd:%d,&venc:%lx!",venc->venc_fd,(unsigned long)venc);

    if (ioctl(venc->venc_fd, VIDIOC_QUERYCAP, &cap) < 0) {
        perror("VIDIOC_QUERYCAP failed");
        close(venc->venc_fd);
//        return -1;
    }
    printf("Driver: %s\n", cap.driver);
    printf("Card: %s\n", cap.card);
    printf("Bus: %s\n", cap.bus_info);
    printf("Capabilities: 0x%08x\n", cap.capabilities);
    
    if (!(cap.capabilities & V4L2_CAP_VIDEO_M2M_MPLANE)) {
        printf("Device does not support V4L2_CAP_VIDEO_M2M_MPLANE\n");
        printf("Try using non-mplane version\n");
        close(venc->venc_fd);
//        return -1;
    }
    printf("Device supports M2M MPLANE\n");

}

void encoder_unmap(int inst_id) {
    venc_info_t *venc = &s_venc_info;

    if (venc->common_buffer[inst_id].virt_addr) {
        VLOG(ERR,"munmap memory common_buffer!,pool count:%d\n",venc->vpu_buffer_pool_count);
        munmap((void *)venc->common_buffer[inst_id].virt_addr, venc->common_buffer[inst_id].size);
    }
}


void encoder_close() {
    venc_destroy();
}

int vl_tahoe_encoder_init(int codec_id, handle_t encode_info) {
//  int ret;
  venc_info_t *venc = &s_venc_info;

  if (check_initparam(encode_info)) {
      printf("check_initparam failed\n");
      return -1;
  }

//  vdi_alloc_common_buffer(encode_info.chn_id);
//  venc_lock();
//  create_channel(encode_info);
//  venc_unlock();

  setup_encoder_params(venc->venc_fd, &encode_info);
  if (setup_src_format_mplane(venc->venc_fd, &encode_info, 1) < 0) {
      close(venc->venc_fd);
      return -1;
  }
  
  if (setup_dst_format_mplane(venc->venc_fd, &encode_info) < 0) {
      close(venc->venc_fd);
      return -1;
  }

    if (setup_buffers_mplane(venc->venc_fd, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE, 2,
                             &src_buf_info, &src_buffers, &src_buf_lengths, &src_buf_count, 0) < 0) {
        close(venc->venc_fd);
        return -1;
    }

    if (setup_buffers_mplane(venc->venc_fd, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE, 2,
                             &dst_buf_info, &dst_buffers, &dst_buf_lengths, &dst_buf_count, 0) < 0) {
        close(venc->venc_fd);
        return -1;
    }
    
    for (int i = 0; i < dst_buf_count; i++) {
        if (queue_buffer_mplane(venc->venc_fd, &dst_buf_info[i]) < 0) {
            close(venc->venc_fd);
            return -1;
        }
    }
    
    if (stream_on(venc->venc_fd, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) < 0) {
        close(venc->venc_fd);
        return -1;
    }
    if (stream_on(venc->venc_fd, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) < 0) {
        close(venc->venc_fd);
        return -1;
    }
    printf("Stream started\n");

  return 0;

//  return (vl_codec_handle_t)mHandle;

//exit:
//  if (mHandle != NULL)
//    free(mHandle);
//  return (vl_codec_handle_t)NULL;
}


int vl_v4l2_switch_to_dmamode( handle_t encode_info,int plane_num) {
    venc_info_t *venc = &s_venc_info;

    VLOG(ERR,"switch to dmamode!\n");

    stream_off(venc->venc_fd, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE);
    stream_off(venc->venc_fd, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE);
    
    for (int i = 0; i < src_buf_count; i++) {
        munmap(src_buffers[i], src_buf_lengths[i]);
    }
    for (int i = 0; i < dst_buf_count; i++) {
        munmap(dst_buffers[i], dst_buf_lengths[i]);
    }

    free(src_buffers);
    free(dst_buffers);
    for (int i = 0; i < src_buf_count; i++) {
        free(src_buf_info[i].m.planes);
    }
    free(src_buf_info);
    free(dst_buf_info);
    free(src_buf_lengths);
    free(dst_buf_lengths);

    setup_encoder_params(venc->venc_fd, &encode_info);
    if (setup_src_format_mplane(venc->venc_fd, &encode_info, plane_num) < 0) {
        close(venc->venc_fd);
        return -1;
    }
  
    if (setup_dst_format_mplane(venc->venc_fd, &encode_info) < 0) {
        close(venc->venc_fd);
        return -1;
    }

    if (setup_buffers_mplane(venc->venc_fd, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE, 2,
                             &src_buf_info, &src_buffers, &src_buf_lengths, &src_buf_count, V4L2_MEMORY_DMABUF) < 0) {
        close(venc->venc_fd);
        return -1;
    }

    if (setup_buffers_mplane(venc->venc_fd, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE, 2,
                             &dst_buf_info, &dst_buffers, &dst_buf_lengths, &dst_buf_count, 0) < 0) {
        close(venc->venc_fd);
        return -1;
    }
    
    for (int i = 0; i < dst_buf_count; i++) {
        if (queue_buffer_mplane(venc->venc_fd, &dst_buf_info[i]) < 0) {
            close(venc->venc_fd);
            return -1;
        }
    }
    
    if (stream_on(venc->venc_fd, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) < 0) {
        close(venc->venc_fd);
        return -1;
    }
    if (stream_on(venc->venc_fd, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) < 0) {
        close(venc->venc_fd);
        return -1;
    }
    printf("Stream started\n");
}



int vl_video_encoder_change_bitrate(handle_t *encode_info,
                            int bitRate)
{
    int ret;
//    VPMultiEncHandle* handle = (VPMultiEncHandle *)codec_handle;

//    if (handle->am_enc_handle == 0) //not init the encoder yet
//        return -1;
//    if (handle->mEncParams.param_change_enable == 0) //no change enabled
//        return -2;
//    ret = AML_MultiEncChangeBitRate(handle->am_enc_handle, bitRate);
//    if (ret != AMVENC_SUCCESS)
//        return -3;
//    hist_reset(handle); //bitrate target change reset the hist
//    handle->mEncParams.bitrate = bitRate;
    encode_info->enc_bitrate = bitRate;
    encode_info->cfg_update = 1;

    return 0;
}

int vl_video_encoder_change_gop(handle_t *encode_info,
                                int IntraQP, int IntraPeriod)
{
    int ret;
//    VPMultiEncHandle* handle = (VPMultiEncHandle *)codec_handle;

//    if (handle->am_enc_handle == 0) //not init the encoder yet
//        return -1;
//    if (handle->mEncParams.param_change_enable == 0) //no change enabled
//        return -2;
    if (IntraQP < 0 || IntraQP > 51 ) {
           VLOG(ERR,"QP value out of range [0, 51]\n");
        return -3;
    }
    if (IntraPeriod <= 1 ) {
           VLOG(ERR,"Invalid Intra Period %d\n", IntraPeriod);
        return -4;
    }
//    ret = AML_MultiEncChangeIntraPeriod(handle->am_enc_handle,
//              IntraQP, IntraPeriod);
//    if (ret != AMVENC_SUCCESS)
//        return -5;
    encode_info->enc_gop = IntraPeriod;
    encode_info->cfg_update = 1;
    return 0;
}

int vl_video_encoder_change_qp(handle_t *encode_info,
                                int minQpI, int maxQpI, int maxDeltaQp,
                                int minQpP, int maxQpP,
                                int minQpB, int maxQpB)
{
    int ret;
//    VPMultiEncHandle* handle = (VPMultiEncHandle *)codec_handle;

//    if (handle->am_enc_handle == 0) //not init the encoder yet
//        return -1;
//    if (handle->mEncParams.param_change_enable == 0) //no change enabled
//        return -2;

    if (minQpI < 0 || minQpI > 51 || maxQpI < 0 || maxQpI > 51 ||
        maxDeltaQp < 0 || maxDeltaQp > 51 || minQpP < 0 || minQpP > 51 ||
        maxQpP < 0 || maxQpP > 51 || minQpB < 0 || minQpB > 51 ||
        maxQpB < 0 || maxQpB > 51) {
        VLOG(ERR,"qp min or qp max out of range [0, 51]\n");
        return -3;
    }
    if (minQpI >= maxQpI || minQpP >= maxQpP || minQpB >= maxQpB) {
        VLOG(ERR,"qp min  or qp_max out of range [min, max]\n");
        return -4;
    }

//    ret = AML_MultiEncChangeQPMinMax(handle->am_enc_handle,
//             minQpI, maxQpI, maxDeltaQp, minQpP, maxQpP, minQpB, maxQpB);
//    if (ret != AMVENC_SUCCESS)
//        return -5;
    return 0;
}

int vl_video_encoder_change_strict_rc(handle_t *encode_info,
                                  int bitrate_window, int skip_threshold)
{
    int ret;
//    VPMultiEncHandle* handle = (VPMultiEncHandle *)codec_handle;

//    if (handle->am_enc_handle == 0) //not init the encoder yet
//        return -1;
    if (bitrate_window > MAX_FRAME_WINDOW)
        bitrate_window = MAX_FRAME_WINDOW;
    else if (bitrate_window < 0)
        bitrate_window = 0;

//    handle->hist_win_len = bitrate_window;
//    handle->hist_skip_thresh = skip_threshold;

//    if (bitrate_window == 0)
//    {// disable
//        hist_reset(handle);
//    }
    encode_info->enc_fps = bitrate_window;
    encode_info->cfg_update = 1;
    return 0;
}

int vl_tahoe_encoder_generate_header(handle_t *encode_info)
{
    int ret = 0;

    venc_lock();
    encode_header(encode_info);
    encode_info->enc_header.vpsPhyAddr = ((((ulong)(venc_read_register(HEADER_ADDR_HIGHER) >> 4) & 0x03)) << 32) | venc_read_register(HEADER_VPS_ADDR);
    encode_info->enc_header.spsPhyAddr = (((ulong)(venc_read_register(HEADER_ADDR_HIGHER) & 0x03)) << 32) | venc_read_register(HEADER_SPS_ADDR);
    encode_info->enc_header.ppsPhyAddr = ((((ulong)(venc_read_register(HEADER_ADDR_HIGHER) >> 2) & 0x03)) << 32) | venc_read_register(HEADER_PPS_ADDR);
    encode_info->enc_header.headerLength = venc_read_register(HEADER_SIZE);

    VLOG(INFO,"header addr %lx %lx %lx\n",encode_info->enc_header.vpsPhyAddr,encode_info->enc_header.spsPhyAddr,encode_info->enc_header.ppsPhyAddr);
    venc_unlock();

    return ret;
}

static int encode_poll(int fd, int timeout) {
    struct pollfd poll_fd[1];
    poll_fd[0].fd = fd;
    poll_fd[0].events = POLLIN | POLLERR;
    return poll(poll_fd, 1, timeout);
}

int num = 0;
bool get_input_buf_forv4l(ulong *pInput,struct v4l2_buffer *src_buf,handle_t *encode_info) {
    venc_info_t *venc = &s_venc_info;
    struct v4l2_plane planes[VIDEO_MAX_PLANES];
    int ret = -1;
    enum v4l2_memory type;

    if (encode_info->current_mode == 3) {
        type = V4L2_MEMORY_DMABUF;
    }
    else {
        type = V4L2_MEMORY_MMAP;
    }

    if (num < 2) {
        memcpy(src_buf,&src_buf_info[num],sizeof(struct v4l2_buffer));
        //src_buf = &src_buf_info[num];
        ret = 0;
        num++;
    } else {
        ret = dequeue_buffer_mplane(venc->venc_fd, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE, src_buf, planes, type);
        while (ret != 0) {
            usleep(1000);
            ret = dequeue_buffer_mplane(venc->venc_fd, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE, src_buf, planes, type);
        }
    }
    if (ret == 0) {
        (*pInput) = src_buffers[src_buf->index];
        return true;
    }
    return false;
}



int vl_tahoe_encoder_encode(handle_t *encode_info,struct v4l2_buffer *src_buf)
{
//    int ret = 0;

    //vdi_debug(encode_info->chn_id);

//    venc_lock();
//    roi_config(encode_info);
//    osd_config(encode_info);
//    if (encode_info->cfg_update == 1) {
//        dynamic_parameter_set(encode_info);
//        encode_info->cfg_update = 0;
//    }
//    encode_one_frame(encode_info);
//    if (encode_poll(s_venc_info.venc_fd, -1) <= 0) {
//        VLOG(ERR,"[%s][%d]poll fail\n", __func__, __LINE__);
//        return -1;
//    }
//    encode_info->enc_es.slice_header_length = venc_read_register(TAHOE_EC_DMA0) & 0x3F;
//    encode_info->enc_es.esPhyAddr = venc_read_register(TAHOE_EC_DMA0) - encode_info->enc_es.slice_header_length;
//    encode_info->enc_es.esPhyAddr |= (((ulong)(venc_read_register(TAHOE_EC_DMA6) & 0x03)) << 32);
//    encode_info->enc_es.eslength = venc_read_register(TAHOE_EC_LENGTH);
//    encode_info->enc_es.frame_type = venc_read_register(OUTPUT_FRAME_TYPE);
//
//    VLOG(INFO,"es buffer phyaddr:%lx,lsb:%x",encode_info->enc_es.esPhyAddr,(venc_read_register(TAHOE_EC_DMA0) - encode_info->enc_es.slice_header_length));
//
//    venc_unlock();
    venc_info_t *venc = &s_venc_info;
    struct v4l2_buffer dst_buf;
    struct v4l2_plane planes[VIDEO_MAX_PLANES];
    int ret = 0;//-1;

    if (encode_info->current_mode == VMALLOC_TYPE) {
        if (src_buf && (src_buf->length > 0) && src_buf->m.planes) {
            int y_size = encode_info->enc_width * encode_info->enc_height;
            int uv_size = (encode_info->enc_width / 2) * (encode_info->enc_height / 2) * 2;
            src_buf->m.planes[0].bytesused = y_size + uv_size;//一个planes的情况
        }
    }

    if (queue_buffer_mplane(venc->venc_fd, src_buf) < 0) {
        printf("queue_buffer_mplane err\n");
    }

    return ret;
}

int vl_tahoe_encoder_destroy(handle_t encode_info) {
//    VPMultiEncHandle *handle = (VPMultiEncHandle *)codec_handle;
//    AML_MultiEncRelease(handle->am_enc_handle);
//#if ENCODE_TIME_OUTER
//    printf("%p#Total encode time : %llu, Total encode frames : %llu\n",
//        handle->am_enc_handle, total_encode_time, total_encode_frames);
//#endif
//    if (handle->mSPSPPSData)
//        free(handle->mSPSPPSData);
//    if (handle)
//        free(handle);
//    venc_lock();
//    destroy_channel(encode_info);
//    venc_unlock();

    venc_info_t *venc = &s_venc_info;
    stream_off(venc->venc_fd, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE);
    stream_off(venc->venc_fd, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE);
    
    for (int i = 0; i < src_buf_count; i++) {
        munmap(src_buffers[i], src_buf_lengths[i]);
    }
    for (int i = 0; i < dst_buf_count; i++) {
        munmap(dst_buffers[i], dst_buf_lengths[i]);
    }
    free(src_buffers);
    free(dst_buffers);
    for (int i = 0; i < src_buf_count; i++) {
        free(src_buf_info[i].m.planes);
    }
    free(src_buf_info);
    free(dst_buf_info);
    free(src_buf_lengths);
    free(dst_buf_lengths);
    
    close(venc->venc_fd);

    return 1;
}

