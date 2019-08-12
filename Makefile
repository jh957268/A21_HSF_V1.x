#	HiFling ECOS project

#define the ecos tool path for ecosconfig
ECOS_TOOL_PATH := $(shell pwd)/tools/bin:$(PATH)
#define cross compiler path
ECOS_MIPSTOOL_PATH := $(shell pwd)/tools/mipsisa32-elf/bin
BRANCH = ADV
PRJ_NAME = ap_router
WIFI_MODE = AP
CHIPSET = mt7628
TARGET = ECOS
WEB_LANG = English
#TFTP_DIR = /tftpboot
TFTP_DIR = ./output
BUILD_DATE := $(shell date +%F%H%M)
ifeq ($(VER),)
VER = 0.00
endif
#IMAGE_NAME = eCos

PATH :=$(ECOS_TOOL_PATH):$(ECOS_MIPSTOOL_PATH):$(PATH)
export PATH ECOS_REPOSITORY BRANCH BOOT_CODE PRJ_NAME CHIPSET WIFI_MODE TARGET TFTP_DIR IMAGE_NAME CONFIG_CROSS_COMPILER_PATH WEB_LANG

ifeq ($(IMAGE_NAME),)
IMAGE_NAME = AKS
endif
HF_SDK_VERSION=4.02.10.08

include .config
INSTALL_DIR = ./

PRJ_DIR=$(shell cat .prjname)

include $(INSTALL_DIR)/include/pkgconf/ecos.mak

do-it-all:  xrouter xrouter.bin zxrouter zxrouter.bin cptftp


CROSS_PREFIX=$(ECOS_COMMAND_PREFIX)
ENDIAN=$(shell echo $(ECOS_GLOBAL_CFLAGS) | sed -e "s/.*-EL.*/-EL/" )
TOPDIR=$(shell pwd)

export TOPDIR ENDIAN CROSS_PREFIX

CFLAGS        = -I$(shell pwd)/include -I$(shell pwd)/include/tcpip/include -DBUILD_VER=$(VER) -DBUILD_IMG=$(IMAGE_NAME)
CXXFLAGS      = $(CFLAGS)
LDFLAGS       = -nostartfiles -Ttarget.ld -L./lib
LDMAP          = -Wl,--cref -Wl,-Map,xrouterLink.map -Wl,-O2

CFLAGS += -D__ECOS -D_KERNEL $(ECOS_GLOBAL_CFLAGS) -include config.h
ifdef CONFIG_IPV6
CFLAGS += -DCYGPKG_NET_INET6
endif

include rules.mak

ifeq ($(word 1,$(shell dos2unix -V 2>&1)),tofrodos)
DOS2UNIX=dos2unix -p
else
DOS2UNIX=dos2unix -k
endif


all: do-it-all

# LIBRARY

ifeq ($(BRANCH),STD)
CFLAGS += -DBRANCH_STD
endif

ifeq ($(BRANCH),ADV)
CFLAGS += -DBRANCH_ADV
endif

ifeq "$(CONFIG_ZLOAD_BUF)" ""
	CONFIG_ZLOAD_BUF = 0x80500000
endif

USERAPPSUBDIRS = user 
USERAPPSUBDIRS += webpages

XROUTER_LIBS += $(shell pwd)/lib/libtarget.a
USER_APP_OBJS = $(join $(USERAPPSUBDIRS), $(foreach n,$(USERAPPSUBDIRS), $(shell echo "/"$(shell echo /$(n).o | sed "s/.*\///"))))

NOW=$(shell date -d "now +8 hour" +"%s")
BT_H=./include/build_time.h
$(BT_H): target.ld
	. scripts/mkversion.sh > .tmpversion
	@mv -f .tmpversion .version
	echo "#define SYS_BUILD_TIME $(NOW)"  > $(BT_H)
	echo "#define SYS_BUILD_COUNT `cat .version`" >> $(BT_H)

#version.o:$(BT_H)
#	echo "mh***********************mh"
#	$(XCC) $(CFLAGS) -o version.o version.c

userappsubdirs: $(patsubst %, _dir_%, $(USERAPPSUBDIRS))
$(patsubst %, _dir_%, $(USERAPPSUBDIRS)) :
	$(MAKE) CFLAGS="$(CFLAGS)" ENDIAN=$(ENDIAN) -C $(patsubst _dir_%, %, $@)

userapps.o:userappsubdirs $(USER_APP_OBJS)
		$(XLD) -r $(ENDIAN) -o userapps.o $(USER_APP_OBJS)

drivers.o:
	unzip -o HFA21.zip
apps.o:
	unzip -o HFA21.zip
xrouters.o:
	unzip -o HFA21.zip

xrouter:  webpages $(XROUTER_LIBS) drivers.o apps.o target.ld userapps.o xrouters.o
	$(XCC) $(LDFLAGS) $(ECOS_GLOBAL_LDFLAGS) -o $@ xrouters.o userapps.o

zxrouter:  xrouter.bin zload/zwebmap.h
	$(MAKE) CFLAGS="$(CFLAGS)" ENDIAN=$(ENDIAN) -C zload
	chmod 777 scripts/mkimage
	./tools/bin/lzma e xrouter.bin  bin.gz
