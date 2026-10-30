#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/semaphore.h>
#include <linux/rwsem.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/sysfs.h>
#include <linux/kobject.h>
#include <linux/cpu.h>
#include <linux/jiffies.h>
#include <linux/mm.h>
#include <linux/swap.h>
#include <linux/vmstat.h>
#include <linux/kernel_stat.h>
#include <linux/tick.h>
#include <linux/version.h>
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 10, 0))
#include <linux/cputime.h>
#endif

#include "dmcc.h"
extern should_cancel_reclaim(struct mm_struct *mm);

/* Globals */

static struct dmcc_module dmcc_module;

static inline unsigned long elapsed_jiffies(unsigned long start)
{
	unsigned long end = jiffies;

	if (end >= start)
		return (unsigned long)(end - start);

	return (unsigned long)(end + (MAX_JIFFY_OFFSET - start) + 1);
}

static bool is_swap_full(int max_percent)
{
	struct sysinfo si;

	si_swapinfo(&si);
	if (si.totalswap == 0)
		return true;
	/* free/total > max_percent% */
	if ((si.totalswap - si.freeswap) * 100 >= si.totalswap * max_percent)
		return true;
	return false;
}

static bool is_memory_free_enough(int free_pages_min)
{
	unsigned long nr_free_pages;

#if (KERNEL_VERSION(4, 14, 0) <= LINUX_VERSION_CODE)
	nr_free_pages = global_zone_page_state(NR_FREE_PAGES);
#else
	nr_free_pages = global_page_state(NR_FREE_PAGES);
#endif
	if (nr_free_pages > free_pages_min)
		return true;
	return false;
}

static bool is_anon_page_enough(int anon_pages_min)
{
	unsigned long nr_pages;

#if (KERNEL_VERSION(4, 9, 0) <= LINUX_VERSION_CODE)
	nr_pages = global_node_page_state(NR_INACTIVE_ANON);
	nr_pages += global_node_page_state(NR_ACTIVE_ANON);
#else
	nr_pages = global_page_state(NR_INACTIVE_ANON);
	nr_pages += global_page_state(NR_ACTIVE_ANON);
#endif

	if (nr_pages > anon_pages_min)
		return true;
	return false;
}

static bool is_avail_above_target(int avail_target)
{
	long available;

	available = si_mem_available();
	if (available > avail_target)
		return true;
	return false;
}

static void dmcc_set_full_clean(struct dmcc_module *dmcc, int set)
{
	dmcc->full_clean_flag = !!set;
	pr_info("dmcc: full_clean_flag = %d\n", dmcc->full_clean_flag);
}

static inline bool is_display_off(struct dmcc_module *dmcc)
{
	return dmcc->display_off;
}

static void dmcc_update_display_stat(struct dmcc_module *dmcc, bool display_on)
{
	dmcc->display_off = !display_on;
	pr_info("dmcc: display_off = %d\n", dmcc->display_off);
}

/**
 * purpose: check cpu is really in idle status
 * arguments:
 *    dmcc: struct dmcc_module. we use dmcc_idle_* values here.
 * return:
 *    none.
 */
static inline bool is_cpu_idle(struct dmcc_module *dmcc)
{
	int threshold = dmcc->idle_threshold;

	/* when display off, we try do more work. */
	if (dmcc->display_off)
		threshold += 5;

	if (dmcc->force_compress_flag)
		threshold = DMCC_MAX_CPULOAD;

	if (dmcc->cpu_load[0] <= threshold &&
	    dmcc->cpu_load[1] <= threshold &&
	    dmcc->cpu_load[2] <= threshold) {
		return true;
	}
	return false;
}

#ifdef arch_idle_time

static u64 get_idle_time(int cpu)
{
	u64 idle;

	idle = kcpustat_cpu(cpu).cpustat[CPUTIME_IDLE];
	if (cpu_online(cpu) && !nr_iowait_cpu(cpu))
		idle += arch_idle_time(cpu);
	return idle;
}

#else

static u64 get_idle_time(int cpu)
{
	u64 idle, idle_time = -1ULL;

	if (cpu_online(cpu))
		idle_time = get_cpu_idle_time_us(cpu, NULL);

	if (idle_time == -1ULL)
		/* !NO_HZ or cpu offline so we can rely on cpustat.idle */
		idle = kcpustat_cpu(cpu).cpustat[CPUTIME_IDLE];
	else
#if (KERNEL_VERSION(4, 14, 0) <= LINUX_VERSION_CODE)
		idle = idle_time * NSEC_PER_USEC;
#else
		idle = usecs_to_cputime64(idle_time);
#endif
	return idle;
}

#endif

