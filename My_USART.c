#include "My_USART.h"

//输入：int输入数字，char[]字符串首地址
//功能：将数字转为字符串
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

//输入：纵轴位置
//功能：每间隔10ms在串口屏中加入位置pos_y的点，效果就是波形逐渐向右平移
//说明：需要将串口屏与串口1相连，以及结合定时器tic的10ms延时
void v_Draw_Usart_addpoint(int pos_y){

		char str1[10];
		itoa(pos_y,str1);
		uart1_send_string("add s0.id,0,");
		uart1_send_string(str1);
		uart1_send_string("\xff\xff\xff");
		delay_ms(10);
}

//输入：纵轴位置数组首地址，串口屏曲线宽度（数组大小）
//功能：
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
	
	 //确保透传结束，以免影响下一条指令
	uart1_send_string("\x01\xff\xff\xff");
}

