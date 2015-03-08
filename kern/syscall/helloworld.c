#include <syscall.h>
#include <lib.h>
//#include <helloworld.h>
int helloworld(){
	kprintf("Hello world! This is a system call.");
	return 0;
}