/**
 *  purpose: get average cpuload from last stat
 * return:
 *    cpu load in percent.
 */
static int _get_cpu_load(clock_t *last_cpu_stat)
{
	int i;
	int cpuload;
	clock_t delta[INDEX_CPU_MAX], tmp = 0;

	u64 user, nice, system, idle;
	u64 total_time, busy_time;

	user = 0;
	nice = 0;
	system = 0;
	idle = 0;
	cpuload = 0;

	for_each_possible_cpu(i) {
		user += kcpustat_cpu(i).cpustat[CPUTIME_USER];
		nice += kcpustat_cpu(i).cpustat[CPUTIME_NICE];
		system += kcpustat_cpu(i).cpustat[CPUTIME_SYSTEM];
		idle += get_idle_time(i);
	}

	total_time = user + nice + system + idle;
	busy_time = user + nice + system;

#if (KERNEL_VERSION(4, 14, 0) <= LINUX_VERSION_CODE)
	tmp = nsec_to_clock_t(busy_time);
#else
	tmp = cputime64_to_clock_t(busy_time);
#endif
	delta[INDEX_CPU_BUSY] = tmp - last_cpu_stat[INDEX_CPU_BUSY];
	last_cpu_stat[INDEX_CPU_BUSY] = tmp;

#if (KERNEL_VERSION(4, 14, 0) <= LINUX_VERSION_CODE)
	tmp = nsec_to_clock_t(total_time);
#else
	tmp = cputime64_to_clock_t(total_time);
#endif
	delta[INDEX_CPU_TOTAL] = tmp - last_cpu_stat[INDEX_CPU_TOTAL];
	last_cpu_stat[INDEX_CPU_TOTAL] = tmp;

	if (delta[INDEX_CPU_TOTAL] == 0)
		return -1;

	cpuload = DMCC_MAX_CPULOAD * delta[INDEX_CPU_BUSY] / delta[INDEX_CPU_TOTAL];

	if (cpuload > DMCC_MAX_CPULOAD)
		cpuload = DMCC_MAX_CPULOAD;

	if (cpuload < 0)
		cpuload = 0;

	return cpuload;
}

/**
 *  purpose: wrapper of _get_cpu_load()
 * return:
 *    cpu load in percent.
 */
static int get_cpu_load(struct dmcc_module *dmcc, bool need_delay)
{
	int cpu_load;
	/* delay some time for calc recent cpu load. */
	if (need_delay) {
		_get_cpu_load(dmcc->last_cpu_stat);
		msleep(25);
	}
	cpu_load = _get_cpu_load(dmcc->last_cpu_stat);
	if (cpu_load >= 0) {
		dmcc->cpu_load[2] = dmcc->cpu_load[1];
		dmcc->cpu_load[1] = dmcc->cpu_load[0];
		dmcc->cpu_load[0] = cpu_load;
	}
	return cpu_load;
}

/**
 * purpose: swap out memory.
 * return count of reclaimed pages.
 */
static int dmcc_swap_out(int nr_pages, int scan_mode, struct dmcc_module *dmcc)
{
	int unit_pages, total, real = 0;

	for (total = 0; total < nr_pages; total += 32) {
		unit_pages =
		    ((total + 32) > nr_pages) ? (nr_pages - total) : 32;
		real += try_to_free_pages_ex(unit_pages, scan_mode);
		if(dmcc->normal_compress_flag && should_cancel_reclaim(NULL))
			break;
		cond_resched();
	}

#ifdef CONFIG_TCT_DMCC_DEBUG
	pr_info("scan mode: %d. swap:  %d/%d pages\n", scan_mode, real,
		nr_pages);
#endif
	/* return real reclaimed page count */
	return real;
}

/* purpose: check is background thread running */
static inline bool dmcc_is_enabled(struct dmcc_module *dmcc)
{
	return !!dmcc->task;
}

/**
 * purpose: check system is ready to wakeup compress thread.
 *    now we check: enable & idle & memory_full & swap_full.
 * arguments:
 *    dmcc: struct dmcc_module.
 *    end: start condition checker or end condition checker.
 * return:
 *    thread exit code.
 */
