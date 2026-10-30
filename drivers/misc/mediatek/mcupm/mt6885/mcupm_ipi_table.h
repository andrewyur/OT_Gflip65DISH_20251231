
#ifndef __MCUPM_IPI_TABLE_H__
#define __MCUPM_IPI_TABLE_H__

#include <mt-plat/mtk_tinysys_ipi.h>
#include "mcupm_ipi_id.h"

#define MCUPM_MBOX_TOTAL 5

/*share memory start address defination*/
#define SMEM_SIZE_80B			0x00000014      //80 Bytes
#define PIN_S_SIZE			SMEM_SIZE_80B
#define PIN_R_SIZE			SMEM_SIZE_80B
#define MBOX_TABLE_SIZE			(PIN_S_SIZE + PIN_R_SIZE)
#define SMEM0_SET_IRQ_REG		SW_INT_SET
#define SMEM0_CLR_IRQ_REG		SW_INT_CLR

/* definition of slot offset for send PINs */
#define PIN_S_OFFSET_PLATFORM		0
#define PIN_S_OFFSET_CPU_DVFS		0
#define PIN_S_OFFSET_FHCTL		0
#define PIN_S_OFFSET_MCDI		0
#define PIN_S_OFFSET_SUSPEND		0

#define PIN_S_MSG_SIZE_PLATFORM		3	//uint 4 byts
#define PIN_S_MSG_SIZE_CPU_DVFS		4	//uint 4 byts
#define PIN_S_MSG_SIZE_FHCTL		9	//uint 4 byts
#define PIN_S_MSG_SIZE_MCDI		3	//uint 4 byts
#define PIN_S_MSG_SIZE_SUSPEND		3	//uint 4 byts


/* definition of slot size for send PINs */
#define PIN_S_SIZE_PLATFORM	PIN_S_SIZE
#define PIN_S_SIZE_CPU_DVFS	PIN_S_SIZE
#define PIN_S_SIZE_FHCTL	PIN_S_SIZE
#define PIN_S_SIZE_MCDI	PIN_S_SIZE
#define PIN_S_SIZE_SUSPEND	PIN_S_SIZE

#define PIN_R_MSG_SIZE_PLATFORM	1	//uint 4 byts
#define PIN_R_MSG_SIZE_CPU_DVFS	4	//uint 4 byts
#define PIN_R_MSG_SIZE_FHCTL	1	//uint 4 byts
#define PIN_R_MSG_SIZE_MCDI	1	//uint 4 byts
#define PIN_R_MSG_SIZE_SUSPEND	1	//uint 4 byts

/* definition of slot size for received PINs */
#define PIN_R_SIZE_PLATFORM	PIN_R_SIZE
#define PIN_R_SIZE_CPU_DVFS	PIN_R_SIZE
#define PIN_R_SIZE_FHCTL	PIN_R_SIZE
#define PIN_R_SIZE_MCDI	PIN_R_SIZE
#define PIN_R_SIZE_SUSPEND	PIN_R_SIZE

/* definition of slot offset for received PINs */
#define PIN_R_OFFSET_PLATFORM	(PIN_S_OFFSET_PLATFORM + PIN_S_SIZE_PLATFORM)
#define PIN_R_OFFSET_CPU_DVFS	(PIN_S_OFFSET_CPU_DVFS + PIN_S_SIZE_CPU_DVFS)
#define PIN_R_OFFSET_FHCTL	(PIN_S_OFFSET_FHCTL + PIN_S_SIZE_FHCTL)
#define PIN_R_OFFSET_MCDI	(PIN_S_OFFSET_MCDI + PIN_S_SIZE_MCDI)
#define PIN_R_OFFSET_SUSPEND	(PIN_S_OFFSET_SUSPEND + PIN_R_SIZE_SUSPEND)

extern struct mtk_ipi_device mcupm_ipidev;

struct mtk_mbox_info mcupm_mbox_table[MCUPM_MBOX_TOTAL] = {
	{0, 0, 0, MBOX_TABLE_SIZE, 1, 1, 0, 0, 0, 0, 0, 0, 0,
		{ { { { 0 } } } }, {0, 0, 0} },
	{0, 0, 1, MBOX_TABLE_SIZE, 1, 1, 0, 0, 0, 0, 0, 0, 0,
		{ { { { 0 } } } }, {0, 0, 0} },
	{0, 0, 2, MBOX_TABLE_SIZE, 1, 1, 0, 0, 0, 0, 0, 0, 0,
		{ { { { 0 } } } }, {0, 0, 0} },
	{0, 0, 3, MBOX_TABLE_SIZE, 1, 1, 0, 0, 0, 0, 0, 0, 0,
		{ { { { 0 } } } }, {0, 0, 0} },
	{0, 0, 4, MBOX_TABLE_SIZE, 1, 1, 0, 0, 0, 0, 0, 0, 0,
		{ { { { 0 } } } }, {0, 0, 0} },
};

