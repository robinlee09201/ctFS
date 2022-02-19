#include "ctfs.h"
#include "ctfs_pgg.h"
#include "ctfs_runtime.h"
#define _GNU_SOURCE
// #define TEST_DRAM
#ifdef CTFS_DEBUG
int ctfs_pause = 1;
#endif
static inline void big_memcpy(void *dest, const void *src, size_t n)
{
	long d0, d1, d2;
	asm volatile(
		"rep ; movsq\n\t"
		"movq %4,%%rcx\n\t"
		"rep ; movsb\n\t"
		: "=&c" (d0), "=&D" (d1), "=&S" (d2)
		: "0" (n >> 3), "g" (n & 7), "1" (dest), "2" (src)
		: "memory");

}

static inline void *intel_memcpy(void * __restrict__ b, const void * __restrict__ a, size_t n){
	char *s1 = b;
	const char *s2 = a;
	for(; 0<n; --n)*s1++ = *s2++;
	return b;
}

int ctfs_mkfs(int flag){
#ifdef TEST_DRAM
	ct_rt.base_addr = (uint64_t)malloc(CT_DAX_ALLOC_SIZE);
#else
	if(flag & CTFS_MKFS_FLAG_RESET_DAX){
		printf("ctFS_mkfs: reseting dax...\n");
		dax_reset("/dev/dax0.0", CT_DAX_ALLOC_SIZE);
	}
	dax_ioctl_init_t frame = {.size = CT_DAX_ALLOC_SIZE};
	ct_rt.base_addr = (uint64_t)dax_start("/dev/dax0.0", &frame);
	ct_rt.mpk[DAX_MPK_DEFAULT] = frame.mpk_default;
	ct_rt.mpk[DAX_MPK_FILE] = frame.mpk_file;
	ct_rt.mpk[DAX_MPK_META] = frame.mpk_meta;
	dax_grant_access(ct_rt.mpk[DAX_MPK_DEFAULT]);
#ifdef CTFS_DEBUG
	printf("mpk value: default: %d, file: %d, meta: %d\n", 
	ct_rt.mpk[DAX_MPK_DEFAULT],
	ct_rt.mpk[DAX_MPK_FILE],
	ct_rt.mpk[DAX_MPK_META]);
	dax_grant_access(ct_rt.mpk[DAX_MPK_DEFAULT]);
#endif
#endif
	if(ct_rt.base_addr == 0){
		printf("Failed to init dax\n");
		return -1;
	}
	ct_rt.super_blk = (ct_super_blk_pt)(ct_rt.base_addr);
	ct_super_blk_pt sb = ct_rt.super_blk;
	strcpy(sb->magic, CT_MAGIC);
	// dax_stop_write(ct_rt.mpk[DAX_MPK_DEFAULT]);
	sb->alloc_prot_bmp = CT_OFFSET_PROT;
	sb->alloc_prot_clock = 0;
	sb->first_pgg = CT_OFFSET_1_PGG;
	sb->lvl9_bmp = CT_OFFSET_L9_BMP;
	sb->next_sub_lvl = PGG_LVL0;
	sb->inode_bmp_touched = 0;
	sb->inode_used = 0;
	sb->inode_hint = 0;

	cache_wb(sb, sizeof(ct_super_blk_t));

	ct_rt.alloc_prot = CT_REL2ABS(sb->alloc_prot_bmp);
	ct_rt.lvl9_bmp = CT_REL2ABS(sb->lvl9_bmp);
	ct_rt.first_pgg = CT_REL2ABS(sb->first_pgg);
	ct_rt.inode_bmp = CT_REL2ABS(CT_OFFSET_IBMP);
	ct_rt.inode_start = CT_REL2ABS(CT_OFFSET_ITABLE);
	ctfs_lock_init(ct_rt.inode_bmp_lock);

	// allocate inode
	index_t root_i = inode_alloc();
	assert(root_i == 0);
	root_i = inode_alloc();
	assert(root_i == 1);
	sb->root_inode = root_i;
	// ct_inode_pt root = &ct_rt.inode_start[root_i];
	// fill the inode
	inode_set_root();


	dax_end();
	return 0;
}

