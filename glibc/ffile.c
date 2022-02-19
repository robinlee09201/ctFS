#include "ffile.h"
#include <sys/types.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#define __open ctfs_open
#define __close ctfs_close
#define __read ctfs_read
#define __write ctfs_write
#define __lseek ctfs_lseek

// MODE NOT STORED YET, need to do if necessary

/* _IO_pos_BAD is an off_t value indicating error, unknown, or EOF.  */
#define POS_BAD ((off_t) -1)

#define _IO_mask_flags(fp, f, mask) \
       ((fp)->_flags = ((fp)->_flags & ~(mask)) | ((f) & (mask)))

FILE * _fopen ( const char * filename, const char * mode ) {
    FILE *fp = malloc(sizeof(FILE));

    if (fp == NULL) {
        return NULL;
    }

    int oflags = 0, omode;
    int read_write;
    int oprot = 0666;
    int i;
    const char *last_recognized;
    
    switch (*mode)
    {
    case 'r':
      omode = O_RDONLY;
      read_write = _IO_NO_WRITES;
      break;
    case 'w':
      omode = O_WRONLY;
      oflags = O_CREAT|O_TRUNC;
      read_write = _IO_NO_READS;
      break;
    case 'a':
      omode = O_WRONLY;
      oflags = O_CREAT|O_APPEND;
      read_write = _IO_NO_READS|_IO_IS_APPENDING;
      break;
    default:
      __set_errno (EINVAL);
      return NULL;
    }

    int un_supported = 0;
    last_recognized = mode;
    for (i = 1; i < 7; ++i)
    {
        switch (*++mode)
        {
        case '\0':
            break;
        case '+':
            omode = O_RDWR;
            read_write &= _IO_IS_APPENDING;
            last_recognized = mode;
            continue;
        case 'x':
            oflags |= O_EXCL;
            last_recognized = mode;
            continue;
        case 'b':
            last_recognized = mode;
            continue;
        case 'm':
        case 'c':
        case 'e':
            un_supported = 1;
            continue;
        default:
            /* Ignore.  */
            continue;
        }
        break;
    }

    if (un_supported == 1) {
        __set_errno (ENOSYS);
        return NULL;
    }

    /*_IO_file_open*/
    int fdesc = __open (filename, omode|oflags, oprot);
    if (fdesc < 0)
        return NULL;

    fp->_fileno = fdesc;
    fp->_flags = _IO_MAGIC_CTFS;
    _IO_mask_flags (fp, read_write,_IO_NO_READS+_IO_NO_WRITES+_IO_IS_APPENDING);

    off_t new_pos = __lseek (fdesc, 0, SEEK_END);
    if (new_pos == POS_BAD && errno != ESPIPE)
	{
	  __close(fdesc);
	  return NULL;
	}

    // in read mode, the file pointer should be set to the begining of the file
    if (read_write == _IO_NO_WRITES) {
        __lseek (fdesc, 0, SEEK_SET);
    }
    /*_IO_file_open*/

    if (fp != NULL) 
    {
        const char *cs;
        cs = strstr (last_recognized + 1, ",ccs=");
        if (cs != NULL)
        {
            __set_errno (ENOSYS);
            return NULL;
        }
    }

    return fp;
}

int _fputs(const char *str, FILE *stream) {
    if (stream->_fileno < 0)
    {
        return -1;
    } else {
        size_t str_len = strlen(str);
        printf("%ld\n", str_len);

        return __write(stream->_fileno, str, str_len);
    }
}

char *_fgets(char *str, int n, FILE *stream) {
    if (stream->_fileno < 0 || n <= 0)
    {
        return NULL;
    }

    int64_t original_file_ptr = __lseek(stream->_fileno, 0, SEEK_CUR);

    ssize_t read_len = __read(stream->_fileno, str, n - 1);

    if (read_len == -1) {
        __lseek(stream->_fileno, original_file_ptr, SEEK_SET);
        return NULL;
    }

    if (read_len == 0 || read_len < n - 1) {
        // means hit EOF

        // hit EOF again, error, return NULL
        if ( (stream->_flags & _IO_EOF_SEEN) != 0 && read_len == 0) {
            return NULL;
        }
        stream->_flags |= _IO_EOF_SEEN;
    }
    str[read_len] = '\0';

    return str;
}

size_t _fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream) {
    if (stream->_fileno < 0)
    {
        return -1;
    } else {
        size_t str_len = size * nmemb;
        return __write(stream->_fileno, ptr, str_len);
    }
}

size_t _fread( void* buffer, size_t size, size_t count, FILE* stream ) {
    if (stream->_fileno < 0)
    {
        return -1;
    } else {
        size_t read_len = size * count;

        int64_t original_file_ptr = __lseek(stream->_fileno, 0, SEEK_CUR);
        size_t actual_read_len = __read(stream->_fileno, buffer, read_len);
        if (actual_read_len == -1) {
            __lseek(stream->_fileno, original_file_ptr, SEEK_SET);
            return 0;
        }


        return actual_read_len;
    }
}


int _fclose ( FILE * stream ) {
    if (stream->_fileno < 0) {
        return EOF;
    }

    __close(stream->_fileno);
    stream->_fileno = -1;

    return 0;
}

int _fseek(FILE *stream, long int offset, int whence) {
    if (stream->_fileno < 0) {
        return EOF;
    }

    return (__lseek(stream->_fileno, offset, whence) == -1);
}

int _fflush(FILE* stream) {
    if (stream->_fileno < 0) {
        return EOF;
    }

    return 0;
}