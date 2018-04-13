/*
 * HC165.h
 *
 *  Created on: 2017��1��6��
 *      Author: Romeli
 */

#ifndef U74HC165_H_
#define U74HC165_H_

#include <cmsis_device.h>

class U74HC165 {
public:
	U74HC165();
	void Init();
	void Read(uint8_t* data, uint8_t& len);

protected:
	virtual void GPIOInit() = 0;
	virtual inline void WritePin_PL(bool state) = 0;
	virtual inline void WritePin_CE(bool state) = 0;
	virtual inline void WritePin_CP(bool state) = 0;
	virtual inline bool ReadPin_DS() = 0;
};

#endif /* U74HC165_H_ */
