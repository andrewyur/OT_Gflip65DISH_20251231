
#ifndef __VPU_IOCTL_H__
#define __VPU_IOCTL_H__

#define VPU_MAX_NUM_PORTS 32
#define VPU_MAX_NUM_PROPS 32
#define VPU_MAX_NUM_CORES 3

#define ALGO_NAMELEN 32
#define VPU_FW_MAGIC      0x4D544B56

typedef uint8_t vpu_id_t;





/*---------------------------------------------------------------------------*/
/*  VPU Property                                                             */
/*---------------------------------------------------------------------------*/
enum vpu_prop_type {
	VPU_PROP_TYPE_CHAR,
	VPU_PROP_TYPE_INT32,
	VPU_PROP_TYPE_INT64,
	VPU_PROP_TYPE_FLOAT,
	VPU_PROP_TYPE_DOUBLE,
	VPU_NUM_PROP_TYPES
};

enum vpu_prop_access {
	VPU_PROP_ACCESS_RDONLY,
	VPU_PROP_ACCESS_RDWR
};

struct vpu_prop_desc {
	vpu_id_t id;
	uint8_t type;      /* the property's data type */
	uint8_t access;    /* directional data exchange */
	uint32_t offset;   /* offset = previous offset + previous size */
	uint32_t count;    /* size = sizeof(type) x count */
	char name[ALGO_NAMELEN];
};

/*---------------------------------------------------------------------------*/
/*  VPU Ports                                                                */
/*---------------------------------------------------------------------------*/
enum vpu_port_usage {
	VPU_PORT_USAGE_IMAGE,
	VPU_PORT_USAGE_DATA,
	VPU_NUM_PORT_USAGES
};

enum vpu_port_dir {
	VPU_PORT_DIR_IN,
	VPU_PORT_DIR_OUT,
	VPU_PORT_DIR_IN_OUT,
	VPU_NUM_PORT_DIRS
};

struct vpu_port {
	vpu_id_t id;
	uint8_t usage;
	uint8_t dir;
	char name[ALGO_NAMELEN];
};

struct vpu_prop {
	uint32_t desc_cnt;
	uint32_t length;
	uint64_t ptr;    /* pointer to data buffer */
	struct vpu_prop_desc desc[VPU_MAX_NUM_PROPS];
};

/*---------------------------------------------------------------------------*/
/*  VPU Algo                                                                 */
/*---------------------------------------------------------------------------*/
struct vpu_algo {
	char name[ALGO_NAMELEN];
	uint8_t port_count;
	struct vpu_port ports[VPU_MAX_NUM_PORTS];
	struct vpu_prop info;  /* information */
	struct vpu_prop sett;  /* setting */

	uint32_t len;     /* binary length */
	uint64_t mva;     /* mapped mva address to the binary */

	/* preload algo */
	uint32_t entry_off;  /* algo entry offset */
	uint32_t iram_len;   /* iram data length */
	uint64_t iram_mva;   /* iram data iova */
};

/*---------------------------------------------------------------------------*/
/*  VPU Register                                                             */
/*---------------------------------------------------------------------------*/
struct vpu_reg_value {
	uint32_t field;
	uint32_t value;
};

struct vpu_reg_values {
	uint8_t reg_count;
	struct vpu_reg_value *regs;
};

/*---------------------------------------------------------------------------*/
/*  VPU Power                                                                */
/*---------------------------------------------------------------------------*/

enum vpu_power_mode {
	VPU_POWER_MODE_DYNAMIC,
	VPU_POWER_MODE_ON,
};

enum vpu_power_opp {
	VPU_POWER_OPP_UNREQUEST = 0xFF,
};

struct vpu_power {
	uint8_t opp_step;
	uint8_t freq_step;
	uint32_t bw; /* unit: MByte/s */

	/* align with core index defined in user space header file */
	unsigned int core;
	uint8_t boost_value;
};

/*---------------------------------------------------------------------------*/
/*  VPU Plane                                                                */
/*---------------------------------------------------------------------------*/
struct vpu_plane {
	uint32_t stride;         /* if buffer type is image */
	uint32_t length;
	uint64_t ptr;            /* mva which is accessible by VPU */
};

