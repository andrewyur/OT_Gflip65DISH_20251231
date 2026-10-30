#include <linux/device.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/module.h>

static struct class *deviceinfo_class = NULL;
static struct device *deviceinfo_dev = NULL;
static DEFINE_MUTEX(devicelock);

struct device* get_deviceinfo_dev(void)
{
    mutex_lock(&devicelock);

    if(!deviceinfo_dev) {
        if(!deviceinfo_class) {
            deviceinfo_class = class_create(THIS_MODULE, "deviceinfo");
            if (IS_ERR_OR_NULL(deviceinfo_class)) {
                pr_err("Failed to create class deviceinfo!\n");
                goto exit_unlock_mutex;
            }
        }

        deviceinfo_dev = device_create(deviceinfo_class, NULL, 0, NULL, "device_info");
        if (IS_ERR_OR_NULL(deviceinfo_dev)) {
            pr_err("Failed to create device device_info!\n");
            goto exit_destroy_class;
        }
    }

    mutex_unlock(&devicelock);
    return deviceinfo_dev;

exit_destroy_class:
    class_destroy(deviceinfo_class);
exit_unlock_mutex:
    mutex_unlock(&devicelock);
    return NULL;
}
EXPORT_SYMBOL_GPL(get_deviceinfo_dev);

typedef struct devlist{
    const char * name;
    char * str;
    struct devlist * next;
}dev_list;

dev_list devinfo_start;
dev_list * devinfo_current = &devinfo_start;

