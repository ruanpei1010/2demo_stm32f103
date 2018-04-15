/******************************************************************************
文件名	：spi_flash.c   
编写者	：尚晓飞
编写日期：2008年10月18日 
简要描述：AT45DB041读写程序
当前版本：V1.0
运行平台：STM32F10x
Copyright (c) 2008-2018, shangxiaofei.
       All rights reserved.
*******************************************************************************/
/***********************   修改记录  ******************************************/
/*-----------------------------------------------------------------------------
修改人: 尚晓飞
日　期: 2008年10月18日
版  本: V1.0  
描　述: 基本正常
-----------------------------------------------------------------------------*/
/* Includes ------------------------------------------------------------------*/
#include "spi_flash.h"
#include "ili932x.h"

/* Private typedef -----------------------------------------------------------*/
#define SPI_FLASH_PageSize    0x1006

/* Private define ------------------------------------------------------------*/


#define BUFFER_1_WRITE 0x84 	// buffer 1 write
#define BUFFER_2_WRITE 0x87 	// buffer 2 write
#define BUFFER_1_READ  0xD4		// buffer 1 read (change to 0xD4 for SPI mode 0,3)
#define BUFFER_2_READ  0xD6		// buffer 2 read (change to 0xD6 for SPI mode 0,3)
#define B1_TO_PAGE_WITH_ERASE 0x83 		// buffer 1 to main memory page program with built-in erase
#define B2_TO_PAGE_WITH_ERASE 0x86 		// buffer 2 to main memory page program with built-in erase
#define B1_TO_PAGE_WITHOUT_ERASE 0x88 	// buffer 1 to main memory page program without built-in erase
#define B2_TO_PAGE_WITHOUT_ERASE 0x89 	// buffer 2 to main memory page program without built-in erase
#define PAGE_PROG_THROUGH_B1 0x82 		// main memory page program through buffer 1
#define PAGE_PROG_THROUGH_B2 0x85 		// main memory page program through buffer 2
#define AUTO_PAGE_REWRITE_THROUGH_B1 0x58 // auto page rewrite through buffer 1
#define AUTO_PAGE_REWRITE_THROUGH_B2 0x59 // auto page rewrite through buffer 2
#define PAGE_TO_B1_COMP 0x60 	// main memory page compare to buffer 1
#define PAGE_TO_B2_COMP 0x61 	// main memory page compare to buffer 2
#define PAGE_TO_B1_XFER 0x53 	// main memory page to buffer 1 transfer
#define PAGE_TO_B2_XFER 0x55 	// main memory page to buffer 2 transfer
#define STATUS_REGISTER 0x57     /* 读状态寄存器 */
#define MAIN_MEMORY_PAGE_READ 0x52 // main memory page read (change to 0xD2 for SPI mode 0,3)
#define PAGE_ERASE 0x81 	// erase a 264 byte page
#define BULK_ERASE 0x50 	// erase 8 pages

#define WIP_Flag   0x80  /* Write In Progress (WIP) flag */
#define SETUP_512 0x3d
#define Dummy_Byte 0xA5

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/*******************************************************************************
* Function Name  : SPI_FLASH_Init
* Description    : Initializes the peripherals used by the SPI FLASH driver.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/

//返回串行FLASH模式
void AT45_INIT(void)
{
	SPI_FLASH_Init();
	SPI_FLASH_SendByte(0xff);	//复位总线
}

//设置AT45DB161D为512字节/页函数，只需运行一次
void Flash_Setup_512(void)
 {
    /* Select the FLASH: Chip Select low */
  SPI_FLASH_CS_LOW();
  /* Send "设置为512字节命令" instruction */
  SPI_FLASH_SendByte(SETUP_512);
  /* Send WriteAddr high nibble address byte to write to */
  SPI_FLASH_SendByte(0x2a);
  /* Send WriteAddr medium nibble address byte to write to */
  SPI_FLASH_SendByte(0x80);
  /* Send WriteAddr low nibble address byte to write to */
  SPI_FLASH_SendByte(0xa6);

  /* Deselect the FLASH: Chip Select high */
  SPI_FLASH_CS_HIGH();
} 

