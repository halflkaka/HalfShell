#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <pwd.h>
#include <assert.h>
#include <errno.h>

#define MAX_USERNAME 1024
#define MAX_PATH 1024
#define MAX_BUFFER_SIZE 1024
#define MAX_ARGUMENT_SIZE 10
#define MAX_HISTORY 100

int indice = 0;

typedef struct Command {
	char* text;
	int count;
} Command;

Command* createCommand(char* line, int i) {
	Command* command = (Command*)malloc(sizeof(Command));
	if (!command) {
		fprintf(stderr, "error: %s\n", strerror(errno));
	}
	command->count = i;
	command->text = line;
	return command;
}

typedef struct Queue {
	int peek, rear;
	int size, capacity;
	Command** commandHistroy;
} Queue;

Queue* createQueue() {
	Queue* queue = (Queue*)malloc(sizeof(Queue));
	queue->size = 0;
	queue->peek = 0;
	queue->rear = MAX_HISTORY - 1;
	queue->capacity = MAX_HISTORY;
	queue->commandHistroy = (Command**)malloc(sizeof(Command*) * MAX_HISTORY);
	if (!queue->commandHistroy) {
		fprintf(stderr, "error: %s\n", strerror(errno));
	}
	return queue;
}

int isFull(Queue* history) {
	return (history->size == MAX_HISTORY);
}

int isEmpty(Queue* history) {
	return (history->size == 0);
}

Command* deque(Queue* history) {
	Command* command = history->commandHistroy[history->peek];
	history->peek = (history->peek + 1) % (history->capacity);
	history->size = history->size - 1;
	return command;
}

void enqueue(Queue* history, Command* command) {
	if (isFull(history)) {
		deque(history);
	}
	history->rear = (history->rear + 1) % (history->capacity);
	history->commandHistroy[history->rear] = command;
	history->size = history->size + 1;
}

void printHistory(Queue* history, long limit) {
	if (limit >= history->size) {
		for (int i = history->peek; i <= history->rear; i++) {
			printf("%d %s\n", history->commandHistroy[i]->count, history->commandHistroy[i]->text);
		}
	} else {
		for (int i = history->peek; i < history->peek + limit; i++) {
			printf("%d %s\n", history->commandHistroy[i]->count, history->commandHistroy[i]->text);
		}
	}
	
}

void clearHistory(Queue* history) {
	history->size = 0;
	history->peek = 0;
	history->rear = history->capacity - 1;
	memset(history->commandHistroy, 0, sizeof(Command*));
}

void promptWithInfo() 
{
	int maxUserSize = MAX_USERNAME;
	int maxPathSize = MAX_PATH;
	uid_t uid;
	char* username;
	char* dir = malloc(sizeof(char) * maxPathSize);
	char* root;
	struct passwd *pw;

	/*Get User ID, inquire user's information by ID*/
	uid = getuid();
	if (uid == -1) {
		fprintf(stderr, "error: %s\n", strerror(errno));
	}

	pw = getpwuid(uid);
	assert(pw != NULL);
	root = pw->pw_dir;
	username = pw->pw_name;
	
	dir = getcwd(dir, maxPathSize);
	assert(dir != NULL);
	/*Compare root and current direction, 
	if we are now at root, replace direction by '~' */
	if (strcmp(dir, root) == 0) {
		dir = realloc(dir, sizeof(char) * 1);
		dir[0] = '~';
	}

	printf("%s:%s$ \n", username, dir);
	free(dir);
}

void prompt() {
	printf("$");
}

/*
	args[0] is command,
	the rest are arguments
*/
char* readCommand(Queue* history) {
	size_t maxBufferSize = MAX_BUFFER_SIZE;
	char* input = malloc(sizeof(char) * maxBufferSize);
	
	if (!input) {
		fprintf(stderr, "error: %s\n", strerror(errno));
	}

	getline(&input, &maxBufferSize, stdin);
	input[strlen(input)-1] = '\0';

	Command* command = createCommand(strdup(input), indice);
	indice++;

	enqueue(history, command);
	return input;
}

