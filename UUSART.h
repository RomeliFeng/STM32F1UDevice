/*
 * UUSART.h
 *
 *  Created on: 2017年9月27日
 *      Author: Romeli
 */

#ifndef UUSART_H_
#define UUSART_H_

#include <cmsis_device.h>
#include <Misc/UMisc.h>
#include <Communication/UStream.h>
#include <Communication/UDMAIO.h>
#include <Event/UEventPool.h>

class UUSART: public UStream {
public:
	enum RS485Status_Typedef {
		RS485Status_Disable, RS485Status_Enable
	};

	enum RS485Dir_Typedef {
		RS485Dir_Tx, RS485Dir_Rx
	};

	UUSART(uint16_t rxBufSize, uint16_t txBufSize, USART_TypeDef* USARTx,
			UIT_Typedef& itUSART);
	UUSART(uint16_t rxBufSize, uint16_t txBufSize, USART_TypeDef* USARTx,
			UIT_Typedef& itUSART, DMA_TypeDef* DMAx,
			DMA_Channel_TypeDef* DMAy_Channelx_Rx,
			DMA_Channel_TypeDef* DMAy_Channelx_Tx, UIT_Typedef& itDMARx,
			UIT_Typedef& itDMATx);
	virtual ~UUSART();

	void Init(uint32_t baud, uint16_t USART_Parity = USART_Parity_No,
			RS485Status_Typedef RS485Status = RS485Status_Disable);

	Status_Typedef Write(uint8_t* data, uint16_t len, bool sync = false)
			override;

	bool CheckFrame();

	virtual bool IsBusy() override;

	void IRQUSART();
	void IRQDMATx();
protected:
	USART_TypeDef* _USARTx;

	volatile bool _newFrame = false;
	RS485Status_Typedef _rs485Status = RS485Status_Disable;

	UIT_Typedef _itUSART;

	virtual void USARTRCCInit() = 0;
	virtual void DMARCCInit() = 0;
	virtual void GPIOInit() = 0;
	virtual void RS485DirCtl(RS485Dir_Typedef dir);

	void USARTInit(uint32_t baud, uint16_t USART_Parity);
	void ITInit(Mode_Typedef mode);
	void DMAInit();

	void RS485StatusCtl(RS485Dir_Typedef dir);
};

#endif /* UUSART_H_ */
