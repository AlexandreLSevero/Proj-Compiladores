#ifndef SYMTAB_H
#define SYMTAB_H

#include <stdio.h>
#include "common.h"

/* Categorias de símbolos na tabela */
typedef enum {
    SYM_VAR,
    SYM_VETOR,
    SYM_PROC,
    SYM_FUNC,
    SYM_PARAM
} SymbolCat;

/* Entrada da Tabela de Símbolos */
typedef struct {
    char      id[256];
    SymbolCat cat;
    DataType  type;
    int       extra;       /* Tamanho do vetor ou qtd de parâmetros */
    char      scope[256];  /* Caminho de escopo completo */

    /* Campos para geração de código MEPA */
    int nivel;      /* Nível léxico de declaração (0 = global) */
    int offset;     /* Deslocamento dentro do nível (0-based)  */
} Symbol;

/* ─── Gerenciamento de Escopo ─────────────────────────────── */
void        ts_scope_push(const char *name);
void        ts_scope_pop(void);
const char *ts_scope_current(void);
int         ts_nivel_atual(void);   /* retorna o nível léxico atual */

/* ─── Operações da Tabela ─────────────────────────────────── */

/*
 * Insere um símbolo no escopo atual.
 * Retorna true em caso de sucesso; false se já existir no mesmo escopo.
 */
bool    ts_insert(const char *id, SymbolCat cat, DataType type, int extra);

/*
 * Busca um símbolo do escopo mais interno para o mais externo.
 * Retorna ponteiro para o símbolo ou NULL se não encontrado.
 */
Symbol *ts_lookup(const char *id);

/* ─── Inicialização e finalização ────────────────────────── */
void ts_init(void);
void ts_free(void);

/* ─── Log --symtab ───────────────────────────────────────── */
void ts_print_log(FILE *out);

#endif /* SYMTAB_H */
