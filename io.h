#pragma once

/**
 * @brief Funcao que imprime o prompt no padrao do fsh.
 */
void print_prompt();

/**
 * @brief Funcao que le a string da entrada padrao.
 * @return char* line:
 * Linha lida pela funcao.
 * @note
 * Essa funcao aloca um espaco de memoria para a variavel line.
 */
char *read_line();

/**
 * @brief Funcao que destaca os comandos dentro de uma string.
 * @param line: Entrada da string a ser processada.
 * @param commands_vec: Vetor de strings (char*) dos comandos lidos e alocados dentro da funcao.
 * @return int commands_count:
 * Quantidade de comandos processados na string.
 * @note
 * Essa funcao tambem retorna os comandos por referencia dentro do vetor char *commands[].
 * O programador deve lembrar que os comandos sao alocados dentro dessa funcao.
 * Essa funcao libera o espaco de memoria ocupado pela variavel line.
 */
int get_commands(char *line, char *commands_vec[]);