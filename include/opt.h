#ifndef OPT_H
#define OPT_H

#include <stdbool.h>

typedef struct {
    char source_path[256];
    bool dump_tokens; // --tokens
    bool dump_symtab; // --symtab
    bool trace;       // --trace
} Options;

// Processa os argumentos da linha de comando.
bool opts_parse(int argc, char* argv[]);

// Retorna as opções configuradas.
Options* opts_get();

#endif
