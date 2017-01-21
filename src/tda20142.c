/* SunPTV-USB   (c) 2016 trinity19683
  TDA20142  BS tuner
  tda20142.c
  2016-01-28
*/

#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <malloc.h>

#include "osdepend.h"
#include "tda20142.h"
#include "message.h"

#define ARRAY_SIZE(x)  (sizeof(x)/(sizeof(x[0])))

struct state_st {
	struct i2c_device_st  i2c_dev;
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

/* public function */

int tda20142_create(void ** const  state)
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
	}else{
		st = *state;
	}
	return 0;
}

int tda20142_destroy(void * const state)
{
	int ret = 0;
	struct state_st* const s = state;
	if(! s) {
		warn_info(0,"null pointer");
		return -1;
	}

	free(s);
	return ret;
}

void* tda20142_i2c_ptr(void * const state)
{
	if(! state)  return NULL;
	return &( ((struct state_st*)state)->i2c_dev );
}


int tda20142_init(void * const state)
{
	int ret, i;
	struct state_st* const s = state;
	struct i2c_device_st* const  dev = &(s->i2c_dev);
	uint8_t utmp;

	if(( ret = writeReg(dev, 0x02,  0x81, 0x81) ))  goto err1;
	if(( ret = writeReg(dev, 0x06,  0x39, 0xb9) ))  goto err1;
	if(( ret = writeReg(dev, 0x07,  0xae, 0xae) ))  goto err1;
	if(( ret = writeReg(dev, 0x0f,  0x80, 0x80) ))  goto err1;
	if(( ret = writeReg(dev, 0x18,  0   , 0x80) ))  goto err1;
	if(( ret = writeReg(dev, 0x1a,  0xc0, 0xc0) ))  goto err1;
	if(( ret = writeReg(dev, 0x22,  0xFF, 0 ) ))  goto err1;
	if(( ret = writeReg(dev, 0x23,  0   , 0x01) ))  goto err1;
	if(( ret = writeReg(dev, 0x25,  0x08, 0x08) ))  goto err1;
	if(( ret = writeReg(dev, 0x27,  0xc0, 0xe0) ))  goto err1;
	if(( ret = writeReg(dev, 0x24,  0x04, 0x34) ))  goto err1;
	if(( ret = writeReg(dev, 0x0d,  0   , 0x20) ))  goto err1;
	if(( ret = writeReg(dev, 0x09,  0xb0, 0xfe) ))  goto err1;
	if(( ret = writeReg(dev, 0x0a,  0x6f, 0xef) ))  goto err1;
	if(( ret = writeReg(dev, 0x0b,  0x7a, 0xfe) ))  goto err1;
	if(( ret = writeReg(dev, 0x0c,  0   , 0 ) ))  goto err1;
	if(( ret = writeReg(dev, 0x19,  0xfa, 0 ) ))  goto err1;
	if(( ret = writeReg(dev, 0x1b,  0   , 0x80) ))  goto err1;
	if(( ret = writeReg(dev, 0x21,  0x40, 0x40) ))  goto err1;
	if(( ret = writeReg(dev, 0x10,  0x90, 0xd0) ))  goto err1;
	if(( ret = writeReg(dev, 0x14,  0x20, 0x30) ))  goto err1;

	if(( ret = writeReg(dev, 0x1a,  0x40, 0x40) ))  goto err1;
	if(( ret = writeReg(dev, 0x18,  0x01, 0x01) ))  goto err1;
	if(( ret = writeReg(dev, 0x18,  0x80, 0x80) ))  goto err1;
	if(( ret = writeReg(dev, 0x1b,  0x80, 0x80) ))  goto err1;
	if(( ret = writeReg(dev, 0x18,  0   , 0x01) ))  goto err1;

	if(( ret = writeReg(dev, 0x0f,  0x80, 0xe0) ))  goto err1;
	if(( ret = writeReg(dev, 0x13,  0x20, 0x30) ))  goto err1;
	if(( ret = writeReg(dev, 0x12,  0xc0, 0xc0) ))  goto err1;
	if(( ret = writeReg(dev, 0x10,  0x20, 0x20) ))  goto err1;
	if(( ret = writeReg(dev, 0x10,  0x20, 0x20) ))  goto err1;    //# twice
	if(( ret = writeReg(dev, 0x0f,  0x20, 0x20) ))  goto err1;
	for(i = 3; i > 0; i-- ) {
		ret = readReg(dev, 0x11, &utmp);
		if( (! ret) && (utmp) )  break;
	}
	if(! i ) {
		warn_info(utmp,"timeout");  goto err0;
	}
	if(( ret = readReg(dev, 0x10, &utmp) ))  goto err1;
	if(( ret = writeReg(dev, 0x0f, utmp, 0x0F) ))  goto err1;
	if(( ret = writeReg(dev, 0x0f,  0x40, 0x40) ))  goto err1;
	if(( ret = writeReg(dev, 0x0f,  0   , 0x20) ))  goto err1;
	if(( ret = writeReg(dev, 0x12,  0   , 0x80) ))  goto err1;
	if(( ret = writeReg(dev, 0x0d,  0x20, 0x60) ))  goto err1;

	dmsg(" tda20142 init done!");
	return 0;
err1:
	warn_info(ret,"failed");
err0:
	return ret;
}

