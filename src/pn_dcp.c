/*
 * pn_dcp.c
 *
 *  Created on: Nov 20, 2025
 *      Author: SergDeev
 */

#include <stdio.h>
#include <string.h>
#include "profinet.h"

extern uint8_t hw_addr[];
extern uint8_t ip_addr[];
extern char pn_name[];
extern uint8_t ip_mask[];
extern uint8_t ip_gateway[];
extern uint16_t VendorID;
extern uint16_t DeviceID;
extern char VendorName[];

uint8_t is_change = 0;

typedef struct { // структура блока DCP
	uint8_t ServiceID;
	uint8_t ServiceType;
	uint32_t Xid;
	uint16_t ResponceDelay;
	uint16_t DCPDataLenght;
} _dcp;

typedef struct {
	uint8_t Option;
	uint8_t Suboption;
	uint16_t DCPBlockLenght;
} _dcp_opt;

typedef struct {
	uint16_t BlockInfo;
	uint8_t IPaddress[4];
	uint8_t Subnetmask[4];
	uint8_t StandardGateway[4];
} _block_ip;

typedef struct {
	uint16_t BlockInfo;
	uint8_t DeviceRoleDetails;
	uint8_t Reserved;
} _block_role;

typedef struct {
	uint16_t BlockInfo;
	uint16_t VendorID;
	uint16_t DeviceID;
} _block_vendor;

typedef struct {
	uint16_t BlockInfo;
	uint8_t NameOfStation;
} _block_name;

typedef struct {
	uint16_t BlockInfo;
	uint8_t DeviceVendorValue;
} _block_vendor_name;

typedef struct {
	uint8_t Option;
	uint8_t Suboption;
} _opt_list;

typedef struct {
	_dcp_opt opt;
	uint16_t BlockInfo;
	_opt_list arr[];
} _blocks;

_dcp* dcp_;

uint8_t* insert_vendorName(uint8_t* dx) {
	_dcp_opt* opt = (_dcp_opt*)dx;
	opt->Option = 2;
	opt->Suboption = 1;
	_block_vendor_name* vn = (_block_vendor_name*)(opt+1);
	vn->BlockInfo = 0;
	uint16_t l = strlen(VendorName);
	memcpy(&vn->DeviceVendorValue, VendorName, l);
	opt->DCPBlockLenght = SWAP16(l+2);

	uint8_t* x = (uint8_t*)&vn->DeviceVendorValue;
	x += l + (l % 2);
	return x;
}

uint8_t* insert_nameOfStation(uint8_t* dx) {
	_dcp_opt* opt = (_dcp_opt*)dx;
	opt->Option = 2;
	opt->Suboption = 2;
	_block_name* nm = (_block_name*)(opt+1);
	nm->BlockInfo = 0;
	uint16_t l = strlen(pn_name);
	memcpy(&nm->NameOfStation, pn_name, l);
	opt->DCPBlockLenght = SWAP16(l+2);

	uint8_t* x = (uint8_t*)&nm->NameOfStation;
	x += l + (l % 2);
	return x;
}

uint8_t* insert_vendorID(uint8_t* dx) {
	_dcp_opt* opt = (_dcp_opt*)dx;
	opt->Option = 2;
	opt->Suboption = 3;
	opt->DCPBlockLenght = SWAP16(0x06);
	_block_vendor* vx = (_block_vendor*)(opt+1);
	vx->BlockInfo = 0;
	vx->VendorID = SWAP16(VendorID);
	vx->DeviceID = SWAP16(DeviceID);
	vx++;
	return (uint8_t*)vx;
}

uint8_t* insert_deviceRole(uint8_t* dx) {
	_dcp_opt* opt = (_dcp_opt*)dx;
	opt->Option = 2;
	opt->Suboption = 4;
	opt->DCPBlockLenght = SWAP16(0x04);
	_block_role* vr = (_block_role*)(opt+1);
	vr->BlockInfo = 0;
	vr->DeviceRoleDetails = 1;
	vr->Reserved = 0;
	vr++;
	return (uint8_t*)vr;
}

uint8_t* insert_IP(uint8_t* dx) {
	_dcp_opt* opt = (_dcp_opt*)dx;
	opt->Option = 1;
	opt->Suboption = 2;
	opt->DCPBlockLenght = SWAP16(14);
	_block_ip* ip = (_block_ip*)(opt+1);
	ip->BlockInfo = SWAP16(1);
	memcpy(&ip->IPaddress,ip_addr, 4);
	memcpy(&ip->Subnetmask,ip_mask, 4);
	memcpy(&ip->StandardGateway,ip_addr, 4);
	ip++;
	return (uint8_t*)ip;
}

