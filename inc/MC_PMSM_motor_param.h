/******************** (C) COPYRIGHT 2008 STMicroelectronics ********************
* File Name          : MC_PMSM_motor_param.h
* Author             : IMS Systems Lab 
* Date First Issued  : 21/11/07
* Description        : This file contains the PM motor parameters.
********************************************************************************
* History:
* 21/11/07 v1.0
* 29/05/08 v2.0
* 14/07/08 v2.0.1
* 28/08/08 v2.0.2
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
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MOTOR_PARAM_H
#define __MOTOR_PARAM_H

// Number of motor pair of poles
#define	POLE_PAIR_NUM 	(u8) 2       /* Number of motor pole pairs */
#define RS               0.42          /* Stator resistance , ohm*/   //相电阻
#define LS               0.0003       /* Stator inductance , H */	  //相电感

// When using Id = 0, NOMINAL_CURRENT is utilized to saturate the output of the 
// PID for speed regulation (i.e. reference torque). 
// Whit MB459 board, the value must be calculated accordingly with formula:
// NOMINAL_CURRENT = (Nominal phase current (A, 0-to-peak)*32767* Rshunt) /0.64
// 使用MB459时：  每A电流对应的A/D采样值（S16）=  Rshunt * Aop(电流增益 = 2.57）* 65536 / 3.3(V)
//   电流可以为负值，所以使用S16，而不是U16    =  Rshunt * 32767 / 0.64	  
											    //这里 0.64 = 1.65 / 2.57
// 同理如果使用ICS，那么电流增益为AV =45（mv/A), 	那么 每A电流对应的A/D采样值（S16）= 
// 电流可以为负值，所以使用S16，而不是U16，	Av * 65536 / 3.3 = Av * 32767 / 1.37 = 1076 (s16类型）
//						则该电流ICS可以采样的最大电流为 = 32767 / 1076 = 30.45（A)
// 如果使用其他类型的电流传感器，那么电流增益的解释为：每变化1安培的电流，输出变化的电压值。
									// 1076 * 15 = 6205	* 3 = 18615
#define NOMINAL_CURRENT             (s16)5289  //motor nominal current (0-pk)，3倍的额定电流
#define MOTOR_MAX_SPEED_RPM         (u32)6000   //maximum speed required 
#define MOTOR_VOLTAGE_CONSTANT      8.9   //Volts RMS ph-ph /kRPM
//Demagnetization current
#define ID_DEMAG    -NOMINAL_CURRENT

#ifdef IPMSM_MTPA
//MTPA parameters, to be defined only for IPMSM and if MTPA control is chosen
#define IQMAX (s16)(15000)
#define SEGDIV (s16)(2921)
#define ANGC {-1412,-2572,-4576,-5200,-5564,-10551,-12664,-15567}
#define OFST {0,105,463,632,764,3012,4162,5997}
#else
#define IQMAX NOMINAL_CURRENT		   //速度环输出最大值
#endif

#ifdef FLUX_WEAKENING
#define FW_VOLTAGE_REF (s16)(985)   //Vs reference, tenth of a percent
#define FW_KP_GAIN (s16)(3000)      //proportional gain of flux weakening ctrl
#define FW_KI_GAIN (s16)(150)      //integral gain of flux weakening ctrl
#define FW_KPDIV ((u16)(32768))     //flux weak ctrl P gain scaling factor
#define FW_KIDIV ((u16)(32768))     //flux weak ctrl I gain scaling factor
#endif

#ifdef FEED_FORWARD_CURRENT_REGULATION
#define CONSTANT1_Q (s32)(6215) 
#define CONSTANT1_D (s32)(6215)
#define CONSTANT2 (s32)(6962)
#endif

/*not to be modified*/
#define MAX_BEMF_VOLTAGE  (u16)((MOTOR_MAX_SPEED_RPM*\
                           MOTOR_VOLTAGE_CONSTANT*SQRT_2)/(1000*SQRT_3))

#define MOTOR_MAX_SPEED_HZ (s16)((MOTOR_MAX_SPEED_RPM*10)/60)

#define _0_1HZ_2_128DPP (u16)((POLE_PAIR_NUM*65536*128)/(SAMPLING_FREQ*10))

#endif /*__MC_PMSM_MOTOR_PARAM_H*/
/**************** (c) 2008  STMicroelectronics ********************************/
