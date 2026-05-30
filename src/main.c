#include <stdio.h>
#include <stdlib.h>
#include "opt.h"
#include "log.h"
#include "lex.h"
#include "symtab.h"
#include "parser.h"
#include "diag.h"
#include "gerador.h"

int main(int argc, char* argv[]) {
    // 1. Processar argumentos da linha de comando
    if (!opts_parse(argc, argv)) {
        return EXIT_FAILURE;
    }

    Options* opt = opts_get();

    // 2. Inicializar o sistema de logs
    log_init(opt->source_path);

    // 3. Inicializar o Analisador Léxico
    if (!lex_init(opt->source_path)) {
        diag_msg_error("Nao foi possivel abrir o arquivo fonte informado.");
        log_close();
        return EXIT_FAILURE;
    }

    // 4. Inicializar a Tabela de Símbolos
    ts_init();

    // 5. Inicializar o Gerador de Código MEPA
    if (!ger_init(opt->source_path)) {
        diag_msg_error("Nao foi possivel criar o arquivo de saida .mepa.");
        ts_free();
        lex_close();
        log_close();
        return EXIT_FAILURE;
    }

    // 6. Executar a Análise Sintática + Semântica + Geração de Código
    if (parse_program()) {
        fprintf(stdout, "SALc: Compilacao concluida com sucesso.\n");

        if (opt->dump_symtab) {
            ts_print_log(log_get_symtab_file());
        }
    } else {
        fprintf(stderr, "SALc: Falha na compilacao.\n");
    }

    // 7. Encerramento ordenado
    ger_close();
    ts_free();
    lex_close();
    log_close();

    return EXIT_SUCCESS;
}