uint8_t* insert_DHCP(uint8_t* dx) {
	_dcp_opt* opt = (_dcp_opt*)dx;
	opt->Option = 5;
	opt->Suboption = 4;
	opt->DCPBlockLenght = SWAP16(3);
	uint8_t* x = (uint8_t*)(opt+1);
	*x = 3;  x++;
	*x = 61; x++;
	*x = 2;  x++;
	*x = 0;  x++;
	return x;
}

int pn_ident(uint8_t* buf, int _all) {
	int lx = 0;
	uint16_t* FrameID = (uint16_t*)buf;
	*FrameID = SWAP16(0xfeff);

	_dcp* dcp = (_dcp*)(FrameID+1);
	dcp->ServiceID = 5;
	dcp->ServiceType = 1;
	dcp->Xid = dcp_->Xid;
	dcp->ResponceDelay = 0;
	dcp->DCPDataLenght = 0; //

	_blocks* blocks = (_blocks*)(dcp+1);
	blocks->opt.Option = 0x02;
	blocks->opt.Suboption = 0x05;
	blocks->opt.DCPBlockLenght = 0; //
	blocks->BlockInfo = 0;

	blocks->arr[0].Option = 2; // Device vendor value
	blocks->arr[0].Suboption = 1;

	blocks->arr[1].Option = 2; // Name of station
	blocks->arr[1].Suboption = 2;

	blocks->arr[2].Option = 2; // VendorID / DeviceID
	blocks->arr[2].Suboption = 3;

	blocks->arr[3].Option = 2; // Device role
	blocks->arr[3].Suboption = 4;

	blocks->arr[4].Option = 2; //
	blocks->arr[4].Suboption = 5;

	blocks->arr[5].Option = 1; //
	blocks->arr[5].Suboption = 1;

	blocks->arr[6].Option = 1; // IP
	blocks->arr[6].Suboption = 2;

	uint8_t* x = &blocks->arr[7].Option;
	blocks->opt.DCPBlockLenght = SWAP16(x-(uint8_t*)&blocks->BlockInfo);

	x = insert_vendorName(x);
	x = insert_nameOfStation(x);
	x = insert_vendorID(x);
	x = insert_deviceRole(x);
	x = insert_IP(x);

	uint8_t l = x - (uint8_t*)blocks;
	dcp->DCPDataLenght = SWAP16(l);

	lx = x - (uint8_t*)FrameID;

	return lx;
}

int pn_get(uint8_t* buf) {
	int lx = 0;
	uint16_t* FrameID = (uint16_t*)buf;
	*FrameID = SWAP16(0xfefd);

	_dcp* dcp = (_dcp*)(FrameID+1);
	dcp->ServiceID = 3;
	dcp->ServiceType = 1;
	dcp->Xid = dcp_->Xid;
	dcp->ResponceDelay = 0;
	dcp->DCPDataLenght = 0; //


	_dcp_opt* opt = (_dcp_opt*)(dcp+1);
	opt->Option = 1;
	opt->Suboption = 2;
	opt->DCPBlockLenght = SWAP16(14);
	_block_ip* ip = (_block_ip*)(opt+1);
	ip->BlockInfo = 1;
	memcpy(&ip->IPaddress,ip_addr, 4);
	memcpy(&ip->Subnetmask,ip_mask, 4);
	memcpy(&ip->StandardGateway,ip_addr, 4);

	opt = (_dcp_opt*)(ip+1);
	opt->Option = 5;
	opt->Suboption = 4;
	opt->DCPBlockLenght = SWAP16(3);
	uint8_t* x = (uint8_t*)(opt+1);
	*x = 3;  x++;
	*x = 61; x++;
	*x = 1;  x++;
	*x = 0;  x++;

	uint16_t l = x - (uint8_t*)(dcp+1);
	dcp->DCPDataLenght = SWAP16(l);

	lx = x - (uint8_t*)FrameID;

	return lx;
}

