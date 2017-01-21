/* SunPTV-USB   (c) 2016 trinity19683
  TC90522, 90532 demodulator
  tc90522.c
  2016-02-11
*/

#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <malloc.h>

#include "osdepend.h"
#include "tc90522.h"
#include "message.h"

#define ARRAY_SIZE(x)  (sizeof(x)/(sizeof(x[0])))


struct state_st {
	struct i2c_device_st  i2c_dev[4];    //# 0x*0:T1, 0x*2:S1, 0x*4:T2, 0x*6:S2
};


/* register  read */
static int  readReg(struct i2c_device_st* const s, const unsigned reg, uint8_t* const val)
{
	uint8_t utmp[1];
	utmp[0] = reg & 0xFF;
	return s->i2c_comm(s->dev, s->addr, 1, utmp, 1, val );
}

/* register  masked write */
static int  writeReg(struct i2c_device_st* const s, const unsigned reg, uint8_t const val, uint8_t const mask)
{
	int r;
	uint8_t utmp[2], rtmp;

	if(0 == mask || 0xFF == mask) {
		utmp[1] = val;
		goto write_reg;
	}
	utmp[0] = reg & 0xFF;
	if(( r = s->i2c_comm(s->dev, s->addr, 1, utmp, 1, &rtmp ) ))
		goto err1;

	utmp[1] = (val & mask) | (rtmp & ~mask);
write_reg:
	utmp[0] = reg & 0xFF;
	r = s->i2c_comm(s->dev, s->addr, 2, utmp, 0, NULL);
err1:
	return r;
}


/* I2C  write & read */
static int  tc90522_I2C(struct i2c_device_st* const s, const unsigned addr, const unsigned wlen, uint8_t* const wdata, const unsigned rlen, uint8_t* const rdata )
{
	int r;
	uint8_t utmp[48];
	if(2 + wlen >= sizeof(utmp)) {
		warn_info(0,"buffer overflow");
		return -EINVAL;
	}

	if(wlen && wdata) {
		//# write
		utmp[0] = 0xFE;  utmp[1] = addr & 0xFE;
		memcpy( &(utmp[2]), wdata, wlen);
		r = s->i2c_comm(s->dev, s->addr, 2 + wlen, utmp, 0, NULL );
		if( r ) {
			warn_info(r,"i2c:%02X failed", addr & 0xFF);  goto err1;
		}
	}
	if(rlen && rdata) {
		//# read
		utmp[0] = 0xFE;  utmp[1] = (addr & 0xFE) | 0x1;
		r = s->i2c_comm(s->dev, s->addr, 2, utmp, rlen, rdata );
		if( r ) {
			if(addr & 0x8000) {    //# obtain error code  mode
				goto err1;
			}
			warn_info(r,"i2c:%02X failed", addr & 0xFF);  goto err1;
		}
	}

err1:
	return r;
}


/* public function */


int tc90522_create(void ** const  state)
{
	struct state_st* st;
	if(NULL == *state) {
		st = malloc(sizeof(struct state_st));
		if(NULL == st) {
			warn_info(errno,"failed to allocate");
			return -1;
		}
		*state = st;
		st->i2c_dev[0].addr = 0;
	}else{
		st = *state;
	}
	return 0;
}

int tc90522_destroy(void * const state)
{
	int r, ret = 0, i;
	struct state_st* const s = state;
	if(! s) {
		warn_info(0,"null pointer");
		return -1;
	}

	//# sleep demod
	for(i = 0; i < 4; i ++) {
		if(! s->i2c_dev[i].addr)  break;
		if(( r = tc90522_powerControl(s, i, 0) ))  {
			warn_info(r,"failed");
			ret = (ret << 8) | (r & 0xFF);
		}
	}

	free(s);
	return ret;
}

void tc90522_attach(void * const state, const unsigned devnum, struct i2c_device_st* const  i2c_dev)
{
	struct i2c_device_st* const  pHostDev = &( ((struct state_st*)state)->i2c_dev[devnum] );
	if( (! i2c_dev) || (! pHostDev->addr) )  return;
	i2c_dev->dev = pHostDev;
	i2c_dev->i2c_comm = (void*)tc90522_I2C;
}

