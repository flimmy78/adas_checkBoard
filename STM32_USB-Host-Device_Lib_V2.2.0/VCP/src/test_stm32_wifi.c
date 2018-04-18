#include "inc_all.h"

#define	ACK_RUN_OK	0
#define	ACK_RUN_ERROR	1

#define PC_CMD_SWITCH  	0x51
#define PC_CMD_WARN  	0x52
#define PC_CMD_RESET  	0x53
#define PC_TEST_STOP  	0x54
#define PC_TEST_BEEP  	0x55

u16 Current0;
u16 Current1;


/****************************************************************************/
/*External  Variables */
extern QueueHandle_t Usb_Queue;
extern Measurement_Para_t MeasPara;
extern volatile bool can_send_flag;
/****************************************************************************/




/****************************************************************************/
/*External  Functions */
extern void ReadADC(void);
extern void ReadINA226(void);
extern void init_INA226_normal(void);
extern void InitADC(void);
extern bool FirmTest(void);
extern bool FirmCmdWrite(u8 *buf, u32 addr, u16 len);
extern bool FirmReady(void);
extern u8 CheckFIFOByte(FIFO_t *stFIFO);
extern bool FirmCmdEnableRP(void);
/****************************************************************************/



/****************************************************************************/
/*Global  Variables */
QueueHandle_t Test_Queue;
FIFO_t		UartRxFifo;

volatile bool g_check_uart_comm_msg_flag = true;


__ALIGN_BEGIN

u8  comm_uart_rx_buffer[MAX_COMM_UART_DMA_RCV_SIZE];
u8  comm_uart_tx_buffer[MAX_COMM_UART_DMA_SND_SIZE];
u8  UartRcvFrameBuffer[MAX_UART_FRAME_LENGTH];

__ALIGN_END;

TEST_STAGE_enum test_stage;
TEST_STAGE_enum MAINBOARD_STATE;

tlv_reply_t board_reply;

/****************************************************************************/



/****************************************************************************/
/*Global  Functions */
u8 GetTestStage(void);
/****************************************************************************/


/****************************************************************************/
/*Local  Variables*/

static u8 uart_dma_start = 0;
static u8 TestCmdBuff[4];

/****************************************************************************/


/****************************************************************************/
/*Local  Functions*/
static void TestServiceCmd(comm_head_t *msg_head);
static void MsgSendFromTest(comm_head_t *test_msg_head, task_id_enum totask);
//static void TestWarnReply(bool status);
static void UartCommFuncs(comm_head_t *msg_head);
static void TestServiceFile(comm_head_t *msg_head);
static void TestSendData(u8 *buf, u16 len);
static bool uart_DMA_finished(void);
static void uart_start_TX_DMA(u32 addr, u32 len);
static bool RcvFrameFromUart(FIFO_t *stFIFO, u8 *framebuf);
static bool TestWriteFile(comm_head_t *msg_head);
static void SetTestStage(u8 test_status);
static bool TestWriteFileReply(comm_head_t *msg_head, bool status);
static void TestReplyFunc(void);

//add by yuzongyong
void test_wifi();
void download_WIFI_BIN();
void download_WIFI_connect();
void download_WIFI_Erase_Sram();
void download_WIFI_Write_Sram_Patch(u8 *msg_data);
void download_WIFI_Erase_Flash();
void download_WIFI_Write_Flash_Patch(u8 *msg_head);
void download_WIFI_Write_FS(u8 *msg_data);
/****************************************************************************/





void InitTestVar(void)
{
	//Uart FIFO
	Test_Queue = xQueueCreate(20, sizeof(msg_queue_t));

	InitFIFO(&UartRxFifo, comm_uart_rx_buffer, MAX_COMM_UART_DMA_RCV_SIZE);
}

static void TestFuncs(comm_head_t *msg_head)
{

	//		crc32 = GetCrc32(&framebuf[MSG_CRC_OFFSET],msg_head->msg_len-MSG_CRC_OFFSET);
	//	if (msg_head->crc32 != crc32 ) return;
	switch (msg_head->service_type) {
	case SERVICE_TYPE_CMD:

		TestServiceCmd(msg_head);
		break;
	case SERVICE_TYPE_FILE:
		//TraceStr("start file op \r\n");
		TestServiceFile(msg_head);
		break;
	default:
		break;
	}

}

static void TestServiceFile(comm_head_t *msg_head)
{
	u8 msg_type;
	msg_type = msg_head->msg_type;
	switch (msg_type) {
	case MSG_TYPE_FILE_WRITE_REQ:
		TestWriteFileReply(msg_head, true);
		TestWriteFile(msg_head);
		break;
	default:
		break;
	}

}
static bool TestWriteFileReply(comm_head_t *msg_head, bool status)
{
	msg_head->start_bytes = 0xAAAAAAAA;
	msg_head->msg_len = 16 + sizeof(TLV_FIRM_FILE_t);
	msg_head->service_type = SERVICE_TYPE_FILE;
	msg_head->msg_type = MSG_TYPE_FILE_WRITE_RESP;
	msg_head->resp = status;
	msg_head->crc32 = 0;

	MsgSendFromTest(msg_head, TASK_USB);
	TraceStr("send write file reply \r\n");
	return true;
}


extern u8 strbuf[];



static bool TestWriteFile(comm_head_t *msg_head)
{
	static u32 TotalCount = 0;
	// In firm mode
	u32 start;
	u8 *p;
	u8 i;

	TLV_FIRM_FILE_t *tlv_firm;
	tlv_firm = (TLV_FIRM_FILE_t *)((u32)msg_head + COMM_HEADER_LENGTH);
	//	len = tlv_firm->len;
	start = tlv_firm->start;
	p = (u8 *)((u32)msg_head + COMM_HEADER_LENGTH + sizeof(TLV_FIRM_FILE_t));

	for (i = 0; i < 64; i++) {
		strbuf[i] = 0;
	}
	TotalCount ++;


	//	sprintf(strbuf,"write memory start=0x%x,len = %d, Total: %d, first 4 bytes: 0x%02x,0x%02x,0x%02x,0x%02x\r\n",
	//		tlv_firm->start,tlv_firm->len,TotalCount,ch[0],ch[1],ch[2],ch[3]);
	//	TraceStr(strbuf);
	//	for (i=0;i<64;i++) strbuf[i] = 0;

	i = 0;
	while (i < 4) {
		FirmCmdWrite(p, start, 256);
		start += 256;
		p += 256;
		i++;
	}

	return true;
}


static bool TestStartTest(void)
{

	bool ret; unsigned int sum; int i = 0;
	TraceStr("TestStartTest 0\r\n");
	drv_boot_low();
	drv_power_off();
	MAINBOARD_STATE = MAINBOARD_OFF;
	vTaskDelay(1000);
	for (i = 0; i < 30; i++) {
		ReadINA226();
	}
	for (sum = 0, i = 0; i < 30; i++) {
		ReadINA226();
		sum = sum + Current1;
	}
	Current1 = sum / 30;
	init_INA226_normal();
	InitADC();
	//MAINBOARD_STATE = MAINBOARD_NONE;
	TraceStr(" drv_power_off()\r\n");
	drv_comm_uart_init();
	SetTestStage(NONE_TEST);
	vTaskDelay(500);
	drv_boot_high();
	vTaskDelay(10);
	drv_power_on();
	TraceStr(" drv_power_on()\r\n");
	vTaskDelay(1000);
	//drv_boot_low();
	//TraceStr("TestStartTest 1\r\n");


	ret = FirmTest();

	return ret;

	//return false;
}

static void TestStartWarn(void)
{

	TestCmdBuff[0] = 0x79;    // 0x79
	TestCmdBuff[1] = PC_CMD_WARN;
	TestCmdBuff[2] = TestCmdBuff[1] ^ 0xFF;
	TestCmdBuff[3] = 0;
	TestSendData(TestCmdBuff, 4);
}

static void TestSwitchScreen(void)
{
	//TraceStr("send switch to mainboard \r\n");
	TestCmdBuff[0] = 0x79;
	TestCmdBuff[1] = PC_CMD_SWITCH;
	TestCmdBuff[2] = TestCmdBuff[1] ^ 0xFF;
	TestCmdBuff[3] = 0;
	TestSendData(TestCmdBuff, 4);
}

static void TestBeep(void)
{
	//TraceStr("send switch to mainboard \r\n");
	TestCmdBuff[0] = 0x79;
	TestCmdBuff[1] = PC_TEST_BEEP;
	TestCmdBuff[2] = TestCmdBuff[1] ^ 0xFF;
	TestCmdBuff[3] = 0;
	TestSendData(TestCmdBuff, 4);
}

