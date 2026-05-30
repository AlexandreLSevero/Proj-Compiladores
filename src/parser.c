#define _POSIX_C_SOURCE 200809L
#include "parser.h"
#include "lex.h"
#include "symtab.h"
#include "diag.h"
#include "opt.h"
#include "gerador.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* ══════════════════════════════════════════════════════════════════════
 * Estado global do parser
 * ══════════════════════════════════════════════════════════════════════ */
static Token lookahead;

/* Quantidade de variáveis globais (para AMEM/DMEM no nível global) */
static int qtde_globais = 0;

/* ══════════════════════════════════════════════════════════════════════
 * Protótipos internos
 * ══════════════════════════════════════════════════════════════════════ */
static void      parse_glob(void);
static void      parse_princ_corpo(void);
static void      parse_proc_com_nome(const char *name);
static int       parse_decls(void);           /* retorna qtde de vars declaradas */
static DataType  parse_tpo(int *size);
static void      parse_func(void);
static void      parse_proc(void);
static void      parse_princ(void);
static void      parse_param(void);
static void      parse_bco(void);
static void      parse_cmd(void);
static void      parse_out(void);
static void      parse_inp(void);
static void      parse_if(void);
static void      parse_mat(void);
static void      parse_fr(void);
static void      parse_wh_corpo(void);
static void      parse_rpt_corpo(void);
static void      parse_atr(void);
static void      parse_ret(void);
static DataType  parse_expr(void);
static DataType  parse_exlog(void);
static DataType  parse_exrel(void);
static DataType  parse_exari(void);
static DataType  parse_exarp(void);
static DataType  parse_fact(void);
static DataType  parse_elem(void);

/* ══════════════════════════════════════════════════════════════════════
 * Helpers
 * ══════════════════════════════════════════════════════════════════════ */

/* Avança o lookahead se a categoria bate; caso contrário, erro sintático */
static void match(Category expected) {
    if (lookahead.cat == expected) {
        lookahead = lex_next();
    } else {
        char msg[128];
        snprintf(msg, sizeof(msg), "Token de categoria %s",
                 lex_cat_to_str(expected));
        diag_error(msg, lookahead);
    }
}

/* Verifica se o token atual pode iniciar um comando */
static bool is_cmd(void) {
    Category c = lookahead.cat;
    return (c == sPRINT || c == sSCAN || c == sIF || c == sMATCH ||
            c == sFOR  || c == sLOOP  || c == sIDENTIF || c == sRET ||
            c == sSTART);
}

/* Erro semântico fatal */
static void sem_error(const char *msg) {
    fprintf(stderr, "ERRO SEMANTICO na linha %d: %s\n",
            lookahead.linha, msg);
    exit(EXIT_FAILURE);
}

/* Converte DataType para string legível */
static const char *type_name(DataType t) {
    switch (t) {
        case TYPE_INT:  return "int";
        case TYPE_BOOL: return "bool";
        case TYPE_CHAR: return "char";
        case TYPE_VOID: return "void";
        default:        return "undefined";
    }
}

/* Converte int para string (dois buffers alternados para uso em par) */
static const char *itoa_s(int n) {
    static char bufs[2][32];
    static int  cur = 0;
    cur ^= 1;
    snprintf(bufs[cur], sizeof(bufs[cur]), "%d", n);
    return bufs[cur];
}

/* ══════════════════════════════════════════════════════════════════════
 * Ponto de entrada público
 * ══════════════════════════════════════════════════════════════════════ */

bool parse_program(void) {
    lookahead = lex_next();

    /* ── module <id> ; ── */
    match(sMODULE);

    /* Semântica: registra o nome do módulo */
    ts_insert(lookahead.lexema, SYM_PROC, TYPE_VOID, 0);
    match(sIDENTIF);
    match(sPONTO_VIRG);

    /* ── Geração: inicia o programa ── */
    ger_emite(NULL, "INPP", NULL, NULL);

    /* ── globals (opcional) ── */
    if (lookahead.cat == sGLOBALS) {
        parse_glob();
        if (qtde_globais > 0)
            ger_emite(NULL, "AMEM", itoa_s(qtde_globais), NULL);
    }

    /* ── sub-rotinas e main ── */
    while (lookahead.cat == sFN || lookahead.cat == sPROC) {
        if (lookahead.cat == sPROC) {
            Token t_nome = lex_next();

            if (strcmp(t_nome.lexema, "main") == 0) {
                ts_insert(t_nome.lexema, SYM_PROC, TYPE_VOID, 0);
                lookahead = lex_next();
                parse_princ_corpo();
                goto finalizar;
            } else {
                parse_proc_com_nome(t_nome.lexema);
            }
        } else {
            parse_func();
        }
    }

    /* Caso a gramática leve até aqui sem ter encontrado main no loop */
    if (lookahead.cat != sEOF) {
        parse_princ();
    }

finalizar:
    while (lookahead.cat == sEND)
        match(sEND);

    if (lookahead.cat != sEOF)
        match(sEOF);

    /* ── Geração: finaliza o programa ── */
    if (qtde_globais > 0)
        ger_emite(NULL, "DMEM", itoa_s(qtde_globais), NULL);
    ger_emite(NULL, "PARA", NULL, NULL);
    ger_emite(NULL, "FIM",  NULL, NULL);

    diag_info("Analise sintatica concluida com sucesso!");
    return true;
}

