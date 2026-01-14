/*
 * pn_icmp.c
 *
 *  Created on: Nov 26, 2025
 *      Author: SergDeev
 */

#include "profinet.h"

extern uint8_t hw_addr[];
extern uint8_t ip_addr[];
extern int16_t ip_ident;

typedef struct { // ICMP
	uint8_t Type;
	uint8_t Code;
	uint16_t Checksum;
	uint16_t Identifier_BE;
	uint16_t Identifier_LE;
	uint16_t Sequence_Number_BE;
	uint16_t Sequence_Number_LE;
	uint8_t data[];
} _icmp;

int pn_icmp(_ipv4* ipv4, uint8_t* s_buf) {
	int len = 0;
	if (ip_equal(ipv4->dst, ip_addr)) {
//		printf("************************ icmp\n");
		_icmp* icmp = (_icmp*)(ipv4+1);
		if (icmp->Type == 8) { // ping
			_ipv4* ipv4_s = (_ipv4*)s_buf;
			ipv4_s->version = ipv4->version;
			ipv4_s->DiffService = ipv4->DiffService;
			ipv4_s->len = ipv4->len;
			ipv4_s->idend = SWAP16(ip_ident); ip_ident++;
			ipv4_s->ttl = 30;
			ipv4_s->protocol = ipv4->protocol;
			ipv4_s->checksum = 0;
			memcpy(&ipv4_s->src, &ipv4->dst, 4);
			memcpy(&ipv4_s->dst, &ipv4->src, 4);
			_icmp* icmp_s = (_icmp*)(ipv4_s+1);
			icmp_s->Identifier_BE = icmp->Identifier_BE;
			icmp_s->Identifier_LE = icmp->Identifier_LE;
			icmp_s->Sequence_Number_BE = icmp->Sequence_Number_BE;
			icmp_s->Sequence_Number_LE = icmp->Sequence_Number_LE;
			memcpy(&icmp_s->data, &icmp->data, 32);
			len = 74;
		}
	}
	return len;
}
