#define PFX "CAM_CAL"
#define pr_fmt(fmt) PFX "[%s] " fmt, __func__


#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/of.h>
#include "cam_cal.h"
#include "cam_cal_define.h"
#include "cam_cal_list.h"
#include <linux/dma-mapping.h>
#ifdef CONFIG_COMPAT
/* 64 bit */
#include <linux/fs.h>
#include <linux/compat.h>
#endif

/* Include platform define if necessary */
#ifdef EEPROM_PLATFORM_DEFINE
#include "eeprom_platform_def.h"
#endif

static DEFINE_SPINLOCK(g_spinLock);

struct gc08a3_otp_t {
    u8  awb_flag;
    u8  awb_param[12];
    u8  awbChksum;
    u8  lsc_flag;
    u8  lsc_param[1868];
    u8  lscChksum;
    u8  af_flag;
    u8  af_param[8];
    u8  afChksum;
};

struct gc08a3_otp_t gc08a3_otp_info;
static struct i2c_client *g_pstI2CclientG;

/* add for linux-4.4 */
#ifndef I2C_WR_FLAG
#define I2C_WR_FLAG		(0x1000)
#define I2C_MASK_FLAG	(0x00ff)
#endif

#define EEPROM_I2C_MSG_SIZE_READ 2
#ifndef EEPROM_I2C_READ_MSG_LENGTH_MAX
#define EEPROM_I2C_READ_MSG_LENGTH_MAX 32
#endif

static int Read_I2C_CAM_CAL(u16 a_u2Addr, u32 ui4_length, u8 *a_puBuff)
{
	int i4RetValue = 0;
	char puReadCmd[2] = { (char)(a_u2Addr >> 8), (char)(a_u2Addr & 0xFF) };
	struct i2c_msg msg[EEPROM_I2C_MSG_SIZE_READ];


	if (ui4_length > EEPROM_I2C_READ_MSG_LENGTH_MAX) {
		pr_debug("exceed one transition %d bytes limitation\n",
			 EEPROM_I2C_READ_MSG_LENGTH_MAX);
		return -1;
	}
	spin_lock(&g_spinLock);
	g_pstI2CclientG->addr =
		g_pstI2CclientG->addr & (I2C_MASK_FLAG | I2C_WR_FLAG);
	spin_unlock(&g_spinLock);

	msg[0].addr = g_pstI2CclientG->addr;
	msg[0].flags = g_pstI2CclientG->flags & I2C_M_TEN;
	msg[0].len = 2;
	msg[0].buf = puReadCmd;

	msg[1].addr = g_pstI2CclientG->addr;
	msg[1].flags = g_pstI2CclientG->flags & I2C_M_TEN;
	msg[1].flags |= I2C_M_RD;
	msg[1].len = ui4_length;
	msg[1].buf = a_puBuff;

	i4RetValue = i2c_transfer(g_pstI2CclientG->adapter,
				msg,
				EEPROM_I2C_MSG_SIZE_READ);

	spin_lock(&g_spinLock);
	g_pstI2CclientG->addr = g_pstI2CclientG->addr & I2C_MASK_FLAG;
	spin_unlock(&g_spinLock);

	if (i4RetValue != EEPROM_I2C_MSG_SIZE_READ) {
		pr_debug("I2C read data failed!!\n");
		return -1;
	}

	return 0;
}

int iReadData_CAM_CAL(unsigned int ui4_offset,
	unsigned int ui4_length, unsigned char *pinputdata)
{
	int i4RetValue = 0;
	int i4ResidueDataLength;
	u32 u4IncOffset = 0;
	u32 u4CurrentOffset;
	u8 *pBuff;

	i4ResidueDataLength = (int)ui4_length;
	u4CurrentOffset = ui4_offset;
	pBuff = pinputdata;
	do {
		if (i4ResidueDataLength >= EEPROM_I2C_READ_MSG_LENGTH_MAX) {
			i4RetValue = Read_I2C_CAM_CAL(
				(u16) u4CurrentOffset,
				EEPROM_I2C_READ_MSG_LENGTH_MAX, pBuff);
			if (i4RetValue != 0) {
				pr_debug("I2C iReadData failed!!\n");
				return -1;
			}
			u4IncOffset += EEPROM_I2C_READ_MSG_LENGTH_MAX;
			i4ResidueDataLength -= EEPROM_I2C_READ_MSG_LENGTH_MAX;
			u4CurrentOffset = ui4_offset + u4IncOffset;
			pBuff = pinputdata + u4IncOffset;
		} else {
			i4RetValue =
			    Read_I2C_CAM_CAL(
			    (u16) u4CurrentOffset, i4ResidueDataLength, pBuff);
			if (i4RetValue != 0) {
				pr_debug("I2C iReadData failed!!\n");
				return -1;
			}
			u4IncOffset += i4ResidueDataLength;
			i4ResidueDataLength = 0;
			u4CurrentOffset = ui4_offset + u4IncOffset;
			pBuff = pinputdata + u4IncOffset;
			/* break; */
		}
	} while (i4ResidueDataLength > 0);



	return 0;
}

unsigned int Common_read_region(struct i2c_client *client, unsigned int addr,
				unsigned char *data, unsigned int size)
{
	g_pstI2CclientG = client;
	if (iReadData_CAM_CAL(addr, size, data) == 0)
		return size;
	else
		return 0;
}

unsigned int gc08a3_read_region(struct i2c_client *client, unsigned int addr,
                                unsigned char *data, unsigned int size)
{
    int i=0;

    pr_err("[ljt]addr =%x size %d\n", addr, size);
    if (addr == 0x0)
    {
        *(u32 *)data = 0x010b00ff;
    }
    else if (addr == 0x1)
    {
        if ((gc08a3_otp_info.lsc_flag == 0x01) || (gc08a3_otp_info.lsc_flag == 0x04))
        {
            pr_err("[ljt]lsc_flag valid\n");
            for(i=0; i<size; i++){
                data[i] = gc08a3_otp_info.lsc_param[i];
            }
        }
    }
    else if (addr == 0x2)
    {
        if ((gc08a3_otp_info.awb_flag == 0x01) || (gc08a3_otp_info.awb_flag == 0x04))
        {
            *data = 1;
        }
    }
    else if (addr == 0x3)
    {
        for(i=0; i<size; i++){
            data[i] = gc08a3_otp_info.awb_param[i];
            //pr_err("[ljt]awb data[%d] =%x \n", i, data[i]);
        }
    }
    else if (addr == 0x4)
    {
        if ((gc08a3_otp_info.af_flag == 0x01) ||(gc08a3_otp_info.af_flag == 0x04))
        {
            *data = 1;
        }
    }
    else if (addr == 0x5)
    {
        for(i=0; i<size; i++){
            data[i] = gc08a3_otp_info.af_param[i];
            //pr_err("[ljt]af data[%d] =%x \n", i, data[i]);
        }
    }
    else
    {
        pr_err("[ljt]error addr\n", addr, size);
    }

    return size;
}
