#define _GNU_SOURCE
#include <dlfcn.h>
#include "ctfs.h"
#include <boost/preprocessor/seq/for_each.hpp>
#include "ctfs_config.h"
#include "glibc/ffile.h"
// #define WRAPPER_DEBUG

#ifdef WRAPPER_DEBUG
#define PRINT_FUNC	printf("\tcalled ctFS func: %s\n", __func__)
#else
#define PRINT_FUNC	;
#endif

#define CT_FD_OFFSET 1024

# define EMPTY(...)
# define DEFER(...) __VA_ARGS__ EMPTY()
# define OBSTRUCT(...) __VA_ARGS__ DEFER(EMPTY)()
# define EXPAND(...) __VA_ARGS_

#define MK_STR(arg) #arg
#define MK_STR2(x) MK_STR(x)
#define MK_STR3(x) MK_STR2(x)
// Information about the functions which are wrapped by EVERY module
// Alias: the standard function which most users will call
#define ALIAS_OPEN   open
#define ALIAS_CREAT  creat
#define ALIAS_EXECVE execve
#define ALIAS_EXECVP execvp
#define ALIAS_EXECV execv
#define ALIAS_MKNOD __xmknod
#define ALIAS_MKNODAT __xmknodat

#define ALIAS_FOPEN  	fopen
#define ALIAS_FOPEN64  	fopen64
#define ALIAS_FREAD  	fread
#define ALIAS_FEOF 	 	feof
#define ALIAS_FERROR 	ferror
#define ALIAS_CLEARERR 	clearerr
#define ALIAS_FWRITE 	fwrite
#define ALIAS_FSEEK  	fseek
#define ALIAS_FTELL  	ftell
#define ALIAS_FTELLO 	ftello
#define ALIAS_FCLOSE 	fclose
#define ALIAS_FPUTS		fputs
#define ALIAS_FGETS		fgets
#define ALIAS_FFLUSH	fflush

#define ALIAS_FSTATFS	fstatfs
#define ALIAS_FDATASYNC	fdatasync
#define ALIAS_FCNTL		fcntl
#define ALIAS_FCNTL2	__fcntl64_nocancel_adjusted
#define ALIAS_OPENDIR		opendir
#define ALIAS_READDIR	readdir
#define ALIAS_READDIR64	readdir64
#define ALIAS_CLOSEDIR	closedir
#define ALIAS_ERROR		__errno_location
#define ALIAS_SYNC_FILE_RANGE	sync_file_range

#define ALIAS_ACCESS access
#define ALIAS_READ   read
#define ALIAS_READ2		__libc_read
#define ALIAS_WRITE  write
#define ALIAS_SEEK   lseek
#define ALIAS_CLOSE  close
#define ALIAS_FTRUNC ftruncate
#define ALIAS_TRUNC  truncate
#define ALIAS_DUP    dup
#define ALIAS_DUP2   dup2
#define ALIAS_FORK   fork
#define ALIAS_VFORK  vfork
#define ALIAS_MMAP   mmap
#define ALIAS_READV  readv
#define ALIAS_WRITEV writev
#define ALIAS_PIPE   pipe
#define ALIAS_SOCKETPAIR   socketpair
#define ALIAS_IOCTL  ioctl
#define ALIAS_MUNMAP munmap
#define ALIAS_MSYNC  msync
#define ALIAS_CLONE  __clone
#define ALIAS_PREAD  pread
#define ALIAS_PREAD64  pread64
#define ALIAS_PWRITE64 pwrite64
#define ALIAS_PWRITE pwrite
//#define ALIAS_PWRITESYNC pwrite64_sync
#define ALIAS_FSYNC  fsync
#define ALIAS_FDSYNC fdatasync
#define ALIAS_FTRUNC64 ftruncate64
#define ALIAS_OPEN64  open64
#define ALIAS_LIBC_OPEN64 __libc_open64
#define ALIAS_SEEK64  lseek64
#define ALIAS_MMAP64  mmap64
#define ALIAS_MKSTEMP mkstemp
#define ALIAS_MKSTEMP64 mkstemp64
#define ALIAS_ACCEPT  accept
#define ALIAS_SOCKET  socket
#define ALIAS_UNLINK  unlink
#define ALIAS_POSIX_FALLOCATE posix_fallocate
#define ALIAS_POSIX_FALLOCATE64 posix_fallocate64
#define ALIAS_FALLOCATE fallocate
#define ALIAS_STAT stat
#define ALIAS_STAT64 stat64
#define ALIAS_FSTAT fstat
#define ALIAS_FSTAT64 fstat64
#define ALIAS_LSTAT lstat
#define ALIAS_LSTAT64 lstat64
/* Now all the metadata operations */
#define ALIAS_MKDIR mkdir
#define ALIAS_RENAME rename
#define ALIAS_LINK link
#define ALIAS_SYMLINK symlink
#define ALIAS_RMDIR rmdir
/* All the *at operations */
#define ALIAS_OPENAT openat
#define ALIAS_SYMLINKAT symlinkat
#define ALIAS_MKDIRAT mkdirat
#define ALIAS_UNLINKAT  unlinkat


