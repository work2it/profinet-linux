/*
 * pn_io.c
 *
 *  Created on: Dec 4, 2025
 *      Author: SergDeev
 */
#pragma pack(push, 1)

#include "profinet.h"

typedef struct {
	uint16_t BlockType;
	uint16_t BlockLength;
	uint8_t BlockVersionHigh;
	uint8_t BlockVersionLow;
} _BlockHeader;

typedef struct { // BlockType = 0x8101
	_BlockHeader Header;
	uint16_t ARType;
	uint8_t ARUUID[16];
	uint16_t SessionKey;
	uint8_t CMResponderMacAdd[6];
	uint16_t CMResponderUDPRTPort;
} _ARBlockRes;

typedef struct { // BlockType = 0x8102
	_BlockHeader Header;
	uint16_t IOCRType;
	uint16_t IOCRReference;
	uint16_t FrameID;
} _IOCRBlockRes;

typedef struct { // BlockType = 0x8103
	_BlockHeader Header;
	uint16_t AlarmCRType;
	uint16_t LocalAlarmReference;
	uint16_t MaxAlarmDataLength;
	uint8_t data[];
} _AlarmCRBlockRes;

typedef struct { // BlockType = 0x0101
	_BlockHeader Header;
	uint16_t ARType;
	uint8_t ARUUID[16];
	uint16_t SessionKey;
	uint8_t CMInitiatorMacAdd[6];
	uint8_t CMInitiatorObjectUUID[16];
	uint32_t ARProperties;
	uint16_t CMInitiatorActivityTimeoutFactor;
	uint16_t CMInitiatorUDPRTPort;
	uint16_t StationNameLength;
	char CMInitiatorStationName[];
} _ARBlockReq;

typedef struct {
	uint32_t API;
	uint16_t NumberOfDataObjects;
} _API_DataObject;

typedef struct { // BlockType = 0x0102
	_BlockHeader Header;
	uint16_t IOCRType;
	uint16_t IOCRReference;
	uint16_t LT;
	uint32_t IOCRProperties;
	uint16_t DataLength;
	uint16_t FrameID;
	uint16_t SendClockFactor;
	uint16_t ReductionRatio;
	uint16_t Phase;
	uint16_t Sequence;
	uint32_t FrameSendOffset;
	uint16_t WatchdogFactor;
	uint16_t DataHoldFactor;
	uint16_t IOCRTagHeader;
	uint8_t IOCRMulticastMACAdd[6];
	uint16_t NumberOfAPIs;
	_API_DataObject API;
} _IOCRBlockReq;

typedef struct {
	uint16_t SlotNumber;
	uint16_t SubslotNumber;
	uint16_t FrameOffset;
} _IODataObject;

typedef struct { // Структура IODataObject и IOCS из IOCRBlock
	uint8_t SlotType; // 0=IODataObject, 1=IOCS
	uint16_t SlotNumber; // номер слота
	uint16_t SubslotNumber; // номер под-слота
	uint16_t FrameOffset; // смещение в rt-буфере
} pIOCRBlock;

typedef struct {
	uint16_t DataDescription;
	uint16_t SubmoduleDataLength;
	uint8_t LenghtIOS;
	uint8_t LenghtIOPS;
} _ExpectDataDescription;

typedef struct {
	uint16_t SubslotNumber;
	uint32_t SubmoduleIdentNumber;
	uint16_t SubmoduleProperties;
	_ExpectDataDescription DataDescription;
} _ExpectSubmodule;

typedef struct { // BlockType = 0x0104
	_BlockHeader Header;
	uint16_t NumberOfAPIs;
	uint32_t API;
	uint16_t SlotNumber;
	uint32_t ModuleIdentNumber;
	uint16_t ModuleProperties;
	uint16_t NumberOfSubmodules;
	_ExpectSubmodule Submodule[];
} ExpectedSubmoduleBlockReq;

typedef struct { // BlockType = 0x0103
	_BlockHeader Header;
	uint16_t AlarmCRType;
	uint16_t LT;
	uint32_t  AlarmCRProperties;
	uint16_t RTATimeoutFactor;
	uint16_t RTARetries;
	uint16_t LocalAlarmReference;
	uint16_t MaxAlarmDataLength;
	uint16_t AlarmCRTagHeaderHigh;
	uint16_t AlarmCRTagHeaderLow;
} _AlarmCRBlockReq;

typedef struct {
	_BlockHeader Header;
	uint16_t SeqNumber;
	uint8_t ARUUID[16];
	uint32_t API;
	uint16_t SlotNumber;
	uint16_t SubslotNumber;
	uint16_t padding_2;
	uint16_t Index;
	uint32_t RecordDataLength;
	uint8_t Padding[24];
} _IOWriteReqHeader;

typedef struct {
	_BlockHeader Header;
	uint16_t SeqNumber;
	uint8_t ARUUID[16];
	uint32_t API;
	uint16_t SlotNumber;
	uint16_t SubslotNumber;
	uint16_t padding_2;
	uint16_t Index;
	uint32_t RecordDataLength;
	uint16_t AdditionValue1;
	uint16_t AdditionValue2;
	uint32_t Status;
	uint8_t Padding[16];
} _IOWriteResHeader;



pModules Modules[128], modules[128];
_IODataObject DataObjects[128]={0};
_IODataObject IOCs[128]={0};

_IOWriteReqHeader WriteReqData[128]={0};
uint16_t WriteReqIndex = 0;

// IOCRBlock содержит структуру RT-данных входов (от устройства в контроллер)
pIOCRBlock IOCRBlock_input[128+4]={0};
uint16_t numIOCRBlock_input=0; // количество модулей
uint16_t input_Block_len=0; // длина блока данных

