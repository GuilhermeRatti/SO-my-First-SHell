#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

/**
 * @brief Funcao que faz o pai esperar ate que o filho em foreground seja finalizado.
 * @param foreground_pid: O pid do processo filho em foreground.
 */
void wait_for_child(pid_t foreground_pid);

/**
 * @brief Funcao que cria um processo filho e executa o comando desejado. Decide se o processo sera executado em foreground ou background.
 * @param command: String do comando a ser executado.
 * @return pid_t: O pid do processo filho criado.
 */
pid_t command_execution(char *command, int command_position);