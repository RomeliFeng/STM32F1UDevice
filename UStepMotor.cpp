/*
 * UStepMotor.cpp
 *
 *  Created on: 2017年11月6日
 *      Author: Romeli
 */

#include <UStepMotor.h>
#include <UStepMotorAccDecUnit.h>
#include <Misc/UDebug.h>

UStepMotor* UStepMotor::_Pool[4];
uint8_t UStepMotor::_PoolSp = 0;

UStepMotor::UStepMotor(TIM_TypeDef* TIMx, uint8_t TIMx_CCR_Ch,
		UIT_Typedef& it) {
	_TIMx = TIMx;
	_TIMx_CCR_Ch = TIMx_CCR_Ch;
	_IT = it;

	switch (_TIMx_CCR_Ch) {
	case 1:
		_TIMx_CCRx = &_TIMx->CCR1;
		break;
	case 2:
		_TIMx_CCRx = &_TIMx->CCR2;
		break;
	case 3:
		_TIMx_CCRx = &_TIMx->CCR3;
		break;
	case 4:
		_TIMx_CCRx = &_TIMx->CCR3;
		break;
	default:
		//Error @Romeli 错误的脉冲输出窗口
		UDebugOut("TIM CCR Ch Error");
		break;
	}
	//自动将对象指针加入资源池
	_Pool[_PoolSp++] = this;

	_TIMy_FRQ = 0;

	_AccDecUnit = 0;	//速度计算单元

	_CurStep = 0;	//当前已移动步数
	_TgtStep = 0;	//目标步数
	_DecelStartStep = 0;	//减速开始步数
	_StepEncoder = 0;

	_Accel = 20000;	//加速度
	_Decel = 20000;	//减速度
	_MaxSpeed = 10000;	//最大速度

	_Limit_CW = 0; //正转保护限位
	_Limit_CCW = 0; //反转保护限位

	_RelativeDir = Dir_CW; //实际方向对应
	_CurDir = Dir_CW; //当前方向

	_Flow = Flow_Stop;
	_StepLimitAction = true;
	_Busy = false;	//当前电机繁忙?
}

UStepMotor::~UStepMotor() {
}

/*
 * author Romeli
 * explain 初始化当前步进电机模块
 * return void
 */
void UStepMotor::Init() {
	GPIOInit();
	TIMInit();
	ITInit();
	_TIMy_FRQ = SystemCoreClock / (_TIMx->PSC + 1);
	Lock();
}

/*
 * author Romeli
 * explain 初始化所有步进电机模块
 * return void
 */
void UStepMotor::InitAll() {
	//初始化池内所有运动模块
	for (uint8_t i = 0; i < _PoolSp; ++i) {
		_Pool[i]->Init();
	}
	if (_PoolSp == 0) {
		//Error @Romeli 无运动模块（无法进行运动）
		UDebugOut("There have no speed control unit exsit");
	}
	if (GetTheLowestPreemptionPriority()
			<= UStepMotorAccDecUnit::GetTheLowestPreemptionPriority()) {
		//Error @Romeli 存在速度计算单元的抢占优先级低于或等于步进电机单元的抢占优先级的情况，需要避免
		UDebugOut("The preemption priority setting error");
	}
	//初始化所有的速度计算单元
	UStepMotorAccDecUnit::InitAll();
}

/*
 * author Romeli
 * explain 获取步进电机模块中权限最低的抢占优先级
 * return uint8_t
 */
uint8_t UStepMotor::GetTheLowestPreemptionPriority() {
	uint8_t preemptionPriority = 0;
	for (uint8_t i = 0; i < _PoolSp; ++i) {
		if (_Pool[i]->_IT.PreemptionPriority > preemptionPriority) {
			preemptionPriority = _Pool[i]->_IT.PreemptionPriority;
		}
	}
	return preemptionPriority;
}

