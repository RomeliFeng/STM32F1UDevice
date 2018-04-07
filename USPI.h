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
	enum SPISpeed_Typedef {
		SPISpeed_18M,
		SPISpeed_9M,
		SPISpeed_4_5M,
		SPISpeed_2_25M,
		SPISpeed_1_125M,
		SPISpeed_562_5K,
		SPISpeed_281_25K
	};

	USPI(uint16_t txBufSize, SPI_TypeDef* SPIx, UIT_Typedef& itSPIx);
	USPI(uint16_t txBufSize, SPI_TypeDef* SPIx, UIT_Typedef& itSPIx,
			DMA_TypeDef* DMAx, DMA_Channel_TypeDef* DMAy_Channelx_Rx,
			DMA_Channel_TypeDef* DMAy_Channelx_Tx, UIT_Typedef& itDMARx,
			UIT_Typedef& itDMATx);
	virtual ~USPI();

	void Init(SPISpeed_Typedef speed);

	virtual bool IsBusy() override;
protected:
	SPI_TypeDef* _SPIx;
	UIT_Typedef _itSPI, _itDMATx, _itDMARx;
	DMA_TypeDef* _DMAx;
	DMA_Channel_TypeDef* _DMAy_Channelx_Rx;
	DMA_Channel_TypeDef* _DMAy_Channelx_Tx;
	uint32_t _DMAy_IT_TCx;

	virtual void GPIOInit();
	virtual void SPIRCCInit() = 0;
private:
	void SPIInit(SPISpeed_Typedef speed);
	void ITInit();

	void calcPrescaler(SPISpeed_Typedef speed);
};

#endif /* USPI_H_ */
