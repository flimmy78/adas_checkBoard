

#include "inc_all.h"

#include "crypt.h"

#define   Ver      "1.1.2"


#define u8		unsigned char
#define u16		unsigned short
#define u32		unsigned int

#define	MAX_FILENAME_LEN		256



char mapfile[MAX_FILENAME_LEN];
char binfile[MAX_FILENAME_LEN];
char cryptfile[MAX_FILENAME_LEN];



void help(void)
{

}

int random_mode = 0;

unsigned char FPGA_key_init[16] = {'A','D','A','S','L','e','a','d','e','r','-','c','r','y','p','t'};

unsigned char FPGA_key[16];

unsigned char cpukey[12];// = {0x33,0xff,0xd8,0x05,0x47,0x4b,0x39,0x30,0x47,0x30,0x18,0x43};
//This cpu id for board 013

unsigned char key[16];

						
#define base_str		"Region$$Table$$Base     "
#define limit_str		"Region$$Table$$Limit    "


//#define FLASH_START_ADDR	0x08000000
#define FLASH_START_ADDR	0x08060000
#define SCATTER_CALL_SIZE	28

#define FLASH_SIZE				(128*1024)
#define CRYPT_MORE_DATA_ADDR	(120*1024)
#define CRYPT_MORE_DATA_SIZE	(FLASH_SIZE - CRYPT_MORE_DATA_ADDR)
#define CRYPT_CRC32_ADDR		(CRYPT_MORE_DATA_ADDR - 4)




#define CRYPT1_IN_LEN		28
#define CRYPT1_OUT_LEN		32

#define DECRYPT1_IN_LEN		32
#define DECRYPT1_OUT_LEN	28

//__scatterload                            0x08009e99   Thumb Code    28  init.o(.text)



void make_key(u8 *cpukey,u8 *key)
{
	key[0] = cpukey[0] ^ cpukey[4] ^ cpukey[8];
	key[1] = cpukey[1] ^ cpukey[5] ^ cpukey[9];
	key[2] = cpukey[2] ^ cpukey[6] ^ cpukey[10];
	key[3] = cpukey[3] ^ cpukey[7] ^ cpukey[11];
	key[4] = cpukey[0] ^ cpukey[5];
	key[5] = cpukey[1] ^ cpukey[6];
	key[6] = cpukey[2] ^ cpukey[7];
	key[7] = cpukey[3] ^ cpukey[4];
	key[8] = cpukey[4] ^ cpukey[11];
	key[9] = cpukey[5] ^ cpukey[10];
	key[10] = cpukey[6] ^ cpukey[9];
	key[11] = cpukey[7] ^ cpukey[8];
	key[12] = cpukey[2] ^ cpukey[6];
	key[13] = cpukey[3] ^ cpukey[7];
	key[14] = cpukey[4] ^ cpukey[8];
	key[15] = cpukey[5] ^ cpukey[9];
}



void get_code_info(u8 *BinBuf)	//BinBuf is origin bin file buf
{
	memcpy(code_scatter,BinBuf,12);
	code_scatter.code_from  =  code_scatter.code_from - FLASH_START_ADDR;
}


void change_key(u8 *key,u8 len)
{
	u8 i;
	u8 tmp;

	tmp = key[0];

	for (i=0;i<(len-1);i++)
	{
		key[i] = key[i+1];
	}
	key[len-1] = tmp;
}

code_scatter_t code_scatter;

u8 CryptTmpBuf[1024*32];

void crypt_main(void)