/*------------------------------------------------------------------------------
函数名称 : SPI_FLASH_Init
函数功能 : SPI初始化
入口参数 :  
出口参数 : 无  
------------------------------------------------------------------------------*/
void SPI_FLASH_Init(void)
{
  SPI_InitTypeDef  SPI_InitStructure;
  GPIO_InitTypeDef GPIO_InitStructure;


  /* Enable SPI1 and GPIO clocks */
  RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOA |RCC_APB2Periph_GPIOB
                         | RCC_APB2Periph_GPIO_CS | RCC_APB2Periph_SPI1, ENABLE);

 //SPI1口重映射
//  GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE); 
//  GPIO_PinRemapConfig(GPIO_Remap_SWJ_Disable, ENABLE); 
  GPIO_PinRemapConfig(GPIO_Remap_SPI1, ENABLE); 

  /* Configure SPI1 pins: SCK, MISO and MOSI */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOB, &GPIO_InitStructure);

  /* Configure I/O for Flash Chip select */
  GPIO_SetBits(GPIO_CS, GPIO_Pin_CS);//置为高
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_CS;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_Init(GPIO_CS, &GPIO_InitStructure);

  /* Deselect the FLASH: Chip Select high */
  SPI_FLASH_CS_HIGH();

  /* SPI1 configuration */
	SPI_Cmd(SPI1, DISABLE); //必须要有才能改变MODE
  SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
  SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
  SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
  SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;
  SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
  SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
  SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4;
  SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
  SPI_InitStructure.SPI_CRCPolynomial = 7;
  SPI_Init(SPI1, &SPI_InitStructure);

  /* Enable SPI1  */
  SPI_Cmd(SPI1, ENABLE);
}

/*------------------------------------------------------------------------------
函数名称 : SPI_FLASH_PageErase
函数功能 : FLASH整页擦除
入口参数 :  PageAddr -要擦除的页地址
出口参数 : 无  
------------------------------------------------------------------------------*/
void SPI_FLASH_PageErase(u32 PageAddr)
{
/* Wait the end of Flash writing */
  SPI_FLASH_WaitForWriteEnd();
  
  /* Select the FLASH: Chip Select low */
  SPI_FLASH_CS_LOW();
  /* Send "Flash to  Buffer" instruction */
  SPI_FLASH_SendByte(PAGE_ERASE);
  /* Send WriteAddr high nibble address byte to write to */	// 3-12-9
  SPI_FLASH_SendByte((PageAddr ) >> 7);					   //右移位： 无关位个数（左边）+ 有效位 - 8。
  /* Send WriteAddr medium nibble address byte to write to */
  SPI_FLASH_SendByte((PageAddr ) << 1);					  //左移位：8 - 右移位或者是 16 -（无关位个数（左边）+ 有效位）
  /* Send WriteAddr low nibble address byte to write to */
  SPI_FLASH_SendByte(Dummy_Byte);
	//|-23-|-22-|21|20|19|18|17|16|15|14|13|12|11|10|9|8|7|6|5|4|3|2|1|0|
	//|2个无关位|------------12位页地址-------------|----10个无关位-----|

  /* Deselect the FLASH: Chip Select high */
  SPI_FLASH_CS_HIGH();
}
/*------------------------------------------------------------------------------
函数名称 : SPI_FLASH_BulkErase
函数功能 : FLASH块擦除
入口参数 :  BulkAddr -要擦除的块地址
出口参数 : 无  
------------------------------------------------------------------------------*/
void SPI_FLASH_BulkErase(u32 BulkAddr)
{
  /* Wait the end of Flash writing */
  SPI_FLASH_WaitForWriteEnd();
  
  /* Select the FLASH: Chip Select low */
  SPI_FLASH_CS_LOW();
  /* Send "BulkErase" instruction */
  SPI_FLASH_SendByte(BULK_ERASE);
  /* Send WriteAddr high nibble address byte to write to */
  SPI_FLASH_SendByte((BulkAddr ) >> 4);						//右移位： 无关位个数（左边）+ 有效位 - 8。
  /* Send WriteAddr medium nibble address byte to write to */
  SPI_FLASH_SendByte((BulkAddr ) << 4);						//左移位：8 - 右移位或者是 16 -（无关位个数（左边）+ 有效位）
  /* Send WriteAddr low nibble address byte to write to */
  SPI_FLASH_SendByte(Dummy_Byte);
	//|-23-|-22-|21|20|19|18|17|16|15|14|13|12|11|10|9|8|7|6|5|4|3|2|1|0|
	//|3个无关位|------------9位页地址-------------|----12个无关位-----|

  /* Deselect the FLASH: Chip Select high */
  SPI_FLASH_CS_HIGH();
}

