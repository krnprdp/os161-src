#include <types.h>
#include <kern/errno.h>
#include <kern/fcntl.h>
#include <kern/wait.h>
#include <mips/trapframe.h>
#include <lib.h>
#include <addrspace.h>
#include <thread.h>
#include <synch.h>
#include <current.h>
#include <syscall.h>

void sys_exit(int _exitcode) {

	(void) _exitcode;
	curthread->status = _MKWAIT_EXIT(_exitcode);

	//V(curthread->t_sem);
	thread_exit();
}

int sys_fork(struct trapframe* tf, pid_t *retval) {

	int result;
	struct trapframe *child_tf;
	struct addrspace* child_addr;
	struct thread* child;

	//Copy parent’s trap frame, and pass it to child thread
	child_tf = kmalloc(sizeof(struct trapframe*));
	*child_tf = *tf;

	// Copy parent’s address space
	//child_addr = kmalloc(sizeof(struct addrspace));
	result = as_copy(curthread->t_addrspace, &child_addr);
	curthread->t_sem = sem_create("sem", 0);

	P(curthread->t_sem);

	//Create child thread (using thread_fork)
	result = thread_fork("child", child_forkentry,
			(struct trapframe *) child_tf, (unsigned long) child_addr, &child);

	//Parent returns with child’s pid immediately
	*retval = child->t_pid;

	return 0;

}

void child_forkentry(void *data1, unsigned long data2) {

	struct trapframe tf;
	//kprintf("getting here?");
	//Load address space into child thread and activate it
	curthread->t_addrspace = (struct addrspace*) data2;
	as_activate(curthread->t_addrspace);

	//Copy the trapframe
	tf = *((struct trapframe *) data1);

	tf.tf_v0 = 0;
	tf.tf_a3 = 0;
	tf.tf_epc += 4;
	//kprintf("getting here??");
	V(curthread->t_sem);
	mips_usermode(&tf);

}

int sys_getpid(pid_t *retval) {

	*retval = curthread->t_pid;
	return 0;
}

