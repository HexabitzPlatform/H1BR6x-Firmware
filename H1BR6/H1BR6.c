/*
    BitzOS (BOS) V0.2.0 - Copyright (C) 2017-2019 Hexabitz
    All rights reserved

    File Name     : H1BR6.c
    Description   : Source code for module H1BR6.
										SPI-based uSD driver with Fatfs.  
		
		Required MCU resources : 
		
			>> USARTs 1,2,3,4,5 for module ports.
			>> DMA1 Ch5, DMA1 Ch6, DMA2 Ch2 for port-to-memory messaging.
			>> DMA1 Ch1, DMA1 Ch3, DMA2 Ch3 for port-to-port streaming.
			>> SPI1 for uSD
			>> PB0 for uSD detect
			>> TIM16 to audio WAVE file timing
			
*/
	
/* Includes ------------------------------------------------------------------*/
#include "BOS.h"

/* Define UART variables */
UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;
UART_HandleTypeDef huart3;
UART_HandleTypeDef huart4;
UART_HandleTypeDef huart5;

/* Module exported parameters ------------------------------------------------*/
module_param_t modParam[NUM_MODULE_PARAMS] = {{.paramPtr=NULL, .paramFormat=FMT_FLOAT, .paramName=""}};

TIM_HandleTypeDef htim16;

/*=================================================================================*/
/*========================= CONSTANTS ==============================================*/
/*=================================================================================*/
const uint8_t numberMap[3] = {1, 10, 100};
const char logHeaderText1[] = "Datalog created by BOS V%d.%d.%d on %s\n";
const char logHeaderText2[] = "Log type: Rate @ %.2f Hz\n\n";
const char logHeaderText3[] = "Log type: Events\n\n";
const char logHeaderTimeDate[] = "%s %s\n";
const char Start_Mark = 0xff;

#define MAX_WAVE_NAME_LENTH 30
#define WAV_SCAN_MODE 			1
#define WAV_STREAM_MODE		 	2
#define LOG_MODE		 				0
#define WAVE_DATA_OFFSET		44

char Const_WAVE_NAME[MAX_WAVE_NAME_LENTH];

/*=================================================================================*/
/*========================= Private variables  ====================================*/
/*=================================================================================*/
uint8_t SD_MODE = 0;
log_t logs[MAX_LOGS] = {0};
logVar_t logVars[MAX_LOG_VARS] = {0};
uint32_t compareValue[MAX_LOG_VARS]; 
/* File system object for SD card logical drive */
FATFS SDFatFs; 
/* SD card logical drive path */
char SDPath[4]; 
/* File objects */
FIL MyFile, tempFile;    
/* File write/read counts */
uint32_t byteswritten, bytesread;                     
char lineBuffer[100];
char tempName[MAX_NAME_LENGTH] = {0};
uint16_t  activeLogs;
TaskHandle_t LogTaskHandle = NULL;
uint8_t temp_uint8 = 0; 
/* Module settings - sequential log naming*/
bool enableSequential = false;
bool enableTimeDateHeader = false;

/* WAVE file parameters */
uint8_t wavebuff[44];
UINT Number_br;
FIL _path_pointer;

struct /* WAVE FILE Header information struct - 44 bytes */
{
/*the "RIFF" chnk descriptor */
char CHUNKID[4];  						//4 byte
uint32_t CHUNKSIZE; 					//4 byte
char FORMAT[4];							 	//4 byte
/*the "fmt" sub-chunk */
char SUBCHhUNK1ID[4]; 				//4 byte
uint32_t SUBCHUNK1SIZE; 			//4 byte
uint16_t AUDIOFMT; 						//2 byte
uint16_t NO_CHANNEL; 					//2 byte
uint32_t SAMPLERATE; 					//4 byte
uint32_t BYTERATE; 						//4 byte
uint16_t BLOCKALIGN; 					//2 byte
uint16_t BITPERSAMPLE;			 	//2 byte
/*the "data" sub-chunk */
char SUBCHUNK2ID[4]; 					//4 byte
uint32_t SUBCHUNK2SIZE; 			//4 byte
	
}WAVEFIL;

uint32_t WAVE_bytes;
uint32_t READ_WAVE_BYTES=WAVE_DATA_OFFSET;		// WAVE header size in bytes
uint8_t SCALE_FAC=1;
uint8_t SCALE_SHIFT=0;
int16_t I;
uint8_t temp_H07R3_ID;
uint8_t temp_H07R3_DST;
//played wave name
char* WAVE_NAME;
//get wave file size
uint32_t WAVE_SIZE;
//calculate number of Byte in Block Sample align
uint8_t NO_BYTE_SAMPLE ;	
//calculate wave byte rate time wait in us 
uint16_t SAMPLETIME ;
//define origrn path pointer for f_close
FIL _wave_pointer;
//define wave BLOCKALIGN buffer
uint8_t WaveAlignBuff[500];			// NO_BYTE_SAMPLE
//define wave sample buffer
//uint8_t WaveSampleBuff[1000];		// WAVEFIL.BLOCKALIGN
uint8_t f_mount_ok=0 ;	
 
/*=================================================================================*/
/*========================= Private function prototypes ===========================*/
/*=================================================================================*/	
void LogTask(void * argument);
uint8_t CheckLogVarEvent(uint16_t varIndex);
Module_Status OpenThisLog(uint16_t logindex, FIL *objFile);
Module_Status MicroSD_Init(void);

/* Create CLI commands --------------------------------------------------------*/

portBASE_TYPE demoCommand( int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString );
portBASE_TYPE addLogCommand( int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString );
portBASE_TYPE deleteLogCommand( int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString );
portBASE_TYPE logVarCommand( int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString );
portBASE_TYPE startCommand( int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString );
portBASE_TYPE stopCommand( int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString );
portBASE_TYPE pauseCommand( int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString );
portBASE_TYPE resumeCommand( int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString );

/* CLI command structure : demo */
const CLI_Command_Definition_t demoCommandDefinition =
{
	( const int8_t * ) "demo", /* The command string to type. */
	( const int8_t * ) "demo:\r\n Run a demo program to test module functionality\r\n\r\n",
	demoCommand, /* The function to run. */
	0 /* No parameters are expected. */
};
/*-----------------------------------------------------------*/
/* CLI command structure : addlog */
const CLI_Command_Definition_t addLogCommandDefinition =
{
	( const int8_t * ) "addlog", /* The command string to type. */
	( const int8_t * ) "addlog:\r\n Add a new log file. Specifiy log name (1st par.); type (2nd par.): 'rate' or 'event'; \
rate (3rd par.): logging rate in Hz (max 1000), delimiter format (4th par.): 'space', 'tab' or 'comma'; index column format \
(5th par.): 'none', 'sample' or 'time'; and index column label text (6th par.)\r\n\r\n",
	addLogCommand, /* The function to run. */
	6 /* Six parameters are expected. */
};
/*-----------------------------------------------------------*/
/* CLI command structure : logvar */
const CLI_Command_Definition_t logVarCommandDefinition =
{
	( const int8_t * ) "logvar", /* The command string to type. */
	( const int8_t * ) "logvar:\r\n Add a new log variable to an existing log (1st par.). Specify variable type (2nd and 3rd par.): \
'port digital', 'port data', 'port button', 'memory uint8', 'memory int8', 'memory uint16', 'memory int16', 'memory uint32', \
'memory int32', 'memory float' ; source (4th par.): ports 'p1'..'px', buttons 'b1'..'bx' or memory location (Flash or RAM); \
and column label text (5th par.)\r\n\r\n",
	logVarCommand, /* The function to run. */
	5 /* Five parameters are expected. */
};
/*-----------------------------------------------------------*/
/* CLI command structure : deletelog */
const CLI_Command_Definition_t deleteLogCommandDefinition =
{
	( const int8_t * ) "deletelog", /* The command string to type. */
	( const int8_t * ) "deletelog:\r\n Delete a log file. Specifiy log name (1st par.) and delete options (2nd par.): 'all' or \
'keepdisk' to keep log on the uSD card\r\n\r\n",
	deleteLogCommand, /* The function to run. */
	2 /* Two parameters are expected. */
};
/*-----------------------------------------------------------*/
/* CLI command structure : start */
const CLI_Command_Definition_t startCommandDefinition =
{
	( const int8_t * ) "start", /* The command string to type. */
	( const int8_t * ) "start:\r\n Start the log with log name (1st par.)\r\n\r\n",
	startCommand, /* The function to run. */
	1 /* One parameter is expected. */
};
/*-----------------------------------------------------------*/
/* CLI command structure : stop */
const CLI_Command_Definition_t stopCommandDefinition =
{
	( const int8_t * ) "stop", /* The command string to type. */
	( const int8_t * ) "stop:\r\n Stop the log with log name (1st par.)\r\n\r\n",
	stopCommand, /* The function to run. */
	1 /* One parameter is expected. */
};
/*-----------------------------------------------------------*/
/* CLI command structure : pause */
const CLI_Command_Definition_t pauseCommandDefinition =
{
	( const int8_t * ) "pause", /* The command string to type. */
	( const int8_t * ) "pause:\r\n Pause the log with log name (1st par.)\r\n\r\n",
	pauseCommand, /* The function to run. */
	1 /* One parameter is expected. */
};
/*-----------------------------------------------------------*/
/* CLI command structure : resume */
const CLI_Command_Definition_t resumeCommandDefinition =
{
	( const int8_t * ) "resume", /* The command string to type. */
	( const int8_t * ) "resume:\r\n Resume the log with log name (1st par.)\r\n\r\n",
	resumeCommand, /* The function to run. */
	1 /* One parameter is expected. */
};
/*-----------------------------------------------------------*/

