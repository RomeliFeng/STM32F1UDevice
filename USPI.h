/*
 * USPI.h
 *
 *  Created on: Apr 7, 2018
 *      Author: Romeli
 */

#ifndef USPI_H_
#define USPI_H_

#include <Communication/UDMAStream.h>
#include <Communication/UStream.h>

class USPI: public UDMAStream {
public:
	uint8_t DataForRead;

	USPI(uint16_t txBufSize, SPI_TypeDef* SPIx, UIT_Typedef& itSPIx);
	USPI(uint16_t txBufSize, SPI_TypeDef* SPIx, UIT_Typedef& itSPIx,
			DMA_TypeDef* DMAx, DMA_Channel_TypeDef* DMAy_Channelx_Rx,
			DMA_Channel_TypeDef* DMAy_Channelx_Tx, UIT_Typedef& itDMARx,
			UIT_Typedef& itDMATx);
	virtual ~USPI();

	void Init(uint16_t SPI_BaudRatePrescaler);

	Status_Typedef Write(uint8_t* data, uint16_t len, bool sync = false)
			override;

	Status_Typedef Read(uint8_t* data, uint16_t len, bool sync = false)
			override;

	virtual bool IsBusy() override;

	void IRQSPI();
	void IRQDMARx();
protected:
	SPI_TypeDef* _SPIx;
	UIT_Typedef _itSPI;

	virtual void GPIOInit() = 0;
	virtual void SPIRCCInit() = 0;

	virtual void SPIInit(uint16_t SPI_BaudRatePrescaler);
	void DMAInit() override;
	void ITInit() override;

	void DMAReceive(uint8_t *&data, uint16_t &len) override;
};

#endif /* USPI_H_ */
