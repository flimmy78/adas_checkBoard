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
#define 	CMD_ERASE_V3_0				0x44
#define		CMD_WRITE_PROTECT		0x63
#define 	CMD_WRITE_UNPROTECT		0x73
#define 	CMD_READ_PROTECT		0x82
#define 	CMD_READ_UNPROTECT		0x92

/****************************************************************************/

#define 	RAM_START_ADDR			0x20002000
#define 	TEST_CODE_START_ADDR	0x14000
#define 	TEST_CODE_LENGTH		(48*1024)


/****************************************************************************/
/*External  Variables */
extern FIFO_t		UartRxFifo;
extern u8  UartRcvFrameBuffer[MAX_UART_FRAME_LENGTH];
extern volatile bool g_check_uart_comm_msg_flag;

extern void MsgSendFromTest1(comm_head_t *test_msg_head, task_id_enum totask);
/****************************************************************************/


/****************************************************************************/
/*External  Functions */
extern void FirmSendData(u8 *buf, u16 len);
extern u16 RcvFirmFrameFromUart(FIFO_t *stFIFO, u8 *framebuf);
extern void ReinitFIFO(FIFO_t *stFIFO);
/****************************************************************************/



/****************************************************************************/
/*Global  Variables */
u8 CommBuf[512];
u32 FirmRcvLen;
/****************************************************************************/



/****************************************************************************/
/*Global  Functions */

/****************************************************************************/


/****************************************************************************/
/*Local  Variables*/
/****************************************************************************/


/****************************************************************************/
/*Local  Functions*/
static bool FirmWaitAck(u16 before, u16 after);
static bool FirmDownload(void);
static bool FirmCmdRead(u32 addr, u16 len);
/****************************************************************************/
static void FirmStart(void)
{
	//unsigned char tt = 'A';
	CommBuf[0] = 0x7F;
	FirmSendData(CommBuf, 1);
	//FirmSendData(&tt,1);
}

static void FirmCmdGet(void)
{

	CommBuf[0] = CMD_GET;
	CommBuf[1] = 0xFF - CommBuf[0];
	FirmSendData(CommBuf, 2);
}

static void FirmCmdGetVer(void)
{

	CommBuf[0] = CMD_GET_VER;
	CommBuf[1] = 0xFF - CommBuf[0];
	FirmSendData(CommBuf, 2);
}

static bool FirmCmdErase(void)
{

	CommBuf[0] = CMD_ERASE;
	CommBuf[1] = 0xFF - CommBuf[0];
	FirmSendData(CommBuf, 2);
	if (FirmWaitAck(300, 20) == false) {
		TraceStr("WRITE ERASE CMD , NO ACK\r\n");
		return false;
	}

	CommBuf[0] = 0xFF;
	FirmSendData(CommBuf, 1);

	vTaskDelay(1);
	CommBuf[0] = 0x00;
	FirmSendData(CommBuf, 1);
	if (FirmWaitAck(33000, 100) == false) {
		TraceStr("Erase FLASH failed \r\n");
		return false;
	}
	TraceStr("Erase code complete \r\n");
	return true;
}

static bool FirmCmdErase_V3_0(void)
{

	CommBuf[0] = CMD_ERASE_V3_0;
	CommBuf[1] = 0xFF - CommBuf[0];
	FirmSendData(CommBuf, 2);
	if (FirmWaitAck(300, 20) == false) {
		TraceStr("WRITE ERASE CMD , NO ACK\r\n");
		return false;
	}

	CommBuf[0] = 0xFF;
	CommBuf[1] = 0xFF;
	CommBuf[2] = 0x00;
	FirmSendData(CommBuf, 3);
	if (FirmWaitAck(33000, 100) == false) {
		TraceStr("Erase FLASH failed \r\n");
		return false;
	}
	TraceStr("Erase code complete \r\n");
	return true;
}

