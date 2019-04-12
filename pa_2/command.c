// Anan Kraisakdawat
// 11503146
// Cpts 360 Lab 2

#include "node.h"
#include "command.h"

// check whether pathname is absolute?
bool isAbsPathname(char* pathname){
    if (pathname[0] == '/')
        return true;
    return false;
}

// breakup pathname to dirname and basename
void getDname_Bname(char *pathname, char *dname, char *bname){
    int i = strlen(pathname) - 1;

    while(pathname[i] != '/' && i > -1)
        i--;

    if (i > -1)
        strncpy(dname, pathname, i);
    dname[i] = '\0';
    strcpy(bname, &pathname[i+1]);
}

// search if directory path exist
NODE* searchDirName(char *pathname, char *dname, NODE* root, NODE* cwd){
    NODE *mnode;
    int i = 0, m = 0;
    int n = strlen(dname);
    char sname[64];

    //start character in dirname depends 
    //on if dirname is absolute or relative
    if(isAbsPathname(pathname)){ 
        mnode = root;
        i++;
        m++;
    } 
    else{
        mnode = cwd;
        //printf("mkdir cwd = %s\n", cwd->name);
    }

    // return if there is no dirname
    if (strcmp(dname, "") == 0)
        return mnode;
    
    // search all path that's mentioned in dirname
    strcpy(sname, "");
    while(i <= n){
        //printf("pathname2 = %s\n", pathname);
        //printf("dname2 = %s\n", dname);
        if(dname[i] != '/' && dname[i] != '\0'){
            //printf("sname = %s\n", sname);
        }
        else{
            //enter the dir
            strncpy(sname, dname + m, i-m);
            sname[i-m] = '\0';

            m = i+1;

            //printf("enter else: sname = %s\n", sname);

            //see if the children is empty first
            if(!mnode->childPtr){
                printf("ERROR: the directory '%s' doesn't exist\n", sname);
                return NULL;
            }

            mnode = mnode->childPtr;
            
            //search all siblings
            while(strcmp(mnode->name, sname) != 0){
                //printf("enter while loop\n");
                if(!mnode->siblingPtr){
                    printf("ERROR: the directory '%s' doesn't exist\n", sname);
                    return NULL;
                }
                //printf("assign mnode to sibling\n");
                mnode = mnode->siblingPtr;
            }
            //printf("Search sibling completed\n");

            //check type of node
            if (mnode->type != 'D'){
                printf("ERROR: '%s' is not a directory\n", sname);
                return NULL;
            }
            //printf("check node type completed\n");
            strcpy(sname, "");
        }
        i++;
    }
    return mnode;
}

// search if the name already exist
bool NoBaseNameExist(NODE *mnode, char *bname){
    NODE *cnode;
    cnode = mnode->childPtr;

    while(cnode){
        //printf("cnode = %s & basename = %s\n", cnode->name, bname);
        if(!strcmp(cnode->name, bname)){
            // directory with same name exist
            printf("ERROR: directory with %s name already exist\n", bname);
            return false;
        }
        cnode = cnode->siblingPtr;
    }
    return true;
}

// add directory or file to the tree
void addNode(NODE *mnode, char *bname, char type){
    NODE *newNode;
    NODE *nullNode;
    nullNode = mnode;
    
    // create new node
    newNode = (NODE *)malloc(sizeof(NODE));
    strcpy(newNode->name, bname);
    newNode->type = type;
    newNode->parentPtr = mnode;
    newNode->siblingPtr = NULL;
    newNode->childPtr = NULL;
    //printf("new node created\n");

    // looking for null through the siblingPtr to insert
    if(!nullNode->childPtr)
        nullNode->childPtr = newNode;
    else{
        nullNode = nullNode->childPtr;

        while(nullNode->siblingPtr){
            nullNode = nullNode->siblingPtr;
        }

        // insert new node
        nullNode->siblingPtr = newNode;
    }

    //printf("new node inserted\n");        
}

// remove a node from tree
void rmNode(NODE *mnode){
    NODE* prNode = mnode->parentPtr;
    NODE* oldersibling = mnode->parentPtr->childPtr;

    //printf("begin remove, mnode->name = %s\n", mnode->name);
    // assign mnode younger sibling to parent's child node
    // if mnode doesn't have oldersibling
    if(!strcmp(oldersibling->name, mnode->name)){
        prNode->childPtr = mnode->siblingPtr;
        //printf("remove at parent's child\n");
    }
    else{
        // find older sibling of mnode
        while (strcmp(oldersibling->siblingPtr->name, mnode->name)){
            oldersibling = oldersibling->siblingPtr;
        }
        // assign younger sibling to older sibling
        oldersibling->siblingPtr = mnode->siblingPtr;
        //printf("remove at older sibling\n");
    }    
    // free mnode
    free(mnode);
    //printf("free node\n");

}

