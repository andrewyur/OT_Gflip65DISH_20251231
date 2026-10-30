
#ifndef __MTK_QOS_PREFETCH_COMMON_H__
#define __MTK_QOS_PREFETCH_COMMON_H__

#ifdef QOS_PREFETCH_SUPPORT
extern void qos_prefetch_tick(int cpu);
#else
__weak void qos_prefetch_tick(int cpu) {}
#endif

#endif
