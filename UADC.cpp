/*
 * UADC.cpp
 *
 *  Created on: 2018年2月6日
 *      Author: Romeli
 */

#include <UADC.h>
#include <Misc/UDebug.h>

/* ADC Software start mask */
#define CR2_EXTTRIG_SWSTART_Set     ((uint32_t)0x00500000)
#define CR2_EXTTRIG_SWSTART_Reset   ((uint32_t)0xFFAFFFFF)

UADC::UADC(ADC_TypeDef* ADCx, uint8_t channelNum, DMA_TypeDef* DMAx,
		DMA_Channel_TypeDef* DMAy_Channelx, UIT_Typedef &it,
		uint8_t overSample) {
	CovertDoneEvent = nullptr;

	_it = it;
	_ADCx = ADCx;
	_channelNum = channelNum;
	_overSample = overSample;
	_range = uint32_t(4095) << _overSample;
	if (_channelNum == 0) {
		//Error @Romeli ADC通道 数量不能为0
		UDebugOut("ADC channel number zero");
	}
	if (_overSample != 0) {
		if (_overSample >= 11) {
			//采样等级 ADC 12位；求和32位，过采样次数需要过采样等级*2位，（32-12）/2=10 最高过采样等级
			//Error @Romeli 过采样等级过高
			UDebugOut("ADC over sample level to high");
		}
		_overSampleCount = 0;
		_data = new uint16_t[_channelNum]();
		_dataSum = new uint32_t[_channelNum]();
	}
	Data = new uint32_t[_channelNum]();

	_ePool = nullptr;
	_DMAx = DMAx;
	_DMAy_Channelx = DMAy_Channelx;
	_DMAy_IT_TCx = 0;
	_busy = false;
}

UADC::~UADC() {
	delete Data;
}

void UADC::Init() {
	GPIOInit();
	ADCInit();
	DMAInit();
	ITInit();
}

void UADC::RefreshData() {
	if (!_busy) {
		_busy = true;
		//开始转换
		_ADCx->CR2 |= CR2_EXTTRIG_SWSTART_Set;
	}
}

void UADC::SetEventPool(UEvent covDoneEvent, UEventPool &pool) {
	CovertDoneEvent = covDoneEvent;
	_ePool = &pool;
}

void UADC::IRQ() {
	_DMAx->IFCR = _DMAy_IT_TCx;
	if (_overSample == 0) {
		_busy = false;
	} else {
		for (uint8_t i = 0; i < _channelNum; ++i) {
			_dataSum[i] += _data[i];
		}
		if (++_overSampleCount >= (1UL << (uint16_t(_overSample) << 1))) {
			//达到过采样次数，开始求平均
			for (uint8_t i = 0; i < _channelNum; ++i) {
				Data[i] = _dataSum[i] >> _overSample;
				_dataSum[i] = 0;
			}
			_overSampleCount = 0;
			_busy = false;
		} else {
			//开始新的转换
			_ADCx->CR2 |= CR2_EXTTRIG_SWSTART_Set;
			return;
		}
	}
	//调用转换完成事件或将事件加入循环
	if (CovertDoneEvent != nullptr) {
		if (_ePool != nullptr) {
			_ePool->Insert(CovertDoneEvent);
		} else {
			CovertDoneEvent();
		}
	}
}

