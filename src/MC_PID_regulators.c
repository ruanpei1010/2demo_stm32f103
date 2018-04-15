/******************** (C) COPYRIGHT 2008 STMicroelectronics ********************
* File Name          : MC_PID_regulators.c
* Author             : IMS Systems Lab 
* Date First Issued  : 21/11/07
* Description        : This file contains the software implementation for the
                       PI(D) regulators.
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

/* Standard include ----------------------------------------------------------*/

#include "stm32f10x.h"
#include "stm32f10x_MClib.h"
// #include "stm32f10x_type.h"
#include "MC_Globals.h"

#define PID_SPEED_REFERENCE  (u16)(PID_SPEED_REFERENCE_RPM/6)

typedef signed long long s64;

/*******************************************************************************
* Function Name  : PID_Init
* Description    : Initialize PID coefficients for torque, flux and speed loop: 
                   Kp_Gain: proportional coeffcient 
                   Ki_Gain: integral coeffcient 
                   Kd_Gain: differential coeffcient 
* Input          : Pointer 1 to Torque PI structure,  
                   Pointer 2 to Flux PI structure,  
                   Pointer 3 to Speed PI structure  
* Output         : None
* Return         : None
*******************************************************************************/
void PID_Init (PID_Struct_t *PID_Torque, PID_Struct_t *PID_Flux, 
                                                        PID_Struct_t *PID_Speed)
{
  hTorque_Reference = PID_TORQUE_REFERENCE;

  PID_Torque->hKp_Gain    = PID_TORQUE_KP_DEFAULT;
  PID_Torque->hKp_Divisor = TF_KPDIV;  

  PID_Torque->hKi_Gain = PID_TORQUE_KI_DEFAULT;
  PID_Torque->hKi_Divisor = TF_KIDIV;
  
  PID_Torque->hKd_Gain = PID_TORQUE_KD_DEFAULT;
  PID_Torque->hKd_Divisor = TF_KDDIV;
  PID_Torque->wPreviousError = 0;
  
  PID_Torque->hLower_Limit_Output=S16_MIN;   //Lower Limit for Output limitation
  PID_Torque->hUpper_Limit_Output= S16_MAX;   //Upper Limit for Output limitation
  PID_Torque->wLower_Limit_Integral = S16_MIN * TF_KIDIV;
  PID_Torque->wUpper_Limit_Integral = S16_MAX * TF_KIDIV;
  PID_Torque->wIntegral = 0;
 
  /**************************************************/
  /************END PID Torque Regulator members*******/
  /**************************************************/

  /**************************************************/
  /************PID Flux Regulator members*************/
  /**************************************************/
  PID_Flux->wIntegral = 0;  // reset integral value 
 
  //对于SM-PMSM电机，Id = 0
  hFlux_Reference = PID_FLUX_REFERENCE;

  PID_Flux->hKp_Gain    = PID_FLUX_KP_DEFAULT;
  PID_Flux->hKp_Divisor = TF_KPDIV;  

  PID_Flux->hKi_Gain = PID_FLUX_KI_DEFAULT;
  PID_Flux->hKi_Divisor = TF_KIDIV;
  
  PID_Flux->hKd_Gain = PID_FLUX_KD_DEFAULT;
  PID_Flux->hKd_Divisor = TF_KDDIV;
  PID_Flux->wPreviousError = 0;
  
  PID_Flux->hLower_Limit_Output=S16_MIN;   //Lower Limit for Output limitation
  PID_Flux->hUpper_Limit_Output= S16_MAX;   //Upper Limit for Output limitation
  PID_Flux->wLower_Limit_Integral = S16_MIN * TF_KIDIV;
  PID_Flux->wUpper_Limit_Integral = S16_MAX * TF_KIDIV;
  PID_Flux->wIntegral = 0;
  
  /**************************************************/
  /************END PID Flux Regulator members*********/
  /**************************************************/

  /**************************************************/
  /************PID Speed Regulator members*************/
  /**************************************************/


  PID_Speed->wIntegral = 0;  // reset integral value 

  hSpeed_Reference = PID_SPEED_REFERENCE;

  PID_Speed->hKp_Gain    = PID_SPEED_KP_DEFAULT;
  PID_Speed->hKp_Divisor = SP_KPDIV;  

  PID_Speed->hKi_Gain = PID_SPEED_KI_DEFAULT;
  PID_Speed->hKi_Divisor = SP_KIDIV;
  
  PID_Speed->hKd_Gain = PID_SPEED_KD_DEFAULT;
  PID_Speed->hKd_Divisor = SP_KDDIV;
  PID_Speed->wPreviousError = 0;
  
  PID_Speed->hLower_Limit_Output= -IQMAX;   //Lower Limit for Output limitation
  PID_Speed->hUpper_Limit_Output= IQMAX;   //Upper Limit for Output limitation
  PID_Speed->wLower_Limit_Integral = -IQMAX * SP_KIDIV;
  PID_Speed->wUpper_Limit_Integral = IQMAX * SP_KIDIV;
  PID_Speed->wIntegral = 0;
  /**************************************************/
  /**********END PID Speed Regulator members*********/
  /**************************************************/

}

