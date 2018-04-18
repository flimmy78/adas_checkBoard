
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

extern u8  comm_uart_rx_buffer[MAX_COMM_UART_DMA_RCV_SIZE];
extern u8  comm_uart_tx_buffer[MAX_COMM_UART_DMA_SND_SIZE];
extern u8  UartRcvFrameBuffer[MAX_UART_FRAME_LENGTH];


/****************************************************************************/



/****************************************************************************/
/*Global  Functions */

/****************************************************************************/


/****************************************************************************/
/*Local  Variables*/





/****************************************************************************/


/****************************************************************************/
/*Local  Functions*/


static bool uart_DMA_finished(void);
static void UartSendData(msg_head_t *msg_head);
static bool uart_DMA_finished(void);
static void MsgSendFromUart(comm_head_t *uart_msg_head, task_id_enum totask);
static void uart_start_TX_DMA(u32 addr, u32 len);
static void TestSendData(u8 *buf, u16 len);

/****************************************************************************/






void ReadFromUartRxFifo(u8 *buf, u16 *len)
{




}