/*------------------------------------------------------------------------------
函数名称 : SPI_FLASH_PageToBuffer1
函数功能 : 读主存到Buffer1
入口参数 :  PageAddr -页地址
出口参数 : 无  
------------------------------------------------------------------------------*/
void SPI_FLASH_PageToBuffer1(u32 PageAddr)
{

  /* Wait the end of Flash writing */
  SPI_FLASH_WaitForWriteEnd();
  
  /* Select the FLASH: Chip Select low */
  SPI_FLASH_CS_LOW();
  /* Send "Flash to  Buffer" instruction */
  SPI_FLASH_SendByte(PAGE_TO_B1_XFER);
  /* Send WriteAddr high nibble address byte to write to */
  SPI_FLASH_SendByte((PageAddr ) >> 7);
  /* Send WriteAddr medium nibble address byte to write to */
  SPI_FLASH_SendByte((PageAddr ) << 1);
  /* Send WriteAddr low nibble address byte to write to */
  SPI_FLASH_SendByte(Dummy_Byte);

  /* Deselect the FLASH: Chip Select high */
  SPI_FLASH_CS_HIGH();

}
/*------------------------------------------------------------------------------
函数名称 : SPI_FLASH_PageToBuffer2
函数功能 : 读主存到Buffer2
入口参数 :  PageAddr -页地址
出口参数 : 无  
------------------------------------------------------------------------------*/
void SPI_FLASH_PageToBuffer2(u32 PageAddr)
{

  /* Wait the end of Flash writing */
  SPI_FLASH_WaitForWriteEnd();
  
  /* Select the FLASH: Chip Select low */
  SPI_FLASH_CS_LOW();
  /* Send "Flash to  Buffer" instruction */
  SPI_FLASH_SendByte(PAGE_TO_B2_XFER);
  /* Send WriteAddr high nibble address byte to write to */
  SPI_FLASH_SendByte((PageAddr ) >> 7);
  /* Send WriteAddr medium nibble address byte to write to */
  SPI_FLASH_SendByte((PageAddr ) << 1);
  /* Send WriteAddr low nibble address byte to write to */
  SPI_FLASH_SendByte(Dummy_Byte);

  /* Deselect the FLASH: Chip Select high */
  SPI_FLASH_CS_HIGH();

}
/*------------------------------------------------------------------------------
函数名称 : SPI_FLASH_Buffer1ProgAutoErase
函数功能 : buffer1写入flash 指定页带擦除
入口参数 :  PageAddr -页地址
出口参数 : 无  
------------------------------------------------------------------------------*/
void SPI_FLASH_Buffer1ProgAutoErase(u32 PageAddr)
{

  /* Wait the end of Flash writing */
  SPI_FLASH_WaitForWriteEnd();
  
  /* Select the FLASH: Chip Select low */
  SPI_FLASH_CS_LOW();
  /* Send "Buffer to Flash " instruction */
  SPI_FLASH_SendByte(B1_TO_PAGE_WITH_ERASE);
  /* Send WriteAddr high nibble address byte to write to */
  SPI_FLASH_SendByte((PageAddr ) >> 7);
  /* Send WriteAddr medium nibble address byte to write to */
  SPI_FLASH_SendByte((PageAddr ) << 1);
  /* Send WriteAddr low nibble address byte to write to */
  SPI_FLASH_SendByte(Dummy_Byte);

  /* Deselect the FLASH: Chip Select high */
  SPI_FLASH_CS_HIGH();

}
/*------------------------------------------------------------------------------
函数名称 : SPI_FLASH_Buffer2ProgAutoErase
函数功能 : buffer2写入flash 指定页带擦除
入口参数 :  PageAddr -页地址
出口参数 : 无  
------------------------------------------------------------------------------*/
void SPI_FLASH_Buffer2ProgAutoErase(u32 PageAddr)
{

  /* Wait the end of Flash writing */
  SPI_FLASH_WaitForWriteEnd();
  
  /* Select the FLASH: Chip Select low */
  SPI_FLASH_CS_LOW();
  /* Send "Buffer to Flash " instruction */
  SPI_FLASH_SendByte(B2_TO_PAGE_WITH_ERASE);
  /* Send WriteAddr high nibble address byte to write to */
  SPI_FLASH_SendByte((PageAddr ) >> 7);
  /* Send WriteAddr medium nibble address byte to write to */
  SPI_FLASH_SendByte((PageAddr ) << 1);
  /* Send WriteAddr low nibble address byte to write to */
  SPI_FLASH_SendByte(Dummy_Byte);

  /* Deselect the FLASH: Chip Select high */
  SPI_FLASH_CS_HIGH();

}
/*------------------------------------------------------------------------------
函数名称 : SPI_FLASH_Buffer1Write
函数功能 : 写入buffer1
入口参数 : pBuffer-待写入数据地址，WriteAddr-buffer地址， 
                         NumByteToWrite-写入字节数
出口参数 : 无  
------------------------------------------------------------------------------*/
void SPI_FLASH_Buffer1Write(u8* pBuffer, u32 WriteAddr, u16 NumByteToWrite)
{

  SPI_FLASH_WaitForWriteEnd();
  /* Select the FLASH: Chip Select low */
  SPI_FLASH_CS_LOW();
  /* Send "Write to Memory " instruction */
  SPI_FLASH_SendByte(BUFFER_1_WRITE);
  /* Send WriteAddr high nibble address byte to write to */
  SPI_FLASH_SendByte(Dummy_Byte);
  /* Send WriteAddr medium nibble address byte to write to */
  SPI_FLASH_SendByte((WriteAddr & 0xFF00) >> 8);
  /* Send WriteAddr low nibble address byte to write to */
  SPI_FLASH_SendByte(WriteAddr & 0xFF);

  /* while there is data to be written on the FLASH */
  while (NumByteToWrite--) 
  {
    /* Send the current byte */
    SPI_FLASH_SendByte(*pBuffer);
    /* Point on the next byte to be written */
    pBuffer++;
  }

  /* Deselect the FLASH: Chip Select high */
  SPI_FLASH_CS_HIGH();

  /* Wait the end of Flash writing */
 // SPI_FLASH_WaitForWriteEnd();
}
/*------------------------------------------------------------------------------
函数名称 : SPI_FLASH_Buffer2Write
函数功能 : 写入buffer1
入口参数 : pBuffer-待写入数据地址，WriteAddr-buffer地址， 
                         NumByteToWrite-写入字节数
出口参数 : 无  
------------------------------------------------------------------------------*/
void SPI_FLASH_Buffer2Write(u8* pBuffer, u32 WriteAddr, u16 NumByteToWrite)
{

  SPI_FLASH_WaitForWriteEnd();
  /* Select the FLASH: Chip Select low */
  SPI_FLASH_CS_LOW();
  /* Send "Write to Memory " instruction */
  SPI_FLASH_SendByte(BUFFER_2_WRITE);
  /* Send WriteAddr high nibble address byte to write to */
  SPI_FLASH_SendByte(Dummy_Byte);
  /* Send WriteAddr medium nibble address byte to write to */
  SPI_FLASH_SendByte((WriteAddr & 0xFF00) >> 8);
  /* Send WriteAddr low nibble address byte to write to */
  SPI_FLASH_SendByte(WriteAddr & 0xFF);

  /* while there is data to be written on the FLASH */
  while (NumByteToWrite--)
  {
    /* Send the current byte */
    SPI_FLASH_SendByte(*pBuffer);
    /* Point on the next byte to be written */
    pBuffer++;
  }

  /* Deselect the FLASH: Chip Select high */
  SPI_FLASH_CS_HIGH();

  /* Wait the end of Flash writing */
 // SPI_FLASH_WaitForWriteEnd();
}

