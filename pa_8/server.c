// This is the echo SERVER server.c
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>

#include <sys/socket.h>
#include <netdb.h>

#define  MAX 256

// Define variables:
struct sockaddr_in  server_addr, client_addr, name_addr;
struct hostent *hp;

int  mysock, client_sock;              // socket descriptors
int  serverPort;                     // server port number
int  r, length, n;                   // help variables

char command[MAX];
char arg[MAX];

char *servercmd[8] = {"get", "put", "ls", "cd", "pwd", "mkdir", "rmdir", "rm"};

// Server initialization code:

int server_init(char *name)
{
    printf("==================== server init ======================\n");   
    // get DOT name and IP address of this host

    printf("1 : get and show server host info\n");
    hp = gethostbyname(name);
    if (hp == 0){
        printf("unknown host\n");
        exit(1);
    }
    printf("    hostname=%s  IP=%s\n",
               hp->h_name,  inet_ntoa(*(long *)hp->h_addr));
  
    //  create a TCP socket by socket() syscall
    printf("2 : create a socket\n");
    mysock = socket(AF_INET, SOCK_STREAM, 0);
    if (mysock < 0){
        printf("socket call failed\n");
        exit(2);
    }

    printf("3 : fill server_addr with host IP and PORT# info\n");
    // initialize the server_addr structure
    server_addr.sin_family = AF_INET;                  // for TCP/IP
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);   // THIS HOST IP address  
    server_addr.sin_port = 0;   // let kernel assign port

    printf("4 : bind socket to host info\n");
    // bind syscall: bind the socket to server_addr info
    r = bind(mysock,(struct sockaddr *)&server_addr, sizeof(server_addr));
    if (r < 0){
        printf("bind failed\n");
        exit(3);
    }

    printf("5 : find out Kernel assigned PORT# and show it\n");
    // find out socket port number (assigned by kernel)
    length = sizeof(name_addr);
    r = getsockname(mysock, (struct sockaddr *)&name_addr, &length);
    if (r < 0){
        printf("get socketname error\n");
        exit(4);
    }

    // show port number
    serverPort = ntohs(name_addr.sin_port);   // convert to host ushort
    printf("    Port=%d\n", serverPort);

    // listen at port with a max. queue of 5 (waiting clients) 
    printf("5 : server is listening ....\n");
    listen(mysock, 5);
    printf("===================== init done =======================\n");
}