struct mtk_mbox_pin_send mcupm_mbox_pin_send[] = {
	{0, PIN_S_OFFSET_PLATFORM, 1, 0, PIN_S_MSG_SIZE_PLATFORM,
		0, CH_S_PLATFORM,
		{ { 0 } }, { 0 }, { { { { 0 } } } } },
	{1, PIN_S_OFFSET_CPU_DVFS, 1, 0, PIN_S_MSG_SIZE_CPU_DVFS,
		1, CH_S_CPU_DVFS,
		{ { 0 } }, { 0 }, { { { { 0 } } } } },
	{2, PIN_S_OFFSET_FHCTL, 1, 0, PIN_S_MSG_SIZE_FHCTL,
		2, CH_S_FHCTL,
		{ { 0 } }, { 0 }, { { { { 0 } } } } },
	{3, PIN_S_OFFSET_MCDI, 1, 0, PIN_S_MSG_SIZE_MCDI,
		3, CH_S_MCDI,
		{ { 0 } }, { 0 }, { { { { 0 } } } } },
	{4, PIN_S_OFFSET_SUSPEND, 1, 0, PIN_S_MSG_SIZE_SUSPEND,
		4, CH_S_SUSPEND,
		{ { 0 } }, { 0 }, { { { { 0 } } } } },

};

struct mtk_mbox_pin_recv mcupm_mbox_pin_recv[] = {
	{0, PIN_R_OFFSET_PLATFORM, 1, 0, 1, 0,
		PIN_R_MSG_SIZE_PLATFORM, 0,
		CH_S_PLATFORM, { 0 }, 0, 0, 0, { { { { 0 } } } },
		{0, 0, 0, 0, 0, 0} },
	{1, PIN_R_OFFSET_CPU_DVFS, 0, 0, 1, 0,
		PIN_R_MSG_SIZE_CPU_DVFS, 1,
		CH_S_CPU_DVFS, { 0 }, 0, 0, 0, { { { { 0 } } } },
		{0, 0, 0, 0, 0, 0} },
	{2, PIN_R_OFFSET_FHCTL, 1, 0, 1, 0,
		PIN_R_MSG_SIZE_FHCTL, 2,
		CH_S_FHCTL, { 0 }, 0, 0, 0, { { { { 0 } } } },
		{0, 0, 0, 0, 0, 0} },
	{3, PIN_R_OFFSET_MCDI, 0, 0, 1, 0,
		PIN_R_MSG_SIZE_MCDI, 3,
		CH_S_MCDI, { 0 }, 0, 0, 0, { { { { 0 } } } },
		{0, 0, 0, 0, 0, 0} },
	{4, PIN_R_OFFSET_SUSPEND, 0, 0, 1, 0,
		PIN_R_MSG_SIZE_SUSPEND, 4,
		CH_S_SUSPEND, { 0 }, 0, 0, 0, { { { { 0 } } } },
		{0, 0, 0, 0, 0, 0} },
};

#define MCUPM_TOTAL_SEND_PIN     (sizeof(mcupm_mbox_pin_send) \
				  / sizeof(struct mtk_mbox_pin_send))
#define MCUPM_TOTAL_RECV_PIN     (sizeof(mcupm_mbox_pin_recv) \
				  / sizeof(struct mtk_mbox_pin_recv))

struct mtk_mbox_device mcupm_mboxdev = {
	.name = "mcupm_mboxdev",
	.pin_recv_table = &mcupm_mbox_pin_recv[0],
	.pin_send_table = &mcupm_mbox_pin_send[0],
	.info_table = &mcupm_mbox_table[0],
	.count = MCUPM_MBOX_TOTAL,
	.recv_count = MCUPM_TOTAL_RECV_PIN,
	.send_count = MCUPM_TOTAL_SEND_PIN,
};

extern void mcupm_ipi_timeout_cb(int ipi_id);
struct mtk_ipi_device mcupm_ipidev = {
	.name = "mcupm_ipidev",
	.id = IPI_DEV_MCUPM,
	.mbdev = &mcupm_mboxdev,
	.timeout_handler = mcupm_ipi_timeout_cb,
};

#endif
