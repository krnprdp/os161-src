#include <types.h>
#include <kern/errno.h>
#include <kern/fcntl.h>
#include <kern/wait.h>
#include <lib.h>
#include <thread.h>
#include <synch.h>
#include <current.h>
#include <syscall.h>

void sys_exit(int _exitcode) {

	(void) _exitcode;
	curthread->status = _MKWAIT_EXIT(_exitcode);

	//kfree(curthread->t_fdtable);

	V(curthread->t_sem);
	thread_exit();
}
int sys_fork( tf, pid_t *retval) {
	struct trapframe *t2;
	struct addrspace childaddr;
	struct thread *child;
	struct fdesc* ft;
	int result,i;
	childaddr = kmalloc((struct addrspace) sizeof(struct addrspace));
	t2 = kmalloc((struct trapframe*) sizeof(struct trapframe*));
	t2 = tf;
	childforkentry(tf);
	as_copy(curthread->t_addrspace, childaddr);

	result=thread_fork(curthread->t_name, *childforkentry(tf, NULL ), tf, NULL,
			*childaddr);
	ft=kmalloc((struct fdesc*)sizeof(struct fdesc*));
	for(i=0;i<OPEN_MAX;i++)
	{
	ft[i]->filelock=curthread->t_fdtable[i].filelock;
	ft[i]->flag=curthread->t_fdtable[i].flag;
	ft[i]->name=curthread->t_fdtable[i].name;
	ft[i]->offset=curthread->t_fdtable[i].offset;
	ft[i]->ref_count=curthread->t_fdtable[i].ref_count;
	ft[i]->vn=curthread->t_fdtable[i].vn;
	}
	return 0;
}




void childforkentry(struct trapfame *tf) {

}

int sys_getpid(pid_t *retval){
	*retval=curthread.t_pid;
	return 0;
}