static unsigned int get_system_stat(struct dmcc_module *dmcc, bool end)
{
	unsigned int ret = 0;
	int value;

	if (!is_cpu_idle(dmcc))
		ret |= WF_CPU_BUSY;

	value = dmcc->free_pages_min + (end ? DMCC_FREE_PAGE_MIN_EX : 0);
	if (is_memory_free_enough(value))
		ret |= WF_MEM_FREE_ENOUGH;

	if (!dmcc->full_clean_flag) {
		value = end ? dmcc->anon_pages_min : dmcc->anon_pages_max;
	} else {
		value = end ? DMCC_ANON_PAGE_MIN_ON_BOOT :
			DMCC_ANON_PAGE_MAX_ON_BOOT;
	}

	if (!is_anon_page_enough(value))
		ret |= WF_NO_ANON_PAGE;

	value = dmcc->swap_percent_low + (end ? DMCC_SWAP_PERCENT_LOW_EX : 0);
	if (is_swap_full(value))
		ret |= WF_SWAP_FULL;

	value = dmcc->avail_target_pages;
	if (is_avail_above_target(value))
		ret |= WF_AVAIL_ENOUGH;

	return ret ? ret : WS_NEED_WAKEUP;
}

/* purpose: wakeup background thread */
static int dmcc_thread_wakeup(struct dmcc_module *dmcc)
{
	/* up semaphore of background thread. */
	up(&dmcc->wait);
	return 0;
}

/**
 * purpose: waiting wakeup event in background thread.
 * arguments:
 *    dmcc: struct dmcc_module.
 *    timeout: timeout to wait. unit is ms. <0 for infinite wait.
 * return:
 *    -ETIME: timeout to wait.
 *         0: received events
 */
static int dmcc_thread_wait(struct dmcc_module *dmcc, long timeout)
{
	/* down semaphore of background thread. */
	if (timeout > 0)
		return down_timeout(&dmcc->wait, msecs_to_jiffies(timeout));
	down(&dmcc->wait);
	return 0;
}

/**
 * purpose: waiting wakeup event in background thread when dmcc pause.
 * arguments:
 *    dmcc: struct dmcc_module.
 */
static void dmcc_thread_wait_for_pause(struct dmcc_module *dmcc)
{
	unsigned long pause_elapsed_time = 0;
	do {
		dmcc_thread_wait(dmcc, DMCC_PAUSE_TIME
			- jiffies_to_msecs(pause_elapsed_time));
		pause_elapsed_time = elapsed_jiffies(dmcc->pause_time);
	} while (jiffies_to_msecs(pause_elapsed_time) < DMCC_PAUSE_TIME);
}

#define DO_SWAP_OUT(mode,dmcc) do { \
		if (ret & WF_CPU_BUSY) { \
			nr_pages = 0; \
			busy_count++; \
		} else if (ret & WF_NO_ANON_PAGE) { \
			nr_pages = 0; \
			anon_count++; \
		} else { \
			one_loop_time = jiffies; \
			nr_pages = dmcc_swap_out(DMCC_NR_SWAP_UNIT_SIZE, mode,dmcc); \
			one_loop_time = elapsed_jiffies(one_loop_time); \
			one_compress_time += one_loop_time; \
		} \
	} while (0)

#define _UPDATE_STATE() do { \
		nr_total_pages += nr_pages; \
		if (!dmcc->force_compress_flag || dmcc->full_clean_flag) { \
			msleep(DMCC_IDLE_FAST); \
			get_cpu_load(dmcc, false); \
		} \
		if (kthread_should_stop()) \
			goto out;  \
		ret = get_system_stat(dmcc, true); \
	} while (0)

/**
 * purpose: main working thread for compress RAM
 * arguments:
 *    unused: unused.
 * return:
 *    thread exit code.
 */
