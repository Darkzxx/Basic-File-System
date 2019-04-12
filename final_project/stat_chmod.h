/************** stat_chmod.h file ****************/
#ifndef ST_CHMOD_H
#define ST_CHMOD_H

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

int my_chmod();         // change file permission
int my_stat();          // stat filename
int my_touch();         // touch filename

#endif