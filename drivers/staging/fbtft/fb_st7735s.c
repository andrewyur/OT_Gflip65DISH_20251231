/*
 * FB driver for the ST7735S LCD Controller
 *
 * Copyright (C) 2013 Noralf Tronnes
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/regulator/consumer.h>
#include <linux/of_gpio.h>

#include "fbtft.h"


#define DRVNAME "st7735s"

static DEFINE_MUTEX(st7735s_mutex);

struct fb_dev{
	int id[3];
	int source;
};

#define TD_TNSQ1402_3T	0x00
#define TXDT144SF_65	0x01
#define GC9104P 0x02

/* MODIFIED-BEGIN by hongwei.tian, 2019-07-26,BUG-8177442*/
static char *sublcd_name = "no sublcd";
module_param(sublcd_name, charp, S_IRUGO);
MODULE_PARM_DESC(sublcd_name, "sub lcd name.");
/* MODIFIED-END by hongwei.tian,BUG-8177442*/

unsigned char sub_lcdinfo[128];

static struct fb_dev dev_inst;

struct regulator *fbtft_spi_vio;

extern char SUB_LCM_module_name[256];

static int init_source(void)
{
	int id0, id1, id2;

	id0 = gpio_get_value(dev_inst.id[0]);
	id1 = gpio_get_value(dev_inst.id[1]);
	id2 = 0;

	dev_inst.source = TD_TNSQ1402_3T;

	pr_info("fbtft %s: id0=%d, id1=%d, id2=%d\n", __func__, id0, id1, id2);

	if ((id0 == 1) && (id1 == 0 || id2 == 0)) {
		dev_inst.source = TD_TNSQ1402_3T;
		/* MODIFIED-BEGIN by hongwei.tian, 2019-07-26,BUG-8177442*/
		sublcd_name = "ST7735S:TDT:128x128";
		snprintf(sub_lcdinfo, PAGE_SIZE, "ST7735S:TDT:128x128");
		pr_info("[fbtft]The source of sub lcd is TD_TNSQ1402_3T\n");
	} else if ((id0 == 0) && (id1 == 1 || id2 == 1)) {
		dev_inst.source = TD_TNSQ1402_3T;
		sublcd_name = "ST7735S:TXD:128x128";
		snprintf(sub_lcdinfo, PAGE_SIZE, "ST7735S:TXD:128x128");
		pr_info("[fbtft]The source of sub lcd is TXDT144SF_65\n");
	} else if (id0 == 0 && id1 == 0 && id2 == 0) {
		dev_inst.source = GC9104P;
		sublcd_name = "GC9104P:COE:128x128";
		snprintf(sub_lcdinfo, PAGE_SIZE, "GC9104P:COE:128x128");
		/* MODIFIED-END by hongwei.tian,BUG-8177442*/
		pr_info("[fbtft]The source of sub lcd is GC9104P\n");
	} else {
		pr_info("[fbtft]err, unknown lcd source.[%s,%d]\n", __func__, __LINE__);
	}

	sprintf(SUB_LCM_module_name, sublcd_name);

	return dev_inst.source;
}

static int get_source(void)
{
	return dev_inst.source;
}

static int init_gpio(struct fbtft_par *par)
{
	struct device_node *np = par->spi->dev.of_node;
	int i = 0;
	int ret = 0;
        struct pinctrl *lcm_spi_pinctrl;
        struct pinctrl_state *spi_all;

	for (i = 0; i < 2; i++) {
		dev_inst.id[i] = of_get_named_gpio(np, "id-gpios", i);

		if (!gpio_is_valid(dev_inst.id[i]))
			pr_err("[fbtft]%s,%d\n", __func__, __LINE__);

		if ((ret = gpio_request(dev_inst.id[i], "id-gpios")))
			pr_err("%s %d gpio_request failed.\n", __func__, __LINE__);

		if ((ret = gpio_direction_input(dev_inst.id[i])))
			pr_err("%s %d, gpio_direction failed.[%d]\n", __func__, __LINE__, ret);
	}

        lcm_spi_pinctrl = devm_pinctrl_get(&(par->spi->dev));
        if (IS_ERR(lcm_spi_pinctrl)) {
                ret = PTR_ERR(lcm_spi_pinctrl);
                pr_err("%s Cannot find lcd spi pinctrl!\n", __func__);
        }
        spi_all = pinctrl_lookup_state(lcm_spi_pinctrl, "state_spi_all");

        if (IS_ERR(spi_all)) {
                ret = PTR_ERR(spi_all);
                pr_err("%s Cannot find state_spi_all %d!\n", __func__, ret);
        } else {
                ret = pinctrl_select_state(lcm_spi_pinctrl, spi_all);
                pr_info("%s: state_spi_all select ret = %d\n", __func__, ret);
        }

	init_source();

	return 0;
}

