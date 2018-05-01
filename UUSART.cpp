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
		UDMAStream(rxBufSize) {
	_periph = Periph_USART;

	_USARTx = USARTx;

	_itUSART = itUSART;

	_mode = Mode_Normal;
}

UUSART::UUSART(uint16_t rxBufSize, uint16_t txBufSize, USART_TypeDef* USARTx,
		UIT_Typedef& itUSART, DMA_TypeDef* DMAx,
		DMA_Channel_TypeDef* DMAy_Channelx_Rx,
		DMA_Channel_TypeDef* DMAy_Channelx_Tx, UIT_Typedef& itDMARx,
		UIT_Typedef& itDMATx) :
		UDMAStream(rxBufSize, txBufSize, DMAx, DMAy_Channelx_Rx,
				DMAy_Channelx_Tx, itDMARx, itDMATx, uint32_t(&USARTx->DR),
				uint32_t(&USARTx->DR)) {
	_periph = Periph_USART;

	_USARTx = USARTx;

	_itUSART = itUSART;

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
	ITInit();
	//使能USART(使用库函数兼容性好)
	USART_Cmd(_USARTx, ENABLE);
}

/*
 * author Romeli
 * explain 从流中读取指定数据长度
 * param data 读回数据存放数组
 * param sync 是否是同步读取
 * return Status_Typedef
 */
Status_Typedef UUSART::Read(uint8_t* data, uint16_t len, bool sync,
		UEvent callBackEvent) {
	//循环读取
	for (uint8_t i = 0; i < len; ++i) {
		data[i] = _rxBuf.Data[_rxBuf.Start];
		SpInc();
	}
	return Status_Ok;
}

/*
 * author Romeli
 * explain 向串口里写数组
 * param1 data 数组地址
 * param2 len 数组长度
 * return Status_Typedef
 */
Status_Typedef UUSART::Write(uint8_t* data, uint16_t len, bool sync,
		UEvent callBackEvent) {
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
			while (_DMATxBusy)
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
		return _DMATxBusy;
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
void UUSART::IRQUSART() {
	//读取串口标志寄存器
	uint16_t staus = _USARTx->SR;
	if ((staus & USART_FLAG_IDLE) != RESET) {
		//帧接收标志被触发
		_newFrame = true;
		if (_mode == Mode_DMA) {
			//关闭DMA接收
			_DMAy_Channelx_Rx->CCR &= (uint16_t) (~DMA_CCR1_EN);

			uint16_t len = uint16_t(_rxBuf.Size - _DMAy_Channelx_Rx->CNDTR);
			//清除DMA标志
//			_DMAx->IFCR = DMA1_FLAG_GL3 | DMA1_FLAG_TC3 | DMA1_FLAG_TE3
//					| DMA1_FLAG_HT3;
			//复位DMA接收区大小
			_DMAy_Channelx_Rx->CNDTR = _rxBuf.Size;
			//循环搬运数据

			for (uint16_t i = 0; i < len; ++i) {
				_rxBuf.Data[_rxBuf.End] = _DMARxBuf.Data[i];
				_rxBuf.End = uint16_t((_rxBuf.End + 1) % _rxBuf.Size);
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
		_rxBuf.Data[_rxBuf.End] = uint8_t(_USARTx->DR);
		_rxBuf.End = uint16_t((_rxBuf.End + 1) % _rxBuf.Size);
	}
#endif
	//串口帧错误中断
	if ((staus & USART_FLAG_ORE) != RESET) {
		//清除标志位
		//Note @Romeli 在F0系列中IDLE等中断要手动清除
		USART_ClearITPendingBit(_USARTx, USART_IT_ORE);
	}
}

/*
 * author Romeli
 * explain 串口DMA发送中断
 * return Status_Typedef
 */
void UUSART::IRQDMATx() {
	UDMAStream::IRQDMATx();
	RS485StatusCtl(RS485Dir_Rx);
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
 * explain 初始化DMA设置
 * return void
 */
void UUSART::DMAInit() {
//	DMA_InitTypeDef DMA_InitStructure;
//
//	//开启DMA时钟
//	DMARCCInit(_DMAx);
//
//	DMA_DeInit(_DMAy_Channelx_Rx);
//	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t) (&_USARTx->DR);
//	DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t) _DMARxBuf.Data;
//	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
//	DMA_InitStructure.DMA_BufferSize = _DMARxBuf.Size;
//	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
//	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
//	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
//	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
//	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
//	DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;
//	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
//	DMA_Init(_DMAy_Channelx_Rx, &DMA_InitStructure);
//
//	DMA_DeInit(_DMAy_Channelx_Tx);
//	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t) (&_USARTx->DR);
//	DMA_InitStructure.DMA_MemoryBaseAddr = 0;				//临时设置，无效
//	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
//	DMA_InitStructure.DMA_BufferSize = 0;				//临时设置，无效
//	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
//	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
//	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
//	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
//	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
//	DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;
//	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
//	DMA_Init(_DMAy_Channelx_Tx, &DMA_InitStructure);
	UDMAStream::DMAInit();

	//串口发送接收的DMA功能
	USART_DMACmd(_USARTx, USART_DMAReq_Tx, ENABLE);
	USART_DMACmd(_USARTx, USART_DMAReq_Rx, ENABLE);

	DMA_Cmd(_DMAy_Channelx_Rx, ENABLE);
	//DMA_Cmd(_DMAy_Channelx_Tx, ENABLE); //发送DMA不开启
}

/*
 * author Romeli
 * explain IT初始化
 * return void
 */
void UUSART::ITInit() {
	NVIC_InitTypeDef NVIC_InitStructure;

	NVIC_InitStructure.NVIC_IRQChannel = _itUSART.NVIC_IRQChannel;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority =
			_itUSART.PreemptionPriority;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = _itUSART.SubPriority;
	NVIC_Init(&NVIC_InitStructure);

	if (_mode == Mode_DMA) {
		UDMAStream::ITInit();

		DMA_ITConfig(_DMAy_Channelx_Tx, DMA_IT_TC, ENABLE);
	} else {
		//开启串口的字节接收中断
		USART_ITConfig(_USARTx, USART_IT_RXNE, ENABLE);
	}

	//开启串口的帧接收中断
	USART_ITConfig(_USARTx, USART_IT_IDLE, ENABLE);
}

void UUSART::RS485StatusCtl(RS485Dir_Typedef dir) {
	if (_rs485Status == RS485Status_Enable) {
		switch (dir) {
		case RS485Dir_Rx:
			//ToDo @Romeli 转换太快会丢数，需要注意
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

/*
 * author Romeli
 * explain 向下移动流指针
 * return void
 */
void UUSART::SpInc() {
	if ((_rxBuf.Size != 0) && (_rxBuf.Start != _rxBuf.End)) {
		//缓冲区指针+1
		_rxBuf.Start = uint16_t((_rxBuf.Start + 1) % _rxBuf.Size);
	}
}
