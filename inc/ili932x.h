#ifndef __ILI932X_H
#define __ILI932X_H

#include "stm32f10x.h"  
#include "tff.h"

/* 选择使用的数据通讯位数：16位、8位、3线-SPI、4线-SPI  */
//#define LCD_16B_IO
#define LCD_8B_IO
// #define LCD_3_SPI
// #define LCD_4_SPI

/*====================================================*/

//定义驱动IC的型号
//#define  ILI9325 
//#define  ILI9320 
//#define  LGDP4531 
//#define  ILI9328 
//#define  ILI9331 
#define  SSD1298
//#define  SSD2119 
//#define  ST7781 

//屏幕旋转定义 数字按照 ID[1:0]AM 按照PDF中的配置定义
#define ID_AM  000   		    //竖向从上到下
//#define ID_AM  001		   //横向从右到左
//#define ID_AM  010	       //竖向从上到下
//#define ID_AM  011		   //横向从右到左
//#define ID_AM  100		   //竖向从上到下
//#define ID_AM  101		   //横向从右到左
//#define ID_AM  110		   //竖向从上到下
//#define ID_AM  111

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* LCD Registers */
#define R0             0x00
#define R1             0x01
#define R2             0x02
#define R3             0x03
#define R4             0x04
#define R5             0x05
#define R6             0x06
#define R7             0x07
#define R8             0x08
#define R9             0x09
#define R10            0x0A
#define R12            0x0C
#define R13            0x0D
#define R14            0x0E
#define R15            0x0F
#define R16            0x10
#define R17            0x11
#define R18            0x12
#define R19            0x13
#define R20            0x14
#define R21            0x15
#define R22            0x16
#define R23            0x17
#define R24            0x18
#define R25            0x19
#define R26            0x1A
#define R27            0x1B
#define R28            0x1C
#define R29            0x1D
#define R30            0x1E
#define R31            0x1F
#define R32            0x20
#define R33            0x21
#define R34            0x22
#define R36            0x24
#define R37            0x25
#define R40            0x28
#define R41            0x29
#define R43            0x2B
#define R45            0x2D
#define R48            0x30
#define R49            0x31
#define R50            0x32
#define R51            0x33
#define R52            0x34
#define R53            0x35
#define R54            0x36
#define R55            0x37
#define R56            0x38
#define R57            0x39
#define R59            0x3B
#define R60            0x3C
#define R61            0x3D
#define R62            0x3E
#define R63            0x3F
#define R64            0x40
#define R65            0x41
#define R66            0x42
#define R67            0x43
#define R68            0x44
#define R69            0x45
#define R70            0x46
#define R71            0x47
#define R72            0x48
#define R73            0x49
#define R74            0x4A
#define R75            0x4B
#define R76            0x4C
#define R77            0x4D
#define R78            0x4E
#define R79            0x4F
#define R80            0x50
#define R81            0x51
#define R82            0x52
#define R83            0x53
#define R96            0x60
#define R97            0x61
#define R106           0x6A
#define R118           0x76
#define R128           0x80
#define R129           0x81
#define R130           0x82
#define R131           0x83
#define R132           0x84
#define R133           0x85
#define R134           0x86
#define R135           0x87
#define R136           0x88
#define R137           0x89
#define R139           0x8B
#define R140           0x8C
#define R141           0x8D
#define R143           0x8F
#define R144           0x90
#define R145           0x91
#define R146           0x92
#define R147           0x93
#define R148           0x94
#define R149           0x95
#define R150           0x96
#define R151           0x97
#define R152           0x98
#define R153           0x99
#define R154           0x9A
#define R157           0x9D
#define R192           0xC0
#define R193           0xC1
#define R229           0xE5

/* LCD color */
#define White          0xFFFF
#define Black          0x0000
#define Grey           0xF7DE
#define Blue           0x001F
#define Blue2          0x051F
#define Red            0xF800
#define Magenta        0xF81F
#define Green          0x07E0
#define Cyan           0x7FFF
#define Yellow         0xFFE0

#define Line0          0
#define Line1          24
#define Line2          48
#define Line3          72
#define Line4          96
#define Line5          120
#define Line6          144
#define Line7          168
#define Line8          192
#define Line9          216

#define Horizontal     0x00
#define Vertical       0x01

 
#define Set_LE  GPIO_SetBits(GPIOB,GPIO_Pin_7)
#define Clr_LE  GPIO_ResetBits(GPIOB,GPIO_Pin_7)

