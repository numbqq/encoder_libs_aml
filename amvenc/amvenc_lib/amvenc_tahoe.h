/*
 * * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 * *
 * * This source code is subject to the terms and conditions defined in below
 * * which is part of this source code package.
 * *
 * * Description:
 */

#ifndef __AMVENC_TAHOE_H__
#define __AMVENC_TAHOE_H__

#ifdef __cplusplus
extern "C" {
#endif

int amvenc_tahoe_load_Module(void);
amvenc_handle_t amvenc_tahoe_init(amvenc_codec_id_t codec_id,
                                    amvenc_info_t encode_info,
                                    amvenc_qp_param_t* qp_tbl);
amvenc_metadata_t amvenc_tahoe_generate_header(amvenc_handle_t handle,
                                                          unsigned char *pHeader,
                                                          unsigned int *pLength);
amvenc_metadata_t amvenc_tahoe_encode(amvenc_handle_t handle,
                                           amvenc_frame_info_t frame_info,
                                           amvenc_frame_type_t type,
                                           unsigned char* out,
                                           amvenc_buffer_info_t *in_buffer_info,
                                           amvenc_buffer_info_t *ret_buffer_info);
int amvenc_tahoe_change_bitrate(amvenc_handle_t handle,          int bitRate);
int amvenc_tahoe_destroy(amvenc_handle_t handle);
int amvenc_tahoe_unload_Module(void);

#ifdef __cplusplus
}
#endif

#endif /* __AMVENC_MULTI_H__ */

