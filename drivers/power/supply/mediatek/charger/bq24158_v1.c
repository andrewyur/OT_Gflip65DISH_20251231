// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * TI BQ24158 charger driver
 * Datasheets:
 * http://www.ti.com/product/bq24158
 */

#include <linux/alarmtimer.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/power_supply.h>
#include <linux/regmap.h>
#include <linux/types.h>
#include <linux/gpio/consumer.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/of_device.h>
#endif
#include <linux/regulator/driver.h>
#include <linux/version.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0))
#include <mtk_charger.h>
#include <charger_class.h>
#else
#include "mtk_charger_intf.h"
#include <mt-plat/mtk_boot_common.h>
#endif

#undef dev_dbg
#define dev_dbg dev_err

#define BQ24158_REG_1			0x00
#define BQ24158_REG_2			0x01
#define BQ24158_REG_3			0x02
#define BQ24158_REG_4			0x03
#define BQ24158_REG_5			0x04
#define BQ24158_REG_6			0x05
#define BQ24158_REG_7			0x06
#define BQ24158_REG_8			0x07

#define BQ24158_MANUFACTURER		"Texas Instruments"

/*
 * When adding support for new devices make sure that enum bq24158_chip and
 * bq24158_chip_name[] always stay in sync!
 */
enum bq24158_chip {
	BQUNKNOWN,
	BQ24157S,
	BQ24158,
	ETA6937
};

static const char *const bq24158_chip_name[] = {
	"unknown",
	"bq24157s",
	"bq24158",
	"eta6937"
};

enum bq24158_fields {
	F_TMR_RST_OTG, F_EN_STAT, F_STAT, F_BOOST, F_FAULT,	    /* REG 1 */
	F_LIN_LIMIT, F_LOWV, F_TE, F_CE, F_HZ_MODE, F_OPA_MODE,   /* REG 2 */
	F_VBAT, F_OTG_PL, F_OTG_EN,				    /* REG 3 */
	F_VENDER, F_PN, F_REVISION,				    /* REG 4 */
	F_RESET, F_ICHRG, ICHRG_OFFSET/*ETA*/, F_ITERM,  				    /* REG 5 */
	F_ICHRG2/*ETA*/, F_LOW_CHG, F_DPM_STATUS, F_CD_STATUS, F_VSREG,	    /* REG 6 */
	F_VMCHRG, F_VMREG,					    /* REG 7 */
	F_VINDPM, F_EN_ILIM2, F_IN_LIMIT_2,					    /* REG 8 */
	F_MAX_FIELDS
};

/* initial field values, converted from uV/uA */
struct bq24158_init_data {
	u32 ichg;	/* charge current      */
	u32 vbat;	/* regulation voltage  */
	u8 vbat_idx;
	u32 weak_vbat;	/* weak battery voltage */
	u8 weak_vbat_idx;
	u32 iterm;	/* termination current */
	u32 iilimit;	/* input current limit */
	u8 iilimit_idx;	
	u32 resistor_sense;		/* m ohm */
	u32 low_chg_current;
};

struct bq24158_state {
	u8 status;
	u8 fault;
	bool power_good;
};

struct bq24158_pinctrl_info {
	struct pinctrl *pinctrl;
	struct pinctrl_state *active;
	struct pinctrl_state *suspend;
};

struct bq24158_device {
	struct i2c_client *client;
	struct device *dev;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0))
	struct power_supply *charger;
	struct power_supply_desc charger_desc;
#endif
	struct charger_device *chg_dev;
	struct alarm otg_kthread_gtimer;
	struct workqueue_struct *otg_boost_workq;
	struct regulator_dev *otg_rdev;
	struct work_struct kick_work;
	unsigned int polling_interval;
	bool polling_enabled;
	bool cd_en;
	bool is_eta;
	/* Begin added by bitao.xiong for GF6CAN-2870 on 2022-03-24 */
	struct timespec charging_begin_time;
	int target_ichg;
	int current_ichg;
	bool is_kpoc;
	bool is_gflip6_series;
	/* End added by bitao.xiong for GF6CAN-2870 on 2022-03-24 */

	enum bq24158_chip chip;

	struct regmap *rmap;
	struct regmap_field *rmap_fields[F_MAX_FIELDS];

	struct bq24158_init_data init_data;
	struct bq24158_state state;

	struct mutex lock; /* protect state data */
	struct bq24158_pinctrl_info pinctrl;

	int bc12_sel;
	const char *chg_name;
	char *model;
};

static bool bq24158_is_volatile_reg(struct device *dev, unsigned int reg)
{
	switch (reg) {
	case BQ24158_REG_4:
		return false;

	default:
		return true;
	}
}

static const struct regmap_config bq24158_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,

	.max_register = BQ24158_REG_8,
	.cache_type = REGCACHE_RBTREE,

	.volatile_reg = bq24158_is_volatile_reg,
};

static const struct reg_field bq24158_reg_fields[] = {
	/* REG 1 */
	[F_TMR_RST_OTG]	= REG_FIELD(BQ24158_REG_1, 7, 7),
	[F_EN_STAT]		= REG_FIELD(BQ24158_REG_1, 6, 6),
	[F_STAT]		= REG_FIELD(BQ24158_REG_1, 4, 5),
	[F_BOOST]		= REG_FIELD(BQ24158_REG_1, 3, 3),
	[F_FAULT]		= REG_FIELD(BQ24158_REG_1, 0, 2),
	/* REG 2 */
	[F_LIN_LIMIT]		= REG_FIELD(BQ24158_REG_2, 6, 7),
	[F_LOWV]		= REG_FIELD(BQ24158_REG_2, 4, 5),
	[F_TE]			= REG_FIELD(BQ24158_REG_2, 3, 3),
	[F_CE]			= REG_FIELD(BQ24158_REG_2, 2, 2),
	[F_HZ_MODE]		= REG_FIELD(BQ24158_REG_2, 1, 1),
	[F_OPA_MODE]		= REG_FIELD(BQ24158_REG_2, 0, 0),
	/* REG 3 */
	[F_VBAT]		= REG_FIELD(BQ24158_REG_3, 2, 7),
	[F_OTG_PL]		= REG_FIELD(BQ24158_REG_3, 1, 1),
	[F_OTG_EN]		= REG_FIELD(BQ24158_REG_3, 0, 0),
	/* REG 4 */
	[F_VENDER]		= REG_FIELD(BQ24158_REG_4, 5, 7),
	[F_PN]			= REG_FIELD(BQ24158_REG_4, 3, 4),
	[F_REVISION]		= REG_FIELD(BQ24158_REG_4, 0, 2),
	/* REG 5 */
	[F_RESET]		= REG_FIELD(BQ24158_REG_5, 7, 7),
	[F_ICHRG]		= REG_FIELD(BQ24158_REG_5, 4, 6),
	[ICHRG_OFFSET]		= REG_FIELD(BQ24158_REG_5, 3, 3), /*ETA*/
	[F_ITERM]		= REG_FIELD(BQ24158_REG_5, 0, 2),
	/* REG 6 */
	[F_ICHRG2]		= REG_FIELD(BQ24158_REG_6, 6, 7), /*ETA*/
	[F_LOW_CHG]		= REG_FIELD(BQ24158_REG_6, 5, 5),
	[F_DPM_STATUS]			= REG_FIELD(BQ24158_REG_6, 4, 4),
	[F_CD_STATUS]		= REG_FIELD(BQ24158_REG_6, 3, 3),
	[F_VSREG]		= REG_FIELD(BQ24158_REG_6, 0, 2),
	/* REG 7 */
	[F_VMCHRG]		= REG_FIELD(BQ24158_REG_7, 4, 7),
	[F_VMREG]		= REG_FIELD(BQ24158_REG_7, 0, 3),
	/* REG 8 */
	[F_VINDPM]		= REG_FIELD(BQ24158_REG_8, 4, 7),
	[F_EN_ILIM2]		= REG_FIELD(BQ24158_REG_8, 3, 3),
	[F_IN_LIMIT_2]		= REG_FIELD(BQ24158_REG_8, 0, 2)
};

