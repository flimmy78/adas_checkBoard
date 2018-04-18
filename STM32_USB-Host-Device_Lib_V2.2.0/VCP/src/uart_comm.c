
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

extern QueueHandle_t Test_Queue;
extern QueueHandle_t Usb_Queue;	

/****************************************************************************/




/****************************************************************************/
/*External  Functions */

extern void UsbSendData(void);
extern u8 CheckFIFOByte(FIFO_t *stFIFO);
/****************************************************************************/



/****************************************************************************/
/*Global  Variables */

FIFO_t		UartRxFifo;

volatile bool g_check_uart_comm_msg_flag = true;

QueueHandle_t Uart_Queue;	

__ALIGN_BEGIN 
	
u8  comm_uart_rx_buffer[MAX_COMM_UART_DMA_RCV_SIZE];
u8  comm_uart_tx_buffer[MAX_COMM_UART_DMA_SND_SIZE];
u8  UartRcvFrameBuffer[MAX_UART_FRAME_LENGTH];

__ALIGN_END;

/****************************************************************************/



/****************************************************************************/
/*Global  Functions */

/****************************************************************************/


/****************************************************************************/
/*Local  Variables*/




static u8 uart_dma_start = 0;
static bool firmware_mode = true;
/****************************************************************************/


/****************************************************************************/
/*Local  Functions*/


static bool uart_DMA_finished(void);
static void UartSendData(msg_head_t *msg_head);
static bool uart_DMA_finished(void);
static void MsgSendFromUart(comm_head_t *uart_msg_head,task_id_enum totask);
static void uart_start_TX_DMA(u32 addr,u32 len);

static void UartCommServiceFile(comm_head_t *msg_head);
static void UartCommServiceWarn(comm_head_t *msg_head);
static void UartCommServiceCmd(comm_head_t *msg_head);

/****************************************************************************/






static void UartReadFile(comm_head_t *msg_head)
{
	
}

static void UartWriteFile(comm_head_t *msg_head)
{
	
}



/* Comm func */
void UartCommFuncs(u8 *framebuf)
{
		comm_head_t *msg_head;
		msg_head = (comm_head_t *)framebuf;
		switch(msg_head->service_type)
		{
			case SERVICE_TYPE_FILE:
				UartCommServiceFile(msg_head);
				break;
			case SERVICE_TYPE_WARN:
				UartCommServiceWarn(msg_head);
				break;
			case SERVICE_TYPE_CMD:
				UartCommServiceCmd(msg_head);
			default:
				break;
		}
}	

/* Comm func */
void UartFirmFuncs(u8 *framebuf,u16 len)
{
	u32 i;
	memset(UartRcvFrameBuffer,0,MAX_UART_FRAME_LENGTH);
	for (i=0;i<len;i++)
		sprintf(&UartRcvFrameBuffer[i*3],"%02d ",framebuf[i]);
	TraceStr(UartRcvFrameBuffer);
}	


static void UartCommServiceFile(comm_head_t *msg_head)
{
		u8 msg_type;
		msg_type = msg_head->msg_type;
		switch(msg_type)
		{
			case MSG_TYPE_FILE_READ_REQ:
				UartReadFile(msg_head);
				break;
			case MSG_TYPE_FILE_WRITE_REQ:
				UartWriteFile(msg_head);
				break;
			default:
				break;
		}








	
}

static void UartCommServiceWarn(comm_head_t *msg_head)
{
	
}
		

static void UartCommServiceCmd(comm_head_t *msg_head)
{
		u8 msg_type;
		msg_type = msg_head->msg_type;
		switch(msg_type)
		{
			case MSG_TYPE_CMD_START_TEST_REQ:
				MsgSendFromUart(msg_head,TASK_TEST);
				break;
			case MSG_TYPE_FILE_WRITE_REQ:
				
				break;
			default:
				break;
		}	
}
	






static void CheckFrameHeaderFromUartFIFO(FIFO_t *stFIFO, u8 *framebuf)
{
	u8 i;
	u32 tmp;
	tmp = stFIFO->head;   /* Remember head */
	for (i=0;i<(UART_HEAD_LENGTH);i++)
	{
		framebuf[i]  = DeFIFO(stFIFO);
	}
	/*Restore head */
	stFIFO->head = tmp;
}



bool RcvFrameFromUart(FIFO_t *stFIFO, u8 *framebuf)
{
	/* Find a head */
	u16 frame_len,i;
	comm_head_t *msg_head;
	
	while (1)
	{
		if (FIFOLen(stFIFO)<(UART_HEAD_LENGTH)) return false;  /* Frame not receive finished, return */
		if (CheckFIFOByte(stFIFO)==(UART_HEAD_START_BYTES&0xFF))
		{

				/* Check it is a head */
				CheckFrameHeaderFromUartFIFO(stFIFO,framebuf);
				msg_head = (comm_head_t *)framebuf;
				frame_len = msg_head->msg_len;
				if (msg_head->start_bytes != UART_HEAD_START_BYTES) 
				{
					DeFIFO(stFIFO);
					continue;
				}
				if ((frame_len>=MAX_UART_FRAME_LENGTH) || (frame_len<MIN_UART_FRAME_LENGTH)) /* Something error,but head is matched */
				{
					/* Remove head,four bytes */
					DeFIFO(stFIFO);
					DeFIFO(stFIFO);
					DeFIFO(stFIFO);
					DeFIFO(stFIFO);
				}
				if (frame_len>FIFOLen(stFIFO)) return false; /*Not receive finished, return */
				/* Ok, now we copy frame from fifo and change fifo pointer */
				for (i=0;i<frame_len;i++) 
					framebuf[i] = DeFIFO(stFIFO);
				return true;
		}
		else 
		{
			DeFIFO(stFIFO);  /* Throw one byte */
			continue;
		}
	}
}


