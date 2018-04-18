/**
  ******************************************************************************
  * @file    usbd_cdc_vcp.c
  * @author  MCD Application Team
  * @version V1.2.0
  * @date    09-November-2015
  * @brief   Generic media access Layer.
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

#ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED
#pragma     data_alignment = 4
#endif /* USB_OTG_HS_INTERNAL_DMA_ENABLED */

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
extern FIFO_t				UsbTxFifo;
extern FIFO_t				UsbRxFifo;
extern volatile bool  		g_check_usb_comm_msg_flag;

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









LINE_CODING linecoding = {
	115200, /* baud rate*/
	0x00,   /* stop bits-1*/
	0x00,   /* parity - none*/
	0x08    /* nb. of bits 8*/
};


USART_InitTypeDef USART_InitStructure;

/* These are external variables imported from CDC core to be used for IN
   transfer management. */
extern uint8_t  APP_Tx_Buffer []; /* Write CDC received data in this buffer.
                                     These data will be sent over USB IN endpoint
                                     in the CDC core functions. */
extern uint32_t APP_Tx_ptr_in;    /* Increment this pointer or roll it back to
                                     start address when writing received data
                                     in the buffer APP_Tx_Buffer. */

/* Private function prototypes -----------------------------------------------*/
static uint16_t VCP_Init(void);
static uint16_t VCP_DeInit(void);
static uint16_t VCP_Ctrl(uint32_t Cmd, uint8_t *Buf, uint32_t Len);
static uint16_t VCP_DataTx(void);
static uint16_t VCP_DataRx(uint8_t *Buf, uint32_t Len);
//static uint16_t VCP_DataRx (uint32_t Len);

//static uint16_t VCP_COMConfig(uint8_t Conf);

CDC_IF_Prop_TypeDef VCP_fops = {
	VCP_Init,
	VCP_DeInit,
	VCP_Ctrl,
	VCP_DataTx,
	VCP_DataRx
};

/* Private functions ---------------------------------------------------------*/
/**
  * @brief  VCP_Init
  *         Initializes the Media on the STM32
  * @param  None
  * @retval Result of the operation (USBD_OK in all cases)
  */
static uint16_t VCP_Init(void)
{
	return USBD_OK;
}

/**
  * @brief  VCP_DeInit
  *         DeInitializes the Media on the STM32
  * @param  None
  * @retval Result of the operation (USBD_OK in all cases)
  */
static uint16_t VCP_DeInit(void)
{

	return USBD_OK;
}


/**
  * @brief  VCP_Ctrl
  *         Manage the CDC class requests
  * @param  Cmd: Command code
  * @param  Buf: Buffer containing command data (request parameters)
  * @param  Len: Number of data to be sent (in bytes)
  * @retval Result of the operation (USBD_OK in all cases)
  */
static uint16_t VCP_Ctrl(uint32_t Cmd, uint8_t *Buf, uint32_t Len)
{
	switch (Cmd) {
	case SEND_ENCAPSULATED_COMMAND:
		/* Not  needed for this driver */
		break;

	case GET_ENCAPSULATED_RESPONSE:
		/* Not  needed for this driver */
		break;

	case SET_COMM_FEATURE:
		/* Not  needed for this driver */
		break;

	case GET_COMM_FEATURE:
		/* Not  needed for this driver */
		break;

	case CLEAR_COMM_FEATURE:
		/* Not  needed for this driver */
		break;

	case SET_LINE_CODING:
		//    linecoding.bitrate = (uint32_t)(Buf[0] | (Buf[1] << 8) | (Buf[2] << 16) | (Buf[3] << 24));
		//    linecoding.format = Buf[4];
		//    linecoding.paritytype = Buf[5];
		//    linecoding.datatype = Buf[6];
		//    /* Set the new configuration */
		//    VCP_COMConfig(OTHER_CONFIG);
		break;

	case GET_LINE_CODING:
		linecoding.bitrate = 921600;
		Buf[0] = (uint8_t)(linecoding.bitrate);
		Buf[1] = (uint8_t)(linecoding.bitrate >> 8);
		Buf[2] = (uint8_t)(linecoding.bitrate >> 16);
		Buf[3] = (uint8_t)(linecoding.bitrate >> 24);
		Buf[4] = linecoding.format;
		Buf[5] = linecoding.paritytype;
		Buf[6] = linecoding.datatype;


		break;

	case SET_CONTROL_LINE_STATE:
		/* Not  needed for this driver */
		break;

	case SEND_BREAK:
		/* Not  needed for this driver */
		break;

	default:
		break;
	}

	return USBD_OK;
}

/**
  * @brief  VCP_DataTx
  *         CDC received data to be send over USB IN endpoint are managed in
  *         this function.
  * @param  Buf: Buffer of data to be sent
  * @param  Len: Number of data to be sent (in bytes)
  * @retval Result of the operation: USBD_OK if all operations are OK else VCP_FAIL
  */

// Send data to PC
static uint16_t VCP_DataTx(void)
{

	APP_Tx_ptr_in = UsbTxFifo.rear;

	return USBD_OK;
}






static uint16_t VCP_DataRx(uint8_t *Buf, uint32_t Len)
{
	u32 i;
	// Copy data in rx buffer
	for (i = 0; i < Len; i++)	{
		EnFIFO(&UsbRxFifo, Buf[i]);
	}

	g_check_usb_comm_msg_flag = true;

	return USBD_OK;
}


void UsbSendData(void)
{
	VCP_DataTx();
}




/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