// IOCRBlock_output содержит структуру RT-данных выходов (от контроллера в устройство)
pIOCRBlock IOCRBlock_output[128]={0};
uint16_t numIOCRBlock_output=0; // количество модулей
uint16_t output_Block_len=0; // длина блока данных

uint16_t numDataObjects, numIOCs, FrameDataObject, FrameIOCs;
uint8_t ar_uuid[16], isUpdated, pn_is_init;

uint16_t* findInputDataObject(uint16_t slot, uint16_t subslot) {
	for (int i=0; i <= numIOCRBlock_input; i++) {
		if (IOCRBlock_input[i].SlotType == 0)
		if (IOCRBlock_input[i].SlotNumber == slot)
		if (IOCRBlock_input[i].SubslotNumber == subslot) return(&IOCRBlock_input[i].FrameOffset);
	}
	return NULL;
}

uint16_t* findOutputDataObject(uint16_t slot, uint16_t subslot) {
	for (int i=0; i <= numIOCRBlock_output; i++) {
		if (IOCRBlock_output[i].SlotType == 0)
		if (IOCRBlock_output[i].SlotNumber == slot)
		if (IOCRBlock_output[i].SubslotNumber == subslot) return(&IOCRBlock_output[i].FrameOffset);
	}
	return NULL;
}

uint16_t* findInputIOCS(uint16_t slot, uint16_t subslot) {
	for (int i=0; i <= numIOCRBlock_input; i++) {
		if (IOCRBlock_input[i].SlotType == 1)
		if (IOCRBlock_input[i].SlotNumber == slot)
		if (IOCRBlock_input[i].SubslotNumber == subslot) return(&IOCRBlock_input[i].FrameOffset);
	}
	return NULL;
}

uint16_t* findOutputIOCS(uint16_t slot, uint16_t subslot) {
	for (int i=0; i <= numIOCRBlock_output; i++) {
		if (IOCRBlock_output[i].SlotType == 1)
		if (IOCRBlock_output[i].SlotNumber == slot)
		if (IOCRBlock_output[i].SubslotNumber == subslot) return(&IOCRBlock_output[i].FrameOffset);
	}
	return NULL;
}

typedef struct {
	_BlockHeader Header;
	uint16_t Reserved_1;
	uint8_t ARUUID[16];
	uint16_t SessionKey;
	uint16_t Reserved_2;
	uint16_t ControlCommand;
	uint16_t ControlBlockProperties;
} _IODControl;

typedef struct {
	_BlockHeader Header;
	uint16_t SeqNumber;
	uint8_t ARUUID[16];
	uint32_t API;
	uint16_t SlotNumber;
	uint16_t SubslotNumber;
	uint8_t padding_2[2];
	uint16_t Index;
	uint32_t RecordDataLength;
	uint8_t padding_24[24];
} _IOReadReqHeader;

typedef struct {
	_BlockHeader Header;
	uint16_t SeqNumber;
	uint8_t ARUUID[16];
	uint32_t API;
	uint16_t SlotNumber;
	uint16_t SubslotNumber;
	uint8_t padding_2[2];
	uint16_t Index;
	uint32_t RecordDataLength;
	uint16_t AdditionValue1;
	uint16_t AdditionValue2;
	uint8_t padding_20[20];
} _IOReadResHeader;

typedef struct {
	_BlockHeader Header;
	uint16_t VendorID;
	char OrderID[20];
	char IMSerialNumber[16];
	uint16_t IMHardwareRevision;
	char IMRevisionPrefix;
	uint8_t IMSWRevisionFunctionalEnhancement;
	uint8_t IM_SWRevisionBugFix;
	uint8_t IMSWRevisionInternalChange;
	uint16_t IMRevisionCounter;
	uint16_t IMProfileID;
	uint16_t IMProfileSpecificType;
	uint8_t IMVersionMajor;
	uint8_t IMVersionMinor;
	uint16_t IM_Supported;
} _I_M0;

typedef struct {
	uint16_t SlotNumber;
	uint32_t ModuleIdentNumber;
	uint16_t NumberOfSubmodules;
	uint16_t SubslotNumber;
	uint32_t SubmoduleIdentNumber;
} _Subslot;

typedef struct {
	_BlockHeader Header;
	uint16_t NumberOfAPIs;
	uint32_t API;
	uint16_t NumberOfModules;
	_Subslot Subslot[];
} _I_M0_FilterDataModule;

typedef struct {
	_BlockHeader Header;
	uint16_t padding;
	uint8_t MRP_DomainUUID[16];
	uint16_t MRP_Role;
	uint16_t MRP_Version;
	uint8_t MRP_LengthDomainName;
	uint8_t MRP_DomainName[];
} _PDInterfaceMrpDataReal;

typedef struct {
	_BlockHeader Header;
	uint8_t LenghtOwnChassisID;
	uint8_t OwnChassisID[9];
	uint8_t MACAddress[6];
	uint16_t padding;
	uint8_t IPAddress[4];
	uint8_t Subnetmask[4];
	uint8_t Gateway[4];
} _PDInterfaceDataReal;

typedef struct {
	_BlockHeader Header;
	uint16_t padding;
	uint16_t SlotNumber;
	uint16_t SubslotNumber;
	uint8_t LengthOwnPortID;
	uint8_t OwnPortID[8];
	uint8_t NumberOfPeers;
	uint16_t padding1;
} _PDPortDataReal;

