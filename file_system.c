//REFERENCES USED:
//We are using fusexmp.c and hello.c from the example folder for guidance
//http://www.maastaar.net/fuse/linux/filesystem/c/2016/05/21/writing-a-simple-filesystem-using-fuse/
//https://www.cs.hmc.edu/~geoff/classes/hmc.cs135.201001/homework/fuse/fuse_doc.html
//https://github.com/manmeet3/FAT-file-system/blob/main/myfilesys.c

#define FUSE_USE_VERSION 26

#ifdef HAVE_CONFIG_H
#include <config.h>
#include <stdlib.h>
#endif

#ifdef linux
/* For pread()/pwrite()/utimensat() */
#define _XOPEN_SOURCE 700
#endif

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#include <stdlib.h>
#include "file_structure.h"
#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#include "file_structure.h"
#include <stdlib.h>
#endif

FILE* disk_fd;
struct superblock *super;
int curr_block = 0;
int curr_block_pos = 0;
struct directory dir_entry; 
int32_t* fattable = NULL;
struct directory directoryblock[16];
int fd;

int read_block(int fd, int blocknum, void* rblock){
	lseek(fd, blocknum*512, SEEK_SET);
	return read(fd, rblock, 512);
}
void* init(struct fuse_conn_info *conn) { 
//Initialize the filesystem. This function can often be left unimplemented,
//but it can be a handy way to perform one-time setup such as allocating 
//variable-sized data structures or initializing a new filesystem.
	disk_fd = fopen("disk_file", "r+");

	fseek(disk_fd, 0, SEEK_SET);
	
	super = calloc(512,1);
	fread(&super, sizeof(struct superblock), 1, disk_fd);

	return NULL;

}

static struct options {
	const char *filename;
	const char *contents;
} options;

static int xmp_getattr(const char *path, struct stat *stbuf)
{
	struct directory *scan_dir = calloc(sizeof(struct directory),1);
	char* name = strrchr(path, '/');
	struct fatblock *fatblock_scan = calloc(sizeof(struct fatblock),1);
	fseek(disk_fd, 512* curr_block, SEEK_SET);
	fread(&fatblock_scan, sizeof(struct fatblock),1, disk_fd);  //set scanned directory block
	int curr_scan_pos = fatblock_scan->fat_entry[curr_block_pos];
	int curr_scan_block = curr_scan_pos/128;
	int curr_scan_block_pos = curr_scan_pos%128;
	
	while(curr_scan_pos > 0){
	    fseek(disk_fd,((super->k + 1) * 512) + curr_scan_pos *64, SEEK_SET);
	    fread(&scan_dir, sizeof(struct directory) , 1, disk_fd);
	    if(scan_dir->file_name == name){
	        break;
	    }else{
	        fseek(disk_fd, (curr_scan_block + 1) * 512, SEEK_SET);
	        fread(&fatblock_scan, sizeof(struct fatblock),1, disk_fd);  //set scanned directory block
	        curr_scan_pos = fatblock_scan -> fat_entry[curr_scan_block_pos];
	        curr_scan_block = curr_scan_pos/128;
            curr_scan_block_pos = curr_scan_pos%128;
	    }
	}
	    
	
	//stbuf->st_uid = getuid();
	//stbuf->st_gid = getgid();
	stbuf->st_ctime= scan_dir->creation_time;
	stbuf->st_atime= scan_dir->access_time;
	//fill in rest of entries
	

	//if ( strcmp( path, "/" ) == 0 )
	//{
	//	stbuf->st_mode = S_IFDIR | 0755;
	//	stbuf->st_nlink = 2;
	//}
	//else
	//{
	//	stbuf->st_mode = S_IFREG | 0644;
	//	stbuf->st_nlink = 1;
	//	stbuf->st_size = 512;
	//}

	return 0;

}

