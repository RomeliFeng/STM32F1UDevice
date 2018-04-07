/*
 * UADC.h
 *
 *  Created on: 2018年2月6日
 *      Author: Romeli
 */

#ifndef UADC_H_
#define UADC_H_

#include <cmsis_device.h>
#include <Misc/UMisc.h>
#include <Event/UEventPool.h>

class UADC {
public:
	volatile uint32_t *Data;
	UEvent CovertDoneEvent;

	UADC(ADC_TypeDef* ADCx, uint8_t channelNum, DMA_TypeDef* DMAx,
			DMA_Channel_TypeDef* DMAy_Channelx, UIT_Typedef &it,
			uint8_t overSample = 0);
	virtual ~UADC();

	void Init();
	void RefreshData();

	void SetEventPool(UEvent covDoneEvent, UEventPool &pool);

	inline uint32_t GetRange() {
		return _range;
	}

	void IRQ();
protected:
	ADC_TypeDef* _ADCx;

	UEventPool* _ePool;

	DMA_TypeDef* _DMAx;
	DMA_Channel_TypeDef* _DMAy_Channelx;
	UIT_Typedef _it;
	uint32_t _DMAy_IT_TCx;
	volatile uint16_t* _data;
	uint32_t* _dataSum;
	uint32_t _range;
	uint8_t _channelNum;
	uint8_t _overSample;
	uint32_t _overSampleCount; //最大过采样等级10，计数需要能到20位
	bool _busy;

	virtual void GPIOInit() = 0;
	virtual void DMARCCInit() = 0;
	virtual void ADCRCCInit() = 0;
	virtual void ADCChannelConfig() = 0;
	void ADCInit();
	void DMAInit();
	void ITInit();
};

#endif /* UADC_H_ */
