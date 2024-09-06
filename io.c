#include "io.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>
#include <errno.h>



void print_prompt()
{
    printf("fsh> ");
}

char *read_line()
{
    char *line = NULL; 
    size_t size = 0;
    int ret = getline(&line, &size, stdin);
    if(ret < 0 && errno == EINTR)
    {
        clearerr(stdin);
        errno = 0;
        if(line != NULL)
            free(line);
        return NULL;
    }
    else if(ret < 0)
    {
        printf("%s\n", strerror(errno));
        printf("%s\n",line);
        err(EXIT_FAILURE, "read_line failed");
    }

    line[strcspn(line, "\n")] = 0;
    return line;
}

int get_commands(char *line, char *commands_vec[])
{
    int commands_count = 0;
    char *token = strtok(line, "#");

    while (token != NULL)
    {
        if (strcmp(token, "\n") == 0)
            break;
        if (commands_count >= 5)
            break;

        char *new_command = (char *)malloc(strlen(token) + 1);
        strcpy(new_command, token);
        commands_vec[commands_count++] = new_command;
        token = strtok(NULL, "#");
    }

    free(line);

    return commands_count;
}