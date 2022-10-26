#include <stdint.h>
#include <math.h>
#include "vsi_string.h"
#include "osal.h"
#include "hevcencapi.h"
#ifdef SUPPORT_AV1
#include "av1encapi.h"
#endif
#include "libvmaf.h"
#include "vmaf.h"
#include "vmaf.pkl.h"
#include "vmaf.pkl.model.h"

#if (defined(__GNUC__) && __GNUC__) || defined(__SUNPRO_C)
#define DECLARE_ALIGNED(n, typ, val) typ val __attribute__((aligned(n)))
#elif defined(_MSC_VER)
#define DECLARE_ALIGNED(n, typ, val) __declspec(align(n)) typ val
#else
#warning No alignment directives known for this compiler.
#define DECLARE_ALIGNED(n, typ, val) typ val
#endif
static inline uint8_t clip_pixel(int val)
{
    return (val > 255) ? 255 : (val < 0) ? 0 : val;
}

//  This is used as a reference when computing the source variance for the
//  purposes of activity masking.
//  Eventually this should be replaced by custom no-reference routines,
//  which will be faster.
const uint8_t AV1_VAR_OFFS[MAX_SB_SIZE] = {
    128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
    128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
    128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
    128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
    128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
    128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
    128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128};

static const uint16_t AV1_HIGH_VAR_OFFS_8[MAX_SB_SIZE] = {
    128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
    128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
    128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
    128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
    128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
    128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
    128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128};

static const uint16_t AV1_HIGH_VAR_OFFS_10[MAX_SB_SIZE] = {
    128 * 4, 128 * 4, 128 * 4, 128 * 4, 128 * 4, 128 * 4, 128 * 4, 128 * 4, 128 * 4, 128 * 4,
    128 * 4, 128 * 4, 128 * 4, 128 * 4, 128 * 4, 128 * 4, 128 * 4, 128 * 4, 128 * 4, 128 * 4,
    128 * 4, 128 * 4, 128 * 4, 128 * 4, 128 * 4, 128 * 4, 128 * 4, 128 * 4, 128 * 4, 128 * 4,
    128 * 4, 128 * 4, 128 * 4, 128 * 4, 128 * 4, 128 * 4, 128 * 4, 128 * 4, 128 * 4, 128 * 4,
    128 * 4, 128 * 4, 128 * 4, 128 * 4, 128 * 4, 128 * 4, 128 * 4, 128 * 4, 128 * 4, 128 * 4,
    128 * 4, 128 * 4, 128 * 4, 128 * 4, 128 * 4, 128 * 4, 128 * 4, 128 * 4, 128 * 4, 128 * 4,
    128 * 4, 128 * 4, 128 * 4, 128 * 4, 128 * 4, 128 * 4, 128 * 4, 128 * 4, 128 * 4, 128 * 4,
    128 * 4, 128 * 4, 128 * 4, 128 * 4, 128 * 4, 128 * 4, 128 * 4, 128 * 4, 128 * 4, 128 * 4,
    128 * 4, 128 * 4, 128 * 4, 128 * 4, 128 * 4, 128 * 4, 128 * 4, 128 * 4, 128 * 4, 128 * 4,
    128 * 4, 128 * 4, 128 * 4, 128 * 4, 128 * 4, 128 * 4, 128 * 4, 128 * 4, 128 * 4, 128 * 4,
    128 * 4, 128 * 4, 128 * 4, 128 * 4, 128 * 4, 128 * 4, 128 * 4, 128 * 4, 128 * 4, 128 * 4,
    128 * 4, 128 * 4, 128 * 4, 128 * 4, 128 * 4, 128 * 4, 128 * 4, 128 * 4, 128 * 4, 128 * 4,
    128 * 4, 128 * 4, 128 * 4, 128 * 4, 128 * 4, 128 * 4, 128 * 4, 128 * 4};

