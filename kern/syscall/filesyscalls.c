#include <types.h>
#include <kern/errno.h>
#include <kern/fcntl.h>
#include <current.h>
#include <lib.h>
#include <limits.h>
#include <copyinout.h>
#include <synch.h>
#include <vfs.h>
#include <syscall.h>

int sys_open(userptr_t filename, int flags, int *retval) {

	int fd = 0;
	int result;
	char *kbuf;
	size_t *len;

	kbuf = kmalloc(sizeof(filename));

	if ((result = copyinstr((const_userptr_t) filename, kbuf, PATH_MAX, len))
			!= 0) {
		return result;
	}

//	if (flags != (O_RDONLY || O_WRONLY || O_RDWR)) {
//		return result;
//	}

	while (curthread->t_fdtable[fd] != 0) {
		fd++;
	}

	curthread->t_fdtable[fd] = (struct fdesc*) kmalloc(sizeof(struct fdesc));

	if (vfs_open(kbuf, flags, 0, &(curthread->t_fdtable[fd]->vn))) {
		return EINVAL;
	}

	curthread->t_fdtable[fd]->name = kbuf;
	curthread->t_fdtable[fd]->flag = flags;
	curthread->t_fdtable[fd]->ref_count = 1;
	curthread->t_fdtable[fd]->offset = 0;
	curthread->t_fdtable[fd]->filelock = lock_create(kbuf);

	*retval = fd;
	return 0;
}

int sys_close(int fd, int *retval) {

	if (fd > OPEN_MAX) {
		return EBADF;
	}
	if (curthread->t_fdtable[fd] == 0) {
		return EBADF;
	}

	curthread->t_fdtable[fd]->ref_count--;

	if (curthread->t_fdtable[fd]->ref_count == 0) {

		vfs_close(curthread->t_fdtable[fd]->vn);
		kfree(curthread->t_fdtable[fd]->name);
		lock_destroy(curthread->t_fdtable[fd]->filelock);
		curthread->t_fdtable[fd]->offset = 0;

		curthread->t_fdtable[fd] = 0;

	}
	*retval = 0;
	return 0;
}
