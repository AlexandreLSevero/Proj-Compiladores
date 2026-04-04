#include "symtab.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define MAX_SYMBOLS 1000
#define MAX_NESTING 20

typedef struct {
    Symbol data[MAX_SYMBOLS];
    int count;
    char scope_stack[MAX_NESTING][128];
    int top_scope;
    int block_counter[MAX_NESTING]; // Para gerar block#1, block#2...
} SymbolTable;

static SymbolTable ts;

void ts_init() {
    ts.count = 0;
    ts.top_scope = -1;
    ts_scope_push("global");
}

void ts_scope_push(const char *name) {
    if (ts.top_scope < MAX_NESTING - 1) {
        ts.top_scope++;
        strcpy(ts.scope_stack[ts.top_scope], name);
        ts.block_counter[ts.top_scope] = 0;
    }
}

void ts_scope_pop() {
    if (ts.top_scope > 0) {
        ts.top_scope--;
    }
}

const char* ts_scope_current() {
    return ts.scope_stack[ts.top_scope];
}

bool ts_insert(const char *id, SymbolCat cat, DataType type, int extra) {
    // Verifica se já existe no escopo ATUAL (não permite duplicatas no mesmo nível)
    for (int i = 0; i < ts.count; i++) {
        if (strcmp(ts.data[i].id, id) == 0 && 
            strcmp(ts.data[i].scope, ts_scope_current()) == 0) {
            return false; 
        }
    }

    if (ts.count < MAX_SYMBOLS) {
        Symbol *s = &ts.data[ts.count++];
        strcpy(s->id, id);
        s->cat = cat;
        s->type = type;
        s->extra = extra;
        strcpy(s->scope, ts_scope_current());
        return true;
    }
    return false;
}

Symbol* ts_lookup(const char *id) {
    // Busca do escopo atual para os mais externos (de trás para frente)
    for (int level = ts.top_scope; level >= 0; level--) {
        for (int i = ts.count - 1; i >= 0; i--) {
            if (strcmp(ts.data[i].id, id) == 0 && 
                strcmp(ts.data[i].scope, ts.scope_stack[level]) == 0) {
                return &ts.data[i];
            }
        }
    }
    return NULL;
}

static const char* cat_to_str(SymbolCat cat) {
    switch(cat) {
        case SYM_VAR:   return "VAR";
        case SYM_VETOR: return "VETOR";
        case SYM_PROC:  return "PROC";
        case SYM_FUNC:  return "FUNC";
        case SYM_PARAM: return "PARAM";
        default:        return "UNK";
    }
}

static const char* type_to_str(DataType type) {
    switch(type) {
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
        // Formato exigido: SCOPE=<descr> id="<lexema>" cat=<categ> tipo=<tipo> extra=<atrib>
        fprintf(out, "SCOPE=%s  id=\"%s\"  cat=%s  tipo=%s  extra=%d\n",
                s->scope, s->id, cat_to_str(s->cat), 
                type_to_str(s->type), s->extra);
    }
}

void ts_free() {
    ts.count = 0;
    ts.top_scope = -1;
}
