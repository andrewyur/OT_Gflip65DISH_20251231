/* Copyright (C) 2019 Tcl Corporation Limited */
/*
 * sn7326_keypad.c - sn7326 Key Switch Controller Driver
 *
 * Copyright (C) 2009 Samsung Electronics
 * Kim Kyuwon <q1.kim@samsung.com>
 *
 * Based on pxa27x_keypad.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Datasheet: http://www.maxim-ic.com/quick_view2.cfm/qv_pk/5456
 */
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/pm.h>
#include <linux/input.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/delay.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/of_regulator.h>
#include <linux/input/matrix_keypad.h>
#include <linux/bitops.h>

#define SN7326_MAX_KEY_ROWS	8
#define SN7326_MAX_KEY_COLS	8
#define SN7326_MAX_KEY_NUM	(SN7326_MAX_KEY_ROWS * SN7326_MAX_KEY_COLS)
#define SN7326_ROW_SHIFT	3
#define SN7326_MAXVALUE_KEY_NUM 240
/*
 * sn7326 registers
 */
#define SN7326_REG_KEYFIFO	0x10
#define SN7326_REG_CONFIG	0x08

/*
 * Configuration register bits
 */
#define SN7326_CFG_INTERRUPT	(0 << 6)|(0 << 5) // MODIFIED by hongwei.tian, 2019-03-27,BUG-7481359
#define SN7326_CFG_DEBOUNCE_ENABLE	(1 << 4)
#define SN7326_CFG_DEBOUNCE_TIME	(1 << 3)
/* MODIFIED-BEGIN by hongwei.tian, 2018-11-05,BUG-7100204*/
#define SN7326_CFG_LONG_KEY	(1 << 2)


#define SN7326_KEY_ROW(k)		(((k) >> 16) & 0x7)
#define SN7326_KEY_COL(k)		((k)>>12 & 0x7)
#define SN7326_KEY_VAL(k)		((k) & 0xfff)

#define SN7326_MATRIX_SCAN_CODE(row, col, row_shift)	(((row) << (row_shift)) + (col))

//extern int i2c_check_status_create(char *name,int value); // MODIFIED by hongwei.tian, 2019-01-23,BUG-7397749

int suspend_flag = 0;
/* MODIFIED-END by hongwei.tian,BUG-7100204*/

struct sn7326_keypad {
	struct i2c_client *client;
	struct device			*dev;
	struct input_dev *input_dev;
	/* matrix key code map */
	unsigned int keycodes[SN7326_MAX_KEY_NUM];
	unsigned int *keyvalue;
	int key_len;
	int rst_pin;
	unsigned short int device_id;

	struct mutex reg_mutex;
	struct delayed_work scan_esd;
	bool key_scan;
};
/*BEGIN add by wang.zhou for GF6TF-9911 */
extern bool sub_lcd_work;
/*END add by wang.zhou for GF6TF-9911 */
#define KPD_SCAN           1
#define KPD_ESD            2
#define ESD_TIMEOUT_MS     3000
#define SN7325_TIMERE_MS   20
#define KEYBOARD_SN7325    0x7325
#define KEYBOARD_SN7326    0x7326
static void kpd_scan_worker(struct work_struct *work);

static int sn7326_write_reg(struct i2c_client *client, u8 reg, u8 val)
{
	int ret = i2c_smbus_write_byte_data(client, reg, val);

	if (ret < 0)
		dev_err(&client->dev, "%s: reg 0x%x, val 0x%x, err %d\n",
			__func__, reg, val, ret);
	return ret;
}

static int sn7326_read_reg(struct i2c_client *client, int reg)
{
	int ret = i2c_smbus_read_byte_data(client, reg);

	if (ret < 0)
		dev_err(&client->dev, "%s: reg 0x%x, err %d\n",
			__func__, reg, ret);
	return ret;
}

