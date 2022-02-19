#include "../ctfs.h"

int main(int argc, char ** argv){
    int flag = 0;
    if(argc > 1){
        if(atoi(argv[1])){
            flag |= CTFS_MKFS_FLAG_RESET_DAX;
            printf("Set to reset dax\n");
        }
    }
    int ret = ctfs_mkfs(flag);
    if(ret){
        printf("Failed to format!!\n");
        return -1;
    }
    printf("ctFS formated!\n");
    return 0;
    // int fd = open("/home/robin/d", O_RDWR);
    // printf("open return %d\n", fd);
    // int fd2 = open("/home/robin/d", O_RDWR);
    // printf("open return %d\n", fd2);
}