int ctfs_init(int flag){
	memset(&ct_rt, 0, sizeof(ct_rt));
	dax_ioctl_init_t frame = {.size = CT_DAX_ALLOC_SIZE};
	ct_rt.base_addr = (uint64_t)dax_start("/dev/dax0.0", &frame);
	ct_rt.super_blk = (ct_super_blk_pt)(ct_rt.base_addr);
	ct_rt.mpk[DAX_MPK_DEFAULT] = frame.mpk_default;
	ct_rt.mpk[DAX_MPK_FILE] = frame.mpk_file;
	ct_rt.mpk[DAX_MPK_META] = frame.mpk_meta;
	dax_grant_access(ct_rt.mpk[DAX_MPK_DEFAULT]);
	assert(strcmp(ct_super->magic, CT_MAGIC)==0);
	ct_super_blk_pt sb = ct_rt.super_blk;
	ct_rt.alloc_prot = CT_REL2ABS(sb->alloc_prot_bmp);
	ct_rt.lvl9_bmp = CT_REL2ABS(sb->lvl9_bmp);
	ct_rt.first_pgg = CT_REL2ABS(sb->first_pgg);
	ct_rt.inode_bmp = CT_REL2ABS(CT_OFFSET_IBMP);
	ct_rt.inode_start = CT_REL2ABS(CT_OFFSET_ITABLE);
	ct_rt.starting_time = time(NULL);
	ct_rt.current_dir = &ct_rt.inode_start[ct_rt.super_blk->root_inode];
	ctfs_lock_init(ct_rt.open_lock);
	ctfs_lock_init(ct_rt.inode_bmp_lock);
	dax_stop_access(ct_rt.mpk[DAX_MPK_DEFAULT]);
	return 0;
}

int ctfs_open (const char *pathname, int flags, ...){
	int fd, res;
	ct_inode_pt c;
	ct_inode_frame_t frame = {.flag = 0, .path = pathname};

	ctfs_lock_acquire(ct_rt.open_lock);
#ifdef CTFS_DEBUG
	printf("****opening %s ******\n", pathname);
#endif
	dax_grant_access(ct_rt.mpk[DAX_MPK_DEFAULT]);
	if(flags & O_CREAT){
		frame.flag |= CT_INODE_FRAME_CREATE;
	}
	if(*pathname != '/'){
		// start from the current dir
		frame.inode_start = ct_rt.current_dir;
	}
	for(fd = 0; fd < CT_MAX_FD; fd++){
		if(ct_rt.fd[fd].inode == NULL){
			break;
		}
	}
	if(fd == CT_MAX_FD){
		ct_rt.errorn = ENFILE;
		dax_stop_access(ct_rt.mpk[DAX_MPK_DEFAULT]);
		ctfs_lock_release(ct_rt.open_lock);
#ifdef CTFS_DEBUG
		ct_rt.fd[fd].cpy_time = 0;
		ct_rt.fd[fd].pswap_time = 0;
		printf("***** Failed open: %s, flag: %x\n****** Due to no fd\n", pathname ,flags);
#endif
		return -1;
	}
	res = inode_path2inode(&frame);
	if(res){
		ct_rt.errorn = res;
		dax_stop_access(ct_rt.mpk[DAX_MPK_DEFAULT]);
		ctfs_lock_release(ct_rt.open_lock);
#ifdef CTFS_DEBUG
		ct_rt.fd[fd].cpy_time = 0;
		ct_rt.fd[fd].pswap_time = 0;
		printf("***** Failed open: %s, flag: %x\n****** Due to file not found\n", pathname ,flags);
#endif
		return -1;
	}
	c = frame.current;
	c->i_otim = time(NULL);
	inode_rt_unlock(frame.current->i_number);

	ct_rt.fd[fd].inode = frame.current;
	ct_rt.fd[fd].offset = 0;
	ct_rt.fd[fd].flags = flags;
	ct_rt.fd[fd].prefaulted_start = 0;
	ct_rt.fd[fd].prefaulted_bytes = 0;
#ifdef CTFS_DEBUG
	ct_rt.fd[fd].cpy_time = 0;
	ct_rt.fd[fd].pswap_time = 0;
	printf("***** #%d opened: %s, flag: %x, inode#: %lu\n",fd , pathname ,flags, frame.current->i_number);
#endif
	dax_stop_access(ct_rt.mpk[DAX_MPK_DEFAULT]);
	ctfs_lock_release(ct_rt.open_lock);
	return fd;
}

