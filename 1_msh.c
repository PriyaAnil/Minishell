#include "common.h"
#include <signal.h>

int status;
pid_t pid;
int ch_flag, z_flag, ctr_flag;
char buff[100], string[100];;
jobs *head = NULL;

void print_jobs(void);

// Parse function to read the commands
int parse_function(char ***argv, char *buff, int *idx_arr, int *argc)
{
	int index = 0, j_index = 0, k_index = 0, count = 0;
	char *str = buff;

	// Calculate the length of the command
	int length =  strlen(buff);

	// Initializing the array for index
	idx_arr[k_index++] = 0;

	while (index <= length)
	{
		// Allocate memory for first command
		if (j_index == 0)
			*argv = malloc(1* sizeof(char *));

		// Reallocate memory for the reamaining commands
		else
			*argv = realloc(*argv, (j_index + 1)*sizeof(char *));

		// Check for spaces and null character
		if (buff[index] == ' ' || buff[index] == '\0')
		{
			// Replace with NULL 
			buff[index] = '\0';

			// If pipe is found
			if (strcmp(str, "|") == 0)
			{
				// Store the next position's index in an array
				idx_arr[k_index++] = j_index + 1;

				// Count the number of pipes
				count++;

				// Replace the pipe with NULL
				*(*argv + j_index) = NULL;
			}

			else
			{

				// Allocate memory for the store the string
				*(*argv + j_index) = malloc(strlen(str) + 1);

				// Store the string 
				strcpy(*(*argv + j_index), str);
			}

			// Increment the argv index
			j_index++;

			// Store the next command 
			str = buff + index + 1;
		}

		// increment the buff index
		index++;
	}

	// Store the argument count
	*argc = j_index;

	// Reallocate memory for the last command
	*argv = realloc(*argv, (j_index + 1)*sizeof(char *));

	// Terminate with NULL
	*(*argv + j_index) = NULL; 

	// Return the count of pipes
	return count;
}


// Execution of commands that involves system calls
int system_call(char *buff)
{
	char *str;

	// If the command is "pwd"
	if (strcmp(buff, "pwd") == 0)
	{
		// Invoke getchwd system call to print the cuurent working directory
		getcwd(buff, 100);
		printf("%s\n", buff);
	}

	// System call for make new directory
	else if (strcmp(buff, "mkdir") == 0)
	{
		mkdir(buff + 6, 0777);
		buff[0] = '\0';
	}

	// Sytem call to change directory
	else if (strcmp(buff, "cd") == 0)
	{
		if ((buff[3]) == '\0' || strlen(buff + 3) == 0)
		{
			str =  "/home/emertxe/";
			printf("%s\n", str);
			chdir(str);
		}
		else 
		{
			chdir(buff + 3);
			buff[3] = '\0';
		}
	}

	// System call to remove directory
	else if (strcmp(buff, "rmdir") == 0)
	{
		rmdir(buff + 6);
		buff[3] = '\0';
	}

	else 
		return 0;

	return 1;
}

// Function to create new prompt
int new_prompt(char *buff, char *new_shell)
{
	if (strncmp(buff, "PS1=", 4) == 0)
	{
		if (buff[4] != ' ' && buff[4] != '\t' )
			strcpy(new_shell, buff + 4);
		else 
			printf("PS1: command not found\n");

		return 1;
	}

	return 0;
}

// Function to return back to the prompt
int return_prompt(char *buff)
{
	if (ctr_flag || strlen(buff) == 0)
	{
		if (ctr_flag)
		{
			printf("\n");
			ctr_flag = 0;
		}
		return 1;
	}
	else 
		return 0;

}

// Function to execute special commands
int check_echo(char **argv)
{
	int index = 0;

	if (strcmp(argv[0], "echo") == 0)
	{
		// To print the exit status
		if (strcmp(argv[1], "$?") == 0)
			printf("%d\n", WEXITSTATUS(status));

		// To print the process id
		else if (strcmp(argv[1], "$$") == 0)
			printf("%d\n", getpid());

		// To print executable path
		else if (strcmp(argv[1], "$SHELL") == 0)
		{
			printf("%s\n", string);
		}

		else 
			return 0;
		return 1;
	}
	return 0;
}

