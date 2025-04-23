/*
 BitzOS (BOS) V0.4.0 - Copyright (C) 2017-2025 Hexabitz
 All rights reserved

 File Name     : H0BR4.c
 Description   : Source code for module H0BR4.
 	 	 	 	 (Description_of_module)

(Description of Special module peripheral configuration):
>>
>>
>>
 */

/* Includes ------------------------------------------------------------------*/
#include "BOS.h"
#include "H1BR6.h"
#include "app_fatfs.h"
#include "string.h"
#include "stdio.h"
#include <stdlib.h>

/* Exported Typedef ********************************************************/
/* Define UART variables */
UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;
UART_HandleTypeDef huart3;
UART_HandleTypeDef huart4;
UART_HandleTypeDef huart5;
UART_HandleTypeDef huart6;

FATFS fs;
FIL MyFile;
FRESULT fresult;
log_t logs[MAX_LOGS] = {0};
logVar_t logVars[MAX_LOG_VARS] = {0};

TaskHandle_t LogTaskHandle = NULL;

/* Private Variables *******************************************************/
/* Module settings - sequential log naming*/
bool enableSequential = true;
bool enableTimeDateHeader = false;

char lineBuffer[100];
char tempName[MAX_NAME_LENGTH] = {0};

uint8_t f_mount_ok=0;
volatile uint8_t couFile = 0;

uint16_t  activeLogs =0;

uint32_t byteswritten =0;
uint32_t compareValue[MAX_LOG_VARS] = {0};

/* Module Parameters */
ModuleParam_t ModuleParam[NUM_MODULE_PARAMS] ={0};

/* Private Function Prototypes *********************************************/
uint8_t ClearROtopology(void);
void Module_Peripheral_Init(void);
void SetupPortForRemoteBootloaderUpdate(uint8_t port);
void RemoteBootloaderUpdate(uint8_t src,uint8_t dst,uint8_t inport,uint8_t outport);
Module_Status Module_MessagingTask(uint16_t code, uint8_t port, uint8_t src, uint8_t dst, uint8_t shift);

/* Local Function Prototypes ***********************************************/
void LogTask(void * argument);
uint8_t CheckLogVarEvent(uint16_t varIndex);
Module_Status OpenThisLog(uint16_t logindex, FIL *objFile);
Module_Status MicroSD_Init(void);

/* Create CLI commands *****************************************************/
portBASE_TYPE demoCommand( int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString );
portBASE_TYPE addLogCommand( int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString );
portBASE_TYPE deleteLogCommand( int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString );
portBASE_TYPE logVarCommand( int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString );
portBASE_TYPE startCommand( int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString );
portBASE_TYPE stopCommand( int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString );
portBASE_TYPE pauseCommand( int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString );
portBASE_TYPE resumeCommand( int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString );

/* CLI command structure ***************************************************/
/* CLI command structure : demo */
const CLI_Command_Definition_t demoCommandDefinition = {
	( const int8_t * ) "demo", /* The command string to type. */
	( const int8_t * ) "demo:\r\n Run a demo program to test module functionality\r\n\r\n",
	demoCommand, /* The function to run. */
	0 /* No parameters are expected. */
};

/***************************************************************************/
/* CLI command structure : addlog */
const CLI_Command_Definition_t addLogCommandDefinition ={
		(const int8_t*) "addlog", /* The command string to type. */
		(const int8_t*) "addlog:\r\n Add a new log file. Specifiy log name (1st par.); type (2nd par.): 'rate' or 'event'; \
rate (3rd par.): logging rate in Hz (max 1000), delimiter format (4th par.): 'space', 'tab' or 'comma'; index column format \
(5th par.): 'none', 'sample' or 'time'; and index column label text (6th par.)\r\n\r\n",
		addLogCommand, /* The function to run. */
		6 /* Six parameters are expected. */
};

/***************************************************************************/
/* CLI command structure : logvar */
const CLI_Command_Definition_t logVarCommandDefinition = {
	( const int8_t * ) "logvar", /* The command string to type. */
	( const int8_t * ) "logvar:\r\n Add a new log variable to an existing log (1st par.). Specify variable type (2nd and 3rd par.): \
'port digital', 'port data', 'port Button', 'memory uint8', 'memory int8', 'memory uint16', 'memory int16', 'memory uint32', \
'memory int32', 'memory float' ; source (4th par.): ports 'p1'..'px', buttons 'b1'..'bx' or memory location (Flash or RAM); \
and column label text (5th par.)\r\n\r\n",
	logVarCommand, /* The function to run. */
	5 /* Five parameters are expected. */
};

/***************************************************************************/
/* CLI command structure : deletelog */
const CLI_Command_Definition_t deleteLogCommandDefinition = {
	( const int8_t * ) "deletelog", /* The command string to type. */
	( const int8_t * ) "deletelog:\r\n Delete a log file. Specifiy log name (1st par.) and delete options (2nd par.): 'all' or \
'keepdisk' to keep log on the uSD card\r\n\r\n",
	deleteLogCommand, /* The function to run. */
	3 /* Two parameters are expected. */
};

/***************************************************************************/
/* CLI command structure : start */
const CLI_Command_Definition_t startCommandDefinition = {
	( const int8_t * ) "start", /* The command string to type. */
	( const int8_t * ) "start:\r\n Start the log with log name (1st par.)\r\n\r\n",
	startCommand, /* The function to run. */
	1 /* One parameter is expected. */
};

/***************************************************************************/
/* CLI command structure : stop */
const CLI_Command_Definition_t stopCommandDefinition = {
	( const int8_t * ) "stop", /* The command string to type. */
	( const int8_t * ) "stop:\r\n Stop the log with log name (1st par.)\r\n\r\n",
	stopCommand, /* The function to run. */
	1 /* One parameter is expected. */
};

/***************************************************************************/
/* CLI command structure : pause */
const CLI_Command_Definition_t pauseCommandDefinition = {
	( const int8_t * ) "pause", /* The command string to type. */
	( const int8_t * ) "pause:\r\n Pause the log with log name (1st par.)\r\n\r\n",
	pauseCommand, /* The function to run. */
	1 /* One parameter is expected. */
};

/***************************************************************************/
/* CLI command structure : resume */
const CLI_Command_Definition_t resumeCommandDefinition = {
	( const int8_t * ) "resume", /* The command string to type. */
	( const int8_t * ) "resume:\r\n Resume the log with log name (1st par.)\r\n\r\n",
	resumeCommand, /* The function to run. */
	1 /* One parameter is expected. */
};

/***************************************************************************/
/************************ Private function Definitions *********************/
/***************************************************************************/
/* @brief  System Clock Configuration
 *         This function configures the system clock as follows:
 *            - System Clock source            = PLL (HSE)
 *            - SYSCLK(Hz)                     = 64000000
 *            - HCLK(Hz)                       = 64000000
 *            - AHB Prescaler                  = 1
 *            - APB1 Prescaler                 = 1
 *            - HSE Frequency(Hz)              = 8000000
 *            - PLLM                           = 1
 *            - PLLN                           = 16
 *            - PLLP                           = 2
 *            - Flash Latency(WS)              = 2
 *            - Clock Source for UART1,UART2,UART3 = 16MHz (HSI)
 */
