/********************************
 * 
 * 
 * 
 *******************************/

#include "ctfs_runtime.h"

#ifdef CTFS_DEBUG
uint64_t ctfs_debug_temp;
#endif

ct_runtime_t ct_rt;


struct timespec stopwatch_start;
struct timespec stopwatch_stop;

void timer_start(){
	clock_gettime(CLOCK_MONOTONIC, &stopwatch_start);
}

uint64_t timer_end(){
	clock_gettime(CLOCK_MONOTONIC, &stopwatch_stop);
	return calc_diff(stopwatch_start, stopwatch_stop);
}


ct_runtime_t* get_rt(){
    return &ct_rt;
}

void ct_time_stamp(struct timespec * time){
    clock_gettime(CLOCK_REALTIME, time);
}

int ct_time_greater(struct timespec * time1, struct timespec * time2){
    if(time1->tv_sec > time2->tv_sec){
        return 1;
    }
    else if(time1->tv_sec < time2->tv_sec){
        return 0;
    }
    else if(time1->tv_nsec > time2->tv_nsec){
        return 1;
    }
    else
        return 0;
}
