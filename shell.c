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
void freeArgv(char** argv);
void cdExecute(char** argv, char* workindirect);
int tokenProcess(char** argv, char* argstring);
void argExec(char ** argv, int argc);
void argStringFormat(char* argstring);

int main()
{
	errno = 0;
	char argstring[PATH_MAX];
	char *token;
	char workindirect[PATH_MAX];
	while(1)
	{
		int argc = 0;
		char* prompt;
		if((prompt != NULL) || ((prompt = getenv("PS1")) != NULL))
			printf("%s: ",prompt);
		else
		{
			if(getcwd(workindirect,PATH_MAX-1) == NULL)ErrorHandle();
			printf("%s: ",workindirect);
		}
		
		char **argv = (char**) malloc(MAX_ARGS * sizeof(char*));
		
		if((fgets(argstring, PATH_MAX, stdin)) == NULL) ErrorHandle();
		argStringFormat(argstring);

		if((strcmp(argstring, "exit")) == 0)
			exit(EXIT_SUCCESS);
		
		argc = tokenProcess(argv, argstring);
		if(strcmp(argv[0], "cd") == 0 && argc < 3)
			cdExecute(argv, workindirect);
		else if(strcmp(argv[0], "cd") == 0 && argc>= 3)
			printf("cd: too many arguments\n");
		else
			argExec(argv, argc);
	}
} 

void ErrorHandle(void)
{
	if(errno)
	{
		perror("Error");
	}
}

void argStringFormat(char* argstring)
{
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
	int argcount = 0;
	char* token = strtok(argstring, " ");
	while(token  != NULL)
	{
		argv[argcount++] = token;
		token = strtok(NULL, " ");
	}
	return argcount;
}

void cdExecute(char** argv, char* workindirect)
{
	int chtest;
	if (argv[1])
		chtest = chdir(argv[1]);
	else
		chtest = chdir(".");

	if(chtest<0) ErrorHandle();

	if ((getcwd(workindirect,PATH_MAX-1)) == NULL) ErrorHandle();
}

void argExec(char** argv, int argc)
{
	int childpid = fork(), waitstat = 0;
	if(childpid == 0)
	{
		execvp(argv[0], argv);
		if(errno) ErrorHandle();
	}else
	{
		wait(&waitstat);
	}
}