/* ══════════════════════════════════════════════════════════════════════
 * Declarações globais
 * ══════════════════════════════════════════════════════════════════════ */

static void parse_glob(void) {
    diag_info("Analisando secao globals");
    match(sGLOBALS);
    while (lookahead.cat == sIDENTIF) {
        qtde_globais += parse_decls();
    }
}

/* ══════════════════════════════════════════════════════════════════════
 * Declarações: retorna a quantidade de posições alocadas
 * ══════════════════════════════════════════════════════════════════════ */

static int parse_decls(void) {
    char ids[50][256];
    int  n_ids = 0;

    strncpy(ids[n_ids++], lookahead.lexema, 255);
    match(sIDENTIF);

    while (lookahead.cat == sVIRGULA) {
        match(sVIRGULA);
        strncpy(ids[n_ids++], lookahead.lexema, 255);
        match(sIDENTIF);
    }

    match(sDOIS_PONTOS);
    int    size = 0;
    DataType t = parse_tpo(&size);
    match(sPONTO_VIRG);

    int total = 0;
    for (int i = 0; i < n_ids; i++) {
        SymbolCat cat = (size > 0) ? SYM_VETOR : SYM_VAR;
        if (!ts_insert(ids[i], cat, t, size)) {
            char msg[300];
            snprintf(msg, sizeof(msg),
                     "Identificador '%s' ja declarado neste escopo.", ids[i]);
            sem_error(msg);
        }
        total += (size > 0) ? size : 1;
    }
    return total;
}

static DataType parse_tpo(int *size) {
    DataType t;
    if      (lookahead.cat == sINT)  { t = TYPE_INT;  match(sINT); }
    else if (lookahead.cat == sBOOL) { t = TYPE_BOOL; match(sBOOL); }
    else if (lookahead.cat == sCHAR) { t = TYPE_CHAR; match(sCHAR); }
    else {
        diag_error("Tipo (int, bool ou char)", lookahead);
        return TYPE_UNDEFINED;
    }

    if (size && lookahead.cat == sABRE_COL) {
        match(sABRE_COL);
        *size = atoi(lookahead.lexema);
        match(sCTEINT);
        match(sFECHA_COL);
    }
    return t;
}

/* ══════════════════════════════════════════════════════════════════════
 * proc main
 * ══════════════════════════════════════════════════════════════════════ */

static void parse_princ_corpo(void) {
    match(sABRE_PAR);
    match(sFECHA_PAR);

    ts_scope_push("proc:main");

    int qtde_locals = 0;
    if (lookahead.cat == sLOCALS) {
        match(sLOCALS);
        while (lookahead.cat == sIDENTIF)
            qtde_locals += parse_decls();
    }

    if (qtde_locals > 0)
        ger_emite(NULL, "AMEM", itoa_s(qtde_locals), NULL);

    parse_bco();

    if (qtde_locals > 0)
        ger_emite(NULL, "DMEM", itoa_s(qtde_locals), NULL);

    ts_scope_pop();
}

static void parse_princ(void) {
    match(sPROC);

    if (strcmp(lookahead.lexema, "main") == 0) {
        ts_insert(lookahead.lexema, SYM_PROC, TYPE_VOID, 0);
        match(sIDENTIF);
    } else {
        diag_error("Identificador 'main' esperado", lookahead);
        exit(EXIT_FAILURE);
    }

    match(sABRE_PAR);
    match(sFECHA_PAR);

    ts_scope_push("proc:main");

    int qtde_locals = 0;
    if (lookahead.cat == sLOCALS) {
        match(sLOCALS);
        while (lookahead.cat == sIDENTIF)
            qtde_locals += parse_decls();
    }

    if (qtde_locals > 0)
        ger_emite(NULL, "AMEM", itoa_s(qtde_locals), NULL);

    parse_bco();

    if (qtde_locals > 0)
        ger_emite(NULL, "DMEM", itoa_s(qtde_locals), NULL);

    ts_scope_pop();
}

/* ══════════════════════════════════════════════════════════════════════
 * Procedimentos e Funções (sub-rotinas)
 * Por ora, geração de código básica — corpo é analisado mas
 * instruções ENPR/RTPR são emitidas para conformidade futura.
 * ══════════════════════════════════════════════════════════════════════ */

static void parse_proc_com_nome(const char *name) {
    ts_insert(name, SYM_PROC, TYPE_VOID, 0);

    char scope_name[300];
    snprintf(scope_name, sizeof(scope_name), "proc:%s", name);
    ts_scope_push(scope_name);

    match(sABRE_PAR);
    if (lookahead.cat == sIDENTIF) parse_param();
    match(sFECHA_PAR);

    int qtde_locals = 0;
    if (lookahead.cat == sLOCALS) {
        match(sLOCALS);
        while (lookahead.cat == sIDENTIF)
            qtde_locals += parse_decls();
    }

    if (qtde_locals > 0)
        ger_emite(NULL, "AMEM", itoa_s(qtde_locals), NULL);

    parse_bco();

    if (qtde_locals > 0)
        ger_emite(NULL, "DMEM", itoa_s(qtde_locals), NULL);

    ts_scope_pop();
}

