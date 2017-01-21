/* SunPTV-USB   (c) 2016 trinity19683
  USB operations
  usbops.h
  2016-01-11
*/
#pragma once
#include "types_u.h"

int usb_reset(HANDLE);
int usb_claim(HANDLE, unsigned int interface);
int usb_release(HANDLE, unsigned int interface);
int usb_setconfiguration(HANDLE, unsigned int confignum);
int usb_setinterface(HANDLE, const unsigned int interface, const unsigned int altsetting);
int usb_clearhalt(HANDLE, unsigned int endpoint);

/*EOF*/
