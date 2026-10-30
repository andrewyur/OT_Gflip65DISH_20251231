
#ifndef __MTK_UDI_MT6785__
#define __MTK_UDI_MT6785__

#include <linux/kernel.h>

/* CPU UDI pin mux ADDR */
/* 1. Write 0x10005400= 44000000 */
/* 2. Write 0x10005410= 00004440 */


#ifdef CONFIG_OF
#define DEVICE_GPIO "mediatek,gpio"
/* 0x10005000 0x1000, UDI pinmux reg */
static void __iomem  *udipin_base;
#endif

#ifdef __KERNEL__
#define UDIPIN_BASE				(udipin_base)
#else
#define UDIPIN_BASE				0x10005000
#endif

/* 0x10005000 0x1000, UDI pinmux reg */
#define UDIPIN_UDI_MUX1			(UDIPIN_BASE+0x400)
#define UDIPIN_UDI_MUX1_VALUE	(0x44000000)
#define UDIPIN_UDI_MUX2			(UDIPIN_BASE+0x410)
#define UDIPIN_UDI_MUX2_VALUE	(0x00004440)



#endif /* __MTK_UDI_MT6785__ */