// save tree in preorder traversal
void preorderSave(FILE *fp, NODE* root, NODE* node){
    char getPath[64];

    if (node == NULL) 
        return; 
    
    // first save data of node
    pwd(getPath, root, node);
    
    //not taking / at the end off it its a root
    if(strcmp(getPath, "/"))
        getPath[strlen(getPath)-1] = 0;
    
    //write to file
    fprintf(fp, "%c %s\n", node->type, getPath); 
  
    // then recur on left sutree 
    preorderSave(fp, root, node->childPtr);  
  
    // now recur on right subtree 
    preorderSave(fp, root, node->siblingPtr);
}

// empty current tree
void clearTree(NODE* node){
    if (node == NULL)
        return;

    // delete subtree first
    clearTree(node->childPtr);
    clearTree(node->siblingPtr);

    // then delete the node
    //printf("delete node: %s\n", node->name);
    free(node);
}

void printPreorder(NODE* node) 
{ 
    if (node == NULL) 
        return; 
    
    /* first print data of node */
    printf("%s ", node->name); 
  
    /* then recur on left sutree */
    printPreorder(node->childPtr);  
  
    /* now recur on right subtree */
    printPreorder(node->siblingPtr); 
}  

// make directory in given pathname
void mkdir(char *pathname, char *dname, char *bname, NODE* root, NODE* cwd){
    NODE *mnode;

    //printf("enter mkdir\n");

    //(1) breakup pathname to dirname and basename
    getDname_Bname(pathname, dname, bname);
    //printf("breakup dname, bname completed\n");

    //(2) search for dirname node
    mnode = searchDirName(pathname, dname, root, cwd);
    if(!mnode)
        return;
    //printf("search for dirname completed, mnode = %s\n", mnode->name);
    
    //(3) insert basename to directory
    if(!NoBaseNameExist(mnode, bname))
        return;
    //printf("No basename exist\n");
    addNode(mnode, bname, 'D');

    //printf("exit mkdir\n");

    printPreorder(root);
    printf("\n");

    printf("insert new directory completed\n");
}

// remove directory from tree file
void rmdir(char *pathname, NODE* root, NODE* cwd){
    NODE* mnode;

    //printf("enter rmdir\n");
    // search for pathname
    mnode = searchDirName(pathname, pathname, root, cwd);
    if(!mnode)
        return;
    //printf("search dirname completed\n");

    // check if dir is empty
    if(mnode->childPtr){
        printf("directory %s isn't empty, only empty directory can be remove\n", mnode->name);
        return;
    }
    //printf("check dir empty completed\n");

    // remove node
    rmNode(mnode);
    printf("remove node completed\n");

    //printf("exit rmdir\n");
    printPreorder(root);
    printf("\n");
}

// list content of directory
void ls(char *pathname, NODE* root, NODE* cwd){
    NODE* mnode;

    // ls cwd if pathname not specify
    if(!strcmp(pathname, ""))
        mnode = cwd;
    else{
        // search through all path directory
        mnode = searchDirName(pathname, pathname, root, cwd);
        if(!mnode)
            return;
    }

    // directory is empty
    if(!mnode->childPtr)
        printf("the directory %s is empty\n", mnode->name);
    else{
        mnode = mnode->childPtr;

        while(mnode){
            printf("[%c %s] ", mnode->type, mnode->name);
            mnode = mnode->siblingPtr;
        }
        printf("\n");
    }

}

// change cwd to pathname or root if no pathname
NODE* cd(char *pathname, NODE* root, NODE* cwd){
    NODE* mnode;

    // cwd = root if no path
    if(!strcmp(pathname, ""))
        return root;

    // find pathname node and check it's a DIR  
    mnode = searchDirName(pathname, pathname, root, cwd);
    if(!mnode)
        return cwd;

    return mnode;
}

// return the pathname of CWD in variable path
void pwd(char* path, NODE* root, NODE* cwd){
    // root case
    if(!strcmp(cwd->name, root->name)){
        strcpy(path, "/");
        return;
    }

    // recursive strcat
    pwd(path, root, cwd->parentPtr);
    strcat(path, cwd->name);
    strcat(path, "/");
}

