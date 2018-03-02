/*
 * HC165.h
 *
 *  Created on: 2017��1��6��
 *      Author: Romeli
 */

#ifndef HC165BASE_H_
#define HC165BASE_H_

#include <cmsis_device.h>

class HC165Base {
public:
	HC165Base();
	void Init();
	void Read(uint8_t*& data, uint8_t& len);

	void Enable();
	void Disable();
protected:
	virtual void GPIOInit() = 0;
	virtual void WritePin_PL(bool state) = 0;
	virtual void WritePin_CE(bool state) = 0;
	virtual void WritePin_CP(bool state) = 0;
	virtual bool ReadPin_DS() = 0;
};

#endif /* HC165BASE_H_ */
