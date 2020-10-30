#ifndef _JPEGENC_API_HEADER_
#define _JPEGENC_API_HEADER_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#define jpegenc_handle_t long

enum jpegenc_mem_type_e {
	JPEGENC_LOCAL_BUFF = 0,
	JPEGENC_CANVAS_BUFF,
	JPEGENC_PHYSICAL_BUFF,
	JPEGENC_DMA_BUFF,
	JPEGENC_MAX_BUFF_TYPE
};

enum jpegenc_frame_fmt_e {
	FMT_YUV422_SINGLE = 0,
	FMT_YUV444_SINGLE,
	FMT_NV21,
	FMT_NV12,
	FMT_YUV420,
	FMT_YUV444_PLANE,
	FMT_RGB888,
	FMT_RGB888_PLANE,
	FMT_RGB565,
	FMT_RGBA8888,
	MAX_FRAME_FMT
};

/**
 * init jpeg encoder
 *
 *@return : opaque handle for jpeg encoder if success, 0 if failed
 */
jpegenc_handle_t jpegenc_init();

/**
 * encode yuv frame with jpeg encoder
 *
 *@param : handle: opaque handle for jpeg encoder to return
 *@param : width: video width
 *@param : height: video height
 *@param : w_stride: stride for width
 *@param : h_stride: stride for height
 *@param : quality: jpeg quality scale value
 *@param : iformat: input yuv format
 *@param : oformat: output for jpeg
 *@param : mem_type: memory buffer type for input yuv
 *@param : dma_fd: if mem_type is dma, this is the shared dma fd
 *@param : in_buf: if mem_type is vmalloc, this is the memory buffer addr for yuv input
 *@param : out_buf: memory buffer addr for output jpeg file data
 *@return : the length of encoded jpge file data, 0 if encoding failed
 */
int jpegenc_encode(jpegenc_handle_t handle, int width, int height, int w_stride, int h_stride, int quality,
		enum jpegenc_frame_fmt_e iformat, enum jpegenc_frame_fmt_e oformat, enum jpegenc_mem_type_e mem_type, int dma_fd, uint8_t *in_buf,
		uint8_t *out_buf);

/**
 * release jpeg encoder
 *
 *@param : handle: opaque handle for jpeg encoder to return
 *@return : 0 if succes, errcode otherwise
 */
int jpegenc_destroy(jpegenc_handle_t handle);


#ifdef __cplusplus
}
#endif


#endif
