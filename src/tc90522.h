/* SunPTV-USB   (c) 2016 trinity19683
  TC90522, 90532 demodulator
  tc90522.h
  2016-02-11
*/
#pragma once

int tc90522_create(void ** const);
int tc90522_destroy(void * const);
void tc90522_attach(void * const, const unsigned devnum, struct i2c_device_st* const);
void* tc90522_i2c_ptr(void * const);
int tc90522_init(void * const);
int tc90522_selectDevice(void * const, const unsigned devnum);
int tc90522_powerControl(void * const, const unsigned devnum, const int isWake);
int tc90522_resetDemod(void * const, const unsigned devnum);
int tc90522_selectStream(void * const, const unsigned devnum, const unsigned tsSel);
int tc90522_readTMCC(void * const, const unsigned devnum, void* const pData);
int tc90522_readStatistic(void * const, const unsigned devnum, unsigned* const data);

/*EOF*/