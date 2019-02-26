/*
 * UStepMotorAccDecUnit.h
 *
 *  Created on: 2017年11月7日
 *      Author: Romeli
 */

#ifndef USTEPMOTORACCDECUNIT
#define USTEPMOTORACCDECUNIT

#include <cmsis_device.h>
#include <Misc/UMisc.h>
#include <UStepMotor.h>

//定时器分频系数限制的最小加速度
#define MINACCDECSPEED 1500

/*
 * author Romeli
 * ps 提取对象时线程不安全，确保提取操作只在主线程中进行
 */
class UStepMotorAccUnit {
public:
	enum Mode_Typedef {
		Mode_Accel, Mode_Decel
	};

	//构造函数
	UStepMotorAccUnit(TIM_TypeDef* TIMx, uint32_t TIMx_FRQ,
			UIT_Typedef& UIT_TIM_Update);
	virtual ~UStepMotorAccUnit();

	void Init();

	static void InitAll();
	static uint8_t GetTheLowestPreemptionPriority();

	static UStepMotorAccUnit* GetFreeUnit(UStepMotor* stepMotor);
	static void Free(UStepMotor* stepMotor);
	void Free();
	void Lock(UStepMotor* stepMotor);
	void Start(Mode_Typedef mode);
	void Stop();

	//内联函数
	inline bool IsBusy() {
		return _busy;
	}
	inline bool IsDone() {
		return _done;
	}
	inline void SetMode(Mode_Typedef mode) {
		_mode = mode;
	}

	uint16_t GetCurSpeed();
	void SetCurSpeed(uint16_t speed);

	//中断服务函数
	void IRQ();
protected:
	static UStepMotorAccUnit* _pool[];
	static uint8_t _poolSp;

	TIM_TypeDef* _TIMx;	//速度计算用定时器
	uint32_t _TIMx_FRQ;		//加减速定时器的时钟频率
	UIT_Typedef& _UIT_TIM_Update; //中断优先级

	UStepMotor* _stepMotor;
	uint16_t _miniAccel;	//最小加速度

	Mode_Typedef _mode;
	uint32_t _accel;
	uint32_t _decel;
	uint16_t _initSpeed;
	uint16_t _maxSpeed;
	bool _done;
	volatile bool _busy;

	//开启TIM时钟
	virtual void BeforeInit() = 0;
	void TIMInit();
	void ITInit();
};

#endif /* USTEPMOTORACCDECUNIT */