void* tc90522_i2c_ptr(void * const state)
{
	if(! state)  return NULL;
	return &( ((struct state_st*)state)->i2c_dev[0] );
}

/* initialize device */
int tc90522_init(void * const state)
{
	int ret, i, j;
	struct state_st* const s = state;
	uint8_t utmp[4];

	if(! s->i2c_dev[0].addr) {
		warn_info(0,"null pointer");
		return -1;
	}

	for(i = 1; i < 4; i ++ ) {
		s->i2c_dev[i] = s->i2c_dev[0];
		s->i2c_dev[i].addr += i << 1;
	}
	for(i = 2; i < 4; i ++ ) {   //# find 2nd demod
		ret = s->i2c_dev[i].i2c_comm(s->i2c_dev[i].dev, s->i2c_dev[i].addr | 0x8000, 1, NULL, 1, utmp);
		if( ret ) {
			s->i2c_dev[i].addr = 0;
			dmsgn("Not found demod #%u, ",i);
		}
	}

	//# demod chip, system reset
	for(i = 0; i < 4; i += 2) {
		if(! s->i2c_dev[i].addr)  continue;
		if(( ret = writeReg(&(s->i2c_dev[i]), 0x01, 0x80, 0) ))  goto err1;
	}
	for(i = 1; i < 4; i += 2) {
		if(! s->i2c_dev[i].addr)  continue;
		if(( ret = writeReg(&(s->i2c_dev[i]), 0x01, 0x90, 0) ))  goto err1;
	}
	miliWait(10);
	//# demod reset
	for(i = 1; i < 4; i += 2) {
		const uint8_t reset_sat[] = {
 /*0x13, 0x00,  0x15, 0x00,  0x17, 0x00,  0x1c, 0x00,  0x1d, 0x00,  0x1f, 0x00,*/
 0x0a, 0x00,  0x10, 0x00,  0x11, 0x00,  0x03, 0x01,
 /*0x51, 0xb0,  0x52, 0x89,  0x53, 0xb3,  0x5a, 0x2d,  0x5b, 0xd3,*/ };
		if(! s->i2c_dev[i].addr)  continue;
		for(j = 0; j < ARRAY_SIZE(reset_sat); j += 2) {
			ret = writeReg(&(s->i2c_dev[i]), reset_sat[j], reset_sat[j + 1], 0);
			if( ret )  goto err1;
		}
	}
	//# demod reset
	for(i = 0; i < 4; i += 2) {
		const uint8_t reset_ter[] = {
 0xb0, 0xa0,  0xb2, 0x3d,
 /*0x03, 0x00,  0x1d, 0x00,  0x1f, 0x30,*/
};
		if(! s->i2c_dev[i].addr)  continue;
		for(j = 0; j < ARRAY_SIZE(reset_ter); j += 2) {
			ret = writeReg(&(s->i2c_dev[i]), reset_ter[j], reset_ter[j + 1], 0);
			if( ret )  goto err1;
		}
	}

	dmsg(" tc90522 init done!");
	return 0;
err1:
	warn_info(ret,"failed");
	return ret;
}

