#ifndef __UTSNAME_H
#define __UTSNAME_H

#define SYS_NMLN ((unsigned long) 256)

struct utsname {
	char sysname[SYS_NMLN];
	char nodename[SYS_NMLN];
	char release[SYS_NMLN];
	char version[SYS_NMLN];
	char machine[SYS_NMLN];
};

#ifdef __cplusplus
extern "C"
#endif
int uname(struct utsname *name);

#endif // __UTSNAME_H
