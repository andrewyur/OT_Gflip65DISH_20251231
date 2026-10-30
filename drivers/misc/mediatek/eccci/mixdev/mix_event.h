
#ifndef __MIX_EVENT_H__
#define __MIX_EVENT_H__

#include <linux/ioctl.h>

#define MIXDEV_IOC_MAGIC 'M'

extern void inject_mix_event(struct sk_buff *skb,
			   struct net_device *dev,
			   struct iphdr *iph);



#endif /* __MIX_EVENT_H__ */
