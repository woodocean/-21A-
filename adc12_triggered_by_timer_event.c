#include "ti_msp_dl_config.h"
#include "board.h"
#include <stdio.h>
#include "my_arm_math.h"
#include "arm_const_structs.h"
#include "arm_math.h"
#include "oled.h"


/*-------------------------宏定义----------------------*/
#define NUM_SAMPLES  		256					//FFT点数
#define NUM_ALL      		264					//总采样点数（FIFO整数倍，避免由于前一次采样的剩余数据使得DMA传输结果分段跳变）
#define ADC_FIFO_SAMPLES 	(NUM_ALL >> 1)		//FIFO传输点数，ADC采样结果16位，每两个合成一个word(32位)
#define EXPECT_Vpp  		 0.8				//期望Vpp
#define Freq_SYS 			32000000.0			//系统时钟频率，ADC分频基于此频率
/*---fft--*/
#define IFFTFLAG 			0					// ifftFlag 0--正向变换   1--反向变换
#define BITREVERSE 			1					// bitreverseflag 1--启用位翻转   0--不进行位翻转

/*--------------------全局变量---------------------------------------------*/
/*ADC*/
volatile bool 		gCheckADC;							//ADC采样标志位
volatile uint16_t 	gAdcResult[NUM_ALL];				//ADC采样数组
/**预处理**/
uint32_t 			temp_period=0;
volatile uint16_t 	adc_sample_time;
float 				facter;								//对输入数组插值预处理，对应的频率放缩因子
														//计算出来的频谱分量的 index 乘以这个就是它们的实际频率
/*FFT*/											
float 				ADC_RATE; 							// 样本的采样速率 
volatile float32_t 	fcom_gAdcResult[NUM_SAMPLES*2];		//FFT输入输出数组
volatile float32_t 	fl_gFFTOutput_mag[NUM_SAMPLES];		//FFT输出频谱
float 				Xiebo[6];							//直流分量和前五次谐波
float 				thd;								//失真度
/*VCA810*/
float 				Vpp;								//输入信号峰峰值
float 				gain;								//vca810增益
/*测量基频*/
float 				freq_base;							//信号基频
int 				base_index;
float 				max_mag;
/*绘图*/

/*debug*/
uint16_t adc_num;

/*--------函数声明------------------*/
float fl_Get_Vpp(volatile uint16_t * data,uint16_t DATASIZE);
void v_Gain_Change(float gain);
void v_DMA_Config(void);
void v_Set_ADC_Freq(float sample_freq);
void v_FFT(void);							//对输入数组插值预处理后，再做fft运算得到频谱
void v_OLED_display(float thd,float *harmo,uint32_t *getTime,float freq_base);
int main(void)
{
	
	int i;
	SYSCFG_DL_init();
	OLED_Init();    //初始化OLED
    OLED_Clear();
	
    while (1) {
		
		/*-------第一阶段——调整被采样信号幅度-----------*/
		ADC_RATE = 256000.0;		//初始采样频率，用于确认基频
		gain = 1.0;					//初始增益
		v_Gain_Change(gain);		
		delay_us(10000);

		//ADC采样
		v_Set_ADC_Freq(ADC_RATE);
		gCheckADC = false;
		v_DMA_Config();
		while (false == gCheckADC) {
			__WFE();
		}
		
		//当采样数组满了，进入增益调节环节
		Vpp = fl_Get_Vpp(gAdcResult,NUM_SAMPLES);
		gain = gain * EXPECT_Vpp / (Vpp );
		v_Gain_Change(gain);	
		delay_us(1000);
		
		/*-------第二阶段——调整采样频率----------------*/
		//再采样一次
		gCheckADC = false;
		v_DMA_Config();
		while (false == gCheckADC) {
			__WFE();
		}
		//计算基频
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
		//调整采样频率
		//ADC_RATE = freq_base * 1.0/(1+1.0/64.0);		
		ADC_RATE = freq_base * 16;
		v_Set_ADC_Freq(ADC_RATE);
		//__BKPT(0);
		
		/*-------第三阶段——计算失真度----------------*/
		gCheckADC = false;
		v_DMA_Config();
		while (false == gCheckADC) {
			__WFE();
		}		
		v_FFT();
		thd = harmony_wave((float32_t *)fl_gFFTOutput_mag,NUM_SAMPLES,Xiebo,6);
		
		/*-------第四阶段——绘图----------------*/
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
		// 下面这个函数仅对正数数组有效果
		// 它会计算波形数组的周期 并使用插值的方法使得最终的波形频率与采样频率呈现出整数倍数的效果
		//temp_period = adapt_signal((uint16_t*)gAdcResult,NUM_SAMPLES);	
		// 下面这个是频率因子
		// 之后计算出来的频谱分量的 index 乘以这个就是它们的实际频率

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
		
		// 计算 cfft 
		// 注意它把计算结果一样存储到一个地址下
		arm_cfft_f32(&arm_cfft_sR_f32_len256,(float32_t*)fcom_gAdcResult,IFFTFLAG,BITREVERSE);

		// 根据 cfft 结果计算幅度 
		arm_cmplx_mag_f32((float32_t*)fcom_gAdcResult,(float32_t*)fl_gFFTOutput_mag,NUM_SAMPLES);
}

//滴答定时器的中断服务函数
void SysTick_Handler(void)
{
}

//改变DAC输出控制VAC810增益，公式为经验公式
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
