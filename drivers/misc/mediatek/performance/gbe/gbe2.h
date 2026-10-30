
#ifndef GBE2_H
#define GBE2_H

#if defined(CONFIG_MTK_GBE)
void fpsgo_comp2gbe_frame_update(int pid, unsigned long long bufID);
#else
static inline void fpsgo_comp2gbe_frame_update(int pid,
	unsigned long long bufID) { }
#endif
extern void gbe2_exit(void);

extern int gbe2_init(void);

#endif

