/************** mkdir_creat_rmdir.c file ****************/
#include "mkdir_creat_rmdir.h"

// mkdir function
int make_dir()
{
    MINODE *mip, *pip;
    int pino;
    char path[256], parent[256], child[64];

    /*1. path   name = "/a/b/c" start mip = root;         dev = root->dev;
            =  "a/b/c" start mip = running->cwd; dev = running->cwd->dev;*/
    // update dev on given pathname
    if(pathname[0] == '/')
    {
        mip = iget(dev, 2);
        dev = root->dev;
    }
    else if(pathname[0] == 0)   // return error if pathname is not given
    {
        printf("Error: cd command need a pathname\n");
        return 0;
    }
    else
    {
        mip = iget(running->cwd->dev, running->cwd->ino);
        dev = running->cwd->dev;
    }

    /*2. Let  
        parent = dirname(pathname);   parent= "/a/b" OR "a/b"
        child  = basename(pathname);  child = "c"*/
    strcpy(path, pathname);
    strcpy(parent, dirname(path));

    strcpy(path, pathname);
    strcpy(child, basename(path));

    /*3. Get the In_MEMORY minode of parent:
         pino  = getino(parent);
         pip   = iget(dev, pino); 

    Verify : (1). parent INODE is a DIR (HOW?)   AND
             (2). child does NOT exists in the parent directory (HOW?);*/
    pino = getino(parent);

    // if pathname not exist
    if(pino == 0)
        return 0;

    pip = iget(dev, pino);

    if(!S_ISDIR(pip->INODE.i_mode))
    {
        printf("%s is not a directory\n", parent);
        return 0;
    }

    if(search(pip, child))
    {
        printf("'%s' already exist\n", child);
        return 0;
    }
               
    //4. call mymkdir(pip, child);
    mymkdir(pip, child);

    /*5. inc parent inodes's link count by 1; 
        touch its atime and mark it DIRTY*/
    pip->INODE.i_links_count++;
    pip->INODE.i_atime = time(0L);
    pip->dirty = 1;

    //6. iput(pip);
    iput(pip);
    iput(mip);
    printf("mkdir completed\n");
}

// mkdir helper : allocates and create dir
void mymkdir(MINODE *pip, char *child)
{
    int ino, bno, i;
    MINODE *mip;
    char buf[BLKSIZE];

    //1. pip points at the parent minode[] of "/a/b", name is a string "c"

    //2. allocate an inode and a disk block for the new directory;
    ino = ialloc(dev);
    bno = balloc(dev);

    /*3. mip = iget(dev, ino) to load the inode into a minode[] (in order to
        wirte contents to the INODE in memory).*/
    mip = iget(dev, ino);

    //4. Write contents to mip->INODE to make it as a DIR INODE.
    mip->INODE.i_mode = 0x41ED;		        // OR 040755: DIR type and permissions
    mip->INODE.i_uid  = running->uid;	    // Owner uid 
    mip->INODE.i_gid  = running->gid;	    // Group Id
    mip->INODE.i_size = BLKSIZE;		    // Size in bytes 
    mip->INODE.i_links_count = 2;	        // Links count=2 because of . and ..
    mip->INODE.i_atime = mip->INODE.i_ctime = mip->INODE.i_mtime = time(0L);  // set to current time
    mip->INODE.i_blocks = 2;                // LINUX: Blocks count in 512-byte chunks 
    mip->INODE.i_block[0] = bno;            // new DIR has one data block   
    
    for(i = 1; i < 15; i++)
        mip->INODE.i_block[i] = 0;
 
    mip->dirty = 1;                         // mark minode dirty

    //5. iput(mip); which should write the new INODE out to disk.
    iput(mip);                              // write INODE to disk

    //***** create data block for new DIR containing . and .. entries ******
    //6. Write . and .. entries into a buf[ ] of BLKSIZE

    get_block(dev, bno, buf);
    cp = buf;
    dp = (DIR *)buf;

    // write "." entry
    dp->inode = ino;
    dp->rec_len = 12;
    dp->name_len = 1;
    strcpy(dp->name, ".");

    // move to next entry space
    cp += dp->rec_len;
    dp = (DIR *)cp;

    // write ".." entry
    dp->inode = pip->ino;
    dp->rec_len = 1012;
    dp->name_len = 2;
    strcpy(dp->name, "..");

    // Then, write buf[ ] to the disk block bno;
    put_block(dev, bno, buf);

    // Finally, enter name ENTRY into parent's directory
    enter_name(pip, ino, child);
}

