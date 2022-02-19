#include <string>
#include <iostream>
#include <filesystem>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#define DEFAULT_PATH "/testdir/"
#define DEFAULT_SIZE 71303168
int main(int argc, char * argv[]) {
	int opt;
	opterr = 0;
	unsigned num = 1000;
	std::string src;
	std::string dest;
	bool print_only = false;
	while((opt = getopt(argc, argv, "s:d:")) != -1){
		switch(opt){
			case 's':{
				src = std::string(optarg);
				break;
			}
			case 'd':{
				dest = std::string(optarg);
				break;
			}
			default: {
				fprintf(stderr, "Usage: %s: \n\t[-s source] \n\t[-d dest]\n",
				argv[0]);
				exit(0);
			}
		}
	}
	int fd = open(src.c_str(), O_RDONLY);
	if(fd < 0){
		return -1;
	}
	auto src_sta = (struct stat*) malloc(sizeof(struct stat));
	fstat(fd, src_sta);
	unsigned long size = src_sta->st_size;

	void * buffer = malloc(size);
	long res = pread(fd, buffer, size, 0);
	if(res == -1){
		std::cout << "Failed to open" << dest << "\n";
		return -1;
	}
	while(res != size){
		std::cout << "retry read!\n";
		res += pread(fd, buffer + res, size - res, res);
	}
	close(fd);

	fd = open(dest.c_str(), O_RDWR | O_CREAT, S_IRWXU);
	if(res == -1){
		std::cout << "Failed to open" << dest << "\n";
		return -1;
	}
	res = pwrite(fd, buffer, size, 0);
	if(res == -1){
		return -1;
	}
	while(res != size){
		std::cout << "retry write!\n";
		res += pwrite(fd, buffer + res, size - res, res);
	}

	std::cout << "write " << size << " bytes finished\nFrom file " << src << " to " << dest << "\n";
}