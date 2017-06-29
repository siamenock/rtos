#!/bin/bash
BASE=$(dirname $0)
VERSION=`$BASE/ver.sh`
MAJOR=`echo $VERSION | awk -F'[.-]' '{ print $1 }'`
MINOR=`echo $VERSION | awk -F'[.-]' '{ print $2 }'`

echo "PacketNgin Release Script"
git commit && git tag v$MAJOR.$MINOR
