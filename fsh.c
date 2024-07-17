#include "fsh.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    static char *args[] = { "rm", "dummy.txt", NULL };
    char *bin_path = malloc(strlen("/bin/") + strlen(args[0]) + 1); // +1 para o caractere nulo
    if (bin_path == NULL) {
        perror("malloc");
        return 1;
    }
    strcpy(bin_path, "/bin/");
    strcat(bin_path, args[0]);
    //execv(bin_path, args);
    printf("%s\n", bin_path);
    printf("%s\n", args[0]);
    free(bin_path);
    return 0;
}