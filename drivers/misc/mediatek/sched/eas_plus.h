
extern int cpu_eff_tp;
extern unsigned long long big_cpu_eff_tp;
/* Stune group info */
#ifdef CONFIG_SCHED_TUNE
extern int group_boost_read(int group_idx);
extern int group_prefer_idle_read(int group_idx);
#else
static int group_boost_read(int group_idx) { return 0; }
static int group_prefer_idle_read(int group_idx) { return 0; }
#endif

