/******************** (C) COPYRIGHT 2008 STMicroelectronics ********************
* File Name          : STM32x_svpwm_ics.c
* Author             : IMS Systems Lab
* Date First Issued  : 21/11/07
* Description        : ICS current reading and PWM generation module 
********************************************************************************
* History:
* 21/11/07 v1.0
* 29/05/08 v2.0
* 09/07/08 v2.0.1
* 14/07/08 v2.0.2
* 17/07/08 v2.0.3
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

#include "STM32F10x_MCconf.h"

#ifdef ICS_SENSORS

/* Includes-------------------------------------------------------------------*/
#include "stm32f10x.h"
#include "stm32f10x_svpwm_ics.h"
#include "MC_Globals.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/

#define NB_CONVERSIONS 16

#define SQRT_3		1.732051
#define T		(PWM_PERIOD * 4)
#define T_SQRT3         (u16)(T * SQRT_3)

#define SECTOR_1	(u32)1
#define SECTOR_2	(u32)2
#define SECTOR_3	(u32)3
#define SECTOR_4	(u32)4
#define SECTOR_5	(u32)5
#define SECTOR_6	(u32)6

#define PHASE_A_MSK       (u32)((u32)(PHASE_A_ADC_CHANNEL) << 10)
#define PHASE_B_MSK       (u32)((u32)(PHASE_B_ADC_CHANNEL) << 10)

#define TEMP_FDBK_MSK     (u32)((u32)(TEMP_FDBK_CHANNEL) <<15)
#define BUS_VOLT_FDBK_MSK (u32)((u32)(BUS_VOLT_FDBK_CHANNEL) <<15)
#define SEQUENCE_LENGHT    0x00100000	//该参数只用在3-shnut方式，其他2种方式不用。

#define ADC_PRE_EMPTION_PRIORITY 1
#define ADC_SUB_PRIORITY 0

#define BRK_PRE_EMPTION_PRIORITY 0
#define BRK_SUB_PRIORITY 0

#define DMA_CH1_PRE_EMPTION_PRIORITY  1
#define DMA_CH1_SUB_PRIORITY		  1

#ifdef IR_2101S
#define LOW_SIDE_POLARITY  TIM_OCNIdleState_Reset	   //停车不锁转子,转子可以自由转动
//#define LOW_SIDE_POLARITY  TIM_OCNIdleState_Set	 //停车锁定转子，绕组刹车
#endif
#ifdef IR_2103S
#define LOW_SIDE_POLARITY  TIM_OCNIdleState_Set		   //停车不锁转子,转子可以自由转动
//#define LOW_SIDE_POLARITY  TIM_OCNIdleState_Reset	 //停车锁定转子，绕组刹车
#endif

#define ADC_RIGHT_ALIGNMENT 3

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static u16 hPhaseAOffset;
static u16 hPhaseBOffset;

/* Private function prototypes -----------------------------------------------*/

void SVPWM_IcsInjectedConvConfig(void);

