/*
 * UTimeTick.cpp
 *
 *  Created on: 2017��1��11��
 *      Author: Romeli
 */

#include <UTimeTick.h>

UTimeTick::UTimeTick(TIM_TypeDef* TIMx, UIT_Typedef& it) {
	_TIMx = TIMx;
	_IT = it;
}

void UTimeTick::Init(uint16_t ms) {
	TIMInit(ms);
	NVICInit();
}

void UTimeTick::TIMInit(uint16_t ms) {
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;

	TIMRCCInit();
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseStructure.TIM_Prescaler = uint16_t(
			(SystemCoreClock >> 1) / 1000 - 1);
	TIM_TimeBaseStructure.TIM_Period = ms << 1;
	TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(_TIMx, &TIM_TimeBaseStructure);
}

void UTimeTick::NVICInit() {
	NVIC_InitTypeDef NVIC_InitStructure;

	NVIC_InitStructure.NVIC_IRQChannel = _IT.NVIC_IRQChannel;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority =
			_IT.PreemptionPriority;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = _IT.SubPriority;
	NVIC_Init(&NVIC_InitStructure);

	TIM_ITConfig(_TIMx, TIM_IT_Update, ENABLE);

	TIM_Cmd(_TIMx, ENABLE);
}

void UTimeTick::IRQ() {
	if ((_TIMx->SR & TIM_IT_Update) != 0) {
		TryDo();
		_TIMx->SR = (uint16_t) ~TIM_IT_Update;
	}
}