static void TestCmdReply(bool status, u8 msg_type, u8 Error_OK_Flag)
{
	comm_head_t *comm_head;
	void *msg_addr = NULL;
	u32 malloc_size;


	/*1. Malloc memory */
	malloc_size = CALC_MALLOC_SIZE(sizeof(comm_head_t));
	msg_addr = pvPortMalloc(malloc_size);

	if (msg_addr == NULL) {
		TraceStr("Malloc memory failed! \r\n");
		return ;
	}
	comm_head = (comm_head_t *)msg_addr;


	comm_head->start_bytes = 0xAAAAAAAA;
	comm_head->msg_len = 16;
	comm_head->service_type = SERVICE_TYPE_CMD;
	comm_head->msg_type = msg_type;
	comm_head->resp = Error_OK_Flag;
	comm_head->crc32 = 0;

	MsgSendFromTest(comm_head, TASK_USB);

	//Free memory
	vPortFree(msg_addr);

	TraceStr("TestCmdReply\r\n");


}


void MsgSendFromTest(comm_head_t *test_msg_head, task_id_enum totask)
{

	msg_queue_t msg_queue;
	msg_head_t *msg_head;
	void *msg_addr = NULL;
	u32 malloc_size;


	/*1. Malloc memory */
	malloc_size = CALC_MALLOC_SIZE(sizeof(msg_head_t) + test_msg_head->msg_len);
	msg_addr = pvPortMalloc(malloc_size);

	if (msg_addr == NULL) {
		TraceStr("Malloc msgaddr error!\r\n");
		Trace("malloc size", malloc_size);
		return;
	}

	/*2. Fill header */
	msg_head = (msg_head_t *)msg_addr;
	msg_head->snd_task_id = TASK_TEST;
	msg_head->rcv_task_id = totask;
	msg_head->cmd = 0;
	msg_head->msg_data = (u8 *)((u32)msg_addr + sizeof(msg_head_t));

	/* 3. Copy data */
	memcpy(msg_head->msg_data, &test_msg_head->start_bytes, test_msg_head->msg_len);


	/* Here we send msg  */
	msg_queue.msg_addr = (u32)msg_addr;

	switch (totask) {
	case TASK_USB:
		if (xQueueSend(Usb_Queue, &msg_queue, 20) != pdTRUE) {
			vPortFree(msg_addr);
		}
		break;
	default:
		break;
	}



}

static void TestBootInIAP(void)
{
	u8 i = 10;
	while (i--) {

		drv_power_off();
		vTaskDelay(1500);
		drv_boot_high();
		vTaskDelay(500);
		drv_power_on();
		vTaskDelay(1000);
		//drv_boot_low();
		/*
		drv_power_off();
		TraceStr(" drv_power_off()\r\n");
		drv_comm_uart_init();
		SetTestStage(NONE_TEST);
		vTaskDelay(500);
		drv_boot_high();
		vTaskDelay(10);
		drv_power_on();
		TraceStr(" drv_power_on()\r\n");
		vTaskDelay(300);
		*/
		if (FirmReady()) {
			break;
		}
	}
}

static void TestResultGet(comm_head_t *msg_head)
{
	tlv_test_t *tlv_test;
	tlv_test = (tlv_test_t *)((u32)msg_head + COMM_HEADER_LENGTH);
	board_reply.CpuID0 = *((u32 *)&tlv_test->cpuid[0]);
	board_reply.CpuID1 = *((u32 *)&tlv_test->cpuid[4]);
	board_reply.CpuID2 = *((u32 *)&tlv_test->cpuid[8]);
	board_reply.TestResult0 = tlv_test->test_result & 0xff;
	board_reply.TestResult1 = (tlv_test->test_result >> 8) & 0xff;

	//Trace("FPGA ver",tlv_test->fpga_ver);



}

static void UartTestServiceCmd(comm_head_t *msg_head)
{
	u8 msg_type;
	msg_type = msg_head->msg_type;
	switch (msg_type) {
	case MSG_TYPE_CMD_TEST_RESULT_RESP:
		//TraceStr("MSG_TYPE_CMD_TEST_RESULT_RESP \r\n");
		TestResultGet(msg_head);
	default:
		break;
	}
}

static void BootNormal(void)
{

	// 1. send can
	can_send_flag = true;

	// 2. power on
	drv_power_off();
	drv_comm_uart_init();
	vTaskDelay(200);
	drv_boot_low();
	vTaskDelay(10);
	drv_power_on();
	vTaskDelay(300);
	TraceStr("boot normal\r\n");
	SetTestStage(TEST_WORK);
}