typedef struct {
	_BlockHeader Header;
	uint16_t padding;
	uint32_t API;
	uint16_t SlotNumber;
	uint16_t SubslotNumber;
} _MultipleBlockHeader;

typedef struct {
	uint16_t SubslotNumber;
	uint32_t SubmoduleIdentNumber;
} _Slot_Subslot;

typedef struct {
	uint16_t SlotNumber;
	uint32_t ModuleIdentNumber;
	uint16_t NumberOfSubslots;
} _Slot;

typedef struct {
	_BlockHeader Header;
	uint16_t NumberOfAPIs;
	uint32_t API;
	uint16_t NumberOfSlots;
} _RealIdentificationData;

typedef struct {
	uint16_t IOCRType;
	uint32_t IOCRProperties;
	uint16_t FRameID;
	uint16_t CycleCounter;
	uint8_t DataStatus;
	uint8_t TransferStatus;
	uint16_t CMInitiatorUDPRTPort;
	uint16_t CMResponderUDPRTPort;
} _IOCR;

typedef struct {
	_BlockHeader Header;
	uint16_t NumberOfARs;
	uint8_t ARUUID[16];
	uint16_t ARType;
	uint32_t ARProperties;
	uint8_t CMInitiatorObjectUUID[16];
	uint16_t StationNameLength;
	uint8_t CMInitiatorStationName[5];
	uint16_t NumberOfIOCs;
	_IOCR IOCR_Input;
	_IOCR IOCR_Output;
	uint16_t AlarmCRType;
	uint16_t LocalAlarmReference;
	uint16_t RemoteAlarmReference;
	uint8_t ParameterServerObjectUUID[16];
	uint16_t Station_NameLength;
	uint16_t NumberOfAPIs;
	uint32_t API;
} _ARDATA;

int numModules;
pModules Modules[128];

void insertUserData(uint16_t slot, uint16_t subslot, uint16_t len, uint8_t* data) {
	for (int i=0; i <= numModules; i++) {
		if ((Modules[i].Slot == slot) && (Modules[i].Subslot == subslot)) {
			if (len > 16) len = 16;
			Modules[i].userDataLen = len;
			memcpy(Modules[i].userData, data, len);
			return;
		}
	}
}

typedef struct {
	uint32_t id;
	char order[20];
	char serial[16];
	uint16_t hwRevision;
	char prefix;
	uint8_t swRevision;
	uint8_t swBugFix;
	uint8_t swChange;
	uint16_t count;
} _device;

_device devices[3]= {{0x10000000,"MX-IM-001","MX IM-001 000",0x0001,'V',0x01,0x00,0x00,0x00,0x0000},
		{0x00000001,"MX-8DI-001","MX DI-001 000",1,'V',0x01,0x00,0x00,0x00,0x0000},
		{0x00000101,"MX-8DQ-001","MX DQ-001 000",1,'V',0x01,0x00,0x00,0x00,0x0000}};

char OrderID[20] = "VIRT SX-001";
char IM_SerialNumber[16] = "SX-IM-001";
uint16_t IM_HW_Revision = 0x0008;
char IM_RevisionPrefix = 'V';
char OwnChassiID[16] = "virtual-x";

