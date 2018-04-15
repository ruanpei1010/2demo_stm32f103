/******************** (C) COPYRIGHT 2008 STMicroelectronics ********************
* File Name          : MC_Keys.c
* Author             : IMS Systems Lab 
* Date First Issued  : 21/11/07
* Description        : This file handles Joystick and button management
********************************************************************************
* History:
* 21/11/07 v1.0
* 29/05/08 v2.0
* 14/07/08 v2.0.1
********************************************************************************
* THE PRESENT SOFTWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT, 
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING 
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*
* THIS SOURCE CODE IS PROTECTED BY A LICENSE.
* FOR MORE INFORMATION PLEASE CAREFULLY READ THE LICENSE AGREEMENT FILE LOCATED
* IN THE ROOT DIRECTORY OF THIS FIRMWARE PACKAGE.
*******************************************************************************/
/* Includes ------------------------------------------------------------------*/
#include "stm32f10x.h"
#include "stm32f10x_MClib.h"
#include "MC_Globals.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
//========================================================
/* Select SPI FLASH: Chip Select pin low  */
#define SPI_KEY_PL_LOW()       GPIO_ResetBits(GPIOA, GPIO_Pin_13)
/* Deselect SPI FLASH: Chip Select pin high */
#define SPI_KEY_PL_HIGH()      GPIO_SetBits(GPIOA, GPIO_Pin_13)

#define KEY_PORT GPIOD

#define Set_573_OC  		 GPIO_SetBits(GPIOA,GPIO_Pin_14)
#define Clr_573_OC 			 GPIO_ResetBits(GPIOA,GPIO_Pin_14)
#define Set_595_Clk 		 GPIO_SetBits(GPIOA,GPIO_Pin_15)
#define Clr_595_Clk 		 GPIO_ResetBits(GPIOA,GPIO_Pin_15)
//=========================================================

#define  ANY_KEY             0x5A
#define  SEL_FLAG        (u8)0x02
#define  RIGHT_FLAG      (u8)0x04
#define  LEFT_FLAG       (u8)0x08
#define  UP_FLAG         (u8)0x10
#define  DOWN_FLAG       (u8)0x20
#define  LEFT_CODE        0xF7
#define  UP_CODE          0xFE
#define  SEL_CODE         0xFB
#define  RIGHT_CODE       0xFD
#define  DOWN_CODE        0xEF

//Variable increment and decrement

#define SPEED_INC_DEC     (u16)10    //速度增量倍数* 6
#define KP_GAIN_INC_DEC   (u16)100
#define KI_GAIN_INC_DEC   (u16)50
#define KD_GAIN_INC_DEC   (u16)50

#ifdef FLUX_WEAKENING
#define KP_VOLT_INC_DEC   (u8)50
#define KI_VOLT_INC_DEC   (u8)50
#define VOLT_LIM_INC_DEC  (u8)5 
#endif

#define TORQUE_INC_DEC    (u16)250
#define FLUX_INC_DEC      (u16)250

#define K1_INC_DEC        (s16)(250)
#define K2_INC_DEC        (s16)(5000)

#define PLL_IN_DEC        (u16)(25)

/* Private macro -------------------------------------------------------------*/
u8 KEYS_Read (void);
u8 SPI_KEY_SendByte(u8);
void Write595(u8); //写数据到74HC595，数码管位码函数
u8 ReadKey(void);

static u8 DATA_74HC595_74HC165;
static u8 ch;
uc8 seg_code[] = { 0x3f, 0x06, 0x5b, 0x4F, 0x66 ,0x6D, 0x7D, 0x07, 0x7F, 0x6F}; 
uc8 dig_code[] = { 0xF7, 0xFB, 0xFD, 0xFE, 0x7F, 0xBF, 0xDF, 0xEF};
u8 seg[8];

/* Private variables ---------------------------------------------------------*/
static u8 bKey;
static u8 bPrevious_key;
static u8 bKey_Flag;

#ifdef FLUX_WEAKENING
extern s16 hFW_P_Gain;
extern s16 hFW_I_Gain;
extern s16 hFW_V_Ref;
#endif

#ifdef OBSERVER_GAIN_TUNING
extern volatile s32 wK1_LO;
extern volatile s32 wK2_LO;
extern volatile s16 hPLL_P_Gain, hPLL_I_Gain;
#endif

#ifdef FLUX_TORQUE_PIDs_TUNING
u8 bMenu_index = CONTROL_MODE_MENU_6;
#else
u8 bMenu_index ;
#endif

//===================数码管显示子程序==============================

void WriteToSeg(u8 seg) 	   //写数码管段码
{
	u16 temp;
	temp = GPIO_ReadOutputData(GPIOC);
	Set_573_OC;		  //OC = 1,置位不锁存
	GPIO_Write(GPIOC, seg |(temp&0xff00));
//	Set_573_OC;		  //OC = 1,置位不锁存
	Clr_573_OC;		 //	OC = 0，复位锁存
}


