#ifndef CTFS_UTIL_H
#define CTFS_UTIL_H

#include <stdint.h>
#include <stddef.h>
#include <x86intrin.h>
#include <pthread.h>
#include <bits/pthreadtypes.h>
#include <inttypes.h>

/************************************************ 
 * Implement utility functions for synchronization
 ************************************************/
#define FETCH_AND_SET_BIT(addr, num)                         \
__sync_fetch_and_or((char*) (((uint64_t)addr)+(num/8)),      \
((char)0x01 << (num%8)))

#define FETCH_AND_unSET_BIT(addr, num)                              \
__sync_and_and_fetch((char*) (((uint64_t)addr)+(num/8)),      \
~((char)0x01 << (num%8))) 

typedef volatile int ctfs_lock_t;

#define ctfs_lock_try(lock)     pthread_spin_trylock(&lock)
#define ctfs_lock_acquire(lock) pthread_spin_lock(&lock)
#define ctfs_lock_release(lock) pthread_spin_unlock(&lock)
#define ctfs_lock_init(lock) pthread_spin_init(&lock, PTHREAD_PROCESS_PRIVATE)

// void bit_lock_acquire(uint64_t *addr, uint64_t num);
// void bit_lock_release(uint64_t *addr, uint64_t num);


/************************************************ 
 * Implement utility functions for bitmap
 ************************************************/
#define BITS_PER_BYTE  8
#define ARRAY_INDEX(b) ((b) / BITS_PER_BYTE) // Given the block number, calculate the index to the uint array
#define BIT_OFFSET(b)  ((b) % BITS_PER_BYTE) // Calculate the bit position within the located uint element

void set_bit (uint64_t *bitmap, size_t b);

void clear_bit (uint64_t *bitmap, size_t b);

int get_bit (uint64_t *bitmap, size_t b);

int64_t find_free_bit_tiny(uint64_t *bitmap, size_t size);

int64_t find_free_bit (uint64_t *bitmap, size_t size, size_t hint);

void bitlock_acquire(uint64_t *bitlock, uint64_t location);

int bitlock_try_acquire(uint32_t *bitlock, uint32_t bit, uint32_t tries);

void bitlock_release(uint64_t *bitlock, uint64_t location);

void avx_cpy(void *dest, const void *src, size_t size);

void avx_cpyt(void *dest, void *src, size_t size);

// void big_memcpy(void *dest, const void *src, size_t n);

/***********************************************
 * Debug
 ***********************************************/
// #define CTFS_DEBUG 2
#ifdef CTFS_DEBUG
extern uint64_t ctfs_debug_temp;
#endif

#endif //CTFS_UTIL_H
