
#ifndef __ION_SEC_HEAP_H__
#define __ION_SEC_HEAP_H__

#include <linux/dma-buf.h>
#include "ion_drv.h"
#ifdef CONFIG_MTK_TRUSTED_MEMORY_SUBSYSTEM
#include "trusted_mem_api.h"
#endif

struct ion_sec_buffer_info {
	struct mutex lock;/*mutex lock on secure buffer*/
	int module_id;
	unsigned int security;
	unsigned int coherent;
	void *VA;
	unsigned int MVA;
	unsigned int FIXED_MVA;
	unsigned long iova_start;
	unsigned long iova_end;
	ion_phys_addr_t priv_phys;
	struct ion_mm_buf_debug_info dbg_info;
	pid_t pid;
};

#ifdef CONFIG_MTK_TRUSTED_MEMORY_SUBSYSTEM
enum TRUSTED_MEM_REQ_TYPE ion_get_trust_mem_type(struct dma_buf *dmabuf);
#endif
#endif
