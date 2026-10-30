

#include "vow_hw.h"
#ifdef CONFIG_MTK_TINYSYS_SCP_SUPPORT
#include "scp_ipi.h"
#endif

unsigned int vow_check_scp_status(void)
{
#ifdef CONFIG_MTK_TINYSYS_SCP_SUPPORT
	return is_scp_ready(SCP_A_ID);
#else
	return 0;
#endif
}
