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
#include <err.h>
#include <errno.h>

int fsh_int_with_children = 0;
int fsh_int_no_children = 0;

int triplets_amount = 0;
pid_t **process_triplets = NULL; // 0 - pid do processo; 1 - pid do processo em foreground; 2 - pid do grupo de processo em background

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
        if (triplets_amount == 0)
            process_triplets = (pid_t **)malloc(sizeof(pid_t *) * 1);
        else
            process_triplets = (pid_t **)realloc(process_triplets, sizeof(pid_t *) * (triplets_amount + 1));
        process_triplets[triplets_amount] = (pid_t *)malloc(sizeof(pid_t) * 3);
        process_triplets[triplets_amount][0] = process_id;
        process_triplets[triplets_amount][1] = foreground_id;
        process_triplets[triplets_amount][2] = background_id;
        triplets_amount++;
    }

    return process_triplets;
}

void sigchld_handler(int sig)
{
    int status;
    pid_t pid;

    // Usar um loop para lidar com múltiplos filhos que podem ter terminado
    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0)
    {
        if (WIFEXITED(status))
        {
            // Muito bem, nao precisa fazer nada, que maravilha! :^)
        }
        else if (WIFSIGNALED(status))
        {
            // Vamos ter que achar o grupo de background e foreground desse processo...
            for (int i = 0; i < triplets_amount; i++)
            {
                if (process_triplets[i][0] == pid)
                {
                    kill(process_triplets[i][1], WTERMSIG(status));
                    killpg(process_triplets[i][2], WTERMSIG(status));
                    break;
                }
            }
            // printf("Process %d was killed by signal %d.\n", pid, WTERMSIG(status));
        }
        else if (WIFSTOPPED(status))
        {
            // Vamos ter que achar o grupo de background e foreground desse processo...
            for (int i = 0; i < triplets_amount; i++)
            {
                if (process_triplets[i][0] == pid)
                {
                    kill(process_triplets[i][1], WSTOPSIG(status));
                    killpg(process_triplets[i][2], WSTOPSIG(status));
                    break;
                }
            }
            // printf("Process %d was stopped by signal %d.\n", pid, WSTOPSIG(status));
        }
    }

    if (pid == -1 && errno != ECHILD)
    {
        perror("waitpid SIGCHILD");
    }
}