char** parseCommand(char* input) {
	int maxArgumentSize = MAX_ARGUMENT_SIZE;
	const char delim[2] = " ";
	char* token;
	char** arguments = malloc(sizeof(char*) * maxArgumentSize);

	if (!arguments) {
		fprintf(stderr, "error: %s\n", strerror(errno));
	}

	token = strtok(input, delim);
	int pos = 0;

	while (token != NULL) {
		arguments[pos++] = token;
		if (pos == maxArgumentSize) break;
		token = strtok(NULL, delim);
	}
	arguments[pos] = NULL;

	return arguments;
}

int execute(char** args) {
	pid_t parent = getpid();
	pid_t pid = fork();

	if (pid == -1) {
		fprintf(stderr, "error: %s\n", strerror(errno));
		return 0;
	} else if (pid == 0) {
		int status = execve(args[0], args, NULL);
		if (status == -1) {
			fprintf(stderr, "error: %s\n", strerror(errno));
			_exit(EXIT_FAILURE);
		}
		exit(1);
	} else {
		int status;
		waitpid(pid, &status, 0);
	}
	return 1;
}

int builtin_exit() {
	return 0;
}

int builtin_cd(char* path) {
	if (path == NULL) {
		fprintf(stderr, "error: Please provide path\n");
	} else if (chdir(path) == -1) {
		fprintf(stderr, "error: %s\n", strerror(errno));
	}
	return 1;
}

int builtin_history(Queue* history, char** args) {
	char* status;
	if (args[1] != NULL && strcmp(args[1], "-c") == 0) { //history -c
		clearHistory(history);
		return 1;
	} else if (args[1] != NULL && strtol(args[1], &status, 10)) { //history [number]
		long limit = strtol(args[1], &status, 10);
		printHistory(history, limit);
		return 1;
	} else if (args[1] != NULL) {
		fprintf(stderr, "error: %s\n", strerror(errno));
		return 1;
	} else {
		printHistory(history, history->size);
	}
	return 1;
}

int wrapper(char** command, Queue* history);

int match(char* s, char* target) {
	if (strlen(s) > strlen(target)) {
		return 0;
	}
	int i;
	for (i = 0; i < strlen(s); i++) {
		if (s[i] != target[i]) {
			return 0;
		}
	}
	return 1;
}

int execute_history(Queue* history, char** args) {
	if (args[1] != NULL) {
		fprintf(stderr, "error: %s\n", strerror(errno));
		return 1;
	}
	if (strlen(args[0]) == 2 && args[0][1] == '!') { //!!
		history->commandHistroy[history->rear]->text = history->commandHistroy[history->rear-1]->text;
		char* line = history->commandHistroy[history->rear]->text;
		return wrapper(parseCommand(strdup(line)), history);
	} else {										//!his
		int i;
		char* copy = (char*)malloc(sizeof(char) * (strlen(args[0])-1));
		if (!copy) {
			fprintf(stderr, "error: %s\n", strerror(errno));
		}
		for (i = 1; i < strlen(args[0]); i++) {
			copy[i-1] = args[0][i];
		}
		for (i = history->rear; i >= history->peek; i--) {
			if (match(copy, history->commandHistroy[i]->text)) {
				history->commandHistroy[history->rear]->text = history->commandHistroy[i]->text;
				return wrapper(parseCommand(strdup(history->commandHistroy[i]->text)), history);
			}
		}
		fprintf(stderr, "error: none of the history command starts with %s \n", copy);
		free(copy);
		return 1;
	}
	
}

int wrapper(char** args, Queue* history) {
	if (args[0] == NULL) {
		return 1;
	}
	if (strcmp(args[0], "exit") == 0) {
		return builtin_exit();
	}
	if (strcmp(args[0], "cd") == 0) {
		return builtin_cd(args[1]);
	}
	if (strcmp(args[0], "history") == 0) {
		return builtin_history(history, args);
	}
	if (args[0][0] == '!') {
		return execute_history(history, args);
	}
	
	return execute(args);
}


int main() {
	char** args;
	char* command;
	int status;
	Queue* history = createQueue();

	do {
		prompt();
		command = readCommand(history);
		args = parseCommand(command);
		status = wrapper(args, history);
		
		free(command);
		free(args);
	} while (status);
	
}