static int dmcc_thread(void *unused)
{
	unsigned int ret;
	int nr_pages, nr_total_pages, busy_count = 0, anon_count = 0;
	unsigned long one_loop_time;
	unsigned long one_compress_time;
	struct task_struct *tsk = current;
	struct dmcc_module *dmcc = &dmcc_module;
	long timeout;

	/* need swap out, PF_FREEZER_SKIP is protection from hung_task. */
	tsk->flags |= PF_MEMALLOC | PF_SWAPWRITE | PF_KSWAPD | PF_FREEZER_SKIP;

	while (true) {
		timeout = DMCC_IDLE_SLOW;
		if (dmcc->passive_mode)
			timeout = DMCC_WAIT_INFINITE;
		/* wait wakeup_event or timeout. */
		if (!dmcc->force_compress_flag)
			dmcc_thread_wait(dmcc, timeout);
		if (kthread_should_stop())
			goto out;
		/* force update cpu load stat. */
		get_cpu_load(dmcc, true);

		nr_total_pages = 0;
		one_compress_time = 0;
		ret = get_system_stat(dmcc, false);
		if (dmcc->pause_flag) {
			pr_info("dmcc pause\n");
			dmcc_thread_wait_for_pause(dmcc);
			dmcc->pause_flag = 0;
			get_cpu_load(dmcc, true);
			ret = get_system_stat(dmcc, true);
			pr_info("dmcc continue\n");
		}
		pr_info("dmcc try to run ,stat %u\n", ret);
		if (dmcc->full_clean_flag) {
			ssleep(DMCC_SLEEP_TIME);
			dmcc->wakeup_count++;
			pr_info("dmcc wakeup: full.\n");

			/* full fill swap area. */
			busy_count = 0;
			anon_count = 0;
			do {
				DO_SWAP_OUT(DMCC_MODE_ANON,dmcc);
				_UPDATE_STATE();
			} while (!(ret & WF_SWAP_FULL) &&
				 !(anon_count > DMCC_MAX_WAIT_COUNT) &&
				 nr_total_pages <
				 dmcc->full_clean_anon_pages);

			dmcc_set_full_clean(dmcc, 0);
			dmcc->nr_full_clean_pages += nr_total_pages;
			dmcc->total_spent_times
			 += one_compress_time;
			pr_info("full cc: mem=%d MB, time=%d ms, out stat=%u, anon_count = %d\n",
				M(nr_total_pages),
				jiffies_to_msecs(one_compress_time),
				ret, anon_count);
		} else if (ret == WS_NEED_WAKEUP || dmcc->force_compress_flag) {
			dmcc->wakeup_count++;
			if (dmcc->force_compress_flag) {
				pr_info("dmcc wakeup: force compress.\n");
				dmcc->normal_compress_flag = 0;
			} else {
				pr_info("dmcc wakeup: normal.\n");
				dmcc->normal_compress_flag = 1;
			}

			/* swap out pages. */
			busy_count = 0;
			do {
				if(dmcc->normal_compress_flag && should_cancel_reclaim(NULL)){
					printk("dmcc reclaim canceled\n");
					break;
				}
				DO_SWAP_OUT(DMCC_MODE_ANON,dmcc);
				_UPDATE_STATE();
				if (dmcc->pause_flag) {
					pr_info("dmcc pause\n");
					dmcc_thread_wait_for_pause(dmcc);
					dmcc->pause_flag = 0;
					get_cpu_load(dmcc, true);
					ret = get_system_stat(dmcc, true);
					pr_info("dmcc continue\n");
				}
				if (dmcc->full_clean_flag || busy_count > 1000)
					break;
				/* force happened in normal compress */
				if (dmcc->force_compress_flag && dmcc->normal_compress_flag)
					break;
				/* force compress complete */
				if (dmcc->force_compress_flag &&
				    nr_total_pages > dmcc->force_once_pages)
					break;
				/* normal compress complete */
				if (dmcc->normal_compress_flag &&
				    nr_total_pages > dmcc->normal_once_pages)
					break;
			} while (!(ret & WF_MEM_FREE_ENOUGH) &&
				 !(ret & WF_SWAP_FULL) &&
				 !(ret & WF_NO_ANON_PAGE) &&
				 !(ret & WF_AVAIL_ENOUGH));

			dmcc->nr_normal_clean_pages += nr_total_pages;
			dmcc->total_spent_times += one_compress_time;

			if (dmcc->normal_compress_flag) {
				pr_info("normal cc: mem=%d MB, time=%d, out_stat=%u, busy=%d\n",
					M(nr_total_pages), jiffies_to_msecs(one_compress_time),
					ret, busy_count);
				dmcc->normal_compress_flag = 0;
			} else {
				pr_info("force cc: mem=%d MB, time=%d, out_stat=%u\n",
					M(nr_total_pages), jiffies_to_msecs(one_compress_time), ret);
				dmcc->force_compress_flag = 0;
			}
		}
	}
out:
	tsk->flags &=
	    ~(PF_MEMALLOC | PF_SWAPWRITE | PF_KSWAPD | PF_FREEZER_SKIP);
	pr_info("dmcc_thread: exit.\n");
	return 0;
}

/**
 * purpose: initialize this module
 * arguments:
 *    dmcc: module handler need to be initialized.
 * return:
 *    none.
 */