static void sn7326_build_keycode(struct sn7326_keypad *keypad)
{
	struct input_dev *input_dev = keypad->input_dev;
	int i;

	for (i = 0; i < keypad->key_len; i++) {
		unsigned int key = keypad->keyvalue[i];
		unsigned int row = SN7326_KEY_ROW(key);
		unsigned int col = SN7326_KEY_COL(key);
		unsigned int scancode = SN7326_MATRIX_SCAN_CODE(row, col,
						SN7326_ROW_SHIFT);
		unsigned short keycode = SN7326_KEY_VAL(key);
		//printk("#####row:%d,col:%d,scancod:%d,key:%x\n",row,col,scancode,keycode);
		keypad->keycodes[scancode] = keycode;
		/*if (5 == row && 3 == col)
			keypad->keycodes[scancode] = 28;*/
		__set_bit(keypad->keycodes[scancode], input_dev->keybit);
	}

	__clear_bit(KEY_RESERVED, input_dev->keybit);
}

static void sn7325_key_scan(struct sn7326_keypad *keypad, bool irq)
{
	struct input_dev *input_dev;
	int row = 0, col = 0, code = 0, dt_ms = 0;
	u16 bMask = 0, i = 0, bKeyVal = 0;
	u8 release = 0;
	static int last_ms = 0, val = 0, old_val = 0;

	if (!keypad)
		return;

	if (true == irq) {
		last_ms = 0;
		val = 0;
		old_val = 0;
	}
	input_dev = keypad->input_dev;
	sn7326_read_reg(keypad->client, 0x00);
	sn7326_write_reg(keypad->client, 0x06, 0xff);
	bMask = 0x0001;
        for (i = 0; i < 4; i++) {
		sn7326_write_reg(keypad->client, 0x03, ~bMask);
		bKeyVal = (sn7326_read_reg(keypad->client, 0x00) & 0x003f); //row OD0~0D5
		if (bKeyVal != 0x003f)
		{
			val = (bMask << 8);
			val |= ((~bKeyVal) & 0x00ff);
			break;
		}
		bMask <<= 1;
        }
	sn7326_write_reg(keypad->client, 0x03, 0x00);
        /*
         * 0x3f: use OD0 ~ OD6
         * 0x0f: use PP0 ~ PP3
         */
	if ((ffs(val & 0x3f) != fls(val & 0x3f)) || (ffs((val >> 8) & 0xf) != fls((val >> 8) & 0xf))) {
		//we don't support multiple press key
		dev_err(&keypad->client->dev, "multiple key press val 0x%x\n", val);
		val = old_val;
		i = 4;
	}
	if (4 == i) {
		release = 1;
		dt_ms = jiffies_to_msecs(jiffies) - last_ms;
		if (dt_ms > 1000) {
		        sn7326_write_reg(keypad->client, 0x06, 0x00); //open keypad interrupt
		        dev_info(&keypad->client->dev, "enable keypad ird\n");
		        goto esdtimer;
		}
	} else {
		last_ms = jiffies_to_msecs(jiffies);
		//if long press key is diffrent, we need release before key
		if (old_val != val && old_val) {
			release = 1;
			row = ffs(old_val & 0x3f) - 1;
			col = ffs((old_val >> 8) & 0xf) - 1;
			code = SN7326_MATRIX_SCAN_CODE(row, col, SN7326_ROW_SHIFT);
			dev_dbg(&keypad->client->dev, "key[%d:%d] %s\n", row, col, !!release ? "auto release" : "press");
			input_event(input_dev, EV_MSC, MSC_SCAN, code);
			input_report_key(input_dev, keypad->keycodes[code], !release);
			input_sync(input_dev);
		}
		release = 0;
	}
	row = ffs(val & 0x3f) - 1;
	col = ffs((val >> 8) & 0xf) - 1;
	code = SN7326_MATRIX_SCAN_CODE(row, col, SN7326_ROW_SHIFT);
	//release once
	if (!old_val && release) {
		keypad->key_scan = true;
		mod_delayed_work(system_wq, &keypad->scan_esd, msecs_to_jiffies(SN7325_TIMERE_MS));
		return;
	}
	if (release) {
		old_val = 0;
	} else {
		old_val = val;
	}
	if (!suspend_flag) {
		if (sub_lcd_work) {
			pr_info("sub_lcd_work, skip keyboard key\n");
		} else {
			dev_dbg(&keypad->client->dev, "key[%d:%d] %s\n", row, col, !!release ? "release" : "press");
			input_event(input_dev, EV_MSC, MSC_SCAN, code);
			input_report_key(input_dev, keypad->keycodes[code], !release);
			input_sync(input_dev);
		}
	}

	keypad->key_scan = true;
	mod_delayed_work(system_wq, &keypad->scan_esd, msecs_to_jiffies(SN7325_TIMERE_MS));
	return;

esdtimer:
	keypad->key_scan = false;
	mod_delayed_work(system_wq, &keypad->scan_esd, msecs_to_jiffies(ESD_TIMEOUT_MS));
}
/* runs in an IRQ thread -- can (and will!) sleep */
static irqreturn_t sn7326_interrupt(int irq, void *dev_id)
{
	struct sn7326_keypad *keypad = dev_id;
	struct input_dev *input_dev = keypad->input_dev;
	int row = 0, col = 0, release = 0, code = 0, num = 0;
	int val = 0;

/* MODIFIED-BEGIN by hongwei.tian, 2019-08-03,BUG-8170402*/
	//printk(KERN_ERR "key sn7326_interrupt\n");
	if(suspend_flag)
		msleep(50);

	keypad->key_scan = false;
	cancel_delayed_work(&keypad->scan_esd);

	mutex_lock(&keypad->reg_mutex);
        if (KEYBOARD_SN7326 == keypad->device_id) {
		do{
			val = sn7326_read_reg(keypad->client, SN7326_REG_KEYFIFO);
			if(val < 0)
			{
				printk(KERN_ERR "key Invalid key press\n");
				break;
			}
			/*BEGIN add by wang.zhou for GF6TF-9911 */
			if (sub_lcd_work) {
				pr_info("sub_lcd_work, skip keyboard key\n");
				continue;
			}
			/*END add by wang.zhou for GF6TF-9911 */
			/* MODIFIED-END by hongwei.tian,BUG-8170402*/
			col = val & 0x7;
			row= (val >> 3) & 0x7;
			release = val & 0x40;
			num = val & 0x80;

			code = SN7326_MATRIX_SCAN_CODE(row, col, SN7326_ROW_SHIFT);
			if(!suspend_flag){
				dev_dbg(&keypad->client->dev,
				"key[%d:%d] %s\n", row, col, !!release ? "release" : "press");
				input_event(input_dev, EV_MSC, MSC_SCAN, code);
				input_report_key(input_dev, keypad->keycodes[code], !!release);
				input_sync(input_dev);
			}
			/* MODIFIED-END by hongwei.tian,BUG-7100204*/
		}while(num);
	} else if (KEYBOARD_SN7325 == keypad->device_id) {
		keypad->key_scan = true;
		sn7325_key_scan(keypad, true);
	}
	mutex_unlock(&keypad->reg_mutex);
	return IRQ_HANDLED;
}
#if 0
static int sn7326_open(struct input_dev *dev)
{
	//struct sn7326_keypad *keypad = input_get_drvdata(dev);

	return 0;
}

