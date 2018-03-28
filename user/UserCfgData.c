

#include <cfg_id.h>
#include <user_cfg_id.h>

struct cfgid_str_t cfg_userid_str_table[]=
{
#define CFG_USER_STR_ENTRY_( c)     { CFG_##c, #c }
     	
    CFG_USER_STR_ENTRY_(AKS_NAME),
    CFG_USER_STR_ENTRY_(AKS_UNIVERSE),
    CFG_USER_STR_ENTRY_(AKS_SUBNET),
    CFG_USER_STR_ENTRY_(TIMO_POWER),
    CFG_USER_STR_ENTRY_(AKS_SETTINGS),
    CFG_USER_STR_ENTRY_(AKS_CHANNEL_WIDTH),
    CFG_USER_STR_ENTRY_(AKS_SECOND_CHANNEL),
    { 0, 0 }
};



