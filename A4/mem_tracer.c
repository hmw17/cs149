/**
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

#include <fcntl.h>

#include <stdarg.h>

#define MAX_FILE_NAME 128
#define MAX_LOG_MESSAGE_LENGTH 256
#define MAX_COMMAND_ARGS 64

#define INITIAL_SIZE 10
char MEM_TRACE_FILE[MAX_FILE_NAME] = "memtrace.out";

struct TRACE_NODE_STRUCT {
   char * functionid;
   struct TRACE_NODE_STRUCT * next;
};

typedef struct TRACE_NODE_STRUCT TRACE_NODE;

static TRACE_NODE * TRACE_TOP = NULL;

void PUSH_TRACE(char * p) {
   TRACE_NODE * tnode;
   static char glob[] = "global";
   if (TRACE_TOP == NULL) {
      TRACE_TOP = (TRACE_NODE * ) malloc(sizeof(TRACE_NODE));
      if (TRACE_TOP == NULL) {
         printf("PUSH_TRACE: memory allocation error\n");
         exit(1);
      }
      TRACE_TOP -> functionid = glob;
      TRACE_TOP -> next = NULL;
   }
   tnode = (TRACE_NODE * ) malloc(sizeof(TRACE_NODE));
   if (tnode == NULL) {
      printf("PUSH_TRACE: memory allocation error\n");
      exit(1);
   }
   tnode -> functionid = p;
   tnode -> next = TRACE_TOP;
   TRACE_TOP = tnode;
}

void POP_TRACE() {
   TRACE_NODE * tnode;
   tnode = TRACE_TOP;
   TRACE_TOP = tnode -> next;
   free(tnode);
}

char * PRINT_TRACE() {
   int depth = 50;
   int i, length, j;
   TRACE_NODE * tnode;
   static char buf[100];
   if (TRACE_TOP == NULL) {
      strcpy(buf, "global");
      return buf;
   }
   sprintf(buf, "%s", TRACE_TOP -> functionid);
   length = strlen(buf);
   for (i = 1, tnode = TRACE_TOP -> next; tnode != NULL && i < depth; i++, tnode = tnode -> next) {
      j = strlen(tnode -> functionid);
      if (length + j + 1 < 100) {
         sprintf(buf + length, ":%s", tnode -> functionid);
         length += j + 1;
      } else {
         break;
      }
   }
   return buf;
}

void fileLoggerFunc(char * log_file, char * message) {
   int fdesc = open(log_file, O_CREAT | O_WRONLY | O_APPEND, 0777);
   if (fdesc == -1) {
      fprintf(stderr, "log file cannot be opened");
      exit(EXIT_FAILURE);
   }
   dprintf(fdesc, "%s\n", message);
   close(fdesc);
}

void * REALLOC(void * p, int t, char * file, int line) {
   void * new_p = realloc(p, t);
   char * message = NULL;
   int length = snprintf(NULL, 0, "File %s, line %d, function %s reallocated the memory segment at address %p to a new size %d\n", file, line, PRINT_TRACE(), new_p, t);
   if (length < 0) {
      return NULL;
   }
   message = malloc(length + 1);
   if (message == NULL) {
      return NULL;
   }
   snprintf(message, length + 1, "File %s, line %d, function %s reallocated the memory segment at address %p to a new size %d\n", file, line, PRINT_TRACE(), new_p, t);

   fileLoggerFunc(MEM_TRACE_FILE, message);
   free(message);
   return new_p;
}

void * MALLOC(int t, char * file, int line) {
   void * p = malloc(t);
   char * message = NULL;
   int length = snprintf(NULL, 0, "File %s, line %d, function %s allocated new memory segment at address %p to size %d\n", file, line, PRINT_TRACE(), p, t);
   if (length < 0) {
      return NULL;
   }
   message = malloc(length + 1);
   if (message == NULL) {
      return NULL;
   }
   snprintf(message, length + 1, "File %s, line %d, function %s allocated new memory segment at address %p to size %d\n", file, line, PRINT_TRACE(), p, t);

   fileLoggerFunc(MEM_TRACE_FILE, message);
   free(message);
   return p;
}

void FREE(void * p, char * file, int line) {
   char * message = NULL;
   int length = snprintf(NULL, 0, "File %s, line %d, function %s deallocated the memory segment at address %p\n", file, line, PRINT_TRACE(), p);
   if (length < 0) {
      return;
   }
   message = malloc(length + 1);
   if (message == NULL) {
      return;
   }
   snprintf(message, length + 1, "File %s, line %d, function %s deallocated the memory segment at address %p\n", file, line, PRINT_TRACE(), p);

   fileLoggerFunc(MEM_TRACE_FILE, message);
   free(message);
   free(p);
}

#define realloc(a, b) REALLOC(a, b, __FILE__, __LINE__)
#define malloc(a) MALLOC(a, __FILE__, __LINE__)
#define free(a) FREE(a, __FILE__, __LINE__)

const char * fileName = NULL;

typedef struct _Node {
   char * val;
   int lineNum;
   struct _Node * next;
}
Node;

typedef struct {
   Node * head;
   Node * tail;
}
List;

List * create_list();

void enqueue(List * list, int lineNum, char * val);

Node * dequeue(List * list);

void free_node(Node * node);
void free_list(List * list);

void PrintNodes(Node * node);

int main(int argc, char ** argv) {

   List * commandList = create_list();
   int command_num = 1;
   int orig_stdout = dup(STDOUT_FILENO);
   char ** commandsArray = NULL;
   int capacity = 0;
   int curr_count = 0;
   char * command = NULL;
   size_t line_len = 0;

   while (getline( & command, & line_len, stdin) != -1) {
      PUSH_TRACE("main (Commands Reading)");
      if (curr_count >= capacity) {
         capacity = (capacity == 0) ? 1 : capacity * 2;
         char ** new_commandsArray = realloc(commandsArray, capacity * sizeof(char * ));
         if (new_commandsArray == NULL) {
            perror("Failed to allocate memory");
            exit(1);
         }
         commandsArray = new_commandsArray;
      }
      commandsArray[curr_count] = malloc(line_len + 1);
      if (commandsArray[curr_count] == NULL) {
         perror("Failed to allocate memory");
         exit(1);
      }
      strcpy(commandsArray[curr_count], command);
      curr_count++;
      POP_TRACE();
      char * token = strtok(command, " \t\n");
      char * cargs[MAX_COMMAND_ARGS] = {
         NULL
      };
      int i = 0;
      while (token != NULL && i < MAX_COMMAND_ARGS - 1) {
         cargs[i] = token;
         i++;
         token = strtok(NULL, " \t\n");
      }

      int pid = fork();

      if (pid == -1) {
         // Exit with failure code if fork fails
         exit(-1);
      }

      if (pid == 0) {

         char output_file_name[MAX_FILE_NAME];
         snprintf(output_file_name, MAX_FILE_NAME, "%d.out", getpid());

         int output_fdesc = open(output_file_name, O_CREAT | O_WRONLY | O_APPEND, 0777);
         if (output_fdesc == -1) {
            fprintf(stderr, "output file cannot be opened");
            exit(EXIT_FAILURE);
         }

         dup2(output_fdesc, STDOUT_FILENO);
         char error_file_name[MAX_FILE_NAME];
         snprintf(error_file_name, MAX_FILE_NAME, "%d.err", getpid());
         int error_fdesc = open(error_file_name, O_CREAT | O_WRONLY | O_APPEND, 0777);
         if (error_fdesc == -1) {
            fprintf(stderr, "error file cannot be opened");
            exit(EXIT_FAILURE);
         }
         dup2(error_fdesc, STDERR_FILENO);

         char log_message[MAX_LOG_MESSAGE_LENGTH];
         snprintf(log_message, MAX_LOG_MESSAGE_LENGTH, "Starting command %d: child PID %d of parent PPID %d", command_num + 1, getpid(), getppid());
         fileLoggerFunc(output_file_name, log_message);

         // Child process:
         execvp(cargs[0], cargs);

         // execv only returns if there was an error

      } else {
         PUSH_TRACE("main");
         char * command_string = NULL;
         size_t command_string_length = 0;

         for (int i = 0; cargs[i] != NULL; i++) {
            command_string_length += strlen(cargs[i]) + 1; // Add 1 for the space character
         }

         command_string = (char * ) malloc(command_string_length + 1);
         command_string[0] = '\0';

         for (int i = 0; cargs[i] != NULL; i++) {
            strcat(command_string, cargs[i]);
            strcat(command_string, " ");
         }
         POP_TRACE();
         PUSH_TRACE("main (calling enqueue)");
         enqueue(commandList, command_num - 1, command_string);
         POP_TRACE();
         PUSH_TRACE("main (calling free on command string)");
         free(command_string);
         POP_TRACE();
         int status;
         int pid = waitpid(-1, & status, 0);
         if (!WIFEXITED(status) || WEXITSTATUS(status) != EXIT_SUCCESS) {
            char erro_log_message[MAX_LOG_MESSAGE_LENGTH];
            char error_file_name[MAX_FILE_NAME];
            snprintf(error_file_name, MAX_FILE_NAME, "%d.err", pid);
            int error_fdesc = open(error_file_name, O_CREAT | O_WRONLY | O_APPEND, 0777);
            if (error_fdesc == -1) {
               fprintf(stderr, "error file cannot be opened");
               exit(EXIT_FAILURE);
            }
            dup2(error_fdesc, STDERR_FILENO);
            snprintf(erro_log_message, MAX_LOG_MESSAGE_LENGTH, "Exited with exitcode = %d", status);
            fileLoggerFunc(error_file_name, erro_log_message);
         } else {
            char output_file_name[MAX_FILE_NAME];
            snprintf(output_file_name, MAX_FILE_NAME, "%d.out", pid);

            int output_fdesc = open(output_file_name, O_CREAT | O_WRONLY | O_APPEND, 0777);
            if (output_fdesc == -1) {
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

      command_num++;
   }

   free(command);
   dup2(orig_stdout, STDOUT_FILENO);
   printf("\n\n------------All commands list--------\n");
   PrintNodes(commandList -> head);
   PUSH_TRACE("main (free list)");
   free_list(commandList);
   close(orig_stdout);
   POP_TRACE();

   PUSH_TRACE("main (free char* array)");
   for (int i = 0; i < curr_count; i++) {
      free(commandsArray[i]);
   }

   free(commandsArray);
   POP_TRACE();

   // Exit with success code
   return 0;
}

List * create_list() {
   PUSH_TRACE("create list");
   List * list = (List * ) malloc(sizeof(List));
   list -> head = NULL;
   list -> tail = NULL;
   POP_TRACE();
   return list;
}

void enqueue(List * list, int lineNum, char * val) {
   PUSH_TRACE("enqueue");
   Node * new_node = (Node * ) malloc(sizeof(Node));
   new_node -> val = (char * ) malloc((INITIAL_SIZE + 1) * sizeof(char));
   strcpy(new_node -> val, val);
   new_node -> next = NULL;

   // Add necessary reallocation if needed based on parameter size
   int len = strlen(val);
   if (len >= INITIAL_SIZE) {
      new_node -> val = (char * ) realloc(new_node -> val, (len + 1) * sizeof(char));
   }
   new_node -> lineNum = lineNum;

   if (list -> head == NULL) {
      list -> head = new_node;
      list -> tail = new_node;
   } else {
      list -> tail -> next = new_node;
      list -> tail = new_node;
   }

   POP_TRACE();
}

Node * dequeue(List * list) {
   Node * node = NULL;
   if (list -> head != NULL) {
      node = list -> head;
      list -> head = list -> head -> next;
   }
   return node;
}

void free_node(Node * node) {
   PUSH_TRACE("free node");
   free(node -> val);
   free(node);
   POP_TRACE();
}

void free_list(List * list) {
   PUSH_TRACE("free list");
   Node * node = list -> head;
   while (node != NULL) {
      Node * next = node -> next;
      free_node(node);
      node = next;
   }
   free(list);
   POP_TRACE();
}

void PrintNodes(Node * node) {
   if (node == NULL) {
      return;
   }
   printf("Line: %d --> %s\n", node -> lineNum, node -> val);
   PrintNodes(node -> next);
}