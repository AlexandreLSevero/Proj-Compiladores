#include "opt.h"
#include <string.h>
#include <stdio.h>

static Options global_opts;

bool opts_parse(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: salc <arquivo.sal> [--tokens] [--symtab] [--trace]\n");
        return false;
    }

    // Reset de opções
    strcpy(global_opts.source_path, argv[1]);
    global_opts.dump_tokens = false;
    global_opts.dump_symtab = false;
    global_opts.trace = false;

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--tokens") == 0) global_opts.dump_tokens = true;
        else if (strcmp(argv[i], "--symtab") == 0) global_opts.dump_symtab = true;
        else if (strcmp(argv[i], "--trace") == 0) global_opts.trace = true;
        else {
            fprintf(stderr, "Opção desconhecida: %s\n", argv[i]);
            return false;
        }
    }
    return true;
}

Options* opts_get() {
    return &global_opts;
}