int ctfs_openat (int dirfd, const char *pathname, int flags, ...){
	int fd, res;
	ct_inode_pt c;
	ct_inode_frame_t frame = {.flag = 0, .path = pathname};
	ctfs_lock_acquire(ct_rt.open_lock);
	dax_grant_access(ct_rt.mpk[DAX_MPK_DEFAULT]);
	if(flags & O_CREAT){
		frame.flag |= CT_INODE_FRAME_CREATE;
	}
	if(*pathname != '/'){
		// start from the given dir
		if(dirfd == AT_FDCWD){
			frame.inode_start = ct_rt.current_dir;
		}
		else if(dirfd >= CT_MAX_FD || ct_rt.fd[dirfd].inode == NULL){
			ct_rt.errorn = EBADF;
			dax_stop_access(ct_rt.mpk[DAX_MPK_DEFAULT]);
			ctfs_lock_release(ct_rt.open_lock);
			return -1;
		}
		else if((ct_rt.fd[dirfd].inode->i_mode & S_IFDIR) == 0){
			ct_rt.errorn = ENOTDIR;
			dax_stop_access(ct_rt.mpk[DAX_MPK_DEFAULT]);
			ctfs_lock_release(ct_rt.open_lock);
			return -1;
		}
		else{
			frame.inode_start = ct_rt.fd[dirfd].inode;
		}
	}
	for(fd = 0; fd < CT_MAX_FD; fd++){
		if(ct_rt.fd[fd].inode == NULL){
			break;
		}
	}
	if(fd == CT_MAX_FD){
		ct_rt.errorn = ENFILE;
		dax_stop_access(ct_rt.mpk[DAX_MPK_DEFAULT]);
		ctfs_lock_release(ct_rt.open_lock);
		return -1;
	}
	res = inode_path2inode(&frame);
	if(res){
		ct_rt.errorn = res;
		dax_stop_access(ct_rt.mpk[DAX_MPK_DEFAULT]);
		ctfs_lock_release(ct_rt.open_lock);
		return -1;
	}
	c = frame.current;
	c->i_otim = time(NULL);
	inode_rt_unlock(frame.current->i_number);

	ct_rt.fd[fd].inode = frame.current;
	ct_rt.fd[fd].offset = 0;
	ct_rt.fd[fd].flags = flags;
	ct_rt.fd[fd].prefaulted_start = 0;
	ct_rt.fd[fd].prefaulted_bytes = 0;
#ifdef CTFS_DEBUG
	ct_rt.fd[fd].cpy_time = 0;
	ct_rt.fd[fd].pswap_time = 0;
#endif
	dax_stop_access(ct_rt.mpk[DAX_MPK_DEFAULT]);
	ctfs_lock_release(ct_rt.open_lock);
	return 0;
}

int ctfs_close(int fd){
	if(fd >= CT_MAX_FD || ct_rt.fd[fd].inode == NULL){
		ct_rt.errorn = EBADF;
		return -1;
	}
	ct_rt.fd[fd].inode = 0;
#ifdef CTFS_DEBUG
	printf("closed fd: %d\n", fd);
#endif
	return 0;
}

ssize_t  ctfs_pread(int fd, void *buf, size_t count, off_t offset){
	if(fd >= CT_MAX_FD || ct_rt.fd[fd].inode == NULL){
		ct_rt.errorn = EBADF;
		return -1;
	}
	if(ct_rt.fd[fd].flags & O_WRONLY){
		ct_rt.errorn = EBADF;
		return -1;
	}
	dax_grant_access(ct_rt.mpk[DAX_MPK_DEFAULT]);
	ino_t inode_n = ct_rt.fd[fd].inode->i_number;
#ifdef CTFS_DEBUG
	ct_inode_t ino = *ct_rt.fd[fd].inode;
#endif
	inode_rw_lock(inode_n);
	if(offset >= ct_rt.fd[fd].inode->i_size){
		inode_rw_unlock(inode_n);
		return 0;
	}
	else if(offset + count >= ct_rt.fd[fd].inode->i_size){
		count = ct_rt.fd[fd].inode->i_size - offset;
	}
	
	void* target = CT_REL2ABS(ct_rt.fd[fd].inode->i_block);
#ifdef CTFS_DEBUG
	timer_start();
#endif
	if(count > PMD_SIZE){
		big_memcpy(buf, target + offset, count);
	}
	else{
		memcpy(buf, target + offset, count);
	}
#ifdef CTFS_DEBUG
	ct_rt.fd[fd].cpy_time += timer_end();
#endif
	inode_rw_unlock(inode_n);
	dax_stop_access(ct_rt.mpk[DAX_MPK_DEFAULT]);
	return count;
}


