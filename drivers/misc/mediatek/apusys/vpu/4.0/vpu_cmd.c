
#include <linux/wait.h>
#include <linux/mutex.h>
#include <linux/sched/clock.h>
#include "vpu_power.h"
#include "vpu_cfg.h"
#include "vpu_cmd.h"
#include "vpu_reg.h"
#include "vpu_algo.h"

static unsigned int vpu_prio(int p)
{
	if (p < 0)
		return 0;
	if (p >= VPU_MAX_PRIORITY)
		return (VPU_MAX_PRIORITY - 1);
	return (unsigned int)p;
}

int vpu_cmd_init(struct platform_device *pdev, struct vpu_device *vd)
{
	int i;
	int ret = 0;
	struct vpu_cmd_ctl *c;
	dma_addr_t iova;

	vd->cmd_timeout = VPU_CMD_TIMEOUT;
	atomic_set(&vd->cmd_prio, 0);
	atomic_set(&vd->cmd_active, 0);
	vd->cmd_prio_max = VPU_MAX_PRIORITY;

	for (i = 0; i < vd->cmd_prio_max; i++) {
		c = &vd->cmd[i];
		mutex_init(&c->lock);
		init_waitqueue_head(&c->wait);
		c->done = false;
		c->boost = VPU_PWR_NO_BOOST;
		c->vi.bin = VPU_MEM_ALLOC;
		c->vi.size = VPU_CMD_SIZE;
		c->exe_cnt = 0;
		iova = vpu_iova_alloc(pdev, &c->vi);
		if (!iova) {
			vd->cmd_prio_max = i;
			vpu_cmd_exit(vd);
			ret = -ENOMEM;
			break;
		}
	}
	return ret;
}

void vpu_cmd_exit(struct vpu_device *vd)
{
	int i;

	for (i = 0; i < vd->cmd_prio_max; i++)
		vpu_iova_free(vd->dev, &vd->cmd[i].vi);
}

void vpu_cmd_clear(struct vpu_device *vd)
{
	int i;
	struct vpu_cmd_ctl *c;
	struct vpu_algo_list *al;

	for (i = 0; i < vd->cmd_prio_max; i++) {
		c = &vd->cmd[i];
		c->cmd = 0;
		c->done = false;
		c->alg_ret = 0;
		c->result = 0;
		c->start_t = 0;
		c->end_t = 0;
		c->boost = VPU_PWR_NO_BOOST;
		if (c->alg && c->alg->al) {
			al = c->alg->al;
			if (al->ops->unload)
				al->ops->unload(al, i);
		}
		atomic_set(&vd->cmd_active, 0);
	}
}

void vpu_cmd_lock(struct vpu_device *vd, int prio)
{
	unsigned int p = vpu_prio(prio);
	struct vpu_cmd_ctl *c = &vd->cmd[p];

	mutex_lock_nested(&c->lock, VPU_MUTEX_CMD + p);
	c->start_t = sched_clock();
	c->end_t = 0;
}

void vpu_cmd_unlock(struct vpu_device *vd, int prio)
{
	struct vpu_cmd_ctl *c = &vd->cmd[vpu_prio(prio)];

	c->end_t = sched_clock();
	mutex_unlock(&c->lock);
}

void vpu_cmd_lock_all(struct vpu_device *vd)
{
	int i;

	for (i = 0; i < vd->cmd_prio_max; i++)
		mutex_lock_nested(&vd->cmd[i].lock, VPU_MUTEX_CMD + i);

	mutex_lock_nested(&vd->lock, VPU_MUTEX_DEV);
}

void vpu_cmd_unlock_all(struct vpu_device *vd)
{
	int i;

	mutex_unlock(&vd->lock);

	for (i = (vd->cmd_prio_max - 1); i >= 0; i--)
		mutex_unlock(&vd->cmd[i].lock);
}

void vpu_cmd_run(struct vpu_device *vd, int prio, uint32_t cmd)
{
	struct vpu_cmd_ctl *c = &vd->cmd[vpu_prio(prio)];

	c->cmd = cmd;
	c->done = false;
	c->alg_ret = 0;
	c->result = 0;
	c->exe_cnt++;
	atomic_inc(&vd->cmd_active);
}