/*
 * author Romeli
 * explain 设置最大速度和加速度
 * param1 maxSpeed 最大脉冲频率
 * param2 accel 加速度
 * return void
 */
void UStepMotor::SetSpeed(uint16_t maxSpeed, uint32_t accel) {
	maxSpeed = maxSpeed < 150 ? uint16_t(150) : maxSpeed;
	accel = accel < 1500 ? 0 : accel;
	_MaxSpeed = maxSpeed;
	_Accel = accel;
	_Decel = accel;
}

/*
 * author Romeli
 * explain 设置参考方向
 * param dir 新的参考方向
 * return void
 */
void UStepMotor::SetRelativeDir(Dir_Typedef dir) {
	_RelativeDir = dir;
}

/*
 * author Romeli
 * explain 设置步进电机保护限位（正转）
 * param cwLimit
 * return void
 */
void UStepMotor::SetLimit_CW(uint8_t limit_CW) {
	_Limit_CW = limit_CW;
}

/*
 * author Romeli
 * explain 设置步进电机保护限位（反转）
 * param cwLimit
 * return void
 */
void UStepMotor::SetLimit_CCW(uint8_t limit_CCW) {
	_Limit_CCW = limit_CCW;
}

/*
 * author Romeli
 * explain 设置步进电机保护限位
 * param cwLimit 正转限位
 * param ccwLimit 反转限位
 * return void
 */
void UStepMotor::SetLimit(uint8_t limit_CW, uint8_t limit_CCW) {
	SetLimit_CW(limit_CW);
	SetLimit_CCW(limit_CCW);
}

/*
 * author Romeli
 * explain 设置步进电机保护限位
 * param dir 方向
 * param limit 限位
 * return void
 */
void UStepMotor::SetLimit(Dir_Typedef dir, uint8_t limit) {
	if (dir == Dir_CW) {
		_Limit_CW = limit;
	} else {
		_Limit_CCW = limit;
	}
}

/*
 * author Romeli
 * explain 移动步进电机
 * param step 欲运动的步数（为0时不会主动停止）
 * param dir 运动方向
 * param sync 是否等待运动结束
 * return void
 */
Status_Typedef UStepMotor::Move(uint32_t step, Dir_Typedef dir, bool sync) {
	//停止如果有的运动
	Stop();
	//设置方向
	SetDir(dir);
	//检测保护限位
	if (!SafetyProtect()) {
		//限位保护触发
		return Status_Error;
	}
	//锁定当前运动
	_Busy = true;
	//使能电机
	Lock();
	//获取可用的速度计算单元
	_AccDecUnit = UStepMotorAccDecUnit::GetFreeUnit(this);
	if (_AccDecUnit == 0) {
		//没有可用的速度计算单元，放弃本次运动任务
		UDebugOut("Get speed control unit fail,stop move");
		_Busy = false;
		return Status_Error;
	}
	//清零当前计数
	_CurStep = 0;
	if (step != 0) {
		//基于步数运动
		_StepLimitAction = true;
		_TgtStep = step;

		if ((_Decel != 0) && (_TgtStep >= 1)) {
			//当减速存在并且处于步数控制的运动流程中
			//从最高速减速
			uint32_t tmpStep = GetDecelStep(_MaxSpeed);
			//无法到达最高速度
			uint32_t tmpStep2 = _TgtStep >> 1;
			//当无法到达最高速度时，半步开始减速
			_DecelStartStep = (
					tmpStep >= tmpStep2 ? tmpStep2 : _TgtStep - tmpStep) - 1;
		} else {
			//减速度为0 或者目标步数为1
			_DecelStartStep = 0;
		}
	} else {
		//持续运动，
		_StepLimitAction = false;
		_TgtStep = 0;
		_DecelStartStep = 0;
	}

	if (_Accel != 0) {
		//切换步进电机状态为加速
		_Flow = Flow_Accel;
		_AccDecUnit->SetMode(UStepMotorAccDecUnit::Mode_Accel);
		//开始加速 目标速度为最大速度
		_AccDecUnit->Start(UStepMotorAccDecUnit::Mode_Accel);
		//Warnning 需确保当前流程为加速
		SetSpeed(_AccDecUnit->GetCurSpeed());
	} else {
		_Flow = Flow_Run;
		SetSpeed(_MaxSpeed);
	}

	TIM_PSC_Reload(_TIMx);
	//开始输出脉冲
	TIM_Clear_Update_Flag(_TIMx);
	TIM_Enable_IT_Update(_TIMx);
	TIM_Enable(_TIMx);

	if (sync) {
		while (_Busy)
			;
	}
	return Status_Ok;
}