// The function return type
#define RETT_OPEN   int
#define RETT_LIBC_OPEN64 int
#define RETT_CREAT  int
#define RETT_EXECVE int
#define RETT_EXECVP int
#define RETT_EXECV int
#define RETT_SHM_COPY void
#define RETT_MKNOD int
#define RETT_MKNODAT int

// #ifdef TRACE_FP_CALLS
#define RETT_FOPEN  FILE*
#define RETT_FOPEN64  FILE*
#define RETT_FWRITE size_t
#define RETT_FSEEK  int
#define RETT_FTELL  long int
#define RETT_FTELLO off_t
#define RETT_FCLOSE int
#define RETT_FPUTS	int
#define RETT_FGETS	char*
#define RETT_FFLUSH	int
// #endif

#define RETT_FSTATFS	int
#define RETT_FDATASYNC	int
#define RETT_FCNTL		int
#define RETT_FCNTL2		int
#define RETT_OPENDIR	DIR *
#define RETT_READDIR	struct dirent *
#define RETT_READDIR64	struct dirent64 *
#define RETT_CLOSEDIR	int
#define RETT_ERROR		int *
#define RETT_SYNC_FILE_RANGE int

#define RETT_ACCESS int
#define RETT_READ   ssize_t
#define RETT_READ2   ssize_t
#define RETT_FREAD  size_t
#define RETT_FEOF   int
#define RETT_FERROR int
#define RETT_CLEARERR void
#define RETT_WRITE  ssize_t
#define RETT_SEEK   off_t
#define RETT_CLOSE  int
#define RETT_FTRUNC int
#define RETT_TRUNC  int
#define RETT_DUP    int
#define RETT_DUP2   int
#define RETT_FORK   pid_t
#define RETT_VFORK  pid_t
#define RETT_MMAP   void*
#define RETT_READV  ssize_t
#define RETT_WRITEV ssize_t
#define RETT_PIPE   int
#define RETT_SOCKETPAIR   int
#define RETT_IOCTL  int
#define RETT_MUNMAP int
#define RETT_MSYNC  int
#define RETT_CLONE  int
#define RETT_PREAD  ssize_t
#define RETT_PREAD64  ssize_t
#define RETT_PWRITE ssize_t
#define RETT_PWRITE64 ssize_t
//#define RETT_PWRITESYNC ssize_t
#define RETT_FSYNC  int
#define RETT_FDSYNC int
#define RETT_FTRUNC64 int
#define RETT_OPEN64  int
#define RETT_SEEK64  off64_t
#define RETT_MMAP64  void*
#define RETT_MKSTEMP int
#define RETT_MKSTEMP64 int
#define RETT_ACCEPT  int
#define RETT_SOCKET  int
#define RETT_UNLINK  int
#define RETT_POSIX_FALLOCATE int
#define RETT_POSIX_FALLOCATE64 int
#define RETT_FALLOCATE int
#define RETT_STAT int
#define RETT_STAT64 int
#define RETT_FSTAT int
#define RETT_FSTAT64 int
#define RETT_LSTAT int
#define RETT_LSTAT64 int
/* Now all the metadata operations */
#define RETT_MKDIR int
#define RETT_RENAME int
#define RETT_LINK int
#define RETT_SYMLINK int
#define RETT_RMDIR int
/* All the *at operations */
#define RETT_OPENAT int
#define RETT_SYMLINKAT int
#define RETT_MKDIRAT int
#define RETT_UNLINKAT int


// The function interface
#define INTF_OPEN const char *path, int oflag, ...
#define INTF_LIBC_OPEN64 const char *path, int oflag, ...

#define INTF_CREAT const char *path, mode_t mode
#define INTF_EXECVE const char *filename, char *const argv[], char *const envp[]
#define INTF_EXECVP const char *file, char *const argv[]
#define INTF_EXECV const char *path, char *const argv[]
#define INTF_SHM_COPY void
#define INTF_MKNOD int ver, const char* path, mode_t mode, dev_t* dev
#define INTF_MKNODAT int ver, int dirfd, const char* path, mode_t mode, dev_t* dev

