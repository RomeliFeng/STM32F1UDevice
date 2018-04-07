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

/*
 * author Romeli
 * ps 提取对象时线程不安全，确保提取操作只在主线程中进行
 */
class UStepMotorAccDecUnit {
public:
	enum Mode_Typedef {
		Mode_Accel, Mode_Decel
	};

	//构造函数
	UStepMotorAccDecUnit(TIM_TypeDef* TIMx, UIT_Typedef& it);
	virtual ~UStepMotorAccDecUnit();

	void Init();

	static void InitAll();
	static uint8_t GetTheLowestPreemptionPriority();

	static UStepMotorAccDecUnit* GetFreeUnit(UStepMotor* stepMotor);
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
	TIM_TypeDef* _TIMx;	//速度计算用定时器
	UStepMotor* _stepMotor;

	UIT_Typedef _it; //中断优先级

	virtual void TIMRCCInit() = 0;
private:
	static UStepMotorAccDecUnit* _pool[];
	static uint8_t _poolSp;

	Mode_Typedef _mode;
	uint32_t _accel;
	uint32_t _decel;
	uint16_t _maxSpeed;
	bool _done;
	volatile bool _busy;

	void TIMInit();
	void ITInit();
};


#endif /* USTEPMOTORACCDECUNIT */
