#ifndef __EEPROM_I2C_COMMON_DRIVER_H
#define __EEPROM_I2C_COMMON_DRIVER_H
#include <linux/i2c.h>

unsigned int Common_read_region(struct i2c_client *client,
				unsigned int addr,
				unsigned char *data,
				unsigned int size);

unsigned int gc08a3_read_region(struct i2c_client *client, unsigned int addr,
				unsigned char *data, unsigned int size);
#endif				/* __CAM_CAL_LIST_H */