// enter_name function to enter name
void enter_name(MINODE *pip, int myino, char* n_name)
{
    int i, blk;
    int need_length, ideal_length, remain;
    char buf[BLKSIZE];

    for(i = 0; i < 12; i++)
    {
        if(pip->INODE.i_block[i] == 0)
            break;
        
        //(1). get parent's data block into a buf[];
        get_block(pip->dev, pip->INODE.i_block[i], buf);

        dp = (DIR *)buf;
        cp = buf;

        // Step to the last entry in a data block
        blk = pip->INODE.i_block[i];

        printf("step to LAST entry in data block %d\n", blk);

        while(cp + dp->rec_len < buf + BLKSIZE)
        {
            cp += dp->rec_len;
            dp = (DIR *)cp;
        }
        //dp now points at last entry

        // need_length
        need_length = 4 * ((8 + strlen(n_name) + 3) / 4);

        // ideal length
        ideal_length = 4 * ((8 + dp->name_len + 3) / 4);

        remain = dp->rec_len - ideal_length;

        if(remain >= need_length)
        {
            dp->rec_len = ideal_length;     // trim rec_len to ideal_length

            // get to the end of rec_len
            cp += dp->rec_len;
            dp = (DIR *)cp;

            dp->inode = myino;
            dp->rec_len = BLKSIZE - (cp - buf);
            dp->name_len = strlen(n_name);
            strcpy(dp->name, n_name);
            
            // write data block to disk
            put_block(pip->dev, blk, buf);
            return;
        }
    }

    // Reach here means: NO space in existing data block(s)
    // Allocate new data block
    blk = balloc(pip->dev);
    pip->INODE.i_block[i] = blk;

    // increase parent's size by BLKSIZE
    pip->INODE.i_size += BLKSIZE;

    // Enter new entry as the first entry in the new data block with rec_len=BLKSIZE
    get_block(pip->dev, blk, buf);

    cp = buf;
    dp = (DIR *)buf;

    dp->inode = myino;
    dp->rec_len = BLKSIZE;
    dp->name_len = strlen(n_name);
    strcpy(dp->name, n_name);

    put_block(pip->dev, blk, buf);
}


// creat function
int creat_file()
{
    MINODE *mip, *pip;
    int pino;
    char path[256], parent[256], child[64];

    /*1. path   name = "/a/b/c" start mip = root;         dev = root->dev;
            =  "a/b/c" start mip = running->cwd; dev = running->cwd->dev;*/
    // update dev on given pathname
    if(pathname[0] == '/')
    {
        mip = iget(dev, 2);
        dev = root->dev;
    }
    else if(pathname[0] == 0)   // return if pathname is not given
    {
        printf("Error: cd command need a pathname\n");
        return -1;
    }
    else
    {
        mip = iget(running->cwd->dev, running->cwd->ino);
        dev = running->cwd->dev;
    }

    /*2. Let  
        parent = dirname(pathname);   parent= "/a/b" OR "a/b"
        child  = basename(pathname);  child = "c"*/
    strcpy(path, pathname);
    strcpy(parent, dirname(path));

    strcpy(path, pathname);
    strcpy(child, basename(path));

    /*3. Get the In_MEMORY minode of parent:
         pino  = getino(parent);
         pip   = iget(dev, pino); 

    Verify : (1). parent INODE is a DIR (HOW?)   AND
             (2). child does NOT exists in the parent directory (HOW?);*/
    pino = getino(parent);

    // if pathname not exist
    if(pino == 0)
        return -1;

    pip = iget(dev, pino);

    if(!S_ISDIR(pip->INODE.i_mode))
    {
        printf("%s is not a directory\n", parent);
        return -1;
    }

    if(search(pip, child))
    {
        printf("'%s' already exist\n", child);
        return 0;
    }
               
    //4. call my_creat(pip, child);
    my_creat(pip, child);

    /*5. touch its atime and mark it DIRTY*/
    pip->INODE.i_atime = time(0L);
    pip->dirty = 1;

    //6. iput(pip);
    iput(pip);
    iput(mip);
    printf("creat completed\n");
}