/*------------------------------------------------------------------------------
函数名称 : SPI_FLASH_Buffer1Read
函数功能 : 读出buffer1
入口参数 : pBuffer-读出数据地址，WriteAddr-buffer地址， 
                         NumByteToRead-读出字节数
出口参数 : 无  
------------------------------------------------------------------------------*/
void SPI_FLASH_Buffer1Read(u8* pBuffer, u32 ReadAddr, u16 NumByteToRead)
{

  SPI_FLASH_WaitForWriteEnd();

  /* Select the FLASH: Chip Select low */
  SPI_FLASH_CS_LOW();

  /* Send "Read from Memory " instruction */
  SPI_FLASH_SendByte(BUFFER_1_READ);

  /* Send ReadAddr high nibble address byte to read from */
  SPI_FLASH_SendByte(Dummy_Byte);
  /* Send ReadAddr medium nibble address byte to read from */
  SPI_FLASH_SendByte((ReadAddr& 0xFF00) >> 8);
  /* Send ReadAddr low nibble address byte to read from */
  SPI_FLASH_SendByte(ReadAddr & 0xFF);

  SPI_FLASH_SendByte(Dummy_Byte);//

 // SPI_FLASH_WaitForWriteEnd();
  
  while (NumByteToRead--) /* while there is data to be read */
  {
    /* Read a byte from the FLASH */
    *pBuffer = SPI_FLASH_SendByte(Dummy_Byte);
    /* Point to the next location where the byte read will be saved */
    pBuffer++;
  }

  /* Deselect the FLASH: Chip Select high */
  SPI_FLASH_CS_HIGH();
}
/*------------------------------------------------------------------------------
函数名称 : SPI_FLASH_Buffer2Read
函数功能 : 读出buffer2
入口参数 : pBuffer-读出数据地址，WriteAddr-buffer地址， 
                         NumByteToRead-读出字节数
出口参数 : 无  
------------------------------------------------------------------------------*/
void SPI_FLASH_Buffer2Read(u8* pBuffer, u32 ReadAddr, u16 NumByteToRead)
{

  SPI_FLASH_WaitForWriteEnd();

  /* Select the FLASH: Chip Select low */
  SPI_FLASH_CS_LOW();

  /* Send "Read from Memory " instruction */
  SPI_FLASH_SendByte(BUFFER_2_READ);

  /* Send ReadAddr high nibble address byte to read from */
  SPI_FLASH_SendByte(Dummy_Byte);
  /* Send ReadAddr medium nibble address byte to read from */
  SPI_FLASH_SendByte((ReadAddr& 0xFF00) >> 8);
  /* Send ReadAddr low nibble address byte to read from */
  SPI_FLASH_SendByte(ReadAddr & 0xFF);

  SPI_FLASH_SendByte(Dummy_Byte);//

 // SPI_FLASH_WaitForWriteEnd();
  
  while (NumByteToRead--) /* while there is data to be read */
  {
    /* Read a byte from the FLASH */
    *pBuffer = SPI_FLASH_SendByte(Dummy_Byte);
    /* Point to the next location where the byte read will be saved */
    pBuffer++;
  }

  /* Deselect the FLASH: Chip Select high */
  SPI_FLASH_CS_HIGH();
}

