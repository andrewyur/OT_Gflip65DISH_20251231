

#ifndef __HELIO_DVFSRC_OPP_H
#define __HELIO_DVFSRC_OPP_H

#if defined(CONFIG_MACH_MT6775)
#include <helio-dvfsrc-opp-mt6775.h>
#elif defined(CONFIG_MACH_MT6771)
#include <helio-dvfsrc-opp-mt6771.h>
#elif defined(CONFIG_MACH_MT8168)
#include <helio-dvfsrc-opp-mt8168.h>
#endif

struct opp_profile {
	int vcore_uv;
	int ddr_khz;
};

extern int get_cur_vcore_dvfs_opp(void);
extern void set_opp_table(int vcore_dvfs_opp, int vcore_uv, int ddr_khz);

extern int get_vcore_opp(int opp);
extern int get_vcore_uv(int opp);
extern int get_cur_vcore_opp(void);
extern int get_cur_vcore_uv(void);
extern void set_vcore_opp(int vcore_dvfs_opp, int vcore_opp);

extern int get_ddr_opp(int opp);
extern int get_ddr_khz(int opp);
extern int get_cur_ddr_opp(void);
extern int get_cur_ddr_khz(void);
extern void set_ddr_opp(int vcore_dvfs_opp, int ddr_opp);

extern void set_vcore_sram_uv_table(int vcore_sram_opp, int vcore_sram_uv);
extern void set_vcore_uv_table(int vcore_opp, int vcore_uv);
extern int get_vcore_uv_table(int vcore_opp);

extern void set_pwrap_cmd(int vcore_opp, int pwrap_cmd);
extern int get_pwrap_cmd(int vcore_opp);
extern int get_opp_ddr_freq(int ddr_opp);
extern void set_opp_ddr_freq(int ddr_opp, int ddr_freq);

#endif
