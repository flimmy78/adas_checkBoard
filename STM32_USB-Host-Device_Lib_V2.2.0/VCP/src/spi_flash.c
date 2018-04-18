/******************** (C) COPYRIGHT 2007 STMicroelectronics ********************
* File Name          : spi_flash.c
* Author             : MCD Application Team
* Date First Issued  : 02/05/2007
* Description        : This file provides a set of functions needed to manage the
*                      communication between SPI peripheral and SPI M25P64 FLASH.
********************************************************************************
* History:
* 04/02/2007: V0.2
* 02/05/2007: V0.1
********************************************************************************
* THE PRESENT SOFTWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/

#include "inc_all.h"


#define USE_RTOS_DELAY
#ifdef USE_RTOS_DELAY


#define 	PAGE_WAIT_TIME			2
#define 	SECTOR_WAIT_TIME		60		//unit:ms
#define		BLOCK_WAIT_TIME			500
#define 	CHIP_WAIT_TIME			50000
#endif



/* Private typedef -----------------------------------------------------------*/
#define WRITE      0x02  /* Write to Memory instruction */
#define WRSR       0x01  /* Write Status Register instruction */ 
#define WREN       0x06  /* Write enable instruction */

#define READ       0x03  /* Read from Memory instruction */
#define RDSR       0x05  /* Read Status Register instruction  */
#define RDID       0x9F  /* Read identification */


#define	SE		   0x20
#define BE         0xD8  /* Block Erase instruction */
#define CHIP_ERASE 0xC7  /* Bulk Erase instruction */

#define WIP_Flag   0x01  /* Write In Progress (WIP) flag */

#define Dummy_Byte 0xA5


extern unsigned int GetCrc32_One(unsigned char  dat,unsigned int a1,unsigned int pos);

extern void clear_watchdog_flag(u8 task_id);

/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/*******************************************************************************
* Function Name  : SPI_FLASH_Init
* Description    : Initializes the peripherals used by the SPI FLASH driver.APB1 42MHz
* Input          : None
* Output         : None
* Return         : None  
*******************************************************************************/


void SPI_FLASH_Init(void)
{

  SPI_InitTypeDef  SPI_InitStructure;
  GPIO_InitTypeDef GPIO_InitStructure;

   
  /* Enable SPI1 and GPIOA clocks */
  RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOA, ENABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1,ENABLE);

  
  SPI_I2S_DeInit(SPI1);


  
  /* Configure SPI1 pins: NSS, SCK, MISO and MOSI */
  GPIO_InitStructure.GPIO_Pin = SPI1_MISO_PIN| SPI1_MOSI_PIN | SPI1_SCK_PIN;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;

  GPIO_Init(SPI1_PORT, &GPIO_InitStructure);


  /* Configure PA.4 as Output push-pull, used as Flash Chip select */
  GPIO_InitStructure.GPIO_Pin = SPI1_CS_PIN;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;

  GPIO_Init(SPI1_PORT, &GPIO_InitStructure);

  /* Deselect the FLASH: Chip Select high */
  SPI_FLASH_ChipSelect(High);

  /* SPI1 configuration */ 
  SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
  SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
  SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;

  #if 1
  SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;
  SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
  #else

  SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
  SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;

  #endif


  SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
  SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4;	//APB1 36MHz, Ok


  SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
  SPI_InitStructure.SPI_CRCPolynomial = 7;
  SPI_Init(SPI1, &SPI_InitStructure);


  /* Enable SPI1  */
  SPI_Cmd(SPI1, ENABLE);   

}


void SPI_FLASH_Init_GPIO(void)
{

//  SPI_InitTypeDef  SPI_InitStructure;
  GPIO_InitTypeDef GPIO_InitStructure;

  /* Configure SPI1 pins: NSS, SCK, MISO and MOSI */

  GPIO_InitStructure.GPIO_Pin = SPI1_MISO_PIN| SPI1_MOSI_PIN | SPI1_SCK_PIN;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;

  GPIO_Init(SPI1_PORT, &GPIO_InitStructure);


}




