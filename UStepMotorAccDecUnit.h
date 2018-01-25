/*
 * UStepMotorAccDecUnit.h
 *
 *  Created on: 2017年11月7日
 *      Author: Romeli
 */

#ifndef USTEPMOTORACCDECUNIT
#define USTEPMOTORACCDECUNIT

#include <cmsis_device.h>
#include <UDebug.h>
#include <UMisc.h>
#include <Device/UStepMotor.h>

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
		return _Busy;
	}
	inline bool IsDone() {
		return _Done;
	}
	inline void SetMode(Mode_Typedef mode) {
		_Mode = mode;
	}

	uint16_t GetCurSpeed();
	void SetCurSpeed(uint16_t speed);

	//中断服务函数
	void IRQ();
protected:
	TIM_TypeDef* _TIMx;	//速度计算用定时器
	UStepMotor* _StepMotor;

	UIT_Typedef _IT; //中断优先级

	virtual void TIMRCCInit() = 0;
private:
	static UStepMotorAccDecUnit* _Pool[];
	static uint8_t _PoolSp;

	Mode_Typedef _Mode;
	uint32_t _Accel;
	uint32_t _Decel;
	uint16_t _MaxSpeed;
	bool _Done;
	volatile bool _Busy;

	void TIMInit();
	void ITInit();
};


#endif /* USTEPMOTORACCDECUNIT */