static void sn7326_close(struct input_dev *dev)
{
	//struct sn7326_keypad *keypad = input_get_drvdata(dev);
}
#endif
static void sn7326_initialize(struct i2c_client *client)
{
	sn7326_write_reg(client, SN7326_REG_CONFIG,
		SN7326_CFG_INTERRUPT  /* Irq clears after host read */
		|SN7326_CFG_LONG_KEY  /* detect long key */
		|SN7326_CFG_DEBOUNCE_TIME /* debounce time */
		|SN7326_CFG_DEBOUNCE_ENABLE);/*enable debounce*/
}
static int sn7326_config_gpio(struct sn7326_keypad *keypad)
{
	int err;

	/*request gpio*/
	if(gpio_is_valid(keypad->rst_pin)){
		err = gpio_request(keypad->rst_pin, "sn7326_rst");
		if (err) {
			dev_err(&keypad->client->dev, "gpio_request (pin) failed\n");
		}
	}
	err = gpio_direction_output(keypad->rst_pin, 1);
	if (err) {
		dev_err(&keypad->client->dev,
		"set_direction for reset gpio failed\n");
	}
	msleep(10);
	gpio_set_value(keypad->rst_pin, 0);
	msleep(40);
	gpio_set_value(keypad->rst_pin, 1);
	msleep(10);

	return err;

}
static int sn7326_parse_dt(struct sn7326_keypad *keypad)
{
	struct device_node *np = keypad->dev->of_node;
	struct property *prop;
	int rc;

	if (!np) {
		dev_err(keypad->dev, "device tree info. missing\n");
		return -EINVAL;
	}

	keypad->rst_pin = of_get_named_gpio(np, "sn,rest-gpio", 0);
	if ((!gpio_is_valid(keypad->rst_pin)))
		return -EINVAL;

	/*get keymap value*/
	prop = of_find_property(np, "sn7326,keymap-value", NULL);
	if (prop && prop->value) {

		keypad->key_len = prop->length / sizeof(u32);

		keypad->keyvalue =devm_kzalloc(keypad->dev,SN7326_MAXVALUE_KEY_NUM, GFP_KERNEL);
		if (!keypad->keyvalue) {
			dev_err(keypad->dev, "Not enough memory for panel config data\n");
			return -ENOMEM;
		}

		rc = of_property_read_u32_array(np, "sn7326,keymap-value", keypad->keyvalue, keypad->key_len);
		if (rc) {
			dev_err(keypad->dev, "Failed to read key codes\n");
			return -EINVAL;
		}

	} else {
		dev_err(keypad->dev,
			"Unable to get configure data, default will be used.\n");
		keypad->key_len = 0;
	}
	return 0;
}

