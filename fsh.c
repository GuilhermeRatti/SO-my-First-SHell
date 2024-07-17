#include "fsh.h"
#include "io.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main()
{
    char *commands_vec[5];
    //TODO: (SIGNALS) IMPLEMENTAR TRATAMENTO DE SINAIS 
    // Lembrar de implementar tratamento de interrupcoes para o pai. Ate agora, estamos usando um loop finito para determinar o comportamento da fsh.
    int j = 0;
    while (j < 2)
    {
        print_prompt();
        char *line = read_line();
        int commands_count = get_commands(line, commands_vec);
        
        //TODO: (SHELLCOMMANDS) IMPLEMENTAR VERIFICACAO DE COMANDOS DE SHELL
        // Implementar verificacao de comandos de shell para a fsh antes de executar os comandos (novos processos).
        for (int i = 0; i < commands_count; i++)
        {
            command_execution(commands_vec[i]);
            free(commands_vec[i]);
        }

        wait_for_children(commands_count);
        j++;
    }
    return 0;
}

//TODO: (BACK/FOREGROUND) WAIT FOR CHILDREN PRECISA DE MUDANCAS 
// A funcao wait_for_children deve esperar pelo processo em foreground somente.
void wait_for_children(int commands_count)
{
    int status;
    pid_t pid;
    while (commands_count > 0) 
    {
        pid = wait(&status);
        printf("Child with PID %ld exited with status 0x%x.\n", (long)pid, status);
        commands_count--;
    }
}

//TODO: (BACK/FOREGROUND) COMMAND EXECUTION PRECISA DE AJUSTES
// A funcao command_execution deve saber se deve executar o processo em background ou em foreground.
void command_execution(char *command)
{
    int args_count = 0;

    char *args_vec[4]; // O proprio processo + 2 parametros + NULL = 4 argumentos.
    char *token = strtok(command, " ");

    while (token != NULL)
    {
        if (strcmp(token, "\n") == 0)
            break;
        char *new_arg = (char *)malloc(strlen(token) + 1);
        strcpy(new_arg, token);
        args_vec[args_count++] = new_arg;
        token = strtok(NULL, " ");
    }

    args_vec[args_count] = NULL;

    char *bin_path = (char *)malloc(strlen("/bin/") + strlen(args_vec[0]) + 1); // +1 para o caractere nulo
    if (bin_path == NULL)
    {
        perror("malloc do /bin/");
        exit(1);
    }
    strcpy(bin_path, "/bin/");
    strcat(bin_path, args_vec[0]);

    pid_t pid = fork();
    
    // Filho executa o processo
    if (pid == 0)
        execv(bin_path, args_vec);
    
    // O pai trata de liberar a memoria alocada
    free(bin_path);
    for (int i = 0; i < args_count; i++)
    {
        free(args_vec[i]);
    }
    free(args_vec);
}