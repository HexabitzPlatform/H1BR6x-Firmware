/*
 BitzOS (BOS) V0.2.5 - Copyright (C) 2017-2021 Hexabitz
 All rights reserved
		
 File Name     : H1BR6.c
 Description   : Header file for module H1BR6.
				 SPI-based uSD driver with Fatfs.
*/
	
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef H1BR6_H
#define H1BR6_H

/* Includes ------------------------------------------------------------------*/
#include "BOS.h"
#include "H1BR6_MemoryMap.h"	
#include "H1BR6_uart.h"	
#include "H1BR6_gpio.h"	
#include "H1BR6_dma.h"		
#include "H1BR6_spi.h"	
#include "H1BR6_sd_spi.h"	
#include "ff_gen_drv.h"
#include "sd_diskio.h"
	
	
/* Exported definitions -------------------------------------------------------*/

#define	modulePN		_H1BR6

/* Port-related definitions */
#define	NumOfPorts		5
#define P_PROG 				P2						/* ST factory bootloader UART */

/* Define available ports */
#define _P1 
#define _P2 
#define _P3 
#define _P4 
#define _P5

/* Define available USARTs */
#define _Usart1 1
#define _Usart2 1
#define _Usart3 1
#define _Usart4 1
#define _Usart5 1

/* Port-UART mapping */
#define P1uart &huart4
#define P2uart &huart2
#define P3uart &huart3
#define P4uart &huart1
#define P5uart &huart5
	
/* Port Definitions */
#define	USART1_TX_PIN		GPIO_PIN_9
#define	USART1_RX_PIN		GPIO_PIN_10
#define	USART1_TX_PORT	GPIOA
#define	USART1_RX_PORT	GPIOA
#define	USART1_AF				GPIO_AF1_USART1

#define	USART2_TX_PIN		GPIO_PIN_2
#define	USART2_RX_PIN		GPIO_PIN_3
#define	USART2_TX_PORT	GPIOA
#define	USART2_RX_PORT	GPIOA
#define	USART2_AF				GPIO_AF1_USART2

#define	USART3_TX_PIN		GPIO_PIN_10
#define	USART3_RX_PIN		GPIO_PIN_11
#define	USART3_TX_PORT	GPIOB
#define	USART3_RX_PORT	GPIOB
#define	USART3_AF				GPIO_AF4_USART3

#define	USART4_TX_PIN		GPIO_PIN_0
#define	USART4_RX_PIN		GPIO_PIN_1
#define	USART4_TX_PORT	GPIOA
#define	USART4_RX_PORT	GPIOA
#define	USART4_AF				GPIO_AF4_USART4

#define	USART5_TX_PIN		GPIO_PIN_3
#define	USART5_RX_PIN		GPIO_PIN_4
#define	USART5_TX_PORT	GPIOB
#define	USART5_RX_PORT	GPIOB
#define	USART5_AF				GPIO_AF4_USART5


/* Module-specific Definitions */

#define EVAL_SPIx                     	SPI1
#define SPIx_CLK_ENABLE()           		__HAL_RCC_SPI1_CLK_ENABLE()
#define SPIx_CLK_DISABLE()          		__HAL_RCC_SPI1_CLK_DISABLE()
#define SPIx_FORCE_RESET()          		__HAL_RCC_SPI1_FORCE_RESET()
#define SPIx_RELEASE_RESET()        		__HAL_RCC_SPI1_RELEASE_RESET()
	
#define SPIx_SCK_PIN                		GPIO_PIN_5              /* PA.05 */
#define SPIx_SCK_GPIO_PORT          		GPIOA                   
#define SPIx_SCK_GPIO_CLK_ENABLE()  		__HAL_RCC_GPIOA_CLK_ENABLE()
#define SPIx_SCK_GPIO_CLK_DISABLE() 		__HAL_RCC_GPIOA_CLK_DISABLE()
#define SPIx_SCK_AF                 		GPIO_AF0_SPI1
	