static void BootNoCan(void)
{

	// 1. send can
	can_send_flag = false;

	// 2. power on
	drv_power_off();
	drv_comm_uart_init();
	vTaskDelay(200);
	drv_boot_low();
	vTaskDelay(10);
	drv_power_on();
	vTaskDelay(300);
	SetTestStage(TEST_SLEEP);
}
extern void ReinitFIFO(FIFO_t *stFIFO);
extern bool FirmCmdGo(u32 addr);
unsigned MCU_CODE_Length = 0;
unsigned int receivedFileLength = 0;
#define 	RAM_START_ADDR			0x20002000
u32 baseaddr = RAM_START_ADDR;
static const u8 hex_table[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
static void TestServiceCmd(comm_head_t *msg_head)
{
	unsigned char *temp; unsigned int i = 0; unsigned short current_Pack_Length = 0;
	unsigned int pack_nNum_Now = 0;
	//char PPP[100];
	unsigned int sum;
	comm_head_t *comm_head; void *msg_addr = NULL; u32 malloc_size;
	char *temp_p = (char *)msg_head;
	u8 msg_type;
	msg_type = msg_head->msg_type;
	switch (msg_type) {
	case MSG_TYPE_CMD_START_TEST_REQ:
		TraceStr("TestServiceCmd start test \r\n");

		/*if (TestStartTest()==true)
			{
				//SetTestStage(TEST_RUNNING);
			}
			else
			{
				//Try again
				if (TestStartTest()==true)
				{

				}
			}*/

		TestStartTest();
		can_send_flag = true;
		TestCmdReply(true, MSG_TYPE_CMD_START_TEST_REQ, ACK_RUN_OK);
		TraceStr("reply  MSG_TYPE_CMD_START_TEST_REQ\r\n");
		break;
	case MSG_TYPE_CMD_WARN_REQ:
		TraceStr("TestServiceCmd MSG_TYPE_CMD_WARN_REQ \r\n");
		TestStartWarn();
		break;
	case MSG_TYPE_CMD_SWITCH_SCREEN_REQ:
		//TraceStr("TestServiceCmd MSG_TYPE_CMD_SWITCH_SCREEN_REQ \r\n");
		TestSwitchScreen();
		break;

	case MSG_TYPE_CMD_BOOT_IN_IAP_MODE_REQ:
		TraceStr("TestServiceCmd MSG_TYPE_CMD_BOOT_IN_IAP_MODE_REQ \r\n");
		can_send_flag = false;
		TestBootInIAP();
		TestCmdReply(true, MSG_TYPE_CMD_BOOT_IN_IAP_MODE_REQ, ACK_RUN_OK);
		break;
	case MSG_TYPE_CMD_TEST_RESULT_REQ:
		TraceStr("TestServiceCmd MSG_TYPE_CMD_TEST_RESULT_REQ \r\n");
		TestReplyFunc();
		break;
	case MSG_TYPE_CMD_CRYPT_REQ:
		can_send_flag = false;
		TraceStr("TestServiceCmd MSG_TYPE_CMD_CRYPT_REQ \r\n");
		FirmCmdEnableRP();
		vTaskDelay(200);   // Wait for system reset
		TestCmdReply(true, MSG_TYPE_CMD_CRYPT_REQ, ACK_RUN_OK);
		break;

	case MSG_TYPE_CMD_WORK_CURRENT_REQ:
		TraceStr("TestServiceCmd MSG_TYPE_CMD_WORK_CURRENT_REQ \r\n");
		BootNormal();
		SetTestStage(TEST_WORK);
		TestCmdReply(true, MSG_TYPE_CMD_WORK_CURRENT_REQ, ACK_RUN_OK);
		break;

	case MSG_TYPE_CMD_SLEEP_CURRENT_REQ:
		can_send_flag = false;
		TraceStr("TestServiceCmd MSG_TYPE_CMD_SLEEP_CURRENT_REQ \r\n");
		BootNoCan();
		SetTestStage(TEST_SLEEP);
		TestCmdReply(true, MSG_TYPE_CMD_SLEEP_CURRENT_REQ, ACK_RUN_OK);
		break;

	case MSG_TYPE_CMD_TEST_MCU_CODE_RESP:
		TraceStr("\r\nTEST TASK: MSG_TYPE_CMD_TEST_MCU_CODE_RESP!\r\n");
		/*
			for(i=0;i<49;i++)
			{
				PPP[2*i + 1] = hex_table[temp_p[i]&0x0f];
				PPP[2*i] = hex_table[(temp_p[i] >> 4) &0x0f];
			}
			PPP[98] = NULL;
			*/
		//TraceStr(PPP);
		temp = (unsigned char *)msg_head;
		current_Pack_Length = temp[16 + 2] + temp[16 + 3] * 256;
		baseaddr = temp[16 + 4] + temp[16 + 5] * 256 + temp[16 + 6] * 256 * 256 +
		           temp[16 + 7] * 256 * 256 * 256;
		for (i = 0; (i < 4) & (current_Pack_Length > 0);) {
			//Trace("current_Pack_Length",current_Pack_Length);
			if (current_Pack_Length >= 256) {
				if (FirmCmdWrite(&temp[16 + 8 + i * 256], baseaddr, 256) == false) {
					TraceStr("CMD_TEST_MCU_CODE WRITE FAILED\r\n");
					break;
				} else {
					i++; baseaddr += 256; current_Pack_Length -= 256;
					TraceStr("CMD_TEST_MCU_CODE WRITE SUCCES\r\n");
				}
			} else if ((current_Pack_Length < 256) & (current_Pack_Length > 0)) {
				if (FirmCmdWrite(&temp[16 + 8 + i * 256], baseaddr,
				                 current_Pack_Length) == false) {
					TraceStr(" 0 0 0 0 CMD_TEST_MCU_CODE WRITE FAILED\r\n");
					break;
				} else {
					i++; current_Pack_Length = 0;
					TraceStr(" 0 0 0 0 CMD_TEST_MCU_CODE WRITE SUCCES\r\n");
				}
			} else {

			}
		}
		TestCmdReply(true, MSG_TYPE_CMD_TEST_MCU_CODE_RESP, ACK_RUN_OK);
		TraceStr("reply  MSG_TYPE_CMD_TEST_MCU_CODE_RESP\r\n");
		break;
	/*
		if(receivedFileLength == MCU_CODE_Length)
		{
			TraceStr("\r\nWrite file over!");
			pack_nNum_Now = 0;
			receivedFileLength = 0;
			MCU_CODE_Length = 0;

			//1. Malloc memory
			 malloc_size = CALC_MALLOC_SIZE(sizeof(comm_head_t));
			 msg_addr = pvPortMalloc(malloc_size);

			 if (msg_addr == NULL)
			 {
				TraceStr("Malloc memory failed! \r\n");
				break;
			 }
			 comm_head = (comm_head_t *)msg_addr;


			comm_head->start_bytes = 0xAAAAAAAA;
			comm_head->msg_len = 16;
			comm_head->service_type = SERVICE_TYPE_CMD;
			comm_head->msg_type = MSG_TYPE_CMD_TEST_MCU_CODE_REQ;
			comm_head->resp = 0;
			comm_head->crc32 = 0;
			//请求发送MCU_CODE
			MsgSendFromTest(comm_head,TASK_USB);

		}
			break;*/

	case MSG_TYPE_CMD_TEST_MCU_CODE_RESTART:
		TraceStr("\r\n MSG_TYPE_CMD_TEST_MCU_CODE_RESTART");
		/*
		if (FirmCmdGo(0x08000000)==true)
		{
			TraceStr("\r\n GO success");


			malloc_size = CALC_MALLOC_SIZE(sizeof(comm_head_t));
			msg_addr = pvPortMalloc(malloc_size);
			if (msg_addr == NULL)
			{
				TraceStr("Malloc memory failed! \r\n");
				break;
			}
			comm_head = (comm_head_t *)msg_addr;
			comm_head->start_bytes = 0xAAAAAAAA;
			comm_head->msg_len = 16;
			comm_head->service_type = SERVICE_TYPE_CMD;
			comm_head->msg_type = MSG_TYPE_CMD_TEST_MCU_CODE_RESTART;
			comm_head->resp = 0;
			comm_head->crc32 = 0;
			//请求发送MCU_CODE
			MsgSendFromTest(comm_head,TASK_USB);


			ReinitFIFO(&UartRxFifo);
			drv_comm_uart_init();
			break;
		}
		else
		{
			TraceStr("\r\n GO failed");
			drv_comm_uart_init();
			break;
		}*/
		drv_power_off();
		//while(true)
		TraceStr(" drv_power_off()\r\n");
		//drv_comm_uart_init();
		drv_boot_low();
		SetTestStage(NONE_TEST);
		vTaskDelay(5000);

		vTaskDelay(10);
		drv_power_on();
		TraceStr(" drv_power_on()\r\n");
		vTaskDelay(2000);
		MAINBOARD_STATE = MAINBOARD_WORK;
		for (i = 0; i < 30; i++) {
			ReadINA226();
		}


		for (sum = 0, i = 0; i < 30; i++) {
			ReadINA226();
			sum = sum + Current0;
		}
		Current0 = sum / 30;


		MAINBOARD_STATE = MAINBOARD_NONE;
		drv_comm_uart_init_ChangeSetting();
		SetTestStage(TEST_RUNNING);
		TestCmdReply(true, MSG_TYPE_CMD_TEST_MCU_CODE_RESTART, ACK_RUN_OK);
		break;


	case MSG_TYPE_CMD_BEEP_REQ:
		TestBeep();
		break;

	case MSG_TYPE_CMD_CAN_SENSOR_TEST_REQ:

		break;

	case MSG_TYPE_CMD_TEST_WIFI_CODE_CONNECT_RESP:
		download_WIFI_connect();
		break;

	case MSG_TYPE_CMD_TEST_WIFI_CODE_ERASE_SRAM_REQ:
		download_WIFI_Erase_Sram();
		break;

	case MSG_TYPE_CMD_TEST_WIFI_CODE_WRITE_SRAM_PATCH_REQ:
		download_WIFI_Write_Sram_Patch((u8 *)msg_head);
		break;

	case MSG_TYPE_CMD_TEST_WIFI_CODE_ERASE_FLASH_REQ:
		download_WIFI_Erase_Flash();
		break;

	case MSG_TYPE_CMD_TEST_WIFI_CODE_WRITE_FLASH_PATCH_REQ:
		download_WIFI_Write_Flash_Patch((u8 *)msg_head);
		break;

	case MSG_TYPE_CMD_TEST_WIFI_CODE_WRITE_FS_REQ:
		download_WIFI_Write_FS((u8 *)msg_head);
		break;

	default:
		break;
	}
}




static void LedFunc(void)
{
	static u32 PreTick = 0;
	u32 CurTick;

	CurTick = xTaskGetTickCount();

	if (CurTick - PreTick > 300) {
		LED1_PORT->ODR ^= LED1_PIN;
		LED2_PORT->ODR ^= LED2_PIN;
		PreTick = CurTick;
	}
}

static void TestReplyFunc(void)
{
	int i; char PPP[108]; unsigned char *temp_p;
	comm_head_t *comm_head;
	tlv_reply_t *tlv_reply;
	void *msg_addr = NULL;
	u32 malloc_size;
	static u32 PreTick;
	u32 CurTick;
	u8 curMode;

	CurTick = xTaskGetTickCount();
	if (CurTick - PreTick < 500) {
		return;
	}
	PreTick = CurTick;


	/*1. Malloc memory */
	malloc_size = CALC_MALLOC_SIZE(COMM_HEADER_LENGTH + TLV_REPLY_LENGTH);
	msg_addr = pvPortMalloc(malloc_size);

	if (msg_addr == NULL) {
		TraceStr("Malloc memory failed! \r\n");
		return ;
	}

	comm_head = (comm_head_t *)msg_addr;

	comm_head->start_bytes = 0xAAAAAAAA;
	comm_head->msg_len = COMM_HEADER_LENGTH + TLV_REPLY_LENGTH ;
	comm_head->service_type = SERVICE_TYPE_CMD;
	comm_head->msg_type = MSG_TYPE_CMD_TEST_RESULT_RESP;
	comm_head->resp = 0;
	comm_head->crc32 = 0;


	tlv_reply = (tlv_reply_t *)((u32)msg_addr + COMM_HEADER_LENGTH);

	tlv_reply->CpuID0 = board_reply.CpuID0;
	tlv_reply->CpuID1 = board_reply.CpuID1;
	tlv_reply->CpuID2 = board_reply.CpuID2;

	//tlv_reply->Current0 = MeasPara.WorkCurrent;
	//tlv_reply->Current1 = MeasPara.SleepCurrent;
	tlv_reply->Current0 = Current0;
	tlv_reply->Current1 = Current1;

	tlv_reply->TestResult0 = board_reply.TestResult0;//board_reply.TestResult0;
	tlv_reply->TestResult1 = board_reply.TestResult1;
	tlv_reply->TestResult2 = 0;
	tlv_reply->TestResult3 = 0;

	tlv_reply->Volatage0 = MeasPara.Voltage[0];
	tlv_reply->Volatage1 = MeasPara.Voltage[1];
	tlv_reply->Volatage2 = MeasPara.Voltage[2];
	tlv_reply->Volatage3 = MeasPara.Voltage[3];
	tlv_reply->Volatage4 = MeasPara.Voltage[4];
	tlv_reply->Volatage5 = MeasPara.Voltage[5];
	tlv_reply->Volatage6 = MeasPara.Voltage[6];
	tlv_reply->Volatage7 = MeasPara.Voltage[7];
	tlv_reply->Volatage8 = MeasPara.Voltage[8];
	curMode = GetTestStage();
	switch (curMode) {
	case TEST_SLEEP:
		tlv_reply->Mode = 3;
		break;
	case TEST_WORK:
		tlv_reply->Mode = 2;
		break;
	case TEST_RUNNING:
		tlv_reply->Mode = 1;
		break;
	default:
		tlv_reply->Mode = 0;
		break;
	}
	temp_p = (unsigned char *)comm_head;
	for (i = 0; i < comm_head->msg_len; i++) {
		PPP[2 * i + 1] = hex_table[temp_p[i] & 0x0f];
		PPP[2 * i] = hex_table[(temp_p[i] >> 4) & 0x0f];
	}
	PPP[2 * (comm_head->msg_len) + 1] = NULL;
	TraceStr("reply code:");
	TraceStr(PPP);

	MsgSendFromTest(comm_head, TASK_USB);

	//Free memory
	vPortFree(msg_addr);

}

static void SetTestStage(u8 test_status)
{
	test_stage = (TEST_STAGE_enum)test_status;
}

u8 GetTestStage(void)
{
	return test_stage;
}

u8 GetMainBoard_State()
{
	return MAINBOARD_STATE;
}



static void ReadTestPara()
{
	static u32 PreTick = 0;
	u32 CurTick ;

	CurTick = xTaskGetTickCount();

	if ((CurTick - PreTick) < 500) {
		return;
	}

	PreTick = CurTick;

	ReadADC();
	ReadINA226();
}


void test_task(void *taskparam)
{
	msg_queue_t msg_queue;
	msg_head_t *msg_head;
	u32 msg_type;

	init_INA226_normal();
	InitADC();


	/* Main loop */
	while (1) {
		//TraceStr(" test task!\r\n");
		if (g_check_uart_comm_msg_flag) {
			if (test_stage >= TEST_RUNNING) {
				g_check_uart_comm_msg_flag = false;
				if (RcvFrameFromUart(&UartRxFifo, UartRcvFrameBuffer)) {
					UartCommFuncs((comm_head_t *)UartRcvFrameBuffer);
				}
			}
		}
		if (xQueueReceive(Test_Queue, &msg_queue, 10) == pdTRUE) {
			//TraceStr("test: msg rcv\r\n");
			//Send data to uart
			msg_head = (msg_head_t *)msg_queue.msg_addr;
			msg_type = (msg_head->snd_task_id << 8) | msg_head->rcv_task_id;
			switch (msg_type) {
			case MSG_USB_TEST:
				TestFuncs((comm_head_t *)msg_head->msg_data);
				break;
			default:
				break;
			}
			vPortFree(msg_head);
		}
		LedFunc();
		ReadTestPara();

	}
}



/* Comm func */
static void UartCommFuncs(comm_head_t *msg_head)
{
	char PPP[81]; int i; char *temp_p;
	switch (msg_head->service_type) {
	case SERVICE_TYPE_FILE:
		//UartTestServiceFile(msg_head);
		break;
	case SERVICE_TYPE_CMD:

		temp_p = (char *)msg_head;

		for (i = 0; i < 40; i++) {
			PPP[2 * i + 1] = hex_table[temp_p[i] & 0x0f];
			PPP[2 * i] = hex_table[(temp_p[i] >> 4) & 0x0f];
		}
		PPP[80] = NULL;
		TraceStr("\r\nmainboard report:");
		TraceStr(PPP);

		UartTestServiceCmd(msg_head);
	default:
		break;
	}
}

#if 0
static void TestWarnReply(bool status)
{

	comm_head_t *comm_head;
	void *msg_addr = NULL;
	u32 malloc_size;


	/*1. Malloc memory */
	malloc_size = CALC_MALLOC_SIZE(sizeof(comm_head_t));
	msg_addr = pvPortMalloc(malloc_size);

	if (msg_addr == NULL) {
		TraceStr("Malloc memory failed! \r\n");
		return ;
	}
	comm_head = (comm_head_t *)msg_addr;


	comm_head->start_bytes = 0xAAAAAAAA;
	comm_head->msg_len = 16;
	comm_head->service_type = SERVICE_TYPE_CMD;
	comm_head->msg_type = MSG_TYPE_CMD_WARN_REQ;
	comm_head->resp = status;
	comm_head->crc32 = 0;

	MsgSendFromTest(comm_head, TASK_USB);

	//Free memory
	vPortFree(msg_addr);


}
#endif




static void CheckFrameHeaderFromUartFIFO(FIFO_t *stFIFO, u8 *framebuf)
{
	u8 i;
	u32 tmp;
	tmp = stFIFO->head;   /* Remember head */
	for (i = 0; i < (UART_HEAD_LENGTH); i++) {
		framebuf[i]  = DeFIFO(stFIFO);
	}
	/*Restore head */
	stFIFO->head = tmp;
}



static bool RcvFrameFromUart(FIFO_t *stFIFO, u8 *framebuf)
{
	/* Find a head */
	u16 frame_len, i;
	comm_head_t *msg_head;

	while (1) {
		if (FIFOLen(stFIFO) < (UART_HEAD_LENGTH)) {
			return false;    /* Frame not receive finished, return */
		}
		if (CheckFIFOByte(stFIFO) == (UART_HEAD_START_BYTES & 0xFF)) {

			/* Check it is a head */
			CheckFrameHeaderFromUartFIFO(stFIFO, framebuf);
			msg_head = (comm_head_t *)framebuf;
			frame_len = msg_head->msg_len;
			if (msg_head->start_bytes != UART_HEAD_START_BYTES) {
				DeFIFO(stFIFO);
				continue;
			}
			if ((frame_len >= MAX_UART_FRAME_LENGTH)
			    || (frame_len <
			        MIN_UART_FRAME_LENGTH)) { /* Something error,but head is matched */
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


u16 RcvFirmFrameFromUart(FIFO_t *stFIFO, u8 *framebuf)
{

	u16 i, len;
	//TraceStr("RcvFirmFrameFromUart\r\n");
	len = FIFOLen(stFIFO);
	if (len == 0) {
		return 0;    /* Frame not receive finished, return */
	}

	for (i = 0; i < len; i++) {
		framebuf[i] = DeFIFO(stFIFO);
	}

	return len;

}

u16 RcvWIFI_FirmFrameFromUart(FIFO_t *stFIFO, u8 *framebuf, u8 length)
{

	u16 i, len;
	//TraceStr("RcvFirmFrameFromUart\r\n");
	len = FIFOLen(stFIFO);
	if (len < length) {
		return 0;    /* Frame not receive finished, return */
	}

	for (i = 0; i < len; i++) {
		framebuf[i] = DeFIFO(stFIFO);
	}

	return len;

}

//static void UartSendData(msg_head_t *msg_head)
//{
//

//	u32 i,len;
//	comm_head_t *comm_head;
//	u8 *p;
//
//	if (uart_dma_start) while (!uart_DMA_finished()) vTaskDelay(1);

//	uart_dma_start = 1;

//	if (msg_head->msg_data == NULL) return;

//	comm_head = (comm_head_t *)msg_head->msg_data;
//	len = comm_head->msg_len;
//	p = (u8 *)msg_head->msg_data;
//	//Copy data
//	for (i=0;i<len;i++)
//	{
//		comm_uart_tx_buffer[i] = *p++;
//	}

//	uart_start_TX_DMA((u32)&comm_uart_tx_buffer,len);

//}


static bool uart_DMA_finished(void)
{

	if ((DMA_GetFlagStatus(DMA1_FLAG_TC7) == SET)
	    || (DMA_GetCurrDataCounter(COMM_UART_TX_DMA) == 0)) {
		DMA_ClearFlag(DMA1_FLAG_TC7);
		DMA_Cmd(COMM_UART_TX_DMA, DISABLE);
		USART_ClearFlag(COMM_UART, USART_IT_TC);
		return true;
	} else {
		return false;
	}
}


static void uart_start_TX_DMA(u32 addr, u32 len)
{
	COMM_UART_TX_DMA->CMAR = addr;
	COMM_UART_TX_DMA->CNDTR = len;
	DMA_Cmd(COMM_UART_TX_DMA, ENABLE);
	USART_DMACmd(COMM_UART, USART_DMAReq_Tx, ENABLE);
}


void FirmSendData(u8 *buf, u16 len)
{
	u32 i;

	if (uart_dma_start) while (!uart_DMA_finished()) {
			vTaskDelay(1);
		}

	uart_dma_start = 1;

	if (buf == NULL) {
		return;
	}
	//Copy data
	for (i = 0; i < len; i++) {
		comm_uart_tx_buffer[i] = buf[i];
	}

	uart_start_TX_DMA((u32)&comm_uart_tx_buffer, len);

}


static void TestSendData(u8 *buf, u16 len)
{
	u32 i;

	if (uart_dma_start) while (!uart_DMA_finished()) {
			vTaskDelay(1);
		}

	uart_dma_start = 1;

	if (buf == NULL) {
		return;
	}
	//Copy data
	for (i = 0; i < len; i++) {
		comm_uart_tx_buffer[i] = buf[i];
	}

	uart_start_TX_DMA((u32)&comm_uart_tx_buffer, len);

}



void MsgSendFromTest1(comm_head_t *test_msg_head, task_id_enum totask)
{

	msg_queue_t msg_queue;
	msg_head_t *msg_head;
	void *msg_addr = NULL;
	u32 malloc_size;


	/*1. Malloc memory */
	malloc_size = CALC_MALLOC_SIZE(sizeof(msg_head_t) + test_msg_head->msg_len);
	msg_addr = pvPortMalloc(malloc_size);

	if (msg_addr == NULL) {
		TraceStr("Malloc msgaddr error!\r\n");
		Trace("malloc size", malloc_size);
		return;
	}

	/*2. Fill header */
	msg_head = (msg_head_t *)msg_addr;
	msg_head->snd_task_id = TASK_TEST;
	msg_head->rcv_task_id = totask;
	msg_head->cmd = 0;
	msg_head->msg_data = (u8 *)((u32)msg_addr + sizeof(msg_head_t));

	/* 3. Copy data */
	memcpy(msg_head->msg_data, &test_msg_head->start_bytes, test_msg_head->msg_len);


	/* Here we send msg  */
	msg_queue.msg_addr = (u32)msg_addr;

	switch (totask) {
	case TASK_USB:
		if (xQueueSend(Usb_Queue, &msg_queue, 20) != pdTRUE) {
			vPortFree(msg_addr);
		}
		break;
	default:
		break;
	}



}



/***********************************  add by yuzongyong  **************************************/
static unsigned char commBuf[1024];   static int rcvFlag = 0;
static int rcvLen = 0;
extern u16 RcvFirmFrameFromUart(FIFO_t *stFIFO, u8 *framebuf);
extern FIFO_t		UartRxFifo;

static unsigned char CMD_Get_Status[] = {0x00, 0x03, 0x23, 0x23};
static unsigned char CMD_Get_Storage_List[] = {0x00, 0x03, 0x27, 0x27};
static unsigned char CMD_Get_Version_Info[] = {0x00, 0x03, 0x2f, 0x2f};
static unsigned char CMD_Get_SRAM_Storage_Info[] = {0x00, 0x07, 0x31, 0x31, 0x00, 0x00, 0x00, 0x00};
static unsigned char CMD_Get_FLASH_Storage_Info[] = {0x00, 0x07, 0x33, 0x31, 0x00, 0x00, 0x00, 0x02};
static unsigned char CMD_Execute_from_RAM[] = {0x00, 0x03, 0x32, 0x32};
static unsigned char CMD_Switch_UART_to_APPS_MCU[] = {0x00, 0x07, 0x5b, 0x33, 0x01, 0x96, 0xe6, 0xab};
static unsigned char CMD_ACK[] = {0x00, 0xcc};
static unsigned char CMD_Raw_SRAM_Storage_Erase[] = {0x00, 0x0f, 0x33, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03};
static unsigned char CMD_Raw_FLASH_Storage_Erase[] = {0x00, 0x0f, 0x33, 0x30, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03};
//Target Connection


#define low		0
#define high	1

void RcvDataFrom_CC3220(unsigned char *rcvbuff, int *len)
{
	*len = RcvFirmFrameFromUart(&UartRxFifo, rcvbuff);
	if (*len > 0) {
		rcvFlag = 1;
	}
}


void sendbreak()
{
	GPIO_InitTypeDef  GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	GPIO_ResetBits(GPIOC, GPIO_Pin_10);
	//GPIO_SetBits(GPIOC,GPIO_Pin_10);
}
void powerOn3_3V()
{
	GPIO_InitTypeDef  GPIO_InitStructure; int len; int cnt = 0;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOD, &GPIO_InitStructure);
	GPIO_SetBits(GPIOD, GPIO_Pin_2); //POWER ON 3.3V
}

void powerOff3_3V()
{
	GPIO_InitTypeDef  GPIO_InitStructure; int len; int cnt = 0;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOD, &GPIO_InitStructure);
	GPIO_ResetBits(GPIOD, GPIO_Pin_2); //POWER ON 3.3V
}
void nRest(unsigned char high_low)
{
	GPIO_InitTypeDef  GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOE, &GPIO_InitStructure);
	if (high_low == low) {
		GPIO_ResetBits(GPIOE, GPIO_Pin_5);    //POWER ON 3.3V
	} else {
		GPIO_SetBits(GPIOE, GPIO_Pin_5);    //POWER ON 3.3V
	}
}
void sop(unsigned char high_low)
{
	GPIO_InitTypeDef  GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOE, &GPIO_InitStructure);
	if (high_low == low) {
		GPIO_ResetBits(GPIOE, GPIO_Pin_6);    //POWER ON 3.3V
	} else {
		GPIO_SetBits(GPIOE, GPIO_Pin_6);    //POWER ON 3.3V
	}
}
bool waitForACK(int times, u8 followLen)
{
	u16 len; u16 i; u16 j;
	for (i = 0; i < times; i++) {
		len = RcvWIFI_FirmFrameFromUart(&UartRxFifo, commBuf, 2);
		if (len >= 2) {
			Trace("len", (unsigned int)len);
			if ((commBuf[len - followLen - 2] == 0x00)
			    && (commBuf[len - followLen - 1] == 0xcc)) {
				break;
			} else if ((commBuf[len - followLen - 2] == 0x00)
			           && (commBuf[len - followLen - 1] == 0x33)) {
				TraceStr("\r\n NACK");
			} else {
				TraceStr("\r\n Error ACK");
				for (j = 0; j < len; j++) {
					Trace("", commBuf[j]);
				}
			}
		}
		vTaskDelay(100);
	}
	if (i >= times) {
		return false;
	} else {
		return true;
	}
}
bool waitForLastStatus(int times)
{
	u16 len; u16 i;
	for (i = 0; i < times; i++) {
		len = RcvWIFI_FirmFrameFromUart(&UartRxFifo, commBuf, 2);
		if (len >= 4) {
			if ((commBuf[len - 1] == commBuf[len - 2]) && (commBuf[len - 3] == 0x03)) {
				break;
			}
		}
		vTaskDelay(100);
	}
	if (i >= times) {
		return false;
	} else {
		return true;
	}
}
#define BREAK_PIN		GPIO_Pin_10
void test_wifi()
{
	GPIO_InitTypeDef GPIO_InitStructure;


	//while (1)
	{
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE, ENABLE);
		//RST_WIFI_PIN
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_Init(GPIOE, &GPIO_InitStructure);
		GPIO_SetBits(GPIOE, GPIO_Pin_3);

		//重新上电
		//GPIO_ResetBits(GPIOE,GPIO_Pin_5);
		drv_power_off();

		vTaskDelay(2000);
		drv_power_on();
		vTaskDelay(500);

		//RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
		//RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE, ENABLE);
		//RST_WIFI_PIN
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_Init(GPIOE, &GPIO_InitStructure);

		//break signal
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_Init(GPIOC, &GPIO_InitStructure);




		//拉低复位脚
		GPIO_ResetBits(GPIOC, BREAK_PIN);
		drv_delayms(10);
		GPIO_ResetBits(GPIOE, GPIO_Pin_5);
		drv_delayms(1000);




		//拉高复位脚
		GPIO_SetBits(GPIOE, GPIO_Pin_5);
		drv_delayms(100);

		//GPIO_SetBits(GPIOB,BREAK_PIN);
		//drv_delayms(5000);
		//GPIO_SetBits(GPIOB,BREAK_PIN);
	}

}


