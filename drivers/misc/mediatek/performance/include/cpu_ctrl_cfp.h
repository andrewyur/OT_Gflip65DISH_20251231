#ifndef CPU_CTRL_CFP_H
#define CPU_CTRL_CFP_H

#include "cpu_ctrl.h"

extern void cpu_ctrl_cfp(struct ppm_limit_data *desired_freq);
extern void cpu_ctrl_cfp_isolation(int enable, int cpu);

extern int cpu_ctrl_cfp_init(struct proc_dir_entry *parent);
extern void cpu_ctrl_cfp_exit(void);
#endif

