// The echo client client.c
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

#define MAX 256

// Define variables
struct hostent *hp;              
struct sockaddr_in  server_addr; 

int server_sock, r;
int SERVER_IP, SERVER_PORT;

char command[MAX];
char arg[MAX];

char *localcmd[7] = {"lcat", "lls", "lcd", "lpwd", "lmkdir", "lrmdir", "lrm"};
char *servercmd[8] = {"get", "put", "ls", "cd", "pwd", "mkdir", "rmdir", "rm"};

// clinet initialization code

int client_init(char *argv[])
{
    printf("======= clinet init ==========\n");

    printf("1 : get server info\n");
    hp = gethostbyname(argv[1]);
    if (hp==0){
        printf("unknown host %s\n", argv[1]);
        exit(1);
    }

    SERVER_IP   = *(long *)hp->h_addr;
    SERVER_PORT = atoi(argv[2]);

    printf("2 : create a TCP socket\n");
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock<0){
        printf("socket call failed\n");
        exit(2);
    }

    printf("3 : fill server_addr with server's IP and PORT#\n");
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = SERVER_IP;
    server_addr.sin_port = htons(SERVER_PORT);

    // Connect to server
    printf("4 : connecting to server ....\n");
    r = connect(server_sock,(struct sockaddr *)&server_addr, sizeof(server_addr));
    if (r < 0){
        printf("connect failed\n");
        exit(1);
    }

    printf("5 : connected OK to \007\n"); 
    printf("---------------------------------------------------------\n");
    printf("hostname=%s  IP=%s  PORT=%d\n", 
            hp->h_name, inet_ntoa(SERVER_IP), SERVER_PORT);
    printf("---------------------------------------------------------\n");

    printf("========= init done ==========\n");
}

main(int argc, char *argv[ ])
{
    int n, cmd_no, filesize;
    int fd, r;
    char line[MAX], ans[MAX];
    struct stat fstat;

    if (argc < 3){
        printf("Usage : client ServerName SeverPort\n");
        exit(1);
    }

    client_init(argv);
    // sock <---> server
    printf("******************  MENU  ******************\n");
    printf("*  get  put  ls  cd  pwd  mkdir  rmdir  rm *\n");
    printf("* lcat      lls lcd lpwd lmkdir lrmdir lrm *\n");
    printf("* press ENTER without any command to quit  *\n");
    printf("********************************************\n");
    while (1){
        printf("input a line : ");
        bzero(line, MAX);                // zero out line[ ]
        bzero(ans, MAX);
        fgets(line, MAX, stdin);         // get a line (end with \n) from stdin

        line[strlen(line)-1] = 0;        // kill \n at end
        if (line[0]==0)                  // exit if NULL line
            exit(0);

        if(!getInput(line))              // get command and argument
            exit(1);                     // exit if fault argument number

        if(isLocalCmd(&cmd_no))         // if it is a local command
        {
            switch(cmd_no)
            {
                case 0:
                    lcat();
                    break;
                case 1:
                    lls();
                    break;
                case 2:
                    lcd();
                    break;
                case 3:
                    lpwd();
                    break;
                case 4:
                    lmkdir();
                    break;
                case 5:
                    lrmdir();
                    break;
                case 6:
                    lrm();
                    break;
            }
        }
        else if(isServerCmd(&cmd_no))
        {
            // send input line to server
            n = write(server_sock, line, MAX);

            if(cmd_no == 0)     // get
            {   
                // read answer from server
                // first get filesize
                n = read(server_sock, ans, MAX);

                // set filesize
                filesize = atoi(ans);

                // if server couldnot stat given path
                if(!filesize)
                {
                    printf("Error: Server couldn't stat given path\n");
                    continue;
                }

                fd = open(arg, O_WRONLY | O_CREAT, 0644);
                strcpy(line, "!!!READY!!!");

                // keep reading until filesize is 0
                while(filesize > 0)
                {   
                    // tell server that client is ready to receive new ans
                    n = write(server_sock, line, MAX);
                    // get new ans
                    n = read(server_sock, ans, MAX);
                    filesize -= n;
                    // write to file
                    n = write(fd, ans, MAX);
                }
                // send finish response
                strcpy(line, "DONE");
                n = write(server_sock, line, MAX);
                close(fd);
                printf("download %s completed\n", arg);
                continue;
            }
            else if(cmd_no == 1)    // put
            {
                // response from server
                n = read(server_sock, ans, MAX);
                // send filesize
                // check file exists and also that it is a reg
                r = stat(arg, &fstat);
                if(r < 0 || !S_ISREG(fstat.st_mode))
                {
                    printf("Error: %s file doesn't exist\n", arg);
                    strcpy(line, "0");
                    n = write(server_sock, line, MAX);
                    continue;
                }
                
                // get filesize to line
                sprintf(line, "%d", fstat.st_size);
                // send filesize to server
                n = write(server_sock, line, MAX);

                fd = open(arg, O_RDONLY);
                n = read(server_sock, ans, MAX);
                
                while(!strcmp(ans, "!!!READY!!!"))
                {
                    // read from file to line
                    n = read(fd, line, MAX);
                    // send line through server
                    n = write(server_sock, line, MAX);
                    // read response from server
                    n = read(server_sock, ans, MAX);
                }
                close(fd);
                printf("upload %s completed\n", arg);
                continue;
            }
            else    // all other commands are process by the server
            {
                // get server response
                n = read(server_sock, ans, MAX);
                strcpy(line, "!!!READY!!!");

                while(strcmp(ans, "DONE"))
                {
                    printf("%s\n", ans);
                    // tell server that client is ready to receive new ans
                    //n = write(server_sock, line, MAX);
                    // get new ans from server
                    n = read(server_sock, ans, MAX);
                }
                printf("%s completed\n", command);
            }
        }
        else
        {  
            printf("ERROR: %s command doesn't exist\n", command);
            exit(1);
        }
    }
}

