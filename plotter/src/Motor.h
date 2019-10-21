/*
 * Motor.h
 *
 *  Created on: 3.10.2019
 *      Author: jaakk
 */

#ifndef MOTOR_H_
#define MOTOR_H_
#include "DigitalIoPin.h"
#include "FreeRTOS.h"
#include "task.h"
#include "board.h"

class Motor {
public:
	DigitalIoPin motor;
	DigitalIoPin limitSwitch1;
	DigitalIoPin limitSwitch2;
	DigitalIoPin DipDirection;
	int steps = 0;
	float lastCoordinate = 0;
	float coordinate = 0;
	bool calibrated = false;
	bool bMove = false;
	bool dir = true;
	Motor(int portM, int pinM, int portS1, int pinS1, int portS2, int pinS2, int portDir, int pinDir);
	void move();
	void stop();
};


#endif /* MOTOR_H_ */
