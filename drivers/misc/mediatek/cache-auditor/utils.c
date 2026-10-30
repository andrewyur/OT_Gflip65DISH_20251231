#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/cgroup-defs.h>
#include <linux/cgroup.h>

#include "internal.h"

#undef pr_fmt
#define pr_fmt(fmt) "Cache-Auditor: " fmt

void __iomem *mcusys_base_addr;
const u32 rg_cpu_ic_age_offset = 0x8410;

int __init init_cache_priority(void)
{
	struct device_node *node;

	pr_info("Init cache priority control");
	node = of_find_compatible_node(NULL, NULL, "mediatek,mcucfg");
	if (!node) {
		pr_notice("[MCUSYS] find mediatek,mcucfg node failed\n");
		return -EINVAL;
	}

	mcusys_base_addr = of_iomap(node, 0);
	if (IS_ERR(mcusys_base_addr))
		return -EINVAL;

	return 0;
}