/*******************************************************************************
* Function Name  : SPI_FLASH_SectorErase
* Description    : Erases the specified FLASH sector.
* Input          : SectorAddr: address of the sector to erase.
* Output         : None
* Return         : None
*******************************************************************************/
void SPI_FLASH_SectorErase(u32 SectorAddr)
{
  SPI_FLASH_Init_GPIO();
  /* Send write enable instruction */
  SPI_FLASH_WriteEnable();
			
  /* Sector Erase */ 
  /* Select the FLASH: Chip Select low */
  SPI_FLASH_ChipSelect(Low);		
  /* Send Sector Erase instruction  */
  SPI_FLASH_SendByte(SE);
  /* Send SectorAddr high nibble address byte */
  SPI_FLASH_SendByte((SectorAddr & 0xFF0000) >> 16);
  /* Send SectorAddr medium nibble address byte */
  SPI_FLASH_SendByte((SectorAddr & 0xFF00) >> 8);
  /* Send SectorAddr low nibble address byte */
  SPI_FLASH_SendByte(SectorAddr & 0xFF);
  /* Deselect the FLASH: Chip Select high */
  SPI_FLASH_ChipSelect(High);	

  #ifdef USE_RTOS_DELAY

  vTaskDelay(SECTOR_WAIT_TIME);
  
  #endif


  /* Wait the end of Flash writing */
  SPI_FLASH_WaitForWriteEnd();	
}



/*******************************************************************************
* Function Name  : SPI_FLASH_BlockErase
* Description    : Erases the specified FLASH sector.
* Input          : SectorAddr: address of the block to erase.
* Output         : None
* Return         : None
*******************************************************************************/
void SPI_FLASH_BlockErase(u32 SectorAddr)
{
  SPI_FLASH_Init_GPIO();
  /* Send write enable instruction */
  SPI_FLASH_WriteEnable();
			
  /* Sector Erase */ 
  /* Select the FLASH: Chip Select low */
  SPI_FLASH_ChipSelect(Low);		
  /* Send Sector Erase instruction  */
  SPI_FLASH_SendByte(BE);
  /* Send SectorAddr high nibble address byte */
  SPI_FLASH_SendByte((SectorAddr & 0xFF0000) >> 16);
  /* Send SectorAddr medium nibble address byte */
  SPI_FLASH_SendByte((SectorAddr & 0xFF00) >> 8);
  /* Send SectorAddr low nibble address byte */
  SPI_FLASH_SendByte(SectorAddr & 0xFF);
  /* Deselect the FLASH: Chip Select high */
  SPI_FLASH_ChipSelect(High);	

  #ifdef USE_RTOS_DELAY

  	vTaskDelay(BLOCK_WAIT_TIME);
  	//should clear watchdog here
  #endif

  

  /* Wait the end of Flash writing */
  SPI_FLASH_WaitForWriteEnd();	
}





/*******************************************************************************
* Function Name  : SPI_FLASH_ChipErase
* Description    : Erases the entire FLASH.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void SPI_FLASH_ChipErase(void)
{ 	
	 SPI_FLASH_Init_GPIO();
  /* Send write enable instruction */
  SPI_FLASH_WriteEnable();	
	
  /* Bulk Erase */ 
  /* Select the FLASH: Chip Select low */
  SPI_FLASH_ChipSelect(Low);		
  /* Send Bulk Erase instruction  */
  SPI_FLASH_SendByte(CHIP_ERASE);
  /* Deselect the FLASH: Chip Select high */
  SPI_FLASH_ChipSelect(High);	

#ifdef USE_RTOS_DELAY

  vTaskDelay(CHIP_WAIT_TIME);
  
#endif

		
  /* Wait the end of Flash writing */
  SPI_FLASH_WaitForWriteEnd();	

}

