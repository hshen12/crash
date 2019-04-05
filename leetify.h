#ifndef _LEETIF_H_
#define _LEETIF_H_

struct command_line {
    char **tokens;
    bool stdout_pipe;
    char *stdout_file;
};

void execute_pipeline(struct command_line *cmds);
void print_cmd(struct command_line cmds);

#endif
