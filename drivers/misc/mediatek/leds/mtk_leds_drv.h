
#include <linux/leds.h>
#include <mtk_leds_hal.h>


#ifdef CONTROL_BL_TEMPERATURE
int setMaxbrightness(int max_level, int enable);
#endif

extern int mt65xx_leds_brightness_set(enum mt65xx_led_type type,
		enum led_brightness level);
#ifdef CONFIG_MTK_LEDS
extern int backlight_brightness_set(int level);
#else
#define backlight_brightness_set(level) do { } while (0)
#endif
extern int disp_bls_set_max_backlight(unsigned int level);
extern void disp_aal_notify_backlight_changed(int bl_1024);
extern bool disp_aal_is_support(void);
extern int disp_bls_set_backlight(int level_1024);
extern int mtkfb_set_backlight_level(unsigned int level);
extern void disp_pq_notify_backlight_changed(int bl_1024);
extern int enable_met_backlight_tag(void);
extern int output_met_backlight_tag(int level);
