/************** link_unlink_symlink.c file ****************/
#include "link_unlink_symlink.h"

void my_link()
{
	char old[64], new[64], temp[64];
	char link_parent[64], link_child[64];
	int ino;
	int p_ino;
	MINODE *mip;
	MINODE *p_mip;
	INODE *ip;
	INODE *p_ip;

	//Checks if file exists
	if(!strcmp(pathname, ""))
	{
		printf("Given file does not exist\n");
		return;
	}

	if(!strcmp(pathname1, ""))
	{
		printf("No new file name given\n");
		return;
	}

	strcpy(old, pathname);
	strcpy(new, pathname1);

	//get oldfilename's inode
	ino = getino(old);

	//old file doesn't exists
	if(!ino)
	{
		printf("%s does not exist\n", old);
		return;
	}

	mip = iget(dev, ino);
	
	//its file not dir
	if(S_ISDIR(mip->INODE.i_mode))
	{
		printf("Can't link directory\n");
		return;
	}
	//printf("got it!\n");

	//get new's dirname
	if(!strcmp(new, "/"))
	{
		strcpy(link_parent, "/");
	}
	else
	{
		strcpy(temp, new);
		strcpy(link_parent, dirname(temp));
	}

	//get new's basename
	strcpy(temp, new);
	strcpy(link_child, basename(temp));

	//get new's parent
	printf("parent = %s\n", link_parent);
	p_ino = getino(link_parent);
	p_mip = iget(dev, p_ino);

	//link parent exists
	if(!p_mip)
	{
		printf("No parent\n");
		return;
	}

	//link parent is dir
	if(!S_ISDIR(p_mip->INODE.i_mode))
	{
		printf("Not a directory\n");
		return;
	}

	//make sure link child does not exist
	if(getino(new))
	{
		printf("%s already exists\n", new);
		return;
	}


	//link new with ino of old file
	enter_name(p_mip, ino, link_child);

	ip = &mip->INODE;

	//increment link count
	ip->i_links_count++;

	p_ip = &p_mip->INODE;

	iput(p_mip);
	iput(mip);

	return;
}


void my_unlink()
{
	int ino, i;
	int parent_ino;
	MINODE *mip;
	MINODE *parent_mip;
	INODE *ip;
	INODE *parent_ip;

	char temp[64];
	char my_dirname[64];
	char my_basename[64];

	//pathname given
	if(!pathname)
	{
		printf("No pathname given\n");
		return;
	}

	//Gets the ino
	ino = getino(pathname);

	//if ino exist
	if(ino == 0)
	{
		printf("Bad path\n");
		return;
	}

	//Get the minode
	mip = iget(dev, ino);

	//Make sure its a file
	if(S_ISDIR(mip->INODE.i_mode))
	{
		printf("Can't unlink a directory\n");
		return;
	}

	ip = &mip->INODE;

	//Decrement link count
	ip->i_links_count--;

	//Deallocate its blocks
	my_truncate(mip);

	//deallocate its inode
	idealloc(dev, ino);

	strcpy(temp, pathname);
	strcpy(my_dirname, dirname(temp));

	strcpy(temp, pathname);
	strcpy(my_basename, basename(temp));

	//Gets the parent and removes the file from its parent
	parent_ino = getino(my_dirname);
	parent_mip = iget(dev, parent_ino);
	parent_ip = &parent_mip->INODE;

	//Removes the child from the parent
	rm_child(parent_mip, my_basename);

	iput(parent_mip);
	iput(mip);

	printf("unlink completed\n");
}


void my_symlink()
{
	int ino, i;
	int link_ino;
	int parent_ino;
	char temp[64], parent[64], child[64];
	char old_name[64];
	
	MINODE *mip;
	MINODE *link_mip;
	MINODE *parent_mip;

	INODE *ip;
	INODE *parent_ip;
	INODE *link_ip;

	strcpy(temp, pathname);
	strcpy(old_name, basename(temp));

	ino = getino(pathname);

	//if oldname doesn't exist, return
	if(!ino)
	{
		printf("%s does not exist", pathname);
		return;
	}

	mip = iget(dev, ino);

	//oldname < 60
	if(strlen(pathname) >= 60)
	{
		printf("oldName longer => 60 char\n");
		return;
	}

	// either a DIR or a REG file
	if(!S_ISDIR(mip->INODE.i_mode) && !S_ISREG(mip->INODE.i_mode))
	{
		printf("oldName is not a reg or dir\n");
		return;
	}

	//get parent and child of oldname file
	strcpy(temp, pathname1);
	strcpy(parent, dirname(temp));
	strcpy(child, basename(pathname1));

	parent_ino = getino(parent);
	parent_mip = iget(dev, parent_ino);


	if(getino(pathname1) > 0)
	{
		printf("%s already exists", pathname1);
		return;
	}

	//create file
	my_creat(parent_mip, child);
	iput(parent_mip);

	link_ino = getino(pathname1);
	link_mip = iget(dev, link_ino);
	link_ip = &link_mip->INODE;

	//set type to LNK
	link_ip->i_mode |= 0120000;

	// write the string oldNAME into the i_block[ ]
	strcpy(link_ip->i_block, old_name);
	// set size
	link_ip->i_size = strlen(old_name);

	link_mip-> dirty = 1;
	iput(link_mip);
	
	printf("symlink completed\n");

}


// truncate minode
int my_truncate(MINODE *mip)
{
    int i;
    // release mip->INODE's data blocks;
    // Deallocate its block

    printf("truncate begin\n");
    for(i = 0; i < 15; i++)
    {
        if(!mip->INODE.i_block[i])
            continue;
        bdealloc(mip->dev, mip->INODE.i_block[i]);
    }
    
    //  update INODE's time field
    mip->INODE.i_atime = time(0L);
    mip->INODE.i_mtime = time(0L);

    // set INODE's size to 0 and mark Minode[ ] dirty
    mip->INODE.i_size = 0;
    mip->dirty = 1;
}