int tc90522_selectDevice(void * const state, const unsigned devnum)
{
	int ret;
	struct state_st* const s = state;
	struct i2c_device_st  *devT, *devS;

	devT = &( s->i2c_dev[devnum & 0xFE] );
	devS = &( s->i2c_dev[devnum | 0x01] );
	if(! devT->addr) { warn_info(0,"invalid device %uT", devnum);  return -2; }
	if(! devS->addr) { warn_info(0,"invalid device %uS", devnum);  return -2; }

	if(devnum & 1) {
		//# sate
		if(( ret = writeReg(devS, 0x01, 0x90, 0) ))  goto err1;
		if(( ret = writeReg(devS, 0x0a, 0x00, 0) ))  goto err1;
		if(( ret = writeReg(devS, 0x10, 0x00, 0) ))  goto err1;
		if(( ret = writeReg(devS, 0x11, 0x00, 0) ))  goto err1;
		if(( ret = writeReg(devS, 0x03, 0x01, 0) ))  goto err1;
		if(( ret = writeReg(devT, 0x1f, 0x20, 0) ))  goto err1;
		miliWait(15);
		if(( ret = writeReg(devT, 0x1f, 0x30, 0) ))  goto err1;
		miliWait(15);
		if(( ret = writeReg(devT, 0x1f, 0x20, 0) ))  goto err1;
		//# S set output mode
		if(( ret = writeReg(devT, 0x0e, 0x77, 0) ))  goto err1;
		if(( ret = writeReg(devT, 0x0f, 0x10, 0) ))  goto err1;
		//# T set sleep mode
		if(( ret = tc90522_powerControl(s, devnum - 1, 0) ))  goto err1;
		//# S set output mode
		if(( ret = writeReg(devS, 0x07, 0x01, 0) ))  goto err1;
		if(( ret = writeReg(devS, 0x08, 0x10, 0) ))  goto err1;
		if(( ret = writeReg(devS, 0x8e, 0x24, 0) ))  goto err1;
		//# S set wake-up mode
		if(( ret = tc90522_powerControl(s, devnum, 1) ))  goto err1;
	}else{
		//# terra
		if(( ret = writeReg(devT, 0x01, 0x50, 0) ))  goto err1;
		if(( ret = writeReg(devT, 0x47, 0x30, 0) ))  goto err1;
		if(( ret = writeReg(devT, 0x25, 0x00, 0) ))  goto err1;
		if(( ret = writeReg(devT, 0x20, 0x00, 0) ))  goto err1;
		if(( ret = writeReg(devT, 0x23, 0x4d, 0) ))  goto err1;
		if(( ret = writeReg(devT, 0x1f, 0x30, 0) ))  goto err1;
		miliWait(15);
		if(( ret = writeReg(devT, 0x1f, 0x20, 0) ))  goto err1;
		miliWait(15);
		if(( ret = writeReg(devT, 0x1f, 0x30, 0) ))  goto err1;
		//# S set sleep mode
		if(( ret = tc90522_powerControl(s, devnum + 1, 0) ))  goto err1;
		//# T set output mode
		if(( ret = writeReg(devT, 0x0e, 0x01, 0) ))  goto err1;
		if(( ret = writeReg(devT, 0x0f, 0x77, 0) ))  goto err1;
		//# T set output mode
		if(( ret = writeReg(devT, 0x71, 0x20, 0) ))  goto err1;
		//# T set wake-up mode
		if(( ret = tc90522_powerControl(s, devnum, 1) ))  goto err1;
	}

	return 0;
err1:
	warn_info(ret,"failed");
	return ret;
}


int tc90522_powerControl(void * const state, const unsigned devnum, const int isWake)
{
	int ret;
	struct state_st* const s = state;
	struct i2c_device_st* const  dev = &(s->i2c_dev[devnum]);

	if(! dev->addr) {
		warn_info(0,"invalid device %u", devnum);
		return -2;
	}
	if(devnum & 1) {  //# sate
		if(( ret = writeReg(dev, 0x13, (isWake) ? 0x00 : 0x80 , 0) ))  goto err1;
		if(( ret = writeReg(dev, 0x17, (isWake) ? 0x00 : 0xff , 0) ))  goto err1;
	}else{  //# terra
		if(( ret = writeReg(dev, 0x03, (isWake) ? 0x00 : 0xf0 , 0) ))  goto err1;
	}

	return 0;
err1:
	warn_info(ret,"failed");
	return ret;
}

int tc90522_resetDemod(void * const state, const unsigned devnum)
{
	int ret;
	struct state_st* const s = state;
	struct i2c_device_st* const  dev = &(s->i2c_dev[devnum]);

	if(! dev->addr) {
		warn_info(0,"invalid device %u", devnum);
		return -2;
	}
	if(devnum & 1) {  //# sate
		//# set AGC
		if(( ret = writeReg(dev, 0x0a, 0xFF, 0) ))  goto err1;
		if(( ret = writeReg(dev, 0x10, 0xb2, 0) ))  goto err1;
		if(( ret = writeReg(dev, 0x11, 0x00, 0) ))  goto err1;
		//# reset PSK(sate) demod
		if(( ret = writeReg(dev, 0x03, 0x01, 0) ))  goto err1;
		//# select TS ID
		if(( ret = writeReg(dev, 0x8f, 0x0, 0) ))  goto err1;
		if(( ret = writeReg(dev, 0x90, 0x0, 0) ))  goto err1;

	}else{  //# terra
		//# set AGC
		if(( ret = writeReg(dev, 0x23, 0x4c, 0) ))  goto err1;
		//# reset OFDM(terra) demod
		if(( ret = writeReg(dev, 0x01, 0x40, 0) ))  goto err1;
		//# select ignore layer (4:A, 2:B, 1:C)
		if(( ret = writeReg(dev, 0x71, 0x0, 0x07) ))  goto err1;
		//#
		if(( ret = writeReg(dev, 0x72, 0x24, 0) ))  goto err1;
		if(( ret = writeReg(dev, 0x75, 0x02, 0) ))  goto err1;
	}

	return 0;
err1:
	warn_info(ret,"failed");

	return ret;
}

