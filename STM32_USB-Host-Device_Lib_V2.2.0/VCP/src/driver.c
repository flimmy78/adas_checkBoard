
#include "inc_all.h"


/******************************************************************************
* @file    		 
* @author  		Gary
* @version 		 
* @date    		 
* @brief  		  
*		   		
******************************************************************************/






/****************************************************************************/
/*External  Variables */
extern can_para_t can_para_500k,can_para_100k;
extern u8  comm_uart_rx_buffer[MAX_COMM_UART_DMA_RCV_SIZE];

/****************************************************************************/




/****************************************************************************/
/*External  Functions */
extern void CAN1_Config(can_para_t *can_para);
extern void CAN2_Config(can_para_t *can_para);

/****************************************************************************/



/****************************************************************************/
/*Global  Variables */

/****************************************************************************/



/****************************************************************************/
/*Global  Functions */

/****************************************************************************/


/****************************************************************************/
/*Local  Variables*/

/****************************************************************************/


/****************************************************************************/
/*Local  Functions*/
static void Init_GPIO(void);
static void NVIC_Config(void);
static void  drv_timer2_init(void);
/****************************************************************************/


/////////////////////////////////////////////////////////////////////////////////////////////////


static void Init_GPIO(void)
{
	
	GPIO_InitTypeDef  GPIO_InitStructure;

	/* Enable the GPIO_LED Clock */

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);


	/* Configure the GPIO_LED pin */
	GPIO_InitStructure.GPIO_Pin = LED1_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(LED1_PORT, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = LED2_PIN;
	GPIO_Init(LED2_PORT, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = LED3_PIN;
	GPIO_Init(LED3_PORT, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = LED4_PIN;
	GPIO_Init(LED4_PORT, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = LED5_PIN;
	GPIO_Init(LED5_PORT, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = LED6_PIN;
	GPIO_Init(LED6_PORT, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = LED7_PIN;
	GPIO_Init(LED7_PORT, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = LED8_PIN;
	GPIO_Init(LED8_PORT, &GPIO_InitStructure);

	GPIO_ResetBits(LED1_PORT,LED1_PIN);
	GPIO_ResetBits(LED2_PORT,LED2_PIN);
	GPIO_ResetBits(LED3_PORT,LED3_PIN);
	GPIO_ResetBits(LED4_PORT,LED4_PIN);
	GPIO_ResetBits(LED5_PORT,LED5_PIN);
	GPIO_ResetBits(LED6_PORT,LED6_PIN);
	GPIO_ResetBits(LED7_PORT,LED7_PIN);
	GPIO_ResetBits(LED8_PORT,LED8_PIN);






	/* Configure the POWER pin */
	GPIO_InitStructure.GPIO_Pin = POWER_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(POWER_PORT, &GPIO_InitStructure);

	GPIO_ResetBits(POWER_PORT,POWER_PIN);

	/* Configure the BOOT pin */
	GPIO_InitStructure.GPIO_Pin = BOOT_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_Init(BOOT_PORT, &GPIO_InitStructure);

	GPIO_ResetBits(BOOT_PORT,BOOT_PIN);

	
}

static void NVIC_Config(void)
{
	NVIC_InitTypeDef  NVIC_InitStructure;

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);


	NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;  
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 12;//0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);  


	NVIC_InitStructure.NVIC_IRQChannel = OTG_FS_IRQn;  
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 13;//1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);  


	NVIC_InitStructure.NVIC_IRQChannel = CAN1_RX0_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 14;//2;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	NVIC_InitStructure.NVIC_IRQChannel = CAN2_RX1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 15;//3;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);


	NVIC_InitStructure.NVIC_IRQChannel = UART4_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 11;//3;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);






	
}





static void  drv_timer2_init(void)
{
	/* system clock = 72Mhz */
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStruct;

	TIM_DeInit(TIM2);
	/* timer6:		uart receive timer out */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2,ENABLE);  //  
	TIM_TimeBaseInitStruct.TIM_Period = TIM_MAX_TICKS-1; 	//500ms 
	TIM_TimeBaseInitStruct.TIM_Prescaler = 720-1; 	// 10us
	TIM_TimeBaseInitStruct.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseInitStruct.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInitStruct.TIM_RepetitionCounter = 0x0000;
	TIM_TimeBaseInit(TIM2,&TIM_TimeBaseInitStruct);
	TIM_ClearFlag(TIM2, TIM_FLAG_Update); 	
	TIM_ITConfig(TIM2,TIM_IT_Update,ENABLE);
	TIM_Cmd(TIM2,ENABLE);
}


void drv_delayus(u32 us)
{
	int i,j;
	for (i=0;i<us;i++)
	for (j=0;j<10;j++)
	;
}

void drv_delayms(u32 ms)
{
	int i;
	for (i=0;i<ms;i++) drv_delayus(1000);
}



void drv_trace_uart_init(void)   
{

	DMA_InitTypeDef DMA_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;

		/* config USART1 clock */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE); 

	/* USART1 GPIO config */
	/* Configure USART1 Tx (PB6) as alternate function push-pull */
	GPIO_InitStructure.GPIO_Pin = UART1_TX;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(UART1_PORT, &GPIO_InitStructure);

	/* Configure USART1 Rx (PB7) as input floating */
	GPIO_InitStructure.GPIO_Pin = UART1_RX;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(UART1_PORT, &GPIO_InitStructure);


	GPIO_PinRemapConfig(GPIO_Remap_USART1, ENABLE);

	
    
	/* Trace send dma */
    DMA_DeInit(TRACE_UART_TX_DMA);  
    DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)(&USART1->DR);  
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)0;   
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;  
    //设置DMA在传输时缓冲区的长度  
    DMA_InitStructure.DMA_BufferSize = 1200;  
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;  
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;  
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;  
    DMA_InitStructure.DMA_MemoryDataSize = DMA_PeripheralDataSize_Byte;  
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;  
    DMA_InitStructure.DMA_Priority = DMA_Priority_High;  
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;  
    DMA_Init(TRACE_UART_TX_DMA,&DMA_InitStructure);  
   // DMA_ITConfig(TRACE_UART_TX_DMA,DMA_IT_TC,DISABLE);  
      
    //使能通道  
    //DMA_Cmd(DMA1_Channel4, ENABLE);  
  


	/* USART1 mode config */
	USART_InitStructure.USART_BaudRate = TRACE_UART_BAUDRATE;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No ;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Tx;


	USART_Init(USART1, &USART_InitStructure); 
	
     
    //采用DMA方式发送  
   // USART_DMACmd(USART1,USART_DMAReq_Tx,ENABLE);  

    //启动串口    
    USART_Cmd(USART1, ENABLE);
}


//#if 0
void drv_wifiTest_uart_init(void)   
{ 

	DMA_InitTypeDef DMA_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;

	/* config USART2 clock */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE); 

	/* USART1 GPIO config */
	/* Configure USART2 Tx (PA.02) as alternate function push-pull */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/* Configure USART2 Rx (PA.03) as input floating */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
     

	/* Trace send dma */
    DMA_DeInit(WifiTest_UART_TX_DMA);  
    DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)(&USART2->DR);  
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)0;   
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;  
    //设置DMA在传输时缓冲区的长度  
    DMA_InitStructure.DMA_BufferSize = 0;  
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;  
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;  
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;  
    DMA_InitStructure.DMA_MemoryDataSize = DMA_PeripheralDataSize_Byte;  
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;  
    DMA_InitStructure.DMA_Priority = DMA_Priority_High;  
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;  
    DMA_Init(WifiTest_UART_TX_DMA,&DMA_InitStructure);  
      

	 /* Now uart rx dma work in circle mode */
	 DMA_DeInit(WifiTest_UART_RX_DMA);  
	 //外设地址  
	 DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)(&USART2->DR);  
	 //内存地址  
	 DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)0;  
	 //dma传输方向单向	
	 DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;  
	 //设置DMA在传输时缓冲区的长度	
	 DMA_InitStructure.DMA_BufferSize = MAX_COMM_UART_DMA_RCV_SIZE;  
	 //设置DMA的外设递增模式，一个外设	
	 DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;  
	 //设置DMA的内存递增模式  
	 DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;  
	 //外设数据字长  
	 DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;  
	 //内存数据字长  
	 DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;  
	 //设置DMA的传输模式  
	 DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;  
	 //设置DMA的优先级别  
	 DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;  
	 //设置DMA的2个memory中的变量互相访问  
	 DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;  
	 DMA_Init(WifiTest_UART_RX_DMA,&DMA_InitStructure);  
	
	 //使能通道  
	// DMA_Cmd(DMA1_Channel3,ENABLE);  
	   
	/* USART2 mode config */
	USART_InitStructure.USART_BaudRate = WifiTest_UART_BAUDRATE;
	USART_InitStructure.USART_WordLength = USART_WordLength_9b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No ;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
	USART_Init(USART2, &USART_InitStructure); 
    USART_Cmd(USART2, ENABLE);
}
//#endif


