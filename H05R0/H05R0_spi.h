/**
  ******************************************************************************
  * File Name          : SPI.h
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
		MODIFIED by Hexabitz for BitzOS (BOS) V0.0.0 - Copyright (C) 2016 Hexabitz
    All rights reserved
*/


/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __H05R0_spi_H
#define __H05R0_spi_H
#ifdef __cplusplus
 extern "C" {
#endif

	 
/* Includes ------------------------------------------------------------------*/
#include "stm32f0xx_hal.h"

	 
/** Exported variables
*/ 
	 
extern SPI_HandleTypeDef hspi1;

	 
/**
  * @brief  Definition for SPI Interface pins (SPI1 used)
  */
#define EVAL_SPIx                        SPI1
#define EVAL_SPIx_CLK_ENABLE()           __HAL_RCC_SPI1_CLK_ENABLE()
#define EVAL_SPIx_CLK_DISABLE()          __HAL_RCC_SPI1_CLK_DISABLE()
#define EVAL_SPIx_FORCE_RESET()          __HAL_RCC_SPI1_FORCE_RESET()
#define EVAL_SPIx_RELEASE_RESET()        __HAL_RCC_SPI1_RELEASE_RESET()

#define EVAL_SPIx_SCK_PIN                GPIO_PIN_3              /* PB.03 */
#define EVAL_SPIx_SCK_GPIO_PORT          GPIOB                   /* GPIOB */
#define EVAL_SPIx_SCK_GPIO_CLK_ENABLE()  __HAL_RCC_GPIOB_CLK_ENABLE()
#define EVAL_SPIx_SCK_GPIO_CLK_DISABLE() __HAL_RCC_GPIOB_CLK_DISABLE()
#define EVAL_SPIx_SCK_AF                 GPIO_AF0_SPI1

#define EVAL_SPIx_MISO_PIN               GPIO_PIN_14             /* PE.14 */
#define EVAL_SPIx_MISO_GPIO_PORT         GPIOE                   /* GPIOE */
#define EVAL_SPIx_MISO_GPIO_CLK_ENABLE() __HAL_RCC_GPIOE_CLK_ENABLE()
#define EVAL_SPIx_MISO_GPIO_CLK_DISABLE() __HAL_RCC_GPIOE_CLK_DISABLE()
#define EVAL_SPIx_MISO_AF                GPIO_AF1_SPI1

#define EVAL_SPIx_MOSI_PIN               GPIO_PIN_15             /* PE.15 */
#define EVAL_SPIx_MOSI_GPIO_PORT         GPIOE                   /* GPIOE */
#define EVAL_SPIx_MOSI_GPIO_CLK_ENABLE() __HAL_RCC_GPIOE_CLK_ENABLE()
#define EVAL_SPIx_MOSI_GPIO_CLK_DISABLE() __HAL_RCC_GPIOE_CLK_DISABLE()
#define EVAL_SPIx_MOSI_AF                GPIO_AF1_SPI1

#define EVAL_SPIx_MOSI_DIR_PIN           GPIO_PIN_2             /* PB.02 */
#define EVAL_SPIx_MOSI_DIR_GPIO_PORT     GPIOB                   /* GPIOB */
#define EVAL_SPIx_MOSI_DIR_GPIO_CLK_ENABLE() __HAL_RCC_GPIOB_CLK_ENABLE()
#define EVAL_SPIx_MOSI_DIR_GPIO_CLK_DISABLE() __HAL_RCC_GPIOB_CLK_DISABLE()

/* Maximum Timeout values for flags waiting loops. These timeouts are not based
   on accurate values, they just guarantee that the application will not remain
   stuck if the SPI communication is corrupted.
   You may modify these timeout values depending on CPU frequency and application
   conditions (interrupts routines ...). */   
#define EVAL_SPIx_TIMEOUT_MAX                 1000

/** Exported functions
*/ 
extern void SPIx_Init(void);
extern void SPIx_Write(uint8_t Value);
extern void SPIx_WriteReadData(const uint8_t *DataIn, uint8_t *DataOut, uint16_t DataLegnth);


#ifdef __cplusplus
}
#endif
#endif /*__H05R0_spi_H */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