void SIGINT_handler_fsh(int signum)
{
    int status;
    pid_t pid = waitpid(-1, &status, WNOHANG);
    if (pid == 0)
    {
        fsh_int_with_children = 1;
    }
    else if (pid < 0 && errno == ECHILD)
    {
        fsh_int_no_children = 1;
    }
    else
    {
        perror("Somehow fsh SIGINT failed");
        exit(-1);
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
    for (int i = 0; i < triplets_amount; i++)
    {
        // printf("Trying to kill %d\n", active_group_ids[i]);
        if (process_triplets[i][0] != -1)
            kill(process_triplets[i][0], SIGSTOP);
    }
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

    struct sigaction fsh_sigstp;
    sigemptyset(&fsh_sigstp.sa_mask);
    fsh_sigstp.sa_handler = SIGSTP_handler_fsh;
    fsh_sigstp.sa_flags = 0;

    if (sigaction(SIGTSTP, &fsh_sigstp, NULL) < 0)
    {
        perror("fsh sigstp failed");
        exit(EXIT_FAILURE);
    }

    struct sigaction fsh_sigchild;
    fsh_sigchild.sa_handler = sigchld_handler;
    sigemptyset(&fsh_sigchild.sa_mask);
    fsh_sigchild.sa_flags = SA_RESTART | SA_NOCLDSTOP;

    if (sigaction(SIGCHLD, &fsh_sigchild, NULL) == -1)
    {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        print_prompt();
        char *line = read_line();
        if (line == NULL && fsh_int_with_children)
        {
            fsh_int_with_children = 0;
            char option;
            for (int i = 0; i < triplets_amount; i++)
            {
                // printf("Trying to kill %d\n", active_group_ids[i]);
                if (process_triplets[i][0] != -1)
                    killpg(process_triplets[i][0], SIGSTOP);
            }

            printf("\nAinda existem processos rodando. Tem certeza que deseja finalizar a fsh? (y/n): ");
            scanf("%c", &option);

            if (option == 'y' || option == 'Y')
            {
                free(line);
                _die();
            }
            else
            {
                // Limpa o buffer de entrada descartando tudo até o fim da linha
                scanf("%*[^\n]");
                getchar(); // captura o \n deixado para trás

                for (int i = 0; i < triplets_amount; i++)
                {
                    // printf("Trying to kill %d\n", active_group_ids[i]);
                    if (process_triplets[i][0] != -1)
                        killpg(process_triplets[i][0], SIGCONT);
                }
            }

            continue;
        }
        else if (line == NULL && fsh_int_no_children)
        {
            fsh_int_no_children = 0;
            free(line);
            _die();
        }
        else if (line == NULL)
        {
            printf("\n");
            continue;
        }

        int commands_count = get_commands(line, commands_vec);
        pid_t foreground_pid = 0, background_pgid = 0, command_pid = 0;

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
            if (strcmp(cleaned_command, "waitall") == 0)
            {
                // TODO: (DOCUMENTAR) FALAR SOBRE NOSSA LIMITACAO EM RELACAO AO WAITALL COM PROCESSOS NETOS
                free(cleaned_command);
                kill(getpid(), SIGCHLD);
                // continue;
            }
            else
            {
                free(cleaned_command);

                command_pid = command_execution(commands_vec[i], i);
            }
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
    pid_t pid = waitpid(foreground_pid, &status, WUNTRACED);

    if (WIFEXITED(status))
    {
        // Muito bem, nao precisa fazer nada, que maravilha! :^)
    }
    else if (WIFSIGNALED(status))
    {
        // Vamos ter que achar o grupo de background e foreground desse processo...
        for (int i = 0; i < triplets_amount; i++)
        {
            if (process_triplets[i][0] == pid)
            {
                kill(process_triplets[i][1], WTERMSIG(status));
                killpg(process_triplets[i][2], WTERMSIG(status));
                break;
            }
        }
        // printf("Process %d was killed by signal %d.\n", pid, WTERMSIG(status));
    }
    else if (WIFSTOPPED(status))
    {
        // Vamos ter que achar o grupo de background e foreground desse processo...
        for (int i = 0; i < triplets_amount; i++)
        {
            if (process_triplets[i][0] == pid)
            {
                kill(process_triplets[i][1], WSTOPSIG(status));
                killpg(process_triplets[i][2], WSTOPSIG(status));
                break;
            }
        }
        // printf("Process %d was stopped by signal %d.\n", pid, WSTOPSIG(status));
    }

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
        struct sigaction child_sigint;
        sigemptyset(&child_sigint.sa_mask);
        child_sigint.sa_handler = SIG_IGN; // Ignorar SIGINT
        child_sigint.sa_flags = 0;

        if (sigaction(SIGINT, &child_sigint, NULL) < 0)
        {
            perror("fsh sigint failed");
            exit(EXIT_FAILURE);
        }

        // Se nao for o primeiro comando (em foreground), redireciona a saida padrao dos processos em background para "/dev/null".
        if (command_position)
        {
            freopen("/dev/null", "w", stdout);
            freopen("/dev/null", "w", stderr);

            fork();
        }

        execv(bin_path, args_vec);

        // Se o execv falhar, o filho deve encerrar.
        perror("execv failed");
        exit(EXIT_FAILURE);
    }

    // O pai trata de liberar a memoria alocada
    free(bin_path);
    for (int i = 0; i < args_count; i++)
    {
        free(args_vec[i]);
    }

    return pid;
}