/*******************************************************************************
* Function Name  : SPI_FLASH_PageWrite
* Description    : Writes more than one byte to the FLASH with a single WRITE
*                  cycle(Page WRITE sequence). The number of byte can't exceed
*                  the FLASH page size.
* Input          : - pBuffer : pointer to the buffer  containing the data to be 
*                    written to the FLASH.
*                  - WriteAddr : FLASH's internal address to write to.
*                  - NumByteToWrite : number of bytes to write to the FLASH,
*                    must be equal or less than "SPI_FLASH_PageSize" value. 
* Output         : None
* Return         : None
*******************************************************************************/
void SPI_FLASH_PageWrite(u8* pBuffer, u32 WriteAddr, u16 NumByteToWrite)
{
	 SPI_FLASH_Init_GPIO();
  /* Enable the write access to the FLASH */
  SPI_FLASH_WriteEnable();		
  
  /* Select the FLASH: Chip Select low */
  SPI_FLASH_ChipSelect(Low);	   
  /* Send "Write to Memory " instruction */    
  SPI_FLASH_SendByte(WRITE);	
  /* Send WriteAddr high nibble address byte to write to */
  SPI_FLASH_SendByte((WriteAddr & 0xFF0000) >> 16);
  /* Send WriteAddr medium nibble address byte to write to */
  SPI_FLASH_SendByte((WriteAddr & 0xFF00) >> 8);  
  /* Send WriteAddr low nibble address byte to write to */
  SPI_FLASH_SendByte(WriteAddr & 0xFF);             
  
  /* while there is data to be written on the FLASH */
  while(NumByteToWrite--) 
  {
    /* Send the current byte */			
    SPI_FLASH_SendByte(*pBuffer); 	               
    /* Point on the next byte to be written */
    pBuffer++; 
  }		
  
  /* Deselect the FLASH: Chip Select high */
  SPI_FLASH_ChipSelect(High);

#ifdef USE_RTOS_DELAY
  
	vTaskDelay(PAGE_WAIT_TIME);
	
#endif



  
  /* Wait the end of Flash writing */
  SPI_FLASH_WaitForWriteEnd();	

}

/*******************************************************************************
* Function Name  : SPI_FLASH_BufferWrite
* Description    : Writes block of data to the FLASH. In this function, the
*                  number of WRITE cycles are reduced, using Page WRITE sequence.
* Input          : - pBuffer : pointer to the buffer  containing the data to be 
*                    written to the FLASH.
*                  - WriteAddr : FLASH's internal address to write to.
*                  - NumByteToWrite : number of bytes to write to the FLASH.
* Output         : None
* Return         : None
*******************************************************************************/
void SPI_FLASH_BufferWrite(u8* pBuffer, u32 WriteAddr, u16 NumByteToWrite)
{
  u8 NumOfPage = 0, NumOfSingle = 0, Addr = 0, count = 0, temp = 0;

  Addr = WriteAddr % SPI_FLASH_PageSize;
  count = SPI_FLASH_PageSize - Addr;
  NumOfPage =  NumByteToWrite / SPI_FLASH_PageSize;
  NumOfSingle = NumByteToWrite % SPI_FLASH_PageSize;
	
  if(Addr == 0) /* WriteAddr is SPI_FLASH_PageSize aligned  */
  {
    if(NumOfPage == 0) /* NumByteToWrite < SPI_FLASH_PageSize */
    {
      SPI_FLASH_PageWrite(pBuffer, WriteAddr, NumByteToWrite);
    }
    else /* NumByteToWrite > SPI_FLASH_PageSize */ 
    {
      while(NumOfPage--)
      {
        SPI_FLASH_PageWrite(pBuffer, WriteAddr, SPI_FLASH_PageSize);
        WriteAddr +=  SPI_FLASH_PageSize;
        pBuffer += SPI_FLASH_PageSize;  
      }    
     
      SPI_FLASH_PageWrite(pBuffer, WriteAddr, NumOfSingle);      
   }
  }
  else /* WriteAddr is not SPI_FLASH_PageSize aligned  */
  {
    if(NumOfPage== 0) /* NumByteToWrite < SPI_FLASH_PageSize */
    {
      if(NumOfSingle > count) /* (NumByteToWrite + WriteAddr) > SPI_FLASH_PageSize */
      {
      	temp = NumOfSingle - count;
      	
      	SPI_FLASH_PageWrite(pBuffer, WriteAddr, count);
        WriteAddr +=  count;
        pBuffer += count; 
        
        SPI_FLASH_PageWrite(pBuffer, WriteAddr, temp);       	
      }
      else
      {
      	SPI_FLASH_PageWrite(pBuffer, WriteAddr, NumByteToWrite);
      }
    }
    else /* NumByteToWrite > SPI_FLASH_PageSize */
    {
      NumByteToWrite -= count;
      NumOfPage =  NumByteToWrite / SPI_FLASH_PageSize;
      NumOfSingle = NumByteToWrite % SPI_FLASH_PageSize;	
      
      SPI_FLASH_PageWrite(pBuffer, WriteAddr, count);
      WriteAddr +=  count;
      pBuffer += count;  
     
      while(NumOfPage--)
      {
        SPI_FLASH_PageWrite(pBuffer, WriteAddr, SPI_FLASH_PageSize);
        WriteAddr +=  SPI_FLASH_PageSize;
        pBuffer += SPI_FLASH_PageSize;  
      }
      
      if(NumOfSingle != 0)
      {
        SPI_FLASH_PageWrite(pBuffer, WriteAddr, NumOfSingle);
      }
    }
  }      	
}