#define SPIx_MISO_PIN               		GPIO_PIN_6              /* PA.06 */
#define SPIx_MISO_GPIO_PORT         		GPIOA                  
#define SPIx_MISO_GPIO_CLK_ENABLE() 		__HAL_RCC_GPIOA_CLK_ENABLE()
#define SPIx_MISO_GPIO_CLK_DISABLE()		__HAL_RCC_GPIOA_CLK_DISABLE()
#define SPIx_MISO_AF                		GPIO_AF0_SPI1
	
#define SPIx_MOSI_PIN               		GPIO_PIN_7              /* PA.07 */
#define SPIx_MOSI_GPIO_PORT         		GPIOA                   
#define SPIx_MOSI_GPIO_CLK_ENABLE() 		__HAL_RCC_GPIOA_CLK_ENABLE()
#define SPIx_MOSI_GPIO_CLK_DISABLE()		__HAL_RCC_GPIOA_CLK_DISABLE()
#define SPIx_MOSI_AF                		GPIO_AF0_SPI1

/* Chip Select macro definition */
#define SD_CS_LOW()                     HAL_GPIO_WritePin(SD_CS_GPIO_PORT, SD_CS_PIN, GPIO_PIN_RESET)
#define SD_CS_HIGH()                    HAL_GPIO_WritePin(SD_CS_GPIO_PORT, SD_CS_PIN, GPIO_PIN_SET)  

/* SD card Control pin */
#define SD_CS_PIN                       GPIO_PIN_4              /* PA.04 */
#define SD_CS_GPIO_PORT                 GPIOA                  
#define SD_CS_GPIO_CLK_ENABLE()         __HAL_RCC_GPIOA_CLK_ENABLE()
#define SD_CS_GPIO_CLK_DISABLE()        __HAL_RCC_GPIOA_CLK_DISABLE()

/* SD Detect Interface pins */
#define SD_DETECT_PIN                   GPIO_PIN_0              /* PB.0 */
#define SD_DETECT_GPIO_PORT             GPIOB                   
#define SD_DETECT_GPIO_CLK_ENABLE()     __HAL_RCC_GPIOB_CLK_ENABLE()
#define SD_DETECT_GPIO_CLK_DISABLE()    __HAL_RCC_GPIOB_CLK_DISABLE()
#define SD_DETECT_EXTI_IRQn             EXTI0_1_IRQn

/* Indicator LED */
#define _IND_LED_PORT		GPIOB
#define _IND_LED_PIN		GPIO_PIN_14

#define MAX_LOGS					10
#define MAX_LOG_VARS			30
#define MAX_DUPLICATE_FILE		((uint8_t)255U)
#define MAX_NAME_LENGTH			((uint8_t)25U)

#define LOG_EXIST				(true)
#define LOG_NOT_EXIST			(false)


#define NUM_MODULE_PARAMS		1

/* H1BR6_Status Type Definition */  
typedef enum 
{
  H1BR6_OK = 0,
	H1BR6_ERR_UnknownMessage = 1,
	H1BR6_ERR_LogNameExists = 2,
	H1BR6_ERR_WrongParams,
	H1BR6_ERR_SD,
	H1BR6_ERR_MaxLogs,
	H1BR6_ERR_MaxLogVars,
	H1BR6_ERR_LogDoesNotExist,
	H1BR6_ERR_LogIsNotActive,
	H1BR6_ERR_MemoryFull,
	H1BR6_ERR_WrongAddress,
	H1BR6_ERROR = 255
} Module_Status;

/* Log enuemrations */
typedef enum { RATE = 1, EVENT } logType_t;
typedef enum { PORT_DIGITAL = 1, PORT_DATA, PORT_BUTTON, MEMORY_DATA_UINT8, MEMORY_DATA_INT8, MEMORY_DATA_UINT16, MEMORY_DATA_INT16, MEMORY_DATA_UINT32,
							 MEMORY_DATA_INT32, MEMORY_DATA_FLOAT } logVarType_t;
