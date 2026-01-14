/*
 * lldp.c
 *
 *  Created on: Dec 3, 2025
 *      Author: SergDeev
 */
#include "profinet.h"

typedef struct {
	uint8_t id; // 0x02
	uint8_t length; // len+1
	uint8_t subtype; // 0x07
	uint8_t name[]; // pn_name
} _lldp_chassis;

typedef struct {
	uint8_t id; // 0x04
	uint8_t length; // len+1
	uint8_t subtype; // 0x07
	uint8_t name[]; // 'port-001'
} _lldp_port;

typedef struct {
	uint8_t id; // 0x06
	uint8_t length; // 2
	uint16_t seconds; // SWAP16(20)
} _lldp_ttl;

typedef struct {
	uint8_t id; // 0x10
	uint8_t length; // 20
	uint8_t str_len; // 5
	uint8_t subtype; // 1
	uint8_t address[4]; // ip
	uint8_t ifindex;
	uint32_t number; // SWAP32(1)
	uint8_t oid_len; // 8
	uint8_t oid[8]; // [0x2b,0x06,0x01,0x04,0x01,0x81,0xc0,0x6e]
} _lldp_address;

typedef struct {
	uint8_t id; // 0xfe
	uint8_t length; // 24
	uint8_t code[3]; // [0x00,0x0e,0xcf]
	uint8_t subtype; // 1
	uint32_t rx_delay_local; // SWAP32(313)
	uint32_t rx_delay_remote; // 0
	uint32_t tx_delay_local; // SWAP32(103)
	uint32_t tx_delay_remote; // 0
	uint32_t cab_delay_remote; // 0
} _lldp_org_name;

typedef struct {
	uint8_t id; // 0xfe
	uint8_t length; // 8
	uint8_t code[3]; // [0x00,0x0e,0xcf]
	uint8_t subtype; // 2
	uint32_t rt_class; // 0 ******************
} _lldp_org_port;

typedef struct {
	uint8_t id; // 0xfe
	uint8_t length; // 10
	uint8_t code[3]; // [0x00,0x0e,0xcf]
	uint8_t subtype; // 5
	uint8_t mac[6]; // mac-address
} _lldp_org_mac;

typedef struct {
	uint8_t id; // 0xfe
	uint8_t length; // 9
	uint8_t code[3]; // [0x00,0x12,0x0f]
	uint8_t config; // 1
	uint8_t status; // 3
	uint16_t auto_neg; // SWAP16(0xec00)
	uint16_t mau_type; // SWAP16(0x0010)
} _lldp_org_phy;

typedef struct {
	uint16_t data; // 0
} _lldp_end;

int lldp(uint8_t* s_buf) {
	eh = (struct ether_header *)s_buf;
  	memcpy(eh->ether_shost, hw_addr, 6); eh->ether_shost[5]++;
  	eh->ether_dhost[0] = 0x01; eh->ether_dhost[1] = 0x80;
  	eh->ether_dhost[2] = 0xc2; eh->ether_dhost[3] = 0x00;
  	eh->ether_dhost[4] = 0x00; eh->ether_dhost[5] = 0x0e;
  	eh->ether_type = SWAP16(0x88cc);

  	eh++;
  	_lldp_chassis* xc = (_lldp_chassis*)eh;
  	xc->id =2;
  	xc->length = strlen(pn_name)+1;
  	xc->subtype = 7;
  	memcpy(&xc->name, &pn_name, strlen(pn_name));

  	_lldp_port* lp = (_lldp_port*)&xc->name[strlen(pn_name)];
  	lp->id = 4;
  	lp->length = strlen("port-001")+1;
  	lp->subtype = 7;
  	memcpy(&lp->name, "port-001", strlen("port-001"));

  	_lldp_ttl* lt = (_lldp_ttl*)&lp->name[strlen("port-001")];
  	lt->id = 6;
  	lt->length = 2;
  	lt->seconds = SWAP16(20);

  	lt++;
  	_lldp_address* la = (_lldp_address*)lt;
  	la->id = 0x10;
  	la->length = 20;
  	la->str_len = 5;
  	la->subtype = 1;
  	memcpy(&la->address, ip_addr, 4);
  	la->ifindex = 2;
  	la->number = SWAP32(1);
  	la->oid_len = 8;
  	la->oid[0] = 0x2b; la->oid[1] = 0x06;
  	la->oid[2] = 0x01; la->oid[3] = 0x04;
  	la->oid[4] = 0x01; la->oid[5] = 0x81;
  	la->oid[6] = 0xc0; la->oid[7] = 0x6e;

  	la++;
  	_lldp_org_name* lon = (_lldp_org_name*)la;
  	lon->id = 0xfe;
  	lon->length = 24;
  	lon->code[0] = 0x00; lon->code[1] = 0x0e; lon->code[2] = 0xcf;
  	lon->subtype = 1;
  	lon->rx_delay_local = SWAP32(313);
  	lon->rx_delay_remote = SWAP32(0);
  	lon->tx_delay_local = SWAP32(103);
  	lon->tx_delay_remote = SWAP32(0);
  	lon->cab_delay_remote = SWAP32(0);

  	lon++;
  	_lldp_org_port* lop = (_lldp_org_port*)lon;
  	lop->id = 0xfe;
  	lop->length = 8;
  	lop->code[0] = 0x00; lop->code[1] = 0x0e; lop->code[2] = 0xcf;
  	lop->subtype = 2;
  	lop->rt_class = 0;

  	lop++;
  	_lldp_org_mac* lom = (_lldp_org_mac*)lop;
  	lom->id = 0xfe;
  	lom->length = 10;
  	lom->code[0] = 0x00; lom->code[1] = 0x0e; lom->code[2] = 0xcf;
  	lom->subtype = 5;
  	memcpy(&lom->mac, hw_addr, 6);

  	lom++;
  	_lldp_org_phy* loph =(_lldp_org_phy*)lom;
  	loph->id = 0xfe;
  	loph->length = 9;
  	loph->code[0] = 0x00; loph->code[1] = 0x12; loph->code[2] = 0x0f;
  	loph->config = 1;
  	loph->status = 3;
  	loph->auto_neg = SWAP16(0x6c00);
  	loph->mau_type = SWAP16(0x0010);

  	loph++;
  	_lldp_end* loe = (_lldp_end*)loph;
  	loe->data = 0;

  	loe++;
  	int lx = (uint8_t*)loe - s_buf;

  	return lx;
}