static inline ssize_t  ctfs_pwrite_normal(int fd, const void *buf, size_t count, off_t offset){
	if(unlikely(fd >= CT_MAX_FD || ct_rt.fd[fd].inode == NULL)){
		ct_rt.errorn = EBADF;
		return 0;
	}
	if(unlikely((ct_rt.fd[fd].flags & (O_WRONLY | O_RDWR)) == 0)){
		ct_rt.errorn = EBADF;
		return 0;
	}
	dax_grant_access(ct_rt.mpk[DAX_MPK_DEFAULT]);
	ino_t inode_n = ct_rt.fd[fd].inode->i_number;
	uint64_t end;
#ifdef CTFS_DEBUG
	ct_inode_t ino = *ct_rt.fd[fd].inode;
#endif
	inode_rw_lock(inode_n);
	end = offset + count;
	if(unlikely(end > ct_rt.fd[fd].inode->i_size)){
#if CTFS_DEBUG > 2
		printf("RESIZE! %lu -> %lu", ct_rt.fd[fd].inode->i_size, end);
		timer_start();
#endif
		if(inode_resize(ct_rt.fd[fd].inode, offset + count)){
			ct_rt.fd[fd].prefaulted_bytes = 0;
		}
#if CTFS_DEBUG > 2
		uint64_t  t = timer_end();
		printf("append took: %lu ns\n", t);
#endif
	}
	void * addr_base = CT_REL2ABS(ct_rt.fd[fd].inode->i_block);
#ifdef CTFS_DEBUG
	ino = *ct_rt.fd[fd].inode;
#endif
#ifdef CTFS_DEBUG
	timer_start();
#endif
	avx_cpy(addr_base + offset, buf, count);
#ifdef CTFS_DEBUG
	ct_rt.fd[fd].cpy_time += timer_end();
#endif
	inode_rw_unlock(inode_n);
	dax_stop_access(ct_rt.mpk[DAX_MPK_DEFAULT]);
	return count;
}

static inline void ctfs_pwrite_atomic_cpy(ct_inode_pt inode ,void * base, void * staging, const void *buf, size_t count, off_t offset){
	avx_cpy(staging + offset, base + offset, count);
	inode->i_finish_swap = 2;
	avx_cpy(base + offset, buf, count);
	return;
}