// #ifdef TRACE_FP_CALLS
#define INTF_FOPEN  const char* __restrict path, const char* __restrict mode
#define INTF_FOPEN64  const char* __restrict path, const char* __restrict mode
#define INTF_FREAD  void* __restrict buf, size_t length, size_t nmemb, FILE* __restrict fp
#define INTF_CLEARERR FILE* fp
#define INTF_FEOF   FILE* fp
#define INTF_FERROR FILE* fp
#define INTF_FWRITE const void* __restrict buf, size_t length, size_t nmemb, FILE* __restrict fp
#define INTF_FSEEK  FILE* fp, long int offset, int whence
#define INTF_FTELL  FILE* fp
#define INTF_FTELLO FILE* fp
#define INTF_FCLOSE FILE* fp
#define INTF_FPUTS	const char *str, FILE *stream
#define INTF_FGETS	char *str, int n, FILE *stream
#define INTF_FFLUSH	FILE* fp
// #endif

#define INTF_FSTATFS	int fd, struct statfs *buf
#define INTF_FDATASYNC	int fd
#define INTF_FCNTL		int fd, int cmd, ...
#define INTF_FCNTL2		int fd, int cmd, void *arg
#define INTF_OPENDIR	const char *path
#define INTF_READDIR	DIR *dirp
#define INTF_READDIR64	DIR *dirp
#define INTF_CLOSEDIR	DIR *dirp
#define INTF_ERROR		void
#define INTF_SYNC_FILE_RANGE int fd, off64_t offset, off64_t nbytes, unsigned int flags


#define INTF_ACCESS const char *pathname, int mode
#define INTF_READ   int file, void* buf, size_t length
#define INTF_READ2   int file, void* buf, size_t length
#define INTF_WRITE  int file, const void* buf, size_t length
#define INTF_SEEK   int file, off_t offset, int whence
#define INTF_CLOSE  int file
#define INTF_FTRUNC int file, off_t length
#define INTF_TRUNC  const char* path, off_t length
#define INTF_DUP    int file
#define INTF_DUP2   int file, int fd2
#define INTF_FORK   void
#define INTF_VFORK  void
#define INTF_MMAP   void *addr, size_t len, int prot, int flags, int file, off_t off
#define INTF_READV  int file, const struct iovec *iov, int iovcnt
#define INTF_WRITEV int file, const struct iovec *iov, int iovcnt
#define INTF_PIPE   int file[2]
#define INTF_SOCKETPAIR   int domain, int type, int protocol, int sv[2]
#define INTF_IOCTL  int file, unsigned long int request, ...
#define INTF_MUNMAP void *addr, size_t len
#define INTF_MSYNC  void *addr, size_t len, int flags
#define INTF_CLONE  int (*fn)(void *a), void *child_stack, int flags, void *arg
#define INTF_PREAD  int file,       void *buf, size_t count, off_t offset
#define INTF_PREAD64  int file,       void *buf, size_t count, off_t offset
#define INTF_PWRITE int file, const void *buf, size_t count, off_t offset
#define INTF_PWRITE64 int file, const void *buf, size_t count, off_t offset
//#define INTF_PWRITESYNC int file, const void *buf, size_t count, off_t offset
#define INTF_FSYNC  int file
#define INTF_FDSYNC int file
#define INTF_FTRUNC64 int file, off64_t length
#define INTF_OPEN64  const char* path, int oflag, ...
#define INTF_SEEK64  int file, off64_t offset, int whence
#define INTF_MMAP64  void *addr, size_t len, int prot, int flags, int file, off64_t off
#define INTF_MKSTEMP char* file
#define INTF_MKSTEMP64 char* file
#define INTF_ACCEPT  int file, struct sockaddr *addr, socklen_t *addrlen
#define INTF_SOCKET  int domain, int type, int protocol
#define INTF_UNLINK  const char* path
#define INTF_POSIX_FALLOCATE int file, off_t offset, off_t len
#define INTF_POSIX_FALLOCATE64 int file, off_t offset, off_t len
#define INTF_FALLOCATE int file, int mode, off_t offset, off_t len
#define INTF_STAT const char *path, struct stat *buf
#define INTF_STAT64 const char *path, struct stat64 *buf
#define INTF_FSTAT int file, struct stat *buf
#define INTF_FSTAT64 int file, struct stat64 *buf
#define INTF_LSTAT const char *path, struct stat *buf
#define INTF_LSTAT64 const char *path, struct stat64 *buf
/* Now all the metadata operations */
#define INTF_MKDIR const char *path, uint32_t mode
#define INTF_RENAME const char *old, const char *new
#define INTF_LINK const char *path1, const char *path2
#define INTF_SYMLINK const char *path1, const char *path2
#define INTF_RMDIR const char *path
/* All the *at operations */
#define INTF_OPENAT int dirfd, const char* path, int oflag, ...
#define INTF_UNLINKAT  int dirfd, const char* path, int flags
#define INTF_SYMLINKAT const char* old_path, int newdirfd, const char* new_path
#define INTF_MKDIRAT int dirfd, const char* path, mode_t mode

