//
// Created by Ding Yuan on 2018-01-15.
//

#include <assert.h>
#include <sys/time.h>
#include <errno.h>
#include <sys/syscall.h>
#include <stdlib.h>
#include <stdio.h>

#include "ctfs_type.h"


inline void bit_lock_acquire(uint64_t *addr, uint64_t num){
    while(FETCH_AND_SET_BIT(addr, num) & ((char)0x01 << num%8)){
        //spinlock
    }
}
inline void bit_lock_release(uint64_t *addr, uint64_t num){
    FETCH_AND_unSET_BIT(addr, num);
}