/*******************************************************************************
* Function Name  : SVPWM_IcsInit
* Description    : It initializes PWM and ADC peripherals
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void SVPWM_IcsInit(void)
{ 
  ADC_InitTypeDef ADC_InitStructure;
  TIM_TimeBaseInitTypeDef TIM1_TimeBaseStructure;
  TIM_OCInitTypeDef TIM1_OCInitStructure;
  TIM_BDTRInitTypeDef TIM1_BDTRInitStructure;
  NVIC_InitTypeDef NVIC_InitStructure;
  GPIO_InitTypeDef GPIO_InitStructure;

  DMA_InitTypeDef   DMA_InitStructure;


  /* ADC1, ADC2, DMA, GPIO, TIM1 clocks enabling -----------------------------*/
  
  /* ADCCLK = PCLK2/6 */
  RCC_ADCCLKConfig(RCC_PCLK2_Div6);

  /* Enable DMA clock */
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);   //DMA使用。

  /* Enable ADC1 clock */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);

  /* Enable ADC2 clock */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC2, ENABLE);
  
  /* Enable GPIOA-GPIOC clock */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOA | 
           				 RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOE, ENABLE);
   
  /* Enable TIM1 clock */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);
  
  /* ADC1, ADC2, PWM pins configurations -------------------------------------*/
  GPIO_StructInit(&GPIO_InitStructure);
  /****** Configure phase A ADC channel GPIO as analog input ****/
  GPIO_InitStructure.GPIO_Pin = PHASE_A_GPIO_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
  GPIO_Init(PHASE_A_GPIO_PORT, &GPIO_InitStructure);
 
  /****** Configure phase B ADC channel GPIO as analog input ****/
  GPIO_InitStructure.GPIO_Pin = PHASE_B_GPIO_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
  GPIO_Init(PHASE_B_GPIO_PORT, &GPIO_InitStructure);

  /****** Configure temperature reading ADC channel GPIO as analog input ****/
  GPIO_InitStructure.GPIO_Pin = TEMP_FDBK_CHANNEL_GPIO_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
  GPIO_Init(TEMP_FDBK_CHANNEL_GPIO_PORT, &GPIO_InitStructure);
 
  /****** Configure bus voltage reading ADC channel GPIO as analog input ****/
  GPIO_InitStructure.GPIO_Pin = BUS_VOLT_FDBK_CHANNEL_GPIO_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
  GPIO_Init(BUS_VOLT_FDBK_CHANNEL_GPIO_PORT, &GPIO_InitStructure);
    
  /****** Configure POT1 reading ADC channel GPIO as analog input ****/
  GPIO_InitStructure.GPIO_Pin = POT1_VOLT_FDBK_CHANNEL_GPIO_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
  GPIO_Init(POT1_VOLT_FDBK_CHANNEL_GPIO_PORT, &GPIO_InitStructure);
  
  /****** Configure BUS_SHUNT reading ADC channel GPIO as analog input ****/
  GPIO_InitStructure.GPIO_Pin = BUS_SHUNT_CURR_CHANNEL_GPIO_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
  GPIO_Init(BUS_SHUNT_CURR_CHANNEL_GPIO_PORT, &GPIO_InitStructure);

  /****** Configure BREAK_SHUNT reading ADC channel GPIO as analog input ****/
  GPIO_StructInit(&GPIO_InitStructure);  
  GPIO_InitStructure.GPIO_Pin = BRK_SHUNT_CURR_CHANNEL_GPIO_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
  GPIO_Init(BRK_SHUNT_CURR_CHANNEL_GPIO_PORT, &GPIO_InitStructure);
  
  /****** Configure AIN0 & AIN1 reading ADC channel GPIO as analog input ****/
  GPIO_StructInit(&GPIO_InitStructure);  
  GPIO_InitStructure.GPIO_Pin = AIN0_VOLT_FDBK_CHANNEL_GPIO_PIN | AIN1_VOLT_FDBK_CHANNEL_GPIO_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
  GPIO_Init(AIN0_VOLT_FDBK_CHANNEL_GPIO_PORT, &GPIO_InitStructure);

  /* TIM1 Peripheral Configuration -------------------------------------------*/
  /* TIM1 Registers reset */
  TIM_DeInit(TIM1);
  TIM_TimeBaseStructInit(&TIM1_TimeBaseStructure);
  /* Time Base configuration */
  TIM1_TimeBaseStructure.TIM_Prescaler = PWM_PRSC;
  TIM1_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_CenterAligned1;
  TIM1_TimeBaseStructure.TIM_Period = PWM_PERIOD;
  TIM1_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV2; //设置死区时间和数字滤波器时间。DTS
  TIM1_TimeBaseStructure.TIM_RepetitionCounter = REP_RATE;
  TIM_TimeBaseInit(TIM1, &TIM1_TimeBaseStructure);

  TIM_OCStructInit(&TIM1_OCInitStructure);
  /* Channel 1, 2,3 and 4 Configuration in PWM mode */
  TIM1_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1; 
  TIM1_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable; 
  TIM1_OCInitStructure.TIM_OutputNState = TIM_OutputNState_Enable;                  
  TIM1_OCInitStructure.TIM_Pulse = 0x505; //dummy value
  TIM1_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High; 

