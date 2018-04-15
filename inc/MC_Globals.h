/******************** (C) COPYRIGHT 2008 STMicroelectronics ********************
* File Name          : MC_Globals.h
* Author             : IMS Systems Lab  
* Date First Issued  : 21/11/07
* Description        : This file contains the declarations of the exported 
*                      variables of module "MC_globals.c".
********************************************************************************
* History:
* 21/11/07 v1.0
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
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MC_GLOBALS_H
#define __MC_GLOBALS_H
/* Includes ------------------------------------------------------------------*/

#include "stm32f10x.h"
#include "MC_type.h"

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported variables --------------------------------------------------------*/

/*Electrical, magnetic and mechanical variables*/

extern Curr_Components Stat_Curr_a_b;              /*Stator currents Ia,Ib*/ 

extern Curr_Components Stat_Curr_alfa_beta;        /*Ialpha & Ibeta, Clarke's  
                                                  transformations of Ia & Ib */

extern Curr_Components Stat_Curr_q_d;         /*Iq & Id, Parke's transformations
                                                of Ialpha & Ibeta, */

extern Volt_Components Stat_Volt_a_b;              /*Stator voltages Va, Vb*/ 

extern Volt_Components Stat_Volt_q_d;         /*Vq & Vd, voltages on a reference
                                          frame synchronous with the rotor flux*/

extern Volt_Components Stat_Volt_alfa_beta;       /*Valpha & Vbeta, RevPark
                                                    transformations of Vq & Vd*/

/*Variable of convenience*/

extern volatile u32 wGlobal_Flags;

extern volatile SystStatus_t State;

extern PID_Struct_t       PID_Torque_InitStructure;
extern PID_Struct_t       PID_Flux_InitStructure;
extern PID_Struct_t       PID_Speed_InitStructure;

extern volatile s16 hFlux_Reference;
extern volatile s16 hTorque_Reference;
extern volatile s16 hSpeed_Reference;

//下面的全局变量系统运行时的实时AD转换参数
extern volatile u16 hPOT1_Volt;  //电位器
extern volatile u16 hAin0_Volt;  //模拟量1
extern volatile u16 hAin1_Volt;  //模拟量2

extern volatile s16 hBreak_Curr;   //刹车电流平均值
extern volatile s16 hBus_Curr;	   //母线电流平均值

#define ADC1_DR_Address    ((uint32_t)0x4001244C)
#define BufferLenght       36     //6 * 6

extern volatile u32  ADC_DualConvertedValueTab[BufferLenght];
extern volatile u16  ADC1_RegularConvertedValueTab[BufferLenght];
extern volatile u16  ADC2_RegularConvertedValueTab[BufferLenght];

//电流环数据采集函数,电流环的数据为10S的采集
//采集过程在void FOC_TorqueCtrl(void)函数里处理
extern s16 Curr_Iq_Data[2500];
extern s16 Curr_Id_Data[2500];

/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
#endif /* __MC_GLOBALS_H */

/******************* (C) COPYRIGHT 2008 STMicroelectronics *****END OF FILE****/
