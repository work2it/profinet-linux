/*
 * profinet.h
 *
 *  Created on: Nov 20, 2025
 *      Author: root
 */
#pragma pack(push, 1)

#include <sys/shm.h>
#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/ether.h>
#include <unistd.h>
#include <stdint.h>
#include "cfg-parse/cfg_parse.h"

#ifndef PROFINET_H_
#define PROFINET_H_

#define ETHER_TYPE 0x8892
#define DEFAULT_IF "enp0s3"
#define BUF_SIZ 1500

extern struct cfg_struct* cfg;

extern uint8_t r_buf[];
extern uint8_t s_buf[];
extern struct ether_header *eh;
extern uint8_t is_change;
extern uint8_t hw_addr[6];
extern uint8_t ip_addr[4];
extern uint8_t ip_mask[4];
extern uint8_t ip_gateway[4];
extern char pn_name[64];
extern char VendorName[64];
extern uint16_t VendorID;
extern uint16_t DeviceID;

uint16_t SWAP16(uint16_t);
uint32_t SWAP32(uint32_t);

int load_config();
void save_config();
int lldp(uint8_t* s_buf);
int pn_ptcp(uint8_t* s_buf);

typedef struct {
	uint8_t State[4];
	uint8_t inputs[8];
	uint8_t outputs[8];
} pProcessData;

typedef struct { // Структура модулей из ExpectedSubmoduleBlock
	uint16_t Slot; // Номер слота
	 uint32_t SubmoduleIdentNumber;  // идентификатор модуля ( 0x01 - DI; 0x10 - DQ; 0x20 - UI; 0x21 - RTD);
	 uint16_t SubmoduleProperties; // 01 - input; 10 - output
	 uint16_t DataDescription;
	 uint16_t SubmoduleDataLength; // размер RT-данных
	 uint16_t userDataLen;  // длина настроечных данных
	 uint8_t userData[16]; // настроечные данные
	 char OrderNumber[20]; // заказной номер
	 char SerialNumber[16]; // серийный номер
	 uint16_t hwVersion; // hw-версия
	 uint8_t swVersion; // sw-версия
	 uint16_t Subslot;
	 uint32_t ModuleIdentNumber;
	 uint16_t* inputDataObject;
	 uint16_t* inputIOCS;
	 uint16_t* outputDataObject;
	 uint16_t* outputIOCS;
	 pProcessData data; // RT-данные
} pModules;

typedef struct {
	uint8_t version;
	uint8_t DiffService;
	uint16_t len;
	uint16_t idend;
	uint16_t flags;
	uint8_t ttl;
	uint8_t protocol;
	uint16_t checksum;
	uint8_t src[4];
	uint8_t dst[4];
} _ipv4;

typedef struct {
	uint16_t src_port;
	uint16_t dst_port;
	uint16_t len;
	uint16_t checksum;
} _udp;

typedef struct {
	uint8_t version;
	uint8_t packet_type;
	uint8_t flags1;
	uint8_t flags2;
	uint8_t byte_order;
	uint16_t float_point;
	uint8_t serial_high;
	uint8_t object_uuid[16];
	uint8_t pnio_uuid[16];
	uint8_t activity[16];
	uint32_t server_boot_time;
	uint32_t interface_ver;
	uint32_t sequence_num;
	uint16_t op_num;
	uint16_t interfece_hint;
	uint16_t activity_hint;
	uint16_t fragment_len;
	uint16_t fragment_num;
	uint8_t auth_proto;
	uint8_t serial_low;
} _rpc;

typedef struct {
	uint32_t ArgsMaximum;
	uint32_t ArgsLength;
	uint32_t MaximumCount;
	uint32_t Offset;
	uint32_t ActualCount;
} _pn_io;

extern int numModules;
extern pModules Modules[128];

int pn_init();
int pn_receive();
void pn_scan();
int pn_dcp(uint8_t* r_buf, uint8_t* s_buf);
int pn_arp(uint8_t* r_buf, uint8_t* s_buf);
int pn_udp(_ipv4* ipv4_r, _ipv4* ipv4_s);
int pn_udp_x(_rpc* rpc_r, _rpc* rpc_s);
int pn_icmp(_ipv4* ipv4, uint8_t* s_buf);
int ip_equal(uint8_t* x, uint8_t* y);
int pn_rpc(_rpc* rpc_r, _rpc* rpc_s);
int pn_io_connect(_pn_io* pn_io_r, _pn_io* pn_io_s);
int pn_io_read(_pn_io* pn_io_r, _pn_io* pn_io_s);
int pn_io_write(_pn_io* pn_io_r, _pn_io* pn_io_s);
int pn_io_control(_pn_io* pn_io_r, _pn_io* pn_io_s);
int pn_rt(uint8_t* s_buf);
void pn_rt_receive(uint8_t* r_buf);

extern uint8_t pn_is_init, rt_is_received;

#endif /* PROFINET_H_ */
