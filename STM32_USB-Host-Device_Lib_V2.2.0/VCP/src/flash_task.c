/******************************************************************************
* @file    		 flash_task.c
* @author  		Gary
* @version
* @date
* @brief  		 flash driver
*
******************************************************************************/

#include "inc_all.h"

#include "spi_flash.h"
#include "stdlib.h"


/****************************************************************************/
/*External  Variables */
extern QueueHandle_t FLASH_Queue;
extern QueueHandle_t HI3518_Queue;
extern NET_FILE_PARA_t cur_file;
extern CAR_TIME_t  	new_car_time;
extern WarnOptionInfo warnOption;

extern u32 ErrCode[3];
extern void MsgSend2Camera(u8 data0);
/****************************************************************************/





/****************************************************************************/
/*External  Functions */
extern u32 SPI_FLASH_CheckCrc32(u32 start_addr, u32 length);
extern u32 drv_GetRtcTime(void);
extern u16 check_file_name(TLV_FILE_PARA_t *file_para);
extern u8  CheckErrCode(void);
extern u32 SPI_FLASH_GetCurLogPos(u32 ReadAddr, u32 size);
extern void ClearErrCode(void);
/****************************************************************************/



/****************************************************************************/
/*Global  Variables */
volatile u8 g_car_time_update_flag = 0;
volatile u8 g_car_date_update_flag = 0;
volatile bool sound_tune_flag = false;
/****************************************************************************/



/****************************************************************************/
/*Global  Functions */
void write_nvram(void);
bool read_nvram(void);

/****************************************************************************/


/****************************************************************************/
/*Local  Variables*/
static NET_FILE_PARA_t flash_file;


/****************************************************************************/


/****************************************************************************/
/*Local  Functions*/
static void flash_check_data(msg_head_t *msg_head);
static bool flash_check_file_crc32(u32 start_addr, u32 filelen, u32 crc32);
static bool flash_update_firmware_info(void);
static void flash_update_firmware_reply(bool success);
static void flash_send_write_first_resp_msg(void);
static void flash_send_write_resp_msg(void);
static void flash_backup_resource(NET_FILE_PARA_t *flash_file);
static void UpgradeReset(void);

/****************************************************************************/






/****************************************************************************/


void flash_task_init(void)
{
	FLASH_Queue = xQueueCreate(20, sizeof(msg_queue_t));
}



void flash_task(void)
{
	msg_queue_t msg_queue;
	msg_head_t *msg_head;
	u32 msg_type;

	read_nvram();

	while (1) {
		if (xQueueReceive(FLASH_Queue, &msg_queue, 100) == pdTRUE) {

			msg_head = (msg_head_t *)msg_queue.msg_addr;
			msg_type = (msg_head->snd_task_id << 8) | msg_head->rcv_task_id;
			//cmd = msg_head->cmd;
			switch (msg_type) {
			case MSG_WIFI_FLASH:
				flash_check_data(msg_head);
				break;
			case MSG_IT_FLASH:
				UpdateTime();
				break;

			default:
				break;
			}
			vPortFree(msg_head);
		}
	}



}


/****************************************************************************************
* Function:		flash_send_write_first_resp_msg
* Parameters:
* Returns:
* Description:
* Created:
* Date:			2015-12-17
****************************************************************************************/

