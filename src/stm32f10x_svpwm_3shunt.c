/******************** (C) COPYRIGHT 2008 STMicroelectronics ********************
* File Name          : STM32x_svpwm_3shunt.c
* Author             : IMS Systems Lab
* Date First Issued  : 28/11/07
* Description        : 3 shunt resistors current reading module
********************************************************************************
* History:
* 28/11/07 v1.0
* 29/05/08 v2.0
* 03/07/08 v2.0.1
* 09/07/08 v2.0.2
* 11/07/08 v2.0.3
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

#ifdef THREE_SHUNT

/* Includes-------------------------------------------------------------------*/
#include "stm32f10x.h"
#include "stm32f10x_svpwm_3shunt.h"
#include "MC_Globals.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/

#define NB_CONVERSIONS 16

#define SQRT_3		1.732051
#define T		    (PWM_PERIOD * 4)
#define T_SQRT3     (u16)(T * SQRT_3)

#define SECTOR_1	(u32)1
#define SECTOR_2	(u32)2
#define SECTOR_3	(u32)3
#define SECTOR_4	(u32)4
#define SECTOR_5	(u32)5
#define SECTOR_6	(u32)6

#define PHASE_A_ADC_CHANNEL     ADC_Channel_11
#define PHASE_B_ADC_CHANNEL     ADC_Channel_10
#define PHASE_C_ADC_CHANNEL     ADC_Channel_6

// Setting for sampling of VBUS and Temp after currents sampling
#define PHASE_A_MSK       (u32)((u32)(PHASE_A_ADC_CHANNEL) << 10)
#define PHASE_B_MSK       (u32)((u32)(PHASE_B_ADC_CHANNEL) << 10)
#define PHASE_C_MSK       (u32)((u32)(PHASE_C_ADC_CHANNEL) << 10)

// Settings for current sampling only
/*#define PHASE_A_MSK       (u32)((u32)(PHASE_A_ADC_CHANNEL) << 15)
#define PHASE_B_MSK       (u32)((u32)(PHASE_B_ADC_CHANNEL) << 15)
#define PHASE_C_MSK       (u32)((u32)(PHASE_C_ADC_CHANNEL) << 15)*/

// Setting for sampling of VBUS and Temp after currents sampling
#define TEMP_FDBK_MSK     (u32)((u32)(TEMP_FDBK_CHANNEL) <<15)
#define BUS_VOLT_FDBK_MSK (u32)((u32)(BUS_VOLT_FDBK_CHANNEL) <<15)

// Settings for current sampling only
//#define TEMP_FDBK_MSK     (u32)(0)
//#define BUS_VOLT_FDBK_MSK (u32)(0)

// Setting for sampling of VBUS and Temp after currents sampling
#define SEQUENCE_LENGHT    0x00100000

// Settings for current sampling only
//#define SEQUENCE_LENGHT    0x00000000

#define ADC_PRE_EMPTION_PRIORITY 1
#define ADC_SUB_PRIORITY 0

#define BRK_PRE_EMPTION_PRIORITY 0
#define BRK_SUB_PRIORITY 0

#define TIM1_UP_PRE_EMPTION_PRIORITY 1
#define TIM1_UP_SUB_PRIORITY 0

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

#define PWM2_MODE 0
#define PWM1_MODE 1

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
u8  bSector;  

u16 hPhaseAOffset;
u16 hPhaseBOffset;
u16 hPhaseCOffset;

u8 PWM4Direction=PWM2_MODE;

/* Private function prototypes -----------------------------------------------*/

void SVPWM_InjectedConvConfig(void);

