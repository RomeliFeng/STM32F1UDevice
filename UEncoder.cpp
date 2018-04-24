/*
 * UEncoder.cpp
 *
 *  Created on: 2017年11月1日
 *      Author: Romeli
 */

#include <UEncoder.h>
#include <Misc/UDebug.h>

UEncoder* UEncoder::_pool[4];
uint8_t UEncoder::_poolSp = 0;

UEncoder::UEncoder(TIM_TypeDef* TIMx, UIT_Typedef& it) {
	_TIMx = TIMx;
	_it = it;

	_exCNT = 0;
	_relativeDir = Dir_Positive;
	_pos = 0;

	_sync = false;
	_flow = Flow_CC1;

	//自动将对象指针加入资源池
	_pool[_poolSp++] = this;
}

UEncoder::~UEncoder() {
}

/*
 * author Romeli
 * param void
 * return void
 */
void UEncoder::Init() {
	GPIOInit();
	TIMInit();
	ITInit();
	//顺序很重要
	TIM_Enable(_TIMx);
}

/*
 * author Romeli
 * explain 初始化所有编码器模块
 * return void
 */
void UEncoder::InitAll() {
	//初始化池内所有单元
	for (uint8_t i = 0; i < _poolSp; ++i) {
		_pool[i]->Init();
	}
	if (_poolSp == 0) {
		//Error @Romeli 无编码器模块
		UDebugOut("There have encoder module exsit");
	}
}

/*
 * author Romeli
 * explain 设置编码器方向，可以把脉冲反向
 * param dir 编码器方向
 * return void
 */
void UEncoder::SetRelativeDir(Dir_Typedef dir) {
	_relativeDir = dir;
}

/*
 * author Romeli
 * explain 编码器位置清零
 * return void
 */
void UEncoder::ClearPos() {
	_exCNT = 0;
	_TIMx->CNT = 0;
	_flow = Flow_CC1;
}

/*
 * author Romeli
 * explain 读取编码器当前位置 非中断安全，需要防止被中断嵌套
 * return int32_t 当前位置
 */
int32_t UEncoder::GetPos() {
//	int32_t pos;
//	int32_t pos1 = int32_t(_exCNT) * 0x10000 + _TIMx->CNT;
//	int32_t pos2 = int32_t(_exCNT) * 0x10000 + _TIMx->CNT;
//	int32_t pos3 = int32_t(_exCNT) * 0x10000 + _TIMx->CNT;
//
//	if (abs(pos1 - pos2) < 0x1000) {
//		//pos3错误
//		pos = pos2;
//	} else if (abs(pos2 - pos3) < 0x1000) {
//		//pos1错误
//		pos = pos3;
//	} else if (abs(pos3 - pos1) < 0x1000) {
//		//pos2错误
//		pos = pos3;
//	} else {
//		pos = pos1;
//	}
//	while (true) {
//		_sync = false;
//		pos = int32_t(_exCNT) * 0x10000 + _TIMx->CNT;
//		if (!_sync) {
//			//取数中没有进入中断
//			if (!TIM_Get_IT_Update(_TIMx)) {
//				//中断标志没有置位，数据安全
//				break;
//			} else {
//				//有中断正在等待，执行中断函数并开始下一次取数
//				IRQ();
//			}
//		}
//		//取数中进入了中断，重新取数
//	}

	int32_t CNT;
	int32_t pos;
	Flow_Typedef flow;
	int32_t off = 0;

	//安全的取数
	while (true) {
		_sync = false;

		CNT = _TIMx->CNT;
		pos = _exCNT;
		flow = _flow;

		if (!_sync) {
			//取数中没有进入中断
			break;
		}
	}

	//补偿加减位置偏移造成的影响
	switch (flow) {
	case Flow_CC1:
		if (CNT > 0xf000) {
			off -= 0x10000;
		}
		break;
	case Flow_CC4:
		if (CNT < 0x1000) {
			off += 0x10000;
		}
		break;
	default:
		break;
	}

	//计算位置
	pos = pos + CNT + off;
	_pos = _relativeDir == Dir_Negtive ? -pos : pos;
	return _pos;
}