int pn_io_read(_pn_io* pn_io_r, _pn_io* pn_io_s) {
	int len = 0;
	_IOReadReqHeader * req = (_IOReadReqHeader*)(pn_io_r+1);
	  pn_io_s->MaximumCount = pn_io_r->MaximumCount;
	  _IOReadResHeader* res = (_IOReadResHeader*)(pn_io_s+1);
	  res->Header.BlockType = 0x0980;
	  res->Header.BlockLength = SWAP16(60);
	  res->Header.BlockVersionHigh = 1;
	  res->SeqNumber = req->SeqNumber;
	  //	  memcpy(&res->ARUUID, &ar_uuid, 16);
	  memcpy(&res->ARUUID, &req->ARUUID, 16);
	  res->API = req->API;
	  res->SlotNumber = req->SlotNumber;
	  res->SubslotNumber = req->SubslotNumber;

	  uint8_t* pp = (uint8_t*)(res+1);

	  if (req->Index == 0x40f8) { // I&M0FilterData
		  res->Index = req->Index;
		  _I_M0_FilterDataModule * fx1 = (_I_M0_FilterDataModule*)pp;
		  fx1->Header.BlockType = 0x3000;
		  fx1->Header.BlockVersionHigh = 1;
		  fx1->NumberOfAPIs = 0x0100;
		  fx1->NumberOfModules = 0; //
		  int k1 = 0;
		  for (int i=0; i<numModules; i++) {
			  if (Modules[i].Subslot == 1) {
				  fx1->Subslot[k1].SlotNumber = SWAP16(Modules[i].Slot);
				  fx1->Subslot[k1].ModuleIdentNumber = SWAP32(Modules[i].ModuleIdentNumber);
				  fx1->Subslot[k1].NumberOfSubmodules = 0x0100;
				  fx1->Subslot[k1].SubslotNumber = 0x0100;
				  uint16_t n = SWAP16(fx1->NumberOfModules); n++;
				  fx1->NumberOfModules = SWAP16(n);
				  k1++;
			  }
		  }
		  pp = (uint8_t*)&fx1->Subslot[k1];
		  uint16_t ln = pp - (uint8_t*)fx1;
		  fx1->Header.BlockLength = SWAP16(ln-4);

		  _I_M0_FilterDataModule * fx2 = (_I_M0_FilterDataModule*)pp;
		  fx2->Header.BlockType = 0x3100;
		  fx2->Header.BlockVersionHigh = 1;
		  fx2->NumberOfAPIs = 0x0100;
		  fx2->NumberOfModules = 0; //
		  int k2 = 0;
		  for (int i=0; i<numModules; i++) {
			  if (Modules[i].Subslot == 1) {
				  fx2->Subslot[k2].SlotNumber = SWAP16(Modules[i].Slot);
			  	  fx2->Subslot[k2].ModuleIdentNumber = SWAP32(Modules[i].ModuleIdentNumber);
			  	  fx2->Subslot[k2].NumberOfSubmodules = 0x0100;
			  	  fx2->Subslot[k2].SubslotNumber = 0x0100;
				  uint16_t n = SWAP16(fx2->NumberOfModules); n++;
				  fx2->NumberOfModules = SWAP16(n);
				  k2++;
		  	  }
		  }
		  pp = (uint8_t*)&fx2->Subslot[k2];
		  ln = pp - (uint8_t*)fx2;
		  fx2->Header.BlockLength = SWAP16(ln-4);

		  _I_M0_FilterDataModule * fx3 = (_I_M0_FilterDataModule*)pp;
		  fx3->Header.BlockType = 0x3200;
		  fx3->Header.BlockVersionHigh = 1;
		  fx3->NumberOfAPIs = 0x0100;
		  fx3->NumberOfModules = 0; //
		  int k3 = 0;
		  for (int i=0; i<numModules; i++) {
			  if ((Modules[i].Slot == 0) && (Modules[i].Subslot == 1)) {
				  fx3->Subslot[k3].SlotNumber = SWAP16(Modules[i].Slot);
			  	  fx3->Subslot[k3].ModuleIdentNumber = SWAP32(Modules[i].ModuleIdentNumber);
			  	  fx3->Subslot[k3].NumberOfSubmodules = 0x0100;
			  	  fx3->Subslot[k3].SubslotNumber = 0x0100;
				  uint16_t n = SWAP16(fx3->NumberOfModules); n++;
				  fx3->NumberOfModules = SWAP16(n);
				  k3++;
		  	  }
		  }
		  pp = (uint8_t*)&fx3->Subslot[k3];
		  ln = pp - (uint8_t*)fx3;
		  fx3->Header.BlockLength = SWAP16(ln-4);

		  uint32_t len_32 = pp - (uint8_t*)fx1;
		  res->RecordDataLength = SWAP32(len_32);

		  len_32 = pp - (uint8_t*)res;
		  pn_io_s->ActualCount = pn_io_s->ArgsLength = len_32;
	  }
	  else if ((req->Index == 0x41f8) || (req->Index == 0x42f8)) {
		  res->Index = req->Index;
		  uint8_t* p0 = pp;
		  for (int i=1; i < numModules; i++) {
			  if (Modules[i].Slot > 0) break;
			  _MultipleBlockHeader* mb = (_MultipleBlockHeader*)pp;
			  mb->Header.BlockType = 0x0004;
			  mb->Header.BlockVersionHigh = 1;
			  mb->API = 0;
			  mb->SlotNumber = 0;
			  mb->SubslotNumber = SWAP16(Modules[i].Subslot);
			  mb->Header.BlockLength = SWAP16(12);
			  pp = (uint8_t*)(mb+1);
			  if (Modules[i].Subslot == 0x8000) {
				  _PDInterfaceMrpDataReal* px = (_PDInterfaceMrpDataReal*)pp;
				  px->Header.BlockType = SWAP16(0x0212);
				  px->Header.BlockLength = SWAP16(36);
				  px->Header.BlockVersionHigh = 1;
				  px->Header.BlockVersionLow = 1;
				  memset(&px->MRP_DomainUUID, 0xff, 16);
				  px->MRP_Role = 0;
				  px->MRP_Version = SWAP16(1);
				  px->MRP_LengthDomainName = 11;
				  memcpy(&px->MRP_DomainName, "mrpdomain-1", 11);
				  pp = &px->MRP_DomainName; pp += 11;

				  _PDInterfaceDataReal* pr = (_PDInterfaceDataReal*)pp;
				  pr->Header.BlockType = SWAP16(0x0240);
				  pr->Header.BlockLength = SWAP16(32);
				  pr->Header.BlockVersionHigh = 1;
				  pr->Header.BlockVersionLow = 0;
				  pr->LenghtOwnChassisID = 9;
				  memcpy(&pr->OwnChassisID, pn_name, 9);
				  memcpy(&pr->MACAddress, hw_addr, 6);
				  memcpy(&pr->IPAddress, ip_addr, 4);
				  memcpy(&pr->Subnetmask, ip_mask, 4);
				  memcpy(&pr->Gateway, ip_addr, 4);

				  pp = &pr->Gateway; pp += 4;

				  uint16_t ln = pp - (uint8_t*)mb;
				  mb->Header.BlockLength = SWAP16(ln-4);
			  }
			  else {
			  }
		  }

		  uint32_t len_32 = pp - p0;
		  res->RecordDataLength = SWAP32(len_32);

		  len_32 = pp - (uint8_t*)res;
		  pn_io_s->ActualCount = pn_io_s->ArgsLength = len_32;
	  }
	  else if (req->Index == 0xf0af) { // I&M0
		  res->Index = 0xf0af;
		  uint32_t ident=0;
		  int idx = 0;
		  for (int i=0; i<numModules; i++) {
			  if ((Modules[i].Slot == SWAP16(req->SlotNumber)) && (Modules[i].Subslot == SWAP16(req->SubslotNumber))) {
				  ident = Modules[i].ModuleIdentNumber;
				  break;
			  }
		  }
		  int x = sizeof(devices)/sizeof(_device);
		  for (int i=0; i<x; i++) {
			  if (devices[i].id == ident) {
				  idx = i;
				  break;
			  }
		  }
		  _I_M0 * fx = (_I_M0*)pp;
		  fx->Header.BlockType = SWAP16(0x0020);
		  fx->Header.BlockLength = SWAP16(56);
		  fx->Header.BlockVersionHigh = 1;
		  fx->VendorID = SWAP16(VendorID);
		  memcpy(&fx->OrderID, devices[idx].order, 20);
		  memcpy(&fx->IMSerialNumber, devices[idx].serial, 16);
		  fx->IMHardwareRevision = SWAP16(devices[idx].hwRevision);
		  fx->IMRevisionPrefix = devices[idx].prefix;
		  fx->IMSWRevisionFunctionalEnhancement = 0;
		  fx->IMSWRevisionInternalChange = 0;
		  fx->IMProfileSpecificType = SWAP16(0x0003);
		  fx->IMVersionMajor = 1;
		  fx->IMVersionMinor = 1;
		  fx->IM_Supported = 0x0e00;

		  pp = (uint8_t*)(fx+1);

		  uint32_t len_32 = pp - (uint8_t*)fx;
		  res->RecordDataLength = SWAP32(len_32);

		  len_32 = pp - (uint8_t*)res;
		  pn_io_s->ActualCount = pn_io_s->ArgsLength = len_32;
	  }
	  else if (req->Index == 0xf1af) {
		  res->Index = 0xf1af;
		  _I_M0 * fx = (_I_M0*)pp;
		  fx->Header.BlockType = SWAP16(0x0021);
		  fx->Header.BlockLength = SWAP16(56);
		  fx->Header.BlockVersionHigh = 1;
		  fx->Header.BlockVersionLow = 0;

		  pp = &fx->Header.BlockVersionLow; pp++;

		  memset(pp, 0x20, 54); pp +=54;

		  uint32_t len_32 = pp - (uint8_t*)fx;
		  res->RecordDataLength = SWAP32(len_32);

		  len_32 = pp - (uint8_t*)res;
		  pn_io_s->ActualCount = pn_io_s->ArgsLength = len_32;
	  }
	  else if (req->Index == 0xf2af) {
		  res->Index = 0xf2af;
		  _I_M0 * fx = (_I_M0*)pp;
		  fx->Header.BlockType = SWAP16(0x0022);
		  fx->Header.BlockLength = SWAP16(18);
		  fx->Header.BlockVersionHigh = 1;
		  fx->Header.BlockVersionLow = 0;

		  pp = &fx->Header.BlockVersionLow; pp++;

		  memset(pp, 0x20, 16); pp +=16;

		  uint32_t len_32 = pp - (uint8_t*)fx;
		  res->RecordDataLength = SWAP32(len_32);

		  len_32 = pp - (uint8_t*)res;
		  pn_io_s->ActualCount = pn_io_s->ArgsLength = len_32;
	  }
	  else if (req->Index == 0xf3af) {
		  res->Index = 0xf3af;
		  _I_M0 * fx = (_I_M0*)pp;
		  fx->Header.BlockType = SWAP16(0x0023);
		  fx->Header.BlockLength = SWAP16(56);
		  fx->Header.BlockVersionHigh = 1;
		  fx->Header.BlockVersionLow = 0;

		  pp = &fx->Header.BlockVersionLow; pp++;

		  memset(pp, 0x20, 54); pp +=54;

		  uint32_t len_32 = pp - (uint8_t*)fx;
		  res->RecordDataLength = SWAP32(len_32);

		  len_32 = pp - (uint8_t*)res;
		  pn_io_s->ActualCount = pn_io_s->ArgsLength = len_32;
	  }
	  else if ((req->Index == 0x0cc0) || (req->Index == 0x02e0)) {
		  res->Index = req->Index;
		  res->RecordDataLength = 0;
		  res->AdditionValue1 = 0;
		  res->AdditionValue2 = 0;

		  memset(res->padding_20, 0x00, 20);

		  uint32_t len_32 = pp - (uint8_t*)res;
		  pn_io_s->ActualCount = pn_io_s->ArgsLength = len_32;
	  }
	  else if ((req->Index == 0x00f0) || (req->Index == 0x01e0) || (req->Index == 0x0c80) || (req->Index == 0x31f8)) {
		  res->Index = req->Index;
		  uint8_t* p0 = pp;
		  _RealIdentificationData* rl = (_RealIdentificationData*)pp;
		  rl->Header.BlockType = 0x1300;
		  rl->Header.BlockVersionHigh = 1;
		  rl->Header.BlockVersionLow = 1;
		  rl->NumberOfAPIs = 0x0100;
		  rl->API = 0;
		  rl->NumberOfSlots = 1;
		  uint8_t* px = (uint8_t*)(rl+1);
		  _Slot* slot;
		  for (int i=0; i<numModules; i++) {
			  if ((i==0) || (Modules[i].Slot != Modules[i-1].Slot)) {
				  if (i != 0) {
					  slot->NumberOfSubslots = SWAP16(slot->NumberOfSubslots);
					  rl->NumberOfSlots++;
				  }
				  slot = (_Slot*)px;
				  slot->SlotNumber = SWAP16(Modules[i].Slot);
				  slot->ModuleIdentNumber = SWAP32(Modules[i].ModuleIdentNumber);
				  slot->NumberOfSubslots = 0;
				  px = (uint8_t*)(slot+1);
			  }
			  else {
				  _Slot_Subslot* subs = (_Slot_Subslot*)px;
				  subs->SubslotNumber = SWAP16(Modules[i].Subslot);
				  subs->SubmoduleIdentNumber = SWAP32(Modules[i].SubmoduleIdentNumber);
				  slot->NumberOfSubslots++;
				  px = (uint8_t*)(subs+1);
			  }
		  }
		  rl->NumberOfSlots = SWAP16(rl->NumberOfSlots);
		  pp = px;
		  uint16_t ln = pp - p0;
		  rl->Header.BlockLength = SWAP16(ln-4);

		  uint32_t len_32 = pp - p0;
		  res->RecordDataLength = SWAP32(len_32);

		  len_32 = pp - (uint8_t*)res;
		  pn_io_s->ActualCount = pn_io_s->ArgsLength = len_32;
	  }
	  else if (req->Index == 0x20f8) { //
		  res->Index = req->Index;

		  _ARDATA * ax = (_ARDATA*)pp;
		  ax->Header.BlockType = 0x1800;
		  ax->Header.BlockVersionHigh = 1;

		  ax->NumberOfARs = SWAP16(1);
		  memcpy(&ax->ARUUID,&req->ARUUID, 16);
		  ax->ARType = SWAP16(1);
		  ax->ARProperties = SWAP32(0x11);
		  memcpy(&ax->CMInitiatorObjectUUID,&ar_uuid,16);
		  ax->StationNameLength = SWAP16(5);
		  memset(&ax->CMInitiatorStationName,0x20,5);

		  pp = (uint8_t*)(ax+1);

		  uint16_t len = pp-(uint8_t*)ax;
		  ax->Header.BlockLength = SWAP16(len-4);

		  uint32_t len_32 = len;
		  res->RecordDataLength = SWAP32(len_32);

		  len_32 = pp - (uint8_t*)res;
		  pn_io_s->ActualCount = pn_io_s->ArgsLength = len_32;
	  }
	  else {
		  res->Index = req->Index;

		  uint32_t len_32 = pp - (uint8_t*)res;
		  res->RecordDataLength = SWAP32(len_32);

		  len_32 = pp - (uint8_t*)res;
		  pn_io_s->ActualCount = pn_io_s->ArgsLength = len_32;
	  }

	  len = pp - (uint8_t*)res;

	return len;
}

