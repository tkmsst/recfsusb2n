/* SunPTV-USB   (c) 2016 trinity19683
  USB operations (Linux OSes)
  usbops.c
  2016-01-11
*/

#include <errno.h>
#include <sys/ioctl.h>
#include <linux/usbdevice_fs.h>
#include "usbops.h"
#include "message.h"

int usb_reset(HANDLE fd)
{
	if(ioctl(fd, USBDEVFS_RESET, 0) < 0) {
		warn_info(errno,"failed");
		return errno;
	}
	return 0;
}

int usb_claim(HANDLE fd, unsigned int interface)
{
	if(ioctl(fd, USBDEVFS_CLAIMINTERFACE, &interface) < 0) {
		if(errno == EBUSY) {
			//# BUSY
			warn_msg(0,"USB interface%u is busy", interface);
		}else{
			warn_info(errno,"failed");
		}
		return errno;
	}
	return 0;
}

int usb_release(HANDLE fd, unsigned int interface)
{
	if(ioctl(fd, USBDEVFS_RELEASEINTERFACE, &interface) < 0) {
		warn_info(errno,"failed");
		return errno;
	}
	return 0;
}

int usb_setconfiguration(HANDLE fd, unsigned int confignum)
{
	return ioctl(fd, USBDEVFS_SETCONFIGURATION, &confignum);
}

int usb_setinterface(HANDLE fd, const unsigned int interface, const unsigned int altsetting)
{
	struct usbdevfs_setinterface setintf;
	setintf.interface = interface;
	setintf.altsetting = altsetting;
	return ioctl(fd, USBDEVFS_SETINTERFACE, &setintf);
}

int usb_clearhalt(HANDLE fd, unsigned int endpoint)
{
	if(ioctl(fd, USBDEVFS_CLEAR_HALT, &endpoint) < 0) {
		warn_info(errno,"EP=%02X failed", endpoint);
		return -errno;
	}
	return 0;
}

/*EOF*/
