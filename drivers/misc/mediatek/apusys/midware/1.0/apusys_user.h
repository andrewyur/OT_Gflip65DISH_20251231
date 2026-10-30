
#ifndef __APUSYS_USER_H__
#define __APUSYS_USER_H__


#include "apusys_drv.h"
#include "apusys_device.h"

struct apusys_user {
	/* basic info */
	uint64_t id;
	pid_t open_pid;
	pid_t open_tgid;
	char comm[TASK_COMM_LEN];
	unsigned int iova_size_max;
	unsigned int iova_size;
	//struct completion comp;

	/* relate cmd */
	struct list_head cmd_list;
	struct mutex cmd_mtx;

	/* memory mgt */
	struct list_head mem_list;
	struct mutex mem_mtx;

	/* acquired dev */
	struct list_head dev_list;
	struct mutex dev_mtx;

	/* secure acquired dev */
	struct list_head secdev_list;
	struct mutex secdev_mtx;

	/* for driver management only */
	struct list_head list;
};

void apusys_user_dump(void *s_file);
void apusys_user_show_log(void *s_file);
void apusys_user_print_log(void);
int apusys_user_insert_cmd(struct apusys_user *user, void *icmd);
int apusys_user_delete_cmd(struct apusys_user *user, void *icmd);
int apusys_user_get_cmd(struct apusys_user *user, void **icmd, uint64_t cmd_id);
int apusys_user_insert_dev(struct apusys_user *user, void *idev);
int apusys_user_delete_dev(struct apusys_user *user, void *idev);
struct apusys_dev_info *apusys_user_get_dev
	(struct apusys_user *user, uint64_t hnd);
int apusys_user_insert_secdev(struct apusys_user *user, void *idev_info);
int apusys_user_delete_secdev(struct apusys_user *user, void *idev_info);
int apusys_user_delete_sectype(struct apusys_user *u, int dev_type);
int apusys_user_insert_mem(struct apusys_user *user, struct apusys_kmem *mem);
int apusys_user_delete_mem(struct apusys_user *user, struct apusys_kmem *mem);
struct apusys_kmem *apusys_user_get_mem(struct apusys_user *u, int fd);
int apusys_create_user(struct apusys_user **user);
int apusys_delete_user(struct apusys_user *user);

int apusys_user_init(void);
void apusys_user_destroy(void);

#endif
