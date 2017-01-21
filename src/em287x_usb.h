/* SunPTV-USB   (c) 2016 trinity19683
  EM287x USB commands (Linux OSes)
  em287x_usb.h
  2016-01-22
*/
#pragma once
#include <stdint.h>

#define USB_TIMEOUT 400   //# USB ctrl timeout (msec)

int em287x_ctrl(const int fd, const uint16_t reg, const uint16_t size, uint8_t* const data, const unsigned mode);

/*EOF*/
