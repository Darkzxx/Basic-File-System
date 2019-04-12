/******************************************************************************
	Author: Tewit Srisomboon
	Date: 11/30/2017
	Description: A user shell program
******************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

int extractArgv(char *line, char *delim, char argv[32][256]);
void processCommand(char *line);
void execCommand(char *line); 

int main(int argc, char *argv[])
{
	char username[32];
	char homeDir[32];

	strcpy(username, argv[1]);

	getcwd(homeDir);

	// Ignore terminal interupts: Ctrl+c, Ctrl+d
	signal(2,1); 

	// Shell loop
	while(1) {
		int uid, pid, status, argc;
		char cwd[256];
		char line[256];
		char argv[32][256];

		uid = getuid();
		getcwd(cwd);

		// Get input line
		do {
			printf("%s@Wanix %s $ ", username, cwd); 
			gets(line);

			//parse userinput line into argument variables "cmd arg1 arg2 ..."
			argc = extractArgv(line, " ", argv);

			//printf("You Entered: %s\n", line);

		} while (argc < 1);

		// Handle cd
		if (strcmp(argv[0], "cd") == 0) {

			if (argc > 1) { 						// cd path
				chdir(argv[1]);		
			}				
			else if (strcmp(cwd, homeDir) != 0) {	// cd
				chdir(homeDir);
			}

			continue;
		}

		// Handle logout, exit
		if (strcmp(argv[0], "logout") == 0 || 
			strcmp(argv[0], "exit") == 0) {
			exit(0);
		}

		// Fork a child process to execute sh command
		pid = fork();
		if (pid < 0) {
			exit(1);
		}

		if (pid) { 
			pid = wait(&status);

			// Sh resumes here
			printf("SH: Child P%d died with exit status %x\n", pid, status);
		} else {
			// Sh's child starts here
			processCommand(line);
		}
	}
	
	return 0;
}
 

int extractArgv(char *line, char *delim, char argv[32][256])
{
	int argc = 0;
	char temp[256];
	char *token;

	strcpy(temp, line);

	//Exract string token in argv
	token = strtok(temp, delim);
	while(token) {
		strcpy(argv[argc], token);
		argc++;
		token = strtok(NULL, delim); 
	}

	return argc;
}

void processCommand(char *line)
{
	int pid, status, pd[2];
	int length;
	char *head, *tail;		// head|tail

	// Check for empty line
	if (line == 0 || line[0] == '\0' || strcmp(line, "") == 0) {
		exit(0);
	}

	length = strlen(line);

	// Get head 
	head = strtok(line, "|");
	printf("Head=[%s]\n", head);
 
	// Get tail if any
	if (strlen(head) < length) {
		tail = line + strlen(head) + 1;
		while(tail[0] == ' '){			//skip space
			tail++;
		}
		printf("Tail=[%s]\n", tail);
  	}
	else {
		tail = 0;
	}

	// Exeucute head only
	if (tail == 0 || tail[0] == '\0') {
		execCommand(head);
		exit(-1);
	}

	// Exeucute Head|Tail
	if (pipe(pd) < 0) {			// Create Pipe
		prints("Pipe failed\n");
	}

	// Create a child process as pipe writer
	pid = fork();
	if (pid < 0) {
		prints("Failed to fork a process for pipe!\n");
	}

	//        --------------------------------------          --------------------------------------
	//           CLOSED | <---READ pd[0]-----           \/           ------READ pd[0]----->    
	// Child  --------------------------------------    /\    --------------------------------------  Parent
	//				     ----WRITE pd[1]---->                        <----WRITE pd[1]---- | CLOSED 
	//        --------------------------------------          --------------------------------------

	if (pid == 0) {		// Child as pipre writer
		// Close READ channel in pipe
		close(pd[0]);		

		// Replace STDOUT with WRITE channel
		close(STDOUT);		
		dup(pd[1]);			

		// Execute Head
		execCommand(head);

		exit(-1);
	} else {			// Parent as pipe reader		
		// Close WRITE channel
		close(pd[1]);

		// Replace STDIN with READ channel
		close(STDIN);
		dup(pd[0]);

		// Recursively extends the pipe
		processCommand(tail);

		// Failed if reaches here
		exit(-1);
	}
}

void execCommand(char *line)
{

	int i = 1, j, argc;
	char argv[32][256];
	char *c;
	char *file;

	if (line == 0 || line[0] == '\0') 
		return;
	
	argc = extractArgv(line, " ", argv); 

	// Check if line contains a redirection symbol
	while(i < argc) {

		c = argv[i];
		file = argv[i + 1]; // LHS filename

		if (strcmp(c, "<") == 0) {						// Replace STDIN with the file
			close(STDIN);
			if (open(file, O_RDONLY) < 0) {
				printf("Redirection Error!\n");
			}

			argc = i; 
			break;  

		} else if (strcmp(c, ">") == 0) {				// Replace STDOUT with file
			close(STDOUT);
			if (open(file, O_WRONLY | O_CREAT) < 0){
				printf("Redirection Error!\n");
			}

			argc = i;
			break;

		} else if (strcmp(c, ">>") == 0) {				// Replace STDOUT with file
			close(STDOUT);
			if (open(file, O_WRONLY | O_APPEND | O_CREAT) < 0) {
				printf("Redirection Error!\n");
			}

			argc = i;
			break;
		}
		i++;
	}

	// Rebuild line before execution
	strcpy(line, argv[0]);
 
	for (j = 1; j < i; j++) { 
		strcat(line, " "); strcat(line, argv[j]);
	}
 	
 	exec(line);

 	return;
}
