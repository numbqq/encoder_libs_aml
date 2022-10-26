/*------------------------------------------------------------------------------
--       Copyright (c) 2015-2019, VeriSilicon Inc. All rights reserved        --
--         Copyright (c) 2011-2014, Google Inc. All rights reserved.          --
--         Copyright (c) 2007-2010, Hantro OY. All rights reserved.           --
--                                                                            --
-- This software is confidential and proprietary and may be used only as      --
--   expressly authorized by VeriSilicon in a written licensing agreement.    --
--                                                                            --
--         This entire notice must be reproduced on all copies                --
--                       and may not be removed.                              --
--                                                                            --
--------------------------------------------------------------------------------
-- Redistribution and use in source and binary forms, with or without         --
-- modification, are permitted provided that the following conditions are met:--
--   * Redistributions of source code must retain the above copyright notice, --
--       this list of conditions and the following disclaimer.                --
--   * Redistributions in binary form must reproduce the above copyright      --
--       notice, this list of conditions and the following disclaimer in the  --
--       documentation and/or other materials provided with the distribution. --
--   * Neither the names of Google nor the names of its contributors may be   --
--       used to endorse or promote products derived from this software       --
--       without specific prior written permission.                           --
--------------------------------------------------------------------------------
-- THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"--
-- AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE  --
-- IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE --
-- ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE  --
-- LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR        --
-- CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF       --
-- SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS   --
-- INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN    --
-- CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)    --
-- ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE --
-- POSSIBILITY OF SUCH DAMAGE.                                                --
--------------------------------------------------------------------------------
--                                                                            --
-  Description : Video Stabilization Standalone API
-
------------------------------------------------------------------------------*/
#ifndef __VIDSTBAPI_H__
#define __VIDSTBAPI_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "base_type.h"

/* Function return values */
typedef enum VideoStbRet_ {
    VIDEOSTB_OK = 0,
    VIDEOSTB_ERROR = -1,
    VIDEOSTB_NULL_ARGUMENT = -2,
    VIDEOSTB_INVALID_ARGUMENT = -3,
    VIDEOSTB_MEMORY_ERROR = -4,
    VIDEOSTB_EWL_ERROR = -5,
    VIDEOSTB_EWL_MEMORY_ERROR = -6,
    VIDEOSTB_HW_BUS_ERROR = -9,
    VIDEOSTB_HW_TIMEOUT = -11,
    VIDEOSTB_HW_RESERVED = -12,
    VIDEOSTB_SYSTEM_ERROR = -13,
    VIDEOSTB_INSTANCE_ERROR = -14,
    VIDEOSTB_HW_RESET = -16
} VideoStbRet;

/* YUV type for initialization */
typedef enum VideoStbInputFormat_ {
    VIDEOSTB_YUV420_PLANAR = 0,           /* YYYY... UUUU... VVVV */
    VIDEOSTB_YUV420_SEMIPLANAR = 1,       /* YYYY... UVUVUV...    */
    VIDEOSTB_YUV420_SEMIPLANAR_VU = 2,    /* YYYY... VUVUVU...    */
    VIDEOSTB_YUV422_INTERLEAVED_YUYV = 3, /* YUYVYUYV...          */
    VIDEOSTB_YUV422_INTERLEAVED_UYVY = 4, /* UYVYUYVY...          */
    VIDEOSTB_RGB565 = 5,                  /* 16-bit RGB           */
    VIDEOSTB_BGR565 = 6,                  /* 16-bit RGB           */
    VIDEOSTB_RGB555 = 7,                  /* 15-bit RGB           */
    VIDEOSTB_BGR555 = 8,                  /* 15-bit RGB           */
    VIDEOSTB_RGB444 = 9,                  /* 12-bit RGB           */
    VIDEOSTB_BGR444 = 10,                 /* 12-bit RGB           */
    VIDEOSTB_RGB888 = 11,                 /* 24-bit RGB           */
    VIDEOSTB_BGR888 = 12,                 /* 24-bit RGB           */
    VIDEOSTB_RGB101010 = 13,              /* 30-bit RGB           */
    VIDEOSTB_BGR101010 = 14               /* 30-bit RGB           */
} VideoStbInputFormat;

typedef const void *VideoStbInst;

typedef struct VideoStbParam_ {
    u32 inputWidth;
    u32 inputHeight;
    u32 stride;
    u32 stabilizedWidth;
    u32 stabilizedHeight;
    VideoStbInputFormat format;
    u32 client_type;
} VideoStbParam;

typedef struct VideoStbResult_ {
    u32 stabOffsetX;
    u32 stabOffsetY;
} VideoStbResult;

/* Version information */
typedef struct {
    u32 major; /* API major version */
    u32 minor; /* API minor version */
} VideoStbApiVersion;

typedef struct {
    u32 swBuild; /* Software build ID */
    u32 hwBuild; /* Hardware build ID */
} VideoStbBuild;

/*------------------------------------------------------------------------------
    API prototypes
------------------------------------------------------------------------------*/

/* Version information */
VideoStbApiVersion VideoStbGetApiVersion(void);
VideoStbBuild VideoStbGetBuild(void);

/* Initialization & release */
VideoStbRet VideoStbInit(VideoStbInst *instAddr, const VideoStbParam *param);
VideoStbRet VideoStbReset(VideoStbInst vidStab, const VideoStbParam *param);
VideoStbRet VideoStbRelease(VideoStbInst vidStab);

/* Stabilize next image based on the current one */
VideoStbRet VideoStbStabilize(VideoStbInst vidStab, VideoStbResult *result, ptr_t referenceFrameLum,
                              ptr_t stabilizedFameLum);

/* API tracing callback function */
void VideoStb_Trace(const char *str);

#ifdef __cplusplus
}
#endif

#endif /* __VIDSTBAPI_H__ */
