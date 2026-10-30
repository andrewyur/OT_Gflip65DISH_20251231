
#ifdef CONFIG_MTK_SCHED_BOOST
/* For multi-scheduling boost support */
enum {
	SCHED_NO_BOOST = 0,
	SCHED_ALL_BOOST,
	SCHED_FG_BOOST,
	SCHED_UNKNOWN_BOOST
};

#define SCHED_PREFER_NONE   0
#define SCHED_PREFER_BIG    1
#define SCHED_PREFER_LITTLE 2
#define SCHED_PREFER_END    3

extern void set_user_space_global_cpuset
		(struct cpumask *global_cpus, int cgroup_id);
extern void unset_user_space_global_cpuset(int cgroup_id);
extern int sched_scheduler_switch(enum SCHED_LB_TYPE new_sched);
#endif
#ifdef CONFIG_SCHED_TUNE
extern int schedtune_task_boost(struct task_struct *tsk);
#else
#define schedtune_task_boost(tsk) 0
#endif

extern unsigned int hmp_cpu_is_slowest(int cpu);
extern unsigned int hmp_cpu_is_fastest(int cpu);
extern bool is_intra_domain(int prev, int target);
extern unsigned long capacity_spare_without(int cpu, struct task_struct *p);
#ifdef CONFIG_MACH_MT6873
extern unsigned long cpu_util_without(int cpu, struct task_struct *p);
extern unsigned long task_util_est(struct task_struct *p);
#endif