#define STRINGIZE(x) #x
#define STRINGIZE_VALUE_OF(x) STRINGIZE(x)
#define MODEL_PATH(ext) STRINGIZE_VALUE_OF(VMAF_MODEL) #ext
int vmaf_prepare_model()
{
    const char *vmaf_pkl_path = MODEL_PATH(.pkl);
    const char *vmaf_model_path = MODEL_PATH(.pkl.model);
    int status = 0;
    if (access(vmaf_pkl_path, R_OK) != 0 || access(vmaf_model_path, R_OK) != 0) {
        FILE *fp = fopen(vmaf_pkl_path, "wb");
        FILE *fp2 = fopen(vmaf_model_path, "wb");
        if (!fp || !fp2)
            return -1;

        if (fwrite(___lib_vmaf_v0_6_1_pkl, 1, ___lib_vmaf_v0_6_1_pkl_len, fp) !=
            ___lib_vmaf_v0_6_1_pkl_len)
            status = -1;
        if (fwrite(___lib_vmaf_v0_6_1_pkl_model, 1, ___lib_vmaf_v0_6_1_pkl_model_len, fp2) !=
            ___lib_vmaf_v0_6_1_pkl_model_len)
            status = -1;

        fclose(fp);
        fclose(fp2);
    }
    return status;
}
static void copy_and_extend_plane(const uint8_t *src, int src_pitch, uint8_t *dst, int dst_pitch,
                                  int w, int h, int extend_top, int extend_left, int extend_bottom,
                                  int extend_right)
{
    int i, linesize;

    // copy the left and right most columns out
    const uint8_t *src_ptr1 = src;
    const uint8_t *src_ptr2 = src + w - 1;
    uint8_t *dst_ptr1 = dst - extend_left;
    uint8_t *dst_ptr2 = dst + w;

    for (i = 0; i < h; i++) {
        memset(dst_ptr1, src_ptr1[0], extend_left);
        memcpy(dst_ptr1 + extend_left, src_ptr1, w);
        memset(dst_ptr2, src_ptr2[0], extend_right);
        src_ptr1 += src_pitch;
        src_ptr2 += src_pitch;
        dst_ptr1 += dst_pitch;
        dst_ptr2 += dst_pitch;
    }

    // Now copy the top and bottom lines into each line of the respective
    // borders
    src_ptr1 = dst - extend_left;
    src_ptr2 = dst + dst_pitch * (h - 1) - extend_left;
    dst_ptr1 = dst + dst_pitch * (-extend_top) - extend_left;
    dst_ptr2 = dst + dst_pitch * (h)-extend_left;
    linesize = extend_left + extend_right + w;

    for (i = 0; i < extend_top; i++) {
        memcpy(dst_ptr1, src_ptr1, linesize);
        dst_ptr1 += dst_pitch;
    }

    for (i = 0; i < extend_bottom; i++) {
        memcpy(dst_ptr2, src_ptr2, linesize);
        dst_ptr2 += dst_pitch;
    }
}

static inline void *memset16(void *dest, int val, size_t length)
{
    size_t i;
    uint16_t *dest16 = (uint16_t *)dest;
    for (i = 0; i < length; i++)
        *dest16++ = val;
    return dest;
}
static void highbd_copy_and_extend_plane(const uint16_t *src, int src_pitch, uint16_t *dst,
                                         int dst_pitch, int w, int h, int extend_top,
                                         int extend_left, int extend_bottom, int extend_right)
{
    int i, linesize;

    // copy the left and right most columns out
    const uint16_t *src_ptr1 = src;
    const uint16_t *src_ptr2 = src + w - 1;
    uint16_t *dst_ptr1 = dst - extend_left;
    uint16_t *dst_ptr2 = dst + w;

    for (i = 0; i < h; i++) {
        memset16(dst_ptr1, src_ptr1[0], extend_left);
        memcpy(dst_ptr1 + extend_left, src_ptr1, w * sizeof(src_ptr1[0]));
        memset16(dst_ptr2, src_ptr2[0], extend_right);
        src_ptr1 += src_pitch;
        src_ptr2 += src_pitch;
        dst_ptr1 += dst_pitch;
        dst_ptr2 += dst_pitch;
    }

    // Now copy the top and bottom lines into each line of the respective
    // borders
    src_ptr1 = dst - extend_left;
    src_ptr2 = dst + dst_pitch * (h - 1) - extend_left;
    dst_ptr1 = dst + dst_pitch * (-extend_top) - extend_left;
    dst_ptr2 = dst + dst_pitch * (h)-extend_left;
    linesize = extend_left + extend_right + w;

    for (i = 0; i < extend_top; i++) {
        memcpy(dst_ptr1, src_ptr1, linesize * sizeof(src_ptr1[0]));
        dst_ptr1 += dst_pitch;
    }

    for (i = 0; i < extend_bottom; i++) {
        memcpy(dst_ptr2, src_ptr2, linesize * sizeof(src_ptr2[0]));
        dst_ptr2 += dst_pitch;
    }
}

