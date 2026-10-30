#ifndef __MTK_MBOX_H__
#define __MTK_MBOX_H__

#include <linux/platform_device.h>
#include <linux/completion.h>
#include <linux/mutex.h>
#include <linux/spinlock.h>

#define MBOX_SLOT_SIZE 4

struct mtk_mbox_pin_send;
struct mtk_mbox_pin_recv;

typedef int (*mbox_rx_cb_t)(void *);
typedef int (*mbox_pin_cb_t)(unsigned int ipi_id, void *prdata, void *data,
	unsigned int len);
typedef void (*mbox_ipi_cb_t)(struct mtk_mbox_pin_recv *, void *);

struct mtk_mbox_recv_record {
	uint32_t poll_count;
	uint32_t recv_irq_count;
	uint32_t notify_count;
	uint32_t cb_count;
	uint64_t pre_timestamp;
	uint64_t post_timestamp;
};

struct mtk_mbox_send_record {
};

struct mtk_mbox_record {
	uint32_t write_count;
	uint32_t busy_count;
	uint32_t trig_irq_count;
};

struct mtk_mbox_pin_send {
	unsigned int mbox     : 4,
				 offset   : 20,
				 send_opt : 2,
				 lock    : 2;
	unsigned int msg_size;
	unsigned int pin_index;
	unsigned int chan_id;
	struct mutex mutex_send;
	struct completion comp_ack;  // to remove
	spinlock_t pin_lock;
	struct mtk_mbox_send_record send_record;
};

struct mtk_mbox_pin_recv {
	unsigned int mbox     : 4,
				 offset   : 20,
				 recv_opt     : 2,
				 lock         : 2,
				 buf_full_opt : 2,
				 cb_ctx_opt   : 2;
	unsigned int msg_size;
	unsigned int pin_index;
	unsigned int chan_id;
	struct completion notify;
	mbox_pin_cb_t mbox_pin_cb;
	void *pin_buf;
	void *prdata;
	spinlock_t pin_lock;
	struct mtk_mbox_recv_record recv_record;
};


struct mtk_mbox_device {
	/* Identity of the device */
	const char *name;
	unsigned int id;
	/* Following are platform specific interface*/
	struct mtk_mbox_pin_recv *pin_recv_table;
	struct mtk_mbox_pin_send *pin_send_table;
	struct mtk_mbox_info *info_table;
	unsigned int count;
	unsigned int recv_count;
	unsigned int send_count;
	void (*memcpy_to_tiny)(void __iomem *dest, const void *src, int size);
	void (*memcpy_from_tiny)(void *dest, const void __iomem *src, int size);
	mbox_ipi_cb_t ipi_cb;
	void *ipi_priv;
	mbox_rx_cb_t pre_cb;
	mbox_rx_cb_t post_cb;
	void *prdata;
	bool log_enable;
};

enum MBOX_PIN_CTX {
	MBOX_CB_IN_ISR   = 0,
	MBOX_CB_IN_PROCESS  = 1,
};

enum MBOX_BUF_OPT {
	MBOX_BUF_FULL_DROP       = 0,
	MBOX_BUF_FULL_ASSERT     = 1,
	MBOX_BUF_FULL_OVERWRITE  = 2,
	MBOX_BUF_COPY_DONE       = 3,
	MBOX_BUF_FULL_RET        = 4,
};

enum MBOX_RECV_OPT {
	MBOX_RECV_MESSAGE  = 0,
	MBOX_RECV_ACK      = 1,
};

struct mtk_mbox_info {
	struct mtk_mbox_device *mbdev;
	int irq_num;
	unsigned int id;
	unsigned int slot;
	unsigned int opt;
	bool enable;
	bool is64d;
	void __iomem *base;
	void __iomem *set_irq_reg;
	void __iomem *clr_irq_reg;
	void __iomem *init_base_reg;
	void __iomem *send_status_reg;
	void __iomem *recv_status_reg;
	spinlock_t mbox_lock;
	struct mtk_mbox_record record;
};

enum MBOX_RETURN {
	MBOX_READ_SZ_ERR  = -6,
	MBOX_WRITE_SZ_ERR = -5,
	MBOX_PARA_ERR     = -4,
	MBOX_CONFIG_ERR   = -3,
	MBOX_IRQ_ERR      = -2,
	MBOX_PLT_ERR      = -1,
	MBOX_DONE         = 0,
	MBOX_PIN_BUSY     = 1,
};

enum {
	MBOX_OPT_DIRECT = 0,
	MBOX_OPT_SMEM   = 1,
	MBOX_OPT_QUEUE_DIR  = 2,
	MBOX_OPT_QUEUE_SMEM  = 3,
};

struct mtk_ipi_msg_hd {
	uint32_t id;
	uint32_t len;
	uint32_t options;
	uint32_t reserved;
};

struct mtk_ipi_msg {
	struct mtk_ipi_msg_hd ipihd;
	void *data;
};

int mtk_mbox_write_hd(struct mtk_mbox_device *mbdev, unsigned int mbox,
		unsigned int slot, void *msg);
int mtk_mbox_read_hd(struct mtk_mbox_device *mbdev, unsigned int mbox,
		unsigned int slot, void *dest);
int mtk_mbox_write(struct mtk_mbox_device *mbdev, unsigned int mbox,
		unsigned int slot, void *data, unsigned int len);
int mtk_mbox_read(struct mtk_mbox_device *mbdev, unsigned int mbox,
		unsigned int slot, void *data, unsigned int len);
int mtk_mbox_clr_irq(struct mtk_mbox_device *mbdev, unsigned int mbox,
		unsigned int irq);
int mtk_mbox_trigger_irq(struct mtk_mbox_device *mbdev, unsigned int mbox,
		unsigned int irq);
unsigned int mtk_mbox_read_recv_irq(struct mtk_mbox_device *mbdev,
		unsigned int mbox);
int mtk_mbox_set_base_addr(struct mtk_mbox_device *mbdev, unsigned int mbox,
		unsigned int addr);
unsigned int mtk_mbox_check_send_irq(struct mtk_mbox_device *mbdev,
		unsigned int mbox, unsigned int pin_index);
int mtk_mbox_probe(struct platform_device *pdev, struct mtk_mbox_device *mbdev,
		unsigned int mbox);
int mtk_mbox_cb_register(struct mtk_mbox_device *mbdev, unsigned int pin_offset,
		mbox_pin_cb_t mbox_pin_cb, void *prdata);
int mtk_mbox_polling(struct mtk_mbox_device *mbdev, unsigned int mbox,
		void *data, struct mtk_mbox_pin_recv *pin_recv);
int mtk_smem_init(struct platform_device *pdev, struct mtk_mbox_device *mbdev,
		unsigned int mbox, void __iomem *base,
		void __iomem *set_irq_reg, void __iomem *clr_irq_reg,
		void __iomem *send_status_reg, void __iomem *recv_status_reg);
void mtk_mbox_dump_all(struct mtk_mbox_device *mbdev);
void mtk_mbox_dump_recv(struct mtk_mbox_device *mbdev, unsigned int pin);
void mtk_mbox_dump_recv_pin(struct mtk_mbox_device *mbdev,
	struct mtk_mbox_pin_recv *pin_recv);
void mtk_mbox_dump_send(struct mtk_mbox_device *mbdev, unsigned int pin);
void mtk_mbox_dump(struct mtk_mbox_device *mbdev, unsigned int mbox);
int mtk_mbox_log_enable(struct mtk_mbox_device *mbdev, bool enable);
void mtk_mbox_reset_record(struct mtk_mbox_device *mbdev);
#endif