void drv_comm_uart_init(void)   
{

	DMA_InitTypeDef DMA_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;

	/* config UART4 clock */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART4, ENABLE);
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA2, ENABLE); 

	USART_DeInit(UART4);

	/* USART1 GPIO config */
	/* Configure USART4 Tx () as alternate function push-pull */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	/* Configure USART4 Rx () as input floating */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	
     

	
    DMA_DeInit(COMM_UART_TX_DMA);  
    DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)(&UART4->DR);  
    DMA_InitStructure.DMA_MemoryBaseAddr = (u32)0;   
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;  
    //设置DMA在传输时缓冲区的长度  
    DMA_InitStructure.DMA_BufferSize = 0;  
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;  
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;  
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;  
    DMA_InitStructure.DMA_MemoryDataSize = DMA_PeripheralDataSize_Byte;  
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;  
    DMA_InitStructure.DMA_Priority = DMA_Priority_High;  
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;  
    DMA_Init(COMM_UART_TX_DMA,&DMA_InitStructure);  
      

	 /* Now uart rx dma work in circle mode */
	 DMA_DeInit(COMM_UART_RX_DMA);  
	 //外设地址  
	 DMA_InitStructure.DMA_PeripheralBaseAddr = 0x40004c04;//(u32)(&UART4->DR);  
	 //内存地址  
	 DMA_InitStructure.DMA_MemoryBaseAddr = (u32)comm_uart_rx_buffer;  
	 //dma传输方向单向	
	 DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;  
	 //设置DMA在传输时缓冲区的长度	
	 DMA_InitStructure.DMA_BufferSize = MAX_COMM_UART_DMA_RCV_SIZE;  
	 //设置DMA的外设递增模式，一个外设	
	 DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;  
	 //设置DMA的内存递增模式  
	 DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;  
	 //外设数据字长  
	 DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;  
	 //内存数据字长  
	 DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;  
	 //设置DMA的传输模式  
	 DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;  
	 //设置DMA的优先级别  
	 DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;  
	 //设置DMA的2个memory中的变量互相访问  
	 DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;  
	 DMA_Init(COMM_UART_RX_DMA,&DMA_InitStructure);  
	
	 //使能通道  
	// DMA_Cmd(DMA1_Channel3,ENABLE);  

	{
		/* USART1 mode config */
		USART_InitStructure.USART_BaudRate = 256000;//460800;//256000;//115200;//921600;
		USART_InitStructure.USART_WordLength = USART_WordLength_9b;//USART_WordLength_9b;
		USART_InitStructure.USART_StopBits = USART_StopBits_1;
		USART_InitStructure.USART_Parity = USART_Parity_Even;
		USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
		USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
	}




	USART_Init(UART4, &USART_InitStructure); 
	USART_ITConfig(UART4,USART_IT_TC,DISABLE);  
	USART_ITConfig(UART4,USART_IT_RXNE,DISABLE);  
	USART_ITConfig(UART4,USART_IT_IDLE,ENABLE);

	USART_Cmd(UART4, ENABLE);

	
	
	//USART_DMACmd(UART4,USART_DMAReq_Tx,ENABLE);  
	USART_DMACmd(UART4,USART_DMAReq_Rx,ENABLE);

	DMA_Cmd(COMM_UART_RX_DMA, ENABLE); 
	//DMA_Cmd(COMM_UART_TX_DMA, ENABLE); 




	
}


