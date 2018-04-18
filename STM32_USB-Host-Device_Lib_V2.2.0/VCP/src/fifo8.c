
#include "inc_all.h"

/*

FIFO 8 bit data buffer

*/


void  InitFIFO(FIFO_t *stFIFO, u8 *buf, u32 size)
{
	stFIFO->head = stFIFO->rear = 0;
	stFIFO->FIFO_size = size;
	stFIFO->databuf = (u8 *)buf;
}

void EnFIFO(FIFO_t *stFIFO, u8 dat)
{
	stFIFO->databuf[stFIFO->rear] = dat;
	stFIFO->rear ++;
	if (stFIFO->rear >= stFIFO->FIFO_size) {
		stFIFO->rear = 0;
	}
}



u8 DeFIFO(FIFO_t *stFIFO)
{
	u8 dat;
	dat = stFIFO->databuf[stFIFO->head];
	stFIFO->head ++;
	if (stFIFO->head >= stFIFO->FIFO_size) {
		stFIFO->head = 0;
	}
	return dat;
}






u8 CheckFIFOByte(FIFO_t *stFIFO)
{
	return stFIFO->databuf[stFIFO->head];
}



bool FIFOIsEmpty(FIFO_t *stFIFO)
{

	if (stFIFO->head == stFIFO->rear) {
		return true;
	}
	return false;
}



/* FIFO data len, FIFO free size = total size - fifolen */
u16 FIFOLen(FIFO_t *stFIFO)
{
	if (stFIFO->rear >= stFIFO->head) {
		return (stFIFO->rear - stFIFO->head);
	} else {
		return (stFIFO->rear + stFIFO->FIFO_size - stFIFO->head);
	}
}

u16  FIFOFreeLen(FIFO_t *stFIFO)
{
	return (stFIFO->FIFO_size - FIFOLen(stFIFO));
}
extern u8  comm_uart_rx_buffer[MAX_COMM_UART_DMA_RCV_SIZE];
void ReinitFIFO(FIFO_t *stFIFO)
{
	/*int i;
	for(i=0;i<MAX_COMM_UART_DMA_RCV_SIZE;i++)
		comm_uart_rx_buffer[i] = 0;*/
	stFIFO->head = stFIFO->rear = 0;
}



bool  EnFIFOBuf(FIFO_t *stFIFO, u8 *buf, u16 len)
{
	u16 tmplen;
	if ((stFIFO->FIFO_size - FIFOLen(stFIFO)) <= len) {
		return false;
	}
	//one copy
	if (stFIFO->head > stFIFO->rear) {
		memcpy(&stFIFO->databuf[stFIFO->rear], buf, len);
		stFIFO->rear += len;
	}
	//two copy
	else {
		if ((stFIFO->FIFO_size - stFIFO->rear) > len) {	//Just one copy
			memcpy(&stFIFO->databuf[stFIFO->rear], buf, len);
			stFIFO->rear += len;
		} else {
			tmplen = stFIFO->FIFO_size - stFIFO->rear;
			memcpy(&stFIFO->databuf[stFIFO->rear], buf, tmplen);
			memcpy(&stFIFO->databuf[0], &buf[tmplen], len - tmplen);
			stFIFO->rear = len - tmplen;
		}
	}

	return true;

}









#if 0
static void CheckFrameHeaderFromFIFO(FIFO_t *stFIFO, u8 *framebuf)
{
	u8 i;
	u32 tmp;
	tmp = stFIFO->head;   /* Remember head */
	for (i = 0; i < (UART_HDR_LENGTH); i++) {
		framebuf[i]  = DeFIFO(stFIFO);
	}
	/*Restore head */
	stFIFO->head = tmp;
}


/****************************************************************************************
* Function:...
* Parameters:
* Returns:.... 		 true: success,   false: failed  to copy data
* Description: 	 	From current fifo pos,copy a frame from fifo and change fifo pointer
* Created:.... 		gary
*
****************************************************************************************/

static bool RcvFrameFromFIFO(FIFO_t *stFIFO, u8 *framebuf)
{
	/* Find a head */
	u16 frame_len, i;
	uart_head_t *uart_head;

	while (1) {
		if (FIFOLen(stFIFO) < (UART_HDR_LENGTH)) {
			return false;    /* Frame not receive finished, return */
		}
		if (CheckFIFOByte(stFIFO) == (UART_START_BYTES & 0xFF)) {

			/* Check it is a head */
			CheckFrameHeaderFromFIFO(stFIFO, framebuf);
			uart_head = (uart_head_t *)framebuf;
			frame_len = uart_head->msg_len;
			if (uart_head->start_bytes != UART_START_BYTES) {
				DeFIFO(stFIFO);
				continue;
			}
			if ((frame_len >= MAX_HI3518_UART_FRAME_BUFFER_SIZE)
			    || (frame_len < UART_HDR_LENGTH)) { /* Something error,but head is matched */
				/* Remove head,four bytes */
				DeFIFO(stFIFO);
				DeFIFO(stFIFO);
				DeFIFO(stFIFO);
				DeFIFO(stFIFO);
			}
			if (frame_len > FIFOLen(stFIFO)) {
				return false;    /*Not receive finished, return */
			}
			/* Ok, now we copy frame from fifo and change fifo pointer */
			for (i = 0; i < frame_len; i++) {
				framebuf[i] = DeFIFO(stFIFO);
			}
			return true;
		} else {
			DeFIFO(stFIFO);  /* Throw one byte */
			continue;
		}
	}
}
#endif

