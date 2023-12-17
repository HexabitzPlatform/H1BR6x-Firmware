/*
 BitzOS (BOS) V0.2.9 - Copyright (C) 2017-2023 Hexabitz
 All rights reserved

 File Name     : main.c
 Description   : Main program body.
 */
/* Includes ------------------------------------------------------------------*/
#include "BOS.h"
uint32_t counter00 = 0;
/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

//void buttonClickedCallback(uint8_t port);
//void buttonPressedForXCallback(uint8_t port, uint8_t eventType);
/* Main function ------------------------------------------------------------*/

int main(void){

	Module_Init();		//Initialize Module &  BitzOS



	//Don't place your code here.
	for(;;){}
}

/*-----------------------------------------------------------*/

/* User Task */
void UserTask(void *argument){


	AddPortButton(MOMENTARY_NO, 3);								// Define a button connected to port P1
		SetButtonEvents(3, 1, 0, 3, 0, 0, 0, 0, 0,1);		// Activate a click event and a pressed_for_x event for 3 seconds

		// Create log and log button clicks
		//if ( CreateLog("Click Logger", EVENT, 10, FMT_TAB, FMT_TIME, "Sample @ 10Hz") == H1BR6_OK )
		if ( CreateLog("Click Logger", EVENT, 10, FMT_TAB, FMT_SAMPLE, "Sample @ 10Hz") == H1BR6_OK )
		{
			LogVar("Click Logger", PORT_BUTTON, P3, "Logger");
			LogVar("Click Logger", MEMORY_DATA_UINT32, (uint32_t)&counter00, "counter00");
			// Do not reset button state after writing the log since we need it to blink LED as well!
			needToDelayButtonStateReset = true;
		}
	//SD_getSpace();
    //SD_writeString("file1.txt","Hello");
	//void SD_writeVariable(char* FileName,uint8_t Variable);
	//void SD_read();
	//void SD_readFile(char* FileName);
	//void SD_fileUpdate();
  //  SD_removeFile("file2.TXT");
  // SD_unmount();
	// put your code here, to run repeatedly.
	while(1){
	//	buttonClickedCallback(3);
		//buttonPressedForXCallback(3, MOMENTARY_NO);
	}
}
void buttonClickedCallback(uint8_t port)
{
	++counter00;

	if (counter00 == 1)	StartLog("Click Logger");

	if ( counter00%10 == 0 ) {
		IND_blink(1000);
	} else {
		IND_blink(200);
	}

	needToDelayButtonStateReset = false;		// Reset button state now
}

void buttonPressedForXCallback(uint8_t port, uint8_t eventType)
{
	// The first PressedForX event we defined in SetButtonEvents was called
	if (eventType == 1)
	{
		StopLog("Click Logger");
		SetButtonEvents(P3, 0, 0, 0, 0, 0, 0, 0, 0,1);
		IND_blink(400); Delay_ms(400); IND_blink(400);
	}

	needToDelayButtonStateReset = false;// Reset button state now
	counter00=0;
}
/*-----------------------------------------------------------*/
