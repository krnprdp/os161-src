#include <types.h>
#include <kern/errno.h>
#include <kern/fcntl.h>
#include <kern/wait.h>
#include <mips/trapframe.h>
#include <lib.h>
#include <addrspace.h>
#include <thread.h>
#include <copyinout.h>
#include <synch.h>
#include <current.h>
#include <syscall.h>

void sys_exit(int _exitcode) {

	ptable[curthread->t_pid]->exited = true;
	ptable[curthread->t_pid]->exitcode = _MKWAIT_EXIT(_exitcode);

	thread_exit();
}

int sys_fork(struct trapframe* tf, pid_t *retval) {

	struct trapframe *child_tf;
	struct addrspace* child_addr;
	struct thread* child;

	// Copy parent’s address space
	child_addr = (struct addrspace*) kmalloc(sizeof(struct addrspace));

	if (as_copy(curthread->t_addrspace, &child_addr)) {
		return ENOMEM;
	}

	//Copy parent’s trap frame, and pass it to child thread
	child_tf = (struct trapframe *) kmalloc(sizeof(struct trapframe));
	*child_tf = *tf;

	//Create child thread (using thread_fork)
	if (thread_fork("child", child_forkentry, (struct trapframe *) child_tf,
			(unsigned long) child_addr, &child)) {
		//panic("thread_fork");
	}

	//Parent returns with child’s pid
	*retval = child->t_pid;
	return 0;

}

void child_forkentry(void *data1, unsigned long data2) {

	struct trapframe tf;

	//Load address space into child thread and activate it
	curthread->t_addrspace = (struct addrspace*) data2;
	as_activate(curthread->t_addrspace);

	//Copy the trapframe
	tf = *((struct trapframe *) data1);

	tf.tf_v0 = 0;
	tf.tf_a3 = 0;
	tf.tf_epc += 4;
	mips_usermode(&tf);
}

int sys_getpid(pid_t *retval) {
	*retval = curthread->t_pid;
	return 0;
}

int sys_waitpid(pid_t pid, int *status, int options, pid_t *retval) {

	if (options != 0) {
		*retval = EINVAL;
		return 0;
	}

	if (!ptable[pid]->exited)
		P(ptable[pid]->sem);

	if (copyout((const void*) &(ptable[pid]->exitcode), (userptr_t) status,
			sizeof(int))) {
		*retval = EFAULT;
		return -1;
	}

	ptable[pid]->ppid = -1;
	ptable[pid] = NULL;

	*retval = pid;
	return 0;

}

