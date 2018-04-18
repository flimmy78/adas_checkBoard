
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
extern  FIFO_t 	UsbTxFifo;
extern 	FIFO_t	UsbRxFifo;
extern  uint32_t APP_Tx_ptr_out,APP_Tx_ptr_in;

extern QueueHandle_t Test_Queue;	
extern QueueHandle_t Uart_Queue;	

/****************************************************************************/




/****************************************************************************/
/*External  Functions */

extern void UsbSendData(void);
extern void CAN1_Config(can_para_t *can_para);
extern void CAN2_Config(can_para_t *can_para);
extern u8 CheckFIFOByte(FIFO_t *stFIFO);
extern u16  FIFOFreeLen( FIFO_t *stFIFO);
/****************************************************************************/



/****************************************************************************/
/*Global  Variables */

volatile bool g_check_usb_comm_msg_flag = true;

__ALIGN_BEGIN uint8_t APP_Tx_Buffer[APP_TX_DATA_SIZE] __ALIGN_END ; 

FIFO_t				UsbTxFifo;
FIFO_t				UsbRxFifo;


QueueHandle_t Usb_Queue;	

/****************************************************************************/



/****************************************************************************/
/*Global  Functions */

/****************************************************************************/


/****************************************************************************/
/*Local  Variables*/
__ALIGN_BEGIN static u8 TxFrameBuff[TMP_FRAME_BUFF_SIZE] __ALIGN_END;

static u8 				UsbRcvFrameBuffer[MAX_USB_FRAME_LENGTH];

__ALIGN_BEGIN static u8 APP_Rx_Buffer[APP_RX_DATA_SIZE]  __ALIGN_END; 

/****************************************************************************/


/****************************************************************************/
/*Local  Functions*/

static void MsgSendFromUsb(comm_head_t *usb_msg_head,task_id_enum totask );

static void UsbCommServiceFile(comm_head_t *msg_head);
static void UsbCommServiceWarn(comm_head_t *msg_head);
static void UsbCommServiceCmd(comm_head_t *msg_head);

/****************************************************************************/










void SendFrame(u8 *buf,u16 len)
{
		if (FIFOFreeLen(&UsbTxFifo) <= len)			// Large buffer
		{
			while (1)
			{
				UsbTxFifo.head = APP_Tx_ptr_out;
				if (FIFOFreeLen(&UsbTxFifo) <= len)
					vTaskDelay(5);
				else break;
			}
		}
		
		UsbTxFifo.head = APP_Tx_ptr_out;
		EnFIFOBuf(&UsbTxFifo,buf,len);
		APP_Tx_ptr_in = UsbTxFifo.rear;
		UsbSendData();
		
}


/* Comm func */
static void UsbCommFuncs(u8 *framebuf)
{

		comm_head_t *msg_head;
//		u32 crc32;
		msg_head = (comm_head_t *)framebuf;
//		crc32 = GetCrc32(&framebuf[MSG_CRC_OFFSET],msg_head->msg_len-MSG_CRC_OFFSET);
	//	if (msg_head->crc32 != crc32 ) return;
		switch(msg_head->service_type)
		{
			case SERVICE_TYPE_FILE:
				UsbCommServiceFile(msg_head);
				break;
			case SERVICE_TYPE_WARN:
				UsbCommServiceWarn(msg_head);
				break;
			case SERVICE_TYPE_CMD:
				UsbCommServiceCmd(msg_head);
				break;

			default:
				break;
		}

}	


static void UsbReadFile(comm_head_t *msg_head)
{
	
}

static void UsbWriteFile(comm_head_t *msg_head)
{
	


	
}


static void UsbCommServiceFile(comm_head_t *msg_head)
{
		u8 msg_type;
		msg_type = msg_head->msg_type;
		switch(msg_type)
		{
			case MSG_TYPE_FILE_READ_REQ:
				UsbReadFile(msg_head);
				break;
			case MSG_TYPE_FILE_WRITE_REQ:
				UsbWriteFile(msg_head);
				break;
			default:
				break;
		}

}

