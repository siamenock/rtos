MAJOR=`git tag | head -1 | awk '{split($0,a,"."); print a[1]}'`
MINOR=`git tag | head -1 | awk '{split($0,a,"."); print a[2]}'`
MICRO=`git rev-list $MAJOR.$MINOR --count`
TAG=`git log | head -1 | awk '{printf("\"%s\"", substr($2, 0, 7))}'`

echo "#ifndef __VERSION_H__
#define __VERSION_H__

#define VERSION_MAJOR   $MAJOR
#define VERSION_MINOR   $MINOR
#define VERSION_MICRO   $MICRO
#define VERSION_TAG	$TAG

#define VERSION         ((VERSION_MAJOR << 16) | (VERSION_MINOR << 8) | (VERSION_MICRO))

#endif /* __VERSION_H__ */"
