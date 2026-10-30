
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
        pr_info("gc9307p_2nd_init\n");
        mt_lcd_spi_write_com(0xfe);
        mt_lcd_spi_write_com(0xef);
        mt_lcd_spi_write_com(0x36);
        mt_lcd_spi_write_data(0x48); 
        mt_lcd_spi_write_com(0x3a);
        mt_lcd_spi_write_data(0x05);
                
        mt_lcd_spi_write_com(0x21);
                        
        mt_lcd_spi_write_com(0x86);	//c9_en
        mt_lcd_spi_write_data(0x98);


        mt_lcd_spi_write_com(0x89);	//e8_en
        mt_lcd_spi_write_data(0x03);
                
        mt_lcd_spi_write_com(0x8b);	//ff_en
        mt_lcd_spi_write_data(0x84);


        mt_lcd_spi_write_com(0x8d);	//
        mt_lcd_spi_write_data(0x33);

        mt_lcd_spi_write_com(0x8e);	 //f0~f3_en
        mt_lcd_spi_write_data(0x0f);
        mt_lcd_spi_write_com(0xe8);
        mt_lcd_spi_write_data(0x12);
        mt_lcd_spi_write_data(0x00);	


                
        mt_lcd_spi_write_com(0xff);
        mt_lcd_spi_write_data(0x62);

        mt_lcd_spi_write_com(0x99);//BVEE Drive Up
        mt_lcd_spi_write_data(0x3e);
        mt_lcd_spi_write_com(0x9d);	
        mt_lcd_spi_write_data(0x4b);
                
        mt_lcd_spi_write_com(0x98);//bvdd 
        mt_lcd_spi_write_data(0x3e);
        mt_lcd_spi_write_com(0x9c);	
        mt_lcd_spi_write_data(0x4b);//20210104
          
        mt_lcd_spi_write_com(0xc3);	
        mt_lcd_spi_write_data(0x30);
        mt_lcd_spi_write_com(0xc4);	
        mt_lcd_spi_write_data(0x0b);
        mt_lcd_spi_write_com(0xc9);	
        mt_lcd_spi_write_data(0x00);

        mt_lcd_spi_write_com(0xF0);
        mt_lcd_spi_write_data(0x88);//V1[5:0]
        mt_lcd_spi_write_data(0x0A);//V2[5:0]
        mt_lcd_spi_write_data(0x1D);//V4[4:0]
        mt_lcd_spi_write_data(0x13);//V6[4:0]-3
        mt_lcd_spi_write_data(0x08);//V0[7:4]/V13[3:0]-3
        mt_lcd_spi_write_data(0x34);//V20[6:0]//0x38

        mt_lcd_spi_write_com(0xF1);
        mt_lcd_spi_write_data(0x4C);//V43[6:0]
        mt_lcd_spi_write_data(0x76);//V27[7:5]/V57[4:0]
        mt_lcd_spi_write_data(0x72);//V36[7:5]/V59[4:0]
        mt_lcd_spi_write_data(0x22);//V61[5:0]
        mt_lcd_spi_write_data(0x20);//V62[5:0]
        mt_lcd_spi_write_data(0x9F);//V50[7:4]/V63[3:0]

        mt_lcd_spi_write_com(0xF2);
        mt_lcd_spi_write_data(0x4B);
        mt_lcd_spi_write_data(0x0A);
        mt_lcd_spi_write_data(0x00);
        mt_lcd_spi_write_data(0x16);
        mt_lcd_spi_write_data(0x0A);
        mt_lcd_spi_write_data(0x31);//0x35

        mt_lcd_spi_write_com(0xF3);
        mt_lcd_spi_write_data(0x47);
        mt_lcd_spi_write_data(0xb4);
        mt_lcd_spi_write_data(0x92);
        mt_lcd_spi_write_data(0x1a);
        mt_lcd_spi_write_data(0x17);
        mt_lcd_spi_write_data(0xaF);


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
        MDELAY(120);
        /* Sleep in */
        mt_lcd_spi_write_com(0x10);
	MDELAY(50);
        lcd_spi_unlock();
}

static void lcm_resume(void)
{
        lcm_init();
}

struct LCM_DRIVER gc9307p_2nd_qvga_spi = {
                .name = "gc9307p_2nd_qvga_spi",
                .set_util_funcs = lcm_set_util_funcs,
                .get_params = lcm_get_params,
                .init = lcm_init,
                .suspend = lcm_suspend,
                .resume = lcm_resume,

};