//设置使用IR2101S或者IR2103S
#ifdef IR_2101S
  TIM1_OCInitStructure.TIM_OCNPolarity = TIM_OCNPolarity_High;   //IR2101S      
#endif
#ifdef IR_2103S
  TIM1_OCInitStructure.TIM_OCNPolarity = TIM_OCNPolarity_Low;      //IR2103S
#endif

  TIM1_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Reset;
  TIM1_OCInitStructure.TIM_OCNIdleState = LOW_SIDE_POLARITY;   // 使用IR2103的NLIN.不锁转子       
  
  TIM_OC1Init(TIM1, &TIM1_OCInitStructure); 

  TIM1_OCInitStructure.TIM_Pulse = 0x505; //dummy value
  TIM_OC2Init(TIM1, &TIM1_OCInitStructure);

  TIM1_OCInitStructure.TIM_Pulse = 0x505; //dummy value
  TIM_OC3Init(TIM1, &TIM1_OCInitStructure);

//这里设置TIM1_CH4用于ADC1、2触发
//===================================================================
  /*Timer1 alternate function full remapping*/  
  GPIO_PinRemapConfig(GPIO_FullRemap_TIM1,ENABLE);  
  
  GPIO_StructInit(&GPIO_InitStructure);
  /* GPIOE Configuration: Channel 1, 1N, 2, 2N, 3, 3N and 4 Output */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11; 
                                
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOE, &GPIO_InitStructure); 
  
  GPIO_StructInit(&GPIO_InitStructure);
  /* GPIOE Configuration: Channel 1N, 2N, 3N */
  GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_12;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOE, &GPIO_InitStructure); 

    /* Lock GPIOE Pin9 and Pin11 Pin 13 (High sides) */
  GPIO_PinLockConfig(GPIOE, GPIO_Pin_9 | GPIO_Pin_11 | GPIO_Pin_13 );
  
  GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_15;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOE, &GPIO_InitStructure); 
  
  /* Automatic Output enable, Break, dead time and lock configuration*/
  TIM1_BDTRInitStructure.TIM_OSSRState = TIM_OSSRState_Enable;
  TIM1_BDTRInitStructure.TIM_OSSIState = TIM_OSSIState_Enable;
  TIM1_BDTRInitStructure.TIM_LOCKLevel = TIM_LOCKLevel_1; 
  TIM1_BDTRInitStructure.TIM_DeadTime = DEADTIME;
  TIM1_BDTRInitStructure.TIM_Break = TIM_Break_Disable;	//没有使能刹车功能。
  TIM1_BDTRInitStructure.TIM_BreakPolarity = TIM_BreakPolarity_Low;
  TIM1_BDTRInitStructure.TIM_AutomaticOutput = TIM_AutomaticOutput_Disable;

  TIM_BDTRConfig(TIM1, &TIM1_BDTRInitStructure);

  TIM_SelectOutputTrigger(TIM1, TIM_TRGOSource_Update);	  //选择TIM1的触发输出模式，使用更新事件作为触发输出。
  
  TIM_ClearITPendingBit(TIM1, TIM_IT_Break);
  TIM_ITConfig(TIM1, TIM_IT_Break, ENABLE);
  
  /* TIM1 counter enable */
  TIM_Cmd(TIM1, ENABLE);

