/* SunPTV-USB   (c) 2016 trinity19683
  MXL136  TV tuner
  mxl136.c
  2016-01-27
*/

#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <malloc.h>

#include "osdepend.h"
#include "mxl136.h"
#include "message.h"

#define ARRAY_SIZE(x)  (sizeof(x)/(sizeof(x[0])))

struct state_st {
	struct i2c_device_st  i2c_dev;
	/* [0] IF, [1] Xtal, [2] mode(ISDB-T, DVB-T, ATSC), [3] bandwidth */
	uint8_t params[4];
};


/* register  write */
static int  writeRegs(struct i2c_device_st* const s, const unsigned wlen, uint8_t* const data)
{
	int r = s->i2c_comm(s->dev, s->addr, wlen, data, 0, NULL);
	if( r )  warn_info(r,"failed");
	return r;
}

/* public function */

int mxl136_create(void ** const  state)
{
	struct state_st* st;
	if(NULL == *state) {
		st = malloc(sizeof(struct state_st));
		if(NULL == st) {
			warn_info(errno,"failed to allocate");
			return -1;
		}
		*state = st;
		st->i2c_dev.addr = 0;

		st->params[0] = 0x00;
		st->params[1] = 0x40;
		st->params[2] = 0x10;
		st->params[3] = 0x15;
	}else{
		st = *state;
	}
	return 0;
}

int mxl136_destroy(void * const state)
{
	int ret = 0, r;
	struct state_st* const s = state;
	if(! s) {
		warn_info(0,"null pointer");
		return -1;
	}

	//# sleep tuner
	if(( r = mxl136_sleep( s ) ))  {
		warn_info(r,"failed");
		ret = (ret << 8) | (r & 0xFF);
	}

	free(s);
	return ret;
}

void* mxl136_i2c_ptr(void * const state)
{
	if(! state)  return NULL;
	return &( ((struct state_st*)state)->i2c_dev );
}


int mxl136_init(void * const state)
{
	int ret;
	struct state_st* const s = state;
	struct i2c_device_st* const  dev = &(s->i2c_dev);
	uint8_t utmp;
	uint8_t init_data[] = {
 0x02,0x00, 0x03,0x40, 0x05,0x04, 0x06,0x10,
 0x2e,0x15, 0x30,0x10, 0x45,0x58, 0x48,0x19,
 0x52,0x03, 0x53,0x44, 0x6a,0x4b, 0x76,0x00,
 0x78,0x18, 0x7a,0x17, 0x85,0x06, 0x01,0x01 };

	//# soft reset
	utmp = 0xFF;
	if(( ret = writeRegs(dev, 1, &utmp ) ))  goto err1;
	/* [0] bit 3-0 IF  4 MHz : 0, 4.5 : 2, 4.57 : 3, 5 : 4, 5.38 : 5, 6 : 6,
	6.28 : 7, 9.1915 : 8, 35.25 : 9, 36.15 : A, 44 : B
	bit 4 normal : 00, invert : 10
	*/
	init_data[1] = s->params[0] & 0x1F;
	/* [1] bit 7-4 Xtal  16 MHz : 0, 20 : 10, 20.25 : 20, 20.48 : 30, 24 : 40, 25 : 50,
	25.14 : 60, 27 : 70, 28.8 : 80, 32 : 90, 40 : A0, 44 : B0, 48 : C0, 49.3811 : D0
	bit 3 Clock_out  on : 08, off : 00
	bit 2-0 Clock_out Ampl : [0, 7]
	*/
	init_data[3] = s->params[1];
	init_data[5] = (s->params[1] >> 4) & 0xF;
	//# bit 4-0 mode  ISDB-T : 10, DVB-T : 11, ATSC : 12
	init_data[7] = s->params[2] & 0x1F;
	if(( ret = writeRegs(dev, ARRAY_SIZE(init_data), init_data ) ))  goto err1;

	dmsg(" mxl136 init done!");
	return 0;
err1:
	warn_info(ret,"failed");
	return ret;
}

int mxl136_sleep(void * const state)
{
	int ret;
	struct i2c_device_st* const  dev = &( ((struct state_st*) state)->i2c_dev );
	static uint8_t reg_data[] = { 0x01,0, 0x0f,0 };

	if(( ret = writeRegs(dev, ARRAY_SIZE(reg_data), reg_data ) ))  { warn_info(ret,"failed");  return ret; }
	return 0;
}

int mxl136_wakeup(void * const state)
{
	int ret;
	struct i2c_device_st* const  dev = &( ((struct state_st*) state)->i2c_dev );
	static uint8_t reg_data[] = { 0x01,0x01 };

	if(( ret = writeRegs(dev, ARRAY_SIZE(reg_data), reg_data ) ))  { warn_info(ret,"failed");  return ret; }
	return 0;
}

static int  setFreqParam(struct state_st* const s, const unsigned  freq)
{
	int ret;
	struct i2c_device_st* const  dev = &(s->i2c_dev);
	unsigned dig_rf_freq;
	uint8_t reg_data[] = {
 0x0f,0x00, 0x0c,0x15,
 0x0d,0xc9, 0x0e,0x83,
 0x1f,0x87, 0x20,0x1f, 0x21,0x87, 0x22,0x1f,
 0x80,0x41, 0x0f,0x01 };

	//# bit 5-0 bandwidth  6 MHz : 15, 7 : 2A, 8 : 3F
	reg_data[3] = s->params[3] & 0x3F;
	//# freq (kHz)
	dig_rf_freq = (freq << 3) / 125;
	reg_data[5] = dig_rf_freq & 0xFF;
	reg_data[7] = (dig_rf_freq >> 8) & 0xFF;
	reg_data[17] = (freq < 333000) ? 0x01 : 0x41;
	if(( ret = writeRegs(dev, ARRAY_SIZE(reg_data), reg_data ) ))  { warn_info(ret,"failed");  goto err0; }

	return 0;
err0:
	return ret;
}

int mxl136_setFreq(void * const state, const unsigned  freq)
{
	int ret;
	struct state_st* const s = state;

	if(60000 > freq || 860000 <= freq) {
		warn_msg(0,"freq %u kHz is out of range.", freq);
		return -EINVAL;
	}

	ret = setFreqParam(s, freq);
	if( ret ) { warn_info(ret,"failed");  goto err1; }
	miliWait(3);

	return 0;
err1:
	return ret;
}


/*EOF*/