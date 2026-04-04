#ifndef COMMON_H
#define COMMON_H

#include <stdbool.h>

// Categorias de Tokens baseadas na EBNF da SAL
typedef enum {
    // Palavras-reservadas
    sMODULE, sGLOBALS, sLOCALS, sFN, sPROC, sMAIN, 
    sSTART, sEND, sRET, sIF, sELSE, sMATCH, sWHEN, 
    sOTHERWISE, sFOR, sTO, sSTEP, sDO, sLOOP, sWHILE, 
    sUNTIL, sINT, sBOOL, sCHAR, sPRINT, sSCAN, 
    sTRUE, sFALSE,

    // Operadores e Delimitadores
    sATRIB,      // :=
    sSOMA,       // +
    sSUBRAT,     // -
    sMULT,       // *
    sDIV,        // /
    sMAIOR,      // >
    sMENOR,      // <
    sMAIORIG,    // >=
    sMENORIG,    // <=
    sIGUAL,      // =
    sDIFERENTE,  // <>
    sAND,        // ^
    sOR,         // v
    sNEG,        // ~
    sIMPLIC,     // =>
    sPTOPTO,     // ..
    sABRE_PAR,   // (
    sFECHA_PAR,  // )
    sABRE_COL,   // [
    sFECHA_COL,  // ]
    sVIRGULA,    // ,
    sPONTO_VIRG, // ;
    sDOIS_PONTOS,// :

    // Terminais Dinâmicos
    sIDENTIF,    // nomes de variáveis/funções
    sCTEINT,     // constantes inteiras
    sCTECHAR,    // 'a'
    sSTRING,     // "texto"
    
    sEOF,        // Fim de arquivo
    sERRO        // Categoria de erro léxico
} Category;

// Estrutura que representa um Token completo
typedef struct {
    Category cat;
    char lexema[256];
    int linha;
} Token;

// Tipos de dados da linguagem SAL
typedef enum {
    TYPE_INT,
    TYPE_BOOL,
    TYPE_CHAR,
    TYPE_VOID,
    TYPE_UNDEFINED
} DataType;

#endif