void Write595(u8 LED_DATA) //写数据到74HC595，数码管位码函数
{
  
  /* Loop while DR register in not emplty */
  while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET);
  /* Send byte through the SPI1 peripheral */
  SPI_I2S_SendData(SPI1, LED_DATA);
  /* Loop while DR register in not emplty */
  while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);

  DATA_74HC595_74HC165 = (u8)SPI_I2S_ReceiveData(SPI1); //记忆最后一次发送的数据

  Set_595_Clk;
  Set_595_Clk;
  Set_595_Clk;
  Clr_595_Clk;		 
  Clr_595_Clk;		 
  Clr_595_Clk;		 
} 

u8 Read165(void) //读74HC165函数,五位键盘码
{ 

  SPI_KEY_PL_LOW();   //DESELECT 74HC165
  SPI_KEY_PL_HIGH();   //DESELECT 74HC165
  SPI_KEY_PL_LOW();   //DESELECT 74HC165
  SPI_KEY_PL_HIGH();   //DESELECT 74HC165
  SPI_KEY_PL_LOW();   //DESELECT 74HC165
  SPI_KEY_PL_HIGH();   //DESELECT 74HC165

  while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET);

  /* Send byte through the SPI1 peripheral */
  SPI_I2S_SendData(SPI1, ANY_KEY);

  /* Wait to receive a byte */
  while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);
  /* Return the byte read from the SPI bus */
  return SPI_I2S_ReceiveData(SPI1);
}

__inline u8 ReadKey(void)//直接读GPIO口的按键
{  
   return ((u8)(GPIO_ReadInputData(KEY_PORT)));
}

void Display_Seg(void)
{ static u8 i,j=0,sign=0;
   if (TB_DisplayDelay_IsElapsed() == TRUE) 
  { 
   
   if(i==2&&sign==0)
      {
	    i=0;
		sign=1;
//	    GPIO_SetBits(GPIOB, GPIO_Pin_8);
//      GPIO_ResetBits(GPIOB, GPIO_Pin_10);
	   }
     else if(i==2&&sign==1)
	  {
	    i=0;
		sign=0;
//	    GPIO_SetBits(GPIOB, GPIO_Pin_10);
//      GPIO_ResetBits(GPIOB, GPIO_Pin_8);
		
	 }
	 if(i!=2)
	    i++;

	if(j<8)
	  {
	   WriteToSeg(seg[j]);
	   Write595(dig_code[j]);
	   j++ ;
	   }
	else
	   j=0;
    } 	
}

//============================数码管显示子程序=============================

/*******************************************************************************
* Function Name  : 键盘和数码管初始化函数
* Description    : Init GPIOs for joystick/button management
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void KEYS_SEG_Init(void)
{ 
  u8 i;
  GPIO_InitTypeDef GPIO_InitStructure;
    
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOD, ENABLE);
 
  GPIO_StructInit(&GPIO_InitStructure);
  
  //设置使用PD口的低8位为按键输入口
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7 ; 
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOD, &GPIO_InitStructure);
 
  GPIO_StructInit(&GPIO_InitStructure);
  
  //设置使用74HC165的/PL端,SPI1口的设置已经完成
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13; 
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  SPI_KEY_PL_HIGH();   //DESELECT 165

  //设置使用74HC573的OC端,SPI1口的设置已经完成
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14; 
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  Clr_573_OC;		  //OC = 0,锁存

  //设置使用74HC595的ST_CLK端,SPI1口的设置已经完成
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15; 
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
//  Clr_595_Clk;		  //OC = 0,锁存
  //初始化数码管显存
  for(i=0;i<8;i++)
    seg[i]=seg_code[i];
}
  
//为了提高按键可靠性，应该对每个键都增加去抖动功能
/*******************************************************************************
* Function Name  : KEYS_Read
* Description    : Reads key from demoboard.
* Input          : None
* Output         : None
* Return         : Return RIGHT, LEFT, SEL, UP, DOWN, KEY_HOLD or NOKEY
*******************************************************************************/
u8 KEYS_Read ( void )
{ 
 //ch = Read165();
   ch = ReadKey();
  /* "RIGHT" key is pressed */
  if(ch==RIGHT_CODE)
  {
    if (bPrevious_key == RIGHT) 
    {
      return KEY_HOLD;
    }
    else
    {
      bPrevious_key = RIGHT;
      return RIGHT;
    }
  }
  /* "LEFT" key is pressed */
  else if(ch==LEFT_CODE)
  {
    if (bPrevious_key == LEFT) 
    {
      return KEY_HOLD;
    }
    else
    {
      bPrevious_key = LEFT;
      return LEFT;
    }
  }
  /* "SEL" key is pressed */
   if(ch==SEL_CODE)
  {
    if (bPrevious_key == SEL) 
    {
      return KEY_HOLD;
    }
    else
    {
      if ( (TB_DebounceDelay_IsElapsed() == FALSE) && (bKey_Flag & SEL_FLAG == SEL_FLAG) )
      {
        return NOKEY;
      }
      else
      {
      if ( (TB_DebounceDelay_IsElapsed() == TRUE) && ( (bKey_Flag & SEL_FLAG) == 0) ) 
      {
        bKey_Flag |= SEL_FLAG;
        TB_Set_DebounceDelay_500us(100); // 50 ms debounce,去抖动时间 = 50ms
      }
      else if ( (TB_DebounceDelay_IsElapsed() == TRUE) && ((bKey_Flag & SEL_FLAG) == SEL_FLAG) )
      {
        bKey_Flag &= (u8)(~SEL_FLAG);
        bPrevious_key = SEL;
        return SEL;
      }
      return NOKEY;
      }
    }
  }

   /* "UP" key is pressed */
  else if(ch==UP_CODE)
  {
    if (bPrevious_key == UP) 
    {
      return KEY_HOLD;
    }
    else
    {
      bPrevious_key = UP;
      return UP;
    }
  }
  /* "DOWN" key is pressed */
  else if(ch==DOWN_CODE)
  {
    if (bPrevious_key == DOWN) 
    {
      return KEY_HOLD;
    }
    else
    {
      bPrevious_key = DOWN;
      return DOWN;
    }
  }
  
  /* No key is pressed */
  else
  {
    bPrevious_key = NOKEY;
    return NOKEY;
  }
}



