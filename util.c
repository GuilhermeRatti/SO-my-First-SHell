#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <termios.h>
#include <ctype.h>

void move_to_foreground(pid_t pgid)
{
    if(getpid() != getpgrp())
    {
        printf("OIOIOIOIOI\n");
    }
    // Obtenha o terminal associado ao processo atual
    int terminal_fd = STDIN_FILENO;

    // Mova o grupo de processos para o foreground
    if (tcsetpgrp(terminal_fd, pgid) == -1)
    {
        perror("tcsetpgrp failed");
        exit(1);
    }
}

// Função para remover espaços em branco de uma string
char *remove_whitespace(const char *str)
{
    size_t length = strlen(str);
    char *result = (char *)malloc(length + 1); // Aloca memória para o resultado
    if (result == NULL)
    {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    char *p = result;
    for (size_t i = 0; i < length; i++)
    {
        if (!isspace((unsigned char)str[i]))
        {
            *p++ = str[i];
        }
    }
    *p = '\0'; // Adiciona o caractere nulo ao final da string

    return result;
}

