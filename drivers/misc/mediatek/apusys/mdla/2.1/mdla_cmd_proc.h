
#ifndef __MDLA_CMD_PROC_H__
#define __MDLA_CMD_PROC_H__
#include "mdla.h"
void mdla_wakeup_source_init(void);
void mdla_command_done(unsigned int core_id);
int mdla_run_command_sync(
	struct mdla_run_cmd *cd,
	struct mdla_dev *mdla_info,
	struct apusys_cmd_hnd *apusys_hd,
	u16 priority);
#endif//__MDLA_CMD_PROC_H__