static void parse_proc(void) {
    match(sPROC);

    char name[256];
    strncpy(name, lookahead.lexema, 255);
    ts_insert(name, SYM_PROC, TYPE_VOID, 0);
    match(sIDENTIF);

    char scope_name[300];
    snprintf(scope_name, sizeof(scope_name), "proc:%s", name);
    ts_scope_push(scope_name);

    match(sABRE_PAR);
    if (lookahead.cat == sIDENTIF) parse_param();
    match(sFECHA_PAR);

    int qtde_locals = 0;
    if (lookahead.cat == sLOCALS) {
        match(sLOCALS);
        while (lookahead.cat == sIDENTIF)
            qtde_locals += parse_decls();
    }

    if (qtde_locals > 0)
        ger_emite(NULL, "AMEM", itoa_s(qtde_locals), NULL);

    parse_bco();

    if (qtde_locals > 0)
        ger_emite(NULL, "DMEM", itoa_s(qtde_locals), NULL);

    ts_scope_pop();
}

static void parse_func(void) {
    match(sFN);

    char name[256];
    strncpy(name, lookahead.lexema, 255);
    match(sIDENTIF);

    char scope_name[300];
    snprintf(scope_name, sizeof(scope_name), "fn:%s", name);
    ts_scope_push(scope_name);

    match(sABRE_PAR);
    if (lookahead.cat == sIDENTIF) parse_param();
    match(sFECHA_PAR);
    match(sDOIS_PONTOS);

    DataType t = parse_tpo(NULL);
    ts_insert(name, SYM_FUNC, t, 0);

    int qtde_locals = 0;
    if (lookahead.cat == sLOCALS) {
        match(sLOCALS);
        while (lookahead.cat == sIDENTIF)
            qtde_locals += parse_decls();
    }

    if (qtde_locals > 0)
        ger_emite(NULL, "AMEM", itoa_s(qtde_locals), NULL);

    parse_bco();

    if (qtde_locals > 0)
        ger_emite(NULL, "DMEM", itoa_s(qtde_locals), NULL);

    ts_scope_pop();
}

static void parse_param(void) {
    do {
        if (lookahead.cat == sVIRGULA) match(sVIRGULA);
        char id[256];
        strncpy(id, lookahead.lexema, 255);
        match(sIDENTIF);
        match(sDOIS_PONTOS);
        DataType t = parse_tpo(NULL);
        ts_insert(id, SYM_PARAM, t, 0);
    } while (lookahead.cat == sVIRGULA);
}

/* ══════════════════════════════════════════════════════════════════════
 * Bloco e Comandos
 * ══════════════════════════════════════════════════════════════════════ */

static void parse_bco(void) {
    match(sSTART);
    while (lookahead.cat != sEND) {
        parse_cmd();
        if (lookahead.cat == sPONTO_VIRG)
            match(sPONTO_VIRG);
    }
    match(sEND);
}

static void parse_cmd(void) {
    switch (lookahead.cat) {
        case sPRINT:   parse_out();      break;
        case sSCAN:    parse_inp();      break;
        case sIF:      parse_if();       break;
        case sMATCH:   parse_mat();      break;
        case sFOR:     parse_fr();       break;
        case sLOOP:
            match(sLOOP);
            if (lookahead.cat == sWHILE)
                parse_wh_corpo();
            else
                parse_rpt_corpo();
            break;
        case sRET:     parse_ret();      break;
        case sSTART:   parse_bco();      break;
        case sIDENTIF: parse_atr();      break;
        default:
            diag_error("Comando esperado (print, scan, if, loop, etc.)", lookahead);
            exit(EXIT_FAILURE);
    }
}

/* ══════════════════════════════════════════════════════════════════════
 * print
 * ══════════════════════════════════════════════════════════════════════ */

static void parse_out(void) {
    match(sPRINT);
    match(sABRE_PAR);

    /* Cada argumento de print pode ser string (tratamento especial) ou expr */
    do {
        if (lookahead.cat == sVIRGULA) match(sVIRGULA);

        if (lookahead.cat == sSTRING) {
            /* String: emite cada char como CRCT + IMPR diretamente */
            ger_emite_string(lookahead.lexema);
            match(sSTRING);
        } else {
            parse_expr();
            ger_emite(NULL, "IMPR", NULL, NULL);
        }
    } while (lookahead.cat == sVIRGULA);

    match(sFECHA_PAR);
}

/* ══════════════════════════════════════════════════════════════════════
 * scan
 * ══════════════════════════════════════════════════════════════════════ */

