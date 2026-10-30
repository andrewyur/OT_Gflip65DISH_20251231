
#ifndef __SSPM_IPI_TABLE_H__
#define __SSPM_IPI_TABLE_H__

#include "sspm_ipi_id.h"
#include "sspm_mbox_pin.h"

struct mtk_mbox_info sspm_mbox_table[SSPM_MBOX_TOTAL] = {
	{0, 0, 0, 32, 0, 1, 0, 0, 0, 0, 0, 0, 0, { { { { 0 } } } } },
	{0, 0, 1, 32, 0, 1, 0, 0, 0, 0, 0, 0, 0, { { { { 0 } } } } },
	{0, 0, 2, 32, 0, 1, 0, 0, 0, 0, 0, 0, 0, { { { { 0 } } } } },
	{0, 0, 3, 32, 0, 1, 0, 0, 0, 0, 0, 0, 0, { { { { 0 } } } } },
	{0, 0, 4, 32, 0, 1, 0, 0, 0, 0, 0, 0, 0, { { { { 0 } } } } },
};

struct mtk_mbox_pin_send sspm_mbox_pin_send[] = {
/* the following will use mbox 0 */
	{0, IPIS_C_PPM_OUT_OFFSET, 1, 0, IPIS_C_PPM_OUT_SIZE,
	 IPIS_C_PPM_OUT_OFFSET, IPIS_C_PPM,
	 { { 0 } }, { 0 }, { { { { 0 } } } } },
	{0, IPIS_C_QOS_OUT_OFFSET, 1, 0, IPIS_C_QOS_OUT_SIZE,
	 IPIS_C_QOS_OUT_OFFSET, IPIS_C_QOS,
	 { { 0 } }, { 0 }, { { { { 0 } } } } },
	{0, IPIS_C_PMIC_OUT_OFFSET, 1, 0, IPIS_C_PMIC_OUT_SIZE,
	 IPIS_C_PMIC_OUT_OFFSET, IPIS_C_PMIC,
	 { { 0 } }, { 0 }, { { { { 0 } } } } },
	{0, IPIS_C_MET_OUT_OFFSET, 1, 0, IPIS_C_MET_OUT_SIZE,
	 IPIS_C_MET_OUT_OFFSET, IPIS_C_MET,
	 { { 0 } }, { 0 }, { { { { 0 } } } } },
	{0, IPIS_C_THERMAL_OUT_OFFSET, 1, 0, IPIS_C_THERMAL_OUT_SIZE,
	 IPIS_C_THERMAL_OUT_OFFSET, IPIS_C_THERMAL,
	 { { 0 } }, { 0 }, { { { { 0 } } } } },
	{0, IPIS_C_GPU_DVFS_OUT_OFFSET, 1, 0, IPIS_C_GPU_DVFS_OUT_SIZE,
	 IPIS_C_GPU_DVFS_OUT_OFFSET, IPIS_C_GPU_DVFS,
	 { { 0 } }, { 0 }, { { { { 0 } } } } },
	{0, IPIS_C_GPU_PM_OUT_OFFSET, 1, 0, IPIS_C_GPU_PM_OUT_SIZE,
	 IPIS_C_GPU_PM_OUT_OFFSET, IPIS_C_GPU_PM,
	 { { 0 } }, { 0 }, { { { { 0 } } } } },
/* the following will use mbox 1 */
	{1, IPIS_C_PLATFORM_OUT_OFFSET, 1, 0, IPIS_C_PLATFORM_OUT_SIZE,
	 IPIS_C_PLATFORM_OUT_OFFSET, IPIS_C_PLATFORM,
	 { { 0 } }, { 0 }, { { { { 0 } } } } },
	{1, IPIS_C_SMI_OUT_OFFSET, 1, 0, IPIS_C_SMI_OUT_SIZE,
	 IPIS_C_SMI_OUT_OFFSET, IPIS_C_SMI,
	 { { 0 } }, { 0 }, { { { { 0 } } } } },
	{1, IPIS_C_CM_OUT_OFFSET, 1, 0, IPIS_C_CM_OUT_SIZE,
	 IPIS_C_CM_OUT_OFFSET, IPIS_C_CM,
	 { { 0 } }, { 0 }, { { { { 0 } } } } },
	{1, IPIS_C_SLBC_OUT_OFFSET, 1, 0, IPIS_C_SLBC_OUT_SIZE,
	 IPIS_C_SLBC_OUT_OFFSET, IPIS_C_SLBC,
	 { { 0 } }, { 0 }, { { { { 0 } } } } },
	{1, IPIS_C_SPM_SUSPEND_OUT_OFFSET, 1, 0, IPIS_C_SPM_SUSPEND_OUT_SIZE,
	 IPIS_C_SPM_SUSPEND_OUT_OFFSET, IPIS_C_SPM_SUSPEND,
	 { { 0 } }, { 0 }, { { { { 0 } } } } },
	{1, IPIR_C_MET_OUT_OFFSET, 1, 0, IPIR_C_MET_OUT_SIZE,
	 IPIR_C_MET_OUT_OFFSET, IPIR_C_MET,
	 { { 0 } }, { 0 }, { { { { 0 } } } } },
	{1, IPIR_C_GPU_DVFS_OUT_OFFSET, 1, 0, IPIR_C_GPU_DVFS_OUT_SIZE,
	 IPIR_C_GPU_DVFS_OUT_OFFSET, IPIR_C_GPU_DVFS,
	 { { 0 } }, { 0 }, { { { { 0 } } } } },
	{1, IPIR_C_PLATFORM_OUT_OFFSET, 1, 0, IPIR_C_PLATFORM_OUT_SIZE,
	 IPIR_C_PLATFORM_OUT_OFFSET, IPIR_C_PLATFORM,
	 { { 0 } }, { 0 }, { { { { 0 } } } } },
	{1, IPIR_C_SLBC_OUT_OFFSET, 1, 0, IPIR_C_SLBC_OUT_SIZE,
	 IPIR_C_SLBC_OUT_OFFSET, IPIR_C_SLBC,
	 { { 0 } }, { 0 }, { { { { 0 } } } } },
};

