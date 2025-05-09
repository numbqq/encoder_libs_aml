#define LOG_TAG "libvpcodec"
#include "vpcodec_1_0.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include "include/AML_HWEncoder.h"
#include "include/enc_define.h"
#ifdef __ANDROID__
#include "h264bitstream.h"
#endif
#ifndef __ANDROID__
#include <h264bitstream/h264_stream.h>
#endif

#define LOG_LINE() printf("[%s:%d]\n", __FUNCTION__, __LINE__)
const char version[] = "Amlogic libvpcodec version 1.0";

#define GE2D_FORMAT_BT601               (0 << 28)
#define GE2D_FORMAT_BT709               (1 << 28)

#define user_sei 1
#define ZEROBYTES_SHORTSTARTCODE 2 //indicates the number of zero bytes in the short start-code prefix
debug_log_level_t g_amvenc_log_level = ERR;

void amvenc_set_log_level(debug_log_level_t level)
{
    char *log_level = getenv("AMVENC_LOG_LEVEL");
    if (log_level) {
        g_amvenc_log_level = (debug_log_level_t)atoi(log_level);
        printf("Set log level by environment to %d\n", g_amvenc_log_level);
    } else {
        g_amvenc_log_level = level;
        printf("Set log level to %d\n", g_amvenc_log_level);
    }
    return;
}

int RBSPtoEBSP(unsigned char *streamBuffer, int begin_bytepos, int end_bytepos, int min_num_bytes)
{
    int i, j, count;
    char NAL_Payload_buffer[1024] = {0};
    for (i = begin_bytepos; i < end_bytepos; i++)
    NAL_Payload_buffer[i] = streamBuffer[i];

    count = 0;
    j = begin_bytepos;
    for (i = begin_bytepos; i < end_bytepos; i++)
    {
        if (count == ZEROBYTES_SHORTSTARTCODE && !(NAL_Payload_buffer[i] & 0xFC))
        {
            streamBuffer[j] = 0x03;
            j++;
            count = 0;
        }
        streamBuffer[j] = NAL_Payload_buffer[i];
        if (NAL_Payload_buffer[i] == 0x00)
          count++;
        else
          count = 0;
        j++;
    }
    while (j < begin_bytepos+min_num_bytes) {
      streamBuffer[j] = 0x00; // cabac stuffing word
      streamBuffer[j+1] = 0x00;
      streamBuffer[j+2] = 0x03;
      j += 3;
    }
    return j;
}

int EBSPtoRBSP(unsigned char *streamBuffer, int begin_bytepos, int end_bytepos) {
    int i, j, count;
    count = 0;

    if (end_bytepos < begin_bytepos)
        return end_bytepos;

    j = begin_bytepos;

    for (i = begin_bytepos; i < end_bytepos; i++)
    { //starting from begin_bytepos to avoid header information
    if (count == ZEROBYTES_SHORTSTARTCODE && streamBuffer[i] == 0x03)
    {
        i++;
        count = 0;
        }
        streamBuffer[j] = streamBuffer[i];
        if (streamBuffer[i] == 0x00)
          count++;
        else
          count = 0;
        j++;
    }
    return j;
}

const char *vl_get_version()
{
    return version;
}

int initEncParams(AMVEncHandle *handle, vl_init_params_t init_param)
{
    memset(&(handle->mEncParams), 0, sizeof(AMVEncParams));
    VLOG(DEBUG,"bit_rate:%d", init_param.bit_rate);
    if (/*(init_param.width % 16 != 0 || */init_param.height % 2 != 0/*)*/)
    {
        VLOG(WARN,"Video frame size %dx%d must be a multiple of 2", init_param.width, init_param.height);
        return -1;
    } else if (init_param.height % 16 != 0) {
        VLOG(WARN,"Video frame height is not standard:%d", init_param.height);
    } else {
        VLOG(INFO,"Video frame size is %d x %d", init_param.width, init_param.height);
    }

    handle->mEncParams.FreeRun = AVC_OFF;
    handle->mEncParams.rate_control = AVC_ON;
    handle->mEncParams.initQP = 0;
    handle->mEncParams.init_CBP_removal_delay = 1600;
    handle->mEncParams.auto_scd = AVC_ON;
    handle->mEncParams.out_of_band_param_set = AVC_ON;
    handle->mEncParams.num_ref_frame = 1;
    handle->mEncParams.num_slice_group = 1;
    handle->mEncParams.nSliceHeaderSpacing = 0;
    handle->mEncParams.fullsearch = AVC_OFF;
    handle->mEncParams.search_range = 16;
    //handle->mEncParams.sub_pel = AVC_OFF;
    //handle->mEncParams.submb_pred = AVC_OFF;
    handle->mEncParams.width = init_param.width;
    handle->mEncParams.height = init_param.height;
    handle->mEncParams.bitrate = init_param.bit_rate;
    handle->mEncParams.frame_rate = 1000 * init_param.frame_rate;  // In frames/ms!
    handle->mEncParams.CPB_size = (uint32)(init_param.bit_rate >> 1);
    handle->mEncParams.MBsIntraRefresh = 0;
    handle->mEncParams.MBsIntraOverlap = 0;
    //handle->mEncParams.encode_once = 1;
    if (ENC_CSC_BT709 == init_param.csc) {
        handle->mEncParams.color_space = GE2D_FORMAT_BT709;
    }
    else {
        handle->mEncParams.color_space = GE2D_FORMAT_BT601;
    }
    // Set IDR frame refresh interval
    /*if ((unsigned) gop == 0xffffffff)
    {
        handle->mEncParams.idr_period = -1;//(mIDRFrameRefreshIntervalInSec * mVideoFrameRate);
    }
    else if (gop == 0)
    {
        handle->mEncParams.idr_period = 0;  // All I frames
    }
    else
    {
        handle->mEncParams.idr_period = gop + 1;
    }*/
    if (init_param.gop == 0 || init_param.gop < 0) {
        handle->mEncParams.idr_period = 0;   //an infinite period, only one I frame
    } else {
        handle->mEncParams.idr_period = init_param.gop; //period of I frame, 1 means all frames are I type.
    }
    // Set profile and level
    if (0 != init_param.profile) {
        handle->mEncParams.profile = (AVCProfile)init_param.profile;//AVC_BASELINE;
        handle->mEncParams.level = (AVCLevel)init_param.level;//AVC_LEVEL4;
    }
    else {
        handle->mEncParams.profile = AVC_MAIN;
        handle->mEncParams.level = AVC_LEVEL4;
    }
    if (AVC_BASELINE == handle->mEncParams.profile) {
        handle->mEncParams.ucode_mode = 1;
    }
    else {
        handle->mEncParams.ucode_mode = 0;
    }
    handle->mEncParams.initQP = 20;
    handle->mEncParams.BitrateScale = AVC_OFF;
    //handle->mEncParams.rate_control = AVC_OFF;
    //add for qp limit
/*
    handle->mEncParams.i_qp_min = 10;
    handle->mEncParams.i_qp_max = 20;
    handle->mEncParams.p_qp_min = 10;
    handle->mEncParams.p_qp_max = 20;*/
    if (init_param.i_qp_max > 0 && init_param.i_qp_min > 0) {
        handle->mEncParams.i_qp_min = init_param.i_qp_min;
        handle->mEncParams.i_qp_max = init_param.i_qp_max;
    }
    if (init_param.p_qp_max > 0 && init_param.p_qp_min > 0) {
        handle->mEncParams.p_qp_min = init_param.p_qp_min;
        handle->mEncParams.p_qp_max = init_param.p_qp_max;
    }

    if (init_param.width * init_param.height > 1280 * 720) {
        handle->mEncParams.encode_once = 1; //fix me!is this right???
    }
    handle->sei.fps = init_param.frame_rate;
    handle->sei.bitrate = handle->mEncParams.bitrate;
    handle->sei.gop = handle->mEncParams.idr_period;
    handle->sei.maxQp_I = handle->mEncParams.i_qp_max;
    handle->sei.minQp_I = handle->mEncParams.i_qp_min;
    handle->sei.maxQp_P = handle->mEncParams.p_qp_max;
    handle->sei.minQp_P = handle->mEncParams.p_qp_min;
    return 0;
}

