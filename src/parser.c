#include "parser.h"
#include "lex.h"
#include "symtab.h"
#include "diag.h"
#include "opt.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static Token lookahead;

// --- Protótipos de Funções Internas ---
void parse_id();
void parse_glob();
void parse_decls();
DataType parse_tpo(int *size);
void parse_subs();
void parse_func();
void parse_proc();
void parse_princ();
void parse_param();
void parse_bco();
void parse_cmd();
void parse_out();
void parse_inp();
void parse_if();
void parse_mat();
void parse_fr();
void parse_wh();
void parse_rpt();
void parse_atr();
void parse_call();
void parse_ret();
void parse_elem();
void parse_expr();
void parse_exlog();
void parse_exrel();
void parse_exari();
void parse_exarp();
void parse_fact();

// --- Funções Auxiliares ---

void match(Category expected) {
    if (lookahead.cat == expected) {
        lookahead = lex_next();
    } else {
        char msg[100];
        sprintf(msg, "Token de categoria %s", lex_cat_to_str(expected));
        diag_error(msg, lookahead);
    }
}

// Verifica se o token atual inicia um comando
bool is_cmd() {
    Category c = lookahead.cat;
    return (c == sPRINT || c == sSCAN || c == sIF || c == sMATCH || 
            c == sFOR || c == sLOOP || c == sIDENTIF || c == sRET || 
            c == sSTART);
}

// --- Implementação dos Não-Terminais ---

bool parse_program() {
    lookahead = lex_next();
    
    // ini ::= “sMODULE” id ";“ glob? subs? princ
    diag_info("Entrando em parse_program");
    match(sMODULE);
    match(sIDENTIF);
    match(sPONTO_VIRG);

    if (lookahead.cat == sGLOBALS) parse_glob();
    if (lookahead.cat == sFN || lookahead.cat == sPROC) parse_subs();
    
    parse_princ();

    if (lookahead.cat != sEOF) {
        diag_error("Fim de Arquivo", lookahead);
    }
    diag_info("Programa analisado com sucesso.");
    return true;
}

void parse_glob() {
    diag_info("Analisando seção globals");
    match(sGLOBALS);
    while (lookahead.cat == sIDENTIF) {
        parse_decls();
    }
}

void parse_decls() {
    char ids[50][256];
    int n_ids = 0;

    strcpy(ids[n_ids++], lookahead.lexema);
    match(sIDENTIF);

    while (lookahead.cat == sVIRGULA) {
        match(sVIRGULA);
        strcpy(ids[n_ids++], lookahead.lexema);
        match(sIDENTIF);
    }

    match(sDOIS_PONTOS);
    int size = 0;
    DataType t = parse_tpo(&size);
    match(sPONTO_VIRG);

    // Registra na Tabela de Símbolos
    for (int i = 0; i < n_ids; i++) {
        SymbolCat cat = (size > 0) ? SYM_VETOR : SYM_VAR;
        if (!ts_insert(ids[i], cat, t, size)) {
            char msg[300];
            sprintf(msg, "Identificador '%s' já declarado neste escopo.", ids[i]);
            diag_msg_error(msg);
            exit(EXIT_FAILURE);
        }
    }
}

DataType parse_tpo(int *size) {
    DataType t;
    if (lookahead.cat == sINT) { t = TYPE_INT; match(sINT); }
    else if (lookahead.cat == sBOOL) { t = TYPE_BOOL; match(sBOOL); }
    else if (lookahead.cat == sCHAR) { t = TYPE_CHAR; match(sCHAR); }
    else { diag_error("Tipo (int, bool ou char)", lookahead); return TYPE_UNDEFINED; }

    if (lookahead.cat == sABRE_COL) {
        match(sABRE_COL);
        *size = atoi(lookahead.lexema);
        match(sCTEINT);
        match(sFECHA_COL);
    }
    return t;
}

void parse_subs() {
    while (lookahead.cat == sFN || lookahead.cat == sPROC) {
        if (lookahead.cat == sFN) parse_func();
        else parse_proc();
    }
}