void SystemClock_Config(void){
	RCC_OscInitTypeDef RCC_OscInitStruct ={0};
	RCC_ClkInitTypeDef RCC_ClkInitStruct ={0};

	/** Configure the main internal regulator output voltage */
	HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

	/* Initializes the RCC Oscillators according to the specified parameters
	 * in the RCC_OscInitTypeDef structure.
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI | RCC_OSCILLATORTYPE_HSE; // Enable both HSI and HSE oscillators
	RCC_OscInitStruct.HSEState = RCC_HSE_ON; // Enable HSE (External High-Speed Oscillator)
	RCC_OscInitStruct.HSIState = RCC_HSI_ON; // Enable HSI (Internal High-Speed Oscillator)
	RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV1; // No division on HSI
	RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT; // Default calibration value for HSI
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON; // Enable PLL
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE; // Set PLL source to HSE
	RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV1; // Prescaler for PLL input
	RCC_OscInitStruct.PLL.PLLN =16; // Multiplication factor for PLL
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2; // PLLP division factor
	RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2; // PLLQ division factor
	RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2; // PLLR division factor
	HAL_RCC_OscConfig(&RCC_OscInitStruct);

	/** Initializes the CPU, AHB and APB buses clocks */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK; // Select PLL as the system clock source
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1; // AHB Prescaler set to 1
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1; // APB1 Prescaler set to 1

	HAL_RCC_ClockConfig(&RCC_ClkInitStruct,FLASH_LATENCY_2); // Configure system clocks with flash latency of 2 WS
}

/***************************************************************************/
/* enable stop mode regarding only UART1 , UART2 , and UART3 */
BOS_Status EnableStopModebyUARTx(uint8_t port){

	UART_WakeUpTypeDef WakeUpSelection;
	UART_HandleTypeDef *huart =GetUart(port);

	if((huart->Instance == USART1) || (huart->Instance == USART2) || (huart->Instance == USART3)){

		/* make sure that no UART transfer is on-going */
		while(__HAL_UART_GET_FLAG(huart, USART_ISR_BUSY) == SET);

		/* make sure that UART is ready to receive */
		while(__HAL_UART_GET_FLAG(huart, USART_ISR_REACK) == RESET);

		/* set the wake-up event:
		 * specify wake-up on start-bit detection */
		WakeUpSelection.WakeUpEvent = UART_WAKEUP_ON_STARTBIT;
		HAL_UARTEx_StopModeWakeUpSourceConfig(huart,WakeUpSelection);

		/* Enable the UART Wake UP from stop mode Interrupt */
		__HAL_UART_ENABLE_IT(huart,UART_IT_WUF);

		/* enable MCU wake-up by LPUART */
		HAL_UARTEx_EnableStopMode(huart);

		/* enter STOP mode */
		HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON,PWR_STOPENTRY_WFI);
	}
	else
		return BOS_ERROR;

}

/***************************************************************************/
/* Enable standby mode regarding wake-up pins:
 * WKUP1: PA0  pin
 * WKUP4: PA2  pin
 * WKUP6: PB5  pin
 * WKUP2: PC13 pin
 * NRST pin
 *  */
BOS_Status EnableStandbyModebyWakeupPinx(WakeupPins_t wakeupPins){

	/* Clear the WUF FLAG */
	__HAL_PWR_CLEAR_FLAG(PWR_FLAG_WUF);

	/* Enable the WAKEUP PIN */
	switch(wakeupPins){

		case PA0_PIN:
			HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN1); /* PA0 */
			break;

		case PA2_PIN:
			HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN4); /* PA2 */
			break;

		case PB5_PIN:
			HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN6); /* PB5 */
			break;

		case PC13_PIN:
			HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN2); /* PC13 */
			break;

		case NRST_PIN:
			/* do no thing*/
			break;
	}

	/* Enable SRAM content retention in Standby mode */
	HAL_PWREx_EnableSRAMRetention();

	/* Finally enter the standby mode */
	HAL_PWR_EnterSTANDBYMode();

	return BOS_OK;
}

/***************************************************************************/
/* Disable standby mode regarding wake-up pins:
 * WKUP1: PA0  pin
 * WKUP4: PA2  pin
 * WKUP6: PB5  pin
 * WKUP2: PC13 pin
 * NRST pin
 *  */
BOS_Status DisableStandbyModeWakeupPinx(WakeupPins_t wakeupPins){

	/* The standby wake-up is same as a system RESET:
	 * The entire code runs from the beginning just as if it was a RESET.
	 * The only difference between a reset and a STANDBY wake-up is that, when the MCU wakes-up,
	 * The SBF status flag in the PWR power control/status register (PWR_CSR) is set */
	if(__HAL_PWR_GET_FLAG(PWR_FLAG_SB) != RESET){
		/* clear the flag */
		__HAL_PWR_CLEAR_FLAG(PWR_FLAG_SB);

		/* Disable  Wake-up Pinx */
		switch(wakeupPins){

			case PA0_PIN:
				HAL_PWR_DisableWakeUpPin(PWR_WAKEUP_PIN1); /* PA0 */
				break;

			case PA2_PIN:
				HAL_PWR_DisableWakeUpPin(PWR_WAKEUP_PIN4); /* PA2 */
				break;

			case PB5_PIN:
				HAL_PWR_DisableWakeUpPin(PWR_WAKEUP_PIN6); /* PB5 */
				break;

			case PC13_PIN:
				HAL_PWR_DisableWakeUpPin(PWR_WAKEUP_PIN2); /* PC13 */
				break;

			case NRST_PIN:
				/* do no thing*/
				break;
		}

		IND_blink(1000);

	}
	else
		return BOS_OK;

}