{
	FILE *fp;
	u32 FileLen;
	u8 *BinBuf;
	u8 *CryptInBuf;
	u8 *CryptOutBuf;
	u8 *CryptBinBuf;

	u32 i,j;
	u16 ra ;
	u32 CryptInLen,CryptOutLen;
	u32 crc32;
	u8 tmpbuf[4];

	// Generate a random data init
	u32 seed = (u32)GetCurrentTime();
	
	printf("Current seed = %d \r\n");
	
	srand(seed); 

	SPI_FLASH_BufferRead(BinBuf,0,12);


	get_code_info(BinBuf);

	i = code_scatter.code_len % CRYPT1_IN_LEN;
	if (i==0) CryptInLen = code_scatter.code_len;
	else CryptInLen = (code_scatter.code_len+CRYPT1_IN_LEN)/CRYPT1_IN_LEN * CRYPT1_IN_LEN;

	CryptInBuf = new u8 [CryptInLen];
	CryptOutBuf = new u8 [CryptInLen*2];
	
	memcpy(CryptInBuf,&BinBuf[code_scatter.code_from],code_scatter.code_len);

	crc32 = GetCrc32(CryptInBuf,code_scatter.code_len);

	//first crypt, we use random data
	i = 0;
	j = 0;
	
	
	while (1)
	{
		ra = rand() & 0xFFFF;
	//	ra = 0x1234;
		encrypt_41F230(ra & 0xFF,(ra >>8) & 0xFF,&CryptInBuf[i],&CryptOutBuf[j]);
		i += 28;
		j += 32;
		if (i>= CryptInLen) break;
	}
	CryptOutLen = j;

	//Crypt buf is ready , now calc with key
	
	make_key(cpukey,key); 
	i = 0;
	while (1)
	{
		if ((i+16)> CryptOutLen) break;
		for (j=0;j<16;j++)
		{
			CryptOutBuf[i+j] ^= key[j];
		}
		i += 16;
	}
	// not calc last bytes


	//just for test
	memcpy(CryptTmpBuf,CryptOutBuf,CryptOutLen);



	//now FPGA crypt
	memcpy(FPGA_key,FPGA_key_init,sizeof(FPGA_key_init));
	j = 0;
	for (i=0;i<CryptOutLen;i++)
	{ 

		CryptOutBuf[i] = encrypt_one(CryptOutBuf[i],FPGA_key[j]);

		if (j==15)
		{
			change_key(FPGA_key,16);
			j = 0;
		}
		else j++;
		
	}



	//Now, copy data to new bin file
	CryptBinBuf = new u8 [FLASH_SIZE];
	memset(CryptBinBuf,0xff,FLASH_SIZE);

	i = code_scatter.code_from;
	memcpy(CryptBinBuf,BinBuf,FileLen);
	//memcpy(CryptBinBuf,BinBuf,i);
	memcpy(&CryptBinBuf[i],CryptOutBuf,code_scatter.code_len);
	memcpy(&CryptBinBuf[CRYPT_MORE_DATA_ADDR],&CryptOutBuf[code_scatter.code_len],CryptOutLen-code_scatter.code_len);

	//for test
	//memcpy(&CryptBinBuf[ORIGIN_DATA_ADDR],CryptTmpBuf,code_scatter.code_len/28*32);
	

	fopen_s(&fp,cryptfile,"wb");
	if (fp==NULL)
	{
		return;
	}
	fwrite(CryptBinBuf,1,FLASH_SIZE,fp);

	//wire crc32 
	fseek(fp,CRYPT_MORE_DATA_ADDR-4,SEEK_SET);

	tmpbuf[0] = crc32 & 0xff;
	tmpbuf[1] = (crc32>>8) & 0xff;
	tmpbuf[2] = (crc32>>16) & 0xff;
	tmpbuf[3] = (crc32>>24) & 0xff;

	fwrite(tmpbuf,1,4,fp);

	printf("crc32(0x%x) \r\n",crc32);


	fclose(fp);



	
	delete [] CryptOutBuf;
	delete [] CryptInBuf;
	delete [] BinBuf;
	delete [] CryptBinBuf;
}



#define DECRYPT_OUTPUT_FILE		"F://work//mcu_m4//mcu_decrypt_code.bin"