/*******************************************************************************
* Function Name  : NEW_SPI_FLASH_BufferWrite
* Description		 : 
*				   
* Input 		 :
*					
*				   
*				  
* Output	:
* Return		: None
*******************************************************************************/
void NEW_SPI_FLASH_BufferWrite(u8* pBuffer, u32 WriteAddr, u16 NumByteToWrite)
{
u16 sector = 0;

	sector = WriteAddr % SPI_FLASH_SECTORSIZE;
	if(sector == 0)
	{
		SPI_FLASH_SectorErase(WriteAddr);
	}
	SPI_FLASH_BufferWrite(pBuffer,WriteAddr,NumByteToWrite);
}


/*******************************************************************************
* Function Name  : check_BufferWrite
* Description		 : 
*				   
* Input 		 :
*					
*				   
*				  
* Output	:
* Return		: None
*******************************************************************************/
void check_BufferWrite(u8* pBuffer, u32 WriteAddr, u16 NumByteToWrite)
{
	u8 dat; 
	u16 length = NumByteToWrite;
	u8 *p;
	u16 i;
	p = pBuffer;

	SPI_FLASH_Init_GPIO();


	SPI_FLASH_ChipSelect(Low);		 

	/* Send "Read from Memory " instruction */
	SPI_FLASH_SendByte(READ);  

	/* Send ReadAddr high nibble address byte to read from */
	SPI_FLASH_SendByte((WriteAddr & 0xFF0000) >> 16);
	/* Send ReadAddr medium nibble address byte to read from */
	SPI_FLASH_SendByte((WriteAddr& 0xFF00) >> 8);	
	/* Send ReadAddr low nibble address byte to read from */
	SPI_FLASH_SendByte(WriteAddr & 0xFF);	   

	i = 0;
	while(length--) /* while there is data to be read */
	{
	  /* Read a byte from the FLASH */			  
	  dat = SPI_FLASH_SendByte(Dummy_Byte);  
	  if (dat != *p) 
	  {
	  		//Trace("new",dat);
			//Trace("old",*p);
			i++;
	  }

	  p++;
	} 

	/* Deselect the FLASH: Chip Select high */
	SPI_FLASH_ChipSelect(High);   

		
//	if (i==0) TraceStr("Packet check ok\r\n");
//	else Trace("Packet error",i);

}






/*******************************************************************************
* Function Name  : SPI_FLASH_BufferRead
* Description    : Reads a block of data from the FLASH.
* Input          : - pBuffer : pointer to the buffer that receives the data read 
*                    from the FLASH.
*                  - ReadAddr : FLASH's internal address to read from.
*                  - NumByteToRead : number of bytes to read from the FLASH.
* Output         : None
* Return         : None
*******************************************************************************/
void SPI_FLASH_BufferRead(u8* pBuffer, u32 ReadAddr, u16 NumByteToRead)
{
	 SPI_FLASH_Init_GPIO();
  /* Select the FLASH: Chip Select low */
  SPI_FLASH_ChipSelect(Low);	   
  
  /* Send "Read from Memory " instruction */
  SPI_FLASH_SendByte(READ);	 
  
  /* Send ReadAddr high nibble address byte to read from */
  SPI_FLASH_SendByte((ReadAddr & 0xFF0000) >> 16);
  /* Send ReadAddr medium nibble address byte to read from */
  SPI_FLASH_SendByte((ReadAddr& 0xFF00) >> 8);  
  /* Send ReadAddr low nibble address byte to read from */
  SPI_FLASH_SendByte(ReadAddr & 0xFF);     
   	
  while(NumByteToRead--) /* while there is data to be read */
  {
    /* Read a byte from the FLASH */			
    *pBuffer = SPI_FLASH_SendByte(Dummy_Byte);   
    /* Point to the next location where the byte read will be saved */
    pBuffer++;
  }	
  
  /* Deselect the FLASH: Chip Select high */
  SPI_FLASH_ChipSelect(High);	
}

