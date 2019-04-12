/************** stat_chmod.c file ****************/
#include "stat_chmod.h"

// touch filename
int my_touch()
{
    int ino;
    MINODE *mip;

    // check pathname exists
    if(pathname[0] == 0)
    {
        printf("Error: command %s need a pathname\n", cmd);
        return -1;
    }

    // update dev on given pathname
    if(pathname[0] == '/')
        dev = root->dev;
    else
        dev = running->cwd->dev;

    // get INODE of filename into memory
    ino = getino(pathname);

    // if pathname not exist
    if(ino == 0)
        creat_file();
    else
    {
        mip = iget(dev, ino);
        mip->INODE.i_atime = time(0L);
        mip->INODE.i_mtime = time(0L);
        mip->INODE.i_ctime = time(0L);
        mip->dirty = 1;
        iput(mip);
    }
    printf("touch completed\n");
}

//  stat filename
int my_stat()
{
    struct stat myst;
    int ino;
    MINODE *mip;
    char fileType[16];

    // check pathname exists
    if(pathname[0] == 0)
    {
        printf("Error: command %s need a pathname\n", cmd);
        return -1;
    }

    // update dev on given pathname
    if(pathname[0] == '/')
        dev = root->dev;
    else
        dev = running->cwd->dev;

    // get INODE of filename into memory
    ino = getino(pathname);

    // if pathname not exist
    if(ino == 0)
        return -1;

    mip = iget(dev, ino);

    // copy dev, ino to myst.st_dev, myst.st_ino; 
    myst.st_dev = mip->dev;
    myst.st_ino = mip->ino;

    printf("ino = %d\n", myst.st_ino);
    printf("nlinks = %d\n", mip->INODE.i_links_count);

    // copy mip->INODE fields to myst fields;
    myst.st_mode = mip->INODE.i_mode;
    myst.st_nlink = mip->INODE.i_links_count;
    myst.st_uid = mip->INODE.i_uid;
    myst.st_gid = mip->INODE.i_gid;
    myst.st_size = mip->INODE.i_size;
    myst.st_blksize = BLKSIZE;
    myst.st_blocks = mip->INODE.i_blocks;
    myst.st_atime = mip->INODE.i_atime;
    myst.st_mtime = mip->INODE.i_mtime;

    if(S_ISREG(myst.st_mode))
        strcpy(fileType, "regular file");
    if(S_ISDIR(myst.st_mode))
        strcpy(fileType, "directory");
    if(S_ISLNK(myst.st_mode))
        strcpy(fileType, "link file");

    printf("File: %s\n", basename(pathname));
    printf("Size: %d\t Blocks: %d\t IO Block: %d\t %s\n", myst.st_size, myst.st_blocks, myst.st_blksize, fileType);
    printf("Device: %d\t", myst.st_dev);
    printf("Inode: %d\t Links: %d\n", ino, myst.st_nlink);
    printf("Uid: %d\t Gid: %d\n", myst.st_uid, myst.st_gid);
    printf("Access: %s", ctime(&myst.st_atime));
    printf("Modify: %s\n", ctime(&myst.st_mtime));

    iput(mip);
}

// change file permission
int my_chmod()
{
    int ino;
    MINODE *mip;
    long int newMode;

    // check pathname is given
    if(pathname[0] == 0 || pathname1[0] == 0)
    {
        printf("Error: command %s need 2 inputs\n", cmd);
        return -1;
    }

    // update dev on given pathname
    if(pathname1[0] == '/')
        dev = root->dev;
    else
        dev = running->cwd->dev;

    ino = getino(pathname1);

    // if pathname not exist
    if(ino == 0)
        return -1;

    mip = iget(dev, ino);

    //convert string input to octal
    newMode = strtol(pathname, NULL, 8);
    printf("new_mode = %ld\n", newMode);
    // change file permission
    mip->INODE.i_mode = (mip->INODE.i_mode & 0xF000) | newMode;

    mip->dirty = 1;
    iput(mip);
}

