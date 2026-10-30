
#ifndef __VPU_HW_H__
#define __VPU_HW_H__

#include "vpu_cfg.h"
#include "vpu_cmn.h"
#include "vpu_reg.h"

struct vpu_code_segment {
	uint32_t vpu_core;   /* core index*/
	uint32_t offset;     /* offset from this partition */
	uint32_t dst_addr;   /* the DDR position is IPU can realize. */
	uint32_t length;     /* total size for this segment */
	uint32_t file_size;  /* file size to copy */
};

/* normal algorithm info */
struct vpu_algo_info {
	uint32_t vpu_core;       /* core index*/
	uint32_t offset;
	uint32_t length;
	char name[ALGO_NAMELEN];
};

/* preloaded algorithm info */
struct vpu_pre_info {
	uint32_t vpu_core;
	uint32_t off;
	uint32_t pAddr;
	uint32_t mem_sz;
	uint32_t file_sz;
	uint32_t flag;
	uint32_t info;
	uint32_t reserve;
	char name[ALGO_NAMELEN];
};


#if VPU_IMG_LEGACY
struct vpu_image_header {
	uint32_t version;
	uint32_t build_date;
	uint32_t header_desc[8];
	uint32_t header_size;
	uint32_t image_size;
	uint32_t code_segment_count;
	struct vpu_code_segment code_segments[VPU_MAX_NUM_CODE_SEGMENTS];
	uint32_t algo_info_count;
	struct vpu_algo_info algo_infos[VPU_MAX_NUM_ALGOS];
	uint32_t reserved[32];
};
#endif

#if VPU_IMG_PRELOAD
struct vpu_image_header {
	uint32_t version;
	uint32_t build_date;
	uint32_t header_desc[8];
	uint32_t header_size;
	uint32_t image_size;
	uint32_t mem_size;
	uint32_t code_segment_count;
	uint32_t seg_info;
	uint32_t seg_size;
	uint32_t pre_info_count;
	uint32_t pre_info;
	uint32_t pre_size;
	uint32_t algo_info_count;
	uint32_t alg_info;
	uint32_t reserved[4];
};
#endif


int vpu_dev_boot(struct vpu_device *vd);

int vpu_dev_boot_sequence(struct vpu_device *vd);

int vpu_dev_set_debug(struct vpu_device *vd);

void vpu_xos_unlock(struct vpu_device *vd);

int vpu_xos_wait_idle(struct vpu_device *vd);

#endif
