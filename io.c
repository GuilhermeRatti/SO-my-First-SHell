#include "io.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void print_prompt()
{
    printf("fsh> ");
}

char *read_line()
{
    char *line = NULL;
    size_t size = 0;
    __ssize_t chars_read;

    chars_read = getline(&line, &size, stdin);
    return line;
}



int main()
{
    while (1)
    {
        print_prompt();
        char *line = read_line();
        free(line);
        exit(0);
    }
    return 0;
}