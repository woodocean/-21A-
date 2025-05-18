#ifndef __OLED_H
#define __OLED_H 

#include <ti/devices/msp/msp.h>
#include <ti/driverlib/driverlib.h>
#include <ti/driverlib/m0p/dl_core.h>

#ifndef u8
#define u8 uint8_t
#endif

#ifndef u16
#define u16 uint16_t
#endif

#ifndef u32
#define u32 uint32_t
#endif


/* Port definition for Pin Group GPIO */
#define GPIO_PORT                                                        (GPIOA)

/* Defines for SCL: GPIOA.12 with pinCMx 34 on package pin 5 */
#define GPIO_SCL_PIN                                            (DL_GPIO_PIN_12)
#define GPIO_SCL_IOMUX                                           (IOMUX_PINCM34)
/* Defines for SDA: GPIOA.14 with pinCMx 36 on package pin 7 */
#define GPIO_SDA_PIN                                            (DL_GPIO_PIN_14)
#define GPIO_SDA_IOMUX                                           (IOMUX_PINCM36)
/* Defines for RES: GPIOA.21 with pinCMx 46 on package pin 17 */
#define GPIO_RES_PIN                                            (DL_GPIO_PIN_21)
#define GPIO_RES_IOMUX                                           (IOMUX_PINCM46)
/* Defines for DC: GPIOA.22 with pinCMx 47 on package pin 18 */
#define GPIO_DC_PIN                                             (DL_GPIO_PIN_22)
#define GPIO_DC_IOMUX                                            (IOMUX_PINCM47)
/* Defines for CS: GPIOA.2 with pinCMx 7 on package pin 42 */
#define GPIO_CS_PIN                                              (DL_GPIO_PIN_2)
#define GPIO_CS_IOMUX                                             (IOMUX_PINCM7)

//-----------------OLED端口定义---------------- 

#define OLED_SCL_Clr() DL_GPIO_clearPins(GPIO_PORT,GPIO_SCL_PIN)//SCL=SCLK
#define OLED_SCL_Set() DL_GPIO_setPins(GPIO_PORT,GPIO_SCL_PIN)

#define OLED_SDA_Clr() DL_GPIO_clearPins(GPIO_PORT,GPIO_SDA_PIN)//SDA=MOSI
#define OLED_SDA_Set() DL_GPIO_setPins(GPIO_PORT,GPIO_SDA_PIN)

#define OLED_RES_Clr()  DL_GPIO_clearPins(GPIO_PORT,GPIO_RES_PIN)//RES
#define OLED_RES_Set()  DL_GPIO_setPins(GPIO_PORT,GPIO_RES_PIN)

#define OLED_DC_Clr()   DL_GPIO_clearPins(GPIO_PORT,GPIO_DC_PIN)//DC
#define OLED_DC_Set()   DL_GPIO_setPins(GPIO_PORT,GPIO_DC_PIN)

#define OLED_CS_Clr()   DL_GPIO_clearPins(GPIO_PORT,GPIO_CS_PIN)//CS
#define OLED_CS_Set()   DL_GPIO_setPins(GPIO_PORT,GPIO_CS_PIN)


#define OLED_CMD  0	//写命令
#define OLED_DATA 1	//写数据


// OLED 的基础控制方法
void OLED_WR_Byte(u8 dat,u8 mode);

// OLED 的硬件初始化
void OLED_Init(void);            // 初始化 OLED 的显示功能
void OLED_DisPlay_On(void);      // 开启 OLED 的供电
void OLED_DisPlay_Off(void);     // 关闭 OLED 的供电

// ------------------------------------------------------------------------------------------------------- //
//                        注意绘图都是在画布上操作 需要进行 refresh 操作写入 OLED 中                          //
// ------------------------------------------------------------------------------------------------------- //
static u8 OLED_GRAM[144][8];
void OLED_Refresh(void);


// OLED 的字符显示
void OLED_ShowChar(u8 x,u8 y,u8 chr,u8 size1,u8 mode);
void OLED_ShowChar6x8(u8 x,u8 y,u8 chr,u8 mode);
void OLED_ShowString(u8 x,u8 y,u8 *chr,u8 size1,u8 mode);
void OLED_ShowNum(u8 x,u8 y,u32 num,u8 len,u8 size1,u8 mode);
void OLED_ShowChinese(u8 x,u8 y,u8 num,u8 size1,u8 mode);
void OLED_ScrollDisplay(u8 num,u8 space,u8 mode);              // 滚动播出汉字字符
void OLED_ShowFloat(u8 x,u8 y,float num,u8 len,u8 size1,u8 mode);

// OLED 专属使用的延时函数
void OLED_Delay(u16 time);

// OLED 的绘图功能   ----------  
void OLED_DrawPoint(u8 x,u8 y,u8 t);
void OLED_DrawLine(u8 x1,u8 y1,u8 x2,u8 y2,u8 mode);
void OLED_DrawCircle(u8 x,u8 y,u8 r);
void OLED_ShowPicture(u8 x,u8 y,u8 sizex,u8 sizey,u8 BMP[],u8 mode);
static u8 WaveBuf[128];
void OLED_PlotWave(u32 waveBuf[], u16 SampleNum);             // 内置转换函数 使得最终显示的波形完全占据整个屏幕

// OLED 的控制显示
void OLED_ColorTurn(u8 i);       // 反转显示的颜色 黑变白 白变黑
void OLED_DisplayTurn(u8 i);     // 180度颠倒显示
void OLED_Clear(void);           // OLED 的清屏操作


#endif

