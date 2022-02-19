//
// Created by Ding Yuan on 2018-01-10.
//
#include <assert.h>
#include "ctfs_util.h"
#include "ctfs_type.h"


// Implement utility functions for bitmap, which is used to maintain free blocks

void set_bit (uint64_t *bitmap, size_t b) {
    FETCH_AND_SET_BIT(bitmap, b);
    _mm_clwb(&((uint8_t*)bitmap)[ARRAY_INDEX(b)]);
}

void clear_bit (uint64_t *bitmap, size_t b) {
    FETCH_AND_unSET_BIT(bitmap, b);
    _mm_clwb((void*)((uint64_t)bitmap)+(b/8));
    // set struct block permission
}

int get_bit (uint64_t *bitmap, size_t b) {
    uint8_t bit = ((uint8_t*)bitmap)[ARRAY_INDEX(b)] & (1 << BIT_OFFSET(b));
    return bit != 0;
}


int64_t find_free_bit_tiny(uint64_t *bitmap, size_t size){
    assert(size <= 64);
    for(uint8_t i = 0; i < size; i++){
        if((*bitmap & ((uint64_t)0b01 << i)) == 0){
            // found
            return (int64_t)i;
        }
    }
    return -1;
}

/* find a free bit,
 * not set it. 
 * @bitmap[in]  pointer ot the bitmap
 * @size[in]    total number of bits in bitmap
 * @hint[in]    search starting point, in bit
 * @return      nth bit. 
 */
int64_t find_free_bit (uint64_t *bitmap, size_t size, size_t hint){
    assert(size%64 == 0);
    uint64_t limit_64 = size >> 6;
    uint64_t start_64 = hint >> 6;
    uint64_t nth_64 = start_64;
    uint64_t ret;
    do{
        if(unlikely(nth_64 >= limit_64)){
            if(start_64 == 0){
                goto failed;
            }
            else{
                nth_64 = 0;
            }
        }
        else if(bitmap[nth_64] == ~(uint64_t)0x0){
            nth_64++;
        }
        else{
            goto found_64;
        }
    }while(nth_64 != start_64);
    return -1;
found_64:
    // printf("%lx\n", bitmap[nth_64]);
    for(uint8_t i = 0; i < 64; i++){
        if((bitmap[nth_64] & ((uint64_t)0b01 << i)) == 0){
            // found
            ret = (nth_64 << 6) + i;
            return (int64_t)ret;
        }
    }
    assert(0);
    // uint64_t ret = size - 64;
    // uint64_t *n_bitmap = bitmap + size/64 - 1;
    // uint8_t* _bitmap;
    // while(1){
    //     if(unlikely(ret>=size)){
    //         goto failed;
    //     }
    //     if(*n_bitmap == ~(uint64_t)(0x0)){
    //         ret -= 64;
    //         n_bitmap--;
    //         continue;
    //     }
    //     else{
    //         break;
    //     }
    // }
    // _bitmap = (uint8_t*)n_bitmap;
    // while(1){
    //     if(*_bitmap == 0xFF){
    //         ret+=8;
    //         _bitmap++;
    //         continue;
    //     }
    //     else{
    //         break;
    //     }
    // }
    // while(1){
    //     if( ((*_bitmap) & (1 << BIT_OFFSET(ret))) == 0){
    //         return ret;
    //     }
    //     else{
    //         ret++;
    //     }
    // }

failed:
    return -1;
}

void bitlock_acquire(uint64_t *bitlock, uint64_t location){
    uint64_t * target = bitlock + (location / 64);
    uint64_t offset = location % 64;
    while((((uint64_t)0b01 << offset) & __sync_fetch_and_or(target, (uint64_t)0b01 << offset)) != 0){
        // pthread_yield();
    }
}

int bitlock_try_acquire(uint32_t *bitlock, uint32_t bit, uint32_t tries){
    uint32_t original = *bitlock & ~(bit);
    while(original != __sync_fetch_and_or(bitlock, tries)){
        original = *bitlock & ~(bit);
        tries --;
        if(tries == 0){
            return 0;
        }
    }
    return 1;
}

void bitlock_release(uint64_t *bitlock, uint64_t location){
    FETCH_AND_unSET_BIT(bitlock, location);
}
