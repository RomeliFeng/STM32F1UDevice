/*
 * UEncoder.h
 *
 *  Created on: 2017年11月1日
 *      Author: Romeli
 */

#ifndef UENCODER_H_
#define UENCODER_H_

#include <cmsis_device.h>
#include <Misc/UMisc.h>

class UEncoder {
public:
	enum Dir_Typedef
		:uint8_t {
			Dir_Positive, Dir_Negtive
	};

	UEncoder(TIM_TypeDef* TIMx, UIT_Typedef& it);
	virtual ~UEncoder();

	//初始化硬件
	static void InitAll();
	void Init();
	//设置当前位置
	void SetRelativeDir(Dir_Typedef dir);
	void ClearPos();
	//获取当前位置
	int32_t GetPos() const;

	//中断服务子函数
	void IRQ();
protected:
	enum Flow_Typedef
		:uint8_t {
			Flow_CC1, Flow_CC2, Flow_CC3, Flow_CC4
	};

	static UEncoder* _pool[];
	static uint8_t _poolSp;

	//编码器定时器
	TIM_TypeDef* _TIMx;

	UIT_Typedef _UIT_TIM_CCx; //中断优先级
	volatile int32_t _exCNT;
	volatile mutable int32_t _pos;
	Dir_Typedef _relativeDir;

	volatile mutable bool _sync; //中断保护用标志

	volatile Flow_Typedef _flow;

	virtual void GPIOInit() = 0;
	virtual void TIMRCCInit() = 0;
	void TIMInit();
	void ITInit();
};

#endif /* UENCODER_H_ */