// creat helper : allocates and create file
void my_creat(MINODE *pip, char *child)
{
    int ino, i;
    MINODE *mip;
    char buf[BLKSIZE];

    //1. pip points at the parent minode[] of "/a/b", name is a string "c"

    //2. allocate an inode for the new file;
    ino = ialloc(dev);

    /*3. mip = iget(dev, ino) to load the inode into a minode[] (in order to
        wirte contents to the INODE in memory).*/
    mip = iget(dev, ino);

    //4. Write contents to mip->INODE to make it as a REG INODE.
    mip->INODE.i_mode = 0x81A4;		        // OR 040755: REG type and permissions
    mip->INODE.i_uid  = running->uid;	    // Owner uid 
    mip->INODE.i_gid  = running->gid;	    // Group Id
    mip->INODE.i_size = 0;		            // Size in bytes = 0 because no data block 
    mip->INODE.i_links_count = 1;	        // Links count=1 for reg file
    mip->INODE.i_atime = mip->INODE.i_ctime = mip->INODE.i_mtime = time(0L);  // set to current time
    mip->INODE.i_blocks = 0;                // LINUX: Blocks count in 512-byte chunks 
    mip->INODE.i_block[0] = 0;              // new DIR has one data block   
    
    for(i = 1; i < 15; i++)
        mip->INODE.i_block[i] = 0;
 
    mip->dirty = 1;                         // mark minode dirty

    //5. iput(mip); which should write the new INODE out to disk.
    iput(mip);                              // write INODE to disk

    // Finally, enter name ENTRY into parent's directory
    enter_name(pip, ino, child);
}

// rmdir function
int rm_dir()
{
    MINODE *mip, *pip;
    int ino, pino, i, empty = 0;
    char buf[BLKSIZE];

    // update dev on given pathname
    if(pathname[0] == '/')
        dev = root->dev;
    else if(pathname[0] == 0)
    {
        printf("Error: cd command need a pathname\n");
        return 0;
    }
    else
        dev = running->cwd->dev;

    // get inumber of pathname
    ino = getino(pathname);
    
    // if pathname not exist
    if(!ino)
        return 0;
    
    // get its minode[ ] pointer
    mip = iget(dev, ino);

    /*check ownership 
       super user : OK
       not super user: uid must match*/
    // uid == 0 for super user
    if(running->uid)    // if not a super user
    {
        printf("You are not a super user! checking uid ...\n");
        if(running->uid != mip->INODE.i_uid)
        {
            printf("uid does not match! rmdir request rejected!\n");
            iput(mip);
            return 0;
        }
        printf("uid matched! proceed to rmdir...\n");
    }

    // check DIR type
    if(!S_ISDIR(mip->INODE.i_mode))
    {
        printf("Error: %s is not a DIR\n", basename(pathname));
        iput(mip);
        return 0;
    }

    // check busy
    if(mip->refCount != 1)
    {
        printf("Error: %s is busy\n", basename(pathname));
        printf("refcount = %d\n", mip->refCount);
        iput(mip);
        return 0;
    }

    // check empty
    if(mip->INODE.i_links_count > 2)
    {
        printf("Error: %s is not empty\n", basename(pathname));
        iput(mip);
        return 0;
    }
    else
    {
        // see whether it has any entries in addition to . and ..
        get_block(mip->dev, mip->INODE.i_block[0], buf);
        cp = buf;
        dp = (DIR *)buf;

        if(strcmp(dp->name, "."))
        {
            printf("Error: %s is not empty\n", basename(pathname));
            iput(mip);
            return 0;
        }

        cp += dp->rec_len;
        dp = (DIR *)cp;

        if(strcmp(dp->name, ".."))
        {
            printf("Error: %s is not empty\n", basename(pathname));
            iput(mip);
            return 0;
        }
    }

    // get parent's ino
    pino = search(mip, "..");

    // Deallocate its block and inode
    for(i = 0; i < 12; i++)
    {
        if(!mip->INODE.i_block[i])
            continue;
        bdealloc(mip->dev, mip->INODE.i_block[i]);
    }
    idealloc(mip->dev, mip->ino);
    iput(mip);

    // get parent DIR's ino and Minode 
    pip = iget(mip->dev, pino);

    // remove child's entry from parent directory
    rm_child(pip, basename(pathname));

    // decrement pip's link_count by 1; 
    pip->INODE.i_links_count--;
    // touch pip's atime, mtime fields;
    pip->INODE.i_atime = time(0L);
    pip->INODE.i_mtime = time(0L);
    // mark pip dirty;
    pip->dirty = 1;
     
    iput(pip);
    printf("rmdir completed\n");
    //return SUCCESS;
}