static void init_lcd_st7735s(struct fbtft_par *par)
{
	/* SWRESET - Software reset */
	pr_info("[fbtft] %s\n", __func__);

	par->fbtftops.reset(par);

	write_reg(par, 0x11);
	mdelay(120);

	write_reg(par, 0xF0, 0x01);

	write_reg(par, 0xB1, 0x09, 0x1C, 0x1D);
	write_reg(par, 0xB2, 0x01, 0x2C, 0x2D);
	write_reg(par, 0xB3, 0x01, 0x2C, 0x2D, 0x01, 0x2C, 0x2D);
	write_reg(par, 0xB4, 0x00);
	write_reg(par, 0xB6, 0xB4, 0xF0);

	write_reg(par, 0xC0, 0xA4, 0x04, 0x84);
	write_reg(par, 0xC1, 0xC6);
	write_reg(par, 0xC2, 0x0A, 0x00);
	write_reg(par, 0xC3,0x8A,0x2A);
	write_reg(par, 0xC4, 0x8A, 0xEE);
	write_reg(par, 0xC5, 0x17);

	write_reg(par, 0x36, 0x08);

	write_reg(par, 0xE0, 0x0A, 0x19, 0x0C, 0x14, 0x2A, 0x28, 0x1F, 0x20,
						0x1E, 0x1A, 0x22, 0x35, 0x00, 0x07, 0x02, 0x10);
	write_reg(par, 0xE1, 0x0A, 0x1A, 0x0C, 0x13, 0x2D, 0x2C, 0x29, 0x2E,
						0x2E, 0x2D, 0x37, 0x3C, 0x00, 0x07, 0x03, 0x10);

	/* COLMOD - Interface pixel format	05:65K	06:262K*/
	write_reg(par, 0x3A, 0x05);

	/*Set Column address*/
//	write_reg(par, 0x2A, 0x00, 0x00, 0x00, 0x7F);

	/*Set Page address*/
//	write_reg(par, 0x2B, 0x00, 0x20, 0x00, 0x9F);

	/* TE OFF- Tearing Effect Line off */
	write_reg(par, 0x34);

	/* DISPON - Display On */
	write_reg(par, 0x29);
	mdelay(10);
	/* NORON - Partial off (Normal) GRAM write start*/
	write_reg(par, 0x2C);
	mdelay(5);
	pr_info("[fbtft] %s done\n", __func__);
}

static void init_lcd_gc9304p(struct fbtft_par *par)
{
	/* SWRESET - Software reset */
	pr_info("[fbtft] %s\n", __func__);

	par->fbtftops.reset(par);

	write_reg(par, 0xFE);
	write_reg(par, 0xFE);
	write_reg(par, 0xEF);

	write_reg(par, 0xB3, 0x03);
	write_reg(par, 0x21);

	write_reg(par, 0x36, 0x08);
	write_reg(par, 0x3A, 0x05);
	write_reg(par, 0xB4, 0x21);
	write_reg(par, 0xB6, 0x01);
	write_reg(par, 0xA3, 0x11);
	write_reg(par, 0xB1, 0xc0);
	write_reg(par, 0xE6, 0x50, 0x43);
	write_reg(par, 0xE7, 0x56, 0x43);

	write_reg(par, 0xF0, 0x0B, 0x4D, 0x1B, 0x5E, 0x8C, 0x2C, 0x2D, 0x00,
				   0x00, 0x10, 0x12, 0x15, 0x14, 0x0F);
	write_reg(par, 0xF1, 0x01, 0x28, 0x2D, 0x3A, 0xB8, 0x0D, 0x0E, 0x00,
				   0x00, 0x08, 0x08, 0x18, 0x16, 0x0F);

	write_reg(par, 0xFE);
	write_reg(par, 0xFF);

	write_reg(par, 0x11);
	mdelay(120);
	/* DISPON - Display On */
	write_reg(par, 0x29);
	mdelay(10);
	/* NORON - Partial off (Normal) GRAM write start*/
	write_reg(par, 0x2C);
	mdelay(5);
}

static void init_lcd(struct fbtft_par *par)
{
	int src = TD_TNSQ1402_3T;

	src = get_source();

	switch (src) {
		case TD_TNSQ1402_3T:
		case TXDT144SF_65:
			init_lcd_st7735s(par);
			break;
		case GC9104P:
			init_lcd_gc9304p(par);
			break;
		default:
			init_lcd_st7735s(par);
			break;
	}
}

