/************** util.c file ****************/
#include "util.h"

// read from a block
int get_block(int dev, int blk, char *buf)
{
    lseek(dev, (long)blk*BLKSIZE, 0);
    read(dev, buf, BLKSIZE);
}   

// write to a block
int put_block(int dev, int blk, char *buf)
{
    lseek(dev, (long)blk*BLKSIZE, 0);
    write(dev, buf, BLKSIZE);
}   

// tokenize pathname into n components: name[0] to name[n-1];
int tokenize(char *pathname)
{
    int i = 0;
    char *token;

    printf("Tokenizing Pathname: %s\n", pathname);

    token = strtok(pathname, "/");

    while(token)
    {
        name[i] = malloc(strlen(token) + 1);
        strcpy(name[i], token);
        printf("name[%d] = %s\n", i, name[i]);
        i++;
        token = strtok(NULL, "/");
    }

    name[i] = NULL;
    n = i;
}

// return minode pointer to loaded INODE
MINODE *iget(int dev, int ino)
{
    MINODE *mip;
    int i, blk, offset;
    char buf[BLKSIZE];

    
    /*(1). Search minode[ ] for an existing entry (refCount > 0) with 
       the needed (dev, ino):
       if found: inc its refCount by 1;
                 return pointer to this minode;*/
    for(i = 0; i < NMINODE; i++)
    {
        mip = &minode[i];
        if((mip->dev == dev) && (mip->ino == ino))
        {
            mip->refCount++;
            return mip;
        }
    }
    

    /*(2). needed entry not in memory:
       find a FREE minode (refCount = 0); Let mip-> to this minode;
       set its refCount = 1;
       set its dev, ino*/
    for(i = 0; i < NMINODE; i++)
    {
        mip = &minode[i];
        if(mip->refCount == 0)
        {
            mip->dev = dev;
            mip->ino = ino;
            mip->refCount++;

            //(3). load INODE of (dev, ino) into mip->INODE:
       
            // get INODE of ino a char buf[BLKSIZE]    
            blk    = (ino-1) / 8 + inode_start;
            offset = (ino-1) % 8;

            //printf("iget: ino=%d blk=%d offset=%d\n", ino, blk, offset);

            get_block(dev, blk, buf);
            ip = (INODE *)buf + offset;
            mip->INODE = *ip;  // copy INODE to mp->INODE

            return mip;
        }
    }

    printf("No FREE minodes\n");
    return 0;
}


void iput(MINODE *mip) // save minode data to disk
{
    int blk, offset;
    char buf[BLKSIZE];

    mip->refCount--;
 
    if (mip->refCount > 0) return; // still being used
    if (!mip->dirty)       return; // has not been modified
 
    // write mip->INODE back to disk
    printf("iput(dev, ino) = iput(%d, %d)\n", mip->dev, mip->ino);

    blk = (mip->ino - 1) / 8 + inode_start;
    offset = (mip->ino - 1) % 8;
    
    get_block(mip->dev, blk, buf);
    ip = (INODE *)buf + offset;
    *ip = mip->INODE;

    put_block(mip->dev, blk, buf);
} 


// search a DIRectory INODE for entry with a given name
// return ino if found; return 0 if NOT
int search(MINODE *mip, char *name)
{
    char nbuf[256], dbuf[BLKSIZE];
    int i;

    ip = &mip->INODE;

    // loop through all direct i_block[i] for current inode ip
    for (i = 0; i < 12; i++)
    {
        if(ip->i_block[i] == 0)
            break;

        get_block(mip->dev, ip->i_block[i], dbuf);
        cp = dbuf;
        dp = (DIR *)dbuf;

        while(cp < dbuf + BLKSIZE)
        {
            strncpy(nbuf, dp->name, dp->name_len);
            nbuf[dp->name_len] = 0;

            if(!strcmp(name, nbuf))
            {
                return dp->inode;
            }

            cp += dp->rec_len;
            dp = (DIR *)cp;
        }
    }

    return 0; 
}


// return inode number of pathname
int getino(char *pname)
{ 
    MINODE *mip;
    int i = 0;
    int inod;
    char nbuf[256];

    printf("getino(pathname), pathname = %s\n", pname);
    
    // return 2 for root
    if(!strcmp(pname, "/"))
        return 2;
    
    // absolute or relative
    if(pname[0] == '/')
        mip = iget(fd, 2);
    else
        mip = iget(running->cwd->dev, running->cwd->ino);
    
    strcpy(nbuf, pname);
    tokenize(nbuf);

    for(i = 0; i < n; i++)
    {
        printf("Searching for %s...", name[i]);
        inod = search(mip, name[i]);
        if(!inod)
        {
            printf(" Not Found\n");
            return 0;
        }
        
        printf(" Found\n");
        iput(mip);
        mip = iget(dev, inod);
    }
    
    iput(mip);
    return inod;

}


// THESE two functions are for pwd(running->cwd), which prints the absolute
// pathname of CWD. 
int findmyname(MINODE *parent, u32 myino, char *myname) 
{
   // parent -> at a DIR minode, find myname by myino
   // get name string of myino: SAME as search except by myino;
    char dbuf[BLKSIZE];
    int i;

    ip = &parent->INODE;

    // loop through all direct i_block[i] for current inode ip
    for (i = 0; i < 12; i++)
    {
        if(ip->i_block[i] == 0)
            break;

        get_block(dev, ip->i_block[i], dbuf);
        cp = dbuf;
        dp = (DIR *)dbuf;

        while(cp < dbuf + BLKSIZE)
        {
            if(myino == dp->inode)
            {
                // copy entry name (string) into myname[ ];
                strncpy(myname, dp->name, dp->name_len);
                myname[dp->name_len] = 0;
                return 1;
            }

            cp += dp->rec_len;
            dp = (DIR *)cp;
        }
    }

    return 0; 
}


int findino(MINODE *mip, u32 *myino) 
{
    // fill myino with ino of .
    *myino = mip->ino; 
    // ino of ..
    return search(mip, "..");
}