/*******************************************************************************
* Function Name  : SPI_FLASH_ReadID
* Description    : Reads FLASH identification.
* Input          : None
* Output         : None
* Return         : FLASH identification
*******************************************************************************/
u32 SPI_FLASH_ReadID(void)
{   	 
  u32 Temp = 0, Temp0 = 0, Temp1 = 0, Temp2 = 0;
  SPI_FLASH_Init_GPIO();

  /* Select the FLASH: Chip Select low */
  SPI_FLASH_ChipSelect(Low);	   
  
  /* Send "RDID " instruction */
  SPI_FLASH_SendByte(0x9F);

  /* Read a byte from the FLASH */			
  Temp0 = SPI_FLASH_SendByte(Dummy_Byte);

  /* Read a byte from the FLASH */			
  Temp1 = SPI_FLASH_SendByte(Dummy_Byte);

  /* Read a byte from the FLASH */			
  Temp2 = SPI_FLASH_SendByte(Dummy_Byte);

  /* Deselect the FLASH: Chip Select high */
  SPI_FLASH_ChipSelect(High);
         
  Temp = (Temp0 << 16) | (Temp1 << 8) | Temp2;

  return Temp;
}

/*******************************************************************************
* Function Name  : SPI_FLASH_StartReadSequence
* Description    : Initiates a read data byte (READ) sequence from the Flash.
*                  This is done by driving the /CS line low to select the device,
*                  then the READ instruction is transmitted followed by 3 bytes
*                  address. This function exit and keep the /CS line low, so the
*                  Flash still being selected. With this technique the whole
*                  content of the Flash is read with a single READ instruction.
* Input          : - ReadAddr : FLASH's internal address to read from.
* Output         : None
* Return         : None
*******************************************************************************/
void SPI_FLASH_StartReadSequence(u32 ReadAddr)
{
	 SPI_FLASH_Init_GPIO();
  /* Select the FLASH: Chip Select low */
  SPI_FLASH_ChipSelect(Low);	   
  
  /* Send "Read from Memory " instruction */
  SPI_FLASH_SendByte(READ);	 

/* Send the 24-bit address of the address to read from -----------------------*/  
  /* Send ReadAddr high nibble address byte */
  SPI_FLASH_SendByte((ReadAddr & 0xFF0000) >> 16);
  /* Send ReadAddr medium nibble address byte */
  SPI_FLASH_SendByte((ReadAddr& 0xFF00) >> 8);  
  /* Send ReadAddr low nibble address byte */
  SPI_FLASH_SendByte(ReadAddr & 0xFF);  
}

/*******************************************************************************
* Function Name  : SPI_FLASH_ReadByte
* Description    : Reads a byte from the SPI Flash.
*                  This function must be used only if the Start_Read_Sequence
*                  function has been previously called.
* Input          : None
* Output         : None
* Return         : Byte Read from the SPI Flash.
*******************************************************************************/
u8 SPI_FLASH_ReadByte(void)
{
  return (SPI_FLASH_SendByte(Dummy_Byte));
}

/*******************************************************************************
* Function Name  : SPI_FLASH_ChipSelect
* Description    : Selects or deselects the FLASH.
* Input          : State : level to be applied on the FLASH's ChipSelect pin.
* Output         : None
* Return         : None
*******************************************************************************/
void SPI_FLASH_ChipSelect(u8 State)
{
  /* Set High or low the chip select line on PA.4 pin */
  #if 0
  GPIO_WriteBit(GPIOA, GPIO_Pin_4, (BitAction)State);
  #else
  GPIO_WriteBit(GPIOA, GPIO_Pin_15, (BitAction)State);

  #endif
}

/*******************************************************************************
* Function Name  : SPI_FLASH_SendByte
* Description    : Sends a byte through the SPI interface and return the byte 
*                  received from the SPI bus.
* Input          : byte : byte to send.
* Output         : None
* Return         : The value of the received byte.
*******************************************************************************/
u8 SPI_FLASH_SendByte(u8 byte)
{
  /* Loop while DR register in not emplty */
  while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET); 

  /* Send byte through the SPI1 peripheral */	
  SPI_I2S_SendData(SPI1, byte);	

  /* Wait to receive a byte */
  while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);

  /* Return the byte read from the SPI bus */  
  return SPI_I2S_ReceiveData(SPI1); 
}