static bool FirmCheckCodeNeedErase()
{
	u32 i;
	FirmCmdRead(0x08000000, 16);
	Trace("rcv num", FirmRcvLen);
	if (FirmRcvLen < 17) {
		return true;
	}

	for (i = 1; i < 17; i++) {
		if (CommBuf[i] != 0xFF) {
			return true;
		}
	}

	TraceStr("Do not erase \r\n");
	return false;

}

static bool FirmCmdRead(u32 addr, u16 len)
{
	u32 i;

	TraceStr("read cmd \r\n");
	// Send Cmd
	CommBuf[0] = CMD_READ;
	CommBuf[1] = 0xFF - CommBuf[0];
	FirmSendData(CommBuf, 2);
	if (FirmWaitAck(0, 0) == false) {
		return false;
	}

	TraceStr("read addr \r\n");
	//Send Addr
	CommBuf[0] = (addr >> 24) & 0xff;
	CommBuf[1] = (addr >> 16) & 0xff;
	CommBuf[2] = (addr >> 8) & 0xff;
	CommBuf[3] = (addr) & 0xff;
	CommBuf[4] = CommBuf[0] ^ CommBuf[1] ^ CommBuf[2] ^ CommBuf[3];
	FirmSendData(CommBuf, 5);
	if (FirmWaitAck(0, 0) == false) {
		return false;
	}

	TraceStr("read num \r\n");
	//Send Read Num
	CommBuf[0] = (len - 1);
	CommBuf[1] = 0xff - CommBuf[0];
	FirmSendData(CommBuf, 2);
	if (FirmWaitAck(200, 20) == false) {
		return false;
	}

	TraceStr("read ok \r\n");
	return true;


}

bool FirmCmdGo(u32 addr)
{

	TraceStr("go cmd\r\n");
	CommBuf[0] = CMD_GO;
	CommBuf[1] = 0xFF - CommBuf[0];
	FirmSendData(CommBuf, 2);
	if (FirmWaitAck(0, 10) == false) {
		return false;
	}

	//Send Addr
	CommBuf[0] = (addr >> 24) & 0xff;
	CommBuf[1] = (addr >> 16) & 0xff;
	CommBuf[2] = (addr >> 8) & 0xff;
	CommBuf[3] = (addr) & 0xff;
	CommBuf[4] = CommBuf[0] ^ CommBuf[1] ^ CommBuf[2] ^ CommBuf[3];
	FirmSendData(CommBuf, 5);
	if (FirmWaitAck(0, 10) == false) {
		return false;
	}

	return true;
}

bool FirmCmdWrite(u8 *buf, u32 addr, u16 len)
{
	u8 xor_value;
	u32 i;
	// Send Cmd
	CommBuf[0] = CMD_WRITE;
	CommBuf[1] = 0xFF - CommBuf[0];
	FirmSendData(CommBuf, 2);
	if (FirmWaitAck(300, 0) == false) {
		TraceStr("WRITE CMD , NO ACK\r\n");
		return false;
	}

	//Send Addr
	CommBuf[0] = (addr >> 24) & 0xff;
	CommBuf[1] = (addr >> 16) & 0xff;
	CommBuf[2] = (addr >> 8) & 0xff;
	CommBuf[3] = (addr) & 0xff;
	CommBuf[4] = CommBuf[0] ^ CommBuf[1] ^ CommBuf[2] ^ CommBuf[3];
	FirmSendData(CommBuf, 5);
	if (FirmWaitAck(300, 0) == false) {
		TraceStr("WRITE ADDR , NO ACK\r\n");
		return false;
	}

	//Send Data
	CommBuf[0] = (len - 1) & 0xff;
	xor_value = CommBuf[0];
	for (i = 0; i < len; i++) {
		CommBuf[i + 1] = buf[i];
		xor_value ^= CommBuf[i + 1];
	}
	CommBuf[len + 1] = xor_value;

	FirmSendData(CommBuf, len + 2);
	if (FirmWaitAck(300, 0) == false) {
		TraceStr("WRITE DATA , NO ACK\r\n");
		return false;
	}
	return true;

}


