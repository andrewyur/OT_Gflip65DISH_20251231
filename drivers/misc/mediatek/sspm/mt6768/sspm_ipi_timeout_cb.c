
#include <linux/module.h>       /* needed by all modules */
#include <mt-plat/sync_write.h>
#include "sspm_define.h"

/* debug API */
__weak void dump_emi_outstanding(void) {}
__weak void mtk_spm_dump_debug_info(void) {}
__weak void usb_dump_debug_register(void) {}
__weak void dpmaif_dump_reg(void) {}

/* platform callback when ipi timeout */
void sspm_ipi_timeout_cb(int ipi_id)
{
	/* for debug EMI use */
	pr_info("%s: dump_emi_outstanding\n", __func__);
	dump_emi_outstanding();
}

