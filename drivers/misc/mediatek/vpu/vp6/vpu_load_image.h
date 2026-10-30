
#ifndef _VPU_LOAD_IMAGE_H_
#define _VPU_LOAD_IMAGE_H_

#include "vpu_cmn.h"

#define VPU_LOAD_IMAGE_USLEEP_MIN	(1000)
#define VPU_LOAD_IMAGE_USLEEP_MAX	(2000)
#define VPU_LOAD_IMAGE_USLEEP_TIMEOUT	(1000000 / (VPU_LOAD_IMAGE_USLEEP_MIN))

int vpu_load_image(struct vpu_device *vpu_device);
int vpu_unload_image(struct vpu_device *vpu_device);
int vpu_check_load_image_state(struct vpu_device *vpu_device);
int vpu_wait_load_image(struct vpu_device *vpu_device);

#endif /* #ifndef _VPU_LOAD_IMAGE_H_ */

