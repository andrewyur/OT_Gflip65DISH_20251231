
#include <mt-plat/perf_tracker.h>
#include "lowmem_hint.h"

static u64 checked_timestamp;
static bool long_trace_check_flag;
static DEFINE_SPINLOCK(check_lock);

static inline bool tracker_do_check(u64 wallclock)
{
	bool do_check = false;
	unsigned long flags;

	/* check interval */
	spin_lock_irqsave(&check_lock, flags);
	if ((s64)(wallclock - checked_timestamp)
			>= (s64)(2 * NSEC_PER_MSEC)) {
		checked_timestamp = wallclock;
		long_trace_check_flag = !long_trace_check_flag;
		do_check = true;
	}
	spin_unlock_irqrestore(&check_lock, flags);

	return do_check;
}

void perf_tracker(u64 wallclock)
{
	long mm_available = -1, mm_free = -1;

	if (!tracker_do_check(wallclock))
		return;

	trigger_lowmem_hint(&mm_available, &mm_free);

	__perf_tracker(wallclock, mm_available, mm_free);
}

#ifdef CONFIG_MTK_PERF_TRACKER
bool hit_long_check(void)
{
	bool do_check = false;
	unsigned long flags;

	spin_lock_irqsave(&check_lock, flags);
	if (long_trace_check_flag)
		do_check = true;
	spin_unlock_irqrestore(&check_lock, flags);
	return do_check;
}
#endif