static void dmcc_setup(struct dmcc_module *dmcc)
{
	init_rwsem(&dmcc->lock);
	sema_init(&dmcc->wait, 0);

	dmcc->passive_mode = 0;
	dmcc->idle_threshold = DMCC_IDLE_THRESHOLD;
	dmcc->swap_percent_low = DMCC_SWAP_PERCENT_LOW;
	dmcc->free_pages_min = DMCC_FREE_PAGE_MIN;
	dmcc->full_clean_file_pages = DMCC_FULL_CLEAN_FILE_PAGE;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 0, 0))
	if (totalram_pages > 3 * TOTAL_RAM_PAGES_1G) { /* >3g */
#else
	if (totalram_pages() > 3 * TOTAL_RAM_PAGES_1G) { /* >3g */
#endif
		dmcc->anon_pages_min = DMCC_ANON_PAGE_MIN
		 + 2 * DMCC_ANON_PAGE_RAM_GAP;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 0, 0))
	} else if (totalram_pages > 2 * TOTAL_RAM_PAGES_1G) { /* 3g */
#else
	} else if (totalram_pages() > 2 * TOTAL_RAM_PAGES_1G) { /* 3g */
#endif
		dmcc->anon_pages_min = DMCC_ANON_PAGE_MIN + DMCC_ANON_PAGE_RAM_GAP;
	} else { /*1g 2g*/
		dmcc->anon_pages_min = DMCC_ANON_PAGE_MIN;
	}

	dmcc->anon_pages_max = dmcc->anon_pages_min + DMCC_ANON_PAGE_START_GAP;
	dmcc->full_clean_anon_pages = DMCC_MAX_RECLAIM_ON_BOOT;
	dmcc->avail_target_pages = TOTAL_RAM_PAGES_1G;

	dmcc->cpu_load[0] = 100;	/* init cpu load as 100% */
	dmcc->force_once_pages = DMCC_ANON_PAGE_FORCE_ONCE;
	dmcc->normal_once_pages = DMCC_ANON_PAGE_NORMAL_ONCE;
}

/* purpose: start background thread */
static int dmcc_thread_start(struct dmcc_module *dmcc)
{
	if (dmcc->task)
		return -EFAIL;

	dmcc_set_full_clean(dmcc, 0);

	dmcc->task = kthread_run(dmcc_thread, NULL, "dmcc");
	if (IS_ERR(dmcc->task)) {
		pr_err("dmcc: failed to start thread\n");
		return -EFAIL;
	}

	pr_info("dmcc: thread started.\n");
	return 0;
}

/* purpose: stop background thread */
static void dmcc_thread_stop(struct dmcc_module *dmcc)
{
	if (!dmcc->task)
		return;
	dmcc_thread_wakeup(dmcc);	/* need wakeup thread first. */
	kthread_stop(dmcc->task);
	dmcc->task = NULL;
	pr_info("dmcc: thread stopped.\n");
}

#define DMCC_MODE_RO 0440
#define DMCC_MODE_RW 0660

#define DMCC_ATTR(_name, _mode, _show, _store) \
	struct kobj_attribute kobj_attr_##_name \
		= __ATTR(_name, _mode, _show, _store)

/* purpose: attr status: is background thread running */
static ssize_t enable_show(struct kobject *kobj, struct kobj_attribute *attr,
			   char *buf)
{
	struct dmcc_module *dmcc = &dmcc_module;

	return scnprintf(buf, PAGE_SIZE, "%u\n", dmcc_is_enabled(dmcc));
}

/* purpose: attr set: background thread start/stop */
static ssize_t enable_store(struct kobject *kobj, struct kobj_attribute *attr,
			    const char *buf, size_t len)
{
	int ret;
	u16 enable;
	struct dmcc_module *dmcc = &dmcc_module;

	ret = kstrtou16(buf, 10, &enable);
	if (ret)
		return ret;
	if (enable)
		dmcc_thread_start(dmcc);
	else
		dmcc_thread_stop(dmcc);
	return len;
}

static ssize_t event_show(struct kobject *kobj, struct kobj_attribute *attr,
			  char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "\n");
}

static ssize_t event_store(struct kobject *kobj, struct kobj_attribute *attr,
			   const char *buf, size_t len)
{
	struct dmcc_module *dmcc = &dmcc_module;

	if (!strncmp(buf, "DISPLAY_OFF", strlen("DISPLAY_OFF"))) {
		dmcc_update_display_stat(dmcc, 0);
	} else if (!strncmp(buf, "DISPLAY_ON", strlen("DISPLAY_ON"))) {
		dmcc_update_display_stat(dmcc, 1);
	} else if (!strncmp(buf, "BOOT_COMPLETE", strlen("BOOT_COMPLETE"))) {
		dmcc_set_full_clean(dmcc, 1);
		dmcc_thread_wakeup(dmcc);
	} else if (!strncmp(buf, "PASSIVE_MODE", strlen("PASSIVE_MODE"))) {
		dmcc->passive_mode = 1;
	} else if (!strncmp(buf, "POSITIVE_MODE", strlen("POSITIVE_MODE"))) {
		dmcc->passive_mode = 0;
		dmcc_thread_wakeup(dmcc);
	} else if (!strncmp(buf, "NORMAL_COMPRESS",
		strlen("NORMAL_COMPRESS"))) {
		dmcc_thread_wakeup(dmcc);
	} else if (!strncmp(buf, "PAUSE_COMPRESS", strlen("PAUSE_COMPRESS"))) {
		dmcc->pause_flag = 1;
		dmcc->pause_time = jiffies;
	} else {
		pr_err("dmcc: unknown event: [%s] size=%zu\n",
		       buf, strlen(buf));
	}
	return len;
}