static void flash_send_write_first_resp_msg(void)
{
	msg_queue_t msg_queue;
	hi3518_UART_msg_t *msg_text;
	msg_head_t *msg_head;
	TLV_FILE_PARA_t  *file_para;
	TP_FILE_PACKET_t *file_packet;
	u32 malloc_size, crc32;
	void *msg_addr;
	uart_head_t *uart_head;


	/*1. Malloc memory */
	malloc_size = CALC_MALLOC_SIZE(sizeof(hi3518_UART_msg_t) + sizeof(
	                                   msg_head_t) + UART_HDR_LENGTH + TLV_FILE_PARA_LENGTH + TP_FILE_PACKET_LENGTH);
	msg_addr = pvPortMalloc(malloc_size);
	if (msg_addr == NULL) {
		return;
	}


	/*2. Fill header */
	msg_head = (msg_head_t *)msg_addr;
	msg_head->snd_task_id = TASK_FLASH;
	msg_head->rcv_task_id = TASK_WIFI;
	msg_head->cmd = NULL;
	msg_head->msg_data = (u8 *)((u32)msg_addr + sizeof(msg_head_t));


	msg_text = (hi3518_UART_msg_t *)msg_head->msg_data;
	msg_text->buff = (u8 *)((u32)msg_text + sizeof(hi3518_UART_msg_t));
	msg_text->len = UART_HDR_LENGTH +  TLV_FILE_PARA_LENGTH + TP_FILE_PACKET_LENGTH
	                ;

	uart_head				= (uart_head_t *)msg_text->buff;
	uart_head->start_bytes 	= UART_START_BYTES;
	uart_head->service_type = SERVICE_FILE;

	uart_head->msg_type 	= MSG_FILE_WRITE_FIRST_RESP;
	uart_head->reserve		= NULL;
	uart_head->msg_len 		= msg_text->len;


	/* Add file para tlv */
	file_para = (TLV_FILE_PARA_t *)&msg_text->buff[UART_HDR_LENGTH];

	file_para->type		= TP_FILE_PARA_ID;
	file_para->len		= TLV_FILE_PARA_LENGTH;
	memcpy((u8 *)&file_para->filename, cur_file.filename, MAX_FILENAME_LEN);
	file_para->filelen = cur_file.filelen;
	file_para->crc32 	= cur_file.crc32;


	/* Add file packet tlv */
	file_packet = (TP_FILE_PACKET_t *)&msg_text->buff[UART_HDR_LENGTH +
	                              TLV_FILE_PARA_LENGTH];

	file_packet->type			= TP_FILE_PACKET_ID;
	file_packet->len			= TP_FILE_PACKET_LENGTH;
	file_packet->total_packets	= flash_file.total_packets;
	file_packet->cur_packet		= flash_file.cur_packet;
	file_packet->cur_data_length = flash_file.cur_packet_length;

	crc32 = GetCrc32((u8 *)((u32)uart_head + CRC_REMOVE_LENGTH),
	                 uart_head->msg_len - CRC_REMOVE_LENGTH);
	uart_head->crc32 = crc32;

	/* Here we send msg  */
	msg_queue.msg_addr = (u32)msg_addr;
	if (xQueueSend(HI3518_Queue, (void *)&msg_queue, 0) != pdTRUE) {
		vPortFree(msg_addr);
		;
	}


}

/****************************************************************************************
* Function:		flash_send_write_resp_msg
* Parameters:
* Returns:
* Description:
* Created:
* Date:			2015-12-17
****************************************************************************************/

