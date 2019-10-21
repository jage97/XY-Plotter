/*
 * DigitalIoPin.cpp
 *
 *  Created on: 22.1.2019
 *      Author: jaakk
 */

#include "DigitalIoPin.h"
#include "chip.h"
#include "board.h"
DigitalIoPin::DigitalIoPin(int port, int pin, bool input, bool pullup, bool invert) {
	 this->pin = pin;
	 this->port = port;
	 if (pullup == true && invert == true){
		 Chip_IOCON_PinMuxSet(LPC_IOCON, port, pin, (IOCON_DIGMODE_EN | IOCON_MODE_PULLUP | IOCON_INV_EN));
	 }
	 if (pullup == true && invert == false){
	 		 Chip_IOCON_PinMuxSet(LPC_IOCON, port, pin, (IOCON_DIGMODE_EN | IOCON_MODE_PULLUP));
	 	 }
	 if (pullup == false && invert == true){
	 	 		 Chip_IOCON_PinMuxSet(LPC_IOCON, port, pin, (IOCON_DIGMODE_EN | IOCON_INV_EN));
	 	 	 }
	 if (pullup == false && invert == false){
	 	 		 Chip_IOCON_PinMuxSet(LPC_IOCON, port, pin, (IOCON_DIGMODE_EN));
	 	 	 }
	 if (input == true){
		 Chip_GPIO_SetPinDIRInput(LPC_GPIO, port, pin);
	 }
	 else {
		 Chip_GPIO_SetPinDIROutput(LPC_GPIO, port, pin);
	 }
}

DigitalIoPin::~DigitalIoPin() {

}

void DigitalIoPin::write(bool value){
	Chip_GPIO_SetPinState(LPC_GPIO, port,pin,value);
}

bool DigitalIoPin::read(){
	return Chip_GPIO_GetPinState(LPC_GPIO, port,pin);
}