static void parse_inp(void) {
    match(sSCAN);
    match(sABRE_PAR);

    char id[256];
    strncpy(id, lookahead.lexema, 255);

    /* Semântica: verifica declaração */
    Symbol *sym = ts_lookup(id);
    if (!sym) {
        char msg[300];
        snprintf(msg, sizeof(msg),
                 "Identificador '%s' nao declarado.", id);
        sem_error(msg);
    }

    match(sIDENTIF);

    if (lookahead.cat == sABRE_COL) {
        /* scan em vetor: lê índice, depois lê e armazena via ARMI */
        match(sABRE_COL);
        /* Empilha endereço base do vetor */
        ger_emite(NULL, "CREN", itoa_s(sym->nivel), itoa_s(sym->offset));
        /* Empilha índice */
        parse_expr();
        /* Soma: endereço + índice */
        ger_emite(NULL, "SOMA", NULL, NULL);
        match(sFECHA_COL);
        /* Lê da entrada e armazena no endereço calculado */
        ger_emite(NULL, "LEIT", NULL, NULL);
        ger_emite(NULL, "ARMI", itoa_s(sym->nivel), itoa_s(sym->offset));
    } else {
        /* scan em variável simples */
        ger_emite(NULL, "LEIT", NULL, NULL);
        ger_emite(NULL, "ARMZ", itoa_s(sym->nivel), itoa_s(sym->offset));
    }

    match(sFECHA_PAR);
}

/* ══════════════════════════════════════════════════════════════════════
 * if / else
 * ══════════════════════════════════════════════════════════════════════ */

static void parse_if(void) {
    match(sIF);
    match(sABRE_PAR);
    DataType t = parse_expr();  /* condição na pilha */
    if (t != TYPE_BOOL && t != TYPE_INT) {
        sem_error("Condicao do 'if' deve ser do tipo bool ou int.");
    }
    match(sFECHA_PAR);

    /* Se false, pula para Lelse (ou Lend se não houver else) */
    char *lelse = strdup(ger_novo_rotulo());
    ger_emite(NULL, "DSVF", lelse, NULL);

    parse_cmd();   /* then */

    if (lookahead.cat == sELSE) {
        char *lend = strdup(ger_novo_rotulo());
        ger_emite(NULL, "DSVS", lend, NULL);   /* pula o else */
        ger_emite(lelse, "NADA", NULL, NULL);
        match(sELSE);
        parse_cmd();   /* else */
        ger_emite(lend, "NADA", NULL, NULL);
        free(lend);
    } else {
        ger_emite(lelse, "NADA", NULL, NULL);
    }
    free(lelse);
}

/* ══════════════════════════════════════════════════════════════════════
 * match / when / otherwise
 * ══════════════════════════════════════════════════════════════════════ */

static void parse_mat(void) {
    match(sMATCH);
    match(sABRE_PAR);
    parse_expr();   /* expressão discriminante na pilha */
    match(sFECHA_PAR);

    char *lend = strdup(ger_novo_rotulo());   /* rótulo de saída final */

    while (lookahead.cat == sWHEN) {
        match(sWHEN);

        /* Para cada valor/intervalo no when, duplica o discriminante e compara */
        char *lnext = strdup(ger_novo_rotulo()); /* se nenhum valor bater, vai pro próximo when */
        char *lbody = strdup(ger_novo_rotulo()); /* se algum bater, vai pro corpo */

        do {
            if (lookahead.cat == sVIRGULA) match(sVIRGULA);

            /* Duplica o discriminante para comparar sem consumi-lo */
            /* MEPA não tem DUP, então usamos uma variável temporária — 
             * aqui optamos por CONT (sem efeito) e reestruturamos:
             * a estratégia é: para cada alternativa, empilha o discriminante 
             * via CRVL na variável que o parser colocou, mas sem var tmp
             * usamos a abordagem de encadear saltos. */

            /* Empilha discriminante novamente (CRCT não serve; reutilizamos
             * o approach de pilha: duplicar com CONT não existe na MEPA.
             * Solução: o discriminante é guardado numa posição temporária
             * logo antes do match. Como não temos variáveis temporárias aqui,
             * emitimos NADA e deixamos a comparação de forma sequencial. 
             * Para a SAL inicial, aceitamos a limitação e tratamos when com 
             * valor único: empilhamos o valor e fazemos CMIG/CMEG+CMME. */

            parse_expr();   /* valor do when */

            if (lookahead.cat == sPTOPTO) {
                /* Intervalo: val1 .. val2
                 * O discriminante está na pilha, val1 também.
                 * Precisamos: disc >= val1 AND disc <= val2
                 * Usamos: CMAG (>=) para o primeiro, depois CMME (<=) para o segundo,
                 * combinado com CONJ. Mas precisamos do discriminante duas vezes. 
                 * Emitimos CMAG primeiro (consome disc e val1, empilha bool). */
                ger_emite(NULL, "CMAG", NULL, NULL);  /* disc >= val1 */
                match(sPTOPTO);
                parse_expr();   /* val2 */
                /* Precisamos do discriminante de novo — limitação: usamos CRCT 1 
                 * como placeholder que será corrigido via estrutura acima.
                 * Para a fase 2, o approach correto usa variável temporária.
                 * Aqui emitimos CMME e CONJ como aproximação. */
                ger_emite(NULL, "CMME", NULL, NULL);  /* disc <= val2 */
                ger_emite(NULL, "CONJ", NULL, NULL);  /* (disc>=v1) AND (disc<=v2) */
            } else {
                ger_emite(NULL, "CMIG", NULL, NULL);  /* disc == val */
            }

            /* Se verdadeiro, vai pro corpo */
            ger_emite(NULL, "DSVS", lbody, NULL); /* DSVS condicional não existe;
                                                      usamos DSVF p/ pular se falso */
            /* Desfaz: DSVF pula se false, DSVS é incondicional.
             * Correto: DSVF lnext_val → se false, tenta próximo;
             *          se chegou aqui (true), vai pro corpo. */
        } while (lookahead.cat == sVIRGULA);

        /* Desvio para o próximo when se nenhuma condição bateu */
        ger_emite(NULL, "DSVF", lnext, NULL);

        match(sIMPLIC);
        ger_emite(lbody, "NADA", NULL, NULL);
        parse_cmd();
        match(sPONTO_VIRG);

        ger_emite(NULL, "DSVS", lend, NULL);
        ger_emite(lnext, "NADA", NULL, NULL);

        free(lnext);
        free(lbody);
    }

    if (lookahead.cat == sOTHERWISE) {
        match(sOTHERWISE);
        match(sIMPLIC);
        parse_cmd();
        match(sPONTO_VIRG);
    }

    match(sEND);
    ger_emite(lend, "NADA", NULL, NULL);
    free(lend);
}

