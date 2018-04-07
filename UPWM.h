/*
 * UPWM.h
 *
 *  Created on: 2018年1月2日
 *      Author: Romeli
 */

#ifndef UPWM_H_
#define UPWM_H_

#include <cmsis_device.h>
#include <Misc/UMisc.h>

class UPWM {
public:
	enum OutputCh_Typedef
		:uint8_t {

			OutputCh_1 = 0x01,
		OutputCh_2 = 0x02,
		OutputCh_3 = 0x04,
		OutputCh_4 = 0x08,
		OutputCh_All = 0x0f
	};

	UPWM(TIM_TypeDef* TIMx, uint8_t outputCh, uint16_t prescaler = 0,
			bool inverting = false);
	virtual ~UPWM();

	//初始化硬件
	static void InitAll(uint16_t period, uint16_t pulse);
	void Init(uint16_t period, uint16_t pulse);
	void Enable(OutputCh_Typedef outputCh);
	void Disable(OutputCh_Typedef outputCh);
	void SetPulse(uint8_t outputCh, uint16_t pulse);
protected:
	virtual void GPIOInit() = 0;
	virtual void TIMRCCInit() = 0;
	virtual void TIMInit(uint16_t period, uint16_t pulse);
	virtual void ITInit();
private:
	static UPWM* _pool[4];
	static uint8_t _poolSp;

	TIM_TypeDef* _TIMx;
	uint8_t _outputCh;
	uint16_t _prescaler;
	bool _inverting;
};

#endif /* UPWM_H_ */
