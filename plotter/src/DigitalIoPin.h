/*
 * DigitalIoPin.h
 *
 *  Created on: 22.1.2019
 *      Author: jaakk
 */

#ifndef DIGITALIOPIN_H_
#define DIGITALIOPIN_H_

class DigitalIoPin {
public:
 DigitalIoPin(int port, int pin, bool input, bool pullup, bool invert);
 virtual ~DigitalIoPin();
 bool read();
 void write(bool value);
private:
 int port;
 int pin;
 // add these as needed
};

#endif /* DIGITALIOPIN_H_ */
