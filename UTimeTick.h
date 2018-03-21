/*
 * TimeTick.h
 *
 *  Created on: 2017��1��11��
 *      Author: Romeli
 */

#ifndef UTIMETICK_H_
#define UTIMETICK_H_

#include <cmsis_device.h>
#include <Event/UEventLoop.h>

class UTimeTick: public UEventLoop {
public:
	UTimeTick(TIM_TypeDef* TIMx, UIT_Typedef& it);
	void Init(uint16_t ms);
	void IRQ();
protected:
	TIM_TypeDef* _TIMx;	//响应时间计算用定时器
	UIT_Typedef _IT; //中断优先级

	virtual void TIMRCCInit() = 0;
private:
	void TIMInit(uint16_t ms);
	void NVICInit();
};

#endif /* UTIMETICK_H_ */
