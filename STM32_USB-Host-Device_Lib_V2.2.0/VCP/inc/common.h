#ifndef  __COMMON_H__
#define __COMMON_H__

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

void Trace(char *buf, u32 dat);
void TraceStr_str(char *buf);
void TraceHex(char *buf, u16 len);



//#define DEBUG_TRACE_STR

#ifdef DEBUG_TRACE_STR
	#define TraceStr(x) \
		do{\
			char aaa[10]={0};\
			sprintf(aaa,"F:%s,L:%d,",__FILE__,__LINE__);\
			TraceStr_str(aaa);\
			TraceStr_str(x);\
		}while(0);
#else
	//#define TraceStr(x) TraceStr_str(x)
		#define TraceStr printf
#endif


#define  	MIN_MALLOC_SIZE					32
#define  	CALC_MALLOC_SIZE(x)				(((x)+MIN_MALLOC_SIZE-1)&(~(u32)(MIN_MALLOC_SIZE-1)))

		
#endif //__COMMON_H__