void av1_copy_and_extend_frame(const YV12_BUFFER_CONFIG *src, YV12_BUFFER_CONFIG *dst)
{
    // Extend src frame in buffer
    const int et_y = dst->border;
    const int el_y = dst->border;
    const int er_y =
        MAX(src->y_width + dst->border, ALIGN_POWER_OF_TWO(src->y_width, 6)) - src->y_width;
    const int eb_y =
        MAX(src->y_height + dst->border, ALIGN_POWER_OF_TWO(src->y_height, 6)) - src->y_height;
    const int uv_width_subsampling = (src->uv_width != src->y_width);
    const int uv_height_subsampling = (src->uv_height != src->y_height);
    const int et_uv = et_y >> uv_height_subsampling;
    const int el_uv = el_y >> uv_width_subsampling;
    const int eb_uv = eb_y >> uv_height_subsampling;
    const int er_uv = er_y >> uv_width_subsampling;

    if (0) {
        highbd_copy_and_extend_plane((uint16_t *)src->y_buffer, src->y_stride,
                                     (uint16_t *)dst->y_buffer, dst->y_stride, src->y_width,
                                     src->y_height, et_y, el_y, eb_y, er_y);

        highbd_copy_and_extend_plane((uint16_t *)src->u_buffer, src->uv_stride,
                                     (uint16_t *)dst->u_buffer, dst->uv_stride, src->uv_width,
                                     src->uv_height, et_uv, el_uv, eb_uv, er_uv);

        highbd_copy_and_extend_plane((uint16_t *)src->v_buffer, src->uv_stride,
                                     (uint16_t *)dst->v_buffer, dst->uv_stride, src->uv_width,
                                     src->uv_height, et_uv, el_uv, eb_uv, er_uv);
        return;
    }

    copy_and_extend_plane(src->y_buffer, src->y_stride, dst->y_buffer, dst->y_stride, src->y_width,
                          src->y_height, et_y, el_y, eb_y, er_y);

    copy_and_extend_plane(src->u_buffer, src->uv_stride, dst->u_buffer, dst->uv_stride,
                          src->uv_width, src->uv_height, et_uv, el_uv, eb_uv, er_uv);

    copy_and_extend_plane(src->v_buffer, src->uv_stride, dst->v_buffer, dst->uv_stride,
                          src->uv_width, src->uv_height, et_uv, el_uv, eb_uv, er_uv);
}

static void variance(const uint8_t *a, int a_stride, const uint8_t *b, int b_stride, int w, int h,
                     uint32_t *sse, int *sum)
{
    int i, j;

    *sum = 0;
    *sse = 0;

    for (i = 0; i < h; ++i) {
        for (j = 0; j < w; ++j) {
            const int diff = a[j] - b[j];
            *sum += diff;
            *sse += diff * diff;
        }

        a += a_stride;
        b += b_stride;
    }
}

#define VAR(W, H)                                                                                  \
    uint32_t aom_variance##W##x##H##_c(const uint8_t *a, int a_stride, const uint8_t *b,           \
                                       int b_stride, uint32_t *sse)                                \
    {                                                                                              \
        int sum;                                                                                   \
        variance(a, a_stride, b, b_stride, W, H, sse, &sum);                                       \
        return *sse - (uint32_t)(((int64_t)sum * sum) / (W * H));                                  \
    }

VAR(64, 64)