/*******************************************************************************
* Function Name  : PID_Speed_Coefficients_update
* Description    : Update Kp, Ki & Kd coefficients of speed regulation loop 
                   according to motor speed. See "MC_PID_Param.h" for parameters 
                   setting
* Input          : s16 
                   Mechanical motor speed with 0.1 Hz resolution 
                   eg: 10Hz <-> (s16)100
* Output         : None
* Return         : None
*******************************************************************************/
void PID_Speed_Coefficients_update(s16 motor_speed, PID_Struct_t *PID_Struct)
{
if ( motor_speed < 0)  
{
  motor_speed = (u16)(-motor_speed);   // absolute value only
}

if ( motor_speed <= Freq_Min )    // motor speed lower than Freq_Min? 
{
  PID_Struct->hKp_Gain = Kp_Fmin;   
  PID_Struct->hKi_Gain = Ki_Fmin;

  #ifdef DIFFERENTIAL_TERM_ENABLED
  PID_Struct->hKd_Gain =Kd_Fmin;
  #endif
}
else if ( motor_speed <= F_1 )
{
  PID_Struct->hKp_Gain = Kp_Fmin + (s32)(alpha_Kp_1*(motor_speed - Freq_Min) / 1024);
  PID_Struct->hKi_Gain = Ki_Fmin + (s32)(alpha_Ki_1*(motor_speed - Freq_Min) / 1024);

  #ifdef DIFFERENTIAL_TERM_ENABLED
  PID_Struct->hKd_Gain = Kd_Fmin + (s32)(alpha_Kd_1*(motor_speed - Freq_Min) / 1024);
  #endif
}
else if ( motor_speed <= F_2 )
{
  PID_Struct->hKp_Gain = Kp_F_1 + (s32)(alpha_Kp_2 * (motor_speed-F_1) / 1024);
  PID_Struct->hKi_Gain = Ki_F_1 + (s32)(alpha_Ki_2 * (motor_speed-F_1) / 1024);

  #ifdef DIFFERENTIAL_TERM_ENABLED
  PID_Struct->hKd_Gain = Kd_F_1 + (s32)(alpha_Kd_2 * (motor_speed-F_1) / 1024);
  #endif
}
else if ( motor_speed <= Freq_Max )
{
  PID_Struct->hKp_Gain = Kp_F_2 + (s32)(alpha_Kp_3 * (motor_speed-F_2) / 1024);
  PID_Struct->hKi_Gain = Ki_F_2 + (s32)(alpha_Ki_3 * (motor_speed-F_2) / 1024);

  #ifdef DIFFERENTIAL_TERM_ENABLED
  PID_Struct->hKd_Gain = Kd_F_2 + (s32)(alpha_Kd_3 * (motor_speed-F_2) / 1024);
  #endif
}
else  // motor speed greater than Freq_Max? 
{
  PID_Struct->hKp_Gain = Kp_Fmax;
  PID_Struct->hKi_Gain = Ki_Fmax;

  #ifdef DIFFERENTIAL_TERM_ENABLED
  PID_Struct->hKd_Gain = Kd_Fmax;
  #endif
}
}

/*******************************************************************************
* Function Name  : PID_Regulator
* Description    : Compute the PI(D) output for a PI(D) regulation.
* Input          : Pointer to the PID settings (*PID_Flux)
                   Speed in s16 format
* Output         : s16
* Return         : None
*******************************************************************************/

/******************************PID 调节函数的说明******************************/