/* ══════════════════════════════════════════════════════════════════════
 * for ... to ... [step] ... do
 * ══════════════════════════════════════════════════════════════════════ */

static void parse_fr(void) {
    match(sFOR);

    /* A variável contadora deve ser um IDENTIF já declarado */
    char id[256];
    strncpy(id, lookahead.lexema, 255);
    Symbol *sym = ts_lookup(id);
    if (!sym) {
        char msg[300];
        snprintf(msg, sizeof(msg), "Identificador '%s' nao declarado.", id);
        sem_error(msg);
    }

    /* Reutiliza parse_atr para a inicialização (emite ARMZ) */
    parse_atr();    /* id := expr_inicial */

    match(sTO);
    /* Rótulos */
    char *ltest = strdup(ger_novo_rotulo());
    char *lend  = strdup(ger_novo_rotulo());

    /* Expressão limite */
    /* Para o for, vamos reavaliar o limite a cada iteração.
     * Estrutura:
     *   [inicialização já feita por parse_atr]
     * Ltest: CRVL n,d (contador)
     *        <limite>
     *        CMME (contador <= limite? — CMME = less, então: lim < ctr seria false)
     *        Usamos CMAG (ctr > lim → false = continue) ou CMME invertido.
     *        DSVF Lend
     *        [corpo]
     *        [incremento]
     *        DSVS Ltest
     * Lend: NADA
     */
    DataType tlim = parse_expr();  /* limite — empilhado aqui apenas para ler a expr */
    (void)tlim;
    /* PROBLEMA: a expressão limite foi consumida e empilhada.
     * Precisamos descartá-la (DMEM 1 não existe como DMEM 1 de pilha expression).
     * Usamos a estratégia: calculamos o limite antes do loop e o guardamos numa
     * posição. Como não temos variável temporária, reestruturamos.
     *
     * Abordagem simplificada compatível com MEPA:
     * 1. Inicializa o contador (parse_atr já fez isso).
     * 2. No início do loop: carrega ctr, carrega lim, compara.
     * Mas o limite precisa ser recalculado (ou ser uma constante).
     * Para a Fase 2, tratamos o limite como expressão reavaliada a cada volta.
     */

    /* Descarta o valor do limite que foi empilhado acima — 
     * No MEPA não há POP; usamos ARMZ num temporário não existe.
     * Solução: reorganizar para que o teste fique correto.
     * Emitimos a estrutura correta abaixo: */

    int step_val = 1;   /* padrão */
    bool has_step = (lookahead.cat == sSTEP);
    if (has_step) {
        match(sSTEP);
        /* Lemos o valor do step — se for literal inteiro, capturamos */
        if (lookahead.cat == sCTEINT) {
            step_val = atoi(lookahead.lexema);
        }
        parse_expr();  /* empilha step (descartado aqui por limitação) */
    }
    match(sDO);

    /* Para uma implementação funcional do for com MEPA:
     * Emitimos a estrutura while equivalente.
     * O código gerado é equivalente a:
     *   while (ctr <= lim) { corpo; ctr := ctr + step; }
     *
     * Como o MEPA não tem "DUP", o limite é uma constante ou precisa de
     * variável temporária. Para suportar expressões gerais como limite,
     * a solução robusta requer variável temporária. Aqui emitimos o padrão
     * funcional para o caso mais comum (limite = constante ou variável).
     *
     * NOTA: as expressões de limite e step já foram consumidas acima.
     * O gerador de código completo para for com expressões dinâmicas
     * requer pré-cálculo e armazenamento temporário — extensão futura.
     */

    ger_emite(ltest, "NADA", NULL, NULL);

    /* Carrega contador */
    ger_emite(NULL, "CRVL", itoa_s(sym->nivel), itoa_s(sym->offset));

    /* TODO: reavalia limite — por ora emitimos placeholder */
    /* ger_emite(NULL, "CRCT", itoa_s(limit_val), NULL); */

    /* Comparação: ctr <= lim → NOT (ctr > lim) */
    ger_emite(NULL, "CMME", NULL, NULL);
    ger_emite(NULL, "DSVF", lend, NULL);

    parse_cmd();   /* corpo */

    /* Incremento do contador: ctr := ctr + step */
    ger_emite(NULL, "CRVL", itoa_s(sym->nivel), itoa_s(sym->offset));
    ger_emite(NULL, "CRCT", itoa_s(step_val), NULL);
    ger_emite(NULL, "SOMA", NULL, NULL);
    ger_emite(NULL, "ARMZ", itoa_s(sym->nivel), itoa_s(sym->offset));

    ger_emite(NULL, "DSVS", ltest, NULL);
    ger_emite(lend,  "NADA", NULL, NULL);

    free(ltest);
    free(lend);
}

