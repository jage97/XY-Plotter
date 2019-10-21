/*
 ===============================================================================
 Name        : main.c
 Author      : $(author)
 Version     :
 Copyright   : $(copyright)
 Description : main definition
 ===============================================================================
 */

#if defined (__USE_LPCOPEN)
#if defined(NO_BOARD_LIB)
#include "chip.h"
#else
#include "board.h"
#endif
#endif
#include <cr_section_macros.h>
#include <vector>
#include "DigitalIoPin.h"
#include "Fmutex.h"
#include "Motor.h"
#include "math.h"
#include "swm_15xx.h"
#include "semphr.h"
#include "FreeRTOS.h"
#include "task.h"
#include <string>
#include "Parser.h"

using namespace std;

volatile uint32_t RIT_count;
SemaphoreHandle_t sbRIT = 0;
SemaphoreHandle_t goDone = 0;

void sendData(vector<string> lineParts);

int speedRIT = 200; //lower is higher
Motor *X;
Motor *Y;
DigitalIoPin *pen;

Parser parser;

//RIT timer implementation

extern "C" {
void RIT_IRQHandler(void) {
	// This used to check if a context switch is required
	portBASE_TYPE xHigherPriorityWoken = pdFALSE;
	// Tell timer that we have processed the interrupt.
	// Timer then removes the IRQ until next match occurs
	Chip_RIT_ClearIntStatus(LPC_RITIMER); // clear IRQ flag
	if (RIT_count > 0) {
		RIT_count--;
		if (RIT_count % 2 == 0) {
			if (X->bMove) { //simple check if that motor has to be moved
				X->stop();
			}
			if (Y->bMove) {
				Y->stop();
			}
		} else {
			if (X->bMove) {
				X->move();
			}
			if (Y->bMove) {
				Y->move();
			}
		}
	} else {
		X->bMove = false;
		Y->bMove = false;
		Chip_RIT_Disable(LPC_RITIMER); // disable timer
		// Give semaphore and set context switch flag if a higher priority task was woken up
		xSemaphoreGiveFromISR(sbRIT, &xHigherPriorityWoken);
	}
	// End the ISR and (possibly) do a context switch
	portEND_SWITCHING_ISR(xHigherPriorityWoken);
}
}
void RIT_start(int count, int us) {
	uint64_t cmp_value;
	// Determine approximate compare value based on clock rate and passed interval
	cmp_value = (uint64_t) Chip_Clock_GetSystemClockRate() * (uint64_t) us
			/ 1000000;
	// disable timer during configuration
	Chip_RIT_Disable(LPC_RITIMER);
	RIT_count = count;
	// enable automatic clear on when compare value==timer value
	// this makes interrupts trigger periodically
	Chip_RIT_EnableCompClear(LPC_RITIMER);
	// reset the counter
	Chip_RIT_SetCounter(LPC_RITIMER, 0);
	Chip_RIT_SetCompareValue(LPC_RITIMER, cmp_value);
	// start counting
	Chip_RIT_Enable(LPC_RITIMER);
	// Enable the interrupt signal in NVIC (the interrupt controller)
	NVIC_EnableIRQ(RITIMER_IRQn);
	// wait for ISR to tell that we're done
	if (xSemaphoreTake(sbRIT, portMAX_DELAY) == pdTRUE) {
		// Disable the interrupt signal in NVIC (the interrupt controller)
		NVIC_DisableIRQ(RITIMER_IRQn);
		xSemaphoreGive(goDone);
	} else {
		// unexpected error
		DEBUGOUT("RIT COULD NOT BE STARTED\n\r");
	}
}

//getting step count for X and Y axis, first moving X and then Y. After calibration motors move to home position.

