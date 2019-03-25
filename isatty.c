/**
 * isatty.c
 *
 * Determines whether or not stdin is connected to a terminal or if data has
 * been piped in.
 *
 * Compile: gcc -g -Wall isatty.c -o isatty
 * Run: ./isatty
 */
#include <stdio.h>
#include <unistd.h>

int main(void) {

    if (isatty(STDIN_FILENO)) {
        /* Interactive mode; turn on the prompt */
        printf("stdin is a TTY; entering interactive mode\n");
    } else {
        printf("data piped in on stdin; entering script mode\n");
    }

    return 0;
}
