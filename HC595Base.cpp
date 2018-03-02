/*
 * HC595.cpp
 *
 *  Created on: 2016��12��24��
 *      Author: Romeli
 */

#include <HC595Base.h>
#include <Tool/UTick.h>

HC595Class::HC595Class() {
}

void HC595Class::Init() {
	GPIOInit();
	Disable();
}

void HC595Class::Write(uint8_t*& data, uint8_t& len) {
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

inline void HC595Class::Disable() {
	WritePin_OE(false);
}

inline void HC595Class::Enable() {
	WritePin_OE(true);
}

void HC595Class::GPIOInit() {
}

void HC595Class::WritePin_DS(bool state) {
}

void HC595Class::WritePin_OE(bool state) {
}

void HC595Class::WritePin_STCP(bool state) {
}

void HC595Class::WritePin_SHCP(bool state) {
}
