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

// TODO: (SHELLCOMMANDS) IMPLEMENTAR A FUNCAO "WAITALL" DO FSH

int fsh_interrupted = 0;

int triplets_amount = 0;
pid_t **process_triplets; // 0 - pid do processo; 1 - pid do processo em foreground; 2 - pid do grupo de processo em background

// int active_group_ids_count = 0;
// pid_t *active_group_ids;

// Só mata grupos com processos ativos; ignora indices com -1
static void _die()
{
    for (int i = 0; i < triplets_amount; i++)
    {
        // printf("Trying to kill %d\n", active_group_ids[i]);
        if (process_triplets[i][0] != -1)
            killpg(process_triplets[i][0], SIGKILL);
    }
    // free(active_group_ids);

    for (int i = 0; i < triplets_amount; i++)
    {
        free(process_triplets[i]);
    }
    free(process_triplets);

    exit(EXIT_SUCCESS);
}

// Verifica se cada grupo possui ao menos 1 processo ativo; senão, define indice como -1
static void _check_groups()
{
    for (int i = 0; i < triplets_amount; i++)
    {
        if (killpg(process_triplets[i][0], 0) == -1)
        {
            process_triplets[i][0] = -1;
        }
    }
}

// Verifica se ha algum espaco livre (-1) em algum indice; senao, aloca espaco para mais um grupo
static pid_t **_register_process(int process_id, int foreground_id, int background_id)
{
    int found_place = 0;
    for (int i = 0; i < triplets_amount; i++)
    {
        if (process_triplets[i][0] == -1)
        {
            process_triplets[i][0] = process_id;
            process_triplets[i][1] = foreground_id;
            process_triplets[i][2] = background_id;
            found_place = 1;
            break;
        }
    }

    if (!found_place)
    {
        process_triplets = (pid_t **)realloc(process_triplets, sizeof(pid_t *) * (triplets_amount + 1));
        process_triplets[triplets_amount] = (pid_t *)malloc(sizeof(pid_t) * 3);
        process_triplets[triplets_amount][0] = process_id;
        process_triplets[triplets_amount][1] = foreground_id;
        process_triplets[triplets_amount][2] = background_id;
        triplets_amount++;
    }

    return process_triplets;
}

void SIGINT_handler_fsh(int signum)
{
    int status;
    pid_t pid = waitpid(-1, &status, WNOHANG);
    if (pid == 0)
    {
        fsh_interrupted = 1;
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

int main()
{
    char *commands_vec[5];
    struct sigaction fsh_sigint;
    sigemptyset(&fsh_sigint.sa_mask);
    fsh_sigint.sa_handler = SIGINT_handler_fsh;
    fsh_sigint.sa_flags = 0;

    if (sigaction(SIGINT, &fsh_sigint, NULL) < 0)
    {
        perror("fsh sigint failed");
        exit(EXIT_FAILURE);
    }

    // TODO: (SIGNALS) IMPLEMENTAR TRATAMENTO DE SINAIS
    //  Lembrar de implementar tratamento de interrupcoes para o pai. Ate agora, estamos usando um loop finito para determinar o comportamento da fsh.

    process_triplets = (pid_t **)malloc(sizeof(pid_t *) * 0); // Simplesmente inicializa o vetor de tuplas de processos

    while (1)
    {
        print_prompt();
        char *line = read_line();
        if(line == NULL && fsh_interrupted)
        {
            fsh_interrupted = 0;
            /*
                Tratamento do CTRL+C
            */
            continue;
        }
        else if(line == NULL)
        {
            perror("read_line failed");
            exit(EXIT_FAILURE);
        }
        int commands_count = get_commands(line, commands_vec);
        pid_t foreground_pid = 0, background_pgid = 0, command_pid = 0;

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
                _check_groups();
                _die();
            }
            free(cleaned_command);

            command_pid = command_execution(commands_vec[i], i);

            if (i == 0)
            {
                foreground_pid = command_pid;
            }
            else if (i == 1)
            {
                background_pgid = command_pid;
            }
            if (i >= 1)
            {
                process_triplets = _register_process(command_pid, foreground_pid, background_pgid);
                setpgid(command_pid, background_pgid);
            }

            free(commands_vec[i]);
        }

        process_triplets = _register_process(foreground_pid, foreground_pid, background_pgid);

        // Verifica se algum grupo de processo foi finalizado para sinalizar espaço livre.
        _check_groups();
        // Registra o grupo de processos em um espaço livre ou aloca espaço extra, se neceesario.
        setpgid(getpid(), getppid());
        if (foreground_pid)
            wait_for_child(foreground_pid);
    }
    return 0;
}

void wait_for_child(pid_t foreground_pid)
{
    int status;
    pid_t pid = waitpid(foreground_pid, &status, 0);
    setpgid(getpid(), getpid());

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
        // Se nao for o primeiro comando (em foreground), redireciona a saida padrao dos processos em background para "/dev/null".
        if (command_position)
        {
            freopen("/dev/null", "w", stdout);
            freopen("/dev/null", "w", stderr);

            fork();
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