/***************************************************************************/
/* Save Command Topology in Flash RO */
uint8_t SaveTopologyToRO(void){

	HAL_StatusTypeDef flashStatus =HAL_OK;

	/* flashAdd is initialized with 8 because the first memory room in topology page
	 * is reserved for module's ID */
	uint16_t flashAdd =8;
	uint16_t temp =0;

	/* Unlock the FLASH control register access */
	HAL_FLASH_Unlock();

	/* Erase Topology page */
	FLASH_PageErase(FLASH_BANK_2,TOPOLOGY_PAGE_NUM);

	/* Wait for an Erase operation to complete */
	flashStatus =FLASH_WaitForLastOperation((uint32_t ) HAL_FLASH_TIMEOUT_VALUE);

	if(flashStatus != HAL_OK){
		/* return FLASH error code */
		return pFlash.ErrorCode;
	}

	else{
		/* Operation is completed, disable the PER Bit */
		CLEAR_BIT(FLASH->CR,FLASH_CR_PER);
	}

	/* Save module's ID and topology */
	if(myID){

		/* Save module's ID */
		temp =(uint16_t )(N << 8) + myID;

		/* Save module's ID in Flash memory */
		HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD,TOPOLOGY_START_ADDRESS,temp);

		/* Wait for a Write operation to complete */
		flashStatus =FLASH_WaitForLastOperation((uint32_t ) HAL_FLASH_TIMEOUT_VALUE);

		if(flashStatus != HAL_OK){
			/* return FLASH error code */
			return pFlash.ErrorCode;
		}

		else{
			/* If the program operation is completed, disable the PG Bit */
			CLEAR_BIT(FLASH->CR,FLASH_CR_PG);
		}

		/* Save topology */
		for(uint8_t row =1; row <= N; row++){
			for(uint8_t column =0; column <= MAX_NUM_OF_PORTS; column++){
				/* Check the module serial number
				 * Note: there isn't a module has serial number 0
				 */
				if(Array[row - 1][0]){
					/* Save each element in topology Array in Flash memory */
					HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD,TOPOLOGY_START_ADDRESS + flashAdd,Array[row - 1][column]);
					/* Wait for a Write operation to complete */
					flashStatus =FLASH_WaitForLastOperation((uint32_t ) HAL_FLASH_TIMEOUT_VALUE);
					if(flashStatus != HAL_OK){
						/* return FLASH error code */
						return pFlash.ErrorCode;
					}
					else{
						/* If the program operation is completed, disable the PG Bit */
						CLEAR_BIT(FLASH->CR,FLASH_CR_PG);
						/* update new flash memory address */
						flashAdd +=8;
					}
				}
			}
		}
	}
	/* Lock the FLASH control register access */
	HAL_FLASH_Lock();
}

/***************************************************************************/
/* Save Command Snippets in Flash RO */
uint8_t SaveSnippetsToRO(void){
	HAL_StatusTypeDef FlashStatus =HAL_OK;
	uint8_t snipBuffer[sizeof(Snippet_t) + 1] ={0};

	/* Unlock the FLASH control register access */
	HAL_FLASH_Unlock();
	/* Erase Snippets page */
	FLASH_PageErase(FLASH_BANK_2,SNIPPETS_PAGE_NUM);
	/* Wait for an Erase operation to complete */
	FlashStatus =FLASH_WaitForLastOperation((uint32_t ) HAL_FLASH_TIMEOUT_VALUE);

	if(FlashStatus != HAL_OK){
		/* return FLASH error code */
		return pFlash.ErrorCode;
	}
	else{
		/* Operation is completed, disable the PER Bit */
		CLEAR_BIT(FLASH->CR,FLASH_CR_PER);
	}

	/* Save Command Snippets */
	int currentAdd = SNIPPETS_START_ADDRESS;
	for(uint8_t index =0; index < NumOfRecordedSnippets; index++){
		/* Check if Snippet condition is true or false */
		if(Snippets[index].Condition.ConditionType){
			/* A marker to separate Snippets */
			snipBuffer[0] =0xFE;
			memcpy((uint32_t* )&snipBuffer[1],(uint8_t* )&Snippets[index],sizeof(Snippet_t));
			/* Copy the snippet struct buffer (20 x NumOfRecordedSnippets). Note this is assuming sizeof(Snippet_t) is even */
			for(uint8_t j =0; j < (sizeof(Snippet_t) / 4); j++){
				HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD,currentAdd,*(uint64_t* )&snipBuffer[j * 8]);
				FlashStatus =FLASH_WaitForLastOperation((uint32_t ) HAL_FLASH_TIMEOUT_VALUE);
				if(FlashStatus != HAL_OK){
					return pFlash.ErrorCode;
				}
				else{
					/* If the program operation is completed, disable the PG Bit */
					CLEAR_BIT(FLASH->CR,FLASH_CR_PG);
					currentAdd +=8;
				}
			}
			/* Copy the snippet commands buffer. Always an even number. Note the string termination char might be skipped */
			for(uint8_t j =0; j < ((strlen(Snippets[index].CMD) + 1) / 4); j++){
				HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD,currentAdd,*(uint64_t* )(Snippets[index].CMD + j * 4));
				FlashStatus =FLASH_WaitForLastOperation((uint32_t ) HAL_FLASH_TIMEOUT_VALUE);
				if(FlashStatus != HAL_OK){
					return pFlash.ErrorCode;
				}
				else{
					/* If the program operation is completed, disable the PG Bit */
					CLEAR_BIT(FLASH->CR,FLASH_CR_PG);
					currentAdd +=8;
				}
			}
		}
	}
	/* Lock the FLASH control register access */
	HAL_FLASH_Lock();
}

/***************************************************************************/
/* Clear Array topology in SRAM and Flash RO */
uint8_t ClearROtopology(void){
	/* Clear the Array */
	memset(Array,0,sizeof(Array));
	N =1;
	myID =0;
	
	return SaveTopologyToRO();
}

/***************************************************************************/
/* Trigger ST factory bootloader update for a remote module */
void RemoteBootloaderUpdate(uint8_t src,uint8_t dst,uint8_t inport,uint8_t outport){

	uint8_t myOutport =0, lastModule =0;
	int8_t *pcOutputString;

	/* 1. Get Route to destination module */
	myOutport =FindRoute(myID,dst);
	if(outport && dst == myID){ /* This is a 'via port' update and I'm the last module */
		myOutport =outport;
		lastModule =myID;
	}
	else if(outport == 0){ /* This is a remote update */
		if(NumberOfHops(dst)== 1)
		lastModule = myID;
		else
		lastModule = Route[NumberOfHops(dst)-1]; /* previous module = Route[Number of hops - 1] */
	}

	/* 2. If this is the source of the message, show status on the CLI */
	if(src == myID){
		/* Obtain the address of the output buffer.  Note there is no mutual
		 * exclusion on this buffer as it is assumed only one command console
		 * interface will be used at any one time. */
		pcOutputString =FreeRTOS_CLIGetOutputBuffer();

		if(outport == 0)		// This is a remote module update
			sprintf((char* )pcOutputString,pcRemoteBootloaderUpdateMessage,dst);
		else
			// This is a 'via port' remote update
			sprintf((char* )pcOutputString,pcRemoteBootloaderUpdateViaPortMessage,dst,outport);

		strcat((char* )pcOutputString,pcRemoteBootloaderUpdateWarningMessage);
		writePxITMutex(inport,(char* )pcOutputString,strlen((char* )pcOutputString),cmd50ms);
		Delay_ms(100);
	}

	/* 3. Setup my inport and outport for bootloader update */
	SetupPortForRemoteBootloaderUpdate(inport);
	SetupPortForRemoteBootloaderUpdate(myOutport);

	/* 5. Build a DMA stream between my inport and outport */
	StartScastDMAStream(inport,myID,myOutport,myID,BIDIRECTIONAL,0xFFFFFFFF,0xFFFFFFFF,false);
}

