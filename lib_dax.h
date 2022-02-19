#ifndef LIB_DAX_H
#define LIB_DAX_H

#include <pthread.h>
#include <inttypes.h>

#define MAP_SIZE (uint64_t)0x1000<<29
// #define  RAM_VER
extern int dax_fd;

#define DAX_MPK_DEFAULT		0
#define DAX_MPK_META		1
#define DAX_MPK_FILE		2

// #define RAM_VER
enum dax_ioctl_types{
    DAX_IOCTL_INIT = 16,
    DAX_IOCTL_READY,
    DAX_IOCTL_RESET,
    DAX_IOCTL_PSWAP,
    DAX_IOCTL_PREFAULT,
    DAX_IOCTL_COW,
    DAX_IOCTL_COPYTEST,
};

struct dax_ioctl_pswap {
    void* ufirst;
    void* usecond;
    uint64_t npgs;
    uint64_t flag;
};
typedef struct dax_ioctl_pswap dax_ioctl_pswap_t;

struct dax_ioctl_init {
    // to kernel: virtual memory size
    uint64_t size;
    // from kernel: physical space total
    uint64_t space_total;
    // from kernel: physical space remaining
    uint64_t space_remain;
    // from kernel: protection tag for metadata
    uint8_t mpk_meta;
    // from kernel: protection tag for file data
    uint8_t mpk_file;
    // from kernel: protection tag for default
    uint8_t mpk_default; 
};
typedef struct dax_ioctl_init dax_ioctl_init_t;

struct dax_ioctl_prefault {
    void* addr;
    uint64_t n_pmd;
};
typedef struct dax_ioctl_prefault dax_ioctl_prefault_t;

struct dax_cow_frame {
    uint64_t src;
    uint64_t dest;
    uint64_t size;
};
typedef struct dax_cow_frame dax_cow_frame_t;

long dax_reset(const char * path, uint64_t dax_size);

long dax_pswap(dax_ioctl_pswap_t *frame);

long dax_prefault(dax_ioctl_prefault_t * frame);

long dax_init(dax_ioctl_init_t* frame);

long dax_ready();

void* dax_start(const char * path, dax_ioctl_init_t* frame);

void dax_end();

void dax_test_cpy(void * buf);

void dax_stop_access(int key);

void dax_stop_write(int key);

void dax_grant_access(int key);

#endif