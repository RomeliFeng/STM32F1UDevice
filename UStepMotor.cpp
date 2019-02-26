/*
 * UStepMotor.cpp
 *
 *  Created on: 2017年11月6日
 *      Author: Romeli
 */

#include <UStepMotor.h>
#include <Misc/UDebug.h>
#include <math.h>
#include <UStepMotorAccUnit.h>
#include <Tool/UTick.h>

UStepMotor* UStepMotor::_pool[4];
uint8_t UStepMotor::_poolSp = 0;

UStepMotor::UStepMotor(TIM_TypeDef* TIMx, uint8_t TIMx_CCR_Ch,
		uint32_t TIMx_FRQ, UIT_Typedef& UIT_TIM_Update) :
		_TIMx(TIMx), _TIMx_CCR_Ch(TIMx_CCR_Ch), _TIMx_FRQ(TIMx_FRQ), _UIT_TIM_Update(
				UIT_TIM_Update) {
	ParamInit();

	_accMode = AccMode_AccUnit;
	//初始化起步速度
	StartSpeedInit();
	//设置中断服务函数为加速度单元用中断服务函数
	_IRQTaget = &UStepMotor::IRQ_AccUnit;
}

UStepMotor::UStepMotor(TIM_TypeDef* TIMx, uint8_t TIMx_CCR_Ch,
		uint32_t TIMx_FRQ, UIT_Typedef& UIT_TIM_Update, StepMap* stepMap,
		uint16_t stepMapSize) :
		_TIMx(TIMx), _TIMx_CCR_Ch(TIMx_CCR_Ch), _TIMx_FRQ(TIMx_FRQ), _UIT_TIM_Update(
				UIT_TIM_Update), _stepMap(stepMap), _stepMapSize(stepMapSize) {
	ParamInit();

	if (uint64_t(_maxSpeedLimit * stepMapSize) > 0xffffffff) {
		//Error @Romeli 32bit不足以支撑当前精度的加减速
		UDebugOut("Acc precision is too high");
	}

	_accMode = AccMode_StepMap;
	//初始化加速表
	StepMapInit();
	//设置中断服务函数为阶梯加速用中断服务函数
	_IRQTaget = &UStepMotor::IRQ_StepMap;
}

UStepMotor::~UStepMotor() {
}

/*
 * author Romeli
 * explain 初始化当前步进电机模块
 * return void
 */
void UStepMotor::Init() {
	BeforeInit();
	TIMInit();
	ITInit();
	SetDir(UStepMotor::Dir_CW);
	Lock();
}

/*
 * author Romeli
 * explain 初始化所有步进电机模块
 * return void
 */
void UStepMotor::InitAll() {
	//初始化池内所有运动模块
	for (uint8_t i = 0; i < _poolSp; ++i) {
		_pool[i]->Init();
	}
	if (_poolSp == 0) {
		//Error @Romeli 无运动模块（无法进行运动）
		UDebugOut("There have no speed control unit exsit");
	}
	if (GetTheLowestPreemptionPriority()
			<= UStepMotorAccUnit::GetTheLowestPreemptionPriority()) {
		//Error @Romeli 存在速度计算单元的抢占优先级低于或等于步进电机单元的抢占优先级的情况，需要避免
		UDebugOut("The preemption priority setting error");
	}
	//初始化所有的速度计算单元
	UStepMotorAccUnit::InitAll();
}

/*
 * author Romeli
 * explain 停止所有步进电机模块
 * return void
 */
void UStepMotor::StopAll() {
	for (uint8_t i = 0; i < _poolSp; ++i) {
		_pool[i]->Stop();
	}
}

/*
 * author Romeli
 * explain 解锁所有步进电机模块
 * return void
 */
void UStepMotor::UnLockAll() {
	for (uint8_t i = 0; i < _poolSp; ++i) {
		_pool[i]->Unlock();
	}
}

/*
 * author Romeli
 * explain 获取步进电机模块中权限最低的抢占优先级
 * return uint8_t
 */