static const u32 bq24158_vbat_map[] = {
	3500000, 3520000, 3540000, 3560000, 3580000, 3600000, 3620000, 3640000,
	3660000, 3680000, 3700000, 3720000, 3740000, 3760000, 3780000, 3800000,
	3820000, 3840000, 3860000, 3880000, 3900000, 3920000, 3940000, 3960000,
	3980000, 4000000, 4020000, 4040000, 4060000, 4080000, 4100000, 4120000,
	4140000, 4160000, 4180000, 4200000, 4220000, 4240000, 4260000, 4280000,
	4300000, 4320000, 4340000, 4360000, 4380000, 4400000, 4420000, 4440000
};

#define BQ24158_VBAT_MAP_SIZE		ARRAY_SIZE(bq24158_vbat_map)

static const u32 bq24158_weak_vbat_map[] = {
	3400000, 3500000, 3600000, 3700000
};

#define BQ24158_WEAK_VBAT_MAP_SIZE		ARRAY_SIZE(bq24158_weak_vbat_map) 

static const u32 bq24158_iilimit_map[] = {
	100000, 500000, 800000
};

#define BQ24158_IILIMIT_MAP_SIZE	ARRAY_SIZE(bq24158_iilimit_map)

static int bq24158_enable_charging(struct charger_device *chg_dev, bool en);
static void enable_boost_polling(struct bq24158_device *bq, bool poll_en);

static int bq24158_field_read(struct bq24158_device *bq,
			      enum bq24158_fields field_id)
{
	int ret;
	int val;

	ret = regmap_field_read(bq->rmap_fields[field_id], &val);
	if (ret < 0)
		return ret;

	return val;
}

static int bq24158_field_write(struct bq24158_device *bq,
			       enum bq24158_fields field_id, u8 val)
{
	return regmap_field_force_write(bq->rmap_fields[field_id], val);
}

static u8 bq24158_find_idx(u32 value, const u32 *map, u8 map_size)
{
	u8 idx;

	for (idx = 1; idx < map_size; idx++)
		if (value < map[idx])
			break;

	return idx - 1;
}

enum bq24158_status {
	STATUS_READY,
	STATUS_CHARGE_IN_PROGRESS,
	STATUS_CHARGE_DONE,
	STATUS_FAULT,
};

enum bq24158_fault {
	FAULT_NORMAL,
	FAULT_INPUT_OVP,
	FAULT_SLEEP,
	FAULT_INPUT_UVLO,
	FAULT_OUTPUT_OVP,
	FAULT_TS,
	FAULT_TIMER,
	FAULT_NO_BAT,
};

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0))
static int bq24158_get_input_current_limit(struct bq24158_device *bq,
					   union power_supply_propval *val)
{
	int ret;

	ret = bq24158_field_read(bq, F_LIN_LIMIT);
	if (ret < 0)
		return ret;

	/*
	 * The "External ILIM" and "Production & Test" modes are not exposed
	 * through this driver and not being covered by the lookup table.
	 * Should such a mode have become active let's return an error rather
	 * than exceeding the bounds of the lookup table and returning
	 * garbage.
	 */
	if (ret >= BQ24158_IILIMIT_MAP_SIZE)
		return -ENODATA;

	val->intval = bq24158_iilimit_map[ret];

	return 0;
}
#endif

static int bq24158_set_charge_current(struct bq24158_device *bq, int mA)
{
	int val, ret;
	int ichg_msb, ichg_lsb, ichg_offset = 550;

	dev_err(bq->dev, "%s charge_current=%d\n", __func__, mA);
	if (bq->is_eta) {
		if (mA < ichg_offset)
			ret = bq24158_field_write(bq, F_LOW_CHG, 1);
		else {
			ret = bq24158_field_write(bq, F_LOW_CHG, 0);
			if (mA > 3050)
				mA = 3050;
			ret = (mA - ichg_offset) / 100;
			ichg_msb = (ret >> 3) & 0x3;
			ichg_lsb = ret & 0x7;
			ret = bq24158_field_write(bq, F_ICHRG2, ichg_msb);
			ret =bq24158_field_write(bq, F_ICHRG, ichg_lsb);
			dev_err(bq->dev, "%s ichg_msb=0x%x, ichg_lsb=0x%x\n", __func__, ichg_msb, ichg_lsb);
		}
	} else {
		if (bq->init_data.resistor_sense <= 0)
			return -EINVAL;

		if (mA <= (bq->init_data.low_chg_current / 1000)) {
			ret = bq24158_field_write(bq, F_LOW_CHG, 1);
		} else {
			ret = bq24158_field_write(bq, F_LOW_CHG, 0);
			val = (mA * bq->init_data.resistor_sense - 37400) / 6800;
			if (val < 0)
				val = 0;
			else if (val > 7)
				val = 7;

			ret = bq24158_field_write(bq, F_ICHRG, val);
		}
	}
	return ret;
}

/* get charge current in mA (platform data must provide resistor sense) */
static int bq24158_get_charge_current(struct bq24158_device *bq)
{
	int ret;
	int ichg_msb, ichg_lsb, ichg_offset;

	if (bq->is_eta) {
		ret = bq24158_field_read(bq, ICHRG_OFFSET);
		if (ret < 0)
			return ret;
		if (ret)
			ichg_offset = 650;
		else
			ichg_offset = 550;
		ret = bq24158_field_read(bq, F_ICHRG);
		if (ret < 0)
			return ret;
		ichg_lsb = ret;
		ret = bq24158_field_read(bq, F_ICHRG2);
		if (ret < 0)
			return ret;
		ichg_msb = ret;
		ret = (ichg_msb << 3 | ichg_lsb);
		dev_err(bq->dev, "%s charge_current=%d\n", __func__, (ret * 100 + ichg_offset));
		return (ret * 100 + ichg_offset);
	} else {
		if (bq->init_data.resistor_sense <= 0)
			return -EINVAL;

		ret = bq24158_field_read(bq, F_ICHRG);
		if (ret < 0)
			return ret;
		return (37400 + 6800 * ret) / bq->init_data.resistor_sense;
	}
}


/* set termination current in mA (platform data must provide resistor sense) */
static int bq24158_set_termination_current(struct bq24158_device *bq, int mA)
{
	int val;
	if (bq->is_eta) {
		val = mA / 50;
	} else {
		if (bq->init_data.resistor_sense <= 0)
			return -EINVAL;

		val = (mA * bq->init_data.resistor_sense - 3400) / 3400;
	}

	if (val < 0)
		val = 0;
	else if (val > 7)
		val = 7;
	return bq24158_field_write(bq, F_ITERM, val);
}