bool vl_video_encode_modifyheader(vl_vui_params_t vui,int framerate,int level,int height,int width,uint8_t *nal, unsigned int *dataLength)
{
    bs_t bs;
    sps_t sps;
    uint32_t i = 0;
    int sps_nalu_size;
    int pps_nalu_size;
    int new_sps_size;
    int pps_start = -1;
    int ret = 0;
    uint8_t new_sps[128] = {0};

    uint8_t *sps_nalu = (uint8_t *) malloc(sizeof(uint8_t) * (*dataLength));
    uint8_t *pps_nalu = (uint8_t *) malloc(sizeof(uint8_t) * (*dataLength));

    if (sps_nalu == NULL || pps_nalu == NULL) {
        VLOG(ERR,"malloc for sps or pps failed");
        return false;
    }

    for (i=0;i<*dataLength-5;i++) {
        if ((uint8_t)nal[i+0] == 0 && (uint8_t)nal[i+1] == 0 && (uint8_t)nal[i+2] == 0 && (uint8_t)nal[i+3] == 1 &&
            (((uint8_t)nal[i+4]) & 0x1f) == 8) {
            pps_start = i;
            VLOG(DEBUG,"pps_start=%d\n", pps_start);
            break;
        }
    }

    if (i >= *dataLength - 5) {
        return false;
    }

    memcpy(new_sps, nal, i);
    memcpy(pps_nalu, nal + i, *dataLength - i);

    sps_nalu_size = i;
    VLOG(NONE,"old sps_nalu_size=%d,datalen:%d", sps_nalu_size,*dataLength);
    pps_nalu_size = *dataLength - i;

    bs_init(&bs, new_sps + 5, sps_nalu_size - 5);
    read_seq_parameter_set_rbsp(&sps, &bs);
    read_rbsp_trailing_bits(&bs);
    sps.level_idc = level;//adjust level
    sps.vui_parameters_present_flag = 1;
    sps.vui.video_full_range_flag = vui.range;
    sps.vui.video_signal_type_present_flag = 1;
    sps.vui.colour_description_present_flag = 1;
    sps.vui.colour_primaries = vui.primaries;
    sps.vui.transfer_characteristics = vui.transfer;
    sps.vui.matrix_coefficients = vui.matrixCoeffs;
    sps.vui.timing_info_present_flag = 1;
    sps.vui.num_units_in_tick = 1000;
    sps.vui.time_scale = framerate * 2;
    sps.vui.fixed_frame_rate_flag = 1;
    if (height != (sps.pic_height_in_map_units_minus1 + 1) << 4 || width != (sps.pic_width_in_mbs_minus1 + 1) << 4) {
        sps.frame_cropping_flag = 1;
        sps.frame_crop_right_offset = (((sps.pic_width_in_mbs_minus1 + 1) << 4) - width) / 2;
        sps.frame_crop_bottom_offset = (((sps.pic_height_in_map_units_minus1 + 1) << 4) - height) / 2;
        VLOG(NONE,"crop info bottom:%d,height:%d,right:%d,width:%d",sps.frame_crop_bottom_offset,height,
                                                                    sps.frame_crop_right_offset,width);
    }

    memset(new_sps + 5, 0, sizeof(new_sps) - 5);
    bs_init(&bs, new_sps + 5, sizeof(new_sps) - 5);

    write_seq_parameter_set_rbsp(&sps, &bs);
    write_rbsp_trailing_bits(&bs);

    new_sps_size = bs.p - bs.start + 5;
    VLOG(NONE,"new_sps_size:%d",new_sps_size);

    new_sps_size = RBSPtoEBSP(new_sps,5,new_sps_size,0);
    VLOG(NONE,"new_sps_size after RBSPtoEBSP:%d",new_sps_size);

    memset(nal, 0, new_sps_size + pps_nalu_size);

    memcpy(nal, new_sps, new_sps_size);
    memcpy(nal + new_sps_size, pps_nalu, pps_nalu_size);

    *dataLength = new_sps_size + pps_nalu_size;

    memset(&sps, 0, sizeof(sps_t));
    new_sps_size = EBSPtoRBSP(new_sps,5,new_sps_size);
    bs_init(&bs, new_sps + 5, new_sps_size - 5);

    read_seq_parameter_set_rbsp(&sps, &bs);

    VLOG(NONE,"hacked: sps.vui_parameters_present_flag=%d\n", sps.vui_parameters_present_flag);
    VLOG(NONE,"hacked: sps.vui.video_full_range_flag=%d\n", sps.vui.video_full_range_flag);
    VLOG(NONE,"hacked: sps.vui.video_signal_type_present_flag=%d\n", sps.vui.video_signal_type_present_flag);
    VLOG(NONE,"hacked: sps.vui.colour_description_present_flag=%d\n", sps.vui.colour_description_present_flag);
    VLOG(NONE,"hacked: sps.vui.colour_primaries=%d\n", sps.vui.colour_primaries);
    VLOG(NONE,"hacked: sps.vui.transfer_characteristics=%d\n", sps.vui.transfer_characteristics);
    VLOG(NONE,"hacked: sps.vui.matrix_coefficients=%d\n", sps.vui.matrix_coefficients);
    VLOG(NONE,"hacked: sps.vui.num_units_in_tick=%d\n", sps.vui.num_units_in_tick);
    VLOG(NONE,"hacked: sps.vui.time_scale=%d\n", sps.vui.time_scale);
    VLOG(NONE,"hacked: sps.levelidc=%d\n", sps.level_idc);

    VLOG(NONE,"sps header len:%d",*dataLength);
    if (sps_nalu)
        free(sps_nalu);
    if (pps_nalu)
        free(pps_nalu);
    return true;
}



