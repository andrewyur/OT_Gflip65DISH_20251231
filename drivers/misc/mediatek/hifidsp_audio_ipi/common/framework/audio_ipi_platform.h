
#ifndef __AUDIO_IPI_PLATFORM_H__
#define __AUDIO_IPI_PLATFORM_H__

#include <linux/types.h>

enum opendsp_id {
	AUDIO_OPENDSP_USE_CM4_A, /* => SCP_A_ID */
	AUDIO_OPENDSP_USE_CM4_B, /* => SCP_B_ID */
	AUDIO_OPENDSP_USE_HIFI3, /* => ADSP_A_ID */
	AUDIO_OPENDSP_USE_HIFI4,
	NUM_OPENDSP_TYPE,
	AUDIO_OPENDSP_ID_INVALID
};

bool audio_opendsp_id_ready(const uint8_t opendsp_id);

bool audio_opendsp_ready(const uint8_t task);
uint32_t audio_get_opendsp_id(const uint8_t task);
uint32_t audio_get_ipi_id(const uint8_t task);

#endif /*__AUDIO_IPI_PLATFORM_H__ */