// Maintain priority for background/Stopped processes
void check_priority(void) 
{
	jobs *ptr = head;

	if (ptr->next == NULL)
	{
		ptr->priority = '+';
		return;
	}

	// Traverse to the last node
	while (ptr->next !=  NULL)
	{
		// Clear the priority
		ptr->priority= ' ';
		ptr = ptr->next;
	}

	ptr->priority = ' ';

	jobs *last = ptr;

	// Check for the 1st Stopped process from the last node
	while (ptr->prev != NULL && strcmp(ptr->state, "Stopped") != 0)
	{
		ptr = ptr->prev;
	}

	// If 1st stopped process is found
	if (ptr != NULL)
	{
		// Set the priority
		ptr->priority = '+';

		// Traverse back to find the 2nd stopped process
		ptr = ptr->prev;
		while (ptr->prev != NULL && strcmp(ptr->state, "Stopped") != 0)
		{
			ptr = ptr->prev;
		}

		// If 2nd stopped process is found
		if (ptr != NULL)
			ptr->priority = '-';

		// If 2nd stopped process not found 
		else
		{
			// Find the 1st running process 
			while (strcmp(last->state, "Running") != 0)
			{
				last = last->prev;
			}

			// Set the priority
			if (last != NULL)
				last->priority = '-';

		}
	}

	// If Stopped process is not found
	else
	{
		// Find the 1st running process
		while (last->prev != NULL && strcmp(last->state, "Running") != 0)
		{
			last = last->prev;
		}

		if (last != NULL)
		{
			// Set the priority
			last->priority = '+';

			last = last->prev;

			// Find the 2nd running process
			while (last->prev != NULL && strcmp(last->state, "Running") != 0)
			{
				last = last->prev;
			}

			// Set the priority
			if (last != NULL)
				last->priority = '-';
		}
	}
}

// Inserting a job
void insert_job(char *str)
{
	// If the head is NULL
	if (head == NULL)
	{
		// Alocate memory
		head = malloc(sizeof(jobs));

		// Store the information in the node
		head->pid =  pid;
		strcpy(head->state, str);
		strcpy(head->process_name, buff);

		if (strcmp(head->state, "Running") == 0)
		{
			strcat(head->process_name, " &");
		}
		head->process_num = 1;
		head->priority = '+';
		head->next = NULL;
		head->prev = NULL;
	}

	else
	{
		jobs *ptr;

		ptr = head;
	
		// Move to the last node of the list
		while (ptr->next != NULL)
			ptr = ptr->next;

		// Create the new node and insert in the last 
		ptr->next = malloc(sizeof(jobs));
		ptr->next->prev = ptr;
		ptr->next->next = NULL;

		ptr->next->process_num = ptr->process_num + 1;
		strcpy(ptr->next->state, str);
		strcpy(ptr->next->process_name, buff);
		
		if (strcmp(ptr->next->state, "Running") == 0)
			strcat(ptr->next->process_name, " &");
		
		ptr->next->priority = ' ';
		ptr->next->pid = pid;

		// Maintain the priority 
		check_priority();
	}
}

// Delete the jobs from the list
void del_job(pid_t pid)
{
	jobs *ptr;

	ptr = head;

	// Check for the procees id in the list
	while (ptr != NULL && ptr->pid != pid)
		ptr = ptr->next;

	// If null 
	if (ptr == NULL)
		printf("Process not present\n");

	else
	{
		// Delete the node
		if (ptr != head)
		{
			ptr->prev->next = ptr->next;

			if (ptr->next != NULL)
				ptr->next->prev = ptr->prev;
		}

		else 
			head = head->next;

		free(ptr);
	}

	// Maintain thr priority
	if (head != NULL)
		check_priority();
}

// Print the job
void print_jobs(void)
{
	if (head == NULL)
		return;

	else
	{
		jobs *ptr;

		ptr = head;

		while (ptr != NULL)
		{
			printf("[%d]%c  %s %s\n", ptr->process_num, ptr->priority, ptr->state, ptr->process_name);
			ptr = ptr->next;
		}

		ptr = head;
		while (ptr != NULL)
		{
			if (strncmp(ptr->state, "Exit", 4) == 0)
				del_job(ptr->pid);

			ptr = ptr->next;
		}
	}

}

