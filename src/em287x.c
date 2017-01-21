/* SunPTV-USB   (c) 2016 trinity19683
  EM287x USB interface
  em287x.c
  2016-02-10
*/

#include <errno.h>
#include <string.h>
#include <malloc.h>

#include "usbops.h"
#include "osdepend.h"
#include "em287x.h"
#include "em287x_usb.h"
#include "em287x_priv.h"
#include "message.h"

#define ARRAY_SIZE(x)  (sizeof(x)/(sizeof(x[0])))


/* register  read, write, masked write */

static int  readReg(struct state_st* const s, const unsigned reg, uint8_t* const val)
{
	return em287x_ctrl(s->fd, reg, 1, val, 0);
}

static int  writeReg(struct state_st* const s, const unsigned reg, uint8_t const val)
{
	uint8_t wtmp = val;
	return em287x_ctrl(s->fd, reg, 1, &wtmp, 1);
}

static int  writeBitReg(struct state_st* const s, const unsigned reg, uint8_t const val, uint8_t const mask)
{
	int r, ret;
	uint8_t wtmp;
	//# mutex lock
	if((ret = uthread_mutex_lock(s->pmutex))) {
		warn_info(ret,"mutex_lock failed");
		return -EINVAL;
	}
	if(0 == mask || 0xFF == mask) {
		wtmp = val;
		goto write_reg;
	}
	if(( r = em287x_ctrl(s->fd, reg, 1, &wtmp, 0) ))
		goto err1;

	wtmp = (val & mask) | (wtmp & ~mask);
write_reg:
	r = em287x_ctrl(s->fd, reg, 1, &wtmp, 1);
err1:
	//# mutex unlock
	if((ret = uthread_mutex_unlock(s->pmutex))) {
		warn_info(ret,"mutex_unlock failed");
		return -EINVAL;
	}
	return r;
}


static int  I2C_check(struct state_st* const s)
{
	int r;
	uint8_t utmp;
	r = readReg(s, REG_I2C_RET, &utmp);
	if( r ) {
		warn_info(r,"failed");
		return -1;
	}
	return utmp;
}

/* I2C  write & read */
static int  em287x_I2C(struct state_st* const s, const unsigned addr, const unsigned wlen, uint8_t* const wdata, const unsigned rlen, uint8_t* const rdata )
{
	int r, i;
	uint8_t utmp;
	//# mutex lock
	if((i = uthread_mutex_lock(s->pmutex))) {
		warn_info(i,"mutex_lock failed");
		return -EINVAL;
	}
	if(
	   ( !(addr & I2C_FLAG_1STBUS) && !(s->i2c_mode & I2C_MODE_2NDBUS) ) ||
	   (  (addr & I2C_FLAG_1STBUS) &&  (s->i2c_mode & I2C_MODE_2NDBUS) )
	  ) {
		if(( r = readReg(s, REG_I2C_MODE, &utmp) )) {
			warn_info(r,"failed");  goto err1;
		}
		utmp &= ~(I2C_MODE_2NDBUS | 0x3);
		utmp |= (addr & I2C_FLAG_1STBUS) ? I2C_MODE_CLK25K : (I2C_MODE_2NDBUS | I2C_MODE_CLK400K);
		if(( r = writeReg(s, REG_I2C_MODE, utmp) )) {
			warn_info(r,"failed");  goto err1;
		}
		s->i2c_mode = utmp;
	}

	if(wlen && wdata) {
		//# write
		r = em287x_ctrl(s->fd, addr & 0xFF, wlen, wdata, (rlen) ? 0x301 : 0x201);
		if( r ) {
			warn_info(r,"failed");  goto err1;
		}
		//# error check
		for(i = 5; i > 0; i--) {
			r = I2C_check(s);
			if( 0 > r || 0 == r || 0x10 == r ) {
				//# error, success, ACK error
				break;
			}
			miliWait(3);
		}
		if( r ) {
			warn_info(r,"i2c:%02X failed", addr & 0xFF);
			r = -EIO;  goto err1;
		}
	}
	if(rlen && rdata) {
		//# read
		r = em287x_ctrl(s->fd, addr & 0xFF, rlen, rdata, (wlen) ? 0x200 : 0x300);
		if( r ) {
			warn_info(r,"failed");  goto err1;
		}
		//# error check
		r = I2C_check(s);
		if( r ) {
			if(addr & 0x8000) {    //# obtain error code  mode
				goto err1;
			}
			warn_info(r,"i2c:%02X failed", addr & 0xFF);
			r = -EIO;  goto err1;
		}
	}

err1:
	//# mutex unlock
	if((i = uthread_mutex_unlock(s->pmutex))) {
		warn_info(i,"mutex_unlock failed");
		return -EINVAL;
	}
	return r;
}