/***************************************************************************/
/* Setup a port for remote ST factory bootloader update:
 * Set baudrate to 57600
 * Enable even parity
 * Set datasize to 9 bits
 */
void SetupPortForRemoteBootloaderUpdate(uint8_t port){

	UART_HandleTypeDef *huart =GetUart(port);
	HAL_UART_DeInit(huart);
	huart->Init.Parity = UART_PARITY_EVEN;
	huart->Init.WordLength = UART_WORDLENGTH_9B;
	HAL_UART_Init(huart);

	/* The CLI port RXNE interrupt might be disabled so enable here again to be sure */
	__HAL_UART_ENABLE_IT(huart,UART_IT_RXNE);

}

/***************************************************************************/
/* H1BR6 module initialization */
void Module_Peripheral_Init(void) {

	/* Array ports */
	MX_USART1_UART_Init();
	MX_USART2_UART_Init();
	MX_USART3_UART_Init();
	MX_USART4_UART_Init();
	MX_USART5_UART_Init();

	SDCardGPIOInit();
	MX_SPI_Init();
	MX_FATFS_Init();

	/* Circulating DMA Channels ON All Module */
	for (int i = 1; i <= NUM_OF_PORTS; i++) {
		if (GetUart(i) == &huart1) {
			dmaIndex[i - 1] = &(DMA1_Channel1->CNDTR);
		} else if (GetUart(i) == &huart2) {
			dmaIndex[i - 1] = &(DMA1_Channel2->CNDTR);
		} else if (GetUart(i) == &huart3) {
			dmaIndex[i - 1] = &(DMA1_Channel3->CNDTR);
		} else if (GetUart(i) == &huart4) {
			dmaIndex[i - 1] = &(DMA1_Channel4->CNDTR);
		} else if (GetUart(i) == &huart5) {
			dmaIndex[i - 1] = &(DMA1_Channel5->CNDTR);
		} else if (GetUart(i) == &huart6) {
			dmaIndex[i - 1] = &(DMA1_Channel6->CNDTR);
		}
	}

	/* Create module special task (if needed) */
	NeedToDelayButtonStateReset = true;

		/* Create the logging task */
		xTaskCreate(LogTask, (const char *) "LogTask", (2*configMINIMAL_STACK_SIZE), NULL, osPriorityNormal-osPriorityIdle, &LogTaskHandle);
}

/***************************************************************************/
/* H1BR6 message processing task */
Module_Status Module_MessagingTask(uint16_t code,uint8_t port,uint8_t src,uint8_t dst,uint8_t shift){
	Module_Status result = H1BR6_OK;
	uint8_t templn;

	switch (code) {

	default:
		result = H1BR6_ERR_UnknownMessage;
		break;
	}

	return result;
}

/***************************************************************************/
/* Get the port for a given UART */
uint8_t GetPort(UART_HandleTypeDef *huart) {

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
	else if (huart->Instance == USART6)
		return P6;

	return 0;
}

/***************************************************************************/
/* Register this module CLI Commands */
void RegisterModuleCLICommands(void){
	    FreeRTOS_CLIRegisterCommand(&demoCommandDefinition);
		FreeRTOS_CLIRegisterCommand(&addLogCommandDefinition);
		FreeRTOS_CLIRegisterCommand(&logVarCommandDefinition);
		FreeRTOS_CLIRegisterCommand(&deleteLogCommandDefinition);
		FreeRTOS_CLIRegisterCommand(&startCommandDefinition);
		FreeRTOS_CLIRegisterCommand(&stopCommandDefinition);
		FreeRTOS_CLIRegisterCommand(&pauseCommandDefinition);
		FreeRTOS_CLIRegisterCommand(&resumeCommandDefinition);
}

/***************************************************************************/
/* This functions is useful only for input (sensors) modules.
 * Samples a module parameter value based on parameter index.
 * paramIndex: Index of the parameter (1-based index).
 * value: Pointer to store the sampled float value.
 */
Module_Status GetModuleParameter(uint8_t paramIndex, float *value) {
	Module_Status status = BOS_OK;

	switch (paramIndex) {

	/* Invalid parameter index */
	default:
		status = BOS_ERR_WrongParam;
		break;
	}

	return status;
}

/***************************************************************************/
/****************************** Local Functions ****************************/
/***************************************************************************/
Module_Status MicroSD_Init(void) {

	fresult = f_mount(&fs, "", 0);

	if (fresult != FR_OK) {
		/* Unmount the drive */
		fresult = f_mount(NULL, "", 1);
		/* SD card malfunction. Replace or re-insert the card and reboot */
		while (1) {
			RTOS_IND_blink(500);
			Delay_ms(500);
		}
	}
	f_mount_ok = 1;

	return H1BR6_OK;
}

