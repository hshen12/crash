#include <stdbool.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <unistd.h>

#include "leetify.h"

void execute_pipeline(struct command_line *cmds) {

    if (!cmds->stdout_pipe) {                   // no more commands
        if (cmds->stdout_file != NULL) {
            int fd = open(cmds->stdout_file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
            if (fd == -1) {
                perror("open file");
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }
        // printf("command is %s\n", cmds->tokens[0]);
        if (execvp(cmds->tokens[0], cmds->tokens) == -1) {
            perror("execvp");
        }
        return;
    } else {
        int fd[2];
        if (pipe(fd) == -1) {
            perror("pipe");
            return;
        }
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
        } else if (pid == 0) {
            /* Child */
            dup2(fd[1], STDOUT_FILENO);
            close(fd[0]);
            // printf("command is %s\n", cmds->tokens[0]);
            if (execvp(cmds->tokens[0], cmds->tokens) == -1) {
                perror("execvp");
            }
            close(fd[1]);
        } else {
            /* Parent */
            dup2(fd[0], STDIN_FILENO);
            close(fd[1]);
            execute_pipeline(cmds + 1);
            close(fd[0]);
        }
    }
}
