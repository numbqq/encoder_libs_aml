#ifndef _INCLUDED_COM_VIDEOPHONE_CODEC
#define _INCLUDED_COM_VIDEOPHONE_CODEC

#ifdef __cplusplus
extern "C" {
#endif

#define vl_codec_handle_t long

typedef enum vl_codec_id_e {
	CODEC_ID_NONE,
	CODEC_ID_VP8,
	CODEC_ID_H261,
    CODEC_ID_H263,
	CODEC_ID_H264, /* ����֧�� */
	CODEC_ID_H265,

}vl_codec_id_t;

typedef enum vl_img_format_e {
	IMG_FMT_NONE,
	IMG_FMT_NV12, /* ����֧�� */

}vl_img_format_t;

typedef enum vl_frame_type_e {
	FRAME_TYPE_NONE,
	FRAME_TYPE_AUTO, /* �������Զ����ƣ�Ĭ�ϣ� */
	FRAME_TYPE_IDR,
	FRAME_TYPE_I,
	FRAME_TYPE_P,

}vl_frame_type_t;

/**
 * ��ȡ�汾��Ϣ
 *
 *@return : �汾��Ϣ
 */
const char * vl_get_version();

/**
 * ��ʼ����Ƶ������
 *
 *@param : codec_id ��������
 *@param : width ��Ƶ���
 *@param : height ��Ƶ�߶�
 *@param : frame_rate ֡��
 *@param : bit_rate ����
 *@param : gop GOPֵ:I֡�����
 *@param : img_format ͼ���ʽ
 *@return : �ɹ����ر�����handle��ʧ�ܷ��� <= 0
 */
vl_codec_handle_t vl_video_encoder_init(vl_codec_id_t codec_id, int width, int height, int frame_rate, int bit_rate, int gop, vl_img_format_t img_format);

/**
 * ��Ƶ����
 *
 *@param : handle ������handle
 *@param : type ֡����
 *@param : in �����������
 *@param : in_size ����������ݳ���
 *@param : out ����������,H.264��Ҫ����(0x00��0x00��0x00��0x01)��ʼ�룬 �ұ���Ϊ I420��ʽ����out�Ŀռ���apk����䣬ͨ��jni���ݽ�����ֱ���ں������޸�out�����ݣ���Ҫ�޸�outָ��ĵ�ַ��
 *@return ���ɹ����ر��������ݳ��ȣ�ʧ�ܷ��� <= 0
 */
int vl_video_encoder_encode(vl_codec_handle_t handle, vl_frame_type_t type, char * in, int in_size, char ** out);

/**
 * ���ٱ�����
 *
 *@param ��handle ��Ƶ������handle
 *@return ���ɹ�����1��ʧ�ܷ���0
 */
int vl_video_encoder_destory(vl_codec_handle_t handle);

/**
 * ��ʼ��������
 *
 *@param : codec_id ����������
 *@return : �ɹ����ؽ�����handle��ʧ�ܷ��� <= 0
 */
vl_codec_handle_t vl_video_decoder_init(vl_codec_id_t codec_id);

/**
 * ��Ƶ����
 *
 *@param : handle ��Ƶ������handle
 *@param : in �����������
 *@param : in_size ����������ݳ���
 *@param : out ���������ݣ��ڲ�����ռ�
 *@return ���ɹ����ؽ��������ݳ��ȣ�ʧ�ܷ��� <= 0
 */
int vl_video_decoder_decode(vl_codec_handle_t handle, char * in, int in_size, char ** out);

/**
 * ���ٽ�����
 *@param : handle ��Ƶ������handle
 *@return ���ɹ�����1��ʧ�ܷ���0
 */
int vl_video_decoder_destory(vl_codec_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif /* _INCLUDED_COM_VIDEOPHONE_CODEC */

