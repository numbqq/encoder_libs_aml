# Options in Testbench for VC9000E VIDEO

## Encoder Input Frames Options

### Source Raw Information

#### Encoder Input/Output Frames Options

| Short | Long Option            | Description                                                    |
|-------|------------------------|----------------------------------------------------------------|
| -i[s] | --input                | Read input video sequence from file. [input.yuv]               |
| -o[s] | --output               | Write output HEVC/H.264 stream to file. [stream.hevc]          |
| -a[n] | --firstPic             | The first picture to encode in the input file. [0]             |
| -b[n] | --lastPic              | The last picture to encode in the input file. [100]            |
|       | --outReconFrame        | 0..1 Reconstructed frame output mode. [1]                      |
|       |                        |  0 - Do not output reconstructed frames.                       |
|       |                        |  1 - Output reconstructed frames.                              |
| -j[n] | --inputRateNumer       | 1..1048575 Input picture rate numerator. [30]                  |
| -J[n] | --inputRateDenom       | 1..1048575 Input picture rate denominator. [1]                 |
| -f[n] | --outputRateNumer      | 1..1048575 Output picture rate numerator. [same as input]      |
| -F[n] | --outputRateDenom      | 1..1048575 Output picture rate denominator. [same as input]    |
|       |                        |  Encoded stream frame rate will be:                            |
|       |                        |   outputRateNumer/outputRateDenom fps                          |
|       | --writeReconToDDR      | 0..1 enable write recon to DDR [1]                             |
|       |                        | enable/disable write recon to DDR in I-frame only encoding.    |
|       |                        |   0: disable recon write to DDR.                               |
|       |                        |   1: enable recon write to DDR.                                |

#### Input and Encoded Frame Resolutions and Cropping Options

| Short | Long Option            | Description                                                    |
|-------|------------------------|----------------------------------------------------------------|
| -w[n] | --lumWidthSrc          | Width of source image in pixels. [176]                         |
| -h[n] | --lumHeightSrc         | Height of source image in pixels. [144]                        |
| -x[n] | --width                | Width of encoded output image in pixels. [--lumWidthSrc]       |
| -y[n] | --height               | Height of encoded output image in pixels. [--lumHeightSrc]     |
| -X[n] | --horOffsetSrc         | Output image horizontal cropping offset, must be even. [0]     |
| -Y[n] | --verOffsetSrc         | Output image vertical cropping offset, must be even. [0]       |
|       | --inputFileList        | User-specified input file list to test resolution change,      |
|       |                        | options in the file can include:                               |
|       |                        |  input file path(-i),                                          |
|       |                        |  first picture(-a),                                            |
|       |                        |  last picture(-b),                                             |
|       |                        |  width(-w),                                                    |
|       |                        |  height(-h).                                                   |
|       |                        | Note: when inputFileList is specified, the input file paths in |
|       |                        |   command line be ignored. and the width and height will be    |
|       |                        |   used as the largest width and height in initialization cfg.  | 
|       |                        |   the input options will be read from the list file directly.  | 

#### Parameters for DEC400 Compressed Table(Tile Status)

| Short | Long Option            | Description                                                    |
|-------|------------------------|----------------------------------------------------------------|
|       | --dec400TableInput     | Read input DEC400 compressed table from file.                  |
|       |                        |  default is "dec400CompTableinput.bin"                         |

### Pre-processing Options

| Short | Long Option            | Description                                                    |
|-------|------------------------|----------------------------------------------------------------|
| -l[n] | --inputFormat          | Input YUV format. [0]                                          |
|       |                        |   0 - YUV420 planar CbCr (IYUV/I420)                           |
|       |                        |   1 - YUV420 semiplanar CbCr (NV12)                            |
|       |                        |   2 - YUV420 semiplanar CrCb (NV21)                            |
|       |                        |   3 - YUYV422 interleaved (YUYV/YUY2)                          |
|       |                        |   4 - UYVY422 interleaved (UYVY/Y422)                          |
|       |                        |   5 - RGB565 16bpp                                             |
|       |                        |   6 - BGR565 16bpp                                             |
|       |                        |   7 - RGB555 16bpp                                             |
|       |                        |   8 - BGR555 16bpp                                             |
|       |                        |   9 - RGB444 16bpp                                             |
|       |                        |   10 - BGR444 16bpp                                            |
|       |                        |   11 - RGB888 32bpp                                            |
|       |                        |   12 - BGR888 32bpp                                            |
|       |                        |   13 - RGB101010 32bpp                                         |
|       |                        |   14 - BGR101010 32bpp                                         |
|       |                        |   15 - I010, YUV420 10 bit (1 pixel in 2 bytes) planar CbCr    |
|       |                        |   16 - P010, YUV420 10 bit (1 pixel in 2 bytes) semiplanar CbCr|
|       |                        |   17 - Packed YUV420 10 bit (4 pixels in 5 bytes) planar       |
|       |                        |        (Verisilicon defined)                                   |
|       |                        |   18 - Y0L2, YUV420 10 bit YCbCr interleaved                   |
|       |                        |        (4 pixels in 8 bytes)                                   |
|       |                        |   19 - YUV420 customer private tile for HEVC                   |
|       |                        |   20 - YUV420 customer private tile for H.264                  |
|       |                        |   21 - YUV420 semiplanar CbCr customer private tile            |
|       |                        |   22 - YUV420 semiplanar CrCb customer private tile            |
|       |                        |   23 - YUV420 P010 10 bit CbCr customer private tile           |
|       |                        |   24 - YUV420 semiplanar CbCr 101010                           |
|       |                        |   26 - YUV420 CrCb 8 bit tile 64x4                             |
|       |                        |   27 - YUV420 CbCr 8 bit tile 64x4                             |
|       |                        |   28 - YUV420 CbCr 10 bit tile 32x4                            |
|       |                        |   29 - YUV420 CbCr 10 bit tile 48x4                            |
|       |                        |   30 - YUV420 CrCb 10 bit tile 48x4                            |
|       |                        |   31 - YUV420 CrCb 8 bit tile 128x2                            |
|       |                        |   32 - YUV420 CbCr 8 bit tile 128x2                            |
|       |                        |   33 - YUV420 CbCr 10 bit tile 96x2                            |
|       |                        |   34 - YUV420 CrCb 10 bit tile 96x2                            |
|       |                        |   35 - YUV420 semiplanar CbCr 8 bit tile 8x8                   |
|       |                        |   36 - YUV420 semiplanar CbCr 10 bit tile 8x8                  |
|       |                        |   37 - YVU420 CrCb 8 bit planar                                |
|       |                        |   38 - YUV420 semiplannar CbCr 8 bit tile 64x2                 |
|       |                        |   39 - YUV420 semiplannar CbCr 10 bit tile 128x2               |
|       |                        |   40 - RGB888 24bpp (MSB)BGR(LSB)                              |
|       |                        |   41 - BGR888 24bpp (MSB)RGB(LSB)                              |
|       |                        |   42 - RBG888 24bpp (MSB)GBR(LSB)                              |
|       |                        |   43 - GBR888 24bpp (MSB)RBG(LSB)                              |
|       |                        |   44 - BRG888 24bpp (MSB)GRB(LSB)                              |
|       |                        |   45 - GRB888 24bpp (MSB)BRG(LSB)                              |
| -O[n] | --colorConversion      |   RGB to YCbCr color conversion type. [0]                      |
|       |                        |   0 - ITU-R BT.601, RGB limited [16..235] (BT601_l.mat)        |
|       |                        |   1 - ITU-R BT.709, RGB limited [16..235] (BT709_l.mat)        |
|       |                        |   2 - User defined, coefficients defined in test bench.        |
|       |                        |   3 - ITU-R BT.2020                                            |
|       |                        |   4 - ITU-R BT.601, RGB full [0..255] (BT601_f.mat)            |
|       |                        |   5 - ITU-R BT.601, RGB limited [0..219] (BT601_219.mat)       |
|       |                        |   6 - ITU-R BT.709, RGB full [0..255] (BT709_f.mat)            |
|       | --mirror               |   Mirror input image. [0]                                      |
|       |                        |   0 - disabled                                                 |
|       |                        |   1 - mirror                                                   |
| -r[n] | --rotation             |   Rotate input image. [0]                                      |
|       |                        |   0 - disabled                                                 |
|       |                        |   1 -  90 degrees right                                        |
|       |                        |   2 -  90 degrees left                                         |
|       |                        |   3 - 180 degrees right                                        |
| -Z[n] | --videoStab            |   Enable video stabilization or scene change detection. [0]    |
|       |                        |   Stabilization works by adjusting cropping offset for every   |
|       |                        |   frame; Scene change detection works by filling the           |
|       |                        |   nextSceneChanged field in output structure according to the  |
|       |                        |   detection result. When this option is enable, it works as    |
|       |                        |   stabilization and scene dection when input resolution is     |
|       |                        |   larger than encoded resolution; and it works as scene dection|
|       |                        |   mode only when the input resolution is the same as the       |
|       |                        |   encoded resolution. In stablization mode, the crop offset    |
|       |                        |   setting will be ignored. The algorithem will select proper   |
|       |                        |   offset to keep the scene stable.                             |
|       | --scaledWidth          | 0..width, width of encoder down-scaled image, [0]              |
|       |                        |   0 - Downscaling is disabled                                  |
|       |                        |   The Down-scaled sequence is written in file scaled.yuv       |
|       | --scaledHeight         | 0..height, height of encoder down-scaled image, [0]            |
|       |                        |   0 - disable written in scaled.yuv                            |
|       | --scaledOutputFormat   | 0..1 scaler output frame format. [0]                           |
|       |                        |   0 - YUV422, 1 - YUV420 semiplanar CbCr (NV12)                |
|       | --interlacedFrame      | 0..1 Flag to indicate if input picture is interlace. [0]       |
|       |                        |   0 - Input progressive field                                  |
|       |                        |   1 - Input frame with fields interlaced [0]                   |
|       | --fieldOrder           | Interlaced field order, 0=bottom first, 1=top first [0]        |
|       | --codedChromaIdc       | Coded chroma format idc in encoded stream.[1]                  |
|       |                        |   0 - 4:0:0 monochroma, only valid for HEVC/H.264.             |
|       |                        |   1 - 4:2:0                                                    |

### Convert Input to Customized Format Options

| Short | Long Option            | Description                                                    |
|-------|------------------------|----------------------------------------------------------------|
|       | --formatCustomizedType | Convert input to customized specific format, set customized    |
|       |                        | input format ID. The converted format will be used as final    |
|       |                        | encoder input. It is used when the private format YUV sequence |
|       |                        | is not available.  It is not supported when multi-tile.        |
|       |                        | Valid Range: -1..13                                            |
|       |                        |  -1    - Disable format conversion (default)                   |
|       |                        |  0..13 - The cusotomer private format ID.                      |

