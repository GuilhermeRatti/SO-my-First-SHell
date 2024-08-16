#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

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