uint8_t waitSendControl=0;

void make_control_io(_pn_io* pn_io_s) {
	_IODControl * IODControlRes = (_IODControl*)(pn_io_s+1);
	IODControlRes->Header.BlockType = SWAP16(0x0112);
	IODControlRes->ControlCommand = SWAP16(0x0002);
}

int pn_io_control(_pn_io* pn_io_r, _pn_io* pn_io_s) {

	pn_io_s->MaximumCount = pn_io_r->MaximumCount;
	pn_io_s->ArgsLength = pn_io_r->ArgsLength;
	pn_io_s->ActualCount = 32;

	_IODControl * IODControlRes = (_IODControl*)(pn_io_s+1);
	_IODControl * IODControlReq = (_IODControl*)(pn_io_r+1);
	uint16_t x = SWAP16(IODControlReq->Header.BlockType);
	uint16_t block_len_16 = 0;
	if (x == 0x0110) {
		x |= 0x8000;
		IODControlRes->Header.BlockType = SWAP16(x);
		IODControlRes->Header.BlockLength = SWAP16(28);
		IODControlRes->Header.BlockVersionHigh = 1;
		memcpy(IODControlRes->ARUUID, IODControlReq->ARUUID,16);
		IODControlRes->SessionKey = IODControlReq->SessionKey;
		IODControlRes->ControlCommand = SWAP16(0x0008);

		block_len_16 = sizeof(_IODControl);

		waitSendControl = 1;
	}

	return block_len_16;
}


