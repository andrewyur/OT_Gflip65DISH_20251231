/* SPDX-License-Identifier: GPL-2.0 */

#ifndef _MT6873_AFE_GPIO_H_
#define _MT6873_AFE_GPIO_H_

struct mtk_base_afe;

int mt6873_afe_gpio_init(struct mtk_base_afe *afe);

int mt6873_afe_gpio_request(struct mtk_base_afe *afe, bool enable,
			    int dai, int uplink);

#endif