/*******************************************************************************
* Function Name  : SVPWM_3ShuntInit
* Description    : It initializes PWM and ADC peripherals
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void SVPWM_3ShuntInit(void)
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
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
  
  /* Enable GPIOA, GPIOC, GPIOE, AFIO clocks */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOE |
  						 RCC_APB2Periph_GPIOC , ENABLE);
  /* Enable ADC1 clock */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);

  /* Enable ADC2 clock */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC2, ENABLE); 
   
  /* Enable TIM1 clock */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);
     
  /* ADC1, ADC2, PWM pins configurations -------------------------------------*/
  GPIO_StructInit(&GPIO_InitStructure);
  /****** Configure PC.00,01,2,3,4,5 (ADC Channels [10..15]) as analog input ****/
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
  GPIO_Init(GPIOC, &GPIO_InitStructure);
     
  GPIO_StructInit(&GPIO_InitStructure);
  /****** Configure PA.06,07(ADC Channels [6..7]) as analog input ****/
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7 ;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

   GPIO_StructInit(&GPIO_InitStructure);
  /****** Configure PB.00,01(ADC Channels [8..9]) as AIN0 、AIN1 ****/
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 ;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
  GPIO_Init(GPIOB, &GPIO_InitStructure);


  /* TIM1 Peripheral Configuration -------------------------------------------*/
  /* TIM1 Registers reset */
  TIM_DeInit(TIM1);
  TIM_TimeBaseStructInit(&TIM1_TimeBaseStructure);
  /* Time Base configuration */
  TIM1_TimeBaseStructure.TIM_Prescaler = 0x0;
  TIM1_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_CenterAligned1;//TIM1 中央对齐模式 1 只在计数器向下计数时触发中断 
  TIM1_TimeBaseStructure.TIM_Period = PWM_PERIOD;
  TIM1_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV2;//设置死区时间和数字滤波器时间。DTS
  
  // Initial condition is REP=0 to set the UPDATE only on the underflow
  TIM1_TimeBaseStructure.TIM_RepetitionCounter = REP_RATE;
  TIM_TimeBaseInit(TIM1, &TIM1_TimeBaseStructure);
  
  TIM_OCStructInit(&TIM1_OCInitStructure);
  /* Channel 1, 2,3 in PWM mode */
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
  TIM1_OCInitStructure.TIM_OCNIdleState = LOW_SIDE_POLARITY;          
  
  TIM_OC1Init(TIM1, &TIM1_OCInitStructure); 
  TIM_OC2Init(TIM1, &TIM1_OCInitStructure);
  TIM_OC3Init(TIM1, &TIM1_OCInitStructure);
  
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

  
  TIM_OCStructInit(&TIM1_OCInitStructure);
  /* Channel 4 Configuration in OC */
  TIM1_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM2;   //和1.2.3通道的PWM1模式正好相反，参考
  													  //	TIM1_CCMR1寄存器的4-6位。	
  TIM1_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable; 
  TIM1_OCInitStructure.TIM_OutputNState = TIM_OutputNState_Disable;                  
  TIM1_OCInitStructure.TIM_Pulse = PWM_PERIOD - 1; 
  
  TIM1_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High; 
  TIM1_OCInitStructure.TIM_OCNPolarity = TIM_OCNPolarity_Low;         
  TIM1_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Reset;
  TIM1_OCInitStructure.TIM_OCNIdleState = LOW_SIDE_POLARITY;            
  
  TIM_OC4Init(TIM1, &TIM1_OCInitStructure);
  
  /* Enables the TIM1 Preload on CC1 Register */
  TIM_OC1PreloadConfig(TIM1, TIM_OCPreload_Enable);
  /* Enables the TIM1 Preload on CC2 Register */
  TIM_OC2PreloadConfig(TIM1, TIM_OCPreload_Enable);
  /* Enables the TIM1 Preload on CC3 Register */
  TIM_OC3PreloadConfig(TIM1, TIM_OCPreload_Enable);
  /* Enables the TIM1 Preload on CC4 Register */
  TIM_OC4PreloadConfig(TIM1, TIM_OCPreload_Enable);

  /* Automatic Output enable, Break, dead time and lock configuration*/
  TIM1_BDTRInitStructure.TIM_OSSRState = TIM_OSSRState_Enable;
  TIM1_BDTRInitStructure.TIM_OSSIState = TIM_OSSIState_Enable;
  TIM1_BDTRInitStructure.TIM_LOCKLevel = TIM_LOCKLevel_1; 
  TIM1_BDTRInitStructure.TIM_DeadTime = DEADTIME;
  TIM1_BDTRInitStructure.TIM_Break = TIM_Break_Disable;	//没有打开紧急停车功能 
  														//NVIC中断向量也要关闭
  TIM1_BDTRInitStructure.TIM_BreakPolarity = TIM_BreakPolarity_Low;
  TIM1_BDTRInitStructure.TIM_AutomaticOutput = TIM_AutomaticOutput_Disable;

  TIM_BDTRConfig(TIM1, &TIM1_BDTRInitStructure);

  TIM_SelectOutputTrigger(TIM1, TIM_TRGOSource_Update);
  
  TIM_ClearITPendingBit(TIM1, TIM_IT_Break);
  TIM_ITConfig(TIM1, TIM_IT_Break,ENABLE);
  
  /* TIM1 counter enable */
  TIM_Cmd(TIM1, ENABLE);
  
  // Resynch to have the Update evend during Undeflow
  TIM_GenerateEvent(TIM1, TIM_EventSource_Update);
  
  // Clear Update Flag
  TIM_ClearFlag(TIM1, TIM_FLAG_Update);
  
  TIM_ITConfig(TIM1, TIM_IT_Update, DISABLE);
  
  TIM_ITConfig(TIM1, TIM_IT_CC4,DISABLE);

