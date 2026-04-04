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
void parse_princ_corpo();
void parse_proc_com_nome(const char* name);
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
void parse_wh_corpo();
void parse_rpt();
void parse_rpt_corpo();
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
    
    // 1. Módulo
    match(sMODULE);
    ts_insert(lookahead.lexema, SYM_VAR, TYPE_VOID, 0); 
    match(sIDENTIF);
    match(sPONTO_VIRG);

    // 2. Globais (Opcional)
    if (lookahead.cat == sGLOBALS) {
        parse_glob();
    }

    // 3. Sub-rotinas (fn ou proc)
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

    // 4. Se o loop terminou sem encontrar o main via 'goto'
    if (lookahead.cat != sEOF) {
        parse_princ();
    }

finalizar:
    while (lookahead.cat == sEND) {
        match(sEND);
    }

    if (lookahead.cat != sEOF) {
        match(sEOF);
    }

    diag_info("Análise sintática concluída com sucesso!");
    return true;
}

void parse_glob() {
    diag_info("Analisando seção globals");
    match(sGLOBALS);
    while (lookahead.cat == sIDENTIF) {
        parse_decls();
    }
}

void parse_princ_corpo() {
    match(sABRE_PAR);
    match(sFECHA_PAR);

    // 1. Entra no escopo do main antes de processar locals ou comandos
    ts_scope_push("proc:main");

    // 2. Verificação de LOCALS
    if (lookahead.cat == sLOCALS) {
        match(sLOCALS);
        while (lookahead.cat == sIDENTIF) {
            parse_decls();
        }
    }

    // 3. Processa o bloco start...end
    parse_bco();
    
    // 4. Sai do escopo
    ts_scope_pop();
}

void parse_proc_com_nome(const char* name) {
    ts_insert(name, SYM_PROC, TYPE_VOID, 0);

    char scope_name[300];
    sprintf(scope_name, "proc:%s", name);
    ts_scope_push(scope_name);

    match(sABRE_PAR);
    if (lookahead.cat == sIDENTIF) parse_param();
    match(sFECHA_PAR);

    if (lookahead.cat == sLOCALS) {
        match(sLOCALS);
        while(lookahead.cat == sIDENTIF) parse_decls();
    }

    parse_bco();
    ts_scope_pop();
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
    while (lookahead.cat == sFN || (lookahead.cat == sPROC && strcmp(lookahead.lexema, "main") != 0)) {
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

void parse_princ() {
    // 1. Consome 'proc'
    match(sPROC);
    
    // 2. Verifica 'main' e insere na TS enquanto o escopo ainda é global
    if (strcmp(lookahead.lexema, "main") == 0) {
        ts_insert(lookahead.lexema, SYM_PROC, TYPE_VOID, 0);
        match(sIDENTIF); 
    } else {
        diag_error("Identificador 'main' esperado", lookahead);
        exit(EXIT_FAILURE);
    }

    match(sABRE_PAR);
    match(sFECHA_PAR);
    
    // 3. Entra no escopo do main ANTES de processar as variáveis locais
    ts_scope_push("proc:main");

    // 4. Tratamento de LOCALS
    if (lookahead.cat == sLOCALS) {
        match(sLOCALS);
        while (lookahead.cat == sIDENTIF) {
            parse_decls();
        }
    }

    parse_bco();
    
    // 5. Sai do escopo
    ts_scope_pop();
}

void parse_proc() {
    match(sPROC);
    
    // 1. Salva o nome do procedimento (lookahead agora é o IDENTIF)
    char name[256];
    strcpy(name, lookahead.lexema);
    
    // 2. Insere na TS no escopo atual (global) antes de entrar no escopo interno
    ts_insert(name, SYM_PROC, TYPE_VOID, 0);
    
    match(sIDENTIF);

    // 3. Cria o novo escopo para o conteúdo interno
    char scope_name[300];
    sprintf(scope_name, "proc:%s", name);
    ts_scope_push(scope_name);

    match(sABRE_PAR);
    if (lookahead.cat == sIDENTIF) parse_param();
    match(sFECHA_PAR);

    if (lookahead.cat == sLOCALS) {
        match(sLOCALS);
        while(lookahead.cat == sIDENTIF) parse_decls();
    }

    parse_bco();
    
    // 4. Sai do escopo e volta para o global
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
        case sPRINT: 
            parse_out(); 
            break;
        case sSCAN:  
            parse_inp(); 
            break;
        case sIF:    
            parse_if();  
            break;
        case sMATCH: 
            parse_mat(); 
            break;
        case sFOR:   
            parse_fr();  
            break;
        case sLOOP:  
            match(sLOOP); // Consome 'loop'
            if (lookahead.cat == sWHILE) {
                parse_wh_corpo(); 
            } else {
                parse_rpt_corpo(); 
            }
            break;
        case sRET:   
            parse_ret(); 
            break;
        case sSTART: 
            parse_bco(); 
            break;
        case sIDENTIF:
            parse_atr(); 
            break;
        default: 
            diag_error("Comando esperado (print, scan, if, loop, etc.)", lookahead);
            exit(EXIT_FAILURE);
    }
}

void parse_out() {
    match(sPRINT);
    match(sABRE_PAR);
    parse_expr();
    while (lookahead.cat == sVIRGULA) {
        match(sVIRGULA);
        parse_expr();
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
    parse_wh_corpo();
}

void parse_wh_corpo() {
    match(sWHILE);
    match(sABRE_PAR);
    parse_expr();
    match(sFECHA_PAR);
    parse_cmd();
}

void parse_rpt() {
    match(sLOOP);
    parse_rpt_corpo();
}

void parse_rpt_corpo() {
    if (lookahead.cat == sSTART) {
        parse_bco();
    } else {
        parse_cmd();
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
    parse_expr();
}

void parse_mat() {
    match(sMATCH);
    match(sABRE_PAR);
    parse_expr();
    match(sFECHA_PAR);

    while (lookahead.cat == sWHEN) {
        match(sWHEN);
        
        do {
            if (lookahead.cat == sVIRGULA) match(sVIRGULA);
            
            parse_expr(); // Lê o valor (ex: 1 ou 100)
            
            if (lookahead.cat == sPTOPTO) {
                match(sPTOPTO);
                parse_expr(); // Lê o fim do intervalo (ex: 199)
            }
        } while (lookahead.cat == sVIRGULA);

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
    if (lookahead.cat == sSTRING || lookahead.cat == sCTEINT || lookahead.cat == sCTECHAR || 
        lookahead.cat == sTRUE || lookahead.cat == sFALSE) {
        match(lookahead.cat);
    } else if (lookahead.cat == sIDENTIF) {
        match(sIDENTIF);
        if (lookahead.cat == sABRE_PAR) { // Chamada
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

        diag_error("Expressão ou Valor", lookahead);
        exit(EXIT_FAILURE); 
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
    parse_exrel()
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