/***************************************************************************/
/* Logging task */
void LogTask(void *argument) {
	static uint8_t newLine = 1; /* Flag indicating if a new line should be started in the log */
	static uint8_t resetButtonState = 0; /* Flag to track if any button states need to be reset */

	/* Loop counters and timing */
	volatile uint8_t i, j;
	volatile uint32_t u32lTick = 0; /* Tick counter for each log */
	volatile uint32_t u32lRate = 0; /* Sampling period in ticks */

	/* Initialize the micro SD card */
	MicroSD_Init();

	/* Infinite loop */
	for (;;) {

		/* Check all active logs */
		for (j = 0; j < MAX_LOGS; j++) {
			u32lTick = HAL_GetTick() - logs[j].t0;         /* Time since last log write */
			u32lRate = configTICK_RATE_HZ / logs[j].rate;  /* Calculate logging interval */

			if (u32lTick >= u32lRate)
				++logs[j].sampleCount;			 /* Increment sample counter */

			if ((activeLogs >> j) & 0x01) {      /* Check if this log is active */
				OpenThisLog(j, &MyFile);         /* Open corresponding file */

				memset(lineBuffer, 0, sizeof(lineBuffer));

				/***************************************************************************/
				/* Loop through log variables **********************************************/
				/***************************************************************************/
				for (i = 0; i < MAX_LOG_VARS; i++) {
					if (logVars[i].type && (logVars[i].logIndex == j)) {
						/* Read data from source based on variable type */
						switch (logVars[i].type) {
						case PORT_BUTTON:
							break;

						case MEMORY_DATA_UINT8:
							logVars[i].source = *(__IO uint8_t*) logVars[i].tempVar;
							break;

						case MEMORY_DATA_INT8:
							logVars[i].source = *(__IO int8_t*) logVars[i].tempVar;
							break;

						case MEMORY_DATA_UINT16:
							logVars[i].source = *(__IO uint16_t*) logVars[i].tempVar;
							break;

						case MEMORY_DATA_INT16:
							logVars[i].source = *(__IO int16_t*) logVars[i].tempVar;
							break;

						case MEMORY_DATA_UINT32:
							logVars[i].source = *(__IO uint32_t*) logVars[i].tempVar;
							break;

						case MEMORY_DATA_INT32:
							logVars[i].source = *(__IO int32_t*) logVars[i].tempVar;
							break;

						case MEMORY_DATA_FLOAT:
							logVars[i].sourceFloat = *(float*) logVars[i].tempVar;
							break;

						default:
							break;
						}

						/*Check for rate or event **************************************************/
						if (((RATE == logs[j].type) && (u32lTick >= u32lRate)) || CheckLogVarEvent(i)) {
							if (newLine) {
								newLine = 0;

								/* Write index column (time or sample count) */
								if (logs[j].indexColumnFormat == FMT_TIME) {
									GetTimeDate();
									sprintf(lineBuffer, "\n%02d:%02d:%02d-%03d", BOS.Time.Hours, BOS.Time.Minutes,
											BOS.Time.Seconds, BOS.Time.mSec);
								} else if (logs[j].indexColumnFormat == FMT_SAMPLE)
									sprintf(lineBuffer, "\n%d", logs[j].sampleCount);
							}

							/* Append delimiter */
							switch (logs[j].delimiterFormat) {
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

							/* Append variable value */
							switch (logVars[i].type) {
							case PORT_DIGITAL:
								//sprintf( ( char * ) buffer, "%d", HAL_GPIO_ReadPin());
								//f_write(&MyFile, buffer, 1, (void *)&byteswritten);
								break;

							case PORT_BUTTON:
								switch (Button[logVars[i].source].State) {
								case OFF: strcat(lineBuffer, "OFF"); break;
								case ON: strcat(lineBuffer, "ON"); break;
								case OPEN: strcat(lineBuffer, "OPEN"); break;
								case CLOSED: strcat(lineBuffer, "CLOSED"); break;
								case CLICKED: strcat(lineBuffer, "CLICKED"); break;
								case DBL_CLICKED: strcat(lineBuffer, "DBL_CLICKED"); break;
								case RELEASED: strcat(lineBuffer, "RELEASED"); break;
								case NONE:
									if (logs[j].type == RATE)
										strcat(lineBuffer, "NORMAL");
									break;

								default:
									break;
								}

								/* Mark for reset */
								if (NONE != Button[logVars[i].source].State)
									resetButtonState = 1;
								break;

							case PORT_DATA:

								break;

							case MEMORY_DATA_UINT8:
								sprintf((char*) lineBuffer, "%s%u", (char*) lineBuffer, logVars[i].source);
								break;

							case MEMORY_DATA_INT8:
								sprintf((char*) lineBuffer, "%s%d", (char*) lineBuffer, logVars[i].source);
								break;

							case MEMORY_DATA_UINT16:
								sprintf((char*) lineBuffer, "%s%u", (char*) lineBuffer, logVars[i].source);
								break;

							case MEMORY_DATA_INT16:
								sprintf((char*) lineBuffer, "%s%d", (char*) lineBuffer, logVars[i].source);
								break;

							case MEMORY_DATA_UINT32:
								sprintf((char*) lineBuffer, "%s%u", (char*) lineBuffer, logVars[i].source);
								break;

							case MEMORY_DATA_INT32:
								sprintf((char*) lineBuffer, "%s%d", (char*) lineBuffer, logVars[i].source);
								break;

							case MEMORY_DATA_FLOAT:
								sprintf((char*) lineBuffer, "%s%f", (char*) lineBuffer, logVars[i].sourceFloat);
								break;

							default:
								break;
							}
						}
					}
				}

				/***************************************************************************/
				/* Write to file if line is ready ******************************************/
				/***************************************************************************/
				if (0 == newLine) {
					f_write(&MyFile, lineBuffer, strlen((const char*) lineBuffer), (void*) &byteswritten);
					newLine = 1; /* Prepare for next line */
				}

				f_close(&MyFile); /* Close file after writing */

				/* Reset time for next sample */
				if (u32lTick >= u32lRate)
					logs[j].t0 = HAL_GetTick();
			} else
				continue; /* Skip inactive logs */
		}

		/* Reset button state if any were triggered */
		if (resetButtonState) {
			DelayButtonStateReset = false;
			resetButtonState = 0;
		}
		taskYIELD();
	}
}

/***************************************************************************/
/* Check if a logged variable event has occurred.
 * varIndex: Log variable array index.
*/
uint8_t CheckLogVarEvent(uint16_t varIndex) {
	uint8_t temp_uint8 = 0;

	switch (logVars[varIndex].type) {
	case PORT_DIGITAL:
		break;

	case PORT_BUTTON:
		if ((Button[logVars[varIndex].source].State != temp_uint8) && (Button[logVars[varIndex].source].State != 0)) {
			temp_uint8 = Button[logVars[varIndex].source].State;
			return 1;
		} else if ((Button[logVars[varIndex].source].State != temp_uint8) && (Button[logVars[varIndex].source].State == 0)) {
			temp_uint8 = Button[logVars[varIndex].source].State;
			return 0;
		}
		break;

	case PORT_DATA:
		break;

	case MEMORY_DATA_UINT8:
		if (*(__IO uint8_t*) &logVars[varIndex].source != (uint8_t) compareValue[varIndex]) {
			*(uint8_t*) &compareValue[varIndex] = *(__IO uint8_t*) &logVars[varIndex].source;
			return 1;
		}
		break;

	case MEMORY_DATA_INT8:
		if (*(__IO int8_t*) &logVars[varIndex].source != (int8_t) compareValue[varIndex]) {
			*(int8_t*) &compareValue[varIndex] = (int8_t) *(__IO int8_t*) &logVars[varIndex].source;
			return 1;
		}
		break;

	case MEMORY_DATA_UINT16:
		if ((uint16_t) *(__IO uint16_t*) &logVars[varIndex].source != (uint16_t) compareValue[varIndex]) {
			*(uint16_t*) &compareValue[varIndex] = (uint16_t) *(__IO uint16_t*) &logVars[varIndex].source;
			return 1;
		}
		break;

	case MEMORY_DATA_INT16:
		if ((int16_t) *(__IO uint16_t*) &logVars[varIndex].source != (int16_t) compareValue[varIndex]) {
			*(int16_t*) &compareValue[varIndex] = (int16_t) *(__IO uint16_t*) &logVars[varIndex].source;
			return 1;
		}
		break;

	case MEMORY_DATA_UINT32:
		if ((uint32_t) *(__IO uint32_t*) &logVars[varIndex].source != (uint32_t) compareValue[varIndex]) {
			compareValue[varIndex] = *(__IO uint32_t*) &logVars[varIndex].source;
			return 1;
		}
		break;

	case MEMORY_DATA_INT32:
		if ((int32_t) *(__IO uint32_t*) &logVars[varIndex].source != (int32_t) compareValue[varIndex]) {
			compareValue[varIndex] = *(__IO uint32_t*) &logVars[varIndex].source;
			return 1;
		}
		break;

	case MEMORY_DATA_FLOAT:
		if (*(__IO uint32_t*) &logVars[varIndex].sourceFloat != *(__IO uint32_t*) &compareValue[varIndex]) {
			*(__IO uint32_t*) &compareValue[varIndex] = *(__IO uint32_t*) &logVars[varIndex].sourceFloat;
			return 1;
		}
		break;

	default:
		break;
	}

	return 0;
}

/***************************************************************************/
/* Open log file if it's closed (and close open one).
 * logindex: Log array index.
 */
Module_Status OpenThisLog(uint16_t logindex, FIL *objFile) {
	FRESULT res;

	while (f_mount_ok == 0) {
		Delay_us(10);
	}

	/* Append log name with extension */
	if ((0U != logs[logindex].file_extension) && (true == enableSequential))
		sprintf((char*) tempName, "%s_%d%s", logs[logindex].name, logs[logindex].file_extension, ".TXT");
	 else
		sprintf((char*) tempName, "%s%s", logs[logindex].name, ".TXT");

	/* Open this log */
	res = f_open(objFile, tempName, FA_OPEN_APPEND | FA_WRITE | FA_READ);

	if (res != FR_OK)
		return H1BR6_ERROR;

	return H1BR6_OK;
}

/***************************************************************************/
/***************************** General Functions ***************************/
/***************************************************************************/
/* Create a new data log.
 * logName: Log file name. Max 10 char.
 * type: RATE or EVENT
 * rate: data rate in Hz (max 1000 Hz).
 * delimiterFormat: FMT_SPACE, FMT_TAB, FMT_COMMA
 * indexColumn: FMT_SAMPLE, FMT_TIME
 * indexColumnLabel: Index Column label text. Max 30 char.
*/
Module_Status CreateLog(char *logName, logType_t type, float rate, delimiterFormat_t delimiterFormat,
		indexColumnFormat_t indexColumnFormat, char *indexColumnLabel) {

	FRESULT res;

	bool extensionFile = false;
	char *pChar = NULL;
	const char logHeaderTimeDate[] = "%s %s\n";
	const char logHeaderText3[] = "Log type: Events\n\n";
	const char logHeaderText2[] = "Log type: Rate @ %.2f Hz\n\n";
	const char logHeaderText1[] = "Datalog created by BOS V%d.%d.%d on %s\n";

	const uint8_t numberMap[3] = { 1, 10, 100 };
	uint8_t i = 0;
	uint8_t countFile = 0;
	uint8_t length = 0;
	uint8_t position = 0;

	while (f_mount_ok == 0) {
		Delay_us(10);
	}

	/* Check if log already exists */
	for (i = 0; i < MAX_LOGS; i++) {
		if ((0U != logs[i].current_extension) && (true == enableSequential))
			sprintf(tempName, "%s_%d", logs[i].name, logs[i].current_extension);
		 else
			sprintf(tempName, "%s", logs[i].name);

		if (!strcmp(tempName, logName))
			return H1BR6_ERR_LogNameExists;
	}

	/* Check parameters are correct */
	if ((type != RATE && type != EVENT) || (delimiterFormat != FMT_SPACE && delimiterFormat != FMT_TAB
			&& delimiterFormat != FMT_COMMA) || (indexColumnFormat != FMT_NONE
			&& indexColumnFormat != FMT_SAMPLE && indexColumnFormat != FMT_TIME) || (rate > 1000))
		return H1BR6_ERR_WrongParams;

	/* Name does not exist. Fill first empty location */
	for (i = 0; i < MAX_LOGS; i++) {
		if (logs[i].name == 0) {
			if (true == enableSequential) {
				pChar = strchr(logName, '_');
				while (pChar != NULL) {
					position = (uint8_t) ((uint32_t) pChar - (uint32_t) logName + 1UL);
					pChar = strchr(pChar + 1, '_');
				}
				/* */
				if (0 != position) {
					pChar = logName + position;
					length = strlen(pChar);

					while ('\0' != (char) *pChar) {
						if ((0x30 <= *pChar) && (*pChar <= 0x39)) {
							countFile += ((uint8_t) (*pChar - 0x30) * numberMap[length - 1]);
						} else {
							countFile = 0;
							break;
						}
						pChar++;
						length--;
					}
				} else {
					countFile = 0;
					logs[i].current_extension = 0;
				}

				if (countFile != 0) {
					extensionFile = true;
					logs[i].current_extension = countFile;
				} else {
					position = 0;
					extensionFile = false;
				}
			}

			/* Append log name with extension */
			sprintf((char*) tempName, "%s%s", logName, ".TXT");

			/* Check if file exists on disk */
			res = f_open(&MyFile, tempName, FA_CREATE_NEW | FA_WRITE | FA_READ);

			if ((false == enableSequential) && (res == FR_EXIST))
				return H1BR6_ERR_LogNameExists;
			 else if ((res != FR_OK) && (FR_EXIST != res))
				return H1BR6_ERR_SD;
			 else if ((true == enableSequential) && (res == FR_EXIST)) {
				countFile = 0;

				do {
					memset((char*) tempName, 0, sizeof(tempName));
					if (false == extensionFile) {
						countFile++;
						sprintf(tempName, "%s_%d%s", logName, countFile, ".TXT");
					} else {
						strncpy(tempName, logName, (size_t) ((uint32_t) position - 1));
						if (0U == countFile) {
							strncat((char*) tempName, ".TXT", 5);
						} else {
							sprintf(tempName, "%s_%d%s", tempName, countFile, ".TXT");
						}
						countFile++;
					}
					res = f_open(&MyFile, tempName, FA_CREATE_NEW | FA_WRITE | FA_READ);
				} while ((FR_EXIST == res) && (MAX_DUPLICATE_FILE > countFile));

				if ((MAX_DUPLICATE_FILE == countFile) && (FR_EXIST == res))
					return H1BR6_ERR_LogNameExists;
				 else if (FR_OK != res)
					return H1BR6_ERR_SD;

				if (true == extensionFile)
					countFile--;
			}

			/* Log created successfuly */
			if ((true == enableSequential) && (0U != position)) {
				logs[i].name = malloc((size_t) position);
				memset(logs[i].name, 0x00U, (size_t) position);
				strncpy(logs[i].name, tempName, (size_t) (position - 1));
			} else {
				length = strlen(logName);
				logs[i].name = malloc(length + 1);
				memset(logs[i].name, 0x00U, (size_t) (length + 1));
				strncpy(logs[i].name, logName, (size_t) length);
			}

			logs[i].file_extension = countFile;
			logs[i].type = type;
			logs[i].rate = rate;
			logs[i].delimiterFormat = delimiterFormat;
			logs[i].indexColumnFormat = indexColumnFormat;
			logs[i].indexColumnLabel = indexColumnLabel;

			/* Write log header */
			char *buffer = malloc(100);
			memset(buffer, 0x00, 100);
			sprintf(buffer, logHeaderText1, _firmMajor, _firmMinor, _firmPatch, ModulePNstring[myPN]);
			res = f_write(&MyFile, buffer, strlen(buffer), (void*) &byteswritten);

			if (enableTimeDateHeader) {
				GetTimeDate();
				sprintf(buffer, logHeaderTimeDate, GetDateString(), GetTimeString());
				res = f_write(&MyFile, buffer, strlen(buffer), (void*) &byteswritten);
			}
			if (type == RATE) {
				sprintf(buffer, logHeaderText2, rate);
				res = f_write(&MyFile, buffer, strlen(buffer), (void*) &byteswritten);
			} else if (type == EVENT) {
				res = f_write(&MyFile, logHeaderText3, strlen(logHeaderText3), (void*) &byteswritten);
			}

			/* Write index label */
			res = f_write(&MyFile, indexColumnLabel, strlen(indexColumnLabel), (void*) &byteswritten);

			f_close(&MyFile);
			free(buffer);

			return H1BR6_OK;
		}
	}

	return H1BR6_ERR_MaxLogs;
}

/***************************************************************************/
/* Save data from a source to an existing data log.
 * logName: Log file name.
 * type: PORT_DIGITAL, PORT_DATA, PORT_BUTTON, MEMORY_DATA.
 * source: data source. Ports (P1-Px), buttons (B1-Bx) or memory location.
 * columnLabel: Column label text. Max 30 char.
*/
Module_Status LogVar(char *logName, logVarType_t type, uint32_t *source, char *ColumnLabel) {
	uint8_t i = 0, j = 0;

	/* Search for this log to make sure it exists */
	for (j = 0; j < MAX_LOGS; j++) {
		if ((0 != logs[j].current_extension) && (true == enableSequential))
			sprintf(tempName, "%s_%d", logs[j].name, logs[j].current_extension);
		 else
			sprintf(tempName, "%s", logs[j].name);

		if (!strcmp(tempName, logName)) {
			/* Make sure there's enough space for this log variable */
			for (i = 0; i < MAX_LOG_VARS; i++) {
				if (logVars[i].type == 0) {
					logVars[i].type = type;

					if (type > 3) {
						if (!((uint32_t) source < FLASH_BASE || (uint32_t) source > (FLASH_BASE + FLASH_SIZE))
								&& ((uint32_t) source < SRAM_BASE || (uint32_t) source > (SRAM_BASE + SRAM_SIZE))
								&& ((uint32_t) source < PERIPH_BASE || (uint32_t) source > (PERIPH_BASE + PERIPH_SIZE)))
							return H1BR6_ERR_WrongAddress;
					}

					if (type > 3)
						logVars[i].tempVar = source;
					 else
						logVars[i].source = (uint32_t) source;

					logVars[i].logIndex = j;
					logVars[i].varLabel = ColumnLabel;

					/* Write delimiter */
					OpenThisLog(j, &MyFile);
					if (logs[j].delimiterFormat == FMT_SPACE)
						f_write(&MyFile, " ", 1, (void*) &byteswritten);
					else if (logs[j].delimiterFormat == FMT_TAB)
						f_write(&MyFile, "\t", 1, (void*) &byteswritten);
					else if (logs[j].delimiterFormat == FMT_COMMA)
						f_write(&MyFile, ",", 1, (void*) &byteswritten);
					/* Write variable label */
					f_write(&MyFile, ColumnLabel, strlen(ColumnLabel), (void*) &byteswritten);
					f_close(&MyFile);

					return H1BR6_OK;
				}
			}
			return H1BR6_ERR_MaxLogVars;
		}
	}

	return H1BR6_ERR_LogDoesNotExist;
}

/***************************************************************************/
/* Start an existing data log.
 * logName: Log file name.
 */
Module_Status StartLog(char *logName) {
	uint8_t j = 0;

	/* Search for this log to make sure it exists */
	for (j = 0; j < MAX_LOGS; j++) {
		if ((0U != logs[j].current_extension) && (true == enableSequential))
			sprintf(tempName, "%s_%d", logs[j].name, logs[j].current_extension);
		else
			sprintf(tempName, "%s", logs[j].name);

		if (!strcmp(tempName, logName)) {
			activeLogs |= (0x01 << j);
			logs[j].t0 = HAL_GetTick();
			logs[j].sampleCount = 1;

			OpenThisLog(j, &MyFile);
			/* Write new line */
			f_write(&MyFile, "\n\r", 2, (void*) &byteswritten);
			f_close(&MyFile);

			return H1BR6_OK;
		}
	}

	return H1BR6_ERR_LogDoesNotExist;
}

/***************************************************************************/
/* Stop a running data log.
 * logName: Log file name.
 */
Module_Status StopLog(char *logName) {
	volatile uint8_t j = 0;

	/* Search for this log to make sure it exists */
	for (j = 0; j < MAX_LOGS; j++) {
		if ((0U != logs[j].current_extension) && (true == enableSequential))
			sprintf(tempName, "%s_%d", logs[j].name, logs[j].current_extension);
		else
			sprintf(tempName, "%s", logs[j].name);

		if (!strcmp(tempName, logName)) {
			if ((activeLogs >> j) & 0x01) {
				/* StopLog only inactive log, don't reset variable*/
				activeLogs &= ~(0x01 << j);
				return H1BR6_OK;
			} else
				return H1BR6_ERR_LogIsNotActive;

		}
	}
	return H1BR6_ERR_LogDoesNotExist;
}

/***************************************************************************/
/* Pause a running data log.
 * logName: Log file name.
*/
Module_Status PauseLog(char *logName) {
	uint8_t j = 0;

	/* Search for this log to make sure it exists */
	for (j = 0; j < MAX_LOGS; j++) {
		if ((0U != logs[j].current_extension) && (true == enableSequential))
			sprintf(tempName, "%s_%d", logs[j].name, logs[j].current_extension);
		else
			sprintf(tempName, "%s", logs[j].name);

		if (!strcmp(tempName, logName)) {
			if ((activeLogs >> j) & 0x01) {
				activeLogs &= ~(0x01 << j);
				return H1BR6_OK;
			} else
				return H1BR6_ERR_LogIsNotActive;
		}
	}

	return H1BR6_ERR_LogDoesNotExist;
}

/***************************************************************************/
/* Resume a paused data log.
 * logName: Log file name.
 */
Module_Status ResumeLog(char *logName) {
	uint8_t j = 0;

	/* Search for this log to make sure it exists */
	for (j = 0; j < MAX_LOGS; j++) {
		if ((0U != logs[j].current_extension) && (true == enableSequential))
			sprintf(tempName, "%s_%d", logs[j].name, logs[j].current_extension);
		else
			sprintf(tempName, "%s", logs[j].name);

		if (!strcmp(tempName, logName)) {
			activeLogs |= (0x01 << j);
			return H1BR6_OK;
		}
	}

	return H1BR6_ERR_LogDoesNotExist;
}

/***************************************************************************/
/* Delete an existing data log.
 * logName: Log file name.
 * options: DELETE_ALL, KEEP_ON_DISK
*/
Module_Status DeleteLog(char *logName, options_t options, char *fileExtension) {
	Module_Status result = H1BR6_ERROR;

	char copy[20] = { '\0' };
	char fileName[30] = { '\0' };
	int i;

	fresult = f_mount(&fs, "", 0);

	if (options == DELETE_ALL) {
		sprintf(fileName, "%s.%s", logName, fileExtension); // Add file extension
		fresult = f_unlink(fileName);

		if (fresult == FR_OK)
			result = H1BR6_OK;

	} else if (options == KEEP_ON_DISK) {
		sprintf(fileName, "%s.%s", logName, fileExtension); // Add file extension
		fresult = f_open(&MyFile, fileName, FA_WRITE | FA_OPEN_ALWAYS);
		fresult = f_truncate(&MyFile);
		f_close(&MyFile);

		if (fresult == FR_OK)
			result = H1BR6_OK;
	}
	return result;
}

/***********************************************************************************/
/* Creates a new file with the specified name and extension.
 * fileName: The name of the file to be created.
 * fileExtension: The extension of the file to be created.
 */
Module_Status CreateFile(char *fileName, char *fileExtension) {
	FRESULT res;
	FILINFO fno;
	char f_fileName[MAX_NAME_LENGTH] = { 0 };

	/* Add a flag to allow card to be initialized on startup*/
	while (f_mount_ok == 0) {
		Delay_us(10);
	}

	if (fileName == NULL || fileExtension == NULL)
		return H1BR6_ERR_WrongParams;

	sprintf(f_fileName, "%s.%s", fileName, fileExtension);

	/*Check if the file already exists */
	res = f_stat(f_fileName, &fno);

	while (res == FR_OK) {
		/*the file was already existed and create a new file with number extension */
		memset(f_fileName, 0x00, MAX_NAME_LENGTH);
		++couFile;

		/* check number of file name was existed */
		if (couFile < MAX_DUPLICATE_FILE) {
			sprintf(f_fileName, "%s_%d.%s", fileName, couFile, fileExtension);
			res = f_stat(f_fileName, &fno);
		} else
			return H1BR6_ERR_LogNameExists;
	}
	/* Create a file and open it */
	res = f_open(&MyFile, f_fileName, FA_CREATE_ALWAYS | FA_READ | FA_WRITE);

	if (res != FR_OK)
		return H1BR6_ERROR;

	/* Close file */
	res = f_close(&MyFile);

	return H1BR6_OK;
}

/***********************************************************************************/
/* Write data to an existing file with a specified name and extension.
* fileName: The name of the file to write data to.
* fileExtension: The extension of the file to write data to.
* data: The data to be written to the file.
*/
Module_Status WriteDatatoFile(char *fileName, char *fileExtension, char *data) {
	FRESULT res;
	FILINFO fno;
	char f_fileName[MAX_NAME_LENGTH] = { 0 };

	if (fileName == NULL || fileExtension == NULL || data == NULL)
		return H1BR6_ERR_WrongParams;

	/*the file name already existed and print it with number extension*/
	if (couFile != 0)
		sprintf(f_fileName, "%s_%d.%s", fileName, couFile, fileExtension);
	else
		sprintf(f_fileName, "%s.%s", fileName, fileExtension);

	/*check whether the file exists or not */
	res = f_stat(f_fileName, &fno);
	if (res != FR_OK)
		return H1BR6_ERR_FileDoesNotExist;

	else {
		/* Create a file with read write access and open it */
		res = f_open(&MyFile, f_fileName, FA_OPEN_APPEND | FA_WRITE);

		if (res != FR_OK)
			return H1BR6_ERROR;

		else { /*Write the entered data to the file*/
			res = f_write(&MyFile, data, strlen(data), (void*) &byteswritten);

			if (res != FR_OK)
				return H1BR6_ERROR;

			/* Close file */
			res = f_close(&MyFile);
		}
		return H1BR6_OK;
	}
}

/***************************************************************************/
/********************************* Commands ********************************/
/***************************************************************************/
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
	res = f_open(&MyFile, "TestFile", FA_CREATE_ALWAYS | FA_WRITE | FA_READ);
	if (res != FR_OK) {
		strcpy( ( char * ) pcWriteBuffer, ( char * ) pcFileMessage);
		return pdFALSE;
	}
	writePxMutex(pcPort, ( char * ) pcOpenMessage, strlen(( char * ) pcOpenMessage), 10, 10);

	/* Verify read / write */
	res = f_write(&MyFile, "HEXABITZ", 8, (void *)&byteswritten);
	if (res != FR_OK) {
		strcpy( ( char * ) pcWriteBuffer, ( char * ) pcFileMessage);
		return pdFALSE;
	}
	char tempStr[10] = {0};
	res = f_lseek(&MyFile, 0);
	res = f_read(&MyFile, tempStr, 8, (void *)&byteswritten);
	if (res != FR_OK || strncmp(tempStr, "HEXABITZ", 8) != 0) {
		strcpy( ( char * ) pcWriteBuffer, ( char * ) pcFileMessage);
		return pdFALSE;
	}
	writePxMutex(pcPort, ( char * ) pcVerifyMessage, strlen(( char * ) pcVerifyMessage), 10, 10);

	/* Close and delete the file */
	res = f_close(&MyFile);
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

/***************************************************************************/
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

/***************************************************************************/
portBASE_TYPE deleteLogCommand( int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString )
{
	Module_Status result = H1BR6_OK;

		int8_t *pcParameterString1, *pcParameterString2,*pcParameterString3;
		portBASE_TYPE xParameterStringLength1 = 0, xParameterStringLength2 = 0,xParameterStringLength3 = 0;
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
		/* Obtain the 2rd parameter string: delete options */
		pcParameterString2 = ( int8_t * ) FreeRTOS_CLIGetParameter (pcCommandString, 2, &xParameterStringLength2);
		/* Obtain the 3rd parameter string: file extension */
		pcParameterString3 = ( int8_t * ) FreeRTOS_CLIGetParameter (pcCommandString, 3, &xParameterStringLength3);


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
			result = DeleteLog((char *)pcParameterString1,options, (char *)pcParameterString3);
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

/***************************************************************************/
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
		result = LogVar((char *)pcParameterString1, type,(uint32_t*)&source, label);
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

/***************************************************************************/
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

/***************************************************************************/
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

/***************************************************************************/
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

/***************************************************************************/
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

/***************************************************************************/
/***************** (C) COPYRIGHT HEXABITZ ***** END OF FILE ****************/
