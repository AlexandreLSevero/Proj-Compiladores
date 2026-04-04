#ifndef LEX_H
#define LEX_H

#include "common.h"
#include <stdio.h>

// Inicializa o léxico abrindo o arquivo fonte
bool lex_init(const char *source_path);

// Retorna o próximo token do arquivo
Token lex_next();

// Fecha o arquivo e limpa memória, se necessário
void lex_close();

// Auxiliar para obter o nome da categoria como string (útil para logs)
const char* lex_cat_to_str(Category cat);

#endif
