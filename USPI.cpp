/*
 * USPI.cpp
 *
 *  Created on: Apr 7, 2018
 *      Author: Romeli
 */

#include <USPI.h>

USPI::USPI(uint16_t txBufSize, SPI_TypeDef* SPIx, UIT_Typedef& itSPIx) :
		UStream(0, txBufSize, 0, 0) {
	//普通模式，不需要接收缓冲
	_periph = Periph_SPI;

	_SPIx = SPIx;
	_itSPI = itSPIx;

	_mode = Mode_Normal;
}

USPI::USPI(uint16_t txBufSize, SPI_TypeDef* SPIx, UIT_Typedef& itSPIx,
		DMA_TypeDef* DMAx, DMA_Channel_TypeDef* DMAy_Channelx_Rx,
		DMA_Channel_TypeDef* DMAy_Channelx_Tx, UIT_Typedef& itDMARx,
		UIT_Typedef& itDMATx) :
		UStream(0, txBufSize, 0, txBufSize) {
	//DMA模式，不需要接收缓冲，打开双发送缓冲
	_periph = Periph_SPI;

	_SPIx = SPIx;

	_itSPI = itSPIx;
	_itDMARx = itDMARx;
	_itDMATx = itDMATx;

	_DMAx = DMAx;
	_DMAy_Channelx_Rx = DMAy_Channelx_Rx;
	_DMAy_Channelx_Tx = DMAy_Channelx_Tx;
	_DMAy_IT_TCx_Rx = CalcDMATC(_DMAy_Channelx_Rx);
	_DMAy_IT_TCx_Tx = CalcDMATC(_DMAy_Channelx_Tx);

	_mode = Mode_DMA;
}

USPI::~USPI() {
}

void USPI::Init(uint16_t SPI_BaudRatePrescaler) {
	GPIOInit();
	SPIInit(SPI_BaudRatePrescaler);
	if (_mode == Mode_DMA) {
		DMAInit();
	}
	ITInit();
	//最后打开SPI
	SPI_Cmd(_SPIx, ENABLE);
}

Status_Typedef USPI::Write(uint8_t* data, uint16_t len, bool sync) {
	switch (_mode) {
	case Mode_Normal:
		for (uint16_t i = 0; i < len; ++i) {
			_SPIx->DR = data[i];
			while ((_SPIx->SR & SPI_I2S_FLAG_RXNE) == 0)
				;
		}
		break;
	case Mode_DMA:
		DMASend(data, len);
		if (sync) {
			while (_DMABusy)
				;
		}
		break;
	default:
		break;
	}
	return Status_Ok;
}

Status_Typedef USPI::Write(uint8_t data, bool sync) {
	return Write(&data, 1, sync);
}

Status_Typedef USPI::Read(uint8_t* data, uint16_t len, bool sync) {
	switch (_mode) {
	case Mode_Normal:

		break;
	case Mode_DMA:

		break;
	default:
		break;
	}
	return Status_Ok;
}

Status_Typedef USPI::Read(uint8_t* data, bool sync) {
	return Read(data, 1, sync);
}

bool USPI::IsBusy() {
	if (_mode == Mode_DMA) {
		return _DMABusy;
	} else {
		return false;
	}
}

void USPI::IRQSPI() {
}

void USPI::IRQDMARx() {
}

void USPI::IRQDMATx() {
}

void USPI::GPIOInit() {
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_7; //SPI CLK
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
}

void USPI::SPIInit(uint16_t SPI_BaudRatePrescaler) {
	SPI_InitTypeDef SPI_InitStructure;

	SPIRCCInit();

	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler;
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
	SPI_InitStructure.SPI_CRCPolynomial = 7;

	SPI_Init(_SPIx, &SPI_InitStructure);
}

void USPI::DMAInit() {
	DMA_InitTypeDef DMA_InitStructure;

	DMARCCInit();

	DMA_DeInit(_DMAy_Channelx_Rx);
	DMA_InitStructure.DMA_PeripheralBaseAddr = uint32_t(&_SPIx->DR);
	DMA_InitStructure.DMA_MemoryBaseAddr = 0; //使用时进行设置
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
	DMA_InitStructure.DMA_BufferSize = 0; //使用时进行设置
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
	DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
	DMA_Init(_DMAy_Channelx_Rx, &DMA_InitStructure);

	DMA_DeInit(_DMAy_Channelx_Tx);
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
	DMA_Init(_DMAy_Channelx_Tx, &DMA_InitStructure);

	DMA_ITConfig(_DMAy_Channelx_Rx, DMA_IT_TC, ENABLE);
	DMA_ITConfig(_DMAy_Channelx_Tx, DMA_IT_TC, ENABLE);

	SPI_I2S_DMACmd(_SPIx, SPI_I2S_DMAReq_Rx, ENABLE);
	SPI_I2S_DMACmd(_SPIx, SPI_I2S_DMAReq_Tx, ENABLE);
}

void USPI::ITInit() {
	NVIC_InitTypeDef NVIC_InitStructure;

	if (_mode == Mode_Normal) {
		NVIC_InitStructure.NVIC_IRQChannel = _itSPI.NVIC_IRQChannel;
		NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority =
				_itSPI.PreemptionPriority;
		NVIC_InitStructure.NVIC_IRQChannelSubPriority = _itSPI.SubPriority;
		NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
		NVIC_Init(&NVIC_InitStructure);

	} else if (_mode == Mode_DMA) {
		NVIC_InitStructure.NVIC_IRQChannel = _itDMATx.NVIC_IRQChannel;
		NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority =
				_itDMATx.PreemptionPriority;
		NVIC_InitStructure.NVIC_IRQChannelSubPriority = _itDMATx.SubPriority;
		NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
		NVIC_Init(&NVIC_InitStructure);

		NVIC_InitStructure.NVIC_IRQChannel = _itDMARx.NVIC_IRQChannel;
		NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority =
				_itDMARx.PreemptionPriority;
		NVIC_InitStructure.NVIC_IRQChannelSubPriority = _itDMARx.SubPriority;
		NVIC_Init(&NVIC_InitStructure);
	}

}

