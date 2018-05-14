/***************************************************/
// Port A
#define		SPI1_CS_PIN			    GPIO_Pin_4
#define		SPI1_SCK_PIN			GPIO_Pin_5
#define		SPI1_MISO_PIN			GPIO_Pin_6
#define		SPI1_MOSI_PIN			GPIO_Pin_7
#define		SPI1_PORT				GPIOA




/***************************************************/
// Port B



#define 	UART1_TX						GPIO_Pin_6
#define 	UART1_RX						GPIO_Pin_7
#define 	UART1_PORT						GPIOB



//POWER
#define		POWER_PIN						GPIO_Pin_9
#define		POWER_PORT						GPIOB





//I2C2


#define		I2C2_SCL_PIN						GPIO_Pin_10
#define		I2C2_SDA_PIN						GPIO_Pin_11
#define		I2C2_SDA_PORT						GPIOB
#define		I2C2_SCL_PORT						GPIOB




//CAN 2
#define 	GPIO_CAN2							GPIOB
#define 	GPIO_Pin_CAN2_RX 					GPIO_Pin_12
#define 	GPIO_Pin_CAN2_TX 					GPIO_Pin_13
#define 	RCC_APB2Periph_GPIO_CAN2   			RCC_APB2Periph_GPIOB
//#define GPIO_Remapping_CAN1				 GPIO_Remap1_CAN1




/***************************************************/
// Port C



#define 	LED7_PIN				GPIO_Pin_6
#define 	LED8_PIN				GPIO_Pin_7

#define 	LED7_PORT				GPIOC
#define 	LED8_PORT				GPIOC




/***************************************************/
// Port D

//CAN1
#define 	GPIO_Remapping_CAN1				 	GPIO_Remap_PD01
#define 	GPIO_CAN1							GPIOD
#define 	GPIO_Pin_CAN1_RX 					GPIO_Pin_0
#define 	GPIO_Pin_CAN1_TX 					GPIO_Pin_1
#define 	RCC_APB2Periph_GPIO_CAN1   			RCC_APB2Periph_GPIOD


#define  	LED1_PIN				GPIO_Pin_10
#define 	LED2_PIN				GPIO_Pin_11
#define 	LED3_PIN				GPIO_Pin_12
#define 	LED4_PIN				GPIO_Pin_13
#define 	LED5_PIN				GPIO_Pin_14
#define 	LED6_PIN				GPIO_Pin_15




#define 	LED1_PORT				GPIOD
#define 	LED2_PORT				GPIOD
#define 	LED3_PORT				GPIOD
#define 	LED4_PORT				GPIOD
#define 	LED5_PORT				GPIOD
#define 	LED6_PORT				GPIOD




#define 	BOOT_PIN				GPIO_Pin_2
#define 	BOOT_PORT				GPIOD




/***************************************************/

// Uart2, mcu_tx,mcu_rx connect to test board







#define 	TRACE_UART_BAUDRATE      115200 //460800
#define 	TRACE_UART_TX_DMA					DMA1_Channel4


#define 	COMM_UART								UART4
#define 	COMM_UART_BAUDRATE					115200
#define		COMM_UART_TX_DMA					DMA2_Channel5
#define		COMM_UART_RX_DMA					DMA2_Channel3


#define 	WifiTest_UART							USART2
#define 	WifiTest_UART_BAUDRATE					115200
#define		WifiTest_UART_TX_DMA					DMA1_Channel7
#define		WifiTest_UART_RX_DMA					DMA1_Channel6

