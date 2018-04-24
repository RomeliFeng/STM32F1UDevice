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
	bool Read(uint8_t* data, uint8_t len);
	bool IsBusy() const;
protected:
	volatile bool _busy;

	virtual void GPIOInit() = 0;
	virtual inline void PL_Set() = 0;
	virtual inline void CP_Set() = 0;
	virtual inline void CE_Set() = 0;
	virtual inline void PL_Reset() = 0;
	virtual inline void CP_Reset() = 0;
	virtual inline void CE_Reset() = 0;
	virtual inline bool DS_Read() = 0;
};

#endif /* U74HC165_H_ */
