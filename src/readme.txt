/******************** (C) COPYRIGHT 2008 STMicroelectronics ********************
* File Name          : readme.txt
* Author             : MCD Application Team
* Version            : V2.0
* Date               : 05/23/2008
* Description        : This sub-directory contains all the user-modifiable files 
*                      needed to create a new project linked with the STM32F10x  
*                      Firmware Library and working with RealView Microcontroller
*                      Development Kit(RVMDK) software toolchain (Version 3.21 and later).
********************************************************************************
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
* FOR MORE INFORMATION PLEASE CAREFULLY READ THE LICENSE AGREEMENT FILE LOCATED 
* IN THE ROOT DIRECTORY OF THIS FIRMWARE PACKAGE.
*******************************************************************************/

Directory contents
===================
- Project.Uv2/.Opt: A pre-configured project file with the provided library structure
                    that produces an executable image with RVMDK.
              
- stm32f10x_vector.s: This file contains the vector table for STM32F10x, Stack
                      and Heap definition. User can also enable the use of external
                      SRAM mounted on STM3210E-EVAL board as data memory.                
                      
- cortexm3_macro.s: Instruction wrappers for special Cortex-M3 instructions. 

- note.txt        : Steps to follow when using the default startup file provided 
                    by RVMDK when creating new projects.
      
How to use it
=============
- Open the Project.Uv2 project
- In the build toolbar select the project config:
    - STM3210B-EVAL: to configure the project for STM32 Medium-density devices
    - STM3210E-EVAL: to configure the project for STM32 High-density devices
- Rebuild all files: Project->Rebuild all target files
- Load project image: Debug->Start/Stop Debug Session
- Run program: Debug->Run (F5)

NOTE:
 - Medium-density devices are STM32F101xx and STM32F103xx microcontrollers where
   the Flash memory density ranges between 32 and 128 Kbytes.
 - High-density devices are STM32F101xx and STM32F103xx microcontrollers where
   the Flash memory density ranges between 256 and 512 Kbytes.
    
******************* (C) COPYRIGHT 2008 STMicroelectronics *****END OF FILE******

******************* (C) COPYRIGHT 2009 MiNiSoft Lab. ***************************

已经定义的I/O管脚：

1、	MCDAC输出 stm32f10x_MCdac.c  定义了TIM3的 PB0,PB1为DAC输出脚。

2、	TIMER2缺省定义为 ENCODER的采集。管脚定义参考void ENC_Init(void)函数。	同时对应的
    中断处理函数也被定义了。   stm32f10x_encoder.c文件里。使用TIM2->PA0,PA1,使用TIM3->
	PA6,PA7,使用TIM4->PB6,PB7。

3、	TIMER2缺省定义为 hall的采集，管脚定义参考void HALL_HallTimerInit(void)函数，同时
    中断定位为input capture and Update (overflow) events generate interrupt。
	stm32f10x_hall.c文件里。使用TIM2->PA0,PA1,PA2，使用TIM3->PA6,PA7,PB0，使用TIM4->PB6
	PB7，PB8。中断处理类型与ENCODER使用类型不同。

4、 MC_pwm_1shunt_prm.h和MC_pwm_3shunt_prm.h文件定义了三相电流检测ADC通道以及温度和
    直流母线电压检测ADC通道。 我们使用单总线电流检测，三相电流使用通道为：PC0、PC1、PC2。
	直流母线电压检测->PA3,温度使用->PC0。电位器调速用PC4，用10K电位器。

7、 stm32f10x_Timebase.c文件里定义了整个系统的时钟（脉搏），还有void SysTickHandler(void)
    中断函数，该中断函数内部提供对扭矩和磁通的PID控制。引起MOTOR的速度改变。

8、 STM32x_svpwm_1shunt.c、STM32x_svpwm_3shunt.c以及STM32x_svpwm_ics.c里面分别定义了
    驱动三相电机的6个PWM通道输出以及使用DMA方式的定义，ADC的工作方式、TIM1的4个通道的
	工作方式以及中断向量的使能。具体参考void SVPWM_1ShuntInit(void)和void SVPWM_3ShuntInit(void)

9、 stm32f10x_it.c文件里定义了void ADC1_2_IRQHandler(void)中断向量、里面包含了启动状态，
    其中使用HALL时，直接把状态设为RUN，其他2个有个启动函数，一般是三段式加速启动。还包含了
	2个中断向量处理函数：void TIM1_BRK_IRQHandler(void)刹车中断和void TIM1_UP_IRQHandler(void)
	TIM1 overflow and update interrupt。

10、 stm32f10x_lcd.c文件里定义了LCD驱动的管脚、芯片型号以及数据读写函数，需要把相关函数转换
    到使用FSMC工作。（该工作已经完成。）

