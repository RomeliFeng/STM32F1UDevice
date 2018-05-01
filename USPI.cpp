/*
 * USPI.cpp
 *
 *  Created on: Apr 7, 2018
 *      Author: Romeli
 */

#include <USPI.h>

USPI::USPI(uint16_t txBufSize, SPI_TypeDef* SPIx, UIT_Typedef& itSPIx) :
		UDMAStream(0), _SPIx(SPIx), _itSPI(itSPIx) {
	DataForRead = 0xff;
	//普通模式，不需要接收缓冲
	_periph = Periph_SPI;
}

USPI::USPI(uint16_t txBufSize, SPI_TypeDef* SPIx, UIT_Typedef& itSPIx,
		DMA_TypeDef* DMAx, DMA_Channel_TypeDef* DMAy_Channelx_Rx,
		DMA_Channel_TypeDef* DMAy_Channelx_Tx, UIT_Typedef& itDMARx,
		UIT_Typedef& itDMATx) :
		UDMAStream(0, txBufSize, DMAx, DMAy_Channelx_Rx, DMAy_Channelx_Tx,
				itDMARx, itDMATx, uint32_t(&SPIx->DR), uint32_t(&SPIx->DR)), _SPIx(
				SPIx), _itSPI(itSPIx) {
	DataForRead = 0xff;
	//DMA模式，不需要接收缓冲，打开双发送缓冲
	_periph = Periph_SPI;
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

Status_Typedef USPI::Read(uint8_t* data, uint16_t len, bool sync,
		UEvent callBackEvent) {
	ReadCallBackEvent = callBackEvent;
	switch (_mode) {
	case Mode_Normal:
		for (uint16_t i = 0; i < len; ++i) {
			_SPIx->DR = DataForRead;
			while ((_SPIx->SR & SPI_I2S_FLAG_RXNE) == 0)
				;
			data[i] = _SPIx->DR;
		}
		break;
	case Mode_DMA:
		DMAReceive(data, len);
		if (sync) {
			while (_DMARxBusy)
				;
		}
		break;
	default:
		break;
	}
	return Status_Ok;
}

Status_Typedef USPI::Write(uint8_t* data, uint16_t len, bool sync,
		UEvent callBackEvent) {
	WriteCallBackEvent = callBackEvent;
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
			while (_DMATxBusy)
				;
		}
		break;
	default:
		break;
	}
	return Status_Ok;
}

bool USPI::IsBusy() {
	if (_mode == Mode_DMA) {
		return _DMATxBusy || _DMARxBusy;
	} else {
		return false;
	}
}

void USPI::IRQSPI() {
}

void USPI::IRQDMARx() {
	//关闭DMA接收
	_DMAy_Channelx_Rx->CCR &= (uint16_t) (~DMA_CCR1_EN);
	_DMAx->IFCR = _DMAy_IT_TCx_Rx;

	_DMARxBusy = false;

	if (ReadCallBackEvent != nullptr) {
		ReadCallBackEvent();
		ReadCallBackEvent = nullptr;
	}
}

void USPI::GPIOInit() {
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_7; //SPI CLK
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
}

void USPI::SPIInit(uint16_t SPI_BaudRatePrescaler, SPIMode_Typedef mode) {
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

	_spiMode = mode;
}

void USPI::DMAInit() {
	UDMAStream::DMAInit();

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
		UDMAStream::ITInit();
		DMA_ITConfig(_DMAy_Channelx_Rx, DMA_IT_TC, ENABLE);
		DMA_ITConfig(_DMAy_Channelx_Tx, DMA_IT_TC, ENABLE);
	}

}

void USPI::DMAReceive(uint8_t *&data, uint16_t &len) {
	//如果有发送任务，等待发送任务完成
	while (_DMATxBusy || _DMARxBusy)
		;

	_DMATxBusy = true;
	_DMARxBusy = true;

	//设置DMA地址
	_DMAy_Channelx_Tx->CMAR = uint32_t(&DataForRead);
	_DMAy_Channelx_Tx->CNDTR = len;
	//设置内存地址自增
	_DMAy_Channelx_Tx->CCR &= (~DMA_CCR1_MINC);

	//设置DMA接收参数，用于接收数据
	_DMAy_Channelx_Rx->CMAR = uint32_t(data);
	_DMAy_Channelx_Rx->CNDTR = len;

	//使能DMA开始发送
	_DMAy_Channelx_Rx->CCR |= DMA_CCR1_EN;
	_DMAy_Channelx_Tx->CCR |= DMA_CCR1_EN;
}
