/************** mkdir_creat_rmdir.h file ****************/
#ifndef MK_RM_CRE_H
#define MK_RM_CRE_H

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

int make_dir();                                         // mkdir function
void mymkdir(MINODE *pip, char *child);                 // mkdir helper : allocates and create dir
void enter_name(MINODE *pip, int myino, char* n_name);  // enter_name function to enter name
int creat_file();                                       // creat function
void my_creat(MINODE *pip, char *child);                // creat helper : allocates and create file
int rm_dir();                                           // rmdir function
void rm_child(MINODE *parent, char* child_name);        // removes the entry [INO rlen nlen name] from parent's data block.

#endif