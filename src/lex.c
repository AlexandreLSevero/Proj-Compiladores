#include "lex.h"
#include "log.h"
#include "opt.h"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

static FILE *source_file = NULL;
static int current_line = 1;

// Tabela de Palavras Reservadas
static struct {
    const char *name;
    Category cat;
} keywords[] = {
    {"bool", sBOOL}, {"char", sCHAR}, {"do", sDO}, {"else", sELSE},
    {"end", sEND}, {"false", sFALSE}, {"fn", sFN}, {"for", sFOR},
    {"globals", sGLOBALS}, {"if", sIF}, {"int", sINT}, {"locals", sLOCALS},
    {"loop", sLOOP}, {"match", sMATCH}, {"module", sMODULE}, 
    {"otherwise", sOTHERWISE}, {"print", sPRINT}, {"proc", sPROC},
    {"ret", sRET}, {"scan", sSCAN}, {"start", sSTART}, {"step", sSTEP},
    {"to", sTO}, {"true", sTRUE}, {"until", sUNTIL}, {"when", sWHEN},
    {"main", sMAIN}, {NULL, sERRO}
};

bool lex_init(const char *source_path) {
    source_file = fopen(source_path, "r");
    current_line = 1;
    return (source_file != NULL);
}

void lex_close() {
    if (source_file) fclose(source_file);
}

static int next_char() {
    int c = fgetc(source_file);
    if (c == '\n') current_line++;
    return c;
}

static void unread_char(int c) {
    if (c == '\n') current_line--;
    ungetc(c, source_file);
}

// Converte a categoria para String para o log
const char* lex_cat_to_str(Category cat) {
    switch(cat) {
        case sIDENTIF: return "sIDENTIF"; case sCTEINT: return "sCTEINT";
        case sMODULE: return "sMODULE";   case sSTART: return "sSTART";
        case sEND: return "sEND";         case sATRIB: return "sATRIB";
        case sIF: return "sIF";           case sPRINT: return "sPRINT";
        case sSOMA: return "sSOMA";       case sSUBRAT: return "sSUBRAT";
        case sMULT: return "sMULT";       case sDIV: return "sDIV";
        case sIGUAL: return "sIGUAL";     case sDIFERENTE: return "sDIFERENTE";
        case sPTOPTO: return "sPTOPTO";   case sIMPLIC: return "sIMPLIC";
        // Adicionar demais conforme necessário para o log...
        default: return "TOKEN_UNK";
    }
}

Token lex_next() {
    Token t;
    int c;

    while ((c = next_char()) != EOF) {
        // Pular espaços e brancos
        if (isspace(c)) continue;

        // Comentários: @ ou @{
        if (c == '@') {
            int next = next_char();
            if (next == '{') { // Bloco @{ ... }@
                while ((c = next_char()) != EOF) {
                    if (c == '}' && (next = next_char()) == '@') break;
                }
            } else { // Linha @...
                while (c != '\n' && c != EOF) c = next_char();
            }
            continue;
        }

        t.linha = current_line;

        // Identificadores e Palavras Reservadas
        if (isalpha(c) || c == '_') {
            int pos = 0;
            t.lexema[pos++] = c;
            while (isalnum(c = next_char()) || c == '_') {
                if (pos < 255) t.lexema[pos++] = c;
            }
            unread_char(c);
            t.lexema[pos] = '\0';

            // Busca na tabela de palavras reservadas
            t.cat = sIDENTIF;
            for (int i = 0; keywords[i].name != NULL; i++) {
                if (strcmp(t.lexema, keywords[i].name) == 0) {
                    t.cat = keywords[i].cat;
                    break;
                }
            }
            goto end_token;
        }

        // Números (sCTEINT)
        if (isdigit(c)) {
            int pos = 0;
            t.lexema[pos++] = c;
            while (isdigit(c = next_char())) {
                if (pos < 255) t.lexema[pos++] = c;
            }
            unread_char(c);
            t.lexema[pos] = '\0';
            t.cat = sCTEINT;
            goto end_token;
        }

        // Strings "..."
        if (c == '"') {
            int pos = 0;
            while ((c = next_char()) != '"' && c != EOF) {
                if (pos < 255) t.lexema[pos++] = c;
            }
            t.lexema[pos] = '\0';
            t.cat = sSTRING;
            goto end_token;
        }

        // Operadores Compostos e Simples
        t.lexema[0] = c;
        t.lexema[1] = '\0';
        switch (c) {
            case ':':
                if ((c = next_char()) == '=') {
                    strcpy(t.lexema, ":=");
                    t.cat = sATRIB;
                } else {
                    unread_char(c);
                    t.cat = sDOIS_PONTOS;
                }
                break;
            case '<':
                if ((c = next_char()) == '>') {
                    strcpy(t.lexema, "<>");
                    t.cat = sDIFERENTE;
                } else if (c == '=') {
                    strcpy(t.lexema, "<=");
                    t.cat = sMENORIG;
                } else {
                    unread_char(c);
                    t.cat = sMENOR;
                }
                break;
            case '>':
                if ((c = next_char()) == '=') {
                    strcpy(t.lexema, ">=");
                    t.cat = sMAIORIG;
                } else {
                    unread_char(c);
                    t.cat = sMAIOR;
                }
                break;
            case '=':
                if ((c = next_char()) == '>') {
                    strcpy(t.lexema, "=>");
                    t.cat = sIMPLIC;
                } else {
                    unread_char(c);
                    t.cat = sIGUAL;
                }
                break;
            case '.':
                if ((c = next_char()) == '.') {
                    strcpy(t.lexema, "..");
                    t.cat = sPTOPTO;
                } else {
                    unread_char(c);
                    t.cat = sERRO;
                }
                break;
            case '+': t.cat = sSOMA; break;
            case '-': t.cat = sSUBRAT; break;
            case '*': t.cat = sMULT; break;
            case '/': t.cat = sDIV; break;
            case '(': t.cat = sABRE_PAR; break;
            case ')': t.cat = sFECHA_PAR; break;
            case '[': t.cat = sABRE_COL; break;
            case ']': t.cat = sFECHA_COL; break;
            case ';': t.cat = sPONTO_VIRG; break;
            case ',': t.cat = sVIRGULA; break;
            case '^': t.cat = sAND; break;
            case 'v': t.cat = sOR; break;
            case '~': t.cat = sNEG; break;
            default:  t.cat = sERRO; break;
        }
        goto end_token;
    }

    t.cat = sEOF;
    strcpy(t.lexema, "EOF");
    t.linha = current_line;

end_token:
    if (opts_get()->dump_tokens) log_token(t);
    return t;
}
