

#include "inc_all.h"





Measurement_Para_t MeasPara;


float ratio[9] = {1.1, RANK2_RATIO, RANK3_RATIO, 1.05, 1.05, 2.05, 2.52, 2.04, 1.86};



__ALIGN_BEGIN  static u16 ADCConvertedValue[TOTAL_ADC_NUM * ADC_AGV_NUM]
__ALIGN_END;     //unit: 0.01V

static void init_ADC_DMA(void);


static void InitADCGPIO(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure;
	RCC_AHBPeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	RCC_AHBPeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	RCC_AHBPeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	/* Configure the GPIO_LED pin */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;     //3.3V
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_Init(GPIOC, &GPIO_InitStructure);



	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;    //12V
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;    //5V
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;    //2.5V
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;    //1.2V
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;    //1.8
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;    //IN_POWER1
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;    //IN_POWER2
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;    //IN_POWER3
	GPIO_Init(GPIOB, &GPIO_InitStructure);

}

void InitADC(void)
{
	u32 i;
	ADC_InitTypeDef ADC_InitStructure;
	RCC_ADCCLKConfig(RCC_PCLK2_Div8);
	ADC_DeInit(ADC1);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
	//RCC_APB2PeriphResetCmd(RCC_APB2Periph_AFIO, ENABLE);

	InitADCGPIO();


	//Clear ram
	for (i = 0; i < TOTAL_ADC_NUM * ADC_AGV_NUM; i++) {
		ADCConvertedValue[i] = 0;
	}


	init_ADC_DMA();


	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
	ADC_InitStructure.ADC_ScanConvMode = ENABLE;
	ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_InitStructure.ADC_NbrOfChannel = TOTAL_ADC_NUM;
	ADC_Init(ADC1, &ADC_InitStructure);




	ADC_RegularChannelConfig(ADC1, ADC_Channel_10, 1,
	                         ADC_SampleTime_71Cycles5);	// PC0, mcu 3.3v,         3.3V
	ADC_RegularChannelConfig(ADC1, ADC_Channel_11, 2,
	                         ADC_SampleTime_71Cycles5);	// PC1, mb 5v             12V
	ADC_RegularChannelConfig(ADC1, ADC_Channel_12, 3,
	                         ADC_SampleTime_71Cycles5);	// PC2,  5V               5V
	ADC_RegularChannelConfig(ADC1, ADC_Channel_13, 4,
	                         ADC_SampleTime_71Cycles5);	// PC3, 3.3V              2.5V
	ADC_RegularChannelConfig(ADC1, ADC_Channel_1, 5,
	                         ADC_SampleTime_71Cycles5);		// PA1, 2.5v              1.8V
	ADC_RegularChannelConfig(ADC1, ADC_Channel_14, 6,
	                         ADC_SampleTime_71Cycles5);	// PC4, 5v                1.2V

	ADC_RegularChannelConfig(ADC1, ADC_Channel_15, 7,
	                         ADC_SampleTime_1Cycles5);  //PC5  IN_POWER1   ADC_SampleTime_41Cycles5
	ADC_RegularChannelConfig(ADC1, ADC_Channel_8, 8,
	                         ADC_SampleTime_1Cycles5);		//PB0  IN_POWER1
	ADC_RegularChannelConfig(ADC1, ADC_Channel_9, 9,
	                         ADC_SampleTime_1Cycles5);		//PB1  IN_POWER1

	//ADC_RegularChannelConfig(ADC1, ADC_Channel_15, 7, ADC_SampleTime_41Cycles5);	// PC5




	ADC_DMACmd(ADC1, ENABLE);
	ADC_Cmd(ADC1, ENABLE);
	ADC_ResetCalibration(ADC1);
	while (ADC_GetResetCalibrationStatus(ADC1));
	ADC_StartCalibration(ADC1);
	while (ADC_GetCalibrationStatus(ADC1));
	ADC_SoftwareStartConvCmd(ADC1, ENABLE);

}



static void init_ADC_DMA(void)
{
	DMA_InitTypeDef DMA_InitStructure;
	/* Enable DMA1 clock */
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
	/* DMA1 channel1 configuration ----------------------------------------------*/
	DMA_DeInit(DMA1_Channel1);
	DMA_InitStructure.DMA_PeripheralBaseAddr = ADC1_DR_Address;
	DMA_InitStructure.DMA_MemoryBaseAddr = (u32)ADCConvertedValue;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
	DMA_InitStructure.DMA_BufferSize = TOTAL_ADC_NUM * ADC_AGV_NUM;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
	DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
	DMA_Init(DMA1_Channel1, &DMA_InitStructure);
	/* Enable DMA1 channel1 */
	DMA_Cmd(DMA1_Channel1, ENABLE);
}



char strbuf[128];
void ReadADC(void)
{
	u32 i, j;
	float m;
	u16 adc;
	//DMA_Cmd(DMA1_Channel1, DISABLE);
	//ADC_DMACmd(ADC1, DISABLE);
	//ADC_SoftwareStartConvCmd(ADC1, DISABLE);

	//TraceStr("\r\n adc value start \r\n");
	for (i = 0; i < TOTAL_ADC_NUM; i++) {
		adc = 0;
		for (j = 0; j < ADC_AGV_NUM; j++) {
			adc += ADCConvertedValue[j * TOTAL_ADC_NUM + i];
		}
		adc = adc / ADC_AGV_NUM;
		m = adc * 3.3 / 4096 * ratio[i] * 329 / 330;
		//m1 = adc * 3.3 /4096 * 329 / 330;
		//sprintf(strbuf,"adc value: %d voltage1 : %f ,voltage2: %f \r\n",adc,m1,m);
		TraceStr(strbuf);
		MeasPara.Voltage[i] = m * 100;

	}
	//TraceStr("\r\n adc value stop \r\n");
}











