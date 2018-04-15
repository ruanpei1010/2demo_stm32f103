/******************** (C) COPYRIGHT 2008 STMicroelectronics ********************
* File Name          : main.c
* Author             : IMS Systems Lab
* Date First Issued  : 21/11/07
* Description        : Main program body.
********************************************************************************
* History:
* 21/11/07 v1.0
* 29/05/08 v2.0
* 27/06/08 v2.0.1
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
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/

void NVIC_Configuration(void);
void GPIO_Configuration(void);
void RCC_Configuration(void);

/*******************************************************************************
* Function Name  : main
* Description    : Main program.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
int main(void)
{

   u8 i = 0;

#ifdef DEBUG
  debug();
#endif
//  SystemInit();
//  RCC_Configuration();	  //SYSCLK = 72MHZ
  NVIC_Configuration();   //USE FLSAH NOT RAM
  GPIO_Configuration();	  //1个LED

  
#ifdef THREE_SHUNT  
  SVPWM_3ShuntInit();//使用了这的初始化。
#elif defined ICS_SENSORS
  SVPWM_IcsInit();
#elif defined SINGLE_SHUNT
  SVPWM_1ShuntInit();	// 设定使用单电阻模式，里面包含AD模块、TIM1模块、DMA模块的初始化
#endif
  
#ifdef ENCODER
   ENC_Init();
   #ifdef OBSERVER_GAIN_TUNING
      STO_StateObserverInterface_Init();
      STO_Init();
   #endif
#elif defined HALL_SENSORS
   HALL_HallTimerInit();
   #ifdef OBSERVER_GAIN_TUNING
      STO_StateObserverInterface_Init();
      STO_Init();
   #endif
#elif defined NO_SPEED_SENSORS		// 设定使用无传感器模式
    STO_StateObserverInterface_Init();
    STO_Init();
   #ifdef VIEW_ENCODER_FEEDBACK
      ENC_Init();
   #elif defined VIEW_HALL_FEEDBACK
      HALL_HallTimerInit();
   #endif
#endif

#ifdef DAC_FUNCTIONALITY   
  MCDAC_Init();
#endif	  

  TB_Init();
  
  PID_Init(&PID_Torque_InitStructure, &PID_Flux_InitStructure, &PID_Speed_InitStructure);

#ifdef BRAKE_RESISTOR
    MCL_Brake_Init();
#endif
  //初始化风扇和蜂鸣器
    MCL_Fan_Init();
    MCL_Bell_Init();
    MCL_Fault_Reset_Init();  
  /* TIM1 Counter Clock stopped when the core is halted */
  DBGMCU_Config(DBGMCU_TIM1_STOP, ENABLE);	  
  
  // Init Bus voltage and Temperature average  
  MCL_Init_Arrays();
  
  //TFT_LCD初始化以及测试  
  LCD_Init();
  LCD_Clear(Blue2);
  LCD_SetTextColor(Blue);
  LCD_SetBackColor(White);
  Test_Color();

  TB_Wait(2000); //延时2S

  AT45_INIT();  //初始化AT45DB_FLASH汉字库接口。

  KEYS_SEG_Init();	//初始化键盘和数码管
  
  Display_Welcome_Message();   //显示公司版权信息
    
  while(1)
  { 
//    Display_Seg();	  //刷新数码管
 
    Display_LCD();	  //刷新LCD，刷新频率为0.5ms *400 = 200ms
 
    MCL_ChkPowerStage();  //检测母线电压和温度  
    //User interface management    
    KEYS_process();   //键盘处理函数
  
    switch (State)
    {
      case IDLE:    // Idle state  ,按下SEL按钮，转到INIT状态
      break;
      
      case INIT:
        MCL_Init();	//初始化各个模块，最后打开AD转换中断，等待从中断里面完成系统启动。
        TB_Set_StartUp_Timeout(5000);	 // 启动时间设为：500 * 2 * 0.5ms = 500ms
        State = START; 	 // 转到START状态
      break;
        
      case START:  		 //等待系统稳定转到RUN状态，启动是由AD中断子程序完成的，使用HALL时，直接转到RUN。
      //passage to state RUN is performed by startup functions; 
      break;
          
      case RUN:   // motor running       
#ifdef ENCODER
        if(ENC_ErrorOnFeedback() == TRUE)
        {
          MCL_SetFault(SPEED_FEEDBACK);
        }
#elif defined HALL_SENSORS        
        if(HALL_IsTimedOut())
        {
//          MCL_SetFault(SPEED_FEEDBACK);
        } 
        if (HALL_GetSpeed() == HALL_MAX_SPEED)	//通过DPP速度计算转子速度（HZ).
        {
//          MCL_SetFault(SPEED_FEEDBACK);
        } 
#elif defined NO_SPEED_SENSORS
      
#endif     
      break;  
      
      case STOP:    // motor stopped
          // shutdown power         
          /* Main PWM Output Disable */
          TIM_CtrlPWMOutputs(TIM1, DISABLE);
          
          State = WAIT;
          
#ifdef THREE_SHUNT          
          SVPWM_3ShuntAdvCurrentReading(DISABLE);
#endif   
#ifdef SINGLE_SHUNT          
          SVPWM_1ShuntAdvCurrentReading(DISABLE);
#endif
          Stat_Volt_alfa_beta.qV_Component1 = Stat_Volt_alfa_beta.qV_Component2 = 0;
          
#ifdef ICS_SENSORS
          SVPWM_IcsCalcDutyCycles(Stat_Volt_alfa_beta);
#elif defined THREE_SHUNT
          SVPWM_3ShuntCalcDutyCycles(Stat_Volt_alfa_beta);
#endif                                                
          TB_Set_Delay_500us(1000); // 1 sec delay
      break;
      
      case WAIT:    // wait state
          if (TB_Delay_IsElapsed() == TRUE) 
          {
#ifdef ENCODER            
            if(ENC_Get_Mechanical_Speed() ==0)             
            {              
              State = IDLE;              
            }
#elif defined HALL_SENSORS      
            if (HALL_IsTimedOut())
            {               
              State=IDLE;
            } 
#elif defined NO_SPEED_SENSORS
            STO_InitSpeedBuffer();
            State=IDLE; 
#endif            
          }
        break;
    
      case FAULT:                   
          if (MCL_ClearFault() == TRUE)	 //如果能够复位FAULT标志，那么系统转到IDLE状态。
          {
            if(wGlobal_Flags & SPEED_CONTROL == SPEED_CONTROL)
            {
              bMenu_index = CONTROL_MODE_MENU_1;
            }
            else
            {
              bMenu_index = CONTROL_MODE_MENU_6;
            }       
#if defined NO_SPEED_SENSORS
            STO_InitSpeedBuffer();
#endif            
            State = IDLE;
            wGlobal_Flags |= FIRST_START;
          }
        break;
    
      default:        
        break;
    }
  }
}