/*------------------------------------------------------------------------------
函数名称 : SPI_FLASH_ABS_READByte
函数功能 : 从SPI的绝对字节地址读一定数量的字节
入口参数 : 如下	NumByteToRead = 1...512
出口参数 : 字节数据 
------------------------------------------------------------------------------*/
void SPI_FLASH_ABS_READByte(u8* pBuffer, u32 ReadAddr, u16 NumByteToRead)
{
    u8 i,j;
    u32 page_base = ReadAddr / PAGE_BYTES;		 //每页字节数
    u32 res = ReadAddr %	PAGE_BYTES;
   if ((u16)(res + BufferSize) <= PAGE_BYTES)
    {
      SPI_FLASH_PageToBuffer2(page_base++);       //flash页读入buffer2
      SPI_FLASH_Buffer2Read(pBuffer, res, NumByteToRead);
	 }
    else
	  {
      SPI_FLASH_PageToBuffer2(page_base++);       //flash页读入buffer2
      SPI_FLASH_Buffer2Read(pBuffer, res, PAGE_BYTES -res);
      SPI_FLASH_PageToBuffer1(page_base++);       //flash页读入buffer2
      SPI_FLASH_Buffer1Read(pBuffer+PAGE_BYTES-res, 0x00, res+BufferSize - PAGE_BYTES);
	 }
 }

