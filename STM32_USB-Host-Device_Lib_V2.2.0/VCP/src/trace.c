
#include "inc_all.h"


#define MAX_TRACE_DMA_BUFFER_SIZE		1024

/****************************************************************************/
/*External  Variables */

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

#ifdef DEBUG_TRACE
__ALIGN_BEGIN static u8 trace_dma_buffer[MAX_TRACE_DMA_BUFFER_SIZE] __ALIGN_END;
static u8 trace_dma_start = 0;
#endif

/****************************************************************************/

#ifdef DEBUG_TRACE

/****************************************************************************/
/*Local  Functions*/

static void trace_start_TX_DMA(u32 addr, u32 len);

/****************************************************************************/




static bool trace_DMA_finished(void)
{

	if ((DMA_GetFlagStatus(DMA1_FLAG_TC4) == SET)
	    || (DMA_GetCurrDataCounter(TRACE_UART_TX_DMA) == 0)) {
		DMA_ClearFlag(DMA1_FLAG_TC4);
		DMA_Cmd(TRACE_UART_TX_DMA, DISABLE);
		return true;
	} else {
		return false;
	}
}



void Trace(char *buf, u32 dat)
{

	u32 i;

	if (trace_dma_start) while (!trace_DMA_finished()) ;

	trace_dma_start = 1;

	if (buf == NULL) {
		return;
	}

	if (dat == 0) {
		i = sprintf((char *)trace_dma_buffer, "%s\r\n", buf);
	} else {
		i = sprintf((char *)trace_dma_buffer, "%s =%d\r\n", buf, dat);
	}

	trace_start_TX_DMA((u32)&trace_dma_buffer, i);

}





void TraceStr(char *buf)
{

	u32 i;
	char *p;
	if (trace_dma_start) while (!trace_DMA_finished()) ;
	trace_dma_start = 1;

	i = 0;

	p = buf;
	while (i < MAX_TRACE_DMA_BUFFER_SIZE) {
		if (*p == NULL) {
			break;
		}
		trace_dma_buffer[i] = *p++;
		i++;
	}
	trace_start_TX_DMA((u32)&trace_dma_buffer, i);

}





static bool IsAscii(u8 ch)
{
	if ((ch >= 0x20) && (ch < 0x80)) {
		return true;
	}
	return false;
}



void TraceBin(char *buf, u16 len)
{


	u32 i;

	if (len == 0) {
		return;
	}
	if (buf == NULL) {
		return;
	}



	if (trace_dma_start) while (!trace_DMA_finished()) ;
	trace_dma_start = 1;


	for (i = 0; i < len; i++) {
		if (IsAscii(buf[i])) {
			trace_dma_buffer[i] = buf[i];
		} else {
			trace_dma_buffer[i] = 0x20;
		}
	}

	trace_start_TX_DMA((u32)&trace_dma_buffer, len);

}





/*
static const u8 hex_table[16]={'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};

void TraceFormat(char buf)
{
		//u32 i = 0;
		trace_dma_buffer[0] = hex_table[buf & 0x0f];
		trace_dma_buffer[1] = hex_table[(buf>>4) & 0x0f];

		trace_start_TX_DMA((u32)&trace_dma_buffer,2);
}
*/
void TraceHex(char *buf, u16 len)
{

	u32 i;

	if (len == 0) {
		return;
	}
	if (len > (MAX_TRACE_DMA_BUFFER_SIZE)) {
		return;
	}


	if (trace_dma_start) while (!trace_DMA_finished()) ;
	trace_dma_start = 1;


	for (i = 0; i < len; i++) {
		trace_dma_buffer[i] = buf[i];
	}


	trace_start_TX_DMA((u32)&trace_dma_buffer, len);


}





/****************************************************************************************
* Function:...
* Parameters:
* Returns:....
* Description:
* Created:.... 		gary
*
****************************************************************************************/


static void trace_start_TX_DMA(u32 addr, u32 len)
{
	TRACE_UART_TX_DMA->CMAR = addr;
	TRACE_UART_TX_DMA->CNDTR = len;
	USART_DMACmd(USART1, USART_DMAReq_Tx, ENABLE);
	DMA_Cmd(TRACE_UART_TX_DMA, ENABLE);
}

#else


// static bool trace_DMA_finished(void)
// {
// 		return true;
// }



void Trace(char *buf, u32 dat) {}

void TraceStr(char *buf)
{


}





// static bool IsAscii(u8 ch)
// {
// 		return true;
// }



void TraceBin(char *buf, u16 len)
{
}

void TraceHex(char *buf, u16 len)
{
}

// static void trace_start_TX_DMA(u32 addr,u32 len)
// {
// }


#endif
