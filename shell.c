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
#include "leetify.h"
#include "expansion.h"

#define JOB_LIST_SIZE 10

int command_number;
char username[LOGIN_NAME_MAX];
char hostname[HOST_NAME_MAX];
char cwd[PATH_MAX];
pid_t pid_list[JOB_LIST_SIZE];
int list_index;
char *line_list[JOB_LIST_SIZE];
bool command_execution;

/**
* Print shell promt information
*/
void print_prompt() {
	printf("--[%d|%s@%s:~%s]--$ ", command_number, username, hostname, cwd);
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

/**
 * Parse the command line into array of strings
 * @param line        line of command
 * @param tokens      string array
 * @param pipe_ptr    num of pipe
 * @param token_ptr   num of strings in array
 * @param redirection boolean, whether need redirection or not 
 * @param background  boolean, whether need background job or not
 * @param ampersand   index of the '&'
 */
void parse_line(char *line, char *tokens[], int *pipe_ptr, int *token_ptr, bool *redirection, bool *background, int *ampersand) {

	char *next_tok = line;
	char *curr_tok;
	int i = 0;

	while(i < 4095 && ((curr_tok = next_token(&next_tok, " \t\r\n\"\'")) != NULL)) {
		if(strncmp(curr_tok, "#", 1) == 0) {
			tokens[i] = (char *) 0;
			break;
		} else if(strcmp(curr_tok, "|") == 0) {
			(*pipe_ptr)++;
			tokens[i] = (char *) 0;
		} else if(strcmp(curr_tok, ">") == 0) {
			*redirection = true;
			tokens[i] = (char *) 0;
		} else {
			tokens[i] = curr_tok;
		}
		if(strncmp(curr_tok, "$", 1) == 0) {
			*ampersand = i;
		}
		i++;
	}

	tokens[i] = (char *) 0;
	(*token_ptr) = i;
	if (i > 0 && strcmp(tokens[i-1], "&") == 0) {
		*background = true;
		tokens[i-1] = (char *) 0;
		
		(*token_ptr) = i - 1;

	}

}

/**
 * Parse the pipe in the command
 * @param tokens    string array
 * @param token_num total number of string
 * @param cmds      struct commands
 * @param output    boolean indicate whether need output or not
 */
void parse_piple(char *tokens[], int token_num, struct command_line cmds[], bool output){

	int i = -1;
	int cmds_index = 0;

	char *curr_tok = NULL;
	if(!output) {
		//if no output
		while(i < token_num) {
			//not reach to the end
			if(curr_tok == NULL) {
				char **cmd_tokens = tokens+(i+1);
				cmds[cmds_index].tokens = cmd_tokens;
				cmds[cmds_index].stdout_pipe = true;
				cmds[cmds_index].stdout_file = NULL;
				cmds_index++;
			}

			i++;
			curr_tok = tokens[i];
		}

		cmds[cmds_index-1].stdout_pipe = false;
	} else {
		//if output needed
		while(i < (token_num-2)) {
			//not reach to the end
			if(curr_tok == NULL) {
				char **cmd_tokens = tokens+(i+1);
				cmds[cmds_index].tokens = cmd_tokens;
				cmds[cmds_index].stdout_pipe = true;
				cmds[cmds_index].stdout_file = NULL;
				cmds_index++;
			}

			i++;
			curr_tok = tokens[i];
		}
		//change the last struct boolean to false
		//no more command behand
		//give the output file name to the last struct
		cmds[cmds_index-1].stdout_pipe = false;
		cmds[cmds_index-1].stdout_file = tokens[token_num-1];
	}
}

/**
 * Handling cd command
 * @param tokens String array
 */
void cd_command(char *tokens[]) {
	char *homedir = getenv("HOME");
	if(tokens[1] == NULL){
		chdir(homedir);
	} else {
		chdir(tokens[1]);
	}
}

/**
 * Set envirment variable
 * @param tokens    string array after tokenization
 * @param token_ptr total number of tokens in the array
 */
void env_command(char *tokens[], int *token_ptr) {

	if(*token_ptr == 3) {
		setenv(tokens[1], tokens[2], 1);
	}
}

/**
 * Handling sigint
 * @param signo 
 */
void sigint_handler(int signo) {

	if(isatty(STDIN_FILENO)) {
		printf("\n");
		fflush(stdout);
		if(!command_execution) {
			print_prompt();
		}
	}
	// exit(0);
}

/**
 * When a background job filished, move the behand job forword
 * @param index the finished job index in the array
 */
void move(int index) {

	for(; index < list_index; index++) {
		pid_list[index] = pid_list[index+1];
		line_list[index] = line_list[index+1];
	}
}

/**
 * Handling sgichild signal
 * @param signo 
 */
void sigchld_handler(int signo) {
	int status;
	pid_t child = waitpid(-1, &status, WNOHANG);

	int i;
	for(i = 0; i < list_index; i++) {
		if(pid_list[i] == child) {
			free(line_list[i]);
			move(i);
			list_index--;
			break;
		}
	}
}

/**
 * Base on the given command, run different history command
 * @param line string array of commands
 */
void history(char *line) {
	char* command = malloc(sizeof(char));
	if(line[1] > 47 && line[1] < 58) {
		//is digit
		find_digit(line+1, command);
	}

	if(line[1] > 96 && line[1] < 123) {
		//is alpha
		find_alpha(line+1, command);
	}

	if(line[1] == 33) {
		//is !!
		find_last_command(command);
	}

	if(command != NULL) {
		//if the command was found
		strcpy(line, command);
	}
	free(command);
}

/**
 * Print out background jobs list
 */
void print_jobs() {
	int i;

	for(i = 0; i < list_index; i++) {
		printf("%d %s\n", pid_list[i], line_list[i]);
	}

}

int main(void) {

	signal(SIGINT, sigint_handler);
	signal(SIGCHLD, sigchld_handler);

	command_number = 0;

	getlogin_r(username, LOGIN_NAME_MAX);
	gethostname(hostname, HOST_NAME_MAX);
	getcwd(cwd, PATH_MAX);
	short_cwd();
	list_index = 0;
	char *line_str;
	command_execution = true;
	while(true) {
		command_execution = false;
		if (isatty(STDIN_FILENO)) {
			print_prompt();
		}

		char *line = NULL;
		size_t line_sz = 0;

		ssize_t sz = getline(&line, &line_sz, stdin);

		char *var = expand_var(line);

		if(var != NULL) {
			//has $
			strcpy(line, var);
			free(var);
		}

		if(sz == EOF || sz == 0) {
			free(line);
			break;
		}

		//!num and !prefix
		if(strncmp(line, "!", 1) == 0) {
			history(line);
		}

		add(line);
		command_number++;

		char *tokens[4096];

		int total_pipe = 0;
		int token_num = 0;
		bool output = false;
		bool background = false;
		int ampersand = -1;

		char line_parse[strlen(line)];
		strcpy(line_parse, line);

		parse_line(line_parse, tokens, &total_pipe, &token_num, &output, &background, &ampersand);

		if(tokens[0] == NULL) {
			free(line);
			continue;
		}

		struct command_line cmds[total_pipe	+1];
		parse_piple(tokens, token_num, cmds, output);

		if(strcmp(tokens[0], "jobs") == 0){
			print_jobs();
			free(line);
			continue;
		}

		if(strcmp(tokens[0], "exit") == 0) {
			free(line);
			free_list();
			exit(0);
		}
		if(strcmp(tokens[0], "clean") == 0) {
			free(line);
			free_list();
			continue;
		}
		if(strcmp(tokens[0], "cd") == 0) {
			cd_command(tokens);
			free(line);
			continue;
		}
		if(strcmp(tokens[0], "setenv") == 0) {
			env_command(tokens, &token_num);
			free(line);
			continue;
		}
		if(strcmp(tokens[0], "history") == 0) {
			print_history();
			free(line);
			continue;
		}

		command_execution = true;
		//fork and execute
		pid_t pid = fork();
		if(pid == 0) {
			//child
			execute_pipeline(cmds);
			fclose(stdin);
		} else if (pid == -1) {
			perror("fork");
		} else {
			//parent
			if(!background){
				int status;
				waitpid(pid, &status, 0);
			} else {
				pid_list[list_index] = pid;
				line[strlen(line)-1] = '\0';
				line_str = malloc(strlen(line)*sizeof(char));
				strcpy(line_str, line);
				line_list[list_index] = line_str;
				list_index++;
			}
		}
		
		free(line);
	}
	free_list();

	return 0;
}