#define	Responses_ACK												0
#define	Responses_NACK											1
#define	Responses_LastStatus								2
#define	Responses_StorageList								3
#define	Responses_StorageInfo								4
#define	Responses_VersionInfo								5

bool WriteData(u8 *buf, u16 bufLen, u16 rcvLen, u8 ResponsesType)
{
	u8 cnt;
	switch (ResponsesType) {
	case Responses_ACK:
		for (cnt = 0; cnt < 5; cnt++) {
			vTaskDelay(1000);
			TestSendData(buf, bufLen);
			if (waitForACK(100, rcvLen - 2) == true) {
				break;
			}
		}
		if (cnt >= 5) {
			return false;
		} else {
			return true;
		}
		break;

	case Responses_NACK:

		break;

	case Responses_LastStatus:
		for (cnt = 0; cnt < 5; cnt++) {
			vTaskDelay(1000);
			TestSendData(buf, bufLen);
			if (waitForLastStatus(100) == true) {
				break;
			}
		}
		if (cnt >= 5) {
			return false;
		} else {
			return true;
		}
		break;

	case Responses_StorageList:

		break;

	case Responses_StorageInfo:

		break;

	case Responses_VersionInfo:

		break;
	}
}

bool waitForStatusOK()
{
	vTaskDelay(10);
	commBuf[5] = 0x00;
	while (true) {
		TestSendData(CMD_Get_Status, 4);
		if (waitForACK(100, 4) == true) {
			if (commBuf[5] == 0x40) {
				break;
			}
		}
	}
	return true;
}

