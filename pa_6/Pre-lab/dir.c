// Anan Kraisakdawat
// 11503146
// Cpts 360 Lab 6

/******************** dir.c ******************/

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>

#define BLKSIZE 1024

// define shorter TYPES, save typing efforts
typedef struct ext2_group_desc  GD;
typedef struct ext2_super_block SUPER;
typedef struct ext2_inode       INODE;
typedef struct ext2_dir_entry_2 DIR;    // need this for new version of e2fs

GD    *gp;
SUPER *sp;
INODE *ip;
DIR   *dp; 
char *cp;

int fd;
int iblock;

int get_block(int fd, int blk, char buf[ ])
{
    lseek(fd,(long)blk*BLKSIZE, 0);
    read(fd, buf, BLKSIZE);
}

int dir()
{
    char buf[BLKSIZE];
    char dbuf[BLKSIZE];
    int dir_count = 0;

    // read GD
    get_block(fd, 2, buf);
    gp = (GD *)buf;
    /****************
    printf("%8d %8d %8d %8d %8d %8d\n",
	    gp->bg_block_bitmap,
	    gp->bg_inode_bitmap,
	    gp->bg_inode_table,
	    gp->bg_free_blocks_count,
	    gp->bg_free_inodes_count,
	    gp->bg_used_dirs_count);
    ****************/ 
    iblock = gp->bg_inode_table;   // get inode start block#
    printf("inode_block=%d\n", iblock);

    // get inode start block     
    get_block(fd, iblock, buf);

    ip = (INODE *)buf + 1;         // ip points at 2nd INODE
  
    printf("i_block[0]=%d\n", ip->i_block[0]);

    get_block(fd, ip->i_block[0], dbuf);

    cp = (char *)dbuf;
    dp = (DIR *)dbuf;
    while(((int)(dp->file_type)) > 0)
    {
        printf("inode = %d\n", dp->inode);
        printf("rec_len = %d\n", dp->rec_len);
        printf("name_len = %d\n", dp->name_len);
        printf("file_type = %d\n", dp->file_type);
        printf("name = %s\n\n", dp->name);

        cp += dp->rec_len;
        dp = (DIR *)cp;
    }

    printf("print search result of ip(file4): %d\n", search(ip, "file4"));
    printf("print search result of ip(nofile): %d\n", search(ip, "nofile"));

    /*****************************
    u16  i_mode;        // same as st_imode in stat() syscall
    u16  i_uid;                       // ownerID
    u32  i_size;                      // file size in bytes
    u32  i_atime;                     // time fields  
    u32  i_ctime;
    u32  i_mtime;
    u32  i_dtime;
    u16  i_gid;                       // groupID
    u16  i_links_count;               // link count
    u32  i_blocks;                    // IGNORE
    u32  i_flags;                     // IGNORE
    u32  i_reserved1;                 // IGNORE
    u32  i_block[15];                 // IMPORTANT, but later
    ***************************/
}

int search(INODE *ip, char *name)
{
    char dbuf[BLKSIZE];
    get_block(fd, ip->i_block[0], dbuf);

    cp = (char *)dbuf;
    dp = (DIR *)dbuf;

    while(((int)(dp->file_type)) > 0)
    {
      if(!strcmp(name, dp->name))
      {
          return dp->inode;
      }

      cp += dp->rec_len;
      dp = (DIR *)cp;
    }

    return 0; 
}

char *disk = "mydisk";
int main(int argc, char *argv[])
{ 
    if (argc > 1)
        disk = argv[1];

    fd = open(disk, O_RDONLY);
    if (fd < 0){
        printf("open %s failed\n", disk);
        exit(1);
    }

    dir();
}