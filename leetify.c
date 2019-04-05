#include <stdbool.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <unistd.h>

#include "leetify.h"


void print_cmd(struct command_line cmds) {
  int i = 0;
  printf("print\n" );
  while (cmds.tokens[i] != NULL) {
    printf("while\n" );
    printf("%s ", cmds.tokens[i++]);
  }
  printf("done\n" );
}












void execute_pipeline(struct command_line *cmds)
{
  // printf("leetif\n" );

    if(cmds->stdout_pipe == false){
      printf("base case\n" );
        if(cmds->stdout_file != NULL){
            int output = open("output.txt", O_CREAT | O_WRONLY | O_TRUNC, 0666);
            dup2(output, STDOUT_FILENO);

        }
        // printf("+++++++++++++++++++++\n" );
        print_cmd(*cmds);
        // printf("++++++++++++++++++++\n" );
        execvp(cmds->tokens[0], cmds->tokens);
        return;
    }

    int fd[2];
    if (pipe(fd) == -1) {
        perror("pipe");
        return;
    }

    pid_t pid = fork();
    if(pid == 0) {
        //child
        printf("recursion\n" );
        close(fd[0]);
        dup2(fd[1], STDOUT_FILENO);
        // printf("0 is %s\n", cmds->tokens[0]);
        print_cmd(*cmds);
        execvp(cmds->tokens[0], cmds->tokens);
        close(fd[1]);
    } else if (pid == -1) {
        perror("fork");
    } else {
        dup2(fd[0], STDIN_FILENO);
        close(fd[1]);
        execute_pipeline(cmds+1);
        close(fd[0]);
    }
}
