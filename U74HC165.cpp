/*
 * HC165.cpp
 *
 *  Created on: 2017��1��6��
 *      Author: Romeli
 */

#include <Tool/UTick.h>
#include <U74HC165.h>

U74HC165::U74HC165() {
}

/*
 * author Romeli
 * explain 初始化74HC165接口
 * return void
 */
void U74HC165::Init() {
	GPIOInit();
}

/*
 * author Romeli
 * explain 从74HC165接口读取并行数据，长度为字节
 * param data 读取回来的数据
 * param len 欲读取数据长度
 * return void
 */
void U74HC165::Read(uint8_t* data, uint8_t len) {
	CP_Reset();
	//发送一个低电平脉冲载入电平
	PL_Reset();
	UTick::Tick(4);
	PL_Set();
	CE_Reset();
	for (int16_t i = len - 1; i >= 0; --i) {
		for (uint8_t mask = 0x80; mask != 0; mask = mask >> 1) {
			//回到低电平，为了下次位移
			CP_Reset();
			if (DS_Read()) {
				data[i] |= mask;
			} else {
				data[i] &= (~mask);
			}
			//在上升沿装载新的数据
			CP_Set();
			UTick::Tick(1);
		}
	}
	CE_Set();
}

inline void U74HC165::PL_Set() {
}

inline void U74HC165::CP_Set() {
}

inline void U74HC165::CE_Set() {
}

inline void U74HC165::PL_Reset() {
}

inline void U74HC165::CP_Reset() {
}

inline void U74HC165::CE_Reset() {
}

inline bool U74HC165::DS_Read() {
	return false;
}