struct mtk_mbox_pin_recv sspm_mbox_pin_recv[] = {
/* the following will use mbox 2 */
	{2, IPIR_I_QOS_IN_OFFSET, 0, 0, 1, 1,
	 IPIR_I_QOS_IN_SIZE, IPIR_I_QOS_IN_OFFSET,
	 IPIR_I_QOS, { 0 }, 0, 0, 0, { { { { 0 } } } },
	 { 0, 0, 0, 0, 0, 0 } },
	{2, IPIR_C_MET_IN_OFFSET, 0, 0, 1, 1,
	 IPIR_C_MET_IN_SIZE, IPIR_C_MET_IN_OFFSET,
	 IPIR_C_MET, { 0 }, 0, 0, 0, { { { { 0 } } } },
	 { 0, 0, 0, 0, 0, 0 } },
	{2, IPIR_C_GPU_DVFS_IN_OFFSET, 0, 0, 1, 1,
	 IPIR_C_GPU_DVFS_IN_SIZE, IPIR_C_GPU_DVFS_IN_OFFSET,
	 IPIR_C_GPU_DVFS, { 0 }, 0, 0, 0, { { { { 0 } } } },
	 { 0, 0, 0, 0, 0, 0 } },
	{2, IPIR_C_PLATFORM_IN_OFFSET, 0, 0, 1, 1,
	 IPIR_C_PLATFORM_IN_SIZE, IPIR_C_PLATFORM_IN_OFFSET,
	 IPIR_C_PLATFORM, { 0 }, 0, 0, 0, { { { { 0 } } } },
	 { 0, 0, 0, 0, 0, 0 } },
	{2, IPIR_C_SLBC_IN_OFFSET, 0, 0, 1, 1,
	 IPIR_C_SLBC_IN_SIZE, IPIR_C_SLBC_IN_OFFSET,
	 IPIR_C_SLBC, { 0 }, 0, 0, 0, { { { { 0 } } } },
	 { 0, 0, 0, 0, 0, 0 } },
	{2, IPIS_C_PPM_IN_OFFSET, 1, 0, 1, 0,
	 IPIS_C_PPM_IN_SIZE, IPIS_C_PPM_IN_OFFSET,
	 IPIS_C_PPM, { 0 }, 0, 0, 0, { { { { 0 } } } },
	 { 0, 0, 0, 0, 0, 0 } },
	{2, IPIS_C_QOS_IN_OFFSET, 1, 0, 1, 0,
	 IPIS_C_QOS_IN_SIZE, IPIS_C_QOS_IN_OFFSET,
	 IPIS_C_QOS, { 0 }, 0, 0, 0, { { { { 0 } } } },
	 { 0, 0, 0, 0, 0, 0 } },
	{2, IPIS_C_PMIC_IN_OFFSET, 1, 0, 1, 0,
	 IPIS_C_PMIC_IN_SIZE, IPIS_C_PMIC_IN_OFFSET,
	 IPIS_C_PMIC, { 0 }, 0, 0, 0, { { { { 0 } } } },
	 { 0, 0, 0, 0, 0, 0 } },
	{2, IPIS_C_MET_IN_OFFSET, 1, 0, 1, 0,
	 IPIS_C_MET_IN_SIZE, IPIS_C_MET_IN_OFFSET,
	 IPIS_C_MET, { 0 }, 0, 0, 0, { { { { 0 } } } },
	 { 0, 0, 0, 0, 0, 0 } },
	{2, IPIS_C_THERMAL_IN_OFFSET, 1, 0, 1, 0,
	 IPIS_C_THERMAL_IN_SIZE, IPIS_C_THERMAL_IN_OFFSET,
	 IPIS_C_THERMAL, { 0 }, 0, 0, 0, { { { { 0 } } } },
	 { 0, 0, 0, 0, 0, 0 } },
	{2, IPIS_C_GPU_DVFS_IN_OFFSET, 1, 0, 1, 0,
	 IPIS_C_GPU_DVFS_IN_SIZE, IPIS_C_GPU_DVFS_IN_OFFSET,
	 IPIS_C_GPU_DVFS, { 0 }, 0, 0, 0, { { { { 0 } } } },
	 { 0, 0, 0, 0, 0, 0 } },
	{2, IPIS_C_GPU_PM_IN_OFFSET, 1, 0, 1, 0,
	 IPIS_C_GPU_PM_IN_SIZE, IPIS_C_GPU_PM_IN_OFFSET,
	 IPIS_C_GPU_PM, { 0 }, 0, 0, 0, { { { { 0 } } } },
	 { 0, 0, 0, 0, 0, 0 } },
	{2, IPIS_C_PLATFORM_IN_OFFSET, 1, 0, 1, 0,
	 IPIS_C_PLATFORM_IN_SIZE, IPIS_C_PLATFORM_IN_OFFSET,
	 IPIS_C_PLATFORM, { 0 }, 0, 0, 0, { { { { 0 } } } },
	 { 0, 0, 0, 0, 0, 0 } },
	{2, IPIS_C_SMI_IN_OFFSET, 1, 0, 1, 0,
	 IPIS_C_SMI_IN_SIZE, IPIS_C_SMI_IN_OFFSET,
	 IPIS_C_SMI, { 0 }, 0, 0, 0, { { { { 0 } } } },
	 { 0, 0, 0, 0, 0, 0 } },
	{2, IPIS_C_CM_IN_OFFSET, 1, 0, 1, 0,
	 IPIS_C_CM_IN_SIZE, IPIS_C_CM_IN_OFFSET,
	 IPIS_C_CM, { 0 }, 0, 0, 0, { { { { 0 } } } },
	 { 0, 0, 0, 0, 0, 0 } },
	{2, IPIS_C_SLBC_IN_OFFSET, 1, 0, 1, 0,
	 IPIS_C_SLBC_IN_SIZE, IPIS_C_SLBC_IN_OFFSET,
	 IPIS_C_SLBC, { 0 }, 0, 0, 0, { { { { 0 } } } },
	 { 0, 0, 0, 0, 0, 0 } },
	{2, IPIS_C_SPM_SUSPEND_IN_OFFSET, 1, 0, 1, 0,
	 IPIS_C_SPM_SUSPEND_IN_SIZE, IPIS_C_SPM_SUSPEND_IN_OFFSET,
	 IPIS_C_SPM_SUSPEND, { 0 }, 0, 0, 0, { { { { 0 } } } },
	 { 0, 0, 0, 0, 0, 0 } },
};

#define SSPM_TOTAL_SEND_PIN	(sizeof(sspm_mbox_pin_send) \
				 / sizeof(struct mtk_mbox_pin_send))
#define SSPM_TOTAL_RECV_PIN	(sizeof(sspm_mbox_pin_recv) \
				 / sizeof(struct mtk_mbox_pin_recv))

struct mtk_mbox_device sspm_mboxdev = {
	.name = "sspm_mboxdev",
	.pin_recv_table = &sspm_mbox_pin_recv[0],
	.pin_send_table = &sspm_mbox_pin_send[0],
	.info_table = &sspm_mbox_table[0],
	.count = SSPM_MBOX_TOTAL,
	.recv_count = SSPM_TOTAL_RECV_PIN,
	.send_count = SSPM_TOTAL_SEND_PIN,
};

extern void sspm_ipi_timeout_cb(int ipi_id);
struct mtk_ipi_device sspm_ipidev = {
	.name = "sspm_ipidev",
	.id = IPI_DEV_SSPM,
	.mbdev = &sspm_mboxdev,
	.timeout_handler = sspm_ipi_timeout_cb,
};

#endif /* __SSPM_IPI_TABLE_H__ */
