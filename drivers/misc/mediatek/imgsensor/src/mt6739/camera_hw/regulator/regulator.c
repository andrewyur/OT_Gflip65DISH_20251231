
#include "regulator.h"

static int regulator_status[IMGSENSOR_SENSOR_IDX_MAX_NUM][REGULATOR_TYPE_MAX_NUM] = {{0},{0}};
static void check_for_regulator_get(struct REGULATOR *preg, struct device *pdevice, enum IMGSENSOR_SENSOR_IDX   sensor_idx, int index);
static void check_for_regulator_put(struct REGULATOR *preg, enum IMGSENSOR_SENSOR_IDX   sensor_idx, int index);
static struct device_node *of_node_record = NULL;
static DEFINE_MUTEX(g_regulator_state_mutex);

static const int regulator_voltage[] = {
	REGULATOR_VOLTAGE_0,
	REGULATOR_VOLTAGE_1000,
	REGULATOR_VOLTAGE_1100,
	REGULATOR_VOLTAGE_1200,
	REGULATOR_VOLTAGE_1210,
	REGULATOR_VOLTAGE_1220,
	REGULATOR_VOLTAGE_1500,
	REGULATOR_VOLTAGE_1800,
	REGULATOR_VOLTAGE_2500,
	REGULATOR_VOLTAGE_2800,
	REGULATOR_VOLTAGE_2900,
};

struct REGULATOR_CTRL regulator_control[REGULATOR_TYPE_MAX_NUM] = {
	{"vcama"},
	{"vcamd"},
	{"vcamio"},
};

static struct REGULATOR reg_instance;


static enum IMGSENSOR_RETURN regulator_init(void *pinstance)
{
	struct REGULATOR *preg = (struct REGULATOR *)pinstance;
	struct device            *pdevice;
	struct device_node       *pof_node;
	int j, i;
	char str_regulator_name[LENGTH_FOR_SNPRINTF];

	pdevice  = gimgsensor_device;
	pof_node = pdevice->of_node;
	pdevice->of_node = of_find_compatible_node(NULL, NULL, "mediatek,camera_hw");
	of_node_record = pdevice->of_node;

	if (pdevice->of_node == NULL) {
		pr_err("regulator get cust camera node failed!\n");
		pdevice->of_node = pof_node;
		return IMGSENSOR_RETURN_ERROR;
	}

	for (j = IMGSENSOR_SENSOR_IDX_MIN_NUM;
		j < IMGSENSOR_SENSOR_IDX_MAX_NUM;
		j++) {
		for (i = 0; i < REGULATOR_TYPE_MAX_NUM; i++) {
			snprintf(str_regulator_name,
				sizeof(str_regulator_name),
				"cam%d_%s",
				j,
				regulator_control[i].pregulator_type);
			preg->pregulator[j][i] =
			    regulator_get(pdevice, str_regulator_name);

			if (preg->pregulator[j][i] == NULL)
				pr_err("regulator[%d][%d]  %s fail!\n",
					j, i, str_regulator_name);

			atomic_set(&preg->enable_cnt[j][i], 0);
			regulator_status[j][i] = 0;
		}
	}
	pdevice->of_node = pof_node;

	return IMGSENSOR_RETURN_SUCCESS;
}
static enum IMGSENSOR_RETURN regulator_release(void *pinstance)
{
	struct REGULATOR *preg = (struct REGULATOR *)pinstance;
	int type, idx;
	struct regulator *pregulator = NULL;
	atomic_t *enable_cnt = NULL;

	for (idx = IMGSENSOR_SENSOR_IDX_MIN_NUM;
		idx < IMGSENSOR_SENSOR_IDX_MAX_NUM;
		idx++) {

		for (type = 0; type < REGULATOR_TYPE_MAX_NUM; type++) {
			pregulator = preg->pregulator[idx][type];
			enable_cnt = &preg->enable_cnt[idx][type];
			if (pregulator != NULL) {
				for (; atomic_read(enable_cnt) > 0; ) {
					regulator_disable(pregulator);
					atomic_dec(enable_cnt);
				}
			}
		}
	}
	return IMGSENSOR_RETURN_SUCCESS;
}

static enum IMGSENSOR_RETURN regulator_set(
	void *pinstance,
	enum IMGSENSOR_SENSOR_IDX   sensor_idx,
	enum IMGSENSOR_HW_PIN       pin,
	enum IMGSENSOR_HW_PIN_STATE pin_state)
{
	struct regulator     *pregulator;
	struct REGULATOR     *preg = (struct REGULATOR *)pinstance;
	int reg_type_offset;
	atomic_t             *enable_cnt;


