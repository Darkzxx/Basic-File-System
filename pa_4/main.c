// Anan Kraisakdawat
// 11503146
// Cpts 360 Lab 4

#include <stdio.h>       // for printf()
#include <stdlib.h>      // for exit()
#include <string.h>      // for strcpy(), strcmp(), etc.
#include <libgen.h>      // for basename(), dirname()
#include <fcntl.h>       // for open(), close(), read(), write()
#include <stdbool.h>

// for stat syscalls
#include <sys/stat.h>
#include <unistd.h>

// for opendir, readdir syscalls
#include <sys/types.h>
#include <dirent.h>

// define BLKSIZE
#define BLKSIZE 4096

// main
int main(int argc, char* argv[])
{
    if(argc < 3)
    {
        printf("usage: myrcp arg1 arg2\n");
        exit(1);
    }

    return myrcp(argv[1], argv[2]);
}


// myrcp : check f1 and f2 then copy f1 to f2 
int myrcp(char* f1, char* f2)
{
    struct stat f1_stat, f2_stat;
    int r1 = stat(f1, &f1_stat);
    int r2 = stat(f2, &f2_stat);

    // 1. stat f1;   if f1 does not exist ==> exit.
    if(r1 < 0)
    {
        printf("Error: %s does not exist\n", f1);
        exit(1);
    }

    // f1 exists: reject if f1 is not REG, DIR or LNK file
    if(!S_ISREG(f1_stat.st_mode) && !S_ISLNK(f1_stat.st_mode) && !S_ISDIR(f1_stat.st_mode))
    {
        printf("Error: %s is not REG, DIR or LNK file\n", f1);
        return 1;
    }

    // 2. stat f2;   reject if f2 exists and is not REG, DIR or LNK file
    if(r2 > -1)
    {
        if(!S_ISREG(f2_stat.st_mode) && !S_ISLNK(f2_stat.st_mode) && !S_ISDIR(f2_stat.st_mode))
        {
            printf("Error: %s is not REG, DIR or LNK file\n", f2);
            return 1;
        }
    }

    // if f1 is reg then depending on f2 we can do
    // cp file to file or file to dir
    if(S_ISREG(f1_stat.st_mode))
    {
        if(r2 < 0 || (r2 > -1 && S_ISREG(f2_stat.st_mode)))
            return cpf2f(f1, f2);
        else
            return cpf2d(f1, f2);
    }

    // if f1 is a dir then we only accept f2 as dir or f2 doesn't exist
    if(S_ISDIR(f1_stat.st_mode))
    {
        if(r2 > -1 && !S_ISDIR(f2_stat.st_mode))
            return 1;
        
        if(r2 < 0)
        {
            if(mkdir(f2, f1_stat.st_mode) < 0)
            {
                perror("mkdir error\n");
                return 1;
            }
            //printf("go to cpd2d from myrcp 1\n");
            return cpd2d(f1, f2, 1);
        }
        
        //printf("go to cpd2d from myrcp 0\n");
        return cpd2d(f1, f2, 0);
    }
}


// cp file to file
int cpf2f(char *f1, char *f2)
{
    struct stat stat_f1, stat_f2;
    int r1 = lstat(f1, &stat_f1);
    int r2 = stat(f2, &stat_f2);
    int f1d, f2d, n, total = 0;
    char buf[BLKSIZE];
    
    //printf("enter cpf2f\n");

    // if f2 exists
    if(r2 > -1)
    {
        //printf("enter f2 exists case\n");
        // 1. reject if f1 and f2 are the SAME file
        if((stat_f1.st_dev == stat_f2.st_dev) && (stat_f1.st_ino == stat_f2.st_ino))
        {
            printf("Error: %s and %s are the same file\n", f1, f2);
            return 1;
        }

        // 2. if f1 is LNK and f2 exists: reject
        if(S_ISLNK(stat_f1.st_mode))
        {
            printf("Error: %s is a link file\n", f1);
            return 1;
        }
        //printf("exit f2 exists case\n");
    }
    else
    {   
        //printf("enter f2 not exists case\n");
        // 3. if f1 is LNK and f2 does not exist: create LNK file f2 SAME as f1
        if(S_ISLNK(stat_f1.st_mode))
        {
            // hardlink if nlink > 0
            if(stat_f1.st_nlink > 0)
            {
                if(link(f1, f2) < 0)
                    perror("link failed\n");
            }
            // symlink otherwise
            else
            {
                if(symlink(f1, f2) < 0)
                    perror("symlink failed\n");
            }

            return 1;
        }
        //printf("exit f2 not exists case\n");
    }

    // open f1 for READ
    if((f1d = open(f1, O_RDONLY)) < 0)
    {
        printf("%s failed to open\n", f1);
        return 1;
    }
    // open f2 for WRITE
    if((f2d = open(f2, O_WRONLY | O_CREAT | O_TRUNC, stat_f1.st_mode)) < 0)
    {
        printf("%s failed to open\n", f2);
        return 1;
    }

    //printf("begin copy\n");
    while((n = read(f1d, buf, BLKSIZE)) > 0)
    {
        write(f2d, buf, n);
        total += n;
    }
    printf("total bytes copied = %d\n", total);
    close(f1d);
    close(f2d);
}


