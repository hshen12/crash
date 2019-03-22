#include <fcntl.h>
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

int main(void) {
    command_number = 0;

    getlogin_r(username, 10);
    gethostname(hostname, 50);
    getcwd(cwd, PATH_MAX);
    short_cwd();

    while(true) {
        // print_prompt();
        char *line = NULL;
        size_t line_sz = 0;

        ssize_t sz = getline(&line, &line_sz, stdin);
        if(sz == EOF) {
            break;
        }

        char *token = strtok(line, " \t\n");
        char *tokens[4096];
        int i = 0;

        while(i < 4095 && token != NULL) {
            tokens[i++] = token;
            // printf("token is %s\n", token);
            token = strtok(NULL, " \t\n");
        }
        // if(i >= 16) {
        //     perror("too many args");
        //     break;
        // }
        // printf("i is %d\n", i);
        tokens[i] = (char *) 0;

        pid_t pid = fork();
        if(pid == 0) {
            //child
            execvp(tokens[0], tokens);
            fclose(stdin);
        } else if (pid == -1) {
            perror("fork");
        } else {
            int status;
            wait(&status);
        }

    }

    return 0;
}
