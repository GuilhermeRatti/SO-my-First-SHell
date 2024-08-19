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

void SIGINT_handler_fsh(int signum)
{
    int status;
    char option;
    pid_t pid = waitpid(-1, &status, WNOHANG);
    if (pid == 0)
    {
        if (killpg(getpgrp(), SIGSTOP) == -1)
        {
            perror("Failed to stop processes in session");
            kill(getpid(), SIGKILL);
        }
        printf("Ainda existem processos rodando. Tem certeza que deseja finalizar a fsh? (y/n)");
        scanf("%c", &option);
        if (option == 'y' || option == 'Y')
        {
            if (killpg(getpgrp(), SIGINT) == -1)
            {
                perror("Failed to stop processes in session");
                kill(getpid(), SIGKILL);
            }
            kill(getpid(), SIGINT);
        }
        else
        {
            if (killpg(getpgrp(), SIGCONT) == -1)
            {
                perror("Failed to continue processes in session");
                kill(getpid(), SIGKILL);
            }
        }
    }
    else if (pid > 0)
    {
        kill(getpid(), SIGINT);
    }
    else
    {
        perror("waitpid failed");
        exit(-1); // Erro ao verificar processos filhos
    }
}

void SIGINT_handler_child(int singum)
{
    pid_t pgid = getpgrp();
    printf("Processo %d finalizado com status 130\n", pgid);
    killpg(pgid, SIGINT);
}

void SIGSTP_handler_fsh(int signum)
{
    killpg(getpgrp(), SIGSTOP);
}

void SIGSTP_handler_child(int signum)
{
    pid_t pgid = getpgrp();
    printf("Processo %d bloqueado com status 148\n", pgid);
    killpg(pgid, SIGSTOP);
}