/* get termination current in mA (platform data must provide resistor sense) */
static int bq24158_get_termination_current(struct bq24158_device *bq)
{
	int ret;

	ret = bq24158_field_read(bq, F_ITERM);
	if (ret < 0)
		return ret;
	if (bq->is_eta) {
		return (ret + 1) * 50;
	} else {
		if (bq->init_data.resistor_sense <= 0)
			return -EINVAL;
		return (3400 + 3400 * ret) / bq->init_data.resistor_sense;
	}
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0))
static int bq24158_get_charger_type(struct bq24158_device *bq)
{
	union power_supply_propval prop;
	static struct power_supply *chg_psy;

	if (chg_psy == NULL) {
		if (bq->bc12_sel == 1)
			chg_psy = power_supply_get_by_name("mtk_charger_type");
		else if (bq->bc12_sel == 2)
			chg_psy = power_supply_get_by_name("ext_charger_type");
	}

	if (IS_ERR_OR_NULL(chg_psy))
		pr_notice("%s Couldn't get chg_psy\n", __func__);
	else {
		power_supply_get_property(chg_psy,
				POWER_SUPPLY_PROP_USB_TYPE, &prop);
		pr_notice("%s type:%d\n", __func__, prop.intval);

		switch (prop.intval) {
		case POWER_SUPPLY_USB_TYPE_UNKNOWN:
			bq->charger_desc.type = POWER_SUPPLY_TYPE_UNKNOWN;
			break;
		case POWER_SUPPLY_USB_TYPE_SDP:
			bq->charger_desc.type = POWER_SUPPLY_TYPE_USB;
			break;
		case POWER_SUPPLY_USB_TYPE_CDP:
			bq->charger_desc.type = POWER_SUPPLY_TYPE_USB_CDP;
			break;
		case POWER_SUPPLY_USB_TYPE_DCP:
			bq->charger_desc.type = POWER_SUPPLY_TYPE_USB_DCP;
			break;
		}
		/*power_supply_changed(bq->charger);*/
	}
	return prop.intval;
}
#endif

static int bq24158_get_chip_state(struct bq24158_device *bq,
				  struct bq24158_state *state)
{
	int ret;

	ret = bq24158_field_read(bq, F_STAT);
	if (ret < 0)
		return ret;

	state->status = ret;

	ret = bq24158_field_read(bq, F_FAULT);
	if (ret < 0)
		return ret;

	state->fault = ret;

	/*
	 * If we have a chip without a dedicated power-good GPIO or
	 * some other explicit bit that would provide this information
	 * assume the power is good if there is no supply related
	 * fault - and not good otherwise. There is a possibility for
	 * other errors to mask that power in fact is not good but this
	 * is probably the best we can do here.
	 */
	switch (state->fault) {
	case FAULT_INPUT_OVP:
	case FAULT_INPUT_UVLO:
		state->power_good = false;
		break;
	default:
		state->power_good = true;
	}

	return 0;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0))
static int bq24158_power_supply_get_property(struct power_supply *psy,
					     enum power_supply_property psp,
					     union power_supply_propval *val)
{
	struct bq24158_device *bq = power_supply_get_drvdata(psy);
	struct bq24158_state state;
	bq24158_get_chip_state(bq, &state);
	dev_dbg(bq->dev, "status/fault = %d/%d\n", state.status, state.fault);

	mutex_lock(&bq->lock);
	bq->state = state;
	mutex_unlock(&bq->lock);
	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = state.power_good;
		break;
	case POWER_SUPPLY_PROP_STATUS:
		if (!state.power_good)
			val->intval = POWER_SUPPLY_STATUS_DISCHARGING;
		else if (state.status == STATUS_READY)
			val->intval = POWER_SUPPLY_STATUS_NOT_CHARGING;
		else if (state.status == STATUS_CHARGE_IN_PROGRESS)
			val->intval = POWER_SUPPLY_STATUS_CHARGING;
		else if (state.status == STATUS_CHARGE_DONE)
			val->intval = POWER_SUPPLY_STATUS_FULL;
		else
			val->intval = POWER_SUPPLY_STATUS_UNKNOWN;
		break;

	case POWER_SUPPLY_PROP_MANUFACTURER:
		val->strval = BQ24158_MANUFACTURER;
		break;

	case POWER_SUPPLY_PROP_MODEL_NAME:
		val->strval = bq->model;
		break;

	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT:
		val->intval = bq->init_data.ichg;
		break;

	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT_MAX:
		val->intval = bq->init_data.ichg;
		break;

	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE:
		val->intval = bq24158_vbat_map[bq->init_data.vbat_idx];
		break;

	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE_MAX:
		val->intval = bq24158_vbat_map[BQ24158_VBAT_MAP_SIZE - 1];
		break;

	case POWER_SUPPLY_PROP_CHARGE_TERM_CURRENT:
		val->intval = bq->init_data.iterm;
		break;

	case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT:
		return bq24158_get_input_current_limit(bq, val);
		
	case POWER_SUPPLY_PROP_USB_TYPE:
		val->intval = bq24158_get_charger_type(bq);
		break;

	default:
		return -EINVAL;
	}

	return 0;
}
#endif

static bool __maybe_unused bq24158_state_changed(struct bq24158_device *bq,
				  struct bq24158_state *new_state)
{
	int ret;

	mutex_lock(&bq->lock);
	ret = (bq->state.status != new_state->status ||
	       bq->state.fault != new_state->fault ||
	       bq->state.power_good != new_state->power_good);
	mutex_unlock(&bq->lock);

	return ret;
}

enum bq24158_loop_status {
	LOOP_STATUS_NONE,
	LOOP_STATUS_IN_DPM,
	LOOP_STATUS_IN_CURRENT_LIMIT,
	LOOP_STATUS_THERMAL,
};

enum bq24158_in_ilimit {
	IILIMIT_100,
	IILIMIT_500,
	IILIMIT_800,
	IILIMIT_NONE,
};


enum bq24158_port_type {
	PORT_TYPE_DCP,		/* Dedicated Charging Port */
	PORT_TYPE_CDP,		/* Charging Downstream Port */
	PORT_TYPE_SDP,		/* Standard Downstream Port */
	PORT_TYPE_NON_STANDARD,
};

/* detect chip type */
static enum bq24158_chip bq24158_detect_chip(struct bq24158_device *bq)
{
	struct i2c_client *client = to_i2c_client(bq->dev);
	int ret = bq24158_field_read(bq, F_PN);

	if (ret < 0)
		return ret;

	switch (client->addr) {
	case 0x6a:
		switch (ret) {
		case 2:
			if (bq->chip == BQ24157S)
				return bq->chip;
			ret = bq24158_field_read(bq, F_REVISION);
			if (ret == 4) {
				ret = bq24158_field_read(bq, F_EN_ILIM2);
				if (ret < 0)
					bq->is_eta = false;
				else
					bq->is_eta = true;
			}
			if (bq->is_eta)
				return ETA6937;
			else
				return BQ24158;
		default:
			return BQUNKNOWN;
		}
		break;
	}

	return BQUNKNOWN;
}

/* detect chip revision */
static int bq24158_detect_revision(struct bq24158_device *bq)
{
	int ret = bq24158_field_read(bq, F_REVISION);
	int chip = bq24158_detect_chip(bq);

	if (ret < 0 || chip < 0)
		return -1;

	switch (chip) {
	case BQ24158:
		if (ret == 3)
			return 0;
		else if (ret == 1)
			return 1;
		return -1;
	case ETA6937:
		if (ret == 4)
			return 0;
	case BQUNKNOWN:
		return -1;
	}

	return -1;
}