static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		       off_t offset, struct fuse_file_info *fi)
{
// The plan for readdir was to go through the fattable and search for an entry where the name == the name of the directory we are looking for. Once that block is found, we determine the size of the file. If it is only 1 block, we read the block in its entirety then return. If it is longer than 1 block, we continually read blocks pointed by the fattable until we reach an entry with a -2. Then we know this is the end of file. So we read that final block then return.	
	return 0;

}

static int xmp_mknod(const char *path, mode_t mode, dev_t rdev)
{
	int i,j;
//	void* file_block = calloc(sizeof(struct directory),1);
	int free_pos = -1;
	int fat_pos = -1;
	struct fatblock *fatblock_1 = calloc(sizeof(struct fatblock),1);//block for extending current directory
	struct fatblock *fatblock_2 = calloc(sizeof(struct fatblock),1);//block for creating new directory
	struct fatblock *fatblock_curr = calloc(sizeof(struct fatblock),1);//current block

	fseek(disk_fd, 512* curr_block, SEEK_SET);
	fread(&fatblock_curr, sizeof(struct fatblock),1, disk_fd);  //set current directory block
	 


	//find free space in FAT
	for(i=1; i < (super->k) + 1; i++){
		fat_pos = i;
		fseek(disk_fd, 512 * fat_pos, SEEK_SET);
	    fread(&fatblock_1, sizeof(struct fatblock), 1, disk_fd);  
		for (j=0; j< 128; j ++){
			free_pos = j;
			if (fatblock_1->fat_entry[free_pos] == 0){
				fatblock_1->fat_entry[free_pos] = -2;
				fatblock_curr->fat_entry[curr_block_pos] = (i-1)*128 + j;
				break;
	    	}
		}
	}
	int dir_first_fat_pos = -1;
	int dir_first_free_pos = -1;
	
	for(i=1; i < (super->k) + 1; i++) {
		dir_first_fat_pos = i;
		fseek(disk_fd, 512 * dir_first_fat_pos, SEEK_SET);
	    fread(&fatblock_2, sizeof(struct fatblock), 1, disk_fd); 
		for(j = 0; j < 128; j++) {
			dir_first_free_pos = j;
			if(fatblock_2->fat_entry[dir_first_free_pos] == 0){
				fatblock_2->fat_entry[dir_first_free_pos] = -2;
				break;
			}
		}
	}
	struct directory *old_dir = calloc(sizeof(struct directory),1);
	fseek(disk_fd,(super->k +1 * 512) + (((curr_block * 128) + curr_block_pos)*64), SEEK_SET);
	fread(&old_dir, sizeof(struct directory) , 1, disk_fd);//might have to memcpy new_dir to buffer first.
	old_dir->modification_time = time(NULL);
	old_dir->access_time = time(NULL);
	old_dir->file_length = old_dir -> file_length +64;
	old_dir->flag = 0;
	
	
	fseek(disk_fd,(super->k +1 * 512) + (((curr_block * 128) + curr_block_pos)*64), SEEK_SET);
	fwrite(old_dir, sizeof(struct directory) , 1, disk_fd);//might have to memcpy new_dir to buffer first.
	
	struct directory *new_dir = calloc(sizeof(struct directory),1);
	char* name = strrchr(path, '/');
	strcpy(new_dir->file_name, name+1);
	//new_dir->file_name = name;
	new_dir->creation_time = time(NULL);
	new_dir->modification_time = time(NULL);
	new_dir->access_time = time(NULL);
	new_dir->file_length = 64;
	new_dir->start_block = ((dir_first_fat_pos-1) * 128) + (dir_first_free_pos);
	new_dir->flag = 0;
	
	fseek(disk_fd,(super->k * 512) + (((fat_pos * 128) + free_pos)*64), SEEK_SET);
	fwrite(new_dir, sizeof(struct directory) , 1, disk_fd);//might have to memcpy new_dir to buffer first.
	
	
	fseek(disk_fd, 512* curr_block, SEEK_SET);
	fwrite(&fatblock_curr, sizeof(struct fatblock),1, disk_fd);  //set current directory block
	
	fseek(disk_fd, 512 * fat_pos, SEEK_SET);
	fwrite(&fatblock_1, sizeof(struct fatblock), 1, disk_fd);  
	
	fseek(disk_fd, 512 * dir_first_fat_pos, SEEK_SET);
	fwrite(&fatblock_2, sizeof(struct fatblock), 1, disk_fd);     
	

	//do stuff in newly formed directory block - does not write entries to self or parent yet
	return 0;
}
	
