#define _GNU_SOURCE
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include "lib_dax.h"
#ifdef RAM_VER
#include "ctfs_type.h"
#endif
int dax_fd = -1;
// pthread_spinlock_t dax_lock;

void dax_stop_access(int key){
	pkey_set(key, PKEY_DISABLE_ACCESS);
}

void dax_stop_write(int key){
	pkey_set(key, PKEY_DISABLE_WRITE);
}

void dax_grant_access(int key){
	pkey_set(key, 0);
}

long dax_reset(const char * path, uint64_t dax_size){
	dax_fd = open(path, O_RDWR);
	if(dax_fd == -1){
		return -1;
	}
	void * addr = mmap(NULL, MAP_SIZE ,PROT_WRITE | PROT_READ, MAP_SHARED ,dax_fd ,0);
	long ret = ioctl(dax_fd, DAX_IOCTL_RESET, (uint64_t)addr);
	munmap(addr, MAP_SIZE);
	// dax_init(dax_size);
	close(dax_fd);
	dax_fd = -1;
	return ret;
}

long dax_pswap(dax_ioctl_pswap_t *frame){
#ifdef RAM_VER
	memcpy((void*)(frame->ufirst), (void*)(frame->usecond), frame->npgs * 4096);
	
	cache_wb((void*)frame->ufirst, frame->npgs * 4096);
	return 0;
#else
	// pthread_spin_lock(&dax_lock);
	// printf("pswap %u!\n", frame->npgs);
	long ret = ioctl(dax_fd, DAX_IOCTL_PSWAP, (uint64_t)frame);
	// pthread_spin_unlock(&dax_lock);            
	return ret;
#endif
}

long dax_prefault(dax_ioctl_prefault_t * frame){
	return ioctl(dax_fd, DAX_IOCTL_PREFAULT, (uint64_t)frame);
}

long dax_init(dax_ioctl_init_t* frame){
	long ret = ioctl(dax_fd, DAX_IOCTL_INIT, (uint64_t)frame);
	return ret;
}

long dax_ready(){
	int ret;
	ioctl(dax_fd, DAX_IOCTL_READY, &ret);
	return ret;
};

void* dax_start(const char * path, dax_ioctl_init_t* frame){
	dax_fd = open(path, O_RDWR);
	if(dax_fd == -1){
		return NULL;
	}
	void * ret = mmap(NULL, MAP_SIZE ,PROT_WRITE | PROT_READ, MAP_SHARED ,dax_fd ,0);
	if(ret == NULL){
		return NULL;
	}
	// pthread_spin_init(&dax_lock, PTHREAD_PROCESS_PRIVATE);
	// if(!dax_ready()){
		dax_init(frame);
	// } 
	return ret;
};

void dax_end(){
	if(dax_fd != -1){
		close(dax_fd);
		dax_fd = -1;
	}
}

void dax_test_cpy(void * buf){
	ioctl(dax_fd, DAX_IOCTL_COPYTEST, (unsigned long)buf);
}