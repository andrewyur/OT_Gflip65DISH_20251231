/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __MTK_UDI_INTERNAL_H__
#define __MTK_UDI_INTERNAL_H__

#include <mtk_udi.h>


#ifdef __KERNEL__
#include <linux/arm-smccc.h>
#include <linux/kernel.h>
#endif


/* define for UDI service */
#ifdef CONFIG_ARM64
#define MTK_SIP_KERNEL_UDI_JTAG_CLOCK		0xC20003A0
#define MTK_SIP_KERNEL_UDI_BIT_CTRL			0xC20003A1
#define MTK_SIP_KERNEL_UDI_WRITE			0xC20003A2
#define MTK_SIP_KERNEL_UDI_READ				0xC20003A3
#else
#define MTK_SIP_KERNEL_UDI_JTAG_CLOCK		0x820003A0
#define MTK_SIP_KERNEL_UDI_BIT_CTRL			0x820003A1
#define MTK_SIP_KERNEL_UDI_WRITE			0x820003A2
#define MTK_SIP_KERNEL_UDI_READ				0x820003A3
#endif

#define BIT_UDI(bit)		(1U << (bit))

#define MSB_UDI(range)	(1 ? range)
#define LSB_UDI(range)	(0 ? range)
#define BITMASK_UDI(r) \
	(((unsigned int) -1 >> (31 - MSB_UDI(r))) & ~((1U << LSB_UDI(r)) - 1))

#define GET_BITS_VAL_UDI(_bits_, _val_) \
	(((_val_) & (BITMASK_UDI(_bits_))) >> ((0) ? _bits_))

/* BITS(MSB:LSB, value) => Set value at MSB:LSB  */
#define BITS_UDI(r, val)	((val << LSB_UDI(r)) & BITMASK_UDI(r))

#define udi_reg_field(addr, range, val)	\
	udi_reg_write(addr, (udi_reg_read(addr) & ~(BITMASK_UDI(range)))\
					| BITS_UDI(range, val))

/* receive string for temp  */
/* #define UDI_FIFOSIZE 16384  */
#define UDI_FIFOSIZE 256

extern unsigned int udi_reg_read(unsigned int addr);
extern void udi_reg_write(unsigned int addr,
				unsigned int val);
extern unsigned int udi_jtag_clock(unsigned int sw_tck,
				unsigned int i_trst,
				unsigned int i_tms,
				unsigned int i_tdi,
				unsigned int count);
extern unsigned int udi_bit_ctrl(unsigned int sw_tck,
				unsigned int i_tdi,
				unsigned int i_tms,
				unsigned int i_trst);


#endif /* __MTK_UDI_INTERNAL_H__ */
