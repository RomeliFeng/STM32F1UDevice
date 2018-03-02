/*
 * HC595.cpp
 *
 *  Created on: 2016��12��24��
 *      Author: Romeli
 */

#include <Tool/UTick.h>
#include <UHC595Base.h>

UHC595Class::UHC595Class() {
}

void UHC595Class::Init() {
	GPIOInit();
	Disable();
}

void UHC595Class::Write(uint8_t*& data, uint8_t& len) {
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

inline void UHC595Class::Disable() {
	WritePin_OE(false);
}

inline void UHC595Class::Enable() {
	WritePin_OE(true);
}

void UHC595Class::GPIOInit() {
}

void UHC595Class::WritePin_DS(bool state) {
}

void UHC595Class::WritePin_OE(bool state) {
}

void UHC595Class::WritePin_STCP(bool state) {
}

void UHC595Class::WritePin_SHCP(bool state) {
}
