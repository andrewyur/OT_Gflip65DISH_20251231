
#define LOG_TAG "LCM"

#ifndef BUILD_LK
#include <linux/kernel.h>
#include <linux/string.h>
#endif

#include "lcm_drv.h"

#ifdef BUILD_LK
#include <platform/mt_gpio.h>
#include <platform/mt_i2c.h>
#include <platform/mt_pmic.h>
#include <platform/upmu_common.h>
#include <string.h>
#elif defined(BUILD_UBOOT)
#include <asm/arch/mt_gpio.h>
#endif

#ifdef BUILD_LK
#define LCM_LOGI(string, args...) dprintf(0, "[LK/" LOG_TAG "]" string, ##args)
#define LCM_LOGD(string, args...) dprintf(1, "[LK/" LOG_TAG "]" string, ##args)
#else
#define LCM_LOGI(fmt, args...) pr_debug("[KERNEL/" LOG_TAG "]" fmt, ##args)
#define LCM_LOGD(fmt, args...) pr_debug("[KERNEL/" LOG_TAG "]" fmt, ##args)
#endif

static struct LCM_UTIL_FUNCS lcm_util;

#define SET_RESET_PIN(v) (lcm_util.set_reset_pin((v)))
#define MDELAY(n) (lcm_util.mdelay(n))
#define UDELAY(n) (lcm_util.udelay(n))

#define dsi_set_cmdq_V22(cmdq, cmd, count, ppara, force_update)                \
	lcm_util.dsi_set_cmdq_V22(cmdq, cmd, count, ppara, force_update)
#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)                       \
	lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)                          \
	lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd) lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)                                     \
	lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd) lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size)                                  \
	lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)

#ifndef BUILD_LK
#include <linux/fs.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#endif

#define FRAME_WIDTH (320)
#define FRAME_HEIGHT (480)

/* physical size in um */
#define LCM_PHYSICAL_WIDTH (68690)
#define LCM_PHYSICAL_HEIGHT (146280)
#define LCM_DENSITY (480)

#define REGFLAG_DELAY 0xFFFC
#define REGFLAG_UDELAY 0xFFFB
#define REGFLAG_END_OF_TABLE 0xFFFD


#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

struct LCM_setting_table {
	unsigned int cmd;
	unsigned char count;
	unsigned char para_list[200];
};

static struct LCM_setting_table lcm_suspend_setting[] = {
	{0x28, 0, {} },
	{REGFLAG_DELAY, 50, {} },
	{0x10, 0, {} },
	{REGFLAG_DELAY, 150, {} },
};

static struct LCM_setting_table __maybe_unused init_setting_vdo[] = {
	 { 0x11 ,1,{0x00}},
     {REGFLAG_DELAY, 120, {}},
	 {0x36, 1, {0x48}},
	 {0x3a, 1, {0x77}},
	 {0xf0, 1, {0xc3}},
	 {0xf0, 1, {0x96}},
	 {0xb4, 1, {0x01}},
	 {0xb7, 1, {0xc6}},
	 {0xe8, 8, {0x40,0x8a,0x00,0x00,0x29,0x19,0xa5,0x33}},
	 {0xb9, 2, {0x02,0xE0}},
	 {0xc0, 2, {0x80,0x61}},
	 {0xc1, 1, {0x11}},
	 {0xc2, 1, {0xa7}},
	 {0xc5, 1, {0x11}},
	 {0xe0, 14,{0xdd,0x25,0x29,0x0c,0x06,0x16,0x44,0x43,0x51,0x1a,0x18,0x15,0x1b,0x21}},
	 {0xe1, 14,{0xdd,0x24,0x28,0x0a,0x05,0x24,0x44,0x43,0x52,0x3a,0x18,0x14,0x22,0x20}},
	 {0xf0, 1, {0x3c}},
	 {0xf0, 1, {0x69}},
	 {0x29 ,1,{0x00}},
	 {REGFLAG_DELAY, 10, {}},
	 {REGFLAG_END_OF_TABLE, 0x00, {}}
};

static struct LCM_setting_table __maybe_unused
	lcm_deep_sleep_mode_in_setting[] = {
		{0x28, 1, {0x00} },
		{REGFLAG_DELAY, 50, {} },
		{0x10, 1, {0x00} },
		{REGFLAG_DELAY, 150, {} },
};

