/*
 * UUSART.cpp
 *
 *  Created on: 2017年9月27日
 *      Author: Romeli
 */

#include <UUSART.h>
#include <Misc/UDebug.h>
#include <cstring>

#define CR1_UE_Set                ((uint16_t)0x2000)  /*!< UUSART Enable Mask */
#define CR1_UE_Reset              ((uint16_t)0xDFFF)  /*!< UUSART Disable Mask */

UUSART::UUSART(uint16_t rxBufSize, uint16_t txBufSize, USART_TypeDef* USARTx,
		UIT_Typedef& itUSART) :
		UStream(rxBufSize, txBufSize, 0, 0) {
	_periph = Periph_USART;

	_USARTx = USARTx;

	_itUSART = itUSART;

	//默认设置
	_DMAx = 0;
	_DMAy_Channelx_Rx = 0;
	_DMAy_Channelx_Tx = 0;
	_DMAy_IT_TCx_Rx = 0;
	_DMAy_IT_TCx_Tx = 0;

	_mode = Mode_Normal;
}

UUSART::UUSART(uint16_t rxBufSize, uint16_t txBufSize, USART_TypeDef* USARTx,
		UIT_Typedef& itUSART, DMA_TypeDef* DMAx,
		DMA_Channel_TypeDef* DMAy_Channelx_Rx,
		DMA_Channel_TypeDef* DMAy_Channelx_Tx, UIT_Typedef& itDMARx,
		UIT_Typedef& itDMATx) :
		UStream(rxBufSize, txBufSize, rxBufSize, txBufSize) {
	_periph = Periph_USART;

	_USARTx = USARTx;

	_DMAx = DMAx;
	_DMAy_Channelx_Rx = DMAy_Channelx_Rx;
	_DMAy_Channelx_Tx = DMAy_Channelx_Tx;
	_DMAy_IT_TCx_Rx = CalcDMATC(_DMAy_Channelx_Rx);
	_DMAy_IT_TCx_Tx = CalcDMATC(_DMAy_Channelx_Tx);

	_itUSART = itUSART;
	_itDMARx = itDMARx;
	_itDMATx = itDMATx;

	_mode = Mode_DMA;
}

UUSART::~UUSART() {
}

/*
 * author Romeli
 * explain 初始化串口
 * param1 baud 波特率
 * param2 USART_Parity 校验位
 * param3 rs485Status 书否是RS485
 * param4 mode 中断模式还是DMA模式
 * return void
 */
void UUSART::Init(uint32_t baud, uint16_t USART_Parity,
		RS485Status_Typedef RS485Status) {
	_rs485Status = RS485Status;
	//GPIO初始化
	GPIOInit();
	//如果有流控引脚，使用切换为接受模式
	RS485StatusCtl(RS485Dir_Rx);
	//USART外设初始化
	USARTInit(baud, USART_Parity);
	if (_mode == Mode_DMA) {
		//DMA初始化
		DMAInit();
	}
	//中断初始化
	ITInit(_mode);
	//使能USART(使用库函数兼容性好)
	USART_Cmd(_USARTx, ENABLE);
}

/*
 * author Romeli
 * explain 向串口里写数组
 * param1 data 数组地址
 * param2 len 数组长度
 * return Status_Typedef
 */
Status_Typedef UUSART::Write(uint8_t* data, uint16_t len, bool sync) {
	if (len == 0) {
		//发送的字节数为0
		return Status_Error;
	}
	//发送前置为发送状态
	RS485StatusCtl(RS485Dir_Tx);
	switch (_mode) {
	case Mode_Normal:
		for (uint16_t i = 0; i < len; ++i) {
			_USARTx->DR = (data[i] & (uint16_t) 0x01FF);
			while ((_USARTx->SR & USART_FLAG_TXE) == RESET)
				;
		}
		//发送完取消发送
		RS485StatusCtl(RS485Dir_Rx);
		break;
	case Mode_DMA:
		//DMA模式 485会在中断中自动置位
		DMASend(data, len);
		if (sync) {
			//同步发送模式，等待发送结束
			while (_DMABusy)
				;
		}
		break;
	default:
		break;
	}
	return Status_Ok;
}

/*
 * author Romeli
 * explain 检查是否收到一帧新的我数据
 * return bool
 */
bool UUSART::CheckFrame() {
	if (_newFrame) {
		//读取到帧接收标志后将其置位
		_newFrame = false;
		return true;
	} else {
		return false;
	}
}

bool UUSART::IsBusy() {
	if (_mode == Mode_DMA) {
		return _DMABusy;
	} else {
		//暂时没有中断自动重发的预定
		return true;
	}
}

/*
 * author Romeli
 * explain 串口接收中断
 * return Status_Typedef
 */