/*
 * author Romeli
 * explain 中断处理函数(中断安全？)
 * return void
 */
void UEncoder::IRQ() {
	_sync = true;
	uint16_t status = _TIMx->SR & (TIM_IT_CCx);
	int32_t exCNT = _exCNT;
	if (status != 0) {
		uint16_t flag = 0;
		if ((status & TIM_IT_CC1) != 0) {
			if (_flow == Flow_CC4) {
				exCNT = _exCNT + 0x10000;
				_exCNT = exCNT;
			}
			_flow = Flow_CC1;
			flag |= TIM_IT_CC1;
		}
		if ((status & TIM_IT_CC2) != 0) {
			_flow = Flow_CC2;
			flag |= TIM_IT_CC2;
		}
		if ((status & TIM_IT_CC3) != 0) {
			_flow = Flow_CC3;
			flag |= TIM_IT_CC3;
		}
		if ((status & TIM_IT_CC4) != 0) {
			if (_flow == Flow_CC1) {
				exCNT = _exCNT - 0x10000;
				_exCNT = exCNT;
			}
			_flow = Flow_CC4;
			flag |= TIM_IT_CC3;
		}
		_TIMx->SR = uint16_t(~(TIM_IT_CCx));
	}
}

void UEncoder::GPIOInit() {
	UDebugOut("This function should be override");
	/*	GPIO_InitTypeDef GPIO_InitStructure;

	 RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

	 GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_9;
	 GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	 GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	 GPIO_Init(GPIOA, &GPIO_InitStructure);*/
}

/*
 * author Romeli
 * explain 初始化定时器为编码器模式，4分频采样
 * return void
 */
void UEncoder::TIMInit() {
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;

	TIMRCCInit();

	TIM_DeInit(_TIMx);

	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseInitStructure.TIM_Prescaler = 0;
	TIM_TimeBaseInitStructure.TIM_Period = 0xffff;
	TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(_TIMx, &TIM_TimeBaseInitStructure);

	TIM_EncoderInterfaceConfig(_TIMx, TIM_EncoderMode_TI12,
	TIM_ICPolarity_Falling, TIM_ICPolarity_Falling);

	TIM_ICInitTypeDef TIM_ICInitStructure;
	TIM_ICStructInit(&TIM_ICInitStructure);
	TIM_ICInitStructure.TIM_ICFilter = 1;
	TIM_ICInit(_TIMx, &TIM_ICInitStructure);
	_TIMx->CNT = 0;

	TIM_OCInitTypeDef TIM_OCInitStructure;
	TIM_OCStructInit(&TIM_OCInitStructure);
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_Timing;
//正转+1信号
	TIM_OCInitStructure.TIM_Pulse = 0x1000;
	TIM_OC1Init(_TIMx, &TIM_OCInitStructure);

//方向检测信号，靠近正转
	TIM_OCInitStructure.TIM_Pulse = 0x3000;
	TIM_OC2Init(_TIMx, &TIM_OCInitStructure);

//方向检测信号，靠近反转
	TIM_OCInitStructure.TIM_Pulse = 0xd000;
	TIM_OC3Init(_TIMx, &TIM_OCInitStructure);

//反转-1信号
	TIM_OCInitStructure.TIM_Pulse = 0xf000;
	TIM_OC4Init(_TIMx, &TIM_OCInitStructure);
}

/*
 * author Romeli
 * explain 初始化定时器溢出中断
 * return void
 */
void UEncoder::ITInit() {
	NVIC_InitTypeDef NVIC_InitStructure;

	NVIC_InitStructure.NVIC_IRQChannel = _it.NVIC_IRQChannel;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority =
			_it.PreemptionPriority;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = _it.SubPriority;
	NVIC_Init(&NVIC_InitStructure);

	TIM_Clear_CCx_Flag(_TIMx);
	TIM_Enable_IT_CCx(_TIMx);
}