/* return chip vender code */
static int bq24158_get_vender_code(struct bq24158_device *bq)
{
	int ret;

	ret = bq24158_field_read(bq, F_VENDER);
	if (ret < 0)
		return 0;

	/* convert to binary */
	return (ret & 0x1) +
	       ((ret >> 1) & 0x1) * 10 +
	       ((ret >> 2) & 0x1) * 100;
}

static int bq24158_dump_registers(struct bq24158_device *bq)
{
	unsigned int reg = BQ24158_REG_1, val;
	int ret;
	int BQ24158_REG_MAX = BQ24158_REG_7;

	if (bq->is_eta)
		BQ24158_REG_MAX = BQ24158_REG_8;

	for (; reg <= BQ24158_REG_MAX; reg++) {
		ret = regmap_read(bq->rmap, reg, &val);
		dev_err_ratelimited(bq->dev,"0x%.2x=%.2x\n", reg, val);
	}

	return ret;
}

static irqreturn_t __maybe_unused bq24158_irq_handler_thread(int irq, void *private)
{
	struct bq24158_device *bq = private;
	int ret;
	struct bq24158_state state;

	if (IS_ERR_OR_NULL(bq))
		goto handled;
	ret = bq24158_get_chip_state(bq, &state);
	if (ret < 0)
		goto handled;

	if (!bq24158_state_changed(bq, &state))
		goto handled;

	mutex_lock(&bq->lock);
	bq->state = state;
	bq24158_dump_registers(bq);
	mutex_unlock(&bq->lock);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0))
	power_supply_changed(bq->charger);
#endif

handled:
	return IRQ_HANDLED;
}

static int bq24158_pinctrl_init(struct bq24158_device *bq)
{
	int ret = 0;
	bq->pinctrl.pinctrl = devm_pinctrl_get(bq->dev);
	if (IS_ERR_OR_NULL(bq->pinctrl.pinctrl)) {
		ret = PTR_ERR(bq->pinctrl.pinctrl);
		goto err_pinctrl_get;
	}
	bq->pinctrl.active = 
		pinctrl_lookup_state(bq->pinctrl.pinctrl,
							"bq24158_active");
	if (IS_ERR_OR_NULL(bq->pinctrl.active)) {
		ret = PTR_ERR(bq->pinctrl.active);
		dev_err(bq->dev,"failed to get pinctrl active state, ret=%d\n", ret);
		goto err_pinctrl_lookup;
	}
	bq->pinctrl.suspend = 
		pinctrl_lookup_state(bq->pinctrl.pinctrl, "bq24158_suspend");
	if (IS_ERR_OR_NULL(bq->pinctrl.suspend)) {
		ret = PTR_ERR(bq->pinctrl.suspend);
		dev_err(bq->dev,"failed to get pinctrl suspend state, ret=%d\n", ret);
		goto err_pinctrl_lookup;
	}
	return 0;
err_pinctrl_lookup:
	devm_pinctrl_put(bq->pinctrl.pinctrl);
err_pinctrl_get:
	bq->pinctrl.pinctrl = NULL;
	return ret;
}

static void bq24158_pinctrl_deinit(struct bq24158_device *bq)
{
	devm_pinctrl_put(bq->pinctrl.pinctrl);
}

static int bq24158_set_pinctrl_state(struct bq24158_device *bq, bool enable)
{
	int ret;
	struct pinctrl_state *state;

	if (!bq->pinctrl.pinctrl)
		return 0;
	if (enable)
		state = bq->pinctrl.active;
	else
		state = bq->pinctrl.suspend;

	ret = pinctrl_select_state(bq->pinctrl.pinctrl, state);
	if (ret)
		dev_err(bq->dev, "failed to set pin state, ret=%d\n", ret);
	bq->cd_en = enable;
	return ret;
}

/* set default values of all properties */
static int bq24158_hw_init(struct bq24158_device *bq)
{
	int i;
	int ret;

	const struct {
		int field;
		u32 value;
	} init_data[] = {
		{F_TMR_RST_OTG, 1},
		{F_EN_STAT, 1},
		{F_OPA_MODE, 0},
		{F_CE, 1},
		{F_TE, 0},
		{F_HZ_MODE,0},
		{F_OTG_EN, 0},
		{F_OTG_PL, 0},
		{ICHRG_OFFSET, 0},
		{F_LOWV, bq->init_data.weak_vbat_idx},
		{F_VBAT, bq->init_data.vbat_idx},
		{F_LIN_LIMIT, bq->init_data.iilimit_idx},
		{F_VSREG, 0x4},
	};
	ret = bq24158_pinctrl_init(bq);
	if (ret < 0) {
		dev_err(bq->dev, "Couldn't initialize pinctrl ret=%d\n", ret);
		return ret;
	}
	bq24158_set_pinctrl_state(bq, true);
	/* configure the charge weak voltage and voltages */
	for (i = 0; i < ARRAY_SIZE(init_data); i++) {
		ret = bq24158_field_write(bq, init_data[i].field,
					  init_data[i].value);
		if (ret < 0)
			return ret;
	}

	bq24158_set_charge_current(bq, bq->init_data.ichg / 1000);
	if (bq->is_eta)
		bq24158_set_termination_current(bq, bq->init_data.iterm / 1000);
	else
		bq24158_set_termination_current(bq, 200);
	bq24158_field_write(bq, F_TE, 1); //enable termination

	bq24158_field_write(bq, F_CE, 0);
	return 0;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0))
static enum power_supply_property bq24158_power_supply_props[] = {
	POWER_SUPPLY_PROP_MANUFACTURER,
	POWER_SUPPLY_PROP_MODEL_NAME,
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_ONLINE,
	/*POWER_SUPPLY_PROP_HEALTH,*/
	POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT,
	POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT_MAX,
	POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE,
	POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE_MAX,
	POWER_SUPPLY_PROP_CHARGE_TERM_CURRENT,
	POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT,
	POWER_SUPPLY_PROP_TYPE,
	POWER_SUPPLY_PROP_USB_TYPE,
};

static char *bq24158_charger_supplied_to[] = {
	"battery",
	"mtk-master-charger"
};

static enum power_supply_usb_type bq24158_charger_usb_types[] = {
	POWER_SUPPLY_USB_TYPE_UNKNOWN,
	POWER_SUPPLY_USB_TYPE_SDP,
	POWER_SUPPLY_USB_TYPE_DCP,
	POWER_SUPPLY_USB_TYPE_CDP,
	POWER_SUPPLY_USB_TYPE_FLOAT,
};

static int bq24158_power_supply_init(struct bq24158_device *bq)
{
	int ret;
	int chip;
	char revstr[8];
	struct power_supply_config psy_cfg = { .drv_data = bq, };

	psy_cfg.supplied_to = bq24158_charger_supplied_to;
	psy_cfg.num_supplicants = ARRAY_SIZE(bq24158_charger_supplied_to);

	bq->charger_desc.name = bq->chg_name;
	bq->charger_desc.type = POWER_SUPPLY_TYPE_USB;
	bq->charger_desc.properties = bq24158_power_supply_props;
	bq->charger_desc.num_properties =
			ARRAY_SIZE(bq24158_power_supply_props);
	bq->charger_desc.get_property = bq24158_power_supply_get_property;
	bq->charger_desc.usb_types = bq24158_charger_usb_types;
	bq->charger_desc.num_usb_types = ARRAY_SIZE(bq24158_charger_usb_types);

	ret = bq24158_detect_chip(bq);
	if (ret < 0)
		chip = BQUNKNOWN;
	else
		chip = ret;

	ret = bq24158_detect_revision(bq);
	if (ret < 0)
		strcpy(revstr, "unknown");
	else
		sprintf(revstr, "1.%d", ret);

	bq->model = devm_kasprintf(bq->dev, GFP_KERNEL,
				"chip %s, revision %s, vender code %.3d",
				bq24158_chip_name[chip], revstr,
				bq24158_get_vender_code(bq));
	if (!bq->model) {
		dev_err(bq->dev, "failed to allocate model name\n");
		return -ENOMEM;
	}

	bq->charger = devm_power_supply_register(bq->dev,
						 &bq->charger_desc,
						 &psy_cfg);

	return PTR_ERR_OR_ZERO(bq->charger);
}
#endif

