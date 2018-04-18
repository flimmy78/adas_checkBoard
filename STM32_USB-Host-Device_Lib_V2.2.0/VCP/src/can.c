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

/****************************************************************************/




/****************************************************************************/
/*External  Functions */


/****************************************************************************/



/****************************************************************************/
/*Global  Variables */
volatile bool can_send_flag = false;
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






/* Interrupt func */




//void CAN2_RX1_IRQHandler(void)
//{

//		CanRxMsg rxMsgCan;
//		if(CAN_GetITStatus(CAN2,CAN_IT_FF1))
//		{
//			CAN_ClearITPendingBit(CAN2,CAN_IT_FF1);/**/
//		}  
//		if(CAN_GetITStatus(CAN2,CAN_IT_FOV1))
//		{
//			CAN_ClearITPendingBit(CAN2,CAN_IT_FOV1);
//		}

//		CAN_Receive(CAN2, CAN_FIFO1, &rxMsgCan);


//}

//void CAN1_RX0_IRQHandler(void)
//{

//	CanRxMsg rxMsgCan;
//	if(CAN_GetITStatus(CAN1,CAN_IT_FF0))
//	{
//		CAN_ClearITPendingBit(CAN1,CAN_IT_FF0);/**/
//	}  
//	if(CAN_GetITStatus(CAN1,CAN_IT_FOV0))
//	{
//		CAN_ClearITPendingBit(CAN1,CAN_IT_FOV0);
//	}

//	CAN_Receive(CAN1, CAN_FIFO0, &rxMsgCan);
//	
//}


void CAN1_Config(can_para_t *can_para)
{
  GPIO_InitTypeDef  			GPIO_InitStructure;
  CAN_InitTypeDef     		CAN_InitStructure;
	CAN_FilterInitTypeDef  	CAN_FilterInitStructure;
	u32 can_prescaler;
	
	
  /* GPIO clock enable */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIO_CAN1, ENABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);

  /* Configure CAN pin: RX */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_CAN1_RX;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;
  GPIO_Init(GPIO_CAN1, &GPIO_InitStructure);
  
  /* Configure CAN pin: TX */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_CAN1_TX;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIO_CAN1, &GPIO_InitStructure);
  
  GPIO_PinRemapConfig(GPIO_Remap2_CAN1 , ENABLE);
  
  /* CANx Periph clock enable */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1, ENABLE);

  

  /* CAN register init */
  CAN_DeInit(CAN1);
	
	if (can_para->enable == false) 
	{
		CAN_ITConfig(CAN1, CAN_IT_FMP0 | CAN_IT_FF0 | CAN_IT_FOV0 | CAN_IT_ERR, ENABLE);
		return;
	}
	
	
  CAN_StructInit(&CAN_InitStructure);

  /* CAN cell init */
  CAN_InitStructure.CAN_TTCM = DISABLE;
  CAN_InitStructure.CAN_ABOM = DISABLE;
  CAN_InitStructure.CAN_AWUM = DISABLE;
  CAN_InitStructure.CAN_NART = DISABLE;
  CAN_InitStructure.CAN_RFLM = DISABLE;
  CAN_InitStructure.CAN_TXFP = ENABLE;
  CAN_InitStructure.CAN_Mode = CAN_Mode_Normal;



	/* 72M /2 / 9 / 4 =  1M */
  /* CAN Baudrate = 1MBps*/
  CAN_InitStructure.CAN_SJW = can_para->sjw;
  CAN_InitStructure.CAN_BS1 = can_para->bs1;
  CAN_InitStructure.CAN_BS2 = can_para->bs2;

  can_prescaler = (CAN_InitStructure.CAN_SJW + 1) + (CAN_InitStructure.CAN_BS1 + 1) + (CAN_InitStructure.CAN_BS2 + 1);
	
  CAN_InitStructure.CAN_Prescaler = (36000000/can_para->baudrate/can_prescaler);

	

  CAN_Init(CAN1, &CAN_InitStructure);

	/* No filter */
  CAN_FilterInitStructure.CAN_FilterNumber = 0;
  CAN_FilterInitStructure.CAN_FilterMode = CAN_FilterMode_IdMask;
  CAN_FilterInitStructure.CAN_FilterScale = CAN_FilterScale_32bit;
  CAN_FilterInitStructure.CAN_FilterIdHigh = 0x0000;
  CAN_FilterInitStructure.CAN_FilterIdLow = 0x0000;
  CAN_FilterInitStructure.CAN_FilterMaskIdHigh = 0x0000;
  CAN_FilterInitStructure.CAN_FilterMaskIdLow = 0x0000;
  CAN_FilterInitStructure.CAN_FilterFIFOAssignment = CAN_Filter_FIFO0;
  CAN_FilterInitStructure.CAN_FilterActivation = ENABLE;
  CAN_FilterInit(&CAN_FilterInitStructure);
 
  CAN_ITConfig(CAN1, CAN_IT_FMP0 | CAN_IT_FF0 | CAN_IT_FOV0 | CAN_IT_ERR, ENABLE);

}