//设置DMA，用于存储ADC1和ADC2的规则组转换值。
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
  
  /* Enable ADC1 */
  ADC_Cmd(ADC1, ENABLE);
  /* Enable ADC2 */
  ADC_Cmd(ADC2, ENABLE);
  
  /* ADC1 configuration ------------------------------------------------------*/
  ADC_StructInit(&ADC_InitStructure);
  ADC_InitStructure.ADC_Mode = ADC_Mode_RegInjecSimult;  //ADC1和ADC2工作在混合同步规则及注入模式
  ADC_InitStructure.ADC_ScanConvMode = ENABLE;
  ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
  ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T4_CC4; //非外部触发，先用软件触发
  ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Left;
  ADC_InitStructure.ADC_NbrOfChannel = 3;		 //BUS_SHUNT+BREAK_SHUNT+Chip Temp
  ADC_Init(ADC1, &ADC_InitStructure);

  ADC_DMACmd(ADC1, ENABLE);   //如何把数据正确取出来？？？，使用DMA中断吧。
   
  /* ADC2 Configuration ------------------------------------------------------*/
  ADC_StructInit(&ADC_InitStructure);  
  ADC_InitStructure.ADC_Mode = ADC_Mode_RegInjecSimult;  //ADC1和ADC2工作在混合同步规则及注入模式
  ADC_InitStructure.ADC_ScanConvMode = ENABLE;
  ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
  ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T4_CC4; //非外部触发，先用软件触发
  ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Left;
  ADC_InitStructure.ADC_NbrOfChannel = 3;		  //BUS_SHUNT+BREAK_SHUNT+Chip Temp
  ADC_Init(ADC2, &ADC_InitStructure);
  
  ADC_ExternalTrigConvCmd(ADC2, ENABLE);

  // ADC_SoftwareStartConvCmd(ADC1, DISABLE);		  //先关闭软件触发规则组，连续扫描
  
  //下面分别设置规则通道组和注入通道组
  /* Temp On CPU  */
  ADC_RegularChannelConfig(ADC1, ADC_Channel_16, 1, ADC_SampleTime_239Cycles5); 
  /* ADC1 Regular Channel BREAK_SHUNT */ 
  ADC_RegularChannelConfig(ADC1, BRK_SHUNT_CURR_CHANNEL,  2, ADC_SampleTime_239Cycles5); 
  /* ADC1 Regular Channel BUS_SHUNT  */
  ADC_RegularChannelConfig(ADC1, BUS_SHUNT_CURR_CHANNEL,  3, ADC_SampleTime_239Cycles5); 

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
  
  SVPWM_3ShuntCurrentReadingCalibration();
    
  /* Configure one bit for preemption priority */
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
  
