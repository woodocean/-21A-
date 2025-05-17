#include <arm_math.h>

#define MAX_NUM_PERIOD 	32
#define NUMSAMPLES		256


void arm_cmplx_pha_q15(
  const q15_t * pSrc,
        float * pDst,
        uint32_t numSamples);
				
// ������������������ڼ������������ʹ�õ�
uint32_t list_period(uint16_t * pSrc, uint32_t numSamples);
// ��������������ǽ�ԭ�źŽ���У����
// ע�������������������Ч
uint32_t adapt_signal(uint16_t * pSrc, uint32_t numSamples);
				
				
				
uint32_t triger(uint16_t *pSrc, uint32_t numSamples, uint32_t triger_point, uint32_t * diff_point, char rise);
				

float harmony_wave(float32_t* fl_gFFTOutput_mag, unsigned int num, float * harmony, unsigned int harmony_num);