bool calibrate() {
	int steps = 0;
	int cali = 0;
	bool caliStart = false;
	int stepList = 0;

	//X axis calibration

	while (cali < 2) { //1 calibration step count
		while (!X->limitSwitch1.read() && !X->limitSwitch2.read()); //1.1 waits that at least one of the limit switches is not active
		X->bMove = true;
		RIT_start(2, speedRIT); //1.2. moves X axis until it gets to limit switch
		if (xSemaphoreTake(goDone, portMAX_DELAY));
		steps++;

		if (!X->limitSwitch1.read() || !X->limitSwitch2.read()) {
			if (caliStart) {  //1.3. after first hit
				stepList = stepList + steps;
				steps = 0;
				cali++;
			} else { // 1.3. first time it arrives to limit switches
				caliStart = true;
				steps = 0;
			}
			if (X->dir) // 1.4. changes direction
				X->dir = false;
			else
				X->dir = true;

			while (!X->limitSwitch1.read() || !X->limitSwitch2.read()) { 	// 1.5. moves out of the limit switches
				//vTaskDelay(1);
				X->bMove = true;											// and removes every steps that it got after hitting them.
				RIT_start(2, speedRIT);
				if (xSemaphoreTake(goDone, portMAX_DELAY));
				if(caliStart){
					stepList--;
				}
			}
		}
	}
	stepList = stepList / 2; 	//2. gets average of the two calibrations
	X->steps = stepList-10;		// and gives that value for X step count.
	DEBUGOUT("%d \n\r", X ->steps);
	for (int i = 0;i < X->steps; i++) { //3. moves X axis to zero point
		DEBUGOUT("%d \n\r",i);
		X->bMove = true;
		RIT_start(2, speedRIT);
		if(xSemaphoreTake(goDone, portMAX_DELAY));
	}

	//Y axis calibration

	steps = 0;
	cali = 0;
	caliStart = false;
	stepList = 0;
	while (cali < 2) {
		while (!Y->limitSwitch1.read() && !Y->limitSwitch2.read())
			;
		Y->bMove = true;
		RIT_start(2, speedRIT);
		if (xSemaphoreTake(goDone, portMAX_DELAY))
			;
		steps++;

		if (!Y->limitSwitch1.read() || !Y->limitSwitch2.read()) {
			if (caliStart) {
				stepList = stepList + steps;
				steps = 0;
				cali++;
			} else {
				caliStart = true;
				steps = 0;
			}
			if (Y->dir)
				Y->dir = false;
			else
				Y->dir = true;

			while (!Y->limitSwitch1.read() || !Y->limitSwitch2.read()) {
				//vTaskDelay(1);
				Y->bMove = true;
				RIT_start(2, speedRIT);
				if (xSemaphoreTake(goDone, portMAX_DELAY));
				if(caliStart){
					stepList--;
				}
			}
		}
	}
	stepList = stepList / 2;
	Y->steps = stepList-10;
	for (int i = 0; i < Y->steps; i++) {
		Y->bMove = true;
		RIT_start(2, speedRIT);
		if (xSemaphoreTake(goDone, portMAX_DELAY));
	}
	Y->calibrated = true;
	X->calibrated = true;
	return true;
}



void plotLineLow(int x0, int y0, int x1, int y1) {
	int dx = x1 - x0;
	int dy = y1 - y0;
	int yi = 1;

	if (dy < 0) {
		yi = -1;
		dy = -dy;
	}

	int D = 2 * dy - dx;
	int y = y0;

	for (int x = x0; x < x1; x++) {
		X->bMove = true;
		RIT_start(2, speedRIT);
		if (xSemaphoreTake(goDone, portMAX_DELAY));
		if (D > 0) {
			Y->bMove = true;
			RIT_start(2, speedRIT);
			if (xSemaphoreTake(goDone, portMAX_DELAY));
			y = y + yi;
			D = D - 2 * dx;
		}
		D = D + 2 * dy;
	}
}

void plotLineHigh(int x0, int y0, int x1, int y1) {
	int dx = x1 - x0;
	int dy = y1 - y0;
	int xi = 1;

	if (dx < 0) {
		xi = -1;
		dx = -dx;
	}

	int D = 2 * dx - dy;
	int x = x0;

	for (int y = y0; y < y1; y++) {
		Y->bMove = true;
		RIT_start(2, speedRIT);
		if (xSemaphoreTake(goDone, portMAX_DELAY));
		if (D > 0) {
			X->bMove = true;
			RIT_start(2, speedRIT);
			if (xSemaphoreTake(goDone, portMAX_DELAY));
			x = x + xi;
			D = D - 2 * dy;
		}
		D = D + 2 * dx;
	}
}
void bres(int x0, int y0, int x1, int y1) {

	if (abs(y1 - y0) < abs(x1 - x0)) {	//Check the direction the curve needs to turn to
		if (y0 > y1) {					//Also check if the curve goes up or down
			Y->dir = false;
		} else {
			Y->dir = true;
		}
		if (x0 > x1) {
			X->dir = false;
			plotLineLow(x1, y1, x0, y0);
		} else {
			X->dir = true;
			plotLineLow(x0, y0, x1, y1);
		}
	} else {
		if (x0 > x1) {
			X->dir = false;
		} else {
			X->dir = true;
		}

		if (y0 > y1) {
			Y->dir = false;
			plotLineHigh(x1, y1, x0, y0);
		} else {
			Y->dir = true;
			plotLineHigh(x0, y0, x1, y1);
		}
	}
}

//servo control initiation

