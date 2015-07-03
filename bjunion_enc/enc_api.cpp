
//#define LOG_NDEBUG 0
#define LOG_TAG "AMLVENC_API"
#include <utils/Log.h>

#include <stdio.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

#include "enc_api.h"
#include <cutils/properties.h>

#include "enc/m8_enc_fast/m8venclib_fast.h"
#include "enc/m8_enc_fast/rate_control_m8_fast.h"
#include "enc/m8_enc/m8venclib.h"
#include "enc/m8_enc/rate_control_m8.h"

#define ENCODER_PATH       "/dev/amvenc_avc"

#define AMVENC_DEVINFO_M8_FAST "AML-M8-FAST"
#define AMVENC_DEVINFO_M8 "AML-M8"

#define AMVENC_AVC_IOC_MAGIC  'E'
#define AMVENC_AVC_IOC_GET_DEVINFO              _IOW(AMVENC_AVC_IOC_MAGIC, 0xf0, unsigned int)

const AMVencHWFuncPtr m8_fast_dev =
{
    InitFastEncode,
    FastEncodeInitFrame,
    FastEncodeSPS_PPS,
    FastEncodeSlice,
    FastEncodeCommit,
    UnInitFastEncode,
};

const AMVencHWFuncPtr m8_dev =
{
    InitM8VEncode,
    M8VEncodeInitFrame,
    M8VEncodeSPS_PPS,
    M8VEncodeSlice,
    M8VEncodeCommit,
    UnInitM8VEncode,
};

const AMVencHWFuncPtr *gdev[] =
{
    &m8_fast_dev,
    &m8_dev,
    NULL,
};

const AMVencRCFuncPtr m8_fast_rc =
{
    FastInitRateControlModule,
    FastRCUpdateBuffer,
    FastRCUpdateFrame,
    FastRCInitFrameQP,
    FastCleanupRateControlModule,
};

const AMVencRCFuncPtr m8_rc =
{
    M8InitRateControlModule,
    M8RCUpdateBuffer,
    M8RCUpdateFrame,
    M8RCInitFrameQP,
    M8CleanupRateControlModule,
};

const AMVencRCFuncPtr *grc[] =
{
    &m8_fast_rc,
    &m8_rc,
    NULL,
};

AMVEnc_Status AMInitRateControlModule(amvenc_hw_t *hw_info)
{
    if (!hw_info)
        return AMVENC_MEMORY_FAIL;
    if ((hw_info->dev_id <= NO_DEFINE) || (hw_info->dev_id >= MAX_DEV) || (hw_info->dev_fd < 0) || (!hw_info->dev_data))
        return AMVENC_FAIL;
    if (grc[hw_info->dev_id]->Initialize != NULL)
        hw_info->rc_data = grc[hw_info->dev_id]->Initialize(&hw_info->init_para);
    if (!hw_info->rc_data)
    {
        ALOGD("AMInitRateControlModule Fail, dev type:%d. fd:%d", hw_info->dev_id, hw_info->dev_fd);
        return AMVENC_FAIL;
    }
    return AMVENC_SUCCESS;
}

AMVEnc_Status AMPreRateControl(amvenc_hw_t *hw_info, int frameInc, bool force_IDR)
{
    AMVEnc_Status ret = AMVENC_FAIL;
    if (!hw_info)
        return AMVENC_MEMORY_FAIL;
    if ((hw_info->dev_id <= NO_DEFINE) || (hw_info->dev_id >= MAX_DEV) || (!hw_info->rc_data))
        return AMVENC_FAIL;
    if (grc[hw_info->dev_id]->PreControl != NULL)
        ret = grc[hw_info->dev_id]->PreControl(hw_info->dev_data, hw_info->rc_data, frameInc, force_IDR);
    return ret;
}

