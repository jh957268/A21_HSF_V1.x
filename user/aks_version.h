static char *rel_date = __DATE__" " __TIME__;

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
#ifdef BUILD_VER
static unsigned char *eCos_ver= STR(BUILD_VER);
#else
static unsigned char *eCos_ver= "9.99";
#endif

#ifdef BUILD_IMG
static unsigned char *build_img= STR(BUILD_IMG);
#else
static unsigned char *build_img= "AKS";
#endif
	