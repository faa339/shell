/*
Author: Faris Alotaibi

Description: An interactive shell.
Exit the shell by typing
	exit 
*/

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

void ErrorHandle(void);
void getPrompt(char* prompt);
void argStringFormat(char* argstring);
int tokenProcess(char** argv, char* argstring);
int redirHandle(char* redirpath, int redirOp);
void cdExecute(char** argv, char* workindirect);
void argExec(char** argv, int argc);

int main()
{
	errno = 0;
	//Setting up all string variables here to avoid unnecessary 
	//reallocation
	char argstring[PATH_MAX];
	char *token = NULL;
	char workindirect[PATH_MAX];
	char **argv = (char**) malloc(MAX_ARGS * sizeof(char*));
	if (argv == NULL) 
	{
		printf("Malloc failure\n");
		exit(EXIT_FAILURE);
	}
	int argc;
	char* prompt = NULL;
	if(signal(SIGINT,SIG_IGN) == SIG_ERR)
	{
		printf("Failed to establish SIGINT handler\n");
		exit(EXIT_FAILURE);
	};
	if(signal(SIGQUIT,SIG_IGN) == SIG_ERR)
	{
		printf("Failed to establish SIGQUIT handler\n");
		exit(EXIT_FAILURE);
	};

	while(1)
	{
		argc = 0;
		argv = memset(argv, '\0', MAX_ARGS * sizeof(char*));
		//Have to clear argv here, dont want it all messy from previous cmnds

		//Check if we were given a custom prompt string
		if((prompt != NULL) ||((prompt = getenv("PS1")) != NULL))
			printf("%s: ",prompt);
		else
		{ 
			if(getcwd(workindirect,PATH_MAX-1) == NULL)
			{
				ErrorHandle();
				exit(EXIT_FAILURE);
			}
			printf("%s: ",workindirect);
		}
		
		if((fgets(argstring, PATH_MAX, stdin)) == NULL) 
		{
			ErrorHandle();
			continue;
		}	
		//If SIGINT or SIGQUIT was sent, the string will be empty
		//Don't even bother processing it, just continue
		argStringFormat(argstring);
		if(strcmp(argstring, "") == 0) continue;

		argc = tokenProcess(argv, argstring);

		if(argc < 0)
		{
			ErrorHandle();
			printf("%d\n", argc);
			continue;
		}

		//If-elses to handle built-in usage possibilities
		if((strcmp(argv[0], "cd") == 0) && argc < 3)
			cdExecute(argv, workindirect);
		else if((strcmp(argv[0], "cd") == 0) && argc>= 3)
			printf("cd: Too many arguments\n");
		else if((strcmp(argv[0], "exit") == 0) && (argc < 3))
			exit(EXIT_SUCCESS);
		else if((strcmp(argv[0], "exit") == 0) && (argc >= 3))
			printf("exit: Too many arguments\n");
		else
			argExec(argv, argc);
	}
} 

void ErrorHandle(void)
{
	//Display the error message
	//Since we want to stay in the shell, we don't exit(EXIT_FAILURE)
	if(errno)
		perror("Error");
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

int redirHandle(char* redirpath, int redirOp)
{
	//Given an fd to redirect to/from and an op
	//Redirect > < 2>
	int fd = 0;
	if(redirOp == STDOUT_FILENO)
	{
		fd = open(redirpath,O_WRONLY|O_CREAT, 0666);
		if(fd < 0) ErrorHandle();

		if(dup2(fd, STDOUT_FILENO) < 0) ErrorHandle();
		close(fd);
		return 0;
	}else if(redirOp == STDIN_FILENO)
	{
		fd = open(redirpath,O_RDONLY|O_CREAT, 0666);
		if(fd < 0) ErrorHandle();
		
		if(dup2(fd, STDIN_FILENO) < 0) ErrorHandle();
		close(fd);
		return 0;
	}else if(redirOp == STDERR_FILENO)
	{
		fd = open(redirpath,O_WRONLY|O_CREAT, 0666);
		if(fd < 0) ErrorHandle();
		
		if(dup2(fd, STDERR_FILENO) < 0) ErrorHandle();
		close(fd);
		return 0;
	}else if(redirOp == O_APPEND)
	{
		fd = open(redirpath,O_RDWR|O_APPEND|O_CREAT, 0666);
		if(fd < 0) ErrorHandle();
		
		if(dup2(fd, STDOUT_FILENO) < 0) ErrorHandle();
		close(fd);
		return 0;
	}else return -1;
}

void cdExecute(char** argv, char* workindirect)
{
	//Func to handle changing directories & changing our environment string
	//to reflect the change
	int chtest = 0;
	if (argv[1])
		chtest = chdir(argv[1]);
	else
		chtest = chdir(".");

	if(chtest<0) ErrorHandle();

	if ((getcwd(workindirect,PATH_MAX-1)) == NULL) ErrorHandle();
}

void argExec(char** argv, int argc)
{
	//Execute any cmnds that arent exit or cd here 
	int pid = fork(), j=0, waitstat=0, redirstat=0;
	if(pid == 0)
	{
		if(signal(SIGINT,SIG_DFL) == SIG_ERR)
		{
			printf("Failed to establish Signal handler\n");
			exit(EXIT_FAILURE);
		}
		if(signal(SIGQUIT,SIG_DFL) == SIG_ERR)
		{
			printf("Failed to establish Signal handler\n");
			exit(EXIT_FAILURE);	
		}
		
		char** execargs = (char**) malloc(argc*sizeof(char*));
		memset(execargs, '\0' , argc * sizeof(char*));
		//Check for any redirection; if none, execargs == argv
		for(int i=0; i<argc; i++)
		{
			if(strcmp(argv[i], "<") == 0)
			{
				redirstat = redirHandle(argv[i+1], STDIN_FILENO);
				if(redirstat < 0)
				{
					printf("Redirection error: could not redirect\n");
					exit(EXIT_FAILURE);
				}
				i+=2;
			}else if(strcmp(argv[i], ">") == 0)
			{
				redirstat = redirHandle(argv[i+1], STDOUT_FILENO);
				if(redirstat < 0)
				{
					printf("Redirection error: could not redirect\n");
					exit(EXIT_FAILURE);
				}
				i+=2;
			}else if(strcmp(argv[i], "2>") == 0)
			{
				redirstat = redirHandle(argv[i+1], STDERR_FILENO);
				if(redirstat < 0)
				{
					printf("Redirection error: could not redirect\n");
					exit(EXIT_FAILURE);
				}
				i+=2;
			}else if(strcmp(argv[i], ">>") == 0)
			{
				redirstat = redirHandle(argv[i+1], O_APPEND);
				if(redirstat < 0)
				{
					printf("Redirection error: could not redirect\n");
					exit(EXIT_FAILURE);
				}
				i+=2;
			}else execargs[j++] = argv[i];
		}
		execvp(execargs[0], execargs);
		if(errno) 
		{
			ErrorHandle();
			exit(EXIT_FAILURE);
		}
	}else
	{
		wait(&waitstat); 
		if(!(WIFEXITED(waitstat))) ErrorHandle();
		//All good parents wait for their children!
	}
}