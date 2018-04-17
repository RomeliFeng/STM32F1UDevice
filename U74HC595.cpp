/*
 * HC595.cpp
 *
 *  Created on: 2016��12��24��
 *      Author: Romeli
 */

#include <Tool/UTick.h>
#include <U74HC595.h>

U74HC595::U74HC595() {
}

/*
 * author Romeli
 * explain 初始化74HC595接口
 * return void
 */
void U74HC595::Init() {
	GPIOInit();
	Disable();
}

/*
 * author Romeli
 * explain 向74HC595接口写入并行数据，长度为字节
 * param data 欲写入的数据
 * param len 欲写入数据长度
 * return void
 */
void U74HC595::Write(uint8_t* data, uint8_t len) {
	STCP_Reset();
	UTick::Tick(1);
	for (int16_t i = len - 1; i >= 0; --i) {
		for (uint8_t mask = 0x80; mask != 0; mask >>= 1) {
			SHCP_Reset();
			UTick::Tick(1);
			if ((mask & data[i]) != 0) {
				DS_Set();
			} else {
				DS_Reset();
			}
			SHCP_Set();
			UTick::Tick(5);
		}
	}
	STCP_Set();
	UTick::Tick(1);
	Enable();
}

/*
 * author Romeli
 * explain 使能输出
 * return void
 */
inline void U74HC595::Enable() {
	//低电平使能
	OE_Reset();
}

/*
 * author Romeli
 * explain 禁用输出
 * return void
 */
inline void U74HC595::Disable() {
	//高电平禁用
	OE_Set();
}

inline void U74HC595::DS_Set() {
}

inline void U74HC595::OE_Set() {
}

inline void U74HC595::STCP_Set() {
}

inline void U74HC595::SHCP_Set() {
}

inline void U74HC595::DS_Reset() {
}

inline void U74HC595::OE_Reset() {
}

inline void U74HC595::STCP_Reset() {
}

inline void U74HC595::SHCP_Reset() {
}

