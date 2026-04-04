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
    {"to", sTO}, {"true", sTRUE}, {"until", sUNTIL}, {"when", sWHEN}, {"while", sWHILE},
    {"v", sOR}, {"^", sAND}, {NULL, sERRO}
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
    switch (cat) {
        // Palavras-reservadas
        case sMODULE:    return "sMODULE";
        case sGLOBALS:   return "sGLOBALS";
        case sLOCALS:    return "sLOCALS";
        case sFN:        return "sFN";
        case sPROC:      return "sPROC";
        case sMAIN:      return "sMAIN";
        case sSTART:     return "sSTART";
        case sEND:       return "sEND";
        case sRET:       return "sRET";
        case sIF:         return "sIF";
        case sELSE:       return "sELSE";
        case sMATCH:      return "sMATCH";
        case sWHEN:       return "sWHEN";
        case sOTHERWISE:  return "sOTHERWISE";
        case sFOR:        return "sFOR";
        case sTO:         return "sTO";
        case sSTEP:       return "sSTEP";
        case sDO:         return "sDO";
        case sLOOP:       return "sLOOP";
        case sWHILE:      return "sWHILE";
        case sUNTIL:      return "sUNTIL";
        case sINT:        return "sINT";
        case sBOOL:       return "sBOOL";
        case sCHAR:       return "sCHAR";
        case sPRINT:      return "sPRINT";
        case sSCAN:       return "sSCAN";
        case sTRUE:       return "sTRUE";
        case sFALSE:      return "sFALSE";

        // Operadores e Delimitadores
        case sATRIB:      return "sATRIB";
        case sSOMA:       return "sSOMA";
        case sSUBRAT:     return "sSUBRAT";
        case sMULT:       return "sMULT";
        case sDIV:        return "sDIV";
        case sMAIOR:      return "sMAIOR";
        case sMENOR:      return "sMENOR";
        case sMAIORIG:    return "sMAIORIG";
        case sMENORIG:    return "sMENORIG";
        case sIGUAL:      return "sIGUAL";
        case sDIFERENTE:  return "sDIFERENTE";
        case sAND:        return "sAND";
        case sOR:         return "sOR";
        case sNEG:        return "sNEG";
        case sIMPLIC:     return "sIMPLIC";
        case sPTOPTO:     return "sPTOPTO";
        case sABRE_PAR:   return "sABRE_PAR";
        case sFECHA_PAR:  return "sFECHA_PAR";
        case sABRE_COL:   return "sABRE_COL";
        case sFECHA_COL:  return "sFECHA_COL";
        case sVIRGULA:    return "sVIRGULA";
        case sPONTO_VIRG: return "sPONTO_VIRG";
        case sDOIS_PONTOS: return "sDOIS_PONTOS";

        // Terminais Dinâmicos
        case sIDENTIF:    return "sIDENTIF";
        case sCTEINT:     return "sCTEINT";
        case sCTECHAR:    return "sCTECHAR";
        case sSTRING:     return "sSTRING";
        case sEOF:        return "sEOF";
        case sERRO:       return "sERRO";

        default:          return "TOKEN_UNK";
    }
}

Token lex_next() {
    Token t;
    int c;

    while ((c = next_char()) != EOF) {
        // 1. Pular espaços e brancos
        if (isspace(c)) continue;

        // 2. Comentários: @ ou @{
        if (c == '@') {
            int next = next_char();
            if (next == '{') { // Bloco @{ ... }@
                while ((c = next_char()) != EOF) {
                    if (c == '}') {
                        if ((next = next_char()) == '@') break;
                        else unread_char(next);
                    }
                }
            } else { // Linha @...
                while (c != '\n' && c != EOF) c = next_char();
            }
            continue;
        }

        t.linha = current_line;

        // 3. Identificadores e Palavras Reservadas
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
            if (opts_get()->dump_tokens) log_token(t);
            return t;
        }

        // 4. Números (sCTEINT)
        if (isdigit(c)) {
            int pos = 0;
            t.lexema[pos++] = c;
            while (isdigit(c = next_char())) {
                if (pos < 255) t.lexema[pos++] = c;
            }
            unread_char(c);
            t.lexema[pos] = '\0';
            t.cat = sCTEINT;
            if (opts_get()->dump_tokens) log_token(t);
            return t;
        }

        // 5. Strings "..."
        if (c == '"') {
            int pos = 0;
            while ((c = next_char()) != '"' && c != EOF) {
                if (pos < 255) t.lexema[pos++] = c;
            }
            t.lexema[pos] = '\0';
            t.cat = sSTRING;
            if (opts_get()->dump_tokens) log_token(t);
            return t;
        }

        // 6. Operadores e Pontuação
        t.lexema[0] = (char)c;
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
                c = next_char();
                if (c == '>') { strcpy(t.lexema, "<>"); t.cat = sDIFERENTE; }
                else if (c == '=') { strcpy(t.lexema, "<="); t.cat = sMENORIG; }
                else { unread_char(c); t.cat = sMENOR; }
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
                    t.cat = sERRO; // SAL não tem ponto sozinho, então é considera ERRO
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
        
        if (opts_get()->dump_tokens) log_token(t);
        return t;
    }

    // 7. Fim de Arquivo
    t.cat = sEOF;
    strcpy(t.lexema, "EOF");
    t.linha = current_line;
    if (opts_get()->dump_tokens) log_token(t);
    return t;
}
