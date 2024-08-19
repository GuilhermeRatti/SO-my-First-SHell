#include "fsh.h"
#include "io.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <termios.h>

// TODO: (SHELLCOMMANDS) IMPLEMENTAR A FUNCAO "WAITALL" DO FSH

// Só mata grupos com processos ativos; ignora indices com -1
static void _die(/*pid_t *active_group_ids, int active_group_ids_count*/)
{
    // for (int i = 0; i < active_group_ids_count; i++)
    // {
    //     printf("Trying to kill %d\n", active_group_ids[i]);
    //     if (active_group_ids[i] != -1)
    //         killpg(active_group_ids[i], SIGKILL);
    // }
    // free(active_group_ids);
    killpg(getpgrp(), SIGKILL);

    exit(EXIT_SUCCESS);
}

// // Verifica se cada grupo possui ao menos 1 processo ativo; senão, define indice como -1
// static void _check_groups(pid_t *active_group_ids, int active_group_ids_count)
// {
//     for (int i = 0; i < active_group_ids_count; i++)
//     {
//         if (killpg(active_group_ids[i], 0) == -1)
//         {
//             active_group_ids[i] = -1;
//         }
//     }
// }

// // Verifica se ha algum espaco livre (-1) em algum indice; senao, aloca espaco para mais um grupo
// static pid_t *_register_group(pid_t *active_group_ids, int *active_group_ids_count, int current_group_id)
// {
//     int found_place = 0;
//     for (int i = 0; i < *active_group_ids_count; i++)
//     {
//         if (active_group_ids[i] == -1)
//         {
//             printf("entrou iha!\n");
//             active_group_ids[i] = current_group_id;
//             found_place = 1;
//             break;
//         }
//     }

//     if (!found_place)
//     {
//         *active_group_ids_count += 1;
//         active_group_ids = (pid_t *)realloc(active_group_ids, sizeof(pid_t) * (*active_group_ids_count));
//         active_group_ids[*active_group_ids_count - 1] = current_group_id;
//     }

//     return active_group_ids;
// }

static void setup_signal_handler_fsh()
{
    struct sigaction sa_int, sa_stp;

    sa_int.sa_handler = SIGINT_handler_fsh;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = 0;

    if (sigaction(SIGINT, &sa_int, NULL) == -1)
    {
        perror("Erro ao configurar manipulador para SIGINT");
        exit(1);
    }

    sa_stp.sa_handler = SIGSTP_handler_fsh;
    sigemptyset(&sa_stp.sa_mask);
    sa_stp.sa_flags = 0;

    if (sigaction(SIGTSTP, &sa_stp, NULL) == -1)
    {
        perror("Erro ao configurar manipulador para SIGTSTP");
        exit(1);
    }
}

static void setup_signal_handler_child()
{
    struct sigaction sa_int, sa_stp;

    sa_int.sa_handler = SIGINT_handler_child;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = 0;

    if (sigaction(SIGINT, &sa_int, NULL) == -1)
    {
        perror("Erro ao configurar manipulador para SIGINT");
        exit(1);
    }

    sa_stp.sa_handler = SIGSTP_handler_child;
    sigemptyset(&sa_stp.sa_mask);
    sa_stp.sa_flags = 0;

    if (sigaction(SIGTSTP, &sa_stp, NULL) == -1)
    {
        perror("Erro ao configurar manipulador para SIGTSTP");
        exit(1);
    }
}

static void setup_default_signal_handler()
{
    struct sigaction sa_default;

    // Redefine o manipulador para SIGINT para o comportamento padrão
    sa_default.sa_handler = SIG_DFL;
    sigemptyset(&sa_default.sa_mask);
    sa_default.sa_flags = 0;

    if (sigaction(SIGINT, &sa_default, NULL) == -1)
    {
        perror("Erro ao redefinir manipulador para SIGINT");
        exit(1);
    }

    // Redefine o manipulador para SIGTSTP para o comportamento padrão
    if (sigaction(SIGTSTP, &sa_default, NULL) == -1)
    {
        perror("Erro ao redefinir manipulador para SIGTSTP");
        exit(1);
    }
}

int main()
{
    char *commands_vec[5];
    // TODO: (SIGNALS) IMPLEMENTAR TRATAMENTO DE SINAIS
    //  Lembrar de implementar tratamento de interrupcoes para o pai. Ate agora, estamos usando um loop finito para determinar o comportamento da fsh.
    setsid();

    //int active_group_ids_count = 1;
    //pid_t *active_group_ids = (pid_t *)malloc(sizeof(pid_t) * 1);
    //active_group_ids[0] = -1;
    setup_signal_handler_fsh();

    while (1)
    {
        print_prompt();
        char *line = read_line();
        int commands_count = get_commands(line, commands_vec);
        pid_t foreground_pid = 0, command_pid = 0;

        // TODO: (SHELLCOMMANDS) IMPLEMENTAR VERIFICACAO DE COMANDOS DE SHELL
        //  Implementar verificacao de comandos de shell para a fsh antes de executar os comandos (novos processos).
        for (int i = 0; i < commands_count; i++)
        {
            char *cleaned_command = remove_whitespace(commands_vec[i]);
            if (strcmp(cleaned_command, "die") == 0)
            {
                free(cleaned_command);

                for (int j = i; j < commands_count; j++)
                {
                    free(commands_vec[j]);
                }

                // Verifica se os grupos de processos ainda estão ativos antes de matar todos.
                //_check_groups(active_group_ids, active_group_ids_count);
                _die(/*active_group_ids, active_group_ids_count*/);
            }
            free(cleaned_command);

            command_pid = command_execution(commands_vec[i], i);

            if (i == 0)
            {
                foreground_pid = command_pid;
            }
            else if (i > 0)
            {
                move_to_foreground(command_pid);
            }

            setpgid(command_pid, foreground_pid);
            free(commands_vec[i]);
        }
        // Verifica se algum grupo de processo foi finalizado para sinalizar espaço livre.
        //_check_groups(active_group_ids, active_group_ids_count);
        // Registra o grupo de processos em um espaço livre ou aloca espaço extra, se neceesario.
        // active_group_ids = _register_group(active_group_ids, &active_group_ids_count, foreground_pid);

        wait_for_child(foreground_pid);
    }
    return 0;
}

void wait_for_child(pid_t foreground_pid)
{
    int status;
    pid_t pid = waitpid(foreground_pid, &status, 0);
    
    //move_to_foreground(getpid());

    printf("Processo %d finalizado com status %d\n", pid, status);
}

pid_t command_execution(char *command, int command_position)
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
        exit(EXIT_FAILURE);
    }
    strcpy(bin_path, "/bin/");
    strcat(bin_path, args_vec[0]);

    pid_t pid = fork();

    // Filho executa o processo
    if (pid == 0)
    {
        pid_t grandchild_pid;
        // Se nao for o primeiro comando (em foreground), redireciona a saida padrao dos processos em background para "/dev/null".
        if (command_position)
        {
            freopen("/dev/null", "w", stdout);
            freopen("/dev/null", "w", stderr);

            if ((grandchild_pid = fork()) == 0)
            {
                setup_default_signal_handler();
            }
            else if (grandchild_pid > 0)
            {
                setup_signal_handler_child();
            }
            else
            {
                perror("Erro ao criar neto");
                exit(EXIT_FAILURE);
            }
        }

        execv(bin_path, args_vec);
    }

    // O pai trata de liberar a memoria alocada
    free(bin_path);
    for (int i = 0; i < args_count; i++)
    {
        free(args_vec[i]);
    }

    return pid;
}