static inline ssize_t  ctfs_pwrite_atomic(int fd, const void *buf, size_t count, off_t offset){
	if(unlikely(fd >= CT_MAX_FD || ct_rt.fd[fd].inode == NULL)){
		ct_rt.errorn = EBADF;
		return -1;
	}
	if(unlikely((ct_rt.fd[fd].flags & (O_WRONLY | O_RDWR)) == 0)){
		ct_rt.errorn = EBADF;
		return -1;
	}
	// if((ct_rt.fd[fd].flags & CTFS_O_ATOMIC) == 0){
	// 	return ctfs_pwrite_normal(fd, buf, count, offset);
	// }
	
	dax_grant_access(ct_rt.mpk[DAX_MPK_DEFAULT]);
	ino_t inode_n = ct_rt.fd[fd].inode->i_number;
	inode_rw_lock(inode_n);
	size_t ori_size = ct_rt.fd[fd].inode->i_size;
	if(ori_size <= offset){
		// pure append
		inode_rw_unlock(inode_n);
		return ctfs_pwrite_normal(fd, buf, count, offset);
	}
	// printf("pwrite atomic count: %lu, offset: %lu, original size: %lu\n", count, offset, ori_size);
	uint64_t end = offset + count;
	// if(ct_rt.fd[fd].inode->i_level == PGG_LVL_NONE || offset >= ct_rt.fd[fd].inode->i_size){
	// 	// normal can handle it atomically
	// 	inode_rw_unlock(inode_n);
	// 	return ctfs_pwrite_normal(fd, buf, count, offset);
	// }
	if(unlikely(end > ori_size)){
#if CTFS_DEBUG > 4
		printf("RESIZE! %lu -> %lu", ct_rt.fd[fd].inode->i_size, end);
		timer_start();
#endif
		if(inode_resize(ct_rt.fd[fd].inode, offset + count)){
			ct_rt.fd[fd].prefaulted_bytes = 0;
		}
#if CTFS_DEBUG > 4
		uint64_t  t = timer_end();
		printf("append took: %lu ns\n", t);
#endif
	}
	void* base = CT_REL2ABS(ct_rt.fd[fd].inode->i_block);
#ifdef CTFS_DEBUG
	ct_inode_t ino = *ct_rt.fd[fd].inode;
#endif
	// prepare a staging space
	relptr_t new = pgg_allocate(ct_rt.fd[fd].inode->i_level);
	if(unlikely(new == 0)){
		printf("OUT of SPACE");
		inode_rw_unlock(inode_n);
		dax_stop_access(ct_rt.mpk[DAX_MPK_DEFAULT]);
		return 0;
	}
	void * staging = CT_REL2ABS(new);
	
#ifdef CTFS_ATOMIC_WRITE_USE_UNDO
	// if the writing is small, use undo log
	if(count <= 4096 * 4){
		ctfs_pwrite_atomic_cpy(ct_rt.fd[fd].inode, base, staging, buf, count, offset);
		goto out;
	}
#endif
	// first the starting residue
	uint64_t cur = offset;
	uint64_t swap_start = cur;
	uint64_t swap_num = 0;
	uint64_t rem = count;
	uint64_t residue = cur & (CT_PAGE_SIZE - 1);

	//lock
	// bitlock_acquire(&ct_rt.pgg_lock, 32);

#ifdef CTFS_DEBUG
	timer_start();
#endif
	if(residue != 0){
		cur = cur & PAGE_MASK;	
		swap_start = cur;
		avx_cpy(staging + cur, base + cur, residue);
		if(rem < CT_PAGE_SIZE - residue){
			char before[2][64];
			avx_cpy(staging + cur + residue, buf, rem); // this is the data
			if(unlikely(ori_size > cur + residue + rem)){
				avx_cpy(staging + cur + residue + rem, base + cur + residue + rem, CT_PAGE_SIZE - residue - rem);
			}
			swap_num = 1;
			goto end_cpy;
		}
		avx_cpy(staging + cur + residue, buf, CT_PAGE_SIZE - residue);
		rem -= CT_PAGE_SIZE - residue;
		swap_num ++;
		cur += CT_PAGE_SIZE;
	}
#ifdef CTFS_DEBUG
	ct_inode_t ino0 = *ct_rt.fd[fd].inode;
#endif
	// middle pages
	uint64_t num = rem & PAGE_MASK;
	avx_cpy(staging + cur, buf + (count - rem), num);
#ifdef CTFS_DEBUG
	ct_inode_t ino1 = *ct_rt.fd[fd].inode;
	ct_super_blk_t sp = *ct_rt.super_blk;
#endif
	rem -= num;
	cur += num;
	swap_num += num >> PAGE_SHIFT;

	// ending residue
	if(rem != 0){
		avx_cpy(staging + cur, buf + (count - rem), rem);
		residue = cur + CT_PAGE_SIZE;
		if(likely(residue > ct_rt.fd[fd].inode->i_size)){
			residue = ct_rt.fd[fd].inode->i_size;
		}
		assert(residue - cur - rem >= 0);
		avx_cpy(staging + cur + rem, base + cur + rem, residue - cur - rem);
		swap_num ++;
	}
	dax_ioctl_pswap_t frame; 
end_cpy:	
	frame = (dax_ioctl_pswap_t){
		.ufirst = base + swap_start, 
		.usecond = staging + swap_start,
		.npgs = swap_num, 
		.flag = (uint64_t)&ct_rt.fd[fd].inode->i_finish_swap
	};
#ifdef CTFS_DEBUG
	ct_rt.fd[fd].cpy_time += timer_end();
	timer_start();
	char before[2][64];
	memcpy(before[0], (void*)frame.ufirst, 64);
	memcpy(before[1], (void*)frame.usecond, 64);
#endif
	dax_pswap(&frame);
#ifdef CTFS_DEBUG
	ct_rt.fd[fd].pswap_time += timer_end();
	ct_inode_t ino2 = *ct_rt.fd[fd].inode;
	char after[2][64];
	memcpy(after[0], (void*)frame.ufirst, 64);
	memcpy(after[1], (void*)frame.usecond, 64);

	if(offset != 0){
		assert(offset - swap_start >= 0);
		if(offset - swap_start){
			assert(memcmp(base + swap_start, staging + swap_start, offset - swap_start) == 0);
		}
		assert(memcmp(base + offset, buf, count)==0);
	}
#endif
out:
	pgg_deallocate(ct_rt.fd[fd].inode->i_level, new);
	// bitlock_release(&ct_rt.pgg_lock, 32);
	ct_rt.fd[fd].inode->i_finish_swap = 0;
	inode_rw_unlock(inode_n);
	dax_stop_access(ct_rt.mpk[DAX_MPK_DEFAULT]);
	return count;
}

ssize_t ctfs_pwrite(int fd, const void *buf, size_t count, off_t offset){
#ifdef CTFS_ATOMIC_WRITE
	return ctfs_pwrite_atomic(fd, buf, count, offset);
#else
	return ctfs_pwrite_normal(fd, buf, count, offset);
#endif
}

