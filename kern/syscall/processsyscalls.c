#include <types.h>
#include <kern/errno.h>
#include <kern/fcntl.h>
#include <kern/wait.h>
#include <thread.h>
#include <current.h>
#include <syscall.h>

void sys_exit(int _exitcode) {

	curthread->status = _MKWAIT_EXIT(_exitcode);
	thread_exit();
}