/* purpose: attr status:  */
static ssize_t idle_threshold_show(struct kobject *kobj,
				   struct kobj_attribute *attr, char *buf)
{
	struct dmcc_module *dmcc = &dmcc_module;

	return scnprintf(buf, PAGE_SIZE, "%d%%\n", dmcc->idle_threshold);
}

/* purpose: attr set:  */
static ssize_t idle_threshold_store(struct kobject *kobj,
				    struct kobj_attribute *attr,
				    const char *buf, size_t len)
{
	int ret;
	u16 value;
	struct dmcc_module *dmcc = &dmcc_module;

	ret = kstrtou16(buf, 10, &value);
	if (ret)
		return ret;
	if (value > 100)
		return -EINVAL;
	dmcc->idle_threshold = value;
	return len;
}

/* purpose: attr status:  */
static ssize_t swap_percent_low_show(struct kobject *kobj,
				     struct kobj_attribute *attr, char *buf)
{
	struct dmcc_module *dmcc = &dmcc_module;

	return scnprintf(buf, PAGE_SIZE, "%d%%, extra: +%d\n",
		       dmcc->swap_percent_low, DMCC_SWAP_PERCENT_LOW_EX);
}

/* purpose: attr set:  */
static ssize_t swap_percent_low_store(struct kobject *kobj,
				      struct kobj_attribute *attr,
				      const char *buf, size_t len)
{
	int ret;
	u16 value;
	struct dmcc_module *dmcc = &dmcc_module;

	ret = kstrtou16(buf, 10, &value);
	if (ret)
		return ret;
	if (value > 100)
		return -EINVAL;
	if (value > 100 - DMCC_SWAP_PERCENT_LOW_EX)
		return value;
	dmcc->swap_percent_low = value;
	return len;
}

/* purpose: attr status:  */
static ssize_t free_size_min_show(struct kobject *kobj,
				  struct kobj_attribute *attr, char *buf)
{
	struct dmcc_module *dmcc = &dmcc_module;

	return scnprintf(buf, PAGE_SIZE, "%d MB , extra: +%d MB\n",
			M(dmcc->free_pages_min),
			M(DMCC_FREE_PAGE_MIN_EX));
}

/* purpose: attr set:  */
static ssize_t free_size_min_store(struct kobject *kobj,
				   struct kobj_attribute *attr, const char *buf,
				   size_t len)
{
	u64 size;
	struct dmcc_module *dmcc = &dmcc_module;

	size = memparse(buf, NULL);

	if (!size)
		return -EINVAL;
	dmcc->free_pages_min = (size >> PAGE_SHIFT);
	return len;
}

/* purpose: attr status:  */
static ssize_t full_clean_size_show(struct kobject *kobj,
				    struct kobj_attribute *attr, char *buf)
{
	struct dmcc_module *dmcc = &dmcc_module;
	int size = M(dmcc->full_clean_file_pages);

	return scnprintf(buf, PAGE_SIZE, "%d MB\n", size);
}

/* purpose: attr set:  */
static ssize_t full_clean_size_store(struct kobject *kobj,
				     struct kobj_attribute *attr,
				     const char *buf, size_t len)
{
	u64 size;
	unsigned long nr_file_pages;
	struct dmcc_module *dmcc = &dmcc_module;

	size = memparse(buf, NULL);
	if (!size)
		return -EINVAL;
#if (KERNEL_VERSION(4, 14, 0) <= LINUX_VERSION_CODE)
	nr_file_pages = global_node_page_state(NR_INACTIVE_FILE);
	nr_file_pages += global_node_page_state(NR_ACTIVE_FILE);
#else
	nr_file_pages = global_page_state(NR_INACTIVE_FILE);
	nr_file_pages += global_page_state(NR_ACTIVE_FILE);
#endif
	size = size >> PAGE_SHIFT;

	/* size should not larger than file cache pages. */
	if (size > nr_file_pages)
		size = nr_file_pages;

	dmcc->full_clean_file_pages = size;
	return len;
}

/* purpose: attr status:  */
static ssize_t dmcc_stat_show(struct kobject *kobj,
			     struct kobj_attribute *attr, char *buf)
{
	struct dmcc_module *dmcc = &dmcc_module;

