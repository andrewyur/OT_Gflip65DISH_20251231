

#ifndef __AP_MD_REG_DUMP_H__
#define __AP_MD_REG_DUMP_H__

/*  dump_md_reg ioremap,for internal dump */
struct dump_reg_ioremap {
	void __iomem **dump_reg;
	unsigned long long addr;
	unsigned long size;
};

enum MD_REG_ID {
	MD_REG_SET_DBGSYS_TIME_OUT_ADDR = 0,
	MD_REG_PC_MONITOR_ADDR,
	MD_REG_BUSMON_ADDR_0,
	MD_REG_BUSMON_ADDR_1,
	MD_REG_USIP_ADDR_0,
	MD_REG_USIP_ADDR_1,
	MD_REG_USIP_ADDR_2,
	MD_REG_USIP_ADDR_3,
	MD_REG_USIP_ADDR_4,
	MD_REG_USIP_ADDR_5,
};
extern void md_io_remap_internal_dump_register(struct ccci_modem *md);
void internal_md_dump_debug_register(unsigned int md_index);

#endif
