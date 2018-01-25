/*
 * UStepMotor.h
 *
 *  Created on: 2017年11月6日
 *      Author: Romeli
 */

#ifndef USTEPMOTOR_H_
#define USTEPMOTOR_H_

#include <cmsis_device.h>
#include <UDebug.h>
#include <UMisc.h>

#define STEP_MOTOR_MIN_SPEED 200

class UStepMotorAccDecUnit;

class UStepMotor {
public:
	friend class UStepMotorAccDecUnit;

	enum Flow_Typedef
		:uint8_t {
			Flow_Stop, Flow_Accel, Flow_Run, Flow_Decel
	};

	enum Dir_Typedef
		:uint8_t {
			Dir_CW, Dir_CCW
	};

	UStepMotor(TIM_TypeDef* TIMx, uint8_t TIMx_CCR_Ch,
			UIT_Typedef& it);
	virtual ~UStepMotor();

	void Init();
	static void InitAll();
	static uint8_t GetTheLowestPreemptionPriority();

	bool IsBusy() {
		return _Busy;
	}
	//设置速度和加速度
	void SetSpeed(uint16_t maxSpeed, uint32_t accel);
	//设置默认电机方向
	void SetRelativeDir(Dir_Typedef dir);
	//设置保护限位
	inline void SetCWLimit(uint8_t cwLimit) {
		_CWLimit = cwLimit;
	}
	inline void SetCCWLimit(uint8_t ccwLimit) {
		_CCWLimit = ccwLimit;
	}
	inline void SetLimit(uint8_t cwLimit, uint8_t ccwLimit) {
		SetCWLimit(cwLimit);
		SetCCWLimit(ccwLimit);
	}
	inline uint32_t GetCurStep() {
		return _CurStep;
	}
	inline uint32_t GetTgtStep() {
		return _TgtStep;
	}
	//根据步数进行移动
	Status_Typedef Move(uint32_t step, Dir_Typedef dir);
	//持续移动
	inline Status_Typedef Run(Dir_Typedef dir) {
		return Move(0, dir);
	}
	//停止移动
	void Stop();
	void StopSlow();
	inline void Lock() {
		SetEnPin(ENABLE);
	}
	inline void Unlock() {
		SetEnPin(DISABLE);
	}
	//保护检测
	void SafetyProtect(uint8_t limit);

	void IRQ();
protected:
	TIM_TypeDef* _TIMx;	//脉冲发生用定时器
	UIT_Typedef _IT; //中断优先级

	uint32_t _TIMy_FRQ;	//脉冲发生定时器的频率，由系统主频/分频数算的
	volatile uint16_t* _TIMx_CCRx; //脉冲发生定时器的输出通道
	uint8_t _TIMx_CCR_Ch;

	virtual void GPIOInit() = 0;
	virtual void TIMRCCInit() = 0;

	virtual void SetDirPin(FunctionalState newState) = 0;
	virtual void SetEnPin(FunctionalState newState) = 0;
private:
	static UStepMotor* _Pool[];
	static uint8_t _PoolSp;

	UStepMotorAccDecUnit* _AccDecUnit;	//速度计算单元

	volatile uint32_t _CurStep;	//当前已移动步数
	volatile uint32_t _TgtStep;	//目标步数
	uint32_t _DecelStartStep;	//减速开始步数

	uint32_t _Accel;	//加速度
	uint32_t _Decel;	//减速度
	uint16_t _MaxSpeed;	//最大速度

	uint8_t _CWLimit; //正转保护限位
	uint8_t _CCWLimit; //反转保护限位

	Dir_Typedef _RelativeDir; //实际方向对应
	Dir_Typedef _CurDir; //当前方向

	volatile Flow_Typedef _Flow;	//当前电机状态
	volatile bool _StepLimit;	//是否由步数限制运动
	volatile bool _Busy;	//当前电机繁忙?

	void TIMInit();
	void ITInit();

	void SetDir(Dir_Typedef dir);	//设置电机方向
	void StartDec();
	uint32_t GetDecelStep(uint16_t speedFrom);
	void SetSpeed(uint16_t speed);
};

#endif /* USTEPMOTOR_H_ */
