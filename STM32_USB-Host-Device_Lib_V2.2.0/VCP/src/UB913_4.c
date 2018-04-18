
#include "inc_all.h"



// I2C Addr 7bit
#define UB914_ADDR		0x60
#define UB913_ADDR		0x58

#define SENSOR_ADDR		0x4C

//I2C reg addr defined
//UB914
#define UB914_ADDR_REG		0x00
#define UB914_RESET_REG 	0x01
#define UB914_CONFIG0_REG 	0x02
#define UB914_CONFIG1_REG 	0x03
#define UB914_SER_ADDR_REG		0x06
#define UB914_SER_ALIAS_REG		0x07
#define UB914_SENSOR_ADDR_REG	0x08
#define UB914_SENSOR_ALIAS_REG	0x010

//UB913
#define UB913_ADDR_REG		0x00
#define UB913_RESET_REG 	0x01
#define UB913_CONFIG_REG 	0x03

#define UB913_DESER_ALIAS_REG	0x07

#define UB913_SENSOR_ADDR_REG	0x08
#define UB913_SENSOR_ALIAS_REG	0x09

#define UB913_GPIO01_REG	0x0D
#define UB913_GPIO23_REG	0x0E

#define UB913_SCL_HIGH_REG	0x11
#define UB913_SCL_LOW_REG	0x12




static I2C_DEV_t I2C_UB914;
static I2C_DEV_t I2C_UB913;

static u8 wait_bus_idle(void);

volatile bool UB913_4_OK = false;


/****************************************************************************************
* Function:... 		I2C basic functions 
* Parameters:  	 
* Returns:.... 		 
* Description: 	 
* Created:.... 		gary
*
****************************************************************************************/

static void i2c_scl_low()
{
	GPIO_ResetBits(I2C1_SCL_PORT,I2C1_SCL_PIN);
}

static void i2c_scl_high()
{
	GPIO_SetBits(I2C1_SCL_PORT,I2C1_SCL_PIN);
}


static void i2c_sda_low()
{
	GPIO_ResetBits(I2C1_SDA_PORT,I2C1_SDA_PIN);
}

static void i2c_sda_high()
{
	GPIO_SetBits(I2C1_SDA_PORT,I2C1_SDA_PIN);
}



static void i2c_sda_dir_in()
{
	//Release SDA
	GPIO_SetBits(I2C1_SDA_PORT,I2C1_SDA_PIN);
}

static void i2c_sda_dir_out()
{
	//GPIO_SetBits(I2C1_SDA_PORT,I2C1_SDA_PIN);;		// NULL	
}

static u8 i2c_sda_value()
{
	if (I2C1_SDA_PORT->IDR & I2C1_SDA_PIN) return 1;	//No ack
	else return 0;		//Ack ok
}

static u8 i2c_scl_value()
{
	if (I2C1_SCL_PORT->IDR & I2C1_SCL_PIN) return 1;
	else return 0;		
}

static void i2c_delay1()
{
	drv_delayus(10);

}

static void i2c_delay2()
{
	drv_delayus(50);
}



static void init_i2c_dev(void)
{
	// UB914
	I2C_UB914.scl_low = i2c_scl_low;
	I2C_UB914.scl_high = i2c_scl_high;
	I2C_UB914.sda_low = i2c_sda_low;
	I2C_UB914.sda_high = i2c_sda_high;
	I2C_UB914.sda_dir_in = i2c_sda_dir_in;
	I2C_UB914.sda_dir_out = i2c_sda_dir_out;
	I2C_UB914.sda_value = i2c_sda_value;
	I2C_UB914.scl_value = i2c_scl_value;
	I2C_UB914.delay1    = i2c_delay1;
	I2C_UB914.delay2    = i2c_delay2;
	I2C_UB914.addr    	 = UB914_ADDR<<1;
	I2C_UB914.clock_stretch    = 1;
	I2C_UB914.wait_bus_idle = wait_bus_idle;

	// UB913
	I2C_UB913.scl_low = i2c_scl_low;
	I2C_UB913.scl_high = i2c_scl_high;
	I2C_UB913.sda_low = i2c_sda_low;
	I2C_UB913.sda_high = i2c_sda_high;
	I2C_UB913.sda_dir_in = i2c_sda_dir_in;
	I2C_UB913.sda_dir_out = i2c_sda_dir_out;
	I2C_UB913.sda_value = i2c_sda_value;
	I2C_UB913.scl_value = i2c_scl_value;
	I2C_UB913.delay1	 = i2c_delay1;
	I2C_UB913.delay2	 = i2c_delay2;
	I2C_UB913.addr 	 = UB913_ADDR<<1;
	I2C_UB913.clock_stretch	= 1;
	I2C_UB913.wait_bus_idle = wait_bus_idle;
	
}


