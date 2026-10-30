#ifndef MTK_GADGET_H
#define MTK_GADGET_H

#ifdef CONFIG_USB_CONFIGFS_UEVENT
extern char *serial_string;
extern int serial_idx;
#endif
extern void composite_setup_complete(struct usb_ep *ep,
		struct usb_request *req);
extern int acm_shortcut(void);

#endif
