#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>

typedef struct ext2_group_desc  GD;
typedef struct ext2_super_block SUPER;
typedef struct ext2_inode       INODE;
typedef struct ext2_dir_entry_2 DIR;    // need this for new version of e2fs

#define BLKSIZE 1024
#define NAME_LEN 256

SUPER *sp;
GD    *gp;
INODE *ip;
DIR   *dp; 

int fd, n;
char *name[64], 			//pathname tokens
		 buf[BLKSIZE],		//block buffer, inode buffer
		 dbuf[BLKSIZE], 	//dir block buffer
		 sbuf[NAME_LEN],  	//dir entry name buffer
		 *cp;

void get_block(int fd, int blk, char buf[]) {
	lseek(fd, (long)blk * BLKSIZE, 0);
	read(fd, buf, BLKSIZE);
}

int is_ext2() {
	get_block(fd, 1, buf);
	sp = (SUPER *)buf;
	if (sp->s_magic != 0xEF53) {
		return 0;
	} 
	return 1;
}

void print_super() {
	printf("s_inodes_count = %d\n", sp->s_inodes_count);
  printf("s_blocks_count = %d\n", sp->s_blocks_count);
  printf("s_free_inodes_count = %d\n", sp->s_free_inodes_count);
  printf("s_free_blocks_count = %d\n", sp->s_free_blocks_count);
  printf("s_first_data_blcok = %d\n", sp->s_first_data_block);
  printf("s_log_block_size = %d\n", sp->s_log_block_size);
  printf("s_blocks_per_group = %d\n", sp->s_blocks_per_group);
  printf("s_inodes_per_group = %d\n", sp->s_inodes_per_group);
  printf("s_mnt_count = %d\n", sp->s_mnt_count);
  printf("s_max_mnt_count = %d\n", sp->s_max_mnt_count);
  printf("s_magic = %x\n", sp->s_magic);
  printf("s_mtime = %s", ctime(&sp->s_mtime));
  printf("s_wtime = %s", ctime(&sp->s_wtime));
}

void print_gd() {
	printf("bg_block_bitmap = %d\n", gp->bg_block_bitmap );
  printf("bg_inode_bitmap = %d\n", gp->bg_inode_bitmap);
  printf("bg_inode_table = %d\n", gp->bg_inode_table);
  printf("bg_free_blocks_count = %d\n", gp->bg_free_blocks_count);
  printf("bg_free_inodes_count = %d\n", gp->bg_free_inodes_count);
  printf("bg_used_dirs_count = %d\n", gp->bg_used_dirs_count);
  printf("bg_flags = %d\n", gp->bg_flags);
  printf("bg_exclude_bitmap_lo = %d\n", gp->bg_exclude_bitmap_lo);
  printf("bg_block_bitmap_csum_lo = %d\n", gp->bg_block_bitmap_csum_lo);
  printf("bg_inode_bitmap_csum_lo = %d\n", gp->bg_inode_bitmap_csum_lo);
  printf("bg_itable_unused = %d\n", gp->bg_itable_unused);
  printf("bg_checksum = %d\n", gp->bg_checksum);
}

void print_inode() {
	printf("i_mode = %4x\n", ip->i_mode);
  printf("i_uid = %d\n", ip->i_uid);
  printf("i_gid = %d\n", ip->i_gid);
  printf("i_size = %d\n", ip->i_size);
  printf("i_time = %s", ctime(&ip->i_ctime));
  printf("i_link = %d\n", ip->i_links_count);
  printf("i_block[0] = %d\n", ip->i_block[0]);
}

void print_dir() {
	get_block(fd, ip->i_block[0], dbuf);
	dp = (DIR *)dbuf;
  cp = dbuf;
  printf("inode  rec_len  name_len name\n");
  while (cp < &dbuf[BLKSIZE]) {
    strncpy(sbuf, dp->name, dp->name_len);	//get file name
    sbuf[dp->name_len] = '\0';    					//add null
    printf("%4d\t%4d\t%4d\t  %s\n", dp->inode, dp->rec_len, dp->name_len, dp->name);
    cp += dp->rec_len;
    dp = (DIR *)cp;
  }
}

