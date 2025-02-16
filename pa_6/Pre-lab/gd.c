// Anan Kraisakdawat
// 11503146
// Cpts 360 Lab 6

/*************** gd.c code ***************/

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;

// define shorter TYPES, save typing efforts
typedef struct ext2_group_desc  GD;
typedef struct ext2_super_block SUPER;
typedef struct ext2_inode       INODE;
typedef struct ext2_dir_entry_2 DIR;    // need this for new version of e2fs

GD    *gp;
SUPER *sp;
INODE *ip;
DIR   *dp; 

#define BLKSIZE 1024

/******************* in <ext2fs/ext2_fs.h>*******************************
struct ext2_group_desc
{
  u32  bg_block_bitmap;          // Bmap block number
  u32  bg_inode_bitmap;          // Imap block number
  u32  bg_inode_table;           // Inodes begin block number
  u16  bg_free_blocks_count;     // THESE are OBVIOUS
  u16  bg_free_inodes_count;
  u16  bg_used_dirs_count;        

  u16  bg_pad;                   // ignore these 
  u32  bg_reserved[3];
};
**********************************************************************/

char buf[BLKSIZE];
int fd;

int get_block(int fd, int blk, char buf[ ])
{
    lseek(fd, (long)blk*BLKSIZE, 0);
    read(fd, buf, BLKSIZE);
}

int gd()
{
    // get s_first_data_block
    get_block(fd, 1, buf);
    sp = (SUPER *)buf;

    // check for EXT2 magic number:

    printf("s_magic = %x\n", sp->s_magic);
    if (sp->s_magic != 0xEF53){
        printf("NOT an EXT2 FS\n");
        exit(1);
    }
    printf("EXT2 FS OK\n");

    // read gd block
    get_block(fd, sp->s_first_data_block + 1, buf);  
    gp = (GD *)buf;

    printf("bg_block_bitmap = %d\n", gp->bg_block_bitmap);
    printf("bg_inode_bitmap = %d\n", gp->bg_inode_bitmap);
    printf("bg_inode_table = %d\n", gp->bg_inode_table);

    printf("bg_free_blocks_count = %d\n", gp->bg_free_blocks_count);
    printf("bg_free_inodes_count = %d\n", gp->bg_free_inodes_count);
    printf("bg_used_dirs_count = %d\n", gp->bg_used_dirs_count);
}

char *disk = "mydisk";

int main(int argc, char *argv[ ])
{ 
    if (argc > 1)
        disk = argv[1];
    fd = open(disk, O_RDONLY);
    if (fd < 0){
        printf("open failed\n");
        exit(1);
    }
    gd();
}