#define nCsPin  GPIO_Pin_14   //PB14
#define RsPin   GPIO_Pin_15   //PB15
#define nWrPin  GPIO_Pin_10	  //PD10
#define nRdPin  GPIO_Pin_13	  //PB13
//	#define nRstPin GPIO_Pin_12	  //NONE
#define Lcd_LightPin GPIO_Pin_14 //PD14
/*
#define Set_nWr (*((volatile unsigned long *) 0x40011010) = nWrPin)
#define Clr_nWr (*((volatile unsigned long *) 0x40011014) = nWrPin)

#define Set_Cs  (*((volatile unsigned long *) 0x40011010) = nCsPin)
#define Clr_Cs  (*((volatile unsigned long *) 0x40011014) = nCsPin)

#define Set_Rs  (*((volatile unsigned long *) 0x40011010) = RsPin)
#define Clr_Rs  (*((volatile unsigned long *) 0x40011014) = RsPin)

#define Set_nRd (*((volatile unsigned long *) 0x40011010) = nRdPin)
#define Clr_nRd (*((volatile unsigned long *) 0x40011014) = nRdPin)

#define Set_Rst (*((volatile unsigned long *) 0x40011010) = nRstPin)
#define Clr_Rst (*((volatile unsigned long *) 0x40011014) = nRstPin)

#define Lcd_Light_ON   (*((volatile unsigned long *) 0x40011010) = Lcd_LightPin)
#define Lcd_Light_OFF  (*((volatile unsigned long *) 0x40011014) = Lcd_LightPin)
*/

#define Set_Cs  GPIO_SetBits(GPIOB,GPIO_Pin_14)
#define Clr_Cs  GPIO_ResetBits(GPIOB,GPIO_Pin_14)

#define Set_Rs  GPIO_SetBits(GPIOB,GPIO_Pin_15)
#define Clr_Rs  GPIO_ResetBits(GPIOB,GPIO_Pin_15)

#define Set_nWr GPIO_SetBits(GPIOD,GPIO_Pin_10)
#define Clr_nWr GPIO_ResetBits(GPIOD,GPIO_Pin_10)

#define Set_nRd GPIO_SetBits(GPIOB,GPIO_Pin_13)
#define Clr_nRd GPIO_ResetBits(GPIOB,GPIO_Pin_13)

// #define Set_Rst GPIO_SetBits(GPIOC,GPIO_Pin_12)
// #define Clr_Rst GPIO_ResetBits(GPIOC,GPIO_Pin_12)

#define Lcd_Light_ON   GPIO_SetBits(GPIOD,GPIO_Pin_14)
#define Lcd_Light_OFF  GPIO_ResetBits(GPIOD,GPIO_Pin_14)



//==================设置字库文件格式============================
#define LINE_NUM 320/24
#define Col_NUM  240/24
#define BufferSize 72	//设置为一个汉字所需要占用的字节数，和字库有关系
						//还记得设置GetChineseCode()函数。
#define HzkBaseAddr 0xb0a1
// #define HzkBaseAddr 0xa1a1  //该参数给GetChineseCode()函数使用


extern FATFS fs;
extern FIL fil;
extern FRESULT res;		//文件系统返回信息

//===============================================================
#define  LCD_WriteRAM_Prepare           Lcd_WR_Start
#define  LCD_WriteRAM 			        DataToWrite
#define  LCD_WriteReg 			        LCD_WR_REG

// #define ili9320_WriteRegister   LCD_WR_REG

typedef union
{
  u16 U16;
  u8 U8[2];
}ColorTypeDef;


/*----- High layer function -----*/
void LCD_Init(void);
void LCD_SetTextColor(vu16 Color);
void LCD_SetBackColor(vu16 Color);
void LCD_ClearLine(u8 Line);
void LCD_Clear(u16 Color);
void LCD_SetCursor(u16 Xpos, u16 Ypos);
void LCD_DrawChar(u8 Xpos, u16 Ypos, uc16 *c);
void LCD_DisplayChar(u8 Line, u16 Column, u8 Ascii);
void LCD_DisplayStringLine(u8 Line, u8 *ptr);
void LCD_SetDisplayWindow(u8 Xpos, u16 Ypos, u8 Height, u16 Width);
void LCD_WindowModeDisable(void);
void LCD_DrawLine(u8 Xpos, u16 Ypos, u16 Length, u8 Direction);
void LCD_DrawRect(u8 Xpos, u16 Ypos, u8 Height, u16 Width);
void LCD_DrawCircle(u8 Xpos, u16 Ypos, u16 Radius);
void LCD_DrawMonoPict(uc32 *Pict);
void LCD_WriteBMP(u32 BmpAddress);

//Lcd初始化及其低级控制函数
void Lcd_Configuration(void);
void Lcd_Initialize(void);
void DataToWrite(u16);
void LCD_WR_REG(u16, u16);
void Lcd_WR_Start(void);
void ili9320_WriteIndex(u16);
void ili9320_WriteData(u16);
u16 ili9320_ReadData(void);
u16 ili9320_ReadRegister(u16);
void ili9320_WriteRegister(u16, u16);
u16 ili9320_BGR2RGB(u16);


