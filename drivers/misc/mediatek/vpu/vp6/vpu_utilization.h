
#ifndef __VPU_UTILIZATION_H__
#define __VPU_UTILIZATION_H__

#include <linux/hrtimer.h>
#include <linux/spinlock.h>


struct vpu_util {
	struct device *dev;
	struct vpu_core *vpu_core;

	spinlock_t lock;

	bool active;
	struct hrtimer timer;
	ktime_t period_time;
	ktime_t period_start;
	unsigned long prev_total;
};
int vpu_utilization_compute_enter(struct vpu_core *vpu_core);
int vpu_utilization_compute_leave(struct vpu_core *vpu_core);
int vpu_dvfs_get_usage(const struct vpu_core *vpu_core,
			unsigned long *total_time, unsigned long *busy_time);
int vpu_init_util(struct vpu_core *vpu_core);
int vpu_deinit_util(struct vpu_core *vpu_core);

#endif /* __VPU_UTILIZATION_H__ */
