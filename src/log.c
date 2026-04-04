#include "log.h"
#include "opt.h"
#include "lex.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Ponteiros de arquivo para os logs opcionais
static FILE *f_tokens = NULL;
static FILE *f_symtab = NULL;
static FILE *f_trace  = NULL;

// Função auxiliar para trocar a extensão do arquivo
static void get_log_name(const char *source, const char *ext, char *dest) {
    strcpy(dest, source);
    char *dot = strrchr(dest, '.');
    if (dot) *dot = '\0'; // Remove a extensão original (.sal)
    strcat(dest, ext);    // Adiciona a nova (.tk, .ts, .trc)
}

void log_init(const char* source_filename) {
    Options *opt = opts_get();
    char log_path[256];

    // Se --tokens estiver ativo, abre arquivo .tk
    if (opt->dump_tokens) {
        get_log_name(source_filename, ".tk", log_path);
        f_tokens = fopen(log_path, "w");
        if (!f_tokens) fprintf(stderr, "Aviso: Não foi possível criar log de tokens.\n");
    }

    // Se --symtab estiver ativo, abre arquivo .ts
    if (opt->dump_symtab) {
        get_log_name(source_filename, ".ts", log_path);
        f_symtab = fopen(log_path, "w");
        if (!f_symtab) fprintf(stderr, "Aviso: Não foi possível criar log da tabela de símbolos.\n");
    }

    // Se --trace estiver ativo, abre arquivo .trc
    if (opt->trace) {
        get_log_name(source_filename, ".trc", log_path);
        f_trace = fopen(log_path, "w");
        if (!f_trace) fprintf(stderr, "Aviso: Não foi possível criar log de rastreamento.\n");
    }
}

void log_token(Token t) {
    if (f_tokens) {
        // Formato exigido: linha <CATEGORIA> "<lexema>"
        fprintf(f_tokens, "%d  %s  \"%s\"\n", 
                t.linha, 
                lex_cat_to_str(t.cat), 
                t.lexema);
    }
}

void log_trace(const char* msg) {
    if (f_trace) {
        fprintf(f_trace, "[TRACE] %s\n", msg);
    }
}

FILE* log_get_symtab_file() {
    return f_symtab;
}

void log_close() {
    if (f_tokens) {
        fclose(f_tokens);
        f_tokens = NULL;
    }
    if (f_symtab) {
        fclose(f_symtab);
        f_symtab = NULL;
    }
    if (f_trace) {
        fclose(f_trace);
        f_trace = NULL;
    }
}
