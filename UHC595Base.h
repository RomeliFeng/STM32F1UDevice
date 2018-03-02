/*
 * HC595.h
 *
 *  Created on: 2016��12��24��
 *      Author: Romeli
 */

#ifndef UHC595BASE_H_
#define UHC595BASE_H_

#include "stm32f10x.h"

class UHC595Class {
public:
	UHC595Class();
	void Init();
	void Write(uint8_t*& data, uint8_t& len);
	void Disable();
	void Enable();
protected:
	virtual void GPIOInit() = 0;
	virtual void WritePin_DS(bool state) = 0;
	virtual void WritePin_OE(bool state) = 0;
	virtual void WritePin_STCP(bool state) = 0;
	virtual void WritePin_SHCP(bool state) = 0;
};

#endif /* UHC595BASE_H_ */