void kpd_scan_worker(struct work_struct *work)
{
	struct sn7326_keypad *keypad = container_of(work, struct sn7326_keypad,
	scan_esd.work);

	bool esd_enable = false;

	if (!keypad)
		return;
	mutex_lock(&keypad->reg_mutex);
	if (false == keypad->key_scan) {
		if (KEYBOARD_SN7325 == keypad->device_id) {
			if (sn7326_read_reg(keypad->client, 0x04) != 0x3f || \
				sn7326_read_reg(keypad->client, 0x05) != 0xf0) {
				esd_enable = true;
			}
		} else if (KEYBOARD_SN7326 == keypad->device_id) {
			if (sn7326_read_reg(keypad->client, SN7326_REG_KEYFIFO) < 0) {
				esd_enable = true;
			}
		}
		if (esd_enable) {
			dev_err(&keypad->client->dev, "eds trigger, keypad id 0x%x\n", keypad->device_id);
			gpio_set_value(keypad->rst_pin, 0);
			msleep(40);
			gpio_set_value(keypad->rst_pin, 1);
			msleep(10);
			if (KEYBOARD_SN7325 == keypad->device_id) {
				//config sn7325 PP0-PP3 output
				sn7326_write_reg(keypad->client, 0x05, 0xf0);
				//config sn7325 PP0-PP3 output low
				sn7326_write_reg(keypad->client, 0x03, 0x00);
				//Disable PP0-PP1 interrupt
				sn7326_write_reg(keypad->client, 0x07, 0xff);

				//config sn7325 OD0-OD5 input
				sn7326_write_reg(keypad->client, 0x04, 0x3f);
				//enable OD0-OD5 interrupt
				sn7326_write_reg(keypad->client, 0x06, 0x00);
			} else if (KEYBOARD_SN7326 == keypad->device_id) {
				if (sn7326_read_reg(keypad->client, SN7326_REG_KEYFIFO) < 0) {
					dev_err(&keypad->client->dev, "failed to detect device\n");
				} else {
					sn7326_initialize(keypad->client);
				}
			}
		}
		keypad->key_scan = false;
		mod_delayed_work(system_wq, &keypad->scan_esd, msecs_to_jiffies(ESD_TIMEOUT_MS));
	} else {
		sn7325_key_scan(keypad, false);
	}
	mutex_unlock(&keypad->reg_mutex);
}