int pn_io_write(_pn_io* pn_io_r, _pn_io* pn_io_s) {
	uint32_t max_count = pn_io_r->MaximumCount;
	uint32_t current_count = 0;
	memset(WriteReqData, 0, sizeof(WriteReqData));
	WriteReqIndex = 0;
	_IOWriteReqHeader * wh = (_IOWriteReqHeader*)(pn_io_r+1);
	uint8_t * pp = (uint8_t*)wh;
	while (current_count < max_count) {
		if (wh->Header.BlockType == 0x0800) {
			uint16_t block_len = SWAP16(wh->Header.BlockLength);
			uint16_t block_index = SWAP16(wh->Index);
			uint32_t dataLen = SWAP32(wh->RecordDataLength);
			memcpy(&WriteReqData[WriteReqIndex], wh, sizeof(_IOWriteReqHeader));
			WriteReqIndex++;
			if (block_index == 0xe040) { // MultipleWrite
				dataLen = 0;
			}
			else if (block_index == 0x007d) {
				uint8_t* x = (uint8_t*)&wh->Padding; x += 24;
				insertUserData(SWAP16(wh->SlotNumber), SWAP16(wh->SubslotNumber), (uint16_t)dataLen, x);
				if (dataLen%2) dataLen++;
			}
			current_count += block_len + 4 + dataLen;
			pp += block_len + 4 + dataLen;
		}
		else break;
		if (*(uint16_t*)pp == 0) {
			pp += 2;
			current_count += 2;
		}
		wh = (_IOWriteReqHeader*)pp;
	  }

// ***** формирование ответа Write response

	pn_io_s->MaximumCount = pn_io_r->MaximumCount;

	_IOWriteResHeader * wh_s = (_IOWriteResHeader*)(pn_io_s+1);
	_IOWriteResHeader * wh0 = wh_s;
	for (int i=0; i<WriteReqIndex; i++) {
		memcpy(wh_s, &WriteReqData[i], sizeof(_IOWriteReqHeader));
		wh_s->Header.BlockType = SWAP16(0x8008);
		wh_s->Header.BlockLength = SWAP16(60);
		wh_s->RecordDataLength = 0;
		wh_s++;
	}

	uint8_t * block_end = (uint8_t*)wh_s;

	pp = (uint8_t*)wh0;
	uint16_t block_len_16 = (uint16_t)(block_end-pp);
	uint32_t block_len_32 = (uint32_t)block_len_16;
	pn_io_s->ArgsLength = pn_io_s->ActualCount = block_len_32;
	wh0->RecordDataLength = SWAP32(block_len_32-sizeof(_IOWriteResHeader));

	pn_io_s->ArgsLength = block_len_32;
	pn_io_s->ActualCount = block_len_32;

	return block_len_16;
}

