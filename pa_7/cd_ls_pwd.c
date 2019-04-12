/************** cd_ls_pwd.c file ****************/
#include "cd_ls_pwd.h"

void ls()
{
    int inod, i;
    MINODE *mip;
    char dbuf[BLKSIZE], nbuf[256];

    // update dev on given pathname
    if(pathname[0] == '/')
        dev = root->dev;
    else
        dev = running->cwd->dev;

    // get inod depending on if pathname is given
    if(pathname[0] == 0)
        inod = running->cwd->ino;
    else
        inod = getino(pathname);
        
    // if pathname not exist
    if(inod == 0)
        return;

    mip = iget(dev, inod);

    // check if it's a dir
    if(!S_ISDIR(mip->INODE.i_mode))
    {
        printf("%s is not a directory\n", basename(pathname));
        return;
    }

    for(i = 0; i < 12; i++)
    {
        if(mip->INODE.i_block[i] == 0)
            return;

        get_block(mip->dev, mip->INODE.i_block[i], dbuf);

        cp = dbuf;
        dp = (DIR *)dbuf;

        while(cp < dbuf + BLKSIZE)
        {
            strncpy(nbuf, dp->name, dp->name_len);
            nbuf[dp->name_len] = 0;

            ls_file(nbuf, dp->inode);

            cp += dp->rec_len;
            dp = (DIR *)cp;
        }
    }
}

void ls_file(char *name, int inod)
{
    MINODE *mip;
    int i;
    char time[64], temp[64];
    char *c = "xwrxwrxwr";

    mip = iget(dev, inod);

    // print type
    if(S_ISREG(mip->INODE.i_mode))
        printf("%c", '-');
    if(S_ISDIR(mip->INODE.i_mode))
        printf("%c", 'd');
    if(S_ISLNK(mip->INODE.i_mode))
        printf("%c", 'l');

    // print permissions
    for(i = 8; i >= 0; i--)
    {
        if(mip->INODE.i_mode & (1 << i))
            printf("%c", c[i]);
        else   
            printf("%c", '-');
    }

    strcpy(temp, ctime(&mip->INODE.i_ctime));
    strncpy(time, &temp[4], 12);
    time[12] = 0;

    // print other info
    printf(" %2d %2d %2d  %s %5d  %s",
        mip->INODE.i_links_count, mip->INODE.i_gid,
        mip->INODE.i_uid, time,
        mip->INODE.i_size, name);

    printf("\n");
}

void chdir()
{
    int inod;
    MINODE *mip;

    // update dev on given pathname
    if(pathname[0] == '/')
        dev = root->dev;
    else
        dev = running->cwd->dev;

    // get ino depending on if pathname is given
    if(pathname[0] == 0)
        inod = running->cwd->ino;
    else
        inod = getino(pathname);
        
    // if pathname not exist
    if(inod == 0)
        return;

    mip = iget(dev, inod);

    // check if it's a dir
    if(!S_ISDIR(mip->INODE.i_mode))
    {
        printf("%s is not a directory\n", basename(pathname));
        return;
    }

    iput(running->cwd); // dispose old cwd
    running->cwd = mip; // get new cwd
}

void pwd(MINODE *wd)
{
    if(wd == root)
        printf("/");
    else
        rpwd(wd);
    
    printf("\n");
}

void rpwd(MINODE *wd)
{
    int myino, parent_ino;
    MINODE *parent;
    char myname[256];

    if(wd == root)
        return;
    
    // get myino = ino of .
    // parent = ino of ..
    parent_ino = findino(wd, &myino);

    if(!parent_ino)
    {
        printf("parent_ino: %d not found\n", parent_ino);
        return;
    }

    parent = iget(dev, parent_ino);

    if(!findmyname(parent, myino, myname))
    {
        printf("myino: %d not found\n", myino);
        return;
    }

    rpwd(parent);

    printf("/%s", myname);
}