void parse_path(char *path) {
	int i = 0;
	char *token = NULL;
	printf("Path = %s\n", path);

	token = strtok(path, "/");

	if (token == NULL) {
		printf("n = 0\n");
		return NULL;
	}

	do {
		name[i] = malloc(strlen(token) + 1);
		strcpy(name[i], token);
		printf("name[%d] = %s\n", i, name[i]);
		i++;
	} while (token = strtok(NULL, "/"));
	n = i;
	printf("n = %d\n", n);
}

int is_dir(INODE *iptr) {
	//get the leading 4 bits: 0100 = DIR, 0100 = REG file
  unsigned short imode = iptr->i_mode;
 	unsigned short first_four = (imode & 0xF000) >> 12; //read the first four and remove the following 12
  printf("iptr->i_mode = %x\n", iptr->i_mode);
  printf("imode = %x\n", imode);
 	printf("imode = %x\n", first_four);

  if(first_four == 4) {	//is DIR
   		return 1;
  }
  return 0;
}

int search_dir(INODE *iptr, char *name) {
	//print i_block[i]
	int i = 0;
	while(i < 15) {
		printf("i_block[%d] = %d\n", i,iptr->i_block[i]);
		i++;
	}

	get_block(fd, iptr->i_block[0], dbuf);
  dp = (DIR *)dbuf;
  cp = dbuf;

  while (cp < &dbuf[BLKSIZE]) {
    strncpy(sbuf, dp->name, dp->name_len);	//get file name
    sbuf[dp->name_len] = '\0';    					//add null
    //compare name
    if (strncmp(sbuf, name, dp->name_len) == 0) { //name matched
    	printf("### Found \"%s\" ###\n", name);
    	return dp->inode;											//return ino of found name
    }
    //advance to the next DIR in the same level 
    cp += dp->rec_len;	
    dp = (DIR *)cp;
  }
  return 0;
}

int main(int argc, char *argv[]) {

	if (argc < 3) {
		printf("Error: invalid number of arguments\n");
		printf("Usage Example: showblock.bin <device name> <path>\n");
		exit(0);
	}
	fd = open(argv[1], O_RDWR);
	if (fd < 0) {
		printf("Error: could not open file \"%s\"\n", argv[1]);
		exit(0);
	}
	if (is_ext2(fd) == 0) {
		printf("\"%s\"\n is not EXT2.", argv[1]);
		exit(0);
	}

	get_block(fd, 1, buf);
	sp = (SUPER *)buf;
	printf("----------------- SUPER -----------------\n");
	print_super();

	get_block(fd, 2, buf);
	gp = (GD *)buf;
	printf("------------------- GD -------------------\n");
	print_gd();

	int inode_begin_block = gp->bg_inode_table;
	get_block(fd, inode_begin_block, buf);
	ip = (INODE *)buf + 1;
	printf("--------------- Root INODE ---------------\n");
	print_inode();

	get_block(fd, ip->i_block[0], dbuf);
  dp = (DIR *)dbuf;

	printf("--------------- Parse Path ---------------\n");
	parse_path(argv[2]);

	printf("--- Directory of \"/\" ---\n");
  print_dir();

	int i = 0, blockno, ino, inumber;
	
	/* search and print dir with specified path*/
	for (i = 0; i < n; i++) {
		printf("----------- Searching for \"%s\" ----------\n", name[i]);
		printf("Press any key to continue the search\n");
  	getchar();
  	/* get inode number */ 
		inumber = search_dir(ip, name[i]);		
		if (inumber == 0) {
			printf("\"%s\" does not exist.\n", name[i]);
			exit(0);
		}

		/* go to deeper level */
    blockno = inode_begin_block + ((inumber-1) / 8);  //calculate position of deeper INODE
  	ino = (inumber-1) % 8;
  	printf("inode_begin_block = %d\n", inode_begin_block);
  	printf("inumber = %d\n", inumber);
  	printf("blockno = %d\n", blockno);
  	printf("ino = %d\n", ino);
  	get_block(fd, blockno, buf);		//read INODE of found name
  	ip = (INODE *)buf + ino;				//ip now points at that INODE
  	
  	if (is_dir(ip) == 0) { 					//check if it is a directory 
  		printf("Error: \"%s\" is not a directory.\n", name[i]);
  		exit(0);
  	}

  	printf("--- Directory of \"%s\" ---\n", name[i]);
		print_dir();
	}

	printf("------------------ Done ------------------\n");
	return 0;
}