/* modify for const qp*/
int initEncParamsFixQp(AMVEncHandle *handle, int width, int height, int frame_rate, int bit_rate, int gop, int fix_qp)
{
    memset(&(handle->mEncParams), 0, sizeof(AMVEncParams));
    VLOG(NONE,"bit_rate:%d", bit_rate);
    if ((width % 16 != 0 || height % 2 != 0))
    {
        VLOG(ERR,"Video frame size %dx%d must be a multiple of 16", width, height);
        return -1;
    } else if (height % 16 != 0) {
        VLOG(WARN,"Video frame height is not standard:%d", height);
    } else {
        VLOG(NONE,"Video frame size is %d x %d", width, height);
    }
    /* modify for const qp*/
    if (fix_qp >= 0)
    {
        handle->mEncParams.rate_control = AVC_OFF;
        handle->mEncParams.initQP = fix_qp;
        printf("handle->mEncParams.rate_control, handle->mEncParams.initQP %d,%d\n",handle->mEncParams.rate_control, handle->mEncParams.initQP);
    }
    else
    {
        handle->mEncParams.rate_control = AVC_ON;
        handle->mEncParams.initQP = 20;
    }

    handle->mEncParams.init_CBP_removal_delay = 1600;
    handle->mEncParams.auto_scd = AVC_ON;
    handle->mEncParams.out_of_band_param_set = AVC_ON;
    handle->mEncParams.num_ref_frame = 1;
    handle->mEncParams.num_slice_group = 1;
    handle->mEncParams.nSliceHeaderSpacing = 0;
    handle->mEncParams.fullsearch = AVC_OFF;
    handle->mEncParams.search_range = 16;
    //handle->mEncParams.sub_pel = AVC_OFF;
    //handle->mEncParams.submb_pred = AVC_OFF;
    handle->mEncParams.width = width;
    handle->mEncParams.height = height;
    handle->mEncParams.bitrate = bit_rate;
    handle->mEncParams.frame_rate = 1000 * frame_rate;  // In frames/ms!
    handle->mEncParams.CPB_size = (uint32)(bit_rate >> 1);
    handle->mEncParams.FreeRun = AVC_OFF;
    handle->mEncParams.MBsIntraRefresh = 0;
    handle->mEncParams.MBsIntraOverlap = 0;
    handle->mEncParams.encode_once = 1;
    // Set IDR frame refresh interval
    /*if ((unsigned) gop == 0xffffffff)
    {
        handle->mEncParams.idr_period = -1;//(mIDRFrameRefreshIntervalInSec * mVideoFrameRate);
    }
    else if (gop == 0)
    {
        handle->mEncParams.idr_period = 0;  // All I frames
    }
    else
    {
        handle->mEncParams.idr_period = gop + 1;
    }*/
    if (gop == 0 || gop < 0) {
        handle->mEncParams.idr_period = 0;   //an infinite period, only one I frame
    } else {
        handle->mEncParams.idr_period = gop; //period of I frame, 1 means all frames are I type.
    }
    // Set profile and level
    handle->mEncParams.profile = AVC_BASELINE;
    handle->mEncParams.level = AVC_LEVEL4;
    handle->mEncParams.BitrateScale = AVC_OFF;
    return 0;
}

vl_codec_handle_t vl_video_encoder_init_fix_qp(vl_codec_id_t codec_id, int width, int height, int frame_rate, int bit_rate, int gop, vl_img_format_t img_format, int fix_qp)
{
    int ret;
    AMVEncHandle *mHandle = new AMVEncHandle;
    bool has_mix = false;
    int dump_opts = 0;
    char *env_h264enc_dump;

    if (mHandle == NULL)
        goto exit;

    memset(mHandle, 0, sizeof(AMVEncHandle));
    amvenc_set_log_level(ERR);
    ret = initEncParamsFixQp(mHandle, width, height, frame_rate, bit_rate, gop, fix_qp);
    if (ret < 0)
        goto exit;

    ret = AML_HWEncInitialize(mHandle, &(mHandle->mEncParams), &has_mix, 2);

    if (ret < 0)
        goto exit;

    mHandle->mSpsPpsHeaderReceived = false;
    mHandle->mNumInputFrames = -1;  // 1st two buffers contain SPS and PPS

    mHandle->fd_in = -1;
    mHandle->fd_out = -1;

    /*env_h264enc_dump = getenv("h264enc_dump");
    LOGAPI("h264enc_dump=%s\n", env_h264enc_dump);
	if (env_h264enc_dump)
        dump_opts = atoi(env_h264enc_dump);
    if (dump_opts == 1) {
        mHandle->fd_in = open("/tmp/h264enc_dump_in.raw", O_CREAT | O_WRONLY);
        if (mHandle->fd_in == -1)
            LOGAPI("OPEN file for dump h264enc input failed: %s\n", strerror(errno));
        mHandle->fd_out = -1;
    } else if (dump_opts == 2) {
        mHandle->fd_in  = -1;
        mHandle->fd_out = open("/tmp/h264enc_dump_out.264", O_CREAT | O_WRONLY);
        if (mHandle->fd_out == -1)
            LOGAPI("OPEN file for dump h264enc output failed: %s\n", strerror(errno));
    } else if (dump_opts == 3) {
        mHandle->fd_in = open("/tmp/h264enc_dump_in.raw", O_CREAT | O_WRONLY);
        if (mHandle->fd_in == -1)
            LOGAPI("OPEN file for dump h264enc input failed: %s\n", strerror(errno));

        mHandle->fd_out = open("/tmp/h264enc_dump_out.264", O_CREAT | O_WRONLY);
        if (mHandle->fd_out == -1)
            LOGAPI("OPEN file for dump h264enc output failed: %s\n", strerror(errno));
    } else {
        LOGAPI("h264enc_dump disabled\n");
        mHandle->fd_in = -1;
        mHandle->fd_out = -1;
    }*/

    return (vl_codec_handle_t) mHandle;

exit:
    if (mHandle != NULL)
        delete mHandle;

    return (vl_codec_handle_t) NULL;
}

