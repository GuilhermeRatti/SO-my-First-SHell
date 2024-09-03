#pragma once

#include <sys/types.h>

/**
 * @brief Funcao que remove whitespace da string dada.
 * @param str: A string a ser processada.
 * @return char*: A string sem whitespace.
 * @note Essa funcao aloca espaco para a string, o programador deve lembrar de liberar a memoria alocada.
 */
char *remove_whitespace(const char *str);

//TODO: DOCUMENTAR
void move_to_foreground(pid_t pgid);


