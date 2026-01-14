/*
 * pn_ptcp.c
 *
 *  Created on: Dec 3, 2025
 *      Author: SergDeev
 */

#include "profinet.h"

typedef struct {
	uint16_t FrameID;
	uint8_t padding[12];
	uint16_t SequenceID;
	uint16_t zero;
	uint32_t delay;
	uint16_t header;
	uint8_t mac[6];
	uint16_t end;

} _ptcp;

int pn_ptcp(uint8_t* s_buf) {
static uint16_t ptcp_count=0;
static uint16_t ptcp_frame = 0xff40;
	eh = (struct ether_header *)s_buf;
  	memcpy(eh->ether_shost, hw_addr, 6); eh->ether_shost[5]++;
  	eh->ether_dhost[0] = 0x01; eh->ether_dhost[1] = 0x80;
  	eh->ether_dhost[2] = 0xc2; eh->ether_dhost[3] = 0x00;
  	eh->ether_dhost[4] = 0x00; eh->ether_dhost[5] = 0x0e;
  	eh->ether_type = SWAP16(0x8892);

	_ptcp* ptcp = (_ptcp*)(eh+1);
	ptcp->FrameID = SWAP16(ptcp_frame);
	ptcp_frame++;
	if (ptcp_frame > 0xff43) ptcp_frame = 0xff40;
	memset(&ptcp->padding, 0, 12);
	ptcp->SequenceID = SWAP16(ptcp_count); ptcp_count++;
  	ptcp->delay = SWAP32(ptcp_count*456270); ptcp_count++;
  	if (ptcp_count > 1) ptcp_count = 0;
  	memcpy(&ptcp->mac, &eh->ether_shost, 6);
  	ptcp->header = SWAP16(0x0c06);

  	uint16_t lx = ((uint8_t*)(ptcp+1)) - ((uint8_t*)s_buf);

	return lx;
}

