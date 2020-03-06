/*
Author: Faris Alotaibi

Description: An interactive shell. 
*/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <linux/limits.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

#define MAX_ARGS 1024

void ErrorHandle(void);
void argStringFormat(char* argstring);
int tokenProcess(char** argv, char* argstring);
int pipeHandle(char** argv, int argc);
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
	char* prompt = NULL;

	while(1)
	{
		int argc = 0;
		//Have to clear argv here, dont want it all messy from previous cmnds
		argv = memset(argv, 0, MAX_ARGS);

		if((prompt != NULL) || ((prompt = getenv("PS1")) != NULL))
			printf("%s: ",prompt);
		else
		{
			if(getcwd(workindirect,PATH_MAX-1) == NULL)ErrorHandle();
			printf("%s: ",workindirect);
		}
		
		if((fgets(argstring, PATH_MAX, stdin)) == NULL) ErrorHandle();
		argStringFormat(argstring);

		argc = tokenProcess(argv, argstring);

		//Block to handle built-in usage possibilities
		if(strcmp(argv[0], "cd") == 0 && argc < 3)
			cdExecute(argv, workindirect);
		else if(strcmp(argv[0], "cd") == 0 && argc>= 3)
			printf("cd: too many arguments\n");
		else if((strcmp(argv[0], "exit")) == 0 && (argc < 3))
			exit(EXIT_SUCCESS);
		else if((strcmp(argv[0], "exit")) == 0 && (argc >= 3))
			printf("exit: too many arguments\n");
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

int pipeHandle(char** argv, int argc)
{
	for(int i=0; i< argc; i++)
	{
		if(strmp(argv[i], "<") == 0)
		{
			return 0
		}else if(strcmp(argv[i], ">") == 0)
		{
			return 0
		}else if(argv[i][strlen(argv[i])-1] == '>')
		{
			return 0
		}else if(argv[i][0] == '<')
		{
			return 0
		}else if(strcmp(argv[i], ">>") == 0)
		{

		}else return -1;
	}
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
	int pid = fork(), waitstat = 0;
	if(pid == 0)
	{
		execvp(argv[0], argv);
		if(errno) ErrorHandle();
	}else
	{
		wait(&waitstat); 
		//All good parents wait for their children!
	}
}