	if (pin > IMGSENSOR_HW_PIN_DOVDD   ||
	    pin < IMGSENSOR_HW_PIN_AVDD    ||
	    pin_state < IMGSENSOR_HW_PIN_STATE_LEVEL_0 ||
	    pin_state >= IMGSENSOR_HW_PIN_STATE_LEVEL_HIGH ||
	    sensor_idx < 0)
		return IMGSENSOR_RETURN_ERROR;

	reg_type_offset = REGULATOR_TYPE_VCAMA;

	check_for_regulator_get(preg, gimgsensor_device, sensor_idx, (reg_type_offset + pin - IMGSENSOR_HW_PIN_AVDD));

	pregulator = preg->pregulator[sensor_idx][reg_type_offset + pin - IMGSENSOR_HW_PIN_AVDD];

	enable_cnt =
		&preg->enable_cnt[sensor_idx][
			reg_type_offset + pin - IMGSENSOR_HW_PIN_AVDD];

	if (pregulator) {
		if (pin_state != IMGSENSOR_HW_PIN_STATE_LEVEL_0) {
			if (regulator_set_voltage(pregulator,
						regulator_voltage[pin_state - IMGSENSOR_HW_PIN_STATE_LEVEL_0],
						regulator_voltage[pin_state - IMGSENSOR_HW_PIN_STATE_LEVEL_0])) {

				pr_err("[regulator]fail to regulator_set_voltage, powertype:%d powerId:%d\n",
							pin,
							regulator_voltage[pin_state - IMGSENSOR_HW_PIN_STATE_LEVEL_0]);
			}
			if (regulator_enable(pregulator)) {
				pr_err("[regulator]fail to regulator_enable, powertype:%d powerId:%d\n",
							pin,
							regulator_voltage[pin_state - IMGSENSOR_HW_PIN_STATE_LEVEL_0]);
				return IMGSENSOR_RETURN_ERROR;
			}
			atomic_inc(enable_cnt);
		} else {
			if (regulator_is_enabled(pregulator))
				pr_debug("[regulator]%d is enabled\n", pin);

			if (regulator_disable(pregulator)) {
				pr_err("[regulator]fail to regulator_disable, powertype: %d\n",
				       pin);
				return IMGSENSOR_RETURN_ERROR;
			}

            check_for_regulator_put(preg, sensor_idx, (reg_type_offset + pin - IMGSENSOR_HW_PIN_AVDD));

			atomic_dec(enable_cnt);
		}
	} else {
		pr_err("regulator == NULL %d %d %d\n",
								reg_type_offset, pin, IMGSENSOR_HW_PIN_AVDD);
	}

	return IMGSENSOR_RETURN_SUCCESS;
}

static void check_for_regulator_get(struct REGULATOR *preg, struct device *pdevice, enum IMGSENSOR_SENSOR_IDX   sensor_idx,int index)
{
    struct device_node *pof_node;
    mutex_lock(&g_regulator_state_mutex);
    if(regulator_status[sensor_idx][index]==0)
    {
        pof_node = pdevice->of_node;
        pdevice->of_node = of_node_record;

        preg->pregulator[sensor_idx][index] = regulator_get(pdevice, regulator_control[index].pregulator_type);

        pdevice->of_node = pof_node;
        regulator_status[sensor_idx][index] = 1;
        pr_err("regulator_dbg regulator_get %s, of_node:%p\n", regulator_control[index].pregulator_type, of_node_record);
    }
    mutex_unlock(&g_regulator_state_mutex);
}

static void check_for_regulator_put(struct REGULATOR *preg, enum IMGSENSOR_SENSOR_IDX   sensor_idx, int index)
{
    mutex_lock(&g_regulator_state_mutex);

    if(regulator_status[sensor_idx][index]==1)
    {
        regulator_put(preg->pregulator[sensor_idx][index]);
        regulator_status[sensor_idx][index]=0;
        pr_err("regulator_dbg regulator_put %s\n", regulator_control[index].pregulator_type);
    }
    mutex_unlock(&g_regulator_state_mutex);
}

static struct IMGSENSOR_HW_DEVICE device = {
	.pinstance = (void *)&reg_instance,
	.init      = regulator_init,
	.set       = regulator_set,
	.release   = regulator_release,
	.id        = IMGSENSOR_HW_ID_REGULATOR
};

enum IMGSENSOR_RETURN imgsensor_hw_regulator_open(
	struct IMGSENSOR_HW_DEVICE **pdevice)
{
	*pdevice = &device;
	return IMGSENSOR_RETURN_SUCCESS;
}

