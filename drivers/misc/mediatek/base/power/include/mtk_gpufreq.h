
#ifndef _MTK_GPUFREQ_H_
#define _MTK_GPUFREQ_H_

#if defined(CONFIG_MACH_MT6885) \
	|| defined(CONFIG_MACH_MT6873) \
	|| defined(CONFIG_MACH_MT6853) \
	|| defined(CONFIG_MACH_MT6893) \
	|| defined(CONFIG_MACH_MT6833)
#include "gpufreq_v1/mtk_gpufreq.h"
#elif defined(CONFIG_MACH_MT6785)
#include "../mt6785/mtk_gpufreq.h"
#elif defined(CONFIG_MACH_MT6771)
#include "../mt6771/mtk_gpufreq.h"
#elif defined(CONFIG_MACH_MT6765)
#include "../mt6765/mtk_gpufreq.h"
#elif defined(CONFIG_MACH_MT6768)
#include "../mt6768/mtk_gpufreq.h"
#elif defined(CONFIG_MACH_MT6739)
#include "../mt6739/mtk_gpufreq.h"
#elif defined(CONFIG_MACH_MT8168)
#include "../mt8168/mtk_gpufreq.h"
#endif

#endif /* _MTK_GPUFREQ_H_ */