void SCT_Init(void) {
	Chip_SCT_Init(LPC_SCT0);
	Chip_SWM_MovablePortPinAssign(SWM_SCT0_OUT0_O, 0, 10);
	DEBUGOUT("SCT_Init \r\n");
	LPC_SCT0->CONFIG |= (1 << 17);              // two 16-bit timers, auto limit
	LPC_SCT0->CTRL_L |= (72 - 1) << 5; // set prescaler, SCTimer/PWM clock = 1 MHz
	LPC_SCT0->MATCHREL[0].L = 2000; // full duty cycle
	LPC_SCT0->MATCHREL[1].L = 1000; // duty upper time

	LPC_SCT0->EVENT[0].STATE = 0xFFFFFFFF;      // event 0 happens in all states
	LPC_SCT0->EVENT[0].CTRL = (1 << 12);               // match 0 condition only

	LPC_SCT0->EVENT[1].STATE = 0xFFFFFFFF;      // event 1 happens in all states
	LPC_SCT0->EVENT[1].CTRL = (1 << 0) | (1 << 12);    // match 1 condition only

	LPC_SCT0->OUT[0].SET = (1 << 0);             // event 0 will set   SCTx_OUT0
	LPC_SCT0->OUT[0].CLR = (1 << 1);             // event 1 will clear SCTx_OUT0

	LPC_SCT0->CTRL_L &= ~(1 << 2);  // unhalt it by clearing bit 2 of CTRL reg }
}

//brains of operation

static void getData(void *pvParameters) {
	int r;
	string line;
	calibrate();
	while (1) {
		r = Board_UARTGetChar(); //1. reads mdraw data char at a time
		if (r != EOF) { //2. when it gets all of data from single line it moves forward
			if (r == '\n') {
				sendData(parser.parse(line)); //3. Parses  line to line parts and sends info about them to send data

				line.clear(); //4. Clears line for parser
				Board_UARTPutSTR("OK\n"); // 5. sends OK to mdraw to tell that next line can be processed.

			} else {
				line.push_back(r); //1. puts the data to back of the line.
			}
		}
	}
}

//after parser gets line, parser sends it to sendData which then sends data to corresponding pins & ports
void sendData(vector<string> lineParts) {
	string inputCommand = lineParts[0];

	if (inputCommand == "G1") { //Go to position. Coordinates are in millimetres.
		lineParts[1].erase(0, 1);
		lineParts[2].erase(0, 1);
		X->coordinate = stof(lineParts[1]) * 60;
		Y->coordinate = stof(lineParts[2]) * 60;
		bres(X->lastCoordinate, Y->lastCoordinate, X->coordinate, Y->coordinate);
		X->lastCoordinate = X->coordinate;
		Y->lastCoordinate = Y->coordinate;

	} else if (inputCommand == "M1") {//Set pen position (control servo). Range 0 - 255. Pen works between 50-100% up time.
		DEBUGOUT("M1 %d\r\n",stoi(lineParts[1]));
		vTaskDelay(10);				// between 1 - 2 ms and pulse frequency is 50 Hz.
		if (stoi(lineParts[1]) != 130) {
			LPC_SCT0->MATCHREL[1].L = 1500;
		} else {
			LPC_SCT0->MATCHREL[1].L = 1000;
		}
	}
}


/* the following is required if runtime statistics are to be collected */
extern "C" {

void vConfigureTimerForRunTimeStats(void) {
	Chip_SCT_Init(LPC_SCTSMALL1);
	LPC_SCTSMALL1->CONFIG = SCT_CONFIG_32BIT_COUNTER;
	LPC_SCTSMALL1->CTRL_U = SCT_CTRL_PRE_L(255) | SCT_CTRL_CLRCTR_L; // set prescaler to 256 (255 + 1), and start timer
}

}

//system setup

static void prvSetupHardware(void) {
	SystemCoreClockUpdate();
	Board_Init();
	SCT_Init();
}

int main(void) {
	prvSetupHardware();

	//Sets laser to be off after program wakes up first time.
	DigitalIoPin laser(0, 12, false, true, true);
	laser.write(false);

	Chip_RIT_Init(LPC_RITIMER);
	NVIC_SetPriority(RITIMER_IRQn, configMAX_SYSCALL_INTERRUPT_PRIORITY + 1);
	goDone = xSemaphoreCreateBinary();
	sbRIT = xSemaphoreCreateBinary();
	X = new Motor(0, 27, 1, 3, 0, 0, 0, 28);
	Y = new Motor(0, 24, 0, 9, 0, 29, 1, 0);
	pen = new DigitalIoPin (0, 10, false, true, true);

	xTaskCreate(getData, "getData",
	configMINIMAL_STACK_SIZE * 9, NULL, (tskIDLE_PRIORITY + 2UL),
			(TaskHandle_t *) NULL);
	/* Start the scheduler */
	vTaskStartScheduler();
	delete X,Y, pen;
	/* Should never arrive here */
	return 1;
}
