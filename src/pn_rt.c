/*
 * pn_rt.c
 *
 *  Created on: Dec 5, 2025
 *      Author: SergDeev
 */

#include "profinet.h"

uint8_t rt_is_received;

extern uint16_t FrameDataObject;
extern uint8_t ControllerMacAddress[6];

uint8_t rx_data[8];

void pn_rt_receive(uint8_t* r_buf) {
	for (int i=0; i<numModules; i++) {
		if (Modules[i].outputDataObject != NULL) {
			uint16_t ix = *Modules[i].outputDataObject;
			memcpy(Modules[i].data.outputs, &r_buf[ix], Modules[i].SubmoduleDataLength);
			memcpy(rx_data, &r_buf[ix], Modules[i].SubmoduleDataLength);
		}
	}
}

int pn_rt(uint8_t* s_buf) {
	static uint16_t tick = 0;

	uint16_t* FrameID = (uint16_t*)s_buf;
	*FrameID = FrameDataObject; FrameID++;
	uint8_t * rt_data = (uint8_t*)FrameID;
	int ix = 0;
	int _max = 0;
	for (int im=0; im < numModules; im++) {
		if (Modules[im].inputDataObject != NULL) {
			ix = *Modules[im].inputDataObject;
			memcpy(Modules[im].data.inputs, rx_data, Modules[im].SubmoduleDataLength);
			memcpy(&rt_data[ix], Modules[im].data.inputs, Modules[im].SubmoduleDataLength);
			ix += Modules[im].SubmoduleDataLength;
			rt_data[ix] = Modules[im].data.State[0];
		}
		else if (Modules[im].inputIOCS != NULL) {
			ix = *Modules[im].inputIOCS;
			rt_data[ix] = Modules[im].data.State[0];
		}
		if (ix > _max) _max = ix;
	}
	_max++;
	if (_max < 40) _max = 40;

	uint16_t * CycleCounter = (uint16_t*)&rt_data[_max];
	*CycleCounter = SWAP16((uint16_t)(30*(uint16_t)tick)); tick++;
	rt_data[_max+2] = 0x35;
	rt_data[_max+3] = 0x00;

	return (_max+6);
}

