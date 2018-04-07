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
	_DMAy_IT_TCx = CalcDMATC(_DMAy_Channelx_Tx);
}

USPI::~USPI() {
}

void USPI::Init(SPISpeed_Typedef speed) {
	GPIOInit();
	ITInit();
	SPIInit();
	//最后打开SPI
	SPI_Cmd(_SPIx,ENABLE);
}

bool USPI::IsBusy() {
}

void USPI::GPIOInit() {
}

void USPI::SPIInit(SPISpeed_Typedef speed) {
}

void USPI::ITInit() {
}
