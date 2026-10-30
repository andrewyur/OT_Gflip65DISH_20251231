#ifndef MTK_TS_WMT_H
#define MTK_TS_WMT_H
typedef int (*wcn_bridge_thermal_query_cb)(void);

extern int tswmt_get_WiFi_tx_tput(void);
struct wcn_platform_bridge {
	wcn_bridge_thermal_query_cb thermal_query_cb;
};

void wcn_export_platform_bridge_register(struct wcn_platform_bridge *cb);
void wcn_export_platform_bridge_unregister(void);

#endif
