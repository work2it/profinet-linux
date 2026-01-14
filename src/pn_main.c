/*
 * pn_main.c
 *
 *  Created on: Nov 20, 2025
 *      Author: SergDeev
 */

#include "profinet.h"
#include <time.h>
#include <fcntl.h>


#define SIEMENS_PORT 34964
uint16_t service_port = 50000;

struct cfg_struct* cfg;
char pn_name[64]="linux-pn-device";
uint8_t hw_addr[6];
uint8_t ip_addr[4];
uint8_t ip_mask[4];
uint8_t ip_gateway[4];

uint16_t VendorID=0x1d6b;
uint16_t DeviceID=0x0001;
char VendorName[64]="Virtual";

int sockfd, sock_siemens, sock_service, sock_service_1;
struct sockaddr_in si_siemens, si_service, si_service_1, si_client;
int slen = sizeof(si_client);
struct ether_header *eh;
struct ether_header *ex;
uint8_t r_buf[1600];
uint8_t s_buf[1600];
struct sockaddr_ll socket_address;
struct ifreq ifopts;
int ifindex = 0;
uint16_t ip_ident = 0;

struct sockaddr_in si_me, si_other;

void save_config() {
	cfg_set(cfg, "PN_NAME", pn_name);
	cfg_save(cfg, "config.ini");
}

int load_config() {
	char sx[64];
	cfg = cfg_init();
	cfg_set(cfg, "IF_NAME", DEFAULT_IF);
	cfg_set(cfg, "PN_NAME", pn_name);
	sprintf(sx,"%04x",VendorID);
	cfg_set(cfg, "VendorID", sx);
	sprintf(sx,"%04x",DeviceID);
	cfg_set(cfg, "DeviceID", sx);
	cfg_set(cfg, "VendorName", VendorName);
	sprintf(sx,"%d",service_port);
	cfg_set(cfg, "service_port", sx);
	int x = cfg_load(cfg, "config.ini");
	if (x == EXIT_FAILURE) {
		printf("Unable to load config.ini\n");
	    save_config();
	}
	else {
		strcpy(pn_name, cfg_get(cfg, "PN_NAME"));
		strcpy(VendorName, cfg_get(cfg, "VendorName"));
		uint16_t val;
		strcpy(sx, cfg_get(cfg, "VendorID"));
		sscanf(sx, "%04x", &val);
		VendorID = val;
		strcpy(sx, cfg_get(cfg, "DeviceID"));
		sscanf(sx, "%04x", &val);
		DeviceID = val;
		strcpy(sx, cfg_get(cfg, "service_port"));
		sscanf(sx, "%d", &val);
		service_port = val;
	}
	return(0);
}

int pn_init() {
	int sockopt;
	char ifName[IFNAMSIZ]="";

	strcpy(ifName, cfg_get(cfg, "IF_NAME"));

	/* Open PF_PACKET socket, listening for EtherType ETHER_TYPE */
	if ((sockfd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) == -1)
	{
		perror("listener: socket");
		return -1;
	}

	/* Set interface to promiscuous mode - do we need to do this every time? */
	strncpy(ifopts.ifr_name, ifName, IFNAMSIZ - 1);

	ioctl(sockfd, SIOCGIFFLAGS, &ifopts);
	ifopts.ifr_flags |= IFF_PROMISC;
	ioctl(sockfd, SIOCSIFFLAGS, &ifopts);
	/* Allow the socket to be reused - incase connection is closed prematurely */
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof sockopt) == -1) {
		perror("setsockopt");
		close(sockfd);
		exit(EXIT_FAILURE);
	}

	ioctl(sockfd, SIOCGIFADDR, &ifopts);
	memcpy(ip_addr, &ifopts.ifr_ifru.ifru_addr.sa_data[2],4);

	ifindex = ifopts.ifr_ifindex;

	ioctl(sockfd, SIOCGIFHWADDR, &ifopts);
	memcpy(hw_addr, &ifopts.ifr_ifru.ifru_hwaddr.sa_data[0],6);

	ioctl(sockfd, SIOCGIFNETMASK, &ifopts);
	memcpy(ip_mask, &ifopts.ifr_ifru.ifru_netmask.sa_data[2],4);


	if ((sock_siemens=socket(AF_INET, SOCK_DGRAM, 0)) == -1)
	{
		perror("udp socket siemens");
		return -1;
	}
	si_siemens.sin_family = AF_INET;
	si_siemens.sin_port = htons(SIEMENS_PORT);
	si_siemens.sin_addr.s_addr = INADDR_ANY;

	if( bind(sock_siemens , (struct sockaddr*)&si_siemens, sizeof(si_siemens) ) == -1)
	{
		perror("bind socket");
		return -1;
	}