u16 RcvFirmFrameFromUart(FIFO_t *stFIFO, u8 *framebuf)
{

	u16 i,len;

	len = FIFOLen(stFIFO);
	if (len == 0 ) return 0;  /* Frame not receive finished, return */

	for (i=0;i<FIFOLen(stFIFO);i++) 
		framebuf[i] = DeFIFO(stFIFO);
	
	return len;
		
}







void InitUartVar(void)
{
	
	//Uart FIFO
	InitFIFO(&UartRxFifo,comm_uart_rx_buffer,MAX_COMM_UART_DMA_RCV_SIZE);
	Uart_Queue = xQueueCreate(20,sizeof(msg_queue_t));
}


void uart_comm_task(void *taskpara)
{
	msg_queue_t msg_queue;
	msg_head_t *msg_head;
	u32 msg_type;
	u16 len;

  /* Main loop */
  while (1)
  {
		if (g_check_uart_comm_msg_flag)
		{
		//	g_check_uart_comm_msg_flag = false;
			if (firmware_mode)
			{
				//len = RcvFirmFrameFromUart(&UartRxFifo,UartRcvFrameBuffer);
				if (len > 0)
				{
					//UartFirmFuncs(UartRcvFrameBuffer,len);
				}
			}
			else
			{
				if (RcvFrameFromUart(&UartRxFifo,UartRcvFrameBuffer))
				{
					UartCommFuncs(UartRcvFrameBuffer);
				}
			}
		}
		if (xQueueReceive(Uart_Queue,&msg_queue,10)==pdTRUE)
		{
			//Send data to uart
			msg_head = (msg_head_t *)msg_queue.msg_addr;
			msg_type = (msg_head->snd_task_id << 8) | msg_head->rcv_task_id;
			switch(msg_type)
			{
				case MSG_USB_UART:
					UartSendData(msg_head);
					break;
				default:
					break;
			}
			vPortFree(msg_head);
		}


  }



}

static void UartSendData(msg_head_t *msg_head)
{
	

	u32 i,len;	
	comm_head_t *comm_head;
	u8 *p;
	
	if (uart_dma_start) while (!uart_DMA_finished()) vTaskDelay(1);

	uart_dma_start = 1;

	if (msg_head->msg_data == NULL) return;

	comm_head = (comm_head_t *)msg_head->msg_data;
	len = comm_head->msg_len;
	p = (u8 *)msg_head->msg_data;
	//Copy data
	for (i=0;i<len;i++)
	{
		comm_uart_tx_buffer[i] = *p++;
	}

	uart_start_TX_DMA((u32)&comm_uart_tx_buffer,len);	

}


static bool uart_DMA_finished(void)
{

	if ((DMA_GetFlagStatus(DMA1_FLAG_TC7)==SET)  || (DMA_GetCurrDataCounter(COMM_UART_TX_DMA)==0))
	{
		DMA_ClearFlag(DMA1_FLAG_TC7);		
		DMA_Cmd(COMM_UART_TX_DMA,DISABLE);
		USART_ClearFlag(COMM_UART,USART_IT_TC);
		return true;
	}
	else return false;
}


static void uart_start_TX_DMA(u32 addr,u32 len)
{
	COMM_UART_TX_DMA->CMAR = addr;
	COMM_UART_TX_DMA->CNDTR = len;
	DMA_Cmd(COMM_UART_TX_DMA, ENABLE);	
	USART_DMACmd(COMM_UART,USART_DMAReq_Tx,ENABLE);  

}


static void MsgSendFromUart(comm_head_t *uart_msg_head,task_id_enum totask)
 {
 
	 msg_queue_t msg_queue;
	 msg_head_t *msg_head;
	 void *msg_addr = NULL;
	 u32 malloc_size;

 
	 /*1. Malloc memory */
	 malloc_size = CALC_MALLOC_SIZE(sizeof(msg_head_t) + uart_msg_head->msg_len);
	 msg_addr = pvPortMalloc(malloc_size);
 
	 if(msg_addr==NULL)
	 {
		 TraceStr("Malloc msgaddr error!\r\n");
		 return;
	 }
	 
	 /*2. Fill header */
	 msg_head = (msg_head_t *)msg_addr;
	 msg_head->snd_task_id = TASK_UART;
	 msg_head->rcv_task_id = totask;
	 msg_head->cmd = 0; 
	 msg_head->msg_data = (u8 *)((u32)msg_addr + sizeof(msg_head_t)); 			

	 /* 3. Copy data */
	memcpy(msg_head->msg_data,&uart_msg_head->start_bytes,uart_msg_head->msg_len);

 
	 /* Here we send msg  */
	 msg_queue.msg_addr = (u32)msg_addr;
	 
	 switch(totask)
	 {
	 	case TASK_USB:
			if (xQueueSend(Usb_Queue, &msg_queue, 20) != pdTRUE)
			{
			   vPortFree(msg_addr);
			}
			break;
		case TASK_TEST:
			if (xQueueSend(Test_Queue, &msg_queue, 20) != pdTRUE)
			{
			   vPortFree(msg_addr);
			}
			break;
		default:
			break;
	 }


 }


void FirmSendData(u8 *buf,u16 len)
 {
	 u32 i;  
	 
	 if (uart_dma_start) while (!uart_DMA_finished()) vTaskDelay(1);
 
	 uart_dma_start = 1;
 
	 if (buf == NULL) return;
	 //Copy data
	 for (i=0;i<len;i++)
	 {
		 comm_uart_tx_buffer[i] = buf[i];
	 }
 
	 uart_start_TX_DMA((u32)&comm_uart_tx_buffer,len);	 
 
 }

void ReadFromUartRxFifo(u8 *buf, u16 *len)
{



	
}
