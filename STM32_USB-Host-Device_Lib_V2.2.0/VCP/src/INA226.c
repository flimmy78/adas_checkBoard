#include "inc_all.h"


#define INA226_ADDR		0x40


#define	CONFIG_REG		0x00
#define	SHUNT_V_REG		0x01
#define BUS_V_REG		0x02
#define POWER_REG		0x03
#define CURRENT_REG		0x04
#define CAL_REG			0x05
#define MASK_REG		0x06
#define ALERT_REG		0x07
#define ID_REG			0xFE
#define DIE_REG			0xFF



//Config reg
#define 	RST_BIT			(1<<15)
#define 	NOT_RST_BIT		(0)

#define 	B14_B12			(4<<12)

#define 	AVG1			(0<<9)
#define 	AVG4			(1<<9)
#define 	AVG16			(2<<9)
#define 	AVG64			(3<<9)
#define 	AVG128			(4<<9)
#define 	AVG256			(5<<9)
#define 	AVG512			(6<<9)
#define 	AVG1024			(7<<9)

#define 	VBUSCT_140us	(0<<6)
#define 	VBUSCT_204us	(1<<6)
#define 	VBUSCT_332us	(2<<6)
#define 	VBUSCT_588us	(3<<6)
#define 	VBUSCT_1100us	(4<<6)
#define 	VBUSCT_2116us	(5<<6)
#define 	VBUSCT_4156us	(6<<6)
#define 	VBUSCT_8244us	(7<<6)


#define 	VSHCT_140us		(0<<3)
#define 	VSHCT_204us		(1<<3)
#define 	VSHCT_332us		(2<<3)
#define 	VSHCT_588us		(3<<3)
#define 	VSHCT_1100us	(4<<3)
#define 	VSHCT_2116us	(5<<3)
#define 	VSHCT_4156us	(6<<3)
#define 	VSHCT_8244us	(7<<3)

#define 	MODE_SHUNT_BUS_CONTINOUS	(7<<0)



static I2C_DEV_t I2C_INA226;


extern Measurement_Para_t MeasPara;

extern u8 GetTestStage(void);
extern u8 GetMainBoard_State();
static void i2c_scl_low()
{
	GPIO_ResetBits(I2C2_SCL_PORT, I2C2_SCL_PIN);
}

static void i2c_scl_high()
{
	GPIO_SetBits(I2C2_SCL_PORT, I2C2_SCL_PIN);
}


static void i2c_sda_low()
{
	GPIO_ResetBits(I2C2_SDA_PORT, I2C2_SDA_PIN);
}

static void i2c_sda_high()
{
	GPIO_SetBits(I2C2_SDA_PORT, I2C2_SDA_PIN);
}



static void i2c_sda_dir_in()
{
	//Release SDA
	GPIO_SetBits(I2C2_SDA_PORT, I2C2_SDA_PIN);
}

static void i2c_sda_dir_out()
{
	//GPIO_SetBits(I2C2_SDA_PORT,I2C2_SDA_PIN);;		// NULL
}

static u8 i2c_sda_value()
{
	if (I2C2_SDA_PORT->IDR & I2C2_SDA_PIN) {
		return 1;    //No ack
	} else {
		return 0;    //Ack ok
	}
}

static u8 i2c_scl_value()
{
	if (I2C2_SCL_PORT->IDR & I2C2_SCL_PIN) {
		return 1;
	} else {
		return 0;
	}
}

static void i2c_delay1()
{
	drv_delayus(10);
}

static void i2c_delay2()
{
	drv_delayus(50);
}


//static u8 wait_bus_idle(void)
//{
//	return 1;
//}