#define CTFS_ALL_OPS	(OPEN) (LIBC_OPEN64) (OPENAT) (CREAT) (CLOSE) (ACCESS)\
						(SEEK) (TRUNC) (FTRUNC) (LINK) (UNLINK) (FSYNC) \
						(READ) (READ2) (WRITE) (PREAD) (PREAD64) (PWRITE) (PWRITE64) (STAT) (STAT64) (FSTAT) (FSTAT64) (LSTAT) (RENAME)\
						(MKDIR) (RMDIR) (FSTATFS) (FDATASYNC) (FCNTL) (FCNTL2) \
						(OPENDIR) (CLOSEDIR) (READDIR) (READDIR64) (ERROR) (SYNC_FILE_RANGE) \
						(FOPEN) (FPUTS) (FGETS) (FWRITE) (FREAD) (FCLOSE) (FSEEK) (FFLUSH)

#define PREFIX(call)				(real_##call)


#define TYPE_REL_SYSCALL(op) 		typedef RETT_##op (*real_##op##_t)(INTF_##op);
#define TYPE_REL_SYSCALL_WRAP(r, data, elem) 		TYPE_REL_SYSCALL(elem)

BOOST_PP_SEQ_FOR_EACH(TYPE_REL_SYSCALL_WRAP, placeholder, CTFS_ALL_OPS)

static struct real_ops{
	#define DEC_REL_SYSCALL(op) 	real_##op##_t op;
	#define DEC_REL_SYSCALL_WRAP(r, data, elem) 	DEC_REL_SYSCALL(elem)
	BOOST_PP_SEQ_FOR_EACH(DEC_REL_SYSCALL_WRAP, placeholder, CTFS_ALL_OPS)
} real_ops;

void insert_real_op(){
	#define FILL_REL_SYSCALL(op) 	real_ops.op = dlsym(RTLD_NEXT, MK_STR3(ALIAS_##op));
	#define FILL_REL_SYSCALL_WRAP(r, data, elem) 	FILL_REL_SYSCALL(elem)
	BOOST_PP_SEQ_FOR_EACH(FILL_REL_SYSCALL_WRAP, placeholder, CTFS_ALL_OPS)
}
#define OP_DEFINE(op)		RETT_##op ALIAS_##op(INTF_##op)

static int inited = 0;

OP_DEFINE(OPEN){
#ifdef WRAPPER_DEBUG
	printf("opened: %s\n", path);
#endif
	if(*path == '\\' || *path != '/' || path[1] == '\\'){
		int ret;
		if(oflag & O_CREAT){
			va_list ap;
			mode_t mode;
			va_start(ap, oflag);
			mode = va_arg(ap, mode_t);
			PRINT_FUNC;
#ifdef WRAPPER_DEBUG
			// printf("\t\tpath: %s\n", path);
#endif
			char * p = path;
			while(*p == '\\'){
				p ++;
			}
			if(p[1] == '\\'){
				p++;
				*p = '/';
			}
			ret = ctfs_open(p, oflag, mode);
			if(ret == -1 && *path != '\\'){
				return real_ops.OPEN(path, oflag, mode);
			}
		}
		else{
#ifdef WRAPPER_DEBUG
			// printf("\t\tpath: %s\n", path);
#endif
			PRINT_FUNC;
			ret = ctfs_open((*path == '\\')? path + 1 : path, oflag);
			if(ret == -1 && *path != '\\'){
				return real_ops.OPEN(path, oflag);
			}
		}
		if(ret == -1){
			return ret;
		}
#ifdef WRAPPER_DEBUG
		printf("open returned: %d\n", ret + CT_FD_OFFSET);
#endif
		return ret + CT_FD_OFFSET;
	}
	else{
		if(oflag & O_CREAT){
			va_list ap;
			mode_t mode;
			va_start(ap, oflag);
			mode = va_arg(ap, mode_t);
			return real_ops.OPEN(path, oflag, mode);
		}
		else{
			return real_ops.OPEN(path, oflag);
		}
	}
}

int ct_open64(const char *path, int oflag, ...){
	if(*path == '\\' || *path != '/'){
		int ret;
		if(oflag & O_CREAT){
			va_list ap;
			mode_t mode;
			va_start(ap, oflag);
			mode = va_arg(ap, mode_t);
			PRINT_FUNC;
#ifdef WRAPPER_DEBUG
			printf("\t\tpath: %s\n", path);
#endif
			ret = ctfs_open((*path == '\\')? path + 1 : path, oflag, mode);
			if(ret == -1 && *path != '\\'){
				return real_ops.OPEN(path, oflag, mode);
			}
		}
		else{
#ifdef WRAPPER_DEBUG
			printf("\t\tpath: %s\n", path);
#endif
			PRINT_FUNC;
			ret = ctfs_open((*path == '\\')? path + 1 : path, oflag);
			if(ret == -1 && *path != '\\'){
				return real_ops.OPEN(path, oflag);
			}
		}
		if(ret == -1){
			return ret;
		}
#ifdef WRAPPER_DEBUG
		printf("open returned: %d\n", ret + CT_FD_OFFSET);
#endif
		return ret + CT_FD_OFFSET;
	}
	else{
		if(oflag & O_CREAT){
			va_list ap;
			mode_t mode;
			va_start(ap, oflag);
			mode = va_arg(ap, mode_t);
			return real_ops.OPEN(path, oflag, mode);
		}
		else{
			return real_ops.OPEN(path, oflag);
		}
	}
}

