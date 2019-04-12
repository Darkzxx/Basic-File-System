/************** open_close_lseek.h file ****************/
#ifndef OPEN_CLOSE_LSEEK_H
#define OPEN_CLOSE_LSEEK_H

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>
#include <unistd.h>

#include "type.h"
#include "global.h"
#include "util.h"
#include "mkdir_creat_rmdir.h"
#include "link_unlink_symlink.h"

int open_file();                // open functione
int close_file();               // close function
int my_lseek();                 // lseek function
int pfd();                      // print fd
int write_file();               // write fd string
int my_write(int fd, char buf[], int nbytes);   // write helper function: work same as write() syscall
int read_file();                // read_file
int myread(int gd, char* buf, int nbyte);       // read helper function like syscall read()
int my_cat();                   // cat filename
int my_cp();                    // cp src dest
int my_mv();                    // mv src dest
int checkOFT(MINODE *mip);      // Check whether the file is ALREADY opened with INCOMPATIBLE mode:
void file_mode(int f_mode, char* r_mode);    // get mode string from mode number
int hasPermission(MINODE *mip, int access);     // check permission access

#endif