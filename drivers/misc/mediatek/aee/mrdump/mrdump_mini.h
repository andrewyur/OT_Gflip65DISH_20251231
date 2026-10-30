
#if !defined(__MRDUMP_MINI_H__)
struct mrdump_mini_extra_misc {
	void (*dump_func)(unsigned long *vaddr, unsigned long *size);
	const char *dump_name;
	unsigned long max_size;
};
void mrdump_mini_add_extra_misc(void);
void mrdump_mini_add_hang_raw(unsigned long vaddr, unsigned long size);
int mrdump_mini_init(void);
extern raw_spinlock_t logbuf_lock;
extern unsigned long *stack_trace;
extern void get_kernel_log_buffer(unsigned long *addr, unsigned long *size,
		unsigned long *start);
extern void get_hang_detect_buffer(unsigned long *addr, unsigned long *size,
		unsigned long *start);
extern void get_gz_log_buffer(unsigned long *addr, unsigned long *paddr,
			unsigned long *size, unsigned long *start);
extern struct ram_console_buffer *ram_console_buffer;
#endif
