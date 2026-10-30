
#include "vpu_cfg.h"
#include "vpu_cmn.h"
#include "mtk_devinfo.h"
#include "vpu_debug.h"
#include <memory/mediatek/emi.h>

void vpu_emi_mpu_set(unsigned long start, unsigned int size)
{
#ifdef CONFIG_MEDIATEK_EMI
	struct emimpu_region_t md_region;

	mtk_emimpu_init_region(&md_region, MPU_PROCT_REGION);
	mtk_emimpu_set_addr(&md_region, start,
			    (start + (unsigned long)size) - 0x1);
	mtk_emimpu_set_apc(&md_region, MPU_PROCT_D0_AP,
			   MTK_EMIMPU_NO_PROTECTION);
	mtk_emimpu_set_apc(&md_region, MPU_PROCT_D5_APUSYS,
			   MTK_EMIMPU_NO_PROTECTION);
	mtk_emimpu_lock_region(&md_region, true);
	mtk_emimpu_set_protection(&md_region);
	mtk_emimpu_free_region(&md_region);
#endif
}

bool vpu_is_disabled(struct vpu_device *vd)
{
	bool ret;
	unsigned int efuse;
	unsigned int seg;
	unsigned int mask;

	mask = 1 << vd->id;

	seg = get_devinfo_with_index(EFUSE_SEG_OFFSET);
	efuse = get_devinfo_with_index(EFUSE_VPU_OFFSET);
	efuse = (efuse >> EFUSE_VPU_SHIFT) & EFUSE_VPU_MASK;
	/* disabled by mask, or disabled by segment */
	ret = (efuse & mask) || ((seg == 0x1) && (vd->id >= 2));

	/* show efuse info to let user know */
	pr_info("%s: seg: 0x%x, efuse: 0x%x, core%d is %s\n",
		__func__, seg, efuse, vd->id,
		ret ? "disabled" : "enabled");

	return ret;
}

