// Anan Kraisakdawat
// 11503146
// Cpts 360 Lab 6

/******* balloc.c: allocate a free blocks, return its block number ******/
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>

// define shorter TYPES, save typing efforts
typedef struct ext2_group_desc  GD;
typedef struct ext2_super_block SUPER;
typedef struct ext2_inode       INODE;
typedef struct ext2_dir_entry_2 DIR;    // need this for new version of e2fs

#define BLKSIZE 1024

GD    *gp;
SUPER *sp;
INODE *ip;
DIR   *dp; 

/********** globals *************/
int fd;
int imap, bmap;  // IMAP and BMAP block number
int ninodes, nblocks, nfreeInodes, nfreeBlocks;
char *disk = "mydisk";

int get_block(int fd, int blk, char buf[ ])
{
    lseek(fd, (long)blk*BLKSIZE, 0);
    read(fd, buf, BLKSIZE);
}

int put_block(int fd, int blk, char buf[ ])
{
    lseek(fd, (long)blk*BLKSIZE, 0);
    write(fd, buf, BLKSIZE);
}

int tst_bit(char *buf, int bit)
{
    int i, j;
    i = bit/8; j=bit%8;
    if (buf[i] & (1 << j))
        return 1;
    return 0;
}

int set_bit(char *buf, int bit)
{
    int i, j;
    i = bit/8; j=bit%8;
    buf[i] |= (1 << j);
}

int clr_bit(char *buf, int bit)
{
    int i, j;
    i = bit/8; j=bit%8;
    buf[i] &= ~(1 << j);
}

int decFreeBlocks(int dev)
{
    char buf[BLKSIZE];

    // dec free blocks count in SUPER and GD
    get_block(dev, 1, buf);
    sp = (SUPER *)buf;
    sp->s_free_blocks_count--;
    put_block(dev, 1, buf);

    get_block(dev, 2, buf);
    gp = (GD *)buf;
    gp->bg_free_blocks_count--;
    put_block(dev, 2, buf);
}

int incFreeBlocks(int dev)
{
    char buf[BLKSIZE];

    // inc free blocks count in SUPER and GD
    get_block(dev, 1, buf);
    sp = (SUPER *)buf;
    sp->s_free_blocks_count++;
    put_block(dev, 1, buf);

    get_block(dev, 2, buf);
    gp = (GD *)buf;
    gp->bg_free_blocks_count++;
    put_block(dev, 2, buf);

}

// allocate 5 blocks
int balloc(int dev)
{
    int  i;
    char buf[BLKSIZE];

    // read bitmap block
    get_block(dev, bmap, buf);

    for (i=0; i < nblocks; i++){
        if (tst_bit(buf, i)==0){
        set_bit(buf,i);
        decFreeBlocks(dev);

        put_block(dev, bmap, buf);

        return i+1;
        }
    }
    printf("balloc(): no more free blocks\n");
    return 0;
}

// deallocates a block number, bno
int bdealloc(int dev, int bno)
{
    int i;
    char buf[BLKSIZE];
    
    // read block_bitmap
    get_block(dev, bmap, buf);

    clr_bit(buf, bno - 1);
    incFreeBlocks(dev);
    
    put_block(dev, bmap, buf);
}

int main(int argc, char *argv[ ])
{
    int i, blk;
    char buf[BLKSIZE];

    if (argc > 1)
        disk = argv[1];

    fd = open(disk, O_RDWR);
    if (fd < 0){
        printf("open %s failed\n", disk);
        exit(1);
    }

    // read SUPER block
    get_block(fd, 1, buf);
    sp = (SUPER *)buf;

    ninodes = sp->s_inodes_count;
    nblocks = sp->s_blocks_count;
    nfreeInodes = sp->s_free_inodes_count;
    nfreeBlocks = sp->s_free_blocks_count;
    printf("ninodes=%d nblocks=%d nfreeInodes=%d nfreeBlocks=%d\n", 
	    ninodes, nblocks, nfreeInodes, nfreeBlocks);

    // read Group Descriptor 0
    get_block(fd, 2, buf);
    gp = (GD *)buf;

    bmap = gp->bg_block_bitmap;
    printf("bmap = %d\n", bmap);
    getchar();

    for (i=0; i < 5; i++){  
        blk = balloc(fd);
        printf("allocated blk = %d\n", blk);
    }
}