#include "ti_msp_dl_config.h"
#include "board.h"
#include <stdio.h>
#include "my_arm_math.h"
#include "arm_const_structs.h"
#include "arm_math.h"
#include "oled.h"


/*-------------------------�궨��----------------------*/
#define NUM_SAMPLES  		256					//FFT����
#define NUM_ALL      		264					//�ܲ���������FIFO����������������ǰһ�β�����ʣ������ʹ��DMA�������ֶ����䣩
#define ADC_FIFO_SAMPLES 	(NUM_ALL >> 1)		//FIFO���������ADC�������16λ��ÿ�����ϳ�һ��word(32λ)
#define EXPECT_Vpp  		 0.8				//����Vpp
#define Freq_SYS 			32000000.0			//ϵͳʱ��Ƶ�ʣ�ADC��Ƶ���ڴ�Ƶ��
/*---fft--*/
#define IFFTFLAG 			0					// ifftFlag 0--����任   1--����任
#define BITREVERSE 			1					// bitreverseflag 1--����λ��ת   0--������λ��ת

/*--------------------ȫ�ֱ���---------------------------------------------*/
/*ADC*/
volatile bool 		gCheckADC;							//ADC������־λ
volatile uint16_t 	gAdcResult[NUM_ALL];				//ADC��������
/**Ԥ����**/
uint32_t 			temp_period=0;
volatile uint16_t 	adc_sample_time;
float 				facter;								//�����������ֵԤ������Ӧ��Ƶ�ʷ�������
														//���������Ƶ�׷����� index ��������������ǵ�ʵ��Ƶ��
/*FFT*/											
float 				ADC_RATE; 							// �����Ĳ������� 
volatile float32_t 	fcom_gAdcResult[NUM_SAMPLES*2];		//FFT�����������
volatile float32_t 	fl_gFFTOutput_mag[NUM_SAMPLES];		//FFT���Ƶ��
float 				Xiebo[6];							//ֱ��������ǰ���г��
float 				thd;								//ʧ���
/*VCA810*/
float 				Vpp;								//�����źŷ��ֵ
float 				gain;								//vca810����
/*������Ƶ*/
float 				freq_base;							//�źŻ�Ƶ
int 				base_index;
float 				max_mag;
/*��ͼ*/

/*debug*/
uint16_t adc_num;

/*--------��������------------------*/
float fl_Get_Vpp(volatile uint16_t * data,uint16_t DATASIZE);
void v_Gain_Change(float gain);
void v_DMA_Config(void);
void v_Set_ADC_Freq(float sample_freq);
void v_FFT(void);							//�����������ֵԤ���������fft����õ�Ƶ��
void v_OLED_display(float thd,float *harmo,uint32_t *getTime,float freq_base);
int main(void)
{
	
	int i;
	SYSCFG_DL_init();
	OLED_Init();    //��ʼ��OLED
    OLED_Clear();
	
    while (1) {
		
		/*-------��һ�׶Ρ��������������źŷ���-----------*/
		ADC_RATE = 256000.0;		//��ʼ����Ƶ�ʣ�����ȷ�ϻ�Ƶ
		gain = 1.0;					//��ʼ����
		v_Gain_Change(gain);		
		delay_us(10000);

		//ADC����
		v_Set_ADC_Freq(ADC_RATE);
		gCheckADC = false;
		v_DMA_Config();
		while (false == gCheckADC) {
			__WFE();
		}
		
		//�������������ˣ�����������ڻ���
		Vpp = fl_Get_Vpp(gAdcResult,NUM_SAMPLES);
		gain = gain * EXPECT_Vpp / (Vpp );
		v_Gain_Change(gain);	
		delay_us(1000);
		
		/*-------�ڶ��׶Ρ�����������Ƶ��----------------*/
		//�ٲ���һ��
		gCheckADC = false;
		v_DMA_Config();
		while (false == gCheckADC) {
			__WFE();
		}
		//�����Ƶ
		v_FFT();
		max_mag = 0;
		base_index  = 0;
		for(i=1;i<NUM_SAMPLES/2;i++){
			if(max_mag < fl_gFFTOutput_mag[i]){
				max_mag = fl_gFFTOutput_mag[i];
				base_index = i;
			}
		}
		freq_base = base_index * facter;
		//��������Ƶ��
		//ADC_RATE = freq_base * 1.0/(1+1.0/64.0);		
		ADC_RATE = freq_base * 16;
		v_Set_ADC_Freq(ADC_RATE);
		//__BKPT(0);
		
		/*-------�����׶Ρ�������ʧ���----------------*/
		gCheckADC = false;
		v_DMA_Config();
		while (false == gCheckADC) {
			__WFE();
		}		
		v_FFT();
		thd = harmony_wave((float32_t *)fl_gFFTOutput_mag,NUM_SAMPLES,Xiebo,6);
		
		/*-------���Ľ׶Ρ�����ͼ----------------*/
		if(freq_base <= 20000){
			ADC_RATE = freq_base * NUM_SAMPLES/4;
		}
		else if((freq_base > 20000)&&(freq_base <=40000)){
			ADC_RATE = freq_base * NUM_SAMPLES/8;
		}else if((freq_base > 40000)&&(freq_base <=100000)){
			ADC_RATE = freq_base * NUM_SAMPLES/16;
		}else{
			ADC_RATE = 1600000;
		}
		v_Set_ADC_Freq(ADC_RATE);
		gCheckADC = false;
		v_DMA_Config();
		while (false == gCheckADC) {
			__WFE();
		}	
		
		max_mag = Xiebo[1];
		for(i =1;i<6;i++){
			Xiebo[i] = Xiebo[i]/max_mag;
		}
		v_OLED_display(thd,&Xiebo[1],(uint32_t *)gAdcResult,freq_base);		
    }
}