int pn_set(uint8_t* buf) {
	int lx = 0;
	uint16_t* FrameID = (uint16_t*)buf;
	*FrameID = SWAP16(0xfefd);
	uint16_t Datalen = SWAP16(dcp_->DCPDataLenght);
	_dcp_opt* opt_ = (_dcp_opt*)(dcp_+1);
	uint8_t* x = NULL;

	_dcp* dcp = (_dcp*)(FrameID+1);
	dcp->ServiceID = dcp_->ServiceID;
	dcp->ServiceType = 1;
	dcp->Xid = dcp_->Xid;
	dcp->ResponceDelay = 0;
	dcp->DCPDataLenght = SWAP16(8); //
	_dcp_opt* opt = (_dcp_opt*)(dcp+1);
	_dcp_opt* opt_begin = opt;


	while (Datalen > 2) {
	  uint16_t xaddr = 0;
	  printf("%d-%d %d\n",opt_->Option,opt_->Suboption, Datalen);
	  if ((opt_->Option == 1) && (opt_->Suboption == 2)) { // IP parameter
		opt->Option = 5;
		opt->Suboption = 4;
		opt->DCPBlockLenght = SWAP16(3);
		x = (uint8_t*)(opt+1);
		*x = 1; x++;
		*x = 2; x++;
		*x = 0; x++;
		*x = 0; x++;
	  }
	  else if ((opt_->Option == 3) && (opt_->Suboption == 61)) { // MAC Address
		opt->Option = 5;
		opt->Suboption = 4;
		opt->DCPBlockLenght = SWAP16(3);
		x = (uint8_t*)(opt+1);
		*x = 3; x++;
		*x = 61; x++;
		*x = 1; x++;
		*x = 0; x++;
	  }
	  else if ((opt_->Option == 2) && (opt_->Suboption == 2)) { // Name os Station
		uint16_t l = SWAP16(opt_->DCPBlockLenght)-2;
		uint8_t* xt = (uint8_t*)(opt_+1) +2;
		memset(pn_name, 0, sizeof(pn_name));
		memcpy(pn_name, xt, l);
		is_change = 1;
		xaddr = l % 2;

		opt->Option = 5;
		opt->Suboption = 4;
		opt->DCPBlockLenght = SWAP16(3);
		x = (uint8_t*)(opt+1);
		*x = 2; x++;
		*x = 2; x++;
		*x = 0; x++;
		*x = 0; x++;
	  }
	  else if ((opt_->Option == 5) && (opt_->Suboption == 2)) { //
		opt->Option = 5;
		opt->Suboption = 4;
		opt->DCPBlockLenght = SWAP16(3);
		x = (uint8_t*)(opt+1);
		*x = 5; x++;
		*x = 3; x++;
		*x = 0; x++;
//		*x = 0; x++;
	  }
	  // next opt s_buf
	  uint16_t ll = (x-(uint8_t*)opt);
	  opt = (_dcp_opt*)x;

	  // next block r_buf
	  uint16_t block_len = SWAP16(opt_->DCPBlockLenght)+xaddr;
	  Datalen -= (block_len+4);
	  uint8_t* x_ = (uint8_t*)opt_;
	  x_ += block_len+4;
	  opt_ = (_dcp_opt*)(x_);
	}
	if (x != NULL) {
		lx = x - (uint8_t*)opt_begin;
		dcp->DCPDataLenght = SWAP16(lx);
		lx = x - (uint8_t*)FrameID;
	}

	return lx;
}

int pn_dcp(uint8_t* r_buf, uint8_t* s_buf) {
	char sx[64]="";
	dcp_ = (_dcp*)r_buf;
	int len = 0;
//	printf("ServiceID:%02x\n",dcp_->ServiceID);
	switch (dcp_->ServiceID) {
		case 0x05: // Identify
			_dcp_opt* opt = (_dcp_opt*)(dcp_+1);
//			printf("Opt:%02x %02x\n",opt->Option, opt->Suboption);
			if (opt->Option == 0xff) len = pn_ident(s_buf, 1);
			else if ((opt->Option == 0x02) && (opt->Suboption == 0x02)) {
				char* x = (char*)(opt+1);
				memcpy(sx,x,SWAP16(opt->DCPBlockLenght));
//				printf("%s\n", sx);
				int n = strcmp(sx,pn_name);
				if (n == 0) len = pn_ident(s_buf, 0);
			}
			break;
		case 0x03: // Get
			len = pn_get(s_buf);
		break;
		case 0x04: // Set
			len = pn_set(s_buf);
		break;
	}
	return len;
}
