
#ifndef _COMMON_MTK_UPOWER_H
#define _COMMON_MTK_UPOWER_H

#if defined(CONFIG_MACH_MT6759) || defined(CONFIG_MACH_MT6763) \
	|| defined(CONFIG_MACH_MT6758) || defined(CONFIG_MACH_MT6739) \
	|| defined(CONFIG_MACH_MT6771) || defined(CONFIG_MACH_MT6775) \
	|| defined(CONFIG_MACH_MT6785) || defined(CONFIG_MACH_MT6885) \
	|| defined(CONFIG_MACH_MT6768) || defined(CONFIG_MACH_MT6765) \
	|| defined(CONFIG_MACH_MT6873) || defined(CONFIG_MACH_MT6853) \
	|| defined(CONFIG_MACH_MT6893) || defined(CONFIG_MACH_MT8168) \
	|| defined(CONFIG_MACH_MT6833)
#include "upower_v2/mtk_unified_power.h"
#endif

#if defined(CONFIG_MACH_MT6799)
#include "upower_v1/mtk_unified_power.h"
#endif

#endif /* _COMMON_MTK_UPOWER_H */