main(int argc, char *argv[])
{
    char *hostname;
    char line[MAX], ans[MAX];
    int cmd_no, r, fd, filesize;
    struct stat fstat;

    if (argc < 2)
        hostname = "localhost";
    else
        hostname = argv[1];
 
    server_init(hostname); 

    // Try to accept a client request
    while(1){
        printf("server: accepting new connection ....\n"); 

        // Try to accept a client connection as descriptor newsock
        length = sizeof(client_addr);
        client_sock = accept(mysock, (struct sockaddr *)&client_addr, &length);
        if (client_sock < 0){
            printf("server: accept error\n");
            exit(1);
        }
        printf("server: accepted a client connection from\n");
        printf("-----------------------------------------------\n");
        printf("        IP=%s  port=%d\n", inet_ntoa(client_addr.sin_addr.s_addr),
                                            ntohs(client_addr.sin_port));
        printf("-----------------------------------------------\n");

        // Processing loop: newsock <----> client
        while(1){
            bzero(line, MAX);
            n = read(client_sock, line, MAX);
            if (n==0){
                printf("server: client died, server loops\n");
                close(client_sock);
                break;
            }
      
            // show the line string
            printf("server: read  n=%d bytes; line=[%s]\n", n, line);

            // get command and argument
            getInput(line);           
            
            // get command number
            cmd_no = isServerCmd();

            if(cmd_no == 0) // get
            {
                // send filesize
                // check file exists and also that it is a reg
                r = stat(arg, &fstat);
                if(r < 0 || !S_ISREG(fstat.st_mode))
                {
                    printf("Error: %s file doesn't exist\n", arg);
                    strcpy(line, "0");
                    n = write(client_sock, line, MAX);
                }
                else
                {
                    // get filesize to line
                    sprintf(line, "%d", fstat.st_size);
                    // send filesize to client
                    n = write(client_sock, line, MAX);

                    fd = open(arg, O_RDONLY);
                    n = read(client_sock, ans, MAX);
                
                    while(!strcmp(ans, "!!!READY!!!"))
                    {
                        // read from file to line
                        n = read(fd, line, MAX);
                        // send line through server
                        n = write(client_sock, line, MAX);
                        // read response from server
                        n = read(client_sock, ans, MAX);
                    }
                    close(fd);
                }
            }
            else if(cmd_no == 1) // put
            {
                // reponse to client
                strcpy(line, "server");
                n = write(client_sock, line, MAX);
                // read answer from client
                // first get filesize
                n = read(client_sock, ans, MAX);

                // set filesize
                filesize = atoi(ans);

                fd = open(arg, O_WRONLY | O_CREAT, 0644);

                // if server couldnot stat given path
                if(!filesize)
                    printf("filesize = 0\n");
                else
                {
                    strcpy(line, "!!!READY!!!");

                    // keep reading until filesize is 0
                    while(filesize > 0)
                    {   
                        // tell server that client is ready to receive new ans
                        n = write(client_sock, line, MAX);
                        // get new ans
                        n = read(client_sock, ans, MAX);
                        filesize -= n;
                        // write to file
                        n = write(fd, ans, MAX);
                    }
                    close(fd);
                }
            }
            else if(cmd_no == 2)    // ls
                myls();
            else if(cmd_no == 3)    // cd
                mycd();
            else if(cmd_no == 4)    // pwd
                mypwd();
            else if(cmd_no == 5)    // mkdir
                mymkdir();
            else if(cmd_no == 6)    // rmdir
                myrmdir();
            else if(cmd_no == 7)    // rm
                myrm();

            // send the finish response
            strcpy(line, "DONE"); 
            n = write(client_sock, line, MAX);

            printf("server: wrote n=%d bytes; ECHO=[%s]\n", n, line);
            printf("server: ready for next request\n");
        }
    }
}


// get command and argument
int getInput(char *line)
{
    char *token;

    // tokenize
    token = strtok(line, " ");

    // get command
    strcpy(command, token);

    // get argument if exist
    token = strtok(NULL, " ");
    if(token)
        strcpy(arg, token);
    else
    {
        bzero(arg, MAX);
        return 1;
    }

    // error if more than 1 argument is enter
    token = strtok(NULL, " ");
    if(token)
    {
        printf("ERROR: only accept 1 argument at most\n");
        return 0;
    }

    return 1;
}

// return command number
int isServerCmd()
{
    int i;

    for(i = 0; i < 8; i++)
    {
        if(!strcmp(command, servercmd[i]))
            return i;
    }

    return -1;
}

// ls to display ls -l format
void ls_file(char* path)
{
    struct stat fstat;
    int i, r, n, len;
    char time[64], temp[128], line[1024];
    char *c = "xwrxwrxwr";

    if((r = lstat(path, &fstat)) < 0)
    {
        printf("Error: can't stat %s\n", path);
        return;
    }

    // print type
    if(S_ISREG(fstat.st_mode))
        line[0] = '-';
    if(S_ISDIR(fstat.st_mode))
        line[0] = 'd';
    if(S_ISLNK(fstat.st_mode))
        line[0] = 'l';

    // print permissions
    for(i = 8; i >= 0; i--)
    {
        if(fstat.st_mode & (1 << i))
            line[9-i] = c[i];
        else   
            line[9-i] = '-';
    }

    // get time in ls -l format
    strcpy(temp, ctime(&fstat.st_ctime));
    strncpy(time, &temp[4], 12);
    time[12] = 0;

    // print other info
    sprintf(temp, " %2d %2d %2d  %s %5d  %s",
        fstat.st_nlink , fstat.st_gid,
        fstat.st_uid, time,
        fstat.st_size, basename(path));

    strcat(line, temp);

    i = 0;
    len = strlen(line);
    while(len > 0)
    {
        n = write(client_sock, &line[256*i], 256);
        len -= 256;
        i++;
    }

    bzero(line,1024);
}


