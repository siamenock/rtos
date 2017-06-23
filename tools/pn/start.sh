#! /bin/bash
BOOT_PARAM=`cat packetngin-boot.param`
echo $BOOT_PARAM

sudo ./pn $BOOT_PARAM
