/******************** (C) COPYRIGHT 2008 STMicroelectronics ********************
* File Name          : stm32f10x_MCdac.c
* Author             : IMS Systems Lab 
* Date First Issued  : 07/06/07
* Description        : This module manages all the necessary function to 
*                      implement DAC functionality
********************************************************************************
* History:
* 28/11/07 v1.0
* 29/05/08 v2.0
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
#include "stm32f10x_MCdac.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
s16 hMeasurementArray[23];

u8 *OutputVariableNames[23] ={
  "0                   ","Ia                  ","Ib                  ",
  "Ialpha              ","Ibeta               ","Iq                  ",
  "Id                  ","Iq ref              ","Id ref              ",
  "Vq                  ","Vd                  ","Valpha              ",
  "Vbeta               ","Measured El Angle   ","Measured Rotor Speed",
  "Observed El Angle   ","Observed Rotor Speed","Observed Ialpha     ",
  "Observed Ibeta      ","Observed B-emf alpha","Observed B-emf beta ",
  "User 1              ","User 2              "};

#if (defined ENCODER || defined VIEW_ENCODER_FEEDBACK || defined HALL_SENSORS\
  || defined VIEW_HALL_FEEDBACK) && (defined NO_SPEED_SENSORS ||\
  defined OBSERVER_GAIN_TUNING)

u8 OutputVar[23]={0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22};
u8 max_out_var_num = 22;

#elif (defined NO_SPEED_SENSORS)
u8 OutputVar[21]={0,1,2,3,4,5,6,7,8,9,10,11,12,15,16,17,18,19,20,21,22};
u8 max_out_var_num = 20;

#elif (defined ENCODER || defined HALL_SENSORS)
u8 OutputVar[17]={0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,21,22};
u8 max_out_var_num = 16;
#endif

u8 bChannel_1_variable=1;
u8 bChannel_2_variable=1;

/*******************************************************************************
* Function Name : MCDAC_Configuration
* Description : provides a short description of the function
* Input : details the input parameters.
* Output : details the output parameters.
* Return : details the return value.
*******************************************************************************/
void MCDAC_Init (void)
{
  TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
  GPIO_InitTypeDef 		GPIO_InitStructure; 
  TIM_OCInitTypeDef 	TIM_OCInitStructure;
  DAC_InitTypeDef       DAC_InitStructure;
  /* Enable GPIOB */
  RCC_APB2PeriphClockCmd( RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOD | RCC_APB2Periph_GPIOA, ENABLE);
  
  //使能DAC模块时钟
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_DAC, ENABLE);

  GPIO_StructInit(&GPIO_InitStructure);
 
  /* Configure PB.6,7 as alternate function output */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_Init(GPIOD, &GPIO_InitStructure);
    
  /* Enable TIM4 clock */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
  //重定向TIM4
  GPIO_PinRemapConfig(GPIO_Remap_TIM4, ENABLE);

  TIM_DeInit(TIM4);

  TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);

  TIM_OCStructInit(&TIM_OCInitStructure);
  
  /* Time base configuration */
  TIM_TimeBaseStructure.TIM_Period = 0x800; //PWM FREQ=35khz         
  TIM_TimeBaseStructure.TIM_Prescaler = 0x0;       
  TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;    
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;   
  TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);

  /* Output Compare PWM Mode configuration: Channel1*/
  TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1; 
  TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;                
  TIM_OCInitStructure.TIM_Pulse = 0x400; //Dummy value;    
  TIM_OC1Init(TIM4, &TIM_OCInitStructure);
  
  /* Output Compare PWM Mode configuration: Channel2 */
  TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;                   
  TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
  TIM_OCInitStructure.TIM_Pulse = 0x400; //Dummy value;    
  TIM_OC2Init(TIM4, &TIM_OCInitStructure);

														    
  //配置TIM4_CC3通道作为TFT_LCD的背光亮度信号。	  //这里没有使能IO口信号。
  TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM2;                   
  TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
  TIM_OCInitStructure.TIM_Pulse = 0x400; //Dummy value 30%;    
  TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_Low;         
  TIM_OC3Init(TIM4, &TIM_OCInitStructure);

  TIM_OC3PreloadConfig(TIM4, TIM_OCPreload_Enable); 

  /* Output Compare PWM Mode configuration: Channel4 */
  
  //配置TIM4_CC4通道作为ADC1,ADC2规则组的触发信号。
  TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;                   
  TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
  TIM_OCInitStructure.TIM_Pulse = 0x400; //Dummy value;    
  TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_Low;         
  TIM_OC4Init(TIM4, &TIM_OCInitStructure);

  TIM_OC4PreloadConfig(TIM4, TIM_OCPreload_Enable); 

 /* TIM1 main Output Enable */
  TIM_CtrlPWMOutputs(TIM4, ENABLE);  
  
  /* Enable TIM4 counter */
  TIM_Cmd(TIM4, ENABLE);

