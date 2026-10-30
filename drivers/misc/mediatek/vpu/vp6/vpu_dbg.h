
#ifndef __VPU_DBG_H__
#define __VPU_DBG_H__


enum VpuFuncMask {
	VFM_NEED_WAIT_VCORE		= 0x1,
	VFM_ROUTINE_PRT_SYSLOG		= 0x2,
	VFM_ROUTINE_SET_MPU		= 0x4
};

enum VpuLogThre {
	/* >1, performance break down check */
	VpuLogThre_PERFORMANCE    = 1,

	/* >2, algo info, opp info check */
	Log_ALGO_OPP_INFO  = 2,

	/* >3, state machine check, while wait vcore/do running */
	Log_STATE_MACHINE  = 3,

	/* >4, dump buffer mva */
	VpuLogThre_DUMP_BUF_MVA   = 4
};

#endif