static void set_addr_win(struct fbtft_par *par, int xs, int ys, int xe, int ye)
{
	int src = TD_TNSQ1402_3T;

	fbtft_par_dbg(DEBUG_SET_ADDR_WIN, par,
		"%s(xs=%d, ys=%d, xe=%d, ye=%d)\n", __func__, xs, ys, xe, ye);
    pr_info("%s(xs=%d, ys=%d, xe=%d, ye=%d)\n", __func__, xs, ys, xe, ye);
	mutex_lock(&st7735s_mutex);
	src = get_source();
	pr_info("fbtft %s: src=%d\n", __func__, src);

	switch (src) {
	case TD_TNSQ1402_3T:
		xs = 0x00;
		xe = 0x7F;
		ys = 0x20;
		ye = 0x9F;
		break;
	case TXDT144SF_65:
		xs = 0x02;
		xe = 0x81;
		ys = 0x21;
		ye = 0xA0;
		break;
	case GC9104P:
		xs = 0x00;
		xe = 0x7f;
		ys = 0x00;
		ye = 0x7f;
	default:
		break;
	}

	/* Column address */
	write_reg(par, 0x2A, xs >> 8, xs & 0xFF, xe >> 8, xe & 0xFF);
	/* Row adress */
	write_reg(par, 0x2B, ys >> 8, ys & 0xFF, ye >> 8, ye & 0xFF);

	/* Memory write */
	write_reg(par, 0x2C);
	mutex_unlock(&st7735s_mutex);
}

#define MY (1 << 7)
#define MX (1 << 6)
#define MV (1 << 5)
static int set_var(struct fbtft_par *par)
{
	fbtft_par_dbg(DEBUG_INIT_DISPLAY, par, "%s()\n", __func__);

	/* MADCTL - Memory data access control
	     RGB/BGR:
	     1. Mode selection pin SRGB
	        RGB H/W pin for color filter setting: 0=RGB, 1=BGR
	     2. MADCTL RGB bit
	        RGB-BGR ORDER color filter panel: 0=RGB, 1=BGR */
	switch (par->info->var.rotate) {
	case 0:
		write_reg(par, 0x36, MX | MY | (par->bgr << 3));
		break;
	case 270:
		write_reg(par, 0x36, MY | MV | (par->bgr << 3));
		break;
	case 180:
		write_reg(par, 0x36, (par->bgr << 3));
		break;
	case 90:
		write_reg(par, 0x36, MX | MV | (par->bgr << 3));
		break;
	}

	return 0;
}

//add begin 快速翻合盖白屏
//int blank_on = 0xff; 
//add end 快速翻合盖白屏

static int set_blank(struct fbtft_par *par, bool on)
{
	int ret = 0;

	pr_info("fbtft: %s %s\n", __func__, on?"blank":"unblank");

#if 0
	//add begin 快速翻合盖白屏
	if(blank_on==on)
       {
         pr_err("fmb blank return \n");
           return 0;
       }
       blank_on = on;
	//add end 快速翻合盖白屏
#endif

	mutex_lock(&st7735s_mutex);
	if (on) {
		write_reg(par, 0x28);
		mdelay(30);
		write_reg(par, 0x10);
		mdelay(120);
		ret = 0;
	} else {
		init_lcd(par);
		set_var(par);
		ret = 1;
	}
	mutex_unlock(&st7735s_mutex);

	return ret;
}

static int init_display(struct fbtft_par *par)
{
	/*int ret = 0;

	ret = regulator_enable(par->vio);
	if (ret) {
		dev_err(&par->pdev->dev, "Regulator enable vio failed ret = %d\n", ret);
		return ret;
	}

	ret = regulator_enable(par->vcc);
	if (ret) {
		dev_err(&par->pdev->dev, "Regulator enable vcc failed ret = %d\n", ret);
		return ret;
	}
	*/
	init_gpio(par);
	mutex_lock(&st7735s_mutex);
	init_lcd(par);
	mutex_unlock(&st7735s_mutex);
	set_blank(par, true);

	return 0;
}


struct fbtft_display display = {
	.regwidth = 8,
	.width = 128,
	.height = 128,
	.fbtftops = {
		.init_display = init_display,
		.set_addr_win = set_addr_win,
		.set_var = set_var,
		.blank = set_blank,
	},
};

FBTFT_REGISTER_DRIVER(DRVNAME, "mediatek,st7735s", &display);

MODULE_ALIAS("spi:" DRVNAME);
MODULE_ALIAS("platform:" DRVNAME);
MODULE_ALIAS("spi:st7735s");
MODULE_ALIAS("platform:st7735s");

MODULE_DESCRIPTION("FB driver for the ST7735R LCD Controller");
MODULE_AUTHOR("Noralf Tronnes");
MODULE_LICENSE("GPL");