void parse_func() {
    match(sFN);
    char name[256];
    strcpy(name, lookahead.lexema);
    match(sIDENTIF);

    char scope_name[300];
    sprintf(scope_name, "fn:%s", name);
    ts_scope_push(scope_name);

    match(sABRE_PAR);
    if (lookahead.cat == sIDENTIF) parse_param();
    match(sFECHA_PAR);
    match(sDOIS_PONTOS);
    DataType t = parse_tpo(NULL);
    
    ts_insert(name, SYM_FUNC, t, 0); // Registra a função no escopo pai (global)

    if (lookahead.cat == sLOCALS) {
        match(sLOCALS);
        while(lookahead.cat == sIDENTIF) parse_decls();
    }

    parse_bco();
    ts_scope_pop();
}

void parse_proc() {
    match(sPROC);
    char name[256];
    strcpy(name, lookahead.lexema);
    match(sIDENTIF);

    char scope_name[300];
    sprintf(scope_name, "proc:%s", name);
    ts_scope_push(scope_name);

    match(sABRE_PAR);
    if (lookahead.cat == sIDENTIF) parse_param();
    match(sFECHA_PAR);

    ts_insert(name, SYM_PROC, TYPE_VOID, 0);

    if (lookahead.cat == sLOCALS) {
        match(sLOCALS);
        while(lookahead.cat == sIDENTIF) parse_decls();
    }

    parse_bco();
    ts_scope_pop();
}

void parse_princ() {
    match(sPROC);
    match(sMAIN);
    match(sABRE_PAR);
    match(sFECHA_PAR);
    
    ts_scope_push("proc:main");
    if (lookahead.cat == sLOCALS) {
        match(sLOCALS);
        while(lookahead.cat == sIDENTIF) parse_decls();
    }
    parse_bco();
    ts_scope_pop();
}

void parse_param() {
    do {
        if (lookahead.cat == sVIRGULA) match(sVIRGULA);
        char id[256];
        strcpy(id, lookahead.lexema);
        match(sIDENTIF);
        match(sDOIS_PONTOS);
        DataType t = parse_tpo(NULL);
        ts_insert(id, SYM_PARAM, t, 0);
    } while (lookahead.cat == sVIRGULA);
}

void parse_bco() {
    match(sSTART);
    while (lookahead.cat != sEND) {
        parse_cmd();
        if (lookahead.cat == sPONTO_VIRG) match(sPONTO_VIRG);
    }
    match(sEND);
}

void parse_cmd() {
    switch (lookahead.cat) {
        case sPRINT: parse_out(); break;
        case sSCAN:  parse_inp(); break;
        case sIF:    parse_if();  break;
        case sMATCH: parse_mat(); break;
        case sFOR:   parse_fr();  break;
        case sLOOP:  
            if (lex_next().cat == sWHILE) parse_wh();
            else parse_rpt(); 
            break;
        case sRET:   parse_ret(); break;
        case sSTART: parse_bco(); break;
        case sIDENTIF:
            parse_atr(); // parse_atr trata id := ou id[x] :=
            break;
        default: diag_error("Comando", lookahead);
    }
}

void parse_out() {
    match(sPRINT);
    match(sABRE_PAR);
    parse_elem();
    while (lookahead.cat == sVIRGULA) {
        match(sVIRGULA);
        parse_elem();
    }
    match(sFECHA_PAR);
}

void parse_inp() {
    match(sSCAN);
    match(sABRE_PAR);
    match(sIDENTIF);
    if (lookahead.cat == sABRE_COL) {
        match(sABRE_COL);
        parse_expr();
        match(sFECHA_COL);
    }
    match(sFECHA_PAR);
}

void parse_if() {
    match(sIF);
    match(sABRE_PAR);
    parse_expr();
    match(sFECHA_PAR);
    parse_cmd();
    if (lookahead.cat == sELSE) {
        match(sELSE);
        parse_cmd();
    }
}

