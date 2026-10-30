
#ifndef _MTK_UDI_H_
#define _MTK_UDI_H_

#if defined(CONFIG_MACH_MT6873)

#include "udi_v2/mtk_udi_mt6873.h"

#elif defined(CONFIG_MACH_MT6885)

#include "udi_v2/mtk_udi_mt6885.h"

#elif defined(CONFIG_MACH_MT6893)

#include "udi_v2/mtk_udi_mt6885.h"

#elif defined(CONFIG_MACH_MT6785)

#include "udi_v2/mtk_udi_mt6785.h"

#elif defined(CONFIG_MACH_MT6768)

#include "udi_v2/mtk_udi_mt6768.h"

#else

#error NO corresponding project of mtk_udi.h header file can be found!!!

#endif

#endif /* _MTK_UDI_H_ */
