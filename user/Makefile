# Makefile

include $(TOPDIR)/.config
include $(TOPDIR)/rules.mak
include $(TOPDIR)/.userconfig

ifeq ($(word 1,$(shell dos2unix -V 2>&1)),tofrodos)
DOS2UNIX=dos2unix -p
else
DOS2UNIX=dos2unix -k
endif

ALL_OBJS = UserMain.o UserCfgData.o e131.o ArtPollReply.o
ALL_HEADS = 


EXTRA_CFLAGS += -I$(TOPDIR)/include/httpd/include
EXTRA_CFLAGS += -I$(TOPDIR)/include/m2m_app
EXTRA_CFLAGS += -I$(TOPDIR)/user/include
EXTRA_CFLAGS += -I$(TOPDIR)/include/hsf_hf

ifeq ($(CONFIG_HTTP_CLINET),y)
EXTRA_CFLAGS += -DECOS_BUILD -I$(TOPDIR)/httpc/
endif

ifeq ($(CONFIG_ENABLE_TLS),y)
EXTRA_CFLAGS += -DECOS_BUILD -I$(TOPDIR)/cyassl-2.6.0/
endif

MODULE = $(shell pwd | sed "s/.*\///" ).o

all: $(MODULE)

$(MODULE): $(ALL_OBJS:.o=.d) $(ALL_HEADS) ../profile.txt
	cp ../profile.txt ./profile.txt
	$(DOS2UNIX) ./profile.txt
	$(XLD) -r $(ENDIAN) -o $(MODULE) $(ALL_OBJS) -bbinary profile.txt
	rm profile.txt

clean:
	rm -f $(MODULE) $(ALL_OBJS) *.d
	
