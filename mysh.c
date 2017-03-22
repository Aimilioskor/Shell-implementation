#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/wait.h>
#include<unistd.h>

//functions declaration
char *read_ln(void);
char **split_ln(char *line);
int do_command(char **args, int pipes);
int selector(char **args);
void make_shell(void);

int (*builtin_func[])(char **);
int exit_func(char **args);
int cd_builtin(char **args);

//global variable
int pipes = 0;

//list with builtin variables
char *builtin_str[] = { "cd", "exit" };


int main(int argc, char **argv) {

	make_shell();

	return (0);
}


void make_shell(void) {

	char *line;
	char **args;
	int status;
	//print a prompt, read then parse and excecute the args
	do {
		printf("$ ");

		line = read_ln();
		args = split_ln(line);
		status = selector(args);

		free(line);
		free(args);

	} while (status);

}

char *read_ln(void) {
	char *line = NULL;
	ssize_t bufsize = 0; 
	getline(&line, &bufsize, stdin);

	return line;
}

#define DELIMETERS " \t\r\n\a"

char **split_ln(char *line) {

	int bufsize = 128;
	int position = 0;
	char **tokens = malloc(bufsize * sizeof(char*));
	char *token;
//pipe var will help finding how many pipes are there in the users command
	char pipe_var = '|';

	if (!tokens) {
		fprintf(stderr, "Allocation error\n");
		exit(EXIT_FAILURE);
	}

	token = strtok(line, DELIMETERS);

//everytime a new command is given the number of pipes must be zeroed
	pipes = 0;

	while (token != NULL) {

		if (*token == pipe_var) {
			pipes++;
		}
		tokens[position] = token;
		position++;

		if (position >= bufsize) {

			bufsize++;
			tokens = realloc(tokens, bufsize * sizeof(char*));

			if (!tokens) {
				fprintf(stderr, "Allocation error\n");
				exit(EXIT_FAILURE);
			}

		}

		token = strtok(NULL, DELIMETERS);
	}

	tokens[position] = NULL;
	return tokens;
}
//selector function decides if the command is a built-in or not
int selector(char **args)

{
	if (args[0] == NULL) {
		return 1;
	}
	int i;
//if it's a built-in then return builtin_func to execute it else return do_command
	for (i = 0; i < 2; i++) {
		if (strcmp(args[0], builtin_str[i]) == 0) {
			return (*builtin_func[i])(args);
		}
	}

	return do_command(args, pipes);
}

int (*builtin_func[])(char **) = {
	&cd_builtin,
	&exit_func
};

/*
 Builtin function implementations.
 */
int cd_builtin(char **args) {
	if (args[1] == NULL) {
		fprintf(stderr, "Expected argument to cd\n");
	} else {
		if (chdir(args[1]) != 0) {
			perror("err");
		}
	}
	return 1;
}

int exit_func(char **args) {
	return 0;
}

int do_command(char **args, int pipes) {
// number of commands the user gives
	const int commandnum = pipes + 1;
	int i = 0;

	int pipefds[2 * pipes];

	for (i = 0; i < pipes; i++) {
		if (pipe(pipefds + i * 2) < 0) {
			perror("Pipe failed\n");
			exit(EXIT_FAILURE);
		}
	}

	int pid;
	int status;
	pid_t wpid;
	int a = 0;
	int b = 0;
	int s = 1;
	int place;
// array to store where the next command starts
	int next_start[10];
	next_start[0] = 0;

//if we find a pipe delete it and hold the command in the next position inside next_start[s]
	while (args[b] != NULL) {
		if (!strcmp(args[b], "|")) {
			args[b] = NULL;
			next_start[s] = b + 1;
			s++;
		}
		b++;
	}

	for (i = 0; i < commandnum; ++i) {
// variable place will show us where in our args to call execvp
		place = next_start[i];
		pid = fork();
		if (pid == 0) {
//if not last command
			if (i < pipes) {
				if (dup2(pipefds[a + 1], 1) < 0) {
					perror("dup2");
					exit(EXIT_FAILURE);
				}
			}

//if not first command and a!= 2*pipes
			if (a != 0) {
				if (dup2(pipefds[a - 2], 0) < 0) {
					perror("dup2");
					exit(EXIT_FAILURE);
				}
			}

			int z;
			for (z = 0; z < 2 * pipes; z++) {
				close(pipefds[z]);
			}

// The commandnum are executed here,
			if (execvp(args[place], args + place) < 0) {
				perror(*args);
				exit(EXIT_FAILURE);
			}
		} else if (pid < 0) {
			perror("error");
			exit(EXIT_FAILURE);
		}

		a += 2;
	}

	for (i = 0; i < 2 * pipes; i++) {
		close(pipefds[i]);
	}

	do {
		wpid = waitpid(pid, &status, WUNTRACED);
	} while (!WIFEXITED(status) && !WIFSIGNALED(status));

	return 1;

}