static void UsbCommServiceWarn(comm_head_t *msg_head)
{
	
}
		
static void UsbCommServiceCmd(comm_head_t *msg_head)
{
		u8 msg_type;
		msg_type = msg_head->msg_type;
		switch(msg_type)
		{
			case MSG_TYPE_CMD_START_TEST_REQ:
				MsgSendFromUsb(msg_head,TASK_TEST);
				break;

			default:
				break;
		}	
}







static void CheckFrameHeaderFromUsbFIFO(FIFO_t *stFIFO, u8 *framebuf)
{
	u8 i;
	u32 tmp;
	tmp = stFIFO->head;   /* Remember head */
	for (i=0;i<(USB_HEAD_LENGTH);i++)
	{
		framebuf[i]  = DeFIFO(stFIFO);
	}
	/*Restore head */
	stFIFO->head = tmp;
}



bool RcvFrameFromUsb(FIFO_t *stFIFO, u8 *framebuf)
{
	/* Find a head */
	u16 frame_len,i;
	comm_head_t *msg_head;
	
	while (1)
	{
		if (FIFOLen(stFIFO)<(USB_HEAD_LENGTH)) return false;  /* Frame not receive finished, return */
		if (CheckFIFOByte(stFIFO)==(USB_HEAD_START_BYTES&0xFF))
		{

				/* Check it is a head */
				CheckFrameHeaderFromUsbFIFO(stFIFO,framebuf);
				msg_head = (comm_head_t *)framebuf;
				frame_len = msg_head->msg_len;
				if (msg_head->start_bytes != USB_HEAD_START_BYTES) 
				{
					DeFIFO(stFIFO);
					continue;
				}
				if ((frame_len>=MAX_USB_FRAME_LENGTH) || (frame_len<MIN_USB_FRAME_LENGTH)) /* Something error,but head is matched */
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


void InitUsbVar(void)
{
	//USB FIFO
	InitFIFO(&UsbTxFifo,APP_Tx_Buffer,APP_TX_DATA_SIZE);
	InitFIFO(&UsbRxFifo,APP_Rx_Buffer,APP_RX_DATA_SIZE);

	Usb_Queue = xQueueCreate(20,sizeof(msg_queue_t));
}


void usb_comm_task(void *taskpara)
{

  /* Main loop */
  while (1)
  {
		if (g_check_usb_comm_msg_flag)
		{
			g_check_usb_comm_msg_flag = false;
			if (RcvFrameFromUsb(&UsbRxFifo,UsbRcvFrameBuffer))
			{
				UsbCommFuncs(UsbRcvFrameBuffer);
			}
		}
		vTaskDelay(10);
  }
}


static void MsgSendFromUsb(comm_head_t *usb_msg_head,task_id_enum totask )
 {
 
	 msg_queue_t msg_queue;
	 msg_head_t *msg_head;
	 void *msg_addr = NULL;
	 u32 malloc_size;


	 
 
	 /*1. Malloc memory */
	 malloc_size = CALC_MALLOC_SIZE(sizeof(msg_head_t) + usb_msg_head->msg_len);
	 msg_addr = pvPortMalloc(malloc_size);
 
	 if(msg_addr==NULL)
	 {
		 TraceStr("Malloc msgaddr error!\r\n");
		 return;
	 }
	 
	 /*2. Fill header */
	 msg_head = (msg_head_t *)msg_addr;
	 msg_head->snd_task_id = TASK_USB;
	 msg_head->rcv_task_id = totask;
	 msg_head->cmd = 0; 
	 msg_head->msg_data = (u8 *)((u32)msg_addr + sizeof(msg_head_t)); 			

	 /* 3. Copy data */
	memcpy(msg_head->msg_data,&usb_msg_head->start_bytes,usb_msg_head->msg_len);

 
	 /* Here we send msg  */
	 msg_queue.msg_addr = (u32)msg_addr;

	 switch(totask)
	 {
	 	case TASK_UART:
			if (xQueueSend(Uart_Queue, &msg_queue, 20) != pdTRUE)
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