static int  set_multiplierB_param(struct i2c_device_st* const  dev, const unsigned  freq_val, unsigned * const  pval)
{
	int ret;
	uint8_t utmp, mval, dval;
	unsigned  uval, bd_lo, bd_hi;
	const unsigned fval_bd[] = { 1345, 2485, 1305, 2525, 1310, 2510, 1290, 2530 };

	if(( ret = readReg(dev, 0x21, &utmp) ))  goto err1;
	uval = (utmp >> 4) & 0x4;
	*pval = (utmp & 0x40) << 10;
	if(( ret = readReg(dev, 0x1b, &utmp) ))  goto err1;
	uval |= (utmp << 1) & 0x2;
	bd_lo = fval_bd[uval];
	bd_hi = fval_bd[uval | 1];

	uval = freq_val >> 1;
	mval = 1;
	if(bd_hi >= uval) {
		dval = 2;
		if(bd_lo > uval) {
			if(bd_hi >= freq_val) {
				if(bd_lo <= freq_val) {
					mval = 2;
				}else{
					mval = 3;
				}
			}else{
				mval = 3;  dval = 4;
			}
		}
	}else{
		dval = 4;
	}

	uval = (freq_val * mval) / dval;
	dmsgn("mulB= %u/%u, %u<%u<%u, ", mval, dval, bd_lo, uval, bd_hi);
	if( bd_lo > uval || bd_hi < uval )  {
		ret = -1;  goto err1;
	}

	if(( ret = writeReg(dev, 0x03, (mval - 1) << 6, 0xC0) ))  goto err1;
	if(( ret = writeReg(dev, 0x1a, dval << 3 , 0x20) ))  goto err1;
	(*pval) |= (mval << 8) | dval;
	return 0;
err1:
	warn_info(ret,"failed");
	return ret;
}

#define opr_div(A,B,C)  {C = A / B; A = A % B;}

