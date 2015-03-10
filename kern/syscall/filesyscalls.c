#include <types.h>
#include <kern/errno.h>
#include <kern/fcntl.h>
#include <kern/iovec.h>
#include <kern/seek.h>
#include <kern/stat.h>
#include <current.h>
#include <lib.h>
#include <limits.h>
#include <copyinout.h>
#include <synch.h>
#include <vfs.h>
#include <uio.h>
#include <vnode.h>
#include <syscall.h>

int sys_open(userptr_t filename, int flags, int *retval) {

	int fd = 0;
	int result;
	char *kbuf;
	size_t len;

	kbuf = kmalloc(sizeof(filename));

	if ((result = copyin((const_userptr_t) filename, kbuf, len)) != 0) {
		return ENODEV;
	}

//	if (flags != (O_RDONLY || O_WRONLY || O_RDWR)) {
//		return result;
//	}

	while (curthread->t_fdtable[fd] != 0) {
		fd++;
	}

	curthread->t_fdtable[fd] = (struct fdesc*) kmalloc(sizeof(struct fdesc));

	if (vfs_open(kbuf, flags, 0664, &(curthread->t_fdtable[fd]->vn))) {
		panic("EINVAL");
	}

	curthread->t_fdtable[fd]->name = kbuf;
	curthread->t_fdtable[fd]->flag = flags;
	curthread->t_fdtable[fd]->ref_count = 1;
	curthread->t_fdtable[fd]->offset = 0;
	curthread->t_fdtable[fd]->filelock = lock_create(kbuf);

	kfree(kbuf);
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

int sys_read(int fd, void *buf, size_t buflen, int *retval) {

	int amt_read;
	struct iovec *iov;
	struct uio *u;

	iov = kmalloc(sizeof(struct iovec));
	u = kmalloc(sizeof(struct uio));

	if (fd > OPEN_MAX) {
		return EBADF;
	}
	if (curthread->t_fdtable[fd] == 0) {
		return EBADF;
	}

	if ((curthread->t_fdtable[fd]->flag != O_RDONLY)
			|| (curthread->t_fdtable[fd]->flag != O_RDWR)) {
		return EBADF;
	}

	lock_acquire(curthread->t_fdtable[fd]->filelock);

	iov->iov_ubase = buf;
	iov->iov_len = buflen;

	u->uio_iov = iov;
	u->uio_iovcnt = 1;
	u->uio_offset = curthread->t_fdtable[fd]->offset;
	u->uio_resid = buflen;
	u->uio_segflg = UIO_USERSPACE;
	u->uio_rw = UIO_READ;
	u->uio_space = curthread->t_addrspace;

	VOP_READ(curthread->t_fdtable[fd]->vn, u);

	amt_read = buflen - u->uio_resid;
	curthread->t_fdtable[fd]->offset = amt_read;

	lock_release(curthread->t_fdtable[fd]->filelock);

	kfree(iov);
	kfree(u);
	*retval = amt_read;
	return 0;
}

int sys_write(int fd, void *buf, size_t nbytes, int *retval) {

	int amt_written;
	struct iovec *iov;
	struct uio *u;

	iov = kmalloc(sizeof(struct iovec));
	u = kmalloc(sizeof(struct uio));

	if (fd > OPEN_MAX) {
		return EBADF;
	}
	if (curthread->t_fdtable[fd] == 0) {
		return EBADF;
	}

	if ((curthread->t_fdtable[fd]->flag != O_WRONLY)
			|| (curthread->t_fdtable[fd]->flag != O_RDWR)) {

		return EBADF;
	}

	lock_acquire(curthread->t_fdtable[fd]->filelock);

	iov->iov_ubase = buf;
	iov->iov_len = nbytes;

	u->uio_iov = iov;
	u->uio_iovcnt = 1;
	u->uio_offset = curthread->t_fdtable[fd]->offset;
	u->uio_resid = nbytes;
	u->uio_segflg = UIO_USERSPACE;
	u->uio_rw = UIO_WRITE;
	u->uio_space = curthread->t_addrspace;

	VOP_WRITE(curthread->t_fdtable[fd]->vn, u);

	amt_written = nbytes - u->uio_resid;
	curthread->t_fdtable[fd]->offset = amt_written;

	lock_release(curthread->t_fdtable[fd]->filelock);

	kfree(iov);
	kfree(u);
	*retval = amt_written;
	return 0;
}
off_t sys_lseek(int fd, off_t pos, int whence, off_t *retval64) {

	struct stat eof;

	if (pos < 0) {
		return EINVAL;
	}
	if (fd > OPEN_MAX) {
		return EBADF;
	}
	if (curthread->t_fdtable[fd] == 0) {
		return EBADF;
	}
	if (whence != SEEK_CUR || whence != SEEK_SET || whence != SEEK_END) {
		return EINVAL;
	}

	//eof = kmalloc(sizeof(struct stat));

	lock_acquire(curthread->t_fdtable[fd]->filelock);

	switch (whence) {
	case SEEK_CUR:
		curthread->t_fdtable[fd]->offset += pos;
		break;
	case SEEK_SET:
		curthread->t_fdtable[fd]->offset = pos;
		break;
	case SEEK_END:
		VOP_STAT(curthread->t_fdtable[fd]->vn, &eof);
		curthread->t_fdtable[fd]->offset = eof.st_size + pos;
		break;
	}

	VOP_TRYSEEK(curthread->t_fdtable[fd]->vn, curthread->t_fdtable[fd]->offset);
	*retval64 = curthread->t_fdtable[fd]->offset;

	lock_release(curthread->t_fdtable[fd]->filelock);
	return 122; //for lseek custom value
}