typedef enum { FMT_SPACE = 1, FMT_TAB, FMT_COMMA } delimiterFormat_t;
typedef enum { FMT_NONE = 0, FMT_SAMPLE, FMT_TIME } indexColumnFormat_t;
typedef enum { DELETE_ALL = 0, KEEP_ON_DISK } options_t;
//WAVE_STATE return values
typedef enum		{WAVE_FILE_OK = 1,	HEADER_CHUNK_OK,HEADER_CHUNK_FAULT,WAVE_FILE_OPEN_FAILD,WAVE_FILE_READ_FAILD,STREAM_WAVE_OK,BITPERSAMPLE_ERR,STREAM_WAVE_FAILD	 = 0xff}WAVE_STATE;


/* Log Struct Type Definition */  
typedef struct
{
	char* name;
	uint8_t file_extension;
	uint8_t current_extension;
	logType_t type; 
	float rate; 
	delimiterFormat_t delimiterFormat; 
	indexColumnFormat_t indexColumnFormat;
	char* indexColumnLabel;
	uint32_t filePtr;
	uint32_t t0;
	uint32_t sampleCount;
} 
log_t;

/* Log Column Struct Type Definition */  
typedef struct
{
	uint8_t logIndex;
	logVarType_t type;
	char* varLabel;
	uint32_t source;
} 
logVar_t;

	
/* WAVE file parameters */
extern uint8_t wavebuff[44];
extern UINT Number_br;
extern FIL _path_pointer;
	

/* Exported variables */
extern log_t logs[MAX_LOGS];
extern logVar_t logVars[MAX_LOG_VARS];

/* Export UART variables */
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
extern UART_HandleTypeDef huart3;
extern UART_HandleTypeDef huart4;
extern UART_HandleTypeDef huart5;



/* Define UART Init prototypes */
extern void MX_USART1_UART_Init(void);
extern void MX_USART2_UART_Init(void);
extern void MX_USART3_UART_Init(void);
extern void MX_USART4_UART_Init(void);
extern void MX_USART5_UART_Init(void);


	
/* -----------------------------------------------------------------------
	|																APIs	 																 	|
   ----------------------------------------------------------------------- 
*/

extern Module_Status CreateLog(char* logName, logType_t type, float rate, delimiterFormat_t delimiterFormat, indexColumnFormat_t indexColumnFormat,\
	char* indexColumnLabel);
extern Module_Status LogVar(char* logName, logVarType_t type, uint32_t source, char* ColumnLabel);
extern Module_Status StartLog(char* logName);
extern Module_Status StopLog(char* logName);
extern Module_Status PauseLog(char* logName);
extern Module_Status ResumeLog(char* logName);
extern Module_Status DeleteLog(char* logName, options_t options);
extern WAVE_STATE StreamWaveToModule(char* Wave_Full_Name, uint8_t H07R3x_ID);
extern WAVE_STATE ScanWaveFile(char* Wave_Full_Name, uint8_t H07R3x_ID);


/* -----------------------------------------------------------------------
	|															Commands																 	|
   ----------------------------------------------------------------------- 
*/

extern const CLI_Command_Definition_t addLogCommandDefinition;
extern const CLI_Command_Definition_t deleteLogCommandDefinition;
extern const CLI_Command_Definition_t logVarCommandDefinition;
extern const CLI_Command_Definition_t startCommandDefinition;
extern const CLI_Command_Definition_t stopCommandDefinition;
extern const CLI_Command_Definition_t pauseCommandDefinition;
extern const CLI_Command_Definition_t resumeCommandDefinition;


#endif /* H1BR6_H */

/************************ (C) COPYRIGHT HEXABITZ *****END OF FILE****/