OP_DEFINE(LIBC_OPEN64){
	if(*path == '\\' || *path != '/'){
		int ret;
		if(oflag & O_CREAT){
			va_list ap;
			mode_t mode;
			va_start(ap, oflag);
			mode = va_arg(ap, mode_t);
			PRINT_FUNC;
#ifdef WRAPPER_DEBUG
			printf("\t\tpath: %s\n", path);
#endif
			ret = ctfs_open((*path == '\\')? path + 1 : path, oflag, mode);
			if(ret == -1 && *path != '\\'){
				return real_ops.OPEN(path, oflag, mode);
			}
		}
		else{
#ifdef WRAPPER_DEBUG
			printf("\t\tpath: %s\n", path);
#endif
			PRINT_FUNC;
			ret = ctfs_open((*path == '\\')? path + 1 : path, oflag);
			if(ret == -1 && *path != '\\'){
				return real_ops.OPEN(path, oflag);
			}
		}
		if(ret == -1){
			return ret;
		}
#ifdef WRAPPER_DEBUG
		printf("open returned: %d\n", ret + CT_FD_OFFSET);
#endif
		return ret + CT_FD_OFFSET;
	}
	else{
		if(oflag & O_CREAT){
			va_list ap;
			mode_t mode;
			va_start(ap, oflag);
			mode = va_arg(ap, mode_t);
			return real_ops.OPEN(path, oflag, mode);
		}
		else{
			return real_ops.OPEN(path, oflag);
		}
	}
}

OP_DEFINE(OPENAT){
	if(*path == '\\' || *path != '/'){
		int ret;
		if(oflag & O_CREAT){
			va_list ap;
			mode_t mode;
			va_start(ap, oflag);
			mode = va_arg(ap, mode_t);
			PRINT_FUNC;
			ret = ctfs_openat(dirfd - CT_FD_OFFSET, (*path == '\\')? path + 1 : path, oflag, mode);
			if(ret == -1 && *path != '\\'){
				return real_ops.OPENAT(dirfd, path, oflag, mode);
			}
		}
		else{
			PRINT_FUNC;
			ret = ctfs_openat(dirfd - CT_FD_OFFSET, (*path == '\\')? path + 1 : path, oflag);
			if(ret == -1 && *path != '\\'){
				return real_ops.OPENAT(dirfd, path, oflag);
			}
		}
		if(ret == -1){
			return ret;
		}
		return ret + CT_FD_OFFSET;
	}
	else{
		if(oflag & O_CREAT){
			va_list ap;
			mode_t mode;
			va_start(ap, oflag);
			mode = va_arg(ap, mode_t);
			return real_ops.OPENAT(dirfd, path, oflag, mode);
		}
		else{
			return real_ops.OPENAT(dirfd, path, oflag);
		}
	}
}

OP_DEFINE(CREAT){
	if(*path == '\\' || *path != '/'){
		PRINT_FUNC;
		int ret = ctfs_open((*path == '\\')? path + 1 : path, O_CREAT, mode);
		if(ret == -1 && *path != '\\' ){
			return real_ops.CREAT(path, mode);
		}
		if(ret != -1){
			ctfs_close(ret);
		}
		return 0;
	}
	else{
		return real_ops.CREAT(path, mode);
	}
}

OP_DEFINE(CLOSE){
	if(file >= CT_FD_OFFSET){
		PRINT_FUNC;
		return ctfs_close(file - CT_FD_OFFSET);
	}
	else{
		return real_ops.CLOSE(file);
	}
}

OP_DEFINE(ACCESS){
	if(*pathname == '\\' || *pathname != '/'){
		PRINT_FUNC;
		int ret = ctfs_access((*pathname == '\\')? pathname + 1 : pathname, mode);
		if(ret == -1 && *pathname != '\\'){
			return real_ops.ACCESS(pathname, mode);
		}
		if(ret != -1){
			return 0;
		}
		return -1;
	}
	else{
		return real_ops.ACCESS(pathname, mode);
	}
}

OP_DEFINE(SEEK){
	if(file >= CT_FD_OFFSET){
		PRINT_FUNC;
		return ctfs_lseek(file - CT_FD_OFFSET, offset, whence);
	}
	else{
		return real_ops.SEEK(file, offset, whence);
	}
}

