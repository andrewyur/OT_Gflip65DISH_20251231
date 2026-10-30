
#include <linux/kconfig.h>
#include <linux/module.h>
#include <linux/string.h>
#include "mrdump_private.h"
#ifdef CONFIG_MTK_DFD_INTERNAL_DUMP
#include <mtk_platform_debug.h>
#endif

#ifdef CONFIG_MTK_DBGTOP
#include <dbgtop.h>
#else
#ifdef CONFIG_MTK_WATCHDOG
#include <mtk_wd_api.h>
#endif
#endif

#ifdef CONFIG_MTK_DBGTOP
static void mrdump_dbgtop_dram_reserved(bool enable)
{
	int res;

	if (mrdump_ddr_reserve_ready) {

		if (enable == true) {
			pr_notice("%s: Trying to enable DDR Reserve Mode(%d)\n",
					__func__, enable);

			res = mtk_dbgtop_dram_reserved(enable);
			if (res == 0) {
				pr_notice("%s: DDR reserved mode enable ok\n",
					 __func__);

#ifdef CONFIG_MTK_DFD_INTERNAL_DUMP
				res = dfd_setup(DFD_BASIC_DUMP);
				if (res == -1)
					pr_notice("%s: DFD_BASIC_DUMP disabled\n",
							__func__);
				else
					pr_notice("%s: DFD_BASIC_DUMP enabled\n",
							__func__);
#else
				pr_notice("%s: config is not enabled yet\n",
						__func__);
#endif
			} else {
				pr_notice("%s: mtk_dbgtop_dram_reserved error(%d)\n",
						__func__, res);
			}
		} else {
			pr_notice("%s: DDR Reserve Mode disabled.(%d)\n",
					__func__, enable);
		}
	}
}
#else
#ifdef CONFIG_MTK_WATCHDOG
static void mrdump_wd_dram_reserved_mode(bool enabled)
{
	int res;
	struct wd_api *wd_api = NULL;

	res = get_wd_api(&wd_api);
	if (res < 0) {
		pr_notice("%s: get wd api error (%d)\n", __func__, res);
	} else {
		if (mrdump_ddr_reserve_ready) {
			res = wd_api->wd_dram_reserved_mode(enabled);
			if (res == 0) {
				if (enabled == true) {
					pr_notice("%s: DDR reserved mode enabled\n",
							__func__);
#ifdef CONFIG_MTK_DFD_INTERNAL_DUMP
					res = dfd_setup(DFD_BASIC_DUMP);
					if (res == -1)
						pr_notice("%s: DFD_BASIC_DUMP disabled\n",
								__func__);
					else
						pr_notice("%s: DFD_BASIC_DUMP enabled\n",
								__func__);
#else
			pr_notice("%s: dfd config is not enabled yet\n",
					__func__);
#endif
				} else {
					pr_notice("%s: DDR reserved mode disabled(%d)\n",
							__func__, res);
				}
			}
		}
	}
}

#endif
#endif

int __init mrdump_hw_init(void)
{
#ifdef CONFIG_MTK_DBGTOP
	mrdump_dbgtop_dram_reserved(true);
#else
#ifdef CONFIG_MTK_WATCHDOG
	mrdump_wd_dram_reserved_mode(true);
#endif
#endif
	pr_info("%s: init_done.\n", __func__);
	return 0;
}

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("MediaTek AEE module");
MODULE_AUTHOR("MediaTek Inc.");