static unsigned  get_ts_id(struct i2c_device_st* const  dev, const unsigned num)
{
	int r;
	uint8_t utmp;
	unsigned uval;
	//if(num >= 8)  return 0;
	if(( r = readReg(dev, 0xc3, &utmp ) ))  goto err1;
	if(utmp & 0x10) {  //# TMCC error
		goto err0;
	}
	if(( r = readReg(dev, 0xce + (num * 2), &utmp ) ))  goto err1;
	uval = utmp << 8;
	if(( r = readReg(dev, 0xcf + (num * 2), &utmp ) ))  goto err1;
	uval |= utmp & 0xFF;
	return uval;
err1:
	warn_info(r,"failed");
err0:
	return 0;
}

int tc90522_selectStream(void * const state, const unsigned devnum, const unsigned tsSel)
{
	int ret;
	struct state_st* const s = state;
	struct i2c_device_st* const  dev = &(s->i2c_dev[devnum]);

	if(! dev->addr) {
		warn_info(0,"invalid device %u", devnum);
		return -2;
	}
	if(devnum & 1) {  //# sate
		unsigned ts_id;
		//# select TS ID
		if(8 > tsSel) {
			ts_id = get_ts_id(dev, tsSel);
			if(0 == ts_id || 0xFFFF == ts_id) {  //# empty
				//warn_msg(0,"TS_ID(%u)= %04X invalid!", tsSel, ts_id);
				ret = 1;
				goto err0;
			}
		}else{
			ts_id = tsSel;
		}
		if(( ret = writeReg(dev, 0x8f, (ts_id >> 8) & 0xFF, 0) ))  goto err1;
		if(( ret = writeReg(dev, 0x90, ts_id & 0xFF, 0) ))  goto err1;

	}else{  //# terra
		//# select ignore layer (4:A, 2:B, 1:C)  e.g.(A+B= OneSeg + FullSeg  or  A= FullSeg)
		if(( ret = writeReg(dev, 0x71, tsSel & 0x07, 0x07) ))  goto err1;
	}

	return 0;
err1:
	warn_info(ret,"failed");
err0:
	return ret;
}

