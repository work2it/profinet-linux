/*
 * pn_rpc.c
 *
 *  Created on: Dec 4, 2025
 *      Author: SergDeev
 */

#include "profinet.h"

void make_control_io(_pn_io* pn_io_s);
extern uint8_t ar_uuid[16];

void make_control_rpc(_rpc* rpc_s) {
	rpc_s->packet_type = 0;
	rpc_s->flags1 = 0x20;
	rpc_s->sequence_num = 0;
	rpc_s->pnio_uuid[0] = 2;
	memcpy(&rpc_s->object_uuid[10], &ar_uuid[10], 6);
	rpc_s->server_boot_time = 0;
	_pn_io* pn_io_s = (_pn_io*)(rpc_s+1);
	pn_io_s->MaximumCount = pn_io_s->ArgsMaximum = 142;
	make_control_io(pn_io_s);
}

int pn_rpc(_rpc* rpc_r, _rpc* rpc_s) {
	int len = 0;
	rpc_s->version = 4;
	rpc_s->packet_type = 2;
	rpc_s->flags1 = 0x0a;
	rpc_s->flags2 = 0;
	rpc_s->byte_order = 0x10;
	rpc_s->float_point = 0;
	rpc_s->op_num = rpc_r->op_num;
	rpc_s->sequence_num = rpc_r->sequence_num;
	memcpy(rpc_s->object_uuid, rpc_r->object_uuid,16);
	memcpy(rpc_s->pnio_uuid, rpc_r->pnio_uuid,16);
	memcpy(rpc_s->activity, rpc_r->activity,16);
	rpc_s->server_boot_time = 0x7742ed93;
	rpc_s->interface_ver = rpc_r->interface_ver;
	rpc_s->interfece_hint = rpc_s->activity_hint = 0xffff;
	_pn_io* pn_io_s = (_pn_io*)(rpc_s+1);
	_pn_io* pn_io_r = (_pn_io*)(rpc_r+1);

//	printf("op_num: %d\n",rpc_r->op_num);
	if (rpc_r->op_num == 0) len = pn_io_connect(pn_io_r, pn_io_s);
	else if ((rpc_r->op_num == 2) || (rpc_r->op_num == 5)) len = pn_io_read(pn_io_r, pn_io_s);
	else if (rpc_r->op_num == 3) len = pn_io_write(pn_io_r, pn_io_s);
	else if (rpc_r->op_num == 4) len = pn_io_control(pn_io_r, pn_io_s);

	if (len > 0) {
		len += 20;
		rpc_s->fragment_len = len;
	}

	return len;
}
