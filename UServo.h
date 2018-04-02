/*
 * UServo.h
 *
 *  Created on: 2018年4月2日
 *      Author: Romeli
 */

#ifndef USERVO_H_
#define USERVO_H_

#include <cmsis_device.h>
#include <UPWM.h>

class UServo {
public:
	UServo(UPWM& uPWM, UPWM::OutputCh_Typedef ch, uint16_t pwmMin,
			uint16_t pwmMax, int16_t angleMin, int16_t angleMax);
	virtual ~UServo();
	void Init(int16_t angle);
	void Set(int16_t angle);
private:
	UPWM& _uPWM;
	UPWM::OutputCh_Typedef _ch;
	int32_t _gain, _offset;
	uint16_t _angleMin, _angleMax;
};

#endif /* USERVO_H_ */