//设置DMA，用于存储ADC1和ADC2的转换值。
//=================================================================================
  /* DMA1 Channel1 Config */
  DMA_DeInit(DMA1_Channel1);
  DMA_StructInit(&DMA_InitStructure);

  DMA_InitStructure.DMA_PeripheralBaseAddr = ADC1_DR_Address;
  DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)ADC_DualConvertedValueTab;
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
  DMA_InitStructure.DMA_BufferSize = BufferLenght;
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Word;
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Word;
  DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
  DMA_InitStructure.DMA_Priority = DMA_Priority_High;
  DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;

  DMA_Init(DMA1_Channel1, &DMA_InitStructure);
  DMA_ClearITPendingBit(DMA1_IT_TC1);
  DMA_ITConfig(DMA1_Channel1, DMA_IT_TC, ENABLE);

  /* DMA1 Channel1 enable */
  DMA_Cmd(DMA1_Channel1, ENABLE);
//=====================================================================================
  
  /* ADC1 registers reset ----------------------------------------------------*/
  ADC_DeInit(ADC1);
  /* ADC2 registers reset ----------------------------------------------------*/
  ADC_DeInit(ADC2);
//============================================================================
  
  /* ADC1 configuration ------------------------------------------------------*/
  ADC_StructInit(&ADC_InitStructure);
  ADC_InitStructure.ADC_Mode = ADC_Mode_RegInjecSimult;  //ADC1和ADC2工作在混合同步规则及注入模式
  ADC_InitStructure.ADC_ScanConvMode = ENABLE;
  ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
  ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T4_CC4; //非外部触发，先用软件触发
  ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Left;	 //数据左对齐。
  ADC_InitStructure.ADC_NbrOfChannel = 3;	//BUS_SHUNT+BREAK_SHUNT+Chip Temp
  ADC_Init(ADC1, &ADC_InitStructure);

  ADC_DMACmd(ADC1, ENABLE);   //如何把数据正确取出来？？？，使用DMA中断吧。
   
  /* ADC2 Configuration ------------------------------------------------------*/
  ADC_StructInit(&ADC_InitStructure);  
  ADC_InitStructure.ADC_Mode = ADC_Mode_RegInjecSimult;  //ADC1和ADC2工作在混合同步规则及注入模式
  ADC_InitStructure.ADC_ScanConvMode = ENABLE;
  ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
  ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
  ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Left;
  ADC_InitStructure.ADC_NbrOfChannel = 3;  //POT1+AIN0+AIN1
  ADC_Init(ADC2, &ADC_InitStructure);

  ADC_ExternalTrigConvCmd(ADC2, ENABLE);

//  ADC_SoftwareStartConvCmd(ADC1, DISABLE);		  //先关闭软件触发规则组，连续扫描


//下面分别设置规则通道组和注入通道组
  /* Temp On CPU  */
  ADC_RegularChannelConfig(ADC1, ADC_Channel_16, 1, ADC_SampleTime_239Cycles5); 
  /* ADC1 Regular Channel BREAK_SHUNT */ 
  ADC_RegularChannelConfig(ADC1, BRK_SHUNT_CURR_CHANNEL,  2, ADC_SampleTime_239Cycles5); 
  /* ADC1 Regular Channel BUS_SHUNT  */
  ADC_RegularChannelConfig(ADC1, BUS_SHUNT_CURR_CHANNEL, 3, ADC_SampleTime_239Cycles5); 

  /* ADC2 Regular Channel POT1 */ 
  ADC_RegularChannelConfig(ADC2, POT1_VOLT_FDBK_CHANNEL,  1, ADC_SampleTime_239Cycles5); 
  /* ADC2 Regular Channel AIN0 */ 
  ADC_RegularChannelConfig(ADC2, AIN0_VOLT_FDBK_CHANNEL,  2, ADC_SampleTime_239Cycles5); 
   /* ADC1 Regular Channel AIN1 */ 
  ADC_RegularChannelConfig(ADC2, AIN1_VOLT_FDBK_CHANNEL,  3, ADC_SampleTime_239Cycles5); 

//=============================================================================

  /* Enable ADC1 */
  ADC_Cmd(ADC1, ENABLE);	 //唤醒ADC1

  ADC_TempSensorVrefintCmd(ENABLE);    // Chanel 16 = Temp On Chip

  //下面是校正ADC1和ADC2 