/*******************************************************************************
* Function Name  : KEYS_process
* Description    : Process key 
* Input          : Key code
* Output         : None
* Return         : None
*******************************************************************************/
void KEYS_process(void)
{
bKey = KEYS_Read();    // read key pushed (if any...)

switch (bMenu_index)
      {        
        case(CONTROL_MODE_MENU_1):
          switch(bKey)
          {
            case UP:
            case DOWN:
              wGlobal_Flags ^= SPEED_CONTROL;
              bMenu_index = CONTROL_MODE_MENU_6;
            break;
            
            case RIGHT:
                bMenu_index = REF_SPEED_MENU;
            break;
            
            case LEFT:
#ifdef DAC_FUNCTIONALITY              
              bMenu_index = DAC_PB1_MENU;
#elif defined OBSERVER_GAIN_TUNING
              bMenu_index = I_PLL_MENU;          
#else              
              bMenu_index = OTHER_ADC_MENU;
#endif              
            break;
            
            case SEL:
              if (State == RUN)
              {
                State = STOP;               
              }
              else if (State== START)
              {
                State = STOP; 
              }
                else if(State == IDLE)
                {
                  State = INIT;
                  bMenu_index = REF_SPEED_MENU;
                }  
              
            break;
            default:
            break;
          }
        break;
        
        case(REF_SPEED_MENU):
          switch(bKey)
          {
            case UP:
              if (hSpeed_Reference <= MOTOR_MAX_SPEED_HZ)
              {
//                hSpeed_Reference += ADC2_RegularConvertedValueTab[0] >> 5 ;
                hSpeed_Reference += SPEED_INC_DEC ;
              }
            break;
              
            case DOWN:
              if (hSpeed_Reference >= -MOTOR_MAX_SPEED_HZ)
              {
//                hSpeed_Reference -= ADC2_RegularConvertedValueTab[0] >> 5 ;
                hSpeed_Reference -= SPEED_INC_DEC ;
              }
            break;
           
            case RIGHT:
                bMenu_index = P_SPEED_MENU;
            break;

            case LEFT:
              if (State == IDLE)
              {
                bMenu_index = CONTROL_MODE_MENU_1;
              }
              else
              {
#ifdef DAC_FUNCTIONALITY              
              bMenu_index = DAC_PB1_MENU;
#elif defined OBSERVER_GAIN_TUNING
              bMenu_index = I_PLL_MENU;          
#else              
              bMenu_index = OTHER_ADC_MENU;
#endif 
              }              
            break;
            
            case SEL:
              if (State == RUN)
              {
                State = STOP;               
              }
              else if(State == START)
              {
                State = STOP;
              }              
                else if(State == IDLE)
                {
                  State = INIT;
                }   
            break;
            default:
            break;
          }
        break;          
        
        case(P_SPEED_MENU):    
          switch(bKey)
          {
            case UP:
              if (PID_Speed_InitStructure.hKp_Gain <= S16_MAX-KP_GAIN_INC_DEC)
              {
                PID_Speed_InitStructure.hKp_Gain += KP_GAIN_INC_DEC;
              }
            break;
              
            case DOWN:
              if (PID_Speed_InitStructure.hKp_Gain >= KP_GAIN_INC_DEC)
              {
              PID_Speed_InitStructure.hKp_Gain -= KP_GAIN_INC_DEC;
              }
            break;
           
            case RIGHT:
                bMenu_index = I_SPEED_MENU;
            break;

            case LEFT:
                bMenu_index = REF_SPEED_MENU;             
            break;
            
            case SEL:
              if (State == RUN)
              {
                State = STOP;               
              }
              else if (State== START)
              {
                State = STOP; 
              }
                else if(State == IDLE)
                {
                  State = INIT;
                }   
            break;
          default:
            break;
          }
        break;

        case(I_SPEED_MENU):    
          switch(bKey)
          {
            case UP:
              if (PID_Speed_InitStructure.hKi_Gain <= S16_MAX-KI_GAIN_INC_DEC)
              {
                PID_Speed_InitStructure.hKi_Gain += KI_GAIN_INC_DEC;
              }
            break;
              
            case DOWN:
              if (PID_Speed_InitStructure.hKi_Gain >= KI_GAIN_INC_DEC)
              {
              PID_Speed_InitStructure.hKi_Gain -= KI_GAIN_INC_DEC;
              }
            break;
           
            case RIGHT:
#ifdef DIFFERENTIAL_TERM_ENABLED                 
                bMenu_index = D_SPEED_MENU;
#else
                bMenu_index = P_TORQUE_MENU;
#endif                
            break;

            case LEFT:
              bMenu_index = P_SPEED_MENU;
            break;
            
            case SEL:
              if (State == RUN)
              {
                State = STOP;               
              }
              else if (State== START)
              {
                State = STOP; 
              }
                else if(State == IDLE)
                {
                  State = INIT;
                }   
            break;
          default:
            break;
          }
        break;        
        
#ifdef DIFFERENTIAL_TERM_ENABLED       
        case(D_SPEED_MENU):    
          switch(bKey)
          {
            case UP:
              if (PID_Speed_InitStructure.hKd_Gain <= S16_MAX-KD_GAIN_INC_DEC)
              {
                PID_Speed_InitStructure.hKd_Gain += KD_GAIN_INC_DEC;
              }
            break;
              
            case DOWN:
              if (PID_Speed_InitStructure.hKd_Gain >= KD_GAIN_INC_DEC)
              {
              PID_Speed_InitStructure.hKd_Gain -= KD_GAIN_INC_DEC;
              }
            break;
           
            case RIGHT:                
                bMenu_index = P_TORQUE_MENU;               
            break;
            
            case LEFT:
              bMenu_index = I_SPEED_MENU;
            break;
            
            case SEL:
              if (State == RUN || State == START)
              {
                State = STOP;               
              }
              else if(State == IDLE)
              {
                State = INIT;
              }   
            break;
          default:
            break;
          }
        break;   
#endif
        case(P_TORQUE_MENU):    
          switch(bKey)
          {
            case UP:
              if (PID_Torque_InitStructure.hKp_Gain <= S16_MAX - KP_GAIN_INC_DEC)
              {
                PID_Torque_InitStructure.hKp_Gain += KP_GAIN_INC_DEC;
              }
            break;
              
            case DOWN:
              if (PID_Torque_InitStructure.hKp_Gain >= KP_GAIN_INC_DEC)
              {
              PID_Torque_InitStructure.hKp_Gain -= KP_GAIN_INC_DEC;
              }
            break;
           
            case RIGHT:
                bMenu_index = I_TORQUE_MENU;
            break;
             
            case LEFT:         
              if ((wGlobal_Flags & SPEED_CONTROL) == SPEED_CONTROL)
              {
#ifdef DIFFERENTIAL_TERM_ENABLED              
                bMenu_index = D_SPEED_MENU;
#else
                bMenu_index = I_SPEED_MENU; 
#endif                
              }
              else
              {
                bMenu_index = ID_REF_MENU;
              }              
            break;
            
            case SEL:
              if (State == RUN)
              {
                State = STOP;               
              }
              else if (State== START)
                {
                  State = STOP; 
                }
                  else if(State == IDLE)
                  {
                    State = INIT;
                  }   
            break;
          default:
            break;
          }
        break;
        
        case(I_TORQUE_MENU):    
          switch(bKey)
          {
            case UP:
              if (PID_Torque_InitStructure.hKi_Gain <= S16_MAX - KI_GAIN_INC_DEC)
              {
                PID_Torque_InitStructure.hKi_Gain += KI_GAIN_INC_DEC;
              }
            break;
              
            case DOWN:
              if (PID_Torque_InitStructure.hKi_Gain >= KI_GAIN_INC_DEC)
              {
              PID_Torque_InitStructure.hKi_Gain -= KI_GAIN_INC_DEC;
              }
            break;
           
            case RIGHT:
#ifdef DIFFERENTIAL_TERM_ENABLED                 
                bMenu_index = D_TORQUE_MENU;
#else
                bMenu_index = P_FLUX_MENU;
#endif                
            break;

            case LEFT:
              bMenu_index = P_TORQUE_MENU;
            break;
            
            case SEL:
              if (State == RUN)
              {
                State = STOP;               
              }
                else if (State== START)
                {
                  State = STOP; 
                }
                  else if(State == IDLE)
                  {
                    State = INIT;
                  }   
            break;
          default:
            break;
          }
        break;
        
#ifdef DIFFERENTIAL_TERM_ENABLED       
        case(D_TORQUE_MENU):    
          switch(bKey)
          {
            case UP:
              if (PID_Torque_InitStructure.hKd_Gain <= S16_MAX - KD_GAIN_INC_DEC)
              {
                PID_Torque_InitStructure.hKd_Gain += KD_GAIN_INC_DEC;
              }
            break;
              
            case DOWN:
              if (PID_Torque_InitStructure.hKd_Gain >= KD_GAIN_INC_DEC)
              {
              PID_Torque_InitStructure.hKd_Gain -= KD_GAIN_INC_DEC;
              }
            break;
           
            case RIGHT:                
                bMenu_index = P_FLUX_MENU;               
            break;
            
            case LEFT:
              bMenu_index = I_TORQUE_MENU;
            break;
            
            case SEL:
              if (State == RUN || State == START)
              {
                State = STOP;               
              }
              else if(State == IDLE)
              {
                State = INIT;
              }   
            break;
          default:
            break;
          }
        break;   
#endif 
         case(P_FLUX_MENU):    
          switch(bKey)
          {
            case UP:
              if (PID_Flux_InitStructure.hKp_Gain <= S16_MAX-KP_GAIN_INC_DEC)
              {
                PID_Flux_InitStructure.hKp_Gain += KP_GAIN_INC_DEC;
              }
            break;
              
            case DOWN:
              if (PID_Flux_InitStructure.hKp_Gain >= KP_GAIN_INC_DEC)
              {
                PID_Flux_InitStructure.hKp_Gain -= KP_GAIN_INC_DEC;
              }
            break;
           
            case RIGHT:
                bMenu_index = I_FLUX_MENU;
            break;
             
            case LEFT:         
#ifdef  DIFFERENTIAL_TERM_ENABLED
              bMenu_index = D_TORQUE_MENU;
#else
              bMenu_index = I_TORQUE_MENU;
#endif              
            break;
            
            case SEL:
              if (State == RUN)
              {
                State = STOP;               
              }
                else if (State== START)
                {
                  State = STOP; 
                }
                  else if(State == IDLE)
                  {
                    State = INIT;
                  }   
            break;
          default:
            break;
          }
        break;
        
        case(I_FLUX_MENU):    
          switch(bKey)
          {
            case UP:
              if (PID_Flux_InitStructure.hKi_Gain <= S16_MAX-KI_GAIN_INC_DEC)
              {
                PID_Flux_InitStructure.hKi_Gain += KI_GAIN_INC_DEC;
              }
            break;
              
            case DOWN:
              if (PID_Flux_InitStructure.hKi_Gain >= KI_GAIN_INC_DEC)
              {
              PID_Flux_InitStructure.hKi_Gain -= KI_GAIN_INC_DEC;
              }
            break;
           
            case RIGHT:
#ifdef DIFFERENTIAL_TERM_ENABLED                 
                bMenu_index = D_FLUX_MENU;
#elif defined FLUX_WEAKENING
                bMenu_index = P_VOLT_MENU;
#else                
                bMenu_index = POWER_STAGE_MENU;
#endif                
            break;

            case LEFT:
              bMenu_index = P_FLUX_MENU;
            break;
            
            case SEL:
              if (State == RUN)
              {
                State = STOP;               
              }
                else if (State== START)
                {
                  State = STOP; 
                }
                  else if(State == IDLE)
                  {
                    State = INIT;
                  }   
            break;
          default:
            break;
          }
        break;
        
#ifdef DIFFERENTIAL_TERM_ENABLED       
        case(D_FLUX_MENU):    
          switch(bKey)
          {
            case UP:
              if (PID_Flux_InitStructure.hKd_Gain <= S16_MAX - KD_GAIN_INC_DEC)
              {
                PID_Flux_InitStructure.hKd_Gain += KD_GAIN_INC_DEC;
              }
            break;
              
            case DOWN:
              if (PID_Flux_InitStructure.hKd_Gain >= KD_GAIN_INC_DEC)
              {
              PID_Flux_InitStructure.hKd_Gain -= KD_GAIN_INC_DEC;
              }
            break;
           
            case RIGHT:
#ifdef FLUX_WEAKENING
                bMenu_index = P_VOLT_MENU;
#else
                bMenu_index = POWER_STAGE_MENU;
#endif                
            break;
            
            case LEFT:              
              bMenu_index = I_FLUX_MENU;
            break;
            
            case SEL:
              if (State == RUN || State == START)
              {
                State = STOP;               
              }
              else if(State == IDLE)
              {
                State = INIT;
              }   
            break;
          default:
            break;
          }
        break;   
#endif

#ifdef FLUX_WEAKENING        
         case(P_VOLT_MENU):    
          switch(bKey)
          {
            case UP:
              if (hFW_P_Gain <= 32500)
              {
                hFW_P_Gain += KP_VOLT_INC_DEC;
              }
            break;
              
            case DOWN:
              if (hFW_P_Gain >= KP_VOLT_INC_DEC)
              {
                hFW_P_Gain -= KP_VOLT_INC_DEC;
              }
            break;
           
            case RIGHT:
                bMenu_index = I_VOLT_MENU;
            break;
             
            case LEFT:
#ifdef DIFFERENTIAL_TERM_ENABLED            
              bMenu_index = D_FLUX_MENU;
#else              
              bMenu_index = I_FLUX_MENU;
#endif              
            break;
            
            case SEL:
              if (State == RUN)
              {
                State = STOP;               
              }
                else if (State== START)
                {
                  State = STOP; 
                }
                  else if(State == IDLE)
                  {
                    State = INIT;
                  }   
            break;
          default:
            break;
          }
        break;
        
        case(I_VOLT_MENU):    
          switch(bKey)
          {
            case UP:
              if (hFW_I_Gain <= 32500)
              {
                hFW_I_Gain += KI_VOLT_INC_DEC;
              }
            break;
              
            case DOWN:
              if (hFW_I_Gain >= KI_VOLT_INC_DEC)
              {
                hFW_I_Gain -= KI_VOLT_INC_DEC;
              }
            break;
           
            case RIGHT:
                bMenu_index = TARGET_VOLT_MENU;
            break;

            case LEFT:
              bMenu_index = P_VOLT_MENU;
            break;
            
            case SEL:
              if (State == RUN)
              {
                State = STOP;               
              }
                else if (State== START)
                {
                  State = STOP; 
                }
                  else if(State == IDLE)
                  {
                    State = INIT;
                  }   
            break;
          default:
            break;
          }
        break;
        
        case(TARGET_VOLT_MENU):    
          switch(bKey)
          {
            case UP:
              if (hFW_V_Ref <= (1000-VOLT_LIM_INC_DEC))
              {
                hFW_V_Ref += VOLT_LIM_INC_DEC;
              }
            break;
              
            case DOWN:
              if (hFW_V_Ref >= VOLT_LIM_INC_DEC)
              {
                hFW_V_Ref -= VOLT_LIM_INC_DEC;
              }
            break;
           
            case RIGHT:
                bMenu_index = POWER_STAGE_MENU;
            break;

            case LEFT:
              bMenu_index = I_VOLT_MENU;
            break;
            
            case SEL:
              if (State == RUN)
              {
                State = STOP;               
              }
                else if (State== START)
                {
                  State = STOP; 
                }
                  else if(State == IDLE)
                  {
                    State = INIT;
                  }   
            break;
          default:
            break;
          }
        break;        
        
#endif        
        
        case(POWER_STAGE_MENU):
          switch(bKey)
          {
            case RIGHT:  
                  bMenu_index = OTHER_ADC_MENU;
            break;
            
            case LEFT:
#ifdef FLUX_WEAKENING
              bMenu_index = TARGET_VOLT_MENU;              
#elif defined DIFFERENTIAL_TERM_ENABLED            
              bMenu_index = D_FLUX_MENU;
#else
              bMenu_index = I_FLUX_MENU;                          
#endif              
            break;
            
            case SEL:
              if (State == RUN)
              {
                State = STOP;               
              }
                else if (State== START)
                {
                  State = STOP; 
                }
                  else if(State == IDLE)
                  {
                    State = INIT;
                  }   
            break;
          default:
            break;
          }
        break; 

//这里是辅助AD转换的显示菜单

       case(OTHER_ADC_MENU):
          switch(bKey)
          {
            case RIGHT:  
#ifdef OBSERVER_GAIN_TUNING
              bMenu_index = K1_MENU;
#elif defined DAC_FUNCTIONALITY
              bMenu_index = DAC_PB0_MENU;
#else              
              if ((wGlobal_Flags & SPEED_CONTROL == SPEED_CONTROL))
              {
                if (State == IDLE)
                {
                  bMenu_index = CONTROL_MODE_MENU_1;
                }
                else
                {
                  bMenu_index = REF_SPEED_MENU;
                }
              }
              else //Torque control
              {
                if (State == IDLE)
                {
                  bMenu_index = CONTROL_MODE_MENU_6;
                }
                else
                {
                  bMenu_index = IQ_REF_MENU;
                }
              }
#endif              
            break;
            
            case LEFT:
              bMenu_index = POWER_STAGE_MENU;                          
             
            break;
            
            case SEL:
              if (State == RUN)
              {
                State = STOP;               
              }
                else if (State== START)
                {
                  State = STOP; 
                }
                  else if(State == IDLE)
                  {
                    State = INIT;
                  }   
            break;
          default:
            break;
          }
        break; 

//上面是增加的辅助ADC转换的显示画面

        case(CONTROL_MODE_MENU_6):
          switch(bKey)
          {
            case UP:
            case DOWN:
              wGlobal_Flags ^= SPEED_CONTROL;
              bMenu_index = CONTROL_MODE_MENU_1;
            break;
            
            case RIGHT:
                bMenu_index = IQ_REF_MENU;
            break;
            
            case LEFT:
#ifdef DAC_FUNCTIONALITY              
              bMenu_index = DAC_PB1_MENU;
#elif defined OBSERVER_GAIN_TUNING
              bMenu_index = I_PLL_MENU;          
#else              
              bMenu_index = OTHER_ADC_MENU;
#endif 
            break;
            
            case SEL:
              if (State == RUN)
              {
                State = STOP;               
              }
                else if (State== START)
                {
                  State = STOP; 
                }
                  else if(State == IDLE)
                  {
                    State = INIT;
                    bMenu_index = IQ_REF_MENU;
                  }   
            break;
            default:
            break;
          }
        break;  
        
        case(IQ_REF_MENU):
          switch(bKey)
          {
            case UP:
              if (hTorque_Reference <= NOMINAL_CURRENT - TORQUE_INC_DEC)
              {
                hTorque_Reference += TORQUE_INC_DEC;
              }
            break;
            
            case DOWN:
            if (hTorque_Reference >= -NOMINAL_CURRENT + TORQUE_INC_DEC)
            {
              hTorque_Reference -= TORQUE_INC_DEC;
            }
            break;
            
            case RIGHT:
                bMenu_index = ID_REF_MENU;
            break;
            
            case LEFT:
              if(State == IDLE)
              {
                bMenu_index = CONTROL_MODE_MENU_6;
              }
              else
              {
#ifdef DAC_FUNCTIONALITY              
              bMenu_index = DAC_PB1_MENU;
#elif defined OBSERVER_GAIN_TUNING
              bMenu_index = I_PLL_MENU;          
#else              
              bMenu_index = OTHER_ADC_MENU;
#endif 
              }
            break;
            
            case SEL:
              if (State == RUN)
              {
                State = STOP;               
              }
                else if (State== START)
                {
                  State = STOP; 
                }
                  else if(State == IDLE)
                  {
                    State = INIT;
                  }   
            break;
            default:
            break;
          }
        break;    
        
        case(ID_REF_MENU):
          switch(bKey)
          {
            case UP:
              if (hFlux_Reference <= NOMINAL_CURRENT - FLUX_INC_DEC)
              {
                hFlux_Reference += FLUX_INC_DEC;
              }
            break;
            
            case DOWN:
              if (hFlux_Reference >= FLUX_INC_DEC - NOMINAL_CURRENT)
              {
                hFlux_Reference -= FLUX_INC_DEC;
              }
            break;
            
            case RIGHT:
              bMenu_index = P_TORQUE_MENU;
            break;
            
            case LEFT:
              bMenu_index = IQ_REF_MENU;
            break;
            
            case SEL:
              if (State == RUN)
              {
                State = STOP;               
              }
                else if (State== START)
                {
                  State = STOP; 
                }
                  else if(State == IDLE)
                  {
                    State = INIT;
                  }   
            break;
            default:
            break;
          }
        break;

#ifdef OBSERVER_GAIN_TUNING
        case(K1_MENU):    
          switch(bKey)
          {
            case UP:
              if (wK1_LO <= -K1_INC_DEC)
              {
                wK1_LO += K1_INC_DEC;
              }
            break;  
             
            case DOWN:
              if (wK1_LO >= -600000)
              {
                wK1_LO -= K1_INC_DEC;
              }
            break;
           
            case RIGHT:
              bMenu_index = K2_MENU;
            break;
             
            case LEFT:         
             bMenu_index = OTHER_ADC_MENU;
            break;
            
            case SEL:
              if (State == RUN)
              {
                State = STOP;               
              }
                else if (State== START)
                {
                  State = STOP; 
                }
                  else if(State == IDLE)
                  {
                    State = INIT;
                  }   
            break;
          default:
            break;
          }
          break;
        
        case(K2_MENU):    
          switch(bKey)
          {
            case UP:
              if (wK2_LO <= S16_MAX * 100 - K2_INC_DEC)
              {
                wK2_LO += K2_INC_DEC;
              }
            break;  
             
            case DOWN:
             if (wK2_LO >= K2_INC_DEC)
              {
                wK2_LO -= K2_INC_DEC;
              }
            break;
           
            case RIGHT:
              bMenu_index = P_PLL_MENU;
            break;
             
            case LEFT:         
             bMenu_index = K1_MENU;
            break;
            
            case SEL:
              if (State == RUN)
              {
                State = STOP;               
              }
                else if (State== START)
                {
                  State = STOP; 
                }
                  else if(State == IDLE)
                  {
                    State = INIT;
                  }   
            break;
          default:
            break;
          }
          break;
          
         case(P_PLL_MENU):    
          switch(bKey)
          {
            case UP:
             if (hPLL_P_Gain <= 32000)
              {
                hPLL_P_Gain += PLL_IN_DEC;
              } 
            break;  
             
            case DOWN:
             if (hPLL_P_Gain > 0)
              {
                hPLL_P_Gain -= PLL_IN_DEC;
              } 
            break;
           
            case RIGHT:
              bMenu_index = I_PLL_MENU;
            break;
             
            case LEFT:         
             bMenu_index = K2_MENU;
            break;
            
            case SEL:
              if (State == RUN)
              {
                State = STOP;               
              }
                else if (State== START)
                {
                  State = STOP; 
                }
                  else if(State == IDLE)
                  {
                    State = INIT;
                  }   
            break;
          default:
            break;
          }
          break;
          
        case(I_PLL_MENU):    
          switch(bKey)
          {
            case UP:
             if (hPLL_I_Gain <= 32000)
              {
                hPLL_I_Gain += PLL_IN_DEC;
              } 
            break;  
             
            case DOWN:
             if (hPLL_I_Gain > 0)
              {
                hPLL_I_Gain -= PLL_IN_DEC;
              } 
            break;
           
            case RIGHT:
#ifdef DAC_FUNCTIONALITY
              bMenu_index = DAC_PB0_MENU;  
#else
              if ((wGlobal_Flags & SPEED_CONTROL == SPEED_CONTROL))
              {
                if (State == IDLE)
                {
                  bMenu_index = CONTROL_MODE_MENU_1;
                }
                else
                {
                  bMenu_index = REF_SPEED_MENU;
                }
              }
              else //Torque control
              {
                if (State == IDLE)
                {
                  bMenu_index = CONTROL_MODE_MENU_6;
                }
                else
                {
                  bMenu_index = IQ_REF_MENU;
                }
              }
#endif              
            break;
             
            case LEFT:         
              bMenu_index = P_PLL_MENU;
            break;
            
            case SEL:
              if (State == RUN)
              {
                State = STOP;               
              }
                else if (State== START)
                {
                  State = STOP; 
                }
                  else if(State == IDLE)
                  {
                    State = INIT;
                  }   
            break;
          default:
            break;
          }
          break;          
#endif
  
#ifdef DAC_FUNCTIONALITY  
        case(DAC_PB0_MENU):    
          switch(bKey)
          {
            case UP:
              MCDAC_Output_Choice(1,DAC_CH1);
            break;
            
            case DOWN:
              MCDAC_Output_Choice(-1,DAC_CH1);
            break;  
           
            case RIGHT:                
                bMenu_index = DAC_PB1_MENU;               
            break;
            
            case LEFT:
#ifdef OBSERVER_GAIN_TUNING              
              bMenu_index = I_PLL_MENU;
#else
              bMenu_index = OTHER_ADC_MENU;
#endif              
            break;
            
            case SEL:
              switch(State)
              {
              case RUN:
                State = STOP;
                break;
              case START:
                State = STOP;
                break;
              case IDLE:
                State = INIT;
                break;
              default:
                break;                
              }   
            break;
          default:
            break;
          }
        break;   

        case(DAC_PB1_MENU):    
          switch(bKey)
          {
            case UP:
              MCDAC_Output_Choice(1,DAC_CH2);
            break;
            
            case DOWN:
              MCDAC_Output_Choice(-1,DAC_CH2);
            break;  
           
            case RIGHT:                
            if ((wGlobal_Flags & SPEED_CONTROL == SPEED_CONTROL))
              {
                if (State == IDLE)
                {
                  bMenu_index = CONTROL_MODE_MENU_1;
                }
                else
                {
                  bMenu_index = REF_SPEED_MENU;
                }
              }
              else //Torque control
              {
                if (State == IDLE)
                {
                  bMenu_index = CONTROL_MODE_MENU_6;
                }
                else
                {
                  bMenu_index = IQ_REF_MENU;
                }
              }               
            break;
            
            case LEFT:             
              bMenu_index = DAC_PB0_MENU;
            break;
            
            case SEL:
              switch(State)
              {
              case RUN:
                State = STOP;
                break;
              case START:
                State = STOP;
                break;
              case IDLE:
                State = INIT;
                break;
              default:
                break;                
              }      
            break;
          default:
            break;
          }
        break; 
#endif 
    
      default:
        break; 
      }
}

/*******************************************************************************
* Function Name  : KEYS_ExportbKey
* Description    : Export bKey variable
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
u8 KEYS_ExportbKey(void)
{
  return(bKey);
}
                   
/******************* (C) COPYRIGHT 2008 STMicroelectronics *****END OF FILE****/