Status_Typedef UUSART::IRQUSART() {
	//读取串口标志寄存器
	uint16_t staus = _USARTx->SR;
	if ((staus & USART_FLAG_IDLE) != RESET) {
		//帧接收标志被触发
		_newFrame = true;
		if (_mode == Mode_DMA) {
			//关闭DMA接收
			_DMAy_Channelx_Rx->CCR &= (uint16_t) (~DMA_CCR1_EN);

			uint16_t len = uint16_t(_rxBuf.size - _DMAy_Channelx_Rx->CNDTR);
			//清除DMA标志
//			_DMAx->IFCR = DMA1_FLAG_GL3 | DMA1_FLAG_TC3 | DMA1_FLAG_TE3
//					| DMA1_FLAG_HT3;
			//复位DMA接收区大小
			_DMAy_Channelx_Rx->CNDTR = _rxBuf.size;
			//循环搬运数据
			for (uint16_t i = 0; i < len; ++i) {
				_rxBuf.data[_rxBuf.end] = _dmaRxBuf.data[i];
				_rxBuf.end = uint16_t((_rxBuf.end + 1) % _rxBuf.size);
			}
			//开启DMA接收
			_DMAy_Channelx_Rx->CCR |= DMA_CCR1_EN;
		}
		//清除标志位
		//Note @Romeli 在F0系列中IDLE等中断要手动清除
		USART_ReceiveData(_USARTx);
		//USART_ClearITPendingBit(_USARTx, USART_IT_IDLE);
		//串口帧接收事件
		if (ReceivedEvent != nullptr) {
			if (_receivedEventPool != nullptr) {
				_receivedEventPool->Insert(ReceivedEvent);
			} else {
				ReceivedEvent();
			}
		}
	}
#ifndef USE_DMA
	//串口字节接收中断置位
	if ((staus & USART_FLAG_RXNE) != RESET) {
		//搬运数据到缓冲区
		_rxBuf.data[_rxBuf.end] = uint8_t(_USARTx->DR);
		_rxBuf.end = uint16_t((_rxBuf.end + 1) % _rxBuf.size);
	}
#endif
	//串口帧错误中断
	if ((staus & USART_FLAG_ORE) != RESET) {
		//清除标志位
		//Note @Romeli 在F0系列中IDLE等中断要手动清除
		USART_ClearITPendingBit(_USARTx, USART_IT_ORE);
	}
	return Status_Ok;
}

/*
 * author Romeli
 * explain 串口DMA发送中断
 * return Status_Typedef
 */
Status_Typedef UUSART::IRQDMATx() {
	//暂时关闭DMA发送
	_DMAy_Channelx_Tx->CCR &= (uint16_t) (~DMA_CCR1_EN);

	_DMAx->IFCR = _DMAy_IT_TCx_Tx;

	//判断当前使用的缓冲通道
	if (_DMAy_Channelx_Tx->CMAR == (uint32_t) _txBuf.data) {
		//缓冲区1发送完成，置位指针
		_txBuf.end = 0;
		//判断缓冲区2是否有数据，并且忙标志未置位（防止填充到一半发送出去）
		if (_txBuf2.end != 0 && _txBuf2.busy == false) {
			//当前使用缓冲区切换为缓冲区2，并加载DMA发送
			_DMAy_Channelx_Tx->CMAR = (uint32_t) _txBuf2.data;
			_DMAy_Channelx_Tx->CNDTR = _txBuf2.end;

			//使能DMA发送
			_DMAy_Channelx_Tx->CCR |= DMA_CCR1_EN;
			return Status_Ok;
		} else {
			_DMAy_Channelx_Tx->CMAR = 0;
			//无数据需要发送，清除发送队列忙标志
			_DMABusy = false;
		}
	} else if (_DMAy_Channelx_Tx->CMAR == (uint32_t) _txBuf2.data) {
		//缓冲区2发送完成，置位指针
		_txBuf2.end = 0;
		//判断缓冲区1是否有数据，并且忙标志未置位（防止填充到一半发送出去）
		if (_txBuf.end != 0 && _txBuf.busy == false) {
			//当前使用缓冲区切换为缓冲区1，并加载DMA发送
			_DMAy_Channelx_Tx->CMAR = (uint32_t) _txBuf.data;
			_DMAy_Channelx_Tx->CNDTR = _txBuf.end;

			//使能DMA发送
			_DMAy_Channelx_Tx->CCR |= DMA_CCR1_EN;
			return Status_Ok;
		} else {
			_DMAy_Channelx_Tx->CMAR = 0;
			//无数据需要发送，清除发送队列忙标志
			_DMABusy = false;
		}
	} else {
		//缓冲区号错误?不应发生
		return Status_Error;
	}

	RS485StatusCtl(RS485Dir_Rx);
	return Status_Ok;
}

/*
 * author Romeli
 * explain GPIO初始化，派生类需实现
 * return void
 */