// make file in a given directory
void creat(char *pathname, char *dname, char *bname, NODE* root, NODE* cwd){
    NODE *mnode;

    //printf("enter creat\n");

    //(1) breakup pathname to dirname and basename
    getDname_Bname(pathname, dname, bname);
    //printf("breakup dname, bname completed\n");

    //(2) search for dirname node
    mnode = searchDirName(pathname, dname, root, cwd);
    if(!mnode)
        return;
    //printf("search for dirname completed\n");
    
    //(3) insert basename to directory
    if(!NoBaseNameExist(mnode, bname))
        return;
    //printf("No basename exist\n");
    addNode(mnode, bname, 'F');

    //printf("exit creat\n");

    printPreorder(root);
    printf("\n");

    printf("insert file completed\n");
}

// remove file from tree
void rm(char *pathname, char *dname, char* bname, NODE* root, NODE* cwd){
    NODE* mnode;

    //printf("enter rm\n");

    //breakup pathname to dirname and basename
    getDname_Bname(pathname, dname, bname);
    //printf("breakup dname, bname completed\n");

    // search for pathname
    mnode = searchDirName(pathname, dname, root, cwd);
    if(!mnode)
        return;
    //printf("search dirname completed\n");

    // check if the directory is empty
    if(mnode->childPtr)
        mnode = mnode->childPtr;
    else{
        printf("%s directory is empty\n", mnode->name);
        return;
    }

    // get to the file
    while(mnode){
        if(!strcmp(mnode->name, bname)){
            break;
        }
        if(!mnode->siblingPtr){
            printf("no %s file exist in %s directory\n", bname, mnode->parentPtr->name);
            return;
        }
        mnode = mnode->siblingPtr;
    }

    // check type
    if(mnode->type != 'F'){
        printf("only directory %s exist, not a file\n", mnode->name);
        return;
    }

    // remove node
    rmNode(mnode);
    printf("remove node completed\n");

    //printf("exit rm\n");
    printPreorder(root);
    printf("\n");
}

// save tree
void save(NODE* root){
    FILE *fp;
    NODE *mnode = root;
    char filename[128];

    printf("enter filename to save: ");
    fgets(filename, 128, stdin);
    filename[strlen(filename)-1] = 0;

    fp = fopen(filename, "w+");

    preorderSave(fp, root, mnode);

    fclose(fp);
    printf("save successful\n");
}

// reload tree
void reload(NODE* root, NODE* cwd){
    FILE *fp;
    char filename[128], line[128];
    char type[2];
    char path[64], dname[64], bname[64];

    // clear current tree
    clearTree(root->childPtr);
    root->childPtr = NULL;
    cwd = root;

    // clear strings
    strcpy(path, "");
    strcpy(dname, "");
    strcpy(bname, "");

    // user input filename to reload
    printf("enter filename: ");
    fgets(filename, 128, stdin);
    filename[strlen(filename)-1] = 0;
    //printf("getname completed\n");

    fp = fopen(filename, "r");
    //printf("fopen completed\n");
    // error if input name is not found
    if (fp == NULL){
        perror("Error: find not found\n");
        return;
    }
    //printf("perror not detected\n");
    // get each line of input file
    while(fgets(line, 128, fp)!= NULL){
        line[strlen(line)-1] = 0;
        sscanf(line, "%s %s", type, path);

        //printf("type = %s, path = %s\n", type, path);
        
        // if not a root (do nothing if its a root)
        if(strcmp(path, "/")){
            // mkdir for type D
            if(!strcmp(type, "D")){
                mkdir(path, dname, bname, root, cwd);
            }
            // creat for type F
            else if(!strcmp(type, "F")){
                creat(path, dname, bname, root, cwd);
            }
            // error else
            else{
                printf("Error: type of path %s is neither 'D' or 'F'\n", path);
            }
        }
         
        // clear strings
        strcpy(path, "");
        strcpy(dname, "");
        strcpy(bname, "");
    }
    fclose(fp);
}

// display all available command
void menu(){
    printf("----------------------------------------------------------\n");
    printf(" mkdir  rmdir  cd  ls  pwd  creat  rm  save  reload  quit\n");
    printf("----------------------------------------------------------\n");
}

// save and terminate program
void quit(NODE* root){
    save(root);
    printf("program terminated\n");
}