#define DEVICEINFO_TAG          "DEVICEINFO"
#define DEVICEINFO_LOG_ERR(fmt, args...) \
    pr_err("%s %s: "fmt, DEVICEINFO_TAG, __func__, ##args)
#define DEVICEINFO_FUN          pr_err(DEVICEINFO_TAG "%s\n", __func__)

#define DEVICEINFO_FUNC(devinfo) \
char devinfo##_module_name[256] = "NA:NA:NA:NA"; \
EXPORT_SYMBOL(devinfo##_module_name); \
dev_list devinfo##_list; \
static ssize_t devinfo##_show(struct device *dev, \
    struct device_attribute *attr, char *buf) { \
    if (!dev) { \
        DEVICEINFO_LOG_ERR("dev is NULL\n"); \
        return 0; \
    } \
    DEVICEINFO_LOG_ERR("%s \n", devinfo##_module_name); \
    return snprintf(buf, PAGE_SIZE, "%s\n", devinfo##_module_name); \
} \
static ssize_t devinfo##_store(struct device* dev, \
    struct device_attribute *attr, const char *buf, size_t count) { \
    snprintf(devinfo##_module_name, count, "%s", buf);\
    return count; \
} \
DEVICE_ATTR(devinfo, S_IWUSR | S_IRUGO, devinfo##_show, devinfo##_store); \
void Create_##devinfo##_node_ForMMI(void) { \
    struct device * devinfo; \
    devinfo = get_deviceinfo_dev(); \
    if (!devinfo || device_create_file(devinfo, &dev_attr_##devinfo) < 0) \
        DEVICEINFO_LOG_ERR("Failed to create device file(%s)!\n", dev_attr_##devinfo.attr.name); \
    else { \
        devinfo##_list.name = dev_attr_##devinfo.attr.name; \
        devinfo##_list.str = devinfo##_module_name; \
        devinfo_current->next = &devinfo##_list; \
        devinfo_current = &devinfo##_list; \
    } \
    return; \
}
/* Common attr for show all device info */
static ssize_t tct_all_deviceinfo_show(struct device *dev,
    struct device_attribute *attr, char *buf) {
    dev_list * p = &devinfo_start;
    int count = 0;

    if (!dev) {
        printk("dev is NULL\n");
        return 0;
    }

    while(p->next)
    {
        p = p->next;
        count += snprintf(&buf[count], PAGE_SIZE, "%s:%s;", p->name, p->str);
    }

    count += snprintf(&buf[count], PAGE_SIZE, "\n");

    return count;
}

static ssize_t tct_all_deviceinfo_store(struct device* dev,
    struct device_attribute *attr, const char *buf, size_t count) {
    return count;
}

DEVICE_ATTR(tct_all_deviceinfo, S_IWUSR | S_IRUGO, tct_all_deviceinfo_show, tct_all_deviceinfo_store);

void Create_tct_all_deviceinfo_node_ForMMI(void) {
    struct device * deviceinfo;

    deviceinfo = get_deviceinfo_dev();

    if (device_create_file(deviceinfo, &dev_attr_tct_all_deviceinfo) < 0)
        pr_err("Failed to create device file(%s)!\n", dev_attr_tct_all_deviceinfo.attr.name);

    return;
}


/*  */
DEVICEINFO_FUNC(CamNameB)
DEVICEINFO_FUNC(CamNameB2)
DEVICEINFO_FUNC(CamNameF)
DEVICEINFO_FUNC(LCM)
DEVICEINFO_FUNC(SUB_LCM)
DEVICEINFO_FUNC(keypad_led)
DEVICEINFO_FUNC(ctp)
DEVICEINFO_FUNC(eMMC)
DEVICEINFO_FUNC(gsensor)
DEVICEINFO_FUNC(psensor)
DEVICEINFO_FUNC(lsensor)
DEVICEINFO_FUNC(gyroscope)
DEVICEINFO_FUNC(compass)
DEVICEINFO_FUNC(NFC)
DEVICEINFO_FUNC(battery_info)
DEVICEINFO_FUNC(speaker1)
DEVICEINFO_FUNC(speaker2)
DEVICEINFO_FUNC(receiver1)
DEVICEINFO_FUNC(receiver2)
DEVICEINFO_FUNC(FM)
DEVICEINFO_FUNC(hall1)
DEVICEINFO_FUNC(hall2)
DEVICEINFO_FUNC(bluetooth)
DEVICEINFO_FUNC(wifi)
DEVICEINFO_FUNC(gps)
DEVICEINFO_FUNC(DTV)
DEVICEINFO_FUNC(ATV)
DEVICEINFO_FUNC(CPU)
DEVICEINFO_FUNC(fp)
DEVICEINFO_FUNC(CamNameB3)
DEVICEINFO_FUNC(CamNameB4)
DEVICEINFO_FUNC(DDR)
/*  */

DEVICEINFO_FUNC(charger)

DEVICEINFO_FUNC(CamOTPF)
DEVICEINFO_FUNC(CamOTPB)
DEVICEINFO_FUNC(CamOTPB2)
DEVICEINFO_FUNC(CamOTPB3)
DEVICEINFO_FUNC(CamOTPB4)

static int __init deviceinfo_init(void) {
    DEVICEINFO_FUN;

    Create_CamNameB_node_ForMMI();
    Create_CamNameB2_node_ForMMI();
    Create_CamNameF_node_ForMMI();
    Create_LCM_node_ForMMI();
    Create_SUB_LCM_node_ForMMI();
    Create_keypad_led_node_ForMMI();
    Create_ctp_node_ForMMI();
    Create_eMMC_node_ForMMI();
    Create_gsensor_node_ForMMI();
    Create_psensor_node_ForMMI();
    Create_lsensor_node_ForMMI();
    Create_gyroscope_node_ForMMI();
    Create_compass_node_ForMMI();
    Create_NFC_node_ForMMI();
    Create_battery_info_node_ForMMI();
    Create_speaker1_node_ForMMI();
    Create_speaker2_node_ForMMI();
    Create_receiver1_node_ForMMI();
    Create_receiver2_node_ForMMI();
    Create_FM_node_ForMMI();
    Create_hall1_node_ForMMI();
    Create_hall2_node_ForMMI();
    Create_bluetooth_node_ForMMI();
    Create_wifi_node_ForMMI();
    Create_gps_node_ForMMI();
    Create_DTV_node_ForMMI();
    Create_ATV_node_ForMMI();
    Create_CPU_node_ForMMI();
    Create_fp_node_ForMMI();
    Create_CamNameB3_node_ForMMI();
    Create_CamNameB4_node_ForMMI();
    Create_DDR_node_ForMMI();

    Create_charger_node_ForMMI();

    Create_CamOTPF_node_ForMMI();
    Create_CamOTPB_node_ForMMI();
    Create_CamOTPB2_node_ForMMI();
    Create_CamOTPB3_node_ForMMI();
    Create_CamOTPB4_node_ForMMI();

    Create_tct_all_deviceinfo_node_ForMMI();

// Begin modified by liujie for defect 11356862
    sprintf(wifi_module_name, "MT6631:MediaTek:Default:AMB0000088C1");
    sprintf(gps_module_name, "MT6631:MediaTek:Default:AMB0000088C1");
    sprintf(bluetooth_module_name, "MT6631:MediaTek:Default:AMB0000088C1");
    sprintf(FM_module_name, "MT6631:MediaTek:Default:AMB0000088C1");
// End modified by liujie for defect 11356862
    return 0;
}
/*----------------------------------------------------------------------------*/

static void __exit deviceinfo_exit(void) {
    DEVICEINFO_FUN;
}
/*----------------------------------------------------------------------------*/
module_init(deviceinfo_init);
module_exit(deviceinfo_exit);
/*----------------------------------------------------------------------------*/
MODULE_AUTHOR("hailong.chen");
MODULE_DESCRIPTION("deviceinfo driver");
MODULE_LICENSE("GPL");