static INLINE ConvolveParams get_conv_params_no_round(int cmp_index, int plane, CONV_BUF_TYPE *dst,
                                                      int dst_stride, int is_compound, int bd)
{
    ConvolveParams conv_params;

    conv_params.is_compound = is_compound;
    conv_params.round_0 = ROUND0_BITS;
    conv_params.round_1 =
        is_compound ? COMPOUND_ROUND1_BITS : 2 * FILTER_BITS - conv_params.round_0;
    const int intbufrange = bd + FILTER_BITS - conv_params.round_0 + 2;
    ASSERT(IMPLIES(bd < 12, intbufrange <= 16));
    if (intbufrange > 16) {
        conv_params.round_0 += intbufrange - 16;
        if (!is_compound)
            conv_params.round_1 -= intbufrange - 16;
    }
    // TODO(yunqing): The following dst should only be valid while
    // is_compound = 1;
    conv_params.dst = dst;
    conv_params.dst_stride = dst_stride;
    conv_params.plane = plane;

    // By default, set do average to 1 if this is the second single prediction
    // in a compound mode.
    conv_params.do_average = cmp_index;
    return conv_params;
}

static INLINE ConvolveParams get_conv_params(int do_average, int plane, int bd)
{
    return get_conv_params_no_round(do_average, plane, NULL, 0, 0, bd);
}

static INLINE const int16_t *
av1_get_interp_filter_subpel_kernel(const InterpFilterParams *const filter_params, const int subpel)
{
    return filter_params->filter_ptr + filter_params->taps * subpel;
}

void av1_convolve_2d_sr_c(const uint8_t *src, int src_stride, uint8_t *dst, int dst_stride, int w,
                          int h, const InterpFilterParams *filter_params_x,
                          const InterpFilterParams *filter_params_y, const int subpel_x_qn,
                          const int subpel_y_qn, ConvolveParams *conv_params)
{
    int16_t im_block[(MAX_SB_SIZE + MAX_FILTER_TAP - 1) * MAX_SB_SIZE];
    int im_h = h + filter_params_y->taps - 1;
    int im_stride = w;
    ASSERT(w <= MAX_SB_SIZE && h <= MAX_SB_SIZE);
    const int fo_vert = filter_params_y->taps / 2 - 1;
    const int fo_horiz = filter_params_x->taps / 2 - 1;
    const int bd = 8;
    const int bits = FILTER_BITS * 2 - conv_params->round_0 - conv_params->round_1;

    // horizontal filter
    const uint8_t *src_horiz = src - fo_vert * src_stride;
    const int16_t *x_filter =
        av1_get_interp_filter_subpel_kernel(filter_params_x, subpel_x_qn & SUBPEL_MASK);
    for (int y = 0; y < im_h; ++y) {
        for (int x = 0; x < w; ++x) {
            int32_t sum = (1 << (bd + FILTER_BITS - 1));
            for (int k = 0; k < filter_params_x->taps; ++k) {
                sum += x_filter[k] * src_horiz[y * src_stride + x - fo_horiz + k];
            }
            ASSERT(0 <= sum && sum < (1 << (bd + FILTER_BITS + 1)));
            im_block[y * im_stride + x] = (int16_t)ROUND_POWER_OF_TWO(sum, conv_params->round_0);
        }
    }

    // vertical filter
    int16_t *src_vert = im_block + fo_vert * im_stride;
    const int16_t *y_filter =
        av1_get_interp_filter_subpel_kernel(filter_params_y, subpel_y_qn & SUBPEL_MASK);
    const int offset_bits = bd + 2 * FILTER_BITS - conv_params->round_0;
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int32_t sum = 1 << offset_bits;
            for (int k = 0; k < filter_params_y->taps; ++k) {
                sum += y_filter[k] * src_vert[(y - fo_vert + k) * im_stride + x];
            }
            ASSERT(0 <= sum && sum < (1 << (offset_bits + 2)));
            int16_t res = ROUND_POWER_OF_TWO(sum, conv_params->round_1) -
                          ((1 << (offset_bits - conv_params->round_1)) +
                           (1 << (offset_bits - conv_params->round_1 - 1)));
            dst[y * dst_stride + x] = clip_pixel(ROUND_POWER_OF_TWO(res, bits));
        }
    }
}

