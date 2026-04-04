#include "diag.h"
#include "log.h"
#include "opt.h"
#include <stdio.h>
#include <stdlib.h>

void diag_error(const char* expected, Token found) {
    fprintf(stderr, "ERRO SINTATICO na linha %d: esperado '%s', encontrado '%s'\n", 
            found.linha, expected, found.lexema);
    exit(EXIT_FAILURE);
}

void diag_info(const char* message) {
    if (opts_get()->trace) {
        log_trace(message);
    }
}

void diag_msg_error(const char* msg) {
    fprintf(stderr, "ERRO: %s\n", msg);
}