static void init_i2c_gpio(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

	/* Configure the GPIO_LED pin */
	GPIO_InitStructure.GPIO_Pin = I2C2_SCL_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_Init(I2C2_SCL_PORT, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = I2C2_SDA_PIN;
	GPIO_Init(I2C2_SDA_PORT, &GPIO_InitStructure);

	GPIO_SetBits(I2C2_SCL_PORT, I2C2_SCL_PIN);
	GPIO_SetBits(I2C2_SDA_PORT, I2C2_SDA_PIN);
}



static void init_i2c_dev(void)
{
	// UB914
	I2C_INA226.scl_low = i2c_scl_low;
	I2C_INA226.scl_high = i2c_scl_high;
	I2C_INA226.sda_low = i2c_sda_low;
	I2C_INA226.sda_high = i2c_sda_high;
	I2C_INA226.sda_dir_in = i2c_sda_dir_in;
	I2C_INA226.sda_dir_out = i2c_sda_dir_out;
	I2C_INA226.sda_value = i2c_sda_value;
	I2C_INA226.scl_value = i2c_scl_value;
	I2C_INA226.delay1    = i2c_delay1;
	I2C_INA226.delay2    = i2c_delay2;
	I2C_INA226.addr    	 = INA226_ADDR << 1;
	I2C_INA226.clock_stretch    = 1;
	//I2C_INA226.wait_bus_idle = wait_bus_idle;

}


void init_INA226_normal(void)
{
	//First byte msb, second byte lsb
	u16 tmp;
	u8 buf[2];
	init_i2c_gpio();
	init_i2c_dev();


	// Read ID
	soft_i2c_read(&I2C_INA226, ID_REG, buf, 2);
	soft_i2c_read(&I2C_INA226, DIE_REG, buf, 2);


	// 1. Reset
	tmp = RST_BIT;
	buf[0] = (tmp >> 8) & 0xFF;
	buf[1] = tmp & 0xFF;
	soft_i2c_write(&I2C_INA226, CONFIG_REG, buf, 2);
	drv_delayms(10);

	// 2. Update cal reg, max_current = 3A, R= 50mR
	// cal = 0.00512/(current_lsb*R) = 1024 = 400H
	// current_lsb = (max_current/32768) = 0.1mA/bit
	buf[0] = 0x04;
	buf[1] = 0x00;
	soft_i2c_write(&I2C_INA226, CAL_REG, buf, 2);


	tmp = NOT_RST_BIT | B14_B12 | AVG512 | VBUSCT_1100us | VSHCT_1100us |
	      MODE_SHUNT_BUS_CONTINOUS;
	buf[0] = (tmp >> 8) & 0xFF;
	buf[1] = tmp & 0xFF;
	soft_i2c_write(&I2C_INA226, CONFIG_REG, buf, 2);



}



void init_INA226_sleep(void)
{
	//First byte msb, second byte lsb
	u16 tmp;
	u8 buf[2];
	init_i2c_gpio();
	init_i2c_dev();


	// Read ID
	soft_i2c_read(&I2C_INA226, ID_REG, buf, 2);
	soft_i2c_read(&I2C_INA226, DIE_REG, buf, 2);


	// 1. Reset
	tmp = RST_BIT;
	buf[0] = (tmp >> 8) & 0xFF;
	buf[1] = tmp & 0xFF;
	soft_i2c_write(&I2C_INA226, CONFIG_REG, buf, 2);
	drv_delayms(10);

	// 2. Update cal reg, max_current = 50mA, R= 20mR
	// cal = 0.00512/(current_lsb*R) = 12800 = 3200H
	// current_lsb = (max_current/32768) = 1.5uA/bit  = 20uA/bit
	buf[0] = 0x32;
	buf[1] = 0x00;
	soft_i2c_write(&I2C_INA226, CAL_REG, buf, 2);


	// 3. start
	tmp = NOT_RST_BIT | B14_B12 | AVG256 | VBUSCT_1100us | VSHCT_1100us |
	      MODE_SHUNT_BUS_CONTINOUS;
	buf[0] = (tmp >> 8) & 0xFF;
	buf[1] = tmp & 0xFF;
	soft_i2c_write(&I2C_INA226, CONFIG_REG, buf, 2);



}


extern u16 Current0;
extern u16 Current1;
void ReadINA226(void)
{
	u16 shunt, bus, cal;
	char strbuf[64]={0};
	//	u16 current;
	u8 buf[2];
	u32 tmp;

	soft_i2c_read(&I2C_INA226, SHUNT_V_REG, buf, 2);
	shunt = buf[0] * 256 + buf[1];
	//Trace("shunt",shunt);

	soft_i2c_read(&I2C_INA226, BUS_V_REG, buf, 2);
	bus = buf[0] * 256 + buf[1];

	//soft_i2c_read(&I2C_INA226,CURRENT_REG,buf,2);
	//current = buf[0]*256 + buf[1];

	soft_i2c_read(&I2C_INA226, CAL_REG, buf, 2);
	cal = buf[0] * 256 + buf[1];

	//Trace("cal",cal);

	MeasPara.BusVoltage = (bus * 10) / 8;						// mV
	//	MeasPara.WorkCurrent = (shunt * cal) / 2048;  				// 0.1mA
	tmp = (shunt * cal) / 2048;  				// 0.1mA


	MeasPara.WorkCurrent = 0;
	MeasPara.SleepCurrent = 0;


	if (GetTestStage() == TEST_WORK) {
		MeasPara.WorkCurrent = tmp;
		MeasPara.SleepCurrent = 0;
	}
	if (GetTestStage() == TEST_SLEEP) {
		MeasPara.WorkCurrent = 0;
		MeasPara.SleepCurrent = tmp;
	}



	if (GetMainBoard_State() == MAINBOARD_WORK) {
		MeasPara.WorkCurrent = tmp;
		MeasPara.SleepCurrent = 0;

		Current0 = MeasPara.WorkCurrent;
	}

	if (GetMainBoard_State() == MAINBOARD_OFF) {
		MeasPara.WorkCurrent = 0;
		MeasPara.SleepCurrent = tmp;
		Current1 = MeasPara.SleepCurrent;
	}

	MeasPara.Power = (MeasPara.WorkCurrent * MeasPara.BusVoltage) / 10000;				// mW
	//sprintf(strbuf,"%4dv, current: %4d,power=%4d\r\n",MeasPara.BusVoltage,MeasPara.WorkCurrent,MeasPara.Power);
	//TraceStr(strbuf);
}

