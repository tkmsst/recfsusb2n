/* fsusb2i   (c) 2015 trinity19683
  USB device file
  usbdevfile.h
  2015-12-09
*/
#pragma once
#include "types_u.h"

HANDLE usbdevfile_alloc(int (*check_func)(const unsigned int*), char** const pdevfile);

/*EOF*/