bool FirmCmdEnableRP(void)
{
	// Send Cmd
	CommBuf[0] = CMD_READ_PROTECT;
	CommBuf[1] = 0xFF - CommBuf[0];
	FirmSendData(CommBuf, 2);
	if (FirmWaitAck(0, 200) == false) {
		return true;
	}

	return true;
}

bool FirmCmdCheckRP()
{
	u32 addr = 0x20000000;
	// Send Cmd
	CommBuf[0] = CMD_READ;
	CommBuf[1] = 0xFF - CommBuf[0];
	FirmSendData(CommBuf, 2);
	if (FirmWaitAck(500, 10) == false) {
		TraceStr("SEND CMD-READ failed\r\n");
		return true;
	}

	//Send Addr
	CommBuf[0] = (addr >> 24) & 0xff;
	CommBuf[1] = (addr >> 16) & 0xff;
	CommBuf[2] = (addr >> 8) & 0xff;
	CommBuf[3] = (addr) & 0xff;
	CommBuf[4] = CommBuf[0] ^ CommBuf[1] ^ CommBuf[2] ^ CommBuf[3];
	FirmSendData(CommBuf, 5);
	if (FirmWaitAck(500, 0) == false) {
		TraceStr("SEND address failed\r\n");
		return true;
	}

	//Read number
	CommBuf[0] = 3;
	CommBuf[1] = CommBuf[0] ^ 0xff;
	FirmSendData(CommBuf, 2);
	if (FirmWaitAck(500, 0) == false) {
		TraceStr("SEND number to reade , failed");
		return true;
	}

	return false;
}

bool FirmCmdDisableRP()
{
	// Send Cmd
	CommBuf[0] = CMD_READ_UNPROTECT;
	CommBuf[1] = 0xFF - CommBuf[0];
	FirmSendData(CommBuf, 2);
	//Wait for cmd ack
	if (FirmWaitAck(0, 10) == false) {
		return true;
	}
	TraceStr("remove rp cmd ack\r\n");
	//Wait for erase ack
	if (FirmWaitAck(25000, 500) == false) {
		return true;
	}
	TraceStr("remove rp cmd complete\r\n");
	return true;
}
#define bootloader_version		V_3_0

bool FirmTest(void)
{
	//Switch to firm mode anyway
	//drv_comm_uart_init();

	ReinitFIFO(&UartRxFifo);

	TraceStr("FirmTest \r\n");
	FirmStart();                                          //CMD : start(0x7f)
	if (FirmWaitAck(0, 10) == false) {
		TraceStr("FirmStart failed\r\n");
		return false;
	} else {
		TraceStr("FirmStart success\r\n");
	}

#ifdef  bootloader_version
	TraceStr("Start FirmCheckCodeNeedErase V 3.0\r\n");
	if (FirmCheckCodeNeedErase()) {                      //CDM : READ(0x11)  and check data
		TraceStr("Start erase code \r\n");
		if (FirmCmdErase_V3_0()) {                              //CMD : ERASE(0x44)
			TraceStr("Erase Firm success\r\n");
		} else {
			TraceStr("Erase Firm failed\r\n");
			return false;
		}
		/*
		TraceStr("FirmStart\r\n");
		FirmStart();                                       //CMD : START(0x7f)
		if (FirmWaitAck(0,10)==false)
		{
			TraceStr("FirmStart failed\r\n");
		}
		*/
	}
	return true;
#else
	TraceStr("Start FirmCheckCodeNeedErase \r\n");
	if (FirmCheckCodeNeedErase()) {                      //CDM : READ(0x11)  and check data
		TraceStr("Start erase code \r\n");
		if (FirmCmdErase()) {                              //CMD : ERASE(0x44)
			TraceStr("Erase Firm success\r\n");
		} else {
			TraceStr("Erase Firm failed\r\n");
			return false;
		}
		/*
		TraceStr("FirmStart\r\n");
		FirmStart();                                       //CMD : START(0x7f)
		if (FirmWaitAck(0,10)==false)
		{
			TraceStr("FirmStart failed\r\n");
		}
		*/
	}
	return true;
#endif
	//TraceStr("write memory start \r\n");
	//FirmDownload();   //请求下载文件
	/*
	if (FirmCmdGo(RAM_START_ADDR)==true)
	{
		ReinitFIFO(&UartRxFifo);
		drv_comm_uart_init();
		return true;
	}
	else
	{
		drv_comm_uart_init();
		return false;
	}*/
}