static int sn7326_probe(struct i2c_client *client,
					const struct i2c_device_id *id)
{
	struct sn7326_keypad *keypad;
	struct input_dev *input_dev;
	int ret;
	int error;
	int key_info;
        struct pinctrl *keypad_pinctrl;
        struct pinctrl_state *i2c_all;

	keypad = devm_kzalloc(&client->dev, sizeof(*keypad), GFP_KERNEL);
	if (!keypad) {
		dev_err(&client->dev, "Unable to allocate memory\n");
		return -ENOMEM;
	}

	keypad->client = client;
	keypad->dev = &client->dev;

        keypad_pinctrl = devm_pinctrl_get(&client->dev);
        if (IS_ERR(keypad_pinctrl)) {
                ret = PTR_ERR(keypad_pinctrl);
                dev_err(&client->dev, "Cannot find keypad_pinctrl!\n");
        }
        i2c_all = pinctrl_lookup_state(keypad_pinctrl, "state_i2c_all");

        if (IS_ERR(i2c_all)) {
                ret = PTR_ERR(i2c_all);
                dev_err(&client->dev, "Cannot find state_i2c_all %d!\n", ret);
        } else {
                ret = pinctrl_select_state(keypad_pinctrl, i2c_all);
                dev_info(&client->dev, "%s: state_i2c_all select ret = %d\n", __func__, ret);
        }

	ret= sn7326_parse_dt(keypad);
	if (ret < 0) {
		dev_err(&client->dev, "Unable to parse DT nodes\n");
		return ret;
	}

	ret=sn7326_config_gpio(keypad);
	if (ret < 0) {
		dev_err(&client->dev, "failed to detect device\n");
		return -ENODEV;
	}
	//config sn7325 OD0-OD5 output
	sn7326_write_reg(keypad->client, 0x04, 0xc0);
	//config sn7325 OD0-OD5 output is 0xc0
	sn7326_write_reg(keypad->client, 0x02, 0xc0);
	/*read 0x00 register status for chip selection*/
	key_info = sn7326_read_reg(keypad->client, 0x00);
	dev_info(&client->dev, "sn732x key_info %d\n", key_info);
	if (key_info <= 0xc0) {  /*chip is sn7325, 0x00 register is OD status*/
		//begin initialize sn7325 register
		//config sn7325 PP0-PP3 output
		sn7326_write_reg(keypad->client, 0x05, 0xf0);
		//config sn7325 PP0-PP3 output low
		sn7326_write_reg(keypad->client, 0x03, 0x00);
		//Disable PP0-PP1 interrupt
		sn7326_write_reg(keypad->client, 0x07, 0xff);

		//config sn7325 OD0-OD5 input
		sn7326_write_reg(keypad->client, 0x04, 0x3f);
		//enable OD0-OD5 interrupt
		sn7326_write_reg(keypad->client, 0x06, 0x00);
		keypad->device_id = 0x7325;
		//end initialize sn7325 register
	} else {
		/* Detect sn7326: The initial Keys FIFO value is '0x10' */
		key_info = sn7326_read_reg(keypad->client, SN7326_REG_KEYFIFO);
		if (key_info < 0) {
			dev_err(&client->dev, "failed to detect device %d\n", key_info);
			return -ENODEV;
		}
		keypad->device_id = 0x7326;
		sn7326_initialize(client);
	}

	dev_dbg(&client->dev, "keys FIFO is 0x%02x\n", ret);

	input_dev = input_allocate_device();
	if (!keypad || !input_dev) {
		dev_err(&client->dev, "failed to allocate memory\n");
		error = -ENOMEM;
		goto failed_free_mem;
	}

	keypad->key_scan = false;
	mutex_init(&keypad->reg_mutex);
	INIT_DELAYED_WORK(&keypad->scan_esd, kpd_scan_worker);

	keypad->input_dev = input_dev;

	input_dev->name = client->name;
	input_dev->id.bustype = BUS_I2C;
	input_dev->open = NULL;//sn7326_open;
	input_dev->close = NULL;//sn7326_close;
	input_dev->dev.parent = &client->dev;

	input_dev->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_REP);
	input_dev->keycodesize = sizeof(keypad->keycodes[0]);
	input_dev->keycodemax = ARRAY_SIZE(keypad->keycodes);
	input_dev->keycode = keypad->keycodes;

	input_set_capability(input_dev, EV_MSC, MSC_SCAN);
	input_set_drvdata(input_dev, keypad);

	sn7326_build_keycode(keypad);
	dev_info(&client->dev, "keyboard-irq %d\n", client->irq);
	error = request_threaded_irq(client->irq, NULL, sn7326_interrupt,
				     IRQF_TRIGGER_LOW | IRQF_ONESHOT ,
				     client->name, keypad);
	if (error) {
		dev_err(&client->dev, "failed to register interrupt\n");
		goto failed_free_mem;
	}

	/* Register the input device */
	error = input_register_device(input_dev);
	if (error) {
		dev_err(&client->dev, "failed to register input device\n");
		goto failed_free_irq;
	}

        device_init_wakeup(&client->dev, 1);

        if (device_may_wakeup(&client->dev))
		enable_irq_wake(client->irq);
        else
		enable_irq(client->irq);

	printk("######sn7326 success keyinfo = %x, chip info 0x%x #######\n",key_info, keypad->device_id);

	return 0;

