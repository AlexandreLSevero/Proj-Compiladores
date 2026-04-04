#ifndef SYMTAB_H
#define SYMTAB_H

#include <stdio.h>
#include "common.h"

// Categorias de símbolos na tabela
typedef enum {
    SYM_VAR,
    SYM_VETOR,
    SYM_PROC,
    SYM_FUNC,
    SYM_PARAM
} SymbolCat;

// Entrada da Tabela de Símbolos
typedef struct {
    char id[256];
    SymbolCat cat;
    DataType type;
    int extra;      // Tamanho do vetor ou Qtd de parâmetros
    char scope[128];
} Symbol;

// Gerenciamento de Escopo
void ts_scope_push(const char *name);
void ts_scope_pop();
const char* ts_scope_current();

// Operações da Tabela
bool ts_insert(const char *id, SymbolCat cat, DataType type, int extra);
Symbol* ts_lookup(const char *id); // Busca do escopo atual para o global

// Inicialização e finalização
void ts_init();
void ts_free();

// Função para o log --symtab (percorre a tabela e exporta)
void ts_print_log(FILE *out);

#endif
