/******************** (C) COPYRIGHT 2008 STMicroelectronics ********************
* File Name          : spi_flash.h
* Author             : MCD Application Team
* Version            : V2.0.2
* Date               : 07/11/2008
* Description        : Header for spi_flash.c file.
********************************************************************************
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*******************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __SPI_FLASH_H
#define __SPI_FLASH_H

/* Includes ------------------------------------------------------------------*/
#include "stm32f10x.h"

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Uncomment the line corresponding to the STMicroelectronics evaluation board
   used to run the example */
//定义AT45DBXX1D芯片的片选引脚 /CS
 #define GPIO_CS                  GPIOD
 #define RCC_APB2Periph_GPIO_CS   RCC_APB2Periph_GPIOD
 #define GPIO_Pin_CS              GPIO_Pin_11 

/* Exported macro ------------------------------------------------------------*/
/* Select SPI FLASH: Chip Select pin low  */
#define SPI_FLASH_CS_LOW()       GPIO_ResetBits(GPIO_CS, GPIO_Pin_CS)
/* Deselect SPI FLASH: Chip Select pin high */
#define SPI_FLASH_CS_HIGH()      GPIO_SetBits(GPIO_CS, GPIO_Pin_CS)

/* Exported functions ------------------------------------------------------- */
//=======================使用哪种FLASH,DB041D还是DB161D========================
#define PAGE_BYTES 512     //使用AT45DB161D
//#define PAGE_BYTES 264		  //使用AT45DB041D

void AT45_INIT(void);
void Flash_Setup_512(void);
void SPI_FLASH_ABS_READByte(u8* pBuffer, u32 ReadAddr, u16 NumByteToRead);

/*----- High layer function -----*/
void SPI_FLASH_Init(void);
void SPI_FLASH_PageErase(u32 PageAddr);
void SPI_FLASH_BulkErase(u32 BulkAddr);

void SPI_FLASH_PageToBuffer1(u32 PageAddr);
void SPI_FLASH_PageToBuffer2(u32 PageAddr);
void SPI_FLASH_Buffer1ProgAutoErase(u32 PageAddr);
void SPI_FLASH_Buffer2ProgAutoErase(u32 PageAddr);

void SPI_FLASH_Buffer1Write(u8* pBuffer, u32 WriteAddr, u16 NumByteToWrite);
void SPI_FLASH_Buffer2Write(u8* pBuffer, u32 WriteAddr, u16 NumByteToWrite);
void SPI_FLASH_Buffer1Read(u8* pBuffer, u32 ReadAddr, u16 NumByteToRead);
void SPI_FLASH_Buffer2Read(u8* pBuffer, u32 ReadAddr, u16 NumByteToWrite);


/*----- Low layer function -----*/
u8 SPI_FLASH_ReadByte(void);
u8 SPI_FLASH_SendByte(u8 byte);


void SPI_FLASH_WaitForWriteEnd(void);

#endif /* __SPI_FLASH_H */

/******************* (C) COPYRIGHT 2008 STMicroelectronics *****END OF FILE****/
