#include "inc_all.h"






/****************************************************************************/
/*External  Variables */
extern QueueHandle_t Usb_Queue;	
extern QueueHandle_t Uart_Queue;	


/****************************************************************************/




/****************************************************************************/
/*External  Functions */
extern void ReadADC(void);
extern void ReadINA226(void);

extern void drv_power_on(void);
extern void drv_power_off(void);
/****************************************************************************/



/****************************************************************************/
/*Global  Variables */
QueueHandle_t Test_Queue;
/****************************************************************************/



/****************************************************************************/
/*Global  Functions */

/****************************************************************************/


/****************************************************************************/
/*Local  Variables*/

/****************************************************************************/


/****************************************************************************/
/*Local  Functions*/

/****************************************************************************/





void InitTestVar(void)
{
	
	//Uart FIFO
	Test_Queue = xQueueCreate(20,sizeof(msg_queue_t));
}


void test_task(void *taskparam)
{
	msg_queue_t msg_queue;
	msg_head_t *msg_head;
	u32 msg_type;
	
	init_INA226_normal();
	InitADC();
	drv_boot_high();
	vTaskDelay(10);
	drv_power_on();
	
	vTaskDelay(1000);
	FirmTest();


	#if 1
	while (1)
	{
			LED1_PORT->ODR ^= LED1_PIN;
			LED2_PORT->ODR ^= LED2_PIN;


		//	ReadADC();
		//	ReadINA226();
		//	vTaskDelay(500);
	}
	#endif


	#if 0
	/* Main loop */
	while (1)
	{
		if (g_check_uart_comm_msg_flag)
		{
			g_check_uart_comm_msg_flag = false;
			if (firmware_mode)
			{
				
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
	#endif
	
}

static  void TestStartTest(void)
{




	
}