/*
 * author Romeli
 * explain 运动步进电机
 * param step 欲移动的步数(+为正传，-为反转，0不转)
 * param sync 是否等待运动结束
 * return void
 */
Status_Typedef UStepMotor::Move(int32_t step, bool sync) {
	if (step > 0) {
		return Move(step, Dir_CW, sync);
	} else if (step < 0) {
		return Move(-step, Dir_CCW, sync);
	} else {
		return Status_Ok;
	}
}

/*
 * author Romeli
 * explain 持续运动步进电机
 * param dir 电机转动方向
 * return void
 */
Status_Typedef UStepMotor::Run(Dir_Typedef dir) {
	return Move(0, dir);
}

/*
 * author Romeli
 * explain 立即停止步进电机
 * return void
 */
void UStepMotor::Stop() {
	//关闭脉冲发生
	TIM_Disable(_TIMx);
	TIM_Disable_IT_Update(_TIMx);
	//尝试释放当前运动模块占用的单元
	UStepMotorAccDecUnit::Free(this);
	//清空忙标志
	_Busy = false;
	switch (_CurDir) {
	case Dir_CW:
		_StepEncoder += _CurStep;
		break;
	case Dir_CCW:
		_StepEncoder -= _CurStep;
		break;
	default:
		break;
	}
	_CurStep = 0;
}

/*
 * author Romeli
 * explain 缓慢步进电机
 * return void
 */
void UStepMotor::StopSlow() {
	//根据当前步数和从当前速度减速所需步数计算目标步数
	_TgtStep = GetDecelStep(_MaxSpeed) + _CurStep;
	//变更模式为步数限制
	_StepLimitAction = true;
	StartDec();
}

/*
 * author Romeli
 * explain 锁定步进电机（使能）
 * return void
 */
void UStepMotor::Lock() {
	SetEnPin(ENABLE);
}

/*
 * author Romeli
 * explain 解锁步进电机（禁用）
 * return void
 */
void UStepMotor::Unlock() {
	SetEnPin(DISABLE);
}

/*
 * author Romeli
 * explain	检测是否可以安全移动（需要在派生类中重写）
 * return bool 是否安全可移动
 */
bool UStepMotor::SafetyProtect() {
	bool status = true;
	if (_Busy) {
		switch (_CurDir) {
		case Dir_CW:
			if (GetLimit_CW()) {
				status = false;
				Stop();
			}
			break;
		case Dir_CCW:
			if (GetLimit_CCW()) {
				status = false;
				Stop();
			}
			break;
		default:
			break;
		}
	}
	return status;
}

/*
 * author Romeli
 * explain 脉冲发生计数用中断服务子函数
 * return void
 */