/*******************************************************************************
* Function Name  : SPI_FLASH_SendHalfWord
* Description    : Sends a Half Word through the SPI interface and return the  
*                  Half Word received from the SPI bus.
* Input          : Half Word : Half Word to send.
* Output         : None
* Return         : The value of the received Half Word.
*******************************************************************************/
u16 SPI_FLASH_SendHalfWord(u16 HalfWord)
{
  /* Loop while DR register in not emplty */
  while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET); 

  /* Send Half Word through the SPI1 peripheral */	
  SPI_I2S_SendData(SPI1, HalfWord);	

  /* Wait to receive a Half Word */
  while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);

  /* Return the Half Word read from the SPI bus */  
  return SPI_I2S_ReceiveData(SPI1); 
}

/*******************************************************************************
* Function Name  : SPI_FLASH_WriteEnable
* Description    : Enables the write access to the FLASH.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void SPI_FLASH_WriteEnable(void)  
{

  /* Select the FLASH: Chip Select low */
  SPI_FLASH_ChipSelect(Low);	
  
  /* Send "Write Enable" instruction */
  SPI_FLASH_SendByte(WREN);
  
  /* Deselect the FLASH: Chip Select high */
  SPI_FLASH_ChipSelect(High);	
}

/*******************************************************************************
* Function Name  : SPI_FLASH_WaitForWriteEnd
* Description    : Polls the status of the Write In Progress (WIP) flag in the  
*                  FLASH's status  register  and  loop  until write  opertaion
*                  has completed.  
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void SPI_FLASH_WaitForWriteEnd(void) 
{
  u8 FLASH_Status = 0;
  
  /* Select the FLASH: Chip Select low */
  SPI_FLASH_ChipSelect(Low);
  	
  /* Send "Read Status Register" instruction */
  SPI_FLASH_SendByte(RDSR);
  
  /* Loop as long as the memory is busy with a write cycle */ 		
  do
  { 	
	 
    /* Send a dummy byte to generate the clock needed by the FLASH 
    and put the value of the status register in FLASH_Status variable */
    FLASH_Status = SPI_FLASH_SendByte(Dummy_Byte);  
	#ifdef USE_RTOS_DELAY
		vTaskDelay(5);
	#endif
																	
  } while((FLASH_Status & WIP_Flag) == SET); /* Write in progress */  

  /* Deselect the FLASH: Chip Select high */
  SPI_FLASH_ChipSelect(High);	 	
}



/*******************************************************************************
* Function Name  : SPI_FLASH_check_crc32
* Description    : Reads a block of data from the FLASH.
* Input          : - pBuffer : pointer to the buffer that receives the data read 
*                    from the FLASH.
*                  - ReadAddr : FLASH's internal address to read from.
*                  - NumByteToRead : number of bytes to read from the FLASH.
* Output         : None
* Return         : None
*******************************************************************************/
u32 SPI_FLASH_CheckCrc32(u32 start_addr, u32 length)
{
  u8 dat;
  u32 a1,i;
  /* Select the FLASH: Chip Select low */

  SPI_FLASH_Init_GPIO();

  
  SPI_FLASH_ChipSelect(Low);	   
  
  /* Send "Read from Memory " instruction */
  SPI_FLASH_SendByte(READ);	 
  
  /* Send ReadAddr high nibble address byte to read from */
  SPI_FLASH_SendByte((start_addr & 0xFF0000) >> 16);
  /* Send ReadAddr medium nibble address byte to read from */
  SPI_FLASH_SendByte((start_addr& 0xFF00) >> 8);  
  /* Send ReadAddr low nibble address byte to read from */
  SPI_FLASH_SendByte(start_addr & 0xFF);     

  a1 = 0;
  i = 0;
  while(length--) /* while there is data to be read */
  {
    /* Read a byte from the FLASH */			
    dat = SPI_FLASH_SendByte(Dummy_Byte);  
	a1 = GetCrc32_One(dat,a1,i);
	i += 1;
  }	
  
  /* Deselect the FLASH: Chip Select high */
  SPI_FLASH_ChipSelect(High);	

  return a1;	

}

