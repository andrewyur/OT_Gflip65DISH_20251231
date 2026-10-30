#ifndef _LOWMEM_HINT_H
#define _LOWMEM_HINT_H

#ifdef CONFIG_MTK_LOWMEM_HINT
extern void trigger_lowmem_hint(long *out_avail_mem,
				long *out_free_mem);
#else
static inline void trigger_lowmem_hint(long *out_avail_mem,
				       long *out_free_mem) {}
#endif
#endif /* _LOWMEM_HINT_H */