## Encoder Stream Coding Tools

### Output Stream Formats

#### Output Stream and Encoding Tools Options

| Short | Long Option            | Description                                                    |
|-------|------------------------|----------------------------------------------------------------|
| -P[n] | --profile              | HEVC encoder supported profiles:                               |
|       |                        |   0 - Main profile (default)                                   |
|       |                        |   1 - Main Still Picture profile                               |
|       |                        |   2 - Main 10 profile                                          |
|       |                        |   3 - MAINREXT profile                                         |
|       |                        | H.264 encoder supported profiles:                              |
|       |                        |   9  - Baseline profile                                        |
|       |                        |   10 - Main profile                                            |
|       |                        |   11 - High profile (default)                                  |
|       |                        |   12 - High 10 profile                                         |
|       |                        | AV1 encoder supported profiles:                                |
|       |                        |   0 - Main profile (default)                                   |
| -L[n] | --level                | 0xFFFF indicates the auto level, sw will generate the level    |
|       |                        | HEVC level. (180 = level 6.0*30) [180]                         |
|       |                        |   Up to level 5.1 High tier is supported in real-time.         |
|       |                        |   For levels 5.0 and 5.1, resolutions up to 4096x2048 are      |
|       |                        |   supported with 4K@60fps performance. For levels greater than |
|       |                        |   level 5.1, support in non-realtime mode.                     |
|       |                        |   Each level has resolution and bitrate limitations:           |
|       |                        |    n  HEVC Resolution Main Tier    High Tier                   |
|       |                        |    .  level Name     Max Bitrate   Max Bitrate                 |
|       |                        |   30   1.0  QCIF       128 kbps        -                       |
|       |                        |   60   2.0  CIF        1.5 Mbps        -                       |
|       |                        |   63   2.1  Q720p      3.0 Mbps        -                       |
|       |                        |   90   3.0  QHD        6.0 Mbps        -                       |
|       |                        |   93   3.1  720p HD    10.0 Mbps       -                       |
|       |                        |   120  4.0  2Kx1080    12.0 Mbps   30 Mbps                     |
|       |                        |   123  4.1  2Kx1080    20.0 Mbps   50 Mbps                     |
|       |                        |   150  5.0  4096x2160  25.0 Mbps   100 Mbps                    |
|       |                        |   153  5.1  4096x2160  40.0 Mbps   160 Mbps                    |
|       |                        |   156  5.2  4096x2160  60.0 Mbps   240 Mbps                    |
|       |                        |   180  6.0  8192x4320  60.0 Mbps   240 Mbps                    |
|       |                        |   183  6.1  8192x4320  120.0 Mbps  480 Mbps                    |
|       |                        |   186  6.2  8192x4320  240.0 Mbps  800 Mbps                    |
|       |                        | H.264 level. (51 = Level 5.1) [51]                             |
|       |                        |   10 - H264_LEVEL_1                                            |
|       |                        |   99 - H264_LEVEL_1_b                                          |
|       |                        |   11 - H264_LEVEL_1_1                                          |
|       |                        |   12 - H264_LEVEL_1_2                                          |
|       |                        |   13 - H264_LEVEL_1_3                                          |
|       |                        |   20 - H264_LEVEL_2                                            |
|       |                        |   21 - H264_LEVEL_2_1                                          |
|       |                        |   22 - H264_LEVEL_2_2                                          |
|       |                        |   30 - H264_LEVEL_3                                            |
|       |                        |   31 - H264_LEVEL_3_1                                          |
|       |                        |   32 - H264_LEVEL_3_2                                          |
|       |                        |   40 - H264_LEVEL_4                                            |
|       |                        |   41 - H264_LEVEL_4_1                                          |
|       |                        |   42 - H264_LEVEL_4_2                                          |
|       |                        |   50 - H264_LEVEL_5                                            |
|       |                        |   51 - H264_LEVEL_5_1                                          |
|       |                        |   52 - H264_LEVEL_5_2                                          |
|       |                        |   60 - H264_LEVEL_6                                            |
|       |                        |   61 - H264_LEVEL_6_1                                          |
|       |                        |   62 - H264_LEVEL_6_2                                          |
|       |                        | AV1 level. (13 = Level 5.1) [13]                               |
|       |                        |   0  - AV1_LEVEL_2.0                                           |
|       |                        |   1  - AV1_LEVEL_2.1                                           |
|       |                        |   2  - AV1_LEVEL_2.2                                           |
|       |                        |   3  - AV1_LEVEL_2.3                                           |
|       |                        |   4  - AV1_LEVEL_3.0                                           |
|       |                        |   5  - AV1_LEVEL_3.1                                           |
|       |                        |   6  - AV1_LEVEL_3.2                                           |
|       |                        |   7  - AV1_LEVEL_3.3                                           |
|       |                        |   8  - AV1_LEVEL_4.0                                           |
|       |                        |   9  - AV1_LEVEL_4.1                                           |
|       |                        |   10 - AV1_LEVEL_4.2                                           |
|       |                        |   11 - AV1_LEVEL_4.3                                           |
|       |                        |   12 - AV1_LEVEL_5.0                                           |
|       |                        |   13 - AV1_LEVEL_5.1                                           |
|       | --tier                 | HEVC/AV1 encoder tier [0]                                      |
|       |                        |   0 - Main tier                                                |
|       |                        |   1 - High tier                                                |
|       | --bitDepthLuma         | Bit depth of luma samples in encoded stream [8]                |
|       |                        |   8=8 bit, 10=10 bit                                           |
|       | --bitDepthChroma       |   bit depth of chroma samples in encoded stream [8]            |
|       |                        |   8=8 bit, 10=10 bit (Must be same as bitDepthLuma now.)       |
| -N[n] | --byteStream           |   Stream type. [1]                                             |
|       |                        |   0 - NAL units. NAL sizes are returned in file <nal_sizes.txt>|
|       |                        |   1 - byte stream according to HEVC Standard Annex B.          |
|       |                        |   NOTE : for AV1, only support byte stream type.               |
|       | --ivf                  |   IVF encapsulation, for AV1 only. [1]                         |
|       |                        |   0 - OBU raw output                                           |
|       |                        |   1 - IVF output (default)                                     |
| -k[n] | --videoRange           |   0..1 Video signal sample range in encoded stream. [0]        |
|       |                        |   0 - Y range in [16..235]; Cb, Cr in [16..240]                |
|       |                        |   1 - Y, Cb, Cr range in [0..255]                              |
|       | --streamBufChain       | Enable two output stream buffers. [0]                          |
|       |                        |   0 - Single output stream buffer.                             |
|       |                        |   1 - Two output stream buffers chained together.              |
|       |                        |   Note: the minimum allowable size of the first stream buffer  |
|       |                        |         is 11k bytes.                                          |
|       | --sendAUD              | 0..1 Enable AUD switch [0]                                     |
|       |                        |   0=Disabled; 1=Enable.                                        |
| -S[n] | --sei                  |   Enable SEI messages (buffering period + picture timing) [0]  |
|       |                        | Writes Supplemental Enhancement Information messages           |
|       |                        | containing information about picture time stamps               |
|       |                        | and picture buffering into stream.                             |
| -z[s] | --userData             | SEI User data file name. File is read and inserted as an SEI   |
|       |                        |   message before the first frame.                              |
|       | --extSEI               | External SEI data file name. File is read and inserted as SEI  |
|       |                        |   messages for each frame.                                     |
|       | --t35=a:b              | See Annex A and B of ITU-T T.35, two bytes [0:0]               |
|       |                        |   a: the country code                                          |
|       |                        |   b: the area code                                             |
|       | --payloadT35File       | A file name, contains the t35_payload_bytes with binary byte   |
|       | --codecFormat          | Select Video Codec Format  [0]                                 |
|       |                        |   0: HEVC video format encoding                                |
|       |                        |   1: H.264 video format encoding                               |
|       |                        |   2: AV1 video format encoding                                 |
|       |                        | or by codec name:                                              |
|       |                        |   hevc: HEVC video format encoding                             |
|       |                        |   h264: H.264 video format encoding                            |
|       |                        |   av1: AV1 video format encoding                               |
| -p[n] | --cabacInitFlag        | 0..1 Initialization value for CABAC. [0]                       |
|       |                        |   (obsolete, only support 0 now.)                                          |
| -K[n] | --enableCabac          | 0=OFF (CAVLC), 1=ON (CABAC). [1]                               |
| -e[n] | --sliceSize            | [0..height/ctu_size] slice size in number of CTU rows [0]      |
|       |                        |   0 to encode each picture in one slice                        |
|       |                        |   [1..height/ctu_size] to encode each slice with N CTU rows    |
|       |                        | NOTE : for AV1, not supported.                                 |
| -D[n] | --disableDeblocking    |   0..1 Disable deblocking filter [0]                           |
|       |                        |   0 = Inloop deblocking filter enabled (better quality)        |
|       |                        |   1 = Inloop deblocking filter disabled                        |
| -M[n] | --enableSao            |   0..1 Disable or enable SAO/CDEF filter [1]                   |
|       |                        | HEVC:                                                          |
|       |                        |   0 = SAO disabled                                             |
|       |                        |   1 = SAO enabled                                              |
|       |                        | AV1:                                                           |
|       |                        |   0 = CDEF disabled                                            |
|       |                        |   1 = CDEF enabled                                             |
| -W[n] | --tc_Offset            | -6..6 Controls deblocking filter parameter. [0]                |
|       |                        |    for H.264, it is alpha_c0_offset, for HEVC, it is tc_offset |
| -E[n] | --beta_Offset          | -6..6 Deblocking filter beta_offset (H.264 and HEVC). [0]      |
|       | --enableDeblockOverride| 0..1 Deblocking override enable flag [0]                       |
|       |                        |   0 = Disable override                                         |
|       |                        |   1 = Enable override                                          |
|       | --deblockOverride      | 0..1 Deblocking override flag [0]                              |
|       |                        |   0 = Do not override filter parameters                        |
|       |                        |   1 = Override filter parameters                               |
|       | --tile=a:b:c           | HEVC Tile setting: [1:1:1]                                     |
|       |                        |   Tile is enabled when num_tile_columns * num_tile_rows > 1.   |
|       |                        |   a: num_tile_columns                                          |
|       |                        |   b: num_tile_rows                                             |
|       |                        |   c: loop_filter_across_tiles_enabled_flag                     |
|       | --enableScalingList    | 0..1 Use average or default scaling list for HEVC. [0]         |
|       |                        |   0 = Use average scaling list.                                |
|       |                        |   1 = Use default scaling list.                                |
|       | --RPSInSliceHeader     | 0..1 Encoding RPS in the slice header for HEVC. [0]            |
|       |                        |   0 = not encoding RPS in the slice header.                    |
|       |                        |   1 = encoding RPS in the slice header.                        |
|       | --resendParamSet       | 0..1 Resend parameter sets(VPS,PPS,SPS) before IDR frames [0]  |
|       |                        |   0 = not resend parameter sets                                |
|       |                        |   1 = resend parameter sets before IDR frames                  |
|       | --POCConfig=a:b:c      | POC type/number of bits settings.                              |
|       |                        |   a: picOrderCntType. [0]                                      |
|       |                        |   b: log2MaxPicOrderCntLsb.[16]                                |
|       |                        |   c: log2MaxFrameNum.[12]                                      |
|       | --psyFactor            | 0..4.0 Strength of psycho-visual encoding. <float> [0]         |
|       |                        |   0 = disable psy-rd.                                          |
|       | --layerInRefIdc        | 0..1 Enable/Disable h264 2bit nal_ref_idc [0]                  |
|       |                        |   0 = nal_ref_idc will be 0 or 1.                              |
|       |                        |   1 = nal_ref_idc will be 0 or 3.                              |
|       | --enableTMVP           | 0..1 Enable/Disable temporal mvp.                              |
|       |                        |   Default: 1 for VP9 or HEVC/AV1 with TMVP supported; else 0.  |
|       |                        |   0 = not use temporal mvp in inter prediction                 |
|       |                        |   1 = use temporal mvp in inter prediction                     |

