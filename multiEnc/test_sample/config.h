//------------------------------------------------------------------------------
// File: config.h
//
// Copyright (c) 2006, Chips & Media.  All rights reserved.
// This file should be modified by some developers of C&M according to product version.
//------------------------------------------------------------------------------
#ifndef __CONFIG_H__
#define __CONFIG_H__

#if defined(_WIN32) || defined(__WIN32__) || defined(_WIN64) || defined(WIN32) || defined(__MINGW32__)
#	define PLATFORM_WIN32
#elif defined(linux) || defined(__linux) || defined(ANDROID)
#	define PLATFORM_LINUX
#elif defined(unix) || defined(__unix)
#   define PLATFORM_QNX
#else
#	define PLATFORM_NON_OS
#endif

#if defined(_MSC_VER)
#	include <windows.h>
#	define inline _inline
#elif defined(__GNUC__)
#elif defined(__ARMCC__)
#else
#  error "Unknown compiler."
#endif

#define API_VERSION_MAJOR       5
#define API_VERSION_MINOR       5
#define API_VERSION_PATCH       51
#define API_VERSION             ((API_VERSION_MAJOR<<16) | (API_VERSION_MINOR<<8) | API_VERSION_PATCH)

#if defined(PLATFORM_NON_OS) || defined (ANDROID) || defined(MFHMFT_EXPORTS) || defined(PLATFORM_QNX) || defined(CNM_SIM_PLATFORM)
//#define SUPPORT_FFMPEG_DEMUX
#else
#define SUPPORT_FFMPEG_DEMUX
#endif

//------------------------------------------------------------------------------
// COMMON
//------------------------------------------------------------------------------
#if defined(linux) || defined(__linux) || defined(ANDROID) || defined(CNM_FPGA_HAPS_INTERFACE)
#define SUPPORT_MULTI_INST_INTR
#endif

// do not define BIT_CODE_FILE_PATH in case of multiple product support. because wave410 and coda980 has different firmware binary format.
#define CORE_0_BIT_CODE_FILE_PATH   "coda960.out"     // for coda960
#define CORE_1_BIT_CODE_FILE_PATH   "coda980.out"     // for coda980
#define CORE_2_BIT_CODE_FILE_PATH   "picasso.bin"     // for wave510
#define CORE_4_BIT_CODE_FILE_PATH   "millet.bin"      // for wave520
#define CORE_5_BIT_CODE_FILE_PATH   "kepler.bin"      // for wave515
#define CORE_6_BIT_CODE_FILE_PATH   "chagall.bin"     // for wave521

//------------------------------------------------------------------------------
// OMX
//------------------------------------------------------------------------------








//------------------------------------------------------------------------------
// WAVE521 or WAVE521C
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// WAVE511
//------------------------------------------------------------------------------


//#define SUPPORT_SW_UART
//#define SUPPORT_SW_UART_V2	// WAVE511 or WAVE521 or WAVE521C
#endif /* __CONFIG_H__ */
 

