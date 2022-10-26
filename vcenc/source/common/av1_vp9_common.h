/*------------------------------------------------------------------------------
--                                                                                                                               --
--       This software is confidential and proprietary and may be used                                   --
--        only as expressly authorized by a licensing agreement from                                     --
--                                                                                                                               --
--                            Verisilicon.                                                                                    --
--                                                                                                                               --
--                   (C) COPYRIGHT 2014 VERISILICON                                                            --
--                            ALL RIGHTS RESERVED                                                                    --
--                                                                                                                               --
--                 The entire notice above must be reproduced                                                 --
--                  on all copies and should not be removed.                                                    --
--                                                                                                                               --
--------------------------------------------------------------------------------
--
--  Description : AV1 VP9 common macro and data
--
------------------------------------------------------------------------------*/
#ifndef __AV1_VP9_COMMON_BASE_H__
#define __AV1_VP9_COMMON_BASE_H__

#include "base_type.h"

#define SW_NONE_FRAME (-1)
#define SW_INTRA_FRAME 0
#define SW_LAST_FRAME 1
#define SW_LAST2_FRAME 2
#define SW_LAST3_FRAME 3
#define SW_GOLDEN_FRAME 4
#define SW_BWDREF_FRAME 5
#define SW_ALTREF2_FRAME 6
#define SW_ALTREF_FRAME 7

#define SW_REF_FRAMES_LOG2 3
#define SW_REF_FRAMES (1 << SW_REF_FRAMES_LOG2)
#define DEFAULT_EXPLICIT_ORDER_HINT_BITS 8

#define SW_EXTREF_FRAME (SW_ALTREF_FRAME + 1)
#define SW_LAST_REF_FRAMES (SW_LAST3_FRAME - SW_LAST_FRAME + 1)
#define SW_INTER_REFS_PER_FRAME (SW_ALTREF_FRAME - SW_LAST_FRAME + 1)

// Maximum number of tile rows and tile columns
#define SW_MAX_TILE_ROWS 1024
#define SW_MAX_TILE_COLS 1024

#define MAX_LOOP_FILTER 63

static const int16_t ac_qlookup_Q3_obu[256] = {
    4,    8,    9,    10,   11,   12,   13,   14,   15,   16,   17,   18,   19,   20,   21,   22,
    23,   24,   25,   26,   27,   28,   29,   30,   31,   32,   33,   34,   35,   36,   37,   38,
    39,   40,   41,   42,   43,   44,   45,   46,   47,   48,   49,   50,   51,   52,   53,   54,
    55,   56,   57,   58,   59,   60,   61,   62,   63,   64,   65,   66,   67,   68,   69,   70,
    71,   72,   73,   74,   75,   76,   77,   78,   79,   80,   81,   82,   83,   84,   85,   86,
    87,   88,   89,   90,   91,   92,   93,   94,   95,   96,   97,   98,   99,   100,  101,  102,
    104,  106,  108,  110,  112,  114,  116,  118,  120,  122,  124,  126,  128,  130,  132,  134,
    136,  138,  140,  142,  144,  146,  148,  150,  152,  155,  158,  161,  164,  167,  170,  173,
    176,  179,  182,  185,  188,  191,  194,  197,  200,  203,  207,  211,  215,  219,  223,  227,
    231,  235,  239,  243,  247,  251,  255,  260,  265,  270,  275,  280,  285,  290,  295,  300,
    305,  311,  317,  323,  329,  335,  341,  347,  353,  359,  366,  373,  380,  387,  394,  401,
    408,  416,  424,  432,  440,  448,  456,  465,  474,  483,  492,  501,  510,  520,  530,  540,
    550,  560,  571,  582,  593,  604,  615,  627,  639,  651,  663,  676,  689,  702,  715,  729,
    743,  757,  771,  786,  801,  816,  832,  848,  864,  881,  898,  915,  933,  951,  969,  988,
    1007, 1026, 1046, 1066, 1087, 1108, 1129, 1151, 1173, 1196, 1219, 1243, 1267, 1292, 1317, 1343,
    1369, 1396, 1423, 1451, 1479, 1508, 1537, 1567, 1597, 1628, 1660, 1692, 1725, 1759, 1793, 1828,
};

static const int16_t ac_qlookup_10_Q3_obu[256] = {
    4,    9,    11,   13,   16,   18,   21,   24,   27,   30,   33,   37,   40,   44,   48,   51,
    55,   59,   63,   67,   71,   75,   79,   83,   88,   92,   96,   100,  105,  109,  114,  118,
    122,  127,  131,  136,  140,  145,  149,  154,  158,  163,  168,  172,  177,  181,  186,  190,
    195,  199,  204,  208,  213,  217,  222,  226,  231,  235,  240,  244,  249,  253,  258,  262,
    267,  271,  275,  280,  284,  289,  293,  297,  302,  306,  311,  315,  319,  324,  328,  332,
    337,  341,  345,  349,  354,  358,  362,  367,  371,  375,  379,  384,  388,  392,  396,  401,
    409,  417,  425,  433,  441,  449,  458,  466,  474,  482,  490,  498,  506,  514,  523,  531,
    539,  547,  555,  563,  571,  579,  588,  596,  604,  616,  628,  640,  652,  664,  676,  688,
    700,  713,  725,  737,  749,  761,  773,  785,  797,  809,  825,  841,  857,  873,  889,  905,
    922,  938,  954,  970,  986,  1002, 1018, 1038, 1058, 1078, 1098, 1118, 1138, 1158, 1178, 1198,
    1218, 1242, 1266, 1290, 1314, 1338, 1362, 1386, 1411, 1435, 1463, 1491, 1519, 1547, 1575, 1603,
    1631, 1663, 1695, 1727, 1759, 1791, 1823, 1859, 1895, 1931, 1967, 2003, 2039, 2079, 2119, 2159,
    2199, 2239, 2283, 2327, 2371, 2415, 2459, 2507, 2555, 2603, 2651, 2703, 2755, 2807, 2859, 2915,
    2971, 3027, 3083, 3143, 3203, 3263, 3327, 3391, 3455, 3523, 3591, 3659, 3731, 3803, 3876, 3952,
    4028, 4104, 4184, 4264, 4348, 4432, 4516, 4604, 4692, 4784, 4876, 4972, 5068, 5168, 5268, 5372,
    5476, 5584, 5692, 5804, 5916, 6032, 6148, 6268, 6388, 6512, 6640, 6768, 6900, 7036, 7172, 7312,
};