AMVEnc_Status AMPostRateControl(amvenc_hw_t *hw_info, bool IDR, int *skip_num, int numFrameBits)
{
    AMVEnc_Status ret = AMVENC_FAIL;
    if (!hw_info)
        return AMVENC_MEMORY_FAIL;
    if ((hw_info->dev_id <= NO_DEFINE) || (hw_info->dev_id >= MAX_DEV) || (!hw_info->rc_data))
        return AMVENC_FAIL;
    if (grc[hw_info->dev_id]->PostControl != NULL)
        ret = grc[hw_info->dev_id]->PostControl(hw_info->dev_data, hw_info->rc_data, IDR, skip_num, numFrameBits);
    return ret;
}

AMVEnc_Status AMRCInitFrameQP(amvenc_hw_t *hw_info, bool IDR, int bitrate, float frame_rate)
{
    AMVEnc_Status ret = AMVENC_FAIL;
    if (!hw_info)
        return AMVENC_MEMORY_FAIL;
    if ((hw_info->dev_id <= NO_DEFINE) || (hw_info->dev_id >= MAX_DEV) || (!hw_info->rc_data))
        return AMVENC_FAIL;
    if (grc[hw_info->dev_id]->InitFrameQP != NULL)
        ret = grc[hw_info->dev_id]->InitFrameQP(hw_info->dev_data, hw_info->rc_data, IDR, bitrate, frame_rate);
    return ret;
}

void AMCleanupRateControlModule(amvenc_hw_t *hw_info)
{
    if (!hw_info)
        return;
    if ((!hw_info->rc_data) && (hw_info->dev_id > NO_DEFINE) && (hw_info->dev_id < MAX_DEV))
        grc[hw_info->dev_id]->Release(hw_info->rc_data);
    hw_info->rc_data = NULL;
    return;
}

AMVEnc_Status InitAMVEncode(amvenc_hw_t *hw_info, int force_mode)
{
    char dev_info[16];
    int iret = -1, fd = -1;
    if (!hw_info)
        return AMVENC_MEMORY_FAIL;
    hw_info->dev_fd = -1;
    hw_info->dev_data = NULL;
    fd = open(ENCODER_PATH, O_RDWR);
    if (fd < 0)
    {
        return AMVENC_FAIL;
    }
    memset(dev_info, 0, sizeof(dev_info));
#if 0//need debug
    iret = ioctl(fd, AMVENC_AVC_IOC_GET_DEVINFO, &dev_info[0]);
    if ((iret < 0) || (strcmp(dev_info, (char *)AMVENC_DEVINFO_MT) == 0) || (dev_info[0] == 0))
    {
        if ((iret < 0) || (dev_info[0] == 0))
            ALOGD("The old encoder driver, not support query the dev info. set as MT type!");
        hw_info->dev_id = MT;
    }
    else if (strcmp(dev_info, (char *)AMVENC_DEVINFO_M8) == 0)
    {
        hw_info->dev_id = M8;
    }
    else
    {
        hw_info->dev_id = NO_DEFINE;
    }
#else
    if ((hw_info->init_para.enc_width >= 1280) && (hw_info->init_para.enc_height >= 720))
        hw_info->dev_id = M8_FAST;
    else
        hw_info->dev_id = M8;
    if (1 == force_mode)
    {
        hw_info->dev_id = M8_FAST;
    }
    else if (2 == force_mode)
    {
        hw_info->dev_id = M8;
    }
    char prop[PROPERTY_VALUE_MAX];
    int value = 0;
    memset(prop, 0, sizeof(prop));
    if (property_get("hw.encoder.forcemode", prop, NULL) > 0)
    {
        sscanf(prop, "%d", &value);
    }
    else
    {
        value = 0;
    }
    if (1 == value)
    {
        hw_info->dev_id = M8_FAST;
    }
    else if (2 == value)
    {
        hw_info->dev_id = M8;
    }
    ALOGI("hw.encoder.forcemode = %d, dev_id=%d. fd:%d", value, hw_info->dev_id, fd);
#endif
    if ((hw_info->dev_id <= NO_DEFINE) || (hw_info->dev_id >= MAX_DEV))
    {
        ALOGD("Not found available hw encoder device, fd:%d", fd);
        close(fd);
        return AMVENC_FAIL;
    }
    if (gdev[hw_info->dev_id]->Initialize != NULL)
        hw_info->dev_data = gdev[hw_info->dev_id]->Initialize(fd, &hw_info->init_para);
    if (!hw_info->dev_data)
    {
        ALOGD("InitAMVEncode Fail, dev type:%d. fd:%d", hw_info->dev_id, fd);
        hw_info->dev_id = NO_DEFINE;
        close(fd);
        return AMVENC_FAIL;
    }
    hw_info->dev_fd = fd;
    return AMVENC_SUCCESS;
}