/* Transmission and Multiplexing Configuration and Control (See ARIB STD-B20, STD-B31) */
int tc90522_readTMCC(void * const state, const unsigned devnum, void* const pData)
{
	int ret, j;
	struct state_st* const s = state;
	struct i2c_device_st* const  dev = &(s->i2c_dev[devnum]);
	uint8_t utmp, val, *ptr = pData;
	uint32_t lval[6];

	if(! dev->addr) {
		warn_info(0,"invalid device %u", devnum);
		return -2;
	}
	if(devnum & 1) {  //# sate
		ptr[0] = 0;
		if(( ret = readReg(dev, 0xe6, &ptr[2] ) ))  goto err1;
		if(( ret = readReg(dev, 0xe7, &ptr[3] ) ))  goto err1;
		if(( ret = readReg(dev, 0xe8, &utmp ) ))  goto err1;
		lval[0] = (utmp >> 4) & 0x7;
		lval[1] = utmp & 0x7;
		for( j = 0; j < 2; j++ ) {
			if(( ret = readReg(dev, 0xe9 +j, &utmp ) ))  goto err1;
			ptr[j *4 +4] = utmp & 0x3F;
		}
		for( j = 0; j < 4; j++ ) {
			if(( ret = readReg(dev, 0xca +j, &utmp ) ))  goto err1;
			ptr[j *4 +12] = utmp & 0x3F;
		}
		for( j = 0; j < 2; j++ ) {
			if(( ret = readReg(dev, 0xc8 +j, &utmp ) ))  goto err1;
			lval[j *2 +2] = utmp >> 4;
			lval[j *2 +3] = utmp & 0xF;
		}
		//# layer * : num of slots, modulation, FEC, 0
		for( j = 0; j < 6; j++ ) {
			ptr += 4;
			if(0 == lval[j] || 0xF == lval[j]) {
				ptr[0] = 0;
				continue;
			}else if(1 == lval[j]) {
				ptr[1] = 4;
				ptr[2] = 0;
			}else if(6 >= lval[j]) {
				ptr[1] = 1;
				ptr[2] = lval[j] - 2;
			}else if(7 == lval[j]) {
				ptr[1] = 5;
				ptr[2] = 1;
			}else{
				ptr[1] = 6;
				ptr[2] = 5;
			}
		}
	}else{  //# terra
		uint8_t txmode;
		if(( ret = readReg(dev, 0xb0, &utmp ) ))  goto err1;
		//# TX mode, guard interval
		txmode = ((utmp >> 6) & 0x3) + 1;
		ptr[0] = txmode;
		val = (utmp >> 4) & 0x3;
		ptr[1] = 32 >> val;
		if(( ret = readReg(dev, 0xb2, &utmp ) ))  goto err1;
		//# Partial reception (OneSeg)
		ptr[2] = ((utmp & 0xc1) == 0x01) ? 1 : 0;
		//#
		if(( ret = readReg(dev, 0xb3, &utmp ) ))  goto err1;
		lval[0] = utmp << 5;
		if(( ret = readReg(dev, 0xb4, &utmp ) ))  goto err1;
		lval[0] |= (utmp >> 3) & 0x1F;
		lval[1] = (utmp & 0x07) << 10;
		if(( ret = readReg(dev, 0xb5, &utmp ) ))  goto err1;
		lval[1] |= utmp << 2;
		if(( ret = readReg(dev, 0xb6, &utmp ) ))  goto err1;
		lval[1] |= (utmp >> 6) & 0x03;
		lval[2] = (utmp & 0x3F) << 7;
		if(( ret = readReg(dev, 0xb7, &utmp ) ))  goto err1;
		lval[2] |= (utmp >> 1) & 0x7F;
		//# layer * : num of seg, modulation, FEC, interleave
		for( j = 0; j < 3; j++ ) {
			ptr += 4;
			val = lval[j] & 0x0F;
			if(0x0F == val || 0 == val) {
				ptr[0] = 0;
				continue;
			}
			ptr[0] = val;
			val = (lval[j] >> 10) & 0x7;
			if(val <= 3)  ptr[1] = val;
			else  ptr[1] = 6;
			ptr[2] = (lval[j] >> 7) & 0x7;
			val = (lval[j] >> 4) & 0x7;
			val = 4 >> (3 - val);
			ptr[3] = val << (3 - txmode);
		}
	}
	return 0;
err1:
	warn_info(ret,"failed");
	return ret;
}

#define mulShift(A,B,S)  (int32_t)(((int64_t)(A) * (B)) >> (S))

static unsigned  CNval_calcS(const unsigned  CNval)
{
	int j, ival, i2val, i3val;

	if(CNval > 37000)  return 100;
	ival = CNval - 3000;
	if(ival < 0)  return 91;
	i2val = 0x2000 + (ival >> 1);
	i3val = ival << 14;
	for(j = 0; ; j++ ) {
		if( i2val < 0 )  i2val = -i2val;
		else if(! i2val )  i2val = 1;
		ival = (i3val / i2val + i2val) >> 1;
		if( ival == i2val || j >= 16 )  break;
		i2val = ival;
	}
	/* ival in [0, 32008]
	   (P - 1.75) << 13 = ival - 0x3800
	*/
	ival -= 0x3800;
	i2val = 979 + ((-41846 * ival) >> 13);
	i2val = 875 + ((i2val * ival) >> 14);
	i2val = 6584 + ((i2val * ival) >> 14);
	i2val = -14590 + ((i2val * ival) >> 15);
	i2val = 20684 + ((i2val * ival) >> 13) + (1 << 3);
	return i2val >> 4;
}