// 8-tap Gaussian convolution filter with sigma = 1.0, sums to 128,
// all co-efficients must be even.
DECLARE_ALIGNED(16, static const int16_t, gauss_filter[8]) = {0, 8, 30, 52, 30, 8, 0, 0};
static void gaussian_blur(const int bit_depth, YV12_BUFFER_CONFIG *source,
                          YV12_BUFFER_CONFIG *blurred)
{
    const int block_size = BLOCK_128X128;
    const int block_w = 128;
    const int block_h = 128;
    const int num_cols = (source->y_width + block_w - 1) / block_w;
    const int num_rows = (source->y_height + block_h - 1) / block_h;
    int row, col;

    ConvolveParams conv_params = get_conv_params(0, 0, bit_depth);
    InterpFilterParams filter = {
        .filter_ptr = gauss_filter, .taps = 8, .interp_filter = EIGHTTAP_REGULAR};

    for (row = 0; row < num_rows; ++row) {
        for (col = 0; col < num_cols; ++col) {
            const int row_offset_y = row * block_h;
            const int col_offset_y = col * block_w;

            uint8_t *src_buf = source->y_buffer + row_offset_y * source->y_stride + col_offset_y;
            uint8_t *dst_buf = blurred->y_buffer + row_offset_y * blurred->y_stride + col_offset_y;

            av1_convolve_2d_sr_c(src_buf, source->y_stride, dst_buf, blurred->y_stride, block_w,
                                 block_h, &filter, &filter, 0, 0, &conv_params);
        }
    }
}

static const double kBaselineVmaf = 97.42773;

// TODO(sdeng): Add the SIMD implementation.
static AOM_INLINE void highbd_unsharp_rect(const uint16_t *source, int source_stride,
                                           const uint16_t *blurred, int blurred_stride,
                                           uint16_t *dst, int dst_stride, int w, int h,
                                           double amount, int bit_depth)
{
    const int max_value = (1 << bit_depth) - 1;
    for (int i = 0; i < h; ++i) {
        for (int j = 0; j < w; ++j) {
            const double val =
                (double)source[j] + amount * ((double)source[j] - (double)blurred[j]);
            dst[j] = (uint16_t)clamp((int)(val + 0.5), 0, max_value);
        }
        source += source_stride;
        blurred += blurred_stride;
        dst += dst_stride;
    }
}

static AOM_INLINE void unsharp_rect(const uint8_t *source, int source_stride,
                                    const uint8_t *blurred, int blurred_stride, uint8_t *dst,
                                    int dst_stride, int w, int h, double amount)
{
    for (int i = 0; i < h; ++i) {
        for (int j = 0; j < w; ++j) {
            const double val =
                (double)source[j] + amount * ((double)source[j] - (double)blurred[j]);
            dst[j] = (uint8_t)clamp((int)(val + 0.5), 0, 255);
        }
        source += source_stride;
        blurred += blurred_stride;
        dst += dst_stride;
    }
}

static AOM_INLINE void unsharp(int bit_depth, int use_highbitdepth,
                               const YV12_BUFFER_CONFIG *source, const YV12_BUFFER_CONFIG *blurred,
                               const YV12_BUFFER_CONFIG *dst, double amount)
{
    if (use_highbitdepth) {
        highbd_unsharp_rect((uint16_t *)source->y_buffer, source->y_stride,
                            (uint16_t *)blurred->y_buffer, blurred->y_stride,
                            (uint16_t *)dst->y_buffer, dst->y_stride, source->y_width,
                            source->y_height, amount, bit_depth);
    } else {
        unsharp_rect(source->y_buffer, source->y_stride, blurred->y_buffer, blurred->y_stride,
                     dst->y_buffer, dst->y_stride, source->y_width, source->y_height, amount);
    }
}

static const uint8_t num_pels_log2_lookup[BLOCK_SIZES_ALL] = {
    4, 5, 5, 6, 7, 7, 8, 9, 9, 10, 11, 11, 12, 13, 13, 14, 6, 6, 8, 8, 10, 10};

