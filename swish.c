/* CSE 306: Sea Wolves Interactive SHell */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/select.h>
#include "env_vars.h"
#include "env_vars.c"
#define _GNU_SOURCE

// Assume no input line will be longer than 1024 bytes
#define MAX_INPUT 1024
#define MAX_PATHS 1024
#define MAX_ARGS  128
#define MAX_DIR	  1024
#define MAX_HISTORY 50

int fileExists(char *path);
char *parsePath(char *command);
int runProgram(char *cmd, char **envp);
char ***parseArgs(char *command);
char **parseArgs2(char *command);
int addJobToList(int pid, char *command, int status, int forground);
int removeJobPID(int pid);
int removeJob(int index);
int updateStatusOfJob(int index, int status);
void printJobs();
int getIndexOfJob(int pid);
int getNewStat(int pid);
int is_ready(int fd);
void writeOutHistory(char **history, int start, int end);
FILE *readInHistory(char **history);
int debug = 0;
int fgId = -1;

typedef struct {
	int 	pid;
	char 	*command;
	int 	status; // 0: running, 1: stopped, 2: done
	int 	forground;
} job;

job *jobs[100];
int 
main (int argc, char ** argv, char **envp) {
	//initEnvVars(envp);
	//printSysEnv();
	
	// parse arguments
	int argI = 0;
	for(argI = 0; argI < argc; argI++)
	{
		write(1, *(argv + argI), strlen(*(argv + argI)));
		write(1, "\n", 1);
		if(strcmp(*(argv + argI), "-d") == 0)
		{
			debug = 1;
		}
	}

	
  int finished = 0;
  char *prompt = "swish> ";
  char cmd[MAX_INPUT];


  //char *home = getValue("$HOME");

 // write(1, home, strlen(home));
 // write(1, "\n", 1);

  char *cwd = malloc(MAX_DIR);
  cwd = getcwd(cwd, MAX_DIR);
  char *prevcwd = malloc(MAX_DIR);
  prevcwd = getcwd(prevcwd, MAX_DIR);
  char *history[MAX_HISTORY];
  int histStart = 0;
  int histEnd = 0;
  int histIndex;
  for(histIndex = 0; histIndex < MAX_HISTORY; histIndex++)
  {
	history[histIndex] = malloc(sizeof(char) * MAX_INPUT );
  }
  
  FILE *file =readInHistory(history);
  
  	int i = 0;
	while(i != MAX_HISTORY)
	{
		// write line
		char temp[MAX_INPUT];
		int n = fscanf(file, "%s", temp);
		if(n <= 0)
		{
			break;
		}
		strcpy(history[i], temp);
		//fprintf(file, "\n");
		i++;
	}
	histEnd = i;
	
	fclose(file);

  while (!finished) {
    char *cursor;
    char last_char;
    int rv;
    int count;

    // Print the prompt
	//printf("[%s] ", cwd);
    rv = write(1, "[", 1);
	if (!rv) { 
      finished = 1;
      break;
    }
    rv = write(1, cwd, strlen(cwd));
	if (!rv) { 
      finished = 1;
      break;
    }
	rv = write(1, "] ", 2);
	if (!rv) { 
      finished = 1;
      break;
    }
	
    rv = write(1, prompt, strlen(prompt));
    
	if (!rv) { 
      finished = 1;
	   
      break;
    }
    int upArrow = 0;
	int downArrow = 0;
	int historyIndex = 0;
	char histBuff[2];
	
	
	
    // read and parse the input
    for(rv = 1, count = 0, 
	  cursor = cmd, last_char = 1;
	rv 
	  && (++count < (MAX_INPUT-1))
	  && (last_char != '\n');
	cursor++) { 

      rv = read(0, cursor, 1);
	  if(*cursor != 27)
	  {
		
	  }
	  //char buff[100];
	  //sprintf(buff, "%d", *cursor);
	  //rv = write(2, buff, strlen(buff));
      last_char = *cursor;
	  if(*cursor == 27)
	  {
		//write(2, "\nup\n", strlen("\nup\n"));
		//upArrow = 1;
		
			upArrow = 0;
			downArrow = 0;
		histBuff[0] = 27;
	  }
	  else if(*cursor == 91 && histBuff[0] == 27)
	  {
	  
			upArrow = 0;
			downArrow = 0;
		histBuff[1] = 91;
	  }
	  else if(*cursor == 65)
	  {
	  
			upArrow = 0;
			downArrow = 0;
		if(histBuff[0] == 27 && histBuff[1] == 91)
		{
			upArrow = 1;
		}
		else
		{
			histBuff[0] = 0;
			histBuff[1] = 0;
		}
	  }
	  else if(*cursor == 66)
	  {
	  
			upArrow = 0;
			downArrow = 0;
		if(histBuff[0] == 27 && histBuff[1] == 91)
		{
			downArrow = 1;
		}
		else
		{
			histBuff[0] = 0;
			histBuff[1] = 0;
		}
	  }
	  else if((upArrow || downArrow) && *cursor == 10)
	  {
		write(1, "\n", 1);
		if(histEnd == histStart)
		{
			strcpy(cmd, "");
		}
		else if(histEnd < histStart)
		{
			if(historyIndex >= histStart || historyIndex <= histEnd)
			{
				strcpy(cmd, history[historyIndex]);
			}
			else
			{
				strcpy(cmd, "");
			}
		}
		else //histEnd > histStart
		{
			if(historyIndex >= histStart && historyIndex <= histEnd)
			{
				strcpy(cmd, history[historyIndex]);
				//write(0, "\nswish> Historical command", strlen("\nswish> Historical command"));
			}
			else
			{
				strcpy(cmd, "");
			}
		}
		//cmd = lscmd;
		upArrow = 0;
		downArrow = 0;
		break;
	  }
	  else
	  {
		rv = write(1, cursor, 1);
		histBuff[0] = 0;
		histBuff[1] = 0;
		upArrow = 0;
		downArrow = 0;
	  }
	  
	  if(upArrow)
	  {
		historyIndex--;
		if(histEnd < histStart)
		{
			if(historyIndex > histEnd && historyIndex < histStart)
			{
				historyIndex = histEnd;
			}
		}
		else if(histEnd > histStart)
		{
			if(historyIndex > histEnd || historyIndex < histStart)
			{
				historyIndex = histEnd;
			}
		}
	  }
	  else if(downArrow)
	  {
		historyIndex++;
		if(histEnd < histStart)
		{
			if(historyIndex > histEnd && historyIndex < histStart)
			{
				historyIndex = histStart;
			}
		}
		else if(histEnd > histStart)
		{
			if(historyIndex > histEnd || historyIndex < histStart)
			{
				historyIndex = histStart;
			}
		}
	  }
	  
	  if(upArrow || downArrow)
	  {
		write(1, "\n[", 2);
		write(1, cwd, strlen(cwd));
		write(1, "]", 1);
		write(1, prompt, strlen(prompt));
		
		if(histEnd == histStart)
		{
		}
		else if(histEnd < histStart)
		{
			if(historyIndex >= histStart || historyIndex <= histEnd)
			{
				write(1, history[historyIndex], strlen(history[historyIndex]));
			}
		}
		else //histEnd > histStart
		{
			if(historyIndex >= histStart && historyIndex <= histEnd)
			{
				write(1, history[historyIndex], strlen(history[historyIndex]));
			}
		}
			
	  }
    } 
    *cursor = '\0';
	
	
	// get rid of new line at the end of a command
	if(count > 0 && *(cursor - 1) == '\n')
	{
		*(cursor - 1) = '\0'; 
	}
	

    if (!rv) { 
      finished = 1;
	  write(2, "error", strlen("error"));
      break;
    }
	
	// update history
	histEnd++;
	if(histEnd == MAX_HISTORY)
	{
		histEnd = 0;
	}
	if(histStart == histEnd)
	{
		histStart++;
		if(histStart == MAX_HISTORY)
		{
			histStart = 0;
		}
	}
	strcpy(history[histEnd], cmd);

	//write to file

    // Execute the command, handling built-in commands separately 
	
	
    //write(1, cmd, strnlen(cmd, MAX_INPUT));
	char *cmdTemp = malloc(sizeof(cmd));
	strcpy(cmdTemp, cmd);
	char *command = strtok (cmdTemp, " ");

	//only if no piping
	if(strncmp(command, "exit", 4) == 0)
	{
		writeOutHistory(history, histStart, histEnd);
		// kill all jobs
		printf("terminating...\n");
		return 0;
	}
	if(strncmp(command, "wolfie", 6) == 0)
	{	
		write(1, "\n", 1);
		write(1, "            |\\      |\\              \n", 37);                
		write(1, "            | \\     | \\             \n", 37);         
		write(1, "            |  \\____|  \\__          \n", 37);
		write(1, "     	  __/              \\         \n", 37);                     
		write(1, "        /             ___   \\       \n", 37);                      
		write(1, "       _\\            <_@_>   \\__    \n", 37);                  
		write(1, "   ___/                         |   \n", 37);                               
		write(1, "  /                             \\__ \n", 37);
		write(1, " /                 ______          \\\n", 37);
		write(1, "/__               |     \\__________|\n", 37);
		write(1, "   /               \\_     \\/\\/\\/\\/\\/\n", 37);
		write(1, "  /                  \\_             \n", 37);
		write(1, " / __     ___          \\/\\/\\/\\/\\/\\  \n", 37);
		write(1, "| / /   _/   | /|  _______________) \n", 37);
		write(1, "\\/ /  _/     |/ | /                 \n", 37);
		write(1, "  / _/          |/                  \n", 37);
		write(1, " |_/                                \n", 37);
		write(1, "\n", 1);
	}
	else if(strncmp(command, "cd", 2) == 0)
	{
		char *arg = strtok (NULL, " ");
		if(arg == NULL)
		{
			chdir(getenv("HOME"));
		}
		else if(strcmp(arg, "-") == 0)
		{
			// prev working directory
			chdir(prevcwd);
		}
		else
		{
			chdir(arg);
		}
		
		strcpy(prevcwd, cwd);
		cwd = getcwd(cwd, MAX_DIR);
	}   
	else if(strncmp(command, "pwd", 3) == 0)
	{
		write(1, cwd, strlen(cwd));
		write(1, "\n", 1);
	}
	else if(strncmp(command, "printenv", 8) == 0)
	{
		printSysEnv();
	}
	else if(strncmp(command, "set", 3) == 0)
	{
		char **args = parseArgs2(cmd);
		int i = 0;
		char *currentArg = *(args + i);
		char *name;
		char *val;

		int nameFound = 0;
		int valFound = 0;

		while(currentArg != NULL)
		{
			//
			if(i == 1)
			{
				if(!checkValidName(currentArg))
				{
					char *errorMsg = "Invalid variable name: ";
					write(2, errorMsg, strlen(errorMsg));
					write(2, currentArg, strlen(currentArg));
					write(2, "\n", 1);
					break;
				}
				else
				{
					name = currentArg;
					nameFound = 1;
				}
			}
			else if(i == 2)
			{
				if(strcmp(currentArg, "=") != 0)
				{
					char *errorMsg = "Invalid format\n";
					write(2, errorMsg, strlen(errorMsg));
					break;
				}
			}
			else if(i == 3)
			{
				val = currentArg;
				valFound = 1;
			}
			else if(i != 0)
			{
				char *errorMsg = "Invalid format\n";
				write(2, errorMsg, strlen(errorMsg));
				break;
			}

			i++;
			currentArg = *(args + i);
		}

		if(nameFound && valFound)
		{
			setenv( &(*(name + 1)), val, 1);
		}
		else
		{
			char *errorMsg = "Invalid format\n";
				write(2, errorMsg, strlen(errorMsg));
		}
	}
	else if(strncmp(command, "jobs", 4) == 0)
	{
		printJobs();
	}
	else if(strncmp(command, "fg", 2) == 0)
	{
		char *arg = strtok (NULL, " ");
		int jobI = atoi(arg);
		if(jobs[jobI - 1] != NULL)
		{
			kill(jobs[jobI - 1]->pid, SIGCONT);
			int status; 
			waitpid(jobs[jobI - 1]->pid, &status, 0);
		}
		else
		{
			write(2, "\nno such job.\n", 14);
		}
	}
	else if(strncmp(command, "bg", 2) == 0)
	{
		char *arg = strtok (NULL, " ");
		int jobI = atoi(arg);
		char buff[100];
		write(2, buff, strlen(buff));
		if(jobs[jobI - 1] != NULL)
		{
			kill(jobs[jobI - 1]->pid, SIGCONT);
		}
		else
		{
			write(2, "\nno such job.\n", 14);
		}
	}
	else
	{
		//char *fileName = parsePath(command)
		runProgram(cmd, envp);
		if(finished)
		{
			write(2, "exit\n", 4);
		}
	}

	free(cmdTemp);
  }
  write(2, "exit\n", 4);
  return 0;
}

