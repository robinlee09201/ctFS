#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <dirent.h>
#include <errno.h>

// #define CTFS 1
#ifdef CTFS
#define __OPEN ctfs_open
#define __CLOSE ctfs_close
#define __READ ctfs_read
#define __WRITE ctfs_write
#define __CREAT ctfs_creat

#define __STAT ctfs_stat
#define __FSTAT ctfs_fstat

#define __OPENDIR ctfs_opendir
#define __CLOSEDIR ctfs_closedir
#define __MKDIR ctfs_mkdir
#define __RENAME ctfs_rename
#define __RMDIR ctfs_rmdir
#define __CHDIR ctfs_chdir
#define __RENAME ctfs_rename
#define __RMDIR ctfs_rmdir

#define __UNLINK ctfs_unlink

#else
#define __OPEN open
#define __CLOSE close
#define __READ read
#define __WRITE write
#define __CREAT creat

#define __STAT stat
#define __FSTAT fstat

#define __OPENDIR opendir
#define __CLOSEDIR closedir
#define __MKDIR mkdir
#define __RENAME rename
#define __RMDIR rmdir
#define __CHDIR chdir
#define __RENAME rename
#define __RMDIR rmdir

#define __UNLINK unlink


#endif

// #define DEFAULT_PATH "/mnt/pmem/testfile"
#define DEFAULT_FOLDER_PATH "testfolder"
#define DEFAULT_FOLDER_PATH_2 "testfolder2"
#define DEFAULT_RENAMED_FOLDER_PATH "renamedfolder"
#define DEFAULT_PATH "testfile"
#define NON_EXIST_PATH "nullfile"

void test_assert(bool condition, int* failed_test, int test_id, const char* desc){
    if(condition){
        printf(".");
    }else{
        (*failed_test)++;
        printf("\n\033[0;41m");
        printf("[FAIL]");
        // printf("\033[0m\n");

        printf("\033[0;31m");
        printf(" Test ID #%d Failed: %s\n", test_id, desc);
        printf("\033[0m");
    }
}

void show_test_result(int failed_test, const char * desc){
    printf("\n");
    if(failed_test == 0){
        printf("\033[0;42m");
        printf("[PASS]");

        printf("\033[0;32m");
        printf(" Passed test: %s\n", desc);
        printf("\033[0m");
    }else{
        printf("\033[0;41m");
        printf("[FAIL]");

        printf("\033[0;31m");
        printf(" Failed %d test(s) in %s.\n", failed_test, desc);
        printf("\033[0m");
    }

    printf("==============\n");
}

void basic_rw_test(){
    int failed_test = 0;
    int ret_code, fd;
    ssize_t str_size;

    // test 0: open an nonexisting file
    ret_code = __OPEN(NON_EXIST_PATH, O_RDONLY);
    test_assert( ret_code == -1, &failed_test, 0, "Open a nonexisting file");

    // test 1: create a new file
    fd = __CREAT(DEFAULT_PATH, S_IRWXU );
    test_assert( fd != -1, &failed_test, 1, "Create a new file");

    // test 2: write 11 characters to the file
    const char write_str[20] = "test string";
    str_size = __WRITE(fd, write_str, strlen(write_str));
    test_assert( str_size == 11, &failed_test, 2, "Write 11 characters to the file");

    // test 3: close file
    ret_code = __CLOSE(fd);
    test_assert( ret_code != -1, &failed_test, 3, "Close file");

    // test 4: read 11 characters from the file
    fd = __OPEN(DEFAULT_PATH, O_RDONLY);
    char read_str[20];
    str_size = __READ(fd, read_str, str_size);
    test_assert( str_size == 11, &failed_test, 4, "Read 11 characters to the file");
    
    // test 4b: Sanity check file content
    bool sanity = true;
    for(size_t i = 0; i < 11; i++){
        if(write_str[i] != read_str[i]){ sanity = false; }
    }
    test_assert( sanity , &failed_test, 5, "Read sanity check");

    // test 4c: write to a read only file
    str_size = __WRITE(fd, write_str, strlen(write_str));
    test_assert( str_size == -1, &failed_test, 6, "Write to a read only file");

    // test 5: fstat the new file
    struct stat stat_buf;
    ret_code = __FSTAT(fd, &stat_buf);
    test_assert( ret_code == 0, &failed_test, 7, "Fstat basic check");
    test_assert( stat_buf.st_nlink == 1 && stat_buf.st_size == 11, &failed_test, 8, "Fstat struct check");

    ret_code = __CLOSE(fd);
    test_assert( ret_code != -1, &failed_test, 8, "Close file");

    // test 6 stat the new file
    ret_code = __STAT(DEFAULT_PATH, &stat_buf);
    test_assert( ret_code == 0, &failed_test, 9, "Stat basic check");
    test_assert( stat_buf.st_nlink == 1 && stat_buf.st_size == 11, &failed_test, 10, "Stat struct check");

    show_test_result(failed_test, "Basic rw test");
}