//  NVIC_StructInit(&NVIC_InitStructure);
  /* Enable the ADC Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = ADC1_2_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = ADC_PRE_EMPTION_PRIORITY;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = ADC_SUB_PRIORITY;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
  
  /* Enable the Update Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = TIM1_UP_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = TIM1_UP_PRE_EMPTION_PRIORITY;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = TIM1_UP_SUB_PRIORITY;
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
* Function Name  : SVPWM_3ShuntCurrentReadingCalibration
* Description    : Store zero current converted values for current reading 
                   network offset compensation in case of 3 shunt resistors 
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/

void SVPWM_3ShuntCurrentReadingCalibration(void)
{
  static u16 bIndex;
  
  /* ADC1 Injected group of conversions end interrupt disabling */
  ADC_ITConfig(ADC1, ADC_IT_JEOC, DISABLE);
  
  hPhaseAOffset=0;
  hPhaseBOffset=0;
  hPhaseCOffset=0;
  
  /* ADC1 Injected conversions trigger is given by software and enabled */ 
  ADC_ExternalTrigInjectedConvConfig(ADC1, ADC_ExternalTrigInjecConv_None);  
  ADC_ExternalTrigInjectedConvCmd(ADC1,ENABLE); 
  
  /* ADC1 Injected conversions configuration */ 
  ADC_InjectedSequencerLengthConfig(ADC1,3);
  ADC_InjectedChannelConfig(ADC1, PHASE_A_ADC_CHANNEL,1,SAMPLING_TIME_CK);
  ADC_InjectedChannelConfig(ADC1, PHASE_B_ADC_CHANNEL,2,SAMPLING_TIME_CK);
  ADC_InjectedChannelConfig(ADC1, PHASE_C_ADC_CHANNEL,3,SAMPLING_TIME_CK);
  
  /* Clear the ADC1 JEOC pending flag */
  ADC_ClearFlag(ADC1, ADC_FLAG_JEOC);  
  ADC_SoftwareStartInjectedConvCmd(ADC1,ENABLE);
   
  /* ADC Channel used for current reading are read 
     in order to get zero currents ADC values*/ 
  for(bIndex=0; bIndex <NB_CONVERSIONS; bIndex++)
  {
    while(!ADC_GetFlagStatus(ADC1,ADC_FLAG_JEOC)) { }
    
    hPhaseAOffset += (ADC_GetInjectedConversionValue(ADC1,ADC_InjectedChannel_1)>>3);
    hPhaseBOffset += (ADC_GetInjectedConversionValue(ADC1,ADC_InjectedChannel_2)>>3);
    hPhaseCOffset += (ADC_GetInjectedConversionValue(ADC1,ADC_InjectedChannel_3)>>3);    
        
    /* Clear the ADC1 JEOC pending flag */
    ADC_ClearFlag(ADC1, ADC_FLAG_JEOC);    
    ADC_SoftwareStartInjectedConvCmd(ADC1,ENABLE);
  }
  
  SVPWM_InjectedConvConfig();  
}



/*******************************************************************************
* Function Name  : SVPWM_InjectedConvConfig
* Description    : This function configure ADC1 for 3 shunt current 
*                  reading and temperature and voltage feedbcak after a 
*                  calibration of the three utilized ADC Channels
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void SVPWM_InjectedConvConfig(void)
{
  /* ADC1 Injected conversions configuration */ 
  ADC_InjectedSequencerLengthConfig(ADC1,2);
  
  ADC_InjectedChannelConfig(ADC1, PHASE_B_ADC_CHANNEL, 1, 
                                                      SAMPLING_TIME_CK);
  ADC_InjectedChannelConfig(ADC1, BUS_VOLT_FDBK_CHANNEL, 
                                                   2, SAMPLING_TIME_CK);
    
  /* ADC1 Injected conversions trigger is TIM1 TRGO */ 
  ADC_ExternalTrigInjectedConvConfig(ADC1, ADC_ExternalTrigInjecConv_T1_TRGO); 
  
  /* ADC2 Injected conversions configuration */ 
  ADC_InjectedSequencerLengthConfig(ADC2,2);
  
  ADC_InjectedChannelConfig(ADC2, PHASE_A_ADC_CHANNEL, 1, 
                                                      SAMPLING_TIME_CK);
  ADC_InjectedChannelConfig(ADC2, TEMP_FDBK_CHANNEL, 2,
                                                      SAMPLING_TIME_CK);
  ADC_ExternalTrigInjectedConvCmd(ADC2,ENABLE);

  /* Bus voltage protection initialization*/                            
  ADC_AnalogWatchdogCmd(ADC1,ADC_AnalogWatchdog_SingleInjecEnable);
  ADC_AnalogWatchdogSingleChannelConfig(ADC1,BUS_VOLT_FDBK_CHANNEL);
  ADC_AnalogWatchdogThresholdsConfig(ADC1, OVERVOLTAGE_THRESHOLD>>3,0x00);
   
  /* ADC1 Injected group of conversions end and Analog Watchdog interrupts
                                                                     enabling */
  ADC_ITConfig(ADC1, ADC_IT_JEOC | ADC_IT_AWD, ENABLE);
}

