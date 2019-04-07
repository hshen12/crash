#include "history.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct list_node_s *h_p = NULL;
struct list_node_s *t_p = NULL;
int list_size = 0;

void free_node(struct list_node_s* node_p) {
  free(node_p->command);
  free(node_p);
}

struct list_node_s* allocate_node(int size) {

  struct list_node_s* newNode = (struct list_node_s*)malloc(sizeof(struct list_node_s));
  newNode->index = list_size;
  newNode->command = malloc((size+1)*sizeof(char));
  newNode->next_p = NULL;
  newNode->prev_p = NULL;
  return newNode;
}

void limit_list() {
  struct list_node_s* temp = t_p;
  temp->prev_p->next_p = NULL;
  t_p = temp->prev_p;
  free_node(temp);
}

void add(char *command) {
  if(h_p == t_p && h_p == NULL) {
    struct list_node_s* newNode = allocate_node(strlen(command));
    // printf("size is %ld\n", sizeof(newNode));
    strcpy(newNode->command, command);
    h_p = newNode;
    t_p = newNode;
    list_size++;
    return;
  } else {
    //add to the front
    struct list_node_s* newNode = allocate_node(strlen(command));
    strcpy(newNode->command, command);
    newNode->next_p = h_p;
    h_p->prev_p = newNode;
    h_p = newNode;
    list_size++;
    if(list_size > HIST_MAX) {
      limit_list();
    }
    return;
  }
}

void find_digit(char *num_str, char* command){
  struct list_node_s* curr_p = h_p;
  int num = atoi(num_str);
  while(curr_p != NULL) {
    if(curr_p->index == num) {
      command = realloc(command, strlen(curr_p->command)*sizeof(char));
      strcpy(command, curr_p->command);
      return;
    }
    curr_p = curr_p->next_p;
  }
}

void find_last_command(char *command) {
  command = realloc(command, strlen(h_p->command)*sizeof(char));
  strcpy(command, h_p->command);
}


void find_alpha(char *prefix, char *command){
  struct list_node_s* curr_p = h_p;
  while (curr_p != NULL) {
    if(strncmp(curr_p->command, prefix, strlen(prefix)-1) == 0) {
      command = realloc(command, strlen(curr_p->command)*sizeof(char));
      strcpy(command, curr_p->command);
      return;
    }
    curr_p = curr_p->next_p;
  }
}


void print_history() {
  struct list_node_s* curr_p = t_p;

  while (curr_p != NULL) {
    printf("%d %s", curr_p->index, curr_p->command);
    curr_p = curr_p->prev_p;
  }
  printf("\n");
}

void free_list() {

  struct list_node_s* node_p = h_p;
  h_p = NULL;
  t_p = NULL;

  while(node_p != NULL){
    struct list_node_s* tmp = node_p;

    node_p = node_p->next_p;

    if(tmp->next_p != NULL)
      tmp->next_p->prev_p = tmp->prev_p;
    tmp->next_p = NULL;
    free_node(tmp);
  }

}