/*------------------------------------------------------------------------------
函数名称 : SPI_FLASH_ReadByte
函数功能 : SPI 读一字节
入口参数 : 无
出口参数 : 字节数据 
------------------------------------------------------------------------------*/
u8 SPI_FLASH_ReadByte(void)
{
  return (SPI_FLASH_SendByte(Dummy_Byte));
}
/*------------------------------------------------------------------------------
函数名称 : SPI_FLASH_SendByte
函数功能 : SPI 发送一字节并读一字节数据
入口参数 : 字节数据
出口参数 : 字节数据 
------------------------------------------------------------------------------*/
u8 SPI_FLASH_SendByte(u8 byte)
{
  /* Loop while DR register in not emplty */
  while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET);

  /* Send byte through the SPI1 peripheral */
  SPI_I2S_SendData(SPI1, byte);

  /* Wait to receive a byte */
  while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);

  /* Return the byte read from the SPI bus */
  return SPI_I2S_ReceiveData(SPI1);
}
/*------------------------------------------------------------------------------
函数名称 : SPI_FLASH_WaitForWriteEnd
函数功能 : 等待FLASH操作完成
入口参数 :  无
出口参数 :  无
------------------------------------------------------------------------------*/
void SPI_FLASH_WaitForWriteEnd(void)
{
  u8 FLASH_Status = 0;

  /* Select the FLASH: Chip Select low */
  SPI_FLASH_CS_LOW();

  /* Send "Read Status Register" instruction */
  SPI_FLASH_SendByte(STATUS_REGISTER);

  /* Loop as long as the memory is busy with a write cycle */
  do
  {
    /* Send a dummy byte to generate the clock needed by the FLASH
    and put the value of the status register in FLASH_Status variable */
    FLASH_Status = SPI_FLASH_SendByte(Dummy_Byte);

  }
  while ((FLASH_Status & WIP_Flag) == RESET); /* Write in progress */

  /* Deselect the FLASH: Chip Select high */
  SPI_FLASH_CS_HIGH();
}

/******************* (C) COPYRIGHT 2008 STMicroelectronics *****END OF FILE****/
