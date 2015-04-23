#include <types.h>
#include <mips/trapframe.h>

//Definition for SYS_exit
void sys_exit(int exitcode);

//Definition for SYS_fork
int sys_fork(struct trapframe *tf,pid_t *retval);
void child_forkentry(void *data1, unsigned long data2);

//Definition for getpid
int sys_getpid(pid_t *retval);