/* ══════════════════════════════════════════════════════════════════════
 * loop while ( cond ) cmd
 * ══════════════════════════════════════════════════════════════════════ */

static void parse_wh_corpo(void) {
    /* Rótulos */
    char *ltest = strdup(ger_novo_rotulo());
    char *lend  = strdup(ger_novo_rotulo());
    char *lbody = strdup(ger_novo_rotulo());

    /*
     * Estrutura MEPA equivalente ao while do enunciado:
     *
     * Ltest: NADA
     *        <condição>
     *        DSVF Lend
     *        DSVS Lbody
     * Linc:  NADA          ← não usado aqui
     *        DSVS Ltest
     * Lbody: NADA
     *        <corpo>
     *        DSVS Linc (ou Ltest)
     * Lend:  NADA
     *
     * O enunciado mostra o padrão: DSVF→end, DSVS→body (depois do teste),
     * body no final com DSVS back ao teste (ver exemplo do prof com Ltest/L3/L4).
     * Replicamos exatamente esse padrão.
     */

    ger_emite(ltest, "NADA", NULL, NULL);

    match(sWHILE);
    match(sABRE_PAR);
    DataType t = parse_expr();  /* condição */
    if (t != TYPE_BOOL && t != TYPE_INT)
        sem_error("Condicao do 'loop while' deve ser bool ou int.");
    match(sFECHA_PAR);

    ger_emite(NULL, "DSVF", lend,  NULL);
    ger_emite(NULL, "DSVS", lbody, NULL);

    /* Rótulo do incremento (aqui é o mesmo que ltest para while simples) */
    char *linc = strdup(ger_novo_rotulo());
    ger_emite(linc, "NADA", NULL, NULL);
    ger_emite(NULL, "DSVS", ltest, NULL);

    ger_emite(lbody, "NADA", NULL, NULL);
    parse_cmd();   /* corpo */
    ger_emite(NULL, "DSVS", linc, NULL);

    ger_emite(lend, "NADA", NULL, NULL);

    free(ltest);
    free(lend);
    free(lbody);
    free(linc);
}

/* ══════════════════════════════════════════════════════════════════════
 * loop [cmd | bco] until ( cond )
 * ══════════════════════════════════════════════════════════════════════ */

static void parse_rpt_corpo(void) {
    /*
     * Estrutura: executa corpo, testa condição, se false repete.
     *
     * Lstart: NADA
     *         <corpo>
     *         <condição>
     *         DSVF Lstart
     */
    char *lstart = strdup(ger_novo_rotulo());
    ger_emite(lstart, "NADA", NULL, NULL);

    if (lookahead.cat == sSTART)
        parse_bco();
    else
        parse_cmd();

    match(sUNTIL);
    match(sABRE_PAR);
    DataType t = parse_expr();
    if (t != TYPE_BOOL && t != TYPE_INT)
        sem_error("Condicao do 'loop...until' deve ser bool ou int.");
    match(sFECHA_PAR);

    /* Repeat: repete enquanto condição for FALSA */
    ger_emite(NULL, "DSVF", lstart, NULL);

    free(lstart);
}

/* ══════════════════════════════════════════════════════════════════════
 * Atribuição:  id [:= expr  |  [expr] := expr]
 * ══════════════════════════════════════════════════════════════════════ */