//==============================================================================
  /* Enable ADC1 reset calibaration register */   
  ADC_ResetCalibration(ADC1);
  
  /* Check the end of ADC1 reset calibration register */
  while(ADC_GetResetCalibrationStatus(ADC1));

  /* Start ADC1 calibaration */
  ADC_StartCalibration(ADC1);
  
  /* Check the end of ADC1 calibration */
  while(ADC_GetCalibrationStatus(ADC1));

 /* Enable ADC2 */
  ADC_Cmd(ADC2, ENABLE);   //唤醒ADC2

  /* Enable ADC2 reset calibaration register */   
  ADC_ResetCalibration(ADC2);
  
  /* Check the end of ADC2 reset calibration register */
  while(ADC_GetResetCalibrationStatus(ADC2));

  /* Start ADC2 calibaration */
  ADC_StartCalibration(ADC2);
  
  /* Check the end of ADC2 calibration */
  while(ADC_GetCalibrationStatus(ADC2));
 //================================================================================== 

  /* ADC1 Injected conversions configuration */ 
  ADC_InjectedSequencerLengthConfig(ADC1,2);
  SVPWM_IcsCurrentReadingCalibration();
    
  /* ADC2 Injected conversions configuration */ 
  ADC_InjectedSequencerLengthConfig(ADC2,2);

  ADC_InjectedChannelConfig(ADC2, PHASE_B_ADC_CHANNEL, 1, 
                                                      SAMPLING_TIME_CK);
  ADC_InjectedChannelConfig(ADC2, TEMP_FDBK_CHANNEL, 2,
                                                      SAMPLING_TIME_CK);
  
  ADC_ExternalTrigInjectedConvCmd(ADC2,ENABLE);

  
  /* Configure TWO bit for preemption priority */
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);	//4个抢先级、4个子优先级
  
//  NVIC_StructInit(&NVIC_InitStructure);
  /* Enable the ADC Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = ADC1_2_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = ADC_PRE_EMPTION_PRIORITY;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = ADC_SUB_PRIORITY;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

  /* Enable the TIM1 BRK Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = TIM1_BRK_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = BRK_PRE_EMPTION_PRIORITY;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = BRK_SUB_PRIORITY;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;		//没有打开紧急停车功能
  NVIC_Init(&NVIC_InitStructure); 
    
  /* Enable the DMA_CHANEL1 Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel1_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = DMA_CH1_PRE_EMPTION_PRIORITY;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = DMA_CH1_SUB_PRIORITY;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

//  ADC_SoftwareStartConvCmd(ADC1, ENABLE);		  //首先软件触发规则组，连续扫描
  /* ADC1、ADC2 regular conversions trigger is TIM4_CC4*/ 
  ADC_ExternalTrigConvCmd(ADC1, ENABLE);
  ADC_ExternalTrigConvCmd(ADC2, ENABLE);

} 