char *parsePath(char *command)
{
	int j = 0;
	// if command contains \, check if it exists then return it
	char currentChar = *command;

	while(currentChar != '\0')
	{
			if(currentChar == '/')
			{
				if(fileExists(command))
				{
					return command;
				}
				else
				{
					return NULL;
				}
			}

			currentChar++;
	}
	//write(1, "\n look in path \n", 16);
	
	// else check if it can be found in PATH
	char *path = getenv("PATH");
	char *pathCopy = malloc(sizeof(char) * strlen(path));
	strcpy(pathCopy, path);

	//int i = 0;
  
	char *cPath = strtok (pathCopy, ":");
	while(cPath != NULL)
	{
		int lPath = strlen(cPath) + strlen(command) + 1;
		char fullPath[lPath];

		strcpy(fullPath, cPath);
		strcat(fullPath, "/");
		strcat(fullPath, command);
		
		if(fileExists(fullPath))
		{
			char *returnPath = malloc(lPath);
			strcpy(returnPath, fullPath);
			
			return returnPath;
		}

		j++;
		cPath = strtok (NULL, ":");
	}
	write(1, "Path not found\n", 15);
	return NULL;
}
int fileExists(char *path)
{
	FILE *file;
	if ((file = fopen(path, "r")) == NULL) {
	  return 0;
	} else {
	  fclose(file);
	}
	return 1;
}