unsigned int av1_get_sby_perpixel_variance(const struct buf_2d *ref, BLOCK_SIZE bs)
{
    ASSERT(bs == BLOCK_64X64);
    unsigned int sse;
    const unsigned int var = aom_variance64x64_c(ref->buf, ref->stride, AV1_VAR_OFFS, 0, &sse);
    return ROUND_POWER_OF_TWO(var, num_pels_log2_lookup[bs]);
}

unsigned int av1_high_get_sby_perpixel_variance(const struct buf_2d *ref, BLOCK_SIZE bs, int bd)
{
    unsigned int var, sse;
    ASSERT(bs == BLOCK_64X64);
    ASSERT(bd == 8 || bd == 10);
    const int off_index = (bd - 8) >> 1;
    const uint16_t *high_var_offs[2] = {AV1_HIGH_VAR_OFFS_8, AV1_HIGH_VAR_OFFS_10};
    var = aom_variance64x64_c(ref->buf, ref->stride, (uint8_t *)high_var_offs[off_index], 0, &sse);
    return ROUND_POWER_OF_TWO(var, num_pels_log2_lookup[bs]);
}

static double frame_average_variance(int bit_depth, int use_highbitdepth,
                                     const YV12_BUFFER_CONFIG *const frame)
{
    const uint8_t *const y_buffer = frame->y_buffer;
    const int y_stride = frame->y_stride;
    const BLOCK_SIZE block_size = BLOCK_64X64;

    const int block_w = 64;
    const int block_h = 64;
    int row, col;
    double var = 0.0, var_count = 0.0;

    // Loop through each block.
    for (row = 0; row < frame->y_height / block_h; ++row) {
        for (col = 0; col < frame->y_width / block_w; ++col) {
            struct buf_2d buf;
            const int row_offset_y = row * block_h;
            const int col_offset_y = col * block_w;

            buf.buf = (uint8_t *)y_buffer + row_offset_y * y_stride + col_offset_y;
            buf.stride = y_stride;

            if (use_highbitdepth) {
                var += av1_high_get_sby_perpixel_variance(&buf, block_size, bit_depth);
            } else {
                var += av1_get_sby_perpixel_variance(&buf, block_size);
            }
            var_count += 1.0;
        }
    }
    var /= var_count;
    return var;
}

// A callback function used to pass data to VMAF.
// Returns 0 after reading a frame.
// Returns 2 when there is no more frame to read.
static int read_frame(float *ref_data, float *main_data, float *temp_data, int stride,
                      void *user_data)
{
    FrameData *frames = (FrameData *)user_data;

    if (!frames->frame_set) {
        const int width = frames->source->y_width;
        const int height = frames->source->y_height;
        ASSERT(width == frames->distorted->y_width);
        ASSERT(height == frames->distorted->y_height);

        if (0) {
            const float scale_factor = 1.0f / (float)(1 << (frames->bit_depth - 8));
            uint16_t *ref_ptr = (uint16_t *)(frames->source->y_buffer);
            uint16_t *main_ptr = (uint16_t *)(frames->distorted->y_buffer);

            for (int row = 0; row < height; ++row) {
                for (int col = 0; col < width; ++col) {
                    ref_data[col] = scale_factor * (float)ref_ptr[col];
                }
                ref_ptr += frames->source->y_stride;
                ref_data += stride / sizeof(*ref_data);
            }

            for (int row = 0; row < height; ++row) {
                for (int col = 0; col < width; ++col) {
                    main_data[col] = scale_factor * (float)main_ptr[col];
                }
                main_ptr += frames->distorted->y_stride;
                main_data += stride / sizeof(*main_data);
            }
        } else {
            uint8_t *ref_ptr = frames->source->y_buffer;
            uint8_t *main_ptr = frames->distorted->y_buffer;

            for (int row = 0; row < height; ++row) {
                for (int col = 0; col < width; ++col) {
                    ref_data[col] = (float)ref_ptr[col];
                }
                ref_ptr += frames->source->y_stride;
                ref_data += stride / sizeof(*ref_data);
            }

            for (int row = 0; row < height; ++row) {
                for (int col = 0; col < width; ++col) {
                    main_data[col] = (float)main_ptr[col];
                }
                main_ptr += frames->distorted->y_stride;
                main_data += stride / sizeof(*main_data);
            }
        }
        frames->frame_set = 1;
        return 0;
    }

    (void)temp_data;
    return 2;
}