static unsigned  CNval_calcT(const uint32_t  CNval)
{
	unsigned  uval, j;
	int ival, i2val, i3val;

	if(CNval < 1404)  return 4000;
	if(CNval > 5000000)  return 100;

	//# calculate 10 * log10(x) << 24
#if defined (_MSC_VER)
	j = 32 - __lzcnt(CNval);
#elif defined (__GNUC__)
	j = 32 - __builtin_clz(CNval);
#else
	i2val = CNval;
	for(j = 0; i2val; j++) i2val >>= 1;
#endif
	if(CNval & (1 << (j - 2)) ) {
		uval = j;
		ival = ((1 << j) - CNval) << (24 - j);
	}else{
		uval = j - 1;
		ival = -(int)((CNval - (1 << (j - 1))) << (25 - j));
	}
	if(! ival) {
		i3val = 0;
		goto skip_frac;
	}
	i2val = ival;
	i3val = ival;
	for(j = 2; j < 40; j++ ) {
		i2val = ((int64_t)i2val * ival) >> 24;
		if(! i2val) break;
		i3val += i2val / (int)j;
	}
	i3val = mulShift( i3val, 1165800373, 28 );
skip_frac:
	uval = mulShift( uval, 1616142483, 29 - 24 );
	i3val = 1130911734 - (uval - i3val) - (20 << 24);
	ival = i3val >> 12;
	i2val = 8389 + ((5033 * ival) >> 15);
	i2val = 2294 + ((i2val * ival) >> 16);
	i2val = 6482166 + ((i2val * ival) >> 10);
	i2val = 8617370 + mulShift( i2val, i3val, 28 ) + (1 << 11);
	return i2val >> 12;
}

int tc90522_readStatistic(void * const state, const unsigned devnum, unsigned* const data)
{
	int ret;
	struct state_st* const s = state;
	struct i2c_device_st* const  dev = &(s->i2c_dev[devnum]);
	unsigned uval;
	uint8_t utmp;

	if(! dev->addr) {
		warn_info(0,"invalid device %u", devnum);
		return -2;
	}
	if(devnum & 1) {  //# sate
		//# status
		if(( ret = readReg(dev, 0xc3, &utmp ) ))  goto err1;
		uval = (~utmp >> 7) & 0x1;   //# has signal
		if(!(utmp & 0x60)) uval |= 0xe;   //# has carrier, viterbi, sync
		if(!(utmp & 0x10)) uval |= 0x10;   //# has lock
		//dmsgn("status= %02X, ", utmp);
		data[0] = uval;
		//# C/N
		if((data[0] & 0x03) == 0x03) {
			if(( ret = readReg(dev, 0xbc, &utmp ) ))  goto err1;
			uval = utmp << 8;
			if(( ret = readReg(dev, 0xbd, &utmp ) ))  goto err1;
			uval |= utmp & 0xFF;
			data[1] = CNval_calcS(uval);
			//dmsgn("CN(%u)= %u, ", uval, data[1]);
		}else{
			data[1] = data[0] & 0x1F;
		}
	}else{  //# terra
		//# status
		if(( ret = readReg(dev, 0x80, &utmp ) ))  goto err1;
		//dmsgn("status= %02X, ", utmp);
		if(utmp & 0xf0) {
			uval = 0;
		}else{
			uval = 0x3;
			if(!(utmp & 0x08))  uval |= 0x8;
			if(!(utmp & 0x04))  uval |= 0x4;
			if(!(utmp & 0x02))  uval |= 0x10;
		}
		data[0] = uval;
		//# C/N
		if((data[0] & 0x03) == 0x03) {
			if(( ret = readReg(dev, 0x8b, &utmp ) ))  goto err1;
			uval = utmp << 16;
			if(( ret = readReg(dev, 0x8c, &utmp ) ))  goto err1;
			uval |= utmp << 8;
			if(( ret = readReg(dev, 0x8d, &utmp ) ))  goto err1;
			uval |= utmp & 0xFF;
			data[1] = CNval_calcT(uval);
			//dmsgn("CN(%u)= %u, ", uval, data[1]);
		}else{
			data[1] = data[0] & 0x1F;
		}
	}

	return 0;
err1:
	warn_info(ret,"failed");
	return ret;
}


/*EOF*/