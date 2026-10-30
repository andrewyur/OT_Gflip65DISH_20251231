
#ifndef _ktz8864a_SW_H_
#define _ktz8864a_SW_H_

#ifndef BUILD_LK
struct ktz8864a_setting_table {
	unsigned char cmd;
	unsigned char data;
};

extern int ktz8864a_read_byte(unsigned char cmd, unsigned char *returnData);
extern int ktz8864a_write_byte(unsigned char cmd, unsigned char writeData);
#endif

#endif
