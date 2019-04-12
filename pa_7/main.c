/****************************************************************************
*                   mount_root Program                                      *
*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>

#include "type.h"
#include "global.h"

/******* WRITE YOUR OWN util.c and others ************/
#include "util.h"
#include "cd_ls_pwd.h"
/****************************************************/

int init();
int mount_root();
int print(MINODE *mip);
int quit();

int init()
{
    int i;
    MINODE *mip;
    PROC   *p;

    printf("init()\n");

    for (i=0; i<NMINODE; i++)
    {
        mip = &minode[i];
        // set all entries to 0;
        mip->dev = 0;
        mip->ino = 0;
        mip->refCount = 0;
        mip->dirty = 0;
        mip->mounted = 0;
        mip->mptr = 0;
    }

    for (i=0; i<NPROC; i++)
    {
        p = &proc[i];
        // set pid = i; uid = i; cwd = 0;
        p->pid = i+1;
        p->uid = i;
        p->status = 0;
        p->cwd = 0;
    }

    root = 0;
}

// load root INODE and set root pointer to it
int mount_root()
{  
    char buf[BLKSIZE];
    SUPER *sp;
    GD    *gp;

    printf("mount_root()\n");

    // read SUPER block
    get_block(fd, 1, buf);  
    sp = (SUPER *)buf;

    // check for EXT2 magic number:
    if (sp->s_magic != 0xEF53){
        printf("NOT an EXT2 FS\n");
        exit(1);
    }

    //record nblocks, ninodes as globals;
    nblocks = sp->s_blocks_count;
    ninodes = sp->s_inodes_count;

    //get GD0 in Block #2:
    get_block(fd, 2, buf);
    gp = (GD *)buf;
    
    //record bmap, imap, inodes_start as globals
    bmap = gp->bg_block_bitmap;
    imap = gp->bg_inode_bitmap;
    inode_start = gp->bg_inode_table;

    root = iget(dev, 2);       // get #2 INODE into minode[ ]
    printf("mounted root OK\n");
}

char *disk = "mydisk";
int main(int argc, char *argv[ ])
{
  int ino;
  char buf[BLKSIZE];
  if (argc > 1)
     disk = argv[1];

  fd = open(disk, O_RDWR);
  if (fd < 0){
     printf("open %s failed\n", disk);  
     exit(1);
  }
  dev = fd;

  init();  
  mount_root();
  printf("root refCount = %d\n", root->refCount);

  printf("creating P0 as running process\n");
  running = &proc[0];
  running->status = READY;
  running->cwd = iget(dev, 2);
  // set proc[1]'s cwd to root also
  //proc[1].cwd = iget(dev, 2);
  printf("root refCount = %d\n", root->refCount);

  while(1){
    printf("input command : [ls|cd|pwd|quit] ");
    fgets(line, 128, stdin);
    line[strlen(line)-1] = 0;

    if (line[0]==0)
       continue;
    pathname[0] = 0;

    sscanf(line, "%s %s", cmd, pathname);
    printf("cmd=%s pathname=%s\n", cmd, pathname);

    if (strcmp(cmd, "ls")==0)
       ls();

    if (strcmp(cmd, "cd")==0)
       chdir();

    if (strcmp(cmd, "pwd")==0)
       pwd(running->cwd);

    if (strcmp(cmd, "quit")==0)
       quit();
  }
}

int print(MINODE *mip)
{
    int blk;
    char dbuf[1024], temp[256];
    int i;

    ip = &mip->INODE;
    for (i=0; i < 12; i++){
        if (ip->i_block[i]==0)
            return 0;
        get_block(dev, ip->i_block[i], dbuf);

        dp = (DIR *)dbuf; 
        cp = dbuf;

        while(cp < dbuf+1024){
            // make dp->name a string in temp[ ]
            strncpy(temp, dp->name, dp->name_len);
            temp[dp->name_len] = 0;

            printf("%d %d %d %s\n", dp->inode, dp->rec_len, dp->name_len, temp);

            cp += dp->rec_len;
            dp = (DIR *)cp;
        }
    }
}
 
int quit()
{
  int i;
  MINODE *mip;
  for (i=0; i<NMINODE; i++){
    mip = &minode[i];
    if (mip->refCount > 0)
      iput(mip);
  }
  exit(0);
}
