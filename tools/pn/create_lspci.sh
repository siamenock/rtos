#!/bin/bash
# Copyright 2013-2014 Antonio Barbalace
# Copyright 2013 Phil Wilshire
###############################################################################
# Generate pci blacklist for multikernel architecture
###############################################################################

# lspci -n | awk ' { print	 ":b" '} | grep -v "^06" | cut -d ':' -f2,3,4 | sed 's/^/0x/' | sed 's/:/:0x/'

# we black list PCI devices based on the class of the device
# 06 class is bridge, subclass 00 is Host bridge, 01 is ISA bridge, etc.
# have a look at /usr/share/pci.ids for a complete list
# note that classes are listed at the end of the file
NOT_BLACKLIST_CLASS="0600 0604"
#NOT_BLACKLIST_CLASS="0604"
#NOT_BLACKLIST_CLASS="06"

# check lspci existence and minimum version requirement ( >= 3.0.0)
command lspci &>/dev/null
if [ $? -ne 0 ]
then
	echo "lspci not found in the system. please install it and try again"
	exit 1
fi
_RET=`lspci --version | sed 's/\([a-z ]*\)\([0-9]*\.[0-9]*\.[0-9]*\)/\2/'`
_VALUE=`echo $_RET | sed 's/\./ /g' | awk '{print $1}'`
if [ "$_VALUE" -lt 3 ]
then
	echo "lspci $_RET is not supported by this script. please update your installation"
	exit 1
fi

#get the list in the right format, maintain it in a temporary file
_LIST_FILE=`mktemp`
if [ -z "$_LIST_FILE" ]
then
	echo "cannot create temporary file (1). check your system, is mktemp present?"
	exit 1
fi
lspci -n | awk '{print $2 $3 ":b,"}' > $_LIST_FILE
if [ ! -s $_LIST_FILE ]
then
	echo "lspci list empty, impossible to continue. check your system"
	exit 1
fi

# grep out the categories that we do not want
__LIST_FILE=`mktemp`
if [ -z "$__LIST_FILE" ]
then
	echo "cannot create temporary file (2). check your system"
	exit 1
fi
for _NBC in $NOT_BLACKLIST_CLASS
do
	cat $_LIST_FILE | grep -v "^$_NBC" > $__LIST_FILE
	cp $__LIST_FILE $_LIST_FILE
done
# check if there are still entries
if [ ! -s $_LIST_FILE ]
then
	echo "list was populated now is empty. create your blacklist manually"
	exit 1
fi

# sort and remove eventual duplicates
sort $_LIST_FILE > $__LIST_FILE
uniq -w 15 $__LIST_FILE > $_LIST_FILE

# reformat and eliminate trailing commas
cat $_LIST_FILE | cut -d ':' -f2,3,4 | sed 's/^/0x/' | sed 's/:/:0x/' > $__LIST_FILE
echo `cat $__LIST_FILE` | sed 's/b, /b,/g' | sed 's/b,$/b/'
rm $__LIST_FILE
rm $_LIST_FILE

exit 0

