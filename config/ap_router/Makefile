#
#
#

TOPDIR	= $(shell cd ../.. ; pwd)
PRJDIR	= $(shell pwd | sed "s/.*\///" )
SRC	= $(TOPDIR)/websrc
DST	= $(TOPDIR)/webpages

ifeq ($(TOPDIR)/.config,$(wildcard $(TOPDIR)/.config))
include $(TOPDIR)/.config
else
include ./outline/defconfig
endif

ifndef	CONFIG_CGI_DIR
CONFIG_CGI_DIR = cgi_v2
endif

ifndef	CONFIG_AUTH_DIR
CONFIG_AUTH_DIR = auth_v2
endif

ifeq ($(WEB_LANG),TChinese)
WEBPAGE_SRC	= $(SRC)/$(shell echo $(CONFIG_WEB_DIR))_tc/
else
WEBPAGE_SRC	= $(SRC)/$(shell echo $(CONFIG_WEB_DIR))
endif

CGI_SRC  = $(SRC)/cgi/$(shell echo $(CONFIG_CGI_DIR))
AUTH_SRC = $(SRC)/auth/$(shell echo $(CONFIG_AUTH_DIR))

RM_PAGES = 
ifdef CONFIG_LOG_SINGLE_POOL
RM_PAGES += securelog.bin
endif

#ifndef	CONFIG_PPTP
#RM_PAGES += wan_pptp
#endif

#ifndef	CONFIG_LT2P
#RM_PAGES += wan_l2tp
#endif

all:	init_make

init_make:	outline webpages
	echo "$(PRJDIR)" > $(TOPDIR)/.prjname
	@cd $(TOPDIR) ; rm -f .version ; make
	
outline:
	@echo "Copy outline..."
	cp -f ./outline/defconfig $(TOPDIR)
	cp -f ./outline/profile.txt $(TOPDIR)
	if [ -e ./outline/dl_cookie.h ] ; then cp -f ./outline/dl_cookie.h $(TOPDIR)/include ; fi
#	if [ -e ./outline/.version ] ; then cp -f ./outline/.version $(TOPDIR) ; else rm -f $(TOPDIR)/.version ; fi

	
webpages: cgi auth web scripts
	find $(DST) -name CVS | xargs rm -rf ;

cgi:
	@echo "Build CGI directory..."
	mkdir -p $(DST)
	rm -rf $(DST)/cgi
	cp -ra $(CGI_SRC) $(DST)/cgi
	if [ -e ./cgi ] ; then cp -raf ./cgi $(DST)/cgi_usr  ; fi
	if [ -e ./cgi/cgi_aux.h ]; then cp ./cgi/cgi_aux.h $(TOPDIR)/include/cgi_aux.h ; fi
	
auth:
	@echo "Build AUTH directory..."
	mkdir -p $(DST)
	rm -rf $(DST)/auth
	cp -ra $(AUTH_SRC) $(DST)/auth
	if [ -e ./auth ] ; then cp -raf ./auth/* $(DST)/auth  ; fi
	
web:
	@echo "Bulid WEB working directory..."
	mkdir -p $(DST)
	rm -rf ./tmp
	rm -rf $(DST)/web_html
	cp -ra $(WEBPAGE_SRC) $(DST)/web_html
	for s in $(RM_PAGES); do rm -f $(DST)/web_html/$$s ; done
	if [ -e ./webpages ] ; then \
		cp -ra ./webpages ./tmp ; \
		cp -ra ./tmp/* $(DST)/web_html ; \
		rm -rf ./tmp ; \
	fi
	
scripts:
	mkdir -p $(DST)
	cp -af $(SRC)/scripts/* $(DST)
	
.PHONY: all webpages outline cgi auth web scripts

