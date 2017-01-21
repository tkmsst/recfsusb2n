/* SunPTV-USB   (c) 2016 trinity19683
  EM287x USB interface
  em287x.h
  2016-01-18
*/
#pragma once
#include <stdint.h>
#include "types_u.h"

typedef void* em287x_state;

int em287x_create(em287x_state* const, struct usb_endpoint_st * const);
int em287x_destroy(const em287x_state);
void em287x_attach(const em287x_state, struct i2c_device_st* const);
int em287x_startstopStream(const em287x_state, const int start);

/*EOF*/