void vpu_cmd_done(struct vpu_device *vd, int prio,
	uint32_t result, uint32_t alg_ret)
{
	struct vpu_cmd_ctl *c = &vd->cmd[vpu_prio(prio)];

	c->done = true;
	c->alg_ret = alg_ret;
	c->result = result;
	atomic_dec(&vd->cmd_active);
}

void vpu_cmd_alg_set(struct vpu_device *vd, int prio, struct __vpu_algo *alg)
{
	vd->cmd[vpu_prio(prio)].alg = alg;
}

struct __vpu_algo *vpu_cmd_alg(struct vpu_device *vd, int prio)
{
	return vd->cmd[vpu_prio(prio)].alg;
}

wait_queue_head_t *vpu_cmd_waitq(struct vpu_device *vd, int prio)
{
	return &vd->cmd[vpu_prio(prio)].wait;
}

void vpu_cmd_wake_all(struct vpu_device *vd)
{
#if VPU_XOS
	int i;

	for (i = 0; i < vd->cmd_prio_max; i++) {
		vd->cmd[i].done = true;
		vd->cmd[i].result = VPU_STATE_ABORT;
		wake_up_interruptible(vpu_cmd_waitq(vd, i));
	}
#endif
}

const char *vpu_cmd_alg_name(struct vpu_device *vd, int prio)
{
	struct __vpu_algo *alg;

	alg = vpu_cmd_alg(vd, prio);

	return (alg) ? alg->a.name : "";
}

int vpu_cmd_result(struct vpu_device *vd, int prio)
{
	switch (vd->cmd[vpu_prio(prio)].result) {
	case VPU_STATE_READY:
	case VPU_STATE_IDLE:
		return 0;
	case VPU_STATE_NOT_READY:
	case VPU_STATE_ERROR:
		return -EIO;
	case VPU_STATE_BUSY:
		return -EBUSY;
	case VPU_STATE_TERMINATED:
	case VPU_STATE_ABORT:
		return -EBADFD;
	default:
		return -EINVAL;
	}
	return 0;
}

uint32_t vpu_cmd_alg_ret(struct vpu_device *vd, int prio)
{
	return vd->cmd[vpu_prio(prio)].alg_ret;
}

static void *vpu_cmd_buf_va(struct vpu_device *vd, int prio)
{
	return (void *)vd->cmd[vpu_prio(prio)].vi.m.va;
}

uint32_t vpu_cmd_buf_iova(struct vpu_device *vd, int prio)
{
	return vd->cmd[vpu_prio(prio)].vi.m.pa;
}

static void vpu_cmd_buf_sync(struct vpu_device *vd, int prio)
{
	vpu_iova_sync_for_device(vd->dev, &vd->cmd[vpu_prio(prio)].vi);
}

int vpu_cmd_buf_set(struct vpu_device *vd, int prio, void *buf, size_t size)
{
	if (size > VPU_CMD_SIZE)
		return -EINVAL;

	memcpy(vpu_cmd_buf_va(vd, prio), buf, size);
	vpu_cmd_buf_sync(vd, prio);
	return 0;
}

static int vpu_cmd_boost_elevate(struct vpu_device *vd)
{
#if VPU_XOS
	int i;
	int boost = -1;

	for (i = 0; i < vd->cmd_prio_max; i++) {
		if (vd->cmd[i].boost == VPU_PWR_NO_BOOST)
			continue;
		boost = max_t(int, vd->cmd[i].boost, boost);
	}

	return (boost >= 0) ? boost : VPU_PWR_NO_BOOST;
#else
	return vd->cmd[0].boost;
#endif
}

int vpu_cmd_boost_set(struct vpu_device *vd, int prio, int boost)
{
	vd->cmd[vpu_prio(prio)].boost = boost;
	return vpu_cmd_boost_elevate(vd);
}

int vpu_cmd_boost_put(struct vpu_device *vd, int prio)
{
	return vpu_cmd_boost_set(vd, prio, VPU_PWR_NO_BOOST);
}

int vpu_cmd_boost(struct vpu_device *vd, int prio)
{
	return vd->cmd[vpu_prio(prio)].boost;
}