//	listen(sock_siemens,1);

	if ((sock_service=socket(AF_INET, SOCK_DGRAM, 0)) == -1)
	{
		perror("udp socket x");
		return -1;
	}
	si_service.sin_family = AF_INET;
	si_service.sin_port = htons(service_port);
	si_service.sin_addr.s_addr = INADDR_ANY;

	if( bind(sock_service , (struct sockaddr*)&si_service, sizeof(si_service) ) == -1)
	{
		perror("bind socket x");
		return -1;
	}

	return sockfd;
}

int pn_receive() {
	int num = recvfrom(sockfd, r_buf, BUF_SIZ, 0, NULL, NULL);
	return num;
}

uint16_t SWAP16(uint16_t dx) {
 uint16_t v = (dx >> 8) | (dx << 8);
 return(v);
}

uint32_t SWAP32(uint32_t x) {
	uint16_t low_word = SWAP16((uint16_t)(x & 0xFFFF));
	uint16_t high_word = SWAP16((uint16_t)((x >> 16) & 0xFFFF));
	uint32_t v = ((uint32_t)low_word << 16) & 0xffff0000U;
	v |= (uint32_t)high_word & 0x0000ffffU;
	return v;
}



uint8_t mac_equal(uint8_t* x, uint8_t* y) {
	for (int i=0; i < 6; i++)
		if (x[i] != y[i]) return(0);
	return(1);
}

uint8_t mac_broadcast(uint8_t* x) {
	for (int i=0; i <= 2; i++)
		if (x[i] != 0) return(0);
	return(1);
}

uint8_t mac_any(uint8_t* x) {
	for (int i=0; i < 6; i++)
		if (x[i] != 0xff) return(0);
	return(1);
}

uint8_t ControllerMacAddress[6];

extern uint16_t FrameIOCs;
extern uint8_t waitSendControl;
uint8_t timeSendControl = 0;

void make_control_udp(_rpc* rpc_s);

