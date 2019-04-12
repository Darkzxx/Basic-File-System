// Anan Kraisakdawat
// 11503146
// Cpts 360 Lab 6

/********* main.c code ***************/

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <math.h>
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
char *cp; 

#define BLKSIZE 1024

int fd;
int iblock;

int get_block(int nd, int blk, char buf[ ])
{
    lseek(nd, (long)blk*BLKSIZE, 0);
    read(nd, buf, BLKSIZE);
}

int print_blk_numbers(int blk, int blk_left, int recurse)
{
    int i = 0, left = blk_left;
    char dbuf[BLKSIZE];

    get_block(fd, blk, dbuf);
    cp = dbuf;
    dp = (DIR *)dbuf;

     while(i < 256 && left > 0)
    {
        printf("%4d ", dp->inode);

        if(recurse > 0)
        {
            printf("\n==========================\n");
            left = print_blk_numbers(dp->inode, left, recurse-1);
            left++;

            get_block(fd, blk, dbuf);
            cp = dbuf + i*4;
            dp = (DIR *)cp;
        }
            
        cp += 4;
        dp = (DIR *)cp;

        if(i % 10 == 9)
            printf("\n");
        i++;
        left--;
    }
    printf("\n");
    printf("************************\n");
    return left;
}

int showblock(char *pname[], int n)
{
    char buf[BLKSIZE];
    char dbuf[BLKSIZE], temp[256];
    int i, blk_total, blk_left;
    int ino, blk, offset;
    u32 *p;

    // read SUPER block
    get_block(fd, 1, buf);  
    sp = (SUPER *)buf;

    // check for EXT2 magic number:
    if (sp->s_magic != 0xEF53){
        printf("NOT an EXT2 FS\n");
        exit(1);
    }

    // read GD
    get_block(fd, 2, buf);
    gp = (GD *)buf;
    iblock = gp->bg_inode_table;   // get inode start block

    // get inode start block     
    get_block(fd, iblock, buf);

    ip = (INODE *)buf + 1;         // ip points at 2nd INODE (root inode)
  
    for(i = 0; i < n; i++)
    {
        ino = search(ip, pname[i]);
        if(ino == 0)
        {
            printf("Can't find %s\n", pname[i]);
            exit(1);
        }

        blk = (ino - 1) / 8 + iblock;   // disk block contain this INODE 
        offset = (ino - 1) % 8;         // offset of INODE in this block
        get_block(fd, blk, dbuf);
        ip = (INODE *)dbuf + offset;    // ip -> new INODE
    }

    // number of blocks
    blk_total = (ip->i_size + BLKSIZE -1)/BLKSIZE;
    printf("blocks = %d\n", blk_total);

    // extract information from ip
    printf("******** DISK BLOCKS ********\n");
    // there are 15 i_blocks
    for(i = 0; i < 15; i++)
    {
        if(ip->i_block[i] == 0)
            break;

        printf("i_block[%d] = %d = %d\n", i, ip->i_block[i], &ip->i_block[i]);
    }

    // extract information from ip
    printf("******** Direct Block Numbers ********\n");
    // there are 12 direct i_blocks
    for(i = 0; i < 12; i++)
    {
        if(ip->i_block[i] == 0)
            break;

        printf("%d ", ip->i_block[i]);
        if(i == 9)
            printf("\n");
    }
    printf("\n");

    // number of block left after direct i_blocks
    blk_left = blk_total - i;

    while(blk_left > 0)
    {
        printf("blocks remain = %d\n", blk_left);
        i = 0;

        // indirect block numbers case
        if(blk_total - blk_left < 12 + 256)
        {
            printf("******** Indirect Block Numbers ********\n");

            blk_left = print_blk_numbers(ip->i_block[12], blk_left, 0);
        }
        // double-indirect case
        else if((blk_total-blk_left >= 12+256) && (blk_total-blk_left < 12+256+pow(256,2)))
        {
            printf("******** Double-indirect Block Numbers ********\n");

            blk_left = print_blk_numbers(ip->i_block[13], blk_left, 1);
        }
        // triple-indirect case
        else if(blk_total-blk_left >= 12+256+pow(256,2))
        {
            printf("******** Triple-indirect Block Numbers ********\n");

            blk_left = print_blk_numbers(ip->i_block[14], blk_left, 2);
        }

    }
    printf("blocks remain = %d\n", blk_left);
    
}

int search(INODE *ip, char *name)
{
    char dbuf[BLKSIZE];
    int i;

    // loop through all direct i_block[i] for current inode ip
    for (i = 0; i < 12; i++)
    {
        if(ip->i_block[i] == 0)
            break;

        get_block(fd, ip->i_block[i], dbuf);
        cp = dbuf;
        dp = (DIR *)dbuf;

        while(cp < dbuf + BLKSIZE)
        {
            if(!strcmp(name, dp->name))
            {
                return dp->inode;
            }

            cp += dp->rec_len;
            dp = (DIR *)cp;
        }
    }

    return 0; 
}

int main(int argc, char *argv[ ])
{ 
    int n = 0, d = 0;
    char *disk, *token;
    char *pname[64], pathname[256];

    //initialize pname
    while(d < 64)
    {
        pname[d] = 0;
        d++;
    }
    d = 0;

    // check amount of input
    if (argc < 2)
    {
         printf("usage showblock <Device> <Pathname>\n");
         exit(1);
    }

    // open disk for read
    disk = argv[1];
    fd = open(disk, O_RDONLY);
    if (fd < 0){
        printf("open showblock failed: Device not found\n");
        exit(1);
    }

    // tokenize pathname
    strcpy(pathname, argv[2]);
    token = strtok(pathname, "/");

    // get pathname token into pname
    while(token)
    {
        pname[n] = token;
        token = strtok(0, "/");
        n++;
    }

    // print n and all pathname
    printf("n = %d", n);
    while(d < n)
    {
        printf("  %s", pname[d]);
        d++;
    } 
    printf("\n");

    // print super block info
    showblock(pname, n);

    return 0;
}