/*******************************************************************************
* Function Name  : GPIO_Configuration
* Description    : Configures the TIM1 Pins.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void GPIO_Configuration(void)
{
  u8 temp = 0xff;
  GPIO_InitTypeDef GPIO_InitStructure;

  /* Enable GPIOC clock */
  RCC_APB2PeriphClockCmd( RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOD, ENABLE);

 //SPI1口重映射
  GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE); 
//  GPIO_PinRemapConfig(GPIO_Remap_SWJ_Disable, ENABLE); 
 
  GPIO_DeInit(GPIOC);
 
  GPIO_StructInit(&GPIO_InitStructure);	  // 3个LED
               
  /* Configure  PC.10 and PC.11 ,PC.12 as Output push-pull for debugging
     purposes */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_Init(GPIOC, &GPIO_InitStructure);    

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;  // DIN4
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_Init(GPIOC, &GPIO_InitStructure);    

  GPIO_DeInit(GPIOA);
  GPIO_StructInit(&GPIO_InitStructure);	  // 1个LED
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_Init(GPIOA, &GPIO_InitStructure);    

  GPIO_DeInit(GPIOB);
  GPIO_StructInit(&GPIO_InitStructure);	  // 1个DIN3
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_Init(GPIOB, &GPIO_InitStructure);    

  GPIO_DeInit(GPIOD);
  GPIO_StructInit(&GPIO_InitStructure);	  // 2个 DIN1,DIN2
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_Init(GPIOD, &GPIO_InitStructure);    

 //关闭LCD背光
  GPIO_ResetBits(GPIOD, GPIO_Pin_14);
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_Init(GPIOD, &GPIO_InitStructure);    


