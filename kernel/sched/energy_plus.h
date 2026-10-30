
#ifdef CONFIG_MTK_UNIFY_POWER
extern int
mtk_idle_power(int cpu_idx, int idle_state, int cpu, void *argu, int sd_level);

extern
int mtk_busy_power(int cpu_idx, int cpu, void *argu, int sd_level);
#endif

