
#ifndef _L3CC_COMMON_H_
#define _L3CC_COMMON_H_
#ifdef CONFIG_MTK_CACHE_CONTROL
extern int ca_force_stop_set_in_kernel(int val);
extern void hook_ca_scheduler_tick(int cpu);
#else
static inline void hook_ca_scheduler_tick(int cpu) {}
#endif
#ifdef CONFIG_MTK_CACHE_PARTITION_CTRL
extern void hook_ca_context_switch(struct rq *rq,
		struct task_struct *prev,
	    struct task_struct *next);

#else
inline void hook_ca_context_switch(struct rq *rq,
		struct task_struct *prev,
	    struct task_struct *next) {}
#endif
#endif