/* -----------------------------------------------------------------------
	|												 Private Functions	 														|
   ----------------------------------------------------------------------- 
*/

/* --- H1BR6 module initialization. 
*/
void Module_Init(void)
{
	/* Init global variables */
	enableSequential = true;
	enableTimeDateHeader = true;
	
	/* Array ports */
	MX_USART1_UART_Init();
	MX_USART2_UART_Init();
	MX_USART3_UART_Init();
	MX_USART4_UART_Init();
	MX_USART5_UART_Init();
	
	/* This module needs more time to process buttons */
	needToDelayButtonStateReset = true;
	
	/* Create the logging task */
	xTaskCreate(LogTask, (const char *) "LogTask", (2*configMINIMAL_STACK_SIZE), NULL, osPriorityNormal-osPriorityIdle, &LogTaskHandle);				
}

/*-----------------------------------------------------------*/

/* --- H1BR6 message processing task. 
*/
Module_Status Module_MessagingTask(uint16_t code, uint8_t port, uint8_t src, uint8_t dst, uint8_t shift)
{
	Module_Status result = H1BR6_OK;
	uint8_t templn;
	
	switch (code)
	{
		case CODE_H1BR6_READ_WAVE :
				//1st parameter H07R3x ID, 2nd parameter stream dst Port
				temp_H07R3_ID = cMessage[port-1][shift];
				temp_H07R3_DST = cMessage[port-1][shift+1];		// Note this is not used
				SD_MODE = WAV_STREAM_MODE;			
		break;
		
		case CODE_H1BR6_SCAN_WAVE :
		  templn = messageLength[port-1]-shift-1;		
			for (uint8_t i=0 ; i<templn ; i++)
			{
				Const_WAVE_NAME[i] = (char) cMessage[port-1][shift+1+i];
			}
			Const_WAVE_NAME[templn] = '.';
			Const_WAVE_NAME[templn+1] = 'w';
			Const_WAVE_NAME[templn+2] = 'a';
			Const_WAVE_NAME[templn+3] = 'v';
			Const_WAVE_NAME[templn+4] = 0;
			//1st parameter H07R3x ID, and for WAV name for the latest parameters
			WAVE_NAME = (char *) &Const_WAVE_NAME[0];
			temp_H07R3_ID = cMessage[port-1][shift];
			SD_MODE = WAV_SCAN_MODE;
		break;
			
		default:
			result = H1BR6_ERR_UnknownMessage;
			break;
	}			

	return result;	
}

/*-----------------------------------------------------------*/

/* --- Register this module CLI Commands 
*/
void RegisterModuleCLICommands(void)
{
	FreeRTOS_CLIRegisterCommand( &demoCommandDefinition );
	FreeRTOS_CLIRegisterCommand( &addLogCommandDefinition );
	FreeRTOS_CLIRegisterCommand( &deleteLogCommandDefinition );
	FreeRTOS_CLIRegisterCommand( &logVarCommandDefinition );
	FreeRTOS_CLIRegisterCommand( &startCommandDefinition );
	FreeRTOS_CLIRegisterCommand( &stopCommandDefinition );
	FreeRTOS_CLIRegisterCommand( &pauseCommandDefinition );
	FreeRTOS_CLIRegisterCommand( &resumeCommandDefinition );
}

/*-----------------------------------------------------------*/

/* --- Get the port for a given UART. 
*/
uint8_t GetPort(UART_HandleTypeDef *huart)
{
	if (huart->Instance == USART4)
			return P1;
	else if (huart->Instance == USART2)
			return P2;
	else if (huart->Instance == USART3)
			return P3;
	else if (huart->Instance == USART1)
			return P4;
	else if (huart->Instance == USART5)
			return P5;
	
	return 0;
}

/*-----------------------------------------------------------*/

/* --- Initialize the micro SD card hardware, link Fatfs and mount the drive. 
*/
Module_Status MicroSD_Init(void)
{	
  
	SD_CardInfo CardInfo;

	/* Initialize the SPI and the SD card */
	if (BSP_SD_Init() == MSD_ERROR) 
	{
		/* Initialization fail. Replace or re-insert the card and reboot */
		while(1) { RTOS_IND_blink(500); Delay_ms(500); }	
	}
	else
	{
		/* Get the uSD size and info */
		BSP_SD_GetCardInfo(&CardInfo);
		
		/* Link the micro SD disk I/O driver */
		if(FATFS_LinkDriver(&SD_Driver, SDPath) == 0)
		{
			/* Mount the default drive */
			if(f_mount(&SDFatFs, SDPath, 1) != FR_OK)
			{
				/* Unmount the drive */
				f_mount(0, SDPath, 0); 
				/* SD card malfunction. Replace or re-insert the card and reboot */
				while(1) { RTOS_IND_blink(500); Delay_ms(500); }	
			}
        f_mount_ok=1;
		}
	}
	
	return H1BR6_OK;
}
/*-----------------------------------------------------------*/

/* --- Logging task. 
*/
void LogTask(void * argument)
{	
  //uint8_t eventResult = 0;
	static uint8_t newLine = 1;
	static uint8_t resetButtonState = 0;
  volatile uint8_t i,j;
  volatile uint32_t u32lTick = 0;
  volatile uint32_t u32lRate = 0;

	/* Initialize the micro SD card */
	MicroSD_Init();
	
	/* Infinite loop */
	for(;;)
	{

		switch (SD_MODE)
		{
			case WAV_SCAN_MODE:	
						ScanWaveFile(WAVE_NAME, temp_H07R3_ID);
						SD_MODE = LOG_MODE;
						break;
			case WAV_STREAM_MODE:
						StreamWaveToModule(WAVE_NAME, temp_H07R3_ID);
						SD_MODE = LOG_MODE;
						break;
			case LOG_MODE:
				/* Check all active logs */
				for( j=0 ; j<MAX_LOGS ; j++)
				{
					u32lTick = HAL_GetTick()-logs[j].t0;
					u32lRate = configTICK_RATE_HZ/logs[j].rate;
					
					if ( u32lTick >= u32lRate )
						++logs[j].sampleCount;				// Advance one sample
					
					if ( (activeLogs >> j) & 0x01 )
					{			
						/* Open this log file if it's closed (and close open one) */
						OpenThisLog(j, &MyFile);
						
						memset(lineBuffer, 0, sizeof(lineBuffer));
						/* Check all registered variables for this log */
						for( i=0 ; i<MAX_LOG_VARS ; i++)
						{
							if (logVars[i].type && (logVars[i].logIndex == j))
							{
								/* Check for rate or event */
								if ( ((RATE == logs[j].type) && (u32lTick >= u32lRate)) || CheckLogVarEvent(i) )
								{					
									if (newLine)
									{
										newLine = 0;										// Event index written once per line     
										
										/* Write index */
										if (logs[j].indexColumnFormat == FMT_TIME)
										{
											GetTimeDate();
											sprintf(lineBuffer, "\n%02d:%02d:%02d-%03d", BOS.time.hours, BOS.time.minutes, BOS.time.seconds, BOS.time.msec);
										}
										else if (logs[j].indexColumnFormat == FMT_SAMPLE)
										{
											sprintf(lineBuffer, "\n%d", logs[j].sampleCount);
										}								
									}
								
									/* Write delimiter */
									switch(logs[j].delimiterFormat)
									{
										case FMT_SPACE:
											strcat(lineBuffer, " ");
											break;
										case FMT_TAB:
											strcat(lineBuffer, "\t");
											break;
										case FMT_COMMA:
											strcat(lineBuffer, ",");
											break;
										default:
											break;
									}
									
									/* Write variable value */
									switch (logVars[i].type)
									{
										case PORT_DIGITAL:
											//sprintf( ( char * ) buffer, "%d", HAL_GPIO_ReadPin());
											//f_write(&MyFile, buffer, 1, (void *)&byteswritten);	
											break;
										
										case PORT_BUTTON:
											switch (button[logVars[i].source].state)
											{
												case OFF:	strcat(lineBuffer, "OFF"); break; 
												case ON:	strcat(lineBuffer, "ON"); break; 
												case OPEN:	strcat(lineBuffer, "OPEN"); break; 
												case CLOSED:	strcat(lineBuffer, "CLOSED"); break; 
												case CLICKED:	strcat(lineBuffer, "CLICKED"); break; 
												case DBL_CLICKED:	strcat(lineBuffer, "DBL_CLICKED"); break; 
												case PRESSED:	strcat(lineBuffer, "PRESSED"); break; 
												case RELEASED:	strcat(lineBuffer, "RELEASED"); break; 
												case PRESSED_FOR_X1_SEC:
													if (button[logVars[i].source].pressedX1Sec)
														sprintf((char *)lineBuffer, "%sPRESSED_FOR_%d_SEC", (char *) lineBuffer, button[logVars[i].source].pressedX1Sec);
													break;
												case PRESSED_FOR_X2_SEC: 
													if (button[logVars[i].source].pressedX2Sec)
														sprintf((char *)lineBuffer, "%sPRESSED_FOR_%d_SEC", (char *) lineBuffer, button[logVars[i].source].pressedX2Sec);
													break;
												case PRESSED_FOR_X3_SEC: 
													if (button[logVars[i].source].pressedX3Sec)
														sprintf((char *)lineBuffer, "%sPRESSED_FOR_%d_SEC", (char *) lineBuffer, button[logVars[i].source].pressedX3Sec); 
													break;
												case RELEASED_FOR_Y1_SEC:	
													if (button[logVars[i].source].releasedY1Sec)
														sprintf((char *)lineBuffer, "%sRELEASED_FOR_%d_SEC", (char *) lineBuffer, button[logVars[i].source].releasedY1Sec); 
													break;
												case RELEASED_FOR_Y2_SEC:	
													if (button[logVars[i].source].releasedY2Sec)
														sprintf((char *)lineBuffer, "%sRELEASED_FOR_%d_SEC", (char *) lineBuffer, button[logVars[i].source].releasedY2Sec); 
													break;
												case RELEASED_FOR_Y3_SEC:	
													if (button[logVars[i].source].releasedY3Sec)
														sprintf((char *)lineBuffer, "%sRELEASED_FOR_%d_SEC", (char *) lineBuffer, button[logVars[i].source].releasedY3Sec); 
													break;
												case NONE: 
													if (logs[j].type == RATE) 
													{
														strcat(lineBuffer, "NORMAL");
													}												
													break;								
												default: 
													break;
											}
											/* Reset button state */
											if (NONE != button[logVars[i].source].state)	
											{
												resetButtonState = 1;
											}
											break;
										
										case PORT_DATA:
											
											break;
										
										case MEMORY_DATA_UINT8: 
											sprintf((char *)lineBuffer, "%s%u", (char *)lineBuffer, *(__IO uint8_t *)logVars[i].source);
											break;

										case MEMORY_DATA_INT8: 
											sprintf((char *)lineBuffer, "%s%d", (char *)lineBuffer, *(__IO int8_t *)logVars[i].source); 
											break;

										case MEMORY_DATA_UINT16: 
											sprintf((char *)lineBuffer, "%s%u", (char *)lineBuffer, *(__IO uint16_t *)logVars[i].source); 
											break;

										case MEMORY_DATA_INT16: 
											sprintf((char *)lineBuffer, "%s%d", (char *)lineBuffer, *(__IO int16_t *)logVars[i].source); 
											break;

										case MEMORY_DATA_UINT32: 
											sprintf((char *)lineBuffer, "%s%u", (char *)lineBuffer, *(__IO uint32_t *)logVars[i].source); 
											break;

										case MEMORY_DATA_INT32: 
											sprintf((char *)lineBuffer, "%s%d", (char *)lineBuffer, *(__IO int32_t *)logVars[i].source); 
											break;

										case MEMORY_DATA_FLOAT: 
											sprintf((char *)lineBuffer, "%s%f", (char *)lineBuffer, *(__IO float *)logVars[i].source);
											break;
										
										default:			
											break;
									}
								}
							}			
						}
						/* Write the lineBuffer into log file */
						if (0 == newLine)
						{
							f_write(&MyFile, lineBuffer, strlen((const char *)lineBuffer), (void *)&byteswritten);
							newLine = 1;            /* Start a new line entry */
						}					
						f_close(&MyFile);
						/* Reset the rate timer */	
						if (u32lTick >= u32lRate)
						{
							logs[j].t0 = HAL_GetTick();
						}
					}	
					else	
					{
						continue;
					}
				}
				break;
		}
		
    /* Reset button state */
    if (resetButtonState)	
    {
      delayButtonStateReset = false;
      resetButtonState = 0;
    }
	  taskYIELD(); 
	}								
	
}