// Signal handler for the child process
void child_sig_handler(int signum, siginfo_t *siginfo, void *context)
{
	char exit[10];
	jobs *jptr = head, *ptr;

	// If the process is exited
	if (siginfo->si_code == CLD_EXITED)
	{	
		sprintf(exit, "%d", siginfo->si_status);

		while (jptr != NULL && jptr->pid != pid)
			jptr = jptr->next;

		// Change the state to exited
		if (jptr != NULL)
		{
			strcpy(jptr->state, "Exit");
			strcat(jptr->state, exit);
		}

		// Set the flag
		ch_flag = 1;
	}

	// If the process is killed
	else if (siginfo->si_code == CLD_KILLED)
	{
		while (jptr != NULL && jptr->pid != pid)
			jptr = jptr->next;

		// Delete the process from the list
		if (jptr != NULL)
		{
			del_job(jptr->pid);
		}

		// Set the flag
		ch_flag = 1;
	}

	// If the process is stopped
	else if (siginfo->si_code == CLD_STOPPED)
	{

		while (jptr != NULL && jptr->pid != pid)
			jptr = jptr->next;

		// Change the state to stopped
		if (jptr != NULL)
			strcpy(jptr->state, "Stopped");
		else
		{
			// If the process is not in the list then insert the process
			insert_job("Stopped");

			ptr = head;
			while (ptr->next != NULL)
				ptr = ptr->next;
		}
		
		// Print the process info
		printf("[%d]%c\t%s\t%s\n", ptr->process_num, ptr->priority, ptr->state, ptr->process_name);

		// Set the flag
		ch_flag = 1;
	}

	// If the flag is continued
	else if (siginfo->si_code == CLD_CONTINUED)
	{

		// Reset the flag
		ch_flag = 0;

		ptr = head;
		while (ptr !=  NULL && ptr->pid != pid)
			ptr = ptr->next;

		// Print the process name
		if (ptr != NULL)
			printf("%s\n", ptr->process_name);
	}
}

// Signal handler for ctrl + c
void ctrl_c_handler(int signum)
{
	if (pid != 0)
	{
		// Send sigkill signal
		kill(pid, SIGKILL);

		// Reset the flag
		ctr_flag = 0;
		printf("\n");
	}

	else 
	{
		// Set the flag
		ctr_flag = 1;
	}
}

// Signal handler for ctrl + z
void ctrl_z_handler(int signum)
{
	// Send sigstop signal
	kill(pid, SIGSTOP);
	printf("\n");
}

// Function call for foreground and background process
int fg_bg(void)
{
	jobs *ptr = head;

	// Foreground process
	if (strcmp(buff, "fg") == 0)
	{
		if (ptr == NULL)
			printf("No process in background\n");

		while (ptr != NULL && ptr->priority != '+')
			ptr = ptr->next;
		
		if (ptr != NULL)
		{	
			ch_flag = 0;

			// Update the pid
			pid = ptr->pid;

			// Change the state
			strcpy(ptr->state, "Running");
			printf("%s\n", ptr->process_name);

			// Send continue signal 
			kill(ptr->pid, SIGCONT);
		}

		// Wait till the flag is set
		while (!ch_flag);

		// Reset the flag
		ch_flag = 0;

	}

	// Background process
	else if (strcmp(buff, "bg") == 0)
	{
		if (ptr == NULL)
			printf("No process in background\n");
		else
		{	
			while (ptr != NULL && ptr->priority != '+')
				ptr = ptr->next;

			if (ptr != NULL)
			{
				// Change the state
				strcpy(ptr->state, "Running");
				
				printf("[%d]%c\t%s\n", ptr->process_num, ptr->priority, ptr->process_name);

				// Send continue signal
				kill(ptr->pid, SIGCONT);
			}
		}
	}

	// Jobs system call
	else if (strcmp(buff, "jobs") == 0)
	{
		// Print the list
		print_jobs();
	}

	else 
		return 0;

	return 1;

}

