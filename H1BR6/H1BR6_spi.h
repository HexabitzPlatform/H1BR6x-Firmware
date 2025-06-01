/*
 BitzOS (BOS) V0.4.0 - Copyright (C) 2017-2025 Hexabitz
 All rights reserved

 File Name     : H1BR6_spi.h
 Description   : This file contains all the function prototypes for
               the spi.c file
 */


/* Define to prevent recursive inclusion ***********************************/
#ifndef __SPI_H__
#define __SPI_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ****************************************************************/
#include "stm32g0xx_hal.h"

/* Exported Variables ******************************************************/
extern SPI_HandleTypeDef  hspi2;

/* Exported Functions ******************************************************/
extern void MX_SPI_Init(void);

#ifdef __cplusplus
}
#endif

#endif /* __SPI_H__ */

/***************** (C) COPYRIGHT HEXABITZ ***** END OF FILE ****************/
