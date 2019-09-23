#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>

struct superblock{
	unsigned long int magic_number; //0xfa19283e
	unsigned long int n; //number of blocks on file system
	unsigned long int k; //number of blocks in FAT
	unsigned long int block_size; //assigned for file system
	unsigned long int root_block; //starting block of the root dir
};

struct fatblock{
	long int fat_entry[128];
};

struct directory {
	char file_name[24]; //File name (null-terminated string, up to 24 bytes long including the null)
	long long int  creation_time;
	long long int  modification_time;
	long long int  access_time;
	unsigned long int file_length;
	long int start_block;
	unsigned long int flag;
	unsigned long int unused;
};


