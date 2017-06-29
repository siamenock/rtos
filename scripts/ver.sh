#!/bin/bash

function is_int() {
	if [ -z $@ ];
	then
		echo 0
		return
	fi

	if [ "$@" -eq "$@" ] 2>/dev/null
	then
		echo $@
	else
		echo 0
	fi
}

MAJOR=`git branch | grep \*`
MAJOR=`echo "$MAJOR" | awk '{split($0,ver,"v"); print ver[2]}'`
MINOR=0
MICRO=`git rev-list HEAD --count`
TAG=`git log | head -1 | awk '{printf("%s", substr($2, 0, 7))}'`

MAJOR=$(is_int $MAJOR);
MINOR=$(is_int $MINOR);
MICRO=$(is_int $MICRO);

echo $MAJOR.$MINOR.$MICRO-$TAG