bool target_connection()
{
	DMA_InitTypeDef DMA_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;

	//1.Flush the UART RX line (CC31xx or CC32xx UART TX line)

	//2.Send a break signal (sending continuous spacing values, no start or stop bits) on the CC31xx or CC32xx UART RX line.

	//3.Power on the device (or reset it if it is already up and running).
	//4.The CC31xx or CC32xx device sends an acknowledgment indication.
	//5.On receiving the acknowledgment indication from the CC31xx or CC32xx device, the main processor stops sending the break signal and flushes the UART lines.

	int len; int cnt = 0;

	test_wifi();
	if (waitForACK(100, 0) == false) {
		return false;
	}

	//初始化串口
	/* Configure USART4 Tx () as alternate function push-pull */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	DMA_DeInit(COMM_UART_TX_DMA);
	DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)(&UART4->DR);
	DMA_InitStructure.DMA_MemoryBaseAddr = (u32)0;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
	//设置DMA在传输时缓冲区的长度
	DMA_InitStructure.DMA_BufferSize = 0;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_PeripheralDataSize_Byte;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
	DMA_InitStructure.DMA_Priority = DMA_Priority_High;
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
	DMA_Init(COMM_UART_TX_DMA, &DMA_InitStructure);

	USART_InitStructure.USART_BaudRate = 921600;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;

	DMA_Cmd(COMM_UART_TX_DMA, ENABLE);

	vTaskDelay(10);
	//6.The main processor sends the Get Storage List command.
	//7.The CC31xx or CC32xx device responds with an Ack followed by a 1-byte storage list bitmap.
	if (WriteData(CMD_Get_Storage_List, 4, 3, Responses_ACK) == false) {
		return false;
	}
}
//Target Detection
bool target_detection()
{
	//1. The main processor sends the Get Version Info command.
	//2. The CC31xx or CC32xx device responds with an Ack, followed by the Version Info response.
	if (WriteData(CMD_Get_Version_Info, 4, 0x21, Responses_ACK) == false) {
		return false;
	}
	//3. The main processor responds with an Ack.
	TestSendData(CMD_ACK, 2);
}
//MUX UART to the Network Processor
bool MUX_UART_to_the_Network_Processor()
{
	u8 cnt;
	DMA_InitTypeDef DMA_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);


	//1. The main processor sends the Switch UART to APPS MCU command.
	//2. The CC32xx device responds with an Ack. This is the command response.
	vTaskDelay(1000);
	if (WriteData(CMD_Switch_UART_to_APPS_MCU, 8, 2, Responses_ACK) == false) {
		return false;
	}
	//vTaskDelay(5000);

	//3. The main processor should send a break signal (sending continuous Spacing values [no Start or Stop
	//   bits]) on the CC32xx UART RX line. The network processor must sense this break signal during power up.
	//sendbreak();
	//break signal
	//4. Because there are cases in which the break signal is missed or the Ack packet is being missed, TI
	//   recommends sending the break signal up to four times. Follow the following pseudocode for details.
	//5. The network processor responds with an Ack. This response is an indication that the network
	//   processor sensed the break signal and entered bootloader mode.

	//vTaskDelay(10000);
	for (cnt = 0; cnt < 5; cnt++) {
		//初始化成IO口
		GPIO_InitStructure.GPIO_Pin = BREAK_PIN;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_Init(GPIOC, &GPIO_InitStructure);
		//GPIO_SetBits(GPIOC,BREAK_PIN);
		//vTaskDelay(100);
		GPIO_ResetBits(GPIOC, BREAK_PIN);
		drv_delayms(1000);

		if (waitForACK(2, 0) == true) {
			break;
		}
		GPIO_SetBits(GPIOC, BREAK_PIN);
		vTaskDelay(1);
	}

	if (cnt >= 5) {
		return false;
	}

	//6. The main processor deasserts the break signal.
	//GPIO_SetBits(GPIOC,BREAK_PIN);
	//drv_delayms(10);

	//初始化成串口
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	DMA_DeInit(COMM_UART_TX_DMA);
	DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)(&UART4->DR);
	DMA_InitStructure.DMA_MemoryBaseAddr = (u32)0;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
	//设置DMA在传输时缓冲区的长度
	DMA_InitStructure.DMA_BufferSize = 0;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_PeripheralDataSize_Byte;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
	DMA_InitStructure.DMA_Priority = DMA_Priority_High;
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
	DMA_Init(COMM_UART_TX_DMA, &DMA_InitStructure);

	USART_InitStructure.USART_BaudRate = 921600;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;

	DMA_Cmd(COMM_UART_TX_DMA, ENABLE);

	return true;
}
//Get SRAM Storage Info
u16 block_size = 0;
u32 block_num = 0;
bool Get_SRAM_Storage_Info()
{
	u8 cnt;
	//vTaskDelay(500);
	//1. The main processor sends the Get Storage Info command. The user must provide the storage ID for the SRAM.
	//2. The CC31xx or CC32xx device responds with an Ack followed by the Storage Info response. The response includes the block size and number of blocks for the SRAM.
	if (WriteData(CMD_Get_SRAM_Storage_Info, 8, 0x0d, Responses_ACK) == false) {
		return false;
	}
	block_size = commBuf[6] + commBuf[5] * 256;
	block_num = commBuf[8] + commBuf[7] * 256;

	Trace("block size", block_size);
	Trace("block num", block_num);

	//3. The main processor responds with an Ack.
	TestSendData(CMD_ACK, 2);

	return true;
}
//Raw Storage Erase -- SRAM
bool Raw_Storage_Erase_SRAM()
{
	u8 cnt;
	vTaskDelay(1000);
	//1. The main processor sends the Raw Storage Erase command. The user must provide the storage ID for the SRAM,
	//   offset in blocks, and number of blocks to erase. In this case, erase three blocks starting from offset 0.
	//2. The CC31xx or CC32xx device responds with an Ack.
	//CMD_Raw_SRAM_Storage_Erase[15] = block_num;
	for (CMD_Raw_SRAM_Storage_Erase[2] = 0, cnt = 3; cnt < 16; cnt++) {
		CMD_Raw_SRAM_Storage_Erase[2] += CMD_Raw_SRAM_Storage_Erase[cnt];
	}
	if (WriteData(CMD_Raw_SRAM_Storage_Erase, 16, 2, Responses_ACK) == false) {
		return false;
	}
	//3. The main processor responds with an Ack.
	//TestSendData(CMD_ACK,2);
	//vTaskDelay(100);
	//4. The main processor sends the Get Status command.
	//5. The CC31xx or CC32xx device responds with an Ack followed by the Last Status response. Only the
	//   fourth byte should be inspected, 0x40 means success whereas other values indicate error.
	//TestSendData(CMD_ACK,2);
	//vTaskDelay(1);
	for (cnt = 0; cnt < 5; cnt++) {
		if (waitForStatusOK() == true) {
			break;
		}
	}
	if (cnt >= 5) {
		TraceStr("\r\n\r\nCMD GET STATUS : erase sram failed\r\n");
		return false;
	}
	//6. The main processor sends an Ack response.
	TestSendData(CMD_ACK, 2);
	return true;
}
//Raw Storage Write -- SRAM
static u8 dataToWrite[4096 + 60];
static u32 dataBeenWrited = 0;
bool Raw_Storage_Write_SRAM(u8 *msg_data)
{
	u16 cnt = 0; u32 offset; u32 length; u8 *chunksum; u32 len = 0;
	len = 16 + msg_data[18] + msg_data[19] * 256 - 1;
	//Length
	dataToWrite[0] = (len >> 8) & 0xff;
	dataToWrite[1] = len & 0xff;

	//Opcode
	dataToWrite[3] = 0x2d;

	//ID
	dataToWrite[4] = 0;
	dataToWrite[5] = 0;
	dataToWrite[6] = 0;
	dataToWrite[7] = 0;

	//Offset
	dataToWrite[11] = dataBeenWrited & 0xff;
	dataToWrite[10] = (dataBeenWrited >> 8) & 0xff;
	dataToWrite[9] = (dataBeenWrited >> 16) & 0xff;
	dataToWrite[8] = (dataBeenWrited >> 24) & 0xff;

	//Amount
	length = msg_data[18] + msg_data[19] * 256;
	dataToWrite[15] = length & 0xff;
	dataToWrite[14] = (length >> 8) & 0xff;
	dataToWrite[13] = (length >> 16) & 0xff;
	dataToWrite[12] = (length >> 24) & 0xff;

	//File data
	for (cnt = 0; cnt < length ; cnt++) {
		dataToWrite[16 + cnt] = msg_data[16 + 8 + cnt];
	}

	//Chunksum
	for (dataToWrite[2] = 0, cnt = 3; cnt < 16 + length; cnt++) {
		dataToWrite[2] += dataToWrite[cnt];
	}
	//1. The main processor sends the Raw Storage Write command in chunks of 4080 bytes.
	//2. The CC31xx or CC32xx device responds with an Ack.
	if (WriteData(dataToWrite, len + 1, 2, Responses_ACK) == false) {
		return false;
	}

	//3. The main processor sends the Get Status command.
	//4. The CC31xx or CC32xx device responds with an Ack followed by the Last Status response.
	//   Only the fourth byte should be inspected; 0x40 means success whereas other values indicate error.
	for (cnt = 0; cnt < 5; cnt++) {
		if (waitForStatusOK() == true) {
			break;
		}
	}
	if (cnt >= 5) {
		TraceStr("\r\n\r\nCMD GET STATUS : erase sram failed\r\n");
		return false;
	}
	//5. The main processor sends an Ack response.
	//TestSendData(CMD_ACK,2);
	dataBeenWrited += length;
	TestSendData(CMD_ACK, 2);
	return true;
	//6. Steps 1-5 repeat if the data is larger than chunk size.
}
//Execute from RAM
bool Execute_from_RAM()
{
	u8 cnt = 0;
	//1. The main processor sends the Execute from RAM command.
	//2. The CC31xx or CC32xx device responds with an Ack. This first Ack is to indicate a command is complete.
	if (WriteData(CMD_Execute_from_RAM, 4, 2, Responses_ACK) == false) {
		return false;
	}
	//3. The CC31xx or CC32xx device sends a second Ack to indicate that initialization is completed.
	for (cnt = 0; cnt < 5; cnt++) {
		if (waitForACK(100, 0) == true) {
			break;
		}
	}

	dataBeenWrited = 0;

	if (cnt >= 5) {
		return false;
	} else {
		return true;
	}
}
//Get SFLASH Storage Info
bool Get_SFLASH_Storage_Info()
{
	//1. The main processor sends the Get Storage Info command. The user must provide the storage ID for the SFLASH.
	//2. The CC31xx or CC32xx device responds with an Ack followed by the Storage Info response. The response includes the block size and number of blocks for the SFLASH.
	if (WriteData(CMD_Get_FLASH_Storage_Info, 8, 0x0d, Responses_ACK) == false) {
		return false;
	}

	block_size = commBuf[6] + commBuf[5] * 256;
	block_num = commBuf[8] + commBuf[7] * 256;

	Trace("block size", block_size);
	Trace("block num", block_num);

	//3. The main processor responds with an Ack.
	TestSendData(CMD_ACK, 2);

	return true;

}
//Raw Storage Erase -- SFLASH
bool Raw_Storage_Erase_SFLASH()
{
	u8 cnt = 0;
	//1. The main processor sends the Raw Storage Erase command. The user must provide the storage ID
	//   for the SFLASH, offset in blocks, and number of blocks to erase. In this case, erase two blocks starting
	//   from offset 33.
	//2. The CC31xx or CC32xx device responds with an Ack.
	CMD_Raw_FLASH_Storage_Erase[15] = 0x02;
	CMD_Raw_FLASH_Storage_Erase[11] = 0x21;
	for (CMD_Raw_FLASH_Storage_Erase[2] = 0, cnt = 3; cnt < 16; cnt++) {
		CMD_Raw_FLASH_Storage_Erase[2] += CMD_Raw_FLASH_Storage_Erase[cnt];
	}

	if (WriteData(CMD_Raw_FLASH_Storage_Erase, 16, 2, Responses_ACK) == false) {
		return false;
	}
	//3. The main processor responds with an Ack.
	//4. The main processor sends the Get Status command.
	//5. The CC31xx or CC32xx device responds with an Ack followed by the Last Status response.
	//   Only the fourth byte should be inspected; 0x40 means success whereas other values indicate error.

	vTaskDelay(3000);
	for (cnt = 0; cnt < 5; cnt++) {
		if (waitForStatusOK() == true) {
			break;
		}
	}
	if (cnt >= 5) {
		TraceStr("\r\n\r\nCMD GET STATUS : erase flash failed\r\n");
		return false;
	}
	//6. The main processor sends an Ack response.
	TestSendData(CMD_ACK, 2);
	return true;
}
//Raw Storage Write -- SFLASH
u32 flashOffset = 33 * 4096 + 8;
bool Raw_Storage_Write_SFLASH(u8 *msg_data)
{
	u16 cnt = 0; u32 offset; u32 length; u8 *chunksum; u32 len = 0;
	len = 16 + msg_data[18] + msg_data[19] * 256 - 1;
	//Length
	dataToWrite[0] = (len >> 8) & 0xff;
	dataToWrite[1] = len & 0xff;

	//Opcode
	dataToWrite[3] = 0x2d;

	//ID
	dataToWrite[4] = 0;
	dataToWrite[5] = 0;
	dataToWrite[6] = 0;
	dataToWrite[7] = 0x02;

	//Offset
	dataToWrite[11] = flashOffset & 0xff;
	dataToWrite[10] = (flashOffset >> 8) & 0xff;
	dataToWrite[9] = (flashOffset >> 16) & 0xff;
	dataToWrite[8] = (flashOffset >> 24) & 0xff;

	//Amount
	length = msg_data[18] + msg_data[19] * 256;
	dataToWrite[15] = length & 0xff;
	dataToWrite[14] = (length >> 8) & 0xff;
	dataToWrite[13] = (length >> 16) & 0xff;
	dataToWrite[12] = (length >> 24) & 0xff;

	//File data
	for (cnt = 0; cnt < length ; cnt++) {
		dataToWrite[16 + cnt] = msg_data[16 + 8 + cnt];
	}

	//Chunksum
	for (dataToWrite[2] = 0, cnt = 3; cnt < 16 + length; cnt++) {
		dataToWrite[2] += dataToWrite[cnt];
	}
	//1. The main processor sends the Raw Storage Write command in chunks of 4080 bytes. The user must
	//   provide the storage ID for the SFLASH, offset in bytes, and number of bytes to program. The content
	//   must reside in the 8 bytes offset of block 33, which makes the total offset equal to 33 * 4096 + 8 =
	//   135176 bytes.
	//2. The CC31xx or CC32xx device responds with an Ack.
	if (WriteData(dataToWrite, len + 1, 2, Responses_ACK) == false) {
		return false;
	}

	//3. The main processor sends the Get Status command.
	//4. The CC31xx or CC32xx device responds with an Ack followed by the Last Status response. Only the
	//   fourth byte should be inspected; 0x40 means success whereas other values indicate error.
	for (cnt = 0; cnt < 5; cnt++) {
		if (waitForStatusOK() == true) {
			break;
		}
	}
	if (cnt >= 5) {
		TraceStr("\r\n\r\nCMD GET STATUS : erase sram failed\r\n");
		return false;
	}

	//5. The main processor sends an Ack response.
	flashOffset += length;
	TestSendData(CMD_ACK, 2);
	return true;
	//6. Steps 1 to 5 repeat if the data is larger than chunk size.
}
//Device Reset
bool Device_Reset()
{
	//重新上电
	drv_power_off();
	vTaskDelay(1000);
	drv_power_on();
	vTaskDelay(500);
}
//FS Programming
bool FS_Programming(u8 *msg_data)
{

	u16 cnt = 0; u32 chunk_size; u32 length; u32 len = 0; u32 rcvLen; u16 *size;
	chunk_size = msg_data[18] + msg_data[19] * 256;
	len = 12 + chunk_size - 1;
	//Length
	dataToWrite[0] = (len >> 8) & 0xff;
	dataToWrite[1] = len & 0xff;

	//Opcode
	dataToWrite[3] = 0x34;

	//Key Size
	dataToWrite[4] = 0;
	dataToWrite[5] = 0;

	//Chunk Size
	dataToWrite[6] = (chunk_size >> 8) & 0xff;
	dataToWrite[7] = chunk_size & 0xff;
	Trace("chunk size ", dataToWrite[6] * 256 + dataToWrite[7]);


	//Flags
	dataToWrite[11] = 0;
	dataToWrite[10] = 0;
	dataToWrite[9] = 0;
	dataToWrite[8] = 0;

	//File data
	for (cnt = 0; cnt < chunk_size ; cnt++) {
		dataToWrite[12 + cnt] = msg_data[16 + 8 + cnt];
	}

	//Chunksum
	for (dataToWrite[2] = 0, cnt = 3; cnt < 12 + chunk_size; cnt++) {
		dataToWrite[2] += dataToWrite[cnt];
	}
	//1. The main processor sends the FS Programming command in chunks of 4096 bytes.
	//2. The CC31xx or CC32xx device responds with an Ack followed by a 4-byte response indicating the
	//   accumulated number of bytes received. The status for the last chunk must be 0 to indicate successful
	//   programming, otherwise, a negative status is returned.
	if (chunk_size == 4096) {
		TestSendData(dataToWrite, chunk_size + 12);
		vTaskDelay(500);
		while (waitForACK(100, 4) == false) {
			vTaskDelay(100);
		}
	} else {
		TestSendData(dataToWrite, chunk_size + 12);
		vTaskDelay(10000);
		while (waitForACK(100, 4) == false) {
			vTaskDelay(100);
		}

		//while(waitForACK(100,4) == false)
		//vTaskDelay(100);
	}

	//if(WriteData(dataToWrite , len + 1 , 6 , Responses_ACK) == false)
	//return false;

	rcvLen = commBuf[2] * 256 * 256 * 256 + commBuf[3] * 256 * 256 + commBuf[4] *
	         256 + commBuf[5];
	Trace("FS Programed Len ", rcvLen);


	if (rcvLen == 0) {
		Device_Reset();
		TraceStr("\r\nDevice restarted!");
		return true;
	} else if ((rcvLen % 4096) == 0) {
		return true;
	} else {
		return false;
	}
	//3. Steps 1 and 2 repeat until the entire image is programmed.

	//4. The image gets extracted and the file system is created.
}



