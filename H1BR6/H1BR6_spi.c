/**
  ******************************************************************************
  * File Name          : H1BR6_SPI.c
  * Description        : This file provides code for the configuration
  *                      of the SPI instances.
  ******************************************************************************
  *
  * COPYRIGHT(c) 2017 STMicroelectronics
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

/*
		MODIFIED by Hexabitz for BitzOS (BOS) V0.2.1 - Copyright (C) 2017-2020 Hexabitz
    All rights reserved
*/

/* Includes ------------------------------------------------------------------*/
#include "BOS.h"


/* Private variables ---------------------------------------------------------*/
uint32_t SpixTimeout = EVAL_SPIx_TIMEOUT_MAX;    /*<! Value of Timeout when SPI communication fails */
SPI_HandleTypeDef hspi1;

/* Private function prototypes -----------------------------------------------*/
/* SPIx bus function */
void              SPIx_FlushFifo(void);
uint32_t          SPIx_Read(void);
void              SPIx_Error (void);
void              SPIx_MspInit(SPI_HandleTypeDef *hspi);


/*----------------------------------------------------------------------------*/
/* Configure SPI                                                              */
/*----------------------------------------------------------------------------*/

/**
  * @brief SPI1 Bus initialization
  * @retval None
  */
void SPIx_Init(void)
{

	/* SPI Config */
	hspi1.Instance = SPI1;
	/* SPI baudrate is set to 12 MHz (PCLK1/SPI_BaudRatePrescaler = 48/4 = 12 MHz) 
	to verify these constraints:
	HX8347D LCD SPI interface max baudrate is  50MHz for write and 6.66MHz for read
	PCLK1 frequency is set to 48 MHz 
	- SD card SPI interface max baudrate is 25MHz for write/read
	*/
	hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
	hspi1.Init.Direction = SPI_DIRECTION_2LINES;
	hspi1.Init.CLKPhase = SPI_PHASE_2EDGE;
	hspi1.Init.CLKPolarity = SPI_POLARITY_HIGH;
	hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
	hspi1.Init.CRCPolynomial = 7;
	hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
	hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
	hspi1.Init.NSS = SPI_NSS_SOFT;
	hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
	hspi1.Init.Mode = SPI_MODE_MASTER;
	
	SPIx_MspInit(&hspi1);
	HAL_SPI_Init(&hspi1);

}

/**
  * @brief SPI Read 4 bytes from device
  * @retval Read data
  */
uint32_t SPIx_Read(void)
{
  HAL_StatusTypeDef status = HAL_OK;
  uint32_t readvalue = 0x0;
  uint32_t writevalue = 0xFFFFFFFF;
  
  status = HAL_SPI_TransmitReceive(&hspi1, (uint8_t*) &writevalue, (uint8_t*) &readvalue, 1, SpixTimeout);

  /* Check the communication status */
  if(status != HAL_OK)
  {
    /* Execute user timeout callback */
    SPIx_Error();
  }

  return readvalue;
}

/**
  * @brief SPI Write a byte to device
  * @param DataIn: value to be written
  * @param DataOut: read value
  * @param DataLegnth: data length
  * @retval None
  */
void SPIx_WriteReadData(const uint8_t *DataIn, uint8_t *DataOut, uint16_t DataLegnth)
{
  HAL_StatusTypeDef status = HAL_OK;

  status = HAL_SPI_TransmitReceive(&hspi1, (uint8_t*) DataIn, DataOut, DataLegnth, SpixTimeout);
    
  /* Check the communication status */
  if(status != HAL_OK)
  {
    /* Execute user timeout callback */
    SPIx_Error();
  }
}

/**
  * @brief SPI Write a byte to device
  * @param Value: value to be written
  * @retval None
  */
void SPIx_Write(uint8_t Value)
{
  HAL_StatusTypeDef status = HAL_OK;
  uint8_t data;

  status = HAL_SPI_TransmitReceive(&hspi1, (uint8_t*) &Value, &data, 1, SpixTimeout);

  /* Check the communication status */
  if(status != HAL_OK)
  {
    /* Execute user timeout callback */
    SPIx_Error();
  }
}

/**
  * @brief  SPIx_FlushFifo
  * @retval None
  */
void SPIx_FlushFifo(void)
{
  HAL_SPIEx_FlushRxFifo(&hspi1);
}

/**
  * @brief SPI error treatment function
  * @retval None
  */
void SPIx_Error (void)
{
  /* De-initialize the SPI communication BUS */
  HAL_SPI_DeInit(&hspi1);
  
  /* Re- Initiaize the SPI communication BUS */
  SPIx_Init();
}

/**
  * @brief SPI MSP Init
  * @param hspi: SPI handle
  * @retval None
  */
void SPIx_MspInit(SPI_HandleTypeDef *hspi)
{
  GPIO_InitTypeDef GPIO_InitStruct;

  /* Enable SPI clock  */
  SPIx_CLK_ENABLE();
  
  /* enable EVAL_SPI gpio clocks */
  SPIx_SCK_GPIO_CLK_ENABLE();
  SPIx_MISO_GPIO_CLK_ENABLE();
  SPIx_MOSI_GPIO_CLK_ENABLE();
  
  /* configure SPI SCK */
  GPIO_InitStruct.Pin       = SPIx_SCK_PIN;
  GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull      = GPIO_NOPULL;
  GPIO_InitStruct.Speed     = GPIO_SPEED_HIGH;
  GPIO_InitStruct.Alternate = SPIx_SCK_AF;
  HAL_GPIO_Init(SPIx_SCK_GPIO_PORT, &GPIO_InitStruct);
  
  /* configure SPI MOSI */
  GPIO_InitStruct.Pin       = SPIx_MOSI_PIN;
  GPIO_InitStruct.Alternate = SPIx_MOSI_AF;
  HAL_GPIO_Init(SPIx_MOSI_GPIO_PORT, &GPIO_InitStruct);
  
  /* configure SPI MISO  */
  GPIO_InitStruct.Pin       = SPIx_MISO_PIN;
  GPIO_InitStruct.Alternate = SPIx_MISO_AF;
  HAL_GPIO_Init(SPIx_MISO_GPIO_PORT, &GPIO_InitStruct);
  
  /* Force the SPI peripheral clock reset */
  SPIx_FORCE_RESET();

  /* Release the SPI peripheral clock reset */
  SPIx_RELEASE_RESET();
}



/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