/*-----------------------------------------------------------*/

/* --- Check if a logged variable event has occured. 
				varIndex: Log variable array index.
*/
uint8_t CheckLogVarEvent(uint16_t varIndex)
{
	switch (logVars[varIndex].type)
	{
		case PORT_DIGITAL:
			break;
		
		case PORT_BUTTON:
			if ((button[logVars[varIndex].source].state != temp_uint8) && (button[logVars[varIndex].source].state != 0)) {
				temp_uint8 = button[logVars[varIndex].source].state;
				return 1;
			} else if ((button[logVars[varIndex].source].state != temp_uint8) && (button[logVars[varIndex].source].state == 0)) {
				temp_uint8 = button[logVars[varIndex].source].state;
				return 0;
			}
			break;
		
		case PORT_DATA:			
			break;
		
		case MEMORY_DATA_UINT8:
			if (*(__IO uint8_t *)logVars[varIndex].source != (uint8_t)compareValue[varIndex]) {
				*(uint8_t*)&compareValue[varIndex] = *(__IO uint8_t *)logVars[varIndex].source;
				return 1;
			}
			break;
			
		case MEMORY_DATA_INT8:
			if (*(__IO int8_t *)logVars[varIndex].source != (int8_t)compareValue[varIndex]) {
				*(int8_t*)&compareValue[varIndex] = (int8_t)*(__IO int8_t *)logVars[varIndex].source;
				return 1;
			}
			break;
			
		case MEMORY_DATA_UINT16:
			if ((uint16_t)*(__IO uint16_t *)logVars[varIndex].source != (uint16_t)compareValue[varIndex]) {
				*(uint16_t*)&compareValue[varIndex] = (uint16_t)*(__IO uint16_t *)logVars[varIndex].source;
				return 1;
			}
			break;
			
		case MEMORY_DATA_INT16:
			if ((int16_t)*(__IO uint16_t *)logVars[varIndex].source != (int16_t)compareValue[varIndex]) {
				*(int16_t*)&compareValue[varIndex] = (int16_t)*(__IO uint16_t *)logVars[varIndex].source;
				return 1;
			}
			break;
			
		case MEMORY_DATA_UINT32:
		case MEMORY_DATA_FLOAT:
			if ((uint32_t)*(__IO uint32_t *)logVars[varIndex].source != (uint32_t)compareValue[varIndex]) {
				compareValue[varIndex] = *(__IO uint32_t *)logVars[varIndex].source;
				return 1;
			}
			break;
			
		case MEMORY_DATA_INT32:
			if ((int32_t)*(__IO uint32_t *)logVars[varIndex].source != (int32_t)compareValue[varIndex]) {
				compareValue[varIndex] = *(__IO uint32_t *)logVars[varIndex].source;
				return 1;
			}
			break;
			
		/*case MEMORY_DATA_FLOAT:
			if (*(__IO uint32_t *)logVars[varIndex].source != (uint32_t)compareValue[varIndex]) {
				compareValue[varIndex] = *(__IO uint32_t *)logVars[varIndex].source;
				return 1;
			}
			break;*/
					
		default:			
			break;
	}
	
	return 0;	
} 

/*-----------------------------------------------------------*/

/* --- Open log file if it's closed (and close open one). 
				logindex: Log array index.
*/
Module_Status OpenThisLog(uint16_t logindex, FIL *objFile)
{ 
  while(f_mount_ok==0){Delay_us(10);}		// Add a flag to allow card to be initialized on startup
	FRESULT res; 
	/* Append log name with extension */
	if ((0U != logs[logindex].file_extension) && (true == enableSequential))
	{
		sprintf((char *)tempName, "%s_%d%s", logs[logindex].name, logs[logindex].file_extension, ".TXT");
	}
	else
	{
		sprintf((char *)tempName, "%s%s", logs[logindex].name, ".TXT");
	}
	/* Open this log */			
	res = f_open(objFile, tempName, FA_OPEN_APPEND | FA_WRITE | FA_READ);
	if (res != FR_OK)	
		return H1BR6_ERROR;	
	return H1BR6_OK;
} 

/*-----------------------------------------------------------*/

