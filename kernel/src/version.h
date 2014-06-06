#ifndef __VERSION_H__
#define __VERSION_H__

#define VERSION		0x000100

#define VERSION_MAJOR	((VERSION >> 16) & 0xff)
#define VERSION_MINOR	((VERSION >> 8) & 0xff)
#define VERSION_MICRO	((VERSION >> 0) & 0xff)

#endif /* __VERSION_H__ */