int runProgram(char *cmd, char **envp)
{
	char *origCommand = malloc(sizeof(char) * strlen(cmd));
	strcpy(origCommand, cmd);
	int runInBackground = 0;
	
	int waitID = 0;
	int len = strlen(cmd);
	int i = len -1;
	while(cmd[i] == ' ' || cmd[i] == '&' || cmd[i] == '\n')
	{
		if(cmd[i] == '&')
		{
			cmd[i] = ' ';
			runInBackground = 1;
			break;
		}
		i++;
	}
	
	
	char ***args = parseArgs(cmd);
	if(args == NULL)
	{
		return -1;
	}
	int jobID = fork();
	int status = 0;
	
	if(jobID >= 0)
	{
		if(jobID == 0) // child process
		{
			int iteration = 0;
			char **currentCommand = args[iteration];
			int count = 0;
			while(args[count] != NULL)
			{
				count++;
			}
			
			while(currentCommand != NULL)
			{
				char buff[100];
				
				int pipefd1[2];
				int pipefd2[2];
				
				if(count > 1)
				{
					if(iteration % 2 == 0)
					{
						if(iteration != count - 1)
						{
							pipe(pipefd1);
							sprintf(buff, "pipe: %d %d\n", pipefd1[0], pipefd1[1]);
							write(2, buff, strlen(buff));
						}
					}
					else
					{
						
						if(iteration != count - 1)
						{
							pipe(pipefd2);
							sprintf(buff,"pipe: %d %d\n", pipefd2[0], pipefd2[1]);
							write(2, buff, strlen(buff));
						}
					}
				}
				
				
				int childPID = fork();
				if(childPID >= 0) // fork was successful
				{
					if(childPID == 0) // child process
					{
						char greater = '>';
						char less = '<';
						
						// check for file redirects

						int firstRedirect = -1;
						
						int lookingForFile = 0;
						int lookingForFD = 1;
						int i = 0;
						// 1 for greater, -1 for less
						int direction = 0;
						int fd = -1;
						char *file;
						
						// setup file redirection
						char *currentArg = *(currentCommand + i);
						while(currentArg != NULL)
						{
							//write(1, currentArg, strlen(currentArg));
							//write(1, "\n", 1);
							int j = 0;
							char currentChar = *(currentArg + j);
							if(lookingForFD)
							{
								while(currentChar != '\0')
								{
									if(currentChar == greater || currentChar == less)
									{
										if(firstRedirect <0)
										{
											firstRedirect = i;
										}
										
										if(currentChar == greater)
										{
											direction = 1;
										}
										else
										{
											direction = -1;
										}
										
										
										if(j == 0)
										{
											if(currentChar == greater)
											{
												fd = 1; // going to stdout
											}
											else
											{
												fd = 0; // going to stdin
											}
										}
										else // parse out file handle
										{
											char fdStr[j + 1];
											strncpy(fdStr, currentArg, j);
											fdStr[j] = '\0';
											
										}
										
										
										if(*(currentArg + j + 1) == '\0')
										{
											lookingForFile = 1;
											
										}
										else
										{
											file = &(*(currentArg + j + 1));
											lookingForFile = 0;
										}
										
										lookingForFD = 0;
									}
									
									j++;
									currentChar = *(currentArg + j);
								}
							}
							else if(lookingForFile)
							{
								file = currentArg;
								lookingForFile = 0;
								lookingForFD = 0;
							}
							
							if(!lookingForFile && !lookingForFD)
							{
								// set redirect
								char buff[30];
								
								if(direction != -1 || fd == 0)
								{
									int fd2 = open(file, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
									sprintf(buff, "%s(%d) <- %d", file, fd2, fd);
									
									if(fd2 >= 0)
									{
										dup2(fd2, fd);
											
										write(1, buff, strlen(buff));
										write(1, "\n", 1);	
									}
									else
									{
										//print error msg
									}				
								}
								
								lookingForFile = 1;
								lookingForFD = 1;
							}
								
							i++;
							currentArg = *(currentCommand + i);
						}
						
						if(firstRedirect >0)
						{
							currentCommand[firstRedirect] = NULL;
						}
						
						
						// set up piping
						// overrides < and > if necessary
						if(count > 1)
						{
							if(iteration % 2 == 0)
							{
								//first command reads from stdin
								if(iteration != 0)
								{
									// read in location
									if(pipefd2[1] != 0)
									{
										close(pipefd2[1]);    /* close write end of pipe              */
									}
									dup2(pipefd2[0],0);   /* make 0 same as read-from end of pipe */
									if(pipefd2[0] != 0)
									{
										close(pipefd2[0]);
									}
									
								}
								
								// last command writes to std out
								if(iteration != count - 1)
								{
									//write to location
									if(pipefd1[0] != 0)
									{
										close(pipefd1[0]);    /* close read end of pipe               */
									}
									dup2(pipefd1[1],1);   /* make 1 same as write-to end of pipe  */
									if(pipefd1[1] != 0)
									{
										close(pipefd1[1]);
									}
									
								}
							}
							else
							{
				
								if(pipefd1[1] != 0)
								{
									close(pipefd1[1]);    /* close write end of pipe              */
								}
								dup2(pipefd1[0],0);   /* make 0 same as read-from end of pipe */
								if(pipefd1[0] != 0)
								{
									close(pipefd1[0]);
								}
								
								
								// last command writes to std out
								if(iteration != count - 1)
								{
									//write to location
									if(pipefd2[0] != 0)
									{
										close(pipefd2[0]);    /* close read end of pipe               */
									}
									dup2(pipefd2[1],1);   /* make 1 same as write-to end of pipe  */
									if(pipefd2[1] != 0)
									{
										close(pipefd2[1]);   
									}
									
								}
							}

						}
						
						execve(currentCommand[0], currentCommand, envp);
						
						// exec fails
						_exit(1);
					}
					else //Parent process
					{
						if(iteration != 0)
						{
							if(iteration % 2 == 0)
							{						
								if(pipefd2[1] != 0)
								{
									close(pipefd2[1]);
								}
							}
							else
							{						
								if(pipefd1[1] != 0)
								{
									close(pipefd1[1]);
								}
							}
						}
						
						if(debug)
						{
							char *runningLine = "RUNNING: ";
							write(2, runningLine, strlen(runningLine));
							write(2, *currentCommand, strlen(*currentCommand));
							write(2, "\n", 1);
						}
						
						
						
						
												int retpid = waitpid(childPID, &waitID, WUNTRACED);
																		if(retpid > 0)
																		{
																			waitID = WEXITSTATUS(waitID);
																		}
							
						
						
						if(debug)
						{
							char *endingLine = "ENDED: ";
							char *returnLine = " (ret=";
							char *lastLine = ")\n";
							write(2, endingLine, strlen(endingLine));
							write(2, *currentCommand, strlen(*currentCommand));
							write(2, returnLine, strlen(returnLine));
							char returnStr[10];
							sprintf(returnStr, "%d", waitID);
							//char returnValStr[10];
							//sprintf(returnValStr, "%d ", returnVal);
							//write(2, returnValStr, strlen(returnValStr));
							write(2, returnStr, strlen(returnStr));
							write(2, lastLine, strlen(lastLine));
						}
						
						if(iteration == count - 1)
						{
							exit(0);
						}
					}
				}
				else // fork failed
				{
					char *message="\n Fork failed, quitting!!!!!!\n";
					write(2, message, strlen(message));
				} 
			
				iteration++;
				currentCommand = args[iteration];
			}
		}
		else // parent process
		{
			if(runInBackground)
			{
				addJobToList(jobID, origCommand, 0, 0);
			}
			else
			{
				fgId=addJobToList(jobID, origCommand, 0, 1);
			}
			
			if(debug)
			{
				//char *runningLine = "RUNNING: ";
				//write(2, runningLine, strlen(runningLine));
				char buff[100];
				sprintf(buff, "RUNNING: %d %s\n", jobID, origCommand);
				write(2, buff, strlen(buff));
				//write(2, "\n", 1);
			}

			// waitpid
			if(!runInBackground)
			{
				while(1)
				{
					if(is_ready(fileno(stdin)))
					{
							struct stat inStat;
							int rv = fstat(0, &inStat);
							char line[100];
							
							sprintf(line, "\nrv: %d errno: %d %s ", rv, errno, strerror(errno));
							char buff[1024];
							int n =	read(fileno(stdin), buff, 1024);
														
							int i;
							for(i = 0; i < n; i++)
							{
								if(buff[i] == 1)
								{
									kill(jobID, SIGINT);
									write(2, "\nctrl a pressed\n", strlen("\nctrl a pressed\n"));
								}
								else if(buff[i] == 2)
								{
									int rv = kill(jobID, SIGTSTP);
									sprintf(line, "\nrv: %d errno: %d %s\n", rv, errno, strerror(errno));
									write(2, line, strlen(line));
									write(2, "\nctrl b pressed\n", strlen("\nctrl b pressed\n"));
								}
							}
						
					}
					
					int waitRet = waitpid(jobID, &status, WUNTRACED | WNOHANG);
					//char line[100];
					//sprintf(line, "\npid: %d status: %d\n", waitRet, status);
					//write(2, line, strlen(line));
					//char temp[100];
					//sprintf(temp, "pid: %d status: %d\n", waitRet, status);
					//write(2, temp, strlen(temp));
					if(waitRet > 0)
					{
						if(WIFEXITED(status))
						{
							updateStatusOfJob(getIndexOfJob(jobID), 2);
							status = WEXITSTATUS(status);
							
							if(debug)
							{
								char buff[100];
								sprintf(buff, "ENDED: %d %s (ret= %d)\n", jobID, origCommand, status);
								write(2, buff, strlen(buff));
								
							}
							
							break;
						}
						else if(WIFSIGNALED(status))
						{
							updateStatusOfJob(getIndexOfJob(jobID), 2);
							if(debug)
							{
								char buff[100];
								sprintf(buff, "Terminated by signal: %d %s\n", jobID, origCommand);
								write(2, buff, strlen(buff));
								
							}
							
							break;
						}
						else if(WIFSTOPPED(status))
						{
							updateStatusOfJob(getIndexOfJob(jobID), 1);
							if(debug)
							{
								char buff[100];
								sprintf(buff, "Stopped by signal: %d %s\n", jobID, origCommand);
								write(2, buff, strlen(buff));
								
							}
							break;
						}
					}
				}
			}
		}
	}
	else
	{
		// fork failed
	}
	
	
	
	/*
	*/
	return waitID; 
}

char ***parseArgs(char *command)
{
	
	char *currentCmd = strtok(command, "|");
	int j = 0;
	char **temp = malloc(sizeof(char *) * MAX_ARGS);
	while(currentCmd != NULL)
	{
		temp[j] = currentCmd;
		j++;
		currentCmd = strtok(NULL, "|");
	}
	temp[j] = NULL;
	
	int i ;
	char ***args = malloc(sizeof(char **) * (j + 1));

	for(i = 0; i < j; i++)
	{
		char **temp2 = malloc(sizeof(char *) * MAX_ARGS);
		char *current = strtok(temp[i], " ");
		int t = 0;
			
		while(current != NULL)
		{
			temp2[t] = current;
			t++;
			current = strtok(NULL, " ");
		}

		args[i] = malloc(sizeof(char *) * (t + 1));
		
		args[i][0] = parsePath(temp2[0]);

		// check if command exists
		
		if(args[i][0] == NULL)
		{
			// write some error message
			return NULL;
		}
		
		int k = 1;

		while(k < t)
		{
			args[i][k] = temp2[k];
			
			k++;
		}
		args[i][k] = NULL; 
	}
	
	args[j] = NULL;
	
	return args;
	
}

char **parseArgs2(char *command)
{
	char **args = malloc(sizeof(char *) * MAX_ARGS);
	int i = 0;
	char *currentArg = strtok (command, " ");;
	while(currentArg != NULL)
	{
		args[i] = currentArg;
		i++;
		currentArg = strtok (NULL, " ");
	}
	args[i] = (char *) 0;
	return args;
}


int addJobToList(int pid, char *command, int status, int forground)
{
	job *newJob = malloc(sizeof(job));
	newJob->pid = pid;
	newJob->command = command;
	newJob->status = status;
	newJob->forground = forground;
	
	// find open spot
	int i;
	for(i = 0; i < 100; i++)
	{
		if(jobs[i] == NULL)
		{
			jobs[i] = newJob;
			return i;
		}
	}
	
	return -1; // malloc more space :(
}

int removeJobPID(int pid)
{
	int i;
	for(i = 0; i < 100; i++)
	{
		if(jobs[i] != NULL)
		{
			if(jobs[i]->pid == pid)
			{
				free(jobs[i]);
				jobs[i] = NULL;
				return i;
			}
		}
	}
	
	return -1;
}

int removeJob(int index)
{
	if(jobs[index] != NULL)
	{
		int pid = jobs[index]->pid;
			free(jobs[index]);
			jobs[index] = NULL;
			return pid;
	}
	return -1;
}

int updateStatusOfJob(int index, int status)
{
	if(jobs[index] != NULL)
	{
		int pid = jobs[index]->pid;
		jobs[index]->status = status;
		return pid;
	}
	return -1;
}

int getIndexOfJob(int pid)
{
	int i;
	for(i = 0; i < 100; i++)
	{
		if(jobs[i] != NULL)
		{
			if(jobs[i]->pid == pid)
			{
				return i;
			}
		}
	}
	
	return -1;
}

void printJobs()
{
	int i;
	for(i = 0; i < 100; i++)
	{
		if(jobs[i] != NULL)
		{
			char *status;
			char buff[100];
			
			if(jobs[i]->status == 0)
			{
				write(2, "check stat\n", strlen( "check stat\n"));
				status = "running";
				int newStat = getNewStat(jobs[i]->pid);
				if(newStat >= 0)
				{
					write(2, "new stat\n", strlen( "new stat\n"));
					jobs[i]-> status = newStat;
				}
				
			}
			
			
			if(jobs[i]->status == 1)
			{
				status = "stopped";
			}
			else if(jobs[i]->status == 2)
			{
				status = "done";
			}
			
			sprintf(buff, "[%d]\t%d\t\t%s\t\t%s\n", i+1, jobs[i]->pid, status, jobs[i]->command);
			write(1, buff, strlen(buff));
			if(jobs[i]->status == 2)
			{
				removeJob(i);
			}
		}
	}
}

int getNewStat(int pid)
{
	int stat;
	int retPid = waitpid(pid, &stat, WNOHANG | WUNTRACED);
	if(retPid == pid)
	{
		if(WIFEXITED(stat))
		{
			return 2;
		}
		else if(WIFSTOPPED(stat))
		{	
			return 1;
		}
		return 0;
	}
	return -1;
	
}


int is_ready(int fd) {
    fd_set fdset;
    struct timeval timeout;
    //int ret;
    FD_ZERO(&fdset);
    FD_SET(fd, &fdset);
    timeout.tv_sec = 0;
    timeout.tv_usec = 1;
   
    select(fd+1, &fdset, NULL, NULL, &timeout);
	if(FD_ISSET(fd, &fdset))
	{
		return 1;
	}
	return 0;
}

void writeOutHistory(char **history, int start, int end)
{
	// open file
	//
	char *filename = ".swish_history";
	char *homedir = getenv("HOME");
	int len = strlen(filename) + strlen(homedir) + 1;
	char filepath[len];
	strcpy(filepath, homedir);
	strcat(filepath, "/");
	strcat(filepath, filename);
	FILE *file = fopen(filepath, "w");
	
	int i = start;
	while(i != end)
	{
		// write line
		fprintf(file, "%s\n", history[i]);
		//fprintf(file, "\n");
		i++;
		i = i % MAX_HISTORY;
	}
	
	fclose(file);
}

FILE *readInHistory(char **history)
{
	char *filename = ".swish_history";
	char *homedir = getenv("HOME");
	int len = strlen(filename) + strlen(homedir) + 1;
	char filepath[len];
	strcpy(filepath, homedir);
	strcat(filepath, "/");
	strcat(filepath, filename);
	return fopen(filepath, "r");
}