// Service type
#define 	SERVICE_TYPE_FILE			0x01
#define 	SERVICE_TYPE_WARN			0x02
#define 	SERVICE_TYPE_CMD			0x03


//Msg type

// SERVICE_TYPE_FILE
#define		MSG_TYPE_FILE_READ_REQ			0x01
#define		MSG_TYPE_FILE_READ_RESP			0x01

#define		MSG_TYPE_FILE_WRITE_REQ			0x02
#define		MSG_TYPE_FILE_WRITE_RESP		0x02

// SERVICE_TYPE_WARN
#define		MSG_TYPE_WARN_REQ				0x01


// SERVICE_TYPE_CMD
#define		MSG_TYPE_CMD_RESET_REQ			0x01
#define		MSG_TYPE_CMD_RESET_RESP			0x01

#define		MSG_TYPE_CMD_SWITCH_SCREEN_REQ	0x02
#define		MSG_TYPE_CMD_SWITCH_SCREEN_RESP	0x02

#define		MSG_TYPE_CMD_TEST_RESULT_REQ	0x03
#define		MSG_TYPE_CMD_TEST_RESULT_RESP	0x03

#define		MSG_TYPE_CMD_GET_MCU_ID_REQ		0x04
#define		MSG_TYPE_CMD_GET_MCU_ID_RESP	0x04

#define		MSG_TYPE_CMD_START_TEST_REQ		0x05
#define		MSG_TYPE_CMD_START_TEST_RESP	0x05





//Tlv type




typedef struct 				//16 bytes
{
	u32 start_bytes;
	u16 msg_len;
	u8  msg_count;
	u8  resp;
	u16 seq;
	u8  service_type;
	u8  msg_type;
	u32 crc32;
}comm_head_t;


typedef struct
{
	u16 Type;
	u16 Len;
	u8  *buf;
}tlv_type_t;

