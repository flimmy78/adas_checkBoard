
typedef enum TASK_ID {

	TASK_USB = 0x01,
	TASK_UART = 0x02,
	TASK_TEST = 0x03,
	TASK_ALL,
}task_id_enum;

	
typedef enum {

	MSG_UART_USB   = (TASK_UART<<8)|TASK_USB,
	MSG_USB_UART = (TASK_USB<<8)|TASK_UART,

	
}msg_type_enum;
	
	



// MSG header for msgs

typedef struct MSG_QUEUE
{
	u32 msg_addr;
}msg_queue_t;


typedef struct MSG_HEADER
{
	u8 *msg_data;			/* Pointer to msg body */
	u16 cmd;
	task_id_enum  snd_task_id;
	task_id_enum  rcv_task_id;
	
}msg_head_t;













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

#define		MSG_DEBUG_MCU_COMMAND 0x05

/**************************************************************************************/
/**************************************************************************************/


#define CAN_CAMERA_QUEUE_MAX_NUM	40


/**************************************************************************************/
/**************************************************************************************/
// Msg cmd between task


/**************************************************************************************/
#define 	FIRM_MODE		0x01
#define 	COMM_MODE		0x02


/**************************************************************************************/


typedef enum
{
	FILE_READ,
	FILE_WRITE,
	FILE_NONE_OP
}TCP_FILE_OP_enum;

/*   IT to UART app msg  */

typedef struct 
{
	u32  len;
	u8   *buff;

}IT_UART_msg_t;


/* uart -> hi3518 */
typedef struct 
{
	u32  len;
	u8   *buff;

}UART_hi3518_msg_t;

/* wifi -> uart */
typedef struct 
{
	u32  len;
	u8   *buff;

}hi3518_UART_msg_t;


typedef enum
{
	FILE_FLASH,
	FILE_ME,
	MAX_FILE_TYPE,
}FILE_TYPE_enum;






typedef struct 
{
	u32 filelen;
	u32 curpos;
	u32 file_type;
	u32 crc32;
	u8 	filename[MAX_FILENAME_LEN];
	u8 *buf;
	u16 total_packets;
	u16 cur_packet;
	u32 cur_packet_length;
	u16 file_flag;
	

}NET_FILE_PARA_t;




typedef struct
{
	u16   type;
	u16   length;
}TLV_HEADER_t;




typedef struct
{
	u16 type;
	u16 len;
	u16	total_packets;
	u16	cur_packet;
	u16 cur_data_length;
}TP_FILE_PACKET_t;


typedef struct
{
	u16  type;
	u16  len;
	u8 	filename[MAX_FILENAME_LEN];
	u32	filelen;
	u32	crc32;

}TLV_FILE_PARA_t;


typedef struct
{
	u16  type;
	u16  len;
	u16  curVideoSource;
	
}TLV_CMD_SWITCH_t;



