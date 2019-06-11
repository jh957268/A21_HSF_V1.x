#include <hsf.h>

struct artnetpollreply {
	uint8_t		ID[8];
	uint8_t		OpCode[2];
	uint8_t		IP_Address[4];
	uint8_t		Port[2];
	uint8_t		VersInfoH;
	uint8_t		VersInfoL;
	uint8_t		NetSwitch;
	uint8_t		SubSwitch;
	uint8_t		OemHi;
	uint8_t		Oem;
	uint8_t		Ubea_Version;
	uint8_t		Status1;
	uint8_t		EstaManLo;
	uint8_t		EstaManHi;
	uint8_t		ShortName[18];
	uint8_t		LongName[64];
	uint8_t		NodeReport[64];
	uint8_t		NumPortsHi;
	uint8_t		NumPortsLo;
	uint8_t		PortTypes[4];
	uint8_t		GoodInput[4];
	uint8_t		GoodOutput[4];
	uint8_t		SwIn[4];
	uint8_t		SwOut[4];
	uint8_t		SwVideo;
	uint8_t		SwMacro;
	uint8_t		SwRemote;
	uint8_t		Spare_Style[4];
	uint8_t		MAC[6];
	uint8_t		BindIp[4];
	uint8_t		BindIndex;
	uint8_t		Status2;
	uint8_t		Filler[26];
} __attribute__((packed));

struct artnetpollreply	artpollreply_msg;

void init_artpollreply_msg(void)
{
	strcpy((char *)(artpollreply_msg.ID), "Art-Net");
	artpollreply_msg.OpCode[0] = 0x00;
	artpollreply_msg.OpCode[1] = 0x20;
}

void send_artpollreply_msg(void)
{
	init_artpollreply_msg();
	hfnet_socketa_send((char *)&artpollreply_msg, sizeof(artpollreply_msg), 100);
}
