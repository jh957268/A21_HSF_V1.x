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

#define ARTNET_DEFAULT_PORT	3939

extern int ratpac_get_str(int id, char *val);
extern void check_gaffer_packet(char *data, uint32_t len);
extern void replace_channel_data(char *data, uint32_t len); 

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
}

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
		init_artpollreply_msg();
		sendto(sockfd, (char *)&artpollreply_msg, sizeof artpollreply_msg, 0, (struct sockaddr *) &dest, sizeof(dest));
	}
	else
	{
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
		}			
	}
}