vl_codec_handle_t vl_video_encoder_init(vl_codec_id_t codec_id, vl_init_params_t init_param)
{
    int ret;
    AMVEncHandle *mHandle = new AMVEncHandle;
    bool has_mix = false;
    int dump_opts = 0;
    char *env_h264enc_dump;

    if (mHandle == NULL)
        goto exit;

    memset(mHandle, 0, sizeof(AMVEncHandle));

    amvenc_set_log_level(ERR);

    ret = initEncParams(mHandle, init_param);
    if (ret < 0)
        goto exit;

    ret = AML_HWEncInitialize(mHandle, &(mHandle->mEncParams), &has_mix, 2);

    if (ret < 0)
        goto exit;

    mHandle->mSpsPpsHeaderReceived = false;
    mHandle->mNumInputFrames = -1;  // 1st two buffers contain SPS and PPS

    mHandle->fd_in = -1;
    mHandle->fd_out = -1;

#if ES_DUMP_ENABLE
    mHandle->esDumpFd = -1;
    mHandle->esDumpFd = open(ES_FILE_NAME, O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (mHandle->esDumpFd < 0)
    {
        printf("open es dump file error!\n");
        goto exit;
    }
#endif

    /*env_h264enc_dump = getenv("h264enc_dump");
    LOGAPI("h264enc_dump=%s\n", env_h264enc_dump);
	if (env_h264enc_dump)
		dump_opts = atoi(env_h264enc_dump);
    if (dump_opts == 1) {
        mHandle->fd_in = open("/tmp/h264enc_dump_in.raw", O_CREAT | O_WRONLY);
        if (mHandle->fd_in == -1)
            LOGAPI("OPEN file for dump h264enc input failed: %s\n", strerror(errno));
        mHandle->fd_out = -1;
    } else if (dump_opts == 2) {
        mHandle->fd_in  = -1;
        mHandle->fd_out = open("/tmp/h264enc_dump_out.264", O_CREAT | O_WRONLY);
        if (mHandle->fd_out == -1)
            LOGAPI("OPEN file for dump h264enc output failed: %s\n", strerror(errno));
    } else if (dump_opts == 3) {
        mHandle->fd_in = open("/tmp/h264enc_dump_in.raw", O_CREAT | O_WRONLY);
        if (mHandle->fd_in == -1)
            LOGAPI("OPEN file for dump h264enc input failed: %s\n", strerror(errno));

        mHandle->fd_out = open("/tmp/h264enc_dump_out.264", O_CREAT | O_WRONLY);
        if (mHandle->fd_out == -1)
            LOGAPI("OPEN file for dump h264enc output failed: %s\n", strerror(errno));
    } else {
        LOGAPI("h264enc_dump disabled\n");
        mHandle->fd_in = -1;
        mHandle->fd_out = -1;
    }*/

    return (vl_codec_handle_t) mHandle;

exit:
    if (mHandle != NULL)
        delete mHandle;

    return (vl_codec_handle_t) NULL;
}

static void write_sei_rbsp_private(bs_t *s, uint8_t *payload, int payload_size, int payload_type)
{
    int i;

    for (i = 0; i <= payload_type-255; i += 255)
        bs_write_u(s, 8, 255);
    bs_write_u(s, 8, payload_type-i);

    for (i = 0; i <= payload_size-255; i += 255)
        bs_write_u(s, 8, 255);
    bs_write_u(s, 8, payload_size-i);

    for (i = 0; i < payload_size; i++)
        bs_write_u(s, 8, payload[i]);
}

int vl_video_encode_sei(vl_sei_payload_t sei_payload, int *in_size, unsigned char *out)
{
    bs_t b;
    int rbsp_size = 0;
    unsigned char start_code[] = {0x00, 0x00, 0x00, 0x01};

    rbsp_size = sei_payload.payload_size + 4 + (sei_payload.payload_type == 5 ? 0 : 16); /*+4 for start code*/
    uint8_t* rbsp_buf = (uint8_t*)calloc(1, rbsp_size);

    if (rbsp_buf == NULL) {
        VLOG(ERR,"malloc for rbsp buf failed");
        return -1;
    }
    bs_init(&b, rbsp_buf, rbsp_size);

    bs_write_u(&b, 1, 0);
    bs_write_u(&b, 2, 0);
    bs_write_u(&b, 5, 6);

    write_sei_rbsp_private(&b, sei_payload.payload, sei_payload.payload_size, sei_payload.payload_type);
    write_rbsp_trailing_bits(&b);

    rbsp_size = bs_pos(&b);
    rbsp_size = RBSPtoEBSP(rbsp_buf, 0, rbsp_size,0);
    memset(out, 0, rbsp_size + 4);
    memcpy(out, start_code, 4);
    memcpy(out + 4, rbsp_buf, rbsp_size);
    *in_size = rbsp_size + 4;

    if (rbsp_buf)
        free(rbsp_buf);

    return *in_size;

}

int vl_video_encode_header(vl_codec_handle_t codec_handle, vl_vui_params_t vui, int in_size, unsigned char *out)
{
    int ret;
    int type;
    AMVEncHandle *handle = (AMVEncHandle *)codec_handle;
    if (!handle->mSpsPpsHeaderReceived)
    {
        ret = AML_HWEncNAL(handle, (unsigned char *)out, (unsigned int *)&in_size/*should be out size*/, &type);
        if (ret == AMVENC_SUCCESS)
        {
            vl_video_encode_modifyheader(vui,handle->mEncParams.frame_rate,(int)handle->mEncParams.level,handle->mEncParams.height,
                                         handle->mEncParams.width,out,(unsigned int *)&in_size);
            handle->mSPSPPSDataSize = 0;
            handle->mSPSPPSData = (uint8_t *)malloc(in_size);
            if (handle->mSPSPPSData)
            {
                handle->mSPSPPSDataSize = in_size;
                memcpy(handle->mSPSPPSData, (unsigned char *)out, handle->mSPSPPSDataSize);
                VLOG(NONE,"get mSPSPPSData size= %d at line %d \n", handle->mSPSPPSDataSize, __LINE__);
#ifndef __ANDROID__
                size_t merge_size = sizeof(sps_t) + sizeof(pps_t) + 5 + 5;
				uint8_t *merge_buf = (uint8_t *) malloc(merge_size);
				size_t merge_pos = 0;

				uint8_t *aux_buf = (uint8_t *) malloc(handle->mSPSPPSDataSize + 5);

				if (!merge_buf || !aux_buf)
					return -1;

				memset(merge_buf, 0, merge_size);
				memset(aux_buf, 0, handle->mSPSPPSDataSize + 5);

				memcpy(aux_buf, handle->mSPSPPSData, handle->mSPSPPSDataSize);

				*((int *)(aux_buf + handle->mSPSPPSDataSize)) = 0x01000000;  // add trailing nal delimeter

				h264_stream_t* h = h264_new();
				uint8_t* p = aux_buf;
				int nal_start, nal_end, nal_size;
				int64_t off = 0;
				size_t sz = handle->mSPSPPSDataSize + 5;
				size_t cursor = 0;

				uint8_t *sps_buf = (uint8_t *) malloc(sizeof(sps_t) + 5);
				uint8_t *pps_buf = (uint8_t *) malloc(sizeof(pps_t) + 5);
				memset(sps_buf, 0, sizeof(sps_t) + 5);
				memset(pps_buf, 0, sizeof(pps_t) + 5);

				while (find_nal_unit(p, sz, &nal_start, &nal_end) > 0) {
				   printf("!! Found NAL at offset %lld (0x%04llX), size %lld (0x%04llX) \n",
						  (long long int)(off + (p - aux_buf) + nal_start),
						  (long long int)(off + (p - aux_buf) + nal_start),
						  (long long int)(nal_end - nal_start),
						  (long long int)(nal_end - nal_start) );

		            p += nal_start;
		            read_nal_unit(h, p, nal_end - nal_start);

		            p += (nal_end - nal_start);
		            sz -= nal_end;

		            if (h->nal->nal_unit_type == NAL_UNIT_TYPE_SPS) {
						h->sps->vui_parameters_present_flag = 1;
						h->sps->vui.timing_info_present_flag = 1;
						h->sps->vui.num_units_in_tick = 1;
						h->sps->vui.time_scale = handle->mEncParams.frame_rate / 500;

						nal_size = write_nal_unit(h, sps_buf, sizeof(sps_t) + 5);

		                *((int*)(merge_buf + merge_pos)) = 0x01000000;
		                merge_pos += 4;
		                memcpy(merge_buf + merge_pos, sps_buf + 1, nal_size);
		                merge_pos += nal_size;

		                free(sps_buf);
		            } else if (h->nal->nal_unit_type == NAL_UNIT_TYPE_PPS) {
		                nal_size = write_nal_unit(h, pps_buf, sizeof(pps_t) + 5);

		                *((int*)(merge_buf + merge_pos)) = 0x01000000;
		                merge_pos += 4;
		                memcpy(merge_buf + merge_pos, pps_buf + 1, nal_size);
		                merge_pos += nal_size;

		                free(pps_buf);
		            }
				}
	            free(handle->mSPSPPSData);
	            handle->mSPSPPSData = merge_buf;
	            handle->mSPSPPSDataSize = merge_pos;
	            free(aux_buf);
	            h264_free(h);
#endif
            }

            handle->mNumInputFrames = 0;
            handle->mSpsPpsHeaderReceived = true;
        }
        else
        {
            VLOG(ERR,"Encode SPS and PPS error, encoderStatus = %d. handle: %p\n", ret, (void *)handle);
            return -1;
        }
    }
    else {
        memcpy(out,handle->mSPSPPSData,handle->mSPSPPSDataSize);
    }
    return handle->mSPSPPSDataSize;
}

vl_enc_result_e vl_video_encoder_getavgqp(vl_codec_handle_t codec_handle, float *avg_qp) {
    AMVEnc_Status ret;
    AMVEncHandle *handle = (AMVEncHandle *)codec_handle;
    float avgqp;

    ret = AML_HWGetAvgQp(handle, &avgqp);
    if (ret == AMVENC_SUCCESS) {
        *avg_qp = avgqp;
    } else {
        VLOG(ERR,"get avgqp fail! ret = %d at line %d", ret, __LINE__);
    }
    return ENC_SUCCESS;
}

int vl_video_encoder_encode(vl_codec_handle_t codec_handle, vl_frame_type_t frame_type, unsigned char *in, int in_size, unsigned char *out, int format)
{
    int ret;
    uint8_t *outPtr = NULL;
    uint32_t dataLength = 0;
    int type;
    size_t rawdata_size = in_size;
    ssize_t io_ret;
    AMVEncHandle *handle = (AMVEncHandle *)codec_handle;
    if (!handle->mSpsPpsHeaderReceived)
    {
        ret = AML_HWEncNAL(handle, (unsigned char *)out, (unsigned int *)&in_size/*should be out size*/, &type);
        if (ret == AMVENC_SUCCESS)
        {
            handle->mSPSPPSDataSize = 0;
            handle->mSPSPPSData = (uint8_t *)malloc(in_size);

            if (handle->mSPSPPSData)
            {
                handle->mSPSPPSDataSize = in_size;
                memcpy(handle->mSPSPPSData, (unsigned char *)out, handle->mSPSPPSDataSize);
                VLOG(NONE,"get mSPSPPSData size= %d at line %d \n", handle->mSPSPPSDataSize, __LINE__);
#ifndef __ANDROID__
                size_t merge_size = sizeof(sps_t) + sizeof(pps_t) + 5 + 5;
				uint8_t *merge_buf = (uint8_t *) malloc(merge_size);
				size_t merge_pos = 0;

				uint8_t *aux_buf = (uint8_t *) malloc(handle->mSPSPPSDataSize + 5);

				if (!merge_buf || !aux_buf)
					return -1;

				memset(merge_buf, 0, merge_size);
				memset(aux_buf, 0, handle->mSPSPPSDataSize + 5);

				memcpy(aux_buf, handle->mSPSPPSData, handle->mSPSPPSDataSize);

				*((int *)(aux_buf + handle->mSPSPPSDataSize)) = 0x01000000;  // add trailing nal delimeter

				h264_stream_t* h = h264_new();
				uint8_t* p = aux_buf;
				int nal_start, nal_end, nal_size;
				int64_t off = 0;
				size_t sz = handle->mSPSPPSDataSize + 5;
				size_t cursor = 0;

				uint8_t *sps_buf = (uint8_t *) malloc(sizeof(sps_t) + 5);
				uint8_t *pps_buf = (uint8_t *) malloc(sizeof(pps_t) + 5);
				memset(sps_buf, 0, sizeof(sps_t) + 5);
				memset(pps_buf, 0, sizeof(pps_t) + 5);

				while (find_nal_unit(p, sz, &nal_start, &nal_end) > 0) {
				   printf("!! Found NAL at offset %lld (0x%04llX), size %lld (0x%04llX) \n",
						  (long long int)(off + (p - aux_buf) + nal_start),
						  (long long int)(off + (p - aux_buf) + nal_start),
						  (long long int)(nal_end - nal_start),
						  (long long int)(nal_end - nal_start) );

		            p += nal_start;
		            read_nal_unit(h, p, nal_end - nal_start);

		            p += (nal_end - nal_start);
		            sz -= nal_end;

		            if (h->nal->nal_unit_type == NAL_UNIT_TYPE_SPS) {
						h->sps->vui_parameters_present_flag = 1;
						h->sps->vui.timing_info_present_flag = 1;
						h->sps->vui.num_units_in_tick = 1;
						h->sps->vui.time_scale = handle->mEncParams.frame_rate / 500;

						nal_size = write_nal_unit(h, sps_buf, sizeof(sps_t) + 5);

		                *((int*)(merge_buf + merge_pos)) = 0x01000000;
		                merge_pos += 4;
		                memcpy(merge_buf + merge_pos, sps_buf + 1, nal_size);
		                merge_pos += nal_size;

		                free(sps_buf);
		            } else if (h->nal->nal_unit_type == NAL_UNIT_TYPE_PPS) {
		                nal_size = write_nal_unit(h, pps_buf, sizeof(pps_t) + 5);

		                *((int*)(merge_buf + merge_pos)) = 0x01000000;
		                merge_pos += 4;
		                memcpy(merge_buf + merge_pos, pps_buf + 1, nal_size);
		                merge_pos += nal_size;

		                free(pps_buf);
		            }
				}
	            free(handle->mSPSPPSData);
	            handle->mSPSPPSData = merge_buf;
	            handle->mSPSPPSDataSize = merge_pos;
	            free(aux_buf);
	            h264_free(h);
#endif
            }

            handle->mNumInputFrames = 0;
            handle->mSpsPpsHeaderReceived = true;
        }
        else
        {
            VLOG(ERR,"Encode SPS and PPS error, encoderStatus = %d. handle: %p\n", ret, (void *)handle);
            return -1;
        }
    }
    if (handle->mNumInputFrames >= 0)
    {
        AMVEncFrameIO videoInput;
        memset(&videoInput, 0, sizeof(videoInput));
        videoInput.height = handle->mEncParams.height;
        videoInput.pitch = ((handle->mEncParams.width + 15) >> 4) << 4;
        /* TODO*/
        videoInput.bitrate = handle->mEncParams.bitrate;
        videoInput.frame_rate = handle->mEncParams.frame_rate / 1000;
        videoInput.coding_timestamp = handle->mNumInputFrames * 1000 / videoInput.frame_rate;  // in ms
        //LOGAPI("mNumInputFrames %lld, videoInput.coding_timestamp %llu, videoInput.frame_rate %f\n", handle->mNumInputFrames, videoInput.coding_timestamp,videoInput.frame_rate);

        videoInput.YCbCr[0] = (unsigned long)&in[0];
        videoInput.YCbCr[1] = (unsigned long)(videoInput.YCbCr[0] + videoInput.height * videoInput.pitch);

        if (format == 0) { //NV12
            videoInput.fmt = AMVENC_NV12;
            videoInput.YCbCr[2] = 0;
        } else if(format == 1) { //NV21
            videoInput.fmt = AMVENC_NV21;
            videoInput.YCbCr[2] = 0;
        } else if (format == 2) { //YV12
            videoInput.fmt = AMVENC_YUV420;
            videoInput.YCbCr[2] = (unsigned long)(videoInput.YCbCr[1] + videoInput.height * videoInput.pitch / 4);
        } else if (format == 3) { //rgb888
            videoInput.fmt = AMVENC_RGB888;
            videoInput.YCbCr[1] = 0;
            videoInput.YCbCr[2] = 0;
        } else if (format == 4) { //bgr888
            videoInput.fmt = AMVENC_BGR888;
        }

        videoInput.canvas = 0xffffffff;
        videoInput.type = VMALLOC_BUFFER;
        videoInput.disp_order = handle->mNumInputFrames;

        videoInput.op_flag = 0;
        if (frame_type == FRAME_TYPE_IDR) //response omx's IDR request
        {
            handle->mKeyFrameRequested = true;
        }

        if (handle->mKeyFrameRequested == true)
        {
            videoInput.op_flag = AMVEncFrameIO_FORCE_IDR_FLAG;
            handle->mKeyFrameRequested = false;
        }
        ret = AML_HWSetInput(handle, &videoInput);
        ++(handle->mNumInputFrames);
        if (ret == AMVENC_SUCCESS || ret == AMVENC_NEW_IDR)
        {
            if (ret == AMVENC_NEW_IDR)
            {
                outPtr = (uint8_t *) out + handle->mSPSPPSDataSize;
                dataLength  = /*should be out size */in_size - handle->mSPSPPSDataSize;
            }
            else
            {
                outPtr = (uint8_t *) out;
                dataLength  = /*should be out size */in_size;
            }

            if (handle->fd_in >= 0) {
                io_ret = write(handle->fd_in, in, rawdata_size);
                if (io_ret == -1) {
                    printf("write raw frame failed: %s\n", errno);
                } else if (io_ret < rawdata_size) {
                    printf("write raw frame: short write %zu vs %zd\n", rawdata_size, io_ret);
                }
            }
        }
        else if (ret < AMVENC_SUCCESS)
        {
            VLOG(NONE,"encoderStatus = %d at line %d, handle: %p", ret, __LINE__, (void *)handle);
            return -1;
        }

        ret = AML_HWEncNAL(handle, (unsigned char *)outPtr, (unsigned int *)&dataLength, &type);
        if (ret == AMVENC_PICTURE_READY)
        {
            if (type == AVC_NALTYPE_IDR)
            {
                if (handle->mSPSPPSData)
                {
                    memcpy((uint8_t *) out, handle->mSPSPPSData, handle->mSPSPPSDataSize);
                    dataLength += handle->mSPSPPSDataSize;
                    VLOG(NONE,"copy mSPSPPSData to buffer size= %d at line %d \n", handle->mSPSPPSDataSize, __LINE__);
                }
            }
            if (handle->fd_out >= 0) {
				io_ret = write(handle->fd_out, (void *)out, dataLength);
                if (io_ret == -1) {
                    printf("write h264 packet failed: %s\n", errno);
                } else if (io_ret < dataLength) {
                printf("write raw h264 packet: short write %zu vs %zd\n", dataLength, io_ret);
                }
            }
        }
        else if ((ret == AMVENC_SKIPPED_PICTURE) || (ret == AMVENC_TIMEOUT))
        {
            dataLength = 0;
            if (ret == AMVENC_TIMEOUT)
            {
                handle->mKeyFrameRequested = true;
                ret = AMVENC_SKIPPED_PICTURE;
            }
            VLOG(NONE,"ret = %d at line %d, handle: %p", ret, __LINE__, (void *)handle);
        }
        else if (ret != AMVENC_SUCCESS)
        {
            dataLength = 0;
        }

        if (ret < AMVENC_SUCCESS)
        {
            VLOG(DEBUG,"encoderStatus = %d at line %d, handle: %p", ret , __LINE__, (void *)handle);
            return -1;
        }
    }

#if ES_DUMP_ENABLE
	if (dataLength >= 0) {
        write(handle->esDumpFd, (unsigned char *)out, dataLength);
    }
#endif

    VLOG(DEBUG,"vl_video_encoder_encode return %d\n",dataLength);
    return dataLength;
}

int int_to_ascii(int value, unsigned char *userData, int index) {
    int num_digits = 0, temp_value = value;

    do {
        num_digits++;
        temp_value /= 10;
    } while (temp_value > 0);

    temp_value = value;
    for (int i = index + num_digits - 1; i >= index; i--) {
        userData[i] = (temp_value % 10) + '0';
        temp_value /= 10;
    }
    return num_digits;
}

void InitUserSei(sei_s *sei) {
    static unsigned char userData[44];
    int userDataIndex = 16;
    int fps,bitrate;
    unsigned char payLoadType[10] = {0x41, 0x56, 0x43, 0x46, 0x49,0x4E,0x49, 0x49, 0x2D, 0x50};

    const unsigned char uuid[16] = {
        0x41, 0xFE, 0x3D, 0xAF, 0x9D, 0x22, 0x4C, 0xD7,
        0x8F, 0x10, 0x79, 0x9F, 0x3F, 0x39, 0x9F, 0xEF
    };

    for (int i = 0; i < 16; i++) {
        userData[i] = uuid[i];
    }
    fps = sei->fps / 5;
    bitrate = sei->bitrate / 100000;
    // fps
    userDataIndex += int_to_ascii(fps, userData, userDataIndex);

    for (int i = 0; i < 3; i++) {
        userData[userDataIndex++] = payLoadType[i];
    }
    // force I
    if (sei->Force_I == 1) {
        userData[userDataIndex++] = payLoadType[3];
        userData[userDataIndex++] = payLoadType[4];
    }
    else {
        userData[userDataIndex++] = payLoadType[5];
        userData[userDataIndex++] = payLoadType[6];
    }

    // bitrate
    userDataIndex += int_to_ascii(bitrate, userData, userDataIndex);

    // I minQP
    userData[userDataIndex++] = payLoadType[7];
    userDataIndex += int_to_ascii(sei->minQp_I, userData, userDataIndex);

    // I maxQP
    userData[userDataIndex++] = payLoadType[8];
    userDataIndex += int_to_ascii(sei->maxQp_I, userData, userDataIndex);

    // P minQP
    userData[userDataIndex++] = payLoadType[9];
    userDataIndex += int_to_ascii(sei->minQp_P, userData, userDataIndex);

    // P maxQP
    userData[userDataIndex++] = payLoadType[8];
    userDataIndex += int_to_ascii(sei->maxQp_P, userData, userDataIndex);

    sei->userDataSize = userDataIndex;
    sei->pUserData = userData;
}


vl_enc_result_e vl_video_encoder_encode_frame(vl_codec_handle_t codec_handle, vl_frame_info_t frame_info, unsigned char *out,int *out_size,vl_frame_type_t *frame_type)
{
    int ret;
    uint8_t *outPtr = NULL;
    uint32_t dataLength = 0;
    int type;
    size_t rawdata_size = frame_info.frame_size;//in_size;
    ssize_t io_ret;
    uint8_t *sei_data = NULL;
    int sei_size = 0;
    AMVEncHandle *handle = (AMVEncHandle *)codec_handle;
    if (!handle->mSpsPpsHeaderReceived)
    {
        ret = AML_HWEncNAL(handle, (unsigned char *)out, (unsigned int *)out_size/*should be out size*/, &type);
        if (ret == AMVENC_SUCCESS)
        {
            handle->mSPSPPSDataSize = 0;
            handle->mSPSPPSData = (uint8_t *)malloc(*out_size);

            if (handle->mSPSPPSData)
            {
                handle->mSPSPPSDataSize = (*out_size);
                memcpy(handle->mSPSPPSData, (unsigned char *)out, handle->mSPSPPSDataSize);
                VLOG(DEBUG,"get mSPSPPSData size= %d at line %d\n", handle->mSPSPPSDataSize, __LINE__);
#ifndef __ANDROID__
                size_t merge_size = sizeof(sps_t) + sizeof(pps_t) + 5 + 5;
				uint8_t *merge_buf = (uint8_t *) malloc(merge_size);
				size_t merge_pos = 0;

				uint8_t *aux_buf = (uint8_t *) malloc(handle->mSPSPPSDataSize + 5);

				if (!merge_buf || !aux_buf)
					return ENC_FAILED;

				memset(merge_buf, 0, merge_size);
				memset(aux_buf, 0, handle->mSPSPPSDataSize + 5);

				memcpy(aux_buf, handle->mSPSPPSData, handle->mSPSPPSDataSize);

				*((int *)(aux_buf + handle->mSPSPPSDataSize)) = 0x01000000;  // add trailing nal delimeter

				h264_stream_t* h = h264_new();
				uint8_t* p = aux_buf;
				int nal_start, nal_end, nal_size;
				int64_t off = 0;
				size_t sz = handle->mSPSPPSDataSize + 5;
				size_t cursor = 0;

				uint8_t *sps_buf = (uint8_t *) malloc(sizeof(sps_t) + 5);
				uint8_t *pps_buf = (uint8_t *) malloc(sizeof(pps_t) + 5);
				memset(sps_buf, 0, sizeof(sps_t) + 5);
				memset(pps_buf, 0, sizeof(pps_t) + 5);

				while (find_nal_unit(p, sz, &nal_start, &nal_end) > 0) {
				   printf("!! Found NAL at offset %lld (0x%04llX), size %lld (0x%04llX) \n",
						  (long long int)(off + (p - aux_buf) + nal_start),
						  (long long int)(off + (p - aux_buf) + nal_start),
						  (long long int)(nal_end - nal_start),
						  (long long int)(nal_end - nal_start) );

		            p += nal_start;
		            read_nal_unit(h, p, nal_end - nal_start);

		            p += (nal_end - nal_start);
		            sz -= nal_end;

		            if (h->nal->nal_unit_type == NAL_UNIT_TYPE_SPS) {
						h->sps->vui_parameters_present_flag = 1;
						h->sps->vui.timing_info_present_flag = 1;
						h->sps->vui.num_units_in_tick = 1;
						h->sps->vui.time_scale = handle->mEncParams.frame_rate / 500;

						nal_size = write_nal_unit(h, sps_buf, sizeof(sps_t) + 5);

		                *((int*)(merge_buf + merge_pos)) = 0x01000000;
		                merge_pos += 4;
		                memcpy(merge_buf + merge_pos, sps_buf + 1, nal_size);
		                merge_pos += nal_size;

		                free(sps_buf);
		            } else if (h->nal->nal_unit_type == NAL_UNIT_TYPE_PPS) {
		                nal_size = write_nal_unit(h, pps_buf, sizeof(pps_t) + 5);

		                *((int*)(merge_buf + merge_pos)) = 0x01000000;
		                merge_pos += 4;
		                memcpy(merge_buf + merge_pos, pps_buf + 1, nal_size);
		                merge_pos += nal_size;

		                free(pps_buf);
		            }
				}
	            free(handle->mSPSPPSData);
	            handle->mSPSPPSData = merge_buf;
	            handle->mSPSPPSDataSize = merge_pos;
	            free(aux_buf);
	            h264_free(h);
#endif
            }

            handle->mNumInputFrames = 0;
            handle->mSpsPpsHeaderReceived = true;
        }
        else
        {
            VLOG(ERR,"Encode SPS and PPS error, encoderStatus = %d. handle:%p\n", ret, (void *)handle);
            return ENC_FAILED;
        }
    }
    if (handle->mNumInputFrames >= 0)
    {
        AMVEncFrameIO videoInput;
        memset(&videoInput, 0, sizeof(videoInput));
        videoInput.height = frame_info.height;//handle->mEncParams.height;
        videoInput.pitch = frame_info.pitch;//((handle->mEncParams.width + 15) >> 4) << 4;
        /* TODO*/
        videoInput.bitrate = frame_info.bitrate;//handle->mEncParams.bitrate;
        videoInput.frame_rate = frame_info.frame_rate / 1000;//frame_rate;
        VLOG(INFO,"frame_rate:%f",videoInput.frame_rate);
        videoInput.coding_timestamp = handle->mNumInputFrames * 1000 / videoInput.frame_rate;  // in ms
        //LOGAPI("mNumInputFrames %lld, videoInput.coding_timestamp %llu, videoInput.frame_rate %f\n", handle->mNumInputFrames, videoInput.coding_timestamp,videoInput.frame_rate);

        //videoInput.YCbCr[0] = frame_info.YCbCr[0];//(unsigned long)&in[0];
        //videoInput.YCbCr[1] = (unsigned long)(videoInput.YCbCr[0] + videoInput.height * videoInput.pitch);

        if (frame_info.fmt == IMG_FMT_NV12) { //NV12
            videoInput.fmt = AMVENC_NV12;
            //videoInput.YCbCr[2] = 0;
        } else if(frame_info.fmt == IMG_FMT_NV21) { //NV21
            videoInput.fmt = AMVENC_NV21;
            //videoInput.YCbCr[2] = 0;
        } else if (frame_info.fmt == IMG_FMT_YV12) { //YV12
            videoInput.fmt = AMVENC_YUV420;
            //videoInput.YCbCr[2] = (unsigned long)(videoInput.YCbCr[1] + videoInput.height * videoInput.pitch / 4);
        } else if (frame_info.fmt == IMG_FMT_RGB888) { //rgb888
            videoInput.fmt = AMVENC_RGB888;
            //videoInput.YCbCr[1] = 0;
            //videoInput.YCbCr[2] = 0;
        } else if (frame_info.fmt == IMG_FMT_BGR888) { //bgr888
            videoInput.fmt = AMVENC_BGR888;
        }
        else if (frame_info.fmt == IMG_FMT_RGBA8888) { //rgba8888
            videoInput.fmt = AMVENC_RGBA8888;
        }
        videoInput.YCbCr[0] = frame_info.YCbCr[0];
        videoInput.YCbCr[1] = frame_info.YCbCr[1];
        videoInput.YCbCr[2] = frame_info.YCbCr[2];
        videoInput.canvas = frame_info.canvas;//0xffffffff;
        videoInput.type = (AMVEncBufferType)frame_info.type;//VMALLOC_BUFFER;
        videoInput.disp_order = handle->mNumInputFrames;
        videoInput.scale_width = frame_info.scale_width;
        videoInput.scale_height = frame_info.scale_height;
        videoInput.crop_left = frame_info.crop_left;
        videoInput.crop_right = frame_info.crop_right;
        videoInput.crop_top = frame_info.crop_top;
        videoInput.crop_bottom = frame_info.crop_bottom;
        if (videoInput.type == DMA_BUFF) {
            videoInput.num_planes = frame_info.plane_num;
            videoInput.shared_fd[0] = frame_info.YCbCr[0];
            videoInput.shared_fd[1] = frame_info.YCbCr[1];
            videoInput.shared_fd[2] = frame_info.YCbCr[2];
            VLOG(INFO,"num_planes:%d,fd:%d",videoInput.num_planes,videoInput.shared_fd[0]);
        }
        videoInput.op_flag = 0;
        handle->sei.Force_I = 0;
        if (frame_info.frame_type == FRAME_TYPE_IDR) //response omx's IDR request
        {
            handle->mKeyFrameRequested = true;
        }

        if (handle->mKeyFrameRequested == true)
        {
            videoInput.op_flag = AMVEncFrameIO_FORCE_IDR_FLAG;
            handle->sei.Force_I = 1;
            handle->mKeyFrameRequested = false;
        }
        VLOG(DEBUG,"before AML_HWSetInput");
        ret = AML_HWSetInput(handle, &videoInput);
        VLOG(DEBUG,"after AML_HWSetInput");
        ++(handle->mNumInputFrames);
        if (ret == AMVENC_SUCCESS || ret == AMVENC_NEW_IDR)
        {
            if (ret == AMVENC_NEW_IDR)
            {
                outPtr = (uint8_t *) out + handle->mSPSPPSDataSize;
                dataLength  = /*should be out size */frame_info.frame_size - handle->mSPSPPSDataSize;
                (*frame_type) = FRAME_TYPE_IDR;
            }
            else
            {
                outPtr = (uint8_t *) out;
                dataLength  = /*should be out size */frame_info.frame_size;
            }

            if (handle->fd_in >= 0) {
                io_ret = write(handle->fd_in, (void *)frame_info.YCbCr[0], rawdata_size);
                if (io_ret == -1) {
                    printf("write raw frame failed: %s\n", errno);
                } else if (io_ret < rawdata_size) {
                    printf("write raw frame: short write %zu vs %zd\n", rawdata_size, io_ret);
                }
            }
        }
        else if (ret < AMVENC_SUCCESS)
        {
            VLOG(ERR,"encoderStatus = %d at line %d, handle: %p", ret, __LINE__, (void *)handle);
            return ENC_FAILED;
        }
        if ((frame_info.sei_payload.payload_size > 0) && (NULL  != frame_info.sei_payload.payload))
        {
            sei_data = outPtr;
            ret = vl_video_encode_sei(frame_info.sei_payload, &sei_size, sei_data);
            if (ret > 0) {
                outPtr = (uint8_t *)sei_data + sei_size;
                VLOG(DEBUG,"sei_size: %d\n", sei_size);
            }
        }
#if user_sei
        if (ret == AMVENC_NEW_IDR) {
            InitUserSei(&handle->sei);
            frame_info.sei_userdata.payload_size = handle->sei.userDataSize;
            frame_info.sei_userdata.payload = handle->sei.pUserData;
            if ((frame_info.sei_userdata.payload_size > 0) && (NULL  != frame_info.sei_userdata.payload))
            {
                sei_data = outPtr;
                frame_info.sei_userdata.payload_type = 5;
                ret = vl_video_encode_sei(frame_info.sei_userdata, &sei_size, sei_data);
                if (ret > 0) {
                    outPtr = (uint8_t *)sei_data + sei_size;
                    VLOG(DEBUG,"unregister user sei_size: %d\n", sei_size);
                }
            }
        }
#endif
        VLOG(DEBUG,"before AML_HWEncNAL");
        ret = AML_HWEncNAL(handle, (unsigned char *)outPtr, (unsigned int *)&dataLength, &type);
        VLOG(DEBUG,"after AML_HWEncNAL");
        if (ret == AMVENC_PICTURE_READY)
        {
            if (type == AVC_NALTYPE_IDR)
            {
                if (handle->mSPSPPSData)
                {
                    memcpy((uint8_t *) out, handle->mSPSPPSData, handle->mSPSPPSDataSize);
                    dataLength += handle->mSPSPPSDataSize;
                    VLOG(DEBUG,"copy mSPSPPSData to buffer size= %d at line %d \n", handle->mSPSPPSDataSize, __LINE__);
                }
            }
            if (sei_size > 0)
            {
                dataLength += sei_size;
            }
            if (handle->fd_out >= 0) {
                io_ret = write(handle->fd_out, (void *)out, dataLength);
                if (io_ret == -1) {
                    printf("write h264 packet failed: %s\n", errno);
                } else if (io_ret < dataLength) {
                printf("write raw h264 packet: short write %zu vs %zd\n", dataLength, io_ret);
                }
            }
        }
        else if ((ret == AMVENC_SKIPPED_PICTURE) || (ret == AMVENC_TIMEOUT))
        {
            dataLength = 0;
            if (ret == AMVENC_TIMEOUT)
            {
                handle->mKeyFrameRequested = true;
                ret = AMVENC_SKIPPED_PICTURE;
            }
            (*out_size) = dataLength;
            VLOG(DEBUG,"ret = %d at line %d, handle: %p", ret, __LINE__, (void *)handle);
            return ENC_SKIPPED_PICTURE;
        }
        else if (ret != AMVENC_SUCCESS)
        {
            dataLength = 0;
        }

        if (ret < AMVENC_SUCCESS)
        {
            (*out_size) = dataLength;
            VLOG(DEBUG,"encoderStatus = %d at line %d, handle: %p", ret , __LINE__, (void *)handle);
            return ENC_FAILED;
        }
    }

#if ES_DUMP_ENABLE
	if (dataLength >= 0) {
        write(handle->esDumpFd, (unsigned char *)out, dataLength);
    }
#endif
    (*out_size) = dataLength;
    VLOG(DEBUG,"vl_video_encoder_encode return %d\n",dataLength);
    return ENC_SUCCESS;
}
/*double round(double r)
{
    return (r>0.0)?floor(r+0.5):ceil(r-0.5);
}*/

int vl_video_encoder_destroy(vl_codec_handle_t codec_handle)
{
    AMVEncHandle *handle = (AMVEncHandle *)codec_handle;

    if (handle->fd_in >= 0) {
        fsync(handle->fd_in);
        close(handle->fd_in);
    }
    if (handle->fd_out >= 0) {
        fsync(handle->fd_out);
        close(handle->fd_out);
    }

    AML_HWEncRelease(handle);
    if (handle->mSPSPPSData)
        free(handle->mSPSPPSData);
    if (handle)
        delete handle;

#if ES_DUMP_ENABLE
    close(handle->esDumpFd);
#endif

    return 1;
}
