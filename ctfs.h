#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/statfs.h>
#include <sys/vfs.h>
#include <inttypes.h>
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
#include <stdint.h>
#include <stdio.h>
#include <assert.h>


#define CTFS_MKFS_FLAG_RESET_DAX        0x01

#define CTFS_O_ATOMIC					010

void print_debug(int fd);

int* ctfs_errno();

int ctfs_mkfs(int flag);

int ctfs_init(int flag);

int ctfs_mkdir(const char *pathname, uint16_t mode);

int ctfs_stat(const char *pathname, struct stat *buf); // TODO

int  ctfs_fstat (int fd, struct stat *buf);// TODO

// int  ctfs_fstat64 (int fd, struct stat64 *buf);

int  ctfs_lstat (const char *pathname, struct stat *buf); // TODO

int  ctfs_statvfs(const char *path, struct statvfs *buf); // TODO

int  ctfs_fstatvfs(int fd, struct statvfs *buf); // TODO

int ctfs_fstatfs(int fd, struct statfs *buf); // ******

int ctfs_truncate(const char *path, off_t length);

int ctfs_ftruncate(int fd, off_t len);

int  ctfs_open (const char *pathname, int flags, ...); 

int ctfs_openat (int dirfd, const char *pathname, int flags, ...);

int  ctfs_creat (const char *pathname, uint16_t mode); 

int  ctfs_close (int fd);

ssize_t  ctfs_write(int fd, const void *buf, size_t count); 

ssize_t  ctfs_pwrite(int fd, const void *buf, size_t count, off_t offset);

ssize_t  ctfs_read(int fd, void *buf, size_t count);

ssize_t  ctfs_pread(int fd, void *buf, size_t count, off_t offset);

int  ctfs_link(const char *oldpath, const char *newpath); // TODO

int  ctfs_unlink (const char *pathname); // TODO

int  ctfs_rmdir (const char *pathname); // TODO

int  ctfs_rename (const char *oldpath, const char *newpath); // ******

DIR* ctfs_opendir(const char *_pathname); // ******

struct dirent * ctfs_readdir(DIR *dirp); // ******

int ctfs_closedir(DIR *dirp); // ******

int  ctfs_chdir(const char *path); // TODO

char *ctfs_getcwd(char *buf, int size); // TODO

int  ctfs_lseek(int fd, int offset, int whence); // ******

int  ctfs_access(const char * pathname, int mode); // ******

int ctfs_fcntl(int fd, int cmd, ...);// ******
