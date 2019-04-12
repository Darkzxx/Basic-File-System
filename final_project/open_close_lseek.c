/************** open_close_lseek.c file ****************/
#include "open_close_lseek.h"

// function to open
// mode = 0|1|2|3 for R|W|RW|APPEND
int open_file()
{
    int mode, ino, f_mode, i, fd;
    int u_perm = 0, g_perm = 0;
    char md[5];
    MINODE *mip;
    OFT *oftp;

    // check pathnames exist
    if(pathname[0] == 0 || pathname1[0] == 0)
    {
        printf("Error: command %s need 2 inputs\n", cmd);
        return -1;
    }

    // update dev on given pathname
    if(pathname[0] == '/')
        dev = root->dev;
    else
        dev = running->cwd->dev;

    ino = getino(pathname);

    // if pathname not exist
    if(ino == 0)
        return -1;

    mip = iget(dev, ino);

    // get mode from pathname1
    if(!strcmp(pathname1, "R") || !strcmp(pathname1, "RD"))
        mode = 0;
    else if(!strcmp(pathname1, "W") || !strcmp(pathname1, "WR"))
        mode = 1;
    else if(!strcmp(pathname1, "RW"))
        mode = 2;
    else if(!strcmp(pathname1, "AP") || !strcmp(pathname1, "APPEND"))
        mode = 3;
    else
        mode = atoi(pathname1);
    
    if(mode < 0 || mode > 3)
    {
        printf("incorrect mode input\n");
        return -1;
    }

    // check its a regular file
    if(!S_ISREG(mip->INODE.i_mode))
    {
        printf("Error: %s is not a regular file\n", basename(pathname));
        iput(mip);
        return -1;
    } 

    // check user's permission
    if(running->uid == mip->INODE.i_uid)
        u_perm = 1;
    // check group permission
    if(running->gid == mip->INODE.i_gid)
        g_perm = 1;
    
    // Check whether the file is ALREADY opened with INCOMPATIBLE mode:
    // If it's already opened for W, RW, APPEND : reject.
    f_mode = checkOFT(mip);
    printf("f_mode = %d\n", f_mode);
    file_mode(f_mode, md);

    if(f_mode > 0)          // already opened for W, RW, APPEND
    {
        printf("Error: %s already open for %s\n", basename(pathname), md);
        iput(mip);
        return -1;
    }
    else if(f_mode == 0)    // already opened for 'R'
    {
        // multiple R mode
        if(mode == 0)
        {
            // if user has no permission to access
            if(!(u_perm && hasPermission(mip, S_IRUSR)) && !(g_perm && hasPermission(mip, S_IRGRP)) && 
                !hasPermission(mip, S_IROTH))
            {
                printf("Error: R permission denied\n");
                iput(mip);
                return -1;
            }
        }
        else
        {
            printf("Error: %s already open for %s\n", basename(pathname), md);
            iput(mip);
            return -1;
        }
    }
    else    // not yet open
    {
        // check access permission
        if(mode == 0)   
        {
            // if user has no permission to access
            if(!(u_perm && hasPermission(mip, S_IRUSR)) && !(g_perm && hasPermission(mip, S_IRGRP)) && 
                !hasPermission(mip, S_IROTH))
            {
                printf("Error: R permission denied\n");
                iput(mip);
                return -1;
            }
        }
        else if(mode == 1)
        {
            // if user has no permission to access
            if(!(u_perm && hasPermission(mip, S_IWUSR)) && !(g_perm && hasPermission(mip, S_IWGRP)) && 
                !hasPermission(mip, S_IWOTH))
            {
                printf("Error: W permission denied\n");
                iput(mip);
                return -1;
            }
        }
        else if(mode == 2)
        {
            // if user has no permission to access
            if(!(u_perm && hasPermission(mip, 384)) && !(g_perm && hasPermission(mip, 48)) && 
                !hasPermission(mip, 6))
            {
                printf("Error: RW permission denied\n");
                iput(mip);
                return -1;
            }
        }
        else if(mode == 3)
        {
            // if user has no permission to access
            if(!(u_perm && hasPermission(mip, S_IWUSR)) && !(g_perm && hasPermission(mip, S_IWGRP)) && 
                !hasPermission(mip, S_IWOTH))
            {
                printf("Error: W permission denied, APPEND denied\n");
                iput(mip);
                return -1;
            }
        }
    }

    printf("permission passed\n");
    // allocate oft and get oftp
    for(i = 0; i < NOFT; i++)
    {
        if(oft[i].mptr == 0)
            oftp = &oft[i];
    }

    printf("oft[i] found\n");
    // fills in values
    oftp->mode = mode;
    oftp->mptr = mip;
    oftp->refCount = 1;
    printf("mode = %d\n", mode);

    // Depending on the open mode 0|1|2|3, set the OFT's offset accordingly
    switch(mode)
    {
        case 0: oftp->offset = 0;       // R: offset = 0
                break;
        case 1: my_truncate(mip);          // W: truncate file to 0 size
                oftp->offset = 0;
                break;
        case 2: oftp->offset = 0;       // RW: do NOT truncate file
                break;
        case 3: oftp->offset = mip->INODE.i_size;   // APPEND mode
                break;
        default: printf("invalid mode\n");
                return -1;
    }

    // find the SMALLEST i in running PROC's fd[ ] such that fd[i] is NULL
    // Let running->fd[i] point at the OFT entry
    for(i = 0; i < NFD; i++)
    {
        if(!running->fd[i])
        {
            running->fd[i] = oftp;
            break;
        }
    }

    if(i == NFD)
    {
        printf("Error: No available fd[i] in running procs\n");
        iput(mip);
        return -1;
    }

    // update INODE's time field
    //for R: touch atime.
    if(mode == 0)
        mip->INODE.i_atime = time(0L); 
    else    //for W|RW|APPEND mode : touch atime and mtime
    {
        mip->INODE.i_atime = time(0L);
        mip->INODE.i_mtime = time(0L);
    }

    // mark dirty
    mip->dirty = 1;

    // return i as the file descriptor
    return i;
}