void download_WIFI_connect()
{
	if (target_connection()) {
		if (target_detection()) {
			if (MUX_UART_to_the_Network_Processor()) {
				//reply to pc
				TestCmdReply(true, MSG_TYPE_CMD_TEST_WIFI_CODE_CONNECT_RESP, ACK_RUN_OK);
			} else {
				TestCmdReply(true, MSG_TYPE_CMD_TEST_WIFI_CODE_CONNECT_RESP, ACK_RUN_ERROR);
				TraceStr("\r\nDownload WIFI : MUX UART to the Network Processor failed\r\n");
			}
		} else {
			TestCmdReply(true, MSG_TYPE_CMD_TEST_WIFI_CODE_CONNECT_RESP, ACK_RUN_ERROR);
			TraceStr("\r\nDownload WIFI : Target Detection failed\r\n");
		}
	} else {
		TestCmdReply(true, MSG_TYPE_CMD_TEST_WIFI_CODE_CONNECT_RESP, ACK_RUN_ERROR);
		TraceStr("\r\nDownload WIFI : Target Connection failed\r\n");
	}
}


void download_WIFI_Erase_Sram()
{
	if (Get_SRAM_Storage_Info()) {
		if (Raw_Storage_Erase_SRAM()) {
			//reply to pc
			TestCmdReply(true, MSG_TYPE_CMD_TEST_WIFI_CODE_ERASE_SRAM_RESP, ACK_RUN_OK);
		} else {
			TraceStr("\r\nDownload WIFI : Raw Storage Erase SRAM failed\r\n");
			TestCmdReply(true, MSG_TYPE_CMD_TEST_WIFI_CODE_ERASE_SRAM_RESP, ACK_RUN_ERROR);
		}
	} else {
		TraceStr("\r\nDownload WIFI : Get SRAM Storage Info failed\r\n");
		TestCmdReply(true, MSG_TYPE_CMD_TEST_WIFI_CODE_ERASE_SRAM_RESP, ACK_RUN_ERROR);
	}
}


