
#ifndef MTK_ADSP_IPI_QUEUE_H
#define MTK_ADSP_IPI_QUEUE_H

#include <linux/types.h>

#define ADSP_IPI_QUEUE_DEFAULT_WAIT_MS (20)

typedef int (*send_handler_t)(
	int id, void *data, unsigned int len
);

int scp_ipi_queue_init(uint32_t opendsp_id);

int scp_ipi_queue_init_ex(uint32_t opendsp_id, send_handler_t send_handler);

int scp_flush_msg_queue(uint32_t opendsp_id);

int scp_send_msg_to_queue(
	uint32_t opendsp_id, /* enum opendsp_id */
	uint32_t ipi_id, /* enum adsp_ipi_id */
	void *buf,
	uint32_t len,
	uint32_t wait_ms);


int scp_dispatch_ipi_hanlder_to_queue(
	uint32_t opendsp_id, /* enum opendsp_id */
	uint32_t ipi_id, /* enum adsp_ipi_id */
	void *buf,
	uint32_t len,
	void (*ipi_handler)(int ipi_id, void *buf, unsigned int len));


#endif /* end of MTK_ADSP_IPI_QUEUE_H */



