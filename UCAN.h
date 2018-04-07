/*
 * UCAN.h
 *
 *  Created on: 2018年4月3日
 *      Author: Romeli
 */

#ifndef UCAN_H_
#define UCAN_H_

#include <cmsis_device.h>
#include <Misc/UMisc.h>
#include <Event/UEventPool.h>

class UCAN {
public:
	struct Data_Typedef {
		uint32_t id;
		uint8_t data[8];
		uint8_t dataSize;
	};

	voidFun ReceiveEvent;

	UCAN(uint8_t rxBufSize, CAN_TypeDef* CANx, UIT_Typedef it);
	virtual ~UCAN();

	void Init(uint16_t idH, uint16_t idL, uint16_t maskIdH, uint16_t maskIdL);
	void Send(Data_Typedef& data);
	void Send(uint32_t id, uint8_t* data, uint8_t size);
	void Send(uint32_t id);
	void Read(Data_Typedef& data);

	uint8_t Available();
	void SetEventPool(voidFun rcvEvent, UEventPool* pool);
	void IRQ();
protected:
	virtual void GPIOInit() = 0;
	virtual void CANRCCInit() = 0;
private:
	CAN_TypeDef* _CANx;
	UEventPool* _ePool;
	UIT_Typedef _it;
	Data_Typedef* _rxBuf;
	uint8_t _rxBufSize;
	uint8_t _rxBufStart, _rxBufEnd;

	void NVICInit();
	void CANInit(uint16_t idH, uint16_t idL, uint16_t maskIdH,
			uint16_t maskIdL);
};

#endif /* UCAN_H_ */
