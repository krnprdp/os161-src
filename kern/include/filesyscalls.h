#include <types.h>

//Definition for SYS_open
int sys_open(userptr_t filename, int flags, int * retval);
//Definition for SYS_close
int sys_close(int fd, int *retval);