/*******************************************************************************
* Function Name  : SVPWM_IcsCurrentReadingCalibration
* Description    : Store zero current converted values for current reading 
                   network offset compensation in case of Ics 
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/

void SVPWM_IcsCurrentReadingCalibration(void)
{
 static u8 bIndex;
   
  /* ADC1 Injected group of conversions end interrupt disabling */
  ADC_ITConfig(ADC1, ADC_IT_JEOC, DISABLE);
  
  hPhaseAOffset=0;
  hPhaseBOffset=0;
   
  /* ADC1 Injected conversions trigger is given by software and enabled */ 
  ADC_ExternalTrigInjectedConvConfig(ADC1, ADC_ExternalTrigInjecConv_None);  
  ADC_ExternalTrigInjectedConvCmd(ADC1,ENABLE); 
  
  /* ADC1 Injected conversions configuration */   
  ADC_InjectedChannelConfig(ADC1, PHASE_A_ADC_CHANNEL,1,SAMPLING_TIME_CK);
  ADC_InjectedChannelConfig(ADC1, PHASE_B_ADC_CHANNEL,2,SAMPLING_TIME_CK);
   
  /* Clear the ADC1 JEOC pending flag */
  ADC_ClearFlag(ADC1, ADC_FLAG_JEOC);   
  ADC_SoftwareStartInjectedConvCmd(ADC1,ENABLE);
  
  /* ADC Channel used for current reading are read 
     in order to get zero currents ADC values*/ 
  for(bIndex=NB_CONVERSIONS; bIndex !=0; bIndex--)
  {
    while(!ADC_GetFlagStatus(ADC1,ADC_FLAG_JEOC)) { }

  //求Q1.15格式的零电流值，16个（零电流值/8）的累加，把最高符号位溢出。  
    hPhaseAOffset += (ADC_GetInjectedConversionValue(ADC1,ADC_InjectedChannel_1)
                                                         >>ADC_RIGHT_ALIGNMENT);
    hPhaseBOffset += (ADC_GetInjectedConversionValue(ADC1,ADC_InjectedChannel_2)
                                                         >>ADC_RIGHT_ALIGNMENT);  
    /* Clear the ADC1 JEOC pending flag */
    ADC_ClearFlag(ADC1, ADC_FLAG_JEOC); 	
    ADC_SoftwareStartInjectedConvCmd(ADC1,ENABLE);
  }
  
  SVPWM_IcsInjectedConvConfig();  
}


/*******************************************************************************
* Function Name  : SVPWM_IcsInjectedConvConfig
* Description    : This function configure ADC1 for ICS current 
*                  reading and temperature and voltage feedbcak after a 
*                  calibration of the utilized ADC Channels for current reading
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void SVPWM_IcsInjectedConvConfig(void)
{
  /* ADC1 Injected conversions configuration */ 
  ADC_InjectedChannelConfig(ADC1, PHASE_A_ADC_CHANNEL, 1, 
                                                      SAMPLING_TIME_CK);
  ADC_InjectedChannelConfig(ADC1, BUS_VOLT_FDBK_CHANNEL, 
                                                   2, SAMPLING_TIME_CK);
  
  /* ADC1 Injected conversions trigger is TIM1 TRGO */ 
  ADC_ExternalTrigInjectedConvConfig(ADC1, ADC_ExternalTrigInjecConv_T1_TRGO); 
  
  /* ADC1 Injected group of conversions end interrupt enabling */
  //  这里才能打开注入AD中断
  ADC_ITConfig(ADC1, ADC_IT_JEOC, ENABLE);
}

/*******************************************************************************
* Function Name  : SVPWM_IcsPhaseCurrentValues
* Description    : This function computes current values of Phase A and Phase B 
*                 in q1.15 format starting from values acquired from the A/D 
*                 Converter peripheral.
* Input          : None
* Output         : Stat_Curr_a_b
* Return         : None
*******************************************************************************/
Curr_Components SVPWM_IcsGetPhaseCurrentValues(void)
{
  Curr_Components Local_Stator_Currents;
  s32 wAux;

      
 // Ia = (hPhaseAOffset)-(PHASE_A_ADC_CHANNEL vale) 
 //零电流减去A/D转换的值去掉最高的符号位。得到Q1.15格式的电流值。 
  wAux = (s32)(hPhaseAOffset)-((ADC1->JDR1)<<1);          
 //Saturation of Ia 
  if (wAux < S16_MIN)
  {
    Local_Stator_Currents.qI_Component1= S16_MIN;
  }  
  else  if (wAux > S16_MAX)
        { 
          Local_Stator_Currents.qI_Component1= S16_MAX;
        }
        else
        {
          Local_Stator_Currents.qI_Component1= wAux;
        }
                     
 // Ib = (hPhaseBOffset)-(PHASE_B_ADC_CHANNEL value)
  wAux = (s32)(hPhaseBOffset)-((ADC2->JDR1)<<1);
 // Saturation of Ib
  if (wAux < S16_MIN)
  {
    Local_Stator_Currents.qI_Component2= S16_MIN;
  }  
  else  if (wAux > S16_MAX)
        { 
          Local_Stator_Currents.qI_Component2= S16_MAX;
        }
        else
        {
          Local_Stator_Currents.qI_Component2= wAux;
        }
  
  return(Local_Stator_Currents); 
}

