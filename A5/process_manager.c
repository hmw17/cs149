/**
* Author names:
* Author emails:
* Last modified date:
* Creation date:
**/


/*

Sample test

> gcc process_manager.c -o process_manager
> ./process_manager
cat names.txt
wc names.txt
> ls
216.err  217.err  883.err  884.err  a.out         countnames_parallel.c  names.txt        process_manager.c
216.out  217.out  883.out  884.out  countnames.c  cs149-A3.pdf           process_manager
> cat 216.out
Starting command 2: child PID 216 of parent PPID 215
Nicky

Dave Joe
Yuan Cheng Chang

Dave Joe
John Smith
Yuan Cheng Chang
Yuan Cheng Chang
Finished child PID 216 of parent PPID 215
> cat 217.out
Starting command 2: child PID 217 of parent PPID 215
 9 16 88 names.txt
Finished child PID 217 of parent PPID 215



*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_FILE_NAME 128
#define MAX_LOG_MESSAGE_LENGTH 256
#define MAX_COMMAND_LENGTH 512
#define MAX_COMMAND_ARGS 64

void fileLoggerFunc(char* log_file, char* message)
{
	int fdesc = open(log_file, O_CREAT | O_WRONLY | O_APPEND, 0777);
	if (fdesc == -1)
	{
		fprintf(stderr, "log file cannot be opened");
		exit(EXIT_FAILURE);
	}
	dprintf(fdesc, "%s\n", message);
	close(fdesc);
}

int main(int argc, char** argv)
{


	char command[MAX_COMMAND_LENGTH];

	int command_num = 1;


	while (fgets(command, MAX_COMMAND_LENGTH, stdin))
	{
		char* token = strtok(command, " \t\n");
		char* cargs[MAX_COMMAND_ARGS] = { NULL };
		int i = 0;
		while (token != NULL && i < MAX_COMMAND_ARGS - 1)
		{
			cargs[i] = token;
			i++;
			token = strtok(NULL, " \t\n");
		}

		int pid = fork();

		if (pid == -1)
		{
			// Exit with failure code if fork fails
			exit(-1);
		}

		if (pid == 0)
		{
			char output_file_name[MAX_FILE_NAME];
			snprintf(output_file_name, MAX_FILE_NAME, "%d.out", getpid());


			int output_fdesc = open(output_file_name, O_CREAT | O_WRONLY | O_APPEND, 0777);
			if (output_fdesc == -1)
			{
				fprintf(stderr, "output file cannot be opened");
				exit(EXIT_FAILURE);
			}

			dup2(output_fdesc, STDOUT_FILENO);
			char error_file_name[MAX_FILE_NAME];
			snprintf(error_file_name, MAX_FILE_NAME, "%d.err", getpid());
			int error_fdesc = open(error_file_name, O_CREAT | O_WRONLY | O_APPEND, 0777);
			if (error_fdesc == -1)
			{
				fprintf(stderr, "error file cannot be opened");
				exit(EXIT_FAILURE);
			}
			dup2(error_fdesc, STDERR_FILENO);



			char log_message[MAX_LOG_MESSAGE_LENGTH];
			snprintf(log_message, MAX_LOG_MESSAGE_LENGTH, "Starting command %d: child PID %d of parent PPID %d", command_num + 1, getpid(), getppid());
			fileLoggerFunc(output_file_name, log_message);


			// Child process:
			int err = execvp(cargs[0], cargs);

			// execv only returns if there was an error

		}
		else
		{
			int status;
			int pid = waitpid(-1, &status, 0);
			if (!WIFEXITED(status) || WEXITSTATUS(status) != EXIT_SUCCESS)
			{
				char erro_log_message[MAX_LOG_MESSAGE_LENGTH];
				char error_file_name[MAX_FILE_NAME];
				snprintf(error_file_name, MAX_FILE_NAME, "%d.err", pid);
				int error_fdesc = open(error_file_name, O_CREAT | O_WRONLY | O_APPEND, 0777);
				if (error_fdesc == -1)
				{
					fprintf(stderr, "error file cannot be opened");
					exit(EXIT_FAILURE);
				}
				dup2(error_fdesc, STDERR_FILENO);
				snprintf(erro_log_message, MAX_LOG_MESSAGE_LENGTH, "Exited with exitcode = %d", status);
				fileLoggerFunc(error_file_name, erro_log_message);
			}
			else
			{
				char output_file_name[MAX_FILE_NAME];
				snprintf(output_file_name, MAX_FILE_NAME, "%d.out", pid);

				int output_fdesc = open(output_file_name, O_CREAT | O_WRONLY | O_APPEND, 0777);
				if (output_fdesc == -1)
				{
					fprintf(stderr, "output file cannot be opened");
					exit(EXIT_FAILURE);
				}
				dup2(output_fdesc, STDOUT_FILENO);

				char log_message_finish[MAX_LOG_MESSAGE_LENGTH];
				snprintf(log_message_finish, MAX_LOG_MESSAGE_LENGTH, "Finished child PID %d of parent PPID %d", pid, getpid());
				fileLoggerFunc(output_file_name, log_message_finish);
			}

			// Parent process
		}
	}

	// Exit with success code
	return 0;
}