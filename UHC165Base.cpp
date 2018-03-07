/*
 * HC165.cpp
 *
 *  Created on: 2017��1��6��
 *      Author: Romeli
 */

#include <Tool/UTick.h>
#include <UHC165Base.h>

UHC165Base::UHC165Base() {
}

/*
 * author Romeli
 * explain 初始化74HC165接口
 * return void
 */
void UHC165Base::Init() {
	GPIOInit();
}

/*
 * author Romeli
 * explain 从74HC165接口读取并行数据，长度为字节
 * param data 读取回来的数据
 * param len 欲读取数据长度
 * return void
 */
void UHC165Base::Read(uint8_t* data, uint8_t& len) {
	//发送一个低电平脉冲载入电平
	WritePin_PL(false);
	UTick::Tick(4);
	WritePin_PL(true);

	WritePin_CP(false);
	WritePin_CE(false);
	UTick::Tick(1);
	for (int16_t i = len - 1; i >= 0; --i) {
		for (uint8_t mask = 0x80; mask != 0; mask = mask >> 1) {
			if (ReadPin_DS()) {
				data[i] |= mask;
				UTick::Tick(1);
			} else {
				data[i] &= (~mask);
			}
			//在上升沿装载新的数据
			WritePin_CP(true);
			UTick::Tick(3);
			//回到低电平，为了下次位移
			WritePin_CP(false);
		}
	}
	WritePin_CE(true);
}

/*
 * author Romeli
 * explain 初始化使用到的GPIO（须在派生类中实现）
 * return void
 */
void UHC165Base::GPIOInit() {
}

/*
 * author Romeli
 * explain 控制PL引脚（须在派生类中实现，需要内联）
 * return void
 */
inline void UHC165Base::WritePin_PL(bool state) {
}

/*
 * author Romeli
 * explain 控制CE引脚（须在派生类中实现，需要内联）
 * return void
 */
inline void UHC165Base::WritePin_CE(bool state) {
}

/*
 * author Romeli
 * explain 控制CP引脚（须在派生类中实现，需要内联）
 * return void
 */
inline void UHC165Base::WritePin_CP(bool state) {
}

/*
 * author Romeli
 * explain 读取引脚（须在派生类中实现，需要内联）
 * return void
 */
inline bool UHC165Base::ReadPin_DS() {
	return true;
}