void decrypt_main(void)
{

	FILE *fp;
	u8 *BinBuf;
	u8 *DeCryptInBuf;
	u8 *DeCryptOutBuf;
	u32 i,j;
	u32 DeCryptInLen,DeCryptOutLen;
	u8 tmpbuf[32];
	u32 crc32;
	u8 debugbuf[32];

	fopen_s(&fp,cryptfile,"rb");
	if (fp==NULL)
	{
		return;
	}

	BinBuf = new u8[FLASH_SIZE];
	fread(BinBuf,1,FLASH_SIZE,fp);
	fseek(fp,CRYPT_CRC32_ADDR,SEEK_SET);
	fread(tmpbuf,1,4,fp);
	fclose(fp);

	crc32 = tmpbuf[0] + (tmpbuf[1]<<8) + (tmpbuf[2]<<16) + (tmpbuf[3]<<24);




	i = code_scatter.code_len % CRYPT1_IN_LEN;
	if (i==0) i = code_scatter.code_len;
	else i = (code_scatter.code_len+CRYPT1_IN_LEN)/CRYPT1_IN_LEN * CRYPT1_IN_LEN;

	DeCryptInLen = i/CRYPT1_IN_LEN * CRYPT1_OUT_LEN;
	DeCryptOutLen = code_scatter.code_len;



	DeCryptInBuf = new u8 [FLASH_SIZE];
	DeCryptOutBuf = new u8 [FLASH_SIZE];

	memcpy(DeCryptInBuf,&BinBuf[code_scatter.code_from],code_scatter.code_len);
	memcpy(&DeCryptInBuf[code_scatter.code_len],&BinBuf[CRYPT_MORE_DATA_ADDR],DeCryptInLen-code_scatter.code_len);

	//now FPGA decrypt
	memcpy(FPGA_key,FPGA_key_init,sizeof(FPGA_key_init));
	j = 0;
	for (i=0;i<DeCryptInLen;i++)
	{ 


		DeCryptInBuf[i] = decrypt_one(DeCryptInBuf[i],FPGA_key[j]);
		if (i<16) tmpbuf[i] = DeCryptInBuf[i];
		if (j==15)
		{
			change_key(FPGA_key,16);
			j = 0;
		}
		else j++;
		
	}

	//calc with cpu key
	make_key(cpukey,key); 
	i = 0;
	while (1)
	{
		if ((i+16)> DeCryptInLen) break;
		for (j=0;j<16;j++)
		{
			DeCryptInBuf[i+j] ^= key[j];
			tmpbuf[j] = DeCryptInBuf[i+j];
		}
		i += 16;
	}
	// not calc last bytes

	i = 0;
	j = 0;
	while (1)
	{
		memcpy(debugbuf,&DeCryptInBuf[i],32);
		decrypt_41F2C0(&DeCryptInBuf[i],tmpbuf);
		memcpy(&DeCryptOutBuf[j],&tmpbuf[3],28);
		i += 32;
		j += 28;
		if (i>= DeCryptInLen) break;
	}

	memcpy(&BinBuf[code_scatter.code_from],DeCryptOutBuf,code_scatter.code_len);
	memset(&BinBuf[CRYPT_MORE_DATA_ADDR],0,CRYPT_MORE_DATA_SIZE);


	if (crc32 != GetCrc32(&BinBuf[code_scatter.code_from],code_scatter.code_len))
	{
		return;
	}



	fopen_s(&fp,DECRYPT_OUTPUT_FILE,"wb");
	if (fp==NULL)
	{
		return;
	}
	fwrite(BinBuf,1,FLASH_SIZE,fp);

	fclose(fp);






	
	delete [] DeCryptOutBuf;
	delete [] DeCryptInBuf;
	delete [] BinBuf;

}



int _tmain(int argc, _TCHAR* argv[])
{


#ifdef DEBUG_CODE
	sprintf_s(mapfile,MAX_FILENAME_LEN,"%s","F://work//mcu_m4//obj//mcu_code.map");
	printf("filename = %s\r\n",mapfile);

	sprintf_s(binfile,MAX_FILENAME_LEN,"%s","F://work//mcu_m4//mcu_code.bin");
	printf("filename = %s\r\n",binfile);


	sprintf_s(cryptfile,MAX_FILENAME_LEN,"%s","F://work//mcu_m4//crypt_code.bin");
	printf("filename = %s\r\n",binfile);
#else
	if (argc<5)
	{
		help();
		return 0;
	}
//	random_mode = 0;
//	if (argc == 5)     // CPU key is must
	{
		char tmpbuf[64];
		sprintf_s(tmpbuf,64,"%S",argv[4]);
		sscanf_s(tmpbuf,"%2X%2X%2X%2X%2X%2X%2X%2X%2X%2X%2X%2X",&cpukey[0],&cpukey[1],&cpukey[2],&cpukey[3],&cpukey[4],&cpukey[5],&cpukey[6],&cpukey[7],&cpukey[8],&cpukey[9],&cpukey[10],&cpukey[11]);
		printf("cpukey[12] = 0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x",cpukey[0],cpukey[1],cpukey[2],cpukey[3],cpukey[4],cpukey[5],cpukey[6],cpukey[7],cpukey[8],cpukey[9],cpukey[10],cpukey[11]);
		random_mode = 1;

		if (tmpbuf =="33FFD805474B393047301843") printf("Now is fixed cpu key mode \r\n");



	}
//	else printf("Now is fixed cpu key mode \r\n");


	sprintf_s(mapfile,MAX_FILENAME_LEN,"%S",argv[1]);
	printf("filename = %s\r\n",mapfile);

	sprintf_s(binfile,MAX_FILENAME_LEN,"%S",argv[2]);
	printf("filename = %s\r\n",binfile);

	sprintf_s(cryptfile,MAX_FILENAME_LEN,"%S",argv[3]);
	printf("filename = %s\r\n",cryptfile);

#endif

	crypt_main();
	decrypt_main();
	return 0;
}




