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


	kfree(curthread->t_fdtable);

	V(curthread->t_sem);
	thread_exit();
}