OP_DEFINE(TRUNC){
	if(*path == '\\' || *path != '/'){
		PRINT_FUNC;
		if(ctfs_truncate((*path == '\\')? path + 1 : path, length) == -1){
			if(*path != '\\'){
				return real_ops.TRUNC(path, length);
			}
			return -1;
		}
		return 0;
	}
	else{
		return real_ops.TRUNC(path, length);
	}
}

OP_DEFINE(FTRUNC){
	if(file >= CT_FD_OFFSET){
		PRINT_FUNC;
		return ctfs_ftruncate(file - CT_FD_OFFSET, length);
	}
	else{
		return real_ops.FTRUNC(file, length);
	}
}

OP_DEFINE(LINK){
	if(*path1 == '\\' || *path1 != '/'){
		PRINT_FUNC;
		if(ctfs_link((*path1 == '\\')? path1 + 1 : path1, (*path2 == '\\')? path2 + 1 : path2) == -1){
			if(*path1 != '\\'){
				return real_ops.LINK(path1, path2);
			}
			return -1;
		}
		return 0;
	}
	else{
		return real_ops.LINK(path1, path2);
	}
}

OP_DEFINE(UNLINK){
	if(*path == '\\' || *path != '/'){
		PRINT_FUNC;
		if(ctfs_unlink((*path == '\\')? path + 1 : path) == -1){
			if(*path != '\\'){
				return real_ops.UNLINK(path);
			}
			return -1;
		}
#ifdef CTFS_DEBUG
		printf("unlinked: %s\n", path);
#endif
		return 0;
	}
	else{
		return real_ops.UNLINK(path);
	}
}

OP_DEFINE(FSYNC){
	if(file >= CT_FD_OFFSET){
		return 0;
	}
	else{
		return real_ops.FSYNC(file);
	}
}

OP_DEFINE(READ){
	if(file >= CT_FD_OFFSET){
		PRINT_FUNC;
		return ctfs_read(file - CT_FD_OFFSET, buf, length);
	}
	else{
		if(real_ops.READ == NULL){
			insert_real_op();
		}
		return real_ops.READ(file, buf, length);
	}
}

ssize_t ct_read(int file, void* buf, size_t length){
	if(file >= CT_FD_OFFSET){
		PRINT_FUNC;
		return ctfs_read(file - CT_FD_OFFSET, buf, length);
	}
	else{
		if(real_ops.READ == NULL){
			insert_real_op();
		}
		return real_ops.READ(file, buf, length);
	}
}

OP_DEFINE(READ2){
	if(file >= CT_FD_OFFSET){
		PRINT_FUNC;
		return ctfs_read(file - CT_FD_OFFSET, buf, length);
	}
	else{
		return real_ops.READ2(file, buf, length);
	}
}

OP_DEFINE(WRITE){
	if(file >= CT_FD_OFFSET){
		// PRINT_FUNC;
		return ctfs_write(file - CT_FD_OFFSET, buf, length);
	}
	else{
		return real_ops.WRITE(file, buf, length);
	}
}

OP_DEFINE(PREAD){
	if(file >= CT_FD_OFFSET){
		PRINT_FUNC;
		// printf("\t\tread: %lu\n", count);
		return ctfs_pread(file - CT_FD_OFFSET, buf, count, offset);
	}
	else{
		return real_ops.PREAD(file, buf, count, offset);
	}
}

OP_DEFINE(PREAD64){
	if(file >= CT_FD_OFFSET){
		// PRINT_FUNC;
		// printf("\t\tread: %lu\n", count);
		return ctfs_pread(file - CT_FD_OFFSET, buf, count, offset);
	}
	else{
		return real_ops.PREAD64(file, buf, count, offset);
	}
}

OP_DEFINE(PWRITE){
	if(file >= CT_FD_OFFSET){
		PRINT_FUNC;
		return ctfs_pwrite(file - CT_FD_OFFSET, buf, count, offset);
	}
	else{
		return real_ops.PWRITE(file, buf, count, offset);
	}
}

OP_DEFINE(PWRITE64){
	if(file >= CT_FD_OFFSET){
		PRINT_FUNC;
		return ctfs_pwrite(file - CT_FD_OFFSET, buf, count, offset);
	}
	else{
		return real_ops.PWRITE64(file, buf, count, offset);
	}
}

OP_DEFINE(STAT){
	if(*path == '\\' || *path != '/'){
		PRINT_FUNC;
		if(ctfs_stat((*path == '\\')? path + 1 : path, buf) == -1){
			if(*path != '\\'){
				return real_ops.STAT(path, buf);
			}
			return -1;
		}
		return 0;
	}
	else{
		return real_ops.STAT(path, buf);
	}
}