// return 1 sector need not erase, all are 0xff
// return 0 sector need  erase, not all 0xff
u8 SPI_FLASH_CheckSector(u32 ReadAddr)
{
  u16 NumByteToRead = SPI_FLASH_SECTORSIZE;
  u8 dat;
   SPI_FLASH_Init_GPIO();
  /* Select the FLASH: Chip Select low */
  SPI_FLASH_ChipSelect(Low);	   
  
  /* Send "Read from Memory " instruction */
  SPI_FLASH_SendByte(READ);	 
  
  /* Send ReadAddr high nibble address byte to read from */
  SPI_FLASH_SendByte((ReadAddr & 0xFF0000) >> 16);
  /* Send ReadAddr medium nibble address byte to read from */
  SPI_FLASH_SendByte((ReadAddr& 0xFF00) >> 8);  
  /* Send ReadAddr low nibble address byte to read from */
  SPI_FLASH_SendByte(ReadAddr & 0xFF);     
   	
  while(NumByteToRead--) /* while there is data to be read */
  {
    /* Read a byte from the FLASH */			
    dat = SPI_FLASH_SendByte(Dummy_Byte);   
    /* Point to the next location where the byte read will be saved */
    if (dat != 0xFF) {SPI_FLASH_ChipSelect(High); return  0;}
  }	
  
  /* Deselect the FLASH: Chip Select high */
  SPI_FLASH_ChipSelect(High);	
  return 1;
}





u32 SPI_FLASH_GetCurLogPos(u32 ReadAddr,u32 size)
{
  u32 len = size;
  u8 i;
  u8 dat[16];
  u32 pos;
  SPI_FLASH_Init_GPIO();
  /* Select the FLASH: Chip Select low */
  SPI_FLASH_ChipSelect(Low);	   
  
  /* Send "Read from Memory " instruction */
  SPI_FLASH_SendByte(READ);	 
  
  /* Send ReadAddr high nibble address byte to read from */
  SPI_FLASH_SendByte((ReadAddr & 0xFF0000) >> 16);
  /* Send ReadAddr medium nibble address byte to read from */
  SPI_FLASH_SendByte((ReadAddr& 0xFF00) >> 8);  
  /* Send ReadAddr low nibble address byte to read from */
  SPI_FLASH_SendByte(ReadAddr & 0xFF);     

  i = 0;	
  pos = ReadAddr;
  while(len>0) /* while there is data to be read */
  {
    /* Read a byte from the FLASH */		
	for (i=0;i<16;i++)  dat[i] = SPI_FLASH_SendByte(Dummy_Byte); 
	if (dat[0] == 0xFF && dat[1] == 0xFF && dat[2] == 0xFF && dat[3] == 0xFF)
		break;
	
	len -= 16;
	pos = pos + 16;
  }	
  
  
  /* Deselect the FLASH: Chip Select high */
  SPI_FLASH_ChipSelect(High);	
   return pos;
}


/********************************************************************************************/


/*******************************************************************************
* Function Name  : SPI_FLASH_check_sum
*******************************************************************************/
u32 SPI_FLASH_CheckSum(u32 start_addr, u32 length)
{
	u8 dat;
	u32 sum;
  /* Select the FLASH: Chip Select low */

  sum = 0;

  SPI_FLASH_Init_GPIO();

  
  SPI_FLASH_ChipSelect(Low);	   
  
  /* Send "Read from Memory " instruction */
  SPI_FLASH_SendByte(READ);	 
  
  /* Send ReadAddr high nibble address byte to read from */
  SPI_FLASH_SendByte((start_addr & 0xFF0000) >> 16);
  /* Send ReadAddr medium nibble address byte to read from */
  SPI_FLASH_SendByte((start_addr& 0xFF00) >> 8);  
  /* Send ReadAddr low nibble address byte to read from */
  SPI_FLASH_SendByte(start_addr & 0xFF); 

  while(length--) /* while there is data to be read */
  {
    /* Read a byte from the FLASH */			
    dat = SPI_FLASH_SendByte(Dummy_Byte);  
	sum += dat;
  }	
  
  /* Deselect the FLASH: Chip Select high */
  SPI_FLASH_ChipSelect(High);	


  return sum;	

}