// close function
int close_file()
{
    int i, fd;
    OFT *oftp;
    MINODE *mip;

    // check pathname exist
    if(pathname[0] == 0)
    {
        printf("Error: close command need a file descriptor\n");
        return -1;
    }

    fd = atoi(pathname);
    
    printf("fd = %d\n", fd);
    // verify fd is within range.
    if(fd < 0 || fd >= NFD)
    {
        printf("Error: fd is out of range\n");
        return -1;
    }
    
    // verify running->fd[fd] is pointing at a OFT entry
    if(running->fd[fd] == 0)
    {
        printf("Error: running->fd[%d] doesn't exist\n", fd);
        return -1;
    }
    // verify that it also exists in oft
    for(i = 0; i < NOFT; i++)
    {
        if(!oft[i].mptr)
            continue;
        if(running->fd[fd] == &oft[i])
            break;
    }
    if(i >= NOFT)
    {
        printf("Error: no such fd in oft\n");
        return -1;
    }
    
    // closing file
    oftp = running->fd[fd];
    running->fd[fd] = 0;
    oftp->refCount--;

    // not last user
    if(oftp->refCount > 0)
    {
        return 0;
    }
    
    // last user of this OFT entry ==> dispose of the Minode[]
    mip = oftp->mptr;
    iput(mip);

    oft[i].mptr = 0;
    
    return 0;
}

// lseek function
int my_lseek()
{
    int fd, position, i, ori;
    OFT *oftp;

    // check pathname are given
    if(pathname[0] == 0 || pathname1[0] == 0)
    {
        printf("Error: command %s need 2 inputs\n", cmd);
        return -1;
    }

    fd = atoi(pathname);
    position = atoi(pathname1);

    // verify fd is within range.
    if(fd < 0 || fd >= NFD)
    {
        printf("Error: fd is out of range\n");
        return -1;
    }

    // From fd, find the OFT entry.
    for(i = 0; i < NOFT; i++)
    {
        if((running->fd[fd]->mptr->dev == oft[i].mptr->dev) &&
            (running->fd[fd]->mptr->ino == oft[i].mptr->ino))
        {
            oftp = &oft[i];
            break;
        }
    }

    if(i >= NOFT)
    {
        printf("Error: fd has not been opened\n");
        return -1;
    }

    // make sure NOT to over run either end of the file.
    if(position < 0 || position > oftp->mptr->INODE.i_size)
    {
        printf("Error: position value can only be between 0 - %d\n", oftp->mptr->INODE.i_size);
        return -1;
    }

    // change OFT entry's offset to position
    ori = oftp->offset;
    oftp->offset = position;

    // return originalPosition
    return ori;
}

