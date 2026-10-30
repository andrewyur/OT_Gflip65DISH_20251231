
extern void mt_usbhost_connect(void);
extern void mt_usbhost_disconnect(void);
extern void mt_vbus_on(void);
extern void mt_vbus_off(void);
extern int usb_otg_set_vbus(int is_on);
extern void mt_usb_connect(void);
extern void mt_usb_disconnect(void);
#ifndef CONFIG_TCPC_CLASS
#ifdef CONFIG_DUAL_ROLE_USB_INTF
extern void mt_usb_dual_role_to_none(void);
extern void mt_usb_dual_role_to_device(void);
extern void mt_usb_dual_role_to_host(void);
extern int mt_usb_dual_role_init(struct device *dev);
#endif
#endif
