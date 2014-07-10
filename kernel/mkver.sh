echo "#ifndef __VERSION_H__
#define __VERSION_H__

#define VERSION_MAJOR   `git tag | awk '{split($0,a,"."); print a[1]}'`
#define VERSION_MINOR   `git tag | awk '{split($0,a,"."); print a[2]}'`
#define VERSION_MICRO   `git rev-list HEAD --count`

#define VERSION         (VERSION_MAJOR << 16) | (VERSION_MINOR << 8) | (VERSION_MICRO)

#endif /* __VERSION_H__ */"
