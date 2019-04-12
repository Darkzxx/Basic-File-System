/**** globals defined in main.c file ****/

#ifndef GLOBAL_H
#define GLOBAL_H

#include "type.h"

MINODE minode[NMINODE];
MINODE *root;
PROC   proc[NPROC], *running;
char gpath[128];
char *name[64];
int n;
int fd, dev;
int nblocks, ninodes, bmap, imap, inode_start;
char line[256], cmd[32], pathname[256];

#endif