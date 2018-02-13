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
		DMA_Channel_TypeDef* DMAy_Channelx, UIT_Typedef &it, uint8_t overSample) :
		_IT(it) {
	CovertDoneEvent = nullptr;

	_ADCx = ADCx;
	_ChannelNum = channelNum;
	_OverSample = overSample;
	_Range = uint32_t(4095) << _OverSample;
	if (_ChannelNum == 0) {
		//Error @Romeli ADC通道 数量不能为0
		UDebugOut("ADC channel number zero");
	}
	if (_OverSample != 0) {
		if (_OverSample >= 11) {
			//采样等级 ADC 12位；求和32位，过采样次数需要过采样等级*2位，（32-12）/2=10 最高过采样等级
			//Error @Romeli 过采样等级过高
			UDebugOut("ADC over sample level to high");
		}
		_OverSampleCount = 0;
		_Data = new uint16_t[_ChannelNum]();
		_DataSum = new uint32_t[_ChannelNum]();
	}
	Data = new uint32_t[_ChannelNum]();

	_EPool = nullptr;
	_DMAx = DMAx;
	_DMAy_Channelx = DMAy_Channelx;
	_DMA_IT_TC = 0;
	_Busy = false;
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
	if (!_Busy) {
		_Busy = true;
		//开始转换
		_ADCx->CR2 |= CR2_EXTTRIG_SWSTART_Set;
	}
}

void UADC::SetEventPool(voidFun covDoneEvent, UEventPool &pool) {
	CovertDoneEvent = covDoneEvent;
	_EPool = &pool;
}

void UADC::IRQ() {
	_DMAx->IFCR = _DMA_IT_TC;
	if (_OverSample == 0) {
		_Busy = false;
	} else {
		for (uint8_t i = 0; i < _ChannelNum; ++i) {
			_DataSum[i] += _Data[i];
		}
		if (++_OverSampleCount >= (1UL << (uint16_t(_OverSample) << 1))) {
			//达到过采样次数，开始求平均
			for (uint8_t i = 0; i < _ChannelNum; ++i) {
				Data[i] = _DataSum[i] >> _OverSample;
				_DataSum[i] = 0;
			}
			_OverSampleCount = 0;
			_Busy = false;
		} else {
			//开始新的转换
			_ADCx->CR2 |= CR2_EXTTRIG_SWSTART_Set;
			return;
		}
	}
	//调用转换完成事件或将事件加入循环
	if (CovertDoneEvent != nullptr) {
		if (_EPool != nullptr) {
			_EPool->Insert(CovertDoneEvent);
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

	if (_ChannelNum > 1) {
		ADC_InitStructure.ADC_ScanConvMode = ENABLE;
	} else {
		ADC_InitStructure.ADC_ScanConvMode = DISABLE;
	}
	ADC_InitStructure.ADC_NbrOfChannel = _ChannelNum;
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

	DMA_InitStructure.DMA_BufferSize = _ChannelNum;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
	if (_OverSample == 0) {
		DMA_InitStructure.DMA_MemoryBaseAddr = uint32_t(Data);
		DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Word;
	} else {
		DMA_InitStructure.DMA_MemoryBaseAddr = uint32_t(_Data);
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

	NVIC_InitStructure.NVIC_IRQChannel = _IT.NVIC_IRQChannel;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority =
			_IT.PreemptionPriority;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = _IT.SubPriority;
	NVIC_Init(&NVIC_InitStructure);
}

void UADC::CalcDMATC() {
	if (_DMAy_Channelx == DMA1_Channel1) {
		_DMA_IT_TC = (uint32_t) DMA1_IT_TC1;
	} else if (_DMAy_Channelx == DMA1_Channel2) {
		_DMA_IT_TC = (uint32_t) DMA1_IT_TC2;
	} else if (_DMAy_Channelx == DMA1_Channel3) {
		_DMA_IT_TC = (uint32_t) DMA1_IT_TC3;
	} else if (_DMAy_Channelx == DMA1_Channel4) {
		_DMA_IT_TC = (uint32_t) DMA1_IT_TC4;
	} else if (_DMAy_Channelx == DMA1_Channel5) {
		_DMA_IT_TC = (uint32_t) DMA1_IT_TC5;
	} else if (_DMAy_Channelx == DMA1_Channel6) {
		_DMA_IT_TC = (uint32_t) DMA1_IT_TC6;
	} else if (_DMAy_Channelx == DMA1_Channel7) {
		_DMA_IT_TC = (uint32_t) DMA1_IT_TC7;
	} else if (_DMAy_Channelx == DMA2_Channel1) {
		_DMA_IT_TC = (uint32_t) DMA2_IT_TC1;
	} else if (_DMAy_Channelx == DMA2_Channel2) {
		_DMA_IT_TC = (uint32_t) DMA2_IT_TC2;
	} else if (_DMAy_Channelx == DMA2_Channel3) {
		_DMA_IT_TC = (uint32_t) DMA2_IT_TC3;
	} else if (_DMAy_Channelx == DMA2_Channel4) {
		_DMA_IT_TC = (uint32_t) DMA2_IT_TC4;
	} else if (_DMAy_Channelx == DMA2_Channel5) {
		_DMA_IT_TC = (uint32_t) DMA2_IT_TC5;
	}
}
