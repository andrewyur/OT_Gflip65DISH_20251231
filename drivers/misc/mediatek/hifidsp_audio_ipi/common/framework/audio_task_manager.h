
#ifndef AUDIO_TASK_MANAGER_H
#define AUDIO_TASK_MANAGER_H

#include <linux/types.h>

#include <audio_task.h>
#include <audio_messenger_ipi.h>
#include <linux/device.h>



typedef void (*task_unloaded_t)(void);




void audio_task_manager_init(void);


void audio_task_manager_deinit(void);


int audio_task_register_callback(
	const uint8_t task_scene,
	recv_message_t recv_message,
	task_unloaded_t task_unloaded_callback);


int audio_load_task(const uint8_t task_scene);

int audio_task_init_ipi_dma(phys_addr_t paddr,
			    void __iomem *vaddr,
			    u64 size);

#endif /* end of AUDIO_TASK_MANAGER_H */



