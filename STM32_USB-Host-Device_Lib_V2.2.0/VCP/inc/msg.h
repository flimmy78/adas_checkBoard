
typedef enum TASK_ID {

	TASK_USB = 0x01,
	TASK_TEST = 0x02,
	TASK_ALL,
} task_id_enum;


typedef enum {

	MSG_USB_TEST = (TASK_USB << 8) | TASK_TEST,
	MSG_TEST_USB = (TASK_TEST << 8) | TASK_USB,
} msg_type_enum;





// MSG header for msgs

typedef struct MSG_QUEUE {
	u32 msg_addr;
} msg_queue_t;


typedef struct MSG_HEADER {
	u8 *msg_data;			/* Pointer to msg body */
	u16 cmd;
	task_id_enum  snd_task_id;
	task_id_enum  rcv_task_id;

} msg_head_t;













#define 	UART_START_BYTES						0xCCCCCCCC
#define		CRC32_FIXED_BYTES						0xBBBBBBBB



#define		MAX_FILENAME_LEN						64

/**************************************************************************************/
/**************************************************************************************/
/**************************************************************************************/


/* TLV type */

#define 	TP_FILE_PARA_ID			0x02
#define		TP_FILE_PACKET_ID		0x04

#define 	TP_WARNING_ID			0x03


#define		TP_FPGA_VER_ID			0x0D
#define		TP_MH_VER_ID			0x07
#define		TP_DEV_VER_ID			0x08
#define		TP_TIME_ID				0x06

#define		TP_LCD_SIZE_ID			0x1A



#define 	TP_CMD_DELAY_ID			0x09
#define 	TP_CMD_WORK_TIME_ID		0x0A
#define     TP_CMD_SWITCH_SCREEN_ID 0x0C

#define		TP_DEBUG_CMD_ID			0x19


#if 0
/* Service type */

#define  	SERVICE_FILE			0x01
#define		SERVICE_WARNING			0x02
#define		SERVICE_SETTING			0x03
//#define     SERVICE_HEARTBEAT		0x05
#define     SERVICE_CMD				0x06
#define     SERVICE_DEBUG  		0x08


/* Msg type */


#define 	MSG_WARNING_REQ    			0x01


#define		MSG_FILE_READ_REQ			0x10
#define		MSG_FILE_WRITE_REQ			0x21
#define		MSG_FILE_WRITE_FIRST_REQ 	0x20

#define		MSG_FILE_READ_FIRST_RESP	0x10
#define		MSG_FILE_READ_RESP			0x11
#define		MSG_FILE_WRITE_FIRST_RESP	0x20
#define		MSG_FILE_WRITE_RESP			0x21


#define 	MSG_PARA_READ_REQ			0x01
#define 	MSG_PARA_SET_REQ			0x02
#define 	MSG_PARA_GET_TIME_REQ		0x03



#define 	MSG_PARA_READ_RESP			0x01
#define 	MSG_PARA_SET_RESP			0x02
#define 	MSG_PARA_GET_TIME_RESP		0x03

#define 	MSG_CMD_RESET_REQ			0x01
#define 	MSG_CMD_RESET_ME_REQ		0x02
#define 	MSG_CMD_RESET_DVR_REQ		0x03
#define 	MSG_CMD_TEST_REQ			0x08
#define   	MSG_CMD_SWITCH_SCREEN   	0x09
#define 	MSG_CMD_POWER_OFF_REQ		0x0a

#define		MSG_CMD_RESET_RESP			0x01
#define		MSG_CMD_RESET_ME_RESP		0x02
#define		MSG_CMD_RESET_DVR_RESP		0x03

#define		MSG_CMD_TEST_RESP			0x08

#define		MSG_DEBUG_MCU_COMMAND 		0x05

#endif
/**************************************************************************************/
/**************************************************************************************/


#define CAN_CAMERA_QUEUE_MAX_NUM	40


/**************************************************************************************/
/**************************************************************************************/
// Msg cmd between task


/**************************************************************************************/

/**************************************************************************************/


typedef enum {
	FILE_READ,
	FILE_WRITE,
	FILE_NONE_OP
} TCP_FILE_OP_enum;

/*   IT to UART app msg  */

typedef struct {
	u32  len;
	u8   *buff;

} IT_UART_msg_t;


/* uart -> hi3518 */
typedef struct {
	u32  len;
	u8   *buff;

} UART_hi3518_msg_t;

/* wifi -> uart */
typedef struct {
	u32  len;
	u8   *buff;

} hi3518_UART_msg_t;


typedef enum {
	FILE_FLASH,
	FILE_ME,
	MAX_FILE_TYPE,
} FILE_TYPE_enum;









typedef struct {
	u16   type;
	u16   length;
} TLV_HEADER_t;




typedef struct {
	u16 type;
	u16 len;
	u16	total_packets;
	u16	cur_packet;
	u16 cur_data_length;
} TP_FILE_PACKET_t;


typedef struct {
	u16  type;
	u16  len;
	u8 	filename[MAX_FILENAME_LEN];
	u32	filelen;
	u32	crc32;

} TLV_FILE_PARA_t;


typedef struct {
	u16  type;
	u16  len;
	u16  curVideoSource;

} TLV_CMD_SWITCH_t;


typedef struct {
	u16  type;
	u16  len;
	u32  start;
} TLV_FIRM_FILE_t;



typedef enum {
	NONE_TEST,
	TEST_DOWNLOAD,
	TEST_RUNNING,
	TEST_OK,
	FIRM_DOWNLOAD,
	TEST_POWER,
	TEST_SLEEP,
	TEST_WORK,

	//ADD BY YUZONGYONG
	MAINBOARD_WORK,   //主板正常工作
	MAINBOARD_OFF,			//主板关机状态
	MAINBOARD_NONE

} TEST_STAGE_enum;













// UART

#define 	UART_HEAD_LENGTH			sizeof(comm_head_t)

#define 	MSG_CRC_OFFSET				8

#define 	UART_HEAD_START_BYTES		0xAAAAAAAA

#define 	MAX_UART_FRAME_LENGTH		(1024+64)
#define 	MIN_UART_FRAME_LENGTH		UART_HEAD_LENGTH

#define 	MAX_COMM_UART_DMA_RCV_SIZE			(2 * 1024 + 200)
#define 	MAX_WIFITest_UART_DMA_RCV_SIZE			(1024)

#define 	MAX_COMM_UART_DMA_SND_SIZE			(2 * 1024 + 200)
#define 	MAX_COMM_UART_FRAME_BUFFER_SIZE		(1024+256)



// ADC and measure


#define 	ADC1_DR_Address    ((u32)0x4001244C)
#define 	ADC_AGV_NUM			16//16
#define		TOTAL_ADC_NUM		9


#define 	RANK1_RATIO			(11/10)		//PC0
#define 	RANK2_RATIO			(167.5/20)
#define 	RANK3_RATIO			(167.5/20)
#define 	RANK4_RATIO			(21/20)
#define 	RANK5_RATIO			(21/20)
#define 	RANK6_RATIO			(41/20)


typedef struct {
	//INA226
	u32 WorkCurrent;			//0.1mA
	u32 SleepCurrent;			//0.1mA
	u32 BusVoltage;				//0.1V
	u32 Power;					// ?
	//ADC
	u32 Voltage[TOTAL_ADC_NUM];	// 0.01V

} Measurement_Para_t;