void UADC::GPIOInit() {
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

	GPIO_StructInit(&GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
}

void UADC::DMARCCInit() {
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
}

void UADC::ADCRCCInit() {
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
}

void UADC::ADCChannelConfig() {
	ADC_RegularChannelConfig(_ADCx, 1, ADC_Channel_0, ADC_SampleTime_1Cycles5);
}

void UADC::ADCInit() {
	ADC_InitTypeDef ADC_InitStructure;

	ADCRCCInit();

	RCC_ADCCLKConfig(RCC_PCLK2_Div8);

	ADC_DeInit(_ADCx);

	if (_channelNum > 1) {
		ADC_InitStructure.ADC_ScanConvMode = ENABLE;
	} else {
		ADC_InitStructure.ADC_ScanConvMode = DISABLE;
	}
	ADC_InitStructure.ADC_NbrOfChannel = _channelNum;
	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;

	ADC_Init(_ADCx, &ADC_InitStructure);

	ADCChannelConfig();

	ADC_DMACmd(_ADCx, ENABLE);
	ADC_Cmd(_ADCx, ENABLE);

	ADC_TempSensorVrefintCmd(ENABLE);

	ADC_ResetCalibration(_ADCx);
	while (ADC_GetResetCalibrationStatus(_ADCx) == SET)
		;
	ADC_StartCalibration(_ADCx);
	while (ADC_GetCalibrationStatus(_ADCx))
		;
}

void UADC::DMAInit() {
	DMA_InitTypeDef DMA_InitStructure;

	DMARCCInit();

	DMA_DeInit(_DMAy_Channelx);

	DMA_InitStructure.DMA_BufferSize = _channelNum;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
	if (_overSample == 0) {
		DMA_InitStructure.DMA_MemoryBaseAddr = uint32_t(Data);
		DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Word;
	} else {
		DMA_InitStructure.DMA_MemoryBaseAddr = uint32_t(_data);
		DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
	}
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
	DMA_InitStructure.DMA_PeripheralBaseAddr = uint32_t(&_ADCx->DR);
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;
	DMA_Init(_DMAy_Channelx, &DMA_InitStructure);

	DMA_ITConfig(_DMAy_Channelx, DMA_IT_TC, ENABLE);
	CalcDMATC();
	DMA_Cmd(_DMAy_Channelx, ENABLE);
}

void UADC::ITInit() {
	NVIC_InitTypeDef NVIC_InitStructure;

	NVIC_InitStructure.NVIC_IRQChannel = _it.NVIC_IRQChannel;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority =
			_it.PreemptionPriority;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = _it.SubPriority;
	NVIC_Init(&NVIC_InitStructure);
}

void UADC::CalcDMATC() {
	if (_DMAy_Channelx == DMA1_Channel1) {
		_DMAy_IT_TCx = (uint32_t) DMA1_IT_TC1;
	} else if (_DMAy_Channelx == DMA1_Channel2) {
		_DMAy_IT_TCx = (uint32_t) DMA1_IT_TC2;
	} else if (_DMAy_Channelx == DMA1_Channel3) {
		_DMAy_IT_TCx = (uint32_t) DMA1_IT_TC3;
	} else if (_DMAy_Channelx == DMA1_Channel4) {
		_DMAy_IT_TCx = (uint32_t) DMA1_IT_TC4;
	} else if (_DMAy_Channelx == DMA1_Channel5) {
		_DMAy_IT_TCx = (uint32_t) DMA1_IT_TC5;
	} else if (_DMAy_Channelx == DMA1_Channel6) {
		_DMAy_IT_TCx = (uint32_t) DMA1_IT_TC6;
	} else if (_DMAy_Channelx == DMA1_Channel7) {
		_DMAy_IT_TCx = (uint32_t) DMA1_IT_TC7;
	} else if (_DMAy_Channelx == DMA2_Channel1) {
		_DMAy_IT_TCx = (uint32_t) DMA2_IT_TC1;
	} else if (_DMAy_Channelx == DMA2_Channel2) {
		_DMAy_IT_TCx = (uint32_t) DMA2_IT_TC2;
	} else if (_DMAy_Channelx == DMA2_Channel3) {
		_DMAy_IT_TCx = (uint32_t) DMA2_IT_TC3;
	} else if (_DMAy_Channelx == DMA2_Channel4) {
		_DMAy_IT_TCx = (uint32_t) DMA2_IT_TC4;
	} else if (_DMAy_Channelx == DMA2_Channel5) {
		_DMAy_IT_TCx = (uint32_t) DMA2_IT_TC5;
	}
}
