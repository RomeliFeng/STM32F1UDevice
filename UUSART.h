/*
 * UUSART.h
 *
 *  Created on: 2017年9月27日
 *      Author: Romeli
 */

#ifndef UUSART_H_
#define UUSART_H_

#include <cmsis_device.h>
#include <Communication/UStream.h>
#include <Event/UEventPool.h>
#include <Misc/UMisc.h>

class UUSART: public UStream {
public:

	enum Mode_Typedef {
		Mode_Normal, Mode_DMA
	};

	enum RS485Status_Typedef {
		RS485Status_Disable, RS485Status_Enable
	};

	enum RS485Dir_Typedef {
		RS485Dir_Tx, RS485Dir_Rx
	};

	voidFun ReceiveEvent;

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

	Status_Typedef Write(uint8_t* data, uint16_t len);

	bool CheckFrame();

	void SetEventPool(voidFun rcvEvent, UEventPool &pool);

	virtual bool IsBusy();

	Status_Typedef IRQUSART();
	Status_Typedef IRQDMATx();
protected:
	USART_TypeDef *_USARTx;
	DMA_TypeDef *_DMAx;
	DMA_Channel_TypeDef *_DMAy_Channelx_Rx;
	DMA_Channel_TypeDef *_DMAy_Channelx_Tx;
	uint32_t _DMAy_IT_TCx;

	virtual void USARTRCCInit() = 0;
	virtual void DMARCCInit() = 0;
	virtual void GPIOInit() = 0;
	virtual void RS485DirCtl(RS485Dir_Typedef dir);
private:
	volatile bool _dmaTxBusy = false;
	volatile bool _newFrame = false;
	RS485Status_Typedef _rs485Status = RS485Status_Disable;

	UEventPool* _ePool;

	UIT_Typedef _itUSART;
	UIT_Typedef _itDMARx;
	UIT_Typedef _itDMATx;

	Mode_Typedef _mode;

	UUSART(USART_TypeDef* USARTx);

	void CalcDMATC();

	void USARTInit(uint32_t baud, uint16_t USART_Parity);
	void ITInit(Mode_Typedef mode);
	void DMAInit();

	Status_Typedef DMASend(uint8_t *&data, uint16_t &len,
			Buffer_Typedef &txBuf);
};

#endif /* UUSART_H_ */
