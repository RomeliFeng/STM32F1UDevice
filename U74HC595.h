/*
 * HC595.h
 *
 *  Created on: 2016��12��24��
 *      Author: Romeli
 */

#ifndef U74HC595_H_
#define U74HC595_H_

#include <cmsis_device.h>
#include <Misc/UMisc.h>

class U74HC595 {
public:
	U74HC595();
	void Init();
	bool Write(uint8_t* data, uint8_t len);

	void Enable();
	void Disable();
	bool IsBusy() const;
protected:
	volatile bool _busy;

	virtual void GPIOInit() = 0;
	virtual inline void DS_Set() = 0;
	virtual inline void OE_Set() = 0;
	virtual inline void STCP_Set() = 0;
	virtual inline void SHCP_Set() = 0;
	virtual inline void DS_Reset() = 0;
	virtual inline void OE_Reset() = 0;
	virtual inline void STCP_Reset() = 0;
	virtual inline void SHCP_Reset() = 0;
};

#endif /* U74HC595_H_ */
