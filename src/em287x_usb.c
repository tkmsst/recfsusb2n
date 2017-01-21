/* SunPTV-USB   (c) 2016 trinity19683
  EM287x USB commands (Linux OSes)
  em287x_usb.c
  2016-01-22
*/

#include <stddef.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <linux/usb/ch9.h>
#include <linux/usbdevice_fs.h>

#include "osdepend.h"
#include "em287x_usb.h"
#include "em287x_priv.h"
#include "message.h"

int em287x_ctrl(const int fd, const uint16_t reg, const uint16_t size, uint8_t* const data, const unsigned mode)
{
	int ret;
	struct usbdevfs_ctrltransfer  usb_cmd = { USB_TYPE_VENDOR|USB_RECIP_DEVICE, 0, 0, reg, size, USB_TIMEOUT, data};
	usb_cmd.bRequestType |= (mode & 0x1) ? USB_DIR_OUT : USB_DIR_IN;
	usb_cmd.bRequest = (mode >> 8) & 0xFF;
	if(ioctl(fd, USBDEVFS_CONTROL, &usb_cmd) < 0) {
		ret = errno;
		warn_info(ret,"%02X_%02X", (mode >> 8) & 0xFF, reg);
		return -ret;
	}
	return 0;
}

/*EOF*/
