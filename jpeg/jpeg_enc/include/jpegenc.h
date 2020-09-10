#ifndef _JPEGENC_HEADER_
#define _JPEGENC_HEADER_

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t u32;

#include "jpegenc_api.h"

typedef struct {
	unsigned char* addr;
	unsigned size;
} jpegenc_buff_t;


typedef struct hw_jpegenc_s {
	uint8_t* src;
	uint32_t src_size;
	uint8_t* dst;
	uint32_t dst_size;
	int quality;
	int qtbl_id;
	uint32_t width;
	uint32_t height;
	int bpp;
	uint32_t canvas;
	enum jpegenc_mem_type_e type;
	enum jpegenc_frame_fmt_e in_format;
	enum jpegenc_frame_fmt_e out_format;
	size_t jpeg_size;
	int timeout;
	int block_mode;

#ifdef simulation
	int input_size;
#endif

	int dev_fd;
	int dma_fd;
	jpegenc_buff_t mmap_buff;
	jpegenc_buff_t input_buf;
	jpegenc_buff_t assit_buf;
	jpegenc_buff_t output_buf;

	u32 y_off;
	u32 u_off;
	u32 v_off;

	u32 y_stride;
	u32 u_stride;
	u32 v_stride;

	u32 h_stride;
} hw_jpegenc_t;

long hw_encode_init(int timeout);
int hw_encode(jpegenc_handle_t handle, uint8_t *src, uint8_t *dst, enum jpegenc_mem_type_e mem_type, int dma_fd, uint32_t width, uint32_t height, int w_stride, int h_stride, int quality,
		uint8_t format, uint8_t oformat, uint32_t* datalen);
int hw_encode_uninit(jpegenc_handle_t handle);


#ifdef __cplusplus
}
#endif

#endif