void CAN2_Config(can_para_t *can_para)
{
  GPIO_InitTypeDef  		GPIO_InitStructure;
  CAN_InitTypeDef     		CAN_InitStructure;
	CAN_FilterInitTypeDef  	CAN_FilterInitStructure;
	u32 can_prescaler;
	
	
  /* GPIO clock enable */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIO_CAN2, ENABLE);

  /* Configure CAN pin: RX */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_CAN2_RX;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;
  GPIO_Init(GPIO_CAN2, &GPIO_InitStructure);
  
  /* Configure CAN pin: TX */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_CAN2_TX;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIO_CAN2, &GPIO_InitStructure);
  
 // GPIO_PinRemapConfig(GPIO_Remapping_CAN1 , ENABLE);
  
  /* CANx Periph clock enable */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN2, ENABLE);

  

  /* CAN register init */
  CAN_DeInit(CAN2);
	
	if (can_para->enable == false) 
	{
		CAN_ITConfig(CAN2, CAN_IT_FMP1 | CAN_IT_FF1 | CAN_IT_FOV1 | CAN_IT_ERR, ENABLE);
		return;
	}
	
	
  CAN_StructInit(&CAN_InitStructure);

  /* CAN cell init */
  CAN_InitStructure.CAN_TTCM = DISABLE;
  CAN_InitStructure.CAN_ABOM = DISABLE;
  CAN_InitStructure.CAN_AWUM = DISABLE;
  CAN_InitStructure.CAN_NART = DISABLE;
  CAN_InitStructure.CAN_RFLM = DISABLE;
  CAN_InitStructure.CAN_TXFP = ENABLE;
  CAN_InitStructure.CAN_Mode = CAN_Mode_Normal;



	/* 72M /2 / 9 / 4 =  1M */
  /* CAN Baudrate = 1MBps*/
  CAN_InitStructure.CAN_SJW = can_para->sjw;
  CAN_InitStructure.CAN_BS1 = can_para->bs1;
  CAN_InitStructure.CAN_BS2 = can_para->bs2;

	can_prescaler = (CAN_InitStructure.CAN_SJW + 1) + (CAN_InitStructure.CAN_BS1 + 1) + (CAN_InitStructure.CAN_BS2 + 1);
	
	CAN_InitStructure.CAN_Prescaler = (36000000/can_para->baudrate/can_prescaler);

	

  CAN_Init(CAN2, &CAN_InitStructure);

	/* No filter */
  CAN_FilterInitStructure.CAN_FilterNumber = 14;
  CAN_FilterInitStructure.CAN_FilterMode = CAN_FilterMode_IdMask;
  CAN_FilterInitStructure.CAN_FilterScale = CAN_FilterScale_32bit;
  CAN_FilterInitStructure.CAN_FilterIdHigh = 0x0000;
  CAN_FilterInitStructure.CAN_FilterIdLow = 0x0000;
  CAN_FilterInitStructure.CAN_FilterMaskIdHigh = 0x0000;
  CAN_FilterInitStructure.CAN_FilterMaskIdLow = 0x0000;
  CAN_FilterInitStructure.CAN_FilterFIFOAssignment = CAN_Filter_FIFO1;
  CAN_FilterInitStructure.CAN_FilterActivation = ENABLE;
  CAN_FilterInit(&CAN_FilterInitStructure);
  

  CAN_ITConfig(CAN2, CAN_IT_FMP1 | CAN_IT_FF1 | CAN_IT_FOV1 | CAN_IT_ERR, ENABLE);
	

}






void can_send(void)
{
	CanTxMsg TxMessage;
	u32 i,j;
	j = 10;

	CAN1_Config(&can_para_500k);
	CAN2_Config(&can_para_500k);
	vTaskDelay(10);	
	j = 10;
	while (j--)
	{
		TxMessage.StdId = 0x378;
		TxMessage.RTR = CAN_RTR_DATA;
		TxMessage.IDE = CAN_ID_STD;
		TxMessage.DLC = 8;
		for (i=0;i<8;i++)
			TxMessage.Data[i] = 0xFF;
	//	CAN_Transmit(CAN1,&TxMessage);
		if (CAN_Transmit(CAN2,&TxMessage) == CAN_TxStatus_NoMailBox)
		{
			TraceStr("*");
		}
		vTaskDelay(5);
	}

	CAN1_Config(&can_para_100k);
	CAN2_Config(&can_para_100k);
	vTaskDelay(10); 
	j = 10;
	while (j--)
	{
		TxMessage.StdId = 0x378;
		TxMessage.RTR = CAN_RTR_DATA;
		TxMessage.IDE = CAN_ID_STD;
		TxMessage.DLC = 8;
		for (i=0;i<8;i++)
			TxMessage.Data[i] = 0xFF;
	//	CAN_Transmit(CAN1,&TxMessage);
		if (CAN_Transmit(CAN2,&TxMessage) == CAN_TxStatus_NoMailBox)
		{
			TraceStr("*");
		}
		vTaskDelay(5);
	}



	
}



void can_task(void *taskparam)
{
	vTaskDelay(2000);
	can_send_flag = true;
	while (1)
	{
		if (can_send_flag)
		{
			can_send();
		}
		else
			vTaskDelay(100);
		
		
	}


	
}
