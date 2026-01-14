/*
 * pn_udp.c
 *
 *  Created on: Nov 26, 2025
 *      Author: SergDeev
 */

#include "profinet.h"

extern uint8_t ip_addr[];
extern uint16_t service_port;

uint16_t ipv4_ident;

void make_control_rpc(_rpc* rpc_s);

void make_control_udp(_rpc* rpc_s) {
	make_control_rpc(rpc_s);
}

int pn_udp(_ipv4* ipv4_r, _ipv4* ipv4_s) {
	int len = 0;
	if (ip_equal(ipv4_r->dst, ip_addr)) {
		ipv4_s->version = ipv4_r->version;
		ipv4_s->DiffService = ipv4_r->DiffService;
		ipv4_s->idend = SWAP16(ipv4_ident); ipv4_ident++;
//		ipv4_s->idend = ipv4_r->idend;
		ipv4_s->flags = ipv4_r->flags;
		ipv4_s->ttl = ipv4_r->ttl;
		ipv4_s->protocol = ipv4_r->protocol;
		ipv4_s->checksum = SWAP16(0x103c);
		memcpy(ipv4_s->src, ipv4_r->dst, 4);
		memcpy(ipv4_s->dst, ipv4_r->src, 4);
		_udp* udp_r = (_udp*)(ipv4_r+1);
		_udp* udp_s = (_udp*)(ipv4_s+1);
		udp_s->src_port = SWAP16(service_port);
		udp_s->dst_port = udp_r->src_port;
		udp_s->checksum = SWAP16(0xe34c);
		_rpc* rpc_r = (_rpc*)(udp_r+1);
		_rpc* rpc_s = (_rpc*)(udp_s+1);
		if (rpc_r->packet_type == 0) {
			len = pn_rpc(rpc_r, rpc_s);
			if (len > 0) {
				len  += 88;
				udp_s->len = SWAP16(len);
				len += 20;
				ipv4_s->len = SWAP16(len);
			}
		}
	}
	return len;
}

int pn_udp_x(_rpc* rpc_r, _rpc* rpc_s) {
	int len = 0;
	if (rpc_r->packet_type == 0) {
		len = pn_rpc(rpc_r, rpc_s);
	}
	return len;
}