// removes the entry [INO rlen nlen name] from parent's data block.
void rm_child(MINODE *parent, char* child_name)
{
    int i, j, rlen;
    char *prev_cp, *temp_cp;
    char buf[BLKSIZE];

    printf("starting rm_child\n");
    // Search parent INODE's data block(s) for the entry of name
    for(i = 0; i < 12; i++)
    {
        get_block(parent->dev, parent->INODE.i_block[i], buf);
        cp = buf;
        dp = (DIR *)buf;
        rlen = 0;

        // only write update block if no more data 
        if(!parent->INODE.i_block[i])
        {
            put_block(parent->dev, parent->INODE.i_block[i], buf);
            continue;
        }

        while(cp < buf + BLKSIZE)
        {
            printf("enter first while\n");
            printf("dp->name = %s\n", dp->name);
            printf("cp = %x, rlen = %d\n", cp, dp->rec_len);
            if(!strcmp(dp->name, child_name))
            {
                printf("same name found\n");
                if(dp->rec_len == BLKSIZE)   // if it is the only entry in block
                {
                    printf("enter first case\n");
                    bdealloc(parent->dev, parent->INODE.i_block[i]);
                    parent->INODE.i_block[i] = 0;
                    parent->INODE.i_size -= BLKSIZE;

                    // move nonzero block upward to close the holes
                    for(j = i+1; j < 12; j++)
                    {
                        if(parent->INODE.i_block[j] != 0)
                            parent->INODE.i_block[j-1] = parent->INODE.i_block[j];
                    }
                }
                else if(cp + dp->rec_len >= buf + BLKSIZE)  // if it is the last entry
                {
                    printf("enter 2nd case\n");
                    // add this entry rec_len to previous entry rec_len
                    rlen = dp->rec_len;
                    
                    cp = prev_cp;
                    dp = (DIR *)cp;

                    dp->rec_len += rlen;
                }
                else if(cp + dp->rec_len < buf + BLKSIZE)   // if it is not the last entry
                {   
                    printf("enter 3rd case\n");
                    rlen = dp->rec_len;
                    temp_cp = cp;
                    // traverse to last entry
                    while(temp_cp + dp->rec_len < buf + BLKSIZE)
                    {
                        temp_cp += dp->rec_len;
                        dp = (DIR *)temp_cp;
                    }
                    // add rlen to last entry's rec_len
                    dp->rec_len += rlen;

                    // swap the content from next entry address with the content current entry address
                    // until reach the end of i_block[i]
                    while(cp + rlen <= buf + BLKSIZE)
                    {
                        *cp = *(cp + rlen);
                        cp++;
                    }
                    cp = prev_cp;
                }   
            }
            prev_cp = cp;
            cp += dp->rec_len;
            dp = (DIR *)cp;
        }
        put_block(parent->dev, parent->INODE.i_block[i], buf);
    }
}

