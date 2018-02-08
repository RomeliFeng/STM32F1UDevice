/*
 * UADC.h
 *
 *  Created on: 2018年2月6日
 *      Author: Romeli
 */

#ifndef UADC_H_
#define UADC_H_

#include <cmsis_device.h>
#include <UMisc.h>
#include <Tool/UEventPool.h>

class UADC {
public:
	volatile uint32_t *Data;
	voidFun CovertDoneEvent;

	UADC(ADC_TypeDef* ADCx, uint8_t channelNum, DMA_TypeDef* DMAx,
			DMA_Channel_TypeDef* DMAy_Channelx, UIT_Typedef &it,
			uint8_t overSample = 0);
	virtual ~UADC();

	void Init();
	void RefreshData();

	void SetEventPool(voidFun covDoneEvent, UEventPool &pool);

	inline uint32_t GetRange() {
		return _Range;
	}

	void IRQ();
protected:
	ADC_TypeDef* _ADCx;

	virtual void GPIOInit() = 0;
	virtual void DMARCCInit() = 0;
	virtual void ADCRCCInit() = 0;
	virtual void ADCChannelConfig() = 0;
private:
	UEventPool* _EPool;

	DMA_TypeDef* _DMAx;
	DMA_Channel_TypeDef* _DMAy_Channelx;
	UIT_Typedef &_IT;
	uint32_t _DMA_IT_TC;
	volatile uint16_t* _Data;
	uint32_t* _DataSum;
	uint32_t _Range;
	uint8_t _ChannelNum;
	uint8_t _OverSample;
	uint32_t _OverSampleCount; //最大过采样等级10，计数需要能到20位
	bool _Busy;


	void ADCInit();
	void DMAInit();
	void ITInit();
	void CalcDMATC();
};

#endif /* UADC_H_ */