void parse_fr() {
    match(sFOR);
    parse_atr();
    match(sTO);
    parse_expr();
    if (lookahead.cat == sSTEP) {
        match(sSTEP);
        parse_expr();
    }
    match(sDO);
    parse_cmd();
}

void parse_wh() {
    match(sLOOP);
    match(sWHILE);
    match(sABRE_PAR);
    parse_expr();
    match(sFECHA_PAR);
    parse_cmd();
}

void parse_rpt() {
    match(sLOOP);
    while (lookahead.cat != sUNTIL) {
        parse_cmd();
        if (lookahead.cat == sPONTO_VIRG) match(sPONTO_VIRG);
    }
    match(sUNTIL);
    match(sABRE_PAR);
    parse_expr();
    match(sFECHA_PAR);
}

void parse_atr() {
    match(sIDENTIF);
    if (lookahead.cat == sABRE_COL) {
        match(sABRE_COL);
        parse_expr();
        match(sFECHA_COL);
    }
    match(sATRIB);
    parse_expr();
}

void parse_ret() {
    match(sRET);
    parse_elem();
}

void parse_mat() {
    match(sMATCH);
    match(sABRE_PAR);
    parse_expr();
    match(sFECHA_PAR);
    while (lookahead.cat == sWHEN) {
        match(sWHEN);
        parse_expr();
        match(sIMPLIC);
        parse_cmd();
        match(sPONTO_VIRG);
    }
    if (lookahead.cat == sOTHERWISE) {
        match(sOTHERWISE);
        match(sIMPLIC);
        parse_cmd();
        match(sPONTO_VIRG);
    }
    match(sEND);
}

// --- Expressões e Precedência ---

void parse_elem() {
    if (lookahead.cat == sSTRING || lookahead.cat == sCTEINT || lookahead.cat == sCTECHAR) {
        match(lookahead.cat);
    } else if (lookahead.cat == sIDENTIF) {
        match(sIDENTIF);
        if (lookahead.cat == sABRE_PAR) { // Chamada de subrotina
            match(sABRE_PAR);
            if (lookahead.cat != sFECHA_PAR) {
                parse_expr();
                while (lookahead.cat == sVIRGULA) {
                    match(sVIRGULA);
                    parse_expr();
                }
            }
            match(sFECHA_PAR);
        } else if (lookahead.cat == sABRE_COL) { // Vetor
            match(sABRE_COL);
            parse_expr();
            match(sFECHA_COL);
        }
    } else {
        parse_expr();
    }
}

void parse_expr() {
    parse_exlog();
    while (lookahead.cat == sOR) {
        match(sOR);
        parse_exlog();
    }
}

void parse_exlog() {
    parse_exrel();
    while (lookahead.cat == sAND) {
        match(sAND);
        parse_exrel();
    }
}

void parse_exrel() {
    parse_exari();
    if (lookahead.cat == sMAIOR || lookahead.cat == sMAIORIG || 
        lookahead.cat == sIGUAL || lookahead.cat == sMENOR || 
        lookahead.cat == sMENORIG || lookahead.cat == sDIFERENTE) {
        match(lookahead.cat);
        parse_exari();
    }
}

void parse_exari() {
    parse_exarp();
    while (lookahead.cat == sSOMA || lookahead.cat == sSUBRAT) {
        match(lookahead.cat);
        parse_exarp();
    }
}

void parse_exarp() {
    parse_fact();
    while (lookahead.cat == sMULT || lookahead.cat == sDIV) {
        match(lookahead.cat);
        parse_fact();
    }
}

void parse_fact() {
    if (lookahead.cat == sNEG) {
        match(sNEG);
        parse_fact();
    } else if (lookahead.cat == sSUBRAT) {
        match(sSUBRAT);
        parse_fact();
    } else if (lookahead.cat == sABRE_PAR) {
        match(sABRE_PAR);
        parse_expr();
        match(sFECHA_PAR);
    } else {
        parse_elem();
    }
}
