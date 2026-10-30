

#ifndef _AUDDRV_GPIO_H_
#define _AUDDRV_GPIO_H_


#include "mtk-auddrv-def.h"
#include "mtk-soc-digital-type.h"



#include <linux/gpio.h>

void AudDrv_GPIO_probe(void *dev);

int AudDrv_GPIO_Request(bool _enable, enum soc_aud_digital_block _usage);

int AudDrv_GPIO_SMARTPA_Select(int mode);
int AudDrv_GPIO_TDM_Select(int mode);

int AudDrv_GPIO_I2S_Select(int bEnable);
int AudDrv_GPIO_EXTAMP_Select(int bEnable, int mode);
int AudDrv_GPIO_EXTAMP2_Select(int bEnable, int mode);
int AudDrv_GPIO_RCVSPK_Select(int bEnable);
int AudDrv_GPIO_HPDEPOP_Select(int bEnable);

int audio_drv_gpio_aud_clk_pull(bool high);

/* Begin modify by wanying.chen 20200104 */
int AudDrv_GPIO_HAC_PA_Select(int bEnable);
/* End   modify by wanying.chen 20200104 */
#endif
