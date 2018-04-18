

/*

FIFO 8 bit data buffer

*/



typedef struct {
	u32 head;
	u32 rear;
	u32 FIFO_size;
	u8 *databuf;
} FIFO_t;




void  InitFIFO(FIFO_t *stFIFO, u8 *buf, u32 size);
void 	EnFIFO(FIFO_t *stFIFO, u8 dat);
bool  EnFIFOBuf(FIFO_t *stFIFO, u8 *buf, u16 len);
bool  DeFIFOBuf(FIFO_t *stFIFO, u8 *buf, u16 len);
u8	 	DeFIFO(FIFO_t *stFIFO);
u16 	FIFOLen(FIFO_t *stFIFO);






