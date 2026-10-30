#ifndef _PERF_TRACKER_H
#define _PERF_TRACKER_H


extern void perf_tracker(u64 wallclock);
extern int perf_tracker_enable(int val);

#ifdef CONFIG_MTK_PERF_TRACKER
extern void __perf_tracker(u64 wallclock,
			 long mm_available,
			 long mm_free);
extern bool hit_long_check(void);
#ifdef CONFIG_MTK_CPU_FREQ
extern unsigned int mt_cpufreq_get_cur_freq(int id);
#else
static inline int mt_cpufreq_get_cur_freq(int id) { return 0; }
#endif
#else
static inline void __perf_tracker(u64 wallclock,
				long mm_available,
				long mm_free) {}
#endif /* CONFIG_MTK_PERF_TRACKER */
#endif /* _PERF_TRACKER_H */
