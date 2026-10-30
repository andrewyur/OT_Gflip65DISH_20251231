
#ifndef __SENINF_H__
#define __SENINF_H__

#include "seninf_cfg.h"
#include "seninf_clk.h"

#include <linux/atomic.h>
#include <linux/mutex.h>

#define SENINF_DEV_NAME "seninf"

struct SENINF {
	dev_t dev_no;
	struct cdev *pchar_dev;
	struct class *pclass;

	struct SENINF_CLK clk;

	void __iomem *pseninf_base[SENINF_MAX_NUM];

	struct mutex seninf_mutex;
	atomic_t seninf_open_cnt;
};
extern MINT32 seninf_dump_reg(void);
#ifdef SENINF_IRQ
extern MINT32 _seninf_irq(MINT32 Irq, void *DeviceId, struct SENINF *pseninf);
#endif

#endif