unsigned int writeBytes = 0;
void download_WIFI_Write_Sram_Patch(u8 *msg_data)
{
	if ((msg_data[16 + 2] + msg_data[16 + 3] * 256) == 0) {
		vTaskDelay(10);
		if (Execute_from_RAM()) {
			TraceStr("\r\nDownload WIFI : Execute from RAM successfully\r\n");
			//reply to pc
			TestCmdReply(true, MSG_TYPE_CMD_TEST_WIFI_CODE_WRITE_SRAM_PATCH_RESP,
			             ACK_RUN_OK);
		} else {
			TraceStr("\r\nDownload WIFI : Execute from RAM failed\r\n");
			TestCmdReply(true, MSG_TYPE_CMD_TEST_WIFI_CODE_WRITE_SRAM_PATCH_RESP,
			             ACK_RUN_ERROR);
		}
	} else {
		if (Raw_Storage_Write_SRAM(msg_data)) {
			TestCmdReply(true, MSG_TYPE_CMD_TEST_WIFI_CODE_WRITE_SRAM_PATCH_RESP,
			             ACK_RUN_OK);
		} else {
			TraceStr("\r\nDownload WIFI : Raw Storage Write SRAM patch failed\r\n");
			TestCmdReply(true, MSG_TYPE_CMD_TEST_WIFI_CODE_WRITE_SRAM_PATCH_RESP,
			             ACK_RUN_ERROR);
		}
	}
}