s16 PID_Regulator(s16 hReference, s16 hPresentFeedback, PID_Struct_t *PID_Struct)
{
  s32 wError, wProportional_Term,wIntegral_Term, houtput_32;
  s64 dwAux; 
#ifdef DIFFERENTIAL_TERM_ENABLED    
  s32 wDifferential_Term;
#endif    
  // error computation
  wError= (s32)(hReference - hPresentFeedback);		  //取得需要误差量	delta_e
 
  // Proportional term computation
  wProportional_Term = PID_Struct->hKp_Gain * wError;	 // wP = Kp * delta_e
														 // wP 为比例总调节量
  // Integral term computation
  if (PID_Struct->hKi_Gain == 0)
  {
    PID_Struct->wIntegral = 0;
  }
  else
  { 
    wIntegral_Term = PID_Struct->hKi_Gain * wError;		   // wI = Ki * delta_e	，本次积分项
    dwAux = PID_Struct->wIntegral + (s64)(wIntegral_Term);	// 积分累积的调节量 = 以前的积分累积量 + 本次的积分项
    
    if (dwAux > PID_Struct->wUpper_Limit_Integral)		   //dwAux为当前积分累积项，下面测试积分饱和度
    {
      PID_Struct->wIntegral = PID_Struct->wUpper_Limit_Integral;	// 超上限
    }
    else if (dwAux < PID_Struct->wLower_Limit_Integral)				//超下限
		{ 
			PID_Struct->wIntegral = PID_Struct->wLower_Limit_Integral;
		}
		else
		{
		 PID_Struct->wIntegral = (s32)(dwAux);		  //不超限, 更新积分累积项为dwAux
		}
  }
  // Differential term computation
#ifdef DIFFERENTIAL_TERM_ENABLED						  //使用微分调节项
  {
  s32 wtemp;
  
  wtemp = wError - PID_Struct->wPreviousError;			  //取得上次和这个的误差之差
  wDifferential_Term = PID_Struct->hKd_Gain * wtemp;	  //wD = Kd * delta_d
  PID_Struct->wPreviousError = wError;    				  // 更新上次误差	

  }
  houtput_32 = (wProportional_Term/PID_Struct->hKp_Divisor+     //输出总的调节量 = 比例调节量/分数因子 +
                PID_Struct->wIntegral/PID_Struct->hKi_Divisor + //				 + 积分调节量/分数因子
                wDifferential_Term/PID_Struct->hKd_Divisor); 	//				 + 微分调节量/分数因子

#else  
  houtput_32 = (wProportional_Term/PID_Struct->hKp_Divisor+ 	//不含微分环节的总调节量
                PID_Struct->wIntegral/PID_Struct->hKi_Divisor);
#endif
  
    if (houtput_32 >= PID_Struct->hUpper_Limit_Output)	   //测试输出是否饱和，超上限
      {
      return(PID_Struct->hUpper_Limit_Output);		  			 	
      }
    else if (houtput_32 < PID_Struct->hLower_Limit_Output) //超下限
      {
      return(PID_Struct->hLower_Limit_Output);
      }
    else 
      {
        return((s16)(houtput_32)); 						   //不超限。输出结果 houtput_32
      }
}		   

/******************** (C) COPYRIGHT 2008 STMicroelectronics *******************/

/**********************下面是TI公司的PID整定函数pid_reg3_calc******************/
/*

void pid_reg3_calc(PIDREG3 *v)
{	
    // Compute the error
    v->Err = v->Ref - v->Fdb;

    // Compute the proportional output
    v->Up = _IQmpy(v->Kp,v->Err);

    // Compute the integral output
    v->Ui = v->Ui + _IQmpy(v->Ki,v->Up) + _IQmpy(v->Kc,v->SatErr);

    // Compute the derivative output
    v->Ud = _IQmpy(v->Kd,(v->Up - v->Up1));

    // Compute the pre-saturated output
    v->OutPreSat = v->Up + v->Ui + v->Ud;

    // Saturate the output
    if (v->OutPreSat > v->OutMax)
      v->Out =  v->OutMax;
    else if (v->OutPreSat < v->OutMin)
      v->Out =  v->OutMin;
    else
      v->Out = v->OutPreSat;

    // Compute the saturate difference
    v->SatErr = v->Out - v->OutPreSat;

    // Update the previous proportional output
    v->Up1 = v->Up; 

}

*/



/***************PID数据结构********************/
/*

typedef struct {  _iq  Ref;   			// Input: Reference input 
				  _iq  Fdb;   			// Input: Feedback input 
				  _iq  Err;				// Variable: Error
				  _iq  Kp;				// Parameter: Proportional gain
				  _iq  Up;				// Variable: Proportional output 
				  _iq  Ui;				// Variable: Integral output 
				  _iq  Ud;				// Variable: Derivative output 	
				  _iq  OutPreSat; 		// Variable: Pre-saturated output
				  _iq  OutMax;		    // Parameter: Maximum output 
				  _iq  OutMin;	    	// Parameter: Minimum output
				  _iq  Out;   			// Output: PID output 
				  _iq  SatErr;			// Variable: Saturated difference
				  _iq  Ki;			    // Parameter: Integral gain
				  _iq  Kc;		     	// Parameter: Integral correction gain
				  _iq  Kd; 		        // Parameter: Derivative gain
				  _iq  Up1;		   	    // History: Previous proportional output
		 	 	  void  (*calc)();	  	// Pointer to calculation function
				 } PIDREG3;	            

typedef PIDREG3 *PIDREG3_handle;
//-----------------------------------------------------------------------------
Default initalizer for the PIDREG3 object.
//-----------------------------------------------------------------------------
#define PIDREG3_DEFAULTS { 0, \
                           0, \
                           0, \
                           _IQ(1.3), \
                           0, \
                           0, \
                           0, \
                           0, \
                           _IQ(1), \
                           _IQ(-1), \
                           0, \
                           0, \
                           _IQ(0.02), \
                           _IQ(0.5), \
                           _IQ(1.05), \
                           0, \
              			  (void (*)(Uint32))pid_reg3_calc }


*/


   
