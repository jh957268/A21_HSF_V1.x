#include <hsf.h>

struct artnet_hdr {
	uint8_t		ID[8];
	uint8_t		OpCode[2];
	uint8_t		ProtVer[2];
	uint8_t		Sequence;
	uint8_t		Physical;
	uint8_t		SubUni;
	uint8_t		Net;
	uint8_t		Length[2];
} __attribute__((packed));

struct artpollreply {
	uint8_t		ID[8];
	uint8_t		OpCode[2];
	uint8_t		IP_Address[4];
	uint8_t		Port[2];
	uint8_t		VersInfoH;
	uint8_t		VersInfoL;
	uint8_t		NetSwitch;
	uint8_t		SubSwitch;
	uint8_t		OemHi;
	uint8_t		OemLo;
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

#define ARTNET_DEFAULT_PORT	6454

extern int ratpac_get_str(int id, char *val);
extern void check_gaffer_packet(char *data, uint32_t len);
extern void replace_channel_data(char *data, uint32_t len); 

extern char ipAddress[];
extern int artnet_enable;

struct artpollreply	artpollreply_msg;
struct artnet_hdr artnet_hdr_msg;

void init_artpollreply_msg(void)
{
	memset((void *)&artpollreply_msg, 0x0, sizeof (artpollreply_msg));
	strcpy((char *)(artpollreply_msg.ID), "Art-Net");
	artpollreply_msg.OpCode[0] = 0x00;
	artpollreply_msg.OpCode[1] = 0x21;
	artpollreply_msg.IP_Address[0] = ipAddress[0];
	artpollreply_msg.IP_Address[1] = ipAddress[1];
	artpollreply_msg.IP_Address[2] = ipAddress[2];
	artpollreply_msg.IP_Address[3] = ipAddress[3];
	artpollreply_msg.Port[0] = 0x36;
	artpollreply_msg.Port[1] = 0x19;
	artpollreply_msg.VersInfoH = 12;
	artpollreply_msg.VersInfoL = 18;
	artpollreply_msg.OemLo = 0xff;
	artpollreply_msg.EstaManLo = 0xD7;
	artpollreply_msg.EstaManLo = 0x51;
	ratpac_get_str(CFG_str2id("AKS_NAME"),(char *)artpollreply_msg.ShortName);
	strcpy((char *)artpollreply_msg.LongName, "Ratpac AKS - Designed by Ratpac Dimmers");
	artpollreply_msg.NumPortsLo = 0x1;
	artpollreply_msg.PortTypes[0] = 0b10000000;
	artpollreply_msg.GoodInput[0] = 0b10000000;
	artpollreply_msg.GoodInput[1] = 0b00001000;
	artpollreply_msg.GoodInput[2] = 0b00001000;
	artpollreply_msg.GoodInput[3] = 0b00001000;
	artpollreply_msg.GoodOutput[0] = 0b10000000;
	artpollreply_msg.Status2 = 15;
	
	memset((void *)&artnet_hdr_msg, 0x0, sizeof (artnet_hdr_msg));
	strcpy((char *)(artnet_hdr_msg.ID), "Art-Net");
	artnet_hdr_msg.OpCode[0] = 0;
	artnet_hdr_msg.OpCode[1] = 0x50;
	artnet_hdr_msg.ProtVer[0] = 0;
	artnet_hdr_msg.ProtVer[1] = 0xe;
	artnet_hdr_msg.Length[0] = 0x2;
	artnet_hdr_msg.Length[1] = 0x0;
}

extern unsigned int dmx_count;
void process_artnet_msg(int sockfd, uint8_t *raw, int len, struct sockaddr_in from)
{
	struct sockaddr_in dest;
	char Uni[4]={0};
	int uni_num, art_sub;	
	
	raw[7] = 0;
	
	if (strcmp((char *)raw, "Art-Net"))
	{
		return;
	}
	memset((char*)&dest,0,sizeof(dest));
	dest.sin_family = AF_INET;
	dest.sin_addr.s_addr = from.sin_addr.s_addr;
	dest.sin_port = htons(ARTNET_DEFAULT_PORT);
	
	if (len <= 50)
	{
		if (raw[9] != 0x20)
			return;
		artpollreply_msg.IP_Address[0] = ipAddress[0];
		artpollreply_msg.IP_Address[1] = ipAddress[1];
		artpollreply_msg.IP_Address[2] = ipAddress[2];
		artpollreply_msg.IP_Address[3] = ipAddress[3];		
		sendto(sockfd, (char *)&artpollreply_msg, sizeof artpollreply_msg, 0, (struct sockaddr *) &dest, sizeof(dest));
	}
	else
	{
		if ((raw[9] != 0x50) || (artnet_enable == 0))
			return;
		//Artnet DMX packet
		check_gaffer_packet((char *)raw, len);
		ratpac_get_str(CFG_str2id("AKS_UNIVERSE"),Uni);	
		uni_num = atoi(Uni);;
		ratpac_get_str(CFG_str2id("AKS_SUBNET"),Uni);
		art_sub = atoi(Uni);
		art_sub <<= 4;
		uni_num = ((uni_num | art_sub) & 0xff);
		if(uni_num == (int)raw[14])
		{
			replace_channel_data((char *)raw, len); 
			hfuart_send(HFUART0, raw,len,1000);
			dmx_count++;
		}			
	}
}

void send_artnet_header(uint8_t seq, char *dmx_slot, int slot_cnt )
{
	int len;
	char *dmx_data = dmx_slot - sizeof(artnet_hdr_msg);
	artnet_hdr_msg.Sequence = seq;
	memcpy((void *)dmx_data, (void *)&artnet_hdr_msg, sizeof(artnet_hdr_msg));
	len = sizeof(artnet_hdr_msg) + slot_cnt;
	if (hfuart_send(HFUART0, dmx_data, len,1000) == len)
	{
		dmx_count++;
	}
}