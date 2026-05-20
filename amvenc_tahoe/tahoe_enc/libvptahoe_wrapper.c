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
#include <stdlib.h>
#include <pthread.h>
#include "tahoe.h"
#include "vp_codec_tahoe.h"
#include "libvptahoe_codec.h"
#include "vdi.h"
#include "enc_driver.h"
#include "tahoe_register.h"
#include <string.h>
#include <sys/poll.h>
#include <fcntl.h>

#define FW_STRING_LEN_MAX  256
#define HE_BUF_SIZE  0x100000

int input_fd_dump = -1;
int output_fd_dump = -1;

#define DUMP_FILE_ENABLE 0


#if VENDOR_FW
#define FW_PATH  "/vendor/lib/firmware/encoder/fw.bin"
#else
#define FW_PATH  "/lib/firmware/encoder/fw.bin"
#endif



vl_codec_tahoe_handle_t vt_tahoe_encoder_init(int codec_id, vt_encode_info_t encode_param) {
    venc_buffer_t hdBuffer[3] = {0};
    char fwfile[FW_STRING_LEN_MAX] = {0};
    int ret = 0;
    int i = 0;
    //venc_buffer_t input_buf = {0};

    tahoe_set_log_level(ERR);
    handle_t *encode_info = (handle_t *)malloc(sizeof(handle_t));
    if (!encode_info) {
        VLOG(ERR,"alloc encoder handle failed!");
        return 0;
    }

    memset(encode_info,0,sizeof(handle_t));

    encode_param.forcePicQpEnable = 0;
    encode_info->enc_width = encode_param.width;
    encode_info->enc_height = encode_param.height;
    encode_info->enc_fps = encode_param.frame_rate;
    encode_info->enc_gop = encode_param.gop;
    if (0 == encode_param.gop) {//fix me ,infinite gop
        encode_info->enc_gop = encode_info->enc_fps;
    }
    encode_info->enc_gop = encode_info->enc_fps;
    encode_info->enc_fmt = codec_id;
    encode_info->profile = 0;//encode_param.profile;
    encode_info->enc_bitrate = encode_param.bit_rate;
    //encode_info->mfdin_input_fmt = encode_param.img_format;
    //encode_info->level = encode_param.level;
    if (encode_param.forcePicQpEnable) {
        encode_info->fix_qp_I = encode_param.forcePicQpI;
        encode_info->fix_qp_P = encode_param.forcePicQpP;
    }

    encode_info->enc_rc_mode = 1; //cbr by default
    encode_info->init_qp = 30;
    encode_info->poc = 0;
    encode_info->buf_stride = encode_info->enc_width;

    if (encode_param.img_format == IMG_FMT_NV12) {
        VLOG(INFO,"img_format is IMG_FMT_NV12 \n");
        encode_info->mfdin_input_fmt = MFDI_INPUT_FMT_NV12;
    } else if (encode_param.img_format == IMG_FMT_NV21) {
        VLOG(INFO,"img_format is IMG_FMT_NV21 \n");
        encode_info->mfdin_input_fmt = MFDI_INPUT_FMT_NV21;
    } else if (encode_param.img_format == IMG_FMT_YUV420P) {
        VLOG(INFO,"img_format is IMG_FMT_YUV420P \n");
        encode_info->mfdin_input_fmt = MFDI_INPUT_FMT_YUV420_PLANAR;
    } else if (encode_param.img_format == IMG_FMT_RGB888) {
        VLOG(INFO,"img_format is IMG_FMT_RGB888 \n");
        encode_info->mfdin_input_fmt = MFDI_INPUT_FMT_RGB888;
    } else if (encode_param.img_format == IMG_FMT_RGBA8888) {
        VLOG(INFO,"img_format is IMG_FMT_RGBA8888 \n");
        encode_info->mfdin_input_fmt = MFDI_INPUT_FMT_RGBA8888;
    }

    VLOG(INFO,"debug param width:%d,height:%d,fps:%d,gop:%d,fmt:%d,profile:%d,bitrate:%d,forceqp:%d",
          encode_info->enc_width,encode_info->enc_height,encode_info->enc_fps,encode_info->enc_gop,encode_info->mfdin_input_fmt,
          encode_info->profile, encode_info->enc_bitrate,encode_param.forcePicQpEnable);
/*
    if (check_initparam(*encode_info)) {
      printf("check_initparam failed\n");
      return 0;
    }*/
    snprintf(fwfile, FW_STRING_LEN_MAX, "%s", FW_PATH);

    if (0 == get_cur_instance()) {
        VLOG(INFO,"loading fw:%s\n",fwfile);
        encoder_open(fwfile);
    }

    if (!find_valid_id(&encode_info->chn_id)) {
        VLOG(ERR,"have no valid instance to use!");
        return -1;
    }

    ret = vl_tahoe_encoder_init(codec_id,*encode_info);
    if (ret < 0) {
        VLOG(ERR,"init encode failed!,ret:%d\n",ret);
        return 0;
    }

    encode_info->input_buf[0].cached = 1;
    if (encode_info->mfdin_input_fmt == MFDI_INPUT_FMT_NV12 || encode_info->mfdin_input_fmt == MFDI_INPUT_FMT_NV21 ||
        encode_info->mfdin_input_fmt == MFDI_INPUT_FMT_YUV420_PLANAR) {
        encode_info->input_buf[0].size = encode_info->enc_width * encode_info->enc_height * 3 / 2;//fix me!!!
    }
    else if(encode_info->mfdin_input_fmt == MFDI_INPUT_FMT_RGB888) {
        encode_info->input_buf[0].size = encode_info->enc_width * encode_info->enc_height * 3;//fix me!!!
    }
    else if(encode_info->mfdin_input_fmt == MFDI_INPUT_FMT_RGBA8888) {
        encode_info->input_buf[0].size = encode_info->enc_width * encode_info->enc_height * 4;//fix me!!!
    }

    if (vdi_allocate_dma_memory(&encode_info->input_buf[0]) < 0) {
        VLOG(ERR,"Can not allocat input Buffer in vmalloc mode!\n");
        return -1;
    }

    for (i = 0; i < 3; i++) {
        memset(&hdBuffer[i], 0, sizeof(hdBuffer[i]));
        hdBuffer[i].size = HE_BUF_SIZE;
        if (vdi_allocate_dma_memory(&hdBuffer[i]) < 0) {
            printf("Can not allocat hd Buffer chnid %d\n", encode_info->chn_id);
            return -1;
        }
        VLOG(INFO,"[%s][%d]hdBuffer[%d].phys_addr:0x%x\n", \
            __func__,__LINE__, i, hdBuffer[i].phys_addr);
        encode_info->header_buf_addr[i] = hdBuffer[i].phys_addr;
    }
#if DUMP_FILE_ENABLE
    input_fd_dump = open("/data/tahoe_dump_input.yuv", O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (input_fd_dump < 0)
    {
        VLOG(ERR,"open input dump file error!\n");
        //goto exit;
    }

    output_fd_dump = open("/data/tahoe_dump_output.h264", O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (output_fd_dump < 0)
    {
        VLOG(ERR,"open output dump file error!\n");
        //goto exit;
    }
#endif
    return (vl_codec_tahoe_handle_t)encode_info;
}

int vt_video_encoder_change_bitrate(vl_codec_tahoe_handle_t codec_handle,
                            int bitRate)
{
    int ret;

    handle_t *encode_info = (handle_t *)codec_handle;
    if (!encode_info) {
        printf("change bitrate,param invalid!!\n");
        return -1;
    }

    ret = vl_video_encoder_change_bitrate(encode_info, bitRate);

    return ret;
}

int vt_video_encoder_change_gop(vl_codec_tahoe_handle_t codec_handle,
                                int IntraQP, int IntraPeriod)
{
    int ret;

    handle_t *encode_info = (handle_t *)codec_handle;
    if (!encode_info) {
        printf("change gop,param invalid!!\n");
        return -1;
    }

    ret = vl_video_encoder_change_gop(encode_info, IntraQP,  IntraPeriod);

    return ret;
}

int vt_video_encoder_change_qp(vl_codec_tahoe_handle_t codec_handle,
                                int minQpI, int maxQpI, int maxDeltaQp,
                                int minQpP, int maxQpP,
                                int minQpB, int maxQpB)
{
    int ret;
    handle_t *encode_info = (handle_t *)codec_handle;
    if (!encode_info) {
        printf("change qp,param invalid!!\n");
        return -1;
    }

    ret = vl_video_encoder_change_qp(encode_info, minQpI, maxQpI, maxDeltaQp, minQpP, maxQpP,
                                     minQpB, maxQpB);

    return ret;
}

int vt_video_encoder_change_strict_rc(vl_codec_tahoe_handle_t codec_handle,
                                  int bitrate_window, int skip_threshold)
{
    int ret;
    handle_t *encode_info = (handle_t *)codec_handle;
    if (!encode_info) {
        printf("change rc mode,param invalid!!\n");
        return -1;
    }

    ret = vl_video_encoder_change_strict_rc(encode_info, bitrate_window, skip_threshold);

    return ret;
}


int vt_tahoe_encoder_generate_header(vl_codec_tahoe_handle_t codec_handle, unsigned char *pHeader, unsigned int *pLength) {
    int ret = 0;
    handle_t *encode_info = (handle_t *)codec_handle;
    if (!encode_info) {
        printf("generate header,param invalid!!\n");
        return -1;
    }

    ret = vl_tahoe_encoder_generate_header(encode_info);
    if (ret != 0) {
        VLOG(ERR,"generate header failed,ret:%d\n",ret);
        return ret;
    }

    get_header(encode_info,pHeader,pLength);

    return ret;
}


vt_encoding_metadata_t vt_tahoe_encoder_encode(vl_codec_tahoe_handle_t codec_handle,vt_frame_type_t type,vt_buffer_info_t *in_buffer_info,unsigned char* out)
{
    int ret = 0;
    int length = 0;
    signed char frame_type = -1;
    vt_encoding_metadata_t result;
    handle_t *encode_info = 0;
    //venc_buffer_t input_buf = {0};
    int header_length = 0;
    int i = 0;
    tahoe_dma_buf_info_t dma_info_map;

    memset(&result,0,sizeof(result));
    encode_info = (handle_t *)codec_handle;
    if (!encode_info) {
        printf("encode frame param not valid!\n");
        result.is_valid = 0;
        result.err_cod = -1;
        return result;
    }

    if (VMALLOC_TYPE == in_buffer_info->buf_type) {
        if (encode_info->mfdin_input_fmt == MFDI_INPUT_FMT_NV12 || encode_info->mfdin_input_fmt == MFDI_INPUT_FMT_NV21 ||
            encode_info->mfdin_input_fmt == MFDI_INPUT_FMT_YUV420_PLANAR) {
            //need fix
            memcpy((char *)encode_info->input_buf[0].virt_addr,(char *)in_buffer_info->buf_info.in_ptr[0],encode_info->input_buf[0].size);

            encode_info->tahoe_enc_pic.inpt_buf_addr[0] = encode_info->input_buf[0].phys_addr;
            encode_info->tahoe_enc_pic.inpt_buf_addr[1] = encode_info->input_buf[0].phys_addr + encode_info->enc_width * encode_info->enc_height;
            encode_info->tahoe_enc_pic.inpt_buf_addr[2] = encode_info->tahoe_enc_pic.inpt_buf_addr[1];
        }
        else if(encode_info->mfdin_input_fmt == MFDI_INPUT_FMT_RGB888) {
            //need fix
            memcpy((char *)encode_info->input_buf[0].virt_addr,(char *)in_buffer_info->buf_info.in_ptr[0],encode_info->input_buf[0].size);
            encode_info->tahoe_enc_pic.inpt_buf_addr[0] = encode_info->input_buf[0].phys_addr;
        }
        else if(encode_info->mfdin_input_fmt == MFDI_INPUT_FMT_RGBA8888) {
            //need fix
            memcpy((char *)encode_info->input_buf[0].virt_addr,(char *)in_buffer_info->buf_info.in_ptr[0],encode_info->input_buf[0].size);
            encode_info->tahoe_enc_pic.inpt_buf_addr[0] = encode_info->input_buf[0].phys_addr;
        }

        encode_info->buf_stride = encode_info->enc_width; //fix me!!!
        VLOG(INFO,"vmalloc mode,input data addr:%x %x %x\n",encode_info->tahoe_enc_pic.inpt_buf_addr[0],encode_info->tahoe_enc_pic.inpt_buf_addr[1],encode_info->tahoe_enc_pic.inpt_buf_addr[2]);
    }
    #if 1
    else if (DMA_TYPE == in_buffer_info->buf_type) {
      vt_dma_info_t *dma_info;
      dma_info = &(in_buffer_info->buf_info.dma_info);

        if (encode_info->mfdin_input_fmt == MFDI_INPUT_FMT_NV12 || encode_info->mfdin_input_fmt == MFDI_INPUT_FMT_NV21) {
            if (dma_info->num_planes != 1 && dma_info->num_planes != 2) {
                VLOG(ERR,"invalid num_planes %d\n", dma_info->num_planes);
                result.is_valid = false;
                result.err_cod = -1;
                return result;
            }
        } else if (encode_info->mfdin_input_fmt == MFDI_INPUT_FMT_YUV420_PLANAR) {
            if (dma_info->num_planes != 1 && dma_info->num_planes != 3) {
                VLOG(ERR,"YUV420P invalid num_planes %d\n", dma_info->num_planes);
                result.is_valid = false;
                result.err_cod = -1;
                return result;
            }
        } else if (encode_info->mfdin_input_fmt == MFDI_INPUT_FMT_RGBA8888 || encode_info->mfdin_input_fmt == MFDI_INPUT_FMT_RGB888) {

            if (dma_info->num_planes != 1) {
                VLOG(ERR,"RGBA/RGB invalid num_planes %d\n", dma_info->num_planes);
                result.is_valid = false;
                result.err_cod = -1;
                return result;
            }
        } else {
            VLOG(ERR,"unsupported color format");
            result.is_valid = false;
            result.err_cod = -1;
            return result;
        }

        memset(&dma_info_map,0,sizeof(dma_info_map));

        dma_info_map.num_planes = dma_info->num_planes;

        for (i = 0; i < dma_info->num_planes; i++) {
            if (dma_info->shared_fd[i] < 0) {
                VLOG(ERR,"invalid dma_fd %d\n", dma_info->shared_fd[i]);
                result.is_valid = false;
                result.err_cod = -1;
                return result;
            }
            dma_info_map.fd[i] = dma_info->shared_fd[i];
            VLOG(INFO,"instance %d:**shared_fd %d\n", encode_info->chn_id,dma_info->shared_fd[i]);
        }
        vdi_config_dma(&dma_info_map);
        VLOG(INFO,"instance %d:dma map num_planes:%d\n",encode_info->chn_id,dma_info_map.num_planes);

        encode_info->buf_stride = ((encode_info->enc_width + 15) >> 4) << 4;
        encode_info->tahoe_enc_pic.inpt_buf_addr[0] = dma_info_map.phys_addr[0];
        if (dma_info_map.num_planes == 1)
        {
            if (MFDI_INPUT_FMT_NV21 == encode_info->mfdin_input_fmt || MFDI_INPUT_FMT_NV12 == encode_info->mfdin_input_fmt ||
                MFDI_INPUT_FMT_YUV420_PLANAR == encode_info->mfdin_input_fmt) {
                encode_info->tahoe_enc_pic.inpt_buf_addr[1] = dma_info_map.phys_addr[0] + encode_info->buf_stride * encode_info->enc_height;
                if (MFDI_INPUT_FMT_YUV420_PLANAR == encode_info->mfdin_input_fmt) {
                  encode_info->tahoe_enc_pic.inpt_buf_addr[2] = encode_info->tahoe_enc_pic.inpt_buf_addr[1] +
                                                                    encode_info->buf_stride * encode_info->enc_height / 4;
              }
            }
        }
        if (dma_info_map.num_planes > 1 )
            encode_info->tahoe_enc_pic.inpt_buf_addr[1]= dma_info_map.phys_addr[1];
        if (dma_info_map.num_planes > 2)
            encode_info->tahoe_enc_pic.inpt_buf_addr[2]= dma_info_map.phys_addr[2];
    }
#endif
    ret = vl_tahoe_encoder_encode(encode_info);
    if (ret < 0) {
        VLOG(ERR,"send encode cmd to encoder failed!ret:%d \n",ret);
        result.is_valid = 0;
        result.err_cod = ret;
        return result;
    }

    if (0 == encode_info->enc_es.frame_type || 1 == encode_info->enc_es.frame_type) {
        VLOG(INFO,"IDR or I frame!!\n");
        vt_tahoe_encoder_generate_header(codec_handle, out, &header_length);

        result.is_key_frame = 1;
    }
    else {
        result.is_key_frame = 0;
    }

    get_bsbframe(encode_info, (out + header_length), &length,&frame_type);
    result.is_valid = 1;
    result.err_cod = 0;
    result.encoded_data_length_in_bytes = length + header_length;
#if DUMP_FILE_ENABLE
    if (input_fd_dump != -1) {
        write(input_fd_dump, (char *)encode_info->input_buf[0].virt_addr, encode_info->input_buf[0].size);
        VLOG(INFO,"dump tahoe input file,%d",encode_info->input_buf[0].size);
    }

    if (output_fd_dump != -1) {
        write(output_fd_dump, (unsigned char *) out, result.encoded_data_length_in_bytes);
        VLOG(INFO,"dump tahoe output file,%d",result.encoded_data_length_in_bytes);
    }
#endif
    if (DMA_TYPE == in_buffer_info->buf_type) {
        ret = vdi_unmap_dma(&dma_info_map);
        if (ret != 0) {
            VLOG(ERR,"vdi_unmap_dma src buffer failed!!\n");
        }
    }
    return result;
}

void vt_tahoe_encoder_destroy(vl_codec_tahoe_handle_t codec_handle)  {
    handle_t *m_handle = (handle_t *)codec_handle;
    if (!m_handle) {
        VLOG(ERR,"destroy handle not valid!\n");
        return;
    }
    VLOG(INFO,"destroy tahoe!!");
    vl_tahoe_encoder_destroy(*m_handle);
    encoder_close();

    set_instance_valid(m_handle->chn_id,false);

    if (m_handle) {
        free(m_handle);
        m_handle = NULL;
    }

    if (input_fd_dump != -1) {
        close(input_fd_dump);
        input_fd_dump = -1;
    }

    if (output_fd_dump != -1) {
        close(output_fd_dump);
        output_fd_dump = -1;
    }

    VLOG(INFO,"destroy tahoe end!!");
}

