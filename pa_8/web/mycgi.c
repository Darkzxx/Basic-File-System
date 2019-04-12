#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>

#define MAX 10000
typedef struct {
    char *name;
    char *value;
} ENTRY;

ENTRY entry[MAX];
int fail = 0;

int identifyCommand(char *command);
void forkchild(char *cmd, char *arg[], char *env[]);
void cat(char *arg);
void ls(char *arg);

main(int argc, char *argv[], char *env[]) 
{
  int i, m, r;
  char cwd[128];
  char *cmd, *arg[2]; 

  m = getinputs();    // get user inputs name=value into entry[ ]
  getcwd(cwd, 128);   // get CWD pathname

  printf("Content-type: text/html\n\n");
  printf("<p>pid=%d uid=%d cwd=%s\n", getpid(), getuid(), cwd);

  printf("<H1>Echo Your Inputs</H1>");
  printf("You submitted the following name/value pairs:<p>");
 
  for(i=0; i <= m; i++){
    printf("%s = %s<p>", entry[i].name, entry[i].value);
    arg[i] = 0;
    if(i == 0){
      cmd = malloc(strlen("/bin/") + strlen(entry[i].value) + 1);
      strcpy(cmd, "/bin/");
      strcat(cmd, entry[i].value);
    }

    if(strcmp(entry[i].value, "")){
      arg[i] = entry[i].value;
    }

  }
  arg[i] = 0;
  printf("<p>");

  /*****************************************************************
   Write YOUR C code here to processs the command
         mkdir dirname
         rmdir dirname
         rm    filename
         cat   filename
         cp    file1 file2
         ls    [dirname] <== ls CWD if no dirname
  *****************************************************************/
  r = identifyCommand(arg[0]);
  printf("<p>------------------ OUTPUT ----------------");
  printf("<p>");

  // mkdir, rmdir, rm, cp
  if(r > -1 && r < 4)
  {
    forkchild(cmd, arg, env);
    if(!fail)
      printf("%s OK<p>", arg[0]);
    else{
      printf("errno %d, %s<p>", errno, strerror(errno));
      fail = 0;
    }
  }
  // cat, ls
  else if (r >= 4)
  {
    // only accept argument from filename1
    if (arg[2] != 0)
      printf("Error: %s only accepts filename1<p>", cmd);
    else{
      if (r == 4)
        cat(arg[1]);
      else if (r == 5)
        ls(arg[1]);
    }
  }
  else
    printf("Command %s doesn't exist<p>", cmd);

  // create a FORM webpage for user to submit again 
  printf("</title>");
  printf("</head>");
  printf("<body bgcolor=\"#FFFFFF\" link=\"#330033\" leftmargin=8 topmargin=8");
  printf("<p>------------------ DO IT AGAIN ----------------\n");
  
  printf("<FORM METHOD=\"POST\" ACTION=\"http://cs360.eecs.wsu.edu/~kraisakdawat/cgi-bin/mycgi.bin\">");

  //------ NOTE : CHANGE ACTION to YOUR login name ----------------------------
  //printf("<FORM METHOD=\"POST\" ACTION=\"http://cs360.eecs.wsu.edu/~YOURNAME/cgi-bin/mycgi.bin\">");
  
  printf("Enter command : <INPUT NAME=\"command\"> <P>");
  printf("Enter filename1: <INPUT NAME=\"filename1\"> <P>");
  printf("Enter filename2: <INPUT NAME=\"filename2\"> <P>");
  printf("Submit command: <INPUT TYPE=\"submit\" VALUE=\"Click to Submit\"><P>");
  printf("</form>");
  printf("------------------------------------------------<p>");

  printf("</body>");
  printf("</html>");
}

// identify the command
// return index of the command in cmd[]
int identifyCommand(char *command){
    char *cmd[] = {"mkdir", "rmdir", "rm", "cp", "cat", "ls"};
    int i = 0;
    
    while(cmd[i]){
        if (!strcmp(command, cmd[i]))
            return i;
        i++;
    }
    return -1;
}

// fork child to execute commands
void forkchild(char *cmd, char *arg[], char *env[])
{
  int pid, status;

  pid = fork();

  // fork may fail
  if(pid < 0){
    perror("fork failed<p>");
    exit(1);
  }

  // parent execute
  if(pid){
    pid = wait(&status);
    printf("child exit status %d<p>", status);
  }
  else{
    execve(cmd, arg, env);
    fail = -1;
  }
}

// cat function to display file content
void cat(char *arg)
{
  struct stat fstat;
  int fd, n, i;
  int r;
  char buf[1024];

  // exit if no argument
  if(arg == NULL || !strcmp(arg, ""))
    return;
  
  // check file exists to cat
  r = stat(arg, &fstat);
  if(r < 0)
  {
    printf("Error: %s file doesn't exist<p>", arg);
    return;
  }

  // check that arg is a reg file
  if(!S_ISREG(fstat.st_mode))
  {
    printf("Error: %s isn't a reg file<p>", arg);
    return;
  }

  fd = open(arg, O_RDONLY);

  // read and output 1024 bytes to the web each time
  while(n = read(fd, buf, 1024))
  {
    for(i = 0; i < n; i++)
    {
      if(buf[i] == '\r' || buf[i] == '\n')
        printf("<br>");
      else if(buf[i] == ' ')
        printf("&nbsp;");
      else if(buf[i] == '\t')
        printf("&emsp;");
      else if(buf[i] == '<')
        printf("&lt");
      else if(buf[i] == '>')
        printf("&gt");
      else
        putchar(buf[i]);
    }
  }
  close(fd);
  printf("<p>");
}

// ls function to display directory content
void ls(char *arg)
{
  struct stat fstat;
  char cwd[1024], path[1024];
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
      printf("Error getting cwd<p>");
      printf("errno %d, %s<p>", errno, strerror(errno));
      return;
    }
    strcpy(path, cwd);
  }
  else 
    strcpy(path, arg);

  // check file exists to cat
  r = stat(path, &fstat);
  if(r < 0)
  {
    printf("Error: %s directory doesn't exist<p>", path);
    return;
  }

  // check that arg is a dir file
  if(!S_ISDIR(fstat.st_mode))
  {
    printf("Error: %s isn't a directory<p>", path);
    return;
  }

  dp = opendir(path);
  while(ep = readdir(dp))
    printf("%s  |  ", ep->d_name);
  printf("<p>");
}
    return;
  }

  // check that arg is a dir file
  if(!S_ISDIR(fstat.st_mode))
  {
    printf("Error: %s isn't a directory<p>", path);
    return;
  }

  dp = opendir(path);
  while(ep = readdir(