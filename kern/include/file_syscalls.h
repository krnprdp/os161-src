#include <types.h>

//Definition for SYS_open
int sys_open(userptr_t filename, int flags, int * retval);

//Definition for SYS_close
int sys_close(int fd, int *retval);

//Definition for SYS_read
int sys_read(int fd, void *buf, size_t buflen, int *retval);

//Definition for SYS_write
int sys_write(int fd, void *buf, size_t nbytes, int *retval);

//Definition for SYS_lseek
off_t sys_lseek(int fd, off_t pos, userptr_t whenceptr, off_t *retval64);

//Definition for SYS_dup2
int sys_dup2(int oldfd, int newfd, int *retval);

//Definition for SYS_chdir
int sys_chdir(userptr_t pathname, int *retval);

//Definition for SYS__getcwd
int sys_getcwd(userptr_t buf, size_t buflen, int *retval);