void pn_scan() {
	uint8_t buf[1000];
	static time_t tm_shm = 0;
	static time_t tm_lldp = 0;
	static time_t tm_ptcp = 0;
    static struct timespec tx_start, tx_end;
    double milliseconds;
    static uint8_t first_start = 1;
    static int res_fx;

    if (first_start) {
    	first_start = 0;
        clock_gettime(CLOCK_MONOTONIC, &tx_start);
    }

    clock_gettime(CLOCK_MONOTONIC, &tx_end);
    milliseconds = (tx_end.tv_sec - tx_start.tv_sec) * 1000.0 +
                   (tx_end.tv_nsec - tx_start.tv_nsec) / 1000000.0;

	int len = 0;

	char client_ip[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &(si_client.sin_addr), client_ip, INET_ADDRSTRLEN);

	len = recvfrom(sock_siemens, r_buf, BUF_SIZ, MSG_DONTWAIT, (struct sockaddr *)&si_client, &slen);
	if (len <= 0) len = recvfrom(sock_service, r_buf, BUF_SIZ, MSG_DONTWAIT, (struct sockaddr *)&si_client, &slen);
	if (len > 0)  {
//		printf("Socket received udp from ip:%s port:%d len:%d\n", client_ip, SWAP16(si_client.sin_port), len);
		int res = pn_udp_x(r_buf, s_buf);
		if (res > 0) {
			if (waitSendControl) {
				waitSendControl = 0;
				memcpy(buf, s_buf, 1000);
				timeSendControl = 1;
			}
			int res_x = sendto(sock_service, s_buf, res+80, 0, (struct sockaddr *)&si_client, slen);
			if (res_x < 0) perror("sendto failed");
			else res_fx = res+80;
		}
	}

	int numbytes = pn_receive();
	if (numbytes > 0) {

		uint16_t fType = 0;
		uint16_t* FrameID;
		len = 0;
		memset(s_buf, 0, sizeof(s_buf));
		eh = &r_buf;
		if (eh->ether_type == SWAP16(0x0806)) { // ARP
			FrameID = (uint16_t*)&eh->ether_type;
			FrameID++;
			len = pn_arp((uint8_t*)FrameID, &s_buf[14]);
		}
		else if (eh->ether_type == SWAP16(0x0800)) {
			FrameID = &eh->ether_type;
			_ipv4* ipv4 = (_ipv4*)(FrameID+1);
			if (ipv4->protocol == 0x01) { // ICMP
				len = pn_icmp(ipv4, &s_buf[14]);
			}
		}
		else if (eh->ether_type == SWAP16(0x8100)) {
			FrameID = (uint16_t*)&eh->ether_type;
			FrameID += 2;
			fType = *FrameID;
			FrameID++;
			printf("8100 %04x\n",*FrameID);
		}
		else {
			fType = eh->ether_type;
			FrameID = (uint16_t*)&eh->ether_type;
			FrameID++;
		}
		if (fType == SWAP16(0x8892)) {

//			printf("%04x\n",fType);

			if (mac_equal((uint8_t*)&eh->ether_dhost, hw_addr) || mac_broadcast((uint8_t*)&eh->ether_dhost[3])) {
				if ((FrameIOCs != 0) && (*FrameID == FrameIOCs)) {
					rt_is_received = 1;
					pn_is_init = 0;
					uint16_t* pp = FrameID; pp++;
					pn_rt_receive((uint8_t*)pp);
//					printf("+ %f\n",milliseconds);
				}
				else switch (SWAP16(*FrameID)) {
					case 0xfefd:
					case 0xfefe:
						uint16_t* x = FrameID; x++;
						len = pn_dcp((uint8_t*)x, &s_buf[14]);
						break;
				}
				memcpy(ControllerMacAddress,&eh->ether_shost, 6);
			}
		}
		if (len > 0) {
			ex = &s_buf;
			memcpy(ex->ether_dhost, eh->ether_shost,6);
			memcpy(ex->ether_shost, hw_addr,6);
			ex->ether_type = eh->ether_type;

			socket_address.sll_ifindex = ifindex;
			socket_address.sll_halen = ETH_ALEN;
			socket_address.sll_family =AF_PACKET;
			socket_address.sll_protocol = ex->ether_type;
			memcpy(&socket_address.sll_addr, ex->ether_dhost, 6);
			int num = sendto(sockfd, &s_buf, len+14, 0, (struct sockaddr*)&socket_address, sizeof(struct sockaddr_ll));
//			printf("Send: %d\n",num);
		}
	}
	time_t tm = time(0);
	if ((tm-tm_lldp) >= 5) {
		tm_lldp = tm;
		len = lldp(s_buf);
		socket_address.sll_ifindex = ifindex;
		socket_address.sll_halen = ETH_ALEN;
		socket_address.sll_family =AF_PACKET;
		socket_address.sll_protocol = eh->ether_type;
		memcpy(&socket_address.sll_addr, eh->ether_dhost, 6);
		int num = sendto(sockfd, &s_buf, len, 0, (struct sockaddr*)&socket_address, sizeof(struct sockaddr_ll));

	}
	if ((tm-tm_ptcp) >= 3) {
		tm_ptcp = tm;
		len = pn_ptcp(s_buf);
		ex = &s_buf;
		socket_address.sll_ifindex = ifindex;
		socket_address.sll_halen = ETH_ALEN;
		socket_address.sll_family =AF_PACKET;
		socket_address.sll_protocol = ex->ether_type;
		memcpy(&socket_address.sll_addr, ex->ether_dhost, 6);
		int num = sendto(sockfd, &s_buf, len, 0, (struct sockaddr*)&socket_address, sizeof(struct sockaddr_ll));
	}
	if (timeSendControl && rt_is_received) {
		timeSendControl = 0;
		memcpy(s_buf, buf, 1000);
		make_control_udp(s_buf);
		si_client.sin_port = SWAP16(SIEMENS_PORT);
		int res_x = sendto(sock_service, s_buf, res_fx, 0, (struct sockaddr *)&si_client, slen);
		if (res_x < 0) perror("sendto failed");
	}
	if ((pn_is_init && (milliseconds > 10)) || rt_is_received) {
		tm_lldp = tm;
		tm_ptcp = tm;
		if (rt_is_received) rt_is_received = 0;
		memcpy(&tx_start, &tx_end, sizeof(tx_start));
		memset(s_buf, 0, sizeof(s_buf));
		len = pn_rt(&s_buf[18]);
		ex = &s_buf;
		memcpy(ex->ether_dhost, ControllerMacAddress,6);
		memcpy(ex->ether_shost, hw_addr,6);
		uint16_t* vlan = (uint16_t*)&ex->ether_type;
		*vlan = SWAP16(0x8100); vlan++;
		*vlan = SWAP16(0xc000); vlan++;
		*vlan = SWAP16(0x8892);
		socket_address.sll_ifindex = ifindex;
		socket_address.sll_halen = ETH_ALEN;
		socket_address.sll_family =AF_PACKET;
		socket_address.sll_protocol = SWAP16(0x8892);//ex->ether_type;
		memcpy(&socket_address.sll_addr, ex->ether_dhost, 6);
		int num = sendto(sockfd, &s_buf, len+18, 0, (struct sockaddr*)&socket_address, sizeof(struct sockaddr_ll));
//		printf("- %d\n",num);
	}
}