static void init_UB914(void)
{
	unsigned char buf[2];
	u8 i;
	u32 j;

	//0. Check ID
	j = 100;
	while (j--)
	{
		soft_i2c_read(&I2C_UB914,UB914_ADDR_REG,buf,1);
		if (buf[0] == 0xC0)  
		{
			TraceStr("UB914 i2c ok\r\n");
			break;
		}
		clear_watchdog_flag(TASK_ALL);
		vTaskDelay(10);
	}

	vTaskDelay(100);


	j= 500;
	while (j--)
	{
		vTaskDelay(2);
		clear_watchdog_flag(TASK_ALL);
		soft_i2c_read(&I2C_UB914,0x06,buf,1);	
	    if (buf[0] == 0xB0) break;
	}
	if (j==0) {TraceStr("Ub913 found \r\n");return;}
	TraceStr("Ub913 found \r\n");

	
	//6. Read Ser ID	
	i |= soft_i2c_read(&I2C_UB914,UB914_SER_ADDR_REG,buf,1);	
	Trace("UB913 I2C Addr",buf[0]);

	//7. Set Ser Alias ID
	buf[0] = UB913_ADDR<<1;		
	soft_i2c_write(&I2C_UB914,UB914_SER_ALIAS_REG,buf,1);

	vTaskDelay(10);

	
	//8. Config sensor addr
	buf[0] = SENSOR_ADDR<<1;		
	soft_i2c_write(&I2C_UB914,UB914_SENSOR_ADDR_REG,buf,1);		

	//0x10. Config sensor alias addr
	buf[0] = SENSOR_ADDR<<1;		
	soft_i2c_write(&I2C_UB914,UB914_SENSOR_ALIAS_REG,buf,1);		

	//For UB914, All GPIO is input, So default setting is ok


	if (i!=0) 
	{	
		Trace("UB914 I2c error",i);
		UB913_4_OK = false;
	}
	else 
	{
		TraceStr("All UB914 i2c is ok\r\n");	
		UB913_4_OK = true;
	}
	
	//Initialize finished
	
}



static void init_UB913(void)
{
	unsigned char buf[2];
	u8 i=0;


	//0x11   Config SCL high time
	buf[0] = 0x18;		//0x82 is 100k
	i |= soft_i2c_write(&I2C_UB913,UB913_SCL_HIGH_REG,buf,1);		
	//0x12  Config SCL low time
	i |= soft_i2c_write(&I2C_UB913,UB913_SCL_LOW_REG,buf,1);		


	// Set GPIO2,3 as uart output
	buf[0] = 0x55;		 
	i |= soft_i2c_write(&I2C_UB913,0x0E,buf,1);		

	if (i!=0) 
	{
		Trace("UB913 I2c error",i);
		UB913_4_OK = false; 
	}
	else 
	{
		TraceStr("All UB913 i2c is ok\r\n");
		UB913_4_OK = true;
	}


	//Initialize finished
}


void init_camera_lvds_chip(void)
{
	init_i2c_dev();
	init_UB914();
	init_UB913();


	/* Default:  UB913 is rising edge latch
			     UB914 is rising edge latch
	*/


}

void read_ub914_mode(void)
{
	u8 i;
	u8 buf[2];
	u32 j;

	while (1)
	{
		if ((I2C_UB914.sda_value() == 1) && (I2C_UB914.scl_value() == 1))
		{
			j++;
		}
		else j=0;
		drv_delayus(10);
		if (j==50) break;
	}


	buf[0] = 0xff;
	i |= soft_i2c_read(&I2C_UB914,0x1a,buf,1);	
	Trace("Read ub914 reg 0x1a",buf[0]);

	buf[0] = 0xff;
	i |= soft_i2c_read(&I2C_UB914,0x1b,buf,1);	
	Trace("Read ub914 reg 0x1b",buf[0]);

	
}





static u8 wait_bus_idle(void)
{
	u32 i,j;

	i = 0;
	j = 0;
	while (1)
	{
		if ((I2C_UB914.sda_value() == 1) && (I2C_UB914.scl_value() == 1))
		{
			j++;
		}
		else j=0;
		drv_delayus(1);
		i++;
		if (j==500) break;
		if (i>20000) return 0;
	}
	return 1;

}






u8 check_ub913(void)
{
	u8 buf[2];

	//Check bus is busy ?
	if (wait_bus_idle())
	{
		soft_i2c_read(&I2C_UB914,0x1c,buf,1);	
		Trace("UB914 0x1c",buf[0]);

		if (buf[0] == 0x13) return 1;
		else return 0;
	}
	else return 0;
}







