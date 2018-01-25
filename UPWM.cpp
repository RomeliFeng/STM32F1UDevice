/*
 * UPWM.cpp
 *
 *  Created on: 2018年1月2日
 *      Author: Romeli
 */

#include <Device/UPWM.h>

UPWM* UPWM::_Pool[4];
uint8_t UPWM::_PoolSp = 0;

UPWM::UPWM(TIM_TypeDef* TIMx, uint8_t OutputCh) {
	_TIMx = TIMx;
	_OutputCh = OutputCh;

	//自动将对象指针加入资源池
	_Pool[_PoolSp++] = this;
}

UPWM::~UPWM() {
}

void UPWM::InitAll(uint16_t period, uint16_t pulse) {
	//初始化池内所有单元
	for (uint8_t i = 0; i < _PoolSp; ++i) {
		_Pool[i]->Init(period, pulse);
	}
	if (_PoolSp == 0) {
		//Error @Romeli 无PWM模块
		UDebugOut("There have pwm module exsit");
	}
}

void UPWM::Init(uint16_t period, uint16_t pulse) {
	GPIOInit();
	TIMInit(period, pulse);
	ITInit();
}

void UPWM::Enable() {
	TIM_Enable(_TIMx);
}

void UPWM::Disable() {
	TIM_Disable(_TIMx);
}

void UPWM::TIMInit(uint16_t period, uint16_t pulse) {
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	TIM_OCInitTypeDef TIM_OCInitStructure;

	TIMRCCInit();

	TIM_DeInit(_TIMx);
	TIM_TimeBaseStructInit(&TIM_TimeBaseInitStructure);
	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInitStructure.TIM_Prescaler = 0;
	TIM_TimeBaseInitStructure.TIM_Period = period;
	TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(_TIMx, &TIM_TimeBaseInitStructure);

	TIM_OCStructInit(&TIM_OCInitStructure);
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_Pulse = pulse;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Reset;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;

	if (IS_TIM_LIST2_PERIPH(_TIMx)) {
		//作为高级定时器必须要开启PWM输出
		TIM_CtrlPWMOutputs(_TIMx, ENABLE);
	}

	if (_OutputCh >= 0x0f) {
		//Error @Romeli PWM通道错误
		UDebugOut("Please check the channel. It looks illegal");
	}

	if ((_OutputCh & OutputCh_1) != 0) {
		TIM_OC1Init(_TIMx, &TIM_OCInitStructure);
		TIM_OC1PreloadConfig(_TIMx, TIM_OCPreload_Enable);
	}
	if ((_OutputCh & OutputCh_2) != 0) {
		TIM_OC2Init(_TIMx, &TIM_OCInitStructure);
		TIM_OC2PreloadConfig(_TIMx, TIM_OCPreload_Enable);
	}
	if ((_OutputCh & OutputCh_3) != 0) {
		TIM_OC3Init(_TIMx, &TIM_OCInitStructure);
		TIM_OC3PreloadConfig(_TIMx, TIM_OCPreload_Enable);
	}
	if ((_OutputCh & OutputCh_4) != 0) {
		TIM_OC4Init(_TIMx, &TIM_OCInitStructure);
		TIM_OC4PreloadConfig(_TIMx, TIM_OCPreload_Enable);
	}
}

void UPWM::ITInit() {
}