void aom_calc_vmaf(const char *model_path, const YV12_BUFFER_CONFIG *source,
                   const YV12_BUFFER_CONFIG *distorted, const int bit_depth, double *const vmaf)
{
    const int width = source->y_width;
    const int height = source->y_height;
    FrameData frames = {source, distorted, 0, bit_depth};
    char *fmt = bit_depth == 10 ? "yuv420p10le" : "yuv420p";
    double vmaf_score;
    const int ret = compute_vmaf(&vmaf_score, fmt, width, height, read_frame,
                                 /* user_data=*/&frames, (char *)model_path,
                                 /* log_path=*/NULL, /* log_fmt=*/NULL, /* disable_clip=*/1,
                                 /* disable_avx=*/0, /* enable_transform=*/0,
                                 /* phone_model=*/0, /* do_psnr=*/0, /* do_ssim=*/0,
                                 /* do_ms_ssim=*/0, /* pool_method=*/NULL, /* n_thread=*/0,
                                 /* n_subsample=*/1, /* enable_conf_interval=*/0);

    *vmaf = vmaf_score;
}

const char *vmaf_model_path = MODEL_PATH(.pkl);
static double cal_approx_vmaf(int bit_depth, double source_variance,
                              YV12_BUFFER_CONFIG *const source, YV12_BUFFER_CONFIG *const sharpened)
{
    double new_vmaf;
    aom_calc_vmaf(vmaf_model_path, source, sharpened, bit_depth, &new_vmaf);
    const double sharpened_var = frame_average_variance(bit_depth, 0, sharpened);
    return source_variance / sharpened_var * (new_vmaf - kBaselineVmaf);
}

static double find_best_frame_unsharp_amount_loop(int bit_depth, YV12_BUFFER_CONFIG *const source,
                                                  YV12_BUFFER_CONFIG *const blurred,
                                                  YV12_BUFFER_CONFIG *const sharpened,
                                                  double best_vmaf, const double baseline_variance,
                                                  const double unsharp_amount_start,
                                                  const double step_size, const int max_loop_count,
                                                  const double max_amount)
{
    const double min_amount = 0.0;
    int loop_count = 0;
    double approx_vmaf = best_vmaf;
    double unsharp_amount = unsharp_amount_start;
    do {
        best_vmaf = approx_vmaf;
        unsharp_amount += step_size;
        if (unsharp_amount > max_amount || unsharp_amount < min_amount)
            break;
        unsharp(bit_depth, 0, source, blurred, sharpened, unsharp_amount);
        approx_vmaf = cal_approx_vmaf(bit_depth, baseline_variance, source, sharpened);

        loop_count++;
    } while (approx_vmaf > best_vmaf && loop_count < max_loop_count);
    unsharp_amount = approx_vmaf > best_vmaf ? unsharp_amount : unsharp_amount - step_size;
    return MIN(max_amount, MAX(unsharp_amount, min_amount));
}

void vmaf_alloc_frame_buffer(const void *encoder, YV12_BUFFER_CONFIG *frame, int width, int height)
{
    const int border = 64;
    unsigned int luma_stride, chroma_stride;
    VCEncGetAlignedStride(width + 2 * border, VCENC_YUV420_PLANAR, &luma_stride, &chroma_stride, 0);
    int lumaSize = luma_stride * (height + border * 2);
    int chromaSize = chroma_stride * (height + border * 2) / 2 * 2;
    const void *ewl_inst = VCEncGetEwl(encoder);
    frame->mem.mem_type = VPU_WR | VPU_RD | EWL_MEM_TYPE_VPU_WORKING;
    EWLMallocLinear(ewl_inst, lumaSize + chromaSize, 0, &frame->mem);
    frame->y_buffer = (uint8_t *)frame->mem.virtualAddress;
    frame->u_buffer = frame->y_buffer + lumaSize;
    frame->v_buffer = frame->u_buffer + chromaSize / 2;
    frame->y_buffer += border * luma_stride + border;
    frame->u_buffer += border / 2 * chroma_stride + border / 2;
    frame->v_buffer += border / 2 * chroma_stride + border / 2;
    frame->y_width = width;
    frame->uv_width = width / 2;
    frame->y_height = height;
    frame->uv_height = height / 2;
    frame->y_stride = luma_stride;
    frame->uv_stride = chroma_stride;
    frame->border = border;
}
void vmaf_free_frame_buffer(const void *encoder, YV12_BUFFER_CONFIG *frame)
{
    const void *ewl_inst = VCEncGetEwl(encoder);
    EWLFreeLinear(ewl_inst, &frame->mem);
}