static int  setFreqParam(struct state_st* const s, const unsigned  freq, const int  mul_val)
{
	int ret, i;
	struct i2c_device_st* const  dev = &(s->i2c_dev);
	uint8_t utmp, mval;
	unsigned  freq_val, uval, u2val, u3val, denomi;
	const unsigned freq_bd[] = { 1720000, 1433000, 1228000, 1075000 };
	const uint8_t reg_mul_val[] = { 0x08, 0x88, 0x28, 0xA8, 0x04 };
	if(0 > mul_val || 4 < mul_val) {
		for(i = 0; i < 4; i++ ) {
			if(freq >= freq_bd[i]) break;
		}
		mval = i & 0xFF;
	}else{
		mval = mul_val & 0xFF;
	}
	if(( ret = writeReg(dev, 0x22, (mval == 4) ? 0x1E : 0xFF, 0 ) ))  goto err1;
	if(( ret = writeReg(dev, 0x23, reg_mul_val[mval] | 0x03, 0xAF ) ))  goto err1;
	mval += 4;
	dmsgn("mulA= %u, ", mval);

	freq_val = freq * mval * 10;
	//# clock = 27 000 kHz
	denomi = 27000;
	opr_div(freq_val, denomi, uval);

	if(( ret = set_multiplierB_param(dev, uval, &u3val) ))  goto err1;
	//# numerator
	mval = (u3val >> 8) & 0xFF;
	uval *= mval;
	freq_val *= mval;
	//# denominator
	mval = (u3val & 0xFF) * 10;
	opr_div(uval, mval, u2val);
	freq_val += uval * denomi;
	denomi *= mval;
	dmsgn("freqVal= %u +%u/%u, ", u2val, freq_val, denomi);
	
	//# remainder * 65536 / denominator
	mval = (u3val >> 16) & 0xFF;
	freq_val <<= 8;
	opr_div(freq_val, denomi, uval);
	u2val += uval >> 8;
	uval &= 0xFF;
	freq_val <<= 8;
	opr_div(freq_val, denomi, u3val);
	uval = (uval << 8) + u3val;
	//#
	if(!(mval & 0x1)) {
		uval = (uval + 1 ) >> 1;
		if(uval < 0x4000) {
			uval += 0x8000;
			u2val --;
		}
	}
	if(u2val < 128) {
		warn_info(u2val,"bug-check");  goto err0;
	}
	utmp = (u2val - 128) & 0xFF;
	dmsgn("freqVal= %02X:%04X, ", utmp, uval);

	if(( ret = writeReg(dev, 0x1e, utmp, 0 ) ))  goto err1;
	if(( ret = writeReg(dev, 0x1f, (uval >> 8) & 0xFF, 0 ) ))  goto err1;
	if(( ret = writeReg(dev, 0x20, uval & 0xFF, 0 ) ))  goto err1;
	return 0;
err1:
	warn_info(ret,"failed");
err0:
	return ret;
}

static int  setClockParam(struct state_st* const s, const unsigned  b_ratio, const unsigned  symbol_rate, const unsigned  bw)
{
	int ret, i;
	struct i2c_device_st* const  dev = &(s->i2c_dev);
	uint8_t utmp, mval, nval;
	unsigned uval, u2val;
	const unsigned drate_bd[] = { 25000, 35000, 45000 };
	const uint8_t  drate_val[] = { 0xF, 0xC, 0xA, 0x8 };
	const unsigned bdrate_bd[] = {
   2850,   3900,  4850,  5800,  6650,  7700,  8900,  9900, 10700, 11400,
  12500,  13500, 14550, 15800, 16800, 17750, 18350, 19200, 20200, 21000,
  22450,  23900, 25000, 26500, 27900, 28750, 29700, 30700, 31450, 33950,
  39000,  42500, 43950, 47350, 51700, 57750, 100000
	};
	const uint8_t  bdrate_val[] = {
  0x13, 0x18, 0x22, 0x25, 0x31, 0x33, 0x41, 0x43, 0x51, 0x52,
  0x60, 0x62, 0x70, 0x72, 0x73, 0x74, 0x3D, 0x75, 0x76, 0x5A,
  0x77, 0x5B, 0x6A, 0x79, 0x6B, 0x7A, 0x4E, 0x5D, 0x6C, 0x7B,
  0x7C, 0x7D, 0x6E, 0x5F, 0x7E, 0x6F, 0x7F
	};

	if(( ret = writeReg(dev, 0x12, 0x40, 0xC0) ))  goto err1;
	if(( ret = writeReg(dev, 0x13, 0x02, 0x03) ))  goto err1;
	if(( ret = writeReg(dev, 0x13, 0   , 0x80) ))  goto err1;
	if(( ret = writeReg(dev, 0x13, 0x20, 0x20) ))  goto err1;
	if(( ret = writeReg(dev, 0x13, 0x20, 0x20) ))  goto err1;    //# twice
	if(( ret = writeReg(dev, 0x13, 0   , 0x80) ))  goto err1;
	if(( ret = writeReg(dev, 0x13, 0x10, 0x10) ))  goto err1;
	for(i = 3; i > 0; i-- ) {
		ret = readReg(dev, 0x15, &utmp);
		if( (! ret) && (utmp & 0x10) )  break;
	}
	if(! i ) {
		warn_info(utmp,"timeout");  goto err0;
	}
	if(( ret = writeReg(dev, 0x13, 0   , 0x10) ))  goto err1;
	if(( ret = writeReg(dev, 0x12, 0   , 0x80) ))  goto err1;

	uval = ( b_ratio * symbol_rate ) / 2 + ( bw * 100 );
	uval /= 72;
	u2val = uval + 2000;
	for(i = 0; i < ARRAY_SIZE(drate_bd); i++ ) {
		if(u2val < drate_bd[i])  break;
	}
	mval = drate_val[i];
	for(i = 0; i < ARRAY_SIZE(bdrate_bd); i++ ) {
		if(uval < bdrate_bd[i])  break;
	}
	nval = bdrate_val[i];
	//dmsgn("ratio_raw= %02X:%02X, ", mval, nval);
	if(( ret = writeReg(dev, 0x0a, mval, 0x0F) ))  goto err1;
	if(( ret = writeReg(dev, 0x0b, nval << 1, 0xFE) ))  goto err1;

	return 0;
err1:
	warn_info(ret,"failed");
err0:
	return ret;
}