/*******************************************************************************
* Function Name  : SVPWM_3ShuntGetPhaseCurrentValues
* Description    : This function computes current values of Phase A and Phase B 
*                 in q1.15 format starting from values acquired from the A/D 
*                 Converter peripheral.
* Input          : None
* Output         : Stat_Curr_a_b
* Return         : None
*******************************************************************************/
Curr_Components SVPWM_3ShuntGetPhaseCurrentValues(void)
{
  Curr_Components Local_Stator_Currents;
  s32 wAux;

  switch (bSector)
   {
   case 4:
   case 5: //Current on Phase C not accessible     
           // Ia = (hPhaseAOffset)-(ADC Channel 11 value)    
            wAux = (s32)(hPhaseAOffset)- ((ADC1->JDR1)<<1);          
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
                     
           // Ib = (hPhaseBOffset)-(ADC Channel 12 value)
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
           break;
           
   case 6:
   case 1:  //Current on Phase A not accessible     
            // Ib = (hPhaseBOffset)-(ADC Channel 12 value)
            wAux = (s32)(hPhaseBOffset)-((ADC1->JDR1)<<1);
            //Saturation of Ib 
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
            // Ia = -Ic -Ib 
            wAux = ((ADC2->JDR1)<<1)-hPhaseCOffset-
                                            Local_Stator_Currents.qI_Component2;
            //Saturation of Ia
            if (wAux> S16_MAX)
            {
               Local_Stator_Currents.qI_Component1 = S16_MAX;
            }
            else  if (wAux <S16_MIN)
                  {
                   Local_Stator_Currents.qI_Component1 = S16_MIN;
                  }
                  else
                  {  
                    Local_Stator_Currents.qI_Component1 = wAux;
                  }
           break;
           
   case 2:
   case 3:  // Current on Phase B not accessible
            // Ia = (hPhaseAOffset)-(ADC Channel 11 value)     
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
     
            // Ib = -Ic-Ia;
            wAux = ((ADC2->JDR1)<<1) - hPhaseCOffset - 
                                            Local_Stator_Currents.qI_Component1;
            // Saturation of Ib
            if (wAux> S16_MAX)
            {
              Local_Stator_Currents.qI_Component2=S16_MAX;
            }
            else  if (wAux <S16_MIN)
                  {  
                    Local_Stator_Currents.qI_Component2 = S16_MIN;
                  }
                  else  
                  {
                    Local_Stator_Currents.qI_Component2 = wAux;
                  }                     
           break;

   default:
           break;
   } 
  
  return(Local_Stator_Currents); 
}

/*******************************************************************************
* Function Name  : SVPWM_3ShuntCalcDutyCycles
* Description    : Computes duty cycle values corresponding to the input value
		   and configures the AD converter and TIM0 for next period 
		   current reading conversion synchronization
* Input          : Stat_Volt_alfa_beta
* Output         : None
* Return         : None
*******************************************************************************/

