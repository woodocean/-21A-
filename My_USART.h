#ifndef	__MY_USART_H__
#define __MY_USART_H__

#include "board.h"

//曲线控件宽度、高度
#define Curve_WIDTH	479
#define Curve_HIGHT	201
#define BAUD	38400	//波特率
void itoa(int num,char str[] );
void v_Draw_Usart_addpoint(int pos_y);
void v_Draw_Picture(int pos_y[],int WIDTH,int POINTS);
#endif