static void flash_send_write_resp_msg(void)
{
	msg_queue_t msg_queue;
	hi3518_UART_msg_t *msg_text;
	msg_head_t *msg_head;
	//TLV_FILE_PARA_t  *file_para;
	TP_FILE_PACKET_t *file_packet;
	u32 malloc_size, crc32;
	void *msg_addr;
	uart_head_t *uart_head;


	/*1. Malloc memory */
	//malloc_size = CALC_MALLOC_SIZE(sizeof(hi3518_UART_msg_t)+sizeof(msg_head_t)+ UART_HDR_LENGTH +TLV_FILE_PARA_LENGTH+TP_FILE_PACKET_LENGTH);
	malloc_size = CALC_MALLOC_SIZE(sizeof(hi3518_UART_msg_t) + sizeof(
	                                   msg_head_t) + UART_HDR_LENGTH);

	msg_addr = pvPortMalloc(malloc_size);

	if (msg_addr == NULL) {
		return;
	}


	/*2. Fill header */
	msg_head = (msg_head_t *)msg_addr;
	msg_head->snd_task_id = TASK_FLASH;
	msg_head->rcv_task_id = TASK_WIFI;
	msg_head->cmd = NULL;
	msg_head->msg_data = (u8 *)((u32)msg_addr + sizeof(msg_head_t));


	msg_text = (hi3518_UART_msg_t *)msg_head->msg_data;
	msg_text->buff = (u8 *)((u32)msg_text + sizeof(hi3518_UART_msg_t));

	//msg_text->len = UART_HDR_LENGTH +  TLV_FILE_PARA_LENGTH+TP_FILE_PACKET_LENGTH ;
	msg_text->len = UART_HDR_LENGTH + TP_FILE_PACKET_LENGTH;

	uart_head				= (uart_head_t *)msg_text->buff;
	uart_head->start_bytes 	= UART_START_BYTES;
	uart_head->service_type = SERVICE_FILE;

	uart_head->msg_type 	= MSG_FILE_WRITE_RESP;
	uart_head->reserve		= NULL;
	uart_head->msg_len 		= msg_text->len;


	/* Add file para tlv */
	//	file_para = (TLV_FILE_PARA_t *)&msg_text->buff[UART_HDR_LENGTH];

	//	file_para->type		= TP_FILE_PARA_ID;
	//	file_para->len		= TLV_FILE_PARA_LENGTH;
	//	memcpy((u8 *)&file_para->filename,cur_file.filename,MAX_FILENAME_LEN);
	//	file_para->file_len = cur_file.filelen;
	//	file_para->crc32 	= cur_file.crc32;


	/* Add file packet tlv */
	file_packet = (TP_FILE_PACKET_t *)&msg_text->buff[UART_HDR_LENGTH];

	file_packet->type			= TP_FILE_PACKET_ID;
	file_packet->len			= TP_FILE_PACKET_LENGTH;
	file_packet->total_packets	= flash_file.total_packets;
	file_packet->cur_packet		= flash_file.cur_packet;
	file_packet->cur_data_length = flash_file.cur_packet_length;

	crc32 = GetCrc32((u8 *)((u32)uart_head + CRC_REMOVE_LENGTH),
	                 uart_head->msg_len - CRC_REMOVE_LENGTH);
	uart_head->crc32 = crc32;

	/* Here we send msg  */
	msg_queue.msg_addr = (u32)msg_addr;
	if (xQueueSend(HI3518_Queue, (void *)&msg_queue, 0) != pdTRUE) {
		vPortFree(msg_addr);
		;
	}





}

/****************************************************************************************
* Function:		flash_write_firmware
* Parameters:
* Returns:
* Description: 	 flash_write_firmware
* Created:
* Date:			2015-12-17
****************************************************************************************/

