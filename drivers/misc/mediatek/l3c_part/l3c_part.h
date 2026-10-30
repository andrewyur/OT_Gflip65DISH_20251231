
#ifndef __L3C_PART_H__
#define __L3C_PART_H__

enum {
	MTK_L3C_PART_MCU,
	MTK_L3C_PART_ACP
};

int mtk_l3c_set_mcu_part(unsigned int ratio);
int mtk_l3c_set_acp_part(unsigned int ratio);
int mtk_l3c_get_part(unsigned int id);

#endif /* __L3C_PART_H__ */

