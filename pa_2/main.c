// Anan Kraisakdawat
// 11503146
// Cpts 360 Lab 2

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "node.h"
#include "command.h"

// global vars
NODE *root;
NODE *cwd;               //root and CWD
char line[128];                 //user input command line
char command[16], pathname[64]; //command and pathname strings
char dname[64], bname[64];      //dirname and basename string holders
char path[64];
int q = 1;

// command table for command search
char *cmd[] = {"mkdir", "rmdir", "ls", "cd", "pwd", "creat", "rm", "save",
             "reload", "menu", "quit", NULL};

// initialize root
void initialize(){
    root = (NODE *)malloc(sizeof(NODE));
    strcpy(root->name, "\\");
    root->type = 'D';
    root->parentPtr = root;
    root->siblingPtr = NULL;
    root->childPtr = NULL;
    cwd = root;
    printf("initialized\n");
}

// get user input command and pathway
void getCommand(){
    fgets(line, 128, stdin);    //get at most 128 char from stdin
    line[strlen(line)-1] = 0;   //take out \n at the end
    sscanf(line, "%s %s", command, pathname); //extract by format 
}

// identify the command
// return index of the command in cmd[]
int identifyCommand(){
    int i = 0;
    while(cmd[i]){
        if (!strcmp(command, cmd[i])){
            return i;
        }
        i++;
    }
    return -1;
    //printf("identify done\n");
}

// clear all global string
void clearGlobalStr(){
    // clear all string
    strcpy(pathname, "");
    strcpy(dname, "");
    strcpy(bname, "");
    strcpy(path, "");
    strcpy(line, "");
    strcpy(command, "");
}

// execute the command
void executeCommand(int cmdExecute){
    switch(cmdExecute){
        case 0: 
            mkdir(pathname, dname, bname, root, cwd);
            break;
        case 1:
            rmdir(pathname, root, cwd);
            break;
        case 2:
            ls(pathname, root, cwd);
            break;
        case 3:
            printf("intial cwd = %s\n", cwd->name);
            cwd = cd(pathname, root, cwd);
            printf("final cwd = %s\n", cwd->name);
            break;
        case 4:
            pwd(path, root, cwd);
            printf("%s", path);
            break;
        case 5:
            creat(pathname, dname, bname, root, cwd);
            break;
        case 6:
            rm(pathname, dname, bname, root, cwd);
            break;
        case 7:
            save(root);
            break;
        case 8:
            reload(root, cwd);
            cwd = root;
            break;
        case 9:
            menu();
            break;
        case 10:
            quit(root);
            q = 0;
            break;
        default: 
            printf("invalid command %s\n", command);
    }
    clearGlobalStr();
}

int main(){
    int cmdIndex;

    initialize();
    printf("type 'menu' to see all available command\n");
    while(q){
        getCommand();
        cmdIndex = identifyCommand();
        executeCommand(cmdIndex);
    }

    return 0;
}

