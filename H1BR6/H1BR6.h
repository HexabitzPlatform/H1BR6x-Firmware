/*
 BitzOS (BOS) V0.4.0 - Copyright (C) 2017-2025 Hexabitz
 All rights reserved
 
 File Name     : H0BR4.h
 Description   : Header file for module H0BR4.
 	 	 	 	 (Description_of_module)

(Description of Special module peripheral configuration):
>>
>>
>>
 */

/* Define to prevent recursive inclusion ***********************************/
#ifndef H1BR6_H
#define H1BR6_H

/* Includes ****************************************************************/
#include "BOS.h"
#include "H1BR6_MemoryMap.h"
#include "H1BR6_uart.h"
#include "H1BR6_spi.h"
#include "H1BR6_gpio.h"
#include "H1BR6_dma.h"
#include "H1BR6_inputs.h"
#include "H1BR6_eeprom.h"
#include "ff_gen_drv.h"
#include "user_diskio.h"
#include "fatfs_sd.h"

/* Exported Macros *********************************************************/
#define	MODULE_PN		_H1BR6

/* Port-related Definitions */
#define	NUM_OF_PORTS	5
#define P_PROG 			P2		/* ST factory bootloader UART */

/* Define Available Ports */
#define _P1
#define _P2
#define _P3
#define _P4
#define _P5

/* Define Available USARTs */
#define _USART1
#define _USART2
#define _USART3
#define _USART4
#define _USART5

/* Port-UART Mapping */
#define UART_P1 &huart4
#define UART_P2 &huart2
#define UART_P3 &huart3
#define UART_P4 &huart1
#define UART_P5 &huart5

/* Module-specific Hardware Definitions ************************************/
/* Port Definitions */
#define	USART1_TX_PIN		GPIO_PIN_9
#define	USART1_RX_PIN		GPIO_PIN_10
#define	USART1_TX_PORT		GPIOA
#define	USART1_RX_PORT		GPIOA
#define	USART1_AF			GPIO_AF1_USART1

#define	USART2_TX_PIN		GPIO_PIN_2
#define	USART2_RX_PIN		GPIO_PIN_3
#define	USART2_TX_PORT		GPIOA
#define	USART2_RX_PORT		GPIOA
#define	USART2_AF			GPIO_AF1_USART2

#define	USART3_TX_PIN		GPIO_PIN_10
#define	USART3_RX_PIN		GPIO_PIN_11
#define	USART3_TX_PORT		GPIOB
#define	USART3_RX_PORT		GPIOB
#define	USART3_AF			GPIO_AF4_USART3

#define	USART4_TX_PIN		GPIO_PIN_0
#define	USART4_RX_PIN		GPIO_PIN_1
#define	USART4_TX_PORT		GPIOA
#define	USART4_RX_PORT		GPIOA
#define	USART4_AF			GPIO_AF4_USART4

#define	USART5_TX_PIN		GPIO_PIN_3
#define	USART5_RX_PIN		GPIO_PIN_2
#define	USART5_TX_PORT		GPIOD
#define	USART5_RX_PORT		GPIOD
#define	USART5_AF			GPIO_AF3_USART5

#define	USART6_TX_PIN		GPIO_PIN_4
#define	USART6_RX_PIN		GPIO_PIN_5
#define	USART6_TX_PORT		GPIOA
#define	USART6_RX_PORT		GPIOA
#define	USART6_AF			GPIO_AF8_USART6

/* SPI Pin Definitions */
#define SD_SPI_SCK_PIN      GPIO_PIN_8
#define SD_SPI_MOSI_PIN     GPIO_PIN_7
#define SD_SPI_MISO_PIN     GPIO_PIN_6
#define SD_SPI_PORT         GPIOB

#define SD_SPI_HANDLER      &hspi2

/* SD-Card GPIO Definitions */
#define SD_C_SELECT_PIN     GPIO_PIN_9
#define SD_C_SELECT_PORT    GPIOB

/* Indicator LED */
#define _IND_LED_PIN		GPIO_PIN_14
#define _IND_LED_PORT		GPIOB

/* Module-specific Macro Definitions ***************************************/
#define MAX_LOGS			    10
#define MAX_LOG_VARS			30
#define MAX_DUPLICATE_FILE		((uint8_t)255U)
#define MAX_NAME_LENGTH			((uint8_t)25U)

#define LOG_EXIST				(true)
#define LOG_NOT_EXIST			(false)

#define NUM_MODULE_PARAMS		1