ssize_t  ctfs_write(int fd, const void *buf, size_t count){
	ssize_t ret = ctfs_pwrite(fd, buf, count, ct_rt.fd[fd].offset);
	if(ret >0){
		ct_rt.fd[fd].offset += ret;
	}
	return ret;
}

ssize_t  ctfs_read(int fd, void *buf, size_t count){
	ssize_t ret = ctfs_pread(fd, buf, count, ct_rt.fd[fd].offset);
	if(ret >0){
		ct_rt.fd[fd].offset += ret;
	}
	return ret;
}

int ctfs_mkdir(const char *pathname, uint16_t mode){
	mode |= S_IFDIR;
	dax_grant_access(ct_rt.mpk[DAX_MPK_DEFAULT]);
	ct_inode_frame_t frame = {.path = pathname, 
		.inode_start = ct_rt.current_dir, 
		.i_mode = mode,
		.flag = CT_INODE_FRAME_CREATE};
	int ret = inode_path2inode(&frame);
	if(!ret){
		inode_rt_unlock(frame.current->i_number);
	}
	dax_stop_access(ct_rt.mpk[DAX_MPK_DEFAULT]);
	return ret;
}

void print_debug(int fd){
#ifdef CTFS_DEBUG
	printf("cpy took %lu ns, pswap took %lu ns\n", 
	ct_rt.fd[fd].cpy_time, 
	ct_rt.fd[fd].pswap_time);
	ct_rt.fd[fd].cpy_time = 0;
	ct_rt.fd[fd].pswap_time = 0;
#endif
	(void)fd;
}

int ctfs_fallocate(int fd, int mode, off_t offset, off_t len){
	printf("called fallocate: %d, off: %lu, size: %lu\n", fd, offset, len);
	if(fd >= CT_MAX_FD || ct_rt.fd[fd].inode == NULL){
		ct_rt.errorn = EBADF;
		return -1;
	}
	if((ct_rt.fd[fd].flags & (O_WRONLY | O_RDWR)) == 0){
		ct_rt.errorn = EBADF;
		return -1;
	}
	if(mode != 0){
		if(mode & ~FALLOC_FL_KEEP_SIZE){
			ct_rt.errorn = EOPNOTSUPP;
			return -1;
		}
	}
	dax_grant_access(ct_rt.mpk[DAX_MPK_DEFAULT]);
	ino_t inode_n = ct_rt.fd[fd].inode->i_number;
	inode_rw_lock(inode_n);
	uint64_t end = offset + len;
	if(end > ct_rt.fd[fd].inode->i_size){
		size_t old = ct_rt.fd[fd].inode->i_size;
		inode_resize(ct_rt.fd[fd].inode, end);
		if(mode & FALLOC_FL_KEEP_SIZE){
			ct_rt.fd[fd].inode->i_size = old;
		}
	}
	inode_wb(ct_rt.fd[fd].inode);
	inode_rw_unlock(inode_n);
	dax_stop_access(ct_rt.mpk[DAX_MPK_DEFAULT]);
	return 0;
}

int ctfs_truncate(const char *path, off_t length){
	printf("called truncate: %s, %lu\n", path, length);
	int res;
	ct_inode_frame_t frame = {.flag = 0, .path = path};
	dax_grant_access(ct_rt.mpk[DAX_MPK_DEFAULT]);
	if(*path != '/'){
		// start from the current dir
		frame.inode_start = ct_rt.current_dir;
	}
	res = inode_path2inode(&frame);
	if(res){
		ct_rt.errorn = res;
		return -1;
	}
	if(length > frame.current->i_size){
		inode_resize(frame.current, length);
	}
	else{
		frame.current->i_size = length;
	}
	inode_wb(frame.current);
	inode_rw_unlock(frame.current->i_number);
	dax_stop_access(ct_rt.mpk[DAX_MPK_DEFAULT]);
	return 0;
}


int ctfs_ftruncate(int fd, off_t len){
	printf("called ftruncate: %d, %lu\n", fd, len);
	if(fd >= CT_MAX_FD || ct_rt.fd[fd].inode == NULL){
		ct_rt.errorn = EBADF;
		return -1;
	}
	if((ct_rt.fd[fd].flags & (O_WRONLY | O_RDWR)) == 0){
		ct_rt.errorn = EBADF;
		return -1;
	}
	dax_grant_access(ct_rt.mpk[DAX_MPK_DEFAULT]);
	ino_t inode_n = ct_rt.fd[fd].inode->i_number;
	inode_rw_lock(inode_n);
	if(len > ct_rt.fd[fd].inode->i_size){
		inode_resize(ct_rt.fd[fd].inode, len);
	}
	else{
		ct_rt.fd[fd].inode->i_size = len;
	}
	inode_wb(ct_rt.fd[fd].inode);
	inode_rw_unlock(inode_n);
	dax_stop_access(ct_rt.mpk[DAX_MPK_DEFAULT]);
	return 0;
}