uint8_t UStepMotor::GetTheLowestPreemptionPriority() {
	uint8_t preemptionPriority = 0;
	for (uint8_t i = 0; i < _poolSp; ++i) {
		if (_pool[i]->_UIT_TIM_Update.PreemptionPriority > preemptionPriority) {
			preemptionPriority = _pool[i]->_UIT_TIM_Update.PreemptionPriority;
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
void UStepMotor::SetSpeed(uint32_t maxSpeed, uint32_t accel) {
	if (_accMode == AccMode_AccUnit) {
		maxSpeed = maxSpeed > 0xffff ? 0xffff : maxSpeed;
		accel = accel < MINACCDECSPEED ? 0 : accel;
	}
	if (maxSpeed > _maxSpeedLimit) {
		maxSpeed = _maxSpeedLimit;
	}
	_maxSpeed = maxSpeed;
	_accel = accel;
	_decel = accel;
	switch (_accMode) {
	case AccMode_AccUnit:
		StartSpeedInit();
		break;
	case AccMode_StepMap:
		StepMapInit();
		break;
	default:
		break;
	}
}

/*
 * author Romeli
 * explain 设置参考方向
 * param dir 新的参考方向
 * return void
 */
void UStepMotor::SetRelativeDir(Dir_Typedef dir) {
	_relativeDir = dir;
}

/*
 * author Romeli
 * explain 设置步进电机保护限位（正转）
 * param cwLimit
 * return void
 */
void UStepMotor::SetLimit_CW(uint8_t limit_CW) {
	_limit_CW = limit_CW;
}

/*
 * author Romeli
 * explain 设置步进电机保护限位（反转）
 * param cwLimit
 * return void
 */
void UStepMotor::SetLimit_CCW(uint8_t limit_CCW) {
	_limit_CCW = limit_CCW;
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
		_limit_CW = limit;
	} else {
		_limit_CCW = limit;
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
	//锁定当前运动
	_busy = true;
	//设置方向
	SetDir(dir);
	//检测保护限位
	if (!SafetyProtect()) {
		//限位保护触发
		return Status_Error;
	}
	//使能电机
	Lock();
	//清零当前计数
	_curStep = 0;
	if (step != 0) {
		//基于步数运动
		_stepLimitAction = true;
		_tgtStep = step;

		if ((_decel != 0) && (_tgtStep >= 1)) {
			//当减速存在并且处于步数控制的运动流程中
			//从最高速减速
			uint32_t tmpStep = GetDecelStep(_maxSpeed);
			//无法到达最高速度
			uint32_t tmpStep2 = _tgtStep >> 1;
			//当无法到达最高速度时，半步开始减速
			_decelStartStep = (
					tmpStep >= tmpStep2 ? tmpStep2 : _tgtStep - tmpStep) - 1;
		} else {
			//减速度为0 或者目标步数为1
			_decelStartStep = 0;
		}
	} else {
		//持续运动，
		_stepLimitAction = false;
		_tgtStep = 0;
		_decelStartStep = 0;
	}

	if (_accel != 0) {
		//切换步进电机状态为加速
		_flow = Flow_Accel;
		switch (_accMode) {
		case AccMode_AccUnit:
			//使用加速单元进行加减速
			//获取可用的速度计算单元
			_accUnit = UStepMotorAccUnit::GetFreeUnit(this);
			if (_accUnit == 0) {
				//没有可用的速度计算单元，放弃本次运动任务
				UDebugOut("Get speed control unit fail,stop move");
				_busy = false;
				return Status_Error;
			}
			//开始加速 目标速度为最大速度
			_accUnit->Start(UStepMotorAccUnit::Mode_Accel);
			//Warnning 需确保当前流程为加速
			SetSpeed(_accUnit->GetCurSpeed());
			break;
		case AccMode_StepMap:
			_stepMapIndex = 0;
			_TIMx->PSC = _stepMap[_stepMapIndex].PSCTab;
			_TIMx->ARR = _stepMap[_stepMapIndex].ARRTab;
			*_TIMx_CCRx = uint16_t(_TIMx->ARR >> 1);
			++_stepMapIndex;
			if (_stepMapIndex >= _stepMapSteps) {
				//一步登天，直接进入加速模式
				_flow = Flow_Run;
			}
			break;
		default:
			break;
		}
	} else {
		_flow = Flow_Run;
		SetSpeed(_maxSpeed);
	}

	TIM_PSC_Reload(_TIMx);
	TIM_Clear_Update_Flag(_TIMx);
	TIM_Enable_IT_Update(_TIMx);
	//开始输出脉冲
	TIM_Enable(_TIMx);

	if (sync) {
		while (_busy)
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

Status_Typedef UStepMotor::Run(int32_t speed) {
	if (speed == 0) {
		Stop();
		return Status_Ok;
	} else {
		if (speed < 0) {
			SetSpeed(-speed, 0);
			return Run(Dir_CCW);
		} else {
			SetSpeed(speed, 0);
			return Run(Dir_CW);
		}
	}
}

/*
 * author Romeli
 * explain 停止步进电机（在流程中正常停止）
 * return void
 */
void UStepMotor::Stop() {
	_flow = Flow_Stop;
	//等待1s停止
	if (!uTick.WaitOne([&] {
		return !_busy;
	}, 1000000)) {
		StopForce();
		UDebugOut("StepMotor stop fail!");
	}
}

/*
 * author Romeli
 * explain 停止步进电机，含加减速
 * return void
 */
void UStepMotor::StopSlow() {
	uint16_t index;
	switch (_accMode) {
	case AccMode_AccUnit:
		//根据当前步数和从当前速度减速所需步数计算目标步数
		_tgtStep = GetDecelStep(_accUnit->GetCurSpeed()) + _curStep;
		//变更模式为步数限制
		_stepLimitAction = true;
		_accUnit->Start(UStepMotorAccUnit::Mode_Decel);
		break;
	case AccMode_StepMap:
		index = _stepMapIndex != 0 ? _stepMapIndex - 1 : 0;
		while (index != 0) {
			if (_stepMap[index].StepTab != _stepMap[index - 1].StepTab) {
				break;
			}
			--index;
		}
		_stepMapIndex = index;

		_tgtStep = _stepMap[_stepMapIndex].StepTab + _curStep;
		_TIMx->PSC = _stepMap[_stepMapIndex].PSCTab;
		_TIMx->ARR = _stepMap[_stepMapIndex].ARRTab;
		*_TIMx_CCRx = uint16_t(_TIMx->ARR >> 1);
		TIM_PSC_Reload(_TIMx);

		_flow = Flow_Decel;
		//变更模式为步数限制
		_stepLimitAction = true;
		break;
	default:
		break;
	}

}

/*
 * author Romeli
 * explain 停止步进电机，强制（不在停止流程中调用可能失步）
 * return void
 */
void UStepMotor::StopForce() {
	//直接关闭脉冲发生，会产生丢步
	TIM_Disable(_TIMx);
	TIM_Clear_Update_Flag(_TIMx);
	TIM_Disable_IT_Update(_TIMx);
	if (_accMode == AccMode_AccUnit) {
		//尝试释放当前运动模块占用的单元
		UStepMotorAccUnit::Free(this);
	}
	//清空忙标志
	_busy = false;
	_curStep = 0;
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
	if (_busy) {
		switch (_curDir) {
		case Dir_CW:
			if (GetLimit_CW()) {
				_emo = true;
				status = false;
				Stop();
			}
			break;
		case Dir_CCW:
			if (GetLimit_CCW()) {
				_emo = true;
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
void UStepMotor::IRQ_AccUnit() {
	_curStep++;
	if (_curDir == Dir_CW) {
		++_stepEncoder;
	} else {
		--_stepEncoder;
	}

	//处于步数限制运动中 并且 到达指定步数，停止运动
	if (_stepLimitAction && (_curStep == _tgtStep)) {
		//当处于步数运动并且到达指定步数时，停止
		_flow = Flow_Stop;
	}

	switch (_flow) {
	case Flow_Accel:
		if (_stepLimitAction) {
			//步数限制运动模式
			if (_curStep >= _decelStartStep) {
				//到达减速步数，进入减速流程
				_accUnit->Start(UStepMotorAccUnit::Mode_Decel);
				_flow = Flow_Decel;
			}
		}
		if (_accUnit->IsDone()) {
			//到达最高步数，开始匀速流程
			_flow = Flow_Run;
		}
		SetSpeed(_accUnit->GetCurSpeed());
		break;
	case Flow_Run:
		if (_decel != 0) {
			if (_stepLimitAction && (_curStep >= _decelStartStep)) {
				//到达减速步数，进入减速流程
				_accUnit->Start(UStepMotorAccUnit::Mode_Decel);
				_flow = Flow_Decel;
			}
		}
		break;
	case Flow_Decel:
		SetSpeed(_accUnit->GetCurSpeed());
		break;
	case Flow_Stop:
		Stop();
		DoMoveDoneEvent();
		//直接返回，防止下一次运动被清除标志
		return;
	default:
		//Error @Romeli 不可能到达位置（内存溢出）
		UDebugOut("Unkown error");
		break;
	}
	TIM_Clear_Update_Flag(_TIMx);
}

void UStepMotor::IRQ_StepMap() {
	_curStep++;
	if (_curDir == Dir_CW) {
		++_stepEncoder;
	} else {
		--_stepEncoder;
	}

	//处于步数限制运动中 并且 到达指定步数，停止运动
	if (_stepLimitAction && (_curStep == _tgtStep)) {
		//当处于步数运动并且到达指定步数时，停止
		_flow = Flow_Stop;
	}

	switch (_flow) {
	case Flow_Accel:
		if (_stepLimitAction) {
			//步数限制运动模式
			if (_curStep >= _decelStartStep) {
				//到达减速步数，进入减速流程
				_flow = Flow_Decel;
			}
		}
		if (_curStep >= _stepMap[_stepMapIndex].StepTab) {
			_TIMx->PSC = _stepMap[_stepMapIndex].PSCTab;
			_TIMx->ARR = _stepMap[_stepMapIndex].ARRTab;
			*_TIMx_CCRx = uint16_t(_TIMx->ARR >> 1);
			TIM_PSC_Reload(_TIMx);
			++_stepMapIndex;
			if (_stepMapIndex == _stepMapSteps) {
				//到达最高步数，开始匀速流程
				_flow = Flow_Run;
			}
		}
		break;
	case Flow_Run:
		if (_decel != 0) {
			if (_stepLimitAction && (_curStep >= _decelStartStep)) {
				//到达减速步数，进入减速流程
				_flow = Flow_Decel;
			}
		}
		break;
	case Flow_Decel:
		if (_stepMapIndex != 0) {
			if ((_tgtStep - _curStep) <= _stepMap[_stepMapIndex - 1].StepTab) {
				_TIMx->PSC = _stepMap[_stepMapIndex - 1].PSCTab;
				_TIMx->ARR = _stepMap[_stepMapIndex - 1].ARRTab;
				*_TIMx_CCRx = uint16_t(_TIMx->ARR >> 1);
				TIM_PSC_Reload(_TIMx);
				--_stepMapIndex;
			}
		}
		break;
	case Flow_Stop:
		StopForce();
		if (!_emo) {
			DoMoveDoneEvent();
		} else {
			_emo = false;
		}
		//直接返回，防止下一次运动被清除标志
		return;
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
void UStepMotor::BeforeInit() {
	UDebugOut("This function should be override");
	/*	GPIO_InitTypeDef GPIO_InitStructure;

	 RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	 RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);

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
 * return bool
 */
bool UStepMotor::GetLimit_CW() {
	return false;
}

/*
 * author Romeli
 * explain 获取反转限位传感器状态（建议在派生类中重写）
 * return bool
 */
bool UStepMotor::GetLimit_CCW() {
	return false;
}

void UStepMotor::DoMoveDoneEvent() {
	if (MoveDoneEvent != nullptr) {
		MoveDoneEvent();
	}
}

void UStepMotor::ParamInit() {
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
	_pool[_poolSp++] = this;

	_accUnit = 0;	//速度计算单元

	_curStep = 0;	//当前已移动步数
	_tgtStep = 0;	//目标步数
	_decelStartStep = 0;	//减速开始步数
	_stepEncoder = 0;

	_accel = 20000;			//加速度
	_decel = 20000;			//减速度
	_maxSpeed = 10000;		//最大速度
	_maxSpeedLimit = 400000;	//最大支持速度200k
	_startSpeed = 0;		//起步速度

	_limit_CW = 0; //正转保护限位
	_limit_CCW = 0; //反转保护限位

	_relativeDir = Dir_CW; //实际方向对应
	_curDir = Dir_CW; //当前方向

	_flow = Flow_Stop;
	_stepLimitAction = true;

	_emo = false;
	_busy = false;	//当前电机繁忙?

	MoveDoneEvent = nullptr;
}

/*
 * author Romeli
 * explain TIM初始化（应在派生类中重写）
 * return void
 */
void UStepMotor::TIMInit() {
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;

	TIM_DeInit(_TIMx);

	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInitStructure.TIM_Prescaler = 0;	//若需更改最小脉冲频率
	TIM_TimeBaseInitStructure.TIM_Period = 0;
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

	NVIC_InitStructure.NVIC_IRQChannel = _UIT_TIM_Update.NVIC_IRQChannel;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority =
			_UIT_TIM_Update.PreemptionPriority;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = _UIT_TIM_Update.SubPriority;
	NVIC_Init(&NVIC_InitStructure);
}

/*
 * author Romeli
 * explain 更改当前步进电机方向
 * param dir 欲改变的方向
 * return void
 */
void UStepMotor::SetDir(Dir_Typedef dir) {
	_curDir = dir;
	if (_curDir == _relativeDir) {
		SetDirPin(ENABLE);
	} else {
		SetDirPin(DISABLE);
	}
}

/*
 * author Romeli
 * explain 计算当前速度减速到怠速需要多少个脉冲
 * param 当前速度
 * return uint32_t
 */
uint32_t UStepMotor::GetDecelStep(uint32_t curSpeed) {
	uint32_t step = 0;
	float t;
	switch (_accMode) {
	case AccMode_AccUnit:
		//梯形的高
		t = (float) (curSpeed - _startSpeed) / (float) _decel;
		//梯形面积公式
		step = uint32_t((_startSpeed + curSpeed) * t) >> 1;
		break;
	case AccMode_StepMap:
		//三角形的长
		t = float(curSpeed) / _decel;
		//梯形面积公式
		step = uint32_t(curSpeed * t) >> 1;
		break;
	default:
		break;
	}
	return step;
}

/*
 * author Romeli
 * explain 计算第一步的频率
 * return void
 */
void UStepMotor::StartSpeedInit() {
	float t = sqrtf(2.0f / _accel);
	_startSpeed = 1 / t;
}

/*
 * author Romeli
 * explain 初始化加速表，理论上只需要在速度发生改变时更新
 * return uint16_t
 */
void UStepMotor::StepMapInit() {
	//Note @Romeli 这里speed使用整形作为单位可能产生运动误差，在速度越低的时候误差越大，可能要更换为浮点数
	float speed = 0, deltaT;
	uint16_t div = 1, psc, arr;
	uint32_t stepn;
	_stepMapSteps = 0;
	for (; div <= _stepMapSize; ++div) {
		speed = float(_maxSpeed * div) / _stepMapSize;
		//当前点速度不同
		if (div == _stepMapSize) {
			deltaT = 0;
		}
		deltaT = speed / _accel;
		stepn = uint32_t(speed * deltaT + 0.5) >> 1;
		if ((stepn == 0) && (div != _stepMapSize)) {
			//当移动距离步数为零，并且速度表不是最后（有可能加速时间不足1步，按0步速度算）
			continue;
		}

		psc = (_TIMx_FRQ / 0x10000 / speed) + 1;
		arr = _TIMx_FRQ / (psc + 1) / speed - 1;
		if (_stepMapSteps == 0) {
		} else if ((stepn != _stepMap[_stepMapSteps - 1].StepTab)
				|| (div == _stepMapSize)) {
			//最后一步即使不满足1步的距离，也设置为最高速度
			if ((psc == _stepMap[_stepMapSteps - 1].PSCTab)
					&& (arr == _stepMap[_stepMapSteps - 1].ARRTab)) {
				continue;
			}
		} else {
			continue;
		}

		_stepMap[_stepMapSteps].StepTab = stepn;
		_stepMap[_stepMapSteps].PSCTab = psc;
		_stepMap[_stepMapSteps].ARRTab = arr;

		++_stepMapSteps;
	}

}

/*
 * author Romeli
 * explain 应用速度到脉冲定时器（不会立即更新）
 * param 速度
 * return void
 */
void UStepMotor::SetSpeed(uint32_t speed) {
	_TIMx->PSC = (_TIMx_FRQ / 0x10000 / speed) + 1;
	_TIMx->ARR = _TIMx_FRQ / (_TIMx->PSC + 1) / speed;
	*_TIMx_CCRx = uint16_t(_TIMx->ARR >> 1);
}
