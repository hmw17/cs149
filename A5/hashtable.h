#ifndef HASHTABLE_H
#define HASHTABLE_H

#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

typedef struct nlist { /* table entry: */
	struct nlist* next; /* next entry in chain */
	char* name; /* defined name, can remove */
	// char* defn; /* replacement text, can remove */
	struct timespec starttime; 
	struct timespec finishtime;
	int index; // this is the line index in the input text file */
	int pid; // the process id. you can use the pid result of wait to lookup in the hashtable */
	char *command; // command. This is good to store for when you decide to restart a command 
	bool isRestarable;
} nlist;

#ifndef HASHSIZE
#define HASHSIZE 101
#endif

static struct nlist *hashtab[HASHSIZE]; /* pointer table */


struct nlist* getEntry(int k);
// unsigned hash(char* s);
struct nlist* lookup(int pid);
// struct nlist* lookup(char* s);
unsigned hash(int pid);
// struct nlist* insert(char* name, char* defn);
struct nlist* insert(char* command, int pid, int index);

#endif /* HASHTABLE_H */
