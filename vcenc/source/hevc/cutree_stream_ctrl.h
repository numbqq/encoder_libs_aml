#include "sw_cu_tree.h"

/**
 * @brief cuTreeChangeResolution(), cutree change to the new resolution.
 * @param struct vcenc_instance *vcenc_instance: encoder instance.
 * @param struct cuTreeCtr *m_param: cutree params.
 * @return VCEncRet: VCENC_OK successful, other unsuccessful.
 */
#ifndef CUTREE_BUILD_SUPPORT
#define cuTreeChangeResolution(vcenc_instance, m_param) (0)
#else
VCEncRet cuTreeChangeResolution(struct vcenc_instance *vcenc_instance, struct cuTreeCtr *m_param);
#endif