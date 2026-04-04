#include <stdio.h>
#include <stdlib.h>
#include "opt.h"
#include "log.h"
#include "lex.h"
#include "symtab.h"
#include "parser.h"
#include "diag.h"

int main(int argc, char* argv[]) {
    // 1. Processar argumentos da linha de comando
    if (!opts_parse(argc, argv)) {
        return EXIT_FAILURE;
    }

    Options* opt = opts_get();

    // 2. Inicializar o sistema de logs (cria os arquivos .tk, .ts, .trc se solicitado)
    log_init(opt->source_path);

    // 3. Inicializar o Analisador Léxico (abre o arquivo fonte)
    if (!lex_init(opt->source_path)) {
        diag_msg_error("Não foi possível abrir o arquivo fonte informado.");
        log_close();
        return EXIT_FAILURE;
    }

    // 4. Inicializar a Tabela de Símbolos (cria o escopo global)
    ts_init();

    // 5. Executar a Análise Sintática
    if (parse_program()) {
        fprintf(stdout, "SALc: Compilação (Fase 1) concluída com sucesso.\n");
        
        // Se a flag --symtab foi passada, descarrega a tabela no arquivo .ts
        if (opt->dump_symtab) {
            ts_print_log(log_get_symtab_file());
        }
    } else {
        fprintf(stderr, "SALc: Falha na compilação.\n");
    }

    // 6. Encerramento ordenado de todos os módulos
    ts_free();
    lex_close();
    log_close();

    return EXIT_SUCCESS;
}
