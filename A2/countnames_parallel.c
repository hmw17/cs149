/**
* Description: This code implements name counter from given file.
* Author names:
* Author emails:
* Last modified date:
* Creation date:
**/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>

#define MAX_NAME 31


/**
* This function creates an array of character pointers (strings)
* with size "count". Each string can store up to "MAX_NAME" characters.
* It also allocates memory to store the characters for each string.
* Input parameters: count (the number of strings to allocate)
* Returns: a pointer to an array of strings
**/
char** createNamesArray(const int count)
{
	char** list = (char**)malloc(count * sizeof(char*));

	char* mem = (char*)malloc(count * MAX_NAME * sizeof(char));

	for (int i = 0; i < count; i++)
	{
		list[i] = &mem[i * MAX_NAME];
	}

	return list;
}

/**
* This function calculates a hash value for a given string "line"
* based on the sum of the ASCII values of its characters.
* The hash value is then modulo-ed by the number of lines in the file.
* Input parameters: line (the string to calculate the hash for), linesCount (the number of lines in the file)
* Returns: an integer value (the calculated hash)
**/
int hash(const char* line, int linesCount)
{
	int hash_sum = 0, i = 0;

	for (i = 0; line[i] != '\0'; i++)
	{
		hash_sum += line[i];
	}

	return hash_sum % linesCount;
}

/**
* This function opens a file with the name provided in the command line argument
* and returns a pointer to the opened file.
* If no filename is provided in the command line, it prints an error message to stderr and returns NULL.
* If the file cannot be opened, it also prints an error message to stderr.
* Input parameters: argc (the number of command line arguments), argv (an array of strings containing the command line arguments)
* Returns: a pointer to the opened file, or NULL if the file cannot be opened.
**/
FILE* openFile(const char* fileName)
{
	FILE* fin = fopen(fileName, "r");

	if (!fin)
	{
		fprintf(stderr, "File %s cannot be opened\n", fileName);
	}

	return fin;
}

/**
* This function counts the number of non-empty lines in a file.
* It also prints a warning message to stderr for any empty lines found.
* Input parameters: fin (a pointer to the opened file)
* Returns: an integer value (the number of non-empty lines in the file)
**/
int countNonEmptyLines(FILE* fin)
{
	int count = 0;
	char line[MAX_NAME];
	int j = 1;

	// Loop through each line in the file
	while (fgets(line, MAX_NAME, fin))
	{
		line[strcspn(line, "\n")] = 0;

		if (strlen(line) != 0)
		{
			count++;
		}
		else
		{
			fprintf(stderr, "Warning - Line %d is empty.\n", j);
		}

		j++;
	}

	return count;
}

/**
* This is the main function that reads a file containing a list of names (one name per line),
* and counts the number of occurrences of each name in the file.
* It uses a hash table (implemented as an array of strings) to store the unique names,
* and an array of integers to store the count of occurrences for each name.
* It then prints the list of unique names with their corresponding counts
**/


int processFile(const char* fileName)
{
	// Open the input file
	FILE* fin = openFile(fileName);

	// If the file couldn't be opened, return an error code
	if (!fin)
	{
		return -1;
	}

	// Count the number of non-empty lines in the file
	int linesCount = countNonEmptyLines(fin);

	// Close the file
	fclose(fin);

	// Allocate memory for the array of names and an array to count their occurrences
	char line[MAX_NAME];
	char** list = createNamesArray(linesCount);
	int* occur = (int*)malloc(sizeof(int) * linesCount);

	// Initialize the occurrences count and the names array
	int i = 0;
	for (i = 0; i < linesCount; i++)
	{
		occur[i] = 0;
		strcpy(list[i], "");
	}

	// Open the input file again
	fin = fopen(fileName, "r");

	// Loop through each line in the file
	while (fgets(line, MAX_NAME, fin))
	{
		// Remove the newline character at the end of the line
		line[strcspn(line, "\n")] = 0;

		// If the line is not empty, process it
		if (strlen(line) != 0)
		{
			// Calculate the hash value of the name and find an available slot in the array of names
			int hash_idx = hash(line, linesCount);
			while (strcmp(line, list[hash_idx]) != 0 && list[hash_idx][0] != '\0')
			{
				hash_idx++;
			}

			// Add the name to the array and increment its occurrences count
			strcpy(list[hash_idx], line);
			occur[hash_idx]++;
		}
	}

	// Close the file
	fclose(fin);

	// Print the list of names and their occurrences count
	for (i = 0; i < linesCount; i++)
	{
		if (occur[i] != 0)
		{
			printf("%s: %d\n", list[i], occur[i]);
		}
	}

	// Free the memory used by the arrays
	free(list[0]);
	free(list);
	free(occur);

	return 1;
}

int main(int argc, char** argv)
{
	if (argc < 2)
	{
		// Print usage message to stderr and exit with failure code if there are no files specified
		fprintf(stderr, "Usage: %s file1 [file2 ...]\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	// Determine the number of files to process
	int files_count = argc - 1;

	// Initialize an array to store the file descriptors for each pipe
	int fds[files_count][2];

	// Create a pipe for each file to be processed
	int i = 0;
	for (i = 0; i < files_count; i++)
	{
		if (pipe(fds[i]) == -1) {
			// Exit with failure code if pipe creation fails
			exit(EXIT_FAILURE);
		}
		// Fork a child process to process the current file
		int pid = fork();

		if (pid == -1)
		{
			// Exit with failure code if fork fails
			exit(-1);
		}

		if (pid == 0)
		{
			// Child process: close the read end of the pipe, process the file, write the status to the pipe, and exit with success code
			close(fds[i][0]); // close read end of pipe
			int statusRead = processFile(argv[i + 1]);
			write(fds[i][1], &statusRead, sizeof(statusRead));
			close(fds[i][1]);
			exit(EXIT_SUCCESS);
		}
		else
		{
			// Parent process: close the write end of the pipe
			close(fds[i][1]); // close write end of pipe
		}
	}

	// Wait for any child process to finish
	for (i = 0; i < files_count; i++)
	{
		int status;
		int pid = waitpid(-1, &status, 0);
		if (!WIFEXITED(status) || WEXITSTATUS(status) != EXIT_SUCCESS)
		{
			// Print an error message to stderr if the child process terminated abnormally
			fprintf(stderr, "Child process %d terminated abnormally\n", pid);
		}

		// Read the status from the pipe for the current file
		int statusRead = 3;
		read(fds[pid % files_count][0], &statusRead, sizeof(statusRead));

		if (statusRead == -1)
		{
			// Print an error message to stderr if an error occurs while reading the pipe
			fprintf(stderr, "An error occured when reading the file with process %d\n", pid);
		}

		// Close the read end of the pipe for the current file
		close(fds[pid % files_count][0]);
	}

	// Exit with success code
	return 0;
}