int ctfs_fstatfs(int fd, struct statfs *buf){
	if(buf == NULL){
		ct_rt.errorn = EINVAL;
		return -1;
	}
	if(fd >= CT_MAX_FD || ct_rt.fd[fd].inode == NULL){
		ct_rt.errorn = EBADF;
		return -1;
	}
	buf->f_type = EXT2_SUPER_MAGIC;
	buf->f_bsize = CT_PAGE_SIZE;
	buf->f_blocks = CT_DAX_ALLOC_SIZE / CT_PAGE_SIZE;
	buf->f_bfree = CT_DAX_ALLOC_SIZE / CT_PAGE_SIZE / 2;
	buf->f_bavail = CT_DAX_ALLOC_SIZE / CT_PAGE_SIZE / 2;
	buf->f_files = CT_SIZE_MAX_INODE;
	buf->f_ffree = CT_SIZE_MAX_INODE / 2;
	buf->f_fsid.__val[0] = 12345;
	buf->f_fsid.__val[0] = 54321;
	buf->f_namelen = CT_MAX_NAME;
	buf->f_frsize = CT_PAGE_SIZE;
	buf->f_flags = 0;
	return 0;
}

int ctfs_rename(const char *oldpath, const char *newpath){
	int res;
	ct_inode_frame_t frame = {.flag = CT_INODE_FRAME_PARENT, .path = oldpath};
	dax_grant_access(ct_rt.mpk[DAX_MPK_DEFAULT]);
	
	// unlink old
	if(*oldpath != '/'){
		// start from the current dir
		frame.inode_start = ct_rt.current_dir;
	}
	res = inode_path2inode(&frame);
	if(res){
		ct_rt.errorn = res;
		dax_stop_access(ct_rt.mpk[DAX_MPK_DEFAULT]);
		return -1;
	}
	ct_inode_pt target = frame.current;
	// ino_t old_num = frame.parent->i_number;
	// ct_dirent_pt old_dir = frame.dirent;
	frame.dirent->d_ino = 0;
	frame.parent->i_ndirent --;
	ct_time_stamp(&frame.parent->i_mtim);
	inode_rt_unlock(frame.parent->i_number);
	if((frame.flag & CT_INODE_FRAME_SAME_INODE_LOCK) == 0){
		inode_rt_unlock(frame.current->i_number);
	}
	
	// link new
	frame = (ct_inode_frame_t){
		.flag = CT_INODE_FRAME_INSTALL | CT_INODE_FRAME_CREATE, 
		.path = newpath, 
		.current = target};
	if(*newpath != '/'){
		// start from the current dir
		frame.inode_start = ct_rt.current_dir;
	}
	res = inode_path2inode(&frame);
	if(res){
		ct_rt.errorn = res;
		dax_stop_access(ct_rt.mpk[DAX_MPK_DEFAULT]);
		return -1;
	}

	return 0;
}

DIR* ctfs_opendir(const char *_pathname){
	int ret = ctfs_open(_pathname, O_RDONLY);
	if(ret == -1){
		return NULL;
	}
	if(ret == 0){
		ret = ctfs_open(_pathname, O_RDONLY);
		ctfs_close(0);
	}
	dax_grant_access(ct_rt.mpk[DAX_MPK_DEFAULT]);
	if((ct_rt.fd[ret].inode->i_mode & S_IFDIR) == 0){
		dax_stop_access(ct_rt.mpk[DAX_MPK_DEFAULT]);
		ctfs_close(ret);
		ct_rt.errorn = ENOTDIR;
		return NULL;
	}
	return (DIR*)(uint64_t)ret;
}

