
#include <linux/device.h>       /* needed by device_* */
#include "scp_ipi_pin.h"
#include "scp_mbox_layout.h"
#include "scp_scpctl.h"

static ssize_t scpctl_store(struct device *kobj
		, struct device_attribute *attr, const char *buf, size_t n)
{
	int ret;
	int magic, type, op;
	struct scpctl_cmd_s cmd;
	char *prompt = "[SCPCTL]:";

	if (sscanf(buf, "%d %d %d", &magic, &type, &op) != 3)
		return -EINVAL;
	pr_notice("%s %d %d %d\n", prompt, magic, type, op);

	if (magic != 666)
		return -EINVAL;

	cmd.type = type;
	cmd.op = op;

	ret = mtk_ipi_send(&scp_ipidev, IPI_OUT_SCPCTL_1, 0, &cmd,
			   PIN_OUT_SIZE_SCPCTL_1, 0);
	if (ret != IPI_ACTION_DONE)
		goto _err;

	return n;

_err:
	pr_notice("%s failed, %d\n", prompt, ret);
	return -EIO;
}
DEVICE_ATTR(scpctl, 0200, NULL, scpctl_store);