static int bq24158_parse_dt(struct bq24158_device *bq)
{
	int ret;
	u32 property;

	/* Required properties */
	ret = device_property_read_u32(bq->dev, "ichg", &bq->init_data.ichg);
	if (ret < 0)
		bq->init_data.ichg = 650000;
	ret = device_property_read_u32(bq->dev, "cv", &property);
	if (ret < 0)
		property = 4200000;

	bq->init_data.vbat = property;
	bq->init_data.vbat_idx = bq24158_find_idx(property, bq24158_vbat_map,
					      BQ24158_VBAT_MAP_SIZE);

	ret = device_property_read_u32(bq->dev, "weak_vbat", &property);
	if (ret < 0)
		property = 3400000;

	bq->init_data.weak_vbat = property;
	bq->init_data.weak_vbat_idx = bq24158_find_idx(property, bq24158_weak_vbat_map,
					      BQ24158_WEAK_VBAT_MAP_SIZE);

	ret = device_property_read_u32(bq->dev, "resistor-sense",
						&bq->init_data.resistor_sense);
	if (ret < 0)
		bq->init_data.resistor_sense = 68;
	bq->init_data.low_chg_current = (22100 / bq->init_data.resistor_sense) * 1000;

	ret = device_property_read_u32(bq->dev, "ieoc",
				       &bq->init_data.iterm);
	if (ret < 0)
		bq->init_data.iterm = 100000;

	/* Optional properties. If not provided use reasonable default. */
	ret = device_property_read_u32(bq->dev, "aicr",
				       &property);
	if (ret < 0) {
		/*
		 * Explicitly set a default value which will be needed for
		 * devices that don't support the automatic setting of the input
		 * current limit through the charger type detection mechanism.
		 */
		bq->init_data.iilimit = bq24158_iilimit_map[IILIMIT_500];
		bq->init_data.iilimit_idx = IILIMIT_500;
	} else {
		bq->init_data.iilimit = property;
		if (property > bq24158_iilimit_map[BQ24158_IILIMIT_MAP_SIZE-1])
			bq->init_data.iilimit_idx = 0x3;
		else 
			bq->init_data.iilimit_idx =
					bq24158_find_idx(property,
							 bq24158_iilimit_map,
							 BQ24158_IILIMIT_MAP_SIZE);
	}

	if (device_property_read_u32(bq->dev, "bc12_sel", &bq->bc12_sel) < 0) {
		dev_notice(bq->dev, "%s: no bc12_sel, using the default set(bc12_sel=1)\n", __func__);
		bq->bc12_sel = 1;
	}

	if (device_property_read_string(bq->dev, "chg_name", &bq->chg_name) < 0) {
		dev_notice(bq->dev, "not specified chg_name,using the default chg_name\n");
		bq->chg_name = "primary_chg";
	}
	/* Begin added by bitao.xiong for GF6CAN-2870 on 2022-03-24 */
	bq->is_gflip6_series = device_property_read_bool(bq->dev, "enable_gflip6_series");
	/* End added by bitao.xiong for GF6CAN-2870 on 2022-03-24 */
	return 0;
}

static int bq24158_dump_register(struct charger_device *chg_dev)
{
	struct bq24158_device *bq = charger_get_data(chg_dev);

	if (IS_ERR_OR_NULL(bq))
		return -EINVAL;
	bq24158_dump_registers(bq);
	return 0;
}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0))
static int bq24158_do_event(struct charger_device *chg_dev, u32 event,
				   u32 args)
{
	struct bq24158_device *bq = charger_get_data(chg_dev);

	dev_info(bq->dev, "%s\n", __func__);

	if (!bq->charger) {
		dev_notice(bq->dev, "%s: cannot get psy\n", __func__);
		return -ENODEV;
	}

	switch (event) {
	case EVENT_FULL:
	case EVENT_RECHARGE:
	case EVENT_DISCHARGE:
		power_supply_changed(bq->charger);
		break;
	default:
		break;
	}

	return 0;
}
#else
static int bq24158_do_event(struct charger_device *chg_dev, unsigned int event,
			   unsigned int args)
{
	if (chg_dev == NULL)
		return -EINVAL;

	pr_info("%s: event = %d\n", __func__, event);

	switch (event) {
	case EVENT_EOC:
		charger_dev_notify(chg_dev, CHARGER_DEV_NOTIFY_EOC);
		break;
	case EVENT_RECHARGE:
		charger_dev_notify(chg_dev, CHARGER_DEV_NOTIFY_RECHG);
		break;
	default:
		break;
	}

	return 0;
}
#endif

static int bq24158_kick_wdt(struct charger_device *chg_dev)
{
	struct bq24158_device *bq = charger_get_data(chg_dev);

	dev_err(bq->dev, "%s\n", __func__);
	return bq24158_field_write(bq, F_TMR_RST_OTG, 0x1);
}

static int bq24158_is_charging_done(struct charger_device *chg_dev,
					   bool *done)
{
	struct bq24158_device *bq = charger_get_data(chg_dev);
	struct bq24158_state chg_stat;
	int ret = 0;

	dev_dbg(bq->dev, "%s\n", __func__);

	ret = bq24158_get_chip_state(bq, &chg_stat);
	if (ret < 0)
		return ret;
	mutex_lock(&bq->lock);
	bq->state = chg_stat;
	mutex_unlock(&bq->lock);

	*done = (chg_stat.status == STATUS_CHARGE_DONE) ? true : false;

	return 0;
}

static int bq24158_enable_charging(struct charger_device *chg_dev, bool en)
{
	struct bq24158_device *bq = charger_get_data(chg_dev);
	int ret = 0;

	dev_dbg(bq->dev, "%s: en=%d\n", __func__, en);

	if (en) {
		ret += bq24158_field_write(bq, F_CE, 0);
		ret += bq24158_field_write(bq, F_HZ_MODE, 0);
		ret += bq24158_field_write(bq, F_OPA_MODE, 0);
	} else {
		ret += bq24158_field_write(bq, F_CE, 1);
	}

	return ret;
}

static int bq24158_charger_is_enabled(struct charger_device *chg_dev, bool *en)
{
	struct bq24158_device *bq = charger_get_data(chg_dev);
	int ret;

	ret = bq24158_field_read(bq, F_CE);
	if (ret < 0) {
		return ret;
	}

	*en = ret;
	dev_dbg(bq->dev, "%s: en=%d\n", __func__, *en);
	return 0;
}

