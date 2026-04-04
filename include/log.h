#ifndef LOG_H
#define LOG_H

#include "common.h"
#include <stdio.h>

// Inicializa os arquivos de log baseados no nome do fonte e opções.
void log_init(const char* source_filename);

// Registra um token no arquivo .tk (se habilitado).
void log_token(Token t);

// Registra uma linha de rastreamento no arquivo .trc (se habilitado).
void log_trace(const char* msg);

// Finaliza e fecha todos os arquivos de log abertos.
void log_close();

// Retorna o ponteiro de arquivo para a TS escrever nela (usado em ts_print_log).
FILE* log_get_symtab_file();

#endif