/***********************************************************************/




const u8 crypt_table_byte_4213E0[256] =
{

0xBC,0x8B,0xE5,0x35,0x1D,0x0E,0xAE,0x23,0xB9,0x4D,0xFE,0x50,0x15,0xC7,0x25,0x60,
0xA5,0xD8,0x1E,0x9A,0x6B,0x9A,0xD5,0xC0,0x60,0xCC,0x50,0x57,0xBB,0x93,0xB3,0x49,
0x94,0x25,0xF6,0x22,0x31,0xFD,0x9A,0xF0,0xDC,0x04,0xB6,0x53,0xCD,0x1B,0xEE,0x70,
0x50,0x44,0x20,0xCE,0xB3,0x2B,0x7A,0xD7,0x91,0xE3,0x9E,0xCD,0xCF,0x0D,0xB6,0xFD,
0xF6,0xB4,0x29,0x6B,0xB3,0xDE,0xC7,0x7F,0xEF,0x08,0xEC,0x63,0x88,0xA8,0x0A,0x51,
0xF9,0xCA,0x6A,0x42,0xC6,0x84,0x1F,0xBB,0xCE,0x52,0x3C,0x0F,0x11,0x0C,0xC9,0x74,
0xE2,0x99,0x90,0x85,0x8D,0x6F,0xCD,0xD3,0xCE,0x65,0xF9,0x63,0x23,0x96,0x15,0x40,
0x0C,0x79,0x95,0x87,0x8E,0xD3,0x20,0x56,0x10,0xC3,0x3A,0x94,0xFA,0x56,0xC5,0x65,
0x25,0xBA,0x61,0xE9,0xC5,0x09,0x29,0xB6,0x34,0x76,0xED,0x80,0xCF,0xFF,0x8A,0x49,
0x9A,0xF0,0xCF,0x7D,0xC4,0x6A,0xB9,0xF9,0xE1,0xA4,0x97,0xFB,0x47,0x6D,0x8C,0x6A,
0x2F,0xB7,0x00,0xB2,0x65,0xA0,0xE4,0x6A,0x53,0xD8,0x7E,0x1C,0xFC,0xE1,0x92,0x8F,
0xB2,0xEA,0xCB,0xFF,0x8B,0x62,0x99,0x2E,0x71,0x95,0x96,0xB9,0x89,0x61,0x32,0x20,
0x1A,0x4B,0x5F,0xDE,0x23,0x3A,0xAE,0xD7,0xBE,0x12,0x6D,0x07,0x28,0x2B,0x61,0xAE,
0xC5,0x04,0x54,0x2E,0x71,0xA2,0xD6,0xD2,0x42,0x37,0x37,0x8D,0xBC,0x0E,0x27,0x9A,
0x50,0x8A,0x6D,0x42,0xBA,0x1B,0x89,0xBC,0x18,0xD6,0x1A,0x09,0x79,0x1D,0x46,0xBF,
0x9A,0x44,0x2F,0x26,0xCF,0x1A,0x92,0x8F,0x1D,0xD4,0xB6,0x6F,0xD9,0xC8,0xAA,0x68

};



u8 myrandom = 0x33;