// print fd
int pfd()
{   
    int i;
    OFT *oftp;
    char md[5];

    printf("\n  fd    mode   offset   INODE\n");
    printf(" ----   ----   ------   -----\n");

    for(i = 0; i < NFD; i++)
    {
        if(!running->fd[i])
            continue;

        oftp = running->fd[i];
        file_mode(oftp->mode, md);

        printf("  %2d     %s      %d     [%d, %d]\n", i, md, oftp->offset, 
            oftp->mptr->dev, oftp->mptr->ino);
    }
}


// write fd string
int write_file()
{
    int fd, nbytes;
    char buf[BLKSIZE];

    // check pathname is give
    if(pathname[0] == 0 || pathname1[0] == 0)
    {
        printf("Error: write need fd and string arguments\n");
        return -1;
    }

    fd = atoi(pathname);
    
    //string into buf
    strcpy(buf, pathname1);
    OFT *oftp = running->fd[fd];

    //file not in write, read/write or append mode
    if(oftp->mode == 0)
    {
        printf("fd is not opened for WR, RW or APPEND\n");
        return -1;
    }
    //call helper function
    return (my_write(fd, buf, strlen(buf)));
}

// write helper function: work same as write() syscall
int my_write(int fd, char buf[], int nbytes)
{
    MINODE *mip = running->fd[fd]->mptr;
    int offset = running->fd[fd]->offset;
    int orbytes = nbytes;
    int count = 0;

    char wbuf[BLKSIZE];
    int lbk, start, remain;
    int blk;

    OFT *oftp = running->fd[fd];

    //Check to make sure the fd can exist
	if(fd < 0 || fd >= NFD)
	{
		printf("fd out of range\n");
		return -1;
	}
    //write data into fd (lbk by lbk)
    while (nbytes > 0)
    {
        //compute Logical Block (lbk) status
        lbk = oftp->offset / BLKSIZE;
        start = oftp->offset% BLKSIZE;
        remain = BLKSIZE - start;
        printf("lbk = %d, start = %d, remain = %d\n", lbk, start, remain);

        //find the correct data block to write into
        if (lbk < 12)
        {
            if(mip->INODE.i_block[lbk] == 0)  //if block has no data
                mip->INODE.i_block[lbk] = balloc(mip->dev);   //allocate a block
            
            blk = mip->INODE.i_block[lbk];      //blk should be a disk block now
            printf("new allocated direct blk = %d\n", blk);

        }

        else if (lbk >= 12 && lbk < 256 + 12)
        {
            int addressbuf[256];

            if (mip->INODE.i_block[12] == 0)    //if block has no data
                mip->INODE.i_block[12] = balloc(mip->dev);     //allocate a block
            
            get_block(dev, mip->INODE.i_block[12], addressbuf);
            
            if (addressbuf[lbk - 12] == 0)    //if block has no data
                addressbuf[lbk -12] = balloc(mip->dev);     //allocate a block

            blk = addressbuf[lbk - 12];
            put_block(dev, mip->INODE.i_block[12], addressbuf);  //update for its new data

            printf("new allocated indirect blk = %d\n", blk);
        }

        else
        {
            int addressbuf1[256];
            int addressbuf2[256];

            if (mip->INODE.i_block[13] ==0)  //if block has no data
                mip->INODE.i_block[13] = balloc(mip->dev);   //allocate a block

            get_block(dev, mip->INODE.i_block[13], addressbuf1);

            if(addressbuf1[(lbk - (256 + 12)) /256] == 0)   //first indirect
                addressbuf1[(lbk - (256 + 12)) /256] = balloc(mip->dev);

            put_block(dev, mip->INODE.i_block[13], addressbuf1);

            get_block(dev, addressbuf1[(lbk - (256 + 12)) /256], addressbuf2);

            if(addressbuf2[(lbk - (256 + 12)) % 256] == 0)  //second indirect
                addressbuf2[(lbk - (256 + 12)) % 256] = balloc(mip->dev);

            //find blk to write to
            blk = addressbuf2[(lbk - (256 + 12)) % 256];

            printf("new allocated double indirect blk = %d", blk);
            
        }

        //now write to blk on disk
        get_block(mip->dev, blk, wbuf);     //read disk block into wbuf

        char *cq = buf;              //read from cq
        char *cp = wbuf + start;     //write to cp

        if (remain > 0)       //if remain = 0, reach the end of lbk
        {
            if (nbytes >= remain)    //write more than remain
            {
                memcpy(cp, cq, remain);
                nbytes -= remain;
                oftp->offset += remain;
                remain = 0;

                if (oftp->offset > mip->INODE.i_size)    //increase file size as needed
                    mip->INODE.i_size = oftp->offset;
            }

            else          //write less than remain
            {
                memcpy(cp, cq, nbytes);
                remain -= nbytes;
                oftp->offset += nbytes;
                nbytes = 0;   //done
                
                if(oftp->offset > mip->INODE.i_size)
                    mip->INODE.i_size = oftp->offset;
            }
        }

        put_block(mip->dev, blk, wbuf);      //write wbuf[] to disk

        //write the next lbk if needed
    }

    mip->dirty = 1;
    printf("wrote %d bytes into fd = %d\n", orbytes, fd);
    iput(mip);
    return orbytes;
}