#### Parameters Setting Constant Chroma

| Short | Long Option            | Description                                                    |
|-------|------------------------|----------------------------------------------------------------|
|       | --enableConstChroma    | 0..1 control to encode chroma as a constant pixel value [0]    |
|       |                        |  0 = Disable.                                                  |
|       |                        |  1 = Enable.                                                   |
|       | --constCb              | The constant pixel value for Cb.                               |
|       |                        |  0..255 for 8-bit [128]; 0..1023 for 10-bit [512].             |
|       | --constCr              | The constant pixel value for Cr.                               |
|       |                        |  0..255 for 8-bit [128]; 0..1023 for 10-bit [512].             |

### Parameters Affecting GOP Pattern

| Short | Long Option            | Description                                                    |
|-------|------------------------|----------------------------------------------------------------|
|       | --smoothPsnrInGOP      | [0,1] disalbe or enable Smooth PSNR for frames in one GOP. [0] |
|       |                        |   0 - disable, 1 - enable                                      |
|       |                        |   Only affects when gopSize > 1.                               |
|       | --gopSize              | 0..8 GOP Structure Size. [0]                                   |
|       |                        |     0  : select the GOP Structure size adaptively;             |
|       |                        |   1..8 : fixed GOP Structure size, will be selected, internal  |
|       |                        |          pre-defined GOP structure will be selected;           |
|       | --gopConfig            |   User-specified GOP configuration file, which defines the GOP |
|       |                        |   structure table.                                             |
|       |                        | Short Term Structure:                                          |
|       |                        | FrmN Type POC QPoffset QPfactor TId num_refs refs used_by_cur  |
|       |                        |  -FrmN: Normal GOP config. The table should contain gopSize    |
|       |                        |        lines, named Frame1, Frame2, etc. The frames are listed |
|       |                        |        in decoding order.                                      |
|       |                        |  -Type: Slice type, can be either P or B.                      |
|       |                        |  -POC: Display order of the frame within a GOP, ranging from 1 |
|       |                        |        to gopSize.                                             |
|       |                        |  -QPoffset: It is added to the QP parameter to set the final QP|
|       |                        |        value used for this fram                                |
|       |                        |  -QPfactor: Weight used during rate distortion optimization.   |
|       |                        |  -TId: temporal layer ID.                                      |
|       |                        |  -num_refs: The number of reference pictures kept for this     |
|       |                        |        frame,including references for current and future       |
|       |                        |        pictures.                                               |
|       |                        |  -refs: A List of num_ref_pics integers, specifying the        |
|       |                        |        delta_POC of the reference pictures kept, relative to   |
|       |                        |        the POC of the current frame or LTR's index.            |
|       |                        |  -used_by_cur: A List of num_ref_pics binaries, specifying     |
|       |                        |        whether the corresponding reference picture is used by  |
|       |                        |        current picture.                                        |
|       |                        | Long Term Structure:                                           |
|       |                        | Frame0 Type QPoffset QPfactor TId num_refs refs used_by_cur \  |
|       |                        |       LTR Offset Interval                                      |
|       |                        |  -Frame0: Special GOP config. The table should contain gopSize |
|       |                        |        lines. The frames are listed in decoding order.         |
|       |                        |  -Type: Slice type. If the value is not reserved, it overrides |
|       |                        |        the value in normal config for special frame. [-255]    |
|       |                        |  -QPoffset: If the value is not reserved, it overrides the     |
|       |                        |        value in normal config for special frame. [-255]        |
|       |                        |  -QPfactor: If the value is not reserved,it overrides the      |
|       |                        |        value in normal config for special frame. [-255]        |
|       |                        |  -TId: temporal layer ID. If the value is not reserved, it     |
|       |                        |        overrides the value in normal config for special frame. |
|       |                        |        [-255]                                                  |
|       |                        |  -num_refs: The number of reference pictures kept for this     |
|       |                        |        frame including references for current and future       |
|       |                        |        pictures.                                               |
|       |                        |  -refs: A List of num_ref_pics integers, specifying the delta  |
|       |                        |        POC of the reference pictures kept, relative to the POC |
|       |                        |        of the current frame or LTR's index.                    |
|       |                        |  -used_by_cur: A List of num_ref_pics binaries, specifying     |
|       |                        |        whether the corresponding reference picture is used by  |
|       |                        |        current picture.                                        |
|       |                        |  -LTR: 0..VCENC_MAX_LT_REF_FRAMES. long term reference setting,|
|       |                        |           0 : common frame,                                    |
|       |                        |     1..VCENC_MAX_LT_REF_FRAMES : long term reference's index   |
|       |                        |  -Offset: If LTR == 0, it is (POC_Frame_Use_LTR(0) - POC_LTR). |
|       |                        |        Where POC_LTR is LTR's POC, POC_Frame_Use_LTR(0) is the |
|       |                        |        first frame after LTR that uses the LTR as reference.   |
|       |                        |           If LTR != 0, it is the offset of the first LTR frame |
|       |                        |        to the first encoded frame.                             |
|       |                        |  -Interval: If LTR == 0, it is the POC delta between two       |
|       |                        |         consecutive frames that use the same LTR as reference. |
|       |                        |         interval=POC_Frame_Use_LTR(n+1) - POC_Frame_Use_LTR(n) |
|       |                        |             If LTR != 0, it is the POC delta between two       |
|       |                        |         consecutive frames that encoded as the same LTR index. |
|       |                        |         interval = POC_LTR(n+1) - POC_LTR(n),                  |
|       |                        |         where the POC_LTR is LTR's POC;                        |
|       |                        |  NOTE: If this config file is not specified, default configure |
|       |                        |        Parameters will be used.                                |
|       | --gopLowdelay          | 0..1 Use default low delay GOP configuration if "--gopConfig"  |
|       |                        | is not specified,only valid for gopSize <= 4. [0]              |
|       |                        |   0 - Disable gopLowdelay mode                                 |
|       |                        |   1 - Enable gopLowdelay mode                                  |
| -V[n] | --bFrameQpDelta        | -1..51, B Frame QP Delta. [-1]                                 |
|       |                        |   It defines the QP difference between B Frame QP and target   |
|       |                        |   QP. If a GOP config file is specified by "--gopConfig", it   |
|       |                        |    willbe overridden.                                          |
|       |                        |    -1 - not take effect, 0-51 - could take effect.             |

### Coding Tools for Quality Control

#### Parameters Affecting Coding and Performance

| Short | Long Option            | Description                                                    |
|-------|------------------------|----------------------------------------------------------------|
|       | --preset               | 0...4 for HEVC. 0..1 for H264. Trade off performance and       |
|       |                        |  compression efficiency. Higher value means high quality but   |
|       |                        |  worse performance. User need explict claim preset when use    |
|       |                        |  this option                                                   |
|       | --rdoLevel             | 1..3 Programable hardware RDO Level [3]                        |
|       |                        |  Lower value means lower quality but better performance.       |
|       |                        |  Higher value means higher quality but worse performance.      |
|       | --enableDynamicRdo     | 0..1 Enable dynamic rdo enable flag [0]                        |
|       |                        |  0 = Disable dynamic rdo                                       |
|       |                        |  1 = Enable dynamic rdo                                        |
|       | --dynamicRdoCu16Bias   | 0..255 Programable hardware dynamic rdo cu16 bias [3]          |
|       | --dynamicRdoCu16Factor | 0..255 Programable hardware dynamic rdo cu16 factor [80]       |
|       | --dynamicRdoCu32Bias   | 0..255 Programable hardware dynamic rdo cu32 bias [2]          |
|       | --dynamicRdoCu32Factor | 0..255 Programable hardware dynamic rdo cu32 factor [32]       |
|       | --enableRdoQuant       | 0..1 Enable/Disable RDO Quantization.                          |
|       |                        | 0 = Disable (Default if hardware does not support RDOQ).       |
|       |                        | 1 = Enable (Default if hardware supports RDOQ).                |

### VUI and SEI

#### Parameters Affecting VUI

| Short | Long Option            | Description                                                    |
|-------|------------------------|----------------------------------------------------------------|
|       | --vuiColordescription  |  Color description in the vui.                                 |
|       |  [=primary:transfer    | primary : 0..9 Index of chromaticity coordinates in Table E.3  |
|       |    :matrix]            |   in HEVC spec [9]                                             |
|       |                        | transfer: 0..2 The reference opto-electronic transfer          |
|       |                        |       characteristic function of the source picture in Table   |
|       |                        |       E.4 in HEVC spec [0]                                     |
|       |                        |        0 = ITU-R BT.2020, 1 = SMPTE ST 2084, 2 = ARIB STD-B67  |
|       |                        | matrix  : 0..9 Index of matrix coefficients used in deriving   |
|       |                        |       luma and chroma signals from the green, blue, and red or |
|       |                        |       Y, Z, and X primaries in Table E.5 in HEVC spec [9]      |
|       | --vuiVideoFormat       | 0..5 video_format in the vui.[5]                               |
|       |                        |  0 - component                                                 |
|       |                        |  1 - PAL                                                       |
|       |                        |  2 - NTSC                                                      |
|       |                        |  3 - SECAM                                                     |
|       |                        |  4 - MAC                                                       |
|       |                        |  5 - UNDEF                                                     |
|       | --vuiVideosignalPresent | 0..1 video signal type Present in the vui.[0]                 |
|       |                        |   0 - video signal type NOT Present in the vui                 |
|       |                        |   1 - video signal type Present in the vui                     |
|       | --vuiAspectRatiot      | aspect ration in the vui                                       |
|       |  [=aspectratioWidth    |  aspectratioWidth : 0...65535 Horizontal size of the sample    |
|       |   :aspectratioHeigh]   |   aspect ratio (in arbitrary units), 0 for unspecified         |
|       |                        | aspectratioHeight : 0...65535 Vertical size of the sample      |
|       |                        |   aspect ratio (in same units as sampleAspectRatioWidth), 0 for|
|       |                        |    unspecified value.                                          |

