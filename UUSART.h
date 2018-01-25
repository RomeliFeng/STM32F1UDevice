/*
 * UUSART.h
 *
 *  Created on: 2017年9月27日
 *      Author: Romeli
 */

#ifndef UUSART_H_
#define UUSART_H_

#include <cmsis_device.h>
#include <Communication/USteam.h>
#include <UDebug.h>
#include <UMisc.h>
#include <functional>

using std::function;

class UUSART: public USteam {
public:

	enum Mode_Typedef {
		Mode_Interrupt, Mode_DMA
	};

	enum RS485Status_Typedef {
		RS485Status_Enable, RS485Status_Disable
	};

	enum RS485Dir_Typedef {
		RS485Dir_Tx, RS485Dir_Rx
	};

	function<void(void)> ReceiveEvent;

	UUSART(uint16_t rxBufSize, uint16_t txBufSize, USART_TypeDef* USARTx,
			UIT_Typedef itUSARTX);
	UUSART(uint16_t rxBufSize, uint16_t txBufSize, USART_TypeDef* USARTx,
			UIT_Typedef itUSARTx, DMA_TypeDef* DMAx,
			DMA_Channel_TypeDef* DMAy_Channelx_Rx,
			DMA_Channel_TypeDef* DMAy_Channelx_Tx, UIT_Typedef itDMAxRx,
			UIT_Typedef itDMAxTx);
	virtual ~UUSART();

	void Init(uint32_t baud, uint16_t USART_Parity = USART_Parity_No,
			RS485Status_Typedef RS485Status = RS485Status_Disable);

	Status_Typedef Write(uint8_t* data, uint16_t len);

	bool CheckFrame();

	Status_Typedef IRQUSART();
	Status_Typedef IRQDMATx();
protected:
	USART_TypeDef *_USARTx;
	DMA_TypeDef *_DMAx;
	DMA_Channel_TypeDef *_DMAy_Channelx_Rx;
	DMA_Channel_TypeDef *_DMAy_Channelx_Tx;
	uint32_t _DMA_IT_TC_TX;

	virtual void USARTRCCInit() = 0;
	virtual void DMARCCInit() = 0;
	virtual void GPIOInit() = 0;
	virtual void RS485DirCtl(RS485Dir_Typedef dir);
private:
	volatile bool _DMATxBusy = false;
	volatile bool _newFrame = false;
	RS485Status_Typedef _RS485Status = RS485Status_Disable;

	DataSteam_Typedef _DMARxBuf;
	DataSteam_Typedef _DMATxBuf;

	UIT_Typedef _ITUSARTx;
	UIT_Typedef _ITDMAxRx;
	UIT_Typedef _ITDMAxTx;

	Mode_Typedef _mode;

	void CalcDMATC();

	void USARTInit(uint32_t baud, uint16_t USART_Parity);
	void ITInit(Mode_Typedef mode);
	void DMAInit();

	Status_Typedef DMASend(DataSteam_Typedef *stack, DataSteam_Typedef *txBuf);
};

#endif /* UUSART_H_ */