11、PC0-PC4->A/D转换，PC4为电位器调节输入。 PA3为直流母线电压检测。
    PA0-PA2缺省为TIM2捕获输入，ENCODER输入。PE15为TIM1的刹车浮空输入端（紧急停车按钮）
	PD13为单独的刹车电阻，开关量输出，PD10为NTC电阻旁路继电器输出开关量控制，改善PFC因数。
	PB0、PB1为TIM3的2个PWM通道输出，仿真D/A转换。PE8-PE13为TIM1的6个互补PWM通道，驱动6个
	桥臂，其中PE9、PE11、PE13上桥臂的引脚定义锁定，防止程序跑飞，烧掉功率模块。PE14为TIM1
	的第4通道，提供A/D采集中断触发周期。PE8-PE14为REMAP引脚功能。

12、 按键定义：上-PD8、下-PD14、左-PE1、右-PE0、SEL-PD12、USER-PB9。  
13、 使用FSMC时，不能使用TIM4_CH2通道和USART2的CK信号冲突。
=================================================================================================
14、 红牛的4个按键定义：PA0、PA8、PC13、PD3。

15、 为了使用TIM2的CH1-CH3通道，需要取下ST3232芯片，使用管脚定义为：PA0-PA2。
     PA1作为TFT模块的LED背光。

16、 取下ST3232，使用PA8-PA11，不能使用USB口了，使用TIM1的CH1-CH4。
     
17、 使用PB0、PB1，作为TIM3的CH3、CH4，作为仿真D/A转换输出。

18、 PC0-PC6共计7个A/D转换通道可以使用，PC3已经定义为电位器。

19、 取下3.2TFT屏幕上的ADS7843，可以用PB12-PB15，作为TIM1的BKIN、CH1N-CH3N。

20、 如果取下AT45DB161D，可以使用PA6、PA7、PB0、PB1作为TIM1的BKIN、CH1N-CH3N。

21、 除此之外，还需要8-10个开关量。其中6个按键、2个NTC旁路继电器和独立的刹车电阻
     继电器。
=========================================================
使用MINISTM32F103RBT6的板子测试PMSM程序按下面的接线顺序：

1、TIM1BKIN--------PB12      TIM1CH1-----------PA8
   TIM1CHIN--------PB13      TIM1CH2-----------PA9
   TIM1CH2N--------PB14      TIM1CH3-----------PA10
   TIM1CH3N--------PB15      TIM1CH4-----------PA11
   (REMAP功能）

2、TIM2的三个通道输入 = PA0、PA1、PA2 (没有FT,注意用DIODE保护)
   TIM3的二个通道输出 = PB4、PB5 可作为DA转换的PWM源 （REMAP功能）
   TIM4的二个通道输入 = PB8、PB9  ---------CH3、CH4  
   注意:凡是可以使用A/D的口线,都不是FT的.
3、PA3、PA4、PA5为三个A/D转换，采样2相电流和母线电压, PA6为散热
   温度,PA7为外部模拟电压输入,PB0,PB1为备用A/D输入.

4、可以作为通用I/O的口线为：PB10、PB11、PA13、PA14、PA15、PB3可作为
   键盘输入和其他状态指示。PD2为LED,PC13为TFT背光。
   UP----PB11             DOWN------PB10
   LEFT----PA14           RIGHT-----PA13
   SEL-----PA15           USER------PB3
   
5、其他通讯口定义：PA11、PA12可以作为USB/CAN总线，PD2为LED指示，PB4,PB5为
   DA0,DA1。PB6,PB7为485口，PB8,PB9为TIM4的3、4输入捕捉通道。

5、需要处理的模块功能：
   MAX3232的RX1、TX1接到了PA9、PA10。
   发光LED-D2、D3为PA0、PA1，PA2为电位器调整。

6、如果口线可以调整，把TFT的驱动改为SPI或者4线方式，或者
   取消LCD，改用74HC595驱动的数码管显示。尽量增加有复用
   功能的口线，比如A/D或者通讯口的引脚。

=================================================================
DISPLAY_LCD使用的函数为:
 LCD_DisplayStringLine(Line0, ptr);
 LCD_ClearLine(Line2);
 LCD_SetTextColor(Blue);
 LCD_DisplayChar(Line4, (u16)(320 -(16*(18-i))),'-')
 LCD_DrawRect(161,97,1,2);
 核心函数: LCD_DrawChar(Line, Column, &ASCII_Table[Ascii * 24]);
========================================================================================
// When using Id = 0, NOMINAL_CURRENT is utilized to saturate the output of the 
// PID for speed regulation (i.e. reference torque). 
// Whit MB459 board, the value must be calculated accordingly with formula:
// NOMINAL_CURRENT = (Nominal phase current (A, 0-to-peak)*32767* Rshunt) /0.64
// 使用MB459时：  每A电流对应的A/D采样值（S16）=  Rshunt * Aop(电流增益 = 2.57）* 65536 / 3.3(V)
											 // =  Rshunt * 32767 / 0.64	  
											 //这里 0.64 = 1.65 / 2.57
// 同理如果使用ICS，那么电流增益为AV =62.5（mv/A), 	那么 每A电流对应的A/D采样值（S16）= 
//						Av * 65536 / 3.3 = Av * 32767 / 1.65 = 1241 (s16类型）
//						则该电流ICS可以采样的最大电流为 = 32767 / 1241 = 26.4（A)
// 如果使用其他类型的电流传感器，那么电流增益的解释为：每变化1安培的电流，输出变化的电压值。
============================================================================================


