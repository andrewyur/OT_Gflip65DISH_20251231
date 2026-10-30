/* Copyright (c) 2017-2018, 2020, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/module.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/leds.h>
#include <linux/of_device.h>
#include <linux/kthread.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/spi/spi.h>
#include <linux/spi/spidev.h>
#include <linux/sched.h>
#include <linux/sched/clock.h>
#include <uapi/linux/sched/types.h>
#include <linux/of_irq.h>
#include "primary_display.h"

#define lcd_SPI_CLOCK_SPEED     (60*1000*1000)
#define ARGB8888_TO_BGR565(x)      \
     ((((x) >> 19) & 0x1F)       | \
     ((((x) >> 10) & 0x3F) <<  5)| \
      (((x) >>  3) & 0x1F) << 11)

struct spi_panel_data {
        struct spi_device   *spidev;
        int byte_per_frame;
        int lcm_width;
        int lcm_height;
        struct mutex spi_tx_mutex;
        void *tx_buf;
        void *tx_buf_bak;
        int disp_wrx_gpio;
        int disp_rst_gpio;
        int disp_te_irq;
        int kickoff_delay;   //for debug cut screen issue, note: TE_freq <= 2 * (platform frame freq)
        struct mutex te_mutex;
        struct mutex buf_mutex;
        struct completion disp_te_comp;
        wait_queue_head_t kickoff_waitq;
        atomic_t thread_flag;
};

static struct spi_panel_data *ctrl_pdata;

static inline void lcd_spi_set_data_mode(unsigned char rs)
{
        gpio_set_value(ctrl_pdata->disp_wrx_gpio, rs);
}

void lcd_spi_lock(void)
{
        if (!ctrl_pdata)
                return;
        mutex_lock(&ctrl_pdata->spi_tx_mutex);
}
void lcd_spi_unlock(void)
{
        if (!ctrl_pdata)
                return;
        mutex_unlock(&ctrl_pdata->spi_tx_mutex);
}

void mt_lcd_spi_write_com(char cmd)
{
        struct spi_message m;
        struct spi_transfer t = {
                .speed_hz = lcd_SPI_CLOCK_SPEED,
                .len = 1,
                .bits_per_word = 8,
        };
        if (ctrl_pdata == NULL)
                return;

        lcd_spi_set_data_mode(0);
        t.tx_buf = &cmd;
        spi_message_init(&m);
        spi_message_add_tail(&t, &m);
        spi_sync(ctrl_pdata->spidev, &m);
}

void mt_lcd_spi_write_data(char data)
{
        struct spi_message m;
        struct spi_transfer t = {
                .speed_hz = lcd_SPI_CLOCK_SPEED,
                .len = 1,
                .bits_per_word = 8,
        };
        if (ctrl_pdata == NULL)
                return;
        lcd_spi_set_data_mode(1);
        t.tx_buf = &data;
        spi_message_init(&m);
        spi_message_add_tail(&t, &m);
        spi_sync(ctrl_pdata->spidev, &m);
}


void mt_lcd_spi_write_reg(unsigned char cmd, unsigned char *val, unsigned char len)
{
        struct spi_message m;
        struct spi_transfer t = {
                .speed_hz = lcd_SPI_CLOCK_SPEED,
                .len = len,
                .bits_per_word = 8,
        };
        if (ctrl_pdata == NULL)
                return;
        mt_lcd_spi_write_com(cmd);
        if (len == 0)
                return;
        lcd_spi_set_data_mode(1);
        t.tx_buf = val;
        spi_message_init(&m);
        spi_message_add_tail(&t, &m);
        spi_sync(ctrl_pdata->spidev, &m);
}

static void lcm_set_blocksize(u16 x, u16 y, u16 w, u16 h)
{
        unsigned char data[32] = { 0 };

        w = w - 1;
        h = h - 1;
        data[0] = (x >> 8);
        data[1] = (x & 0xFF);
        data[2] = (w >> 8);
        data[3] = (w & 0xFF);
        mt_lcd_spi_write_reg(0x2a, data, 4);
        data[0] = (y >> 8);
        data[1] = (y & 0xFF);
        data[2] = (h >> 8);
        data[3] = (h & 0xFF);
        mt_lcd_spi_write_reg(0x2b, data, 4);
        mt_lcd_spi_write_reg(0x2c, data, 0);
}

static void WriteDispData(u32 length, void *screen_data)
{
        u8 status = 0;
        struct spi_message m;
        struct spi_transfer t = {
                .speed_hz = lcd_SPI_CLOCK_SPEED,
                .len = length,
                .bits_per_word = 16,
        };
        lcd_spi_set_data_mode(1);
        t.tx_buf = screen_data;
        t.rx_buf = NULL;
        spi_message_init(&m);
        spi_message_add_tail(&t, &m);
        status = spi_sync(ctrl_pdata->spidev, &m);
        if (status < 0) {
                pr_err("%s write data error status = %d\n", __func__, status);
        }
}

static irqreturn_t spi_panel_te_handler(int irq, void *data)
{
    if (!ctrl_pdata)
            return IRQ_HANDLED;

    complete(&ctrl_pdata->disp_te_comp);
    return IRQ_HANDLED;
}

void spi_send_data(u16 x, u16 y, u16 w, u16 h, void *screen_data)
{
        
        int i = 0;
        u16 *frame = (u16*)screen_data;

        if (ctrl_pdata == NULL)
                return;

        for (i = 0; i < w * h; i++)
                frame[i] = (frame[i] << 8) | (frame[i] >> 8);

        lcd_spi_lock();
        WriteDispData(w * h * 2, screen_data);
        lcd_spi_unlock();
}

void spi_lcm_update(u16 x, u16 y, u16 w, u16 h)
{
        pr_info("spi_lcm_update ++");
        lcm_set_blocksize(x, y, w, h);
}

bool mt_lcd_spi_gpio_init(void)
{
        int rc = 0;
        struct device_node *node;

        if (!ctrl_pdata)
                return false;

        if (gpio_is_valid(ctrl_pdata->disp_wrx_gpio)) {
                rc = gpio_request(ctrl_pdata->disp_wrx_gpio, "lcm-wrx-gpio");
                if (rc < 0) {
                        dev_err(&ctrl_pdata->spidev->dev, "lcm-wrx-gpio request failed\n");
                        return false;
                }
                rc = gpio_direction_output(ctrl_pdata->disp_wrx_gpio, 1);
                if (rc < 0) {
                        dev_err(&ctrl_pdata->spidev->dev, "lcm-wrx-gpio set output failed\n");
                        return false;
                }
        } else {
                pr_err("lcm-wrx-gpio invalid\n");
        }

        node = of_find_compatible_node(NULL, NULL, "mediatek, DSI_TE-eint");
        if (node) {
                ctrl_pdata->disp_te_irq = irq_of_parse_and_map(node, 0);
                if (request_irq(ctrl_pdata->disp_te_irq, spi_panel_te_handler, IRQF_TRIGGER_RISING, "DSI_TE-eint", NULL)) {
                        pr_err("request dsi_te irq failed\n");
                        return false;
                }
        } else {
                return false;
        }
        return true;
}

void spi_te_irq_enable(bool enable)
{
        static bool te_irq_enable = false;

        if (enable && te_irq_enable)
                return;

        if (!enable && !te_irq_enable)
                return;

        if (ctrl_pdata) {
                if (enable) {
                        enable_irq(ctrl_pdata->disp_te_irq);
                        te_irq_enable = true;
                        udelay(ctrl_pdata->kickoff_delay);
                } else {
                        disable_irq(ctrl_pdata->disp_te_irq);
                        te_irq_enable = false;
                }
        }
}

int spi_display_kickoff(uint8_t *txbuf, uint32_t pitch, uint32_t size, enum DISP_FORMAT src_fmt)
{
        int rc = -1;
        uint32_t height = 0;
        uint8_t Bpp = 2;
        uint32_t i = 0, j = 0, k = 0;
        uint32_t *bufva = NULL;
        uint16_t *lcdbuf = NULL;

        if (!ctrl_pdata) {
                return rc;
        }
        lcdbuf = (uint16_t *)(ctrl_pdata->tx_buf);

        mutex_lock(&ctrl_pdata->buf_mutex);
        if (DISP_FORMAT_RGB565 == src_fmt) {
                Bpp = 2;
                if (size != pitch * ctrl_pdata->lcm_height * Bpp) {
                        goto size_err;
                }
                for (height = 0; height < ctrl_pdata->lcm_height; height++) {
                    memcpy(ctrl_pdata->tx_buf + height * ctrl_pdata->lcm_width * Bpp, txbuf + height * pitch * Bpp, ctrl_pdata->lcm_width * Bpp);
                }
        } else {
                Bpp = 4;
                if (size != pitch * ctrl_pdata->lcm_height * Bpp) {
                        goto size_err;
                }
                for(i = 0; i < ctrl_pdata->lcm_height; i++) {
                       bufva = (uint32_t*)txbuf + i * pitch;
                       for(j = 0; j < ctrl_pdata->lcm_width; j++) {
                               lcdbuf[k++] = ARGB8888_TO_BGR565(*(bufva + j));
                       }
                }
        }
        mutex_unlock(&ctrl_pdata->buf_mutex);
        atomic_set(&ctrl_pdata->thread_flag, 1);
        wake_up_interruptible(&ctrl_pdata->kickoff_waitq);
        return 0;

size_err:
        pr_err("spi lcm don't support partial display, pitch %d, size %d, src_fmt %d\n", pitch, size, src_fmt);
        mutex_unlock(&ctrl_pdata->buf_mutex);
        return rc;
}

static int spi_display_thread(void *data)
{
        int rc = 0;
        struct sched_param param = { .sched_priority = 94 };
        sched_setscheduler(current, SCHED_FIFO, &param);

        for( ;; ) {
                wait_event_interruptible(ctrl_pdata->kickoff_waitq, atomic_read(&ctrl_pdata->thread_flag));
                atomic_set(&ctrl_pdata->thread_flag, 0);

                mutex_lock(&ctrl_pdata->te_mutex);
                spi_te_irq_enable(true);
                reinit_completion(&ctrl_pdata->disp_te_comp);
                rc = wait_for_completion_timeout(&ctrl_pdata->disp_te_comp, msecs_to_jiffies(100));
                if (rc == 0) {
                    pr_err("wait panel TE time out\n");
                }
                mutex_lock(&ctrl_pdata->buf_mutex);
                memcpy(ctrl_pdata->tx_buf_bak, ctrl_pdata->tx_buf, ctrl_pdata->byte_per_frame);
                mutex_unlock(&ctrl_pdata->buf_mutex);
                spi_send_data(0, 0, 240, 320, ctrl_pdata->tx_buf_bak);
                mutex_unlock(&ctrl_pdata->te_mutex);
        }

        pr_info("exit spi display thread\n");
        return 0;
}

static ssize_t show_kickoff_delay(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "kick off delay (%d) ms\n", ctrl_pdata->kickoff_delay);
}

static ssize_t store_kickoff_delay(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t size)
{
    sscanf(buf, "%d", &ctrl_pdata->kickoff_delay);
    return size;
}

static DEVICE_ATTR(kickoff_delay, 0664, show_kickoff_delay, store_kickoff_delay);

static int lcm_spi_panel_probe(struct spi_device *spidev)
{
        int rc = 0;
        struct pinctrl *lcm_spi_pinctrl;
        struct pinctrl_state *spi_all;
        unsigned int lcm_width = 0;
        unsigned int lcm_height = 0;
        unsigned int lcm_bpp = 0;

        int irq;
        int cs;
        int cpha, cpol, cs_high;
        u32 max_speed;

        irq = spidev->irq;
        cs = spidev->chip_select;
        cpha = (spidev->mode & SPI_CPHA) ? 1:0;
        cpol = (spidev->mode & SPI_CPOL) ? 1:0;
        cs_high = (spidev->mode & SPI_CS_HIGH) ? 1:0;
        max_speed = spidev->max_speed_hz;
        pr_info("cs[%x] CPHA[%x] CPOL[%x] CS_HIGH[%x] Max_speed[%d]\n",
            cs, cpha, cpol, cs_high, max_speed);

        if (!spidev->dev.of_node) {
                pr_err("SPI driver only supports device tree probe\n");
                return -ENOTSUPP;
        }

        lcm_spi_pinctrl = devm_pinctrl_get(&(spidev->dev));
        if (IS_ERR(lcm_spi_pinctrl)) {
                rc = PTR_ERR(lcm_spi_pinctrl);
                pr_err("Cannot find lcd spi pinctrl!\n");
        }
        spi_all = pinctrl_lookup_state(lcm_spi_pinctrl, "state_spi_all");

        if (IS_ERR(spi_all)) {
                rc = PTR_ERR(spi_all);
                pr_err("Cannot find state_spi_all %d!\n", rc);
        } else {
                rc = pinctrl_select_state(lcm_spi_pinctrl, spi_all);
                pr_info("%s: state_spi_all select rc = %d\n", __func__, rc);
        }

        ctrl_pdata = spi_get_drvdata(spidev);
        if (!ctrl_pdata) {
                ctrl_pdata = devm_kzalloc(&spidev->dev,
                                          sizeof(struct spi_panel_data),
                                          GFP_KERNEL);
                if (!ctrl_pdata) {
                        pr_err("%s: FAILED: cannot alloc spi panel\n",
                               __func__);
                        rc = -ENOMEM;
                        goto error_no_mem;
                }
                spi_set_drvdata(spidev, ctrl_pdata);
        }

        ctrl_pdata->spidev = spidev;
        if (of_property_read_u32(spidev->dev.of_node, "lcm-width", &lcm_width)) {
                dev_err(&spidev->dev, "not define lcm-width\n");
                return -ENXIO;
        }
        if (of_property_read_u32(spidev->dev.of_node, "lcm-height", &lcm_height)) {
                dev_err(&spidev->dev, "not define lcm-height\n");
                return -ENXIO;
        }
        if (of_property_read_u32(spidev->dev.of_node, "lcm-bpp", &lcm_bpp)) {
                dev_err(&spidev->dev, "not define lcm-bpp\n");
                return -ENXIO;
        }
        ctrl_pdata->lcm_width = lcm_width;
        ctrl_pdata->lcm_height = lcm_height;
        ctrl_pdata->byte_per_frame = lcm_width * lcm_height * lcm_bpp / 8;

        ctrl_pdata->disp_wrx_gpio = of_get_named_gpio(spidev->dev.of_node, "lcm-wrx-gpio", 0);

        init_completion(&ctrl_pdata->disp_te_comp);
        mutex_init(&ctrl_pdata->te_mutex);
        mutex_init(&ctrl_pdata->spi_tx_mutex);
        mutex_init(&ctrl_pdata->buf_mutex);
        atomic_set(&ctrl_pdata->thread_flag, 0);

        ctrl_pdata->tx_buf = kmalloc(ctrl_pdata->byte_per_frame + 64, GFP_KERNEL | GFP_DMA);
        ctrl_pdata->tx_buf = (void *)ALIGN_TO((unsigned long)(ctrl_pdata->tx_buf), 64);
        ctrl_pdata->tx_buf_bak = kmalloc(ctrl_pdata->byte_per_frame + 64, GFP_KERNEL | GFP_DMA);
        ctrl_pdata->tx_buf_bak = (void *)ALIGN_TO((unsigned long)(ctrl_pdata->tx_buf_bak), 64);

        ctrl_pdata->spidev->mode = SPI_MODE_0;
        ctrl_pdata->spidev->bits_per_word = 8;
        ctrl_pdata->spidev->max_speed_hz = lcd_SPI_CLOCK_SPEED;

        device_create_file(&spidev->dev, &dev_attr_kickoff_delay);
        ctrl_pdata->kickoff_delay = 100;

        init_waitqueue_head(&ctrl_pdata->kickoff_waitq);
        kthread_run(spi_display_thread, NULL, "spi_display_thread");

        if (!mt_lcd_spi_gpio_init())
                goto error_gpio;

        pr_info("%s: spi panel  initialized\n", __func__);
        return 0;

error_gpio:
error_no_mem:
        devm_kfree(&spidev->dev, ctrl_pdata);
        ctrl_pdata = NULL;
        return rc;
}

static int lcm_spi_panel_remove(struct spi_device *spidev)
{
        return 0;
}

static const struct of_device_id lcm_spi_panel_match[] = {
        { .compatible = "mediatek,lcd-spi-display", },
        {},
};
MODULE_DEVICE_TABLE(of, lcm_spi_panel_match);

static const struct spi_device_id lcd_spi_id[] = {
    {"lcd_spi", 0},
    {},
};

static struct spi_driver spi_display_driver = {
        .probe = lcm_spi_panel_probe,
        .remove = lcm_spi_panel_remove,
        .driver = {
                .name = "lcd_spi",
                .bus = &spi_bus_type,
                .owner  = THIS_MODULE,
                .of_match_table = of_match_ptr(lcm_spi_panel_match),
        },
        .id_table = lcd_spi_id,
};

static int __init lcm_spi_display_init(void)
{
        int status = 0;

        status = spi_register_driver(&spi_display_driver);
        if (status) {
                pr_err("%s, Failed to register SPI driver.\n", __func__);
                return -EINVAL;
        }
        return status;
}
subsys_initcall(lcm_spi_display_init);

static void __exit lcm_spi_display_exit(void)
{
        spi_unregister_driver(&spi_display_driver);
}
module_exit(lcm_spi_display_exit);

MODULE_DESCRIPTION("The device control driver for lcd spi");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("wang.zhou@tcl.com");
