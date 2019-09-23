//References used: https://github.com/manmeet3/FAT-file-system/blob/main/createdisk.c
//went to office hours and was told we can use github as references
//While main idea of how to create a geneal file system file is copiped, logic and mathematical values are significantly changed to make this an actual fat.
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "file_structure.h"
#include <time.h>

int main(int argc, char **argv) {
	int disk = open(argv[1], O_WRONLY | O_CREAT);

	
	if(argc <= 1) {
		perror("Disk file is NULL");
		return(-1);
	}
	if(argc > 3) {
		perror("Too many arugments");
		return(-1);
	}
	if(disk == -1) {
		perror("Invalid disk file");
		return(-1);
	}
	lseek(disk,0,SEEK_SET);

	struct superblock* super_block = calloc(512,1);
	super_block->magic_number = 0xfa19283e;
	super_block->block_size = 512; //WHERE YOU EDIT THE BLOCK SIZE, EITHER 512B OR 8KB
	super_block->n = strtol(argv[2], &argv[2], 0);
	int block_size = super_block-> block_size;
	int n = super_block->n;

	//want to set up fat size (k)
	if(n % block_size == 0) {
		super_block->k = (n / (block_size/4) );
	} else{
		super_block->k = (n / (block_size/4)) + 1;
	}

	int k = super_block->k;
	//initialize root_block, 0
	super_block->root_block = 0;

	/*You can read or write a sector by using lseek() to seek to the appropriate offset
	(512 * block_number), followed by a read() or write() of 512 bytes.*/

	int root_block = super_block->root_block; 
	lseek(disk, 0*block_size, SEEK_SET);
	write(disk, super_block, block_size);

	//FAT allocation
	/*A 0 in the FAT means that a block is free, and a -2  means that the block is the last one in the file. 
	Otherwise, the entry in the file allocation is the block number of the next block in the file. */
	
	struct fatblock* fat_block = calloc(block_size, 1);

	memset(fat_block, 0, block_size); 

	int i;
	for(i = 0; i < k + 1; i++) {
		int j;
		for(j = 0; j < block_size/4; j++) {

				fat_block->fat_entry[j] = 0;

			if(i==1 && j==0){//make sure values in first block - pointing to root directory, is set to  1, linking to block 1 EOF.
				fat_block->fat_entry[0] = 1;
			}else if(i == 1 && j == 1){
			    fat_block->fat_entry[1] = -2;
			}
		}


	/*You can read or write a sector by using lseek() to seek to the appropriate offset
	(512 * block_number), followed by a read() or write() of 512 bytes.*/
		lseek(disk, (1+i)* block_size, SEEK_SET);
		write(disk, fat_block, block_size);

	}
	//need to do root directory

	struct directory* dir_entry = calloc(block_size,1);
	//memset(dir_entry, 0, block_size);

	//Directory entries in this file system are 64 bytes long at minimum
	dir_entry->file_length = 64;

	//If the directory entry is free, the starting block field should be set to zero.
	dir_entry->start_block = 1;
	/*The only flag you must maintain is the DIRECTORY flag (bit 0 of the flags field in the directory entry),
	a single bit that indicates whether the directory entry is for a regular file or a directory. A 0 means 
	it's a regular file; a 1 means it's a directory.*/
	dir_entry->flag = 1; //its a directory
	//File name (null-terminated string, up to 24 bytes long including the null)
	char fullname[24] = "/";
	strcpy(dir_entry->file_name, fullname);
    dir_entry -> creation_time = time(NULL);
    dir_entry -> modification_time = time(NULL);
    dir_entry -> access_time = time(NULL);


	lseek(disk, root_block * block_size, SEEK_SET);
	write(disk, dir_entry, block_size);

    ftruncate(disk, (32*k+1)*block_size); // might only need 17
	//mkdir(argv[1], 0777);
	close(disk);
	return(0);
}

