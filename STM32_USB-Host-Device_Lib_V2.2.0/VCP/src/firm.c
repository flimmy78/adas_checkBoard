#include "inc_all.h"

#define 	ACK			0x79
#define 	NACK		0x1F


#define 	CMD_GET					0x00
#define		CMD_GET_VER 			0x01
#define 	CMD_GET_ID				0x02
#define 	CMD_READ				0x11
#define 	CMD_GO					0x21
#define		CMD_WRITE				0x31
#define 	CMD_ERASE				0x43
#define		CMD_WRITE_PROTECT		0x63
#define 	CMD_WRITE_UNPROTECT		0x73
#define 	CMD_READ_PROTECT		0x82
#define 	CMD_READ_UNPROTECT		0x92












/****************************************************************************/
/*External  Variables */
extern FIFO_t		UartRxFifo;
extern u8  UartRcvFrameBuffer[MAX_UART_FRAME_LENGTH];
extern volatile bool g_check_uart_comm_msg_flag;
/****************************************************************************/




/****************************************************************************/
/*External  Functions */
extern void FirmSendData(u8 *buf,u16 len);
extern u16 RcvFirmFrameFromUart(FIFO_t *stFIFO, u8 *framebuf);
/****************************************************************************/



/****************************************************************************/
/*Global  Variables */
u8 CommBuf[512];

/****************************************************************************/



/****************************************************************************/
/*Global  Functions */

/****************************************************************************/


/****************************************************************************/
/*Local  Variables*/

/****************************************************************************/


/****************************************************************************/
/*Local  Functions*/
static bool FirmWaitAck(void);
static bool FirmDownload(void);
/****************************************************************************/


static void FirmStart(void)
{
	CommBuf[0] = 0x7F;
	FirmSendData(CommBuf,1);
	FirmWaitAck();

}

static void FirmCmdGet(void)
{

	CommBuf[0] = CMD_GET;
	CommBuf[1] = 0xFF - CommBuf[0];
	FirmSendData(CommBuf,2);
	FirmWaitAck();
}

static void FirmCmdGo(void)
{

	CommBuf[0] = CMD_GO;
	CommBuf[1] = 0xFF - CommBuf[0];
	FirmSendData(CommBuf,2);
	FirmWaitAck();
}


static void FirmCmdWrite(u8 *buf,u32 addr,u16 len)
{
	u8 xor_value;
	u32 i;
	// Send Cmd
	CommBuf[0] = CMD_WRITE;
	CommBuf[1] = 0xFF - CommBuf[0];
	FirmSendData(CommBuf,2);
	FirmWaitAck();	
	
	//Send Addr
	CommBuf[0] = (addr>>24) & 0xff;
	CommBuf[1] = (addr>>16) & 0xff;
	CommBuf[2] = (addr>>8) & 0xff;
	CommBuf[3] = (addr) & 0xff;
	CommBuf[4] = CommBuf[0] ^ CommBuf[1] ^ CommBuf[2] ^ CommBuf[3];
	FirmSendData(CommBuf,5);
	FirmWaitAck();		

	//Send Data
	CommBuf[0] = (len - 1) & 0xff;
	xor_value = CommBuf[0];
	for(i=0;i<len;i++)
	{
		CommBuf[i+1] = buf[i];
		xor_value ^= CommBuf[i+1];
	}
	FirmSendData(CommBuf,len+2);
	FirmWaitAck();	
	

	
}

void FirmTest(void)
{

	FirmStart();
	FirmCmdGet();

	FirmDownload();
	FirmCmdGo();
}

#if 0
static bool FirmWaitAck(void)
{
	u32 i;
	u16 len;
	len = RcvFirmFrameFromUart(&UartRxFifo,CommBuf);
	for (i=0;i<len;i++)
		sprintf((char *)&UartRcvFrameBuffer[i*3],"%02x ",CommBuf[i]);
	TraceStr(UartRcvFrameBuffer);
}
#else

static bool FirmWaitAck(void)
{
	u32 i;
	u16 len;
	//g_check_uart_comm_msg_flag = false;

	// Wait some time
	i = 0;
	while (1)
	{
		i++;
		if (i>2000) 
		{
			TraceStr("ACK timeout\r\n");
			return false;
		}
		//if (g_check_uart_comm_msg_flag) break;
		vTaskDelay(1);
	}

	len = RcvFirmFrameFromUart(&UartRxFifo,CommBuf);

	for (i=0;i<len;i++)
		sprintf((char *)&UartRcvFrameBuffer[i*3],"%02x ",CommBuf[i]);
	TraceStr(UartRcvFrameBuffer);

	
	if (CommBuf[0] == 0x79) return true;
	else 
	{
		TraceStr("ACK Error\r\n");
		return false;
	}

}
#endif




static bool FirmDownload(void)
{
	u32 test_code_base = 0x14000;
	u32 test_code_length = 48*1024;
	u32 baseaddr = 0x20000000;
	u32 i,num;
	u32 len = test_code_length;
	u8 *p = (u8 *)test_code_base;

	i = 0;
	while (1)
	{
		if ((i+256)<=len)
		{
			FirmCmdWrite(p,baseaddr,256);
			i = i + 256;
			baseaddr += 256;
			p += 256;
		}
		else
		{
			num = ((len - i)+8)/4 * 4;
			FirmCmdWrite(p,baseaddr,num);
			i = i + num;
			baseaddr += num;
			p += num;
			break;
		}
	}





	
}