// read_file
int read_file()
{
    int nbyte, fd;
    char buf[BLKSIZE];
    OFT *oftp;

    // check pathname is given
    if(pathname[0] == 0 || pathname1[0] == 0)
    {
        printf("Error: read need fd and nbyte arguments\n");
        return -1;
    }

    // ask for a fd  and  nbytes to read;
    fd = atoi(pathname);
    nbyte = atoi(pathname1);

    // check file is open for R|RW
    oftp = running->fd[fd];

    if(oftp->mode != 0 && oftp->mode != 2)
    {
        printf("file is not open for R or RW\n");
        return -1;
    }

    return (myread(fd, buf, nbyte));
}

// read helper function like syscall read()
int myread(int gd, char* buf, int nbyte)
{
    int lbk, offset, start, remain, avail, blk;
    int dbuf[256], ebuf[256];
    char readbuf[BLKSIZE];
    MINODE *mip;
    int count = 0, lesser;
    char *cq = buf;

    offset = running->fd[gd]->offset;
    mip = iget(running->fd[gd]->mptr->dev, running->fd[gd]->mptr->ino);

    // At this moment, the file has ... available for read
    avail = running->fd[gd]->mptr->INODE.i_size - offset;
    
    while(nbyte > 0 && avail > 0)
    {
        printf("enter read while\n");
        // the current byte position, offset falls in a LOGICAL block (lbk)
        lbk = offset / BLKSIZE;

        // the byte to start read in that logical block
        start = offset % BLKSIZE;

        if(lbk < 12)    // lbk is a direct block
            blk = mip->INODE.i_block[lbk];  // map LOGICAL lbk to PHYSICAL blk
        else if(lbk >= 12 && lbk < 256 + 12)
        {
            // indirect blocks
            get_block(mip->dev, mip->INODE.i_block[12], dbuf);
            blk = dbuf[lbk-12];
        }
        else
        {
            // double indirect blocks
            get_block(mip->dev, mip->INODE.i_block[13], dbuf);
            get_block(mip->dev, dbuf[(lbk-12-256)/256], ebuf);
            blk = ebuf[(lbk-12-256)%256];
        }

        /* get the data block into readbuf[BLKSIZE] */
        get_block(mip->dev, blk, readbuf);

        /* copy from startByte to buf[ ], at most remain bytes in this block */
        cp = readbuf + start;
        // the number of bytes remaining in the logical block
        remain = BLKSIZE - start;

        if (remain > 0)
        {
            // if there is more in buf and avail than size of remain
            // copy everything in remain
            if(nbyte >= remain && avail >= remain)
            {
                memcpy(cq, cp, remain);
                running->fd[gd]->offset += remain;
                count += remain;
                avail -= remain;
                nbyte -= remain;
                remain = 0;
            }
            else    // if nbyte or avail less than remain
            {
                // we take the minimum of avail, nbyte and remain
                lesser = avail;
                if(nbyte < avail)
                    lesser = nbyte;
                
                // copy the lesser amount left
                memcpy(cq, cp, lesser);
                running->fd[gd]->offset += lesser;
                count += lesser;
                avail -= lesser;
                nbyte -= lesser;
                remain -= lesser;   
            }
        }

        // if one data block is not enough, loop back to OUTER while for more ...
    }
    printf("myread: read %d char from file descriptor %d\n", count, fd);  
    iput(mip);
    return count;   // count is the actual number of bytes read
}

