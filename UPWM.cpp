/*
 * UPWM.cpp
 *
 *  Created on: 2018年1月2日
 *      Author: Romeli
 */

#include <UPWM.h>
#include <Misc/UDebug.h>

UPWM* UPWM::_Pool[4];
uint8_t UPWM::_PoolSp = 0;

UPWM::UPWM(TIM_TypeDef* TIMx, uint8_t OutputCh, uint16_t prescaler) {
	_TIMx = TIMx;
	_OutputCh = OutputCh;
	_Prescaler = prescaler;

	//自动将对象指针加入资源池
	_Pool[_PoolSp++] = this;
}

UPWM::~UPWM() {
}

/*
 * author Romeli
 * explain 使用频率和占空比初始化所有的PWM模块，默认不开启输出
 * param1 period 定时器时钟/period=频率
 * param2 pulse 占空比
 * return void
 */
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

/*
 * author Romeli
 * explain 使用频率和占空比初始化PWM模块，默认不开启输出
 * param1 period 定时器时钟/period=频率
 * param2 pulse 占空比
 * return void
 */
void UPWM::Init(uint16_t period, uint16_t pulse) {
	GPIOInit();
	TIMInit(period, pulse);
	ITInit();
	TIM_Enable(_TIMx);
}

/*
 * author Romeli
 * explain 开启PWM输出
 * return void
 */
void UPWM::Enable(OutputCh_Typedef outputCh) {
	if ((outputCh & _OutputCh & OutputCh_1) != 0) {
		TIM_CCxCmd(_TIMx, TIM_Channel_1, TIM_CCx_Enable);
	}
	if ((outputCh & _OutputCh & OutputCh_2) != 0) {
		TIM_CCxCmd(_TIMx, TIM_Channel_2, TIM_CCx_Enable);
	}
	if ((outputCh & _OutputCh & OutputCh_3) != 0) {
		TIM_CCxCmd(_TIMx, TIM_Channel_3, TIM_CCx_Enable);
	}
	if ((outputCh & _OutputCh & OutputCh_4) != 0) {
		TIM_CCxCmd(_TIMx, TIM_Channel_4, TIM_CCx_Enable);
	}
}

/*
 * author Romeli
 * explain 关闭PWM输出
 * param outputCh 需要关闭的通道
 * return void
 */
void UPWM::Disable(OutputCh_Typedef outputCh) {
	if ((outputCh & _OutputCh & OutputCh_1) != 0) {
		TIM_CCxCmd(_TIMx, TIM_Channel_1, TIM_CCx_Disable);
	}
	if ((outputCh & _OutputCh & OutputCh_2) != 0) {
		TIM_CCxCmd(_TIMx, TIM_Channel_2, TIM_CCx_Disable);
	}
	if ((outputCh & _OutputCh & OutputCh_3) != 0) {
		TIM_CCxCmd(_TIMx, TIM_Channel_3, TIM_CCx_Disable);
	}
	if ((outputCh & _OutputCh & OutputCh_4) != 0) {
		TIM_CCxCmd(_TIMx, TIM_Channel_4, TIM_CCx_Disable);
	}
}

void UPWM::SetPulse(uint8_t outputCh, uint16_t pulse) {
	if ((outputCh & _OutputCh & OutputCh_1) != 0) {
		_TIMx->CCR1 = pulse;
	}
	if ((outputCh & _OutputCh & OutputCh_2) != 0) {
		_TIMx->CCR2 = pulse;
	}
	if ((outputCh & _OutputCh & OutputCh_3) != 0) {
		_TIMx->CCR3 = pulse;
	}
	if ((outputCh & _OutputCh & OutputCh_4) != 0) {
		_TIMx->CCR4 = pulse;
	}
}

void UPWM::TIMInit(uint16_t period, uint16_t pulse) {
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	TIM_OCInitTypeDef TIM_OCInitStructure;

	TIMRCCInit();

	TIM_TimeBaseStructInit(&TIM_TimeBaseInitStructure);
	TIM_DeInit(_TIMx);

	TIM_TimeBaseStructInit(&TIM_TimeBaseInitStructure);
	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInitStructure.TIM_Prescaler = _Prescaler;
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