static void flash_write_firmware(msg_head_t *msg_head)
{
	UART_hi3518_msg_t 	*msg_text;
	TLV_FILE_PARA_t 	*file_para;
	uart_head_t			*net_head;
	TP_FILE_PACKET_t 	*file_packet;
	u8 					*pbuff;
	static u32 			flash_start_addr;
	u32 				crc32;
	u8 volume = 0;
	int i = 0;

	msg_text = (UART_hi3518_msg_t *)msg_head->msg_data;
	net_head = (uart_head_t *)msg_text->buff;
	if (net_head->msg_type == MSG_FILE_WRITE_FIRST_REQ) {
		file_para = (TLV_FILE_PARA_t *)((u32)net_head + UART_HDR_LENGTH);
		file_packet	= (TP_FILE_PACKET_t *)((u32)net_head + UART_HDR_LENGTH +
		                                   TLV_FILE_PARA_LENGTH);
		if ((file_para->type == TP_FILE_PARA_ID)
		    && (file_packet->type == TP_FILE_PACKET_ID)) {
			flash_file.curpos			= 0;
			flash_file.filelen 			= file_para->filelen;
			flash_file.total_packets	= file_packet->total_packets;
			flash_file.cur_packet		= file_packet->cur_packet;
			flash_file.cur_packet_length = file_packet->cur_data_length;
			flash_file.crc32 			= file_para->crc32;
			flash_file.file_flag 		= check_file_name(file_para);
			memcpy((u8 *)&flash_file.filename, (u8 *)&file_para->filename,
			       MAX_FILENAME_LEN);
		}
		//Trace("\r\nfirst packet total len ",flash_file.filelen);
		switch (flash_file.file_flag) {
		case MCU_SF_FILE_NUM:
			flash_start_addr =	MCU_FW_ADDR;
			break;
		case FPGA_SF_FILE_NUM:
			flash_start_addr = FPGA_FW_ADDR;
			break;
		case PICTURE_FILE_NUM:
			flash_start_addr = PICTURE_ADDR;
			break;
		case CAR_INFO_FILE_NUM:
			flash_start_addr = CAR_INFO_ADDR;
			break;
		case WARN_OPTION_FILE_NUM:
			flash_start_addr = WARN_OPTION_ADDR;
			break;
		default:
			break;
		}


		pbuff = (u8 *)((u32)net_head + TLV_FILE_PARA_LENGTH + UART_HDR_LENGTH +
		               TP_FILE_PACKET_LENGTH);

		if (flash_file.total_packets == 1) {

			crc32 = GetCrc32(pbuff, file_packet->cur_data_length - 4);
			memcpy(&pbuff[file_packet->cur_data_length - 4], &crc32, 4);

			//Update warn
			if (flash_file.file_flag == WARN_OPTION_FILE_NUM) {
				volume = warnOption.Volume;
				memcpy(&warnOption, pbuff, file_packet->cur_data_length);
				if (volume != warnOption.Volume && warnOption.Volume > 0) {
					sound_tune_flag = true;
					for (i = 0; i < 8; i++) {
						MsgSend2Camera((warnOption.Volume << 5) | (SOUND_FCW_HARD & 0x1F));
						vTaskDelayWdg(50, TASK_FLASH);
					}
					sound_tune_flag = false;
				}
				TraceStr("warn option updated\r\n");
				Trace("warn volume", warnOption.Volume);
			}


			//Clear file status
			flash_file.crc32 = 0;
			flash_file.filelen = 0;
			flash_file.curpos = 0;
			flash_file.file_flag = 0;
			cur_file.crc32 = 0;
			cur_file.file_type = MAX_FILE_TYPE;
			cur_file.curpos = 0;
			cur_file.filelen = 0;
			TraceStr("Small file updated");


		}

		NEW_SPI_FLASH_BufferWrite(pbuff, flash_file.curpos + flash_start_addr,
		                          file_packet->cur_data_length);
		flash_file.curpos += file_packet->cur_data_length;
		flash_send_write_first_resp_msg();


	} else if (net_head->msg_type == MSG_FILE_WRITE_REQ) {

		file_packet	= (TP_FILE_PACKET_t *)((u32)net_head + UART_HDR_LENGTH);
		if ((flash_file.file_flag != NULL)
		    && (file_packet->type == TP_FILE_PACKET_ID)) {
			if (file_packet->cur_packet == flash_file.cur_packet + 1) {
				flash_file.cur_packet++;
				flash_file.cur_packet_length = file_packet->cur_data_length;

				pbuff = (u8 *)((u32)net_head + UART_HDR_LENGTH + TP_FILE_PACKET_LENGTH);

				//SPI_FLASH_BufferWrite(pbuff,flash_file.curpos+NEW_FW_ADDR,msg_text->len);
				NEW_SPI_FLASH_BufferWrite(pbuff, flash_file.curpos + flash_start_addr,
				                          file_packet->cur_data_length);
				//	check_BufferWrite(pbuff,flash_file.curpos+flash_start_addr, file_packet->cur_data_length);
				flash_file.curpos += file_packet->cur_data_length;
				flash_send_write_resp_msg();

			}

		}
	}

	/* All data transfer finished */
	if (file_packet->cur_packet == flash_file.total_packets) {
		if (flash_file.curpos == flash_file.filelen) {	// File is transfer finished
			if (flash_file.file_flag	 == MCU_SF_FILE_NUM
			    || flash_file.file_flag	 == FPGA_SF_FILE_NUM) {
				//update flag
				if (flash_update_firmware_info()) {
					flash_update_firmware_reply(true);
				} else {
					flash_update_firmware_reply(false);
				}
				TraceStr("mcu or fpga updated\r\n");
				//	UpgradeReset();
			} else if (flash_file.file_flag == PICTURE_FILE_NUM) {
				cur_file.filelen = 0;
				if (flash_check_file_crc32(PICTURE_ADDR, flash_file.filelen,
				                           flash_file.crc32)) {
					Trace("rcv crc32", flash_file.crc32);
					TraceStr("update picture crc32 ok \r\n");
					flash_backup_resource(&flash_file);
					//	UpgradeReset();

				} else {
					TraceStr("update picture crc32 error \r\n");
				}
			}

			/* clear all variable about flash */
			flash_file.crc32 = 0;
			flash_file.filelen = 0;
			flash_file.curpos = 0;
			flash_file.file_flag = 0;
			cur_file.crc32 = 0;
			cur_file.file_type = MAX_FILE_TYPE;
			cur_file.curpos = 0;
			cur_file.filelen = 0;

		}
	}
}


