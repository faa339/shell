/*
Author: Faris Alotaibi

Description: An interactive shell.
Exit the shell by typing
	exit 
*/
#define _POSIX_C_SOURCE 1
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <linux/limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>

#define MAX_ARGS 1024

void errorHandle(void);
void setDirectory(char* workindirect);
void argStringFormat(char* argstring);
int tokenProcess(char** argv, char* argstring);
int redirHandle(char* redirpath, int redirOp);
int builtInCheck(char ** argv, int argc);
void cdExecute(char** argv);
int createEargs(char** execargs, char** argv, int argc);
void argExec(char** argv, int argc);
void signalSetup(__sighandler_t handler);

int main()
{
	errno = 0;
	//Setting up all string variables here to avoid unnecessary reallocs
	char argstring[PATH_MAX];
	char workindirect[PATH_MAX];
	char **argv = (char**) malloc((MAX_ARGS * sizeof(char*))+1);
	if (argv == NULL) 
	{
		fprintf(stderr,"Malloc failure\n");
		exit(EXIT_FAILURE);
	}
	int argc;
	char* userprompt = getenv("PS1");
	//We want to ignore SIGINTS and SIGQUITS while in our shells main process
	signalSetup(SIG_IGN);
	while(1)
	{
		argc = 0;
		argv = memset(argv, 0, MAX_ARGS * sizeof(char*));
		//Have to clear argv here, dont want it all messy from previous cmnds

		//Check if we were given a custom prompt string
		if((userprompt != NULL))
			printf("%s: ",userprompt);
		else
		{   //get the current directory if PS1 not specified
			setDirectory(workindirect);
			printf("%s: ",workindirect);
		}
		
		if((fgets(argstring, PATH_MAX, stdin)) == NULL) 
		{
			errorHandle();
			continue;
		}	
		if(strcmp(argstring, "\n") == 0) continue;
		//Don't bother tokenizing a string with only \n, just continue
		//But do remove the \n that fgets adds to the end of an arg
		argStringFormat(argstring);

		argc = tokenProcess(argv, argstring);

		//Check if we were given cd or exit, else execute the arg
		if(builtInCheck(argv, argc) == 1)
			argExec(argv, argc);
	}
} 

void errorHandle(void)
{
	//Display the error message
	//Since we want to stay in the shell, we don't exit(EXIT_FAILURE)
	if(errno != 0)
	{
		perror("Error");
		errno = 0;
	}	
}

void setDirectory(char* workindirect)
{
	//Get our current directory
	if((getcwd(workindirect,PATH_MAX-1)) == NULL)
	{
		errorHandle();
		exit(EXIT_FAILURE);
	}
}

void argStringFormat(char* argstring)
{
	//Put a null character at the first \n seen (end of input for us)
	for(int i=0; i<PATH_MAX; i++)
	{
		if(argstring[i] == '\n')
		{
			argstring[i] = '\0';
			break;
		}
	}
}

int tokenProcess(char** argv, char* argstring)
{
	//Turn argstring into a series of tokens and place it into argv
	//Then return an argc 
	int argcount = 0;
	char* token = strtok(argstring, " ");
	while(token  != NULL)
	{ 	
		argv[argcount++] = token;
		token = strtok(NULL, " ");
	}

	return argcount;
}

int builtInCheck(char** argv, int argc)
{
	//Execute cd or exec
	//Return 1 if the command wasnt cd or exec
	if((strcmp(argv[0], "cd") == 0) && argc < 3)
	{
		cdExecute(argv);
		return 0;
	}
	else if((strcmp(argv[0], "cd") == 0) && argc>= 3)
	{	
		fprintf(stderr,"cd: Too many arguments\n");
		return 0;
	}
	else if((strcmp(argv[0], "exit") == 0) && (argc < 3))
	{
		exit(EXIT_SUCCESS);
	}
	else if((strcmp(argv[0], "exit") == 0) && (argc >= 3))
	{
		fprintf(stderr,"exit: Too many arguments\n");
		return 0;
	}
	else
		return 1;
}

void signalSetup(__sighandler_t handler)
{
	//Set up signals in here based on value of handler
	struct sigaction new_action;
	new_action.sa_handler = handler;
	
	if((sigaction(SIGINT, &new_action,NULL) < 0))
	{
		errorHandle();
		exit(EXIT_FAILURE);
	}
	if((sigaction(SIGQUIT, &new_action, NULL)) < 0)
	{
		errorHandle();
		exit(EXIT_FAILURE);
	}
}

