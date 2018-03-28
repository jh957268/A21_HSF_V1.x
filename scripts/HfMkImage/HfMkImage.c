
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAX_PROFILE_SIZE			15*1024
#define HDR_SIZE					32


unsigned int chk_sum(int * p, int len)
{
    int wl=len>>2;
    unsigned int sum=0;

   	while (wl-- > 0 )
       	sum += *p++;
    return sum;
}

int main(int argc,char *argv[])
{
	FILE *fImage,*fProfile;
	struct stat file_stat={0};
	int i;
	int dst_image_size=704*1024;
	int image_file_size=0;
	int profile_size=0;
	int max_profile_size=0;
	char *data;
	unsigned char hdr[HDR_SIZE]={0};
	unsigned int  check_sum=0;
	unsigned int  cfg_len=0;	

	fImage=fProfile=NULL;
	
	if(argc<7)
	{
		printf("HfMkImage eCos.img profile.txt 1M\n");
		return 1;
	}
	
	if((fImage=fopen(argv[2],"a"))==NULL)
	{
		printf("can't open the file %s\n",argv[1]);
		return 1;
	}
	
	if((fProfile=fopen(argv[4],"r"))==NULL)
	{
		printf("can't open the profile %s\n",argv[2]);
		return 1;
	}
	stat(argv[2],&file_stat);
	image_file_size = file_stat.st_size;
	stat(argv[4],&file_stat);
	profile_size = file_stat.st_size;
	
	if(profile_size+HDR_SIZE+16>MAX_PROFILE_SIZE)
	{
		printf("the length of profile is too big\n");
		return 1;
	}
	//printf("src image file size %d\n",image_file_size);
	//printf("profile size %d\n",profile_size);
	
	if(strcmp(argv[6],"1M")==0)
	{
		dst_image_size = (704-1)*1024;
	}
	else if(strcmp(argv[6],"2M")==0)
	{
		dst_image_size = (832-1)*1024;
	}
	else if(strcmp(argv[6],"4M")==0)
	{
		dst_image_size = (832-1)*1024;
	}
	else
	{
		printf("invalid flash size %s\n",argv[6]);
	}
	
	max_profile_size = dst_image_size-image_file_size;
	if(max_profile_size<=MAX_PROFILE_SIZE)
	{
		printf("Image file is too big!\n");
		return 0;
	}

	data = (char*)malloc(max_profile_size);
	if(data==NULL)
	{
		printf("no memory\n");
		return 0;
	}
	
	memset(data,0xff,max_profile_size);
	memset(data+max_profile_size-MAX_PROFILE_SIZE,0,MAX_PROFILE_SIZE);
	fread(data+max_profile_size-MAX_PROFILE_SIZE+32,1,MAX_PROFILE_SIZE,fProfile);
	cfg_len = MAX_PROFILE_SIZE-HDR_SIZE-16;
	check_sum = chk_sum(data+max_profile_size-MAX_PROFILE_SIZE+32,cfg_len);
	printf("cfg_len=%d check_sum=%d\n",cfg_len,check_sum);
	hdr[0] = check_sum>>24;
	hdr[1] = (check_sum>>16)&0xff;
	hdr[2] = (check_sum>>8)&0xff;
	hdr[3] = (check_sum)&0xff;
	hdr[4] = cfg_len>>24;
	hdr[5] = (cfg_len>>16)&0xff;
	hdr[6] = (cfg_len>>8)&0xff;
	hdr[7] = (cfg_len)&0xff;
	memcpy(hdr+8,"dcfg",4);
	memcpy(data+max_profile_size-MAX_PROFILE_SIZE,hdr,32);
	
	fwrite(data,1,max_profile_size,fImage);
	
	fclose(fProfile);
	fclose(fImage);
	free(data);
	return 0;
}
