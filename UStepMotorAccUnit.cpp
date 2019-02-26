/*
 * UStepMotorAccDecUnit.cpp
 *
 *  Created on: 2017年11月7日
 *      Author: Romeli
 */

#include <Misc/UDebug.h>
#include <UStepMotorAccUnit.h>

UStepMotorAccUnit* UStepMotorAccUnit::_pool[4];
uint8_t UStepMotorAccUnit::_poolSp = 0;

/*
 * author Romeli
 * explain 把自身加入资源池，并且初始化变量
 */
UStepMotorAccUnit::UStepMotorAccUnit(TIM_TypeDef* TIMx, uint32_t TIMx_FRQ,
		UIT_Typedef& UIT_TIM_Update) :
		_TIMx(TIMx), _TIMx_FRQ(TIMx_FRQ), _UIT_TIM_Update(UIT_TIM_Update) {
	_miniAccel = _TIMx_FRQ / 0x10000;
	//自动将对象指针加入资源池
	_pool[_poolSp++] = this;

	/* Do not care what their value */
	_stepMotor = 0;
	_mode = Mode_Accel;
	_maxSpeed = 10000;
	_accel = 20000;
	_decel = 20000;
	_busy = false;
	_done = false;
}

UStepMotorAccUnit::~UStepMotorAccUnit() {
}

/*
 * author Romeli
 * explain 初始化速度计算单元
 * return void
 */
void UStepMotorAccUnit::Init() {
	_stepMotor = 0;
	_busy = false;
	_done = false;

	BeforeInit();
	TIMInit();
	ITInit();
}

/*
 * author Romeli
 * explain 初始化所有速度计算单元
 * return void
 */
void UStepMotorAccUnit::InitAll() {
	//初始化池内所有单元
	for (uint8_t i = 0; i < _poolSp; ++i) {
		_pool[i]->Init();
	}
	if (_poolSp == 0) {
		//Warnning @Romeli 无速度计算单元（无法进行加速运动）
	}
}

/*
 * author Romeli
 * explain 获取加减速电机模块中权限最低的抢占优先级
 * return uint8_t
 */
