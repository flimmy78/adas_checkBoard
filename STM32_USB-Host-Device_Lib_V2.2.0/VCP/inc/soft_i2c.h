


/***********************************************************************************
 *#defines
 ***********************************************************************************/
//stm32f4

#define ERROR_CODE_TRUE			0
#define ERROR_CODE_FALSE		1
#define ERROR_CODE_WRITE_ADDR	10
#define ERROR_CODE_WRITE_DATA	20
#define ERROR_CODE_READ_ADDR	30
#define ERROR_CODE_READ_DATA	40
#define ERROR_CODE_START_BIT	50
#define ERROR_CODE_APROCESS		60
#define ERROR_CODE_DENY			70


/***********************************************************************************/
// * Variables
 
 typedef struct 
 {
	 
	 void	 (*scl_low)(void);
	 void	 (*scl_high)(void);
	 void	 (*sda_low)(void);
	 void	 (*sda_high)(void);  
	 void	 (*sda_dir_in)(void);
	 void	 (*sda_dir_out)(void);
	 u8 	 (*sda_value)(void);	 
	 u8 	 (*scl_value)(void);
	 void	 (*delay1)(void);			//5 us
	 void	 (*delay2)(void);			// 100us
	 u8 	 addr;
	 u8 	 clock_stretch;
 }I2C_DEV_t;


// ***********************************************************************************/

/***********************************************************************************
 * External Variables
 ***********************************************************************************/

/***********************************************************************************
 * Function Prototypes
 ***********************************************************************************/

unsigned char soft_i2c_write(I2C_DEV_t *i2c_dev, unsigned char subaddr, unsigned char *buff, int bytenum);
unsigned char soft_i2c_read(I2C_DEV_t *i2c_dev, unsigned char subaddr, unsigned char *buff, int bytenum);

/***********************************************************************************
 * External Function
 ***********************************************************************************/

/* EOF */