static int xmp_mkdir(const char *path, mode_t mode)
{
	int i,j;
//	void* file_block = calloc(sizeof(struct directory),1);
	int free_pos = -1;
	int fat_pos = -1;
	struct fatblock *fatblock_1 = calloc(sizeof(struct fatblock),1);//block for extending current directory
	struct fatblock *fatblock_2 = calloc(sizeof(struct fatblock),1);//block for creating new directory
	struct fatblock *fatblock_curr = calloc(sizeof(struct fatblock),1);//current block

	fseek(disk_fd, 512* curr_block, SEEK_SET);
	fread(&fatblock_curr, sizeof(struct fatblock),1, disk_fd);  //set current directory block
	 


	//find free space in FAT
	for(i=1; i < (super->k) + 1; i++){
		fat_pos = i;
		fseek(disk_fd, 512 * fat_pos, SEEK_SET);
	    fread(&fatblock_1, sizeof(struct fatblock), 1, disk_fd);  
		for (j=0; j< 128; j ++){
			free_pos = j;
			if (fatblock_1->fat_entry[free_pos] == 0){
				fatblock_1->fat_entry[free_pos] = -2;
				fatblock_curr->fat_entry[curr_block_pos] = (i-1)*128 + j;
				break;
	    	}
		}
	}
	int dir_first_fat_pos = -1;
	int dir_first_free_pos = -1;
	
	for(i=1; i < (super->k) + 1; i++) {
		dir_first_fat_pos = i;
		fseek(disk_fd, 512 * dir_first_fat_pos, SEEK_SET);
	    fread(&fatblock_2, sizeof(struct fatblock), 1, disk_fd); 
		for(j = 0; j < 128; j++) {
			dir_first_free_pos = j;
			if(fatblock_2->fat_entry[dir_first_free_pos] == 0){
				fatblock_2->fat_entry[dir_first_free_pos] = -2;
				break;
			}
		}
	}
	struct directory *old_dir = calloc(sizeof(struct directory),1);
	fseek(disk_fd,(super->k +1 * 512) + (((curr_block * 128) + curr_block_pos)*64), SEEK_SET);
	fread(&old_dir, sizeof(struct directory) , 1, disk_fd);//might have to memcpy new_dir to buffer first.
	old_dir->modification_time = time(NULL);
	old_dir->access_time = time(NULL);
	old_dir->file_length = old_dir -> file_length +64;
	old_dir->flag = 0;
	
	//struct directory *old_dir = calloc(sizeof(struct directory),1);
	fseek(disk_fd,(super->k +1 * 512) + (((curr_block * 128) + curr_block_pos)*64), SEEK_SET);
	fwrite(old_dir, sizeof(struct directory) , 1, disk_fd);// write back old directory
	
	//struct directory *old_dir = calloc(sizeof(struct directory),1);
	//char* name = strrchr(path, '/');
	struct directory *new_dir = calloc(sizeof(struct directory),1);
	char* name = strrchr(path, '/');
	strcpy(new_dir->file_name, name+1);
	//new_dir->file_name = name;
	new_dir->creation_time = time(NULL);
	new_dir->modification_time = time(NULL);
	new_dir->access_time = time(NULL);
	new_dir->file_length = 64;
	new_dir->start_block = ((dir_first_fat_pos-1) * 128) + (dir_first_free_pos);
	new_dir->flag = 1;
	
	fseek(disk_fd,(super->k * 512) + (((fat_pos * 128) + free_pos)*64), SEEK_SET);
	fwrite(new_dir, sizeof(struct directory) , 1, disk_fd);// write new directory
	
	
	fseek(disk_fd, 512* curr_block, SEEK_SET);
	fwrite(&fatblock_curr, sizeof(struct fatblock),1, disk_fd);  //set current directory block
	
	fseek(disk_fd, 512 * fat_pos, SEEK_SET);
	fwrite(&fatblock_1, sizeof(struct fatblock), 1, disk_fd);  //set second block of current directory
	
	fseek(disk_fd, 512 * dir_first_fat_pos, SEEK_SET);
	fwrite(&fatblock_2, sizeof(struct fatblock), 1, disk_fd);  //set fatblock of new directory   
	

	//do stuff in newly formed directory block - does not write entries to self or parent yet ( no . or .. entries. Redo process as above - including freeing blocks and Placing copies of the same old and new directory entries)
	return 0;

}