void SVPWM_3ShuntCalcDutyCycles (Volt_Components Stat_Volt_Input)
{
   s32 wX, wY, wZ, wUAlpha, wUBeta;
   u16  hTimePhA=0, hTimePhB=0, hTimePhC=0, hTimePhD=0;
   u16  hDeltaDuty;
    
   wUAlpha = Stat_Volt_Input.qV_Component1 * T_SQRT3 ;
   wUBeta = -(Stat_Volt_Input.qV_Component2 * T);

   wX = wUBeta;
   wY = (wUBeta + wUAlpha)/2;
   wZ = (wUBeta - wUAlpha)/2;
   
  // Sector calculation from wX, wY, wZ
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
    }
   
   /* Duty cycles computation */
  PWM4Direction=PWM2_MODE;
    
  switch(bSector)
  {  
    case SECTOR_1:
        hTimePhA = (T/8) + ((((T + wX) - wZ)/2)/131072);
				hTimePhB = hTimePhA + wZ/131072;
				hTimePhC = hTimePhB - wX/131072;
                
                // ADC Syncronization setting value             
                if ((u16)(PWM_PERIOD-hTimePhA) > TW_AFTER)
                {
                  hTimePhD = PWM_PERIOD - 1;
                }
                else
                {
                  hDeltaDuty = (u16)(hTimePhA - hTimePhB);
                  
				  // Definition of crossing point
                  if (hDeltaDuty > (u16)(PWM_PERIOD-hTimePhA)*2) 
                  {
                      hTimePhD = hTimePhA - TW_BEFORE; // Ts before Phase A 
                  }
                  else
                  {
                      hTimePhD = hTimePhA + TW_AFTER; // DT + Tn after Phase A
                     
                    if (hTimePhD >= PWM_PERIOD)
                    {
                      // Trigger of ADC at Falling Edge PWM4
                      // OCR update
                      
                      //Set Polarity of CC4 Low
                      PWM4Direction=PWM1_MODE;
                      
                      hTimePhD = (2 * PWM_PERIOD) - hTimePhD-1;
                    }
                  }
                }
                
                // ADC_InjectedChannelConfig(ADC1, PHASE_B_CHANNEL,1,
                //                                    SAMPLING_TIME_CK);               
                ADC1->JSQR = PHASE_B_MSK + BUS_VOLT_FDBK_MSK + SEQUENCE_LENGHT;
                //ADC_InjectedChannelConfig(ADC2, PHASE_C_CHANNEL,1,
                //                                    SAMPLING_TIME_CK);                     
                ADC2->JSQR = PHASE_C_MSK + TEMP_FDBK_MSK + SEQUENCE_LENGHT;                                         
                break;
    case SECTOR_2:
                hTimePhA = (T/8) + ((((T + wY) - wZ)/2)/131072);
				hTimePhB = hTimePhA + wZ/131072;
				hTimePhC = hTimePhA - wY/131072;
                
                // ADC Syncronization setting value
                if ((u16)(PWM_PERIOD-hTimePhB) > TW_AFTER)
                {
                  hTimePhD = PWM_PERIOD - 1;
                }
                else
                {
                  hDeltaDuty = (u16)(hTimePhB - hTimePhA);
                  
                  // Definition of crossing point
                  if (hDeltaDuty > (u16)(PWM_PERIOD-hTimePhB)*2) 
                  {
                    hTimePhD = hTimePhB - TW_BEFORE; // Ts before Phase B 
                  }
                  else
                  {
                    hTimePhD = hTimePhB + TW_AFTER; // DT + Tn after Phase B
                    
                    if (hTimePhD >= PWM_PERIOD)
                    {
                      // Trigger of ADC at Falling Edge PWM4
                      // OCR update
                      
                      //Set Polarity of CC4 Low
                      PWM4Direction=PWM1_MODE;
                      
                      hTimePhD = (2 * PWM_PERIOD) - hTimePhD-1;
                    }
                  }
                }
                
                //ADC_InjectedChannelConfig(ADC1, PHASE_A_CHANNEL,1,
                //                                     SAMPLING_TIME_CK);               
                ADC1->JSQR = PHASE_A_MSK + BUS_VOLT_FDBK_MSK + SEQUENCE_LENGHT;                
                //ADC_InjectedChannelConfig(ADC2, 
                //                   PHASE_C_CHANNEL,1,SAMPLING_TIME_CK);                              
				ADC2->JSQR = PHASE_C_MSK + TEMP_FDBK_MSK + SEQUENCE_LENGHT;
                break;

    case SECTOR_3:
                hTimePhA = (T/8) + ((((T - wX) + wY)/2)/131072);
				hTimePhC = hTimePhA - wY/131072;
				hTimePhB = hTimePhC + wX/131072;
		
                // ADC Syncronization setting value
                if ((u16)(PWM_PERIOD-hTimePhB) > TW_AFTER)
                {
                  hTimePhD = PWM_PERIOD - 1;
                }
                else
                {
                  hDeltaDuty = (u16)(hTimePhB - hTimePhC);
                  
                  // Definition of crossing point
                  if (hDeltaDuty > (u16)(PWM_PERIOD-hTimePhB)*2) 
                  {
                    hTimePhD = hTimePhB - TW_BEFORE; // Ts before Phase B 
                  }
                  else
                  {
                    hTimePhD = hTimePhB + TW_AFTER; // DT + Tn after Phase B
                    
                    if (hTimePhD >= PWM_PERIOD)
                    {
                      // Trigger of ADC at Falling Edge PWM4
                      // OCR update
                      
                      //Set Polarity of CC4 Low
                      PWM4Direction=PWM1_MODE;
                      
                      hTimePhD = (2 * PWM_PERIOD) - hTimePhD-1;
                    }
                  }
                }
                
                //ADC_InjectedChannelConfig(ADC1, PHASE_A_CHANNEL,1,
                //                                    SAMPLING_TIME_CK);               
                ADC1->JSQR = PHASE_A_MSK + BUS_VOLT_FDBK_MSK + SEQUENCE_LENGHT;                
                //ADC_InjectedChannelConfig(ADC2, PHASE_C_CHANNEL,1,
                //                                    SAMPLING_TIME_CK);                                        
				ADC2->JSQR = PHASE_C_MSK + TEMP_FDBK_MSK + SEQUENCE_LENGHT;
                break;
    
    case SECTOR_4:
                hTimePhA = (T/8) + ((((T + wX) - wZ)/2)/131072);
                hTimePhB = hTimePhA + wZ/131072;
                hTimePhC = hTimePhB - wX/131072;
                
                // ADC Syncronization setting value
                if ((u16)(PWM_PERIOD-hTimePhC) > TW_AFTER)
                {
                  hTimePhD = PWM_PERIOD - 1;
                }
                else
                {
                  hDeltaDuty = (u16)(hTimePhC - hTimePhB);
                  
                  // Definition of crossing point
                  if (hDeltaDuty > (u16)(PWM_PERIOD-hTimePhC)*2)
                  {
                    hTimePhD = hTimePhC - TW_BEFORE; // Ts before Phase C 
                  }
                  else
                  {
                    hTimePhD = hTimePhC + TW_AFTER; // DT + Tn after Phase C
                    
                    if (hTimePhD >= PWM_PERIOD)
                    {
                      // Trigger of ADC at Falling Edge PWM4
                      // OCR update
                      
                      //Set Polarity of CC4 Low
                      PWM4Direction=PWM1_MODE;
                      
                      hTimePhD = (2 * PWM_PERIOD) - hTimePhD-1;
                    }
                  }
                }
                
                //ADC_InjectedChannelConfig(ADC1, PHASE_A_CHANNEL,1,
                //                                    SAMPLING_TIME_CK);             
                ADC1->JSQR = PHASE_A_MSK + BUS_VOLT_FDBK_MSK + SEQUENCE_LENGHT;                
                //ADC_InjectedChannelConfig(ADC2, PHASE_B_CHANNEL,1,
                //                                     SAMPLING_TIME_CK);                                    
				ADC2->JSQR = PHASE_B_MSK + TEMP_FDBK_MSK + SEQUENCE_LENGHT;
                break;  
    
    case SECTOR_5:
                hTimePhA = (T/8) + ((((T + wY) - wZ)/2)/131072);
				hTimePhB = hTimePhA + wZ/131072;
				hTimePhC = hTimePhA - wY/131072;
                
                // ADC Syncronization setting value
                if ((u16)(PWM_PERIOD-hTimePhC) > TW_AFTER)
                {
                  hTimePhD = PWM_PERIOD - 1;
                }
                else
                {
                  hDeltaDuty = (u16)(hTimePhC - hTimePhA);
                  
                  // Definition of crossing point
                  if (hDeltaDuty > (u16)(PWM_PERIOD-hTimePhC)*2) 
                  {
                    hTimePhD = hTimePhC - TW_BEFORE; // Ts before Phase C 
                  }
                  else
                  {
                    hTimePhD = hTimePhC + TW_AFTER; // DT + Tn after Phase C
                    
                    if (hTimePhD >= PWM_PERIOD)
                    {
                      // Trigger of ADC at Falling Edge PWM4
                      // OCR update
                      
                      //Set Polarity of CC4 Low
                      PWM4Direction=PWM1_MODE;
                      
                      hTimePhD = (2 * PWM_PERIOD) - hTimePhD-1;
                    }
                  }
                }
                
                //ADC_InjectedChannelConfig(ADC1, PHASE_A_CHANNEL,1,
                //                                   SAMPLING_TIME_CK);              
                ADC1->JSQR = PHASE_A_MSK + BUS_VOLT_FDBK_MSK + SEQUENCE_LENGHT;                
                //ADC_InjectedChannelConfig(ADC2, PHASE_B_CHANNEL,1,
                //                                     SAMPLING_TIME_CK);                                      
				ADC2->JSQR = PHASE_B_MSK + TEMP_FDBK_MSK + SEQUENCE_LENGHT;
		break;
                
    case SECTOR_6:
                hTimePhA = (T/8) + ((((T - wX) + wY)/2)/131072);
				hTimePhC = hTimePhA - wY/131072;
				hTimePhB = hTimePhC + wX/131072;
                
                // ADC Syncronization setting value
                if ((u16)(PWM_PERIOD-hTimePhA) > TW_AFTER)
                {
                  hTimePhD = PWM_PERIOD - 1;
                }
                else
                {
                  hDeltaDuty = (u16)(hTimePhA - hTimePhC);
                  
                  // Definition of crossing point
                  if (hDeltaDuty > (u16)(PWM_PERIOD-hTimePhA)*2) 
                  {
                    hTimePhD = hTimePhA - TW_BEFORE; // Ts before Phase A 
                  }
                  else
                  {
                    hTimePhD = hTimePhA + TW_AFTER; // DT + Tn after Phase A
                    
                    if (hTimePhD >= PWM_PERIOD)
                    {
                      // Trigger of ADC at Falling Edge PWM4
                      // OCR update
                      
                      //Set Polarity of CC4 Low
                      PWM4Direction=PWM1_MODE;
                      
                      hTimePhD = (2 * PWM_PERIOD) - hTimePhD-1;
                    }
                  }
                }
                
                //ADC_InjectedChannelConfig(ADC1, PHASE_B_CHANNEL,1,
                //                                    SAMPLING_TIME_CK);     
                ADC1->JSQR = PHASE_B_MSK + BUS_VOLT_FDBK_MSK + SEQUENCE_LENGHT;                
                //ADC_InjectedChannelConfig(ADC2, PHASE_C_CHANNEL,1,
                //                                    SAMPLING_TIME_CK);                               
				ADC2->JSQR = PHASE_C_MSK + TEMP_FDBK_MSK + SEQUENCE_LENGHT;
                break;
    default:
		break;
   }
  
  if (PWM4Direction == PWM2_MODE)
  {
    //Set Polarity of CC4 High
    TIM1->CCER &= 0xDFFF;    
  }
  else
  {
    //Set Polarity of CC4 Low
    TIM1->CCER |= 0x2000;
  }
  
  /* Load compare registers values */ 
  TIM1->CCR1 = hTimePhA;
  TIM1->CCR2 = hTimePhB;
  TIM1->CCR3 = hTimePhC;
  TIM1->CCR4 = hTimePhD; // To Syncronyze the ADC
}