void download_WIFI_Erase_Flash()
{
	if (Get_SFLASH_Storage_Info()) {
		if (Raw_Storage_Erase_SFLASH()) {
			//reply to pc
			TestCmdReply(true, MSG_TYPE_CMD_TEST_WIFI_CODE_ERASE_FLASH_RESP, ACK_RUN_OK);
		} else {
			TraceStr("\r\nDownload WIFI : Raw Storage Erase FLASH failed\r\n");
			TestCmdReply(true, MSG_TYPE_CMD_TEST_WIFI_CODE_ERASE_FLASH_RESP, ACK_RUN_ERROR);
		}
	} else {
		TraceStr("\r\nDownload WIFI : Get FLASH Storage Info failed\r\n");
		TestCmdReply(true, MSG_TYPE_CMD_TEST_WIFI_CODE_ERASE_FLASH_RESP, ACK_RUN_ERROR);
	}
}


void download_WIFI_Write_Flash_Patch(u8 *msg_head)
{
	if (Raw_Storage_Write_SFLASH(msg_head)) {
		//reply to pc
		TestCmdReply(true, MSG_TYPE_CMD_TEST_WIFI_CODE_WRITE_FLASH_PATCH_RESP,
		             ACK_RUN_OK);
	} else {
		TraceStr("\r\nDownload WIFI : Raw Storage Write FLASH patch failed\r\n");
		TestCmdReply(true, MSG_TYPE_CMD_TEST_WIFI_CODE_WRITE_FLASH_PATCH_RESP,
		             ACK_RUN_ERROR);
	}
}


void download_WIFI_Write_FS(u8 *msg_data)
{
	if (FS_Programming(msg_data)) {
		//reply to pc
		TestCmdReply(true, MSG_TYPE_CMD_TEST_WIFI_CODE_WRITE_FS_RESP, ACK_RUN_OK);
	} else {
		TraceStr("\r\nDownload WIFI : FS Programming failed\r\n");
		TestCmdReply(true, MSG_TYPE_CMD_TEST_WIFI_CODE_WRITE_FS_RESP, ACK_RUN_ERROR);
	}
}






void rcvFromTarget(void *taskparam)
{
	TraceStr("start rcvFromTarget process \r\n");
	while (true) {
		if (rcvFlag == 0) {
			RcvDataFrom_CC3220(commBuf, &rcvLen);
		}
		vTaskDelay(100);//延时
	}
}