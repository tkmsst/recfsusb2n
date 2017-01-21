/* SunPTV-USB   (c) 2016 trinity19683
  TDA20142  BS tuner
  tda20142.h
  2016-01-21
*/
#pragma once

int tda20142_create(void ** const);
int tda20142_destroy(void * const);
void* tda20142_i2c_ptr(void * const);
int tda20142_init(void * const);
int tda20142_setFreq(void * const, const unsigned  freq);

/*EOF*/