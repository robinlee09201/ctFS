#include <stdio.h>
#include "../ctfs.h"

/* Magic number and bits for the _flags field.  The magic number is
   mostly vestigial, but preserved for compatibility.  It occupies the
   high 16 bits of _flags; the low 16 bits are actual flag bits.  */

#define _IO_MAGIC         0xFBAD0000 /* Magic number */
#define _IO_MAGIC_CTFS    0xCFCF0000 
#define _IO_MAGIC_MASK    0xFFFF0000
#define _IO_USER_BUF          0x0001 /* Don't deallocate buffer on close. */
#define _IO_UNBUFFERED        0x0002
#define _IO_NO_READS          0x0004 /* Reading not allowed.  */
#define _IO_NO_WRITES         0x0008 /* Writing not allowed.  */
#define _IO_EOF_SEEN          0x0010
#define _IO_ERR_SEEN          0x0020
#define _IO_DELETE_DONT_CLOSE 0x0040 /* Don't call close(_fileno) on close.  */
#define _IO_LINKED            0x0080 /* In the list of all open files.  */
#define _IO_IN_BACKUP         0x0100
#define _IO_LINE_BUF          0x0200
#define _IO_TIED_PUT_GET      0x0400 /* Put and get pointer move in unison.  */
#define _IO_CURRENTLY_PUTTING 0x0800
#define _IO_IS_APPENDING      0x1000
#define _IO_IS_FILEBUF        0x2000
                           /* 0x4000  No longer used, reserved for compat.  */
#define _IO_USER_LOCK         0x8000

#define __set_errno(e) (errno = (e))

// struct FILE
// {
//     int _flags;		/* High-order word is _IO_MAGIC; rest is flags. */

//     // /* The following pointers correspond to the C++ streambuf protocol. */
//     // char *_IO_read_ptr;	/* Current read pointer */
//     // char *_IO_read_end;	/* End of get area. */
//     // char *_IO_read_base;	/* Start of putback+get area. */
//     // char *_IO_write_base;	/* Start of put area. */
//     // char *_IO_write_ptr;	/* Current put pointer. */
//     // char *_IO_write_end;	/* End of put area. */
//     // char *_IO_buf_base;	/* Start of reserve area. */
//     // char *_IO_buf_end;	/* End of reserve area. */

//     // /* The following fields are used to support backing up and undo. */
//     // char *_IO_save_base; /* Pointer to start of non-current get area. */
//     // char *_IO_backup_base;  /* Pointer to first valid character of backup area */
//     // char *_IO_save_end; /* Pointer to end of non-current get area. */

//     // struct _IO_marker *_markers;

//     // struct FILE *_chain;

//     int _fileno;
//     // int _flags2;
//     // __off_t _old_offset; /* This used to be _offset but it's too small.  */

//     // /* 1+column number of pbase(); 0 is unknown. */
//     // unsigned short _cur_column;
//     // signed char _vtable_offset;
//     // char _shortbuf[1];

//     // _IO_lock_t *_lock;
//     // __off64_t _offset;
//     // /* Wide character stream stuff.  */
//     // struct _IO_codecvt *_codecvt;
//     // struct _IO_wide_data *_wide_data;
//     // struct FILE *_freeres_list;
//     // void *_freeres_buf;
//     // size_t __pad5;
//     // int _mode;
//     // /* Make sure we don't get into trouble again.  */
//     // char _unused2[15 * sizeof (int) - 4 * sizeof (void *) - sizeof (size_t)];
// };

FILE * _fopen ( const char * filename, const char * mode );

int _fputs(const char *str, FILE *stream);

char *_fgets(char *str, int n, FILE *stream);

size_t _fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);

size_t _fread( void* buffer, size_t size, size_t count, FILE* stream );

int _fclose(FILE * stream);

int _fseek(FILE *stream, long int offset, int whence);

int _fflush(FILE* stream);