// get command and argument
int getInput(char *line)
{
    char *token, temp[MAX];

    strcpy(temp, line);
    // tokenize
    token = strtok(temp, " ");

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

// check if its a local command
// return command number in cmd_no
int isLocalCmd(int *cmd_no)
{
    int i;

    for(i = 0; i < 7; i++)
    {
        if(!strcmp(command, localcmd[i]))
        {
            *cmd_no = i;
            return 1;
        }
    }

    return 0;
}

// check if its a server command
// return command number in cmd_no
int isServerCmd(int *cmd_no)
{
    int i;

    for(i = 0; i < 8; i++)
    {
        if(!strcmp(command, servercmd[i]))
        {
            *cmd_no = i;
            return 1;
        }
    }

    return 0;
}


// lcat function to display local file content
void lcat()
{
    struct stat fstat;
    int fd, n, i;
    int r;
    char buf[1024];

    // exit if no argument
    if(arg == NULL || !strcmp(arg, ""))
    {
        printf("Error: command lcat need an argument\n");   
        return;
    }
  
    // check file exists to cat
    r = stat(arg, &fstat);
    if(r < 0)
    {
        printf("Error: %s file doesn't exist\n", arg);
        return;
    }

    // check that arg is a reg file
    if(!S_ISREG(fstat.st_mode))
    {
        printf("Error: %s isn't a reg file\n", arg);
        return;
    }

    fd = open(arg, O_RDONLY);

    // read and output 1024 bytes to the web each time
    while(n = read(fd, buf, 1024))
    {
        for(i = 0; i < n; i++)
            putchar(buf[i]);
    }
    close(fd);
    printf("\n");
}

// lls to display ls -l format
void lls_file(char* path)
{
    struct stat fstat;
    int i, r;
    char time[64], temp[64];
    char *c = "xwrxwrxwr";

    if((r = lstat(path, &fstat)) < 0)
    {
        printf("Error: can't stat %s\n", path);
        return;
    }

    // print type
    if(S_ISREG(fstat.st_mode))
        printf("%c", '-');
    if(S_ISDIR(fstat.st_mode))
        printf("%c", 'd');
    if(S_ISLNK(fstat.st_mode))
        printf("%c", 'l');

    // print permissions
    for(i = 8; i >= 0; i--)
    {
        if(fstat.st_mode & (1 << i))
            printf("%c", c[i]);
        else   
            printf("%c", '-');
    }

    // get time in ls -l format
    strcpy(temp, ctime(&fstat.st_ctime));
    strncpy(time, &temp[4], 12);
    time[12] = 0;

    // print other info
    printf(" %2d %2d %2d  %s %5d  %s",
        fstat.st_nlink , fstat.st_gid,
        fstat.st_uid, time,
        fstat.st_size, basename(path));

    printf("\n");
}


// lls function to display local directory content
void lls()
{
    struct stat fstat;
    char cwd[1024], path[1024], npath[1024];
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
        lls_file(npath);
    }
    printf("\n");
}

// local cd function
void lcd()
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


void lpwd()
{
    char* ptr;
    char cwd[1024];

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

    printf("%s\n", cwd);
}


void lmkdir()
{
    if(arg[0] == 0)
    {
        printf("Error: command lmkdir need an argument\n");
        return;
    }

    if((mkdir(arg,0755)) < 0)
        printf("mkdir() error: errno %d, %s<p>", errno, strerror(errno));
}

void lrmdir()
{
    if(arg[0] == 0)
    {
        printf("Error: command lrmdir need an argument\n");
        return;
    }

    if((rmdir(arg)) < 0)
        printf("rmdir() error: errno %d, %s<p>", errno, strerror(errno));
}


void lrm()
{
    if(arg[0] == 0)
    {
        printf("Error: command lrm need an argument\n");
        return;
    }

    if((unlink(arg)) < 0)
        printf("unlink() error: errno %d, %s<p>", errno, strerror(errno));
}
