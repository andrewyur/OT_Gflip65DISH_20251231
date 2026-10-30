

#ifndef __MTU3_PRIV__
#define __MTU3_PRIV__

struct ssusb_priv {
	struct regulator *vusb10;
};

extern void usb_audio_req(bool on);
extern int mtu3_phy_init_debugfs(struct phy *phy);
extern int mtu3_phy_exit_debugfs(void);
extern void u3phywrite32(struct phy *phy, int offset, int mask, int value);
extern struct ssusb_mtk *get_ssusb(void);
extern void ssusb_switch_usbpll_div13(struct ssusb_mtk *ssusb, bool on);

#endif

