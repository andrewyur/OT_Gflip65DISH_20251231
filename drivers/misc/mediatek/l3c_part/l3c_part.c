
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/printk.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <mt-plat/sync_write.h>
#include <mt-plat/mtk_io.h>

#include <asm/arm_dsu_l3c.h>
#include <l3c_part.h>

static unsigned char init_done;
static struct platform_driver l3c_part_drv = {
	.driver = {
		.name = "l3c_part_drv",
		.owner = THIS_MODULE,
	},
};

static ssize_t l3c_part_ctrl_show(struct device_driver *driver, char *buf)
{
	ssize_t ret;

	ret = snprintf(buf, PAGE_SIZE, "%s:%x\n%s:%x\n%s:%x\n",
		"CLUSTERPARTCR_EL1", __dsu_l3c_read_partcr(),
		"CLUSTERACPSID_EL1", __dsu_l3c_read_acpsid(),
		"CLUSTERSTASHSID_EL1", __dsu_l3c_read_stashsid());

	return strlen(buf);
}

static DRIVER_ATTR_RO(l3c_part_ctrl);

static int __init mtk_l3c_part_init(void)
{
	int ret;

	__dsu_l3c_write_partcr(0x8F);
	__dsu_l3c_write_acpsid(0x1);
	__dsu_l3c_write_stashsid(0x1);

	ret = platform_driver_register(&l3c_part_drv);
	if (ret) {
		pr_info("%s: init fail, ret 0x%x\n", __func__, ret);
		return ret;
	}

	ret = driver_create_file(&l3c_part_drv.driver,
		&driver_attr_l3c_part_ctrl);
	if (ret) {
		pr_info("%s: fail to create l3c_part_ctrl\n", __func__);
		return ret;
	}

	init_done = 1;

	return 0;
}

static void __exit mtk_l3c_part_exit(void)
{
	platform_driver_unregister(&l3c_part_drv);
}

int mtk_l3c_set_mcu_part(unsigned int ratio)
{
	int val_part;

	if (!init_done)
		return -1;

	if ((ratio > 4) || (ratio == 0))
		return -1;

	val_part = __dsu_l3c_read_partcr();
	val_part &= ~0xF;
	val_part |= (1 << ratio) - 1;

	__dsu_l3c_write_partcr(val_part);

	return 0;
}

int mtk_l3c_set_acp_part(unsigned int ratio)
{
	int val_part;

	if (!init_done)
		return -1;

	if ((ratio > 4) || (ratio == 0))
		return -1;

	val_part = __dsu_l3c_read_partcr();
	val_part &= ~0xF0;
	val_part |= ((1 << ratio) - 1) << (8 - ratio);

	__dsu_l3c_write_partcr(val_part);

	return 0;
}

int mtk_l3c_get_part(unsigned int id)
{
	unsigned int val_part;

	val_part = __dsu_l3c_read_partcr();
	switch (id) {
	case MTK_L3C_PART_ACP:
		val_part >>= 4;
	case MTK_L3C_PART_MCU:
		break;
	default:
		return -1;
	}

	return (val_part & 0xF);
}

module_init(mtk_l3c_part_init);
module_exit(mtk_l3c_part_exit);

