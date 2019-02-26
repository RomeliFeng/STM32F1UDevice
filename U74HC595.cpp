/*
 * HC595.cpp
 *
 *  Created on: 2016��12��24��
 *      Author: Romeli
 */

#include <Tool/UTick.h>
#include <U74HC595.h>

U74HC595::U74HC595() {
	_busy = false;
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
 * return bool 是否成功，在同时访问时会失败
 */
bool U74HC595::Write(uint8_t* data, uint8_t len) {
	if (_busy) {
		//防止打断
		return false;
	}
	STCP_Reset();
	uTick.Tick(1);
	for (int16_t i = len - 1; i >= 0; --i) {
		for (uint8_t mask = 0x80; mask != 0; mask >>= 1) {
			SHCP_Reset();
			uTick.Tick(1);
			if ((mask & data[i]) != 0) {
				DS_Set();
			} else {
				DS_Reset();
			}
			SHCP_Set();
			uTick.Tick(5);
		}
	}
	STCP_Set();
	uTick.Tick(1);
	Enable();
	return true;
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

/*
 * author Romeli
 * explain 查询是否繁忙
 * return bool
 */
bool U74HC595::IsBusy() const {
	return _busy;
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