/* H1BR6_Status Type Definition */
typedef enum {
	H1BR6_OK = 0,
	H1BR6_ERR_UNKNOWNMESSAGE = 1,
	H1BR6_ERR_LOG_NAME_EXISTS = 2,
	H1BR6_ERR_WRONGPARAMS,
	H1BR6_ERR_SD,
	H1BR6_ERR_MAX_LOGS,
	H1BR6_ERR_MAX_LOG_VARS,
	H1BR6_ERR_LOG_DOES_NOT_EXIST,
	H1BR6_ERR_LOG_IS_NOT_ACTIVE,
	H1BR6_ERR_MEMORY_FULL,
	H1BR6_ERR_WRONG_ADDRESS,
	H1BR6_ERR_FILENAMEEXISTS,
	H1BR6_ERR_FILE_DOES_NOT_EXIST,
	H1BR6_ERROR = 255
} Module_Status;

/* Log type enumeration */
typedef enum {
	RATE = 1,      /* Logging at a specific Rate */
	EVENT          /* Event-based logging */
} logType_t;

/* Log variable data type enumeration */
typedef enum {
	PORT_DIGITAL = 1,
	PORT_DATA,
	PORT_BUTTON,
	MEMORY_DATA_UINT8,
	MEMORY_DATA_INT8,
	MEMORY_DATA_UINT16,
	MEMORY_DATA_INT16,
	MEMORY_DATA_UINT32,
	MEMORY_DATA_INT32,
	MEMORY_DATA_FLOAT
} logVarType_t;

/* Delimiter format enumeration */
typedef enum {
	FMT_SPACE = 1,     /* Space-delimited format */
	FMT_TAB,           /* Tab-delimited format */
	FMT_COMMA          /* Comma-separated format */
} delimiterFormat_t;

/* Index column format enumeration */
typedef enum {
	FMT_NONE = 0,      /* No index column */
	FMT_SAMPLE,        /* Index based on sample number */
	FMT_TIME           /* Index based on timestamp */
} indexColumnFormat_t;

/* Logging options enumeration */
typedef enum {
	DELETE_ALL = 0,    /* Delete all log files */
	KEEP_ON_DISK       /* Keep existing files on disk */
} options_t;

/* Main log structure definition */
typedef struct {
	logType_t Type;                          /* Type of logging (RATE or EVENT) */
	delimiterFormat_t DelimiterFormat;       /* Format used to separate values */
	indexColumnFormat_t IndexColumnFormat;   /* Format of the index column */

	char *Name;                              /* Name of the log */
	char *IndexColumnLabel;                  /* Label for the index column */
	uint8_t FileExtension;                   /* File extension used for current log */
	uint8_t CurrentExtension;                /* Current file extension index or version */
	uint32_t t0;                             /* Start time of the logging session */
	uint32_t SampleCount;                    /* Number of samples logged so far */
	volatile float Rate;                     /* Logging Rate in Hz (samples per second) */
} log_t;

/* Log column structure definition */
typedef struct {
	logVarType_t Type;                 /* Type of the variable being logged */

	char *VarLabel;                    /* Label for the variable column */
	uint8_t LogIndex;                  /* Index of this variable in the log */
	volatile uint32_t Source;          /* Address or source of the data */
	volatile uint32_t *TempVar;        /* Pointer to temporary storage or buffer */
	volatile float SourceFloat;        /* Floating-point source value, if applicable */
} logVar_t;

/* Export UART variables */
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
extern UART_HandleTypeDef huart3;
extern UART_HandleTypeDef huart4;
extern UART_HandleTypeDef huart5;
extern UART_HandleTypeDef huart6;

/* Define UART Init prototypes */
extern void MX_USART1_UART_Init(void);
extern void MX_USART2_UART_Init(void);
extern void MX_USART3_UART_Init(void);
extern void MX_USART4_UART_Init(void);
extern void MX_USART5_UART_Init(void);
extern void MX_USART6_UART_Init(void);
extern void SystemClock_Config(void);

/***************************************************************************/
/***************************** General Functions ***************************/
/***************************************************************************/
Module_Status StartLog(char *logName);
Module_Status StopLog(char *logName);
Module_Status PauseLog(char *logName);
Module_Status ResumeLog(char *logName);
Module_Status CreateFile(char *fileName, char *fileExtension);
Module_Status WriteDatatoFile(char *fileName, char *fileExtension, char *data);
Module_Status DeleteLog(char *logName, options_t options, char *fileExtension);
Module_Status LogVar(char *logName, logVarType_t type, uint32_t *source, char *ColumnLabel);
Module_Status CreateLog(char *logName, logType_t type, float Rate, delimiterFormat_t delimiterFormat,
		indexColumnFormat_t indexColumnFormat, char *IndexColumnLabel);

#endif /* H1BR6_H */

/***************** (C) COPYRIGHT HEXABITZ ***** END OF FILE ****************/