static void flash_backup_resource(NET_FILE_PARA_t *flash_file)
{
	u8 *buf;
	u32 SrcAddr, DstAddr;
	u32 i;
	u32 BufSize;

	SrcAddr = PICTURE_ADDR;
	DstAddr = PICTURE_BACKUP_ADDR;

	BufSize = 1024 * 4;
	buf = (u8 *)pvPortMalloc(BufSize);
	if (buf == NULL) {
		return;
	}

	for (i = 0; i <	flash_file->filelen; i += (BufSize)) {
		clear_watchdog_flag(TASK_FLASH);
		SPI_FLASH_SectorErase(DstAddr);
		SPI_FLASH_BufferRead(buf, SrcAddr, BufSize);
		clear_watchdog_flag(TASK_FLASH);
		SPI_FLASH_BufferWrite(buf, DstAddr, BufSize);
		SrcAddr += BufSize;
		DstAddr += BufSize;
	}

	TraceStr("backup pic ok \r\n");
	vPortFree(buf);


}


static void flash_send_read_first_resp_msg(TLV_FILE_PARA_t *read_file_para)
{
	msg_head_t *msg_head;
	msg_queue_t msg_queue;
	hi3518_UART_msg_t *msg_text;
	TLV_FILE_PARA_t *file_para;
	u32 malloc_size, filelen, crc32, data_start;
	void *msg_addr = NULL;
	uart_head_t *uart_head;
	TP_FILE_PACKET_t *file_packet;
	u16 file_num;
	u32 read_start;
	u32 read_len;


	file_num = check_file_name(read_file_para);

	if (file_num == CAR_INFO_FILE_NUM) {
		read_start = CAR_INFO_ADDR;
		read_len  = CAR_INFO_LENGTH;
	} else if (file_num == WARN_OPTION_FILE_NUM) {
		read_start = WARN_OPTION_ADDR;
		read_len  = WARN_OPTION_LENGTH;
	} else {	//
		return;
	}



	/*1. Malloc memory */
	malloc_size = CALC_MALLOC_SIZE(sizeof(hi3518_UART_msg_t) + sizeof(
	                                   msg_head_t) + UART_HDR_LENGTH + TLV_FILE_PARA_LENGTH + TP_FILE_PACKET_LENGTH +
	                               read_len);
	msg_addr = pvPortMalloc(malloc_size);

	if (msg_addr == NULL) {
		TraceStr("Malloc msgaddr error!\r\n");
		return;
	}

	/*2. Fill header */
	msg_head = (msg_head_t *)msg_addr;
	msg_head->snd_task_id = TASK_FLASH;
	msg_head->rcv_task_id = TASK_WIFI;
	msg_head->cmd = NULL;
	msg_head->msg_data = (u8 *)((u32)msg_addr + sizeof(msg_head_t));


	{
		msg_text = (hi3518_UART_msg_t *)msg_head->msg_data;
		filelen =  read_len;
		msg_text->buff = (u8 *)((u32)msg_text + sizeof(hi3518_UART_msg_t));
		msg_text->len = UART_HDR_LENGTH + TLV_FILE_PARA_LENGTH + TP_FILE_PACKET_LENGTH +
		                filelen;

		uart_head				= (uart_head_t *)msg_text->buff;
		uart_head->start_bytes 	= UART_START_BYTES;
		uart_head->service_type = SERVICE_FILE;
		uart_head->msg_type 	= MSG_FILE_READ_FIRST_RESP;
		uart_head->reserve		= NULL;
		uart_head->msg_len 		= msg_text->len;


		/* Add file para tlv */
		file_para = (TLV_FILE_PARA_t *)&msg_text->buff[UART_HDR_LENGTH];
		file_para->type	=	TP_FILE_PARA_ID;
		file_para->len	=	TLV_FILE_PARA_LENGTH;
		memcpy((u8 *)&file_para->filename, cur_file.filename, MAX_FILENAME_LEN);
		file_para->filelen = 	filelen;

		file_para->crc32	=	NULL;

		/* Add file packet*/
		file_packet						=	(TP_FILE_PACKET_t *)&msg_text->buff[UART_HDR_LENGTH +
		                                                    TLV_FILE_PARA_LENGTH];
		{
			file_packet->type				=	TP_FILE_PACKET_ID;
			file_packet->len				=	TP_FILE_PACKET_LENGTH;
			file_packet->total_packets		=	1;
			file_packet->cur_packet			=	1;
			file_packet->cur_data_length	=	filelen;
		}


		/* Copy file data */
		data_start = UART_HDR_LENGTH + TLV_FILE_PARA_LENGTH + TP_FILE_PACKET_LENGTH;

		SPI_FLASH_BufferRead(&msg_text->buff[data_start], read_start, read_len);

		file_para->crc32 = GetCrc32((u8 *)(&msg_text->buff[data_start]), filelen);

		crc32			 = GetCrc32((u8 *)((u32)uart_head + CRC_REMOVE_LENGTH),
		                            uart_head->msg_len - CRC_REMOVE_LENGTH);
		uart_head->crc32 = crc32;

		TraceStr("read flash fs ok \r\n");

	}


	/* Here we send msg  */
	msg_queue.msg_addr = (u32)msg_addr;
	if (xQueueSend(HI3518_Queue, (void *)&msg_queue, 0) != pdTRUE) {
		vPortFree(msg_addr);
		;
	}



}



