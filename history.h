#ifndef _HISTORY_H_
#define _HISTORY_H_

#define HIST_MAX 100

struct list_node_s {
    char *command;
    int index;
    double run_time;
    struct list_node_s *prev_p;
    struct list_node_s *next_p;
    /* What else do we need here? */
};

void add(char *command);
void print_history();
void find_digit(char *num_str, char* command);
void find_alpha(char *prefix, char *command);
void find_last_command(char *command);

#endif