enum vpu_buf_format {
	VPU_BUF_FORMAT_DATA,
	VPU_BUF_FORMAT_IMG_Y8,
	VPU_BUF_FORMAT_IMG_YV12,
	VPU_BUF_FORMAT_IMG_NV21,
	VPU_BUF_FORMAT_IMG_YUY2,
	VPU_BUF_FORMAT_IMPL_DEFINED = 0xFF,
};

struct vpu_buffer {
	vpu_id_t port_id;
	uint8_t format;
	uint8_t plane_count;
	uint32_t width;
	uint32_t height;
	struct vpu_plane planes[3];
};

enum vpu_req_status {
	VPU_REQ_STATUS_SUCCESS,
	VPU_REQ_STATUS_BUSY,
	VPU_REQ_STATUS_TIMEOUT,
	VPU_REQ_STATUS_INVALID,
	VPU_REQ_STATUS_FLUSH,
	VPU_REQ_STATUS_FAILURE,
};

#define VPU_REQ_FLAG_TST(req, flag) \
	((req)->flags & VPU_REQ_F_##flag)
#define VPU_REQ_FLAG_SET(req, flag) \
	((req)->flags |= (VPU_REQ_F_##flag))
#define VPU_REQ_FLAG_CLR(req, flag) \
	((req)->flags &= (~VPU_REQ_F_##flag))

#define VPU_REQ_F_ALG_RELOAD 0x1LL  /* reload algo */
#define VPU_REQ_F_ALG_CUSTOM 0x2LL  /* use the given algo at priv */
#define VPU_REQ_F_ALG_PRELOAD 0x4LL  /* using preloaded algo */
#define VPU_REQ_F_PREEMPT_TEST 0x8LL  /* preempt test mode */

/* 3 prioritys of req */
#define VPU_REQ_MAX_NUM_PRIORITY 21

struct vpu_request {
	uint32_t version;  /* command version and magic number */
	char algo[ALGO_NAMELEN];
	uint64_t flags;
	pid_t owner;
	uint8_t status;
	uint8_t buffer_count;
	uint32_t sett_length;
	uint64_t sett_ptr;       /* pointer to the request setting */
	uint64_t priv;           /* reserved for user */
	struct vpu_buffer buffers[VPU_MAX_NUM_PORTS];
	/* driver usage only, fd in user space / ion handle in kernel */
	uint64_t buf_ion_infos[VPU_MAX_NUM_PORTS * 3];
	struct vpu_power power_param;
	uint64_t busy_time;      /* ns */
	int prio;  /* exection priority assigned by driver */
	uint32_t algo_ret;       /* algorithm return value */
};

struct vpu_status {
	int vpu_core_index;
	bool vpu_core_available;
	int pool_list_size;
};

struct vpu_dev_debug_info {
	int dev_fd;
	char callername[32];
	pid_t open_pid;
	pid_t open_tgid;
};

enum VPU_OPP_PRIORIYY {
	DEBUG = 0,
	THERMAL = 1,
	POWER_HAL = 2,
	EARA_QOS = 3,
	NORMAL = 4,
	VPU_OPP_PRIORIYY_NUM
};

struct vpu_lock_power {
/* align with core index defined in user space header file*/
	unsigned int core;
	uint8_t max_boost_value;
	uint8_t min_boost_value;
	bool lock;
	enum VPU_OPP_PRIORIYY priority;
};

#ifdef CONFIG_MTK_GZ_SUPPORT_SDSP
extern int mtee_sdsp_enable(u32 on);
#endif

/*---------------------------------------------------------------------------*/
/*  VPU Customer Cmd                                                         */
/*---------------------------------------------------------------------------*/
enum vpu_user_cmd {
	VPU_UCMD_GET_ALGO = 0x8001,
	VPU_UCMD_GET_OPPTABLE,
	VPU_UCMD_MAX,
};

#define VPU_UCMD_CHECK(va, flag) \
	(*(uint32_t *)(va) == VPU_UCMD_##flag)
#define VPU_UCMD_SET(va, flag) \
	(*(uint32_t *)(va) = VPU_UCMD_##flag)

struct vpu_uget_algo {
	enum vpu_user_cmd cmd;
	char name[ALGO_NAMELEN];
};

#endif