static void flash_read_warn(msg_head_t *msg_head)
{



	UART_hi3518_msg_t 	*msg_text;
	uart_head_t 		*uart_head;
	TLV_FILE_PARA_t 	*file_para;

	msg_text = (UART_hi3518_msg_t *)msg_head->msg_data;
	uart_head = (uart_head_t *)msg_text->buff;
	file_para = (TLV_FILE_PARA_t *)((u32)uart_head + UART_HDR_LENGTH);


	//Now just one packet
	flash_send_read_first_resp_msg(file_para);

}


static void flash_check_data(msg_head_t *msg_head)
{

	//	msg_queue_t 		msg_queue;
	UART_hi3518_msg_t 	*msg_text;
	uart_head_t 		*uart_head;

	msg_text = (UART_hi3518_msg_t *)msg_head->msg_data;
	uart_head = (uart_head_t *)msg_text->buff;

	if (uart_head->msg_type == MSG_FILE_READ_REQ) {
		flash_read_warn(msg_head);
	} else if ((uart_head->msg_type == MSG_FILE_WRITE_FIRST_REQ)
	           || (uart_head->msg_type == MSG_FILE_WRITE_REQ)) {
		flash_write_firmware(msg_head);
	}



}

static bool flash_update_firmware_info(void)
{
	firm_para_t *fw_para;
	firm_info_t *firm_info;
	bool ret;

	fw_para = (firm_para_t *)pvPortMalloc(sizeof(firm_para_t));

	if (fw_para == NULL) {
		return false;
	}

	/* Read  data */
	SPI_FLASH_BufferRead((void *)fw_para, PARA_REGION_START, sizeof(firm_para_t));


	switch (flash_file.file_flag) {
	case MCU_SF_FILE_NUM:
		firm_info = &fw_para->mcu_firm;
		firm_info->firm_start = MCU_FW_ADDR;

		//Clear nvram, if bootloader update failed */
		nvram_data.mcu_check_err_count = 0;
		nvram_data.mcu_code_need_check = 0;
		break;
	case FPGA_SF_FILE_NUM:
		firm_info = &fw_para->fpga_firm;
		firm_info->firm_start = FPGA_FW_ADDR;
		//Clear nvram, if bootloader update failed */
		nvram_data.fpga_check_err_count = 0;
		nvram_data.fpga_code_need_check = 0;

		break;
	case PICTURE_FILE_NUM:
		firm_info = &fw_para->pic_firm;
		firm_info->firm_start = PICTURE_ADDR;
		break;
	default:
		TraceStr("File type error\r\n");
		ret = false;
		goto FreeMemOut;
		//		break;
	}

	/* Here firm_info is assigned before */
	firm_info->firm_crc32 = flash_file.crc32;
	firm_info->firm_length = flash_file.filelen;

	firm_info->firm_update_flag = FW_UPDATA_FLAG;
	firm_info->firm_update_flag_xor = 0xFFFFFFFF ^ firm_info->firm_update_flag;
	Trace("firm_start", firm_info->firm_start);
	Trace("firm_length", firm_info->firm_length);
	Trace("firm_crc32", firm_info->firm_crc32);
	ret = false;
	Trace("mcu update flag", fw_para->mcu_firm.firm_update_flag);
	Trace("fpga update flag", fw_para->fpga_firm.firm_update_flag);
	if (flash_check_file_crc32(firm_info->firm_start, firm_info->firm_length,
	                           firm_info->firm_crc32)) {
		/*Update crc32 */
		firm_info->crc32 = GetCrc32((u8 *)firm_info, sizeof(firm_info_t) - 4);

		SPI_FLASH_SectorErase(PARA_REGION_START);
		SPI_FLASH_BufferWrite((u8 *)fw_para, PARA_REGION_START, sizeof(firm_para_t));
		ret = true;
		write_nvram();
		TraceStr("update firmware info ok \r\n");
		goto FreeMemOut;
	} else {
		TraceStr("File Crc32 check err, update failed! \r\n");
	}


FreeMemOut:
	vPortFree(fw_para);
	return ret;
}