static int bq24158_set_ichg(struct charger_device *chg_dev, u32 uA)
{
	struct bq24158_device *bq = charger_get_data(chg_dev);
	int ret = 0;
	int mA = uA / 1000;

	/* Begin modified by bitao.xiong for GF6CAN-2870 on 2022-03-24 */
	if (bq->is_gflip6_series && bq->is_eta && bq->is_kpoc)
		bq->target_ichg = mA;
	else {
		if (uA < bq->init_data.low_chg_current)
			return bq24158_field_write(bq, F_LOW_CHG, 1);

		bq24158_field_write(bq, F_LOW_CHG, 0);
		ret = bq24158_set_charge_current(bq, mA);
	}
	/* End modified by bitao.xiong for GF6CAN-2870 on 2022-03-24 */
	dev_dbg(bq->dev, "%s: ichg=%duA, ret=%d\n", __func__, uA, ret);
	return ret;
}

static int bq24158_get_min_ichg(struct charger_device *chg_dev, u32 *uA)
{
	struct bq24158_device *bq = charger_get_data(chg_dev);

	*uA = bq->init_data.low_chg_current;
	return 0;
}

static int bq24158_get_ichg(struct charger_device *chg_dev, u32 *uA)
{
	struct bq24158_device *bq = charger_get_data(chg_dev);
	int ret = 0;

	ret = bq24158_field_read(bq, F_LOW_CHG);
	if (ret >= 1)
		return bq24158_get_min_ichg(chg_dev, uA);
	if (!ret) {
		ret = bq24158_get_charge_current(bq);
		if (ret < 0)
			return ret;
		*uA = ret * 1000;
		dev_dbg(bq->dev, "%s: ichg=%duA, ret=%d\n", __func__, *uA, ret);
		return 0;
	}

	dev_dbg(bq->dev, "%s: ichg=%duA, ret=%d\n", __func__, *uA, ret);
	return 0;
}

static int bq24158_set_cv(struct charger_device *chg_dev, u32 uV)
{
	struct bq24158_device *bq = charger_get_data(chg_dev);
	u8 data = 0;
	if (bq->is_eta)
		uV += 20 * 1000;
	data = bq24158_find_idx(uV, bq24158_vbat_map,
					      BQ24158_VBAT_MAP_SIZE);
	bq->init_data.vbat_idx = data;
	bq->init_data.vbat = bq24158_vbat_map[data];
	dev_dbg(bq->dev, "%s: cv = %d, cv(real)=%d\n", __func__, uV,
				bq->init_data.vbat);
	
	return bq24158_field_write(bq, F_VBAT, data);
}

static int bq24158_get_cv(struct charger_device *chg_dev, u32 *uV)
{
	struct bq24158_device *bq = charger_get_data(chg_dev);
	*uV = bq->init_data.vbat; 
	return 0;
}


static int bq24158_set_aicr(struct charger_device *chg_dev, u32 uA)
{
	struct bq24158_device *bq = charger_get_data(chg_dev);
	u8 data = 0;

	if (uA > bq24158_iilimit_map[BQ24158_IILIMIT_MAP_SIZE - 1]) {
		bq->init_data.iilimit = 1050000;
		data = 0x3;
		bq->init_data.iilimit_idx = 0x3;
		dev_dbg(bq->dev, "%s: aicr=%d, aicr(real) not limit\n", __func__,
				uA);
		return bq24158_field_write(bq, F_LIN_LIMIT, data);
	} else {
		data = bq24158_find_idx(uA, bq24158_iilimit_map,
						      BQ24158_IILIMIT_MAP_SIZE);
		bq->init_data.iilimit_idx = data;
		bq->init_data.iilimit = bq24158_iilimit_map[data];
		dev_dbg(bq->dev, "%s: aicr=%d, aicr(real)=%d\n", __func__,
			uA, bq->init_data.iilimit);
		return bq24158_field_write(bq, F_LIN_LIMIT, data);
	}
}

static int bq24158_get_aicr(struct charger_device *chg_dev, u32 *uA)
{
	struct bq24158_device *bq = charger_get_data(chg_dev);

	*uA = bq->init_data.iilimit;
	return 0;
}

static int bq24158_get_min_aicr(struct charger_device *chg_dev, u32 *uA)
{
	*uA = 100000;
	return 0;
}

static int bq24158_set_ieoc(struct charger_device *chg_dev, u32 uA)
{
	struct bq24158_device *bq = charger_get_data(chg_dev);
	dev_dbg(bq->dev, "%s: ieoc = %d\n", __func__, uA);
	bq->init_data.iterm = uA;
	return bq24158_set_termination_current(bq, uA / 1000);
}

static int bq24158_get_ieoc(struct charger_device *chg_dev, u32 *uA)
{
	struct bq24158_device *bq = charger_get_data(chg_dev);

	int ret;
	ret = bq24158_get_termination_current(bq);
	if (ret <0)
		return ret;
	*uA = ret * 1000;
	bq->init_data.iterm = ret;
	dev_dbg(bq->dev, "%s: ieoc = %d\n", __func__, *uA);
	return 0;
}

static int bq24158_enable_te(struct charger_device *chg_dev, bool en)
{
	struct bq24158_device *bq = charger_get_data(chg_dev);
	
	dev_dbg(bq->dev, "%s: en = %d\n", __func__, en);
	
	return bq24158_field_write(bq, F_TE, en);
}


static int bq24158_plug_in(struct charger_device *chg_dev)
{
	struct bq24158_device *bq = charger_get_data(chg_dev);
	unsigned int boot_mode = 0;
	int ret = 0;

	dev_dbg(bq->dev, "%s\n", __func__);

	ret = bq24158_hw_init(bq);

	if (ret < 0) {
		dev_err(bq->dev, "%s: call bq24158_hw_init fail\n", __func__);
		return ret;
	}

	/* Begin added by bitao.xiong for GF6CAN-2870 on 2022-03-24 */
	if (bq->is_gflip6_series && bq->is_eta) {
		boot_mode = get_boot_mode();
		bq->is_kpoc = (boot_mode == KERNEL_POWER_OFF_CHARGING_BOOT
			|| boot_mode == LOW_POWER_OFF_CHARGING_BOOT);
		if (bq->is_kpoc) {
			enable_boost_polling(bq, true);
			get_monotonic_boottime(&bq->charging_begin_time);
			bq->target_ichg = 550;
			bq->current_ichg = 0;
		}
	}
	/* End added by bitao.xiong for GF6CAN-2870 on 2022-03-24 */
	return ret;
}

static int bq24158_plug_out(struct charger_device *chg_dev)
{
	struct bq24158_device *bq = charger_get_data(chg_dev);
	int ret = 0;

	dev_dbg(bq->dev, "%s\n", __func__);

	ret = bq24158_enable_te(chg_dev, false);
	if (ret < 0)
		dev_err(bq->dev, "%s: disable te failed\n", __func__);

	/* Begin added by bitao.xiong for GF6CAN-2870 on 2022-03-24 */
	if (bq->is_gflip6_series && bq->is_eta && bq->is_kpoc) {
		enable_boost_polling(bq, false);
		bq->target_ichg = 550;
		bq->current_ichg = 0;
	}
	/* End added by bitao.xiong for GF6CAN-2870 on 2022-03-24 */
	return ret;
}

static int bq24158_get_mivr_state(struct charger_device *chg_dev, bool *in_loop)
{
	struct bq24158_device *bq = charger_get_data(chg_dev);
	int ret = 0;

	ret = bq24158_field_read(bq, F_DPM_STATUS);
	if (ret < 0) {
		return ret;
	}
	*in_loop = ret;
	dev_dbg(bq->dev, "%s: in_loop=%d\n", __func__, ret);
	return 0;
}

