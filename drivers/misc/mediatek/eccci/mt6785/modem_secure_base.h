
#ifndef __MODEM_SECURE_BASE_H__
#define __MODEM_SECURE_BASE_H__

#include <mt-plat/mtk_secure_api.h>

#define mdreg_write32(reg_id, value)		\
	mt_secure_call(MTK_SIP_KERNEL_CCCI_GET_INFO, reg_id, value, 0, 0)

#endif				/* __MODEM_SECURE_BASE_H__ */