void UStepMotor::IRQ() {
	_CurStep++;

	//处于步数限制运动中 并且 到达指定步数，停止运动
	if (_StepLimitAction && (_CurStep == _TgtStep)) {
		//当处于步数运动并且到达指定步数时，停止
		_Flow = Flow_Stop;
	}

	switch (_Flow) {
	case Flow_Accel:
		if (_StepLimitAction) {
			//步数限制运动模式
			if (_CurStep >= _DecelStartStep) {
				//到达减速步数，进入减速流程
				StartDec();
				_Flow = Flow_Decel;
			}
		}
		if (_AccDecUnit->IsDone()) {
			//到达最高步数，开始匀速流程
			_Flow = Flow_Run;
		}
		SetSpeed(_AccDecUnit->GetCurSpeed());
		break;
	case Flow_Run:
		if (_Decel != 0) {
			if (_StepLimitAction && (_CurStep >= _DecelStartStep)) {
				//到达减速步数，进入减速流程
				StartDec();
				_Flow = Flow_Decel;
			}
		}
		break;
	case Flow_Decel:
		SetSpeed(_AccDecUnit->GetCurSpeed());
		break;
	case Flow_Stop:
		Stop();
		break;
	default:
		//Error @Romeli 不可能到达位置（内存溢出）
		UDebugOut("Unkown error");
		break;
	}
	TIM_Clear_Update_Flag(_TIMx);
}

/*
 * author Romeli
 * explain GPIO初始化（应在派生类中重写）
 * return void
 */
void UStepMotor::GPIOInit() {
	UDebugOut("This function should be override");
	/*	GPIO_InitTypeDef GPIO_InitStructure;

	 RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

	 //Init pin PUL on TIM2 CH1
	 GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
	 GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	 GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	 GPIO_Init(GPIOC, &GPIO_InitStructure);

	 //Init pin DIR&EN on TIM2 CH2&3
	 GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9;
	 GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	 GPIO_Init(GPIOC, &GPIO_InitStructure);

	 GPIOC->BRR = GPIO_Pin_8 | GPIO_Pin_9;*/
}

/*
 * author Romeli
 * explain 设置方向引脚状态（应在派生类中重写）
 * return void
 */
void UStepMotor::SetDirPin(FunctionalState newState) {
	if (newState != DISABLE) {

	} else {

	}
}

/*
 * author Romeli
 * explain 设置使能引脚状态（应在派生类中重写）
 * return void
 */
void UStepMotor::SetEnPin(FunctionalState newState) {
	if (newState != DISABLE) {

	} else {

	}
}

/*
 * author Romeli
 * explain 获取正转限位传感器状态（建议在派生类中重写）
 * return uint32_t
 */
bool UStepMotor::GetLimit_CW() {
	//ToDo 不应该这么写，应删除限位传感器uint32_t 改为传感器位号？？？细化为检测正传传感器 和 反转传感器触发信号
	return false;
}

/*
 * author Romeli
 * explain 获取反转限位传感器状态（建议在派生类中重写）
 * return uint32_t
 */
bool UStepMotor::GetLimit_CCW() {
	//ToDo 不应该这么写，应删除限位传感器uint32_t 改为传感器位号？？？细化为检测正传传感器 和 反转传感器触发信号
	return false;
}

/*
 * author Romeli
 * explain TIM初始化（应在派生类中重写）
 * return void
 */
