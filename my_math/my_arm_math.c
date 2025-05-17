#include "my_arm_math.h"
#include "math.h"

/*��������*/
volatile uint32_t rise_1[NUMSAMPLES/MAX_NUM_PERIOD + 1];	
volatile uint32_t rise_2[NUMSAMPLES/MAX_NUM_PERIOD + 1];
volatile uint32_t period1[NUMSAMPLES/MAX_NUM_PERIOD + 1];
volatile float temp[NUMSAMPLES];

void arm_cmplx_pha_q15(
  const q15_t * pSrc,
        float * pDst,
        uint32_t numSamples)
{
    uint32_t i;
    for(i=0;i<numSamples;i++)
    {
        pDst[i] =  atan2f(pSrc[2*i+1],pSrc[2*i]);
    }
}    


// ����˵��
// pSrc �Ǵ�������ʱ������
// numSamples �Ǵ������������С
// triger_point �Ǵ����Ĵ�С
// diff_point �Ǵ洢��ͬ����֮��ʱ���ϵĲ�ֵ��С
// rise �Ǵ����������ش���('T')  �����½��ش���(else)
// ���յķ���ֵ�������� diff_point ���ȥ����Ч�����ָ���
uint32_t triger(uint16_t *pSrc, uint32_t numSamples, uint32_t triger_point, uint32_t * diff_point, char rise)
{

	uint32_t i;
    uint32_t j;
	j = 0;

    for(i=0; i<numSamples -1;i++)
    {
        if(rise=='T')
        {
            if((pSrc[i] <= triger_point)&&(pSrc[i+1] > triger_point))
            {
                diff_point[j] = i;
                j++;
            }
        }
        else
        {
            if((pSrc[i] >= triger_point)&&(pSrc[i+1] < triger_point))
            {
                diff_point[j] = i;
                j++;
            }
        }
    }
    for(i=j-1;i>0;i--)
    {
        diff_point[i] = diff_point[i]-diff_point[0];
    }
    return j;
}


// ��������������� Ҫ�� numSamples �ķ�Χ������Ҫ��һ������������
// ���Ҵ����������� pSrc Ӧ��ȫ���������� �����и���
uint32_t list_period(uint16_t * pSrc, uint32_t numSamples)
{
    uint32_t max,min;
    uint32_t i, j;
    max = pSrc[0];
    min = pSrc[0];
    for(i=0;i<numSamples;i++)
    {
        if(pSrc[i] > max)
            max = pSrc[i];
        if(pSrc[i] < min)
            min = pSrc[i];
    }
    
    
    uint32_t triger1;
    uint32_t triger2;
    triger1 = (max*2 +min)/3;
    triger2 = (max+ min*2)/3;

    
    /*
    uint32_t down_1[triger_num];
    uint32_t down_2[triger_num];
    */
    
    uint32_t rise_num1, rise_num2;
    // uint32_t down_num1, down_num2;
    
    rise_num1 = triger(pSrc, numSamples, triger1, (uint32_t *)rise_1, 'T');
    rise_num2 = triger(pSrc, numSamples, triger2, (uint32_t *)rise_2, 'T');
    /*
    down_num1 = triger(pSrc, numSamples, triger1, down_1, 'F');
    down_num2 = triger(pSrc, numSamples, triger2, down_2, 'F');
      __BKPT(0);
    */
    uint32_t p_num_1 = 0;
    for(i=1; i<rise_num1;i++)
    {
        for(j=1; j<rise_num2; j++)
        {
            if((rise_1[i] <= rise_2[j] +1) && (rise_1[i] >= rise_2[j] -1))
            {
				//__BKPT(0);
                period1[p_num_1] = rise_1[i];
                p_num_1++;
            }
        }
    }
    
    /*
    uint32_t period2[triger_num];
    uint32_t p_num_2 = 0;
    for(i=0; i<down_num1;i++)
    {
        for(j=0; j<down_num2; j++)
        {
            if((down_1[i] <= down_2[j] +1) && (down_1[i] >= down_2[j] -1))
            {
                period2[p_num_2] = down_1[i];
                p_num_2++;
            }
        }
    }
      __BKPT(0);
    */
    
    // uint32_t period[triger_num];
    // uint32_t k = 0;
    // for(i=0; i<p_num_1;i++)
    // {
    //     for(j=0; j<p_num_2; j++)
    //     {
    //         if((period1[i] <= period2[j] +1) && (period1[i] >= period2[j] -1))
    //         {
    //             period[k] = period1[i];
    //             k++;
    //         }
    //     }
    // }
    //  __BKPT(0);
    
    if(p_num_1>=1)
        return period1[p_num_1-1];
    return 0;
}


