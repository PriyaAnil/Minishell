#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio_ext.h>
#include <signal.h>
#define BUFFER_SIZE 256

#include <errno.h>
#include <getopt.h>

typedef struct _node
{
	char process_name[20];
	char priority;
	char state[20];
	pid_t pid;
	int process_num;
	struct _node *next, *prev;
}jobs;

#endif
