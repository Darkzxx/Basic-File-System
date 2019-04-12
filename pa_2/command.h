// Anan Kraisakdawat
// 11503146
// Cpts 360 Lab 2
#include "node.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#ifndef COMMAND_H
#define COMMAND_H

/***************** call function *****************/
void mkdir(char *pathname, char *dname, char *bname, NODE* root, NODE* cwd);
void rmdir(char *pathname, NODE* root, NODE* cwd);
void ls(char *pathname, NODE* root, NODE* cwd);
NODE* cd(char *pathname, NODE* root, NODE* cwd);
void pwd(char *path, NODE* root, NODE* cwd);
void creat(char *pathname, char *dname, char *bname, NODE* root, NODE* cwd);
void rm(char *pathname, char *dname, char *bname, NODE* root, NODE* cwd);
void save(NODE* root);
void reload(NODE* root, NODE* cwd);
void menu();
void quit(NODE *root);

/***************** helper function *****************/
bool isAbsPathname(char *pathname);      //check whether pathname is absolute?
void getDname_Bname(char *pathname, char *dname, char *bname); // breakup pathname to dirname and basename
NODE* searchDirName(char *pathname, char *dname, NODE* root, NODE* cwd);    // search dirname if they exist
bool NoBaseNameExist(NODE *cnode, char *bname);     // search if the name already exist
void addNode(NODE *mnode, char *bname, char type);  // add new node to the tree
void rmNode(NODE *mnode); // remove a node from tree
void preorderSave(FILE *fp, NODE* root, NODE* node); // save tree 
void clearTree(NODE* node); // empty tree
void printPreorder(NODE* node); 

#endif