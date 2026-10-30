
#ifndef __APUSYS_THREAD_POOL_H__
#define __APUSYS_THREAD_POOL_H__

enum {
	APUSYS_THREAD_STATUS_IDLE,
	APUSYS_THREAD_STATUS_BUSY,

	APUSYS_THREAD_STATUS_MAX,
};

#define APUSYS_THD_TASK_FILE_PATH "/dev/stune/low_latency/tasks"

typedef int (*routine_func)(void *, void *);

/* for dump thread status */
void thread_pool_dump(void);

int thread_pool_trigger(void *cmd, void *dev_info);
void thread_pool_set_group(void);

int thread_pool_add_once(void);
int thread_pool_delete(int num);
int thread_pool_init(routine_func func_ptr);
int thread_pool_destroy(void);

#endif
