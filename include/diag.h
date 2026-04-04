#ifndef DIAG_H
#define DIAG_H

#include "common.h"

// Reporta um erro sintático ou léxico e encerra a execução (ou marca falha).
void diag_error(const char* expected, Token found);

// Reporta informações de rastreamento (usado pelo --trace).
void diag_info(const char* message);

// Reporta erros genéricos (ex: arquivo não encontrado).
void diag_msg_error(const char* msg);

#endif
