#include <linux/mm.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/mm_inline.h>
#include <linux/ctype.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/gfp.h>
#include <linux/time.h>
#include <linux/timer.h>
#include <linux/sysctl.h>
#include <linux/input.h>
#include <linux/sysfs.h>
#include <linux/slab.h>

#define TOUCH_INTERVAL_MSEC (3000)

#define ABORT_BY_TOUCHING (1 << 0)
#define ABORT_BY_APP_STARTING (1 << 1)
#define ABORT_BY_MM_LOCK (1 << 2)

static int abort_reclaim = 0;
static atomic_t input_touching = ATOMIC_INIT(0);
static struct delayed_work input_event_wk;
static u64 last_input_time = 0;

int should_cancel_reclaim(struct mm_struct *mm){
	int ret = 0;

	/*got touch event*/
	if(atomic_read(&input_touching))
		ret |= ABORT_BY_TOUCHING;

	/* abort reclaim in case app starting*/
	if(abort_reclaim)
		ret |= ABORT_BY_APP_STARTING;

	if(mm && !list_empty(&mm->mmap_sem.wait_list)){
		ret |= ABORT_BY_MM_LOCK;
	}

	return ret;
}
EXPORT_SYMBOL(should_cancel_reclaim);

static void input_event_check(struct work_struct *work)
{
	atomic_set(&input_touching,0);
}

static void user_action_detect_input_event(struct input_handle *handle,
		unsigned int type, unsigned int code, int value)
{
	u64 now;

	now = ktime_to_us(ktime_get());
	if (now - last_input_time < (TOUCH_INTERVAL_MSEC*MSEC_PER_SEC))
		return;

	atomic_set(&input_touching,1);

	mod_delayed_work(system_wq, &input_event_wk,
					msecs_to_jiffies(TOUCH_INTERVAL_MSEC));

	last_input_time = ktime_to_us(ktime_get());
}

static int user_action_detect_input_connect(struct input_handler *handler,
		struct input_dev *dev, const struct input_device_id *id)
{
	struct input_handle *handle;
	int error;

	handle = kzalloc(sizeof(struct input_handle), GFP_KERNEL);
	if (!handle)
		return -ENOMEM;

	handle->dev = dev;
	handle->handler = handler;
	handle->name = "userdetector";

	error = input_register_handle(handle);
	if (error)
		goto err2;

	error = input_open_device(handle);
	if (error)
		goto err1;

	return 0;
err1:
	input_unregister_handle(handle);
err2:
	kfree(handle);
	return error;
}

static void user_action_detect_input_disconnect(struct input_handle *handle)
{
	input_close_device(handle);
	input_unregister_handle(handle);
	kfree(handle);
}

static const struct input_device_id user_action_detect_ids[] = {
	/* multi-touch touchscreen */
	{
		.flags = INPUT_DEVICE_ID_MATCH_EVBIT |
			INPUT_DEVICE_ID_MATCH_ABSBIT,
		.evbit = { BIT_MASK(EV_ABS) },
		.absbit = { [BIT_WORD(ABS_MT_POSITION_X)] =
			BIT_MASK(ABS_MT_POSITION_X) |
			BIT_MASK(ABS_MT_POSITION_Y) },
	},
	/* touchpad */
	{
		.flags = INPUT_DEVICE_ID_MATCH_KEYBIT |
			INPUT_DEVICE_ID_MATCH_ABSBIT,
		.keybit = { [BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH) },
		.absbit = { [BIT_WORD(ABS_X)] =
			BIT_MASK(ABS_X) | BIT_MASK(ABS_Y) },
	},
	/* Keypad */
	{
		.flags = INPUT_DEVICE_ID_MATCH_EVBIT,
		.evbit = { BIT_MASK(EV_KEY) },
	},
	{ },
};

static struct input_handler user_action_detect_input_handler = {
	.event          = user_action_detect_input_event,
	.connect        = user_action_detect_input_connect,
	.disconnect     = user_action_detect_input_disconnect,
	.name           = "userdetector",
	.id_table       = user_action_detect_ids,
};


static struct ctl_table user_action_detect_kern_table[] = {
  	{
  		.procname	= "abort_reclaim",
  		.data		= &abort_reclaim,
  		.maxlen		= sizeof(abort_reclaim),
  		.mode		= 0666,
  		.proc_handler	= proc_dointvec,
  	},
	{}
};

static struct ctl_table root_table[] = {
  	{
  		.procname	= "vm",
  		.mode		= 0555,
  		.child		= user_action_detect_kern_table,
  	},
  	{}
};

static int __init user_action_detect_init(void)
{
	register_sysctl_table(root_table);
	INIT_DELAYED_WORK(&input_event_wk, input_event_check);
	input_register_handler(&user_action_detect_input_handler);
	return 0;
}

static void __exit user_action_detect_exit(void)
{
	return;
}


module_init(user_action_detect_init);
module_exit(user_action_detect_exit);
MODULE_LICENSE("GPL");
