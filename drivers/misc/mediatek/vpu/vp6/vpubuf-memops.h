
#ifndef _VPUBUF_MEMOPS_H
#define _VPUBUF_MEMOPS_H

#include "vpubuf-core.h"
#include <linux/mm.h>

struct vpu_vmarea_handler {
	atomic_t *refcount;
	void (*put)(void *arg);
	void *arg;
};

extern const struct vm_operations_struct vpu_common_vm_ops;

struct frame_vector *vpu_create_framevec(unsigned long start,
					 unsigned long length, bool write);
void vpu_destroy_framevec(struct frame_vector *vec);

#endif
