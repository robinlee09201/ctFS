#include <string>
#include <iostream>
#include <filesystem>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <unistd.h>
namespace fs = std::filesystem;
#define DEFAULT_PATH "/testdir/"
#define DEFAULT_SIZE 71303168
int main(int argc, char * argv[]) {
	int opt;
	opterr = 0;
	unsigned num = 1000;
	std::string path = DEFAULT_PATH;
	char path_buf[100];
	bool print_only = false;
	while((opt = getopt(argc, argv, "n:p:cP")) != -1){
		switch(opt){
			case 'n':{
				num = atoi(optarg);
				break;
			}
			case 'c':{
				path = "\\" + path;
				break;
			}
			case 'p':{
				path = std::string(optarg);
				break;
			}
			case 'P':{
				print_only = true;
				break;
			}
			default: {
				fprintf(stderr, "Usage: %s: \n\t[-w write] \n\t[-r read] \n\t[-R random] \n\t[-a append] \n\t[-S fsync] \n\t[-s size] \n\t[-n number of files] \n\t[-G size in GB] \n\t[-M size in MB] \n\t[-K size in KB] \n\t[-p path] \n\t[-F use file stream]\n",
				argv[0]);
				exit(0);
			}
		}
	}
	if(print_only){
		int i = 0;
		for (const auto & entry : fs::directory_iterator(path))
		{
			auto p = entry.path();
			std::cout << p << std::endl;
			i++;
		}
		std::cout << "total " << i << "files\n";
		return 0;
	}
	std::vector<std::string> path_vec;
	for(int i = 0; i < num; i++){
		path_vec.push_back(path + std::to_string(i));
	}
	int * buf = (int*)malloc(4096);
	*buf = 2;
	for (auto p : path_vec){
		int fd = open(p.c_str(), O_RDWR | O_CREAT, S_IRWXU);
		for(unsigned long i = 0; i < DEFAULT_SIZE / 64; i++){
			unsigned long res = pwrite(fd, buf, 64, i * 64);
			if(res != 64){
				std::cout << "error!! write: "<< res <<std::endl; 
			}
		}
		close(fd);
	}
	unsigned n = 0;
    for (const auto & entry : fs::directory_iterator(path))
	{
		auto p = entry.path();
		int fd = open(p.c_str(), O_RDWR, S_IRWXU);
		if(fd < 0){
			std::cout << "error open: " << p << std::endl;
		}
		else{
			close(fd);
			n++;
		}
	}
    std::cout << "total of "<<n<<" files created" <<std::endl;
}