	return scnprintf(buf, PAGE_SIZE,
		"clean pages: full=%d MB, normal=%d MB\n"
		" boot anon : min=%d MB, max=%d MB\n"
		" wake count: %d, time=%d\n"
		"       mode: %s",
		M(dmcc->nr_full_clean_pages), M(dmcc->nr_normal_clean_pages),
		M(DMCC_ANON_PAGE_MIN_ON_BOOT), M(DMCC_ANON_PAGE_MAX_ON_BOOT),
		dmcc->wakeup_count, jiffies_to_msecs(dmcc->total_spent_times),
		dmcc->passive_mode ? "passive" : "positive");
}

static ssize_t can_compress_show(struct kobject *kobj,
				 struct kobj_attribute *attr, char *buf)
{
	struct dmcc_module *dmcc = &dmcc_module;
	int value = dmcc->anon_pages_max;

	if (!is_anon_page_enough(value) || !dmcc->task || dmcc->passive_mode)
		return scnprintf(buf, PAGE_SIZE, "0\n");

	value = dmcc->swap_percent_low;
	if (is_swap_full(value))
		return scnprintf(buf, PAGE_SIZE, "0\n");

	value = dmcc->avail_target_pages;
	if (is_avail_above_target(value))
		return scnprintf(buf, PAGE_SIZE, "0\n");

	if (dmcc->pause_flag)
		return scnprintf(buf, PAGE_SIZE, "0\n");

	return scnprintf(buf, PAGE_SIZE, "1\n");
}

/* purpose: show the size of max anon page to reclaim  */
static ssize_t max_anon_clean_size_show(struct kobject *kobj,
					struct kobj_attribute *attr, char *buf)
{
	struct dmcc_module *dmcc = &dmcc_module;
	int size = M(dmcc->full_clean_anon_pages);

	return scnprintf(buf, PAGE_SIZE, "%d MB\n", size);
}

/* purpose: set max anon page size to reclaim */
static ssize_t max_anon_clean_size_store(struct kobject *kobj,
					 struct kobj_attribute *attr,
					 const char *buf, size_t len)
{
	u64 size;
	struct dmcc_module *dmcc = &dmcc_module;

	size = memparse(buf, NULL);
	if (!size)
		return -EINVAL;

	size = size >> PAGE_SHIFT;
	dmcc->full_clean_anon_pages = size;
	return len;
}

/* purpose: target avail mem to compress  */
static ssize_t avail_target_show(struct kobject *kobj,
				 struct kobj_attribute *attr, char *buf)
{
	struct dmcc_module *dmcc = &dmcc_module;
	int size = M(dmcc->avail_target_pages);

	return scnprintf(buf, PAGE_SIZE, "%d MB\n", size);
}

static ssize_t avail_target_store(struct kobject *kobj,
				  struct kobj_attribute *attr,
				  const char *buf, size_t len)
{
	u64 size;
	struct dmcc_module *dmcc = &dmcc_module;
	/* read as mb */
	size = memparse(buf, NULL);
	if (!size)
		return -EINVAL;

	size = size >> PAGE_SHIFT;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 0, 0))
	if (size > totalram_pages * 4)
#else
	if (size > totalram_pages() * 4)
#endif
		return -EINVAL;
	dmcc->avail_target_pages = size;
	return len;
}

static ssize_t anon_target_show(struct kobject *kobj,
				struct kobj_attribute *attr, char *buf)
{
	struct dmcc_module *dmcc = &dmcc_module;

	return scnprintf(buf, PAGE_SIZE, "%d MB ,extra:%d MB\n",
			 M(dmcc->anon_pages_min), M(dmcc->anon_pages_max));
}

static ssize_t anon_target_store(struct kobject *kobj,
				 struct kobj_attribute *attr,
				 const char *buf, size_t len)
{
	u64 size;
	struct dmcc_module *dmcc = &dmcc_module;
	/* read as mb */
	size = memparse(buf, NULL);
	if (!size)
		return -EINVAL;

	size = size >> PAGE_SHIFT;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 0, 0))
	if (size > totalram_pages * 4)
#else
	if (size > totalram_pages() * 4)
#endif
		return -EINVAL;
	dmcc->anon_pages_min = size;
	dmcc->anon_pages_max = dmcc->anon_pages_min + DMCC_ANON_PAGE_START_GAP;
	return len;
}

static ssize_t force_once_show(struct kobject *kobj,
			       struct kobj_attribute *attr, char *buf)
{
	struct dmcc_module *dmcc = &dmcc_module;
	int size = M(dmcc->force_once_pages);

	return scnprintf(buf, PAGE_SIZE, "%d MB\n", size);
}