#### Parameters Affecting HDR10

| Short | Long Option            | Description                                                    |
|-------|------------------------|----------------------------------------------------------------|
|       | --writeOnceHDR10       | 0 - write before each IDR, 1 - only write once in StrmStart [0]|
|       | --HDR10_display        | Mastering display color volume SEI message                     |
|       |  [=dx0:dy0:dx1:dy1:    | dx0 : 0...50000 Component 0 normalized x chromaticity          |
|       |   dx2:dy2:wx:wy:       |       coordinates [0]                                          |
|       |   max:min]             | dy0 : 0...50000 Component 0 normalized y chromaticity          |
|       |                        |       coordinates [0]                                          |
|       |                        | dx1 : 0...50000 Component 1 normalized x chromaticity          |
|       |                        |       coordinates [0]                                          |
|       |                        | dy1 : 0...50000 Component 1 normalized y chromaticity          |
|       |                        |       coordinates [0]                                          |
|       |                        | dx2 : 0...50000 Component 2 normalized x chromaticity          |
|       |                        |       coordinates [0]                                          |
|       |                        | dy2 : 0...50000 Component 2 normalized y chromaticity          |
|       |                        |       coordinates [0]                                          |
|       |                        | wx  : 0...50000 White point normalized x chromaticity          |
|       |                        |       coordinates [0]                                          |
|       |                        | wy  : 0...50000 White point normalized y chromaticity          |
|       |                        |       coordinates [0]                                          |
|       |                        | max : Nominal maximum display luminance [0]                    |
|       |                        | min : Nominal minimum display luminance [0]                    |
|       | --HDR10_lightlevel     | Content light level info SEI message                           |
|       |  [=maxlevel:avglevel]  |  maxlevel : max content light level                            |
|       |                        |  avglevel : max picture average light level                    |

### Parameters Affecting Noise Reduction: (obsolete)

| Short | Long Option            | Description                                                    |
|-------|------------------------|----------------------------------------------------------------|
|       | --noiseReductionEnable |   (obsolete) Enable/disable noise reduction (NR). [0]          |
|       |                        |  0 = Disable NR.                                               |
|       |                        |  1 = Enable NR.                                                |
|       | --noiseLow             |   (obsolete) 1..30 minimum noise value [10]                    |
|       | --noiseFirstFrameSigma |   (obsolete) 1..30 noise estimation for start frames [11]      |

### Parameters Affecting Smart Background Detection: (obsolete)

| Short | Long Option            | Description                                                    |
|-------|------------------------|----------------------------------------------------------------|
|       | --smartConfig          |   (obsolete) Usr configuration file for the Smart Algorithm.   |

### Parameters for Disable EndOfSequence

| Short | Long Option   | Description                                                             |
| ----- | -----------   | ----------------------------------------------------------------------- |
|       | --disableEOS  | disableEOS=1, EOS bytes not be written before stream closed. [0]        |
|       |               | disableEOS=0, EOS bytes be written normally before stream closed.       |

### Parameters Affecting AV1's Verification When Using ffmpeg Framework, NOT Using When Encoding

| Short | Long Option              | Description                                                  |
| ----- | ------------------------ | -------------------------------------------------------------|
|       | --modifiedTileGroupSize  | modified space size of OBU_TILE_GROUP which coded by HW. [0] |
|       |                          | 0 = not modified.                                            |
|       |                          | 1 = modified.                                                |

### Parameters Affecting the Process of Endcoding

| Short | Long Option | Description                                                               |
| ----- | ----------- | ------------------------------------------------------------------------- |
|       | --reEncode  | encode frame again, like get output buffer overflow from hw, we can use   |
|       |             | app's callback to reallocate the output buffer, then use same regs with   |
|       |             | different output buffer to encode the current frame again. Currently, for |
|       |             | non-vcmd, support single-core and multi-core, for vcmd, support single    |
|       |             | core, but for multi-core, just support single stream, that is means the   |
|       |             | multi stream with multi-core don't be support. [0]                        |
|       |             | 0 = Disable reEncode.                                                     |
|       |             | 1 = Enable reEncode.                                                      |

### Parameters Affecting Motion Estimation and Global Motion

| Short | Long Option            | Description                                                    |
|-------|------------------------|----------------------------------------------------------------|
|       | --gmvFile              | File containing ME Search Range Offset for list0.              |
|       |                        |   Search Offsets of each frame are listed sequentially line by |
|       |                        |   line. To set frame-level offset, there should be only one    |
|       |                        |   coordinate per line for the whole frame. For example:        |
|       |                        |     (MVX, MVY)                                                 |
|       |                        | When SEARCH_RANGE_ROW_OFFSET_TEST is enable,                   |
|       |                        |   To set CTU-row level offsets, there should be multiple       |
|       |                        |   coordinates per line for all CTU rows. For example:          |
|       |                        |     (MVX0, MVY0) (MVX1, MVY1) ...                              |
|       |                        |   Some common delimiters are allowed between offsets, such as  |
|       |                        |      '()' white-space ',' ';'.                                 |
|       | --gmv                  | MVX:MVY. Horizontal and vertical offsets of ME Search Range    |
|       |                        |  for list0 at frame level. [0:0]                               |
|       |                        |  If both "--gmvFile" and "--gmv" are specified, only gmvFile   |
|       |                        |  will be applied.                                              |
|       |                        | MVX should be 64-aligned and within the range of [-128, +128]. |
|       |                        | MVY should be 16-aligned and within the range of [-128, +128]. |
|       | --gmvList1             | MVX:MVY. ME Search Range Offset for list1 at frame level. [0:0]|
|       | --gmvList1File         | File containing ME Search Range Offset for list1.              |
|       | --MEVertRange          | ME vertical search range.  [0]                                 |
|       |                        |  Should be 24,48,64 for H.264; 40,64 for HEVC/AV1.             |
|       |                        |  0 - The maximum search range of this version will be used.    |
|       |                        | Note: Only valid if the function is supported in this version. |

## Rate Control

### Parameters Setting RC Mode

| Short | Long Option            | Description                                                    |
|-------|------------------------|----------------------------------------------------------------|
|       | --rcMode               | Select Rate Control Mode  [0]/[cvbr]                           |
|       |                        |   0: CVBR Mode: Constrained VBR mode, which saves bits when    |
|       |                        |      slow-motion/simple-scene and uses the saved bits for      |
|       |                        |      complexity scene to improve the whole quality.            |
|       |                        |      CVBR mode is default rcMode, if options as '--cpbSize=x', |
|       |                        |      '--vbr=1' and '--crf=x' are set, rcMode will be altered to|
|       |                        |       other related rcModes instead of CVBR mode.              |
|       |                        |      If rcMode is set to other rcModes by user, the rcMode has |
|       |                        |      higher priority than options above, that is to say rcMode |
|       |                        |      remains unchanged and the options above don't work.       |
|       |                        |   1: CBR Mode: Constant Bit Rate Mode, which outputs constant  |
|       |                        |      bitrate to transmission path. In this mode, cpbSize is    |
|       |                        |      needed and 2x bitrate by default if not set.              |
|       |                        |      Larger cpbSize is more probably to avoid overflow.        |
|       |                        |      With option '--hrdConformance=1', it can avoid underflow. |
|       |                        |      Smaller cpbSize can make bitrate more stable but add risk |
|       |                        |      of overflow.                                              |
|       |                        |      2x bitrate is recommended and it's better not less than 1x|
|       |                        |      bitrate.                                                  |
|       |                        |   2: VBR Mode: Variable Bit Rate Mode, which is equivalent to  |
|       |                        |      option '--vbr=1'. It's strongly recommended to set qpMin  |
|       |                        |      together in this mode. qpMin=10 is a recommended value.   |
|       |                        |   3: ABR Mode: Average Bit Rate Mode, which tunes qp by gap    |
|       |                        |      between actual bitrate and target bitrate (not by scene   |
|       |                        |      complexity) to make average bitrate reach target bitrate. |
|       |                        |   4: CRF Mode: Constant Rate Factor Mode, which is to keep a   |
|       |                        |      certain level of quality based on crf value and frame     |
|       |                        |      rate, working as constant slice QP.                       |
|       |                        |      CRF is working only when look-ahead is turned on and will |
|       |                        |      disable VBR mode if both enabled.                         |
|       |                        |      In CRF mode, option '--crf' need be set.                  |
|       |                        |      If option '--crf' is set, CRF mode is set automatically.  |
|       |                        |   5: CQP Mode: Constant QP Mode, which uses constant QP for all|
|       |                        |      slices (I slice may be modified by intraQpDelta further). |
|       |                        |      In CQP mode, option '--picRc' doesn't work.               |
|       |                        | or by RC mode name:                                            |
|       |                        |   cvbr: CVBR Mode                                              |
|       |                        |   cbr : CBR Mode                                               |
|       |                        |   vbr : VBR Mode                                               |
|       |                        |   abr : ABR Mode                                               |
|       |                        |   crf : CRF Mode                                               |
|       |                        |   cqp : CQP Mode                                               |

