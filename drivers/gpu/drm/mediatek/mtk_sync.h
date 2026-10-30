
#ifndef _MTK_SYNC_H
#define _MTK_SYNC_H

#define TIMEOUT_NEVER -1


#ifdef __KERNEL__

#include <linux/version.h>
#include <linux/dma-fence.h>

struct sync_timeline {
	struct kref		kref;
	char			name[32];

	/* protected by lock */
	u64			context;
	int			value;

	struct rb_root		pt_tree;
	struct list_head	pt_list;
	spinlock_t		lock;

	struct list_head	sync_timeline_list;
};

struct sync_pt {
	struct dma_fence base;
	struct list_head link;
	struct rb_node node;
};


struct fence_data {
	__u32 value;
	char name[32];
	__s32 fence; /* fd of new fence */
};



struct sync_timeline *mtk_sync_timeline_create(const char *name);

void mtk_sync_timeline_destroy(struct sync_timeline *obj);

void mtk_sync_timeline_inc(struct sync_timeline *obj, u32 value);

int mtk_sync_fence_create(struct sync_timeline *obj, struct fence_data *data);

#endif /* __KERNEL __ */

#endif /* _MTK_SYNC_H */
