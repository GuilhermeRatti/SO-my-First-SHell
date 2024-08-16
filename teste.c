#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(){
    // printf("entrada: ");
    // char *line = NULL;
    // size_t size = 0;
    // __ssize_t chars_read;

    // chars_read = getline(&line, &size, stdin);
    // line[strcspn(line, "\n")] = 0;
    // char *token = strtok(line," ");

    // printf("%s",token);

    // free(line);
    char string[10];
    scanf("%s", string);
    if(strcmp(string, "die") == 0){
        printf("die\n");
    }

    return 0;
}