static bool flash_check_file_crc32(u32 start_addr, u32 filelen, u32 crc32)
{
	u32 a1;
	a1 = SPI_FLASH_CheckCrc32(start_addr, filelen);
	if (a1 != crc32) {
		Trace("firmware crc32 error", a1);
		return false;
	}
	return true;

}

static void flash_update_firmware_reply(bool success)
{
	msg_queue_t msg_queue;
	hi3518_UART_msg_t *msg_text;
	msg_head_t *msg_head;

	TLV_FILE_PARA_t *file_para;
	u32 malloc_size, crc32;
	void *msg_addr;
	uart_head_t *net_head;


	/*1. Malloc memory */
	malloc_size = CALC_MALLOC_SIZE(sizeof(hi3518_UART_msg_t) + sizeof(
	                                   msg_head_t) + UART_HDR_LENGTH + TLV_FILE_PARA_LENGTH);
	msg_addr = pvPortMalloc(malloc_size);
	if (msg_addr == NULL) {
		return;
	}


	/*2. Fill header */
	msg_head = (msg_head_t *)msg_addr;
	msg_head->snd_task_id = TASK_FLASH;
	msg_head->rcv_task_id = TASK_WIFI;
	msg_head->cmd = NULL;
	msg_head->msg_data = (u8 *)((u32)msg_addr + sizeof(msg_head_t));


	msg_text = (hi3518_UART_msg_t *)msg_head->msg_data;
	msg_text->buff = (u8 *)((u32)msg_text + sizeof(hi3518_UART_msg_t));
	msg_text->len = UART_HDR_LENGTH +  TLV_FILE_PARA_LENGTH ;

	net_head = (uart_head_t *)msg_text->buff;
	net_head				= (uart_head_t *)msg_text->buff;
	net_head->start_bytes 	= UART_START_BYTES;
	net_head->service_type 	= SERVICE_FILE;

	net_head->msg_type 		= MSG_FILE_WRITE_RESP;
	net_head->reserve		= NULL;
	net_head->msg_len 		= msg_text->len;



	/* Add file para tlv */
	file_para = (TLV_FILE_PARA_t *)&msg_text->buff[UART_HDR_LENGTH];

	memcpy((u8 *)&file_para->filename, flash_file.filename, MAX_FILENAME_LEN);
	file_para->filelen = flash_file.filelen;
	file_para->crc32 = flash_file.crc32;

	if (success == false) { /* Write error */
		file_para->filelen = 0;
		file_para->crc32 = 0;
	}

	crc32 = GetCrc32((u8 *)((u32)net_head + CRC_REMOVE_LENGTH),
	                 net_head->msg_len - CRC_REMOVE_LENGTH);
	net_head->crc32 = crc32;

	/* Here we send msg  */
	msg_queue.msg_addr = (u32)msg_addr;
	if (xQueueSend(HI3518_Queue, (void *)&msg_queue, 0) != pdTRUE) {
		vPortFree(msg_addr);
		;
	}


}



static void UpgradeReset(void)
{

	vTaskDelayWdg(100, TASK_FLASH);
	NVIC_SystemReset();
	/* If error, wait for watchdog reset*/
	while (1);

}