/* obtain chip version, type */
static int  identify_state(struct state_st* const s)
{
	int ret, i;
	uint8_t utmp[8], chipid;
	const uint8_t chipid_table[] = { 0x41, 0x72, 0 };
	const unsigned chipid_val[] = { 2874, 28178 };

	if(( ret = readReg(s, REG_CHIPID, &chipid) ))  goto err1;
	for(i = 0; i < 10 && (chipid_table[i]) ; i++) {
		if(chipid == chipid_table[i])  goto match_chipid;
	}
	warn_msg(0,"unknown chip=%02X", chipid);
	return -1;
match_chipid:
	s->chip_id = chipid_val[i];

	//# read EEPROM
	if(( ret = writeReg(s, REG_I2C_MODE, 0x40 | I2C_MODE_CLK25K ) ))  goto err1;
	if(( ret = readReg(s, REG_I2C_MODE, utmp) ))  goto err1;
	s->i2c_mode = utmp[0];
	//# addr = 0x0068
	utmp[0] = 0;  utmp[1] = 0x68;
	if(( ret = em287x_I2C(s, I2C_ADDR_EEPROM, 2, utmp, 4, utmp ) ))  {
		warn_msg(ret,"failed to read EEPROM");
		goto err1;
	}
	s->config_id = (utmp[0] << 16) | (utmp[1] << 24) | utmp[2] | (utmp[3] << 8);

	dmsgn("Chip= %u_%02X, config= %08X, ", s->chip_id, chipid, s->config_id);
	return 0;
err1:
	return ret;
}

/* reset device, initialize */
static int  initDevice(struct state_st* const s)
{
	int ret;

	if(( ret = writeReg(s, 0x0d, 0xFF ) ))  goto err1;

	//# demod: power on
	if(( ret = writeReg(s, REG_GPIO_P0, 0xFE ) ))  goto err1;
	//# demod: reset
	if(( ret = writeBitReg(s, REG_GPIO_P0, 0x00, 0x40 ) ))  goto err1;
	miliWait(100);
	if(( ret = writeBitReg(s, REG_GPIO_P0, 0x40, 0x40 ) ))  goto err1;
	miliWait(10);

	if(( ret = writeReg(s, 0x12, 0x27 ) ))  goto err1;
	if(( ret = writeReg(s, 0x0d, 0x42 ) ))  goto err1;

	if(( ret = writeReg(s, 0x15, 0x20 ) ))  goto err1;
	if(( ret = writeReg(s, 0x16, 0x20 ) ))  goto err1;
	if(( ret = writeReg(s, 0x17, 0x20 ) ))  goto err1;
	if(( ret = writeReg(s, 0x18, 0 ) ))  goto err1;
	if(( ret = writeReg(s, 0x19, 0 ) ))  goto err1;
	if(( ret = writeReg(s, 0x1a, 0 ) ))  goto err1;
	if(( ret = writeReg(s, 0x23, 0 ) ))  goto err1;
	if(( ret = writeReg(s, 0x24, 0 ) ))  goto err1;
	if(( ret = writeReg(s, 0x26, 0 ) ))  goto err1;
	if(( ret = writeReg(s, 0x13, 0x08 ) ))  goto err1;

	if(( ret = writeReg(s, 0x0c, 0x10 ) ))  goto err1;
	if(( ret = usb_setinterface(s->fd, 0, 1) )) {
		warn_info(ret,"failed");  goto err1;
	}
	if(( ret = writeReg(s, 0x5f, 0x80 ) ))  goto err1;

	return 0;
err1:
	return ret;
}