// copy file to dir
int cpf2d(char *f1, char* f2)
{   
    char* fname = basename(f1);
    char f2_fname[512];
    bool fnameInf2 = false;
    struct stat stat_f2_fname;
    struct dirent *ep;
    DIR *dp = opendir(f2);

    // get f2/x name
    if(f2[strlen(f2)-1] == '/')
    {
        //f2_fname = (char *)malloc(strlen(f2) + strlen(fname) + 1);
        strcpy(f2_fname, f2);
        strcat(f2_fname, fname);
    }
    else
    {
        //f2_fname = (char *)malloc(strlen(f2) + strlen(fname) + 2);
        strcpy(f2_fname, f2);
        strcat(f2_fname, "/");
        strcat(f2_fname, fname);
    }

    // find if f1 already exist in f2 
    while((ep = readdir(dp)))
    {
        if (!strcmp(ep->d_name, fname))
        {
            fnameInf2 = true;
            break;
        }
    }

    // if f1 already exist
    if(fnameInf2)
    {
        if(stat(f2_fname, &stat_f2_fname) < 0)
        {
            printf("Error: can't stat %s\n", f2_fname);
            //free(f2_fname);
            return 1;
        }
        
        if(S_ISREG(stat_f2_fname.st_mode))
            cpf2f(f1, f2_fname);
        if(S_ISDIR(stat_f2_fname.st_mode))
            cpf2d(f1, f2_fname);
    }
    // if f1 doesn't exist
    else
        cpf2f(f1, f2_fname);
    
    //free(f2_fname);
}


