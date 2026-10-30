#ifndef LOAD_TRACK_H
#define LOAD_TRACK_H

#include <linux/cpumask.h>

#ifdef CONFIG_MTK_LOAD_TRACKER

#define reg_loading_tracking(p_fn, polling_ms, cpu_mask) \
reg_loading_tracking_sp(p_fn, polling_ms, cpu_mask, __func__)
extern int reg_loading_tracking_sp(void (*fn)(int loading, int mask_loading),
	unsigned long polling_ms, const struct cpumask *cpu_mask, const char *caller);

#define unreg_loading_tracking(p_fn) \
unreg_loading_tracking_sp(p_fn, __func__)
extern int unreg_loading_tracking_sp(void (*fn)(int loading, int mask_loading),
	const char *caller);

#else

#define reg_loading_tracking(p_fn, polling_ms, cpu_mask) \
reg_loading_tracking_sp(p_fn, polling_ms, cpu_mask, __func__)
static inline int reg_loading_tracking_sp(void (*fn)(int loading, int mask_loading),
	unsigned long polling_ms, const struct cpumask *cpu_mask, const char *caller)
{ return -EINVAL; }

#define reg_loading_tracking(p_fn, polling_ms, cpu_mask) \
reg_loading_tracking_sp(p_fn, polling_ms, cpu_mask, __func__)
static inline int unreg_loading_tracking_sp(void (*fn)(int loading, int mask_loading),
	const char *caller)
{ return -EINVAL; }

#endif

#endif

