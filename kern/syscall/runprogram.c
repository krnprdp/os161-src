/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2009
 *	The President and Fellows of Harvard College.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Sample/test code for running a user program.  You can use this for
 * reference when implementing the execv() system call. Remember though
 * that execv() needs to do more than this function does.
 */

#include <types.h>
#include <kern/errno.h>
#include <kern/fcntl.h>
#include <lib.h>
#include <thread.h>
#include <current.h>
#include <addrspace.h>
#include <vm.h>
#include <vfs.h>
#include <syscall.h>
#include <test.h>
#include <synch.h>

/*
 * Load program "progname" and start running it in usermode.
 * Does not return except on error.
 *
 * Calls vfs_open on progname and thus may destroy it.
 */
int runprogram(char *progname) {
	struct vnode *v;
	vaddr_t entrypoint, stackptr;
	int result;

	/*Initialize Console*/

	char *consolein;
	char *consoleout;
	char *consoleerr;

	consolein = kstrdup("con:");
	consoleout = kstrdup("con:");
	consoleerr = kstrdup("con:");

	if (vfs_open(consolein, O_RDONLY, 0, &curthread->t_fdtable[0]->vn)) {
		return EINVAL;
	}
	kfree(consolein);
	curthread->t_fdtable[0]->name = consolein;
	curthread->t_fdtable[0]->flag = O_RDONLY;
	curthread->t_fdtable[0]->ref_count = 1;
	curthread->t_fdtable[0]->filelock = lock_create("STDIN");

	if (vfs_open(consoleout, O_WRONLY, 0, &curthread->t_fdtable[1]->vn)) {
		return EINVAL;
	}
	kfree(consoleout);
	curthread->t_fdtable[1]->name = consoleout;
	curthread->t_fdtable[1]->flag = O_WRONLY;
	curthread->t_fdtable[1]->ref_count = 1;
	curthread->t_fdtable[1]->filelock = lock_create("STDOUT");

	if (vfs_open(consoleerr, O_WRONLY, 0, &curthread->t_fdtable[2]->vn)) {
		return EINVAL;
	}
	kfree(consoleerr);
	curthread->t_fdtable[2]->name = consoleerr;
	curthread->t_fdtable[2]->flag = O_WRONLY;
	curthread->t_fdtable[2]->ref_count = 1;
	curthread->t_fdtable[2]->filelock = lock_create("STDERR");

	/*Console Initialized*/

	/* Open the file. */
	result = vfs_open(progname, O_RDONLY, 0, &v);
	if (result) {
		return result;
	}

	/* We should be a new thread. */
	KASSERT(curthread->t_addrspace == NULL);

	/* Create a new address space. */
	curthread->t_addrspace = as_create();
	if (curthread->t_addrspace == NULL ) {
		vfs_close(v);
		return ENOMEM;
	}

	/* Activate it. */
	as_activate(curthread->t_addrspace);

	/* Load the executable. */
	result = load_elf(v, &entrypoint);
	if (result) {
		/* thread_exit destroys curthread->t_addrspace */
		vfs_close(v);
		return result;
	}

	/* Done with the file now. */
	vfs_close(v);

	/* Define the user stack in the address space */
	result = as_define_stack(curthread->t_addrspace, &stackptr);
	if (result) {
		/* thread_exit destroys curthread->t_addrspace */
		return result;
	}

	/* Warp to user mode. */
	enter_new_process(0 /*argc*/, NULL /*userspace addr of argv*/, stackptr,
			entrypoint);

	/* enter_new_process does not return. */
	panic("enter_new_process returned\n");

	return EINVAL;
}