### Rate Control and Output Stream Bitrate
| Short | Long Option            | Description                                                    |
|-------|------------------------|----------------------------------------------------------------|
| -R[n] | --intraPicRate         |   Intra-picture rate in frames. [0]                            |
|       |                        | Forces every Nth frame to be encoded as intra frame.           |
|       |                        |   0 = Do not force                                             |
| -B[n] | --bitPerSecond         | 10000..MaxByLevel, target bitrate for rate control in bps.     |
|       |                        |   default is 1000000 bps                                       |
|       | --tolMovingBitRate     | 0..2000, percent tolerance of max moving bitrate. [100]        |
|       |                        |  Use to limit the instantaneous bitrate. the Max value is,     |
|       |                        |   max_moving_bitrate = target_bitrate*(100+tolMovingBitRate)%  |
|       | --monitorFrames        | 10..120, frames count monitored for moving bitrate [frame rate]|
|       | --bitVarRangeI         | (obsolete) 10..10000, percent variations over the average bits |
|       |                        |   per frame for I frame [10000]                                |
|       | --bitVarRangeP         | 10..10000, percent variations over the average bits per frame  |
|       |                        |   for P frame [10000]                                          |
|       | --bitVarRangeB         | 10..10000, percent variations over the average bits per frame  |
|       |                        |   for B frame [10000]                                          |
|       | --staticSceneIbitPercent| 0...100 I frame bits percent of bitrate in static scene [80]  |
|       | --crf[n]               | -1..51 Constant rate factor mode,   [-1] -1 means disable.     |
|       |                        |   CRF mode is to keep a certain level of quality based on crf  |
|       |                        |   value and frame rate, working as constant slice QP.          |
|       |                        |   CRF is working only when look-ahead is turned on and will    |
|       |                        |   disable VBR mode if both enabled.                            |
| -U[n] | --picRc                | [0,1] 0=OFF, 1=ON Picture rate control. [0]                    |
|       |                        |   Calculates a new target QP for every frame.                  |
| -u[n] | --ctbRc                | 0..3 CTB QP adjustment mode for different targets. [0]         |
|       |                        |   0 = No CTB QP adjustment.                                    |
|       |                        |   1 = CTB QP adjustment for Subjective Quality only.           |
|       |                        |   2 = CTB QP adjustment for Rate Control only.                 |
|       |                        |      (valid only when HwCtbRcVersion >= 1)                     |
|       |                        |   3 = CTB QP adjustment for both Subjective Quality and Rate   |
|       |                        |       Control. (valid only when HwCtbRcVersion >= 1)           |
|       |                        |   4 = CTB QP adjustment for Subjective Quality only, reversed. |
|       |                        |   6 = CTB QP adjustment for both Subjective Quality reversed   |
|       |                        |       and Rate Control. (valid only when HwCtbRcVersion >= 1)  |
|       | --blockRCSize          | 0..2 Block size in CTB QP adjustment for Subjective Quality [0]|
|       |                        |   0=64x64 ,1=32x32, 2=16x16                                    |
|       | --rcQpDeltaRange       | Max absolute value of CU/MB QP delta relative to frame QP in   |
|       |                        |   CTB RC [10]                                                  |
|       |                        |   0..15 for HwCtbRcVersion <= 1;                               |
|       |                        |   0..51 for HwCtbRcVersion > 1.                                |
|       | --rcBaseMBComplexity   | 0..31 MB complexity base in CTB QP adjustment for Subjective   |
|       |                        |   Quality [15]                                                 |
|       |                        |  qpOffset is larger for CU/MB with larger complexity offset    |
|       |                        |    from rcBaseMBComplexity.                                    |
|       | --tolCtbRcInter        | Tolerance of CTB Rate Control for INTER frames. <float> [0.0]  |
|       |                        |   CTB RC will try to limit INTER frame bits within the range   |
|       |                        |     [targetPicSize/(1+tolCtbRcInter),                          |
|       |                        |      targetPicSize*(1+tolCtbRcInter)].                         |
|       |                        |   A negative number means no bitrate limit in CTB RC.          |
|       | --tolCtbRcIntra        | Tolerance of CTB Rate Control for INTRA frames. <float> [-1.0] |
|       | --ctbRowQpStep         | The maximum accumulated QP adjustment step per CTB Row allowed |
|       |                        | by CTB rate control.                                           |
|       |                        |   Default value is [4] for H.264 and [16] for HEVC.            |
|       |                        |   QP_step_per_CTB = MIN((ctbRowQpStep / Ctb_per_Row),4).       |
|       | --picQpDeltaRange      | Min:Max. Qp_Delta range in picture RC.                         |
|       |                        |   Min: -10..-1 Minimum Qp_Delta in picture RC. [-4]            |
|       |                        |   Max:  1..10  Maximum Qp_Delta in picture RC. [4]             |
|       |                        |   This range only applies to two neighboring frames of the same|
|       |                        |   coding type. This range doesn't apply when HRD overflow      |
|       |                        |   occurs.                                                      |
|       | --fillerData           | 0..1 Enable/Disable fill Data when HRD off, need to set cpbSize|
|       |                        |      when fillerData is 1 [0]                                  |
|       |                        |      0 - OFF, 1 - ON                                           |
| -C[n] | --hrdConformance       | 0..1 HRD conformance checking turn on or off. [0]              |
|       |                        |     0 - OFF, 1 - ON, just valid when hevc/h264                 |
|       |                        |   Uses standard defined model to limit bitrate variance.       |
| -c[n] | --cpbSize              | HRD Coded Picture Buffer size in bits. [0]                     |
|       |                        |   If hrdConformance=1, default value 0 means the max CPB for   |
|       |                        |   current level. Or default value 0 means RC doesn't consider  |
|       |                        |   cpb buffer limit.                                            |
|       | --cpbMaxRate           | Max local bitrate (bit/s). [0]                                 |
|       |                        |   If cpbSize is set but cpbMaxRate is not set or smaller than  |
|       |                        |      bitrate, cpbMaxRate is defaultly set to bitrate.          |
|       |                        |   If cpbMaxRate == bitrate, it's CBR mode,                     |
|       |                        |   if cpbMaxRate > bitrate, it's VBR mode.                      |
| -g[n] | --bitrateWindow        | 1..300, Bitrate window length in frames [intraPicRate]         |
|       |                        |   Rate control allocates bits for one window and tries to match|
|       |                        |   the target bitrate at the end of each window.                |
|       |                        |   Typically window begins with an intra frame, but this is not |
|       |                        |   mandatory.                                                   |
|       | --LTR=a:b:c[:d]        | a:b:c[:d] long term reference setting                          |
|       |                        |     a: POC_delta of two frames which are used as LTR.          |
|       |                        |     b: POC_Frame_Use_LTR(0) - POC_LTR. POC_LTR is LTR's POC,   |
|       |                        |        POC_Frame_Use_LTR(0) is the first frame after LTR that  |
|       |                        |        uses the LTR as reference.                              |
|       |                        |     c: POC_Frame_Use_LTR(n+1) - POC_Frame_Use_LTR(n). The POC  |
|       |                        |        delta between two consecutive frames that use the same  |
|       |                        |        LTR as reference.                                       |
|       |                        |     d: QP delta for frame using LTR. [0]                       |
|       | --numRefP              | 1..2 number of reference frames for P frame. [1]               |
| -s[n] | --picSkip              | 0..1 Picture skip rate control. [0]                            |
|       |                        |   0 - OFF, not allows rate control to skip frames.             |
|       |                        |   1 - ON, allows rate control to skip frames if needed.        |
| -q[n] | --qpHdr                | -1..51, Initial QP used in the beginning of stream. [26]       |
|       |                        |   -1 - Encoder calculates initial QP when picture rc is enable.|
|       |                        |   NOTE: specify as "-q-1" The initial .                        |
| -n[n] | --qpMin                | 0..51, Minimum frame header QP for any slices. [0]             |
| -m[n] | --qpMax                | 0..51, Maximum frame header QP for any slices. [51]            |
|       | --qpMinI               | 0..51, Minimum frame header QP for I slices. [0]               |
|       |                        |   Note: will overriding qpMin for I slices.                    |
|       | --qpMaxI               | 0..51, Maximum frame header QP for I slices.  [51]             |
|       |                        |   Note: will overriding qpMax for I slices.                    |
| -A[n] | --intraQpDelta         | -51..51, Intra QP delta. [-5]                                  |
|       |                        | QP difference between target QP and intra frame QP.            |
| -G[n] | --fixedIntraQp         |   0..51, Fixed Intra QP, 0 = disabled. [0]                     |
|       |                        | Use fixed QP value for every intra frame in stream.            |
| -I[n] | --chromaQpOffset       | -12..12 Chroma QP offset. [0]                                  |
|       | --vbr                  |   0=OFF, 1=ON. Variable Bitrate control by qpMin. [0]          |
|       | --sceneChange          | Frame1:Frame2:..:Frame20. Specify scene change frames.         |
|       |                        |   The frame number is separated by ':'. Max number of scene    |
|       |                        |   change frames is 20.                                         |
|       | --gdrDuration          | P frame number to refresh as a I frame in GDR. [0]             |
|       |                        |   0 : disable GDR (Gradual decoder refresh),                   |
|       |                        |   >0: enable GDR                                               |
|       |                        |  Note: The starting point of GDR is the frame with type set to |
|       |                        |   VCENC_INTRA_FRAME. intraArea and roi1Area are used to        |
|       |                        |   implement the GDR function. The GDR begin to work from the   |
|       |                        |   second IDR frame.                                            |
|       | --skipFramePOC         | 0..n Force encode whole frame as Skip Frame when POC=n. [0]    |
|       |                        |   0 : Disable.                                                 |
|       |                        |   none zero: set the frame POC to be encoded as Skip Frame.    |
|       | --insertIDR            | 0..n Force encode whole frame as IDR Frame when picture_cnt=n. |
|       |                        |   0 : Disable.                                                 |
|       |                        |   none zero: set the frame picture_cnt to be encoded as IDR    |
|       |                        |   Frame.                                                       |

### Parameters Affecting lookahead Encoding

| Short | Long Option      | Description                                                          |
| ----- | ---------------- | ---------------------------------------------------------------------|
|       | --lookaheadDepth | 0, 4..40 The number of look-ahead frames. [0]                        |
|       |                  | 0  - Disable 2-pass encoding.                                        |
|       |                  | 4-40 - Set the number of look-ahead frames and enable 2-pass         |
|       |                  | encoding.                                                            |
|       | --halfDsInput    | External provided half-downsampled input YUV file.                   |
|       | --aq_mode        | Mode for Adaptive Quantization [0]                                   |
|       |                  | 0:none                                                               |
|       |                  | 1:uniform AQ                                                         |
|       |                  | 2:auto variance                                                      |
|       |                  | 3:auto variance with bias to dark scenes.                            |
|       | --aq_strength    | 0..3.0 Strength factor in AQ mode. [1.00]                            |
|       |                  | can reduces blocking and blurring in flat and textured areas.        |
|       | --tune           | 0..4 Tune video quality for different target. [0]                    |
|       |                  | [0,psnr] - tune psnr: --aq_mode=0 --psyFactor=0                      |
|       |                  | [1,ssim] - tune ssim: --aq_mode=2 --psyFactor=0                      |
|       |                  | [2,visual] - tune visual: --aq_mode=2 --psyFactor=0.75               |
|       |                  | [3,sharpness_visual] - tune sharpness visual:  --aq_mode=2           |
|       |                  | --psyFactor=0.75  --inLoopDSRatio=0  --enableRdoQuant=0              |
|       |                  | [4,vmaf] = tune vmaf: enable vmaf preprocessing                      |
|       | --inLoopDSRatio  | 0..1 in-loop downsample ratio for first pass. [1]                    |
|       |                  | 0 - no downsample, 1 - 1/2 downsample.                               |

## ROI

### Parameters Affecting Coding