static struct LCM_setting_table __maybe_unused lcm_sleep_out_setting[] = {
	{0x11, 1, {0x00} },
	{REGFLAG_DELAY, 120, {} },
	{0x29, 1, {0x00} },
	{REGFLAG_DELAY, 50, {} },
};


static void push_table(void *cmdq, struct LCM_setting_table *table,
		       unsigned int count, unsigned char force_update)
{
	unsigned int i;
	unsigned int cmd;

	for (i = 0; i < count; i++) {
		cmd = table[i].cmd;
		switch (cmd) {
		case REGFLAG_DELAY:
			if (table[i].count <= 10)
				MDELAY(table[i].count);
			else
				MDELAY(table[i].count);
			break;
		case REGFLAG_UDELAY:
			UDELAY(table[i].count);
			break;
		case REGFLAG_END_OF_TABLE:
			break;
		default:
			dsi_set_cmdq_V22(cmdq, cmd, table[i].count,
					 table[i].para_list, force_update);
			break;
		}
	}
}

static void lcm_set_util_funcs(const struct LCM_UTIL_FUNCS *util)
{
	memcpy(&lcm_util, util, sizeof(struct LCM_UTIL_FUNCS));
}

static void lcm_get_params(struct LCM_PARAMS *params)
{
	memset(params, 0, sizeof(struct LCM_PARAMS));

	params->type = LCM_TYPE_DSI;

	params->width = FRAME_WIDTH;
	params->height = FRAME_HEIGHT;
	
	
	//params->density = LCM_DENSITY;

	params->dsi.mode = BURST_VDO_MODE;
	params->dsi.switch_mode = CMD_MODE;
	
	//lcm_dsi_mode = SYNC_PULSE_VDO_MODE;
	
//	LCM_LOGI("%s: lcm_dsi_mode %d\n", __func__, lcm_dsi_mode);
	params->dsi.switch_mode_enable = 0;

	/* DSI */
	/* Command mode setting */
	params->dsi.LANE_NUM = LCM_ONE_LANE;
	/* The following defined the fomat for data coming from LCD engine. */
	params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
	params->dsi.data_format.trans_seq = LCM_DSI_TRANS_SEQ_MSB_FIRST;
	params->dsi.data_format.padding = LCM_DSI_PADDING_ON_LSB;
	params->dsi.data_format.format = LCM_DSI_FORMAT_RGB888;

	/* Highly depends on LCD driver capability. */
	params->dsi.packet_size = 256;
	/* video mode timing */

	params->dsi.PS = LCM_PACKED_PS_24BIT_RGB888;
        params->dsi.HS_TRAIL= 4;
        params->dsi.CLK_TRAIL=4;
	params->dsi.vertical_sync_active				= 4;
	params->dsi.vertical_backporch					= 20;
	params->dsi.vertical_frontporch					= 20;
	params->dsi.vertical_active_line				= FRAME_HEIGHT; //480

	params->dsi.horizontal_sync_active				= 8;
	params->dsi.horizontal_backporch				= 80;
	params->dsi.horizontal_frontporch				= 80;
	params->dsi.horizontal_active_pixel				= FRAME_WIDTH; //320

    params->dsi.PLL_CLOCK = 159;


}


static void lcm_init(void)
{
	SET_RESET_PIN(1);
	MDELAY(20);
	SET_RESET_PIN(0);
	MDELAY(20);

	SET_RESET_PIN(1);
	MDELAY(140);
	
    //lcm_init_set();
	push_table(NULL, init_setting_vdo, ARRAY_SIZE(init_setting_vdo), 1);
	
	LCM_LOGI("st7796u----lcm mode = vdo mode ----\n");
}

static void lcm_suspend(void)
{
	push_table(NULL, lcm_suspend_setting, ARRAY_SIZE(lcm_suspend_setting),
		   1);
}

static void lcm_resume(void)
{
	lcm_init();
}

struct LCM_DRIVER st7796u_qvga_dsi_vdo_drv = {
	.name = "st7796u_qvga_dsi_vdo",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params = lcm_get_params,
	.init = lcm_init,
	.suspend = lcm_suspend,
	.resume = lcm_resume,
	//.init_power = lcm_init_power,
	//.resume_power = lcm_resume_power,
	//.suspend_power = lcm_suspend_power,
	
	//.set_backlight_cmdq = lcm_setbacklight_cmdq,
	//.ata_check = lcm_ata_check,
	//.update = lcm_update,
};
