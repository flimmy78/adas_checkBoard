#ifndef bool
#define	bool			unsigned char
#define true			1
#define false			(!true)
#endif


#define 	u8 		unsigned char
#define  	u16		unsigned short
#define 	u32 	unsigned int



void drv_delayms(u32 ms);
void drv_delayus(u32 us);

void Trace(char *buf,u32 dat);
void TraceStr(char *buf);

#define  	MIN_MALLOC_SIZE					32
#define  	CALC_MALLOC_SIZE(x)				(((x)+MIN_MALLOC_SIZE-1)&(~(u32)(MIN_MALLOC_SIZE-1)))

