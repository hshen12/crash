#include <fcntl.h>
#include <signal.h>
#include <pwd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "debug.h"
#include "history.h"
#include "timer.h"
#include "tokenizer.h"

int command_number;
char username[10];
char hostname[10];
char cwd[PATH_MAX];
int token_num;

/**
* Print shell promt information
*/
void print_prompt() {
	printf("\n--[%d|%s@%s:~%s]--$ ", command_number, username, hostname, cwd);
	fflush(stdout);
}

/**
* Modify the cwd to eliminate the user home directory
*/
void short_cwd() {
	int count = 0;
	int i;
	char*p;

	for(i = 0; i < strlen(cwd); i++) {
		if(cwd[i] == '/') {
			count++;
		}
		if(count == 3) {
			p = cwd+i;
			strcpy(cwd, p);
			break;
		}
	}
}

void clean_common(char *tokens[]) {
	int i;

	for(i = 0; i < 4095; i++) {
		if(tokens[i] == ((char *) 0)) {
			break;
		}
		if(strncmp(tokens[i], "#", 1) == 0) {
			tokens[i] = (char *) 0;
		}
	}

}

void parse_line(char *line, char *tokens[]) {

	char *next_tok = line;
	char *curr_tok;
	int i = 0;

	while(i < 4095 && ((curr_tok = next_token(&next_tok, " \t\r\n")) != NULL)) {
		tokens[i++] = curr_tok;
	}

	tokens[i] = (char *) 0;
	token_num = i;
}

void cd_command(char *tokens[]) {
		char *homedir = getenv("HOME");
		if(tokens[1] == NULL){
			chdir(homedir);
		} else {
			chdir(tokens[1]);
		}
}

void exit_command(char *tokens[]) {
	exit(0);
}

void env_command(char *tokens[]) {

	if(token_num == 3) {
		setenv(tokens[1], tokens[2], 1);
	}
}

void sigint_handler(int signo) {
	// exit(0);
}

int main(void) {

	signal(SIGINT, sigint_handler);

	command_number = 0;

	getlogin_r(username, 10);
	gethostname(hostname, 50);
	getcwd(cwd, PATH_MAX);
	short_cwd();

	while(true) {
		if (isatty(STDIN_FILENO)) {
			print_prompt();
		}

		char *line = NULL;
		size_t line_sz = 0;

		ssize_t sz = getline(&line, &line_sz, stdin);

		if(sz == EOF) {
			break;
		}

		char *tokens[4096];
		parse_line(line, tokens);

		if(tokens[0] == NULL) {
			continue;
		}

		clean_common(tokens);

		if(tokens[0] == NULL) {
			continue;
		}

		if(strcmp(tokens[0], "exit") == 0) {
			exit_command(tokens);
		}
		if(strcmp(tokens[0], "cd") == 0) {
			cd_command(tokens);
		}
		if(strcmp(tokens[0], "setenv") == 0) {
			env_command(tokens);
		}

		pid_t pid = fork();
		if(pid == 0) {
			//child
			int ret = execvp(tokens[0], tokens);
			if(ret == -1) {
				break;
			}
			fclose(stdin);
		} else if (pid == -1) {
			perror("fork");
		} else {
			//parent
			int status;
			wait(&status);

		}

	}

	return 0;
}
