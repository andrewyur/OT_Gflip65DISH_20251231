
#ifndef _MT2701_AFE_CLOCK_CTRL_H_
#define _MT2701_AFE_CLOCK_CTRL_H_

struct mtk_base_afe;

int mt2701_init_clock(struct mtk_base_afe *afe);
int mt2701_afe_enable_clock(struct mtk_base_afe *afe);
void mt2701_afe_disable_clock(struct mtk_base_afe *afe);

int mt2701_turn_on_a1sys_clock(struct mtk_base_afe *afe);
void mt2701_turn_off_a1sys_clock(struct mtk_base_afe *afe);

int mt2701_turn_on_a2sys_clock(struct mtk_base_afe *afe);
void mt2701_turn_off_a2sys_clock(struct mtk_base_afe *afe);

int mt2701_turn_on_afe_clock(struct mtk_base_afe *afe);
void mt2701_turn_off_afe_clock(struct mtk_base_afe *afe);

void mt2701_mclk_configuration(struct mtk_base_afe *afe, int id, int domain,
			       int mclk);

#endif
