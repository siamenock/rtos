#!/bin/bash

MAJOR=`git branch | grep \* | cut -d ' ' -f2`
MINOR=0
MICRO=`git rev-list HEAD --count`

if [ "$MAJOR" == "" ]; then
	MAJOR=0
	MINOR=0
fi

echo $MAJOR.$MINOR.$MICRO