/* sleep device, USB suspend */
static int  sleepDevice(struct state_st* const s)
{
	int ret;

	if(( ret = usb_setinterface(s->fd, 0, 0) )) {
		warn_info(ret,"failed");  goto err1;
	}

	if(( ret = writeBitReg(s, 0x5f, 0x00, 0x0F) ))  goto err1;
	if(( ret = writeReg(s, REG_GPIO_P0, 0xFF ) ))  goto err1;
	if(( ret = writeReg(s, 0x0c, 0 ) ))  goto err1;
	return 0;
err1:
	return ret;
}



/* public function */

int em287x_create(em287x_state* const  state, struct usb_endpoint_st * const pusbep)
{
	int ret;
	struct state_st* st;
	uint8_t utmp;
	if(NULL == *state) {
		st = malloc(sizeof(struct state_st));
		if(NULL == st) {
			warn_info(errno,"failed to allocate");
			return -1;
		}
		*state = st;
		st->pmutex = NULL;
	}else{
		st = *state;
	}
	if((ret = uthread_mutex_init(&st->pmutex)) != 0) {
		warn_info(ret,"failed");
		return -2;
	}
	pusbep->endpoint = EP_TS1;
	pusbep->dev = st;
	pusbep->startstopFunc = em287x_startstopStream;
	st->chip_id = 0;
	st->fd = pusbep->fd;
	if(( ret = usb_claim(st->fd, 0) )) {
		warn_info(ret,"failed");
		return -5;
	}
	if(( ret = identify_state(st) )) {
		warn_info(ret,"failed");
		return -6;
	}
	if(( ret = initDevice(st) )) {
		warn_info(ret,"failed");
		return -7;
	}

	//# TS endpoint
	if(( ret = readReg(st, 0x0b, &utmp) ))  { warn_info(ret,"failed"); return -8; }
	if( utmp & 0x02 ) {
		//# set Isochronous transfer size (unit: packet)
		if(( ret = writeReg(st, 0x5e, 5 ) ))  { warn_info(ret,"failed"); return -8; }
		pusbep->endpoint |= 0x100;
		pusbep->xfer_size = 188 * 5;
	}else{
		//# set BULK transfer size (unit: packet)
		if(( ret = writeReg(st, 0x5d, 245 - 1 ) ))  { warn_info(ret,"failed"); return -8; }
		pusbep->xfer_size = 188 * 245;
	}

	dmsg(" em287x init done!");

	return 0;
}

int em287x_destroy(const em287x_state state)
{
	int r, ret = 0;
	struct state_st* const s = state;
	if(NULL == s) {
		warn_info(0,"null pointer");
		return -1;
	}

	if((r = sleepDevice(s))) {
		warn_info(r,"failed");
		ret = (ret << 8) | (r & 0xFF);
	}
	if((r = usb_release(s->fd, 0))) {
		warn_info(r,"failed");
		ret = (ret << 8) | (r & 0xFF);
	}
	if((r = uthread_mutex_destroy(s->pmutex))) {
		warn_info(r,"mutex_destroy failed");
		ret = (ret << 8) | (r & 0xFF);
	}
	free(s);
	return ret;
}

void em287x_attach(const em287x_state state, struct i2c_device_st* const  i2c_dev)
{
	if(! i2c_dev)  return;
	i2c_dev->dev = state;
	i2c_dev->i2c_comm = (void*)em287x_I2C;
}

int em287x_startstopStream(const em287x_state state, const int start)
{
	int r;
	struct state_st* const s = state;
	uint8_t utmp;
	if(start) {
		r = readReg(s, 0x5f, &utmp);
		if( r ) { warn_info(r,"failed");  return -5; }
		if(utmp & 0x08) { warn_msg(0,"TS encrypt detected!"); }
		utmp = (utmp & 0xF0) | 0x05;
		r = em287x_ctrl(s->fd, 0x785f, 1, &utmp, 0x1401);
		if( r ) { warn_info(r,"failed");  return -6; }
	}else{  //# stop
		r = writeBitReg(s, 0x5f, 0x00, 0x0F);
		if( r ) { warn_info(r,"failed");  return -5; }
	}
	return 0;
}



/*EOF*/