| Short | Long Option            | Description                                                    |
|-------|------------------------|----------------------------------------------------------------|
|       | --cir                  | start:interval for Cyclic Intra Refresh.                       |
|       |                        |   Forces CTBs in intra mode. [0:0]                             |
|       | --intraArea            | left:top:right:bottom CTB coordinates                          |
|       |                        |   Specifies the rectangular area of CTBs to force encoding in  |
|       |                        |   intra mode.                                                  |
|       | --ipcm1Area            | left:top:right:bottom CTB coordinates                          |
|       | --ipcm2Area            | left:top:right:bottom CTB coordinates                          |
|       | --ipcm3Area            | left:top:right:bottom CTB coordinates                          |
|       | --ipcm4Area            | left:top:right:bottom CTB coordinates                          |
|       | --ipcm5Area            | left:top:right:bottom CTB coordinates                          |
|       | --ipcm6Area            | left:top:right:bottom CTB coordinates                          |
|       | --ipcm7Area            | left:top:right:bottom CTB coordinates                          |
|       | --ipcm8Area            | left:top:right:bottom CTB coordinates                          |
|       |                        |   Specifies the rectangular area of CTBs to force encoding in  |
|       |                        |   IPCM mode.                                                   |
|       | --ipcmMapEnable        | Enable the IPCM Map. 0-disable, 1-enable. [0]                  |
|       | --ipcmMapFile          | User-specified IPCM map file, which defines the IPCM flag for  |
|       |                        |   each CTB in the frame. The block size is 64x64 for HEVC and  |
|       |                        |   16x16 for H.264.                                             |
|       | --roi1Area             | left:top:right:bottom CTB coordinates                          |
|       | --roi2Area             | left:top:right:bottom CTB coordinates                          |
|       | --roi3Area             | left:top:right:bottom CTB coordinates                          |
|       | --roi4Area             | left:top:right:bottom CTB coordinates                          |
|       | --roi5Area             | left:top:right:bottom CTB coordinates                          |
|       | --roi6Area             | left:top:right:bottom CTB coordinates                          |
|       | --roi7Area             | left:top:right:bottom CTB coordinates                          |
|       | --roi8Area             | left:top:right:bottom CTB coordinates                          |
|       |                        |   Specifies the rectangular area of CTBs as Region Of Interest |
|       |                        |   with lower QP. the CTBs in the edges are all inclusive.      |
|       | --roi1DeltaQp          | -30..0 (-51..51 if absolute ROI QP supported), QP delta value  |
|       |                        |   for ROI 1 CTBs. [0]                                          |
|       | --roi2DeltaQp          | -30..0 (-51..51 if absolute ROI QP supported), QP delta value  |
|       |                        |   for ROI 2 CTBs. [0]                                          |
|       | --roi3DeltaQp          | -30..0 (-51..51 if absolute ROI QP supported), QP delta value  |
|       |                        |   for ROI 3 CTBs. [0]                                          |
|       | --roi4DeltaQp          | -30..0 (-51..51 if absolute ROI QP supported), QP delta value  |
|       |                        |   for ROI 4 CTBs. [0]                                          |
|       | --roi5DeltaQp          | -30..0 (-51..51 if absolute ROI QP supported), QP delta value  |
|       |                        |   for ROI 5 CTBs. [0]                                          |
|       | --roi6DeltaQp          | -30..0 (-51..51 if absolute ROI QP supported), QP delta value  |
|       |                        |   for ROI 6 CTBs. [0]                                          |
|       | --roi7DeltaQp          | -30..0 (-51..51 if absolute ROI QP supported), QP delta value  |
|       |                        |   for ROI 7 CTBs. [0]                                          |
|       | --roi8DeltaQp          | -30..0 (-51..51 if absolute ROI QP supported), QP delta value  |
|       |                        |   for ROI 8 CTBs. [0]                                          |
|       | --roi1Qp               | 0..51, absolute QP value for ROI 1 CTBs. [-1]. Negative value  |
|       |                        |   means invalid. other value is used to calculate the QP in the|
|       |                        |   ROI area.                                                    |
|       | --roi2Qp               | 0..51, absolute QP value for ROI 2 CTBs. [-1]                  |
|       | --roi3Qp               | 0..51, absolute QP value for ROI 3 CTBs. [-1]                  |
|       | --roi4Qp               | 0..51, absolute QP value for ROI 4 CTBs. [-1]                  |
|       | --roi5Qp               | 0..51, absolute QP value for ROI 5 CTBs. [-1]                  |
|       | --roi6Qp               | 0..51, absolute QP value for ROI 6 CTBs. [-1]                  |
|       | --roi7Qp               | 0..51, absolute QP value for ROI 7 CTBs. [-1]                  |
|       | --roi8Qp               | 0..51, absolute QP value for ROI 8 CTBs. [-1]                  |
|       |                        |   roi1Qp..roi8Qp are only valid when absolute ROI QP is        |
|       |                        |   supported. Use either roiDeltaQp or roiQp.                   |
|       | --roiMapDeltaQpEnable  | Enable the QP delta for ROI regions in the frame. [0]          |
|       |                        |   The block size is defined by "--roiMapDeltaQpBlockUnit". and |
|       |                        |   the QP values are defined by "--roiMapDeltaQpFile" in text   |
|       |                        |   format or "-roiMapInfoBinFile" in binary format. The Map size|
|       |                        |   should be calculated according to the picture width and      |
|       |                        |   height after aligned with 64 pixels.                         |
|       |                        |    0 = disable                                                 |
|       |                        |    1 = enable.                                                 |
|       | --roiMapDeltaQpBlockUnit| Set the DeltaQp block size for ROI DeltaQp map file.  [0]     |
|       |                        |   0-64x64,1-32x32,2-16x16,3-8x8.                               |
|       |                        |   Note: 8x8 is not supported for H264.                         |
|       | --roiMapDeltaQpFile    | User-specified Qp map file in text format, which defines the   |
|       |                        |   delta QP or absolute QP value for each block in the frame.   |
|       |                        |   When absolute ROI QP is not supported,                       |
|       |                        |     - The QP delta range is from -30 to 0;                     |
|       |                        |   When absolute ROI QP is supported,                           |
|       |                        |     - The QP delta range is from -51 to 51 if                  |
|       |                        |     - Absolute QP range is from 0 to 51 and specified with a   |
|       |                        |       prefix 'a', for example, 'a26'.                          |
|       | --roiMapInfoBinFile    | User-specified ROI map info binary file, which defines the QP  |
|       |                        |   delta or absolute ROI QP for each block in the frame.        |
|       |                        |   The block size is defined by "--roiMapDeltaQpBlockUnit", one |
|       |                        |   byte per block. The byte bitmap of QpDelta/Skip/IPCM/Rdoq is |
|       |                        |   related to "--RoiQpDeltaVer".                                |
|       |                        |   when RoiQpDeltaVer = 1/2/3, both Delta Qp and absolute Qp    |
|       |                        |    supported. For delta QP, the range is from -51 to 51. For   |
|       |                        |    absolute QP, the range is from 0 to 51.                     |
|       |                        |   RoiQpDeltaVer = 4, only deltaQp supported:                   |
|       |                        |     - QP delta range is from -32 to 31.                        |
|       | --RoiQpDeltaVer        | 1..4, ROI Map format number.                                   |
|       |                        |   Determine the format used in roiMapInfoBinFile.  see         |
|       |                        |   "Hantro.VC9000E.Memory.Buffer.and.Format.Organization" for   |
|       |                        |   the detail format description for different formats.         |
|       | --RoimapCuCtrlInfoBinFile | User-specified ROI map CU control info binary file, which   |
|       |                        | defines the ROI CU control info for each CU in the frame. When |
|       |                        | ROI Map is defined and enableat the same time, the QP part will|
|       |                        | be ignored for ROI Map has higherpriority.                     |
|       |                        |  - The CU control info size is related to "--RoiCuCtrlVer".    |
|       |                        |  - The number of controlled blocks is aligned to 64 of picture |
|       |                        |       width and height in 8x8 unit.                            |
|       | --RoiCuCtrlVer         | 3..7, ROI map CU control format number.                        |
|       |                        |   Determine the format used in RoimapCuCtrlInfoBinFile. see    |
|       |                        |   "Hantro.VC9000E.Memory.Buffer.and.Format.Organization" for   |
|       |                        |   the detail format description for different formats.         |

### Parameters Setting Skip Map

| Short | Long Option            | Description                                                    |
|-------|------------------------|----------------------------------------------------------------|
|       | --skipMapEnable        | 0..1 Enable Skip Map Mode. 0-Disable, 1-Enable. [0]            |
|       |                        |   when fuse sw_enc_HWRoiMapVersion is set to 2:                |
|       |                        |     when HW_ID_MAJOR >= 0x82 or (HW_ID_MAJOR == 0x60 &&        |
|       |                        |      HW_ID_MINOR>=0x010), Need to specify "--RoiQpDeltaVer" as |
|       |                        |      2 to enable.                                              |
|       |                        |     otherwise NOT need to specify "--RoiQpDeltaVer".           |
|       |                        |   when fuse sw_enc_HWRoiMapVersion is set to 3, Need to specify|
|       |                        |    "RoiQpDeltaVer" as 2 to enable.                             |
|       | --skipMapBlockUnit     | Block size in Skip Map Mode. [0]                               |
|       |                        |   0-64x64, 1-32x32, 2-16x16.                                   |
|       |                        |   Note: Only 64x64 and 32x32 are valid for HEVC.               |
|       | --skipMapFile          | User-specified Skip Map File, which defines if each block is   |
|       |                        |  forcely encoded as the Skip mode or not. 1 indicates the block|
|       |                        |  will be encoded as skip mode forcely. and 0 indicates not. To |
|       |                        |  calculate block width and height, picture width and height    |
|       |                        |  after alignment to 64 pixels should be used.                  |

### Parameters Setting Rdoq Map

| Short | Long Option            | Description                                                    |
|-------|------------------------|----------------------------------------------------------------|
|       | --rdoqMapEnable        |   0..1 Enable Rdoq Map Mode. 0-Disable, 1-Enable. [0]          |

## Coding Status

### Parameters Affecting Reporting