void basic_directory_test(){
    int failed_test = 0;
    int status, fd;

    // test 0: mkdir a new directory
    status = __MKDIR(DEFAULT_FOLDER_PATH, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    test_assert( status == 0, &failed_test, 0, "Make a new directory");
    
    // test 1: rename the new directory
    status = __RENAME(DEFAULT_FOLDER_PATH, DEFAULT_RENAMED_FOLDER_PATH);
    test_assert( status == 0, &failed_test, 1, "Rename the new directory");

    // test 2: open the original directory
    DIR* dir = __OPENDIR(DEFAULT_FOLDER_PATH);
    test_assert( dir == 0, &failed_test, 2, "Open a non-exist directory");
    test_assert( errno == ENOENT, &failed_test, 2, "Check error code");

    // test 3&4: open and close the renamed directory
    dir = __OPENDIR(DEFAULT_RENAMED_FOLDER_PATH);
    test_assert( dir != 0, &failed_test, 3, "Open a non-exist directory");
    status = __CLOSEDIR(dir);
    test_assert( status == 0, &failed_test, 4, "Close a directory");

    // test 5: remove a directory and check sanity
    status = __RMDIR(DEFAULT_RENAMED_FOLDER_PATH);
    test_assert( status == 0, &failed_test, 5, "Remove a directory");
    dir = __OPENDIR(DEFAULT_RENAMED_FOLDER_PATH);
    test_assert( dir == 0, &failed_test, 5, "Check directory gets removed");
    test_assert( errno == ENOENT, &failed_test, 5, "Check error code");

    // test 6: change directory
    status = __MKDIR(DEFAULT_FOLDER_PATH, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    char file_src[50];
    memset(file_src, 0, 50);
    strcat(file_src, DEFAULT_FOLDER_PATH);
    strcat(file_src, "/");
    strcat(file_src, DEFAULT_PATH);

    // create file under new directory
    fd = __CREAT(file_src, S_IRWXU );
    test_assert( fd != -1, &failed_test, 6, "Create a new file");
    status = __CLOSE(fd);
    test_assert( status != -1, &failed_test, 6, "Close the opened file");

    // enter new directory
    status = __CHDIR(DEFAULT_FOLDER_PATH);
    test_assert( status == 0, &failed_test, 6, "Change directory");

    // open the file through new working directory
    fd = __OPEN(DEFAULT_PATH, O_RDONLY);
    test_assert( fd != -1, &failed_test, 6, "Open the new file");
    status = __CLOSE(fd);
    test_assert( status != -1, &failed_test, 6, "Close the opened file");

    // test 7: change name
    // return to old directory
    status = __CHDIR("..");
    test_assert( status == 0, &failed_test, 7, "Change directory");
    
    status = __RENAME(DEFAULT_FOLDER_PATH, DEFAULT_FOLDER_PATH_2);
    test_assert( status == 0, &failed_test, 7, "Rename directory");

    // test 8: delete directory
    status = __RMDIR(DEFAULT_FOLDER_PATH_2);
    test_assert( status == -1, &failed_test, 8, "Delete directory with file in it");

    // test 9: unlink DEFAULT_PATH under directory DEFAULT_FOLDER_PATH_2
    memset(file_src, 0, 50);
    strcat(file_src, DEFAULT_FOLDER_PATH_2);
    strcat(file_src, "/");
    strcat(file_src, DEFAULT_PATH);
    status = __UNLINK(file_src);
    test_assert( status == 0, &failed_test, 9, "Delete file in a directory");

    // test 10: delete empty directory
    status = __RMDIR(DEFAULT_FOLDER_PATH_2);
    test_assert( status == 0, &failed_test, 10, "Delete empty directory");

    // test 11: test hard link (one file and over one hard link)


    show_test_result(failed_test, "Basic directory test");
}

int main(){
    int failed_test = 0;
    basic_rw_test();
    basic_directory_test();
    
    return 0;
}