OP_DEFINE(STAT64){
	if(*path == '\\' || *path != '/'){
		PRINT_FUNC;
		if(ctfs_stat((*path == '\\')? path + 1 : path, (struct stat*)buf) == -1){
			if(*path != '\\'){
				return real_ops.STAT(path, (struct stat*)buf);
			}
			return -1;
		}
		return 0;
	}
	else{
		return real_ops.STAT64(path, buf);
	}
}

OP_DEFINE(FSTAT){
	if(file >= CT_FD_OFFSET){
		PRINT_FUNC;
		return ctfs_fstat(file - CT_FD_OFFSET, buf);
	}
	else{
		return real_ops.FSTAT(file, buf);
	}
}

OP_DEFINE(FSTAT64){
	if(file >= CT_FD_OFFSET){
		PRINT_FUNC;
		return ctfs_fstat(file - CT_FD_OFFSET, (struct stat*)buf);
	}
	else{
		return real_ops.FSTAT(file, (struct stat*)buf);
	}
}


OP_DEFINE(LSTAT){
	if(*path == '\\' || *path != '/'){
		PRINT_FUNC;
		if(ctfs_stat((*path == '\\')? path + 1 : path, buf) == -1){
			if(*path != '\\'){
				return real_ops.LSTAT(path, buf);
			}
			return -1;
		}
		return 0;
	}
	else{
		return real_ops.LSTAT(path, buf);
	}
}

OP_DEFINE(RENAME){
	if(*old == '\\' || *old != '/'){
		PRINT_FUNC;
		if(ctfs_rename((*old == '\\')? old + 1 : old, (*new == '\\')? new + 1 : new) == -1){
			if(*new != '\\'){
				return real_ops.RENAME(new, new);
			}
			return -1;
		}
		return 0;
	}
	else{
		return real_ops.RENAME(old, new);
	}
}

OP_DEFINE(MKDIR){
	if(*path == '\\' || *path != '/'){
		PRINT_FUNC;
		if(ctfs_mkdir((*path == '\\')? path + 1 : path, mode) == -1){
			if(*path != '\\'){
				return real_ops.MKDIR(path, mode);
			}
			return -1;
		}
		return 0;
	}
	else{
		return real_ops.MKDIR(path, mode);
	}
}

OP_DEFINE(RMDIR){
	if(*path == '\\' || *path != '/'){
		PRINT_FUNC;
		if(ctfs_rmdir((*path == '\\')? path + 1 : path) == -1){
			if(*path != '\\'){
				return real_ops.RMDIR(path);
			}
			return -1;
		}
		return 0;
	}
	else{
		return real_ops.RMDIR(path);
	}
}

OP_DEFINE(FSTATFS){
	if(fd >= CT_FD_OFFSET){
		PRINT_FUNC;
		return ctfs_fstatfs(fd - CT_FD_OFFSET, buf);
	}
	else{
		return real_ops.FSTATFS(fd, buf);
	}
}

OP_DEFINE(FDATASYNC){
	if(fd >= CT_FD_OFFSET){
		return 0;
	}
	else{
		return real_ops.FDATASYNC(fd);
	}
}

OP_DEFINE(FCNTL){
	va_list ap;
	int ret;
	if(fd >= CT_FD_OFFSET){
		PRINT_FUNC;
		switch (cmd)
		{
		case F_GETFD :
			return FD_CLOEXEC;
			break;
		case F_GETFL :
			// printf("got GETFL!\n");
			ret = ctfs_fcntl(fd - CT_FD_OFFSET, cmd);
			return ret;
			break;
		case F_SETFD :
			return 0;
			break;
		case F_SETFL :
			va_start(ap, cmd);
			int val = va_arg(ap, int);
			return ctfs_fcntl(fd - CT_FD_OFFSET, cmd, val);
			break;
		case F_SET_RW_HINT:
			return 0;
			break;
		default:
			return 0;
			break;
		}
	}
	else{
		va_list ap;
		va_start(ap, cmd);
		switch (cmd)
		{
		case F_GETFD :
		case F_GETFL :
			real_ops.FCNTL(fd, cmd);
			break;
		case F_SETFD :
		case F_SETFL :
			
			real_ops.FCNTL(fd, cmd, va_arg(ap, int));
		case F_SET_RW_HINT:
			return real_ops.FCNTL(fd, cmd, va_arg(ap, void*));
			break;
		default:
			return real_ops.FCNTL(fd, cmd);
			break;
		}
	}
	return 0;
}

OP_DEFINE(OPENDIR){
	if(*path == '\\' || *path != '/'){
		PRINT_FUNC;
		DIR* ret = ctfs_opendir((*path == '\\')? path + 1 : path);
		if(ret == NULL){
			if(*path != '\\'){
				return real_ops.OPENDIR(path);
			}
			return NULL;
		}
		return (DIR*)((uint64_t)ret);
	}
	else{
		if(inited == 0){
			insert_real_op();
		}
		return real_ops.OPENDIR(path);
	}
}