//下面是设置DAC模块

//配置I/O端口
  GPIO_StructInit(&GPIO_InitStructure);
 /* Once the DAC channel is enabled, the corresponding GPIO pin is automatically 
     connected to the DAC converter. In order to avoid parasitic consumption, 
     the GPIO pin should be configured in analog */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
  GPIO_Init(GPIOD, &GPIO_InitStructure);

 //配置DAC模块基本设置
  DAC_StructInit(&DAC_InitStructure);

  /* DAC channel1 Configuration */
  DAC_InitStructure.DAC_Trigger = DAC_Trigger_None;
  DAC_InitStructure.DAC_WaveGeneration = DAC_WaveGeneration_None;
  DAC_InitStructure.DAC_OutputBuffer = DAC_OutputBuffer_Disable;
  DAC_Init(DAC_Channel_1, &DAC_InitStructure);
  /* DAC channel2 Configuration */
  DAC_Init(DAC_Channel_2, &DAC_InitStructure);

  DAC_SetChannel1Data(DAC_Align_12b_R, 0x2048);

  DAC_SetChannel2Data(DAC_Align_12b_R, 0x2048);

  /* Enable DAC Channel1: Once the DAC channel1 is enabled, PA.04 is 
     automatically connected to the DAC converter. */
  DAC_Cmd(DAC_Channel_1, ENABLE);
  /* Enable DAC Channel2: Once the DAC channel2 is enabled, PA.05 is 
     automatically connected to the DAC converter. */
  DAC_Cmd(DAC_Channel_2, ENABLE);

}

/*******************************************************************************
* Function Name : MCDAC_Output
* Description : provides a short description of the function
* Input : details the input parameters.
* Output : details the output parameters.
* Return : details the return value.
*******************************************************************************/
void MCDAC_Update_Output(void)
{	u16 dac_ch1;
    u16 dac_ch2;
	dac_ch1 = (u16)((s16)((hMeasurementArray[OutputVar[bChannel_1_variable]]+32768)/16));
	dac_ch2 = (u16)((s16)((hMeasurementArray[OutputVar[bChannel_2_variable]]+32768)/16));
    TIM_SetCompare1(TIM4, dac_ch1 / 2);
    TIM_SetCompare2(TIM4, dac_ch2 / 2);
    DAC_SetChannel1Data(DAC_Align_12b_R, dac_ch1);
    DAC_SetChannel2Data(DAC_Align_12b_R, dac_ch2);
}

/*******************************************************************************
* Function Name : MCDAC_Send_Output_Value
* Description : provides a short description of the function
* Input : details the input parameters.
* Output : details the output parameters.
* Return : details the return value.
*******************************************************************************/
void MCDAC_Update_Value(u8 bVariable, s16 hValue)
{
  hMeasurementArray[bVariable] = hValue;
}

/*******************************************************************************
* Function Name : MCDAC_Output_Choice
* Description : provides a short description of the function
* Input : details the input parameters.
* Output : details the output parameters.
* Return : details the return value.
*******************************************************************************/
void MCDAC_Output_Choice(s8 bStep, u8 bChannel)
{
  if (bChannel == DAC_CH1)
  {
     bChannel_1_variable += bStep;
     if (bChannel_1_variable > max_out_var_num)
     {
       bChannel_1_variable = 1;
     }
     else if (bChannel_1_variable == 0)
     {
       bChannel_1_variable = max_out_var_num;
     }
  }
  else
  {
     bChannel_2_variable += bStep;
     if (bChannel_2_variable > max_out_var_num)
     {
       bChannel_2_variable = 1;
     }
     else if (bChannel_2_variable == 0)
     {
       bChannel_2_variable = max_out_var_num;
     }
  }
}

/*******************************************************************************
* Function Name : MCDAC_Output_Var_Name
* Description : provides a short description of the function
* Input : details the input parameters.
* Output : details the output parameters.
* Return : details the return value.
*******************************************************************************/
u8 *MCDAC_Output_Var_Name(u8 bChannel)
{
  u8 *temp;
  if (bChannel == DAC_CH1)
  {
    temp = OutputVariableNames[OutputVar[bChannel_1_variable]];
  }
  else
  {
    temp = OutputVariableNames[OutputVar[bChannel_2_variable]];
  }
  return (temp);
}
    
  

/******************* (C) COPYRIGHT 2008 STMicroelectronics *****END OF FILE****/
