/*
 * Motor.cpp
 *
 *  Created on: 3.10.2019
 *      Author: jaakk
 */

#include "Motor.h"

Motor::Motor(int portM, int pinM, int portS1, int pinS1, int portS2, int pinS2,
		int portDir, int pinDir) :
		motor(portM, pinM, false, true, true), limitSwitch1(portS1, pinS1, true,
				true, false), limitSwitch2(portS2, pinS2, true, true, false), DipDirection(
				portDir, pinDir, false, true, true) {
}

void Motor::move() {
    if (calibrated == false) {
        DipDirection.write(dir);
        motor.write(true);
    }
    if (calibrated) {
        if (limitSwitch1.read() && limitSwitch2.read() ) {
            DipDirection.write(dir);
            motor.write(true);
        }
    }
}
void Motor::stop() {
    if (calibrated == false) {
        DipDirection.write(dir);
        motor.write(false);
    }
    if (calibrated) {
        if (limitSwitch1.read() && limitSwitch2.read() ) {
            DipDirection.write(dir);
            motor.write(false);
        }
    }
}
