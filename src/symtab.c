#include "symtab.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define MAX_SYMBOLS 1000
#define MAX_NESTING  20

/* ────────────────────────────────────────────────────────────
 * Estrutura interna da tabela
 * ──────────────────────────────────────────────────────────── */
typedef struct {
    Symbol data[MAX_SYMBOLS];
    int    count;

    /* Pilha de escopos */
    char scope_stack[MAX_NESTING][256];
    int  top_scope;     /* índice do topo; -1 = vazio */

    /*
     * offset_counter[n]: próximo deslocamento livre no nível n.
     * Incrementa a cada variável/parâmetro inserido naquele nível.
     */
    int offset_counter[MAX_NESTING];
} SymbolTable;

static SymbolTable ts;

/* ────────────────────────────────────────────────────────────
 * Helpers internos
 * ──────────────────────────────────────────────────────────── */

/* Monta o caminho de escopo completo até o nível n (inclusivo).
 * Ex.: "global" ou "global.proc:main" */
static void build_scope_path(int level, char *out, size_t out_sz) {
    out[0] = '\0';
    for (int i = 0; i <= level; i++) {
        if (i > 0) strncat(out, ".", out_sz - strlen(out) - 1);
        strncat(out, ts.scope_stack[i], out_sz - strlen(out) - 1);
    }
}

/* ────────────────────────────────────────────────────────────
 * Inicialização / finalização
 * ──────────────────────────────────────────────────────────── */

void ts_init(void) {
    ts.count     = 0;
    ts.top_scope = -1;
    memset(ts.offset_counter, 0, sizeof(ts.offset_counter));
    ts_scope_push("global");
}

void ts_free(void) {
    ts.count     = 0;
    ts.top_scope = -1;
}

/* ────────────────────────────────────────────────────────────
 * Gerenciamento de escopo
 * ──────────────────────────────────────────────────────────── */

void ts_scope_push(const char *name) {
    if (ts.top_scope < MAX_NESTING - 1) {
        ts.top_scope++;
        strncpy(ts.scope_stack[ts.top_scope], name,
                sizeof(ts.scope_stack[0]) - 1);
        ts.scope_stack[ts.top_scope][sizeof(ts.scope_stack[0]) - 1] = '\0';
        ts.offset_counter[ts.top_scope] = 0;
    }
}

void ts_scope_pop(void) {
    if (ts.top_scope > 0) {
        ts.top_scope--;
    }
}

const char *ts_scope_current(void) {
    return ts.scope_stack[ts.top_scope];
}

int ts_nivel_atual(void) {
    return ts.top_scope;
}

/* ────────────────────────────────────────────────────────────
 * Inserção
 * ──────────────────────────────────────────────────────────── */

bool ts_insert(const char *id, SymbolCat cat, DataType type, int extra) {
    /* Monta o caminho do escopo atual */
    char current_path[512];
    build_scope_path(ts.top_scope, current_path, sizeof(current_path));

    /* Verifica duplicata exata no mesmo escopo */
    for (int i = 0; i < ts.count; i++) {
        if (strcmp(ts.data[i].id, id) == 0 &&
            strcmp(ts.data[i].scope, current_path) == 0) {
            return false;
        }
    }

    if (ts.count >= MAX_SYMBOLS) return false;

    Symbol *s  = &ts.data[ts.count++];
    strncpy(s->id,    id,           sizeof(s->id)    - 1);
    strncpy(s->scope, current_path, sizeof(s->scope) - 1);
    s->id[sizeof(s->id) - 1]       = '\0';
    s->scope[sizeof(s->scope) - 1] = '\0';
    s->cat   = cat;
    s->type  = type;
    s->extra = extra;
    s->nivel = ts.top_scope;
    s->rotulo_mepa[0] = '\0';
    s->n_params = 0;

    /* Variáveis, vetores e parâmetros recebem offset e incrementam o contador.
     * Procedimentos e funções não ocupam espaço na pilha de dados. */
    if (cat == SYM_VAR || cat == SYM_VETOR || cat == SYM_PARAM) {
        s->offset = ts.offset_counter[ts.top_scope];
        /* Vetores ocupam 'extra' células; vars/params ocupam 1 */
        ts.offset_counter[ts.top_scope] += (cat == SYM_VETOR && extra > 0) ? extra : 1;
    } else {
        s->offset = -1; /* procs/funcs não têm offset de dados */
    }

    return true;
}

/* ────────────────────────────────────────────────────────────
 * Busca (do escopo atual para o mais externo)
 * ──────────────────────────────────────────────────────────── */

Symbol *ts_lookup(const char *id) {
    /*
     * Percorre a pilha de escopos do mais interno para o mais externo.
     * Para cada nível, monta o caminho completo e busca na tabela.
     */
    for (int level = ts.top_scope; level >= 0; level--) {
        char path[512];
        build_scope_path(level, path, sizeof(path));

        for (int i = ts.count - 1; i >= 0; i--) {
            if (strcmp(ts.data[i].id, id) == 0 &&
                strcmp(ts.data[i].scope, path) == 0) {
                return &ts.data[i];
            }
        }
    }
    return NULL;
}

/* ────────────────────────────────────────────────────────────
 * Log --symtab
 * ──────────────────────────────────────────────────────────── */

static const char *cat_to_str(SymbolCat cat) {
    switch (cat) {
        case SYM_VAR:   return "VAR";
        case SYM_VETOR: return "VETOR";
        case SYM_PROC:  return "PROC";
        case SYM_FUNC:  return "FUNC";
        case SYM_PARAM: return "PARAM";
        default:        return "UNK";
    }
}

static const char *type_to_str(DataType type) {
    switch (type) {
        case TYPE_INT:  return "INT";
        case TYPE_BOOL: return "BOOL";
        case TYPE_CHAR: return "CHAR";
        case TYPE_VOID: return "VOID";
        default:        return "UNDEFINED";
    }
}

void ts_print_log(FILE *out) {
    if (!out) return;
    for (int i = 0; i < ts.count; i++) {
        Symbol *s = &ts.data[i];
        fprintf(out,
                "SCOPE=%s  id=\"%s\"  cat=%s  tipo=%s  extra=%d"
                "  nivel=%d  offset=%d\n",
                s->scope, s->id, cat_to_str(s->cat),
                type_to_str(s->type), s->extra,
                s->nivel, s->offset);
    }
}
