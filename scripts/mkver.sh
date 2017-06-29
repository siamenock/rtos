#!/bin/bash

BASE=$(dirname $0)
VERSION=`$BASE/ver.sh`
MAJOR=`echo $VERSION | awk -F'[.-]' '{ print $1 }'`
MINOR=`echo $VERSION | awk -F'[.-]' '{ print $2 }'`
MICRO=`echo $VERSION | awk -F'[.-]' '{ print $3 }'`
TAG=`echo $VERSION | awk -F'[.-]' '{ print $4 }'`

echo "#ifndef __VERSION_H__
#define __VERSION_H__

#define VERSION_MAJOR   $MAJOR
#define VERSION_MINOR   $MINOR
#define VERSION_MICRO   $MICRO
#define VERSION_TAG	0x$TAG

#define VERSION         \"$MAJOR.$MINOR.$MICRO-$TAG\"

#endif /* __VERSION_H__ */"
