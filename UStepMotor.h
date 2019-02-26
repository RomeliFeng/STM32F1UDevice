/*
 * UStepMotor.h
 *
 *  Created on: 2017年11月6日
 *      Author: Romeli
 */

#ifndef USTEPMOTOR_H_
#define USTEPMOTOR_H_

#include <cmsis_device.h>
#include <Misc/UMisc.h>

class UStepMotorAccUnit;

class UStepMotor {
public:
	friend class UStepMotorAccUnit;

	enum Dir_Typedef
		:uint8_t {
			Dir_CW, Dir_CCW
	};

	struct StepMap {
		uint16_t PSCTab;
		uint16_t ARRTab;
		uint32_t StepTab;
	};

	UEventFun MoveDoneEvent;

	UStepMotor(TIM_TypeDef* TIMx, uint8_t TIMx_CCR_Ch, uint32_t TIMx_FRQ,
			UIT_Typedef& UIT_TIM_Update);
	UStepMotor(TIM_TypeDef* TIMx, uint8_t TIMx_CCR_Ch, uint32_t TIMx_FRQ,
			UIT_Typedef& UIT_TIM_Update, StepMap* stepMap,
			uint16_t stepMapSize);
	virtual ~UStepMotor();

	void Init();
	static void InitAll();
	static void StopAll();
	static void UnLockAll();
	static uint8_t GetTheLowestPreemptionPriority();

	bool IsBusy() {
		return _busy;
	}
	//设置速度和加速度
	void SetSpeed(uint32_t maxSpeed, uint32_t accel);
	//设置默认电机方向
	void SetRelativeDir(Dir_Typedef dir);
	//设置保护限位
	void SetLimit_CW(uint8_t limit_CW);
	void SetLimit_CCW(uint8_t limit_CCW);
	void SetLimit(uint8_t limit_CW, uint8_t limit_CCW);
	void SetLimit(Dir_Typedef dir, uint8_t limit);
	inline uint32_t GetCurStep() {
		return _curStep;
	}
	inline uint32_t GetTgtStep() {
		return _tgtStep;
	}
	inline int32_t GetPos() {
		return _stepEncoder;
	}
	inline void SetPos(int32_t pos) {
		_stepEncoder = pos;
	}

	//根据步数进行移动
	Status_Typedef Move(uint32_t step, Dir_Typedef dir, bool sync = false);
	Status_Typedef Move(int32_t step, bool sync = false);
	//持续移动
	Status_Typedef Run(Dir_Typedef dir);
	Status_Typedef Run(int32_t speed);
	//停止移动
	void Stop();
	void StopSlow();
	void StopForce();
	void Lock();
	void Unlock();
	//保护检测
	bool SafetyProtect();

	inline void IRQ() {
		(this->*this->_IRQTaget)();
	}
	void IRQ_AccUnit();
	void IRQ_StepMap();
protected:
	enum Flow_Typedef
		:uint8_t {
			Flow_Stop, Flow_Accel, Flow_Run, Flow_Decel
	};

	enum AccMode_Typedef
		:uint8_t {
			AccMode_AccUnit, AccMode_StepMap,
	};

	static UStepMotor* _pool[];
	static uint8_t _poolSp;

	TIM_TypeDef* _TIMx;	//脉冲发生用定时器
	uint8_t _TIMx_CCR_Ch;
	uint32_t _TIMx_FRQ;		//脉冲发生定时器的时钟频率
	UIT_Typedef& _UIT_TIM_Update; //中断优先级

#ifdef USERLIB_F1
	volatile uint16_t* _TIMx_CCRx; //脉冲发生定时器的输出通道
#elif defined USERLIB_F4
	volatile uint32_t* _TIMx_CCRx; //脉冲发生定时器的输出通道
#endif

	AccMode_Typedef _accMode;
	UStepMotorAccUnit* _accUnit;	//速度计算单元

	volatile uint32_t _curStep;	//当前已移动步数
	volatile uint32_t _tgtStep;	//目标步数
	volatile int32_t _stepEncoder;
	uint32_t _decelStartStep;	//减速开始步数

	uint32_t _accel;			//加速度
	uint32_t _decel;			//减速度
	uint32_t _maxSpeed;			//最大速度
	uint32_t _maxSpeedLimit;	//最大支持速度
	uint32_t _startSpeed;		//起步速度

	StepMap* _stepMap;
	uint16_t _stepMapIndex;
	uint16_t _stepMapSize;
	uint16_t _stepMapSteps;

	Dir_Typedef _relativeDir; //实际方向对应
	Dir_Typedef _curDir; //当前方向

	volatile Flow_Typedef _flow;	//当前电机状态
	volatile bool _stepLimitAction;	//是否由步数限制运动
	volatile bool _emo;		//紧急停止标志
	volatile bool _busy;	//当前电机繁忙?

	uint8_t _limit_CW; //正转保护限位
	uint8_t _limit_CCW; //反转保护限位

	void (UStepMotor::*_IRQTaget)(void);

	//开启TIM时钟 初始化GPIO
	virtual void BeforeInit() = 0;

	virtual void SetDirPin(FunctionalState newState) = 0;
	virtual void SetEnPin(FunctionalState newState) = 0;

	virtual bool GetLimit_CW();
	virtual bool GetLimit_CCW();

	virtual void DoMoveDoneEvent();

	void ParamInit();
	void StartSpeedInit();
	void StepMapInit();

	void TIMInit();
	void ITInit();

	void SetDir(Dir_Typedef dir);	//设置电机方向
	uint32_t GetDecelStep(uint32_t curSpeed);
	void SetSpeed(uint32_t speed);
};

#endif /* USTEPMOTOR_H_ */