failed_free_irq:
	free_irq(client->irq, keypad);
failed_free_mem:
	input_free_device(input_dev);
	kfree(keypad);
	return error;
}

static int sn7326_remove(struct i2c_client *client)
{
	struct sn7326_keypad *keypad = i2c_get_clientdata(client);

	free_irq(client->irq, keypad);
	input_unregister_device(keypad->input_dev);
	kfree(keypad);

	return 0;
}
#ifdef CONFIG_PM
static int sn7326_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
		printk(KERN_ERR "key sn7326_suspend\n"); // MODIFIED by hongwei.tian, 2019-08-03,BUG-8170402
        suspend_flag=1; // MODIFIED by hongwei.tian, 2018-11-05,BUG-7100204
	if (device_may_wakeup(&client->dev))
		enable_irq_wake(client->irq);
	return 0;
}

static int sn7326_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
		printk(KERN_ERR "key sn7326_resume\n"); // MODIFIED by hongwei.tian, 2019-08-03,BUG-8170402
        suspend_flag=0; // MODIFIED by hongwei.tian, 2018-11-05,BUG-7100204
	if (device_may_wakeup(&client->dev))
		disable_irq_wake(client->irq);
	return 0;
}
#endif

static SIMPLE_DEV_PM_OPS(sn7326_pm, sn7326_suspend, sn7326_resume);

static struct of_device_id sn7326_match_table[] = {
	{ .compatible = "sn7326,matrix-keypad", },
	{ },
};

static const struct i2c_device_id sn7326_ids[] = {
	{ "sn7326", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, sn7326_ids);

static struct i2c_driver sn7326_i2c_driver = {
	.driver = {
		.name = "sn7326",
		.owner		= THIS_MODULE,
		.of_match_table = sn7326_match_table,
		.pm   = &sn7326_pm,
	},
	.probe		= sn7326_probe,
	.remove		= sn7326_remove,
	.id_table	= sn7326_ids,
};

module_i2c_driver(sn7326_i2c_driver);

MODULE_AUTHOR("Kim Kyuwon <q1.kim@samsung.com>");
MODULE_DESCRIPTION("sn7326 Key Switch Controller Driver");
MODULE_LICENSE("GPL v2");
