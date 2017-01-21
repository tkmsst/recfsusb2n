/* SunPTV-USB   (c) 2016 trinity19683
  type definition (Linux OSes)
  types_u.h
  2016-01-22
*/
#pragma once

typedef int HANDLE;
typedef void* PMUTEX;

struct usb_endpoint_st {
	HANDLE fd;    //# devfile descriptor
	unsigned  endpoint;    //# USB endpoint
	void* dev;
	int (* startstopFunc)(void * const  dev, const int start);
	unsigned  xfer_size;    //# transfer unit size
};

struct i2c_device_st {
	void* dev;    //# device pointer
	int (* i2c_comm)(void* const, const unsigned addr, const unsigned wlen, void* const wdata, const unsigned rlen, void* const rdata );
	unsigned char  addr;
};

/*EOF*/