void drv_comm_uart_init_ChangeSetting(void)
{

	DMA_InitTypeDef DMA_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;

	/* config UART4 clock */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART4, ENABLE);
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA2, ENABLE); 

	USART_DeInit(UART4);

	/* USART1 GPIO config */
	/* Configure USART4 Tx () as alternate function push-pull */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	/* Configure USART4 Rx () as input floating */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	
     

	
    DMA_DeInit(COMM_UART_TX_DMA);  
    DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)(&UART4->DR);  
    DMA_InitStructure.DMA_MemoryBaseAddr = (u32)0;   
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;  
    //设置DMA在传输时缓冲区的长度  
    DMA_InitStructure.DMA_BufferSize = 0;  
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;  
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;  
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;  
    DMA_InitStructure.DMA_MemoryDataSize = DMA_PeripheralDataSize_Byte;  
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;  
    DMA_InitStructure.DMA_Priority = DMA_Priority_High;  
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;  
    DMA_Init(COMM_UART_TX_DMA,&DMA_InitStructure);  
      

	 /* Now uart rx dma work in circle mode */
	 DMA_DeInit(COMM_UART_RX_DMA);  
	 //外设地址  
	 DMA_InitStructure.DMA_PeripheralBaseAddr = 0x40004c04;//(u32)(&UART4->DR);  
	 //内存地址  
	 DMA_InitStructure.DMA_MemoryBaseAddr = (u32)comm_uart_rx_buffer;  
	 //dma传输方向单向	
	 DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;  
	 //设置DMA在传输时缓冲区的长度	
	 DMA_InitStructure.DMA_BufferSize = MAX_COMM_UART_DMA_RCV_SIZE;  
	 //设置DMA的外设递增模式，一个外设	
	 DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;  
	 //设置DMA的内存递增模式  
	 DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;  
	 //外设数据字长  
	 DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;  
	 //内存数据字长  
	 DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;  
	 //设置DMA的传输模式  
	 DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;  
	 //设置DMA的优先级别  
	 DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;  
	 //设置DMA的2个memory中的变量互相访问  
	 DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;  
	 DMA_Init(COMM_UART_RX_DMA,&DMA_InitStructure);  
	
	 //使能通道  
	// DMA_Cmd(DMA1_Channel3,ENABLE);  

	{
		/* USART1 mode config */
		USART_InitStructure.USART_BaudRate = 460800;
		USART_InitStructure.USART_WordLength = USART_WordLength_8b;
		USART_InitStructure.USART_StopBits = USART_StopBits_1;
		USART_InitStructure.USART_Parity = USART_Parity_No;
		USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
		USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
	}




	USART_Init(UART4, &USART_InitStructure); 
	USART_ITConfig(UART4,USART_IT_TC,DISABLE);  
	USART_ITConfig(UART4,USART_IT_RXNE,DISABLE);  
	USART_ITConfig(UART4,USART_IT_IDLE,ENABLE);

	USART_Cmd(UART4, ENABLE);

	
	
	//USART_DMACmd(UART4,USART_DMAReq_Tx,ENABLE);  
	USART_DMACmd(UART4,USART_DMAReq_Rx,ENABLE);

	DMA_Cmd(COMM_UART_RX_DMA, ENABLE); 
	//DMA_Cmd(COMM_UART_TX_DMA, ENABLE); 
	

}


