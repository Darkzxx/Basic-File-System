/************** util.h file ****************/
#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>

#include "type.h"
#include "global.h"

int get_block(int dev, int blk, char *buf);         // read from a block
int put_block(int dev, int blk, char *buf);         // write to a block
int tst_bit(char *buf, int bit);                    // test bit
int set_bit(char *buf, int bit);                    // set bit = 1
int clr_bit(char *buf, int bit);                    // clear bit = 0
int tokenize(char *pathname);                       // tokenize pathname into n components: name[0] to name[n-1]
MINODE *iget(int dev, int ino);                     // return minode pointer to loaded INODE
void iput(MINODE *mip);                             // save minode data to disk
int search(MINODE *mip, char *name);                // search a DIRectory INODE for entry with a given name
int getino(char *pname);                            // return inode number of pathname
int decFreeInodes(int dev);                         // decrease free inodes count
int incFreeInodes(int dev);                         // increase free inodes count
int decFreeBlocks(int dev);                         // decrease free block count
int incFreeBlocks(int dev);                         // increase free block count
int ialloc(int dev);                                // allocate inode
int idealloc(int dev, int ino);                     // deallocates by inode number, ino
int balloc(int dev);                                // allocate block
int bdealloc(int dev, int bno);                     // deallocates a block number, bno

// THESE two functions are for pwd(running->cwd), which prints the absolute
// pathname of CWD.
int findmyname(MINODE *parent, u32 myino, char *myname);        
int findino(MINODE *mip, u32 *myino);

#endif