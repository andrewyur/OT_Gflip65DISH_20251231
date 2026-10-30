
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/dma-mapping.h>
#include <linux/vmalloc.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/file.h>

#include "vpubuf-core.h"
#include "vpubuf-memops.h"
#if 0	/* to do */
struct frame_vector *vpu_create_framevec(unsigned long start,
					 unsigned long length, bool write)
{
	int ret;
	unsigned long first, last;
	unsigned long nr;
	struct frame_vector *vec;

	first = start >> PAGE_SHIFT;
	last = (start + length - 1) >> PAGE_SHIFT;
	nr = last - first + 1;
	vec = frame_vector_create(nr);
	if (!vec)
		return ERR_PTR(-ENOMEM);
	ret = get_vaddr_frames(start & PAGE_MASK, nr, write, true, vec);
	if (ret < 0)
		goto out_destroy;
	/* We accept only complete set of PFNs */
	if (ret != nr) {
		ret = -EFAULT;
		goto out_release;
	}
	return vec;
out_release:
	put_vaddr_frames(vec);
out_destroy:
	frame_vector_destroy(vec);
	return ERR_PTR(ret);
}

void vpu_destroy_framevec(struct frame_vector *vec)
{
	put_vaddr_frames(vec);
	frame_vector_destroy(vec);
}
#endif
static void vpu_common_vm_open(struct vm_area_struct *vma)
{
	struct vpu_vmarea_handler *h = vma->vm_private_data;

	pr_debug("%s: %p, refcount: %d, vma: %08lx-%08lx\n",
		 __func__, h, atomic_read(h->refcount), vma->vm_start,
		 vma->vm_end);

	atomic_inc(h->refcount);
}

static void vpu_common_vm_close(struct vm_area_struct *vma)
{
	struct vpu_vmarea_handler *h = vma->vm_private_data;

	pr_debug("%s: %p, refcount: %d, vma: %08lx-%08lx\n",
		 __func__, h, atomic_read(h->refcount), vma->vm_start,
		 vma->vm_end);

	h->put(h->arg);
}

const struct vm_operations_struct vpu_common_vm_ops = {
	.open = vpu_common_vm_open,
	.close = vpu_common_vm_close,
};
