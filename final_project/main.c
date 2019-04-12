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
#include "util.h"
/************ level 1 ************/
#include "cd_ls_pwd.h"
#include "mkdir_creat_rmdir.h"
#include "link_unlink_symlink.h"
#include "stat_chmod.h"
/************ level 2 ************/
#include "open_close_lseek.h"
/*********************************/

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

    for (i=0; i<NOFT; i++)
    {
        oft[i].mode = 0;
        oft[i].refCount = 0;
        oft[i].offset = 0;
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
  char *token;

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
    printf("-------------------- input command ---------------------\n");
    printf(" [ls|cd|pwd|mkdir|creat|rmdir|link|unlink|symlink|quit] \n");
    printf("                   [stat|chmod|touch]                   \n");
    printf("      [open|close|lseek|pfd|write|read|cat|cp|mv]       \n");
    printf("--------------------------------------------------------\n");
    printf(">> ");
    fgets(line, 128, stdin);
    line[strlen(line)-1] = 0;

    if (line[0]==0)
       continue;
    pathname[0] = 0;
    pathname1[0] = 0;

    sscanf(line, "%s %s %s", cmd, pathname, pathname1);

    if(pathname1[0] == 0)
        printf("pathname1[0] = 0\n");

    token = strtok(line, " ");
    strcpy(cmd, token);
    token = strtok(NULL, " ");
    if(token != 0)
    {
        strcpy(pathname, token);
        token = strtok(NULL, "\0");
        if(token != 0)
            strcpy(pathname1, token);
    }

    printf("cmd=%s pathname=%s pathname1=%s\n", cmd, pathname, pathname1);

    if (strcmp(cmd, "ls")==0)
        ls();

    if (strcmp(cmd, "cd")==0)
        ch_dir();

    if (strcmp(cmd, "pwd")==0)
        pwd(running->cwd);

    if (strcmp(cmd, "mkdir")==0)
        make_dir();
    
    if(strcmp(cmd, "creat")==0)
        creat_file();

    if(strcmp(cmd, "rmdir")==0)
        rm_dir();

    if(strcmp(cmd, "stat")==0)
        my_stat();

    if(strcmp(cmd, "chmod")==0)
        my_chmod();

    if(strcmp(cmd, "touch")==0)
        my_touch();

    if(strcmp(cmd, "link")==0)
        my_link();

    if(strcmp(cmd, "unlink")==0)
        my_unlink();

    if(strcmp(cmd, "symlink")==0)
        my_symlink();

    if(strcmp(cmd, "open")==0)
        open_file();
    
    if(strcmp(cmd, "close")==0) 
        close_file();

    if(strcmp(cmd, "lseek")==0)
        my_lseek();
    
    if(strcmp(cmd, "pfd")==0)
        pfd();

    if(strcmp(cmd, "write")==0)
        write_file();

    if(strcmp(cmd, "read")==0)
        read_file();

    if(strcmp(cmd, "cat")==0)
        my_cat();
    
    if(strcmp(cmd, "cp")==0)
        my_cp();

    if(strcmp(cmd, "mv")==0)
        my_mv();

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
