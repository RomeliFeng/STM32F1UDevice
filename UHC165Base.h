/*
 * HC165.h
 *
 *  Created on: 2017��1��6��
 *      Author: Romeli
 */

#ifndef UHC165BASE_H_
#define UHC165BASE_H_

#include <cmsis_device.h>

class UHC165Base {
public:
	UHC165Base();
	void Init();
	void Read(uint8_t* data, uint8_t& len);

protected:
	virtual void GPIOInit() = 0;
	virtual inline void WritePin_PL(bool state) = 0;
	virtual inline void WritePin_CE(bool state) = 0;
	virtual inline void WritePin_CP(bool state) = 0;
	virtual inline bool ReadPin_DS() = 0;
};

#endif /* UHC165BASE_H_ */