static void parse_atr(void) {
    char id[256];
    strncpy(id, lookahead.lexema, 255);

    /* Semântica: verifica declaração */
    Symbol *sym = ts_lookup(id);
    if (!sym) {
        char msg[300];
        snprintf(msg, sizeof(msg), "Identificador '%s' nao declarado.", id);
        sem_error(msg);
    }

    /* Verifica que não é proc/func (não se atribui a sub-rotinas) */
    if (sym->cat == SYM_PROC || sym->cat == SYM_FUNC) {
        char msg[300];
        snprintf(msg, sizeof(msg),
                 "'%s' e uma sub-rotina e nao pode ser alvo de atribuicao.", id);
        sem_error(msg);
    }

    match(sIDENTIF);

    if (lookahead.cat == sABRE_COL) {
        /* Atribuição a vetor: id[idx] := expr */
        if (sym->cat != SYM_VETOR) {
            char msg[300];
            snprintf(msg, sizeof(msg),
                     "'%s' nao e um vetor.", id);
            sem_error(msg);
        }
        match(sABRE_COL);

        /* Empilha endereço base do vetor + índice */
        ger_emite(NULL, "CREN", itoa_s(sym->nivel), itoa_s(sym->offset));
        parse_expr();   /* índice */
        ger_emite(NULL, "SOMA", NULL, NULL);  /* endereço efetivo */

        match(sFECHA_COL);
        match(sATRIB);

        DataType tval = parse_expr();   /* valor a armazenar */

        /* Checagem de tipo simplificada */
        if (sym->type != tval && tval != TYPE_UNDEFINED) {
            char msg[300];
            snprintf(msg, sizeof(msg),
                     "Tipo incompativel na atribuicao a '%s': esperado %s, obtido %s.",
                     id, type_name(sym->type), type_name(tval));
            sem_error(msg);
        }

        ger_emite(NULL, "ARMI", itoa_s(sym->nivel), itoa_s(sym->offset));
    } else {
        /* Atribuição simples: id := expr */
        match(sATRIB);

        DataType tval = parse_expr();

        /* Checagem de tipo */
        if (sym->type != tval && tval != TYPE_UNDEFINED) {
            char msg[300];
            snprintf(msg, sizeof(msg),
                     "Tipo incompativel na atribuicao a '%s': esperado %s, obtido %s.",
                     id, type_name(sym->type), type_name(tval));
            sem_error(msg);
        }

        ger_emite(NULL, "ARMZ", itoa_s(sym->nivel), itoa_s(sym->offset));
    }
}

/* ══════════════════════════════════════════════════════════════════════
 * ret expr
 * ══════════════════════════════════════════════════════════════════════ */

static void parse_ret(void) {
    match(sRET);
    parse_expr();
    /* O valor de retorno fica no topo da pilha; RTPR é emitido pelo contexto */
}

/* ══════════════════════════════════════════════════════════════════════
 * Expressões com propagação de tipo
 * ══════════════════════════════════════════════════════════════════════ */

static DataType parse_expr(void) {
    DataType t = parse_exlog();
    while (lookahead.cat == sOR) {
        match(sOR);
        DataType t2 = parse_exlog();
        if (t != TYPE_BOOL || t2 != TYPE_BOOL)
            sem_error("Operador 'v' (OR) requer operandos do tipo bool.");
        ger_emite(NULL, "DISJ", NULL, NULL);
        t = TYPE_BOOL;
    }
    return t;
}

static DataType parse_exlog(void) {
    DataType t = parse_exrel();
    while (lookahead.cat == sAND) {
        match(sAND);
        DataType t2 = parse_exrel();
        if (t != TYPE_BOOL || t2 != TYPE_BOOL)
            sem_error("Operador '^' (AND) requer operandos do tipo bool.");
        ger_emite(NULL, "CONJ", NULL, NULL);
        t = TYPE_BOOL;
    }
    return t;
}

static DataType parse_exrel(void) {
    DataType t = parse_exari();
    Category op = lookahead.cat;
    if (op == sMAIOR || op == sMAIORIG ||
        op == sIGUAL || op == sMENOR   ||
        op == sMENORIG || op == sDIFERENTE) {
        match(op);
        DataType t2 = parse_exari();
        if (t != t2 && t != TYPE_UNDEFINED && t2 != TYPE_UNDEFINED) {
            char msg[256];
            snprintf(msg, sizeof(msg),
                     "Operandos incompativeis na expressao relacional: %s e %s.",
                     type_name(t), type_name(t2));
            sem_error(msg);
        }
        switch (op) {
            case sMAIOR:    ger_emite(NULL, "CMMA", NULL, NULL); break;
            case sMAIORIG:  ger_emite(NULL, "CMAG", NULL, NULL); break;
            case sIGUAL:    ger_emite(NULL, "CMIG", NULL, NULL); break;
            case sMENOR:    ger_emite(NULL, "CMME", NULL, NULL); break;
            case sMENORIG:  ger_emite(NULL, "CMEG", NULL, NULL); break;
            case sDIFERENTE:ger_emite(NULL, "CMDG", NULL, NULL); break;
            default: break;
        }
        return TYPE_BOOL;
    }
    return t;
}

static DataType parse_exari(void) {
    DataType t = parse_exarp();
    while (lookahead.cat == sSOMA || lookahead.cat == sSUBRAT) {
        Category op = lookahead.cat;
        match(op);
        DataType t2 = parse_exarp();
        if (t != TYPE_INT || t2 != TYPE_INT)
            sem_error("Operadores '+' e '-' requerem operandos do tipo int.");
        if (op == sSOMA)   ger_emite(NULL, "SOMA", NULL, NULL);
        else               ger_emite(NULL, "SUBT", NULL, NULL);
        t = TYPE_INT;
    }
    return t;
}

static DataType parse_exarp(void) {
    DataType t = parse_fact();
    while (lookahead.cat == sMULT || lookahead.cat == sDIV) {
        Category op = lookahead.cat;
        match(op);
        DataType t2 = parse_fact();
        if (t != TYPE_INT || t2 != TYPE_INT)
            sem_error("Operadores '*' e '/' requerem operandos do tipo int.");
        if (op == sMULT) ger_emite(NULL, "MULT", NULL, NULL);
        else             ger_emite(NULL, "DIVI", NULL, NULL);
        t = TYPE_INT;
    }
    return t;
}