/* purpose: wake compress ignoring cpu */
static ssize_t force_once_store(struct kobject *kobj,
				struct kobj_attribute *attr,
				const char *buf, size_t len)
{
	u64 size;
	struct dmcc_module *dmcc = &dmcc_module;
	/* read as mb */
	size = memparse(buf, NULL);
	if (!size)
		return -EINVAL;

	if (dmcc->full_clean_flag)
		return -EINVAL;

	size = size << (20 - PAGE_SHIFT);
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 0, 0))
	if (size > totalram_pages * 4)
#else
	if (size > totalram_pages() * 4)
#endif
		return -EINVAL;
	dmcc->force_once_pages = size;
	dmcc->force_compress_flag = 1;
	dmcc_thread_wakeup(dmcc);
	return len;
}

static DMCC_ATTR(enable, DMCC_MODE_RW, enable_show, enable_store);
static DMCC_ATTR(can_compress, DMCC_MODE_RO, can_compress_show, NULL);
static DMCC_ATTR(event, DMCC_MODE_RW, event_show, event_store);
static DMCC_ATTR(idle_threshold, DMCC_MODE_RW, idle_threshold_show,
		idle_threshold_store);
static DMCC_ATTR(swap_percent_low, DMCC_MODE_RW, swap_percent_low_show,
		swap_percent_low_store);
static DMCC_ATTR(free_size_min, DMCC_MODE_RW, free_size_min_show,
		free_size_min_store);
static DMCC_ATTR(full_clean_size, DMCC_MODE_RW, full_clean_size_show,
		full_clean_size_store);
static DMCC_ATTR(stat, DMCC_MODE_RO, dmcc_stat_show, NULL);
static DMCC_ATTR(max_anon_clean_size, DMCC_MODE_RW, max_anon_clean_size_show,
		max_anon_clean_size_store);
static DMCC_ATTR(avail_target, DMCC_MODE_RW, avail_target_show,
		avail_target_store);
static DMCC_ATTR(anon_target, DMCC_MODE_RW, anon_target_show,
		anon_target_store);
static DMCC_ATTR(force_once, DMCC_MODE_RW, force_once_show,
		force_once_store);

static struct attribute *dmcc_attrs[] = {
	&kobj_attr_enable.attr,
	&kobj_attr_can_compress.attr,
	&kobj_attr_event.attr,
	&kobj_attr_idle_threshold.attr,
	&kobj_attr_swap_percent_low.attr,
	&kobj_attr_free_size_min.attr,
	&kobj_attr_full_clean_size.attr,
	&kobj_attr_stat.attr,
	&kobj_attr_max_anon_clean_size.attr,
	&kobj_attr_avail_target.attr,
	&kobj_attr_anon_target.attr,
	&kobj_attr_force_once.attr,
	NULL,
};

static struct attribute_group dmcc_module_attr_group = {
	.attrs = dmcc_attrs,
};

/**
 * purpose: create sysfs nodes for module
 * arguments:
 *    none
 * return:
 *    kobject : for future destroy.
 */
static struct kobject *sysfs_create(void)
{
	int err;
	struct kobject *kobj = NULL;

	kobj = kobject_create_and_add("dmcc", kernel_kobj);

	if (!kobj) {
		pr_err("dmcc: failed to create sysfs node.\n");
		return NULL;
	}
	err = sysfs_create_group(kobj, &dmcc_module_attr_group);
	if (err) {
		pr_err("dmcc: failed to create sysfs attrs.\n");
		kobject_put(kobj);
		return NULL;
	}
	return kobj;
}

/**
 * purpose: destroy sysfs nodes
 * arguments:
 *    kobj : kobject for release.
 * return:
 *    none
 */
static void sysfs_destroy(struct kobject *kobj)
{
	if (!kobj)
		return;
	kobject_put(kobj);
}

/* purpose: this module init */
static int __init dmcc_init(void)
{
	/* int ret; */
	struct dmcc_module *dmcc = &dmcc_module;

	pr_info("dmcc init...\n");

	dmcc_setup(dmcc);

	dmcc->kobj = sysfs_create();
	if (!dmcc->kobj)
		goto failed_to_create_sysfs;

	pr_info("dmcc inited successfully\n");
	return 0;

failed_to_create_sysfs:
	return -EFAIL;
}

/* purpose: this module de-init */
static void __exit dmcc_exit(void)
{
	struct dmcc_module *dmcc = &dmcc_module;

	sysfs_destroy(dmcc->kobj);
	dmcc->kobj = NULL;

	dmcc_thread_stop(dmcc);
}

module_init(dmcc_init);
module_exit(dmcc_exit);