static void enable_boost_polling(struct bq24158_device *bq, bool poll_en)
{
	struct timespec time, time_now, end_time;
	ktime_t ktime;

	if (bq) {
		if (poll_en) {
			get_monotonic_boottime(&time_now);
			time.tv_sec = bq->polling_interval;
			time.tv_nsec = 0;
			timespec_add(time_now, time);
			ktime = ktime_set(end_time.tv_sec, end_time.tv_nsec);
			alarm_start(&bq->otg_kthread_gtimer, ktime);
			bq->polling_enabled = true;
		} else {
			bq->polling_enabled = false;
			alarm_cancel(&bq->otg_kthread_gtimer);
		}
	}
}

static void usbotg_boost_kick_work(struct work_struct *work)
{
	ktime_t ktime;
	struct timespec time, time_now, end_time, diff_time;
	struct bq24158_device *boost_manager =
		container_of(work, struct bq24158_device, kick_work);
	int vbus = 0;
	int ret;

	pr_debug_ratelimited("bq24158 otg detect\n");

	bq24158_field_write(boost_manager, F_TMR_RST_OTG, 0x1);
	/* Begin added by bitao.xiong for GF6CAN-2870 on 2022-03-24 */
	if (boost_manager->is_gflip6_series && boost_manager->is_eta && boost_manager->is_kpoc) {
		ret = bq24158_field_read(boost_manager, F_BOOST);
		if (!ret) {
			vbus = battery_get_vbus();
			if (vbus >= 0 && vbus < 2500 && !mt_charger_plugin()) {
				boost_manager->target_ichg = 550;
			} else {
				get_monotonic_boottime(&time_now);
				diff_time = timespec_sub(time_now, boost_manager->charging_begin_time);
				dev_err(boost_manager->dev, "%s target_ichg=%d, current_ichg=%d, diff_time=%ds\n",
						__func__, boost_manager->target_ichg, boost_manager->current_ichg, diff_time.tv_sec);
				if (boost_manager->current_ichg != boost_manager->target_ichg) {
					if (diff_time.tv_sec < 60) {
						boost_manager->current_ichg = 550;
						bq24158_set_charge_current(boost_manager, boost_manager->current_ichg);
					} else {
						boost_manager->current_ichg = boost_manager->target_ichg;
						bq24158_field_write(boost_manager, F_EN_ILIM2, 0); /* Input Limit is set by REG01<7:6> */
						bq24158_set_charge_current(boost_manager, boost_manager->target_ichg);
					}
				}
			}
		}
	}
	/* End added by bitao.xiong for GF6CAN-2870 on 2022-03-24 */
	if (boost_manager->polling_enabled == true) {
		get_monotonic_boottime(&time_now);
		time.tv_sec = boost_manager->polling_interval;
		time.tv_nsec = 0;
		end_time = timespec_add(time_now, time);
		ktime = ktime_set(end_time.tv_sec, end_time.tv_nsec);
		alarm_start(&boost_manager->otg_kthread_gtimer, ktime);
	}
}

static enum alarmtimer_restart usbotg_gtimer_func(struct alarm *alarm,
						 ktime_t now)
{
	struct bq24158_device *boost_manager =
		container_of(alarm, struct bq24158_device,
			     otg_kthread_gtimer);

	queue_work(boost_manager->otg_boost_workq,
		   &boost_manager->kick_work);

	return ALARMTIMER_NORESTART;
}

static int bq24158_enable_otg(struct charger_device *chg_dev, bool en)
{
	struct bq24158_device *bq = charger_get_data(chg_dev);
	int ret = 0;

	dev_dbg(bq->dev, "%s: en = %d\n", __func__, en);
	if (en) {
		bq24158_field_write(bq, F_OPA_MODE, 1);
	} else {
		bq24158_field_write(bq, F_OPA_MODE, 0);
	}
	enable_boost_polling(bq, en);
	return ret;
}

static int bq24158_enable_vbus(struct regulator_dev *rdev)
{
	struct bq24158_device *bq = rdev_get_drvdata(rdev);

	int ret;
	ret = bq24158_field_write(bq, F_OPA_MODE, 1);
	return ret;
}

static int bq24158_disable_vbus(struct regulator_dev *rdev)
{
	struct bq24158_device *bq = rdev_get_drvdata(rdev);
	int ret;
	ret = bq24158_field_write(bq, F_OPA_MODE, 0);
	return ret;
}

static int bq24158_is_enabled_vbus(struct regulator_dev *rdev)
{
	struct bq24158_device *bq = rdev_get_drvdata(rdev);
	int ret;
	ret = bq24158_field_read(bq, F_DPM_STATUS);
	if (ret < 0) {
		return ret;
	}

	return ret;
}

static const struct regulator_ops bq24158_vbus_ops = {
	.enable = bq24158_enable_vbus,
	.disable = bq24158_disable_vbus,
	.is_enabled = bq24158_is_enabled_vbus,
};

static const struct regulator_desc bq24158_otg_rdesc = {
	.of_match = "usb-otg-vbus",
	.name = "usb-otg-vbus",
	.ops = &bq24158_vbus_ops,
	.owner = THIS_MODULE,
	.type = REGULATOR_VOLTAGE,
	.fixed_uV = 5000000,
	.n_voltages = 1,
};

static const struct charger_ops bq24158_chg_ops = {
	/* enable */
	.enable = bq24158_enable_charging,
	.is_enabled = bq24158_charger_is_enabled,
	/* charging current */
	.get_charging_current = bq24158_get_ichg,
	.set_charging_current = bq24158_set_ichg,
	.get_min_charging_current = bq24158_get_min_ichg,
	/* charging voltage */
	.set_constant_voltage = bq24158_set_cv,
	.get_constant_voltage = bq24158_get_cv,
	/* charging input current */
	.get_input_current = bq24158_get_aicr,
	.set_input_current = bq24158_set_aicr,
	.get_min_input_current = bq24158_get_min_aicr,
	/* set termination current */
	.get_eoc_current = bq24158_get_ieoc,
	.set_eoc_current = bq24158_set_ieoc,
	/* charing termination */
	.enable_termination = bq24158_enable_te,
	/* OTG */
	.enable_otg = bq24158_enable_otg,
	/* misc */
	.is_charging_done = bq24158_is_charging_done,
	.dump_registers = bq24158_dump_register,
	/* Event */
	.event = bq24158_do_event,
	/* kick wdt */
	.kick_wdt = bq24158_kick_wdt,
	.plug_in = bq24158_plug_in,
	.plug_out = bq24158_plug_out,
	.get_mivr_state = bq24158_get_mivr_state,
};

static const struct charger_properties bq24158_chg_props = {
	.alias_name = "bq24158",
};

#if IS_ENABLED(CONFIG_TCT_DEVICEINFO)
extern char charger_module_name[256];
#endif

