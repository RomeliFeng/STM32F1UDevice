/*
 * UServo.cpp
 *
 *  Created on: 2018年4月2日
 *      Author: Romeli
 */

#include <UServo.h>

UServo::UServo(UPWM& uPWM, UPWM::OutputCh_Typedef ch, uint16_t pwmMin,
		uint16_t pwmMax, int16_t angleMin, int16_t angleMax) :
		_uPWM(uPWM) {
	_ch = ch;
	_angleMin = angleMin;
	_angleMax = angleMax;
	_gain = (int32_t(pwmMax - pwmMin) * 1000) / (angleMax - angleMin);
	_offset = pwmMin;
}

UServo::~UServo() {
}

/*
 * author Romeli
 * explain 初始化舵机到目标角度
 * param angle 目标角度
 * return void
 */
void UServo::Init(int16_t angle) {
	Set(angle);
}

/*
 * author Romeli
 * explain 运动舵机到目标角度
 * param angle 目标角度
 * return void
 */
void UServo::Set(int16_t angle) {
	angle = angle < _angleMin ? _angleMin : angle;
	angle = angle > _angleMax ? _angleMax : angle;
	uint16_t duty = ((int32_t) angle * _gain) / 1000 + _offset;
	_uPWM.SetPulse(_ch, duty);
}