static const int16_t ac_qlookup_12_Q3_obu[256] = {
    4,     13,    19,    27,    35,    44,    54,    64,    75,    87,    99,    112,   126,
    139,   154,   168,   183,   199,   214,   230,   247,   263,   280,   297,   314,   331,
    349,   366,   384,   402,   420,   438,   456,   475,   493,   511,   530,   548,   567,
    586,   604,   623,   642,   660,   679,   698,   716,   735,   753,   772,   791,   809,
    828,   846,   865,   884,   902,   920,   939,   957,   976,   994,   1012,  1030,  1049,
    1067,  1085,  1103,  1121,  1139,  1157,  1175,  1193,  1211,  1229,  1246,  1264,  1282,
    1299,  1317,  1335,  1352,  1370,  1387,  1405,  1422,  1440,  1457,  1474,  1491,  1509,
    1526,  1543,  1560,  1577,  1595,  1627,  1660,  1693,  1725,  1758,  1791,  1824,  1856,
    1889,  1922,  1954,  1987,  2020,  2052,  2085,  2118,  2150,  2183,  2216,  2248,  2281,
    2313,  2346,  2378,  2411,  2459,  2508,  2556,  2605,  2653,  2701,  2750,  2798,  2847,
    2895,  2943,  2992,  3040,  3088,  3137,  3185,  3234,  3298,  3362,  3426,  3491,  3555,
    3619,  3684,  3748,  3812,  3876,  3941,  4005,  4069,  4149,  4230,  4310,  4390,  4470,
    4550,  4631,  4711,  4791,  4871,  4967,  5064,  5160,  5256,  5352,  5448,  5544,  5641,
    5737,  5849,  5961,  6073,  6185,  6297,  6410,  6522,  6650,  6778,  6906,  7034,  7162,
    7290,  7435,  7579,  7723,  7867,  8011,  8155,  8315,  8475,  8635,  8795,  8956,  9132,
    9308,  9484,  9660,  9836,  10028, 10220, 10412, 10604, 10812, 11020, 11228, 11437, 11661,
    11885, 12109, 12333, 12573, 12813, 13053, 13309, 13565, 13821, 14093, 14365, 14637, 14925,
    15213, 15502, 15806, 16110, 16414, 16734, 17054, 17390, 17726, 18062, 18414, 18766, 19134,
    19502, 19886, 20270, 20670, 21070, 21486, 21902, 22334, 22766, 23214, 23662, 24126, 24590,
    25070, 25551, 26047, 26559, 27071, 27599, 28143, 28687, 29247,
};

/* Shift down with rounding for use when n >= 0, value >= 0 */
#define ROUND_POWER_OF_TWO(value, n) (((value) + (((1 << (n)) >> 1))) >> (n))

/* Constant values while waiting for the sequence header */
#define FRAME_ID_LENGTH 15
#define DELTA_FRAME_ID_LENGTH 14

// An enum for single reference types (and some derived values).
enum ATTRIBUTE_PACKED {
    NONE_FRAME = -1,
    INTRA_FRAME,
    LAST_FRAME,
    LAST2_FRAME,
    LAST3_FRAME,
    GOLDEN_FRAME,
    BWDREF_FRAME,
    ALTREF2_FRAME,
    ALTREF_FRAME,
    REF_FRAMES,

    // Extra/scratch reference frame. It may be:
    // - used to update the ALTREF2_FRAME ref (see lshift_bwd_ref_frames()), or
    // - updated from ALTREF2_FRAME ref (see rshift_bwd_ref_frames()).
    EXTREF_FRAME = REF_FRAMES,

    // Number of inter (non-intra) reference types.
    INTER_REFS_PER_FRAME = ALTREF_FRAME - LAST_FRAME + 1,

    // Number of forward (aka past) reference types.
    FWD_REFS = GOLDEN_FRAME - LAST_FRAME + 1,

    // Number of backward (aka future) reference types.
    BWD_REFS = ALTREF_FRAME - BWDREF_FRAME + 1,

    SINGLE_REFS = FWD_REFS + BWD_REFS,
};

#define MV_CLASSES 11
#define CLASS0_BITS 1 /* bits at integer precision for class 0 */
#define CLASS0_SIZE (1 << CLASS0_BITS)
#define MV_OFFSET_BITS (MV_CLASSES + CLASS0_BITS - 2)
#define MV_BITS_CONTEXTS 6
#define MV_FP_SIZE 4

#define MV_MAX_BITS (MV_CLASSES + CLASS0_BITS + 2)
#define MV_MAX ((1 << MV_MAX_BITS) - 1)
#define MV_VALS ((MV_MAX << 1) + 1)

#define MV_IN_USE_BITS 14
#define MV_UPP (1 << MV_IN_USE_BITS)
#define MV_LOW (-(1 << MV_IN_USE_BITS))
#define MV_JOINTS 4

#endif
