/* SunPTV-USB   (c) 2016 trinity19683
  EM287x USB interface
  em287x_priv.h
  2016-01-18
*/
#pragma once

#define EP_TS1  0x84   //# TS data

#define I2C_ADDR_EEPROM    0x10A0
#define I2C_FLAG_1STBUS    0x1000
#define I2C_MODE_2NDBUS    0x04
#define I2C_MODE_CLK1500K  0x03
#define I2C_MODE_CLK25K    0x02
#define I2C_MODE_CLK400K   0x01
#define I2C_MODE_CLK100K   0x00

#define REG_I2C_RET    0x05
#define REG_I2C_MODE   0x06
#define REG_CHIPID     0x0A
#define REG_GPIO_P0    0x80
#define REG_GPIO_P0IN  0x84


struct state_st {
	HANDLE fd;    //# devfile descriptor
	PMUTEX pmutex;    //# mutex for USB device (pthread_mutex_t)
	uint32_t chip_id;    //# em287x chip ID (0=invalid)
	uint32_t config_id;    //# device config ID
	uint8_t i2c_mode;
};

/*EOF*/