// Anan Kraisakdawat
// 11503146
// Cpts 360 Lab 3

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

char line[1024], cmdcpy[64];
char *commandLine;
bool quit = false;
char* home;

char *cmd[] = {"cd", "exit", NULL};

/*********** All available commands' function *********/

// format commands to ***ptr
void formatText(int cmdc, char* line, char *CMD[64][64]){
    char *tok = strtok(line, "|");
    char *save[64];
    int i = cmdc, j=0, k=0;
    
    while(tok != NULL){    
        save[j] = tok;
        tok = strtok(0, "|");
        j++;
    }
    
    save[j] = NULL;
    j = 0;
    
    while(save[j] != NULL){
        tok = strtok(save[j], " ");
        while(tok != NULL){
            CMD[i][k] = tok;
            tok = strtok(0, " ");
            k++;
        }
        CMD[i][++k] = NULL;
        k = 0;
        j++;
        i++;
    }
    
    *CMD[i] = NULL;
    i = 0;
    k = 0;
    
    while(*CMD[i] != NULL){
        while(CMD[i][k] != NULL){
            printf("CMD[%d][%d] = %s\n", i, k, CMD[i][k]);
            k++;
        }
        k=0;
        i++;
    }
}

// change directory to argument or home if no argument
void cd(){
    char *argument;
    int fail = -1;

    commandLine = strtok(line, " ");
    //move commandLine to arg part
    commandLine = strtok(0, " ");
    // check if argument exists
    if(commandLine){
        argument = commandLine;
        commandLine = strtok(0, " ");
        // check if more than 1 argument is given
        // return if it is
        if(commandLine){
            printf("Error: More than one argument is given for cd\n");
            return;
        }
    }
    else{
        // use home if no argument
        argument = home;
    }

    fail = chdir(argument);

    if(fail)
        printf("Error: pathname %s is not found\n", argument); 
}

void execCommand(char *head, char *env[]){
    char *arg[256], command[64];
    char *s, *filename;
    int i = 0, redir = -1;

    // get command and argument
    s = strtok(head, " ");
    //printf("head = %s\n", head);

    // identify io_redirection
    while(s){
        arg[i] = s;

        if(!strcmp(s, "<")){
            s = strtok(0, " ");
            filename = s;
            redir = 0;
        }
        else if(!strcmp(s, ">")){
            s = strtok(0, " ");
            filename = s;
            redir = 1;
        }
        else if(!strcmp(s, ">>")){
            s = strtok(0, " ");
            filename = s;
            redir = 2;
        }

        if(redir > -1){
            i--;
        }

        s = strtok(0, " ");
        i++;
    }
    arg[i] = 0;

    /*for(int n = 0; arg[n]; n++){
        printf("arg[%d] = %s\n", n, arg[n]);
    }*/

    // io_redirect
    if(!redir){
        close(0);
        open(filename, O_RDONLY);
    }
    else if(redir == 1){
        close(1);
        open(filename, O_WRONLY | O_CREAT, 0644);
    }
    else if(redir == 2){
        close(1);
        open(filename, O_WRONLY | O_APPEND | O_CREAT);
    }

    // get command to input
    strcpy(command, "/bin/");
    strcat(command, arg[0]);

    //printf("command = %s\n", command);

    // execute
    execve(command, arg, env);
    printf("ERROR: Couldn't find command %s\n", arg[0]);
    exit(1);
}

// forkChild to process other commands
void forkChild(char *L, char *env[]){
    int pid, status;
    int pd[2];
    char *head, *tail;

    // get head
    //printf("L = %s\n", L);
    head = strtok(L, "|");
    //printf("L = %s\n", L);

    tail = strtok(0, "\0");
    //printf("tail = %s\n", tail);

    if(!tail){
        //printf("enter no pipe\n");
        pid = fork();

        // fork may fail
        if(pid < 0){
            perror("fork failed\n");
            exit(1);
        }

        // parent execute
        if(pid){
            pid = wait(&status);
            printf("child exit status %d\n", status);
        }
        else{
            execCommand(head, env);
        }
    }
    // else pipe process
    else{
        //printf("enter pipe\n");;

        // create pipe
        pipe(pd);
        // fork child
        pid = fork();

        // fork may fail
        if(pid < 0){
            perror("fork failed\n");
            exit(1);
        }
        
        // child as pipe writer
        if(!pid){
            close(pd[0]);

            close(1);
            dup(pd[1]);
            

            // process head
            execCommand(head, env);
            //close(pd[1]);
            
        }
        // parent as pipe reader
        else{
            close(pd[1]);
            close(0);
            dup(pd[0]);
            
            execCommand(tail, env);
            //close(pd[0]); 
        }
    }
}

/*****************************************************/

/*********** get, identify and process input *********/

// get user input command and argument
void getCommand(){
    int i = 0;;
    // loop until we get command
    while(1){
        printf("myShell>> ");
        fgets(line, 1024, stdin);    //get at most 256 char from stdin
        line[strlen(line)-1] = 0;   //take out \n at the end
    
        // check line input
        if (strcmp(line, "") && strcmp(line, "\n")){
            while(line[i] != ' ' && line[i] != '\0')
                i++;
            strncpy(cmdcpy, line, i);
            break;
        }
    }
}

// identify the command
// return index of the command in cmd[]
int identifyCommand(){
    int i = 0;
    while(cmd[i]){
        if (!strcmp(cmdcpy, cmd[i])){
            return i;
        }
        i++;
    }
    return -1;
}

// process command from input index
void processCommand(int cmdIndex, char *env[]){
    // handle simple commands
    if(cmdIndex > -1){
        switch(cmdIndex){
            case 0:
                cd();
                break;
            case 1:
                quit = true;
                exit(1);
                break;
        }
    }
    // fork child process for other commands
    else{
        forkChild(line, env);
    }
}
/*****************************************************/

/******************      main      *******************/

int main(int argc, char *argv[], char *env[]){
    int cmdIndex = -1;
    home = getenv("HOME");

    while(!quit){
        getCommand();
        cmdIndex = identifyCommand();
        processCommand(cmdIndex, env);
    }
}