// ����������� ����У������������������
// ����֮���Ƶ�׷���
// �������ֵ�� 0 ˵������������д���û���޸�ֵ
uint32_t adapt_signal(uint16_t * pSrc, uint32_t numSamples)
{
    uint32_t period;
    period = list_period(pSrc, (uint32_t)numSamples);
    //period = 216;
    if(period==0)
        return 0;
    if(period>numSamples)
        return 0;
	//__BKPT(0);
  int i;
    float loc;
    uint32_t loc_int;
    uint32_t loc_yu;
	//__BKPT(0);
    for(i=0;i<numSamples;i++)
    {
        loc = ((float)i)*period/numSamples;
        loc_int = (int)loc;
        loc_yu  = (i*period)%numSamples;
        // ��������ײ�ֵ�����Ż� ���Ǻ��� M0 оƬ����������֧��
        //temp[i] = ((float)pSrc[loc_int-1]) * loc_yu /numSamples * (loc_yu - numSamples)/2/numSamples;
      //temp[i] +=((float)pSrc[loc_int]) * (loc_yu +numSamples)/numSamples*(numSamples- loc_yu)/numSamples;
        //temp[i] +=((float)pSrc[loc_int+1]) *(loc_yu+numSamples)/numSamples*loc_yu/2/numSamples;
        temp[i] = ((float)pSrc[loc_int]) * (numSamples-loc_yu)/numSamples;
    temp[i]+= ((float)pSrc[loc_int+1]) * loc_yu/numSamples;
    }
	//__BKPT(0);
    for(i=1;i<numSamples;i++)
    {
        pSrc[i] = (unsigned int)temp[i];
    }
    return period;
}


// ����������� ����������ĳ����Ƶ��г������
// ����������źŵ� THD ֵ
// harmony �д洢��ȥ�ĵ�0��Ԫ����ֱ�������ķ���
// ��1��Ԫ���ǻ���������
// harmony_num ������ harmony ���ж��ٸ�Ԫ��
float harmony_wave(float32_t* fl_gFFTOutput_mag, unsigned int num, float * harmony, unsigned int harmony_num)
{
	int i;
	int index;
	int base_index = 0;
	float temp = 0;
	float thd=0;
	
		for(i=1;i<NUMSAMPLES/2;i++){
			if(temp < fl_gFFTOutput_mag[i]){
				temp = fl_gFFTOutput_mag[i];
				base_index = i;
			}
		}
		for(i=0;i<harmony_num;i++)
		{
			index = (int)(base_index*i);
			if(index == 0){
				harmony[i] = fl_gFFTOutput_mag[index];
			} else{
				harmony[i] = fl_gFFTOutput_mag[index] * fl_gFFTOutput_mag[index] + fl_gFFTOutput_mag[index -1]*fl_gFFTOutput_mag[index-1]+fl_gFFTOutput_mag[index+1]*fl_gFFTOutput_mag[index+1];
				harmony[i] = sqrt(harmony[i]/3);
			}
			
		}

		for(i=2;i<harmony_num;i++)
		{
			thd += harmony[i]*harmony[i];
		}
		
		thd = sqrt(thd)/(harmony[1]);

		return thd;
}		