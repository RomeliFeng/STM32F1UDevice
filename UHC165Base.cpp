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

void UHC165Base::Init() {
	GPIOInit();
}

inline void UHC165Base::Enable() {
	WritePin_CE(false);
}

inline void UHC165Base::Disable() {
	WritePin_CE(true);
}

void UHC165Base::Read(uint8_t*& data, uint8_t& len) {
	//发送一个低电平脉冲载入电平
	WritePin_PL(false);
	UTick::Tick(4);
	WritePin_PL(true);

	WritePin_CP(false);
	WritePin_CE(false);
	UTick::Tick(1);
	for (uint8_t i = 0; i < len; ++i) {
		for (uint8_t mask = 0x80; mask != 0; mask = mask >> 1) {
			if (ReadPin_DS()) {
				data[i] |= mask;
			} else {
				UTick::Tick(1);
			}
			//在上升沿装载新的数据
			WritePin_CP(true);
			UTick::Tick(1);
			//回到低电平，为了下次位移
			WritePin_CP(false);
		}
	}
	WritePin_CE(true);
}

void UHC165Base::GPIOInit() {
}

inline void UHC165Base::WritePin_PL(bool state) {
}

inline void UHC165Base::WritePin_CE(bool state) {
}

inline void UHC165Base::WritePin_CP(bool state) {
}

inline bool UHC165Base::ReadPin_DS() {
	return true;
}
