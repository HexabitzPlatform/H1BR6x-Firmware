/*
 BitzOS (BOS) V0.2.5 - Copyright (C) 2017-2021 Hexabitz
 All rights reserved

   File Name          : H1BR6_SPI.h
   Description        : This file provides code for the configuration
                        of the SPI instances.

*/


/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __H1BR6_spi_H
#define __H1BR6_spi_H
#ifdef __cplusplus
 extern "C" {
#endif

	 
/* Includes ------------------------------------------------------------------*/
#include "stm32f0xx_hal.h"

	 
/** Exported variables
*/ 
	 
extern SPI_HandleTypeDef hspi1;


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
#endif /*__H1BR6_spi_H */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
