
#include <linux/types.h>
#include <linux/tracepoint.h>
#include <linux/tracepoint-defs.h>
#include <linux/string.h>
#include "apu_tp.h"

#define apu_tp_foreach(tbl, t) \
	for ((t) = (tbl); (t)->name != NULL; (t)++)

static void apu_tp_lookup(struct tracepoint *tp, void *priv)
{
	struct apu_tp_tbl *t, *tbl;

	tbl = (struct apu_tp_tbl *)priv;

	apu_tp_foreach(tbl, t) {
		if (!strcmp(t->name, tp->name))
			t->tp = tp;
	}
}

void apu_tp_exit(struct apu_tp_tbl *tbl)
{
	struct apu_tp_tbl *t;

	apu_tp_foreach(tbl, t) {
		if (!t->tp)
			continue;
		tracepoint_probe_unregister(t->tp, t->func, NULL);
		t->registered = false;
	}
}

int apu_tp_init(struct apu_tp_tbl *tbl)
{
	struct apu_tp_tbl *t;

	/* install tracepoints */
	for_each_kernel_tracepoint(apu_tp_lookup, tbl);
	apu_tp_foreach(tbl, t) {
		if (!t->tp) {
			pr_info("%s: %s was not found\n", __func__, t->name);
			apu_tp_exit(tbl);  /* free registered entries */
			return -EINVAL;
		}
		tracepoint_probe_register(t->tp, t->func, NULL);
		t->registered = true;
	}
	return 0;
}

