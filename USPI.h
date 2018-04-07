/*
 * USPI.h
 *
 *  Created on: Apr 7, 2018
 *      Author: Romeli
 */

#ifndef USPI_H_
#define USPI_H_

#include <Communication/UStream.h>

class USPI: UStream {
public:
	USPI(uint16_t txBufSize, SPI_TypeDef* SPIx, UIT_Typedef& itSPIx);
	USPI(uint16_t txBufSize, SPI_TypeDef* SPIx, UIT_Typedef& itSPIx,
			DMA_TypeDef* DMAx, DMA_Channel_TypeDef* DMAy_Channelx_Rx,
			DMA_Channel_TypeDef* DMAy_Channelx_Tx, UIT_Typedef& itDMARx,
			UIT_Typedef& itDMATx);
	virtual ~USPI();

	void Init(uint16_t  SPI_BaudRatePrescaler);

	virtual bool IsBusy() override;
protected:
	SPI_TypeDef* _SPIx;
	UIT_Typedef _itSPI, _itDMATx, _itDMARx;
	DMA_TypeDef* _DMAx;
	DMA_Channel_TypeDef* _DMAy_Channelx_Rx;
	DMA_Channel_TypeDef* _DMAy_Channelx_Tx;
	uint32_t _DMAy_IT_TCx;

	virtual void GPIOInit() = 0;
	virtual void SPIRCCInit() = 0;
	virtual void SPIInit(uint16_t SPI_BaudRatePrescaler);
	void DMAInit();
	void ITInit();
};

#endif /* USPI_H_ */
