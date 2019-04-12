// Anan Kraisakdawat
// 11503146
// Cpts 360 Lab 7

#ifndef TYPE_H
#define TYPE_H

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#include <ext2fs/ext2_fs.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>


/*************** type.h file *********************************/
typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;

typedef struct ext2_super_block SUPER;
typedef struct ext2_group_desc  GD;
typedef struct ext2_inode       INODE;
typedef struct ext2_dir_entry_2 DIR;  

SUPER *sp;
GD    *gp;
INODE *ip;
DIR   *dp;
char *cp;   

#define FREE        0
#define READY       1

#define BLKSIZE  1024

#define NMINODE    64
#define NOFT       64
#define NFD        10
#define NMOUNT      4
#define NPROC       2

typedef struct minode{
  INODE INODE;
  int dev;
  int ino;
  int refCount;
  int dirty;
  int mounted;
  struct mntable *mptr;
}MINODE;

typedef struct oft{
  int  mode;
  int  refCount;
  MINODE *mptr;
  int  offset;
}OFT;

typedef struct proc{
  struct proc *next;
  int pid;
  int ppid;
  int status;
  int uid;
  int gid;
  MINODE *cwd;
  OFT *fd[NFD];
}PROC;

#endif