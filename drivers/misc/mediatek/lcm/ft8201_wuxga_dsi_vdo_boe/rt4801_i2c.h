
#ifndef _rt4801_SW_H_
#define _rt4801_SW_H_

#ifndef BUILD_LK
struct rt4801_setting_table {
	unsigned char cmd;
	unsigned char data;
};

extern int rt4801_read_byte(unsigned char cmd, unsigned char *returnData);
extern int rt4801_write_byte(unsigned char cmd, unsigned char writeData);
#endif

#endif
