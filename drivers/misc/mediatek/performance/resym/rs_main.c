
#include <linux/module.h>

#include "rs_base.h"
#include "rs_trace.h"
#include "rs_usage.h"
#include "rs_state.h"

#define RS_SYSFS_DIR_NAME "resym"

struct kobject *rs_kobj;

static int __init rs_init(void)
{
	rs_kobj = kobject_create_and_add(RS_SYSFS_DIR_NAME, kernel_kobj);

	rs_trace_init();
	rs_usage_init();
	rs_state_init();

	return 0;
}

static void __exit rs_exit(void)
{
	rs_usage_exit();
	rs_state_exit();

	kobject_put(rs_kobj);
	rs_kobj = NULL;
}

module_init(rs_init);
module_exit(rs_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("MediaTek Resource Symphony");
MODULE_AUTHOR("MediaTek Inc.");

