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

int command_number;
char username[LOGIN_NAME_MAX];
char hostname[HOST_NAME_MAX];
char cwd[PATH_MAX];
// int token_num;

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

bool parse_line(char *line, char *tokens[], int *pipe_ptr, int *token_ptr) {

	char *next_tok = line;
	char *curr_tok;
	int i = 0;
  bool redirection = false;

	while(i < 4095 && ((curr_tok = next_token(&next_tok, " \t\r\n")) != NULL)) {
    if(strncmp(curr_tok, "#", 1) == 0) {
      tokens[i] = (char *) 0;
    } else if(strcmp(curr_tok, "|") == 0) {
      (*pipe_ptr)++;
			tokens[i] = (char *) 0;
    } else if(strcmp(curr_tok, ">") == 0) {
      redirection = true;
      // (*pipe_ptr)++;
      tokens[i] = (char *) 0;
    } else {
      tokens[i] = curr_tok;
    }
		i++;
	}

	tokens[i] = (char *) 0;
	(*token_ptr) = i;
  return redirection;
}

void parse_piple(char *tokens[], int *token_num, struct command_line cmds[], bool output){
  // printf("output is %s\n", tokens[*token_num-1]);
  int i = -1;
  int cmds_index = 0;

  char *curr_tok = NULL;
  if(output) {
    while(i < (*token_num)) {
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
    printf("here\n" );
    while(i < (*token_num)-2) {
      printf("curr is %s\n", curr_tok);
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
    cmds[cmds_index-1].stdout_file = tokens[*token_num-1];
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

	command_number = 0;

	getlogin_r(username, LOGIN_NAME_MAX);
	gethostname(hostname, HOST_NAME_MAX);
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

    add(line);

    //!num and !prefix
		if(strncmp(line, "!", 1) == 0) {
			history(line);
		}

		char *tokens[4096];

		int total_pipe = 0;
		int token_num = 0;
		int *pipe_ptr = &total_pipe;
		int *token_ptr = &token_num;

		bool output = parse_line(line, tokens, pipe_ptr, token_ptr);
    struct command_line cmds[total_pipe	+1];
    parse_piple(tokens, token_ptr, cmds, output);

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
			env_command(tokens, token_ptr);
		}
		if(strcmp(tokens[0], "history") == 0) {
			print_history();
			continue;
		}

		pid_t pid = fork();
		if(pid == 0) {
			//child
			execute_pipeline(cmds);
			fclose(stdin);
		} else if (pid == -1) {
			perror("fork");
		} else {
			//parent
			int status;
			wait(&status);
		}
		free(line);
	}

	return 0;
}
