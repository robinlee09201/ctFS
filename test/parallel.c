#include "../lib_dax.h"
#include "../ctfs.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <pthread.h>
#include <inttypes.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

long calc_diff(struct timespec start, struct timespec end){
	return (end.tv_sec * (long)(1000000000) + end.tv_nsec) -
	(start.tv_sec * (long)(1000000000) + start.tv_nsec);
}

enum test_mode{
	TEST_READ,
	TEST_WRITE,
	TEST_ATOMIC_WRITE
};


struct test_frame{
	int tid;
	enum test_mode mode;
	char* folder;
	uint64_t size; 
	uint64_t round;
	uint64_t total_time;
	uint64_t total_bytes;
};
typedef struct test_frame test_frame_t;

uint64_t run_test(test_frame_t * frame){
	char path[256];
	struct timespec stopwatch_start;
	struct timespec stopwatch_stop;
	uint64_t ret = 0, round = frame->round, size = frame->size;
	sprintf(path, "%s/t%d", frame->folder, frame->tid);
	int fd;
	if(frame->mode == TEST_ATOMIC_WRITE){
		fd = open(path, O_RDWR | O_CREAT | CTFS_O_ATOMIC,  S_IRWXU);
	}
	else {
		fd = open(path, O_RDWR | O_CREAT,  S_IRWXU);
	}
	if(fd < 0){
		printf("Thread %d error: open %s failed!\n", frame->tid, path);
		return 0;
	}

	char * buf = malloc(frame->size);
	for(uint64_t i = 0; i < size; i++){
		buf[i] = i % 128;
	}
	clock_gettime(CLOCK_MONOTONIC, &stopwatch_start);
	switch (frame->mode)
	{
	case TEST_READ:
		for(uint64_t i = 0; i < round; i++){
			ret += pread(fd, buf, size, i * size);
		}
		printf("Thread #%d: Finished read %lu\n", frame->tid, ret);
		break;

	case TEST_WRITE:
		for(uint64_t i = 0; i < round; i++){
			ret += pwrite(fd, buf, size, i * size);
		}
		printf("Thread #%d: Finished write %lu\n", frame->tid, ret);
		break;
	case TEST_ATOMIC_WRITE:
		for(uint64_t i = 0; i < round; i++){
			ret += pwrite(fd, buf, size, i * size);
		}
		printf("Thread #%d: Finished atomic write %lu\n", frame->tid, ret);
		break;
	default:
		break;
	}
	clock_gettime(CLOCK_MONOTONIC, &stopwatch_stop);
	frame->total_time = (uint64_t)calc_diff(stopwatch_start, stopwatch_stop);
	printf("Thread #%d: total time: %lu ns\n", frame->tid, frame->total_time);
	free(buf);
	close(fd);
	frame->total_bytes = ret;
	return ret;
}

int main(int argc, char ** argv){
	if(argc < 6){
		printf("usage: path_to_folder num_thread size round op\n");
		return -1;
	}
	// struct timespec stopwatch_start;
	// struct timespec stopwatch_stop;
	pthread_attr_t attr;
	uint64_t total_bytes = 0, total_time = 0, real_time;
	char * path = argv[1];
	int num_thread = atoi(argv[2]);
	long long size = atoll(argv[3]);
	long long round = atoll(argv[4]);
	enum test_mode mode = atoi(argv[5]);
	printf("start test on %d threads, size: %ld, round: %ld, mode: %d\n", num_thread, size, round, mode);
	printf("\tpath: %s\n", path);
	test_frame_t *frames = malloc(num_thread * sizeof(test_frame_t));
	pthread_t *threads = malloc(num_thread * sizeof(pthread_t));
	pthread_attr_init(&attr);
	for(int i=0; i<num_thread; i++){
		frames[i] = (test_frame_t){.folder = path, 
		.mode = mode, 
		.round = round, 
		.size = size,
		.tid = i};
		pthread_create(&threads[i], &attr, run_test, &frames[i]);
	}
	// clock_gettime(CLOCK_MONOTONIC, &stopwatch_start);
	for(int i=0; i<num_thread; i++){
		uint64_t ret;
		pthread_join(threads[i], (void**)&ret);
		total_bytes += frames[i].total_bytes;
		total_time += frames[i].total_time;
	}
	// clock_gettime(CLOCK_MONOTONIC, &stopwatch_stop);
	real_time = total_time / num_thread;
	printf("%d threads performed %lu of I/O in %lu ns\n", num_thread, total_bytes, real_time);
	printf("\ttotal time: %lu\n", total_time);
	printf("\tthroughput: %f GB/s\n", (double)total_bytes / (double)real_time);
	return 0;
}