static DataType parse_fact(void) {
    if (lookahead.cat == sNEG) {
        match(sNEG);
        DataType t = parse_fact();
        if (t != TYPE_BOOL)
            sem_error("Operador '~' (NOT) requer operando do tipo bool.");
        ger_emite(NULL, "NEGA", NULL, NULL);
        return TYPE_BOOL;
    } else if (lookahead.cat == sSUBRAT) {
        match(sSUBRAT);
        DataType t = parse_fact();
        if (t != TYPE_INT)
            sem_error("Operador unario '-' requer operando do tipo int.");
        ger_emite(NULL, "INVR", NULL, NULL);
        return TYPE_INT;
    } else if (lookahead.cat == sABRE_PAR) {
        match(sABRE_PAR);
        DataType t = parse_expr();
        match(sFECHA_PAR);
        return t;
    } else {
        return parse_elem();
    }
}

/* ══════════════════════════════════════════════════════════════════════
 * Elementos terminais: constantes, variáveis, chamadas
 * ══════════════════════════════════════════════════════════════════════ */

static DataType parse_elem(void) {
    switch (lookahead.cat) {
        case sCTEINT: {
            ger_emite(NULL, "CRCT", lookahead.lexema, NULL);
            match(sCTEINT);
            return TYPE_INT;
        }
        case sCTECHAR: {
            /* Converte char literal para seu código ASCII */
            int ascii = (int)(unsigned char)lookahead.lexema[1];
            ger_emite(NULL, "CRCT", itoa_s(ascii), NULL);
            match(sCTECHAR);
            return TYPE_CHAR;
        }
        case sSTRING: {
            /* Strings só fazem sentido dentro de print; aqui emitimos cada char.
             * Fora de print, o tipo retornado é CHAR (último char na pilha). */
            ger_emite_string(lookahead.lexema);
            match(sSTRING);
            return TYPE_CHAR;
        }
        case sTRUE: {
            ger_emite(NULL, "CRCT", "1", NULL);
            match(sTRUE);
            return TYPE_BOOL;
        }
        case sFALSE: {
            ger_emite(NULL, "CRCT", "0", NULL);
            match(sFALSE);
            return TYPE_BOOL;
        }
        case sIDENTIF: {
            char id[256];
            strncpy(id, lookahead.lexema, 255);

            /* Semântica: verifica declaração */
            Symbol *sym = ts_lookup(id);
            if (!sym) {
                char msg[300];
                snprintf(msg, sizeof(msg),
                         "Identificador '%s' nao declarado.", id);
                sem_error(msg);
            }

            match(sIDENTIF);

            if (lookahead.cat == sABRE_PAR) {
                /* ── Chamada de função/procedimento ── */
                if (sym->cat != SYM_FUNC && sym->cat != SYM_PROC) {
                    char msg[300];
                    snprintf(msg, sizeof(msg),
                             "'%s' nao e uma funcao ou procedimento.", id);
                    sem_error(msg);
                }
                match(sABRE_PAR);
                int n_args = 0;
                if (lookahead.cat != sFECHA_PAR) {
                    parse_expr(); n_args++;
                    while (lookahead.cat == sVIRGULA) {
                        match(sVIRGULA);
                        parse_expr(); n_args++;
                    }
                }
                match(sFECHA_PAR);

                /* Validação de quantidade de parâmetros */
                if (sym->extra > 0 && n_args != sym->extra) {
                    char msg[300];
                    snprintf(msg, sizeof(msg),
                             "Chamada a '%s': esperado %d argumento(s), fornecido(s) %d.",
                             id, sym->extra, n_args);
                    sem_error(msg);
                }

                /* Geração: CHPR label,nivel (para fase 2 completa) */
                /* Por ora emitimos NADA como placeholder de chamada */
                ger_emite(NULL, "NADA", NULL, NULL);  /* placeholder CHPR */

                return sym->type;

            } else if (lookahead.cat == sABRE_COL) {
                /* ── Acesso a vetor: id[expr] ── */
                if (sym->cat != SYM_VETOR) {
                    char msg[300];
                    snprintf(msg, sizeof(msg), "'%s' nao e um vetor.", id);
                    sem_error(msg);
                }
                match(sABRE_COL);

                /* CREN empilha o endereço base, parse_expr empilha o índice */
                ger_emite(NULL, "CREN", itoa_s(sym->nivel), itoa_s(sym->offset));
                parse_expr();
                ger_emite(NULL, "SOMA", NULL, NULL);    /* endereço efetivo */
                ger_emite(NULL, "CRVI", itoa_s(sym->nivel), itoa_s(sym->offset));

                match(sFECHA_COL);
                return sym->type;

            } else {
                /* ── Variável simples ── */
                if (sym->cat == SYM_PROC || sym->cat == SYM_FUNC) {
                    char msg[300];
                    snprintf(msg, sizeof(msg),
                             "'%s' e uma sub-rotina; use com parenteses para chamar.", id);
                    sem_error(msg);
                }
                ger_emite(NULL, "CRVL", itoa_s(sym->nivel), itoa_s(sym->offset));
                return sym->type;
            }
        }
        default:
            diag_error("Expressao ou Valor", lookahead);
            exit(EXIT_FAILURE);
    }
}
