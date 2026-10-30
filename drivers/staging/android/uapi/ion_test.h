
#ifndef _UAPI_LINUX_ION_TEST_H
#define _UAPI_LINUX_ION_TEST_H

#include <linux/ioctl.h>
#include <linux/types.h>

struct ion_test_rw_data {
	__u64 ptr;
	__u64 offset;
	__u64 size;
	int write;
	int __padding;
};

#define ION_IOC_MAGIC		'I'

#define ION_IOC_TEST_SET_FD \
			_IO(ION_IOC_MAGIC, 0xf0)

#define ION_IOC_TEST_DMA_MAPPING \
			_IOW(ION_IOC_MAGIC, 0xf1, struct ion_test_rw_data)

#define ION_IOC_TEST_KERNEL_MAPPING \
			_IOW(ION_IOC_MAGIC, 0xf2, struct ion_test_rw_data)

#endif /* _UAPI_LINUX_ION_H */