ifdef	CONFIG_ZWEB
	$(XLD) $(ENDIAN) $(LD_EXTRA) -Ttext=$(CONFIG_ZLOAD_BUF) -Tzload/zload.ld -o zxrouter zload/zload.o -\( -bbinary bin.gz -bbinary webdata.bin -\)
else
	$(XLD) $(ENDIAN) $(LD_EXTRA) -Ttext=$(CONFIG_ZLOAD_BUF) -Tzload/zload.ld -o zxrouter zload/zload.o -\( -bbinary bin.gz -\)
endif
	$(XNM) $@ | grep -v '\(compiled\)\|\(\.o$$\)\|\( [aUw] \)\|\(\.\.ng$$\)\|\(LASH[RL]DI\)' | sort > $@.map

zxrouter.bin: zxrouter
	chmod -Rf 777 scripts/imghdr/imghdr
	$(XOC) -O binary $(@:.bin=) $@
	scripts/imghdr/imghdr zxrouter.bin t.bin 0x`$(XOD) -s ./xrouters.o | grep "data.sys_sw_version" -A 1 | tail -n 1 | cut -b7-14` $(NOW) "$(FWM_DESC)" ; rm zxrouter.bin ; mv t.bin zxrouter.bin
#	scripts/imghdr/imghdr zxrouter.bin t.bin 0x`$(XOD) -s ./version.o | grep "data.sys_sw_version" -A 1 | tail -n 1 | cut -b7-14` $(NOW) "$(FWM_DESC)" ; rm zxrouter.bin ; mv t.bin zxrouter.bin
#	cat zxrouter.bin scripts/zero.bin > zxrouter.pad.bin
	scripts/mkimage -A mips -T standalone -C none -a $(CONFIG_ZLOAD_BUF) -e $(CONFIG_ZLOAD_BUF) -n "zxrouter" -d zxrouter.bin zxrouter.img
	$(XOD) -d xrouter > ecos.dump
	
zload/zwebmap.h: xrouter.map
	grep "zweb_location" xrouter.map | sed -e 's/^\(.*\) \(.*\) \(.*\)$\/\#define ZWEB_LOC 0x\1/' > zload/zwebmap.h



EXLIBS_NAME = HFA21EX
USER_NEWAPP_OBJS = apps.o 
LIBS_VERSION_FN = $(LIBS_NAME).version

FWM_DESC += $(shell date -d "1970/01/01 +$(NOW) sec" +"%T %x")

cptftp:	xrouter.bin zxrouter.bin
	cp -f xrouter.bin $(IMAGE_NAME).bin
	cp -f zxrouter.bin z$(IMAGE_NAME).bin
	cp -f zxrouter.img $(IMAGE_NAME).img
	cp -f zxrouter.img $(TFTP_DIR)/$(IMAGE_NAME).img
	rm -Rf xrouter.* zxrouter*
	#rm -Rf version.o
	rm -Rf $(EXLIBS_NAME)
	rm -Rf $(LIBS_VERSION_FN)
	date +%F%H%M >$(LIBS_VERSION_FN)
	echo $(HF_SDK_VERSION)>>$(LIBS_VERSION_FN)	
ifeq ($(CONFIG_M2M_HW_SMT),y)
	echo "SMT" >>$(LIBS_VERSION_FN)
endif
	$(XLD) -r $(ENDIAN) -o appsnew.o $(USER_NEWAPP_OBJS)
	cp appsnew.o apps.o
	zip $(EXLIBS_NAME) apps.o drivers.o xrouters.o $(LIBS_VERSION_FN)
	-rm -f drivers.o apps.o xrouters.o appsnew.o
	rm -f  userapps.o
ifeq ($(CONFIG_M2M_HW_SMT),y)
	cp -f $(IMAGE_NAME).img $(TFTP_DIR)/$(IMAGE_NAME)-$(BUILD_DATE)-$(VER).img
	#cp -f $(IMAGE_NAME).img $(TFTP_DIR)/$(IMAGE_NAME)_SMT.img
else
	cp -f $(IMAGE_NAME).img $(TFTP_DIR)/$(IMAGE_NAME).img
endif


webpages:
#	if [ -e ./.prjname ] ; then make webpages -C config/$(PRJ_DIR) ; fi
	if [ ! -d ./webpages ] ; then make -C config/$(PRJ_DIR) ; fi

.PHONY: all webpages clean $(BT_H)

clean:
	@echo "clean all.."
	-rm -f xrouter	xrouter.o zxrouter* $(IMAGE_NAME).* z$(IMAGE_NAME).* bin.gz
	-rm -f zxrouter.pad.bin zxrouter.img
	-rm -f userapps.o
	-rm -f drivers.o apps.o xrouters.o
	-rm -rf webpages
	-rm -f webdata.bin
	-rm -f profile.txt
	-rm -f jooxroutermap
	-rm -f ./output/*
	make clean -C ./user/
	
permission:
	chmod u+rx scripts/imghdr/imghdr
	chmod u+rx scripts/imghdr/imghdr_bn
	chmod u+rx scripts/imghdr/cookie



