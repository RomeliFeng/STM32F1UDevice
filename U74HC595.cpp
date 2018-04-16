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
	WritePin_STCP(false);
	UTick::Tick(1);
	for (int16_t i = len - 1; i >= 0; --i) {
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

/*
 * author Romeli
 * explain 使能输出
 * return void
 */
inline void U74HC595::Enable() {
	//低电平使能
	WritePin_OE(false);
}

/*
 * author Romeli
 * explain 禁用输出
 * return void
 */
inline void U74HC595::Disable() {
	//高电平禁用
	WritePin_OE(true);
}

/*
 * author Romeli
 * explain 初始化使用到的GPIO（须在派生类中实现）
 * return void
 */
void U74HC595::GPIOInit() {
}

/*
 * author Romeli
 * explain 控制DS引脚（须在派生类中实现，需要内联）
 * return void
 */
inline void U74HC595::WritePin_DS(bool state) {
}

/*
 * author Romeli
 * explain 控制OE引脚（须在派生类中实现，需要内联）
 * return void
 */
inline void U74HC595::WritePin_OE(bool state) {
}

/*
 * author Romeli
 * explain 控制STCP引脚（须在派生类中实现，需要内联）
 * return void
 */
inline void U74HC595::WritePin_STCP(bool state) {
}

/*
 * author Romeli
 * explain 控制SHCP引脚（须在派生类中实现，需要内联）
 * return void
 */
inline void U74HC595::WritePin_SHCP(bool state) {
}