static int xmp_unlink(const char *path) //https://stackoverflow.com/questions/5467725/how-to-delete-a-directory-and-its-contents-in-posix-c
{
//	int res;

//	res = unlink(path);
//	if (res == -1)
//		return -errno;

//	return 0;

	int res;
	res = remove(path);
	if(res) {
		perror(path);
	}
	return 0;
}



static int xmp_rmdir(const char *path) // https://stackoverflow.com/questions/5467725/how-to-delete-a-directory-and-its-contents-in-posix-c
{
    //1.find start of directory to remove (see getattr), keeping previous entry recorded to write in fatblock
    //2.if exists, find next entry in fat_block sequence, and write it in prev block, else write -2.
    //3. set current fat_block entry to 0.
    //No need to remove any directory entries from data, since they will be overwritten with mkdir.


	return 0;
}

static int xmp_open(const char *path, struct fuse_file_info *fi)
{
//1. find directory as in get_attr, check that it is a file, not a directory by looking at flags
//2. set global curr_directory to block number of data of selected directory
//3. Possibly record FAT number of open file in queue.

	return 0;

}

static int xmp_read(const char *path, char *buf, size_t size, off_t offset,
		    struct fuse_file_info *fi)
{
    // 1. Check file pointer is set(set in open)
    // 2. Check file is readable
    // 3. Set size to file size
    // 3. Read file data block by data block, keeping track of position, until reached final block and outputted size%64 bytes

	size = 0;

	return size;

}

static int xmp_write(const char *path, const char *buf, size_t size,
		     off_t offset, struct fuse_file_info *fi)
{
    // Input data into file, splitting it and recording it in different blocks, creating new blocks as necessary
	return 0;
}

static struct fuse_operations xmp_oper = {	//don't need all operations, will comment out some
	.getattr	= xmp_getattr,
//	.access		= xmp_access,
//	.readlink	= xmp_readlink,
	.readdir	= xmp_readdir,	//read directory
	.mknod		= xmp_mknod,	//creates file
	.mkdir		= xmp_mkdir,	//creates directory
//	.symlink	= xmp_symlink,
//	.unlink		= xmp_unlink,	//deletes file
	.rmdir		= xmp_rmdir,	//deletes empty directory
//	.rename		= xmp_rename,
//	.link		= xmp_link,	//shares
//	.chmod		= xmp_chmod,
//	.chown		= xmp_chown,
//	.truncate	= xmp_truncate,
	.open		= xmp_open,	//file open
	.read		= xmp_read,	//file read
	.write		= xmp_write,	//file writes
//	.statfs		= xmp_statfs,	//file system stats
//	.release	= xmp_release,
//	.fsync		= xmp_fsync,
};

int main(int argc, char *argv[])
{
	if(argc < 1) {
		perror("File is NULL");
		return(-1);
	}

//	open("test", O_WRONLY | O_CREAT);

	return fuse_main(argc, argv, &xmp_oper, NULL);
}


