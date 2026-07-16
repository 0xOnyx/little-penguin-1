#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/usb.h>
#include <linux/hid.h>

static const struct usb_device_id my_usb_tbale[] = {
	{
		USB_INTERFACE_INFO(
			USB_INTERFACE_CLASS_HID,
			USB_INTERFACE_SUBCLASS_BOOT,
			USB_INTERFACE_PROTOCOL_KEYBOARD
		)
	},
	{}
};

MODULE_DEVICE_TABLE(usb, my_usb_tbale);

static int __init my_init(void)
{
	pr_info("Keyboard plugged in!\n");
	return 0;
}

static void __exit my_exit(void)
{
	pr_info("Cleaning up module.\n");
}

module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("jerdos-s");
MODULE_DESCRIPTION("USB keyboard hotplug module");