OP_DEFINE(CLOSEDIR){
	if((uint64_t)dirp < CT_MAX_FD){
		PRINT_FUNC;
		return ctfs_closedir((DIR*) ((uint64_t)dirp));
	}
	else{
		return real_ops.CLOSEDIR(dirp);
	}
}

OP_DEFINE(READDIR){
	if((uint64_t)dirp < CT_MAX_FD){
		PRINT_FUNC;
		return ctfs_readdir((DIR*) ((uint64_t)dirp));
	}
	else{
		return real_ops.READDIR(dirp);
	}
}

OP_DEFINE(READDIR64){
	if((uint64_t)dirp < CT_MAX_FD){
		PRINT_FUNC;
		return (struct dirent64*)ctfs_readdir((DIR*) ((uint64_t)dirp));
	}
	else{
		return real_ops.READDIR64(dirp);
	}
}

static int err;

OP_DEFINE(ERROR){
	if(real_ops.ERROR==0){
		insert_real_op();
	}
	if(*ctfs_errno() == 0){
		return real_ops.ERROR();
	}
	else{
		err = *ctfs_errno();
		*ctfs_errno() = 0;
		return &err;
	}
}


OP_DEFINE(SYNC_FILE_RANGE){
	if(fd >= CT_FD_OFFSET ){
		PRINT_FUNC;
		return 0;
	}
	else{
		return real_ops.SYNC_FILE_RANGE(fd, offset, nbytes, flags);
	}
}

/*******************************************************
 * File stream functions
 *******************************************************/
OP_DEFINE(FOPEN){
	if(*path == '\\' || *path != '/'){
		FILE * ret;
		PRINT_FUNC;
		ret = _fopen((*path == '\\')? path + 1 : path, mode);
		if(ret == NULL && *path != '\\'){
			return real_ops.FOPEN(path, mode);
		}
		return ret;
	}
	else{
		return real_ops.FOPEN(path, mode);
	}
}

OP_DEFINE(FPUTS){
	if(1){
		if((stream->_flags & _IO_MAGIC_MASK) == _IO_MAGIC_CTFS){
			PRINT_FUNC;
			return _fputs(str, stream);
		}
	}
		return real_ops.FPUTS(str, stream);
}

OP_DEFINE(FGETS){
	if(stream){
		if((stream->_flags & _IO_MAGIC_MASK) == _IO_MAGIC_CTFS){
			PRINT_FUNC;
			return _fgets(str, n, stream);
		}
	}
		return real_ops.FGETS(str, n, stream);
}

OP_DEFINE(FWRITE){
	if(1){
		if((fp->_flags & _IO_MAGIC_MASK) == _IO_MAGIC_CTFS){
			PRINT_FUNC;
			return _fwrite(buf, length, nmemb, fp);
			// return nmemb;
		}
	}
	return real_ops.FWRITE(buf, length, nmemb, fp);
}

OP_DEFINE(FREAD){
	if(fp){
		if((fp->_flags & _IO_MAGIC_MASK) == _IO_MAGIC_CTFS){
			PRINT_FUNC;
			return _fread(buf, length, nmemb, fp);
		}
	}
	return real_ops.FREAD(buf, length, nmemb, fp);
}

OP_DEFINE(FCLOSE){
	if(fp){
		if((fp->_flags & _IO_MAGIC_MASK) == _IO_MAGIC_CTFS){
			PRINT_FUNC;
			return _fclose(fp);
		}
	}
	return real_ops.FCLOSE(fp);
}

OP_DEFINE(FSEEK){
	if(fp){
		if((fp->_flags & _IO_MAGIC_MASK) == _IO_MAGIC_CTFS){
			PRINT_FUNC;
			return _fseek(fp, offset, whence);
		}
	}
	return real_ops.FSEEK(fp, offset, whence);
}

OP_DEFINE(FFLUSH){
	if(fp){
		if((fp->_flags & _IO_MAGIC_MASK) == _IO_MAGIC_CTFS){
			PRINT_FUNC;
			return _fflush(fp);
		}
	}
	return real_ops.FFLUSH(fp);
}



static __attribute__((constructor(120) )) void init_method(void)
{
    if(real_ops.ERROR == 0){
		insert_real_op();
		inited = 1;
	}
    
	printf("Starting to initialize ctFS. \nInstalling real syscalls...\n");
    printf("Real syscall installed. Initializing ctFS...\n");
#ifdef CTFS_ATOMIC_WRITE
	printf("Atomic write is enabled!\n");
#endif
	if(real_ops.ERROR != 0){
		ctfs_init(0);
		printf("ctFS initialized. \nNow the program begins.\n");
		return;
	}
	printf("Fialed to init\n");
	inited = 0;
	
}


