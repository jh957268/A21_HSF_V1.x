#!/bin/bash
source ../.config

web_dir=$1
pageflag=
flag_type="NO_AUTH CACHE"

printhead()
{
	echo "/*" >> $filename
 	echo " * Generated by Ralink WEB converter:`date`" >> $filename
 	echo " * Do not edit" >> $filename
 	echo " */" >> $filename
}

getflag()
{
	var=$(echo "$1"|sed -e "s/.*\///g")
	var1=$(echo $var|sed -e "s/.*\.//g")
	for m in $flag_type ; do
		for j in `grep $m ./web.cfg | sed -e "s/$m=//"`; do
		if [ "$j" = "$var" ] || [ "$j" = "*.$var1" ] ; then
				pageflag=$pageflag"|"$m
		fi
		done
	done
	echo $pageflag
}

#generate webpage.c
filename=webpage.c

printhead

echo "#include <http_proc.h>" >> $filename
echo "#include \"webpage.h\"" >> $filename
echo "#include \"web_config.h\"" >> $filename

echo >> $filename
echo "time_t time_pages_created =(`date +%s`+28800);"  >> $filename
#if	[ "$CONFIG_ZWEB" = "y" ] ; then
	#echo "char *zweb_location = NULL;" >> $filename
#else
	echo "extern char _binary_webdata_bin_start[];" >> $filename
	echo "char *zweb_location = &_binary_webdata_bin_start[0];" >> $filename
#fi

echo >> $filename

echo "webpage_entry webpage_table[]= {" >> $filename

rm -f ../webdata.bin

	if	[ "$CONFIG_ZWEB" = "y" ] ; then
		echo "WEBPAGES is compressed!"
	else
		echo "WEBPAGES without compression!"
	fi

for i in `cat web_list`; do
#	fn=`echo $i | sed -e "s/[\.\/]/_/g"`
	if	[ "$CONFIG_ZWEB" = "y" ] ; then
		pageflag="WEBP_COMPRESS"
	else
		pageflag="WEBP_NORMAL"
	fi
	getflag $i
	./webcomp_sin $i $web_dir ./web_c $filename $pageflag ../include/autoconf.h
	if [ $? -ne 0 ]; then
		exit -1
	fi
done

echo "{0 , NULL, 0, 0, 0, NULL, 0}," >> $filename
echo "};" >> $filename

#if	[ ! "$CONFIG_ZWEB" = "y" ] ; then
#	gzip ../webdata.bin ; mv ../webdata.bin.gz ../webdata.bin
#fi
echo "#define WEBDATA_SZ `stat -t ../webdata.bin | cut -d ' ' -f 2`" >> webpage.h

#generate webpage.h
filename=webpage.h
printhead

find ./web_c -name "*.c" | sed -e 's/.\/web_c\/\(.*\).c/extern void \1(http_req *req);/' >> $filename
cat ./web_c/cgi_list >> $filename


# generate .objfiles
filename=./web_c/.objfiles
echo "WEB_OBJ = \\" >$filename
find ./web_c -name "*.c" | sed -e 's/.\/web_c\/\(.*\).c/.\/\1.o \\/' >> $filename

