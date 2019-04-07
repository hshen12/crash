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

#define JOB_LIST_SIZE 10

int command_number;
char username[LOGIN_NAME_MAX];
char hostname[HOST_NAME_MAX];
char cwd[PATH_MAX];
pid_t pid_list[JOB_LIST_SIZE];
int list_index;
char *line_list[JOB_LIST_SIZE];

/**
* Print shell promt information
*/
void print_prompt() {
	printf("--[%d|%s@%s:~%s]--$ ", command_number, username, hostname, cwd);
	fflush(stdout);
}


int string_ends_with(const char * str, const char * suffix)
{
  int str_len = strlen(str);
  int suffix_len = strlen(suffix);

  return 
    (str_len >= suffix_len) &&
    (0 == strcmp(str + (str_len-suffix_len), suffix));
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

void parse_line(char *line, char *tokens[], int *pipe_ptr, int *token_ptr, bool *redirection, bool *background) {

	char *next_tok = line;
	char *curr_tok;
	int i = 0;

	while(i < 4095 && ((curr_tok = next_token(&next_tok, " \t\r\n")) != NULL)) {
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
		i++;
	}

	tokens[i] = (char *) 0;
	(*token_ptr) = i;
	// printf("str is |%s|\n", line);
	if (i > 0 && strcmp(tokens[i-1], "&") == 0) {
	// if(string_ends_with(line, "&")) {
		*background = true;
		// printf("str is %s\n", tokens[i-1]);
		tokens[i-1] = (char *) 0;
		
		(*token_ptr) = i - 1;

	}

}

void parse_piple(char *tokens[], int token_num, struct command_line cmds[], bool output){

	int i = -1;
	int cmds_index = 0;

	char *curr_tok = NULL;
	if(!output) {

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
		cmds[cmds_index-1].stdout_pipe = false;
		cmds[cmds_index-1].stdout_file = tokens[token_num-1];
	}
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

void env_command(char *tokens[], int *token_ptr) {

	if(*token_ptr == 3) {
		setenv(tokens[1], tokens[2], 1);
	}
}

void sigint_handler(int signo) {

	fflush(stdout);
	// exit(0);
}

void move(int index) {
	for(; index < list_index; index++) {
		pid_list[index] = pid_list[index+1];
		line_list[index] = line_list[index+1];
	}
}

void sigchld_handler(int signo) {
	int status;
	pid_t child = waitpid(-1, &status, WNOHANG);
	// while (child = waitpid(-1, &status, WNOHANG) != 0) {



	int i = 0;
	for(i = 0; i < list_index; i++) {
		if(pid_list[i] == child) {
			list_index--;
			move(i);
			break;
		}
	// }
}
}

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
		find_last_command(command);
	}

	if(command != NULL) {
		//if the command was found
		strcpy(line, command);
	}
	free(command);
}

int main(void) {

	// signal(SIGINT, sigint_handler);
	signal(SIGCHLD, sigchld_handler);

	command_number = 0;

	getlogin_r(username, LOGIN_NAME_MAX);
	gethostname(hostname, HOST_NAME_MAX);
	getcwd(cwd, PATH_MAX);
	short_cwd();

	while(true) {
		list_index = 0;
		if (isatty(STDIN_FILENO)) {
			print_prompt();
		}

		char *line = NULL;
		size_t line_sz = 0;

		ssize_t sz = getline(&line, &line_sz, stdin);

		if(sz == EOF || sz == 0) {
			break;
		}

		//!num and !prefix
		if(strncmp(line, "!", 1) == 0) {
			history(line);
		}

		add(line);

		char *tokens[4096];

		int total_pipe = 0;
		int token_num = 0;
		bool output = false;
		bool background = false;

		parse_line(line, tokens, &total_pipe, &token_num, &output, &background);
		struct command_line cmds[total_pipe	+1];
		parse_piple(tokens, token_num, cmds, output);

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
			env_command(tokens, &token_num);
		}
		if(strcmp(tokens[0], "history") == 0) {
			print_history();
			continue;
		}

		// printf("bool is %d\n", background);
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
					// printf("cmd is %s\n", cmds[0].tokens[0]);
					// printf("cmd is %s\n", cmds[0].tokens[1]);
					// printf("cmd is %s\n", cmds[0].tokens[3]);
					char line_str[strlen(line) + 1];
					strcpy(line_str, line);
					line_list[list_index] = line_str;
					list_index++;
				}
			}
			// //background job
			// pid_t pid = fork();
			// if(pid == 0) {
			// 	//child
			// 	execute_pipeline(cmds);
			// 	fclose(stdin);
			// } else if (pid == -1) {
			// 	perror("fork");
			// } else {
			// 	//parent
			// 	pid_list[list_index] = pid;
			// 	char line_str[strlen(line)];
			// 	strcpy(line_str, line);
			// 	line_list[list_index] = line_str;
			// 	list_index++;
			// 	// int status;
			// 	// waitpid(pid, &status, 0);
			// }
		
		free(line);
	}

	return 0;
}