// Main program
int main()
{
	int argc, pipe_count, index_array[100], idx, jdx;
	char shell[100] = "msh";
	char **argv = NULL;
	jobs *ptr;
	struct sigaction act_1, act_2, act_3;

	// Clear all the structures
	memset(&act_1, 0, sizeof(act_1));
	memset(&act_2, 0, sizeof(act_2));
	memset(&act_3, 0, sizeof(act_3));

	// Set the flag
	act_1.sa_flags = SA_SIGINFO;

	// Assigning signal handlers to the members of the structures
	act_1.sa_sigaction = child_sig_handler;
	act_2.sa_handler = ctrl_c_handler;

	// Signal call for ctrl + c
	sigaction(SIGINT, &act_2, NULL);


	// Signal call for child process
	sigaction(SIGCHLD, &act_1, NULL);

	// system call to get the present working directory
	getcwd(string, 100);

	while (1)
	{
		act_3.sa_handler = SIG_IGN;
		
		// Signal call for ctrl + z 
		sigaction(SIGTSTP, &act_3, NULL);
		
		// Print the default prompt
		printf("%s :> ", shell);

		// Clear the buff
		memset(&buff, 0, sizeof(buff));

		// Scan the command
		scanf("%[^\n]", buff);

		// Clear the stdin
		__fpurge(stdin);

		act_3.sa_handler = ctrl_z_handler;

		// Signal call for ctrl + z 
		sigaction(SIGTSTP, &act_3, NULL);

		// If command is exit terminate
		if (strcmp(buff, "exit") == 0)	
			break;

		// Function calls 
		if (return_prompt(buff) || new_prompt(buff, shell)|| fg_bg())
			continue;

		// Store the pipe count by calling the parse function
		pipe_count = parse_function(&argv, buff, index_array, &argc);

		// Declare pipes 
		int p[pipe_count][2];

		// If the pipe count is 0
		if (pipe_count == 0)
		{
			
			// Function calls
			if (system_call(buff) || check_echo(argv))
				continue;

			// Invoke fork system call to create child process
			switch (pid = fork())
			{
				// Error check
				case -1:
					perror("fork");
					exit(EXIT_FAILURE);

				// Child process	
				case 0:
					
					// Check for '&' symbol
					if (strcmp(argv[argc-1], "&") == 0)
					{
						// Replace with NULL
						argv[argc - 1] = NULL;
					
						act_2.sa_handler = SIG_IGN;	
						act_3.sa_handler = SIG_IGN;

						// Signal call for ctrl + z 
						sigaction(SIGINT, &act_2, NULL);
						
						// Signal call for ctrl + z 
						sigaction(SIGTSTP, &act_3, NULL);

						execvp(argv[index_array[0]], argv + index_array[0]);
					}

					else
						//System call to  execute the commnad
						execvp(argv[index_array[0]], argv + index_array[0]);


					break;

				// Parent process	
				default:
					
					// Check for '&' symbol in the command
					if (strcmp(argv[argc-1], "&") == 0)
					{
						// Insert the process in the list
						insert_job("Running");
						ptr = head;

						while (ptr->next != NULL)
							ptr = ptr->next;
						if (ptr != NULL)
							printf("[%d] %d\n", ptr->process_num, ptr->pid);

						pid = 0;
					}
	
					else
					{	
						// Wait till the child process terminates
						while (!ch_flag);

						// Reset the flag
						ch_flag = 0;
					}
			}
		}

		// If the pipe count is non zero
		else
		{
			dup2(0, 77);

			// Initialize the pipe and hanlde the error
			for (idx = 0; idx < pipe_count + 1; idx++)
			{
				if (idx != pipe_count && pipe(p[idx]) == -1)
				{
					perror("pipe");
					exit(-1);
				}

				// Invoke fork system call and create child process	
				switch (pid = fork())
				{

					// Error check
					case -1:
						perror("fork");
						exit(EXIT_FAILURE);

					// Child process	
					case 0:
						
						if (idx != pipe_count)	
						{
							// Close the read end of the pipe
							close(p[idx][0]);

							// Writing to the pipe
							dup2(p[idx][1], 1);
						}

						// System call to execute the command
						execvp(argv[index_array[idx]], argv + index_array[idx]);
						break;

					default:
						
						// Wait till the child process terminates
						while (!ch_flag);

						// Reset the flag
						ch_flag = 0;

						if (idx != pipe_count)
						{
							// Close the write end of the pipe
							close(p[idx][1]);

							// Read from the pipe
							dup2(p[idx][0], 0);
						}

						else
							dup2(77, 0);
				}
			}
		}
	}
}