// copy dir to dir
int cpd2d(char *f1, char *f2, int f2_i)
{
    bool nameinf2 = false;
    char f2_f1[512], f1_x[512];
    char f1_bname[512], f2_dname[512];
    struct stat stat_f1, stat_f2;
    struct stat stat_f2_f1, stat_f1_x;
    struct dirent *ep1, *ep2;
    DIR *dp1 = opendir(f1);
    DIR *dp2 = opendir(f2);

    //printf("f1 = %s    f2 = %s\n", f1, f2);

    if(stat(f1, &stat_f1) < 0)
    {
        printf("Error: can't stat %s\n", f1);
        return 1;
    }

    if(stat(f2, &stat_f2) < 0)
    {
        printf("Error: can't stat %s\n", f2);
        return 1;
    }

    // reject if f1 and f2 are the SAME file
    if((stat_f1.st_dev == stat_f2.st_dev) && (stat_f1.st_ino == stat_f2.st_ino))
    {
        printf("Error: %s and %s are the same directory\n", f1, f2);
        return 1;
    }
    
    // check if it's a subdirectory
    //f1_bname = (char *)malloc(sizeof(f1));
    strcpy(f1_bname, f1);
    //f2_dname = (char *)malloc(sizeof(f2));
    strcpy(f2_dname, f2);
    while(strcmp(f2_dname, "/") && strcmp(f2_dname, "."))
    {
        if(!strcmp(f2_dname, f1))
        {
            printf("Error: Can't copy to subdirectory\n");
            return 1;
        }

        strcpy(f2_dname, dirname(f2_dname));
    }
    

    // f2 in myrcp wasn't create before calling a.out
    if(!f2_i)
    {
        // get f2/x name
        if(f2[strlen(f2)-1] == '/')
        {
            //f2_f1 = (char *)malloc(strlen(f2) + strlen(basename(f1)) + 1);
            strcpy(f2_f1, f2);
            strcat(f2_f1, basename(f1));
        }
        else
        {
            //f2_f1 = (char *)malloc(strlen(f2) + strlen(basename(f1)) + 2);
            strcpy(f2_f1, f2);
            strcat(f2_f1, "/");
            strcat(f2_f1, basename(f1));
        }
        //printf("Done getting f2_f1\n");

        while((ep2 = readdir(dp2)))
        {   
            // check if dir with basename(f1) exists in f2
            if(!strcmp(ep2->d_name, basename(f1)))
            {
                printf("found same name ep2->d_name = %s\n", ep2->d_name);
                nameinf2 = true;
                break;
            }
        }

        //printf("Done getting nameinf2 = %d\n", nameinf2);
        // if f1 name is in f2
        // see if f1 name in f2 is a reg or dir
        if(nameinf2)
        {
            if(stat(f2_f1, &stat_f2_f1) < 0)
            {
                printf("Error: can't stat %s\n", f2_f1);
                //free(f2_f1);
                return 1;
            }

            // if it is reg then print error
            if(S_ISREG(stat_f2_f1.st_mode))
            {
                printf("cannot overwrite non-directory file '%s' with directory '%s'\n",
                    f2_f1, f1);
                //free(f2_f1);
                return 1;
            }

            // if it is dir then print error
            if(S_ISDIR(stat_f2_f1.st_mode))
            {
                printf("same filename already exist\n");
                return 1;
            }
        }
        // f1 name is not in f2
        else
        {
            // create dir f1 in f2
            if(mkdir(f2_f1, stat_f1.st_mode) < 0)
            {
                perror("mkdir error\n");
                return 1;
            }

            // get all files in f1
            while((ep1 = readdir(dp1)))
            {
                //printf("ep1->d_name = %s\n", ep1->d_name);
                if((!strcmp(ep1->d_name, ".")) || (!strcmp(ep1->d_name, "..")))
                    continue;

                // get f1/x name
                if(f1[strlen(f1)-1] == '/')
                {
                    //f1_x = (char *)malloc(strlen(f1) + strlen(ep1->d_name) + 1);
                    strcpy(f1_x, f1);
                    strcat(f1_x, ep1->d_name);
                }
                else
                {
                    //f1_x = (char *)malloc(strlen(f1) + strlen(ep1->d_name) + 2);
                    strcpy(f1_x, f1);
                    strcat(f1_x, "/");
                    strcat(f1_x, ep1->d_name);
                }
            
                // stat f1_x
                if(stat(f1_x, &stat_f1_x) < 0)
                {   
                    printf("Error: can't stat %s\n", f1_x);
                    //free(f1_x);
                    return 1;
                }

                // check f1_x reg or dir
                // if reg : cpf2d
                if(S_ISREG(stat_f1_x.st_mode))
                    cpf2d(f1_x, f2_f1);
            
                // if dir : cpd2d
                if(S_ISDIR(stat_f1_x.st_mode))
                {   
                    //printf("begin recursive\nf1_x = %s\nf2_f1 = %s\n", f1_x, f2_f1);
                    cpd2d(f1_x, f2_f1, 0);
                }
            }
        }
    }
    // f2 didn't exist before a.out was called
    else
    {
        // get all files in f1
        while((ep1 = readdir(dp1)))
        {
            //printf("ep1->d_name = %s\n", ep1->d_name);
            if((!strcmp(ep1->d_name, ".")) || (!strcmp(ep1->d_name, "..")))
                continue;

            // get f1/x name
            if(f1[strlen(f1)-1] == '/')
            {
                //f1_x = (char *)malloc(strlen(f1) + strlen(ep1->d_name) + 1);
                strcpy(f1_x, f1);
                strcat(f1_x, ep1->d_name);
            }
            else
            {
                //f1_x = (char *)malloc(strlen(f1) + strlen(ep1->d_name) + 2);
                strcpy(f1_x, f1);
                strcat(f1_x, "/");
                strcat(f1_x, ep1->d_name);
            }
            
            // stat f1_x
            if(stat(f1_x, &stat_f1_x) < 0)
            {   
                printf("Error: can't stat %s\n", f1_x);
                //free(f1_x);
                return 1;
            }

            // check f1_x reg or dir
            // if reg : cpf2d
            if(S_ISREG(stat_f1_x.st_mode))
                cpf2d(f1_x, f2);
            
            // if dir : cpd2d
            if(S_ISDIR(stat_f1_x.st_mode))
            {   
                //printf("begin recursive\nf1_x = %s\nf2_f1 = %s\n", f1_x, f2);
                cpd2d(f1_x, f2, 0);
            }
        }
    }
}