/**
	READ_WAVE_FILE_HEADER
				Read Wave file .wav header information 
				this func called by StreamWaveToModule
* @param Wave_Path : Wave full file name
*/
WAVE_STATE READ_WAVE_FILE_HEADER(char* Wave_Path)
{
  while(f_mount_ok==0){Delay_us(10);}		// Add a flag to allow card to be initialized on startup

	//try to open wave file
	if (f_open (&_path_pointer,Wave_Path,FA_READ)==FR_OK)
	{
				//read header from wave file
				if(f_read ( &_path_pointer,  &wavebuff, 44,  &Number_br)!=FR_OK)
				{
					//close wave file	
					f_close (&_path_pointer);
					return WAVE_FILE_READ_FAILD;
				}	
				//close wave file	
				f_close (&_path_pointer);
	}
	else
	{
		f_close (&_path_pointer);
		return	WAVE_FILE_OPEN_FAILD;
	}


	//GET CHUNK descriptor
	for(uint8_t _i=0 ; _i<4 ; _i++)				
	{
	WAVEFIL.CHUNKID[_i] = wavebuff[_i];
	WAVEFIL.FORMAT[_i] = wavebuff[_i+8];
	WAVEFIL.SUBCHhUNK1ID[_i] = wavebuff[_i+12];
	WAVEFIL.SUBCHUNK2ID[_i] = wavebuff[_i+36];
	}
			
	//GET CHUNK SIZE	
	WAVEFIL.CHUNKSIZE = (wavebuff[7]<<24)+(wavebuff[6]<<16)+(wavebuff[5]<<8)+wavebuff[4];
	WAVEFIL.SUBCHUNK1SIZE = (wavebuff[19]<<24)+(wavebuff[18]<<16)+(wavebuff[17]<<8)+wavebuff[16];
	WAVEFIL.SUBCHUNK2SIZE = (wavebuff[43]<<24)+(wavebuff[42]<<16)+(wavebuff[41]<<8)+wavebuff[40];
	
	//GET AUDIO descriptor		
	WAVEFIL.AUDIOFMT = (wavebuff[21]<<8)+wavebuff[20];
	WAVEFIL.NO_CHANNEL = (wavebuff[23]<<8)+wavebuff[22];
	WAVEFIL.SAMPLERATE = (wavebuff[27]<<24)+(wavebuff[26]<<16)+(wavebuff[25]<<8)+wavebuff[24];
	WAVEFIL.BYTERATE = (wavebuff[31]<<24)+(wavebuff[30]<<16)+(wavebuff[29]<<8)+wavebuff[28];
	WAVEFIL.BLOCKALIGN = (wavebuff[33]<<8)+wavebuff[32];
	WAVEFIL.BITPERSAMPLE = (wavebuff[35]<<8)+wavebuff[34];
	
	/* Test file header is correct */
		if(WAVEFIL.CHUNKID[1] != 'R' && WAVEFIL.CHUNKID[1] != 'I' &&  WAVEFIL.CHUNKID[1] != 'F' &&  WAVEFIL.CHUNKID[1] != 'F')	
				{return HEADER_CHUNK_FAULT;}
		else if (WAVEFIL.FORMAT[1] != 'W' && WAVEFIL.FORMAT[1] != 'A' &&  WAVEFIL.FORMAT[1] != 'V' &&  WAVEFIL.FORMAT[1] != 'E')
				{return HEADER_CHUNK_FAULT;}
		else if (WAVEFIL.SUBCHhUNK1ID[1] != 'f' && WAVEFIL.SUBCHhUNK1ID[1] != 'm' &&  WAVEFIL.SUBCHhUNK1ID[1] != 't' &&  WAVEFIL.SUBCHhUNK1ID[1] != 0x20)
				{return HEADER_CHUNK_FAULT;}
		else if (WAVEFIL.SUBCHUNK2ID[1] != 'd' && WAVEFIL.SUBCHUNK2ID[1] != 'a' &&  WAVEFIL.SUBCHUNK2ID[1] != 't' &&  WAVEFIL.SUBCHUNK2ID[1] != 'a')
				{return HEADER_CHUNK_FAULT;}
		else if (WAVEFIL.AUDIOFMT != 1)  // Audio Format != PCM
				{return HEADER_CHUNK_FAULT;}
		else if (WAVEFIL.NO_CHANNEL > 2)  // Number of audio channel more than 2 channel
				{return HEADER_CHUNK_FAULT;}
		else if (WAVEFIL.SUBCHUNK1SIZE != 0x10)  // chunk size 2 error must be 16 byte
				{return HEADER_CHUNK_FAULT;}
		else if (WAVEFIL.BITPERSAMPLE > 16 ) 		// bit per sample more than 16 bit
				{return BITPERSAMPLE_ERR;}
		else 
				{
				//get wave file size in samples and bytes
				WAVE_SIZE	=	WAVEFIL.SUBCHUNK2SIZE + READ_WAVE_BYTES;
				WAVE_bytes	= WAVEFIL.SUBCHUNK2SIZE/((WAVEFIL.BITPERSAMPLE/8)*WAVEFIL.NO_CHANNEL);
				//calculate number of Byte in Block Sample align
				NO_BYTE_SAMPLE =	WAVEFIL.BLOCKALIGN*(WAVEFIL.BITPERSAMPLE/8)*WAVEFIL.NO_CHANNEL;	
				//calculate wave byte rate time wait in us 
				SAMPLETIME = (1000000/WAVEFIL.SAMPLERATE)-20;
				return HEADER_CHUNK_OK; 
					
				}
}

/*-----------------------------------------------------------*/