// cat filename
int my_cat()
{
    char mybuf[BLKSIZE];
    int i, k, fd;

    // check pathname is given
    if(pathname[0] == 0)
    {
        printf("Error: cat need a pathname argument\n");
        return -1;
    }

    // open file for read
    strcpy(pathname1, "R");
    
    if((fd = open_file()) < 0)
        return -1;

    while((i = myread(fd, mybuf, BLKSIZE)) > 0)
    {
        mybuf[i] = 0;   // null char at the end
        
        // spit out chars from mybuf[ ] but handle \n properly
        for(k = 0; k < i; k++)
            putchar(mybuf[k]);   
    }
    printf("\n");
    sprintf(pathname, "%d", fd);
    pathname1[0] = 0;

    if(close_file() < 0)
        return -1;
}

// cp src dest
int my_cp()
{
    char src[256], dest[256], buf[BLKSIZE];
    int gd, hd, i;

    // check pathname is given
    if(pathname[0] == 0 || pathname1[0] == 0)
    {
        printf("Error: cp need src and desc arguments\n");
        return -1;
    }

    strcpy(src, pathname);
    strcpy(dest, pathname1);

    // open src for read
    strcpy(pathname1, "R");
    gd = open_file();
    if(gd < 0)
    {
        printf("open gd failed\n");
        return -1;
    }

    // open desc for W
    strcpy(pathname, dest);
    strcpy(pathname1, "W");
    
    // creat file if not already exist
    if(creat_file() < 0)
    {
        printf("creat %s failed\n", basename(dest));
        return -1;
    }

    hd = open_file();
    if(hd < 0)
    {
        printf("open hd failed\n");
        return -1;
    }

    // transfer file from src to dest
    while(i = myread(gd, buf, BLKSIZE))
        my_write(hd, buf, i);
    

    sprintf(pathname, "%d", gd);
    close_file();

    sprintf(pathname, "%d", hd);
    close_file();

    return 0;
}


// mv src dest
int my_mv()
{
    int ino;
    char src[256], dest[256];
    MINODE *mip;
    
    // check pathnames exist
    if(pathname[0] == 0 || pathname1[0] == 0)
    {
        printf("Error: command %s need 2 pathnames\n", cmd);
        return -1;
    }

    strcpy(src, pathname);
    strcpy(dest, pathname1);

    // update dev on given pathname
    if(pathname[0] == '/')
        dev = root->dev;
    else
        dev = running->cwd->dev;

    // verify src exists; get its INODE in ==> you already know its dev
    ino = getino(src);
    // if source not exist
    if(ino == 0)
        return -1;

    mip = iget(dev, ino);

    // check whether src is on the same dev
    if(mip->dev == running->cwd->dev)   // same dev
    {
        //Hard link dst with src
        my_link();
        // unlink src
        my_unlink();
    }
    else    // not same dev
    {
        //cp src to dst
        my_cp();
        // unlink src
        my_unlink();
    }
    
    iput(mip);
    return 0;
}


// Check whether the file is ALREADY opened with INCOMPATIBLE mode:
// If it's already opened for R, W, RW, APPEND return mode else return -1
int checkOFT(MINODE *mip)
{
    int i;

    for(i = 0; i < NOFT; i++)
    {
        if(oft[i].mptr == 0)
            continue;

        if((oft[i].mptr->dev == mip->dev) && (oft[i].mptr->ino == mip->ino))
        {
            if((oft[i].refCount > 0) && (oft[i].mode > -1))
                return oft[i].mode;
        }
    }
    return -1;
}

// get mode string from mode number
void file_mode(int f_mode, char *r_mode)
{
    switch(f_mode)
    {
        case 1: strcpy(r_mode, "WR");
            break;
        case 2: strcpy(r_mode, "RW");
            break;
        case 3: strcpy(r_mode, "AP");
            break;
        default: strcpy(r_mode, "RD");
            break;
    }
}

// check permission access
int hasPermission(MINODE *mip, int access)
{
    if((mip->INODE.i_mode & access) != 0)
        return 1;
    return 0;
}
