
#include "inc_all.h"





#define SCL_LOW			i2c_dev->scl_low()
#define SCL_HIGH		i2c_dev->scl_high()
#define SDA_DIR_IN		i2c_dev->sda_dir_in()	
#define SDA_DIR_OUT		//i2c_dev->sda_dir_out()	
#define SDA_LOW			i2c_dev->sda_low()
#define SDA_HIGH		i2c_dev->sda_high()
#define SDA_DETECT		i2c_dev->sda_value()
#define SCL_DETECT		i2c_dev->scl_value()
#define I2C_DELAY		i2c_dev->delay1()			// Delay 5us
#define I2C_DELAY_LONG	i2c_dev->delay2()			// Delay 100us



static void _i2c_start(I2C_DEV_t *i2c_dev);
static void _i2c_stop(I2C_DEV_t *i2c_dev);
static void _i2c_ack_send(I2C_DEV_t *i2c_dev);
static unsigned char _i2c_ack_detect(I2C_DEV_t *i2c_dev);


static void _i2c_start(I2C_DEV_t *i2c_dev)
{

	SDA_HIGH;
	I2C_DELAY;
	I2C_DELAY;
	SCL_HIGH;
	I2C_DELAY;
	I2C_DELAY;
	SDA_LOW;
	I2C_DELAY;
	SCL_LOW;
	I2C_DELAY;
}

static void _i2c_stop(I2C_DEV_t *i2c_dev)
{

	SDA_LOW;
	I2C_DELAY;
	SCL_HIGH;
	I2C_DELAY;
	SDA_HIGH;
	I2C_DELAY_LONG;
}

static unsigned char _i2c_write_byte(I2C_DEV_t *i2c_dev,unsigned char data)
{
	unsigned char i;



	for(i = 0; i< 8; i++)
	{
		if( (data << i) & 0x80) 
			SDA_HIGH;
		else SDA_LOW;
		I2C_DELAY;
		SCL_HIGH;
		I2C_DELAY;

//		if (i2c_dev->clock_stretch)
//		{
//			j = 1000;
//			while (j--)
//			{
//				if (SCL_DETECT) break;
//				drv_delayus(1);
//			}
//		}
		SCL_LOW;
		I2C_DELAY;
	}

	if(_i2c_ack_detect(i2c_dev)) {

		return ERROR_CODE_FALSE;
	}
	return ERROR_CODE_TRUE;
}

static unsigned char _i2c_read_byte(I2C_DEV_t *i2c_dev)
{
	unsigned char i, data;

	data = 0;
	SDA_DIR_IN;
	for(i = 0; i< 8; i++){
		data <<= 1;
		I2C_DELAY;
		SCL_HIGH;
		I2C_DELAY;
		if (SDA_DETECT) data |= 0x01;
		SCL_LOW;
		I2C_DELAY;
	}
	
	return data;
}

static unsigned char _i2c_ack_detect(I2C_DEV_t *i2c_dev)
{
	u32 i;
	SDA_DIR_IN;	// SDA Input Mode
	I2C_DELAY;
	SCL_HIGH;
	I2C_DELAY;

	if (i2c_dev->clock_stretch)
	{
		i = 1000;
		while (i--)
		{
			drv_delayus(1);
			if (SCL_DETECT) break;
		}
	//	if (i==0) TraceStr("i2c timeout \r\n");
	}


	if (SDA_DETECT)
	{
		SDA_DIR_OUT;

		return ERROR_CODE_FALSE; // false
	}

	I2C_DELAY;
	SCL_LOW;
	SDA_DIR_OUT;
	return ERROR_CODE_TRUE; // true
}

static void _i2c_ack_send(I2C_DEV_t *i2c_dev)
{

	SDA_DIR_OUT;
	SDA_LOW;
	I2C_DELAY;
	SCL_HIGH;
	I2C_DELAY;
	SCL_LOW;
	I2C_DELAY;
}


static void _i2c_nack_send(I2C_DEV_t *i2c_dev)
{

	SDA_DIR_OUT;
	SDA_HIGH;
	I2C_DELAY;
	SCL_HIGH;
	I2C_DELAY;
	SCL_LOW;
	I2C_DELAY;
}


unsigned char soft_i2c_write(I2C_DEV_t *i2c_dev, unsigned char sub_addr, unsigned char *buff, int ByteNo)
{
	unsigned char i;

	//TraceStr("Write**");

	_i2c_start(i2c_dev);
	I2C_DELAY;
	if(_i2c_write_byte(i2c_dev,i2c_dev->addr)) {
		_i2c_stop(i2c_dev);

		return (ERROR_CODE_WRITE_ADDR+1);
	}
	if(_i2c_write_byte(i2c_dev,sub_addr)) {
		_i2c_stop(i2c_dev);

		return (ERROR_CODE_WRITE_ADDR+2);
	}
	for(i = 0; i<ByteNo; i++) {
		if(_i2c_write_byte(i2c_dev,buff[i])) {
			_i2c_stop(i2c_dev);
			return ERROR_CODE_WRITE_DATA;
		}
	}
	I2C_DELAY;
	_i2c_stop(i2c_dev);
	I2C_DELAY_LONG;
	


	return ERROR_CODE_TRUE;
}

unsigned char soft_i2c_read(I2C_DEV_t *i2c_dev, unsigned char sub_addr, unsigned char *buff, int ByteNo)
{
	unsigned char i;

	_i2c_start(i2c_dev);
	I2C_DELAY;
	if(_i2c_write_byte(i2c_dev,i2c_dev->addr)) {
		_i2c_stop(i2c_dev);

		return (ERROR_CODE_READ_ADDR+1);
	}
	if(_i2c_write_byte(i2c_dev,sub_addr)) {
		_i2c_stop(i2c_dev);
		return (ERROR_CODE_READ_ADDR+2);
	}
	_i2c_start(i2c_dev);
	I2C_DELAY;
	if(_i2c_write_byte(i2c_dev,i2c_dev->addr+1)) {
		_i2c_stop(i2c_dev);
		return (ERROR_CODE_READ_ADDR+3);
	}
	for(i = 0; i<ByteNo; i++) 
	{
		buff[i] = _i2c_read_byte(i2c_dev);
		if (i==ByteNo-1) _i2c_nack_send(i2c_dev);
		else  _i2c_ack_send(i2c_dev);
	}
	I2C_DELAY;
	_i2c_stop(i2c_dev);
	
	return ERROR_CODE_TRUE;
}







/* EOF */