static int bq24158_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = client->adapter;
	struct device *dev = &client->dev;
	struct bq24158_device *bq;
	struct regulator_config config = {};
	int ret;
	int i;
	int chip;
	char revstr[8];
	unsigned int boot_mode = 0;

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
		dev_err(dev, "No support for SMBUS_BYTE_DATA\n");
		return -ENODEV;
	}

	bq = devm_kzalloc(dev, sizeof(*bq), GFP_KERNEL);
	if (!bq)
		return -ENOMEM;

	bq->client = client;
	bq->dev = dev;
	bq->chip = (enum bq24158_chip)id->driver_data;
	
	dev_err(dev, "%s: bq->chip=%d\n", __func__, bq->chip);

	mutex_init(&bq->lock);

	bq->rmap = devm_regmap_init_i2c(client, &bq24158_regmap_config);
	if (IS_ERR(bq->rmap)) {
		dev_err(dev, "failed to allocate register map\n");
		return PTR_ERR(bq->rmap);
	}

	for (i = 0; i < ARRAY_SIZE(bq24158_reg_fields); i++) {
		const struct reg_field *reg_fields = bq24158_reg_fields;

		bq->rmap_fields[i] = devm_regmap_field_alloc(dev, bq->rmap,
							     reg_fields[i]);
		if (IS_ERR(bq->rmap_fields[i])) {
			dev_err(dev, "cannot allocate regmap field\n");
			return PTR_ERR(bq->rmap_fields[i]);
		}
	}

	i2c_set_clientdata(client, bq);

	if (!dev->platform_data) {
		ret = bq24158_parse_dt(bq);
		if (ret < 0) {
			dev_err(dev, "Cannot read device properties.\n");
			return ret;
		}
	} else {
		return -ENODEV;
	}

	ret = bq24158_field_read(bq, F_VENDER);
	if (ret < 0) {
		dev_err(dev, "Cannot read chip vendor id.\n");
		return ret;
	}

	ret = bq24158_detect_chip(bq);
	if (ret < 0)
		chip = BQUNKNOWN;
	else
		chip = ret;

	ret = bq24158_detect_revision(bq);
	if (ret < 0)
		strcpy(revstr, "unknown");
	else
		sprintf(revstr, "1.%d", ret);

	dev_info(bq->dev, "charge_info %s, revision %s, vender code %.3d",
						bq24158_chip_name[chip], revstr,
								bq24158_get_vender_code(bq));

#if IS_ENABLED(CONFIG_TCT_DEVICEINFO)
	if (chip != BQUNKNOWN)
		sprintf(charger_module_name, "chip %s, revision %s, vender code %.3d",
				bq24158_chip_name[chip], revstr,
				bq24158_get_vender_code(bq));
#endif

	/* Begin modified by bitao.xiong for GF6CAN-2870 on 2022-03-24 */
	if (bq->is_eta) {
		regmap_write(bq->rmap, BQ24158_REG_7, 0x3A);
		if (bq->is_gflip6_series) {
			boot_mode = get_boot_mode();
			bq->is_kpoc = (boot_mode == KERNEL_POWER_OFF_CHARGING_BOOT
				|| boot_mode == LOW_POWER_OFF_CHARGING_BOOT);
			if (bq->is_kpoc)
				get_monotonic_boottime(&bq->charging_begin_time);
			else
				bq24158_field_write(bq, F_EN_ILIM2, 0); /* Input Limit is set by REG01<7:6> */
		}
	} else {
		regmap_write(bq->rmap, BQ24158_REG_7, 0x7A);
	}
	/* End modified by bitao.xiong for GF6CAN-2870 on 2022-03-24 */
	ret = bq24158_hw_init(bq);
	if (ret < 0) {
		dev_err(dev, "Cannot initialize the chip.\n");
		return ret;
	}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0))
	ret = bq24158_power_supply_init(bq);
	if (ret < 0) {
		dev_err(dev, "Failed to register power supply\n");
		return ret;
	}
#endif

	/* charger class register */
	bq->chg_dev = charger_device_register(bq->chg_name, bq->dev,
						bq, &bq24158_chg_ops,
						&bq24158_chg_props);
	if (IS_ERR_OR_NULL(bq->chg_dev)) {
		dev_err(bq->dev, "charger device register fail\n");
		ret = PTR_ERR(bq->chg_dev);
	}
	alarm_init(&bq->otg_kthread_gtimer, ALARM_BOOTTIME,
		  usbotg_gtimer_func);

	bq->otg_boost_workq =
			create_singlethread_workqueue("otg_boost_workq");
	INIT_WORK(&bq->kick_work, usbotg_boost_kick_work);
	bq->polling_interval = 20;
	/* Begin added by bitao.xiong for GF6CAN-2870 on 2022-03-24 */
	bq->target_ichg = 550;
	bq->current_ichg = 0;
	/* End added by bitao.xiong for GF6CAN-2870 on 2022-03-24 */
	/* STAT irq configuration */
	if (client->irq) {
		ret = devm_request_threaded_irq(dev, client->irq, NULL,
						bq24158_irq_handler_thread,
						IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
						"bq24158_chg_stat_irq", bq);
		if (ret) {
			dev_err(dev, "Failed STAT irq=%d request ret = %d\n",
				client->irq, ret);
			return ret;
		}
		enable_irq_wake(client->irq);
	}

	/* otg regulator */
	config.dev = dev;
	config.driver_data = bq;
	bq->otg_rdev = devm_regulator_register(bq->dev,
					&bq24158_otg_rdesc, &config);
	if (IS_ERR(bq->otg_rdev)) {
		pr_info("%s: register otg regulator failed (%d)\n", __func__, PTR_ERR(bq->otg_rdev));
		return PTR_ERR(bq->otg_rdev);
	}
	return 0;

}

static int bq24158_remove(struct i2c_client *client)
{
	struct bq24158_device *bq = i2c_get_clientdata(client);

	bq24158_field_write(bq, F_RESET, 1); /* reset to defaults */
	devm_free_irq(&client->dev, client->irq, bq);
	bq24158_pinctrl_deinit(bq);
	charger_device_unregister(bq->chg_dev);

	return 0;
}

static void bq24158_shutdown(struct i2c_client *client)
{
	struct bq24158_device *bq = i2c_get_clientdata(client);
	pr_info("%s: shutdown\n", __func__);
	devm_free_irq(&client->dev, client->irq, bq);
	bq24158_pinctrl_deinit(bq);
	charger_device_unregister(bq->chg_dev);
}

#ifdef CONFIG_PM_SLEEP
static int bq24158_suspend(struct device *dev)
{
	return 0;
}

static int bq24158_resume(struct device *dev)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0))
	struct bq24158_device *bq = dev_get_drvdata(dev);
	/* signal userspace, maybe state changed while suspended */
	power_supply_changed(bq->charger);
#endif
	return 0;
}
#endif

static const struct dev_pm_ops bq24158_pm = {
	SET_SYSTEM_SLEEP_PM_OPS(bq24158_suspend, bq24158_resume)
};

static const struct i2c_device_id bq24158_i2c_ids[] = {
	{ "bq2415x", BQUNKNOWN },
	{ "bq24158", BQ24158 },
	{},
};
MODULE_DEVICE_TABLE(i2c, bq24158_i2c_ids);

#ifdef CONFIG_OF
static const struct of_device_id bq24158_of_match[] = {
	{ .compatible = "mediatek,bq24158", },
	{ },
};
MODULE_DEVICE_TABLE(of, bq24158_of_match);
#endif

static struct i2c_driver bq24158_driver = {
	.driver = {
		.name = "bq24158",
#ifdef CONFIG_OF
		.of_match_table = of_match_ptr(bq24158_of_match),
#endif
		.pm = &bq24158_pm,
	},
	.probe = bq24158_probe,
	.remove = bq24158_remove,
	.shutdown = bq24158_shutdown,
	.id_table = bq24158_i2c_ids,
};
module_i2c_driver(bq24158_driver);

MODULE_AUTHOR("Bitao Xiong <bitao.xiong@tcl.com>");
MODULE_VERSION("0.3");
MODULE_DESCRIPTION("bq24158 charger driver, compatible with eta6937");
MODULE_LICENSE("GPL");
