#include "My_USART.h"

//���룺int�������֣�char[]�ַ����׵�ַ
//���ܣ�������תΪ�ַ���
void itoa(int num,char str[] )
	{
	int sign = num,i = 0,j = 0;
	char temp[11];
		
	if(sign == 0){
		str[j] = '0';
		str[j+1] = '\0';
	}
	else{
		if(sign<0){
		num = -num;
	}
	do
	{
	temp[i] = num%10+'0';       
	num/=10;
	i++;
	}while(num>0);
	if(sign<0)
	{
	temp[i++] = '-';
	}
	temp[i] = '\0';
	i--;
	while(i>=0)
	{
	str[j] = temp[i];
	j++;
	i--;
	}
	str[j] = '\0';
	}		
}

//���룺����λ��
//���ܣ�ÿ���10ms�ڴ������м���λ��pos_y�ĵ㣬Ч�����ǲ���������ƽ��
//˵������Ҫ���������봮��1�������Լ���϶�ʱ��tic��10ms��ʱ
void v_Draw_Usart_addpoint(int pos_y){

		char str1[10];
		itoa(pos_y,str1);
		uart1_send_string("add s0.id,0,");
		uart1_send_string(str1);
		uart1_send_string("\xff\xff\xff");
		delay_ms(10);
}

//���룺����λ�������׵�ַ�����������߿�ȣ������С��
//���ܣ�
void v_Draw_Picture(int pos_y[],int WIDTH,int POINTS){
	int i,j;
	char str1[10];
	char gap = WIDTH / POINTS - 1;
	
	itoa(WIDTH,str1);
	uart1_send_string("addt s0.id,0,479");
	//uart1_send_string(str1);
	uart1_send_string("\xff\xff\xff");
	
	delay_ms(100);
	
	for(i=0;i<POINTS+3;i++){
		uart1_send_char((char)(pos_y[i]));
		for(j=0;j<gap;j++){
			uart1_send_char(0x00);
		}
	}
	
	 //ȷ��͸������������Ӱ����һ��ָ��
	uart1_send_string("\x01\xff\xff\xff");
}