void Driver_Init(void)
{

	Init_GPIO();

	CAN1_Config(&can_para_100k);
	CAN2_Config(&can_para_100k);

	drv_timer2_init();
	NVIC_Config();
	drv_trace_uart_init();
	drv_comm_uart_init();
	drv_wifiTest_uart_init();
}


void drv_power_on(void)
{

	GPIO_SetBits(POWER_PORT,POWER_PIN);
	//GPIO_InitTypeDef GPIO_InitStructure;
	//RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE, ENABLE); 
	
	//GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
	//GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	//GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	//GPIO_Init(GPIOE, &GPIO_InitStructure);
	//GPIO_ResetBits(GPIOE,GPIO_Pin_6);
}


void drv_power_off(void)
{
	GPIO_ResetBits(POWER_PORT,POWER_PIN);
	
	//GPIO_InitTypeDef GPIO_InitStructure;
	//RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE, ENABLE); 
	
	//GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
	//GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	//GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	//GPIO_Init(GPIOE, &GPIO_InitStructure);
	//GPIO_ResetBits(GPIOE,GPIO_Pin_6);
}

void drv_boot_high(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = BOOT_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_Init(BOOT_PORT, &GPIO_InitStructure);
	GPIO_SetBits(BOOT_PORT,BOOT_PIN);
}

void drv_boot_low(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = BOOT_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_Init(BOOT_PORT, &GPIO_InitStructure);
	GPIO_ResetBits(BOOT_PORT,BOOT_PIN);
}