void UStepMotor::TIMInit() {
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;

	TIMRCCInit();

	TIM_DeInit(_TIMx);

	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInitStructure.TIM_Prescaler = 7;	//若需更改最小脉冲频率
	TIM_TimeBaseInitStructure.TIM_Period = (uint16_t) ((SystemCoreClock
			/ STEP_MOTOR_MIN_SPEED) >> 3);
	TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(_TIMx, &TIM_TimeBaseInitStructure);

	//!!!!非常重要，启动ARR预装载寄存器
	TIM_ARRPreloadConfig(_TIMx, DISABLE);

	TIM_OCInitTypeDef TIM_OCInitStructure;

	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1; //PWM模式1
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable; //主输出使能
	TIM_OCInitStructure.TIM_OutputNState = TIM_OutputNState_Disable; //关闭副输出
	TIM_OCInitStructure.TIM_Pulse = (uint16_t) (_TIMx->ARR >> 1); //脉冲宽度设置为50%
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCNPolarity_High; //主输出有效电平为高电平
	TIM_OCInitStructure.TIM_OCNPolarity = TIM_OCNPolarity_High; //副输出有效电平高低电平
	TIM_OCInitStructure.TIM_OCIdleState = TIM_OCNIdleState_Reset; //主输空闲时为无效电平
	TIM_OCInitStructure.TIM_OCNIdleState = TIM_OCNIdleState_Reset; //副输空闲时为无效电平

	switch (_TIMx_CCR_Ch) {
	case 1:
		TIM_OC1Init(_TIMx, &TIM_OCInitStructure);
		//!!!!非常重要，启动CCRx预装载寄存器
		TIM_OC1PreloadConfig(_TIMx, TIM_OCPreload_Disable);
		break;
	case 2:
		TIM_OC2Init(_TIMx, &TIM_OCInitStructure);
		//!!!!非常重要，启动CCRx预装载寄存器
		TIM_OC2PreloadConfig(_TIMx, TIM_OCPreload_Disable);
		break;
	case 3:
		TIM_OC3Init(_TIMx, &TIM_OCInitStructure);
		//!!!!非常重要，启动CCRx预装载寄存器
		TIM_OC3PreloadConfig(_TIMx, TIM_OCPreload_Disable);
		break;
	case 4:
		TIM_OC4Init(_TIMx, &TIM_OCInitStructure);
		//!!!!非常重要，启动CCRx预装载寄存器
		TIM_OC4PreloadConfig(_TIMx, TIM_OCPreload_Disable);
		break;
	default:
		//Error @Romeli 错误的脉冲输出窗口
		UDebugOut("TIM CCR Ch Error");
		break;
	}

	if (IS_TIM_LIST2_PERIPH(_TIMx)) {
		//作为高级定时器必须要开启PWM输出
		TIM_CtrlPWMOutputs(_TIMx, ENABLE);
	}
}

/*
 * author Romeli
 * explain IT初始化
 * return void
 */
void UStepMotor::ITInit() {
	NVIC_InitTypeDef NVIC_InitStructure;
	//设置中断

	NVIC_InitStructure.NVIC_IRQChannel = _IT.NVIC_IRQChannel;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority =
			_IT.PreemptionPriority;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = _IT.SubPriority;
	NVIC_Init(&NVIC_InitStructure);
}

/*
 * author Romeli
 * explain 更改当前步进电机方向
 * param dir 欲改变的方向
 * return void
 */
void UStepMotor::SetDir(Dir_Typedef dir) {
	_CurDir = dir;
	if (_CurDir == _RelativeDir) {
		SetDirPin(ENABLE);
	} else {
		SetDirPin(DISABLE);
	}
}

/*
 * author Romeli
 * explain 开始减速
 * return void
 */
void UStepMotor::StartDec() {	//关闭速度计算单元
//开始减速计算
	_AccDecUnit->Start(UStepMotorAccDecUnit::Mode_Decel);
}

/*
 * author Romeli
 * explain 计算当前速度减速到怠速需要多少个脉冲
 * param 当前速度
 * return uint32_t
 */
uint32_t UStepMotor::GetDecelStep(uint16_t speedFrom) {
	float n = (float) (speedFrom - STEP_MOTOR_MIN_SPEED) / (float) _Decel;
	return (uint32_t) ((float) (speedFrom + STEP_MOTOR_MIN_SPEED) * n) >> 1;
}

/*
 * author Romeli
 * explain 应用速度到脉冲定时器（不会立即更新）
 * param 速度
 * return void
 */
void UStepMotor::SetSpeed(uint16_t speed) {
	if (speed <= 150) {
		//Error @Romeli 过低的速度，不应该发生
		UDebugOut("Under speed show be disappear");
	}
	_TIMx->ARR = (uint16_t) (_TIMy_FRQ / speed);
	*_TIMx_CCRx = (uint16_t) (_TIMx->ARR >> 1);
}
