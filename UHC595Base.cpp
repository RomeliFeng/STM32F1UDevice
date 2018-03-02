/*
 * HC595.cpp
 *
 *  Created on: 2016��12��24��
 *      Author: Romeli
 */

#include <Tool/UTick.h>
#include <UHC595Base.h>

UHC595Base::UHC595Base() {
}

void UHC595Base::Init() {
	GPIOInit();
	Disable();
}

void UHC595Base::Write(uint8_t*& data, uint8_t& len) {
	WritePin_STCP(false);
	UTick::Tick(1);
	for (uint8_t i = 0; i < len; ++i) {
		for (uint8_t mask = 0x80; mask != 0; mask >>= 1) {
			WritePin_SHCP(false);
			UTick::Tick(1);
			if ((mask & data[i]) != 0) {
				WritePin_DS(true);
			} else {
				WritePin_DS(false);
			}
			WritePin_SHCP(true);
			UTick::Tick(5);
		}
	}
	WritePin_STCP(true);
	UTick::Tick(1);
	Enable();
}

inline void UHC595Base::Disable() {
	WritePin_OE(false);
}

inline void UHC595Base::Enable() {
	WritePin_OE(true);
}

void UHC595Base::GPIOInit() {
}

inline void UHC595Base::WritePin_DS(bool state) {
}

inline void UHC595Base::WritePin_OE(bool state) {
}

inline void UHC595Base::WritePin_STCP(bool state) {
}

inline void UHC595Base::WritePin_SHCP(bool state) {
}
