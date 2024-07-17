#pragma once

//TODO: (BACK/FOREGROUND) REVISAR ESSA DOCUMENTACAO
// Revisar depois que implementar execucao de filhos em background.

/**
 * @brief Funcao que faz o pai esperar ate que todos os filhos tenham sido finalizados.
 * @param commands_count: Quantidade total de filhos sendo executados.
 */
void wait_for_children(int commands_count);

//TODO: (BACK/FOREGROUND) REVISAR ESSA DOCUMENTACAO
// Revisar depois que implementar execucao de filhos em background.

/**
 * @brief Funcao que cria um processo filho e executa o comando desejado.
 * @param command: String do comando a ser executado.
 */
void command_execution(char *command);