/**
	StreamWaveToPort
				Stream Wave file sound samples thru uart
				this func called by StreamWaveToModule
* @param Wave_Path : Wave full file name
* @param _port : H1BR6x stramming port
*/
WAVE_STATE StreamWaveToPort(char* Wave_Path, uint8_t _port)
{
	  READ_WAVE_BYTES=44;
		SCALE_FAC=1;
		SCALE_SHIFT=0;
				
//		WAVEFIL.BITPERSAMPLE = 8;
		if (WAVEFIL.NO_CHANNEL == 1 && WAVEFIL.BITPERSAMPLE == 8)
				{SCALE_FAC=1;SCALE_SHIFT=0;}				//read sample channel
		else if (WAVEFIL.NO_CHANNEL == 1 && WAVEFIL.BITPERSAMPLE == 16)
				{SCALE_FAC=2;SCALE_SHIFT=0;} 		//read sample MSB from channel
		else if (WAVEFIL.NO_CHANNEL == 2 && WAVEFIL.BITPERSAMPLE == 8)
				{SCALE_FAC=2;SCALE_SHIFT=1;}			//read sample from right channel
		else if (WAVEFIL.NO_CHANNEL == 2 && WAVEFIL.BITPERSAMPLE == 16)
				{SCALE_FAC=4;SCALE_SHIFT=2;}			//read sample from MSB right channel
		else {return HEADER_CHUNK_FAULT;}

		
		if (f_open (&_path_pointer,Wave_Path,FA_READ)!=FR_OK) {
			f_close(&_path_pointer);
			return	WAVE_FILE_OPEN_FAILD;
		} else {
			_wave_pointer=_path_pointer;
		}
		
		__TIM16_CLK_ENABLE();	

		/* Peripheral configuration */	
		htim16.Instance = TIM16;
		htim16.Init.Prescaler = 0;
		htim16.Init.CounterMode = TIM_COUNTERMODE_UP;
		htim16.Init.Period = (uint16_t)((SystemCoreClock/WAVEFIL.SAMPLERATE)-1);
		HAL_TIM_Base_Init(&htim16);
		HAL_NVIC_SetPriority(TIM16_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(TIM16_IRQn);
		HAL_TIM_Base_Start_IT(&htim16);
		portENTER_CRITICAL();
		
		do
		{		
      f_lseek (&_path_pointer,READ_WAVE_BYTES);
			f_read (&_path_pointer, &WaveAlignBuff, 500,  &Number_br);
			
			for(uint8_t Align=0 ; Align<WAVEFIL.BLOCKALIGN ;Align++)
			{		
				//stream to port 'send one sample from port'	
				for( I=0 ; I<500 ;I++)
			{
				writePxMutex(_port,(char *) &WaveAlignBuff[I],1,0,0);
				Delay_us(SAMPLETIME);
			}
				while (__HAL_TIM_GET_FLAG(&htim16, TIM_FLAG_UPDATE) == false){};
			}					
			READ_WAVE_BYTES +=500;
			
		} while(READ_WAVE_BYTES <= WAVE_SIZE);
  	portEXIT_CRITICAL();
		f_close (&_wave_pointer);
		HAL_TIM_Base_Stop(&htim16);
		Delay_ms(20);
		// Reset size variables for next transfer
		WAVE_bytes = 0;
		WAVE_SIZE = 0;
		
		return STREAM_WAVE_OK;
}

/*-----------------------------------------------------------*/

/**
	ScanWaveFile
				scan for Wave file sound and send responce to H07R3x 
				this func called by message parser
* @param Wave_Full_Name : Wave full file name
* @param H07R3x_ID : H07R3 module ID
*/
WAVE_STATE ScanWaveFile(char* Wave_Full_Name , uint8_t H07R3x_ID)
{
	  while(f_mount_ok==0){Delay_us(10);}		// Add a flag to allow card to be initialized on startup
	
	WAVE_STATE result;
	result = READ_WAVE_FILE_HEADER(Wave_Full_Name);
	
	//send responce to H07R3x
	if(result == HEADER_CHUNK_OK)
	{
		//send wave file Data size in bytes
		messageParams[0]=(uint8_t) (WAVE_bytes>>24);
		messageParams[1]=(uint8_t) (WAVE_bytes>>16);
		messageParams[2]=(uint8_t) (WAVE_bytes>>8);
		messageParams[3]=(uint8_t)  WAVE_bytes;
		//send wave file sample rate
		messageParams[4]=(uint8_t) (WAVEFIL.SAMPLERATE>>24);
		messageParams[5]=(uint8_t) (WAVEFIL.SAMPLERATE>>16);
		messageParams[6]=(uint8_t) (WAVEFIL.SAMPLERATE>>8);
		messageParams[7]=(uint8_t)  WAVEFIL.SAMPLERATE;
	}
	else
	{
		//send error param
		messageParams[0]=(uint8_t) 0xFF;
		messageParams[1]=(uint8_t) 0xFF;
		messageParams[2]=(uint8_t) 0xFF;
		messageParams[3]=(uint8_t) 0xFF;
		//send error param
		messageParams[4]=(uint8_t) 0xFF;
		messageParams[5]=(uint8_t) 0xFF;
		messageParams[6]=(uint8_t) 0xFF;
		messageParams[7]=(uint8_t) 0xFF;
	}
	IND_blink(100);
	SendMessageToModule(H07R3x_ID, CODE_H07R3_SCAN_WAVE_RESPONSE,8);
	
	return result;
}

/*-----------------------------------------------------------*/


/* -----------------------------------------------------------------------
	|																APIs	 																 	|
   ----------------------------------------------------------------------- 
*/

/* --- Create a new data log. 
				logName: Log file name. Max 10 char. 
				type: RATE or EVENT
				rate: data rate in Hz (max 1000 Hz).
				delimiterFormat: FMT_SPACE, FMT_TAB, FMT_COMMA
				indexColumn: FMT_SAMPLE, FMT_TIME
				indexColumnLabel: Index Column label text. Max 30 char.
*/
Module_Status CreateLog(char* logName, logType_t type, float rate, delimiterFormat_t delimiterFormat, indexColumnFormat_t indexColumnFormat,\
	char* indexColumnLabel)
{ 
  while(f_mount_ok==0){Delay_us(10);}  // Add a flag to allow card to be initialized on startup
  FRESULT res; 
	uint8_t i=0;
	uint8_t countFile = 0;
	char *pChar = NULL;
	uint8_t length = 0;
	bool extensionFile = false;
	uint8_t position = 0;
	/* Check if log already exists */
	for( i=0 ; i<MAX_LOGS ; i++)
	{
		if((0U != logs[i].current_extension)  && (true == enableSequential))
		{
			sprintf(tempName, "%s_%d", logs[i].name, logs[i].current_extension);
		}
		else
		{
			sprintf(tempName, "%s", logs[i].name);
		}
		if(!strcmp(tempName, logName))
		{
			return H1BR6_ERR_LogNameExists;
		}
	}
	
	/* Check parameters are correct */
	if ( (type != RATE && type != EVENT)	||
			 (delimiterFormat != FMT_SPACE && delimiterFormat != FMT_TAB && delimiterFormat != FMT_COMMA)	||
			 (indexColumnFormat != FMT_NONE && indexColumnFormat != FMT_SAMPLE && indexColumnFormat != FMT_TIME)	||
			 (rate > 1000) )
		return H1BR6_ERR_WrongParams;				
					
	/* Name does not exist. Fill first empty location */
	for( i=0 ; i<MAX_LOGS ; i++)
	{
		if(logs[i].name == 0)
		{
			if (true == enableSequential)
			{
				pChar = strchr(logName , '_');
				while (pChar != NULL)
				{
					position = (uint8_t)((uint32_t)pChar - (uint32_t)logName + 1UL);
					pChar = strchr(pChar + 1 , '_');
				}
				/* */
				if(0 != position)
				{
					pChar = logName + position;
			
					length = strlen(pChar);
					while ('\0' != (char)*pChar)
					{
						if((0x30 <= *pChar) && (*pChar <= 0x39))
						{
							countFile += ((uint8_t)(*pChar - 0x30) * numberMap[length - 1]);
						}
						else
						{
							countFile = 0;
							break;
						}
						pChar++;
						length--;
					}
				}
				else
				{
					countFile = 0;
					logs[i].current_extension = 0;
				}
				
				if (countFile != 0)
				{
					extensionFile = true;
					logs[i].current_extension = countFile ;
				}
				else
				{
					position = 0;
					extensionFile = false;
				}
			}
			/* Append log name with extension */
			sprintf((char *)tempName, "%s%s", logName, ".TXT");
			/* Check if file exists on disk */
			res = f_open(&tempFile, tempName, FA_CREATE_NEW | FA_WRITE | FA_READ);
			if ((false == enableSequential) && (res == FR_EXIST))
			{
				return H1BR6_ERR_LogNameExists;		
			}
			else if ((res != FR_OK) && (FR_EXIST != res))
			{
				return H1BR6_ERR_SD;
			}
			else if ((true == enableSequential) && (res == FR_EXIST))
			{
				countFile = 0;
				do
				{
					memset((char *)tempName, 0, sizeof(tempName));
					if(false == extensionFile)
					{
						countFile++;
						sprintf(tempName, "%s_%d%s", logName, countFile, ".TXT");
					}
					else
					{
						strncpy(tempName, logName, (size_t)((uint32_t)position - 1));
						if(0U == countFile)
						{
							strncat ((char *)tempName, ".TXT", 4);
						}
						else
						{
							sprintf(tempName, "%s_%d%s", tempName, countFile, ".TXT");
						}
						countFile++;
					}
					res = f_open(&tempFile, tempName, FA_CREATE_NEW | FA_WRITE | FA_READ);
				}while ((FR_EXIST == res) && (MAX_DUPLICATE_FILE > countFile));
				
				if((MAX_DUPLICATE_FILE == countFile) && (FR_EXIST == res))
				{
					return H1BR6_ERR_LogNameExists;
				}
				else if (FR_OK != res)
				{
					return H1BR6_ERR_SD;
				}
				
				if(true == extensionFile)
				{
					countFile--;
				}
			}
			
			/* Log created successfuly */
			if ((true == enableSequential) && (0U != position))
			{
				logs[i].name = malloc((size_t)position);
				memset(logs[i].name, 0x00U, (size_t)position);
				strncpy(logs[i].name, tempName, (size_t)(position - 1));
			}
			else
			{
				length = strlen(logName);
				logs[i].name = malloc(length + 1);
				memset(logs[i].name, 0x00U, (size_t)(length + 1));
				strncpy(logs[i].name, logName, (size_t)length);
			}
			logs[i].file_extension = countFile;
			logs[i].type = type;
			logs[i].rate = rate;
			logs[i].delimiterFormat = delimiterFormat;
			logs[i].indexColumnFormat = indexColumnFormat;
			logs[i].indexColumnLabel = indexColumnLabel;
			
			/* Write log header */
			char *buffer = malloc(100);
			memset (buffer, 0x00, 100);
			sprintf(buffer, logHeaderText1, _firmMajor, _firmMinor, _firmPatch, modulePNstring[myPN]);
			res = f_write(&tempFile, buffer, strlen(buffer), (void *)&byteswritten);
			if (enableTimeDateHeader)
			{
				GetTimeDate();
				sprintf(buffer, logHeaderTimeDate, GetDateString(), GetTimeString());
				res = f_write(&tempFile, buffer, strlen(buffer), (void *)&byteswritten);
			}
			if(type == RATE) 
			{
				sprintf(buffer, logHeaderText2, rate);
				res = f_write(&tempFile, buffer, strlen(buffer), (void *)&byteswritten);
			} 
			else if (type == EVENT) 
			{
				res = f_write(&tempFile, logHeaderText3, strlen(logHeaderText3), (void *)&byteswritten);	
			}
			
			/* Write index label */
			res = f_write(&tempFile, indexColumnLabel, strlen(indexColumnLabel), (void *)&byteswritten);
			
			f_close(&tempFile);
			free(buffer);
			
			return H1BR6_OK;
		}
  }	

	return H1BR6_ERR_MaxLogs;	
}

/*-----------------------------------------------------------*/

/* --- Save data from a source to an existing data log. 
				logName: Log file name.
				type: PORT_DIGITAL, PORT_DATA, PORT_BUTTON, MEMORY_DATA.
				source: data source. Ports (P1-Px), buttons (B1-Bx) or memory location.
				columnLabel: Column label text. Max 30 char.
*/
Module_Status LogVar(char* logName, logVarType_t type, uint32_t source, char* ColumnLabel)
{
	uint8_t i = 0, j = 0;

	/* Search for this log to make sure it exists */
	for( j=0 ; j<MAX_LOGS ; j++)
	{
		if((0 != logs[j].current_extension) && (true == enableSequential))
		{
			sprintf(tempName, "%s_%d", logs[j].name, logs[j].current_extension);
		}
		else
		{
			sprintf(tempName, "%s", logs[j].name);
		}
		
		if (!strcmp(tempName, logName))
		{
			/* Make sure there's enough space for this log variable */
			for( i=0 ; i<MAX_LOG_VARS ; i++)
			{
				if(logVars[i].type == 0)
				{		
					logVars[i].type = type;
					if(type > 3) {
					if ((source < FLASH_BASE || source > (FLASH_BASE+FLASH_SIZE)) && (source < SRAM_BASE || source > (SRAM_BASE+SRAM_SIZE)) && (source < PERIPH_BASE || source > (PERIPH_BASE+PERIPH_SIZE)))
								return H1BR6_ERR_WrongAddress;}
					
					logVars[i].source = source;
					logVars[i].logIndex = j;
					logVars[i].varLabel = ColumnLabel;
					
					/* Write delimiter */
					OpenThisLog(j, &tempFile);
					if (logs[j].delimiterFormat == FMT_SPACE)
						f_write(&tempFile, " ", 1, (void *)&byteswritten);
					else if (logs[j].delimiterFormat == FMT_TAB)
						f_write(&tempFile, "\t", 1, (void *)&byteswritten);
					else if (logs[j].delimiterFormat == FMT_COMMA)
						f_write(&tempFile, ",", 1, (void *)&byteswritten);
					/* Write variable label */
					f_write(&tempFile, ColumnLabel, strlen(ColumnLabel), (void *)&byteswritten);
					
					f_close(&tempFile);
					
					return H1BR6_OK;
				}
			}
			return H1BR6_ERR_MaxLogVars;
		}
	}		

	return H1BR6_ERR_LogDoesNotExist;	
}

/*-----------------------------------------------------------*/

/* --- Start an existing data log. 
				logName: Log file name.
*/
Module_Status StartLog(char* logName)
{
	uint8_t j = 0;

	/* Search for this log to make sure it exists */
	for( j=0 ; j<MAX_LOGS ; j++)
	{
		if((0U != logs[j].current_extension) && (true == enableSequential))
		{
			sprintf(tempName, "%s_%d", logs[j].name, logs[j].current_extension);
		}
		else
		{
			sprintf(tempName, "%s", logs[j].name);
		}
		if (!strcmp(tempName, logName))
		{
			activeLogs |= (0x01 << j);
			logs[j].t0 = HAL_GetTick();
			logs[j].sampleCount = 1;
			OpenThisLog(j, &tempFile);
			/* Write new line */
			f_write(&tempFile, "\n\r", 2, (void *)&byteswritten);		
			f_close(&tempFile);
			
			return H1BR6_OK;
		}		
	}

	return H1BR6_ERR_LogDoesNotExist;	
}

/*-----------------------------------------------------------*/

/* --- Stop a running data log. 
				logName: Log file name.
*/
Module_Status StopLog(char* logName)
{
	volatile uint8_t j = 0;

	/* Search for this log to make sure it exists */
	for( j=0 ; j<MAX_LOGS ; j++)
	{
		if((0U != logs[j].current_extension) && (true == enableSequential))
		{
			sprintf(tempName, "%s_%d", logs[j].name, logs[j].current_extension);
		}
		else
		{
			sprintf(tempName, "%s", logs[j].name);
		}
		if (!strcmp(tempName, logName))
		{
			if ( (activeLogs >> j) & 0x01 )
			{
				/* StopLog only inactive log, don't reset variable*/
				activeLogs &= ~(0x01 << j);
				return H1BR6_OK;
			}
			else
			{
				return H1BR6_ERR_LogIsNotActive;
			}
		}		
	}
	return H1BR6_ERR_LogDoesNotExist;	
}

/*-----------------------------------------------------------*/

/* --- Pause a running data log. 
				logName: Log file name.
*/
Module_Status PauseLog(char* logName)
{
	uint8_t j = 0;

	/* Search for this log to make sure it exists */
	for( j=0 ; j<MAX_LOGS ; j++)
	{
		if((0U != logs[j].current_extension)  && (true == enableSequential))
		{
			sprintf(tempName, "%s_%d", logs[j].name, logs[j].current_extension);
		}
		else
		{
			sprintf(tempName, "%s", logs[j].name);
		}
		if (!strcmp(tempName, logName))
		{
			if ( (activeLogs >> j) & 0x01 )
			{
				activeLogs &= ~(0x01 << j);
				return H1BR6_OK;
			}
			else
				return H1BR6_ERR_LogIsNotActive;
		}		
	}

	return H1BR6_ERR_LogDoesNotExist;	
} 

/*-----------------------------------------------------------*/

/* --- Resume a paused data log. 
				logName: Log file name.
*/
Module_Status ResumeLog(char* logName)
{
	uint8_t j = 0;

	/* Search for this log to make sure it exists */
	for( j=0 ; j<MAX_LOGS ; j++)
	{
		if((0U != logs[j].current_extension)  && (true == enableSequential))
		{
			sprintf(tempName, "%s_%d", logs[j].name, logs[j].current_extension);
		}
		else
		{
			sprintf(tempName, "%s", logs[j].name);
		}
		if (!strcmp(tempName, logName))
		{
			activeLogs |= (0x01 << j);
			return H1BR6_OK;
		}		
	}

	return H1BR6_ERR_LogDoesNotExist;	
} 

/*-----------------------------------------------------------*/

/* --- Delete an existing data log. 
				logName: Log file name.
				options: DELETE_ALL, KEEP_ON_DISK
*/
Module_Status DeleteLog(char* logName, options_t options)
{
	Module_Status result = H1BR6_OK;

	
	// Free ptemp vars
	
	return result;	
}

/*-----------------------------------------------------------*/

/**
	StreamWaveToModule
				Stream Wave file sound samples to specified module
				this API called by message parser when receiving CODE_H1BR6_READ_WAVE.
				Make sure destination module is configured in PlayWaveFromPort mode.
* @param Wave_Path : WAVE file name  with extension
* @param H07R3x_ID : distant H07R3 module ID
*/

WAVE_STATE StreamWaveToModule(char* Wave_Full_Name, uint8_t H07R3x_ID)
{
	while(f_mount_ok==0){Delay_us(10);}		// Add a flag to allow card to be initialized on startup
	
	if ( Wave_Full_Name != NULL )
	{
			uint8_t port;
		
			if (!WAVE_bytes)		// Read file header if it was not scanned before
			{
				WAVE_STATE result = READ_WAVE_FILE_HEADER(Wave_Full_Name);

				if(result != HEADER_CHUNK_OK)	return WAVE_FILE_READ_FAILD;
		
				// calculate number of bytes to be streamed
				WAVE_bytes	= WAVEFIL.SUBCHUNK2SIZE/((WAVEFIL.BITPERSAMPLE/8)*WAVEFIL.NO_CHANNEL);
			}
		
			// Find best output port for destination module 
			port = FindRoute(myID, H07R3x_ID); 
		
			// Start a single-cast DMA stream across the array.	
			if ( StartScastDMAStream(0, myID, 0, H07R3x_ID, FORWARD, WAVE_bytes, 0xFFFFFFFF, 0) != BOS_OK )
					{return STREAM_WAVE_FAILD;}
			//Delay_ms(100);
			//IND_blink(100);	
					
			return (StreamWaveToPort(Wave_Full_Name, port));
	}
	return WAVE_FILE_READ_FAILD;
}

/*-----------------------------------------------------------*/

/* -----------------------------------------------------------------------
	|															Commands																 	|
   ----------------------------------------------------------------------- 
*/

portBASE_TYPE demoCommand( int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString )
{
	static const int8_t *pcOpenMessage = ( int8_t * ) "Test file created\r\n";
	static const int8_t *pcVerifyMessage = ( int8_t * ) "Write/read operations verified\r\n";
	static const int8_t *pcDeleteMessage = ( int8_t * ) "Test file deleted\r\n";
	static const int8_t *pcFileMessage = ( int8_t * ) "Testing failed\r\n";
	FRESULT res;
	
	/* Remove compile time warnings about unused parameters, and check the
	write buffer is not NULL.  NOTE - for simplicity, this example assumes the
	write buffer length is adequate, so does not check for buffer overflows. */
	( void ) pcCommandString;
	( void ) xWriteBufferLen;
	configASSERT( pcWriteBuffer );

	/* Create a file */
	res = f_open(&tempFile, "TestFile", FA_CREATE_ALWAYS | FA_WRITE | FA_READ);
	if (res != FR_OK) {
		strcpy( ( char * ) pcWriteBuffer, ( char * ) pcFileMessage);		
		return pdFALSE;
	}
	writePxMutex(PcPort, ( char * ) pcOpenMessage, strlen(( char * ) pcOpenMessage), 10, 10);
	
	/* Verify read / write */
	res = f_write(&tempFile, "HEXABITZ", 8, (void *)&byteswritten);
	if (res != FR_OK) {
		strcpy( ( char * ) pcWriteBuffer, ( char * ) pcFileMessage);		
		return pdFALSE;
	}	
	char tempStr[10] = {0};
	res = f_lseek(&tempFile, 0);
	res = f_read(&tempFile, tempStr, 8, (void *)&byteswritten);
	if (res != FR_OK || strncmp(tempStr, "HEXABITZ", 8) != 0) {
		strcpy( ( char * ) pcWriteBuffer, ( char * ) pcFileMessage);		
		return pdFALSE;
	}		
	writePxMutex(PcPort, ( char * ) pcVerifyMessage, strlen(( char * ) pcVerifyMessage), 10, 10);
	
	/* Close and delete the file */
	res = f_close(&tempFile);
	res = f_unlink("TestFile");
	if (res != FR_OK) {
		strcpy( ( char * ) pcWriteBuffer, ( char * ) pcFileMessage);		
		return pdFALSE;
	}	
	strcpy( ( char * ) pcWriteBuffer, ( char * ) pcDeleteMessage);
	
	/* There is no more data to return after this single string, so return
	pdFALSE. */
	return pdFALSE;
}

/*-----------------------------------------------------------*/

portBASE_TYPE addLogCommand( int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString )
{
	Module_Status result = H1BR6_OK;
	
	int8_t *pcParameterString1, *pcParameterString2, *pcParameterString3, *pcParameterString4, *pcParameterString5, *pcParameterString6; 
	portBASE_TYPE xParameterStringLength1 = 0, xParameterStringLength2 = 0, xParameterStringLength3 = 0; 
	portBASE_TYPE xParameterStringLength4 = 0, xParameterStringLength5 = 0, xParameterStringLength6 = 0;
	logType_t type; delimiterFormat_t dformat; indexColumnFormat_t iformat; float rate;
	char *name, *index;
	static const int8_t *pcOKMessage = ( int8_t * ) "Log created successfully\r\n";
	static const int8_t *pcWrongValue = ( int8_t * ) "Log creation failed. Wrong parameters\r\n";
	static const int8_t *pcLogExists = ( int8_t * ) "Log creation failed. Log name already exists\r\n";
	static const int8_t *pcSDerror = ( int8_t * ) "Log creation failed. SD card error\r\n";
	static const int8_t *pcMaxLogs = ( int8_t * ) "Log creation failed. Maximum number of logs reached\r\n";
	static const int8_t *pcMemoryFull = ( int8_t * ) "Variable was not added to log. Internal memory full\r\n";
	
	/* Remove compile time warnings about unused parameters, and check the
	write buffer is not NULL.  NOTE - for simplicity, this example assumes the
	write buffer length is adequate, so does not check for buffer overflows. */
	( void ) xWriteBufferLen;
	configASSERT( pcWriteBuffer );
	
	/* Obtain the 1st parameter string: log name */
	pcParameterString1 = ( int8_t * ) FreeRTOS_CLIGetParameter (pcCommandString, 1, &xParameterStringLength1);
	/* Obtain the 2nd parameter string: log type */
	pcParameterString2 = ( int8_t * ) FreeRTOS_CLIGetParameter (pcCommandString, 2, &xParameterStringLength2);
	/* Obtain the 3rd parameter string: log rate */
	pcParameterString3 = ( int8_t * ) FreeRTOS_CLIGetParameter (pcCommandString, 3, &xParameterStringLength3);
	/* Obtain the 4th parameter string: delimiter format */
	pcParameterString4 = ( int8_t * ) FreeRTOS_CLIGetParameter (pcCommandString, 4, &xParameterStringLength4);
	/* Obtain the 5th parameter string: index format */
	pcParameterString5 = ( int8_t * ) FreeRTOS_CLIGetParameter (pcCommandString, 5, &xParameterStringLength5);
	/* Obtain the 6th parameter string: index label */
	pcParameterString6 = ( int8_t * ) FreeRTOS_CLIGetParameter (pcCommandString, 6, &xParameterStringLength6);
	
	/* log name */
	pcParameterString1[xParameterStringLength1] = 0;		// Get rid of the remaining parameters
	name = (char *)malloc(strlen((const char *)pcParameterString1) + 1);		// Move string out of the stack
	memset (name, 0, strlen((const char *)pcParameterString1) + 1);
	if (name == NULL)	
		result = H1BR6_ERR_MemoryFull;
	else
		strcpy(name, (const char *)pcParameterString1);
	
	/* type */
	if (!strncmp((const char *)pcParameterString2, "rate", xParameterStringLength2))
		type = RATE;
	else if (!strncmp((const char *)pcParameterString2, "event", xParameterStringLength2))
		type = EVENT;		
	else
		result = H1BR6_ERR_WrongParams;
	
	/* rate */
	rate = atof( ( const char * ) pcParameterString3 );
	if (rate < 0.0f || rate > 1000.0f)
		result = H1BR6_ERR_WrongParams;
	
	/* delimiter format */
	if (!strncmp((const char *)pcParameterString4, "space", xParameterStringLength4))
		dformat = FMT_SPACE;
	else if (!strncmp((const char *)pcParameterString4, "tab", xParameterStringLength4))
		dformat = FMT_TAB;		
	else if (!strncmp((const char *)pcParameterString4, "comma", xParameterStringLength4))
		dformat = FMT_COMMA;		
	else
		result = H1BR6_ERR_WrongParams;
	
	/* index format */
	if (!strncmp((const char *)pcParameterString5, "sample", xParameterStringLength5))
		iformat = FMT_SAMPLE;
	else if (!strncmp((const char *)pcParameterString5, "time", xParameterStringLength5))
		iformat = FMT_TIME;		
	else if (!strncmp((const char *)pcParameterString5, "none", xParameterStringLength5))
		iformat = FMT_NONE;		
	else
		result = H1BR6_ERR_WrongParams;

	/* index name */
	pcParameterString6[xParameterStringLength6] = 0;		// Get rid of the remaining parameters
	index = (char *)malloc(strlen((const char *)pcParameterString6) + 1);		// Move string out of the stack
	memset (index, 0, strlen((const char *)pcParameterString6) + 1);
	if (index == NULL)	
		result = H1BR6_ERR_MemoryFull;
	else
		strcpy(index, (const char *)pcParameterString6);
	
	/* Create the log */
	if (result == H1BR6_OK) {
		result = CreateLog(name, type, rate, dformat, iformat, index);	
	} else {
		free(index);
	}
	free(name);
	/* Respond to the command */
	if (result == H1BR6_OK) {
		strcpy( ( char * ) pcWriteBuffer, ( char * ) pcOKMessage);
	} else if (result == H1BR6_ERR_WrongParams) {
		strcpy( ( char * ) pcWriteBuffer, ( char * ) pcWrongValue);
	} else if (result ==  H1BR6_ERR_LogNameExists) {
		strcpy( ( char * ) pcWriteBuffer, ( char * ) pcLogExists);
	} else if (result ==  H1BR6_ERR_SD) {
		strcpy( ( char * ) pcWriteBuffer, ( char * ) pcSDerror);
	} else if (result ==  H1BR6_ERR_MaxLogs) {
		strcpy( ( char * ) pcWriteBuffer, ( char * ) pcMaxLogs);
	} else if (result ==  H1BR6_ERR_MemoryFull) {
		strcpy( ( char * ) pcWriteBuffer, ( char * ) pcMemoryFull);
	}
	
	/* There is no more data to return after this single string, so return
	pdFALSE. */
	return pdFALSE;
}

/*-----------------------------------------------------------*/

portBASE_TYPE deleteLogCommand( int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString )
{
	Module_Status result = H1BR6_OK;
	
	int8_t *pcParameterString1, *pcParameterString2; 
	portBASE_TYPE xParameterStringLength1 = 0, xParameterStringLength2 = 0; 
	options_t options; 
	static const int8_t *pcOKMessage1 = ( int8_t * ) "Log deleted both internally and from the disk\r\n";
	static const int8_t *pcOKMessage2 = ( int8_t * ) "Log deleted internally and kept on the disk\r\n";
	static const int8_t *pcWrongValue = ( int8_t * ) "Log deletion failed. Wrong parameters\r\n";
	
	/* Remove compile time warnings about unused parameters, and check the
	write buffer is not NULL.  NOTE - for simplicity, this example assumes the
	write buffer length is adequate, so does not check for buffer overflows. */
	( void ) xWriteBufferLen;
	configASSERT( pcWriteBuffer );
	
	/* Obtain the 1st parameter string: log name */
	pcParameterString1 = ( int8_t * ) FreeRTOS_CLIGetParameter (pcCommandString, 1, &xParameterStringLength1);
	/* Obtain the 2nd parameter string: delete options */
	pcParameterString2 = ( int8_t * ) FreeRTOS_CLIGetParameter (pcCommandString, 2, &xParameterStringLength2);
	
	/* log name */
	pcParameterString1[xParameterStringLength1] = 0;		// Get rid of the remaining parameters
	
	/* type */
	if (!strncmp((const char *)pcParameterString2, "all", xParameterStringLength2))
		options = DELETE_ALL;
	else if (!strncmp((const char *)pcParameterString2, "keepdisk", xParameterStringLength2))
		options = KEEP_ON_DISK;		
	else
		result = H1BR6_ERR_WrongParams;

	/* Delete the log */
	if (result == H1BR6_OK) {
		result = DeleteLog((char *)pcParameterString1, options);	
	}
	
	/* Respond to the command */
	if (result == H1BR6_OK && options == DELETE_ALL) {
		strcpy( ( char * ) pcWriteBuffer, ( char * ) pcOKMessage1);
	} else if (result == H1BR6_OK && options == KEEP_ON_DISK) {
		strcpy( ( char * ) pcWriteBuffer, ( char * ) pcOKMessage2);
	} else if (result == H1BR6_ERR_WrongParams) {
		strcpy( ( char * ) pcWriteBuffer, ( char * ) pcWrongValue);
	} 
	
	/* There is no more data to return after this single string, so return
	pdFALSE. */
	return pdFALSE;
}

/*-----------------------------------------------------------*/

portBASE_TYPE logVarCommand( int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString )
{
	Module_Status result = H1BR6_OK;
	
	int8_t *pcParameterString1, *pcParameterString2, *pcParameterString3, *pcParameterString4, *pcParameterString5; 
	portBASE_TYPE xParameterStringLength1 = 0, xParameterStringLength2 = 0, xParameterStringLength3 = 0; 
	portBASE_TYPE xParameterStringLength4 = 0, xParameterStringLength5 = 0;
	logVarType_t type; uint32_t source; char *label;
	static const int8_t *pcOKMessage = ( int8_t * ) "Variable added to log successfully\r\n";
	static const int8_t *pcWrongValue = ( int8_t * ) "Variable was not added to log. Wrong parameters\r\n";
	static const int8_t *pcLogDoesNotExist = ( int8_t * ) "Variable was not added to log. Log does not exist\r\n";
	static const int8_t *pcMemoryFull = ( int8_t * ) "Variable was not added to log. Internal memory full\r\n";
	static const int8_t *pcMaxLogVars = ( int8_t * ) "Variable was not added to log. Maximum number of logged variables reached\r\n";
	static const int8_t *pcWrongAddress = ( int8_t * ) "Variable was not added to log. Wrong in Address\r\n\t Adress must be start with 0x2 'SRAM' , 0x4 'Peripheral' or 0x08 'Flash'\r\n";
	
	/* Remove compile time warnings about unused parameters, and check the
	write buffer is not NULL.  NOTE - for simplicity, this example assumes the
	write buffer length is adequate, so does not check for buffer overflows. */
	( void ) xWriteBufferLen;
	configASSERT( pcWriteBuffer );
	
	/* Obtain the 1st parameter string: log name */
	pcParameterString1 = ( int8_t * ) FreeRTOS_CLIGetParameter (pcCommandString, 1, &xParameterStringLength1);
	/* Obtain the 2nd parameter string: variable type 1 */
	pcParameterString2 = ( int8_t * ) FreeRTOS_CLIGetParameter (pcCommandString, 2, &xParameterStringLength2);
	/* Obtain the 3rd parameter string: variable type 2 */
	pcParameterString3 = ( int8_t * ) FreeRTOS_CLIGetParameter (pcCommandString, 3, &xParameterStringLength3);
	/* Obtain the 4th parameter string: variable source */
	pcParameterString4 = ( int8_t * ) FreeRTOS_CLIGetParameter (pcCommandString, 4, &xParameterStringLength4);
	/* Obtain the 5th parameter string: variable column label */
	pcParameterString5 = ( int8_t * ) FreeRTOS_CLIGetParameter (pcCommandString, 5, &xParameterStringLength5);
	
	/* log name */
	pcParameterString1[xParameterStringLength1] = 0;		// Get rid of the remaining parameters
	
	/* variable type */
	if (!strncmp((const char *)pcParameterString2, "port", xParameterStringLength2)) {
		if (!strncmp((const char *)pcParameterString3, "digital", xParameterStringLength3)) {
			type = PORT_DIGITAL;
		} else if (!strncmp((const char *)pcParameterString3, "data", xParameterStringLength3)) {
			type = PORT_DATA;
		} else if (!strncmp((const char *)pcParameterString3, "button", xParameterStringLength3)) {
			type = PORT_BUTTON;
		} else
			result = H1BR6_ERR_WrongParams;
	} else if (!strncmp((const char *)pcParameterString2, "memory", xParameterStringLength2)) {
		if (!strncmp((const char *)pcParameterString3, "uint8", xParameterStringLength3)) {
			type = MEMORY_DATA_UINT8;
		} else if (!strncmp((const char *)pcParameterString3, "int8", xParameterStringLength3)) {
			type = MEMORY_DATA_INT8;
		} else if (!strncmp((const char *)pcParameterString3, "uint16", xParameterStringLength3)) {
			type = MEMORY_DATA_UINT16;
		} else if (!strncmp((const char *)pcParameterString3, "int16", xParameterStringLength3)) {
			type = MEMORY_DATA_INT16;
		} else if (!strncmp((const char *)pcParameterString3, "uint32", xParameterStringLength3)) {
			type = MEMORY_DATA_UINT32;
		} else if (!strncmp((const char *)pcParameterString3, "int32", xParameterStringLength3)) {
			type = MEMORY_DATA_INT32;
		} else if (!strncmp((const char *)pcParameterString3, "float", xParameterStringLength3)) {
			type = MEMORY_DATA_FLOAT;
		} else
			result = H1BR6_ERR_WrongParams;		
	} else
		result = H1BR6_ERR_WrongParams;
	
	/* source */
	if (type == PORT_BUTTON && pcParameterString4[0] == 'b')
		source = ( uint8_t ) atol( ( char * ) pcParameterString4+1 );
	else if ((type == PORT_DIGITAL || type == PORT_DATA) && pcParameterString4[0] == 'p')
		source = ( uint8_t ) atol( ( char * ) pcParameterString4+1 );
	else if (!strncmp((const char *)pcParameterString4, "0x", 2)) {
		source = strtoul(( const char * ) pcParameterString4, NULL, 16);
	} else
		result = H1BR6_ERR_WrongParams;
	
	/* variable column label */
	pcParameterString5[xParameterStringLength5] = 0;		// Get rid of the remaining parameters
	label = (char *)malloc(strlen((const char *)pcParameterString5) + 1);		// Move string out of the stack
	memset (label, 0, strlen((const char *)pcParameterString5) + 1);
	if (label == NULL)	
		result = H1BR6_ERR_MemoryFull;
	else
		strcpy(label, (const char *)pcParameterString5);
	
	/* Add the variable to the log */
	if (result == H1BR6_OK) {
		result = LogVar((char *)pcParameterString1, type, source, label);	
	} else {
		free(label);
	}
	
	/* Respond to the command */
	if (result == H1BR6_OK) {
		strcpy( ( char * ) pcWriteBuffer, ( char * ) pcOKMessage);
	} else if (result == H1BR6_ERR_WrongParams) {
		strcpy( ( char * ) pcWriteBuffer, ( char * ) pcWrongValue);
	} else if (result ==  H1BR6_ERR_LogDoesNotExist) {
		strcpy( ( char * ) pcWriteBuffer, ( char * ) pcLogDoesNotExist);
	} else if (result ==  H1BR6_ERR_MemoryFull) {
		strcpy( ( char * ) pcWriteBuffer, ( char * ) pcMemoryFull);
	} else if (result ==  H1BR6_ERR_MaxLogVars) {
		strcpy( ( char * ) pcWriteBuffer, ( char * ) pcMaxLogVars);
	} else if (result == H1BR6_ERR_WrongAddress) {
		strcpy( ( char * ) pcWriteBuffer, ( char * ) pcWrongAddress);
	} 
	
	/* There is no more data to return after this single string, so return
	pdFALSE. */
	return pdFALSE;
}

/*-----------------------------------------------------------*/

portBASE_TYPE startCommand( int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString )
{
	Module_Status result = H1BR6_OK;
	
	int8_t *pcParameterString1; 
	portBASE_TYPE xParameterStringLength1 = 0;  
	static const int8_t *pcOKMessage = ( int8_t * ) "%s started logging\r\n";
	static const int8_t *pcLogDoesNotExist = ( int8_t * ) "Log does not exist\r\n";

	/* Remove compile time warnings about unused parameters, and check the
	write buffer is not NULL.  NOTE - for simplicity, this example assumes the
	write buffer length is adequate, so does not check for buffer overflows. */
	( void ) xWriteBufferLen;
	configASSERT( pcWriteBuffer );
	
	/* Obtain the 1st parameter string: log name */
	pcParameterString1 = ( int8_t * ) FreeRTOS_CLIGetParameter (pcCommandString, 1, &xParameterStringLength1);

	/* Start the log */
	if (result == H1BR6_OK) {
		result = StartLog((char *)pcParameterString1);	
	}
	
	/* Respond to the command */
	if (result == H1BR6_OK) {
		sprintf( ( char * ) pcWriteBuffer, ( char * ) pcOKMessage, pcParameterString1);
	} else if (result ==  H1BR6_ERR_LogDoesNotExist) {
		strcpy( ( char * ) pcWriteBuffer, ( char * ) pcLogDoesNotExist);
	} 
	
	/* There is no more data to return after this single string, so return
	pdFALSE. */
	return pdFALSE;
}

/*-----------------------------------------------------------*/
portBASE_TYPE stopCommand( int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString )
{
	Module_Status result = H1BR6_OK;
	
	int8_t *pcParameterString1; 
	portBASE_TYPE xParameterStringLength1 = 0;  
	static const int8_t *pcOKMessage = ( int8_t * ) "%s stoped logging\r\n";
	static const int8_t *pcIsNotActive = ( int8_t * ) "%s was not running\r\n";
	static const int8_t *pcLogDoesNotExist = ( int8_t * ) "Log does not exist\r\n";

	/* Remove compile time warnings about unused parameters, and check the
	write buffer is not NULL.  NOTE - for simplicity, this example assumes the
	write buffer length is adequate, so does not check for buffer overflows. */
	( void ) xWriteBufferLen;
	configASSERT( pcWriteBuffer );
	
	/* Obtain the 1st parameter string: log name */
	pcParameterString1 = ( int8_t * ) FreeRTOS_CLIGetParameter (pcCommandString, 1, &xParameterStringLength1);

	/* Stop the log */
	if (result == H1BR6_OK) {
		result = StopLog((char *)pcParameterString1);	
	}
	
	/* Respond to the command */
	if (result == H1BR6_OK) {
		sprintf( ( char * ) pcWriteBuffer, ( char * ) pcOKMessage, pcParameterString1);
	} else if (result ==  H1BR6_ERR_LogIsNotActive) {
		sprintf( ( char * ) pcWriteBuffer, ( char * ) pcIsNotActive, pcParameterString1);
	} else if (result ==  H1BR6_ERR_LogDoesNotExist) {
		strcpy( ( char * ) pcWriteBuffer, ( char * ) pcLogDoesNotExist);
	} 
	
	/* There is no more data to return after this single string, so return
	pdFALSE. */
	return pdFALSE;
}

/*-----------------------------------------------------------*/

portBASE_TYPE pauseCommand( int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString )
{
	Module_Status result = H1BR6_OK;
	
	int8_t *pcParameterString1; 
	portBASE_TYPE xParameterStringLength1 = 0; 
	static const int8_t *pcOKMessage = ( int8_t * ) "%s paused logging\r\n";
	static const int8_t *pcIsNotActive = ( int8_t * ) "%s was not running\r\n";
	static const int8_t *pcLogDoesNotExist = ( int8_t * ) "Log does not exist\r\n";

	/* Remove compile time warnings about unused parameters, and check the
	write buffer is not NULL.  NOTE - for simplicity, this example assumes the
	write buffer length is adequate, so does not check for buffer overflows. */
	( void ) xWriteBufferLen;
	configASSERT( pcWriteBuffer );
	
	/* Obtain the 1st parameter string: log name */
	pcParameterString1 = ( int8_t * ) FreeRTOS_CLIGetParameter (pcCommandString, 1, &xParameterStringLength1);

	/* Pause the log */
	if (result == H1BR6_OK) {
		result = PauseLog((char *)pcParameterString1);	
	}
	
	/* Respond to the command */
	if (result == H1BR6_OK) {
		sprintf( ( char * ) pcWriteBuffer, ( char * ) pcOKMessage, pcParameterString1);
	} else if (result ==  H1BR6_ERR_LogIsNotActive) {
		sprintf( ( char * ) pcWriteBuffer, ( char * ) pcIsNotActive, pcParameterString1);
	} else if (result ==  H1BR6_ERR_LogDoesNotExist) {
		strcpy( ( char * ) pcWriteBuffer, ( char * ) pcLogDoesNotExist);
	}  
	
	/* There is no more data to return after this single string, so return
	pdFALSE. */
	return pdFALSE;
}

/*-----------------------------------------------------------*/

portBASE_TYPE resumeCommand( int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString )
{
	Module_Status result = H1BR6_OK;
	
	int8_t *pcParameterString1; 
	portBASE_TYPE xParameterStringLength1 = 0; 
	static const int8_t *pcOKMessage = ( int8_t * ) "%s resumed logging\r\n";
	static const int8_t *pcLogDoesNotExist = ( int8_t * ) "Log does not exist\r\n";

	/* Remove compile time warnings about unused parameters, and check the
	write buffer is not NULL.  NOTE - for simplicity, this example assumes the
	write buffer length is adequate, so does not check for buffer overflows. */
	( void ) xWriteBufferLen;
	configASSERT( pcWriteBuffer );
	
	/* Obtain the 1st parameter string: log name */
	pcParameterString1 = ( int8_t * ) FreeRTOS_CLIGetParameter (pcCommandString, 1, &xParameterStringLength1);

	/* Resume the log */
	if (result == H1BR6_OK) {
		result = ResumeLog((char *)pcParameterString1);	
	}
	
	/* Respond to the command */
	if (result == H1BR6_OK) {
		sprintf( ( char * ) pcWriteBuffer, ( char * ) pcOKMessage, pcParameterString1);
	} else if (result ==  H1BR6_ERR_LogDoesNotExist) {
		strcpy( ( char * ) pcWriteBuffer, ( char * ) pcLogDoesNotExist);
	} 
	
	/* There is no more data to return after this single string, so return
	pdFALSE. */
	return pdFALSE;
}

/*-----------------------------------------------------------*/

/************************ (C) COPYRIGHT HEXABITZ *****END OF FILE****/
