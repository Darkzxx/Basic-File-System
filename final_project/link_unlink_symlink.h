/************** link_unlink_symlink.h file ****************/
#ifndef LK_ULK_SLK_H
#define LK_ULK_SLK_H

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
#include "mkdir_creat_rmdir.h"

void my_link();       //link function
void my_unlink();     //unlink function
void my_symlink();    //symlink function
int my_truncate(MINODE *mip);   // truncate minode

#endif