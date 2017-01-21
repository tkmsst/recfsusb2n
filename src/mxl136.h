/* SunPTV-USB   (c) 2016 trinity19683
  MXL136  TV tuner
  mxl136.h
  2016-01-27
*/
#pragma once

int mxl136_create(void ** const);
int mxl136_destroy(void * const);
void* mxl136_i2c_ptr(void * const);
int mxl136_init(void * const);
int mxl136_sleep(void * const);
int mxl136_wakeup(void * const);
int mxl136_setFreq(void * const, const unsigned  freq);

/*EOF*/