| Short | Long Option            | Description                                                    |
|-------|------------------------|----------------------------------------------------------------|
|       | --enableOutputCuInfo   | 0..2 Enable/Disable writting CU encoding information out [0]   |
|       |                        |  0 = Disable.                                                  |
|       |                        |  1 = Enable.                                                   |
|       |                        |  2 = Enable and also write information in file <cuInfo.txt>.   |
|       | --cuInfoVersion        | 0..2 Defines cuInfo format. Only affect pass 2 if lookahead is |
|       |                        |   used. [-1]                                                   |
|       |                        |  -1 = format auto decided by VC9000E and HW support.           |
|       |                        |   0 - format 0; Only available with HW cuInfo Version 0        |
|       |                        |   1 - format 1; Available with HW cuInfo Version 1 or 2        |
|       |                        |   2 - format 2; Only available with HW cuInfo Version 2        |
|       |                        | For lookahead pass 1, format 2 will output as IM input;        |
|       |                        |   Software cutree only support cuInfo format 1.                |
|       | --enableOutputCtbBits  | 0..2 Enable/Disable writting CTB encoded bits to DDR [0]       |
|       |                        |   0- Disable.                                                  |
|       |                        |   1- Enable. 2 bytes per CTB. Raster Scan CTBs                 |
|       |                        |   2- Enable and also write information in file <ctbBits.txt>.  |
|       | --hashtype             |   hash type for frame data hash. [0]                           |
|       |                        |   0=disable, 1=crc32, 2=checksum32.                            |
|       | --ssim                 |   0..1 Enable/Disable SSIM calculation [1]                     |
|       |                        | 0 - Disable.                                                   |
|       |                        | 1 - Enable.                                                    |
|       | --psnr                 |   0..1 Enable/Disable PSNR calculation [1]                     |
|       |                        | 0 = Disable.                                                   |
|       |                        | 1 = Enable.                                                    |
|       | --enableVuiTimingInfo  |    Write VUI timing info in SPS. [1]                           |
|       |                        |  0=disable.                                                    |
|       |                        |  1=enable.                                                     |

## Peripherals & Hardware Setup

### Parameters Affecting Reference Frame Compressor

| Short | Long Option            | Description                                                    |
|-------|------------------------|----------------------------------------------------------------|
|       | --compressor           |   0..3 Enable/Disable Embedded Compression [0]                 |
|       |                        |   0 = Disable compression                                      |
|       |                        |   1 = Only enable luma compression                             |
|       |                        |   2 = Only enable chroma compression                           |
|       |                        |   3 = Enable both luma and chroma compression                  |
|       | --enableP010Ref        |   0..1 Enable/Disable P010 reference format [0]                |
|       |                        |   0 = Disable.                                                 |
|       |                        |   1 = Enable.                                                  |

### OSD & Mosaic

#### Parameters for OSD Overlay Controls

Supporting max 12 OSD regions (i=01,02..12). Any two OSD regions should not share same CTB.

| Short | Long Option            | Description                                                    |
|-------|------------------------|----------------------------------------------------------------|
|       | --overlayEnables       | 8 bits indicate enable for 8 overlay region. [0]               |
|       |                        |   1: region 1 enabled                                          |
|       |                        |   2: region 2 enabled                                          |
|       |                        |   3: region 1 and 2 enabled                                    |
|       |                        |   and so on.                                                   |
|       | --olInputi             | input file for overlay region 1-8. [olInputi.yuv]              |
|       |                        |   for example --olInput1                                       |
|       | --olFormati            | 0..2 Specify the overlay input format. [0]                     |
|       |                        |   0: ARGB8888                                                  |
|       |                        |   1: NV12                                                      |
|       |                        |   2: Bitmap                                                    |
|       | --olAlphai             | 0..255 Specify the global alpha value for NV12 and Bitmap      |
|       |                        |   overlay format. [0]                                          |
|       | --olWidthi             | Width of overlay region. Must be set if region enabled. [0]    |
|       |                        |   Must be under 8 aligned for bitmap format.                   |
|       | --olHeighti            | Height of overlay region. Must be set if region enabled. [0]   |
|       | --olXoffseti           | Horizontal offset of overlay region top left pixel. [0]        |
|       |                        |   must be under 2 aligned condition. [0]                       |
|       | --olYoffseti           | Vertical offset of overlay region top left pixel. [0]          |
|       |                        |   must be under 2 aligned condition. [0]                       |
|       | --olYStridei           | Luma stride in bytes. Default value is based on format.        |
|       |                        |   [olWidthi * 4] if ARGB8888.                                  |
|       |                        |   [olWidthi] if NV12.                                          |
|       |                        |   [olWidthi / 8] if Bitmap.                                    |
|       | --olUVStridei          | Chroma stride in bytes. Default value is based on luma stride. |
|       | --olCropXoffseti       | OSD cropping top left horizontal offset. [0]                   |
|       |                        |   must be under 2 aligned condition.                           |
|       |                        |   for bitmap format, must be under 8 aligned condition.        |
|       | --olCropYoffseti       | OSD cropping top left vertical offset. [0]                     |
|       |                        |   must be under 2 aligned condition.                           |
|       |                        |   must be under 8 aligned condition for bitmap format.         |
|       | --olCropWidthi         | OSD cropping width. [olWidthi]                                 |
|       |                        |   for bitmap format, must be under 8 aligned condition.        |
|       | --olCropHeighti        | OSD cropping height. [olHeighti]                               |
|       | --olBitmapYi           | OSD bitmap format Y value. [0]                                 |
|       | --olBitmapUi           | OSD bitmap format U value. [0]                                 |
|       | --olBitmapVi           | OSD bitmap format V value. [0]                                 |

#### Parameters for Mosaic Area Controls

Supporting max 12 Mosaic regions (i=01,02..12). Any region should aligned to CTB.
Mosaic and OSD region should not be both turned on at the same time. Mosaic has higher priority.
For example,
* OSD region 1 should not be turned on when Mosaic region 01 is on.
* OSD region 1 can be turned on when other Mosaic region(02-12) is on.

| Short | Long Option            | Description                                                    |
|-------|------------------------|----------------------------------------------------------------|
|       | --mosaicEnables        | 12 bits indicate enable for 12 mosaic region. [0]              |
|       |                        |   1: region 1 enabled                                          |
|       |                        |   2: region 2 enabled                                          |
|       |                        |   3: region 1 and 2 enabled                                    |
|       |                        |   and so on.                                                   |
|       | --mosAreai             | Mosaic region i parameters                                     |
|       |                        |   left:top:right:bottom pixel coordinates                      |
|       |                        |   All coordinates must be under CTB/MB aligned.                |

### Parameters Affecting Low Latency Input

| Short | Long Option            | Description                                                    |
|-------|------------------------|----------------------------------------------------------------|
|       | --inputLineBufferMode  |0..4 Input buffer mode control. [0]                             |
|       |                        |0 = Disable input line buffer.                                  |
|       |                        |1 = Enable. SW handshaking. Loop-back enabled.                  |
|       |                        |2 = Enable. HW handshaking. Loop-back enabled.                  |
|       |                        |3 = Enable. SW handshaking. Loop-back disabled.                 |
|       |                        |4 = Enable. HW handshaking. Loop-back disabled.                 |
|       | --inputLineBufferDepth |0..511. The number of CTB/MB rows to control loop-back          |
|       |                        | and handshaking. [1]                                           |
|       |                        | When loop-back mode is enabled, there are two continuous       |
|       |                        |  ping-pong input buffers; each contains inputLineBufferDepth   |
|       |                        |  CTB or MB rows.                                               |
|       |                        | When HW handshaking is enabled, handshaking signal is processed|
|       |                        |  per inputLineBufferDepth CTB/MB rows.                         |
|       |                        | When SW handshaking is enabled, IRQ is issued and Read Count   |
|       |                        |  Register is updated every time inputLineBufferDepth CTB/MB    |
|       |                        |  rows have been read.                                          |
|       |                        | 0 is only allowed with inputLineBufferMode=3, IRQ won't be sent|
|       |                        |  and Read Count Register won't be updated.                     |
|       | --inputLineBuffer      | 0..1023. Handshake sync amount for every loop-back. [0]        |
|       |   AmountPerLoopback    |    0    - Disable input line buffer.                           |
|       |                        |  1~1023 - The number of slices per loop-back                   |

### Input Frame and Reference Frame Buffer Alignment Options

| Short | Long Option            | Description                                                    |
|-------|------------------------|----------------------------------------------------------------|
|       | --inputAlignmentExp    | 0, 4..12 Alignment value of input frame buffer. [4]            |
|       |                        |   0 = Disable alignment                                        |
|       |                        |   4..12 = Base address of input frame buffer and each line are |
|       |                        |    aligned to 2^inputAlignmentExp.                             |
|       | --refAlignmentExp      | 0, 4..12 Reference frame buffer alignment [0]                  |
|       |                        |   0 = Disable alignment                                        |
|       |                        |   4..12 = Base address of reference frame buffer and each line |
|       |                        |     are aligned to 2^refAlignmentExp.                          |
|       | --refChromaAlignmentExp| 0, 4..12 Reference frame buffer alignment for chroma [0]       |
|       |                        |  0 = Disable alignment                                         |
|       |                        |  4..12 = Base address of chroma reference frame buffer and each|
|       |                        |    line are aligned to 2^refChromaAlignmentExp.                |
|       | --aqInfoAlignmentExp   | 0, 4..12 AQ information output buffer alignment [0]            |
|       |                        |  0 = Disable alignment                                         |
|       |                        |  4..12 = Base address of AQ information output buffer and each |
|       |                        |    line are aligned to 2^aqInfoAlignmentExp                    |
|       | --tileStreamAlignmentExp| 0..15 tile stream alignment [0]                               |
|       |                        |  0..15 = Base address and size of tile stream are aligned to   |
|       |                        |     2^tileTrailAlignmentExp.                                   |


### Parameters Affecting Stream Multi-segment Output

| Short | Long Option            | Description                                                    |
|-------|------------------------|----------------------------------------------------------------|
|       |--streamMultiSegmentMode| 0..2 Stream multi-segment mode control. [0]                    |
|       |                        | 0 = Disable stream multi-segment mode.                         |
|       |                        | 1 = Enable stream multi-segment mode. No SW handshaking.       |
|       |                        |   Loop-back is enabled.                                        |
|       |                        | 2 = Enable stream multi-segment mode. SW handshaking. Loop-back|
|       |                        |  is enabled.                                                   |
|       | --streamMultiSegmentAmount | 2..16. The total amount of segments to control loop-back   |
|       |                        |   /SW handshaking/IRQ. [4]                                     |

### Parameters for AXI

| Short | Long Option            | Description                                                    |
|-------|------------------------|----------------------------------------------------------------|
|       | --AXIAlignment         | AXI alignment setting (in hexadecimal format). [0]             |
|       |                        |   swreg367 bit[29:26] AXI_burst_align_wr_cuinfo                |
|       |                        |   swreg320 bit[31:28] AXI_burst_align_wr_common                |
|       |                        |   swreg320 bit[27:24] AXI_burst_align_wr_stream                |
|       |                        |   swreg320 bit[23:20] AXI_burst_align_wr_chroma_ref            |
|       |                        |   swreg320 bit[19:16] AXI_burst_align_wr_luma_ref              |
|       |                        |   swreg320 bit[15:12] AXI_burst_align_rd_common                |
|       |                        |   swreg320 bit[11: 8] AXI_burst_align_rd_prp                   |
|       |                        |   swreg320 bit[ 7: 4] AXI_burst_align_rd_ch_ref_prefetch       |
|       |                        |   swreg320 bit[ 3: 0] AXI_burst_align_rd_lu_ref_prefetch       |
|       | --burstMaxLength       | Maximum AXI burst length. [16]                                 |