// 测试函数
void ili9320_Test(void);
void Test_Color(void);



//Lcd高级控制函数
void Lcd_SetCursor(u16 x,u16 y);
void Lcd_Clear(u16 Color);
void Lcd_ClearCharBox(u8 x,u16 y,u16 Color);
void Lcd_SetBox(u8 xStart,u16 yStart,u8 xLong,u16 yLong,u16 x_offset,u16 y_offset);
void Lcd_ColorBox(u8 x,u16 y,u8 xLong,u16 yLong,u16 Color);
void Lcd_WriteASCII(u8 x,u8 y,u16 x_offset,u16 y_offset,u16 CharColor,u16 CharBackColor,u8 ASCIICode);
void Lcd_WriteASCIIClarity(u8 x,u8 y,u8 x_offset,u8 y_offset,u16 CharColor,u8 ASCIICode);
void Lcd_Write32X16ASCII(u8 x,u8 y,u8 x_offset,u8 y_offset,u16 CharColor,u16 CharBackColor,u8 ASCIICode);
void Lcd_Write32X16ASCIIClarity(u8 x,u8 y,u8 x_offset,u8 y_offset,u16 CharColoru8,u8 ASCIICode);
void Lcd_Write32X16ASCIIWrite(u8 x,u8 y,u8 x_offset,u8 y_offset,u16 CharColor,u16 CharBackColor,u8 ASCIICode);
void Lcd_WriteString(u8 x,u8 y,u16 x_offset,u16 y_offset,u16 CharColor,u16 CharBackColor,char *s);
void Lcd_Write32X16String(u8 x,u8 y,u8 x_offset,u8 y_offset,u16 CharColor,u16 CharBackColor,char *s);
void Lcd_WriteStringClarity(u8 x,u8 y,u8 x_offset,u8 y_offset,u16 CharColor,char *s);
void Lcd_Write32X16StringClarity(u8 x,u8 y,u8 x_offset,u8 y_offset,u16 CharColor,char *s);

void Lcd_WriteChinese(u8 x,u8 y,u16 x_offset,u16 y_offset,u16 CharColor,u16 CharBackColor,u16 ChineseCode);
void Lcd_WriteChineseClarity(u8 x,u8 y,u8 x_offset,u8 y_offset,u16 CharColor,u16 ChineseCode);
void Lcd_Write32X32Chinese(u8 x,u8 y,u8 x_offset,u8 y_offset,u16 CharColor,u16 CharBackColor,u16 ChineseCode);
void Lcd_Write32X32ChineseClarity(u8 x,u8 y,u8 x_offset,u8 y_offset,u16 CharColor,u16 ChineseCode);
void Lcd_WriteChineseString(u8 x,u8 y,u8 x_offset,u8 y_offset,u16 CharColor,u16 CharBackColor,char *s);
void Lcd_WriteChineseStringClarity(u8 x,u8 y,u8 x_offset,u8 y_offset,u16 CharColor,char *s);
void Lcd_Write32X32ChineseString(u8 x,u8 y,u8 x_offset,u8 y_offset,u16 CharColor,u16 CharBackColor,char *s);
void Lcd_Write32X32ChineseStringClarity(u8 x,u8 y,u8 x_offset,u8 y_offset,u16 CharColor,char *s);
void LcdWritePictureFromSPI(u8 xStart,u16 yStart,u8 xLong,u16 yLong,u32 BaseAddr);

void GetASCIICode(u8* pBuffer,u8 ASCII,u32 BaseAddr);
void GetChineseCode(u8* pBuffer,u16 ChineseCode,u32 BaseAddr);
void GetSpiChineseCode(u8* pBuffer,u16 ChineseCode,u32 BaseAddr);
void Get320240PictureCode(u8* pBuffer,u32 BufferCounter,u32 BaseAddr);

void Delay_nms(vu32);
void ili9320_Delay(vu32);


/*定义常见颜色*/
#define red 0x001f
#define blue 0xf800
#define green 0x07e0
#define black 0x0000
#define white 0xffff
#define yellow 0x07ff
#define orange 0x05bf
#define Chocolate4 0x4451
#define Grey 0xefbd    //灰色


//定义FLASH中的数据首地址
#define logo 0x51000
#define key24048 0x4a000
#define key8048 0x48000
#define key2480 0x77000
#define key2448 0x78000
#define BatteryHight 0x79000
#define BatteryMiddle 0x7a000
#define BatteryLow 0x7b000
#define BatteryTooLow 0x7c000
#define key24032 0x80000 
#define PowOffkey24048 0x84000

#define ASCII_Offset 0x47000
#define Chinese_Offset 0x0000
#define Chinese_Offset_24_DOT 0x34e00


#endif