/*******************************************************************************
* Function Name  : SVPWM_3ShuntAdvCurrentReading
* Description    :  It is used to enable or disable the advanced current reading.
			if disabled the current readign will be performed after update event
* Input          : cmd (ENABLE or DISABLE)
* Output         : None
* Return         : None
*******************************************************************************/
void SVPWM_3ShuntAdvCurrentReading(FunctionalState cmd)
{
  if (cmd == ENABLE)
  {
    // Enable ADC trigger sync with CC4
    //ADC_ExternalTrigInjectedConvConfig(ADC1, ADC_ExternalTrigInjecConv_T1_CC4);  
    ADC1->CR2 |= 0x00001000;  //使用TIM1的CC4标志来触发AD转换。
    
    // Enable UPDATE ISR
    // Clear Update Flag
    TIM_ClearFlag(TIM1, TIM_FLAG_Update);
    TIM_ITConfig(TIM1, TIM_IT_Update, ENABLE);
  }
  else
  {
    // Disable UPDATE ISR
    TIM_ITConfig(TIM1, TIM_IT_Update, DISABLE);

    // Sync ADC trigger with Update
    //ADC_ExternalTrigInjectedConvConfig(ADC1, ADC_ExternalTrigInjecConv_T1_TRGO);
    ADC1->CR2 &=0xFFFFEFFF;
    
    // ReEnable EXT. ADC Triggering
    ADC1->CR2 |=0x00008000;    
  }
}