AMVEnc_Status AMVEncodeInitFrame(amvenc_hw_t *hw_info, unsigned *yuv, AMVEncBufferType type, AMVEncFrameFmt fmt, bool IDRframe)
{
    AMVEnc_Status ret = AMVENC_FAIL;
    if (!hw_info)
        return AMVENC_MEMORY_FAIL;
    if ((hw_info->dev_id <= NO_DEFINE) || (hw_info->dev_id >= MAX_DEV) || (hw_info->dev_fd < 0) || (!hw_info->dev_data))
        return AMVENC_FAIL;
    if (gdev[hw_info->dev_id]->InitFrame != NULL)
        ret = gdev[hw_info->dev_id]->InitFrame(hw_info->dev_data, yuv, type, fmt, IDRframe);
    return ret;
}

AMVEnc_Status AMVEncodeSPS_PPS(amvenc_hw_t *hw_info, unsigned char *outptr, int *datalen)
{
    AMVEnc_Status ret = AMVENC_FAIL;
    if (!hw_info)
        return AMVENC_MEMORY_FAIL;
    if ((hw_info->dev_id <= NO_DEFINE) || (hw_info->dev_id >= MAX_DEV) || (hw_info->dev_fd < 0) || (!hw_info->dev_data))
        return AMVENC_FAIL;
    if (gdev[hw_info->dev_id]->EncodeSPS_PPS != NULL)
        ret = gdev[hw_info->dev_id]->EncodeSPS_PPS(hw_info->dev_data, outptr, datalen);
    return ret;
}

AMVEnc_Status AMVEncodeSlice(amvenc_hw_t *hw_info, unsigned char *outptr, int *datalen)
{
    AMVEnc_Status ret = AMVENC_FAIL;
    if (!hw_info)
        return AMVENC_MEMORY_FAIL;
    if ((hw_info->dev_id <= NO_DEFINE) || (hw_info->dev_id >= MAX_DEV) || (hw_info->dev_fd < 0) || (!hw_info->dev_data))
        return AMVENC_FAIL;
    if (gdev[hw_info->dev_id]->EncodeSlice != NULL)
        ret = gdev[hw_info->dev_id]->EncodeSlice(hw_info->dev_data, outptr, datalen);
    return ret;
}

AMVEnc_Status AMVEncodeCommit(amvenc_hw_t *hw_info, bool IDR)
{
    AMVEnc_Status ret = AMVENC_FAIL;
    if (!hw_info)
        return AMVENC_MEMORY_FAIL;
    if ((hw_info->dev_id <= NO_DEFINE) || (hw_info->dev_id >= MAX_DEV) || (hw_info->dev_fd < 0) || (!hw_info->dev_data))
        return AMVENC_FAIL;
    if (gdev[hw_info->dev_id]->Commit != NULL)
        ret = gdev[hw_info->dev_id]->Commit(hw_info->dev_data, IDR);
    return ret;
}

void UnInitAMVEncode(amvenc_hw_t *hw_info)
{
    if (!hw_info)
        return;
    if ((hw_info->dev_data) && (hw_info->dev_id > NO_DEFINE) && (hw_info->dev_id < MAX_DEV))
        gdev[hw_info->dev_id]->Release(hw_info->dev_data);
    hw_info->dev_data = NULL;
    if (hw_info->dev_fd >= 0)
    {
        close(hw_info->dev_fd);
    }
    hw_info->dev_fd = -1;
    hw_info->dev_id = NO_DEFINE;
    return;
}