//  GPIO_ResetBits(GPIOA, GPIO_Pin_15);
//  GPIO_SetBits(GPIOC, GPIO_Pin_12);
//  GPIO_ResetBits(GPIOC, GPIO_Pin_11);
//  GPIO_SetBits(GPIOC, GPIO_Pin_10);
 
  //输入检测第一位
  if((GPIO_ReadInputDataBit(GPIOD, GPIO_Pin_8))==RESET)
        GPIO_ResetBits(GPIOA, GPIO_Pin_15);
  	else
        GPIO_SetBits(GPIOA, GPIO_Pin_15);
  //输入检测第二位
  if((GPIO_ReadInputDataBit(GPIOD, GPIO_Pin_9))==RESET)
          GPIO_ResetBits(GPIOC, GPIO_Pin_10);
	else
        GPIO_SetBits(GPIOC, GPIO_Pin_10);
  //输入检测第三位
  if((GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_9))==RESET)
        GPIO_ResetBits(GPIOC, GPIO_Pin_11);
	else
        GPIO_SetBits(GPIOC, GPIO_Pin_11);
  //输入检测第四位
  if((GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_13))==RESET)
        GPIO_ResetBits(GPIOC, GPIO_Pin_12);
	else
        GPIO_SetBits(GPIOC, GPIO_Pin_12);
 
}

/*******************************************************************************
* Function Name  : RCC_Configuration
* Description    : Configures the different system clocks.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void RCC_Configuration(void)
{ 
  ErrorStatus HSEStartUpStatus;

  /* RCC system reset(for debug purpose) */
  RCC_DeInit();

  /* Enable HSE */
  RCC_HSEConfig(RCC_HSE_ON);

  /* Wait till HSE is ready */
  HSEStartUpStatus = RCC_WaitForHSEStartUp();
  
  if(HSEStartUpStatus == SUCCESS)
  {
    /* HCLK = SYSCLK */
    RCC_HCLKConfig(RCC_SYSCLK_Div1); 
  
    /* PCLK2 = HCLK */
    RCC_PCLK2Config(RCC_HCLK_Div1); 

    /* PCLK1 = HCLK/2 */
    RCC_PCLK1Config(RCC_HCLK_Div2);

    /* Flash 2 wait state */
    FLASH_SetLatency(FLASH_Latency_2);
    /* Enable Prefetch Buffer */
    FLASH_PrefetchBufferCmd(FLASH_PrefetchBuffer_Enable);

    /* PLLCLK = 8MHz * 9 = 72 MHz */
    RCC_PLLConfig(RCC_PLLSource_HSE_Div1, RCC_PLLMul_9);

    /* Enable PLL */ 
    RCC_PLLCmd(ENABLE);

    /* Wait till PLL is ready */
    while(RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET)
    {
    }

    /* Select PLL as system clock source */
    RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);

    /* Wait till PLL is used as system clock source */
    while(RCC_GetSYSCLKSource() != 0x08)
    {
    }
  }
}

/*******************************************************************************
* Function Name  : NVIC_Configuration
* Description    : Configures the Vector Table base address.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void NVIC_Configuration(void)
{
#ifdef  VECT_TAB_RAM  
  /* Set the Vector Table base location at 0x20000000 */ 
  NVIC_SetVectorTable(NVIC_VectTab_RAM, 0x0); 
#else  /* VECT_TAB_FLASH  */
  /* Set the Vector Table base location at 0x08000000 */ 
  NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0x0);   
#endif
}

#ifdef  DEBUG
/*******************************************************************************
* Function Name  : assert_failed
* Description    : Reports the name of the source file and the source line number
*                  where the assert_param error has occurred.
* Input          : - file: pointer to the source file name
*                  - line: assert_param error line source number
* Output         : None
* Return         : None
*******************************************************************************/
void assert_failed(u8* file, u32 line)
{ 
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
    //printf("Wrong parameters value: file %s on line %d\r\n", file, line);
  }
}
#endif

/******************* (C) COPYRIGHT 2008 STMicroelectronics *****END OF FILE****/
