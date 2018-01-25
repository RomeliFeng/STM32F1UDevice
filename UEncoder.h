/*
 * UEncoder.h
 *
 *  Created on: 2017年11月1日
 *      Author: Romeli
 */

#ifndef UENCODER_H_
#define UENCODER_H_

#include <cmsis_device.h>
#include <UDebug.h>
#include <UMisc.h>

class UEncoder {
public:
	UEncoder(TIM_TypeDef* TIMx, UIT_Typedef& it);
	virtual ~UEncoder();

	//初始化硬件
	static void InitAll();
	void Init();
	//设置当前位置
	void Set(int32_t pos);
	//获取当前位置
	int32_t Get() const;

	//中断服务子函数
	void IRQ();
protected:
	//编码器定时器
	TIM_TypeDef* _TIMx;

	virtual void GPIOInit() = 0;
	virtual void TIMRCCInit() = 0;
private:
	static UEncoder* _Pool[];
	static uint8_t _PoolSp;

	UIT_Typedef _IT; //中断优先级
	volatile int16_t _ExCNT;

	void TIMInit();
	void ITInit();
};

#endif /* UENCODER_H_ */
