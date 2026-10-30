
#ifndef __MTK_SPM_IRQ_EDGE_H__
#define __MTK_SPM_IRQ_EDGE_H__


struct edge_trigger_irq_list {
	const char *name;
	int order;
	unsigned int wakesrc;
};

static struct edge_trigger_irq_list list[] = {
	{ "mediatek,kp",		0,	R12_KP_IRQ_B },
};

#endif /* __MTK_SPM_IRQ_EDGE_H__ */
