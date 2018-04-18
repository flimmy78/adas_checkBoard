

/* Map info store in mcubin last,  code_scatter_t */


typedef struct
{
	u32 StartAddr;
	u32 Length;
	u32 CrcFile;
	u32 CrcBinInfo;
	
}BIN_INFO_t;



typedef struct
{
	BIN_INFO_t McuBin;
	BIN_INFO_t FpgaBin;
	BIN_INFO_t PicBin;
	BIN_INFO_t WarnBin;
	BIN_INFO_t CarBin;
}SPI_FLASH_INFO_t;




#define 	SPI_FLASH_INFO_START_ADDR		0x0
#define 	SPI_FLASH_INFO_MAX_LENGTH		(1024*4)
#define 	SPI_FLASH_INFO_END_ADDR			(SPI_FLASH_INFO_START_ADDR + SPI_FLASH_INFO_MAX_LENGTH)


#define 	MCUBIN_START_ADDR				SPI_FLASH_INFO_END_ADDR		
#define 	MCUBIN_MAX_LENGTH				(1024*256)
#define 	MCUBIN_END_ADDR					(MCUBIN_START_ADDR + MCUBIN_MAX_LENGTH)

#define 	FPGABIN_START_ADDR				MCUBIN_END_ADDR		
#define 	FPGABIN_MAX_LENGTH				(1024*512)
#define 	FPGABIN_END_ADDR				(FPGABIN_START_ADDR + FPGABIN_MAX_LENGTH)

#define 	PICBIN_START_ADDR				FPGABIN_END_ADDR		
#define 	PICBIN_MAX_LENGTH				(1024*512)
#define 	PICBIN_END_ADDR					(PICBIN_START_ADDR + PICBIN_MAX_LENGTH)


#define 	WARNBIN_START_ADDR				PICBIN_END_ADDR		
#define 	WARNBIN_MAX_LENGTH				(1024*4)
#define 	WARNBIN_END_ADDR				(WARNBIN_START_ADDR + WARNBIN_MAX_LENGTH)

#define 	CARBIN_START_ADDR				WARNBIN_END_ADDR		
#define 	CARBIN_MAX_LENGTH				(1024*4)
#define 	CARBIN_END_ADDR					(CARBIN_START_ADDR + CARBIN_MAX_LENGTH)






