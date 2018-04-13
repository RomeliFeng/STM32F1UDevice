/*
 * HC595.h
 *
 *  Created on: 2016��12��24��
 *      Author: Romeli
 */

#ifndef U74HC595_H_
#define U74HC595_H_

#include <cmsis_device.h>

class U74HC595 {
public:
	U74HC595();
	void Init();
	void Write(uint8_t* data, uint8_t& len);

	void Enable();
	void Disable();
protected:
	virtual void GPIOInit() = 0;
	virtual inline void WritePin_DS(bool state) = 0;
	virtual inline void WritePin_OE(bool state) = 0;
	virtual inline void WritePin_STCP(bool state) = 0;
	virtual inline void WritePin_SHCP(bool state) = 0;
};

#endif /* U74HC595_H_ */
