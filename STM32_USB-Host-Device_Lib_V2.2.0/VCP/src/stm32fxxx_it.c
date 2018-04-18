/**
  ******************************************************************************
  * @file    stm32fxxx_it.c
  * @author  MCD Application Team
  * @version V1.2.0
  * @date    09-November-2015
  * @brief   Main Interrupt Service Routines.
  *          This file provides all exceptions handler and peripherals interrupt
  *          service routine.
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






/****************************************************************************/
/*External  Variables */

extern volatile bool g_check_uart_comm_msg_flag;
extern FIFO_t		UartRxFifo;

/****************************************************************************/




/****************************************************************************/
/*External  Functions */

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

/****************************************************************************/







extern USB_OTG_CORE_HANDLE           USB_OTG_dev;
extern uint32_t USBD_OTG_ISR_Handler(USB_OTG_CORE_HANDLE *pdev);
#ifdef USB_OTG_HS_DEDICATED_EP1_ENABLED
xxx
extern uint32_t USBD_OTG_EP1IN_ISR_Handler(USB_OTG_CORE_HANDLE *pdev);
extern uint32_t USBD_OTG_EP1OUT_ISR_Handler(USB_OTG_CORE_HANDLE *pdev);
#endif

/******************************************************************************/
/*             Cortex-M Processor Exceptions Handlers                         */
/******************************************************************************/

/**
  * @brief   This function handles NMI exception.
  * @param  None
  * @retval None
  */
void NMI_Handler(void)
{
}

/**
  * @brief  This function handles Hard Fault exception.
  * @param  None
  * @retval None
  */
void HardFault_Handler(void)
{
	/* Go to infinite loop when Hard Fault exception occurs */
	while (1) {
	}
}

/**
  * @brief  This function handles Memory Manage exception.
  * @param  None
  * @retval None
  */
void MemManage_Handler(void)
{
	/* Go to infinite loop when Memory Manage exception occurs */
	while (1) {
	}
}

/**
  * @brief  This function handles Bus Fault exception.
  * @param  None
  * @retval None
  */
void BusFault_Handler(void)
{
	/* Go to infinite loop when Bus Fault exception occurs */
	while (1) {
	}
}

/**
  * @brief  This function handles Usage Fault exception.
  * @param  None
  * @retval None
  */
void UsageFault_Handler(void)
{
	/* Go to infinite loop when Usage Fault exception occurs */
	while (1) {
	}
}

/**
  * @brief  This function handles SVCall exception.
  * @param  None
  * @retval None
  */
void SVC_Handler(void)
{
}

/**
  * @brief  This function handles Debug Monitor exception.
  * @param  None
  * @retval None
  */
void DebugMon_Handler(void)
{
}

/**
  * @brief  This function handles PendSVC exception.
  * @param  None
  * @retval None
  */
void PendSV_Handler(void)
{
}

/**
  * @brief  This function handles SysTick Handler.
  * @param  None
  * @retval None
  */
void SysTick_Handler(void)
{
	/* Information panel */
	//  LCD_SetTextColor(Green);
	//  LCD_SetTextColor(LCD_LOG_DEFAULT_COLOR);
}

/**
  * @brief  This function handles OTG_HS Handler.
  * @param  None
  * @retval None
  */
void OTG_FS_IRQHandler(void)
{
	USBD_OTG_ISR_Handler(&USB_OTG_dev);
}

#ifdef USB_OTG_HS_DEDICATED_EP1_ENABLED
/**
  * @brief  This function handles EP1_IN Handler.
  * @param  None
  * @retval None
  */
void OTG_HS_EP1_IN_IRQHandler(void)
{
	USBD_OTG_EP1IN_ISR_Handler(&USB_OTG_dev);
}

/**
  * @brief  This function handles EP1_OUT Handler.
  * @param  None
  * @retval None
  */
void OTG_HS_EP1_OUT_IRQHandler(void)
{
	USBD_OTG_EP1OUT_ISR_Handler(&USB_OTG_dev);
}
#endif




void TIM2_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET) {
		TIM_ClearITPendingBit(TIM2, TIM_FLAG_Update);
		//		SystemTime._10us += TIM_MAX_TICKS;
	}
}

extern u8  comm_uart_rx_buffer[MAX_COMM_UART_DMA_RCV_SIZE];
/* Comm with main board */
void UART4_IRQHandler(void)
{

	if (USART_GetITStatus(UART4, USART_IT_TXE) != RESET) {

	}

	if (USART_GetITStatus(UART4, USART_IT_RXNE) != RESET) {
		//USART_ReceiveData(UART4);
	}

	if (USART_GetITStatus(UART4, USART_IT_IDLE) != RESET) {

		USART_ReceiveData(UART4);
		//update fifo head pointer
		UartRxFifo.rear = (MAX_COMM_UART_DMA_RCV_SIZE - COMM_UART_RX_DMA->CNDTR);
		//Trace("UartRxFifo.head",UartRxFifo.head);
		//Trace("comm_uart_rx_buffer[head]",comm_uart_rx_buffer[UartRxFifo.head]);

		g_check_uart_comm_msg_flag = true;

	}
}





void CAN1_RX0_IRQHandler(void)
{
	CanRxMsg rxMsgME;
	volatile int i = 0;

	if (CAN_GetITStatus(CAN1, CAN_IT_FF0)) {
		CAN_ClearITPendingBit(CAN1, CAN_IT_FF0);    /**/
	}
	if (CAN_GetITStatus(CAN1, CAN_IT_FOV0)) {
		CAN_ClearITPendingBit(CAN1, CAN_IT_FOV0);
	}

	CAN_Receive(CAN1, CAN_FIFO0, &rxMsgME);


	if (rxMsgME.StdId >= 0x7FF) {
		i = 1;
		return;
	}

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////





unsigned char can_sensor_flag = 0 ; // 0 : NO , 1 : OK
void CAN2_RX1_IRQHandler(void)
{
	CanRxMsg	  rxMsgCar;
	static int send_can_flag = 0;

	if (CAN_GetITStatus(CAN2, CAN_IT_BOF)) {
		CAN_ClearITPendingBit(CAN2, CAN_IT_BOF);
	}
	if (CAN_GetITStatus(CAN2, CAN_IT_FF1)) {
		CAN_ClearITPendingBit(CAN2, CAN_IT_FF1);    /**/
	}
	if (CAN_GetITStatus(CAN2, CAN_IT_FOV1)) {
		CAN_ClearITPendingBit(CAN2, CAN_IT_FOV1);
	}
	if (CAN_GetITStatus(CAN2, CAN_IT_EPV)) {
		CAN_ClearITPendingBit(CAN2, CAN_IT_FOV1);
	}

	CAN_Receive(CAN2, CAN_FIFO1, &rxMsgCar);

	if ((rxMsgCar.Data[0] == 0xf0) && (rxMsgCar.Data[1] == 0xf0)
	    && (rxMsgCar.Data[2] == 0xf0) && (rxMsgCar.Data[3] == 0xf0)
	    && (rxMsgCar.Data[4] == 0xf0) && (rxMsgCar.Data[5] == 0xf0)
	    && (rxMsgCar.Data[6] == 0xf0) && (rxMsgCar.Data[7] == 0xf0)) {
		can_sensor_flag = 1;
	} else {
		can_sensor_flag = 0;
	}
	//if (rxMsgCar.StdId == 0x378)
	//{
	//send_can_flag = 1;
	//}




}