void UUSART::GPIOInit() {
	/*	GPIO_InitTypeDef GPIO_InitStructure;
	 //开启GPIOC时钟
	 RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC | RCC_APB2Periph_AFIO, ENABLE);

	 GPIO_PinRemapConfig(GPIO_PartialRemap_USART3, ENABLE);

	 //设置PC10复用输出模式（TX）
	 GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	 GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	 GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	 GPIO_Init(GPIOC, &GPIO_InitStructure);

	 //设置PC11上拉输入模式（RX）
	 GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
	 GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	 GPIO_Init(GPIOC, &GPIO_InitStructure);

	 if (status == RS485Status_Enable) {
	 //设置PC12流控制引脚
	 GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
	 GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	 GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	 GPIO_Init(GPIOC, &GPIO_InitStructure);
	 }*/
}

void UUSART::RS485DirCtl(RS485Dir_Typedef dir) {
	if (dir == RS485Dir_Rx) {

	} else {

	}
}

void UUSART::USARTInit(uint32_t baud, uint16_t USART_Parity) {
	USART_InitTypeDef USART_InitStructure;
	//开启USART3时钟
	USARTRCCInit();

	//配置USART3 全双工 停止位1 无校验
	USART_DeInit(_USARTx);

	USART_InitStructure.USART_BaudRate = baud;
	USART_InitStructure.USART_HardwareFlowControl =
	USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_InitStructure.USART_Parity = USART_Parity;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	if (USART_Parity == USART_Parity_No) {
		USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	} else {
		USART_InitStructure.USART_WordLength = USART_WordLength_9b;
	}

	USART_Init(_USARTx, &USART_InitStructure);
}

/*
 * author Romeli
 * explain IT初始化
 * return void
 */
void UUSART::ITInit(Mode_Typedef mode) {
	NVIC_InitTypeDef NVIC_InitStructure;

	NVIC_InitStructure.NVIC_IRQChannel = _itUSART.NVIC_IRQChannel;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority =
			_itUSART.PreemptionPriority;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = _itUSART.SubPriority;
	NVIC_Init(&NVIC_InitStructure);

	if (mode == Mode_DMA) {
		NVIC_InitStructure.NVIC_IRQChannel = _itDMARx.NVIC_IRQChannel;
		NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority =
				_itDMARx.PreemptionPriority;
		NVIC_InitStructure.NVIC_IRQChannelSubPriority = _itDMARx.SubPriority;
		NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
		NVIC_Init(&NVIC_InitStructure);

		NVIC_InitStructure.NVIC_IRQChannel = _itDMATx.NVIC_IRQChannel;
		NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority =
				_itDMATx.PreemptionPriority;
		NVIC_InitStructure.NVIC_IRQChannelSubPriority = _itDMATx.SubPriority;
		NVIC_Init(&NVIC_InitStructure);
		//串口发送接收的DMA功能
		USART_DMACmd(_USARTx, USART_DMAReq_Tx, ENABLE);
		USART_DMACmd(_USARTx, USART_DMAReq_Rx, ENABLE);
	} else {
		//开启串口的字节接收中断
		USART_ITConfig(_USARTx, USART_IT_RXNE, ENABLE);
	}

	//开启串口的帧接收中断
	USART_ITConfig(_USARTx, USART_IT_IDLE, ENABLE);
}

/*
 * author Romeli
 * explain 初始化DMA设置
 * return void
 */
void UUSART::DMAInit() {
	DMA_InitTypeDef DMA_InitStructure;

	//开启DMA时钟
	DMARCCInit();

	DMA_DeInit(_DMAy_Channelx_Tx);

	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t) (&_USARTx->DR);
	DMA_InitStructure.DMA_MemoryBaseAddr = 0;				//临时设置，无效
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
	DMA_InitStructure.DMA_BufferSize = 10;				//临时设置，无效
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
	DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;

	DMA_Init(_DMAy_Channelx_Tx, &DMA_InitStructure);
	DMA_ITConfig(_DMAy_Channelx_Tx, DMA_IT_TC, ENABLE);
	//发送DMA不开启
//	DMA_Cmd(DMA1_Channel4, ENABLE);

	DMA_DeInit(_DMAy_Channelx_Rx);
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t) (&_USARTx->DR);
	DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t) _dmaRxBuf.data;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
	DMA_InitStructure.DMA_BufferSize = _dmaRxBuf.size;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
	DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;

	DMA_Init(_DMAy_Channelx_Rx, &DMA_InitStructure);
	DMA_Cmd(_DMAy_Channelx_Rx, ENABLE);
}

void UUSART::RS485StatusCtl(RS485Dir_Typedef dir) {
	if (_rs485Status == RS485Status_Enable) {
		switch (dir) {
		case RS485Dir_Rx:
			while ((_USARTx->SR & USART_FLAG_TC) == RESET)
				;
			RS485DirCtl(RS485Dir_Rx);
			break;
		case RS485Dir_Tx:
			RS485DirCtl(RS485Dir_Tx);
			break;
		default:
			break;
		}
	}
}
