#ifndef CTFS_TYPE_H
#define CTFS_TYPE_H
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/statfs.h>
#include <sys/vfs.h>
#include <linux/magic.h>
#include <linux/falloc.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <error.h>
#include <errno.h>
#include <pthread.h>
#include <inttypes.h>
#include <time.h>
#include <x86intrin.h>
#include <stdarg.h> 

#include "ctfs_config.h"
#include "lib_dax.h"
#include "ctfs_util.h"

#ifdef __GNUC__
#define likely(x)       __builtin_expect((x), 1)
#define unlikely(x)     __builtin_expect((x), 0)
#else
#define likely(x)       (x)
#define unlikely(x)     (x)
#endif

/***********************************************
 * clear cache utility
 ***********************************************/
#define cache_wb_one(addr) _mm_clwb(addr)

#define cache_wb(target, size)              \
    for(size_t i=0; i<size; i+=64){         \
        _mm_clwb((void*)target + i);        \
    }                                       \
    _mm_clwb((void*)target + size - 1);


/* Convensions */
#define PAGE_MASK   (~((uint64_t)0x0fff))

typedef uint64_t index_t;
typedef uint64_t relptr_t;   // relative pointer
typedef int8_t pgg_level_t;

#define CT_ABS2REL(addr)    (((relptr_t)(addr)) - ct_rt.base_addr)
#define CT_REL2ABS(addr)    ((void*)((addr) + ct_rt.base_addr))


/* end of Convensions */

/******************************************
 * In-pmem structures 
 ******************************************/

/* inode. No need for block map
 * 128B. Provides same functionality as ext4
 */
struct ct_inode{
	// 64-bit fields
    ino_t       i_number;
    size_t      i_size;
    relptr_t    i_block;
	size_t		i_ndirent;      // indicate the size of inode table
    nlink_t     i_nlink;

	// 32-bit fields
	uint32_t	i_lock;
    uid_t       i_uid;
    gid_t       i_gid;
    mode_t      i_mode;
	// 16-bit fields
    pgg_level_t i_level;
    char        i_finish_swap;

    // times
    struct timespec i_atim; /* Time of last access */
    struct timespec i_mtim; /* Time of last modification */
    struct timespec i_ctim; /* Time of last status change */
    time_t          i_otim; /* Time of last open */

	// 8-bit fields
	

    /* Padding.
     * 128B - 120B = 8
     */
    char        padding[7];
};
typedef struct ct_inode ct_inode_t;
typedef ct_inode_t* ct_inode_pt;

struct ct_dirent{
    __ino64_t d_ino;
    __off64_t d_off;
    unsigned short int d_reclen;
    unsigned char d_type;
    char d_name[CT_MAX_NAME + 1];
};
typedef struct ct_dirent ct_dirent_t;
typedef ct_dirent_t* ct_dirent_pt;

/******************************************
 * In-RAM structures 
 ******************************************/

#endif