void v_Adapt_In_SingleTs(uint16_t *gADCResult){
	
}
void v_DMA_Config(void){
	/*reset*/
	SYSCFG_DL_DMA_init();
	DL_ADC12_stopConversion(ADC12_0_INST);
	NVIC_DisableIRQ(ADC12_0_INST_INT_IRQN);
	DL_DMA_disableChannel(DMA, DMA_CH0_CHAN_ID);
	
	
	DL_DMA_setSrcAddr(DMA, DMA_CH0_CHAN_ID,
        (uint32_t) DL_ADC12_getFIFOAddress(ADC12_0_INST));

    DL_DMA_setDestAddr(DMA, DMA_CH0_CHAN_ID, (uint32_t) &gAdcResult[0]);
    DL_DMA_setTransferSize(DMA, DMA_CH0_CHAN_ID, ADC_FIFO_SAMPLES);

    DL_DMA_enableChannel(DMA, DMA_CH0_CHAN_ID);

    /* Setup interrupts on device */
    NVIC_EnableIRQ(ADC12_0_INST_INT_IRQN);
	DL_ADC12_startConversion(ADC12_0_INST);
}

float fl_Get_Vpp(volatile uint16_t * data,uint16_t DATASIZE){
	uint16_t min,max;
	float result;
	uint16_t i;
	min = 0xffff;
	max = 0;
	for(i=0;i<DATASIZE;i++){
		if(min > data[i]) min = data[i];
		if(max < data[i]) max = data[i];
	}
	
	result = (max - min)*3.3/4096.0;
	
	return result;
}

void v_Set_ADC_Freq(float sample_freq){
	uint16_t sample_time;
	sample_time = (uint16_t)(Freq_SYS/sample_freq);
	
	if(sample_time < 2){
		sample_time = 2;
	}
	
	DL_ADC12_disableConversions(ADC12_0_INST);
	delay_us(10000);
	DL_ADC12_setSampleTime0(ADC12_0_INST,sample_time);
	DL_ADC12_setSampleTime1(ADC12_0_INST,0);
	delay_us(10000);
	DL_ADC12_enableConversions(ADC12_0_INST);
}
void ADC12_0_INST_IRQHandler(void)
{
    switch (DL_ADC12_getPendingInterrupt(ADC12_0_INST)) {
        case DL_ADC12_IIDX_DMA_DONE:
					gCheckADC = true;
					DL_ADC12_stopConversion(ADC12_0_INST);
					NVIC_DisableIRQ(ADC12_0_INST_INT_IRQN);


            break;
        default:
            break;
    }
}

void v_FFT(void)
{
		int i;
		// �������������������������Ч��
		// ������㲨����������� ��ʹ�ò�ֵ�ķ���ʹ�����յĲ���Ƶ�������Ƶ�ʳ��ֳ�����������Ч��
		//temp_period = adapt_signal((uint16_t*)gAdcResult,NUM_SAMPLES);	
		// ���������Ƶ������
		// ֮����������Ƶ�׷����� index ��������������ǵ�ʵ��Ƶ��

		for(i=0;i<NUM_SAMPLES;i++){
			fcom_gAdcResult[2*i+1] = (float)gAdcResult[i]; 
			fcom_gAdcResult[2*i] = 0.0; 
		}
	  if(temp_period==0)
		{
			facter = ((float)ADC_RATE)/ NUM_SAMPLES;
		}
		else
		{
			facter = ((float)ADC_RATE)/ temp_period;
		}
		
		// ���� cfft 
		// ע�����Ѽ�����һ���洢��һ����ַ��
		arm_cfft_f32(&arm_cfft_sR_f32_len256,(float32_t*)fcom_gAdcResult,IFFTFLAG,BITREVERSE);

		// ���� cfft ���������� 
		arm_cmplx_mag_f32((float32_t*)fcom_gAdcResult,(float32_t*)fl_gFFTOutput_mag,NUM_SAMPLES);
}

//�δ�ʱ�����жϷ�����
void SysTick_Handler(void)
{
}

//�ı�DAC�������VAC810���棬��ʽΪ���鹫ʽ
void v_Gain_Change(float gain){
	uint16_t Gain_Data;
	Gain_Data = (uint16_t)(292.2 * logf(gain) + 1247.7);
	DL_DAC12_output12(DAC0,Gain_Data);
	delay_us(500);
	DL_DAC12_enable(DAC0);
	delay_us(500);
}

void v_OLED_display(float thd,float *harmo,uint32_t *getTime,float freq_base){
	int i;
	uint16_t num;
	
	if(freq_base <= 20000){
		num = NUM_SAMPLES/4;
	}
	else if((freq_base > 20000)&&(freq_base <=40000)){
		num = NUM_SAMPLES/8;
	}else if((freq_base > 40000)&&(freq_base <=100000)){
		num = NUM_SAMPLES/16;
	}else{
		num = NUM_SAMPLES/16;
	}
		
	OLED_Clear();				
	OLED_PlotWave(getTime, num);
	OLED_Refresh();
	delay_ms(1000);
	
	OLED_Clear();				
	OLED_ShowString(0,0,(uint8_t *)"harmony",8,1);
	for(i=0;i<5;i++)
	{
		OLED_ShowNum(0,(i+1)*8,i+1,1,8,1);
		OLED_ShowString(10,(i+1)*8,(uint8_t *)"---",8,1);
		OLED_ShowFloat(35,(i+1)*8,harmo[i],6,8,1);
	  }
		
	OLED_ShowString(0,6*8,(uint8_t *)"thd",8,1);
	OLED_ShowFloat(7,7*8,thd,6,8,1);
		  
	delay_ms(500);
	OLED_Refresh();
	delay_ms(3000);
}