uint8_t UStepMotorAccUnit::GetTheLowestPreemptionPriority() {
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
 * explain 从速度计算单元池中提取一个可用单元
 * return SMSCUnit* 可用单元的指针
 */
UStepMotorAccUnit* UStepMotorAccUnit::GetFreeUnit(UStepMotor* stepMotor) {
	//遍历池内所有单元
	UStepMotorAccUnit* unit;
	for (uint8_t i = 0; i < _poolSp; ++i) {
		unit = _pool[i];
		if (!unit->_busy) {
			//如果当前单元空闲，锁定当前单元供本次运动使用
			unit->Lock(stepMotor);
			return unit;
		} else {
			if (!unit->_stepMotor->_busy) {
				//Error @Romeli 释放了一个被锁定的速度控制单元（待验证）
				UDebugOut("There have no speed control unit exsit");
				//如果当前单元被占用，但是运动模块空闲，视为当前单元空闲，锁定当前单元供本次运动使用
				unit->Free();
				unit->Lock(stepMotor);
				return unit;
			}
		}
	}
	//Error @Romeli 无可用的速度计算单元（超出最大同时运动轴数，应该避免）
	UDebugOut("There have no available speed control unit");
	return 0;
}

/*
 * author Romeli
 * explain 释放当前速度计算单元
 * return void
 */
void UStepMotorAccUnit::Free(UStepMotor* stepMotor) {
	UStepMotorAccUnit* unit;
	//释放当前运动模块所占用的加减速单元
	for (uint8_t i = 0; i < _poolSp; ++i) {
		unit = _pool[i];
		if (unit->_stepMotor == stepMotor) {
			unit->Free();
		}
	}
}

/*
 * author Romeli
 * explain 解锁当前单元
 * return void
 */
void UStepMotorAccUnit::Free() {
	//关闭当前单元
	Stop();
	//复位标志位，解锁当前单元
	_busy = false;
}

/*
 * author Romeli
 * explain 锁定当前单元准备运动
 * param stepMotor 欲使用当前单元的运动模块
 * return void
 */
void UStepMotorAccUnit::Lock(UStepMotor* stepMotor) {
	//存储当前单元的运动模块
	_stepMotor = stepMotor;
	//存储加速度
	_accel = _stepMotor->_accel;
	//存储初始速度
	_initSpeed = _stepMotor->_startSpeed;
	//存储最大速度
	_maxSpeed = _stepMotor->_maxSpeed;
	//存储减速度
	_decel = _stepMotor->_decel;
	//置忙标志位，锁定当前单元
	_busy = true;
}

/*
 * author Romeli
 * explain 启动速度计算单元
 * param1 dir 加速还是减速
 * param2 tgtSpeed 目标速度
 * return void
 */
void UStepMotorAccUnit::Start(Mode_Typedef mode) {
	//关闭可能存在的计算任务
	Stop();
	SetMode(mode);
	_done = false;

	uint16_t initCNT = 0;
	switch (_mode) {
	case Mode_Accel:
		initCNT = _initSpeed;
		_TIMx->PSC = uint16_t(_TIMx_FRQ / _accel);
		_TIMx->ARR = _maxSpeed;
		break;
	case Mode_Decel: {
		//减速流程中，定时器的值和速度的关系是（speed=_maxSpeed - CNT）
		initCNT = uint16_t(_maxSpeed - _TIMx->CNT);
		_TIMx->PSC = uint16_t(_TIMx_FRQ / _decel);
		_TIMx->ARR = uint16_t(_maxSpeed - _initSpeed);
		break;
	}
	default:
		//Error @Romeli 错误的加减速模式，不应该发生
		UDebugOut("Unkown speed change mode");
		SetMode(Mode_Accel);
		_TIMx->CNT = _stepMotor->_startSpeed;
		_done = true;
		return;
	}

	TIM_PSC_Reload(_TIMx);	//更新时会清空CNT，需要注意
	_TIMx->CNT = initCNT;

	//开始速度计算
	TIM_Clear_Update_Flag(_TIMx);
	TIM_Enable_IT_Update(_TIMx);
	TIM_Enable(_TIMx);
}

/*
 * author Romeli
 * explain 关闭当前速度计算单元
 * return void
 */
void UStepMotorAccUnit::Stop() {
	//关闭速度计算定时器
	TIM_Disable_IT_Update(_TIMx);
	TIM_Disable(_TIMx);
	//清除速度计算定时器中断标志
}

/*
 * author Romeli
 * explain 根据TIM寄存器计算当前速度
 * return uint16_t
 */
uint16_t UStepMotorAccUnit::GetCurSpeed() {
	//读取当前速度
	uint16_t speed = uint16_t(_done ? _TIMx->ARR : _TIMx->CNT);
	switch (_mode) {
	case Mode_Accel:
		return speed;
		break;
	case Mode_Decel:
		//计算当前速度（部分定时器没有向下计数）
		return uint16_t(_maxSpeed - speed);
		break;
	default:
		//Error @Romeli 错误的状态，不应该发生（超出最大同时运动轴数，应该避免）
		UDebugOut("Status Error!");
		return _stepMotor->_startSpeed;
		break;
	}
}

/*
 * author Romeli
 * explain 设置当前速度，用于加速度为0的情况
 * param speed 速度
 * return void
 */
void UStepMotorAccUnit::SetCurSpeed(uint16_t speed) {
	if (speed < _stepMotor->_startSpeed) {
		//Error @Romeli 速度小于最低速度
		UDebugOut("There have no available speed control unit");
		speed = _stepMotor->_startSpeed;
	}
	_TIMx->CNT = speed;
}

/*
 * author Romeli
 * explain 速度计算单元更新速度用的中断服务子程序
 * return void
 */
void UStepMotorAccUnit::IRQ() {
	//停止
	Stop();
	_TIMx->CNT = _TIMx->ARR;
	_done = true;
}

/*
 * author Romeli
 * explain 初始化定时器设置（此函数应在派生类中重写）
 * return void
 */
void UStepMotorAccUnit::TIMInit() {
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;

	TIM_DeInit(_TIMx);

	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseInitStructure.TIM_Prescaler = 0xffff;
	TIM_TimeBaseInitStructure.TIM_Period = 0xffff;
	TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(_TIMx, &TIM_TimeBaseInitStructure);

	TIM_ARRPreloadConfig(_TIMx, ENABLE);
}

/*
 * author Romeli
 * explain 初始化中断设置
 * return void
 */
void UStepMotorAccUnit::ITInit() {
	NVIC_InitTypeDef NVIC_InitStructure;
	//设置中断
	NVIC_InitStructure.NVIC_IRQChannel = _UIT_TIM_Update.NVIC_IRQChannel;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority =
			_UIT_TIM_Update.PreemptionPriority;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = _UIT_TIM_Update.SubPriority;
	NVIC_Init(&NVIC_InitStructure);
}

