/*
 BitzOS (BOS) V0.4.0 - Copyright (C) 2017-2025 Hexabitz
 All rights reserved

 File Name  : H1BR6_spi.c
 Description: Configures and manages SPI2 peripheral.
 SPI2: Initializes in master mode (8MHz clock), 2-line full duplex.
 GPIO: Sets up PB6-8 pins with AF4/1 for SPI functionality.
*/

/* Includes ****************************************************************/
#include "BOS.h"

/* Exported Variables ******************************************************/
SPI_HandleTypeDef hspi2;

/* Local Function Prototypes ***********************************************/
void MX_SPI2_Init(void);

/***************************************************************************/
/* Configure SPI ***********************************************************/
/***************************************************************************/
/* SPI Configuration */
void MX_SPI_Init(void) {

	MX_SPI2_Init();

}

/***************************************************************************/
/* SPI init function */
void MX_SPI2_Init(void) {
	/* SPI2 parameter configuration*/
	hspi2.Instance = SPI2;
	hspi2.Init.Mode = SPI_MODE_MASTER;
	hspi2.Init.Direction = SPI_DIRECTION_2LINES;
	hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
	hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
	hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
	hspi2.Init.NSS = SPI_NSS_SOFT;
	hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
	hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
	hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
	hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
	hspi2.Init.CRCPolynomial = 7;
	hspi2.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
	hspi2.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;

	HAL_SPI_MspInit(&hspi2);
	HAL_SPI_Init(&hspi2);

}

/***************************************************************************/
void HAL_SPI_MspInit(SPI_HandleTypeDef *hspi) {
	GPIO_InitTypeDef GPIO_InitStruct = { 0 };
	if (hspi->Instance == SPI2) {
		/* Peripheral clock enable */
		__HAL_RCC_SPI2_CLK_ENABLE();

		__HAL_RCC_GPIOB_CLK_ENABLE();
		/**SPI2 GPIO Configuration
		 PB6     ------> SPI2_MISO
		 PB7     ------> SPI2_MOSI
		 PB8     ------> SPI2_SCK
		 */
		GPIO_InitStruct.Pin = SD_SPI_MISO_PIN;
		GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
		GPIO_InitStruct.Alternate = GPIO_AF4_SPI2;
		HAL_GPIO_Init(SD_SPI_PORT, &GPIO_InitStruct);

		GPIO_InitStruct.Pin = SD_SPI_MOSI_PIN | SD_SPI_SCK_PIN;
		GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
		GPIO_InitStruct.Alternate = GPIO_AF1_SPI2;
		HAL_GPIO_Init(SD_SPI_PORT, &GPIO_InitStruct);
	}
}

/***************************************************************************/
void HAL_SPI_MspDeInit(SPI_HandleTypeDef *hspi) {
	if (hspi->Instance == SPI2) {
		/* Peripheral clock disable */
		__HAL_RCC_SPI2_CLK_DISABLE();

		/**SPI2 GPIO Configuration
		 PB6     ------> SPI2_MISO
		 PB7     ------> SPI2_MOSI
		 PB8     ------> SPI2_SCK
		 */
		HAL_GPIO_DeInit(SD_SPI_PORT, SD_SPI_MISO_PIN | SD_SPI_MOSI_PIN | SD_SPI_SCK_PIN);
	}
}

/***************************************************************************/
/***************** (C) COPYRIGHT HEXABITZ ***** END OF FILE ****************/
