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

#endif