### IRQ Type

#### Parameters for IRQ Type Mask

| Short | Long Option            | Description                                                    |
|-------|------------------------|----------------------------------------------------------------|
|       | --irqTypeMask          | irq Type mask setting (in binary format). [111110000]          |
|       |                        |   swreg1 bit24 irq_type_sw_reset_mask, default 1               |
|       |                        |   swreg1 bit23 irq_type_fuse_error_mask, default 1             |
|       |                        |   swreg1 bit22 irq_type_buffer_full_mask, default 1            |
|       |                        |   swreg1 bit21 irq_type_bus_error_mask, default 1              |
|       |                        |   swreg1 bit20 irq_type_timeout_mask, default 1                |
|       |                        |   swreg1 bit19 irq_type_strm_segment_mask, default 0           |
|       |                        |   swreg1 bit18 irq_type_line_buffer_mask, default 0            |
|       |                        |   swreg1 bit17 irq_type_slice_rdy_mask, default 0              |
|       |                        |   swreg1 bit16 irq_type_frame_rdy_mask, default 0              |


#### Parameters for IRQ Type Cutree Mask

| Short | Long Option            | Description                                                    |
|-------|------------------------|----------------------------------------------------------------|
|       | --irqTypeCutreeMask    | irq Type cutree mask setting (in binary format). [111110000]   |
|       |                        |                irq_type_sw_reset_mask, default 1               |
|       |                        |                irq_type_fuse_error_mask, default 1             |
|       |                        |                irq_type_buffer_full_mask, default 1            |
|       |                        |   Imswreg1 bit21 irq_type_bus_error_mask, default 1            |
|       |                        |   Imswreg1 bit20 irq_type_timeout_mask, default 1              |
|       |                        |                irq_type_strm_segment_mask, default 0           |
|       |                        |                irq_type_line_buffer_mask, default 0            |
|       |                        |                irq_type_slice_rdy_mask, default 0              |
|       |                        |   Imswreg1 bit16 irq_type_frame_rdy_mask, default 0            |
|       |                        |                                                                |
|       |                        |   In order to be consistent with irqTypeMask, we set it with   |
|       |                        |   9 bits. But only parsing irq_type_bus_error_mask,            |
|       |                        |   irq_type_timeout_mask and irq_type_frame_rdy_mask.           |

### Parameters for Control the Peripherals(Only for C-Model)

| Short | Long Option            | Description                                                    |
|-------|------------------------|----------------------------------------------------------------|
|       | --useVcmd              | 0..1 Disable/Enable VCMD                                       |
|       | --useDec400            | 0..1 Disable/Enable DEC400                                     |
|       | --useL2Cache           | 0..1 Disable/Enable L2Cache                                    |

### Parameters for External SRAM

| Short | Long Option            | Description                                                    |
|-------|------------------------|----------------------------------------------------------------|
|       | --extSramLumHeightBwd  | [0..16] Exteranl SRAM capacity for luma backward reference.    |
|       |                        |   0=no external SRAM,                                          |
|       |                        |   1..16=The number of line count 4*extSramLumHeightBwd         |
|       |                        |   default: hevc - 16, h264 - 12                                |
|       | --extSramChrHeightBwd  | [0..16] Exteranl SRAM capacity for chroma backward reference.  |
|       |                        |   0=no external SRAM,                                          |
|       |                        |   1..16=The number of line count 4*extSramChrHeightBwd         |
|       |                        |   default: hevc - 8, h264 - 6                                  |
|       | --extSramLumHeightFwd  | [0..16] Exteranl SRAM capacity for luma forward reference.     |
|       |                        |   0=no external SRAM,                                          |
|       |                        |   1..16=The number of line count 4*extSramLumHeightFwd         |
|       |                        |   default: hevc - 16, h264 - 12                                |
|       | --extSramChrHeightFwd  | [0..16] Exteranl SRAM capacity for chroma forward reference.   |
|       |                        |   0=no external SRAM,                                          |
|       |                        |   1..16=The number of line count 4*extSramChrHeightFwd         |
|       |                        |   default: hevc - 8, h264 - 6                                  |

## Debug and Testing

### Parameters for Debug

| Short | Long Option            | Description                                                    |
|-------|------------------------|----------------------------------------------------------------|
|       | --dumpRegister         |   0..1 dump register enable [0]                                |
|       |                        |  when enable, software will dump register values to sw_reg.trc.|
|       |                        |                                                                |
|       |                        |  0: no register dump.                                          |
|       |                        |  1: print register dump.                                       |
|       | --rasterscan           |   0..1 raster scan enable [0]                                  |
|       |                        |  When enable, Raster scan output will dump recon on FPGA & HW  |
|       |                        |  0: disable raster scan output.                                |
|       |                        |  1: enable raster scan output.                                 |

### Temporary Testing Multiple Encoder Instances in Multi-thread Mode or Multi-process Mode

| Short | Long Option            | Description                                                    |
|-------|------------------------|----------------------------------------------------------------|
|       | --multimode            | Specify parallel running mode  [0]                             |
|       |                        |  0: disable                                                    |
|       |                        |  1: multi-thread mode                                          |
|       |                        |  2: multi-process mode                                         |
|       | --streamcfg            | Specify the filename storing encoder options                   |

### Parameters Affecting Runtime Log Level

| Short | Long Option            | Description                                                    |
|-------|------------------------|----------------------------------------------------------------|
|       | --verbose              |   0..1 Log printing mode [0]                                   |
|       |                        |  0: Print brief information.                                   |
|       |                        |  1: Print more detailed information.                           |

### Parameters for Control Log&Print Message
| Short | Long Option              | Description                                                  |
| ----- | ------------------------ | -------------------------------------------------------------|
|       | --logOutDir              | to control log message output device. [0]                    |
|       |                          | 0 = all log output to stdout.                                |
|       |                          | 1 = all log use one log file.                                |
|       |                          | 2 = each thread of each instance has its own log file.       |
|       | --logOutLevel            | to control log message output level. [3]                     |
|       |                          | 0 = "quiet": no output.                                      |
|       |                          | 1 = "fatal": serious errors.                                 |
|       |                          | 2 = "error": error happens.                                  |
|       |                          | 3 = "warn": may not well used.                               |
|       |                          | 4 = "info": provide some information                         |
|       |                          | 5 = "debug": provide some information                        |
|       |                          | 6 = "all": every thing                                       |
|       | --logTraceMap            | to control log message output trace information for          |
|       |                          | prompt and debug. [63]=b`0111111                             |
|       |                          | 0 = dump API call.                                           |
|       |                          | 1 = dump registers.                                          |
|       |                          | 2 = dump EWL.                                                |
|       |                          | 3 = dump memory usage.                                       |
|       |                          | 4 = dump Rate Control Status                                 |
|       |                          | 5 = output full command line                                 |
|       |                          | 6 = output performance information                           |
|       | --logCheckMap            | to control log message output check information for test     |
|       |                          | only. [1]=b`00001                                            |
|       |                          | 0 = output recon YUV data.                                   |
|       |                          | 1 = output quality PSNR/SSIM for each frame.                 |
|       |                          | 2 = output VBV information for checking RC.                  |
|       |                          | 3 = output RC information for RC profiling.                  |
|       |                          | 4 = output features for coverage                             |

## Internal Dev Usage

### Parameters for Internal DEV Usage, not Recommended for Users

| Short | Long Option            | Description                                                    |
|-------|------------------------|----------------------------------------------------------------|
|       | --parallelCoreNum      | 1..4 The number of cores run in parallel at frame level.[1]    |
|       |                        | It is set to 1, when num_tile_columns > 1.                     |
|       | --smoothingIntra       | 0..1 Enable or disable the strong_intra_smoothing_enabled_flag |
|       |                        | (HEVC only). 0=Normal, 1=Strong. [1]                           |
|       | --testId               | Internal test ID. [0]                                          |
|       | --roiMapDeltaQpBinFile | User-specified DeltaQp map binary file, only valid when        |
|       |                        | lookaheadDepth > 0. Every byte stands for a block unit deltaQp |
|       |                        | in raster-scan order. The block unit size is defined by        |
|       |                        | "--roiMapDeltaQpBlockUnit" (0-64x64,1-32x32,2-16x16,3-8x8).    |
|       |                        | Frame width and height are aligned to block unit size when     |
|       |                        | setting deltaQp per block. Frame N data is followed by Frame   |
|       |                        | N+1 data immediately in display order.                         |
|       | --RoimapCuCtrlIndexBinFile | User-specified ROI map CU control index binary file,       |
|       |                        | which defines where the ROI info is included in the bin file   |
|       |                        |  "--RoimapCuCtrlInfoBinFile" (InfoBinFile).                    |
|       |                        | If the InfoBinFile is not specified, the ROI info about CUs in |
|       |                        | each CTB is included in the file "RoimapCuCtrlInfoBinFile".    |
|       |                        | If InfoBinFile is specified, the ROI info about CUs in each CTB|
|       |                        | should be included in the file "RoimapCuCtrlIndexBinFile".     |
|       | --TxTypeSearchEnable   | 0..1 Enable AV1 Tx type search. [0]                            |
|       |                        |   0=Disable; 1=Enable.                                         |
|       | --av1InterFiltSwitch   | 0..1 Enable AV1 interp filter switch. [1]                      |
|       |                        |   0=Disable; 1=Enable.                                         |
|       | --rdLog                | output rd log in profile.log                                   |
|       | --replaceMvFile        | File name for user provided ME1N MVs.                          |
|       |                        | raster scan 32x32 blocks, 21 MVs for each block. For each MV,  |
|       |                        | (Hor+Ver)*(sizeof(i16))*(L0+L1)=8byte; 21 MV position:         |
|       |                        | 32x32(0,0),                                                    |
|       |                        | 16x16(0,0), 16x16(16,0), 16x16(0,16), 16x16(16,16),            |
|       |                        | 8x8(0,0), 8x8(8,0), 8x8(16,0), 8x8(24,0),                      |
|       |                        | 8x8(0,8), 8x8(8,8), 8x8(16,8), 8x8(24,8),                      |
|       |                        | 8x8(0,16), 8x8(8,16), 8x8(16,16), 8x8(24,16),                  |
|       |                        | 8x8(0,24), 8x8(8,24), 8x8(16,24), 8x8(24,24)                   |
|       | --osdDec400TableInput  | Read osd input DEC400 compressed table from file. [NULL]       |