struct dirent * ctfs_readdir(DIR *dirp){
	int fd = (int)(uint64_t)dirp;
	if(fd >= CT_MAX_FD || ct_rt.fd[fd].inode == NULL){
		ct_rt.errorn = EBADF;
		return NULL;
	}
	if(ct_rt.fd[fd].flags & O_WRONLY){
		ct_rt.errorn = EBADF;
		return NULL;
	}
	dax_grant_access(ct_rt.mpk[DAX_MPK_DEFAULT]);
	if((ct_rt.fd[fd].inode->i_mode & S_IFDIR) == 0){
		dax_stop_access(ct_rt.mpk[DAX_MPK_DEFAULT]);
		ct_rt.errorn = ENOTDIR;
		return NULL;
	}
	ino_t inode_n = ct_rt.fd[fd].inode->i_number;
	inode_rw_lock(inode_n);
	size_t dirent_size = ct_rt.fd[fd].inode->i_size / sizeof(ct_dirent_t);
	ct_dirent_pt target = CT_REL2ABS(ct_rt.fd[fd].inode->i_block);
	while(1){
		if(ct_rt.fd[fd].offset >= dirent_size){
			inode_rw_unlock(inode_n);
			dax_stop_access(ct_rt.mpk[DAX_MPK_DEFAULT]);
			return NULL;
		}
#ifdef CTFS_DEBUG
			ct_dirent_t dir_temp = target[ct_rt.fd[fd].offset];
			ct_inode_t ino_temp = *ct_rt.fd[fd].inode;
#endif
		if(target[ct_rt.fd[fd].offset].d_ino != 0){
			break;
		} 
		ct_rt.fd[fd].offset++;
	}
	ct_rt.fd[fd].temp_dirent.d_ino = target[ct_rt.fd[fd].offset].d_ino;
	ct_rt.fd[fd].temp_dirent.d_off = target[ct_rt.fd[fd].offset].d_off;
	ct_rt.fd[fd].temp_dirent.d_reclen = target[ct_rt.fd[fd].offset].d_reclen;
	ct_rt.fd[fd].temp_dirent.d_type = target[ct_rt.fd[fd].offset].d_type;
	strcpy(ct_rt.fd[fd].temp_dirent.d_name, target[ct_rt.fd[fd].offset].d_name);
	ct_rt.fd[fd].offset++;
	inode_rw_unlock(inode_n);
	dax_stop_access(ct_rt.mpk[DAX_MPK_DEFAULT]);
	return &ct_rt.fd[fd].temp_dirent;
}

int ctfs_closedir(DIR *dirp){
	return ctfs_close((int)(uint64_t)dirp);
}

int  ctfs_lseek(int fd, int offset, int whence){
	if(fd >= CT_MAX_FD || ct_rt.fd[fd].inode == NULL){
		ct_rt.errorn = EBADF;
		return -1;
	}
	switch (whence)
	{
	case SEEK_SET:
		ct_rt.fd[fd].offset = offset;
		break;

	case SEEK_CUR:
		ct_rt.fd[fd].offset += offset;
		if(ct_rt.fd[fd].offset < 0){
			ct_rt.fd[fd].offset = 0;
		}
		break;

	case SEEK_END:
		dax_grant_access(ct_rt.mpk[DAX_MPK_DEFAULT]);
		ct_rt.fd[fd].offset = ct_rt.fd[fd].inode->i_size + offset;
		dax_stop_access(ct_rt.mpk[DAX_MPK_DEFAULT]);
		break;
	
	default:
		ct_rt.errorn = EINVAL;
		return -1;
		break;
	}

	return 0;
}

int  ctfs_access(const char * pathname, int mode){
	int res;
	ct_inode_frame_t frame = {.flag = 0, .path = pathname};
	dax_grant_access(ct_rt.mpk[DAX_MPK_DEFAULT]);
	if(*pathname != '/'){
		// start from the current dir
		frame.inode_start = ct_rt.current_dir;
	}
	res = inode_path2inode(&frame);
	if(res){
		ct_rt.errorn = res;
		dax_stop_access(ct_rt.mpk[DAX_MPK_DEFAULT]);
		return -1;
	}
	inode_rt_unlock(frame.current->i_number);
	return 0;
}

int ctfs_fcntl(int fd, int cmd, ...){
	va_list ap;
	if(fd >= CT_MAX_FD || ct_rt.fd[fd].inode == NULL){
		ct_rt.errorn = EBADF;
		return -1;
	}
	switch (cmd)
	{
	case F_GETFL:
		// printf("the flags are: %x\n", ct_rt.fd[fd].flags);
		return ct_rt.fd[fd].flags;
		break;
	case F_SETFL:
		
		va_start(ap, cmd);
		ct_rt.fd[fd].flags = va_arg(ap, int);
		return 0;
	default:
		return 0;
		break;
	}
	return 0;
}

int* ctfs_errno(){
	return &ct_rt.errorn;
}