/*******************************************************************************
* Function Name  : SVPWMUpdateEvent
* Description    :  Routine to be performed inside the update event ISR  it reenable the ext adc. triggering
		        It must be assigned to pSVPWM_UpdateEvent pointer.	
* Input           : None
* Output         : None
* Return         : None
*******************************************************************************/
void SVPWMUpdateEvent(void)
{
  // ReEnable EXT. ADC Triggering
  ADC1->CR2 |= 0x00008000;
  
  // Clear unwanted current sampling
  ADC_ClearFlag(ADC1, ADC_FLAG_JEOC);
}

/*******************************************************************************
* Function Name  : SVPWMEOCEvent
* Description    :  Routine to be performed inside the end of conversion ISR
		         It computes the bus voltage and temperature sensor sampling 
		        and disable the ext. adc triggering.	
* Input           : None
* Output         : None
* Return         : None
*******************************************************************************/
u8 SVPWMEOCEvent(void)
{
  // Store the Bus Voltage and temperature sampled values
  h_ADCTemp = ADC_GetInjectedConversionValue(ADC2,ADC_InjectedChannel_2);
  h_ADCBusvolt = ADC_GetInjectedConversionValue(ADC1,ADC_InjectedChannel_2);
    
  if ((State == START) || (State == RUN))
  {          
    // Disable EXT. ADC Triggering
    ADC1->CR2 = ADC1->CR2 & 0xFFFF7FFF;
  }
  return ((u8)(1));
}

#endif

/******************* (C) COPYRIGHT 2008 STMicroelectronics *****END OF FILE****/  