static signed short   encrypt_41F230(char a1, char a2, const void *input_buffer, u8 * a4)  //a4 = xx8700 +62H, output buffer, input_buffer is input buffer,v44, 0x1c length, a2 length = 0x21,  a1 = v34 or -2
{																		//v44[0] =0, v5 or random, v45 or others are random			
	u8 v4; // al@1
	u8 v5;
	u8 v6; // bl@1
	u32 v7; // ebp@1
	u8 result;
	u8 *v9; // esi@3
	u8 v10; // edx@3
	u8 v11; // dl@4
	u8 v12; // cl@4
	u8 v13; // [sp+13h] [bp-1h]@1
	u8 v14; // [sp+24h] [bp+10h]@1



  v6 = 0;
  v4 = myrandom;    
  v7 = (u32)a4;
  v14 = v4 + 81;
  v13 = v4 - 46;
  *(u8 *)(v7 + 1) = a1;
  *(u8 *)v7 = v4;
  *(u8 *)(v7 + 2) = a2;
  memcpy((void *)(v7 + 3), input_buffer, 0x1C);
  v5 = 0;
  do
    v6 += *(u8 *)(v5++ + v7);
  while ( v5 < 31 );
  if (v4<0xE0) v10 = v4;
  else v10 = v4-0xe0;
 // v10 = v4 % 0xE0;
  *(u8 *)(v7 + 31) = v6;
  result = 1;
  v9 = (u8 *)(&crypt_table_byte_4213E0[v10]);
  do
  {
    v11 = v14 + (u8)(2 * result) + (*(u8 *)(result + v7) ^ (*v9));
    v12 = v13 + (result++);
    *(u8 *)(v7 - 1 + result) = v12 ^ v11;
    ++v9;
  }
  while ( result < 32 );

  
  return result;
}

/***********************************************************************************************/








/***********************************************************************************************/



//1307
static signed short   decrypt_41F2C0(u8 *input_buffer, u8 *a2)   // input_buffer input, a2 output, decrypt   this function test ok
{
  u8 v2; // ecx@1
  u8 v3; // bl@1
  u8 *v4; // esi@1
  u8 v5; // al@1
  u8 v6; // al@2
  u8 result; // eax@4
  u8 v9; // [sp+11h] [bp-27h]@1
  u8 v10; // [sp+12h] [bp-26h]@1
  u8 v13; // [sp+33h] [bp-5h]@3



  a2[0] = 0;

  v5 = *input_buffer;
  v10 = *input_buffer - 46;
  v3 = v5 + 81;
  v9 = 0;
  v2 = 1;

  if (v5>=0xe0) v5 -= 0xe0;	

	  
  v4 = (u8 *)(&crypt_table_byte_4213E0[v5]);
  do
  {
    v6 = (*v4) ^ ((input_buffer[v2] ^ (v10 + v2)) - (v2<<1) - v3);
    ++v2;
    v9 += v6;
	a2[v2-1] = v6;
    ++v4;
  }
  while ( v2 < 32 );
  
  v13 = a2[0x20-1];
  if ( (u8)(v9 + *input_buffer) - (u8)(v13<<1) )
  {
    result = 0xFF;
  }
  else
  {
    //memcpy(a2, a2, 0x20u);
    result = 0;
  }

  return result;
}


static u8 encrypt_one(u8 indat,u8 key)    
{

	u8 tmpdat;
	tmpdat = 0;

	if (indat & 0x01)   tmpdat |= 0x04;
	if (indat & 0x02)   tmpdat |= 0x08;
	if (indat & 0x04)   tmpdat |= 0x40;
	if (indat & 0x08)   tmpdat |= 0x80;
	if (indat & 0x10)   tmpdat |= 0x01;
	if (indat & 0x20)   tmpdat |= 0x02;
	if (indat & 0x40)   tmpdat |= 0x10;
	if (indat & 0x80)   tmpdat |= 0x20;
	return (tmpdat^key);

}

static u8 decrypt_one(u8 indat,u8 key)    
{

	u8 tmpdat;
	u8 in;
	tmpdat = 0;
	in = indat^key;

	if (in & 0x01)   tmpdat |= 0x10;
	if (in & 0x02)   tmpdat |= 0x20;
	if (in & 0x04)   tmpdat |= 0x01;
	if (in & 0x08)   tmpdat |= 0x02;
	if (in & 0x10)   tmpdat |= 0x40;
	if (in & 0x20)   tmpdat |= 0x80;
	if (in & 0x40)   tmpdat |= 0x04;
	if (in & 0x80)   tmpdat |= 0x08;
	return (tmpdat);

}


/*********************************************************/