// myls function to display local directory content
void myls()
{
    struct stat fstat;
    char cwd[1024], path[1024], npath[1024];;
    char *ptr;
    int r;
    struct dirent *ep;
    DIR *dp;

    // if no argument make it cwd
    if(arg == NULL || !strcmp(arg, ""))
    {
        ptr = getcwd(cwd, 1024);
        // if getcwd() errors
        if(ptr == NULL)
        {
            printf("Error getting cwd\n");
            printf("errno %d, %s\n", errno, strerror(errno));
            return;
        }
        strcpy(path, cwd);
    }
    else 
        strcpy(path, arg);

    // check file exists to lls
    r = stat(path, &fstat);
    if(r < 0)
    {
        printf("Error: %s directory doesn't exist\n", path);
        return;
    }

    // check that arg is a dir file
    if(!S_ISDIR(fstat.st_mode))
    {
        printf("Error: %s isn't a directory\n", path);
        return;
    }

    dp = opendir(path);
    while(ep = readdir(dp))
    {
        strcpy(npath, path);
        if(npath[strlen(npath)-1] != '/')
            strcat(npath, "/");
        strcat(npath, ep->d_name);
        ls_file(npath);
    }
}


// cd function
void mycd()
{
    int r, status;
    struct stat fstat;

    // exit if no argument
    if(arg == NULL || !strcmp(arg, ""))
    {
        printf("Error: command lcd need an argument\n");   
        return;
    }

    // check file exists to lcd
    r = stat(arg, &fstat);
    if(r < 0)
    {
        printf("Error: %s directory doesn't exist\n", arg);
        return;
    }

    // check that arg is a dir file
    if(!S_ISDIR(fstat.st_mode))
    {
        printf("Error: %s isn't a directory\n", arg);
        return;
    }

    if((status = chdir(arg)) < 0)
        printf("chdir() error: errno %d, %s<p>", errno, strerror(errno));
}

void mypwd()
{
    char* ptr;
    char cwd[1024];
    int len, i;

    if(arg[0] != 0)
    {
        printf("Error: command lpwd doesn't take in argument\n");
        return;
    }

    ptr = getcwd(cwd, 1024);
    // if getcwd() errors
    if(ptr == NULL)
    {
        printf("Error getting cwd\n");
        printf("errno %d, %s\n", errno, strerror(errno));
        return;
    }   

    i = 0;
    len = strlen(cwd);
    while(len > 0)
    {
        n = write(client_sock, &cwd[256*i], 256);
        len -= 256;
        i++;
    }
}

void mymkdir()
{
    if(arg[0] == 0)
    {
        printf("Error: command lmkdir need an argument\n");
        return;
    }

    if((mkdir(arg,0755)) < 0)
        printf("mkdir() error: errno %d, %s<p>", errno, strerror(errno));
}

void myrmdir()
{
    if(arg[0] == 0)
    {
        printf("Error: command lrmdir need an argument\n");
        return;
    }

    if((rmdir(arg)) < 0)
        printf("rmdir() error: errno %d, %s<p>", errno, strerror(errno));
}


void myrm()
{
    if(arg[0] == 0)
    {
        printf("Error: command lrm need an argument\n");
        return;
    }

    if((unlink(arg)) < 0)
        printf("unlink() error: errno %d, %s<p>", errno, strerror(errno));
}