bool FirmReady(void)
{
	//Switch to firm mode anyway
	drv_comm_uart_init();

	ReinitFIFO(&UartRxFifo);

	TraceStr("Boot in Firm mode \r\n");
	FirmStart();
	if (FirmWaitAck(0, 20) == false) {
		TraceStr("Firm mode not Ready \r\n");
		return false;
	} else {
		TraceStr("Firm mode Ready \r\n");
	}
	return true;
}

static bool FirmWaitAck(u16 before, u16 after)
{
	u32 i;
	g_check_uart_comm_msg_flag = false;
	if (before < 100) {
		i = 100;
	} else {
		i = before;
	}
	while (1) {
		TraceStr("@");
		if (i > 1) {
			i = i - 1;
		}
		if (g_check_uart_comm_msg_flag) {
			break;
		}
		if (i == 1) {
			TraceStr("ACK timeout\r\n");
			return false;
		}
		vTaskDelay(10);
	}
	if (after == 0) {
		vTaskDelay(1);
	} else {
		vTaskDelay(after);
	}

	FirmRcvLen = RcvFirmFrameFromUart(&UartRxFifo, CommBuf);

	if (CommBuf[0] == 0x79) {
		return true;
	} else {
		TraceStr("ACK Error\r\n");
		Trace("ack value", CommBuf[0]);
		return false;
	}
}

//unsigned char MCUCodeBuf[1024];
static bool FirmDownload(void)
{
	u32 baseaddr = RAM_START_ADDR;
	u32 i, num;
	u32 len = TEST_CODE_LENGTH;
	u8 *p = (u8 *)TEST_CODE_START_ADDR;
	/*
		i = 0;
		while (1)
		{
			if ((i+256)<=len)
			{
				if (FirmCmdWrite(p,baseaddr,256)==false) return false;
				i = i + 256;
				baseaddr += 256;
				p += 256;
			}
			else
			{
				num = ((len - i)+8)/4 * 4;
				if (FirmCmdWrite(p,baseaddr,num)==false) return false;
				i = i + num;
				baseaddr += num;
				p += num;
				break;
			}
		}
		*/
	/*
	comm_head_t *comm_head;
	void *msg_addr = NULL;
	u32 malloc_size;
	//1. Malloc memory
	 malloc_size = CALC_MALLOC_SIZE(sizeof(comm_head_t));
	 msg_addr = pvPortMalloc(malloc_size);

	 if (msg_addr == NULL)
	 {
	 	TraceStr("Malloc memory failed! \r\n");
	 	return false;
	 }
	 comm_head = (comm_head_t *)msg_addr;


	comm_head->start_bytes = 0xAAAAAAAA;
	comm_head->msg_len = 16;
	comm_head->service_type = SERVICE_TYPE_CMD;
	comm_head->msg_type = MSG_TYPE_CMD_TEST_MCU_CODE_REQ;
	comm_head->resp = 0;
	comm_head->crc32 = 0;
	//请求发送MCU_CODE
	MsgSendFromTest1(comm_head,TASK_USB);
	//接收MCU_CODE，写入


	//回复 接收完毕
	*/
	return true;
}






