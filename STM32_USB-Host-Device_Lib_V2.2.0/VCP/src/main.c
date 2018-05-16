/**
  ******************************************************************************
  * @file    app.c
  * @author  MCD Application Team
  * @version V1.2.0
  * @date    09-November-2015
  * @brief   This file provides all the Application firmware functions.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2015 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an "AS IS" BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/

#include "inc_all.h"


/******************************************************************************
* @file
* @author  		Gary
* @version
* @date
* @brief
*
******************************************************************************/

/*   task  priority,  numeric higher priority higher*/

#define			USB_TASK_PRIO			(configMAX_PRIORITIES - 1)
#define			TEST_TASK_PRIO			(configMAX_PRIORITIES - 2)
#define			CAN_TASK_PRIO			(configMAX_PRIORITIES - 3)


/****************************************************************************/
/*External  Variables */

extern uint8_t APP_Tx_Buffer   [APP_TX_DATA_SIZE]  ;

/****************************************************************************/

/****************************************************************************/
/*External  Functions */

extern void usb_comm_task(void *taskpara);
extern void test_task(void *taskparam);
extern void can_task(void *taskparam);

extern void InitUsbVar(void);
extern void InitTestVar(void);


/****************************************************************************/

/****************************************************************************/
/*Global  Variables */

__ALIGN_BEGIN USB_OTG_CORE_HANDLE    USB_OTG_dev __ALIGN_END ;


can_para_t can_para_500k, can_para_100k;

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


int fputc(int ch, FILE *F)
{
     USART_SendData(USART1,(u8)ch);
	   while(USART_GetFlagStatus(USART1, USART_FLAG_TXE)== RESET);
	   return ch;
}

static void InitVar(void)
{
	InitUsbVar();
	InitTestVar();

	// Init can 500k
	can_para_500k.baudrate = 500 * 1000;
	can_para_500k.sjw = 1 - 1;
	can_para_500k.bs1 = 9 - 1;
	can_para_500k.bs2 = 8 - 1;
	can_para_500k.enable = 1;

	// Init can 100k
	can_para_100k.baudrate = 100 * 1000;
	can_para_100k.sjw = 1 - 1;
	can_para_100k.bs1 = 9 - 1;
	can_para_100k.bs2 = 8 - 1;
	can_para_100k.enable = 1;

}

extern void RCC_GetClocksFreq(RCC_ClocksTypeDef *RCC_Clocks);



void ReadADC(void);


int main(void)
{
	//  u32 i = 0;
	RCC_ClocksTypeDef RCC_Clocks;
	RCC_GetClocksFreq(&RCC_Clocks);
	InitVar();
	Driver_Init();
	//Full Speed
	USBD_Init(&USB_OTG_dev,USB_OTG_FS_CORE_ID, &USR_desc,&USBD_CDC_cb,&USR_cb);
	SystemCoreClockUpdate();

	printf("  system build time: %s,%s\r\n",__DATE__,__TIME__);
#if 1

	xTaskCreate(test_task, 	"test task",	configMINIMAL_STACK_SIZE, NULL,
	            TEST_TASK_PRIO, NULL);
	xTaskCreate(usb_comm_task, 	"usb comm task",	configMINIMAL_STACK_SIZE, NULL,
	            USB_TASK_PRIO, NULL);
	xTaskCreate(can_task, 	"can task",	configMINIMAL_STACK_SIZE, NULL,
	            USB_TASK_PRIO, NULL);
	vTaskStartScheduler();
#else  //for debug adc
	InitADC();
	while (1) {
		ReadADC();
		drv_delayms(1000);
	}
#endif
}

void vApplicationMallocFailedHook(void)
{
}