int redirHandle(char* redirpath, int redirOp)
{
	//Given an fd to redirect to/from and an op
	//Redirecting >, <, 2>, and >> only
	int fd = 0;
	if(redirOp == STDOUT_FILENO)
	{
		fd = open(redirpath,O_WRONLY|O_CREAT, 0666);
		if(fd < 0) errorHandle();

		if(dup2(fd, STDOUT_FILENO) < 0) errorHandle();
		close(fd);
		return 0;
	}else if(redirOp == STDIN_FILENO)
	{
		fd = open(redirpath,O_RDONLY|O_CREAT, 0666);
		if(fd < 0) errorHandle();
		
		if(dup2(fd, STDIN_FILENO) < 0) errorHandle();
		close(fd);
		return 0;
	}else if(redirOp == STDERR_FILENO)
	{
		fd = open(redirpath,O_WRONLY|O_CREAT, 0666);
		if(fd < 0) errorHandle();
		
		if(dup2(fd, STDERR_FILENO) < 0) errorHandle();
		close(fd);
		return 0;
	}else if(redirOp == O_APPEND)
	{
		fd = open(redirpath,O_WRONLY|O_APPEND|O_CREAT, 0666);
		if(fd < 0) errorHandle();
		
		if(dup2(fd, STDOUT_FILENO) < 0) errorHandle();
		close(fd);
		return 0;
	}else return -1;
}

void cdExecute(char** argv)
{
	//Func to handle changing directories & changing our environment string
	int chtest = 0;
	if (argv[1]) // if we have a location to change to
		chtest = chdir(argv[1]);
	else
		chtest = chdir(".");

	if(chtest<0) errorHandle();
}

int createEargs(char** execargs, char** argv, int argc)
{ 
	//Constructs array to be passed to execvp, checking for redir characters
	//If theres no redirection, execargs == argv
	int eargs=1, redirstat=0;
	execargs[0] = argv[0];
	//Iterate over argv looking for any redirection characters
	for(int i=1; i<argc; i++)
	{
		if(((strcmp(argv[i], "<") == 0) && argv[i+1]))
		{
			redirstat = redirHandle(argv[i+1], STDIN_FILENO);
			if(redirstat < 0)
			{
				fprintf(stderr,"Redirection error: could not redirect\n");
				exit(EXIT_FAILURE);
			}
			i++;
		}else if((strcmp(argv[i], ">") == 0) && argv[i+1])
		{
			redirstat = redirHandle(argv[i+1], STDOUT_FILENO);
			if(redirstat < 0)
			{
				fprintf(stderr,"Redirection error: could not redirect\n");
				exit(EXIT_FAILURE);
			}
			i++;
		}else if((strcmp(argv[i], "2>") == 0) && argv[i+1])
		{
			redirstat = redirHandle(argv[i+1], STDERR_FILENO);
			if(redirstat < 0)
			{
				fprintf(stderr,"Redirection error: could not redirect\n");
				exit(EXIT_FAILURE);
			}
			i++;
		}else if((strcmp(argv[i], ">>") == 0) && argv[i+1])
		{
			redirstat = redirHandle(argv[i+1], O_APPEND);
			if(redirstat < 0)
			{
				fprintf(stderr,"Redirection error: could not redirect\n");
				exit(EXIT_FAILURE);
			}
			i++;
		}else execargs[eargs++] = argv[i];
	}
	return eargs; //argc but for our exec args string
}

void argExec(char** argv, int argc)
{
	//Execute any cmnds that arent exit or cd here
	int execount=0, waitstat=0, exectest = 0;
	int pid = fork();
	if(pid == 0)
	{
		//Change our signals back to default in child for early exits
		signalSetup(SIG_DFL);
		char** execargs = (char**) malloc((argc * sizeof(char*))+1);
		if(execargs < 0)
		{
			errorHandle();
			exit(EXIT_FAILURE);
		}
		execargs = memset(execargs, 0 , argc * sizeof(char*));
		execount = createEargs(execargs, argv, argc);
		//Manually null terminating our string to be safe
		execargs[execount] = NULL; 
		exectest = execvp(execargs[0], execargs);
		if(exectest < 0) 
		{
			errorHandle();
			exit(EXIT_FAILURE);
		}
	}else
	{
		wait(&waitstat); 
		if(!(WIFEXITED(waitstat))) errorHandle();
		//All good parents wait for their children!
	}
}

