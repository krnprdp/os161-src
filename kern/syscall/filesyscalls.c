
#include <syscall.h>
#include <lib.h>
#include <types.h>
//#include <errno.h>
//#include <fcntl.h>

int sys_open(userptr_t filename, int flags,int *retval) {
	//KASSERT(filename != NULL);

//	if (filename == NULL ) {
//		err = EFAULT;
//		return -1;
//	}
//
//	if (flags != O_RDONLY || flags != O_WRONLY || flags != O_RDWR) {
//		err = EINVAL;
//		return -1;
//	}

	(void) flags;
	(void) filename;
	retval = 0;
	return 0;
}
