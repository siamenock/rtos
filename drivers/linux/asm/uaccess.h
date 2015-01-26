#ifndef __ASM_UACCESS_H__
#define __ASM_UACCESS_H__

unsigned long copy_from_user(void* to, const void* from, unsigned long n);
unsigned long copy_to_user(void* to, const void* from, unsigned long n);

#endif /* __ASM_UACCESS_H__ */
