#include "../lib_dax.h"
#include "../ctfs_runtime.h"
#include <time.h>

int main(int argc, char ** argv){
	struct timespec stopwatch_start;
    struct timespec stopwatch_stop;
	long time_diff;
	if(argc < 2){
		printf("need number of pages!\n");
		return -1;
	}
	int num = atoi(argv[1]);
	uint64_t offset = 0;
	if(argc == 3){
		offset = 4096 * atoll(argv[2]);
		printf("offset %ul \n", offset);
	}
	
	// dax_reset("/dev/dax0.0", CT_DAX_ALLOC_SIZE);
	dax_ioctl_init_t frame = {.size = CT_DAX_ALLOC_SIZE};
	void* base_addr = dax_start("/dev/dax0.0", &frame);
	dax_grant_access(frame.mpk_default);
	char ret = getchar();
	if(ret == 's'){
		memset(base_addr + offset, 0, num * CT_PAGE_SIZE);
		memset(base_addr + offset + pgg_size[9], 1, num * CT_PAGE_SIZE);
	}
	
	printf("pswap %u pages \nbefore: f: %lx, s: %lx\n", num,  *(uint64_t*)(base_addr + offset), *(uint64_t*)(base_addr + offset + pgg_size[9]));
	dax_ioctl_pswap_t ps_frame = {.npgs = num, .ufirst = base_addr + offset, .usecond = base_addr + offset + pgg_size[9]};
	clock_gettime(CLOCK_MONOTONIC, &stopwatch_start);
	dax_pswap(&ps_frame);
	clock_gettime(CLOCK_MONOTONIC, &stopwatch_stop);
	time_diff = calc_diff(stopwatch_start, stopwatch_stop);
	printf("after: f: %lx, s: %lx\n", *(uint64_t*)(base_addr + offset), *(uint64_t*)(base_addr  + offset + pgg_size[9]));
	printf("time used: %ld ns\nspeed: %f GB/s\n\n", time_diff, (double)(num * CT_PAGE_SIZE) / (double)time_diff);
	return 0;
}
