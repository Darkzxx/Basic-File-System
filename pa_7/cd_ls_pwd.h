/************** cd_ls_pwd.h file ****************/
#ifndef CD_LS_PWD_H
#define CD_LS_PWD_H

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

void ls();
void ls_file(char *name, int inod);
void chdir();
void pwd(MINODE *wd);
void rpwd(MINODE *wd);

#endif