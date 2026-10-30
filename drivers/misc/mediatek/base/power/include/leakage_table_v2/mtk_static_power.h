
#ifndef __MTK_STATIC_POWER_H__
#define __MTK_STATIC_POWER_H__

#include <linux/types.h>

/* #define MTK_SPOWER_UT */
#if defined(CONFIG_MACH_MT6759)
#include "mtk_static_power_mt6759.h"
#endif

#if defined(CONFIG_MACH_MT6763)
#include "mtk_static_power_mt6763.h"
#endif

#if defined(CONFIG_MACH_MT6758)
#include "mtk_static_power_mt6758.h"
#endif

#if defined(CONFIG_MACH_MT6739)
#include "mtk_static_power_mt6739.h"
#endif

#if defined(CONFIG_MACH_MT6765)
#include "mtk_static_power_mt6765.h"
#endif

#if defined(CONFIG_MACH_MT6771)
#include "mtk_static_power_mt6771.h"
#endif

#if defined(CONFIG_MACH_MT6775)
#include "mtk_static_power_mt6775.h"
#endif

#if defined(CONFIG_MACH_MT6768)
#include "mtk_static_power_mt6768.h"
#endif

#if defined(CONFIG_MACH_MT6785)
#include "mtk_static_power_mt6785.h"
#endif

#if defined(CONFIG_MACH_MT6885)
#if !defined(TRIGEAR_LEAKAGE)
#include "mtk_static_power_mt6885.h"
#else
#include "mtk_static_power_mt6893.h"
#endif
#endif

#if defined(CONFIG_MACH_MT6893)
#include "mtk_static_power_mt6893.h"
#endif

#if defined(CONFIG_MACH_MT6873)
#include "mtk_static_power_mt6873.h"
#endif

#if defined(CONFIG_MACH_MT6853)
#include "mtk_static_power_mt6853.h"
#endif

#if defined(CONFIG_MACH_MT6833)
#include "mtk_static_power_mt6833.h"
#endif

#if defined(CONFIG_MACH_MT8168)
#include "mtk_static_power_mt8168.h"
#endif

#undef  BIT
#define BIT(bit)	(1U << (bit))

#define MSB(range)	(1 ? range)
#define LSB(range)	(0 ? range)
#define GET_BITS_VAL(_bits_, _val_) \
	(((_val_) & (BITMASK(_bits_))) >> ((0) ? _bits_))

extern u32 get_devinfo_with_index(u32 index);

extern int mt_spower_get_leakage(int dev, unsigned int voltage, int degree);
extern int mt_spower_get_efuse_lkg(int dev);

extern int mt_spower_init(void);

#endif
