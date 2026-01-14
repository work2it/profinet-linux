/*
 * pn_arp.c
 *
 *  Created on: Nov 26, 2025
 *      Author: SergDeev
 */

#include "profinet.h"

extern uint8_t hw_addr[];
extern uint8_t ip_addr[];

typedef struct { //ARP
	uint16_t Hardware_type;
	uint16_t Protocol_type;
	uint8_t Hardware_size;
	uint8_t Protocol_size;
	uint16_t Opcode;
	uint8_t Sender_MAC_address[6];
	uint8_t Sender_IP_address[4];
	uint8_t Target_MAC_address[6];
	uint8_t Target_IP_address[4];
} _arp;

int ip_equal(uint8_t* x, uint8_t* y) {
	for (int i=0; i<4; i++) {
		if (x[i] != y[i]) return 0;
	}
	return 1;
}

int pn_arp(uint8_t* r_buf, uint8_t* s_buf) {
	int len = 0;
	_arp* arp = (_arp*)r_buf;
	if (ip_equal(arp->Target_IP_address,ip_addr)) {
//		printf("************************ arp\n");
		_arp* arp_s = (_arp*)s_buf;
		arp_s->Hardware_type = SWAP16(1);
		arp_s->Protocol_type = SWAP16(0x0800);
		arp_s->Hardware_size = 6;
		arp_s->Protocol_size = 4;
		arp_s->Opcode = 0x0200;
		memcpy(&arp_s->Sender_MAC_address, hw_addr, 6);
		memcpy(&arp_s->Sender_IP_address, ip_addr, 4);
		memcpy(&arp_s->Target_MAC_address, &arp->Sender_MAC_address, 6);
		memcpy(&arp_s->Target_IP_address, &arp->Sender_IP_address, 4);
		uint8_t* x = (uint8_t*)(arp_s+1);
		len = x - s_buf;
//		if (len < 46) len = 46;
	}
	return len;
}