/*******************************************************************************
* Function Name  : SVPWM_IcsCalcDutyCycles
* Description    : Computes duty cycle values corresponding to the input value
		   and configures 
* Input          : Stat_Volt_alfa_beta
* Output         : None
* Return         : None
*******************************************************************************/

void SVPWM_IcsCalcDutyCycles (Volt_Components Stat_Volt_Input)
{
   u8 bSector;
   s32 wX, wY, wZ, wUAlpha, wUBeta;
   u16  hTimePhA=0, hTimePhB=0, hTimePhC=0;		//下面是REV_CLARK变换过程
    
   wUAlpha = Stat_Volt_Input.qV_Component1 * T_SQRT3 ;
   wUBeta = -(Stat_Volt_Input.qV_Component2 * T);

   wX = wUBeta;
   wY = (wUBeta + wUAlpha)/2;
   wZ = (wUBeta - wUAlpha)/2;
									   //上面是REV_CLARK变换过程
  // Sector calculation from wX, wY, wZ	  //下面是查找定子电流的扇区号
   if (wY<0)
   {
      if (wZ<0)
      {
        bSector = SECTOR_5;
      }
      else // wZ >= 0
        if (wX<=0)
        {
          bSector = SECTOR_4;
        }
        else // wX > 0
        {
          bSector = SECTOR_3;
        }
   }
   else // wY > 0
   {
     if (wZ>=0)
     {
       bSector = SECTOR_2;
     }
     else // wZ < 0
       if (wX<=0)
       {  
         bSector = SECTOR_6;
       }
       else // wX > 0
       {
         bSector = SECTOR_1;
       }
    }							   //上面是查找定子电流的扇区号
   
   /* Duty cycles computation */
  
  switch(bSector)		  //根据所在扇区号，计算三相占空比
  {  
    case SECTOR_1:
    case SECTOR_4:
                hTimePhA = (T/8) + ((((T + wX) - wZ)/2)/131072);
		hTimePhB = hTimePhA + wZ/131072;
		hTimePhC = hTimePhB - wX/131072;                                       
                break;
    case SECTOR_2:
    case SECTOR_5:  
                hTimePhA = (T/8) + ((((T + wY) - wZ)/2)/131072);
        	hTimePhB = hTimePhA + wZ/131072;
		hTimePhC = hTimePhA - wY/131072;
                break;

    case SECTOR_3:
    case SECTOR_6:
                hTimePhA = (T/8) + ((((T - wX) + wY)/2)/131072);
		hTimePhC = hTimePhA - wY/131072;
		hTimePhB = hTimePhC + wX/131072;
                break;
    default:
		break;
   }
  
  /* Load compare registers values */
   
  TIM1->CCR1 = hTimePhA;
  TIM1->CCR2 = hTimePhB;
  TIM1->CCR3 = hTimePhC;
}

/*******************************************************************************
* Function Name  : SVPWMEOCEvent
* Description    :  Routine to be performed inside the end of conversion ISR
* Input           : None
* Output         : None
* Return         : None
*******************************************************************************/
u8 SVPWMEOCEvent(void)
{
  // Store the Bus Voltage and temperature sampled values
  h_ADCTemp = ADC_GetInjectedConversionValue(ADC2,ADC_InjectedChannel_2);
  h_ADCBusvolt = ADC_GetInjectedConversionValue(ADC1,ADC_InjectedChannel_2);
  return ((u8)(1));
}
#endif //ICS_SENSORS
/******************* (C) COPYRIGHT 2008 STMicroelectronics *****END OF FILE****/  
