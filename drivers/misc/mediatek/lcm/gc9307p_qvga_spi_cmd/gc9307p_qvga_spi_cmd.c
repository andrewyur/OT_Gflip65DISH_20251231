
#ifndef BUILD_LK
#include <linux/string.h>
#endif
#include "lcm_drv.h"
#include "lcd_spi_driver.h"

#ifdef BUILD_LK
#include <platform/mt_gpio.h>
#elif defined(BUILD_UBOOT)
#include <asm/arch/mt_gpio.h>
#endif

#define FRAME_WIDTH     (240)
#define FRAME_HEIGHT    (320)

static struct LCM_UTIL_FUNCS lcm_util;

#define SET_RESET_PIN(v)    (lcm_util.set_reset_pin((v)))

#define UDELAY(n)  (lcm_util.udelay(n))
#define MDELAY(n)  (lcm_util.mdelay(n))

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

        /* enable tearing-free*/
        //params->dbi.te_mode                                  = LCM_DBI_TE_MODE_VSYNC_ONLY;
        //params->dbi.te_edge_polarity         = LCM_POLARITY_RISING;

        params->physical_width = 43;
        params->physical_height = 57;
        params->dsi.mode   = SYNC_PULSE_VDO_MODE;
        //params->dsi.mode = CMD_MODE;

        /*
         * DSI
         * Command mode setting
         */
        params->dsi.LANE_NUM = LCM_ONE_LANE;
        /* The following defined the fomat for data coming from LCD engine. */
        params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
        params->dsi.data_format.trans_seq = LCM_DSI_TRANS_SEQ_MSB_FIRST;
        params->dsi.data_format.padding = LCM_DSI_PADDING_ON_LSB;
        params->dsi.data_format.format = LCM_DSI_FORMAT_RGB565;

        /*
         * Highly depends on LCD driver capability.
         * Not support in MT6573
         */

        params->dsi.packet_size = 256;
        params->dsi.PS = LCM_PACKED_PS_16BIT_RGB565;

        params->dsi.vertical_sync_active = 4;
        params->dsi.vertical_backporch = 36;
        params->dsi.vertical_frontporch = 40;
        params->dsi.vertical_active_line = FRAME_HEIGHT;
        params->dsi.horizontal_sync_active = 10;
        params->dsi.horizontal_backporch = 25;
        params->dsi.horizontal_frontporch = 25;
        params->dsi.horizontal_active_pixel = FRAME_WIDTH;

        /* params->dsi.esd_check_enable = 0;*/
        params->dsi.PLL_CLOCK = 60;
}

static void GC9307P_Init(void)
{
        pr_info("gc9307p_init\n");
        mt_lcd_spi_write_com(0xfe);
        mt_lcd_spi_write_com(0xfe);
        mt_lcd_spi_write_com(0xef);

        mt_lcd_spi_write_com(0x36);
        mt_lcd_spi_write_data(0x48);

        mt_lcd_spi_write_com(0x3a);
        mt_lcd_spi_write_data(0x05);

        mt_lcd_spi_write_com(0x21);

        mt_lcd_spi_write_com(0x86);
        mt_lcd_spi_write_data(0x98);

        mt_lcd_spi_write_com(0x89);
        mt_lcd_spi_write_data(0x03);

        mt_lcd_spi_write_com(0x8b);
        mt_lcd_spi_write_data(0x84);

        mt_lcd_spi_write_com(0x8d);
        mt_lcd_spi_write_data(0x33);

        mt_lcd_spi_write_com(0x8e);
        mt_lcd_spi_write_data(0x0f);

        mt_lcd_spi_write_com(0xe8);
        mt_lcd_spi_write_data(0x13);
        mt_lcd_spi_write_data(0x00);

        mt_lcd_spi_write_com(0xff);
        mt_lcd_spi_write_data(0x62);

        mt_lcd_spi_write_com(0x98);
        mt_lcd_spi_write_data(0x3e);

        mt_lcd_spi_write_com(0x9c);
        mt_lcd_spi_write_data(0x4b);

        mt_lcd_spi_write_com(0x99);
        mt_lcd_spi_write_data(0x3e);

        mt_lcd_spi_write_com(0x9d);
        mt_lcd_spi_write_data(0x4b);

        mt_lcd_spi_write_com(0xc3);
        mt_lcd_spi_write_data(0x2D);
        mt_lcd_spi_write_com(0xc4);
        mt_lcd_spi_write_data(0x2D);
        mt_lcd_spi_write_com(0xc9);
        mt_lcd_spi_write_data(0x0F);

        mt_lcd_spi_write_com(0xF0);
        mt_lcd_spi_write_data(0x80);
        mt_lcd_spi_write_data(0x02);
        mt_lcd_spi_write_data(0x07);
        mt_lcd_spi_write_data(0x07);
        mt_lcd_spi_write_data(0x06);
        mt_lcd_spi_write_data(0x34);

        mt_lcd_spi_write_com(0xF1);
        mt_lcd_spi_write_data(0x4D);
        mt_lcd_spi_write_data(0x77);
        mt_lcd_spi_write_data(0x76);
        mt_lcd_spi_write_data(0x29);
        mt_lcd_spi_write_data(0x2B);
        mt_lcd_spi_write_data(0x9F);

        mt_lcd_spi_write_com(0xF2);
        mt_lcd_spi_write_data(0x40);
        mt_lcd_spi_write_data(0x00);
        mt_lcd_spi_write_data(0x06);
        mt_lcd_spi_write_data(0x07);
        mt_lcd_spi_write_data(0x07);
        mt_lcd_spi_write_data(0x24);

        mt_lcd_spi_write_com(0xF3);
        mt_lcd_spi_write_data(0x36);
        mt_lcd_spi_write_data(0xD6);
        mt_lcd_spi_write_data(0xB6);
        mt_lcd_spi_write_data(0x03);
        mt_lcd_spi_write_data(0x04);
        mt_lcd_spi_write_data(0xBF);

        mt_lcd_spi_write_com(0x35);
        mt_lcd_spi_write_data(0x00);

        mt_lcd_spi_write_com(0x44);
        mt_lcd_spi_write_data(0x00);
        mt_lcd_spi_write_data(0x0a);

        mt_lcd_spi_write_com(0x11);
        MDELAY(0x78);

        mt_lcd_spi_write_com(0x29);
        mt_lcd_spi_write_com(0x2c);
}

static void lcm_init(void)
{
        SET_RESET_PIN(1);
        MDELAY(10);
        SET_RESET_PIN(0);
        MDELAY(10);

        SET_RESET_PIN(1);
        MDELAY(120);

        lcd_spi_lock();
        GC9307P_Init();
        lcd_spi_unlock();

}

static void lcm_suspend(void)
{

        lcd_spi_lock();
        /* Display off */
        mt_lcd_spi_write_com(0x28);
        MDELAY(50);
        /* Sleep in */
        mt_lcd_spi_write_com(0x10);
        lcd_spi_unlock();
}

static void lcm_resume(void)
{
        lcm_init();
}

struct LCM_DRIVER gc9307p_qvga_spi = {
                .name = "gc9307p_qvga_spi",
                .set_util_funcs = lcm_set_util_funcs,
                .get_params = lcm_get_params,
                .init = lcm_init,
                .suspend = lcm_suspend,
                .resume = lcm_resume,

};
