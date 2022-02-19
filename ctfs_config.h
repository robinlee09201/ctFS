#ifndef CTFS_CONFIG_H
#define CTFS_CONFIG_H

// #define CTFS_ATOMIC_WRITE
// #define CTFS_ATOMIC_WRITE_USE_UNDO

#define CT_PAGE_SIZE        		((uint64_t)0x01 << 12)
#define CT_PGGSIZE_LV0      		(4 * (uint64_t)1024)
#define CT_PGGSIZE_LV1      		(8 * CT_PGGSIZE_LV0)
#define CT_PGGSIZE_LV2      		(8 * CT_PGGSIZE_LV1)
#define CT_PGGSIZE_LV3      		(8 * CT_PGGSIZE_LV2)
#define CT_PGGSIZE_LV4      		(8 * CT_PGGSIZE_LV3)
#define CT_PGGSIZE_LV5      		(8 * CT_PGGSIZE_LV4)
#define CT_PGGSIZE_LV6      		(8 * CT_PGGSIZE_LV5)
#define CT_PGGSIZE_LV7      		(8 * CT_PGGSIZE_LV6)
#define CT_PGGSIZE_LV8      		(8 * CT_PGGSIZE_LV7)
#define CT_PGGSIZE_LV9      		(8 * CT_PGGSIZE_LV8)

#define CT_LIMITSIZE_LV0    		(4 * (uint64_t)1024)
#define CT_LIMITSIZE_LV1    		(8 * CT_LIMITSIZE_LV0)
#define CT_LIMITSIZE_LV2    		(8 * CT_LIMITSIZE_LV1)
#define CT_LIMITSIZE_LV3    		(8 * CT_LIMITSIZE_LV2)
#define CT_LIMITSIZE_LV4    		(8 * CT_LIMITSIZE_LV3)
#define CT_LIMITSIZE_LV5    		(8 * CT_LIMITSIZE_LV4)
#define CT_LIMITSIZE_LV6    		(8 * CT_LIMITSIZE_LV5)
#define CT_LIMITSIZE_LV7    		(8 * CT_LIMITSIZE_LV6)
#define CT_LIMITSIZE_LV8            (8 * CT_LIMITSIZE_LV7)
#define CT_LIMITSIZE_LV9            (8 * CT_LIMITSIZE_LV8)

#define CT_ALLOC_PROT_SIZE          64

#define CT_DAX_ALLOC_SIZE  			((uint64_t)0x01 << 41)  // 2TB

#define CT_OFFSET_IBMP              ((uint64_t) 512 << 10)  // 512KB
#define CT_OFFSET_ITABLE            ((uint64_t) 512 << 20)  // 512MB
#define CT_OFFSET_PROT              ((uint64_t) 1 << 10)    // 1KB
#define CT_OFFSET_L9_BMP            ((uint64_t) 2 << 10)    // 2KB
#define CT_OFFSET_1_PGG  			((uint64_t) 0x01 << 39) // 512GB

#define CT_SIZE_MAX_INODE           ((CT_OFFSET_ITABLE - CT_OFFSET_IBMP) << 3)
#define CT_INODE_BITLOCK_SLOTS      65536
#define CT_INODE_RW_SLOTS			65536
#define CT_MAX_NAME					231
#define CT_MAGIC            		"ctfs_v1"
#define CT_MAX_FD					4096

#define CT_FAILSAFE_NFRAMES			32

#define PAGE_SHIFT					12
#define PMD_SHIFT					21
#define PTRS_PER_PMD				512
#define PTRS_PER_PTE				512
#define PMD_SIZE					(((uint64_t) 0x01) << PMD_SHIFT)
#define PMD_MASK					(~(PMD_SIZE - 1))

// errors

#define	EPERM		 1	/* Operation not permitted */
#define	ENOENT		 2	/* No such file or directory */
#define	ESRCH		 3	/* No such process */
#define	EINTR		 4	/* Interrupted system call */
#define	EIO		    5	/* I/O error */
#define	ENXIO		 6	/* No such device or address */
#define	E2BIG		 7	/* Argument list too long */
#define	ENOEXEC		 8	/* Exec format error */
#define	EBADF		 9	/* Bad file number */
#define	ECHILD		10	/* No child processes */
#define	EAGAIN		11	/* Try again */
#define	ENOMEM		12	/* Out of memory */
#define	EACCES		13	/* Permission denied */
#define	EFAULT		14	/* Bad address */
#define	ENOTBLK		15	/* Block device required */
#define	EBUSY		16	/* Device or resource busy */
#define	EEXIST		17	/* File exists */
#define	EXDEV		18	/* Cross-device link */
#define	ENODEV		19	/* No such device */
#define	ENOTDIR		20	/* Not a directory */
#define	EISDIR		21	/* Is a directory */
#define	EINVAL		22	/* Invalid argument */
#define	ENFILE		23	/* File table overflow */
#define	EMFILE		24	/* Too many open files */
#define	ENOTTY		25	/* Not a typewriter */
#define	ETXTBSY		26	/* Text file busy */
#define	EFBIG		27	/* File too large */
#define	ENOSPC		28	/* No space left on device */
#define	ESPIPE		29	/* Illegal seek */
#define	EROFS		30	/* Read-only file system */
#define	EMLINK		31	/* Too many links */
#define	EPIPE		32	/* Broken pipe */
#define	EDOM		33	/* Math argument out of domain of func */
#define	ERANGE		34	/* Math result not representable */
#define CTFS_HACK

// #define DAX_DEBUGGING 1

#endif