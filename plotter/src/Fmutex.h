/*
 * Fmutex.h
 *
 *  Created on: 27.8.2019
 *      Author: jaakk
 */

#ifndef FMUTEX_H_
#define FMUTEX_H_
#include "FreeRTOS.h"
#include "semphr.h"
#include <mutex>
class Fmutex {
public:
	Fmutex();
	virtual ~Fmutex();
	void lock();
	void unlock();
private:
	SemaphoreHandle_t mutex;
};

#endif /* FMUTEX_H_ */