int pn_io_connect(_pn_io* pn_io_r, _pn_io* pn_io_s) {
	memset(&modules, 0, sizeof(modules));
	numModules =0;

	memset(&DataObjects, 0, sizeof(DataObjects));
	numDataObjects = 0;

	memset(&IOCs, 0, sizeof(IOCs));
	numIOCs = 0;

	memset(&IOCRBlock_input, 0, sizeof(IOCRBlock_input));
	memset(&IOCRBlock_output, 0, sizeof(IOCRBlock_output));

	// **** разбор ARBlockReq
	_ARBlockReq * arblock_req = (_ARBlockReq*)(pn_io_r+1);
	memcpy(&ar_uuid, &arblock_req->CMInitiatorObjectUUID, 16); // ar_uuid требуется для Control request от устройства
	uint16_t ln = SWAP16(arblock_req->Header.BlockLength); ln += 4;
	uint8_t * pp = (uint8_t*)arblock_req; pp += ln;

	// **** разбор первого блока IOCRBlockReq (входа)
	_IOCRBlockReq * iocrblock_req = (_IOCRBlockReq*)pp;
	memset(IOCRBlock_input,0, sizeof(IOCRBlock_input));
	FrameDataObject = iocrblock_req->FrameID;
	numDataObjects = SWAP16(iocrblock_req->API.NumberOfDataObjects);
	input_Block_len = SWAP16(iocrblock_req->DataLength);
	_IOCRBlockReq* tmp = iocrblock_req; tmp++;
	_IODataObject* dataobject = (_IODataObject*)tmp;
	for (int i=0; i<numDataObjects; i++) {
		IOCRBlock_input[i].SlotNumber = SWAP16(dataobject->SlotNumber);
		IOCRBlock_input[i].SubslotNumber = SWAP16(dataobject->SubslotNumber);
		IOCRBlock_input[i].FrameOffset = SWAP16(dataobject->FrameOffset);
		IOCRBlock_input[i].SlotType = 0; // IODataObject
		dataobject++;
	}
	uint16_t * pNum = (uint16_t*)dataobject;
	uint16_t numIOCS = SWAP16(*pNum);
	pNum++;
	_IODataObject * iocs = (_IODataObject*)pNum;
	for (int i=0; i<numIOCS; i++) {
		IOCRBlock_input[numDataObjects+i].SlotNumber = SWAP16(iocs->SlotNumber);
		IOCRBlock_input[numDataObjects+i].SubslotNumber = SWAP16(iocs->SubslotNumber);
		IOCRBlock_input[numDataObjects+i].FrameOffset = SWAP16(iocs->FrameOffset);
		IOCRBlock_input[numDataObjects+i].SlotType = 1; // IOCS
		iocs++;
	}
	numIOCRBlock_input=numDataObjects+numIOCS;

	ln = SWAP16(iocrblock_req->Header.BlockLength); ln += 4;
	pp = (uint8_t*)iocrblock_req; pp += ln;

	// **** разбор второго блока IOCRBlockReq (выхода)
	_IOCRBlockReq * iocrblock_req_1 = (_IOCRBlockReq*)pp;
	memset(IOCRBlock_output,0, sizeof(IOCRBlock_output));
	FrameIOCs = iocrblock_req_1->FrameID;
	numDataObjects = SWAP16(iocrblock_req_1->API.NumberOfDataObjects);
	output_Block_len = SWAP16(iocrblock_req_1->DataLength);
	tmp = iocrblock_req_1; tmp++;
	dataobject = (_IODataObject*)tmp;
	for (int i=0; i<numDataObjects; i++) {
		IOCRBlock_output[i].SlotNumber = SWAP16(dataobject->SlotNumber);
		IOCRBlock_output[i].SubslotNumber = SWAP16(dataobject->SubslotNumber);
		IOCRBlock_output[i].FrameOffset = SWAP16(dataobject->FrameOffset);
		IOCRBlock_output[i].SlotType = 0; // DataObject
		dataobject++;
	}
	pNum = (uint16_t*)dataobject;
	numIOCS = SWAP16(*pNum);
	pNum++;
	iocs = (_IODataObject*)pNum;
	for (int i=0; i<numIOCS; i++) {
		IOCRBlock_output[numDataObjects+i].SlotNumber = SWAP16(iocs->SlotNumber);
		IOCRBlock_output[numDataObjects+i].SubslotNumber = SWAP16(iocs->SubslotNumber);
		IOCRBlock_output[numDataObjects+i].FrameOffset = SWAP16(iocs->FrameOffset);
		IOCRBlock_output[numDataObjects+i].SlotType = 1; // IOCS
		iocs++;
	}
	numIOCRBlock_output=numDataObjects+numIOCS;

	ln = SWAP16(iocrblock_req_1->Header.BlockLength); ln += 4;
	pp = (uint8_t*)iocrblock_req_1; pp += ln;

	// **** разбор списка ExpectedSubmoduleBlockReq
	memset(Modules, 0 , sizeof(Modules));
	ExpectedSubmoduleBlockReq * module_req = (ExpectedSubmoduleBlockReq*)pp;
	numModules = 0;
	while (module_req->Header.BlockType == SWAP16(0x0104)) {
		uint16_t slot=SWAP16(module_req->SlotNumber);
		uint16_t subslots=SWAP16(module_req->NumberOfSubmodules);
		uint32_t ModuleIdentNumber=SWAP32(module_req->ModuleIdentNumber);
		for (int i=0; i<subslots; i++) {
			Modules[numModules].data.State[0] = 0x80;
			Modules[numModules].Slot = slot;
			Modules[numModules].Subslot = SWAP16(module_req->Submodule[i].SubslotNumber);
			Modules[numModules].ModuleIdentNumber = ModuleIdentNumber;
			Modules[numModules].SubmoduleIdentNumber = SWAP32(module_req->Submodule[i].SubmoduleIdentNumber);
			Modules[numModules].SubmoduleProperties = SWAP16(module_req->Submodule[i].SubmoduleProperties);
			Modules[numModules].DataDescription = SWAP16(module_req->Submodule[i].DataDescription.DataDescription);
			Modules[numModules].SubmoduleDataLength = SWAP16(module_req->Submodule[i].DataDescription.SubmoduleDataLength);
			Modules[numModules].inputDataObject = findInputDataObject(Modules[numModules].Slot, Modules[numModules].Subslot);
			Modules[numModules].inputIOCS = findInputIOCS(Modules[numModules].Slot, Modules[numModules].Subslot);
			Modules[numModules].outputDataObject = findOutputDataObject(Modules[numModules].Slot, Modules[numModules].Subslot);
			Modules[numModules].outputIOCS = findOutputIOCS(Modules[numModules].Slot, Modules[numModules].Subslot);
			numModules++;
			if (numModules > input_Block_len) break;
		}
		ln = SWAP16(module_req->Header.BlockLength); ln += 4;
		pp = (uint8_t*)module_req; pp += ln;
		module_req = (ExpectedSubmoduleBlockReq*)pp;
	}
	_AlarmCRBlockReq * alarm_req = (_AlarmCRBlockReq*)module_req;
	isUpdated = 1;

	// Формирование ответа на запрос соединения

	pn_io_s->MaximumCount = pn_io_r->MaximumCount;

	_ARBlockRes * arblock_res = (_ARBlockRes*)(pn_io_s+1);
	arblock_res->Header.BlockType = SWAP16(0x8101);
	arblock_res->Header.BlockLength = SWAP16(30);
	arblock_res->Header.BlockVersionHigh = 1;
	arblock_res->ARType = SWAP16(0x0001);
	memcpy(arblock_res->ARUUID, arblock_req->ARUUID,16);
	arblock_res->SessionKey = arblock_req->SessionKey;
	memcpy(arblock_res->CMResponderMacAdd, &hw_addr, 6);
	arblock_res->CMResponderUDPRTPort = arblock_req->CMInitiatorUDPRTPort;

	_IOCRBlockRes * iocrblock_res = (_IOCRBlockRes*)(arblock_res+1);
	iocrblock_res->Header.BlockType = SWAP16(0x8102);
	iocrblock_res->Header.BlockLength = SWAP16(8);
	iocrblock_res->Header.BlockVersionHigh = 1;
	iocrblock_res->IOCRType = iocrblock_req->IOCRType;
	iocrblock_res->IOCRReference = iocrblock_req->IOCRReference;
	iocrblock_res->FrameID = iocrblock_req->FrameID;

	_IOCRBlockRes * iocrblock_res_1 = iocrblock_res; iocrblock_res_1++;
	iocrblock_res_1->Header.BlockType = SWAP16(0x8102);
	iocrblock_res_1->Header.BlockLength = SWAP16(8);
	iocrblock_res_1->Header.BlockVersionHigh = 1;
	iocrblock_res_1->IOCRType = iocrblock_req_1->IOCRType;
	iocrblock_res_1->IOCRReference = iocrblock_req_1->IOCRReference;
	if (iocrblock_req_1->FrameID == 0xffff) {
		uint16_t idx = SWAP16(iocrblock_req_1->Phase); idx += 0x7fff;
		FrameIOCs = iocrblock_res_1->FrameID = SWAP16(0x8000);
	}
	else iocrblock_res_1->FrameID = iocrblock_req_1->FrameID;

	_AlarmCRBlockRes * alarm_res = (_AlarmCRBlockRes*)(iocrblock_res_1+1);
	alarm_res->Header.BlockType = SWAP16(0x8103);
	alarm_res->Header.BlockLength = SWAP16(8);
	alarm_res->Header.BlockVersionHigh = 1;
	alarm_res->AlarmCRType = alarm_req->AlarmCRType;
	alarm_res->LocalAlarmReference = SWAP16(0);
	alarm_res->MaxAlarmDataLength = SWAP16(200);

	uint8_t * block_end = (uint8_t*)(alarm_res+1);

	pp = (uint8_t*)arblock_res;
	uint16_t block_len_16 = (uint16_t)(block_end-pp);
	uint32_t block_len_32 = block_len_16;
	pn_io_s->ArgsLength = block_len_32;
	pn_io_s->ActualCount = block_len_32;

	pn_is_init = 1;

	return block_len_16;
}