static double find_best_frame_unsharp_amount(const void *encoder, int bit_depth,
                                             YV12_BUFFER_CONFIG *const source,
                                             YV12_BUFFER_CONFIG *const blurred,
                                             YV12_BUFFER_CONFIG *const sharpened,
                                             const double unsharp_amount_start,
                                             const double step_size, const int max_loop_count,
                                             const double max_filter_amount)
{
    const int width = source->y_width;
    const int height = source->y_height;

    const double baseline_variance = frame_average_variance(bit_depth, 1, source);
    double unsharp_amount;
    if (unsharp_amount_start <= step_size) {
        unsharp_amount = find_best_frame_unsharp_amount_loop(bit_depth, source, blurred, sharpened,
                                                             0.0, baseline_variance, 0.0, step_size,
                                                             max_loop_count, max_filter_amount);
    } else {
        double a0 = unsharp_amount_start - step_size, a1 = unsharp_amount_start;
        double v0, v1;
        unsharp(bit_depth, 0, source, blurred, sharpened, a0);
        v0 = cal_approx_vmaf(bit_depth, baseline_variance, source, sharpened);
        unsharp(bit_depth, 0, source, blurred, sharpened, a1);
        v1 = cal_approx_vmaf(bit_depth, baseline_variance, source, sharpened);
        if (fabs(v0 - v1) < 0.01) {
            unsharp_amount = a0;
        } else if (v0 > v1) {
            unsharp_amount = find_best_frame_unsharp_amount_loop(
                bit_depth, source, blurred, sharpened, v0, baseline_variance, a0, -step_size,
                max_loop_count, max_filter_amount);
        } else {
            unsharp_amount = find_best_frame_unsharp_amount_loop(
                bit_depth, source, blurred, sharpened, v1, baseline_variance, a1, step_size,
                max_loop_count, max_filter_amount);
        }
    }

    return unsharp_amount;
}
void vmaf_frame_preprocessing(const void *pEncInst, const int bit_depth, u8 *lum, u8 *cb, u8 *cr,
                              int width, int height, VMAFCtrl *ctl)
{
    YV12_BUFFER_CONFIG source;
    unsigned int luma_stride, chroma_stride;
    VCEncGetAlignedStride(width, VCENC_YUV420_PLANAR, &luma_stride, &chroma_stride, 0);
    source.y_buffer = lum;
    int lumaSize = luma_stride * height;
    int chromaSize = chroma_stride * height / 2 * 2;
    source.u_buffer = cb;
    source.v_buffer = cr;
    source.y_width = width;
    source.y_height = height;
    source.uv_width = width / 2;
    source.uv_height = height / 2;
    source.y_stride = luma_stride;
    source.uv_stride = chroma_stride;
    source.border = 0;

    av1_copy_and_extend_frame(&source, &ctl->source_extended);
    gaussian_blur(bit_depth, &ctl->source_extended, &ctl->blurred);

    const double best_frame_unsharp_amount =
        find_best_frame_unsharp_amount(pEncInst, bit_depth, &source, &ctl->blurred, &ctl->sharpened,
                                       ctl->last_frame_unsharp_amount, 0.05, 20, 1.01);
    ctl->last_frame_unsharp_amount = best_frame_unsharp_amount;

    unsharp(bit_depth, 0, &source, &ctl->blurred, &source, best_frame_unsharp_amount);
}
