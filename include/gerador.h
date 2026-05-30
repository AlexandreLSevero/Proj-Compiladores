#ifndef GERADOR_H
#define GERADOR_H

#include <stdio.h>
#include <stdbool.h>

/*
 * gerador.h — Módulo de Geração de Código MEPA
 *
 * Responsável por:
 *   - Emitir instruções MEPA formatadas no arquivo .mepa
 *   - Gerenciar rótulos (labels) únicos e sequenciais
 */

/* Inicializa o gerador abrindo o arquivo de saída .mepa.
 * source_path: caminho do arquivo .sal (ex: "prog.sal" → gera "prog.mepa")
 * Retorna true em caso de sucesso. */
bool ger_init(const char *source_path);

/* Encerra o gerador e fecha o arquivo. */
void ger_close(void);

/*
 * Emite uma instrução MEPA no arquivo de saída.
 *
 * Formato de saída:
 *   [rotulo:] MNEMONICO [param1[,param2]]
 *
 * Parâmetros NULL são simplesmente ignorados.
 * ATENÇÃO: parâmetros de instrução com dois endereços são separados por
 *          vírgula SEM espaço (requisito do interpretador MEPA).
 *
 * Exemplos:
 *   ger_emite("L1", "NADA", NULL, NULL)   → "L1: NADA"
 *   ger_emite(NULL, "CRCT", "5", NULL)    → "     CRCT 5"
 *   ger_emite(NULL, "CRVL", "0", "2")     → "     CRVL 0,2"
 *   ger_emite(NULL, "ARMZ", "0", "0")     → "     ARMZ 0,0"
 */
void ger_emite(const char *rotulo, const char *mnemonico,
               const char *param1, const char *param2);

/*
 * Gera e retorna um novo rótulo único (ex: "L1", "L2", ...).
 * O buffer retornado é estático — copie se precisar guardar.
 */
char *ger_novo_rotulo(void);

/*
 * Emite uma string como sequência de CRCT <ascii> (sem empilhar —
 * cada caractere é emitido e deve ser seguido de IMPR pelo chamador).
 * Retorna o número de caracteres emitidos.
 * O parâmetro 'str' deve conter o lexema com aspas (ex: "\"hello\"").
 */
int ger_emite_string(const char *str);

#endif /* GERADOR_H */