static int  setAGC_Param(struct state_st* const s)
{
	int ret, i;
	struct i2c_device_st* const  dev = &(s->i2c_dev);
	uint8_t utmp, AgcMode;
	unsigned uval;
	const unsigned AGCval_bd[] = { 0, 7, 63 };
	const uint8_t A2mode_val[] = { 3, 2, 0, 4 };
	const uint8_t A3mode_val[] = { 3, 7, 15, 6 };

	if(( ret = readReg(dev, 0x0d, &utmp) ))  goto err1;
	if(utmp & 0x80) {
		warn_info(ret,"failed");  goto err0;
	}
	if(( ret = writeReg(dev, 0x0d, utmp | 0x80, 0 ) ))  goto err1;
	miliWait(5);
	if(( ret = readReg(dev, 0x0d, &utmp) ))  goto err1;
	uval = (utmp & 0x1) << 8;
	if(( ret = readReg(dev, 0x0e, &utmp) ))  goto err1;
	uval |= utmp & 0xFF;
	if(( ret = writeReg(dev, 0x0d, 0   , 0x80) ))  goto err1;
	dmsgn("AgcCtrl= %u, ", uval);
	for(i = 0; i < 3; i++ ) {
		if(uval <= AGCval_bd[i])  break;
	}
	AgcMode = i;

	if(( ret = readReg(dev, 0x07, &utmp) ))  goto err1;
	utmp = (utmp & 0xac) | (0x02 & 0x53);
	if(( ret = writeReg(dev, 0x06, 0   , 0x40) ))  goto err1;
	if(( ret = writeReg(dev, 0x07, utmp, 0 ) ))  goto err1;

	if(( ret = readReg(dev, 0x06, &utmp) ))  goto err1;
	utmp &= 0x48;
	if(A2mode_val[AgcMode] != 4)  utmp |= 0x80;
	if(A2mode_val[AgcMode] == 3)  utmp |= 0x01;
	if(!( A2mode_val[AgcMode] & 0xFC ))  utmp |= (A2mode_val[AgcMode] & 0x3) << 4;
	if(( ret = writeReg(dev, 0x06, utmp, 0 ) ))  goto err1;
	if(( ret = writeReg(dev, 0x09, (0x5 << 5) | (0x4 << 2), 0xFC) ))  goto err1;
	if(( ret = writeReg(dev, 0x0a, 0x3 << 5, 0xe0) ))  goto err1;
	if(( ret = writeReg(dev, 0x0c, A3mode_val[AgcMode] << 4, 0xF0) ))  goto err1;

	return 0;
err1:
	warn_info(ret,"failed");
err0:
	return ret;
}

int tda20142_setFreq(void * const state, const unsigned  freq)
{
	int ret;
	struct state_st* const s = state;

	if( 900000 > freq || 2450000 < freq )  {
		warn_msg(0,"freq %u kHz is out of range.", freq);
		return -EINVAL;
	}

	ret = setFreqParam(s, freq, -1);
	if( ret ) { warn_info(ret,"failed");  goto err1; }

	ret = setClockParam(s, 120, 28860, 3920 );
	if( ret ) { warn_info(ret,"failed");  goto err1; }

	ret = setAGC_Param( s );
	if( ret ) { warn_info(ret,"failed");  goto err1; }

	return 0;
err1:
	return ret;
}


/*EOF*/